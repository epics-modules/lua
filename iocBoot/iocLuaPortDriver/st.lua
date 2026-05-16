epicsEnvSet("LUA_SCRIPT_PATH", ".:./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

luash("scripts/test.lua", {P="test:", PORT="TEST"})

---------------
iocInit()
---------------

epics = require("epics")

dbpr("test:readback")

epics.put("test:setpoint", 20.1)
dbpr("test:readback")
