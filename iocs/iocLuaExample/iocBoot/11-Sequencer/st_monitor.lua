-- Sequencer Library Example IOC -- Voltage Monitor
--
-- Demonstrates seq library transitions with condition functions,
-- action callbacks, unconditional transitions, event flags, and
-- the "exit" state.
--
-- Run with: ../../bin/<arch>/testLuaShell st_monitor.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

luaAddModule("../..")

luaLoadFile("monitor.lua", {P="lua:"})

iocInit()

epics = require("epics")
event = require("event")
osi = require("osi")

-- Wait for sequencer to start
osi.sleep(0.5)

--
--
-- === Drive voltage above the high threshold (8.0) ===
-- Expected: Monitor prints "ALARM" and alarm PV changes to 1

epics.put("lua:voltage", 10.0)
osi.sleep(0.5)
-- Expected: 1
epics.get("lua:alarm")

--
--
-- === Drive voltage below the low threshold (3.0) ===
-- Expected: Monitor prints "OK" and alarm PV changes to 0

epics.put("lua:voltage", 1.0)
osi.sleep(0.5)
-- Expected: 0
epics.get("lua:alarm")

--
--
-- === Voltage in the hysteresis band (between 3.0 and 8.0) ===
-- Expected: alarm stays in its current state (0 = OK)

epics.put("lua:voltage", 5.0)
osi.sleep(0.5)
-- Expected: 0 (still OK, hasn't crossed HI threshold)
epics.get("lua:alarm")

--
--
-- === Drive above threshold again ===

epics.put("lua:voltage", 9.0)
osi.sleep(0.5)
-- Expected: 1
epics.get("lua:alarm")

--
--
-- === Stop the monitor via named event flag ===

event.flag("stopMonitor"):set()
osi.sleep(0.5)
