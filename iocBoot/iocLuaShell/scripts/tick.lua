epics = require("epics")

local flip = false

for i = 1,10 do
	flip = not flip

	if (flip) then
		print("Tick")
	else
		print("Tock")
	end
	
	epics.sleep(1.0)
end
