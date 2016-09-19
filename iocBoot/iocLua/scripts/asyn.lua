-- Lua script to demonstrate reading from an asyn port
-- function writes an HTTP request to a given ip port
-- and then prints the result to the console.

WriteTimeout = 4.0
OutTerminator = "\n\n"
InTerminator = "\n"

function get_html(port)
	asyn.write("GET / HTTP/1.0", port)
	local input = asyn.read(port)
	
	while (input ~= nil) do
		print(input)
	
		input = asyn.read(port)
	end	
end
