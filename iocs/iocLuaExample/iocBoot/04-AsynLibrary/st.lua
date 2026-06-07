-- Asyn writing and reading example script
--
-- Demonstrates reading from an asyn port using a luascript record
-- with asyn.client. Connects to a web server and fetches an HTTP response.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

drvAsynIPPortConfigure("IP", "www.google.com:80", 0, 0, 0)

dbLoadRecords("./asyn.db", "P=lua:,R=test:,PORT=IP")

iocInit()

dbpf("lua:test:script.PROC", 1)
