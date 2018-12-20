# Script record example script

dbLoadDatabase("../../dbd/testLuaIoc.dbd")
testLuaIoc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

dbLoadRecords("./examples.db", "P=lua:,Q=test:")

#######
iocInit
#######
