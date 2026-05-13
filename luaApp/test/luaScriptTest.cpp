/*
 * Tests for the luascriptRecord
 *
 * Exercises inline CODE expressions, file-based scripts,
 * string/table returns, error handling, input fields,
 * output options, and asynchronous processing.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <epicsThread.h>
#include <envDefs.h>

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

static void testNumericReturn(void)
{
    testDiag("===== luascriptRecord: numeric return =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 3.0);
    testdbPutFieldOk("test:setB", DBF_DOUBLE, 4.0);
    testdbPutFieldOk("test:add.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:add.VAL", DBF_DOUBLE, 7.0);

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 10.0);
    testdbPutFieldOk("test:setB", DBF_DOUBLE, -3.0);
    testdbPutFieldOk("test:add.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:add.VAL", DBF_DOUBLE, 7.0);
}

static void testStringReturn(void)
{
    testDiag("===== luascriptRecord: string return =====");

    testdbPutFieldOk("test:str.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:str.SVAL", DBF_STRING, "hello");
}

static void testTableReturn(void)
{
    testDiag("===== luascriptRecord: table return =====");

    testdbPutFieldOk("test:tbl.PROC", DBF_LONG, 1);
    /* After processing, ASIZ should be non-zero (array was created) */
    testdbGetFieldEqual("test:tbl.ASIZ", DBF_LONG, (int)(3 * sizeof(int)));
}

static void testErrorHandling(void)
{
    testDiag("===== luascriptRecord: error handling =====");

    testdbPutFieldOk("test:err.PROC", DBF_LONG, 1);

    /* ERR field should be populated with an error message */
    DBADDR addr;
    if (dbNameToAddr("test:err.ERR", &addr) == 0)
    {
        char err_msg[200];
        long nElements = 1;
        dbGetField(&addr, DBR_STRING, err_msg, NULL, &nElements, NULL);
        testOk(strlen(err_msg) > 0, "ERR field is populated: '%s'", err_msg);
    }
    else
    {
        testFail("Could not find test:err.ERR");
    }
}

static void testFileBasedScript(void)
{
    testDiag("===== luascriptRecord: file-based script =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 5.0);
    testdbPutFieldOk("test:setB", DBF_DOUBLE, 8.0);
    testdbPutFieldOk("test:file.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:file.VAL", DBF_DOUBLE, 13.0);
}

static void testStringInputs(void)
{
    testDiag("===== luascriptRecord: string inputs =====");

    testdbPutFieldOk("test:setAA", DBF_STRING, "world");
    testdbPutFieldOk("test:strinp.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:strinp.SVAL", DBF_STRING, "hello world");
}

static void testAsyncProcessing(void)
{
    testDiag("===== luascriptRecord: async processing =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 21.0);
    testdbPutFieldOk("test:async.PROC", DBF_LONG, 1);

    /* Wait for the async thread to complete */
    epicsThreadSleep(0.5);

    testdbGetFieldEqual("test:async.VAL", DBF_DOUBLE, 42.0);
}

MAIN(luaScriptTest)
{
    testPlan(0);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    epicsEnvSet("LUA_SCRIPT_PATH", "..");
    testdbReadDatabase("luaScriptTest.db", "..", "P=test:");

    eltc(0);
    testIocInitOk();
    eltc(1);

    testNumericReturn();
    testStringReturn();
    testTableReturn();
    testErrorHandling();
    testFileBasedScript();
    testStringInputs();
    testAsyncProcessing();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
