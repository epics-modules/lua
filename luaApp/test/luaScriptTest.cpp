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

/* --- Array I/O tests --- */

static void testDoubleArrayInput(void)
{
    testDiag("===== luascriptRecord: double array input =====");

    testdbPutFieldOk("test:arr_double.PROC", DBF_LONG, 1);
    /* arr_sum adds 1+2+3+4+5 = 15 */
    testdbGetFieldEqual("test:arr_double.VAL", DBF_DOUBLE, 15.0);
}

static void testIntArrayInput(void)
{
    testDiag("===== luascriptRecord: integer array input =====");

    testdbPutFieldOk("test:arr_int.PROC", DBF_LONG, 1);
    /* arr_count returns #AA = 5 */
    testdbGetFieldEqual("test:arr_int.VAL", DBF_DOUBLE, 5.0);
}

static void testCharArrayInput(void)
{
    testDiag("===== luascriptRecord: char array input (string) =====");

    testdbPutFieldOk("test:arr_char.PROC", DBF_LONG, 1);
    /* str_len returns #AA where AA is "Hello" (5 chars) */
    testdbGetFieldEqual("test:arr_char.VAL", DBF_DOUBLE, 5.0);
}

static void testStringArrayInput(void)
{
    testDiag("===== luascriptRecord: string array input =====");

    testdbPutFieldOk("test:arr_string.PROC", DBF_LONG, 1);
    /* str_first returns AA[1] = "alpha" */
    testdbGetFieldEqual("test:arr_string.SVAL", DBF_STRING, "alpha");
}

static void testArrayOutput(void)
{
    testDiag("===== luascriptRecord: array output =====");

    testdbPutFieldOk("test:arr_output.PROC", DBF_LONG, 1);
    /* Returns {10,20,30} -- ASIZ should be non-zero */
    testdbGetFieldEqual("test:arr_output.ASIZ", DBF_LONG, (int)(3 * sizeof(double)));
}

/* --- Additional async tests --- */

static void testAsyncSameResult(void)
{
    testDiag("===== luascriptRecord: async same result as sync =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 7.0);
    testdbPutFieldOk("test:setB", DBF_DOUBLE, 3.0);
    testdbPutFieldOk("test:async_val.PROC", DBF_LONG, 1);

    epicsThreadSleep(0.5);

    testdbGetFieldEqual("test:async_val.VAL", DBF_DOUBLE, 10.0);
}

static void testAsyncPactClears(void)
{
    testDiag("===== luascriptRecord: async PACT clears =====");

    testdbPutFieldOk("test:async_val.PROC", DBF_LONG, 1);

    epicsThreadSleep(0.5);

    testdbGetFieldEqual("test:async_val.PACT", DBF_SHORT, 0);
}

static void testAsyncError(void)
{
    testDiag("===== luascriptRecord: async error sets alarm =====");

    testdbPutFieldOk("test:async_err.PROC", DBF_LONG, 1);

    epicsThreadSleep(0.5);

    testdbGetFieldEqual("test:async_err.SEVR", DBF_SHORT, (int) INVALID_ALARM);
    testdbGetFieldEqual("test:async_err.STAT", DBF_SHORT, (int) CALC_ALARM);
}

/* --- IVOA tests --- */

static void testIvoaContinue(void)
{
    testDiag("===== luascriptRecord: IVOA continue normally =====");

    testdbPutFieldOk("test:ivoa_continue.PROC", DBF_LONG, 1);
    /* Error occurred but IVOA=Continue -- alarm should be set */
    testdbGetFieldEqual("test:ivoa_continue.SEVR", DBF_SHORT, (int) INVALID_ALARM);
}

static void testIvoaDontDrive(void)
{
    testDiag("===== luascriptRecord: IVOA don't drive outputs =====");

    /* Set target to a known value */
    testdbPutFieldOk("test:ivoa_target", DBF_DOUBLE, 123.0);

    testdbPutFieldOk("test:ivoa_dont.PROC", DBF_LONG, 1);
    /* Target should NOT have changed */
    testdbGetFieldEqual("test:ivoa_target.VAL", DBF_DOUBLE, 123.0);
    /* Alarm should still be set */
    testdbGetFieldEqual("test:ivoa_dont.SEVR", DBF_SHORT, (int) INVALID_ALARM);
}

static void testIvoaSetToIvov(void)
{
    testDiag("===== luascriptRecord: IVOA set to IVOV =====");

    /* Set target to a known value */
    testdbPutFieldOk("test:ivoa_target", DBF_DOUBLE, 0.0);

    testdbPutFieldOk("test:ivoa_setivov.PROC", DBF_LONG, 1);
    /* VAL should be IVOV (99.0) */
    testdbGetFieldEqual("test:ivoa_setivov.VAL", DBF_DOUBLE, 99.0);
}

static void testIvoaNoEffectOnSuccess(void)
{
    testDiag("===== luascriptRecord: IVOA no effect on success =====");

    testdbPutFieldOk("test:ivoa_success.PROC", DBF_LONG, 1);
    /* Code returns 42, not IVOV (99) */
    testdbGetFieldEqual("test:ivoa_success.VAL", DBF_DOUBLE, 42.0);
}

/* --- OOPT tests --- */

static void testOoptNever(void)
{
    testDiag("===== luascriptRecord: OOPT never =====");

    /* Set target to known value */
    testdbPutFieldOk("test:oopt_target", DBF_DOUBLE, 0.0);

    testdbPutFieldOk("test:oopt_never.PROC", DBF_LONG, 1);
    /* VAL is 42 but OOPT=Never, so target should NOT change */
    testdbGetFieldEqual("test:oopt_never.VAL", DBF_DOUBLE, 42.0);
    testdbGetFieldEqual("test:oopt_target.VAL", DBF_DOUBLE, 0.0);
}


/* --- POPT/PCAL tests --- */

static void testPoptAlways(void)
{
    testDiag("===== luascriptRecord: POPT always =====");

    testdbPutFieldOk("test:setA", DBF_DOUBLE, 5.0);
    testdbPutFieldOk("test:popt_always.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:popt_always.VAL", DBF_DOUBLE, 6.0);

    /* Process again without changing A -- should still run CODE */
    testdbPutFieldOk("test:popt_always.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:popt_always.VAL", DBF_DOUBLE, 6.0);
}

static void testPoptConditional(void)
{
    testDiag("===== luascriptRecord: POPT conditional =====");

    /* Change A and process -- PCAL="_A" should be true, CODE runs */
    testdbPutFieldOk("test:setA", DBF_DOUBLE, 10.0);
    testdbPutFieldOk("test:popt_cond.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:popt_cond.VAL", DBF_DOUBLE, 11.0);

    /* Process again without changing A -- _A is false, CODE skipped */
    testdbPutFieldOk("test:popt_cond.PROC", DBF_LONG, 1);
    /* VAL should still be 11.0 from the previous process */
    testdbGetFieldEqual("test:popt_cond.VAL", DBF_DOUBLE, 11.0);

    /* Change A again -- CODE should run again */
    testdbPutFieldOk("test:setA", DBF_DOUBLE, 20.0);
    testdbPutFieldOk("test:popt_cond.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:popt_cond.VAL", DBF_DOUBLE, 21.0);
}

static void testPcalEmpty(void)
{
    testDiag("===== luascriptRecord: PCAL empty =====");

    /* POPT=Conditional but PCAL is empty -- CODE should NOT run */
    testdbPutFieldOk("test:popt_empty.PROC", DBF_LONG, 1);
    /* VAL should remain 0 (never processed) */
    testdbGetFieldEqual("test:popt_empty.VAL", DBF_DOUBLE, 0.0);
}

static void testPcalChangedFlag(void)
{
    testDiag("===== luascriptRecord: PCAL changed flag =====");

    /* Set A to a value, then process the conditional record */
    testdbPutFieldOk("test:setA", DBF_DOUBLE, 5.0);
    testdbPutFieldOk("test:popt_cond.PROC", DBF_LONG, 1);
    double val1;
    testdbGetFieldEqual("test:popt_cond.VAL", DBF_DOUBLE, 6.0);

    /* Set A to the SAME value -- _A should be false */
    testdbPutFieldOk("test:setA", DBF_DOUBLE, 5.0);
    testdbPutFieldOk("test:popt_cond.PROC", DBF_LONG, 1);
    /* VAL unchanged because _A was false */
    testdbGetFieldEqual("test:popt_cond.VAL", DBF_DOUBLE, 6.0);
}

static void testPcalError(void)
{
    testDiag("===== luascriptRecord: PCAL error =====");

    testdbPutFieldOk("test:popt_err.PROC", DBF_LONG, 1);

    /* ERR field should be populated with PCAL error */
    DBADDR addr;
    if (dbNameToAddr("test:popt_err.ERR", &addr) == 0)
    {
        char err_msg[256];
        long nElements = 1;
        dbGetField(&addr, DBR_STRING, err_msg, NULL, &nElements, NULL);
        testOk(strlen(err_msg) > 0, "ERR field populated with PCAL error: '%s'", err_msg);
    }
    else
    {
        testFail("Could not find test:popt_err.ERR");
    }

    /* VAL should remain 0 (CODE never ran) */
    testdbGetFieldEqual("test:popt_err.VAL", DBF_DOUBLE, 0.0);
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

    /* Initialize waveform test data (replaces JSON INP syntax for 3.15 compat) */
    {
        DBADDR addr;
        double dvals[] = {1.0, 2.0, 3.0, 4.0, 5.0};
        dbNameToAddr("test:double_wf", &addr);
        dbPutField(&addr, DBF_DOUBLE, dvals, 5);

        epicsInt32 ivals[] = {10, 20, 30, 40, 50};
        dbNameToAddr("test:int_wf", &addr);
        dbPutField(&addr, DBF_LONG, ivals, 5);

        char cvals[] = {72, 101, 108, 108, 111};  /* "Hello" */
        dbNameToAddr("test:char_wf", &addr);
        dbPutField(&addr, DBF_CHAR, cvals, 5);

        char svals[3][MAX_STRING_SIZE];
        memset(svals, 0, sizeof(svals));
        strncpy(svals[0], "alpha", MAX_STRING_SIZE - 1);
        strncpy(svals[1], "beta",  MAX_STRING_SIZE - 1);
        strncpy(svals[2], "gamma", MAX_STRING_SIZE - 1);
        dbNameToAddr("test:string_wf", &addr);
        dbPutField(&addr, DBF_STRING, svals, 3);
    }

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

    /* Array I/O */
    testDoubleArrayInput();
    testIntArrayInput();
    testCharArrayInput();
    testStringArrayInput();
    testArrayOutput();

    /* Async processing */
    testAsyncSameResult();
    testAsyncPactClears();
    testAsyncError();

    /* IVOA */
    testIvoaContinue();
    testIvoaDontDrive();
    testIvoaSetToIvov();
    testIvoaNoEffectOnSuccess();

    /* OOPT */
    testOoptNever();

    /* POPT/PCAL */
    testPoptAlways();
    testPoptConditional();
    testPcalEmpty();
    testPcalChangedFlag();
    testPcalError();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
