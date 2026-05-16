TOP = ../..
include $(TOP)/configure/CONFIG

USR_CPPFLAGS += -DUSE_TYPED_RSET

# Generate test DBD (merges base + asyn + lua support)
TARGETS += $(COMMON_DIR)/luaTest.dbd
DBDDEPENDS_FILES += luaTest.dbd$(DEP)
luaTest_DBD += base.dbd
luaTest_DBD += asyn.dbd
luaTest_DBD += drvAsynIPPort.dbd
luaTest_DBD += luaSupport.dbd
TESTFILES += $(COMMON_DIR)/luaTest.dbd

testHarness_SRCS += luaTest_registerRecordDeviceDriver.cpp

PROD_LIBS += lua
PROD_LIBS += asyn
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)

# --- luascriptRecord tests ---
TESTPROD_HOST += luaScriptTest
luaScriptTest_SRCS += luaScriptTest.cpp
luaScriptTest_SRCS += luaTest_registerRecordDeviceDriver.cpp
testHarness_SRCS += luaScriptTest.cpp
TESTFILES += ../luaScriptTest.db
TESTFILES += ../luaScriptTest.lua
TESTS += luaScriptTest

# --- DTYP "lua" device support tests ---
TESTPROD_HOST += luaDtypTest
luaDtypTest_SRCS += luaDtypTest.cpp
luaDtypTest_SRCS += luaTest_registerRecordDeviceDriver.cpp
testHarness_SRCS += luaDtypTest.cpp
TESTFILES += ../luaDtypTest.db
TESTFILES += ../luaDtypTest.lua
TESTS += luaDtypTest

# --- luaPortDriver tests ---
TESTPROD_HOST += luaPortDriverTest
luaPortDriverTest_SRCS += luaPortDriverTest.cpp
luaPortDriverTest_SRCS += luaTest_registerRecordDeviceDriver.cpp
testHarness_SRCS += luaPortDriverTest.cpp
TESTFILES += ../luaPortDriverTest.db
TESTFILES += ../luaPortDriverTest.lua
TESTFILES += ../luaNewDriverTest.db
TESTFILES += ../luaNewDriverTest.lua
TESTS += luaPortDriverTest

# --- Lua shell tests ---
TESTPROD_HOST += luaShellTest
luaShellTest_SRCS += luaShellTest.cpp
luaShellTest_SRCS += luaTest_registerRecordDeviceDriver.cpp
testHarness_SRCS += luaShellTest.cpp
TESTS += luaShellTest

# --- Test harness for embedded targets ---
testHarness_SRCS += epicsRunLuaTests.c
luaTestHarness_SRCS += $(testHarness_SRCS)
PROD_vxWorks = luaTestHarness
PROD_RTEMS = luaTestHarness

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

include $(TOP)/configure/RULES
