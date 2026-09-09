/*
 * Concurrency stress test for DTYP "lua" device support.
 *
 * Several records of different types all share a SINGLE named Lua state.
 * Multiple worker threads hammer dbProcess() on those records
 * concurrently. Without per-state locking, the shared Lua stack is
 * corrupted and the process crashes or produces a wrong shared counter.
 * With the Stage 1/2 per-state lock (LuaStateGuard in device support),
 * access is serialized and the run is clean.
 *
 * This test is inherently timing-dependent: without the lock the failure
 * is highly probable but not guaranteed on a single run, so a high
 * iteration count is used to make the race reproduce reliably.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <envDefs.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsAtomic.h>

#include "luaEpics.h"

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

/* Records that share the state, and how many times each is processed. */
static const char* kReadRecords[] = {
    "test:ai1", "test:ai2", "test:li1", "test:si1"
};
static const char* kWriteRecords[] = {
    "test:ao1", "test:lo1"
};

static const int NUM_THREADS = 8;
static const int ITERS_PER_THREAD = 4000;

static volatile int g_startFlag = 0;
static epicsEventId g_doneEvent;
static int g_threadsRemaining = 0;

static void processRecord(const char* pvname)
{
    DBADDR addr;
    if (dbNameToAddr(pvname, &addr) == 0)
    {
        dbScanLock(addr.precord);
        dbProcess(addr.precord);
        dbScanUnlock(addr.precord);
    }
}

static void worker(void* arg)
{
    int id = (int)(long) arg;

    /* Spin until all threads are ready, to maximize contention. */
    while (!g_startFlag)    { epicsThreadSleep(0.0); }

    const int nRead  = (int)(sizeof(kReadRecords)  / sizeof(kReadRecords[0]));
    const int nWrite = (int)(sizeof(kWriteRecords) / sizeof(kWriteRecords[0]));

    for (int i = 0; i < ITERS_PER_THREAD; i++)
    {
        /* Each thread touches read and write records, all sharing the
         * same Lua state, from different EPICS scan-lock domains. */
        processRecord(kReadRecords[(id + i) % nRead]);
        processRecord(kWriteRecords[(id + i) % nWrite]);
    }

    if (epicsAtomicDecrIntT(&g_threadsRemaining) == 0)
    {
        epicsEventSignal(g_doneEvent);
    }
}

static void testConcurrentSharedState(void)
{
    testDiag("===== DTYP lua: concurrent shared-state stress =====");

    lua_State* shared = luaFindNamedState("stress_state");
    testOk(shared != NULL, "shared named state exists");
    if (!shared)    { return; }

    g_doneEvent = epicsEventCreate(epicsEventEmpty);
    g_threadsRemaining = NUM_THREADS;
    g_startFlag = 0;

    for (int t = 0; t < NUM_THREADS; t++)
    {
        epicsThreadCreate("stressWorker",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC) worker,
                          (void*)(long) t);
    }

    /* Release the workers. */
    g_startFlag = 1;

    /* Wait for completion (generous timeout; a deadlock would hang here
     * and the harness would report failure). */
    int finished = (epicsEventWait(g_doneEvent) == epicsEventOK);
    testOk(finished, "all worker threads completed without deadlock");

    /* If we got here without crashing, the shared stack survived
     * concurrent access. Verify the shared counter equals the exact
     * number of processings -- corruption or lost updates would make
     * this wrong. Each iteration processes 1 read + 1 write = 2 bumps. */
    lua_getglobal(shared, "shared_counter");
    lua_Integer counter = lua_tointeger(shared, -1);
    lua_pop(shared, 1);

    lua_Integer expected = (lua_Integer) NUM_THREADS * ITERS_PER_THREAD * 2;

    testOk(counter == expected,
           "shared_counter == %lld (expected %lld)",
           (long long) counter, (long long) expected);

    epicsEventDestroy(g_doneEvent);
}

/*
 * Phase 2: cross-subsystem stress. luascript records (sync AND async)
 * share the SAME named state as the DTYP records, and all are hammered
 * concurrently. This exercises the Stage 3 lock-ordering fix: the async
 * luascript callback must release the Lua lock before taking dbScanLock
 * (scanLock -> luaState everywhere), or this deadlocks.
 *
 * Counting is not deterministic here (async records may be skipped by
 * dbProcess while pact is TRUE), so the assertion is the meaningful
 * property: no crash and no deadlock.
 */
static const char* kMixedRecords[] = {
    "test:ai1", "test:li1", "test:si1", "test:ao1",
    "test:script_sync", "test:script_async",
    /* linked_ai shares a lock set with script_async (via FLNK). A
     * worker holding its scanLock while wanting luaState, opposite the
     * async callback, is the AB-BA the Stage 3 ordering fix prevents. */
    "test:linked_ai"
};

static void mixedWorker(void* arg)
{
    int id = (int)(long) arg;

    while (!g_startFlag)    { epicsThreadSleep(0.0); }

    const int n = (int)(sizeof(kMixedRecords) / sizeof(kMixedRecords[0]));

    for (int i = 0; i < ITERS_PER_THREAD; i++)
    {
        processRecord(kMixedRecords[(id + i) % n]);
    }

    if (epicsAtomicDecrIntT(&g_threadsRemaining) == 0)
    {
        epicsEventSignal(g_doneEvent);
    }
}

static void testCrossSubsystem(void)
{
    testDiag("===== luascript + DTYP: cross-subsystem shared-state stress =====");

    g_doneEvent = epicsEventCreate(epicsEventEmpty);
    g_threadsRemaining = NUM_THREADS;
    g_startFlag = 0;

    for (int t = 0; t < NUM_THREADS; t++)
    {
        epicsThreadCreate("mixedWorker",
                          epicsThreadPriorityMedium,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC) mixedWorker,
                          (void*)(long) t);
    }

    g_startFlag = 1;

    /* Bounded wait: a lock-ordering deadlock manifests as a timeout
     * here (failure) rather than an indefinite hang. */
    int finished = (epicsEventWaitWithTimeout(g_doneEvent, 60.0) == epicsEventOK);
    testOk(finished, "cross-subsystem workers completed without deadlock");

    /* Give any outstanding async luascript callbacks time to finish
     * before shutdown. */
    epicsThreadSleep(0.5);

    epicsEventDestroy(g_doneEvent);
}

MAIN(luaConcurrencyTest)
{
    testPlan(4);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    epicsEnvSet("LUA_SCRIPT_PATH", "..");

    /* Create and register the shared named state, and load the stress
     * script into it, BEFORE loading the database. parseINPOUT() will
     * then bind every "@stress_state" record to this one lua_State via
     * luaFindNamedState(). */
    {
        lua_State* shared = luaNamedState("stress_state");

        std::string path = luaLocateFile(std::string("luaConcurrencyTest.lua"));
        if (!path.empty())
        {
            luaL_dofile(shared, path.c_str());
        }
        else
        {
            testDiag("WARNING: luaConcurrencyTest.lua not found");
        }
    }

    testdbReadDatabase("luaConcurrencyTest.db", "..", "P=test:");

    eltc(0);
    testIocInitOk();
    eltc(1);

    testConcurrentSharedState();
    testCrossSubsystem();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
