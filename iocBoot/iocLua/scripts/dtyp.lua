function next_int(record)
	local curr_val = record["VAL"]

	print("Previous Value: " .. curr_val)
	print("Changing to: " .. (curr_val + 1))

	return curr_val + 1
end

function next_bool(record)
	local curr_val = record["VAL"]

	if curr_val == 0 then
		return "True"
	else
		return "False"
	end
end
