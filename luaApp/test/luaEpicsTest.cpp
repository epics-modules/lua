/*
 * Tests for the epics library (epics.get, epics.put, epics.pv)
 *
 * Exercises the local PV fast path (dbGetField/dbPutField),
 * PV object creation and field access, return conventions,
 * and the info() function.
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


/* Helper: run a Lua string and check if it succeeded */
static int doLua(lua_State* L, const char* code)
{
	return luaL_dostring(L, code);
}


/* ---- Local PV fast path tests ---- */

static void testGetLocalDouble(void)
{
	testDiag("===== epics library: get local double =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "result = epics.get('etest:test_ai')");
	
	lua_getglobal(L, "result");
	testOk(lua_isnumber(L, -1), "epics.get returns a number");
	testOk(lua_tonumber(L, -1) == 42.5, "Value is 42.5, got %g", lua_tonumber(L, -1));
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testGetLocalString(void)
{
	testDiag("===== epics library: get local string =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "result = epics.get('etest:test_si')");
	
	lua_getglobal(L, "result");
	testOk(lua_isstring(L, -1), "epics.get returns a string");
	testOk(strcmp(lua_tostring(L, -1), "hello") == 0, 
	       "Value is 'hello', got '%s'", lua_tostring(L, -1));
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testGetLocalInteger(void)
{
	testDiag("===== epics library: get local integer =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "result = epics.get('etest:test_li')");
	
	lua_getglobal(L, "result");
	testOk(lua_isinteger(L, -1), "epics.get returns an integer");
	testOk(lua_tointeger(L, -1) == 100, "Value is 100, got %lld", 
	       (long long) lua_tointeger(L, -1));
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testPutLocalDouble(void)
{
	testDiag("===== epics library: put local double =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	int status = doLua(L, "epics.put('etest:test_ao', 99.9)");
	testOk(status == 0, "epics.put succeeds");
	
	testdbGetFieldEqual("etest:test_ao.VAL", DBF_DOUBLE, 99.9);
	
	lua_close(L);
}

static void testPutLocalString(void)
{
	testDiag("===== epics library: put local string =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	int status = doLua(L, "epics.put('etest:test_so', 'goodbye')");
	testOk(status == 0, "epics.put succeeds");
	
	testdbGetFieldEqual("etest:test_so.VAL", DBF_STRING, "goodbye");
	
	lua_close(L);
}

static void testGetLocalWaveform(void)
{
	testDiag("===== epics library: get local waveform =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "result = epics.get('etest:test_dwf')");
	
	lua_getglobal(L, "result");
	testOk(lua_istable(L, -1), "epics.get on waveform returns a table");
	
	if (lua_istable(L, -1))
	{
		lua_geti(L, -1, 1);
		testOk(lua_tonumber(L, -1) == 1.0, "First element is 1.0, got %g", lua_tonumber(L, -1));
		lua_pop(L, 1);
		
		int len = (int) luaL_len(L, -1);
		testOk(len == 5, "Table has 5 elements, got %d", len);
	}
	lua_pop(L, 1);
	
	lua_close(L);
}


/* ---- PV object tests ---- */

static void testPvCreate(void)
{
	testDiag("===== epics library: pv creation =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "pv = epics.pv('etest:test_ai')");
	
	lua_getglobal(L, "pv");
	testOk(lua_isuserdata(L, -1), "epics.pv returns userdata");
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testPvName(void)
{
	testDiag("===== epics library: pv.name =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "pv = epics.pv('etest:test_ai')");
	doLua(L, "result = pv.name");
	
	lua_getglobal(L, "result");
	testOk(lua_isstring(L, -1), "pv.name is a string");
	testOk(strcmp(lua_tostring(L, -1), "etest:test_ai") == 0,
	       "pv.name is 'etest:test_ai', got '%s'", lua_tostring(L, -1));
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testPvReadField(void)
{
	testDiag("===== epics library: pv field read =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "pv = epics.pv('etest:test_ai')");
	doLua(L, "result = pv.VAL");
	
	lua_getglobal(L, "result");
	testOk(lua_isnumber(L, -1), "pv.VAL is a number");
	testOk(lua_tonumber(L, -1) == 42.5, "pv.VAL is 42.5, got %g", lua_tonumber(L, -1));
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testPvWriteField(void)
{
	testDiag("===== epics library: pv field write =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "pv = epics.pv('etest:test_ao')");
	doLua(L, "pv.VAL = 77.7");
	
	testdbGetFieldEqual("etest:test_ao.VAL", DBF_DOUBLE, 77.7);
	
	lua_close(L);
}

static void testPvTostring(void)
{
	testDiag("===== epics library: pv tostring =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "pv = epics.pv('etest:test_ai')");
	doLua(L, "result = tostring(pv)");
	
	lua_getglobal(L, "result");
	testOk(strcmp(lua_tostring(L, -1), "etest:test_ai") == 0,
	       "tostring(pv) is 'etest:test_ai', got '%s'", lua_tostring(L, -1));
	lua_pop(L, 1);
	
	lua_close(L);
}


/* ---- Return convention tests ---- */

static void testPutReturnsNothing(void)
{
	testDiag("===== epics library: put returns nothing on success =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "result = epics.put('etest:test_ao', 1.0)");
	
	lua_getglobal(L, "result");
	testOk(lua_isnil(L, -1), "epics.put returns nothing (nil in global)");
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testPutReturnsErrorString(void)
{
	testDiag("===== epics library: put returns error on failure =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "result = epics.put('nonexistent:pv', 1.0)");
	
	lua_getglobal(L, "result");
	testOk(lua_isstring(L, -1), "epics.put returns error string on failure");
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testGetReturnsNilOnError(void)
{
	testDiag("===== epics library: get returns nil+error on failure =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "val, err = epics.get('nonexistent:pv')");
	
	lua_getglobal(L, "val");
	testOk(lua_isnil(L, -1), "epics.get returns nil on failure");
	lua_pop(L, 1);
	
	lua_getglobal(L, "err");
	testOk(lua_isstring(L, -1), "epics.get returns error string as second value");
	lua_pop(L, 1);
	
	lua_close(L);
}


/* ---- info() function tests ---- */

static void testInfoNoArgs(void)
{
	testDiag("===== epics library: info() no args =====");

	lua_State* L = luaCreateState();
	
	int status = doLua(L, "info()");
	testOk(status == 0, "info() with no args doesn't crash");
	
	lua_close(L);
}

static void testInfoNil(void)
{
	testDiag("===== epics library: info(nil) =====");

	lua_State* L = luaCreateState();
	
	int status = doLua(L, "info(nil)");
	testOk(status == 0, "info(nil) doesn't crash");
	
	lua_close(L);
}

static void testInfoLibrary(void)
{
	testDiag("===== epics library: info(epics) =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	int status = doLua(L, "info(epics)");
	testOk(status == 0, "info(epics) doesn't crash");
	
	/* Verify _doc table exists */
	doLua(L, "result = epics._doc ~= nil");
	lua_getglobal(L, "result");
	testOk(lua_toboolean(L, -1), "epics._doc table exists");
	lua_pop(L, 1);
	
	lua_close(L);
}

static void testInfoPvObject(void)
{
	testDiag("===== epics library: info(pv_object) =====");

	lua_State* L = luaCreateState();
	
	doLua(L, "epics = require('epics')");
	doLua(L, "pv = epics.pv('etest:test_ai')");
	int status = doLua(L, "info(pv)");
	testOk(status == 0, "info(pv) doesn't crash");
	
	lua_close(L);
}


MAIN(luaEpicsTest)
{
	testPlan(0);

	testdbPrepare();

	testdbReadDatabase("luaTest.dbd", NULL, NULL);
	luaTest_registerRecordDeviceDriver(pdbbase);

	testdbReadDatabase("luaEpicsTest.db", "..", "P=etest:");

	eltc(0);
	testIocInitOk();
	eltc(1);

	/* Initialize waveform test data (replaces JSON INP syntax for 3.15 compat) */
	{
		DBADDR addr;
		double dvals[] = {1.0, 2.0, 3.0, 4.0, 5.0};
		dbNameToAddr("etest:test_dwf", &addr);
		dbPutField(&addr, DBF_DOUBLE, dvals, 5);

		char cvals[] = {72, 101, 108, 108, 111};  /* "Hello" */
		dbNameToAddr("etest:test_cwf", &addr);
		dbPutField(&addr, DBF_CHAR, cvals, 5);
	}

	/* Local PV fast path */
	testGetLocalDouble();
	testGetLocalString();
	testGetLocalInteger();
	testPutLocalDouble();
	testPutLocalString();
	testGetLocalWaveform();

	/* PV object */
	testPvCreate();
	testPvName();
	testPvReadField();
	testPvWriteField();
	testPvTostring();

	/* Return conventions */
	testPutReturnsNothing();
	testPutReturnsErrorString();
	testGetReturnsNilOnError();

	/* info() function */
	testInfoNoArgs();
	testInfoNil();
	testInfoLibrary();
	testInfoPvObject();

	testIocShutdownOk();
	testdbCleanup();

	return testDone();
}
