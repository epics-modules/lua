-- DTYP "lua" Device Support Example IOC
--
-- Demonstrates Lua callback functions for ai, ao, bi, longin,
-- stringin, and stringout records using DTYP "lua" device support.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadRecords("./dtyp.db", "P=lua:,R=test:")

iocInit()

--
--
-- === ai: Incrementing Counter ===
-- next_int(record, 10) reads record.VAL and adds 10 each process.
--
-- Expected: 10 (0 + 10)
dbpf("lua:test:ai.PROC", 1)
dbgf("lua:test:ai.VAL")
--
-- Expected: 20 (10 + 10)
dbpf("lua:test:ai.PROC", 1)
dbgf("lua:test:ai.VAL")


--
--
-- === bi: Boolean Toggle ===
-- next_bool returns "True" or "False" based on the current value.
--
-- Expected: True (was 0 -> returns "True")
dbpf("lua:test:bi.PROC", 1)
dbgf("lua:test:bi.VAL")
--
-- Expected: False (was 1 -> returns "False")
dbpf("lua:test:bi.PROC", 1)
dbgf("lua:test:bi.VAL")

--
--
-- === longin: Epoch Seconds ===
-- epoch_seconds returns os.time() as an integer.
--
-- Expected: current Unix timestamp
dbpf("lua:test:longin.PROC", 1)
dbgf("lua:test:longin.VAL")

--
--
-- === stringin: Formatted Timestamp ===
-- timestamp returns os.date() as a formatted string.
--
-- Expected: current date/time like "2025-05-27 14:30:00"
dbpf("lua:test:stringin.PROC", 1)
dbgf("lua:test:stringin.VAL")

--
--
-- === ao: Output Callback ===
-- on_write receives the PV object and prints record.VAL and record.EGU.
-- Output callbacks return nil for success.
--
-- Expected: prints "on_write: received 42.5 "
dbpf("lua:test:ao.VAL", "42.5")

--
--
-- === stringout: String Output Callback ===
-- on_string_write prints the string being written.
--
-- Expected: prints 'on_string_write: received "hello world"'
dbpf("lua:test:stringout.VAL", "hello world")
