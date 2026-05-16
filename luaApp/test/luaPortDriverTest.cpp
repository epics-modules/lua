/*
 * Tests for the luaPortDriver
 *
 * Tests both the old-style script-based API and the new
 * asyn.driver.new() API.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <envDefs.h>

#include "luaEpics.h"
#include "luaPortDriver.h"

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

/* --- Old API tests (script-based) --- */

static void testReadInt32(void)
{
    testDiag("===== luaPortDriver (old API): readInt32 =====");

    testdbPutFieldOk("test:readback.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:readback.VAL", DBF_DOUBLE, 0.0);
}

static void testWriteInt32(void)
{
    testDiag("===== luaPortDriver (old API): writeInt32 =====");

    testdbPutFieldOk("test:setpoint.VAL", DBF_DOUBLE, 42.0);

    testdbPutFieldOk("test:readback.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:readback.VAL", DBF_DOUBLE, 42.0);
}

static void testReadFloat64(void)
{
    testDiag("===== luaPortDriver (old API): readFloat64 =====");

    testdbPutFieldOk("test:float_rb.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("test:float_rb.VAL", DBF_DOUBLE, 3.14159);
}

/* --- asyn.client tests --- */

static void testClientApi(void)
{
    testDiag("===== asyn.client: API structure =====");

    /*
     * Test the asyn.client table structure and callable interface.
     * We can't create actual asynOctetClient objects in unit tests
     * because they require real asyn communication ports (serial, TCP)
     * and would block on synchronous connect/disconnect. asynPortDriver
     * ports don't support the asynOctet synchronous interface.
     */

    lua_State* state = luaCreateState();

    int status = luaL_dostring(state, "asyn = require('asyn')");
    testOk(status == 0, "require('asyn') succeeded");

    /* Verify asyn.client is a table */
    status = luaL_dostring(state, "ct = type(asyn.client)");
    lua_getglobal(state, "ct");
    testOk(lua_isstring(state, -1) && strcmp(lua_tostring(state, -1), "table") == 0,
           "asyn.client is a table");
    lua_pop(state, 1);

    /* Verify asyn.client.find is a function */
    status = luaL_dostring(state, "cf = type(asyn.client.find)");
    lua_getglobal(state, "cf");
    testOk(lua_isstring(state, -1) && strcmp(lua_tostring(state, -1), "function") == 0,
           "asyn.client.find is a function");
    lua_pop(state, 1);

    /* Verify asyn.client is callable (has __call metatable) */
    status = luaL_dostring(state, "mt = getmetatable(asyn.client);"
        "has_call = mt and type(mt.__call) == 'function'");
    lua_getglobal(state, "has_call");
    testOk(lua_toboolean(state, -1), "asyn.client has __call metamethod");
    lua_pop(state, 1);

    lua_close(state);
}

/* --- New API tests (asyn.driver.new) --- */

static void testNewApiDefaults(void)
{
    testDiag("===== asyn.driver.new: default values =====");

    /* COMPUTED param was set to 3.14159 as default */
    testdbPutFieldOk("new:computed.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("new:computed.VAL", DBF_DOUBLE, 3.14159);
}

static void testNewApiWrite(void)
{
    testDiag("===== asyn.driver.new: write + read =====");

    /* Write a setpoint value */
    testdbPutFieldOk("new:setpoint.VAL", DBF_DOUBLE, 10.0);

    /* Read back: read callback returns SETPOINT.value * self.scale (2.0) */
    testdbPutFieldOk("new:readback.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("new:readback.VAL", DBF_DOUBLE, 20.0);
}

static void testNewApiInitState(void)
{
    testDiag("===== asyn.driver.new: init function state =====");

    /* Write setpoint = 5, read should return 5 * scale(2.0) = 10 */
    testdbPutFieldOk("new:setpoint.VAL", DBF_DOUBLE, 5.0);
    testdbPutFieldOk("new:readback.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("new:readback.VAL", DBF_DOUBLE, 10.0);
}

MAIN(luaPortDriverTest)
{
    testPlan(0);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    epicsEnvSet("LUA_SCRIPT_PATH", "..");

    /* Old API: create driver from script */
    new luaPortDriver("TESTPORT", "luaPortDriverTest.lua", NULL);
    testdbReadDatabase("luaPortDriverTest.db", "..", "P=test:,PORT=TESTPORT");

    /* New API: create driver from Lua code */
    {
        lua_State* newState = luaCreateState();
        lua_pushstring(newState, "NEWPORT");
        lua_setglobal(newState, "PORT");
        if (luaLoadScript(newState, "luaNewDriverTest.lua"))
        {
            testAbort("Failed to load luaNewDriverTest.lua");
        }
    }
    testdbReadDatabase("luaNewDriverTest.db", "..", "P=new:,PORT=NEWPORT");

    eltc(0);
    testIocInitOk();
    eltc(1);

    /* Old API tests */
    testReadInt32();
    testWriteInt32();
    testReadFloat64();

    /* New API tests */
    testNewApiDefaults();
    testNewApiWrite();
    testNewApiInitState();

    /* asyn.client tests */
    testClientApi();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
