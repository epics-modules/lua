-- Database Library Example IOC
--
-- Demonstrates the db library for creating and inspecting EPICS
-- records from Lua, including db.record, db.loadRecords,
-- db.loadTemplate, and db.list.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

-- Create records and load templates before iocInit
luaLoadFile("db_examples.lua", {P="lua:"})

iocInit()

epics = require("epics")
db = require("db")

--
--
-- === Records created by db.record ===

-- Expected: 25
epics.get("lua:created_ai")

-- Expected: "created by db.record"
epics.get("lua:created_si")

-- Fields set in the chained call are visible
-- Expected: "degC"
dbgf("lua:created_ai.EGU")

-- Expected: "Created from Lua"
dbgf("lua:created_ai.DESC")

--
--
-- === Records loaded by db.loadRecords with table macros ===

-- Expected: "First sensor"
dbgf("lua:sensor_1.DESC")

-- Expected: "V"
dbgf("lua:sensor_2.EGU")

--
--
-- === Records loaded by db.loadTemplate ===
-- Three records created from the pattern substitution

-- Expected: "Channel 10", "Channel 11", "Channel 12"
dbgf("lua:sensor_10.DESC")
dbgf("lua:sensor_11.DESC")
dbgf("lua:sensor_12.DESC")

-- All share the global PREC="1"
-- Expected: 1
dbgf("lua:sensor_10.PREC")
dbgf("lua:sensor_12.PREC")

-- Each has the per-row EGU from the pattern
-- Expected: "mA"
dbgf("lua:sensor_11.EGU")

--
--
-- === db.list: Enumerate all records ===

records = db.list()
print("Total records: " .. #records)
do for _, rec in ipairs(records) do print("  " .. rec.name) end end
