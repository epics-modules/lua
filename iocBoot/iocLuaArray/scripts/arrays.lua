-- arrays.lua
--
-- Example functions demonstrating array input and output
-- with the luascript record.

-- Scale every element of a double array by a factor.
-- Input:  AA (table of doubles from waveform)
-- Output: table of doubles (written to OUT waveform via AVAL)
function scale(factor)
	if type(AA) ~= "table" then return end

	local result = {}
	for i, v in ipairs(AA) do
		result[i] = v * factor
	end

	return result
end

-- Sum all elements of a double array.
-- Input:  AA (table of doubles from waveform)
-- Output: scalar double (stored in VAL)
function sum()
	if type(AA) ~= "table" then return 0.0 end

	local total = 0.0
	for _, v in ipairs(AA) do
		total = total + v
	end

	return total
end

-- Reverse an integer array.
-- Input:  AA (table of integers from waveform)
-- Output: table of integers (written to OUT waveform via AVAL)
function reverse()
	if type(AA) ~= "table" then return end

	local result = {}
	local n = #AA
	for i, v in ipairs(AA) do
		result[n - i + 1] = v
	end

	return result
end

-- Return the length of a char waveform string.
-- Input:  AA (string from char waveform, the default representation)
-- Output: scalar integer (stored in VAL)
function strlen()
	if type(AA) ~= "string" then return 0 end

	return #AA
end

-- Uppercase the first string from a string array.
-- Input:  AA (table of strings from string waveform)
-- Output: string (stored in SVAL)
function upper()
	if type(AA) ~= "table" then return "" end

	return string.upper(AA[1] or "")
end
