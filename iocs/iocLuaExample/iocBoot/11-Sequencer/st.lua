-- Sequencer Library Example IOC
--
-- Demonstrates the seq library as a Lua replacement for the EPICS
-- State Notation Language (SNL) sequencer. Implements a traffic
-- light controller with records and state machine in one file.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

luaAddModule("../..")

-- Load the traffic light definition (creates records + registers sequencer)
luaLoadFile("traffic.lua", {P="lua:"})

iocInit()

-- The sequencer starts automatically after iocInit.
-- Watch the traffic light cycle for a few rounds.

epics = require("epics")
osi = require("osi")

--
--
-- === Observe the traffic light cycling ===

for i = 1, 20 do
	osi.sleep(1.0)
	do 
		r = epics.get("lua:red"); 
		y = epics.get("lua:yellow"); 
		g = epics.get("lua:green"); 
		print(string.format("  Red: %d  Yellow: %d  Green: %d", r, y, g)) 
	end
end
