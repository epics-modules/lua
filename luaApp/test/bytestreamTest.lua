-- bytestreamTest.lua
--
-- Assertion fixture for the bytestream library, driven by bytestreamTest.cpp.
-- Builds a global `results` list of { ok = bool, msg = string } records.

local bs = require("bytestream")

results = {}

-- Evaluate producer thunk under pcall so one failure does not abort the
-- whole fixture, then compare its result to `want`.
local function eq(fn, want, msg)
	local ok, got = pcall(fn)
	if not ok then
		results[#results + 1] = { ok = false,
			msg = string.format("%s (error: %s)", msg, tostring(got)) }
	else
		results[#results + 1] = { ok = (got == want),
			msg = string.format("%s (got %s, want %s)",
				msg, tostring(got), tostring(want)) }
	end
end

---------------------------------------------------------------------------
-- Baseline: match / format still work
---------------------------------------------------------------------------

eq(function() return bs.match("%d", "42") end, 42, "match %d")
eq(function() return bs.match("%f", "3.14") end, 3.14, "match %f")
eq(function() return bs.match("%s", "hello") end, "hello", "match %s")
eq(function() return bs.format("%d", 42) end, "42", "format %d")
eq(function() return bs.format("%05d", 42) end, "00042", "format %05d")
eq(function() return bs.format("%.2f", 3.14159) end, "3.14", "format %.2f")
eq(function() return bs.format("%08b", 42) end, "00101010", "format %08b")

eq(function() return (bs.match("VOLTS %f %s", "VOLTS 3.14 V")) end, 3.14,
	"match multi: volts value")
eq(function() local _, u = bs.match("VOLTS %f %s", "VOLTS 3.14 V"); return u end, "V",
	"match multi: unit string")

---------------------------------------------------------------------------
-- * (ignore) flag parity on READ: consumed but not captured
---------------------------------------------------------------------------

eq(function() return bs.match("%*d %d", "10 20") end, 20, "read %*d skips first int")
eq(function() return bs.match("%*f,%f", "3.14,1.27") end, 1.27, "read %*f skips first float")
eq(function() return bs.match("%*s %d", "skip 42") end, 42, "read %*s skips string")
eq(function() return bs.match("VOLTS %*f,AMPS %f", "VOLTS 3.14,AMPS 1.27") end, 1.27,
	"read %*f in mixed literal context")
eq(function() return bs.match("STATUS:%*{off|on|standby} TEMP:%f", "STATUS:on TEMP:25.3") end, 25.3,
	"read %*{enum} skips enum")
eq(function() return bs.match("%*b %d", "1010 7") end, 7, "read %*b skips binary")
eq(function() return bs.match("%*1c%d", "X42") end, 42, "read %*1c skips one char (explicit width)")

-- ignore must not shift capture positions of following converters
eq(function() local a = bs.match("%*d %d %d", "1 2 3"); return a end, 2,
	"read %*d: first captured value after skip")
eq(function() local _, b = bs.match("%*d %d %d", "1 2 3"); return b end, 3,
	"read %*d: second captured value after skip")

---------------------------------------------------------------------------
-- * (ignore) flag parity on WRITE.
--
-- Shipped semantics (preserved by the centralization refactor): a *
-- converter emits nothing AND consumes no argument. Arguments are
-- positional, so the following converter still reads the first arg.
---------------------------------------------------------------------------

eq(function() return bs.format("%*d%d", 10, 20) end, "10",
	"write %*d emits nothing and consumes no arg")
eq(function() return bs.format("A%*dB%d", 1, 2) end, "AB1",
	"write %*d emits nothing between literals, next arg is first")
eq(function() return bs.format("x%*dy", 1) end, "xy",
	"write %*d produces only the literal text")

---------------------------------------------------------------------------
-- Custom format via helpers: bytestream.reader / bytestream.writer
---------------------------------------------------------------------------

-- %B : boolean rendered as the strings "true"/"false"
bs.add_format {
	identifier = "B",
	read = bs.reader {
		pattern_fn = function(e)
			return e.C(e.P("true") + e.P("false"))
		end,
		conversion = function(x) return x == "true" end,
	},
	write = function(flags)
		return function(value)
			return value and "true" or "false"
		end
	end,
}

eq(function() return bs.match("%B", "true") end, true, "custom %B read true")
eq(function() return bs.match("%B", "false") end, false, "custom %B read false")
eq(function() return bs.format("%B", true) end, "true", "custom %B write true")
eq(function() return bs.format("%B", false) end, "false", "custom %B write false")

-- The * flag works for the custom format for FREE (library owns it).
-- Read: "true" is consumed but not captured, %d reads 9.
-- Write: %*B emits nothing and consumes no arg, %d reads the first arg.
eq(function() return bs.match("%*B %d", "true 9") end, 9, "custom %*B read skips (library-owned *)")
eq(function() return bs.format("%*B%d", 9) end, "9", "custom %*B write skips (library-owned *)")

-- %H : hex integer read via a numeric reader helper, written via writer("X")
bs.add_format {
	identifier = "H",
	read = bs.reader {
		pattern_fn = function(e)
			return e.C((e.digit + e.R("af") + e.R("AF"))^1)
		end,
		conversion = function(x) return tonumber(x, 16) end,
	},
	write = bs.writer("X"),
}

eq(function() return bs.match("%H", "ff") end, 255, "custom %H read via reader helper")
eq(function() return bs.format("%H", 255) end, "FF", "custom %H write via writer helper")
eq(function() return bs.format("%04H", 255) end, "00FF", "custom %H honors width/zero-pad via writer")

---------------------------------------------------------------------------
-- Custom format from scratch (no helpers): * still handled by the library
---------------------------------------------------------------------------

-- %Y : reversed string.
bs.add_format {
	identifier = "Y",
	read = function(flags)
		local lpeg = require("lpeg")
		return lpeg.C((lpeg.P(1) - lpeg.P(" "))^1)
		     / function(s) return s:reverse() end
	end,
	write = function(flags)
		return function(value)
			return tostring(value):reverse()
		end
	end,
}

eq(function() return bs.match("%Y", "abc") end, "cba", "scratch %Y read reverses")
eq(function() return bs.format("%Y", "abc") end, "cba", "scratch %Y write reverses")
eq(function() return bs.match("%*Y %d", "abc 4") end, 4, "scratch %*Y read skips (library-owned *)")
eq(function() return bs.format("%*Y%d", 4) end, "4", "scratch %*Y write skips (library-owned *)")

return results
