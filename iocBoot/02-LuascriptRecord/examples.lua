-- Script record example
--
-- Demonstrates luascript records with external Lua functions,
-- POPT/PCAL conditional processing, and inline CODE.
--
-- Run with: ../../bin/<arch>/testLuaShell examples.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadRecords("./examples.db", "P=lua:,R=test:")

iocInit()
