-- Lua Port Driver Example IOC
--
-- Demonstrates creating an asynPortDriver entirely in Lua using
-- asyn.driver.new, with read/write callbacks and db.record.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

epicsEnvSet("LUA_SCRIPT_PATH", ".:./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

luaLoadFile("scripts/driver.lua", {P="test:", PORT="TEST"})

---------------
iocInit()
---------------

dbpr("test:readback")

dbpf("test:setpoint", 20.1)
dbpr("test:readback")
