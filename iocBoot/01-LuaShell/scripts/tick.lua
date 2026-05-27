osi = require("osi")

local flip = false

for i = 1,10 do
	flip = not flip

	if (flip) then
		print("Tick")
	else
		print("Tock")
	end
	
	osi.sleep(1.0)
end
