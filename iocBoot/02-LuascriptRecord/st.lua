-- Luascript Record Example IOC
--
-- Demonstrates luascript records with external Lua functions,
-- POPT/PCAL conditional processing, and inline CODE.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

dbLoadRecords("./examples.db", "P=lua:,R=test:")

iocInit()

--
--
-- === State Machine ===
-- state_machine cycles through string inputs AA-JJ on each process.
-- PINI=1 means it already processed once at init (curr=0 -> AA="Hello")

--
-- Expected: "Hello" (first state, set at init)
dbgf("lua:test:state_machine.SVAL")

--
-- Process again: curr advances to 1 -> BB="World"
dbpf("lua:test:state_machine.PROC", 1)
dbgf("lua:test:state_machine.SVAL")

--
-- Process again: curr advances to 2 -> CC="123ABC"
dbpf("lua:test:state_machine.PROC", 1)
dbgf("lua:test:state_machine.SVAL")

--
--
-- === Pattern Matching ===
-- pattern_match runs pattern_demo('%d+') on CC="123ABC", extracting digits.
-- PINI=1, so it already ran at init.

-- Expected: "123"
dbgf("lua:test:pattern_match.SVAL")

--
--
-- === Conditional Processing (POPT/PCAL) ===
-- conditional has POPT=Conditional, PCAL="_A or _B"
-- CODE only runs when input A or B changes.
-- At init, all changed flags are true so it ran once: A+B+C = 1+2+3 = 6

dbgf("lua:test:conditional.VAL")

--
-- Change C only -- conditional should NOT re-run (PCAL checks _A or _B)
dbpf("lua:test:cval.VAL", "100")
dbgf("lua:test:conditional.VAL")

--
-- Change A -- conditional WILL re-run: A+B+C = 10+2+100 = 112
dbpf("lua:test:aval.VAL", "10")
dbgf("lua:test:conditional.VAL")

--
--
-- === Threshold Gating ===
-- threshold has POPT=Conditional, PCAL="_A and A > 5"
-- Inline CODE: "return A * 10". Only runs when A changes AND A > 5.

-- A is currently 10 (set above), and _A was true from the CPP update
-- Expected: 100 (10 * 10)
dbgf("lua:test:threshold.VAL")

--
-- Set A to 3 (below threshold) -- _A is true but A <= 5, so PCAL is false
dbpf("lua:test:aval.VAL", "3")
-- Expected: still 100 (CODE did not run)
dbgf("lua:test:threshold.VAL")

--
-- Set A to 7 (above threshold) -- _A is true and A > 5, PCAL is true
dbpf("lua:test:aval.VAL", "7")
-- Expected: 70 (7 * 10)
dbgf("lua:test:threshold.VAL")
