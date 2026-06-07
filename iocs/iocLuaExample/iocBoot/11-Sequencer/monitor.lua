-- monitor.lua
--
-- Voltage monitor sequencer -- demonstrates condition functions,
-- action callbacks, unconditional transitions, event flags, and
-- the "exit" state.
--
-- The monitor watches a voltage PV and sets an alarm PV when the
-- voltage exceeds a high threshold. The alarm clears when the
-- voltage drops below a low threshold (hysteresis). The monitor
-- can be stopped externally via a named event flag.

local seq   = require("seq")
local db    = require("db")
local epics = require("epics")
local event = require("event")

-- Create records
db.record("ao", P .. "voltage") { VAL = "0", PREC = "2" }
db.record("bo", P .. "alarm")   { ZNAM = "OK", ONAM = "ALARM" }

-- PV objects
local voltage = epics.pv(P .. "voltage")
local alarm   = epics.pv(P .. "alarm")

-- Named event flag for external stop signal
local stop = event.flag("stopMonitor")

-- Thresholds with hysteresis
local HI = 8.0
local LO = 3.0

local prog = seq.program("voltageMonitor")

-- Initial setup: unconditional transition to normal
prog:state("init", {
	seq.when() {
		action = function()
			alarm.VAL = 0
			print("Monitor: started (HI=" .. HI .. " LO=" .. LO .. ")")
		end,
		next = "normal",
	},
})

-- Normal state: watch for voltage exceeding the high threshold
prog:state("normal", {
	seq.when(function() return voltage.VAL > HI end) {
		action = function()
			alarm.VAL = 1
			print("Monitor: ALARM (voltage " .. voltage.VAL .. " > " .. HI .. ")")
		end,
		next = "alarm",
	},

	seq.when(function() return stop:test() end) {
		action = function() print("Monitor: stopped") end,
		next = "exit",
	},

	seq.when(seq.delay(0.1)) {
		next = "normal",
	},
})

-- Alarm state: watch for voltage dropping below the low threshold
prog:state("alarm", {
	seq.when(function() return voltage.VAL < LO end) {
		action = function()
			alarm.VAL = 0
			print("Monitor: OK (voltage " .. voltage.VAL .. " < " .. LO .. ")")
		end,
		next = "normal",
	},

	seq.when(function() return stop:test() end) {
		action = function() print("Monitor: stopped") end,
		next = "exit",
	},

	seq.when(seq.delay(0.1)) {
		next = "alarm",
	},
})

seq.register(prog)
