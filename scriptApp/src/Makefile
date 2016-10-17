TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# xxxRecord.h will be created from xxxRecord.dbd

DBD += scriptSupport.dbd
DBDINC += luascriptRecord.dbd

SRC_DIRS += ../lua/core

INC += lua.h
INC += lualib.h
INC += lauxlib.h
INC += luaconf.h
INC += luaEpics.h

LIBRARY_IOC += script
script_SRCS += lapi.c
script_SRCS += lauxlib.c
script_SRCS += lbaselib.c
script_SRCS += lbitlib.c
script_SRCS += lcode.c
script_SRCS += lcorolib.c
script_SRCS += lctype.c
script_SRCS += ldblib.c
script_SRCS += ldebug.c
script_SRCS += ldo.c
script_SRCS += ldump.c
script_SRCS += lfunc.c
script_SRCS += lgc.c
script_SRCS += linit.c
script_SRCS += liolib.c
script_SRCS += llex.c
script_SRCS += lmathlib.c
script_SRCS += lmem.c
script_SRCS += loadlib.c
script_SRCS += lobject.c
script_SRCS += lopcodes.c
script_SRCS += loslib.c
script_SRCS += lparser.c
script_SRCS += lstate.c
script_SRCS += lstring.c
script_SRCS += lstrlib.c
script_SRCS += ltable.c
script_SRCS += ltablib.c
script_SRCS += ltm.c
script_SRCS += lua.c
script_SRCS += luac.c
script_SRCS += lundump.c
script_SRCS += lutf8lib.c
script_SRCS += lvm.c
script_SRCS += lzio.c
script_SRCS += lasynlib.cpp
script_SRCS += lepicslib.cpp

script_SRCS += luascriptRecord.c
script_SRCS += luaSoft.c
script_SRCS += luascriptUtil.cpp
script_SRCS += luaEpics.cpp

script_LIBS += asyn
script_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_IOC = scriptioc

DBD += scriptioc.dbd
scriptioc_DBD += base.dbd
scriptioc_DBD += drvAsynIPPort.dbd
scriptioc_DBD += scriptSupport.dbd

scriptioc_LIBS += script
scriptioc_LIBS += asyn
scriptioc_LIBS += $(EPICS_BASE_IOC_LIBS)

scriptioc_SRCS += scriptioc_registerRecordDeviceDriver.cpp

scriptioc_SRCS_DEFAULT += scriptMain.cpp
scriptioc_SRCS_vxWorks += -nil-


#=============================
# build an ioc application for testing

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
