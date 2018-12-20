# Asyn writing and reading example script

dbLoadDatabase("../../dbd/testLuaIoc.dbd")
testLuaIoc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

drvAsynIPPortConfigure("IP", "www.google.com:80", 0,0,0)

dbLoadRecords("./asyn.db", "P=lua:,Q=test:,PORT=IP")

#######
iocInit
#######

dbpf("lua:test:script.PROC", 1)
