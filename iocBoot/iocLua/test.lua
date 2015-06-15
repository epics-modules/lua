curr = -1

function state_demo()
	-- Demonstrates the use of the record as
	-- a state machine. Can replace SNL

	curr = curr + 1
	curr = curr % 10

	if     curr == 0 then 
		OUT0 = AA; return
	elseif curr == 1 then 
		OUT0 = BB; return
	elseif curr == 2 then 
		OUT0 = CC; return
	elseif curr == 3 then 
		OUT0 = DD; return
	elseif curr == 4 then 
		OUT0 = EE; return
	elseif curr == 5 then 
		OUT0 = FF; return
	elseif curr == 6 then 
		OUT0 = GG; return
	elseif curr == 7 then 
		OUT0 = HH; return
	elseif curr == 8 then 
		OUT0 = II; return
	elseif curr == 9 then 
		OUT0 = JJ; return
	end
end

function push_demo()
	OUT0 = A
	OUT1 = B
	OUT2 = C
	OUT3 = D
	OUT4 = E
	OUT5 = F
	OUT6 = G
	OUT7 = H
	OUT8 = I
end

function regex_demo(num_or_let)
	-- Demonstrates the regex processing 
	-- capabilities. Can replace scalc

	OUT0 = CC
	OUT1 = string.match(CC, "%d+")
	OUT2 = string.match(CC, "%a+")
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
	OUT0 = f()
end

function test()
	OUT0 = FF
	local tempa, tempb, tempc = string.match(FF, "VOLTS%?(.*),(.*),(.*)")
	OUT1 = tempa
	OUT2 = tempb
	OUT3 = tempc
	
	OUT4 = tonumber(tempa)
	OUT5 = tonumber(tempb)
	OUT6 = tonumber(tempc)
end

function clear_val()
	OUT0 = 0
	OUT1 = 0
	OUT2 = 0
	OUT3 = 0
	OUT4 = 0
	OUT5 = 0
	OUT6 = 0
	OUT7 = 0
	OUT8 = 0
end

function clear_sval()
	OUT0 = ""
	OUT1 = ""
	OUT2 = ""
	OUT3 = ""
	OUT4 = ""
	OUT5 = ""
	OUT6 = ""
	OUT7 = ""
	OUT8 = ""
end
