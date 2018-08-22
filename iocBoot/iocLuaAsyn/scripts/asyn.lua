-- Lua script to demonstrate reading from an asyn port
-- function writes an HTTP request to a given ip port
-- and then prints the result to the console.


-- These values get used when the port object is created
WriteTimeout = 4.0
OutTerminator = "\n\n"
InTerminator = "\n"

function get_html(port)

	-- Creates a port object linking to the
	-- asyn port with the given name
	p = asyn.port(port)
	
	
	-- Write a string across the port
	p:write("GET / HTTP/1.0")
	
	
	
	-- Read input until there isn't any left
	local input = p:read()
	
	while (input ~= nil) do
		print(input)
		
		input = p:read()
	end
end
