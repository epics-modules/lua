# Linux startup script

< ./nfsCommands

load("../../bin/vxWorks-ppc32/scriptioc.munch")

################################################################################
# Tell EPICS all about the record types, device-support modules, drivers,
# etc. in the software we just loaded (xxx.munch)
dbLoadDatabase("../../dbd/scriptioc.dbd")
scriptioc_registerRecordDeviceDriver(pdbbase)

dbLoadRecords("./test.db", "P=lua:,Q=test:")

epicsEnvSet("LUA_SCRIPT_PATH",".")

#######
iocInit
#######
