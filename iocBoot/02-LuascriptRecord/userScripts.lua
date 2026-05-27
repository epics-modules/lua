-- Loads a set of userscripts for testing purposes
--
-- Run with: ../../bin/<arch>/testLuaShell userScripts.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadRecords("../../luaApp/Db/luascripts10.db", "P=lua:,R=test:")

iocInit()
