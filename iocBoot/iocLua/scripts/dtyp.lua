-- Lua script to demonstrate lua device support

-- Function to increment an ai record by a 
-- given value everytime it's processed.
function next_int(record, amount)

	-- record is an object representing an
	-- epics PV, you can use array notation
	-- to access values from the pv
	local curr_val = record["VAL"]

	-- Log information to the shell
	-- Amount is a parameter fed by the INP field of the record
	print("Previous Value: " .. curr_val)
	print("Changing to: " .. (curr_val + amount))

	-- Returned values are used as if they were raw device readings
	return curr_val + amount
end

-- Function to flip-flop a bi record between
-- True and False each time it's processed
function next_bool(record)
	local curr_val = record["VAL"]

	if curr_val == 0 then
		return "True"
	else
		return "False"
	end
end
