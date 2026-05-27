-- Device support example script
--
-- Demonstrates DTYP "lua" device support for ai and bi records.
-- Lua callback functions are defined in scripts/dtyp.lua.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadRecords("./dtyp.db", "P=lua:,R=test:")

iocInit()
