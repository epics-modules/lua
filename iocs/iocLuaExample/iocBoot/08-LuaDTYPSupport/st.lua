-- DTYP "lua" Device Support Example IOC
--
-- Demonstrates Lua callback functions for ai, ao, bi, longin,
-- stringin, and stringout records, plus the long-string (lsi/lso)
-- and array (waveform/aao) record types, using DTYP "lua" device
-- support.
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


--
--
-- === lsi: Long String Input ===
-- read_status returns a string longer than 40 characters; lsi stores it
-- in full (unlike stringin, which caps at 40).
--
-- Expected: the full status string, with LEN > 40
dbpf("lua:test:lsi.PROC", 1)
dbgf("lua:test:lsi.VAL$")
dbgf("lua:test:lsi.LEN")

--
--
-- === lso: Long String Output ===
-- on_long_write reads the full outgoing value from record.VAL. Writing
-- through the "VAL$" char-array field stores the whole string.
--
-- Expected: prints "on_long_write: received 55 chars: ..."
dbpf("lua:test:lso.VAL$", "This string is definitely longer than forty characters.")

--
--
-- === waveform: Array Input ===
-- read_ramp returns a Lua table {1.5, 3.0, 4.5, 6.0, 7.5}; the elements
-- are stored in the DOUBLE buffer and NORD is set to 5.
--
-- Expected: VAL = [1.5, 3, 4.5, 6, 7.5], NORD = 5
dbpf("lua:test:waveform.PROC", 1)
dbgf("lua:test:waveform.VAL")
dbgf("lua:test:waveform.NORD")

--
--
-- === aao: Array Output ===
-- on_array_write reads the outgoing array from record.VAL as a Lua table
-- and prints the element count and sum.
--
-- Expected: prints "on_array_write: received 4 elements, sum = 100"
dbpf("lua:test:aao.VAL", "[10, 20, 30, 40]")
