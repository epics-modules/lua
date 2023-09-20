-- Lua script to demonstrate reading from an asyn port
-- function writes an HTTP request to a given ip port
-- and then prints the result to the console.

asyn = require("asyn")

function get_html(port)

	-- Creates a client object linking to the
	-- asyn port with the given name. In and
	-- Out terminators, as well as timeouts,
	-- are copied from the global scope.
	p = asynOctetClient.find(port, 0, "")
	p.InTerminator = "\n"
	p.OutTerminator = "\n\n"
		
	-- Write a string across the port
	p:write("GET / HTTP/1.0")
	
	-- Read input until there isn't any left
	local input = p:read()
	
	while (input ~= nil) do
		print(input)
		
		input = p:read()
	end
end
