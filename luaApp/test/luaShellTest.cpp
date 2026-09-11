/*
 * Tests for the Lua shell integration
 *
 * Exercises luaCmd (single command execution),
 * luaCreateState / luaNamedState API, and
 * the iocsh command bridge.
 */

#include <string.h>

#include <dbUnitTest.h>
#include <epicsUnitTest.h>
#include <testMain.h>

#include <dbAccess.h>
#include <errlog.h>
#include <iocsh.h>
#include <envDefs.h>
#include <epicsThread.h>
#include <stdio.h>
#include <unistd.h>

#include "luaEpics.h"
#include "luaShell.h"

extern "C" {
    void luaTest_registerRecordDeviceDriver(struct dbBase *);
}

static void testCreateState(void)
{
    testDiag("===== Lua shell: luaCreateState =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "luaCreateState returns non-NULL");

    if (state)
    {
        /* Verify the state is functional by running simple Lua */
        int status = luaL_dostring(state, "x = 1 + 1");
        testOk(status == 0, "luaL_dostring succeeds in new state");

        lua_getglobal(state, "x");
        int val = lua_tointeger(state, -1);
        testOk(val == 2, "Lua arithmetic works: 1+1 = %d", val);
        lua_pop(state, 1);

        lua_close(state);
    }
}

static void testNamedState(void)
{
    testDiag("===== Lua shell: luaNamedState =====");

    lua_State* state1 = luaNamedState("test_shared");
    testOk(state1 != NULL, "luaNamedState returns non-NULL");

    lua_State* state2 = luaNamedState("test_shared");
    testOk(state1 == state2, "Same name returns same state pointer");

    lua_State* state3 = luaNamedState("test_other");
    testOk(state3 != NULL, "Different name returns non-NULL");
    testOk(state3 != state1, "Different name returns different state");

    /* Verify data persists in named state */
    if (state1)
    {
        luaL_dostring(state1, "shared_var = 99");
        lua_getglobal(state2, "shared_var");
        int val = lua_tointeger(state2, -1);
        testOk(val == 99, "Variable persists in named state: shared_var = %d", val);
        lua_pop(state2, 1);
    }
}

static void testLuaCmd(void)
{
    testDiag("===== Lua shell: luaCmd =====");

    /* luaCmd runs a Lua string through iocsh registration */
    int status = iocshCmd("luaCmd \"epicsEnvSet('LUA_TEST_VAR', 'success')\"");
    testOk(status == 0, "luaCmd via iocshCmd returns 0");
}

/* ---- Stage 2: canonical run/load family ---- */

static void testRunString(void)
{
    testDiag("===== run/load: luaRunString =====");

    /* Single-chunk execution: a local defined earlier in the string is
     * visible later in the same string (whole-chunk scope). Observable
     * effect via epicsEnvSet through the iocsh scope. */
    int status = luaRunString(
        "local x = 40; local y = 2; iocsh.epicsEnvSet('LUA_RS_VAR', tostring(x+y))",
        NULL, NULL);
    testOk(status == 0, "luaRunString returns 0");

    const char* val = getenv("LUA_RS_VAR");
    testOk(val != NULL && strcmp(val, "42") == 0,
           "luaRunString ran as one chunk (locals shared): LUA_RS_VAR='%s'",
           val ? val : "(null)");
}

static void testRunStringMacros(void)
{
    testDiag("===== run/load: luaRunString macros =====");

    int status = luaRunString(
        "iocsh.epicsEnvSet('LUA_RS_MACRO', tostring(P))",
        "P=hello", NULL);
    testOk(status == 0, "luaRunString with macros returns 0");

    const char* val = getenv("LUA_RS_MACRO");
    testOk(val != NULL && strcmp(val, "hello") == 0,
           "macro P delivered to luaRunString: LUA_RS_MACRO='%s'",
           val ? val : "(null)");
}

static void testRunFileSync(void)
{
    testDiag("===== run/load: luaRunFile synchronous =====");

    /* Point the fixture at a temp output file we can read back. */
    char outpath[] = "/tmp/luaRunFileTest_sync.XXXXXX";
    int fd = mkstemp(outpath);
    testOk(fd >= 0, "temp output file created");
    if (fd >= 0)    { close(fd); }

    epicsEnvSet("LUA_RUNFILE_OUT", outpath);

    /* luaRunFileTest.lua is on LUA_SCRIPT_PATH ("..") set in main. */
    int status = luaRunFile("luaRunFileTest.lua", NULL, NULL);
    testOk(status == 0, "luaRunFile (sync) returns 0, got %d", status);

    /* Synchronous: the result must be present immediately on return. */
    char buf[64] = {0};
    FILE* f = fopen(outpath, "r");
    testOk(f != NULL, "output file readable");
    if (f)
    {
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        fclose(f);
    }
    testOk(strcmp(buf, "42") == 0,
           "luaRunFile ran whole file as one chunk: got '%s'", buf);

    remove(outpath);
}

static void testRunFileMacros(void)
{
    testDiag("===== run/load: luaRunFile macros =====");

    char outpath[] = "/tmp/luaRunFileTest_macro.XXXXXX";
    int fd = mkstemp(outpath);
    if (fd >= 0)    { close(fd); }
    epicsEnvSet("LUA_RUNFILE_OUT", outpath);

    int status = luaRunFile("luaRunFileTest.lua", "P=world", NULL);
    testOk(status == 0, "luaRunFile with macros returns 0");

    char buf[64] = {0};
    FILE* f = fopen(outpath, "r");
    if (f)
    {
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = '\0';
        fclose(f);
    }
    testOk(strcmp(buf, "42,world") == 0,
           "macro P delivered to luaRunFile: got '%s'", buf);

    remove(outpath);
}

static void testRunFileNotFound(void)
{
    testDiag("===== run/load: luaRunFile not found =====");

    int status = luaRunFile("no_such_script_xyz.lua", NULL, NULL);
    testOk(status == -1, "luaRunFile returns -1 for missing file, got %d", status);

    int status2 = luaRunFile("", NULL, NULL);
    testOk(status2 == -1, "luaRunFile returns -1 for empty filename, got %d", status2);
}

static void testRunFileAsync(void)
{
    testDiag("===== run/load: luaRunFile async option =====");

    char outpath[] = "/tmp/luaRunFileTest_async.XXXXXX";
    int fd = mkstemp(outpath);
    if (fd >= 0)    { close(fd); }
    remove(outpath);   /* start absent; the async thread creates it */
    epicsEnvSet("LUA_RUNFILE_OUT", outpath);

    /* Fixture busy-waits this many seconds before writing its output,
     * so a synchronous run would block luaRunFile for the full delay
     * (and the file would exist on return), while an async run returns
     * immediately with the file still absent. */
    epicsEnvSet("LUA_RUNFILE_DELAY", "0.3");

    int status = luaRunFile("luaRunFileTest.lua", NULL, "async=true");
    testOk(status == 0, "luaRunFile async returns 0 immediately");

    /* Non-blocking proof: the delayed output must NOT be present yet. */
    FILE* immediate = fopen(outpath, "r");
    testOk(immediate == NULL, "async luaRunFile returned before the file was written");
    if (immediate)    { fclose(immediate); }

    /* Poll for the background thread to produce the file. */
    char buf[64] = {0};
    int found = 0;
    for (int i = 0; i < 100 && !found; i++)   /* up to ~5s */
    {
        epicsThreadSleep(0.05);
        FILE* f = fopen(outpath, "r");
        if (f)
        {
            size_t n = fread(buf, 1, sizeof(buf) - 1, f);
            buf[n] = '\0';
            fclose(f);
            if (n > 0)    { found = 1; }
        }
    }
    testOk(found && strcmp(buf, "42") == 0,
           "async luaRunFile eventually produced result: got '%s'", buf);

    epicsEnvSet("LUA_RUNFILE_DELAY", "");   /* clear for other tests */
    remove(outpath);
}

static void testDeprecatedAliasesForward(void)
{
    testDiag("===== run/load: deprecated aliases still forward =====");

    /* luaCmd -> luaRunString equivalent */
    int s1 = luaCmd("iocsh.epicsEnvSet('LUA_ALIAS_CMD','ok')", NULL);
    testOk(s1 == 0, "luaCmd alias returns 0");
    const char* v1 = getenv("LUA_ALIAS_CMD");
    testOk(v1 != NULL && strcmp(v1, "ok") == 0, "luaCmd alias executed");

    /* luaLoadFile -> luaRunFile (sync) */
    char outpath[] = "/tmp/luaRunFileTest_alias.XXXXXX";
    int fd = mkstemp(outpath);
    if (fd >= 0)    { close(fd); }
    epicsEnvSet("LUA_RUNFILE_OUT", outpath);

    int s2 = luaLoadFile("luaRunFileTest.lua", NULL);
    testOk(s2 == 0, "luaLoadFile alias returns 0");

    char buf[64] = {0};
    FILE* f = fopen(outpath, "r");
    if (f) { size_t n = fread(buf, 1, sizeof(buf)-1, f); buf[n]='\0'; fclose(f); }
    testOk(strcmp(buf, "42") == 0, "luaLoadFile alias ran the file: '%s'", buf);
    remove(outpath);
}

static void testLoadParams(void)
{
    testDiag("===== Lua shell: luaLoadParams =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for param test");

    if (state)
    {
        int count = luaLoadParams(state, "10, 'hello', 3.14");
        testOk(count == 3, "luaLoadParams returns 3 for three params, got %d", count);

        /* Check types and values (they're on the stack in order) */
        testOk(lua_isnumber(state, -3), "First param is a number");
        testOk(lua_isstring(state, -2), "Second param is a string");
        testOk(lua_isnumber(state, -1), "Third param is a number");

        double p1 = lua_tonumber(state, -3);
        const char* p2 = lua_tostring(state, -2);
        double p3 = lua_tonumber(state, -1);

        testOk(p1 == 10.0, "First param value: %g", p1);
        testOk(p2 && strcmp(p2, "hello") == 0, "Second param value: '%s'", p2 ? p2 : "(null)");
        testOk(p3 > 3.13 && p3 < 3.15, "Third param value: %g", p3);

        lua_pop(state, count);
        lua_close(state);
    }
}

static void testLoadParamsEmpty(void)
{
    testDiag("===== Lua shell: luaLoadParams empty/whitespace tokens =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for empty-param test");

    if (state)
    {
        /* Adjacent commas -> empty middle token (previously crashed) */
        int count = luaLoadParams(state, "1,,3");
        testOk(count == 3, "luaLoadParams('1,,3') returns 3, got %d", count);
        testOk(lua_isstring(state, -2), "Empty middle token is a string");
        {
            const char* mid = lua_tostring(state, -2);
            testOk(mid && strcmp(mid, "") == 0, "Empty middle token is empty string");
        }
        lua_pop(state, count);

        /* Leading comma -> empty first token */
        count = luaLoadParams(state, ",2");
        testOk(count == 2, "luaLoadParams(',2') returns 2, got %d", count);
        lua_pop(state, count);

        /* Whitespace-only token (previously crashed) */
        count = luaLoadParams(state, "1, ,3");
        testOk(count == 3, "luaLoadParams('1, ,3') returns 3, got %d", count);
        lua_pop(state, count);

        lua_close(state);
    }
}

static void testLoadMacrosEmptyValue(void)
{
    testDiag("===== Lua shell: luaLoadMacros empty value =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for empty-macro test");

    if (state)
    {
        /* Empty macro value P= previously crashed in strtolua */
        luaLoadMacros(state, "P=,Q=hello");

        int status = luaL_dostring(state, "result = (P == '') and (Q == 'hello')");
        testOk(status == 0, "Script referencing macros runs without error");

        lua_getglobal(state, "result");
        testOk(lua_toboolean(state, -1), "P is empty string and Q is 'hello'");
        lua_pop(state, 1);

        luaPopScope(state);   /* luaLoadMacros pushed a scope */
        lua_close(state);
    }
}

static void testLoadMacrosCoercion(void)
{
    testDiag("===== Lua shell: luaLoadMacros type coercion =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for macro-coercion test");

    if (state)
    {
        /* After the parseDefsToTable refactor, macros must still be
         * type-coerced by strtolua: numbers -> number, true -> boolean,
         * bare words -> string. This guards the shared front-half. */
        luaLoadMacros(state, "N=5,B=true,S=word");

        int status = luaL_dostring(state,
            "result = (type(N)=='number' and N==5) and "
            "(type(B)=='boolean' and B==true) and "
            "(type(S)=='string' and S=='word')");
        testOk(status == 0, "Script referencing coerced macros runs");

        lua_getglobal(state, "result");
        testOk(lua_toboolean(state, -1),
               "N coerced to number, B to boolean, S to string");
        lua_pop(state, 1);

        luaPopScope(state);   /* luaLoadMacros pushed a scope */
        lua_close(state);
    }
}

static void testParseOptions(void)
{
    testDiag("===== Lua shell: luaParseOptions / luaOptionBool =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for options test");

    if (state)
    {
        int top_before = lua_gettop(state);

        /* Options string parses like macros but is NOT installed as a
         * scope -- it is read out by the C caller via luaOptionBool. */
        luaParseOptions(state, "async=true,verbose=false");
        testOk(lua_istable(state, -1), "luaParseOptions leaves a table on the stack");
        testOk(lua_gettop(state) == top_before + 1,
               "luaParseOptions pushes exactly one value");

        int idx = lua_gettop(state);

        testOk(luaOptionBool(state, idx, "async", 0) == 1,
               "async=true reads as true");
        testOk(luaOptionBool(state, idx, "verbose", 1) == 0,
               "verbose=false reads as false");
        testOk(luaOptionBool(state, idx, "missing", 1) == 1,
               "absent key returns the default (1)");
        testOk(luaOptionBool(state, idx, "missing", 0) == 0,
               "absent key returns the default (0)");

        /* luaOptionBool must be stack-neutral (get + pop). */
        testOk(lua_gettop(state) == idx,
               "luaOptionBool leaves the stack unchanged");

        /* Options are NOT injected as globals (no scope pushed). */
        lua_getglobal(state, "async");
        testOk(lua_isnil(state, -1), "options are not injected as globals");
        lua_pop(state, 1);

        lua_pop(state, 1);   /* pop the options table */
        lua_close(state);
    }
}

static void testOptionBoolForms(void)
{
    testDiag("===== Lua shell: luaOptionBool value forms =====");

    lua_State* state = luaCreateState();

    if (state)
    {
        /* String forms that must read as false. */
        luaParseOptions(state, "a=false,b=0,c=no,d=off");
        int idx = lua_gettop(state);
        testOk(luaOptionBool(state, idx, "a", 1) == 0, "\"false\" -> false");
        testOk(luaOptionBool(state, idx, "b", 1) == 0, "\"0\" -> false");
        testOk(luaOptionBool(state, idx, "c", 1) == 0, "\"no\" -> false");
        testOk(luaOptionBool(state, idx, "d", 1) == 0, "\"off\" -> false");
        lua_pop(state, 1);

        /* Truthy forms. */
        luaParseOptions(state, "a=true,b=1,c=yes,d=on");
        idx = lua_gettop(state);
        testOk(luaOptionBool(state, idx, "a", 0) == 1, "\"true\" -> true");
        testOk(luaOptionBool(state, idx, "b", 0) == 1, "1 (number) -> true");
        testOk(luaOptionBool(state, idx, "c", 0) == 1, "\"yes\" -> true");
        testOk(luaOptionBool(state, idx, "d", 0) == 1, "\"on\" -> true");
        lua_pop(state, 1);

        /* NULL option list -> empty table, everything is default. */
        luaParseOptions(state, NULL);
        idx = lua_gettop(state);
        testOk(lua_istable(state, -1), "NULL option list yields a table");
        testOk(luaOptionBool(state, idx, "async", 1) == 1,
               "NULL options: key absent -> default");
        lua_pop(state, 1);

        lua_close(state);
    }
}

static void testNullNamedState(void)
{
    testDiag("===== Lua shell: luaNamedState(NULL) =====");

    lua_State* state = luaNamedState(NULL);
    testOk(state == NULL, "luaNamedState(NULL) returns NULL");
}

static void testRegisterState(void)
{
    testDiag("===== Lua shell: luaRegisterState =====");

    /* Use luaNamedState to create a persistent state (it won't be
     * closed during test shutdown since named states persist) */
    lua_State* state = luaNamedState("test_reg");
    testOk(state != NULL, "Named state created");

    /* luaNamedState registers it, so it should be findable */
    testOk(luaFindNamedState("test_reg") == state, "luaFindNamedState returns the state");

    /* luaRegisterState with a different name */
    luaRegisterState(state, "test_reg_alias");
    testOk(luaFindNamedState("test_reg_alias") == state, "State accessible under alias name");

    /* luaStateIsRegistered should find it */
    testOk(luaStateIsRegistered(state) != 0, "State is registered");

    /* A fresh unregistered state should not be registered */
    lua_State* fresh = luaL_newstate();
    testOk(luaStateIsRegistered(fresh) == 0, "Fresh state is not registered");
    lua_close(fresh);
}

static void testRegisterStateCollision(void)
{
    testDiag("===== Lua shell: luaRegisterState name collision =====");

    lua_State* stateA = luaNamedState("collide_a");
    lua_State* stateB = luaNamedState("collide_b");
    testOk(stateA != NULL && stateB != NULL && stateA != stateB,
           "Two distinct named states created");

    /* Register a fresh name to stateA */
    luaRegisterState(stateA, "collide_name");
    testOk(luaFindNamedState("collide_name") == stateA,
           "Name registered to state A");

    /* Attempt to register the SAME name to a DIFFERENT state (stateB).
     * The C API must reject the overwrite and preserve the A binding. */
    luaRegisterState(stateB, "collide_name");
    testOk(luaFindNamedState("collide_name") == stateA,
           "Collision rejected: name still bound to state A");

    /* Idempotent re-register of the same name to the same state (A):
     * should be a harmless no-op and preserve the binding. */
    luaRegisterState(stateA, "collide_name");
    testOk(luaFindNamedState("collide_name") == stateA,
           "Idempotent re-register keeps binding to state A");
    testOk(luaStateIsRegistered(stateA) != 0, "State A still registered");

    /* Lua path: first registration in state A succeeds. */
    int status = luaL_dostring(stateA, "luaRegisterState('lua_collide')");
    testOk(status == LUA_OK, "Lua luaRegisterState('lua_collide') succeeds in state A");

    /* Lua path: second registration of the same name from state B must
     * raise a Lua error (abort the chunk). */
    status = luaL_dostring(stateB, "luaRegisterState('lua_collide')");
    testOk(status != LUA_OK, "Lua luaRegisterState collision aborts the script");
    testOk(luaFindNamedState("lua_collide") == stateA,
           "Lua collision rejected: name still bound to state A");
}

static void testStateLock(void)
{
    testDiag("===== Lua shell: luaLockState / luaUnlockState =====");

    /* Managed state (created via luaCreateState): lock/unlock works */
    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for lock test");

    if (state)
    {
        luaLockState(state);
        luaUnlockState(state);
        testPass("lock/unlock pair on managed state completes");

        /* Recursive: same thread locks twice, unlocks twice (epicsMutex
         * is recursive). If this hangs, the test harness will time out. */
        luaLockState(state);
        luaLockState(state);
        luaUnlockState(state);
        luaUnlockState(state);
        testPass("recursive lock/unlock on managed state completes");

        luaStateUnref(state);  /* close the state (also frees its lock) */
    }

    /* Unmanaged state (raw luaL_newstate): lock/unlock are safe no-ops */
    lua_State* raw = luaL_newstate();
    testOk(raw != NULL, "Raw state created");

    if (raw)
    {
        luaLockState(raw);
        luaUnlockState(raw);
        testPass("lock/unlock no-op on unmanaged state does not crash");
        lua_close(raw);
    }

    /* NULL is a safe no-op */
    luaLockState(NULL);
    luaUnlockState(NULL);
    testPass("lock/unlock no-op on NULL does not crash");

    /* After closing a state, a fresh managed state still locks cleanly
     * (sanity that teardown removed the registry entry). */
    lua_State* state2 = luaCreateState();
    if (state2)
    {
        luaLockState(state2);
        luaUnlockState(state2);
        testPass("lock/unlock on a fresh state after prior teardown works");
        luaStateUnref(state2);
    }
}

/* A trivial C function to register by name for the ownership test. */
static int l_test_registered_fn(lua_State* state)
{
    lua_pushinteger(state, 4242);
    return 1;
}

static void testRegisterFunctionNameOwnership(void)
{
    testDiag("===== Lua shell: registered name ownership (bug #12) =====");

    /* Register with a name whose backing storage is freed immediately
     * after the call. The registry must own a copy of the name, not the
     * caller's pointer. */
    {
        std::string scoped_name("scoped_reg_fn");
        luaRegisterFunction(scoped_name.c_str(), l_test_registered_fn);
    }  /* scoped_name destroyed here; its c_str() is now invalid */

    /* A state created after registration should have the function bound
     * as a global under the intended name. */
    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created after registration");

    if (state)
    {
        int status = luaL_dostring(state, "result = scoped_reg_fn()");
        testOk(status == 0, "calling scoped_reg_fn() succeeds");

        lua_getglobal(state, "result");
        testOk(lua_tointeger(state, -1) == 4242,
               "registered function returns 4242, got %lld",
               (long long) lua_tointeger(state, -1));
        lua_pop(state, 1);

        luaStateUnref(state);
    }
}

static void testFindNamedStateNotFound(void)
{
    testDiag("===== Lua shell: luaFindNamedState not found =====");

    testOk(luaFindNamedState("nonexistent_state") == NULL,
           "luaFindNamedState returns NULL for unknown name");
}

static void testLuaCmdTableMacros(void)
{
    testDiag("===== Lua shell: luaMacrosFromTable =====");

    /* Use a bare Lua state (not luaCreateState) to avoid
     * any EPICS library interactions during shutdown */
    lua_State* state = luaL_newstate();
    testOk(state != NULL, "State created for macro test");

    if (state)
    {
        lua_newtable(state);
        lua_pushstring(state, "dev1:");
        lua_setfield(state, -2, "P");
        lua_pushstring(state, "sensor");
        lua_setfield(state, -2, "R");

        std::string macros = luaMacrosFromTable(state, lua_gettop(state));
        lua_pop(state, 1);

        /* Order of keys in Lua tables is not guaranteed, check both */
        testOk(macros.find("P=dev1:") != std::string::npos,
               "macros contains P=dev1: : '%s'", macros.c_str());
        testOk(macros.find("R=sensor") != std::string::npos,
               "macros contains R=sensor : '%s'", macros.c_str());

        lua_close(state);
    }
}

/* --- info() function tests --- */

static void testInfoNoArgs(void)
{
    testDiag("===== Lua shell: info() no args =====");

    lua_State* state = luaCreateState();
    testOk(state != NULL, "State created for info test");

    int status = luaL_dostring(state, "info()");
    testOk(status == 0, "info() with no args succeeds");

    lua_close(state);
}

static void testInfoNilInput(void)
{
    testDiag("===== Lua shell: info(nil) =====");

    lua_State* state = luaCreateState();

    int status = luaL_dostring(state, "info(nil)");
    testOk(status == 0, "info(nil) succeeds");

    lua_close(state);
}


MAIN(luaShellTest)
{
    testPlan(0);

    testdbPrepare();

    testdbReadDatabase("luaTest.dbd", NULL, NULL);
    luaTest_registerRecordDeviceDriver(pdbbase);

    /* Script fixtures (luaRunFileTest.lua) are installed in the test
     * directory ".." relative to the O.<arch> run directory. */
    epicsEnvSet("LUA_SCRIPT_PATH", "..");

    eltc(0);
    testIocInitOk();
    eltc(1);

    testCreateState();
    testNamedState();
    testLuaCmd();

    /* Stage 2: canonical run/load family */
    testRunString();
    testRunStringMacros();
    testRunFileSync();
    testRunFileMacros();
    testRunFileNotFound();
    testRunFileAsync();
    testDeprecatedAliasesForward();

    testLoadParams();
    testLoadParamsEmpty();
    testLoadMacrosEmptyValue();
    testLoadMacrosCoercion();
    testParseOptions();
    testOptionBoolForms();
    testNullNamedState();
    testRegisterState();
    testRegisterStateCollision();
    testStateLock();
    testRegisterFunctionNameOwnership();
    testFindNamedStateNotFound();
    testLuaCmdTableMacros();

    /* info() function */
    testInfoNoArgs();
    testInfoNilInput();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
