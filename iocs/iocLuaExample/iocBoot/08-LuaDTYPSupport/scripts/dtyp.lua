-- dtyp.lua
--
-- Callback functions for DTYP "lua" device support examples.
--
-- Every callback receives a PV object as its first argument.
-- Use dot notation (record.VAL, record.EGU, etc.) to read or
-- write record fields. Additional parameters from the INP/OUT
-- field follow the PV object.
--
-- Input callbacks return a value to set the record.
-- Output callbacks read the value being written and return nil.


-- === Input Callbacks ===

-- Increment the record's value by a fixed step each time it processes.
-- Demonstrates: reading record fields, passing parameters, returning a number.
function next_int(record, amount)
	local curr_val = record.VAL

	print("  next_int: " .. curr_val .. " + " .. amount .. " = " .. (curr_val + amount))

	return curr_val + amount
end

-- Toggle the record between True and False on each process.
-- Demonstrates: returning a string that maps to ONAM/ZNAM.
function next_bool(record)
	if record.VAL == 0 then
		return "True"
	else
		return "False"
	end
end

-- Return the current Unix epoch time as an integer.
-- Demonstrates: longin expects an integer return value.
function epoch_seconds(record)
	return math.tointeger(os.time())
end

-- Return the current date and time as a formatted string.
-- Demonstrates: stringin expects a string return value.
function timestamp(record)
	return os.date("%Y-%m-%d %H:%M:%S")
end


-- === Output Callbacks ===

-- Log a numeric value being written to an ao record.
-- Demonstrates: reading the written value from record.VAL,
-- returning nil to indicate success.
function on_write(record)
	print("  on_write: received " .. record.VAL .. " " .. record.EGU)
end

-- Log a string value being written to a stringout record.
-- Demonstrates: reading the written string from record.VAL.
function on_string_write(record)
	print("  on_string_write: received \"" .. record.VAL .. "\"")
end


-- === Long String Callbacks (lsi/lso) ===

-- Return a status string longer than 40 characters.
-- Demonstrates: lsi carries strings beyond the stringin 40-char limit.
function read_status(record)
	return "System nominal: all 12 subsystems reporting OK at " .. os.date("%H:%M:%S")
end

-- Log the full (untruncated) string being written to an lso record.
-- Demonstrates: reading a long string from record.VAL returns the whole
-- value, not a 40-character-truncated copy.
function on_long_write(record)
	local value = record.VAL
	print("  on_long_write: received " .. #value .. " chars: \"" .. value .. "\"")
end


-- === Array Callbacks (waveform/aao) ===

-- Return a generated ramp as a Lua table.
-- Demonstrates: an input array callback returns a table; its elements are
-- stored in the record's buffer (converted to FTVL) and NORD is set.
function read_ramp(record)
	local ramp = {}
	for i = 1, 5 do
		ramp[i] = i * 1.5
	end
	return ramp
end

-- Read the outgoing array from record.VAL and log it.
-- Demonstrates: an output array callback reads record.VAL as a Lua table.
function on_array_write(record)
	local arr = record.VAL

	local sum = 0
	for _, v in ipairs(arr) do
		sum = sum + v
	end

	print("  on_array_write: received " .. #arr .. " elements, sum = " .. sum)
end
