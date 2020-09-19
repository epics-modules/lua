epicsEnvSet("LUA_SCRIPT_PATH", ".:./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

luaPortDriver("TEST", "driver.lua", "P=x:, R=y:, START=10")

---------------
iocInit()
---------------

epics = require("epics")

epics.get("x:y:Value")

epics.put("x:y:Increment", 1)
epics.get("x:y:Value")

epics.put("x:y:Increment", 5)
epics.get("x:y:Value")


epics.put("x:y:Set", 5)
epics.get("x:y:Value")
