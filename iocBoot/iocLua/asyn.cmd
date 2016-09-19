# Linux startup script

< ../nfsCommands

#load("../../bin/vxWorks-ppc32/scriptioc.munch")

################################################################################
# Tell EPICS all about the record types, device-support modules, drivers,
# etc. in the software we just loaded (xxx.munch)
dbLoadDatabase("../../dbd/scriptioc.dbd")
scriptioc_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH","./scripts")

drvAsynIPPortConfigure("IP", "cars.uchicago.edu:80", 0,0,0)

dbLoadRecords("./asyn.db", "P=lua:,Q=test:,PORT=IP")

#######
iocInit
#######

dbpf("lua:test:script.PROC", 1)
