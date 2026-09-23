/*
 * Tests for the bytestream library (pure-Lua module).
 *
 * The heavy lifting lives in the Lua fixture bytestreamTest.lua, which
 * exercises bytestream.match / bytestream.format, the * (ignore) flag on
 * both read and write, and custom format specifiers registered via
 * bytestream.add_format (both using the public bytestream.reader /
 * bytestream.writer helpers and hand-rolled from scratch).
 *
 * The fixture returns a list of { ok = bool, msg = string } records.
 * This harness runs the fixture in a fully-wired Lua state (lpeg is
 * statically linked and registered by luaCreateState) and turns each
 * record into an epicsUnitTest assertion. Keeping the assertions in Lua
 * avoids duplicating the expected values in C++ and keeps the test
 * readable next to the library it exercises.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>
#include <dbAccess.h>
#include <envDefs.h>

#include "luaEpics.h"

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

/* Run the fixture and return the number of result records reported (so we
 * can testPlan an exact count), or -1 on a load/run failure. */
static int countResults(lua_State* state)
{
    lua_getglobal(state, "results");
    if (!lua_istable(state, -1))
    {
        lua_pop(state, 1);
        return -1;
    }
    int n = (int) lua_rawlen(state, -1);
    lua_pop(state, 1);
    return n;
}

MAIN(bytestreamTest)
{
    /* Load the test DBD so the library registrars (in particular the one
     * that registers lpeg via require) fire before we create a state. */
    testdbPrepare();
    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    /* Make both the test fixtures (..) and the library source dir
     * (../../src/libs, relative to the O.<arch> run dir) reachable by
     * require(). A state created after this gets it via rebuildPaths. */
    epicsEnvSet("LUA_SCRIPT_PATH", "..:../../src/libs");

    lua_State* state = luaCreateState();
    if (!state)
    {
        testPlan(1);
        testOk(0, "luaCreateState returned NULL");
        return testDone();
    }

    std::string path = luaLocateFile(std::string("bytestreamTest.lua"));
    if (path.empty())
    {
        testPlan(1);
        testOk(0, "bytestreamTest.lua not found on LUA_SCRIPT_PATH");
        luaStateUnref(state);
        return testDone();
    }

    int status = luaL_dofile(state, path.c_str());
    if (status != LUA_OK)
    {
        const char* err = lua_tostring(state, -1);
        testPlan(1);
        testOk(0, "bytestreamTest.lua failed to run: %s", err ? err : "(unknown)");
        luaStateUnref(state);
        return testDone();
    }

    int n = countResults(state);
    if (n < 0)
    {
        testPlan(1);
        testOk(0, "bytestreamTest.lua did not produce a 'results' table");
        luaStateUnref(state);
        return testDone();
    }

    testPlan(n);

    lua_getglobal(state, "results");
    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(state, -1, i);           /* results[i] */

        lua_getfield(state, -1, "ok");
        int ok = lua_toboolean(state, -1);
        lua_pop(state, 1);

        lua_getfield(state, -1, "msg");
        const char* msg = lua_tostring(state, -1);
        testOk(ok, "%s", msg ? msg : "(no message)");
        lua_pop(state, 1);                   /* msg */

        lua_pop(state, 1);                   /* results[i] */
    }
    lua_pop(state, 1);                       /* results */

    luaStateUnref(state);

    testdbCleanup();

    return testDone();
}
