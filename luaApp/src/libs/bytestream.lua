--
-- bytestream: byte stream formatting and parsing library for EPICS IOCs
--
-- Provides scanf-style parsing (bytestream.match) and printf-style formatting
-- (bytestream.format) using LPeg, plus a client wrapper around asyn.client
-- for structured device I/O.
--
-- Format specifiers follow StreamDevice conventions:
--   %[flags][width][.precision]<specifier>
--
-- Flags: * (ignore/skip), # (alternate), - (left-align), 0 (zero-pad),
--        ? (lenient), ! (exact width)
--
-- Specifiers: s c d u o x f e g E G b r %{enum0|enum1|...}
--

local lpeg, asyn

local function get_lpeg()
	if not lpeg then lpeg = require("lpeg") end
	return lpeg
end

local function get_asyn()
	if not asyn then asyn = require("asyn") end
	return asyn
end


---------------------------------------------------------------------------
-- Module table
---------------------------------------------------------------------------

local bytestream = {}
bytestream.inputs  = {}
bytestream.outputs = {}


---------------------------------------------------------------------------
-- Helpers (all local)
---------------------------------------------------------------------------

local function strip(s)
	return (s:gsub("%s", ""))
end

local function hextonumber(sign, val)
	if not val then return 0 end
	if strip(sign) == "-" then return -1 * tonumber(val, 16)
	else                       return tonumber(val, 16)
	end
end

-- Convert an integer to a binary string (e.g. 42 -> "101010")
local function int_to_bin(n)
	if n == 0 then return "0" end

	local bits = {}
	local neg = (n < 0)
	if neg then n = -n end

	while n > 0 do
		bits[#bits + 1] = (n % 2 == 1) and "1" or "0"
		n = math.floor(n / 2)
	end

	local s = string.reverse(table.concat(bits))
	if neg then s = "-" .. s end
	return s
end


---------------------------------------------------------------------------
-- Pattern environment builder (used by read-side)
---------------------------------------------------------------------------

local function generate_patterns(flags)
	local P = get_lpeg()

	local e = {}
	lpeg.locale(e)

	e.P = lpeg.P
	e.R = lpeg.R
	e.S = lpeg.S
	e.C = lpeg.C

	e.opt     = function(pat) return pat^-1 end
	e.if_hash = function(pat) return flags.hash     and pat or lpeg.P(true) end
	e.if_neg  = function(pat) return flags.left_pad and pat or lpeg.P(true) end

	e.sign           = e.S("+-")
	e.optsign        = e.opt(e.sign) * e.if_hash(e.space^0)
	e.uint           = e.digit^1
	e.decimal        = e.uint * e.opt(lpeg.P(".") * e.uint)
	e.signed_int     = e.optsign * e.uint
	e.signed_decimal = e.optsign * e.decimal
	e.floating_point = e.signed_decimal * e.opt(e.S("eE") * e.signed_int)

	e.max_width = function(pattern, max_width, exact_width)
		if not max_width then return pattern end

		local min_width = exact_width and max_width or 1

		return lpeg.Cmt(lpeg.P(true),
			function(s, i)
				local last_offset = nil

				for index = min_width, max_width do
					local extract, offset = lpeg.match(
						lpeg.C(lpeg.P(index)) * lpeg.Cp(), s, i)

					if not offset then return last_offset or false end

					if lpeg.match(pattern + lpeg.P(-1), extract) then
						last_offset = offset
					end
				end

				return last_offset
			end)
	end

	return e
end


---------------------------------------------------------------------------
-- Read-side: basic_reader factory
---------------------------------------------------------------------------

local function basic_reader(datatable)
	-- Compute default value once
	local defaultvalue = datatable.defaultvalue
	if defaultvalue == nil then
		defaultvalue = datatable.pattern_fn and nil  -- deferred
	end

	return function(flags)
		local e = generate_patterns(flags)
		e.flags = flags

		-- Call the pattern function with the environment
		local output = datatable.pattern_fn(e)

		output = lpeg.Ct(e.max_width(output, flags.width, flags.exact_width))
		output = output / table.unpack / datatable.conversion

		-- Compute default lazily if needed
		if defaultvalue == nil and datatable.conversion then
			defaultvalue = datatable.conversion("") or datatable.conversion("0")
		end

		if not flags.strict then
			local dv = defaultvalue
			output = output / function(x) return x or dv end
		end

		-- * flag: match but discard (no capture contribution)
		if flags.ignore then
			-- Rebuild without captures: just match the underlying text pattern.
			-- Re-invoke pattern_fn but wrap result in a non-capturing match.
			local raw_pat = datatable.pattern_fn(e)

			-- Strip captures by matching via Cmt: consume the matched text,
			-- return only the new position (no extra values = no captures).
			local width_pat = e.max_width(raw_pat, flags.width, flags.exact_width)
			output = lpeg.Cmt(width_pat, function(s, pos)
				return pos
			end)
		end

		return output
	end
end


---------------------------------------------------------------------------
-- Read-side: compile a single format specifier into an LPeg pattern
---------------------------------------------------------------------------

local flag_chars = "*#+0-?=!"

local function parse_fields(data)
	local fl = data.flags or ""
	local output = {}

	output.width       = data.width
	output.precision   = data.precision
	output.strict      = not fl:find("?", 1, true)
	output.ignore      = fl:find("*", 1, true) ~= nil
	output.exact_width = fl:find("!", 1, true) ~= nil
	output.left_pad    = fl:find("-", 1, true) ~= nil
	output.pad_zeroes  = fl:find("0", 1, true) ~= nil
	output.compare     = fl:find("=", 1, true) ~= nil
	output.hash        = fl:find("#", 1, true) ~= nil

	return output
end

local function compile_input(format_specifier, format_function)
	local P = get_lpeg()

	local search = lpeg.P("%")
	search = search * lpeg.Cg(lpeg.S(flag_chars)^(-#flag_chars), "flags")
	search = search * lpeg.Cg(lpeg.locale().digit^0 / tonumber, "width")
	search = search * lpeg.Cg((lpeg.P(".") * lpeg.C(lpeg.locale().digit^1))^-1 / tonumber, "precision")
	search = search * lpeg.P(format_specifier)

	return lpeg.Ct(search) / parse_fields / format_function
end


---------------------------------------------------------------------------
-- Read-side: compile an enum specifier  %{val0|val1|val2}
---------------------------------------------------------------------------

local function compile_enum_input()
	local P = get_lpeg()

	-- Match %[flags]{enum0|enum1|...}
	local search = lpeg.P("%")
	search = search * lpeg.Cg(lpeg.S(flag_chars)^(-#flag_chars), "flags")
	search = search * lpeg.Cg(lpeg.locale().digit^0 / tonumber, "width")
	search = search * lpeg.Cg(lpeg.Cc(nil), "precision")
	search = search * lpeg.P("{") * lpeg.Cg(lpeg.C((lpeg.P(1) - lpeg.P("}"))^1), "body") * lpeg.P("}")

	local function build_enum_reader(data)
		local flags = parse_fields(data)
		local body = data.body or ""

		-- Map from value string to original 0-based index,
		-- with array part for ordering by length (longest first)
		local enum_map = {}
		for val in body:gmatch("[^|]+") do
			enum_map[val] = #enum_map  -- 0-based index
			enum_map[#enum_map + 1] = val  -- array part
		end

		-- Sort array part by length descending (longest match first)
		table.sort(enum_map, function(a, b) return #a > #b end)

		-- Build alternation: try longest strings first
		local pattern = lpeg.P(false)
		for _, val in ipairs(enum_map) do
			pattern = pattern + lpeg.P(val) * lpeg.Cc(enum_map[val])
		end

		if flags.ignore then
			-- Match without capturing: consume the text, return only position
			return lpeg.Cmt(pattern, function(s, pos) return pos end)
		end

		return pattern
	end

	return lpeg.Ct(search) / build_enum_reader
end


---------------------------------------------------------------------------
-- Read-side: match function
---------------------------------------------------------------------------

-- Cache of compiled input grammars: format_string -> lpeg_pattern
local input_cache = setmetatable({}, { __mode = "v" })

function bytestream.match(specifier, input)
	local P = get_lpeg()

	local compiled = input_cache[specifier]

	if not compiled then
		-- Build converter: alternation of all registered single-char specifiers
		local converter = lpeg.P(false)
		for key, value in pairs(bytestream.inputs) do
			converter = converter + compile_input(key, value)
		end

		-- Add enum specifier
		converter = converter + compile_enum_input()

		local rawtext = (lpeg.P(1) - lpeg.P("%"))^1 / lpeg.P
		local ctrl    = lpeg.P("%%") / function() return lpeg.P("%") end
		local item    = rawtext + converter + ctrl
		local line    = lpeg.Cf(item^1, function(x, y) return x * y end)

		compiled = line:match(specifier)
		if not compiled then
			error("bytestream: cannot parse format specifier: " .. specifier)
		end

		input_cache[specifier] = compiled
	end

	return lpeg.match(compiled, input)
end


---------------------------------------------------------------------------
-- Write-side: basic_writer factory
---------------------------------------------------------------------------

-- Sentinel for ignored write conversions (no arg consumed, no output)
local IGNORE_WRITE = { ignore = true }

local function basic_writer(specifier_char)
	return function(flags)
		if flags.ignore then return IGNORE_WRITE end

		return function(value)
			local fmt = "%"
			if flags.left_pad   then fmt = fmt .. "-" end
			if flags.pad_zeroes then fmt = fmt .. "0" end
			if flags.width      then fmt = fmt .. tostring(flags.width) end
			if flags.precision  then fmt = fmt .. "." .. tostring(flags.precision) end
			fmt = fmt .. specifier_char
			return string.format(fmt, value)
		end
	end
end


---------------------------------------------------------------------------
-- Write-side: compile a single output format specifier
---------------------------------------------------------------------------

local function compile_output(format_specifier, format_function)
	local P = get_lpeg()

	local search = lpeg.P("%")
	search = search * lpeg.Cg(lpeg.S(flag_chars)^(-#flag_chars), "flags")
	search = search * lpeg.Cg(lpeg.locale().digit^0 / tonumber, "width")
	search = search * lpeg.Cg((lpeg.P(".") * lpeg.C(lpeg.locale().digit^1))^-1 / tonumber, "precision")
	search = search * lpeg.P(format_specifier)

	return lpeg.Ct(search) / parse_fields / format_function
end

local function compile_enum_output()
	local P = get_lpeg()

	local search = lpeg.P("%")
	search = search * lpeg.Cg(lpeg.S(flag_chars)^(-#flag_chars), "flags")
	search = search * lpeg.Cg(lpeg.locale().digit^0 / tonumber, "width")
	search = search * lpeg.Cg(lpeg.Cc(nil), "precision")
	search = search * lpeg.P("{") * lpeg.Cg(lpeg.C((lpeg.P(1) - lpeg.P("}"))^1), "body") * lpeg.P("}")

	local function build_enum_writer(data)
		local flags = parse_fields(data)
		local body = data.body or ""

		-- Split on |
		local enums = {}
		for val in body:gmatch("[^|]+") do
			enums[#enums + 1] = val
		end

		if flags.ignore then return IGNORE_WRITE end

		return function(value)
			local idx = math.floor(tonumber(value) or 0)
			local s = enums[idx + 1]  -- 0-based to 1-based
			if not s then
				error("bytestream: enum index " .. tostring(idx) .. " out of range (0.." .. tostring(#enums - 1) .. ")")
			end
			return s
		end
	end

	return lpeg.Ct(search) / build_enum_writer
end


---------------------------------------------------------------------------
-- Write-side: format function
---------------------------------------------------------------------------

-- Cache of compiled output grammars: format_string -> list of segments
local output_cache = setmetatable({}, { __mode = "v" })

function bytestream.format(specifier, ...)
	local P = get_lpeg()
	local args = table.pack(...)

	local segments = output_cache[specifier]

	if not segments then
		-- Build output converter: alternation of all registered single-char specifiers
		local converter = lpeg.P(false)
		for key, value in pairs(bytestream.outputs) do
			converter = converter + compile_output(key, value)
		end

		-- Add enum output
		converter = converter + compile_enum_output()

		-- Capture converter results as {type="fmt", func=fn}
		local fmt_item = converter / function(fn) return { type = "fmt", func = fn } end

		-- Raw text: anything not %
		local rawtext = lpeg.C((lpeg.P(1) - lpeg.P("%"))^1) / function(s) return { type = "lit", text = s } end

		-- %% -> literal %
		local ctrl = lpeg.P("%%") / function() return { type = "lit", text = "%" } end

		local item = rawtext + fmt_item + ctrl
		local line = lpeg.Ct(item^1)

		segments = line:match(specifier)
		if not segments then
			error("bytestream: cannot parse format specifier: " .. specifier)
		end

		output_cache[specifier] = segments
	end

	-- Walk the segments, consuming args for format items
	local parts = {}
	local arg_idx = 1

	for _, seg in ipairs(segments) do
		if seg.type == "lit" then
			parts[#parts + 1] = seg.text
		elseif seg.type == "fmt" then
			local fn = seg.func
			if type(fn) == "table" and fn.ignore then
				-- * flag: skip, consume no arg, emit nothing
			elseif type(fn) == "function" then
				if arg_idx > args.n then
					error("bytestream: not enough arguments for format specifier")
				end
				parts[#parts + 1] = fn(args[arg_idx])
				arg_idx = arg_idx + 1
			end
		end
	end

	return table.concat(parts)
end


---------------------------------------------------------------------------
-- Format registration
---------------------------------------------------------------------------

function bytestream.add_format(cvt)
	if cvt.read  then bytestream.inputs[cvt.identifier]  = cvt.read  end
	if cvt.write then bytestream.outputs[cvt.identifier] = cvt.write end
end


---------------------------------------------------------------------------
-- Built-in format specifiers
---------------------------------------------------------------------------

-- String formats

bytestream.add_format {
	identifier = "s",
	read = basic_reader {
		pattern_fn = function(e) return e.C((lpeg.P(1) - e.space)^1) end,
		conversion = tostring
	},
	write = basic_writer("s")
}

bytestream.add_format {
	identifier = "c",
	read = basic_reader {
		pattern_fn = function(e) return e.C(lpeg.P(1)^1) end,
		conversion = tostring
	},
	write = function(flags)
		if flags.ignore then return IGNORE_WRITE end
		return function(value)
			local s = tostring(value)
			if flags.width then
				return s:sub(1, flags.width)
			else
				return s:sub(1, 1)
			end
		end
	end
}

-- Integer formats

bytestream.add_format {
	identifier = "d",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.signed_int) end,
		conversion = tonumber
	},
	write = basic_writer("d")
}

bytestream.add_format {
	identifier = "u",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.uint) end,
		conversion = tonumber
	},
	write = basic_writer("u")
}

bytestream.add_format {
	identifier = "o",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.if_neg(e.optsign) * lpeg.R("07")^1) end,
		conversion = function(x) return tonumber(strip(x), 8) end
	},
	write = basic_writer("o")
}

bytestream.add_format {
	identifier = "x",
	read = basic_reader {
		pattern_fn = function(e)
			return e.C(e.if_neg(e.optsign))
			     * e.opt(lpeg.P("0") * lpeg.S("xX"))
			     * e.C((e.digit + lpeg.R("af") + lpeg.R("AF"))^1)
		end,
		conversion = hextonumber
	},
	write = basic_writer("x")
}

-- Float formats

bytestream.add_format {
	identifier = "f",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.floating_point) end,
		conversion = function(x) return tonumber(strip(x)) end
	},
	write = basic_writer("f")
}

bytestream.add_format {
	identifier = "e",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.floating_point) end,
		conversion = function(x) return tonumber(strip(x)) end
	},
	write = basic_writer("e")
}

bytestream.add_format {
	identifier = "g",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.floating_point) end,
		conversion = function(x) return tonumber(strip(x)) end
	},
	write = basic_writer("g")
}

bytestream.add_format {
	identifier = "E",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.floating_point) end,
		conversion = function(x) return tonumber(strip(x)) end
	},
	write = basic_writer("E")
}

bytestream.add_format {
	identifier = "G",
	read = basic_reader {
		pattern_fn = function(e) return e.C(e.floating_point) end,
		conversion = function(x) return tonumber(strip(x)) end
	},
	write = basic_writer("G")
}

-- Binary format (%b)

bytestream.add_format {
	identifier = "b",
	read = basic_reader {
		pattern_fn = function(e)
			local bindigit = lpeg.S("01")
			return e.C(e.opt(e.sign) * bindigit^1)
		end,
		conversion = function(x)
			local s = strip(x)
			local neg = false
			if s:sub(1,1) == "-" then neg = true; s = s:sub(2) end
			if s:sub(1,1) == "+" then s = s:sub(2) end
			local n = tonumber(s, 2)
			if n and neg then n = -n end
			return n
		end
	},
	write = function(flags)
		if flags.ignore then return IGNORE_WRITE end
		return function(value)
			local s = int_to_bin(math.floor(tonumber(value) or 0))

			-- Apply width: pad or truncate
			if flags.width then
				if #s < flags.width then
					local pad = flags.pad_zeroes and "0" or " "
					if flags.left_pad then
						s = s .. string.rep(pad, flags.width - #s)
					else
						s = string.rep(pad, flags.width - #s) .. s
					end
				end
			end

			return s
		end
	end
}

-- Raw bytes format (%r)

bytestream.add_format {
	identifier = "r",
	read = basic_reader {
		pattern_fn = function(e)
			-- Width determines byte count; default 1
			local count = (e.flags and e.flags.width) or 1
			return e.C(lpeg.P(count))
		end,
		conversion = tostring
	},
	write = function(flags)
		if flags.ignore then return IGNORE_WRITE end
		return function(value)
			return tostring(value)
		end
	end
}


---------------------------------------------------------------------------
-- Client: wraps asyn.client for structured I/O
---------------------------------------------------------------------------

local client_methods = {}
local client_mt = {}

function client_mt.__index(self, key)
	if client_methods[key] then return client_methods[key] end
	-- Delegate property reads to underlying asyn client
	return self._asyn[key]
end

function client_mt.__newindex(self, key, val)
	if key == "_asyn" or key == "_port" then
		rawset(self, key, val)
	else
		-- Delegate property writes to underlying asyn client
		self._asyn[key] = val
	end
end

function client_mt.__tostring(self)
	return "bytestream.client(" .. tostring(self._asyn.portName) .. ")"
end

function client_mt.__gc(self)
	-- asyn client handles its own cleanup via __gc
end

function client_methods:write(fmt, ...)
	local data
	if select("#", ...) > 0 then
		data = bytestream.format(fmt, ...)
	else
		data = fmt
	end

	self._asyn:write(data)
	return self  -- enable chaining: dev:write(...):read(...)
end

function client_methods:read(fmt)
	local raw = self._asyn:read()

	if raw == nil then
		error("bytestream: no data read from port " .. tostring(self._asyn.portName))
	end

	if fmt then
		return bytestream.match(fmt, raw)
	else
		return raw
	end
end

function client_methods:flush()
	self._asyn:flush()
	return self
end

function client_methods:trace(...)
	self._asyn:trace(...)
	return self
end

function client_methods:traceio(...)
	self._asyn:traceio(...)
	return self
end

function bytestream.client(portName, addr)
	local c = get_asyn().client(portName, addr or 0)
	return setmetatable({ _asyn = c }, client_mt)
end


---------------------------------------------------------------------------
-- Documentation
---------------------------------------------------------------------------

bytestream._doc = {
	["bytestream"]  = "Byte stream formatting and parsing library for device I/O",
	[".match"]      = "match(fmt, input) - parse input string, return extracted values",
	[".format"]     = "format(fmt, ...) - format values into an output string",
	[".client"]     = "client(port [,addr]) - create a bytestream client wrapping asyn",
	[".add_format"] = "add_format(cvt) - register a custom format specifier",
	[".inputs"]     = "Table of registered read-side format functions",
	[".outputs"]    = "Table of registered write-side format functions",
}


return bytestream
