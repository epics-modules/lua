/*
 * Tests for the event library (event.flag)
 *
 * Exercises event flag creation (anonymous and named),
 * set/clear/test/testAndClear operations, wait with
 * timeout, and cross-state named flag sharing.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>

#include "luaEpics.h"

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}


static int doLua(lua_State* L, const char* code)
{
	return luaL_dostring(L, code);
}


static void testFlagInitialState(void)
{
	testDiag("===== event flag: initial state =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");
	doLua(L, "flag = event.flag()");
	doLua(L, "result = flag:test()");

	lua_getglobal(L, "result");
	testOk(lua_isboolean(L, -1), "flag:test() returns a boolean");
	testOk(!lua_toboolean(L, -1), "New flag is initially false");
	lua_pop(L, 1);

	lua_close(L);
}


static void testFlagSetAndTest(void)
{
	testDiag("===== event flag: set and test =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");
	doLua(L, "flag = event.flag()");
	doLua(L, "flag:set()");
	doLua(L, "result = flag:test()");

	lua_getglobal(L, "result");
	testOk(lua_toboolean(L, -1), "flag:test() returns true after set()");
	lua_pop(L, 1);

	/* Set again (idempotent) */
	doLua(L, "flag:set()");
	doLua(L, "result2 = flag:test()");

	lua_getglobal(L, "result2");
	testOk(lua_toboolean(L, -1), "flag:test() still true after double set()");
	lua_pop(L, 1);

	lua_close(L);
}


static void testFlagClear(void)
{
	testDiag("===== event flag: clear =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");
	doLua(L, "flag = event.flag()");
	doLua(L, "flag:set()");
	doLua(L, "flag:clear()");
	doLua(L, "result = flag:test()");

	lua_getglobal(L, "result");
	testOk(!lua_toboolean(L, -1), "flag:test() returns false after clear()");
	lua_pop(L, 1);

	lua_close(L);
}


static void testFlagTestAndClear(void)
{
	testDiag("===== event flag: testAndClear =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");
	doLua(L, "flag = event.flag()");
	doLua(L, "flag:set()");
	doLua(L, "was_set = flag:testAndClear()");
	doLua(L, "now_set = flag:test()");

	lua_getglobal(L, "was_set");
	testOk(lua_toboolean(L, -1), "testAndClear() returns true when flag was set");
	lua_pop(L, 1);

	lua_getglobal(L, "now_set");
	testOk(!lua_toboolean(L, -1), "flag:test() returns false after testAndClear()");
	lua_pop(L, 1);

	/* testAndClear on already-cleared flag */
	doLua(L, "was_set2 = flag:testAndClear()");

	lua_getglobal(L, "was_set2");
	testOk(!lua_toboolean(L, -1), "testAndClear() returns false when flag was not set");
	lua_pop(L, 1);

	lua_close(L);
}


static void testFlagWaitTimeout(void)
{
	testDiag("===== event flag: wait with timeout =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");
	doLua(L, "flag = event.flag()");

	/* Wait on unset flag -- should timeout */
	doLua(L, "result = flag:wait(0.1)");

	lua_getglobal(L, "result");
	testOk(!lua_toboolean(L, -1), "wait(0.1) returns false on timeout");
	lua_pop(L, 1);

	/* Set flag, then wait -- should return immediately */
	doLua(L, "flag:set()");
	doLua(L, "result2 = flag:wait(0.1)");

	lua_getglobal(L, "result2");
	testOk(lua_toboolean(L, -1), "wait(0.1) returns true when flag is set");
	lua_pop(L, 1);

	/* Wait with zero timeout (instant check) */
	doLua(L, "flag:clear()");
	doLua(L, "result3 = flag:wait(0)");

	lua_getglobal(L, "result3");
	testOk(!lua_toboolean(L, -1), "wait(0) returns false when flag is not set");
	lua_pop(L, 1);

	lua_close(L);
}


static void testNamedFlagSharing(void)
{
	testDiag("===== event flag: named flag sharing =====");

	lua_State* L1 = luaCreateState();
	lua_State* L2 = luaCreateState();

	doLua(L1, "event = require('event')");
	doLua(L2, "event = require('event')");

	/* Create named flag in both states */
	doLua(L1, "flag1 = event.flag('testSharedFlag')");
	doLua(L2, "flag2 = event.flag('testSharedFlag')");

	/* Set in state 1, test in state 2 */
	doLua(L1, "flag1:set()");
	doLua(L2, "result = flag2:test()");

	lua_getglobal(L2, "result");
	testOk(lua_toboolean(L2, -1), "Named flag set in state 1 is visible in state 2");
	lua_pop(L2, 1);

	/* Clear in state 2, test in state 1 */
	doLua(L2, "flag2:clear()");
	doLua(L1, "result = flag1:test()");

	lua_getglobal(L1, "result");
	testOk(!lua_toboolean(L1, -1), "Named flag cleared in state 2 is cleared in state 1");
	lua_pop(L1, 1);

	lua_close(L1);
	lua_close(L2);
}


static void testAnonymousFlagsIndependent(void)
{
	testDiag("===== event flag: anonymous flags are independent =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");
	doLua(L, "a = event.flag()");
	doLua(L, "b = event.flag()");
	doLua(L, "a:set()");
	doLua(L, "b_result = b:test()");

	lua_getglobal(L, "b_result");
	testOk(!lua_toboolean(L, -1), "Setting anonymous flag a does not affect flag b");
	lua_pop(L, 1);

	lua_close(L);
}


static void testFlagTostring(void)
{
	testDiag("===== event flag: tostring =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");

	doLua(L, "anon = tostring(event.flag())");
	lua_getglobal(L, "anon");
	testOk(strcmp(lua_tostring(L, -1), "event.flag: (anonymous)") == 0,
	       "Anonymous flag tostring: '%s'", lua_tostring(L, -1));
	lua_pop(L, 1);

	doLua(L, "named = tostring(event.flag('myTestFlag'))");
	lua_getglobal(L, "named");
	testOk(strcmp(lua_tostring(L, -1), "event.flag: myTestFlag") == 0,
	       "Named flag tostring: '%s'", lua_tostring(L, -1));
	lua_pop(L, 1);

	lua_close(L);
}


static void testFlagInfo(void)
{
	testDiag("===== event flag: info() support =====");

	lua_State* L = luaCreateState();

	doLua(L, "event = require('event')");

	/* info(event) should not crash */
	int status = doLua(L, "info(event)");
	testOk(status == 0, "info(event) succeeds");

	/* info(flag) should not crash */
	doLua(L, "flag = event.flag()");
	status = doLua(L, "info(flag)");
	testOk(status == 0, "info(flag) succeeds");

	lua_close(L);
}


MAIN(luaEventTest)
{
	testPlan(0);

	testdbPrepare();

	testdbReadDatabase("luaTest.dbd", NULL, NULL);
	luaTest_registerRecordDeviceDriver(pdbbase);

	eltc(0);
	testIocInitOk();
	eltc(1);

	testFlagInitialState();
	testFlagSetAndTest();
	testFlagClear();
	testFlagTestAndClear();
	testFlagWaitTimeout();
	testNamedFlagSharing();
	testAnonymousFlagsIndependent();
	testFlagTostring();
	testFlagInfo();

	testIocShutdownOk();
	testdbCleanup();

	return testDone();
}
