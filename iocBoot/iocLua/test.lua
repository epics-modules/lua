curr = -1

function state_demo()
	-- Demonstrates the use of the record as
	-- a state machine. Can replace SNL

	curr = curr + 1
	curr = curr % 10

	if     curr == 0 then 
		return AA
	elseif curr == 1 then 
		return BB
	elseif curr == 2 then 
		return CC
	elseif curr == 3 then 
		return DD
	elseif curr == 4 then 
		return EE
	elseif curr == 5 then 
		return FF
	elseif curr == 6 then 
		return GG
	elseif curr == 7 then 
		return HH
	elseif curr == 8 then 
		return II
	elseif curr == 9 then 
		return JJ
	end
end

function regex_demo(num_or_let)
	-- Demonstrates the regex processing 
	-- capabilities. Can replace scalc
	
	return string.match(CC, "%d+")
end


function arg_demo(arg1)
	-- Demonstrates that you can pass
	-- arguments to the function.
	print(arg1)
end

function log_demo()
	-- Demonstrates logging capabilities
	logfile = io.open("test.log", "a+")

	logfile:write(os.date("%c\n", os.time()))
	
	logfile:close()
end

function string_demo()
	f = load(EE)
	return f()
end

function test()
	local tempa, tempb, tempc = string.match(FF, "VOLTS%?(.*),(.*),(.*)")
	
	return tempa
end
