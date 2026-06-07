-- Event Library Example IOC
--
-- Demonstrates the event library for inter-thread signaling
-- using anonymous and named event flags.
--
-- Run with: ../../bin/<arch>/testLuaShell st.lua

dbLoadDatabase("../../dbd/testLuaShell.dbd")
testLuaShell_registerRecordDeviceDriver(pdbbase)

epicsEnvSet("LUA_SCRIPT_PATH", "./scripts")

iocInit()

event = require("event")
osi = require("osi")

--
--
-- === Anonymous Flag: Basic Operations ===
-- Anonymous flags are local to this Lua state.

flag = event.flag()

-- Expected: false (new flags start cleared)
flag:test()

flag:set()
-- Expected: true
flag:test()

-- testAndClear returns the previous value and clears the flag
-- Expected: true
flag:testAndClear()

-- Expected: false (was cleared by testAndClear)
flag:test()

--
--
-- === Named Flag: Cross-Thread Signaling ===
-- Spawn a background worker that sets "workerReady" after init.
-- Named flags are shared across all Lua states -- the worker
-- and this script both call event.flag("workerReady") and get
-- the same underlying flag.

luaSpawn("worker.lua")

ready = event.flag("workerReady")

-- Block until the worker signals it's ready (up to 5 seconds)
-- Expected: true
print("Waiting for worker...")
ready:wait(5.0)
print("Worker is ready")

--
--
-- === Named Flag: Stopping a Background Task ===
-- The worker polls event.flag("stopWorker") each iteration.
-- Setting it here tells the worker to exit.

osi.sleep(3.0)

stop = event.flag("stopWorker")
print("Sending stop signal...")
stop:set()

-- Give the worker time to see the flag and exit
osi.sleep(1.5)
