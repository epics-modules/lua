TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CPPFLAGS += -DUSE_TYPED_RSET


LIBRARY_IOC += lua

DBD += luaSupport.dbd


INC += luaEpics.h
lua_SRCS += luaEpics.cpp


# Build core lua language
SRC_DIRS += $(TOP)/luaApp/src/core
INC += lua.h
INC += lualib.h
INC += lauxlib.h
INC += luaconf.h

lua_SRCS += lapi.c
lua_SRCS += lauxlib.c
lua_SRCS += lbaselib.c
lua_SRCS += lbitlib.c
lua_SRCS += lcode.c
lua_SRCS += lcorolib.c
lua_SRCS += lctype.c
lua_SRCS += ldblib.c
lua_SRCS += ldebug.c
lua_SRCS += ldo.c
lua_SRCS += ldump.c
lua_SRCS += lfunc.c
lua_SRCS += lgc.c
lua_SRCS += linit.c
lua_SRCS += liolib.c
lua_SRCS += llex.c
lua_SRCS += lmathlib.c
lua_SRCS += lmem.c
lua_SRCS += loadlib.c
lua_SRCS += lobject.c
lua_SRCS += lopcodes.c
lua_SRCS += loslib.c
lua_SRCS += lparser.c
lua_SRCS += lstate.c
lua_SRCS += lstring.c
lua_SRCS += lstrlib.c
lua_SRCS += ltable.c
lua_SRCS += ltablib.c
lua_SRCS += ltm.c
lua_SRCS += lundump.c
lua_SRCS += lutf8lib.c
lua_SRCS += lvm.c
lua_SRCS += lzio.c


# Include device support
SRC_DIRS += $(TOP)/luaApp/src/devSupport
INC += devUtil.h

lua_SRCS += devLuaAi.c
lua_SRCS += devLuaAo.c
lua_SRCS += devLuaBi.c
lua_SRCS += devLuaBo.c
lua_SRCS += devLuaMbbi.c
lua_SRCS += devLuaMbbo.c
lua_SRCS += devLuaLongin.c
lua_SRCS += devLuaLongout.c
lua_SRCS += devLuaStringin.c
lua_SRCS += devLuaStringout.c
lua_SRCS += devUtil.cpp


# Include luascript record
SRC_DIRS += $(TOP)/luaApp/src/rec
DBDINC += luascriptRecord.dbd

lua_SRCS += luascriptRecord.cpp
lua_SRCS += luaSoft.c


# Build lua shell
SRC_DIRS += $(TOP)/luaApp/src/shell
INC += luaShell.h

lua_SRCS += luaShell.cpp


# Build lua libraries
SRC_DIRS += $(TOP)/luaApp/src/libs

lua_SRCS += lasynlib.cpp
lua_SRCS += lepicslib.cpp


lua_LIBS += asyn
lua_LIBS += $(EPICS_BASE_IOC_LIBS)




#Build an IOC with the ioc shell

PROD_IOC = testLuaIoc

DBD += testLuaIoc.dbd
testLuaIoc_DBD += base.dbd
testLuaIoc_DBD += drvAsynIPPort.dbd
testLuaIoc_DBD += luaSupport.dbd

testLuaIoc_LIBS += lua
testLuaIoc_LIBS += asyn
testLuaIoc_LIBS += $(EPICS_BASE_IOC_LIBS)

testLuaIoc_SRCS += testLuaIoc_registerRecordDeviceDriver.cpp

testLuaIoc_SRCS_DEFAULT += luaMain.cpp
testLuaIoc_SRCS_vxWorks += -nil-



#And build an IOC with the luashell

PROD_IOC += testLuaShell

DBD += testLuaShell.dbd
testLuaShell_DBD += base.dbd
testLuaShell_DBD += drvAsynIPPort.dbd
testLuaShell_DBD += luaSupport.dbd

testLuaShell_LIBS += lua
testLuaShell_LIBS += asyn
testLuaShell_LIBS += $(EPICS_BASE_IOC_LIBS)

testLuaShell_SRCS += testLuaShell_registerRecordDeviceDriver.cpp

testLuaShell_SRCS_DEFAULT += luashMain.cpp
testLuaShell_SRCS_vxWorks += -nil-



#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
