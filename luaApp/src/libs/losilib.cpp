#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsStdio.h>
#include "luaEpics.h"

static int l_setStdout(lua_State* state)
{
	lua_settop(state, 1);

	const char* f_out = luaL_checkstring(state, 1);

	FILE* redirect = fopen(f_out, "w");

	if (redirect == NULL)
	{
		return luaL_error(state, "Error in opening file (%s)\n", f_out);
	}

	lua_getfield(state, LUA_REGISTRYINDEX, "OSI_REDIRECTS_STACK");
	lua_len(state, -1);
	int length = lua_tointeger(state, -1);
	lua_pop(state, 1);

	// Push current thread output location to the end of the stack
	FILE* current = epicsGetThreadStdout();
	lua_pushlightuserdata(state, (void*) current);
	lua_seti(state, -2, length + 1);

	lua_pop(state, 1);

	epicsSetThreadStdout(redirect);

	return 0;
}

static int l_resetStdout(lua_State* state)
{
	lua_getfield(state, LUA_REGISTRYINDEX, "OSI_REDIRECTS_STACK");
	lua_len(state, -1);
	int length = lua_tointeger(state, -1);
	lua_pop(state, 1);

	if (length == 0)
	{
		lua_pop(state, 1);
		return 0;
	}

	FILE* current = epicsGetThreadStdout();

	fclose(current);

	lua_geti(state, -1, length);
	FILE* previous = (FILE*) lua_touserdata(state, -1);

	epicsSetThreadStdout(previous);

	lua_pop(state, 1);
	lua_pushnil(state);
	lua_seti(state, -2, length);
	lua_pop(state, 1);

	return 0;
}

static void l_osisleep(double seconds)    { epicsThreadSleep(seconds); }

int luaopen_osi (lua_State *L)
{
	LuaModule osimod (L, "osi");
	osimod.fun("startRedirectOut", l_setStdout);
	osimod.fun("endRedirectOut", l_resetStdout);
	osimod.fun("sleep", l_osisleep);
	
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "OSI_REDIRECTS_STACK");

	lua_getglobal(L, "osi");
	
	return 1;
}

static void libosiRegister(void)    { luaRegisterLibrary("osi", luaopen_osi); }

extern "C"
{
	epicsExportRegistrar(libosiRegister);
}
