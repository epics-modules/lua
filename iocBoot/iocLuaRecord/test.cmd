# Script record example script

dbLoadDatabase("../../dbd/scriptioc.dbd")
scriptioc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

dbLoadRecords("./test.db", "P=lua:,Q=test:")

#######
iocInit
#######
