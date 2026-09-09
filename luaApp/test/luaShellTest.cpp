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

    eltc(0);
    testIocInitOk();
    eltc(1);

    testCreateState();
    testNamedState();
    testLuaCmd();
    testLoadParams();
    testLoadParamsEmpty();
    testLoadMacrosEmptyValue();
    testNullNamedState();
    testRegisterState();
    testRegisterStateCollision();
    testFindNamedStateNotFound();
    testLuaCmdTableMacros();

    /* info() function */
    testInfoNoArgs();
    testInfoNilInput();

    testIocShutdownOk();
    testdbCleanup();

    return testDone();
}
