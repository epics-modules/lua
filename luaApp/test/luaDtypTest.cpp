/*
 * Tests for DTYP "lua" device support
 *
 * Exercises the supported record types (ai, ao, bi, bo, longin,
 * longout, mbbi, mbbo, mbbiDirect, mbboDirect, stringin, stringout,
 * lsi, lso, waveform, aai, aao, and int64in/int64out on base 7.0+) with
 * Lua-based device support.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <envDefs.h>
#include <alarm.h>
#include <link.h>

#include "luaEpics.h"
#include "devUtil.h"

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
    /* Integer return sets RVAL; return 0 enables RVAL->VAL conversion */
    testdbGetFieldEqual("test:bi.RVAL", DBF_LONG, 1);
    testdbGetFieldEqual("test:bi.VAL", DBF_LONG, 1);
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
    testDiag("===== DTYP lua: mbbi (no state values) =====");

    processRecord("test:mbbi");
    /* No ZRVL..FFVL defined: sets VAL directly, returns 2 */
    testdbGetFieldEqual("test:mbbi.VAL", DBF_LONG, 2);
}

static void testMbbiRval(void)
{
    testDiag("===== DTYP lua: mbbi (with state values, RVAL conversion) =====");

    processRecord("test:mbbi_rval");
    /* ZRVL..TWVL defined: sets RVAL, returns 0 for RVAL->VAL conversion */
    testdbGetFieldEqual("test:mbbi_rval.RVAL", DBF_LONG, 2);
    testdbGetFieldEqual("test:mbbi_rval.VAL", DBF_LONG, 2);
}

static void testMbbo(void)
{
    testDiag("===== DTYP lua: mbbo =====");

    testdbPutFieldOk("test:mbbo.VAL", DBF_LONG, 1);
}

static void testMbbiDirect(void)
{
    testDiag("===== DTYP lua: mbbiDirect (bitfield) =====");

    processRecord("test:mbbiDirect");
    /* read_bits returns 5 (bits 0 and 2); set directly, returns 2 */
    testdbGetFieldEqual("test:mbbiDirect.VAL", DBF_LONG, 5);
}

static void testMbboDirect(void)
{
    testDiag("===== DTYP lua: mbboDirect =====");

    testdbPutFieldOk("test:mbboDirect.VAL", DBF_LONG, 5);
}

#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 0, 0)
static void testInt64in(void)
{
    testDiag("===== DTYP lua: int64in (64-bit precision) =====");

    processRecord("test:int64in");
    /* read_bignum returns 5e9, which exceeds 32-bit range; the value
     * must be preserved exactly (no truncation or double rounding). */
    testdbGetFieldEqual("test:int64in.VAL", DBF_INT64, (epicsInt64) 5000000000LL);
}

static void testInt64out(void)
{
    testDiag("===== DTYP lua: int64out =====");

    testdbPutFieldOk("test:int64out.VAL", DBF_INT64, (epicsInt64) 9000000000LL);
}
#endif

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

/* Read a long-string record's VAL as a full char array (not the
 * 40-char-truncated DBF_STRING view). The "$" field modifier requests
 * the char-array view of a long string field. */
static void getLongString(const char* pvname, char* out, size_t outsize)
{
    DBADDR addr;
    out[0] = '\0';
    if (dbNameToAddr(pvname, &addr) != 0) { return; }

    long nReq = (long) outsize - 1;
    long options = 0;
    if (dbGetField(&addr, DBR_CHAR, out, &options, &nReq, NULL) == 0)
    {
        if (nReq < 0) { nReq = 0; }
        if ((size_t) nReq >= outsize) { nReq = (long) outsize - 1; }
        out[nReq] = '\0';
    }
}

static void testLsi(void)
{
    testDiag("===== DTYP lua: lsi (long string in, > 40 chars) =====");

    /* Expected value matches read_longstring() in luaDtypTest.lua:
     * string.rep("abcdefghij", 10) = 100 chars. */
    char expected[128];
    for (int i = 0; i < 10; i++) { memcpy(expected + i * 10, "abcdefghij", 10); }
    expected[100] = '\0';

    processRecord("test:lsi");

    char got[256];
    getLongString("test:lsi.VAL$", got, sizeof(got));

    testOk(strcmp(got, expected) == 0,
           "lsi.VAL holds full 100-char string (len=%d)", (int) strlen(got));
    testdbGetFieldEqual("test:lsi.LEN", DBF_ULONG, 101);  /* 100 chars + nul */
}

static void testLso(void)
{
    testDiag("===== DTYP lua: lso (long string out, > 40 chars) =====");

    /* Write a 100-char string to the char-array view of VAL, process the
     * record (runs the Lua callback), then read VAL back and confirm the
     * full value was stored without truncation at 40 chars. */
    char value[128];
    for (int i = 0; i < 10; i++) { memcpy(value + i * 10, "0123456789", 10); }
    value[100] = '\0';

    DBADDR addr;
    if (dbNameToAddr("test:lso.VAL$", &addr) == 0)
    {
        long status = dbPutField(&addr, DBR_CHAR, value, (long) strlen(value) + 1);
        testOk(status == 0, "dbPutField(lso.VAL$, 100-char string) ok");
    }
    else
    {
        testFail("could not resolve test:lso.VAL$");
    }

    char got[256];
    getLongString("test:lso.VAL$", got, sizeof(got));
    testOk(strcmp(got, value) == 0,
           "lso.VAL holds full 100-char string after write (len=%d)",
           (int) strlen(got));
}

static void testWaveformDouble(void)
{
    testDiag("===== DTYP lua: waveform (double array) =====");

    processRecord("test:wf_double");

    double expected[] = {1.5, 2.5, 3.5, 4.5};
    testdbGetArrFieldEqual("test:wf_double", DBF_DOUBLE, 4, 4, expected);
    testdbGetFieldEqual("test:wf_double.NORD", DBF_ULONG, 4);
}

static void testWaveformString(void)
{
    testDiag("===== DTYP lua: waveform (string array) =====");

    processRecord("test:wf_string");

    testdbGetFieldEqual("test:wf_string.NORD", DBF_ULONG, 3);

    char expected[3][MAX_STRING_SIZE];
    memset(expected, 0, sizeof(expected));
    strcpy(expected[0], "alpha");
    strcpy(expected[1], "beta");
    strcpy(expected[2], "gamma");
    testdbGetArrFieldEqual("test:wf_string", DBF_STRING, 3, 3, expected);
}

static void testAaiInt(void)
{
    testDiag("===== DTYP lua: aai (int array) =====");

    processRecord("test:aai_int");

    epicsInt32 expected[] = {10, 20, 30};
    testdbGetArrFieldEqual("test:aai_int", DBF_LONG, 3, 3, expected);
    testdbGetFieldEqual("test:aai_int.NORD", DBF_ULONG, 3);
}

static void testAaoDouble(void)
{
    testDiag("===== DTYP lua: aao (double array out) =====");

    /* Write an array to aao, then process so the Lua callback runs and
     * reads record.VAL via the PV object. Confirm the stored array. */
    double value[] = {5.0, 6.0, 7.0, 8.0};

    /* testdbPutArrFieldOk is not available on base 3.15, so put the
     * array directly via dbPutField (a core API present on all base
     * versions). aao itself is not version-specific. */
    DBADDR addr;
    long status = dbNameToAddr("test:aao_double", &addr);
    testOk(status == 0, "resolved test:aao_double");
    status = dbPutField(&addr, DBR_DOUBLE, value, 4);
    testOk(status == 0, "put 4 doubles to aao");

    processRecord("test:aao_double");

    testdbGetArrFieldEqual("test:aao_double", DBF_DOUBLE, 4, 4, value);
}

static void testUdfClearedOnRead(void)
{
    testDiag("===== DTYP lua: UDF cleared after successful read =====");

    /* ai: UDF should be cleared after successful read */
    processRecord("test:ai");
    testdbGetFieldEqual("test:ai.UDF", DBF_SHORT, 0);

    /* longin */
    processRecord("test:longin");
    testdbGetFieldEqual("test:longin.UDF", DBF_SHORT, 0);

    /* stringin */
    processRecord("test:stringin");
    testdbGetFieldEqual("test:stringin.UDF", DBF_SHORT, 0);
}

static void testReadErrorSetsAlarm(void)
{
    testDiag("===== DTYP lua: read error sets alarm =====");

    processRecord("test:ai_err");

    testdbGetFieldEqual("test:ai_err.SEVR", DBF_SHORT, (int) INVALID_ALARM);
    testdbGetFieldEqual("test:ai_err.STAT", DBF_SHORT, (int) READ_ALARM);
}

/* ---- Stage 5: quoting/paren-aware INP/OUT parsing ---- */

/* Build a struct link carrying an instio string and run parseINPOUT.
 * Uses the luaDtypTest.lua fixture (on LUA_SCRIPT_PATH="..") as the
 * filename so file resolution succeeds and we can inspect the parsed
 * fields. Caller must free via freeProto(). */
static Protocol* parseInst(const char* instr)
{
    struct link l;
    memset(&l, 0, sizeof(l));
    l.type = INST_IO;
    l.value.instio.string = (char*) instr;
    return parseINPOUT(&l);
}

static void freeProto(Protocol* p)
{
    if (!p) { return; }
    /* The state created by parseINPOUT for a file-based proto is owned
     * here; release it. (Named-state protos are not created in these
     * tests.) */
    if (p->state) { luaStateUnref(p->state); }
    delete p;
}

static void testParseBasic(void)
{
    testDiag("===== parseINPOUT: basic fields =====");

    Protocol* p = parseInst("luaDtypTest.lua read_value() SENSOR");
    testOk(p != NULL, "parseINPOUT succeeds for basic form");
    if (p)
    {
        testOk(strcmp(p->function_name, "read_value") == 0,
               "function name = 'read_value', got '%s'", p->function_name);
        testOk(strcmp(p->param_list, "") == 0,
               "empty param list, got '%s'", p->param_list);
        testOk(strcmp(p->portname, "SENSOR") == 0,
               "portname = 'SENSOR', got '%s'", p->portname);
        freeProto(p);
    }
}

static void testParseNoPort(void)
{
    testDiag("===== parseINPOUT: no portname =====");

    Protocol* p = parseInst("luaDtypTest.lua read_value(1, 2, 3)");
    testOk(p != NULL, "parseINPOUT succeeds without portname");
    if (p)
    {
        testOk(strcmp(p->function_name, "read_value") == 0,
               "function name = 'read_value', got '%s'", p->function_name);
        testOk(strcmp(p->param_list, "1, 2, 3") == 0,
               "param list = '1, 2, 3', got '%s'", p->param_list);
        testOk(strcmp(p->portname, "") == 0,
               "portname empty, got '%s'", p->portname);
        freeProto(p);
    }
}

static void testParseQuotedComma(void)
{
    testDiag("===== parseINPOUT: comma inside quoted param =====");

    /* The comma inside "a,b" must NOT split the param list, and must
     * NOT be mistaken for a portname boundary. */
    Protocol* p = parseInst("luaDtypTest.lua read_value(1, \"a,b\", 3)");
    testOk(p != NULL, "parseINPOUT succeeds with quoted comma");
    if (p)
    {
        testOk(strcmp(p->param_list, "1, \"a,b\", 3") == 0,
               "param list preserves quoted comma, got '%s'", p->param_list);
        freeProto(p);
    }
}

static void testParseNestedParens(void)
{
    testDiag("===== parseINPOUT: nested parens in param list =====");

    /* The inner ')' must not prematurely end the param list. */
    Protocol* p = parseInst("luaDtypTest.lua read_value(g(1,2), 3) PORTZ");
    testOk(p != NULL, "parseINPOUT succeeds with nested parens");
    if (p)
    {
        testOk(strcmp(p->param_list, "g(1,2), 3") == 0,
               "param list captures nested parens, got '%s'", p->param_list);
        testOk(strcmp(p->portname, "PORTZ") == 0,
               "portname after outer ) = 'PORTZ', got '%s'", p->portname);
        freeProto(p);
    }
}

static void testParseQuotedSpace(void)
{
    testDiag("===== parseINPOUT: space inside quoted param =====");

    /* A space inside the quoted param must not be read as the portname
     * delimiter (the old find_last_of(' ') bug). */
    Protocol* p = parseInst("luaDtypTest.lua read_value(\"hello world\")");
    testOk(p != NULL, "parseINPOUT succeeds with quoted space");
    if (p)
    {
        testOk(strcmp(p->function_name, "read_value") == 0,
               "function name intact, got '%s'", p->function_name);
        testOk(strcmp(p->param_list, "\"hello world\"") == 0,
               "quoted-space param preserved, got '%s'", p->param_list);
        testOk(strcmp(p->portname, "") == 0,
               "no spurious portname from quoted space, got '%s'", p->portname);
        freeProto(p);
    }
}

static void testParseQuotedSpaceWithPort(void)
{
    testDiag("===== parseINPOUT: quoted space + real portname =====");

    Protocol* p = parseInst("luaDtypTest.lua read_value(\"hello world\") PORTX");
    testOk(p != NULL, "parseINPOUT succeeds");
    if (p)
    {
        testOk(strcmp(p->param_list, "\"hello world\"") == 0,
               "quoted-space param preserved, got '%s'", p->param_list);
        testOk(strcmp(p->portname, "PORTX") == 0,
               "portname after ) = 'PORTX', got '%s'", p->portname);
        freeProto(p);
    }
}

/* --- luaLoadParams tokenization (top-level comma split) --- */

static void testParamsQuotedComma(void)
{
    testDiag("===== luaLoadParams: quoted comma is one token =====");

    lua_State* s = luaCreateState();
    int n = luaLoadParams(s, "1, \"a,b\", 3");
    testOk(n == 3, "3 params for '1, \"a,b\", 3', got %d", n);
    if (n == 3)
    {
        testOk(lua_isstring(s, -2) && strcmp(lua_tostring(s, -2), "a,b") == 0,
               "middle token is 'a,b', got '%s'",
               lua_isstring(s, -2) ? lua_tostring(s, -2) : "(non-string)");
    }
    lua_pop(s, n);
    luaStateUnref(s);
}

static void testParamsNestedParens(void)
{
    testDiag("===== luaLoadParams: nested parens stay one token =====");

    lua_State* s = luaCreateState();
    /* g(1,2) is undefined in strtolua's sandbox, so it falls back to the
     * raw string token -- but crucially it is ONE token, not split. */
    int n = luaLoadParams(s, "g(1,2), 3");
    testOk(n == 2, "2 top-level params for 'g(1,2), 3', got %d", n);
    lua_pop(s, n > 0 ? n : 0);
    luaStateUnref(s);
}

static void testParamsPreservedCounts(void)
{
    testDiag("===== luaLoadParams: legacy empty-token counts preserved =====");

    lua_State* s = luaCreateState();

    int n1 = luaLoadParams(s, "1,,3");  lua_pop(s, n1);
    testOk(n1 == 3, "'1,,3' -> 3, got %d", n1);

    int n2 = luaLoadParams(s, ",2");    lua_pop(s, n2);
    testOk(n2 == 2, "',2' -> 2, got %d", n2);

    int n3 = luaLoadParams(s, "1, ,3"); lua_pop(s, n3);
    testOk(n3 == 3, "'1, ,3' -> 3, got %d", n3);

    int n4 = luaLoadParams(s, "1,");    lua_pop(s, n4);
    testOk(n4 == 1, "'1,' -> 1 (trailing comma, legacy), got %d", n4);

    int n5 = luaLoadParams(s, "");      lua_pop(s, n5);
    testOk(n5 == 0, "'' -> 0, got %d", n5);

    luaStateUnref(s);
}

MAIN(luaDtypTest)
{
    testPlan(0);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    epicsEnvSet("LUA_SCRIPT_PATH", "..");
    testdbReadDatabase("luaDtypTest.db", "..", "P=test:");

#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 0, 0)
    testdbReadDatabase("luaDtypInt64Test.db", "..", "P=test:");
#endif

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
    testMbbiRval();
    testMbbo();
    testMbbiDirect();
    testMbboDirect();
#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 0, 0)
    testInt64in();
    testInt64out();
#endif
    testStringin();
    testStringout();
    testLsi();
    testLso();
    testWaveformDouble();
    testWaveformString();
    testAaiInt();
    testAaoDouble();
    testUdfClearedOnRead();
    testReadErrorSetsAlarm();

    /* Stage 5: quoting/paren-aware INP/OUT parsing */
    testParseBasic();
    testParseNoPort();
    testParseQuotedComma();
    testParseNestedParens();
    testParseQuotedSpace();
    testParseQuotedSpaceWithPort();
    testParamsQuotedComma();
    testParamsNestedParens();
    testParamsPreservedCounts();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
