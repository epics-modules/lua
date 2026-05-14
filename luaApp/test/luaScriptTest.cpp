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
#include <alarm.h>

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

static void testUdfClearing(void)
{
    testDiag("===== luascriptRecord: UDF clearing =====");

    /* UDF should be 1 (TRUE) before first successful process */
    testdbGetFieldEqual("test:udf.UDF", DBF_SHORT, 1);

    testdbPutFieldOk("test:udf.PROC", DBF_LONG, 1);

    /* After processing, UDF should be cleared */
    testdbGetFieldEqual("test:udf.UDF", DBF_SHORT, 0);
    testdbGetFieldEqual("test:udf.VAL", DBF_DOUBLE, 42.0);
}

static void testCalcAlarmOnError(void)
{
    testDiag("===== luascriptRecord: alarm on error =====");

    testdbPutFieldOk("test:err.PROC", DBF_LONG, 1);

    /* CALC_ALARM is set first (INVALID_ALARM severity). checkAlarms then
     * checks UDF (still TRUE since no value was produced), but UDF_ALARM
     * at INVALID_ALARM cannot override since recGblSetSevr only replaces
     * when new severity is strictly greater. STAT = CALC_ALARM. */
    testdbGetFieldEqual("test:err.SEVR", DBF_SHORT, (int) INVALID_ALARM);
    testdbGetFieldEqual("test:err.STAT", DBF_SHORT, (int) CALC_ALARM);

    /* ERR field should be populated */
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

static void testHihiAlarm(void)
{
    testDiag("===== luascriptRecord: HIHI alarm =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 95.0);
    testdbPutFieldOk("test:alarm_hihi.PROC", DBF_LONG, 1);

    testdbGetFieldEqual("test:alarm_hihi.SEVR", DBF_SHORT, (int) MAJOR_ALARM);
    testdbGetFieldEqual("test:alarm_hihi.STAT", DBF_SHORT, (int) HIHI_ALARM);
}

static void testHighAlarm(void)
{
    testDiag("===== luascriptRecord: HIGH alarm =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 55.0);
    testdbPutFieldOk("test:alarm_hihi.PROC", DBF_LONG, 1);

    testdbGetFieldEqual("test:alarm_hihi.SEVR", DBF_SHORT, (int) MINOR_ALARM);
    testdbGetFieldEqual("test:alarm_hihi.STAT", DBF_SHORT, (int) HIGH_ALARM);
}

static void testLowAlarm(void)
{
    testDiag("===== luascriptRecord: LOW alarm =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, -55.0);
    testdbPutFieldOk("test:alarm_hihi.PROC", DBF_LONG, 1);

    testdbGetFieldEqual("test:alarm_hihi.SEVR", DBF_SHORT, (int) MINOR_ALARM);
    testdbGetFieldEqual("test:alarm_hihi.STAT", DBF_SHORT, (int) LOW_ALARM);
}

static void testLoloAlarm(void)
{
    testDiag("===== luascriptRecord: LOLO alarm =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, -95.0);
    testdbPutFieldOk("test:alarm_hihi.PROC", DBF_LONG, 1);

    testdbGetFieldEqual("test:alarm_hihi.SEVR", DBF_SHORT, (int) MAJOR_ALARM);
    testdbGetFieldEqual("test:alarm_hihi.STAT", DBF_SHORT, (int) LOLO_ALARM);
}

static void testNoAlarm(void)
{
    testDiag("===== luascriptRecord: no alarm in range =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 25.0);
    testdbPutFieldOk("test:alarm_hihi.PROC", DBF_LONG, 1);

    testdbGetFieldEqual("test:alarm_hihi.SEVR", DBF_SHORT, (int) NO_ALARM);
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
    testUdfClearing();
    testCalcAlarmOnError();
    testHihiAlarm();
    testHighAlarm();
    testLowAlarm();
    testLoloAlarm();
    testNoAlarm();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
