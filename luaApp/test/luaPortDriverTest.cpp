/*
 * Tests for the luaPortDriver
 *
 * Creates a Lua-defined asynPortDriver, connects records to it,
 * and verifies read/write operations through the Lua callbacks.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <envDefs.h>

#include "luaPortDriver.h"

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

static void testReadInt32(void)
{
    testDiag("===== luaPortDriver: readInt32 =====");

    /* Initial read should return 0 (current_value starts at 0) */
    testdbPutFieldOk("test:readback.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:readback.VAL", DBF_DOUBLE, 0.0);
}

static void testWriteInt32(void)
{
    testDiag("===== luaPortDriver: writeInt32 =====");

    /* Write a value via the setpoint record */
    testdbPutFieldOk("test:setpoint.VAL", DBF_DOUBLE, 42.0);

    /* Read it back via the readback record */
    testdbPutFieldOk("test:readback.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:readback.VAL", DBF_DOUBLE, 42.0);
}

static void testReadFloat64(void)
{
    testDiag("===== luaPortDriver: readFloat64 =====");

    testdbPutFieldOk("test:float_rb.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:float_rb.VAL", DBF_DOUBLE, 3.14159);
}

MAIN(luaPortDriverTest)
{
    testPlan(0);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    epicsEnvSet("LUA_SCRIPT_PATH", "..");

    /* Create the luaPortDriver before loading records that use it */
    new luaPortDriver("TESTPORT", "luaPortDriverTest.lua", NULL);

    testdbReadDatabase("luaPortDriverTest.db", "..", "P=test:,PORT=TESTPORT");

    eltc(0);
    testIocInitOk();
    eltc(1);

    testReadInt32();
    testWriteInt32();
    testReadFloat64();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
