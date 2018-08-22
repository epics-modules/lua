# Loads a set of userscripts for testing purposes

dbLoadDatabase("../../dbd/scriptioc.dbd")
scriptioc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

dbLoadRecords("../../luaApp/Db/luascripts10.db", "P=lua:,R=test:")

#######
iocInit
#######
