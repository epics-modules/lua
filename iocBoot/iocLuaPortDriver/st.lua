epicsEnvSet("LUA_SCRIPT_PATH", ".:./scripts")

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

luaPortDriver("TEST", "driver.lua", "START=10")

---------------
iocInit()
---------------

asyn = require("asyn")
test = asyn.driver("TEST")

print(test:readParam("INCREMENTOR"))
print(test:readParam("INCREMENTOR"))
