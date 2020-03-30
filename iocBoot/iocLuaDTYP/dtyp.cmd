# Device support example script


dbLoadDatabase("../../dbd/testLuaIoc.dbd")
testLuaIoc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

dbLoadRecords("./dtyp.db", "P=lua:,R=test:")

#######
iocInit
#######
