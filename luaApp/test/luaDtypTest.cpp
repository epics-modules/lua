/*
 * Tests for DTYP "lua" device support
 *
 * Exercises all 10 standard record types (ai, ao, bi, bo,
 * longin, longout, mbbi, mbbo, stringin, stringout) with
 * Lua-based device support.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <envDefs.h>

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

/* Helper: process a record by name using dbProcess */
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

static void testAi(void)
{
    testDiag("===== DTYP lua: ai =====");

    processRecord("test:ai");
    testdbGetFieldEqual("test:ai.VAL", DBF_DOUBLE, 42.5);
}

static void testAo(void)
{
    testDiag("===== DTYP lua: ao =====");

    /* ao write calls the Lua function; verify no crash */
    processRecord("test:ao");
    testPass("ao processed without crash");
}

static void testBi(void)
{
    testDiag("===== DTYP lua: bi (integer return) =====");

    processRecord("test:bi");
    /* Integer return sets RVAL; return 2 skips RVAL->VAL conversion */
    testdbGetFieldEqual("test:bi.RVAL", DBF_LONG, 1);
}

static void testBiStringReturn(void)
{
    testDiag("===== DTYP lua: bi (string return matching ONAM) =====");

    processRecord("test:bi_str");
    testdbGetFieldEqual("test:bi_str.VAL", DBF_LONG, 1);
}

static void testBo(void)
{
    testDiag("===== DTYP lua: bo =====");

    testdbPutFieldOk("test:bo.VAL", DBF_LONG, 1);
}

static void testLongin(void)
{
    testDiag("===== DTYP lua: longin =====");

    processRecord("test:longin");
    testdbGetFieldEqual("test:longin.VAL", DBF_LONG, 100);
}

static void testLongout(void)
{
    testDiag("===== DTYP lua: longout =====");

    testdbPutFieldOk("test:longout.VAL", DBF_LONG, 55);
}

static void testMbbi(void)
{
    testDiag("===== DTYP lua: mbbi =====");

    processRecord("test:mbbi");
    /* mbbi device support returns 2 (skip conversion), sets VAL directly */
    testdbGetFieldEqual("test:mbbi.VAL", DBF_LONG, 2);
}

static void testMbbo(void)
{
    testDiag("===== DTYP lua: mbbo =====");

    testdbPutFieldOk("test:mbbo.VAL", DBF_LONG, 1);
}

static void testStringin(void)
{
    testDiag("===== DTYP lua: stringin =====");

    processRecord("test:stringin");
    testdbGetFieldEqual("test:stringin.VAL", DBF_STRING, "test_value");
}

static void testStringout(void)
{
    testDiag("===== DTYP lua: stringout =====");

    testdbPutFieldOk("test:stringout.VAL", DBF_STRING, "output_test");
}

MAIN(luaDtypTest)
{
    testPlan(0);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    epicsEnvSet("LUA_SCRIPT_PATH", "..");
    testdbReadDatabase("luaDtypTest.db", "..", "P=test:");

    eltc(0);
    testIocInitOk();
    eltc(1);

    testAi();
    testAo();
    testBi();
    testBiStringReturn();
    testBo();
    testLongin();
    testLongout();
    testMbbi();
    testMbbo();
    testStringin();
    testStringout();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
