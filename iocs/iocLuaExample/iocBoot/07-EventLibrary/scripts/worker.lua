-- worker.lua
--
-- Background worker that demonstrates named event flags.
-- Sets "workerReady" after initialization, then loops
-- until "stopWorker" is set.
--
-- Named event flags are shared across all Lua states --
-- calling event.flag("name") with the same name from
-- different states returns a reference to the same flag.

local event = require("event")
local osi = require("osi")

local ready = event.flag("workerReady")
local stop  = event.flag("stopWorker")

print("Worker: initializing...")
osi.sleep(0.5)

print("Worker: ready")
ready:set()

local count = 0
while not stop:test() do
	count = count + 1
	print("Worker: tick " .. count)
	osi.sleep(1.0)
end

print("Worker: stopped after " .. count .. " ticks")
