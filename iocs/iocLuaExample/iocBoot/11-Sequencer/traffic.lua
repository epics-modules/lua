-- traffic.lua
--
-- Traffic light sequencer -- a Lua equivalent of the classic SNL
-- traffic light example. Creates the PV records and defines the
-- state machine in a single file.
--
-- Loaded via luaLoadFile before iocInit. The sequencer starts
-- automatically after iocInit completes.

local seq   = require("seq")
local db    = require("db")
local epics = require("epics")

-- Create records (before iocInit)
db.record("bo", P .. "red")    { ZNAM = "Off", ONAM = "On" }
db.record("bo", P .. "yellow") { ZNAM = "Off", ONAM = "On" }
db.record("bo", P .. "green")  { ZNAM = "Off", ONAM = "On" }

-- PV objects (accessed after iocInit by the sequencer callbacks)
local red    = epics.pv(P .. "red")
local yellow = epics.pv(P .. "yellow")
local green  = epics.pv(P .. "green")

-- Timing (seconds)
local redTime    = 3.0
local greenTime  = 4.0
local yellowTime = 1.0

-- Define the state machine
local prog = seq.program("trafficLight")

prog:state("red", {
	entry = function()
		red.VAL = 1; yellow.VAL = 0; green.VAL = 0
		print("Traffic: RED")
	end,

	seq.when(seq.delay(redTime)) {
		next = "green",
	},
})

prog:state("green", {
	entry = function()
		red.VAL = 0; yellow.VAL = 0; green.VAL = 1
		print("Traffic: GREEN")
	end,

	seq.when(seq.delay(greenTime)) {
		next = "yellow",
	},
})

prog:state("yellow", {
	entry = function()
		red.VAL = 0; yellow.VAL = 1; green.VAL = 0
		print("Traffic: YELLOW")
	end,

	seq.when(seq.delay(yellowTime)) {
		next = "red",
	},
})

-- Register for execution after iocInit
seq.register(prog)
