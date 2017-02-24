int_state = 0
bool_state = 0

function next_int(record)
	print(record["name"])

	int_state = int_state + 1
	return int_state
end

function next_bool()
	if bool_state == 0 then
		bool_state = 1
		return "True"
	else
		bool_state = 0
		return "False"
	end
end
