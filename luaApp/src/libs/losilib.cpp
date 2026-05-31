#include <epicsThread.h>
#include <epicsTime.h>
#include <epicsVersion.h>
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

static int l_osisleep(lua_State* state)
{
	double seconds = lua_tonumber(state, 1);
	epicsThreadSleep(seconds);
	return 0;
}


/*
 * osi.monotonic() -- monotonic time in seconds as a double.
 * Suitable for measuring intervals. Not affected by system clock
 * adjustments.
 */
static int l_osimonotonic(lua_State* state)
{
#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 0, 0)
	epicsUInt64 ns = epicsMonotonicGet();
	lua_pushnumber(state, ns * 1e-9);
#else
	/* Fallback for base 3.15: use epicsTimeGetCurrent */
	epicsTimeStamp ts;
	epicsTimeGetCurrent(&ts);
	lua_pushnumber(state, ts.secPastEpoch + ts.nsec * 1e-9);
#endif
	return 1;
}


/*
 * osi.time() -- current EPICS time as a double.
 * Returns seconds past the EPICS epoch (January 1, 1990 00:00:00 UTC)
 * with sub-second precision.
 */
static int l_ositime(lua_State* state)
{
	epicsTimeStamp ts;
	epicsTimeGetCurrent(&ts);
	lua_pushnumber(state, ts.secPastEpoch + ts.nsec * 1e-9);
	return 1;
}


/*
 * osi.timestr([time] [, format]) -- format a timestamp as a string.
 *
 * Arguments are disambiguated by type:
 *   osi.timestr()                  -- current time, default format
 *   osi.timestr("format")          -- current time, custom format
 *   osi.timestr(time)              -- specific time, default format
 *   osi.timestr(time, "format")    -- specific time, custom format
 *
 * The default format is "%Y-%m-%d %H:%M:%S.%03f" (millisecond precision).
 * The %0Nf specifier is an EPICS extension for fractional seconds with
 * N digits of precision.
 */
static int l_ositimestr(lua_State* state)
{
	epicsTimeStamp ts;
	const char* fmt = "%Y-%m-%d %H:%M:%S.%03f";

	if (lua_isnumber(state, 1))
	{
		/* First arg is a timestamp */
		double t = lua_tonumber(state, 1);
		ts.secPastEpoch = (epicsUInt32) t;
		ts.nsec = (epicsUInt32) ((t - ts.secPastEpoch) * 1e9);

		if (lua_isstring(state, 2))
		{
			fmt = lua_tostring(state, 2);
		}
	}
	else if (lua_isstring(state, 1))
	{
		/* First arg is a format string, use current time */
		fmt = lua_tostring(state, 1);
		epicsTimeGetCurrent(&ts);
	}
	else
	{
		/* No args, use current time and default format */
		epicsTimeGetCurrent(&ts);
	}

	char buf[64];
	epicsTimeToStrftime(buf, sizeof(buf), fmt, &ts);
	lua_pushstring(state, buf);
	return 1;
}


int luaopen_osi (lua_State *L)
{
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "OSI_REDIRECTS_STACK");

	static const luaL_Reg mylib[] = {
		{"startRedirectOut", l_setStdout},
		{"endRedirectOut",   l_resetStdout},
		{"sleep",            l_osisleep},
		{"monotonic",        l_osimonotonic},
		{"time",             l_ositime},
		{"timestr",          l_ositimestr},
		{NULL, NULL}
	};

	luaL_newlib(L, mylib);

	/* Documentation for info(osi) */
	lua_newtable(L);
	lua_pushstring(L, ".sleep(seconds)"); lua_rawseti(L, -2, 1);
	lua_pushstring(L, ".monotonic()                -- monotonic time in seconds"); lua_rawseti(L, -2, 2);
	lua_pushstring(L, ".time()                     -- EPICS epoch time in seconds"); lua_rawseti(L, -2, 3);
	lua_pushstring(L, ".timestr([time] [, format]) -- format time as string"); lua_rawseti(L, -2, 4);
	lua_pushstring(L, ".startRedirectOut(filename)"); lua_rawseti(L, -2, 5);
	lua_pushstring(L, ".endRedirectOut()"); lua_rawseti(L, -2, 6);
	lua_setfield(L, -2, "_doc");

	return 1;
}

static void libosiRegister(void)    { luaRegisterLibrary("osi", luaopen_osi); }

extern "C"
{
	epicsExportRegistrar(libosiRegister);
}
