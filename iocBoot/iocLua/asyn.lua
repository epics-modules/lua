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
