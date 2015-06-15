TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# xxxRecord.h will be created from xxxRecord.dbd

DBDINC += scriptRecord.dbd

SRC_DIRS += ../lua

INC += lua.h
INC += lualib.h
INC += lauxlib.h
INC += luaconf.h

LIBRARY_IOC += interp
interp_SRCS += lapi.c
interp_SRCS += lauxlib.c
interp_SRCS += lbaselib.c
interp_SRCS += lbitlib.c
interp_SRCS += lcode.c
interp_SRCS += lcorolib.c
interp_SRCS += lctype.c
interp_SRCS += ldblib.c
interp_SRCS += ldebug.c
interp_SRCS += ldo.c
interp_SRCS += ldump.c
interp_SRCS += lfunc.c
interp_SRCS += lgc.c
interp_SRCS += linit.c
interp_SRCS += liolib.c
interp_SRCS += llex.c
interp_SRCS += lmathlib.c
interp_SRCS += lmem.c
interp_SRCS += loadlib.c
interp_SRCS += lobject.c
interp_SRCS += lopcodes.c
interp_SRCS += loslib.c
interp_SRCS += lparser.c
interp_SRCS += lstate.c
interp_SRCS += lstring.c
interp_SRCS += lstrlib.c
interp_SRCS += ltable.c
interp_SRCS += ltablib.c
interp_SRCS += ltm.c
interp_SRCS += lua.c
interp_SRCS += luac.c
interp_SRCS += lundump.c
interp_SRCS += lutf8lib.c
interp_SRCS += lvm.c
interp_SRCS += lzio.c


LIBRARY_IOC += script

script_SRCS += scriptRecord.c
script_SRCS += devScriptSoft.c
script_SRCS += scriptUtil.cpp

#LIB_INSTALLS_Linux += ../os/Linux/liblua.a

script_LIBS += interp
script_LIBS += $(EPICS_BASE_IOC_LIBS)


PROD_IOC = scriptioc

DBD += scriptioc.dbd
scriptioc_DBD += base.dbd
scriptioc_DBD += scriptSupport.dbd

scriptioc_LIBS += script
scriptioc_LIBS += interp
scriptioc_LIBS += $(EPICS_BASE_IOC_LIBS)


scriptioc_SRCS         += scriptioc_registerRecordDeviceDriver.cpp

scriptioc_SRCS_DEFAULT += scriptMain.cpp
scriptioc_SRCS_vxWorks += -nil-


#=============================
# build an ioc application for testing

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
