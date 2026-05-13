/*
 * Test harness runner for all Lua module tests.
 * Used for embedded targets (vxWorks, RTEMS) and
 * can also batch-run all tests on host.
 */

#include <epicsUnitTest.h>
#include <epicsExit.h>

int luaScriptTest(void);
int luaDtypTest(void);
int luaPortDriverTest(void);
int luaShellTest(void);

void epicsRunLuaTests(void)
{
    testHarness();

    runTest(luaScriptTest);
    runTest(luaDtypTest);
    runTest(luaPortDriverTest);
    runTest(luaShellTest);

    epicsExit(0);
}
