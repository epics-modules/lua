-- Bytestream Library Example IOC
--
-- Demonstrates the bytestream library for structured byte stream
-- formatting and parsing. Each example file creates its own records
-- and defines the Lua callback functions in a single file.
--
-- Run with:
--   ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

-- Register the lua module's lib/<arch>/ and bin/<arch>/ directories
-- so that require("bytestream"), require("re"), luaLoadFile, and
-- @file references can find installed Lua files.
luaAddModule("../..")

-- Load formatting and parsing examples.
-- Creates luascript records and registers the "bytestream_demo" state.
luaLoadFile("bytestream_examples.lua", {P="bs:"})

-- Device I/O examples (requires a real asyn port).
-- Uncomment and configure for your instrument:
--
--   luaLoadFile("device.lua", {P="bs:", PORT="SENSOR", IP="192.168.1.100:5025"})

iocInit()
