-- Array I/O Example IOC
--
-- Demonstrates luascript record array input and output with:
--   - Double waveform input  -> table of numbers in Lua -> scaled output
--   - Integer waveform input -> table of integers in Lua -> reversed output
--   - Char waveform input    -> Lua string -> string length
--   - String waveform input  -> table of strings in Lua -> uppercase
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

dbLoadRecords("./arrays.db", "P=lua:,R=arr:")

iocInit()

-- Initialize waveform test data
epics = require("epics")

epics.put("lua:arr:double_input", {1.0, 2.0, 3.0, 4.0, 5.0})
epics.put("lua:arr:int_input",    {10, 20, 30, 40, 50})
epics.put("lua:arr:char_input",   "Hello, World!")
epics.put("lua:arr:string_input", {"Hello", "World", "Lua", "EPICS", "Array"})

-- Process each record and display results
print("\n--- Double array: scale by 2.0 ---")
dbpf("lua:arr:scale.PROC", 1)
dbgf("lua:arr:double_output")

print("\n--- Double array: sum ---")
dbpf("lua:arr:sum.PROC", 1)
dbgf("lua:arr:sum.VAL")

print("\n--- Integer array: reverse ---")
dbpf("lua:arr:reverse.PROC", 1)
dbgf("lua:arr:int_output")

print("\n--- Char waveform: string length ---")
dbpf("lua:arr:strlen.PROC", 1)
dbgf("lua:arr:strlen.VAL")

print("\n--- String array: uppercase first element ---")
dbpf("lua:arr:upper.PROC", 1)
dbgf("lua:arr:upper.SVAL")
