# Loads a set of userscripts for testing purposes

dbLoadDatabase("../../dbd/testLuaIoc.dbd")
testLuaIoc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

dbLoadRecords("../../luaApp/Db/luascripts10.db", "P=lua:,R=test:")

#######
iocInit
#######
