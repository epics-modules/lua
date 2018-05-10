# Device support example script


dbLoadDatabase("../../dbd/scriptioc.dbd")
scriptioc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

dbLoadRecords("./dtyp.db", "P=lua:,Q=test:")

#######
iocInit
#######
