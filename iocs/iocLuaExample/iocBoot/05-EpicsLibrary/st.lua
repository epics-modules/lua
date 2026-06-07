-- Epics Library Example IOC
--
-- Demonstrates the epics library for reading and writing PVs
-- from Lua scripts, including arrays, options, and PV objects.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

dbLoadRecords("./epics_examples.db", "P=lua:")

iocInit()

epics = require("epics")

--
--
-- === Scalar Get/Put ===
-- epics.get reads a PV value (uses local DB access for local PVs)

-- Expected: 42.5
epics.get("lua:ai")

-- epics.put writes a value, epics.get reads it back
epics.put("lua:ao", 99.9)
-- Expected: 99.9
epics.get("lua:ao")

--
--
-- === Integer and String PVs ===

-- Expected: 100
epics.get("lua:longin")

-- Expected: "hello"
epics.get("lua:stringin")

epics.put("lua:stringout", "goodbye")
-- Expected: "goodbye"
epics.get("lua:stringout")

--
--
-- === Array I/O ===
-- Write a Lua table to a double waveform, read it back

epics.put("lua:double_wf", {1.1, 2.2, 3.3, 4.4, 5.5})
-- epics.get returns a table for array PVs
do vals = epics.get("lua:double_wf"); for i, v in ipairs(vals) do print(i, v) end end

-- Integer arrays work the same way
epics.put("lua:int_wf", {10, 20, 30})
do vals = epics.get("lua:int_wf"); for i, v in ipairs(vals) do print(i, v) end end

--
--
-- === Char Waveform as String ===
-- Char waveforms are returned as Lua strings by default

epics.put("lua:char_wf", "Hello, World!")
-- Expected: "Hello, World!"
epics.get("lua:char_wf")

-- Use {string=false} to get a table of byte values instead
do vals = epics.get("lua:char_wf", {string=false}); for i, v in ipairs(vals) do print(i, v) end end

--
--
-- === Options Table ===

-- Read only the first 3 elements of a waveform with {count=N}
do vals = epics.get("lua:double_wf", {count=3}); for i, v in ipairs(vals) do print(i, v) end end

-- Read an enum PV as its string label with {string=true}
-- Expected: "off" (index 0 = ZRST)
epics.get("lua:mode", {string=true})

--
--
-- === PV Object ===
-- epics.pv creates a proxy object for a PV

pv = epics.pv("lua:ai")

-- .name returns the PV name
-- Expected: "lua:ai"
pv.name

-- Field access via dot syntax
-- Expected: 42.5
pv.VAL

-- Write via dot syntax
pv.VAL = 100.0
-- Expected: 100.0
pv.VAL

-- :get() method with options
-- Expected: 100.0
pv:get("VAL")

--
--
-- === Error Handling ===
-- Reading a nonexistent PV returns nil, "error message"

-- Expected: nil    <error string>
epics.get("nonexistent:pv")
