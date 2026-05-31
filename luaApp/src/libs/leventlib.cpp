/*
 * leventlib.cpp -- Event synchronization library for Lua EPICS
 *
 * Provides event flags for inter-thread signaling. Anonymous flags
 * are local to the creating Lua state and garbage-collected. Named
 * flags are shared across all Lua states by name and persist for the
 * lifetime of the IOC.
 */

#include <string>
#include <string.h>
#include <map>

#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsGuard.h>
#include <epicsTime.h>
#include <epicsExport.h>

#include "luaEpics.h"


/*
 * =========================================================================
 * Event flag: a boolean flag with mutex protection and an epicsEvent
 * for wakeup signaling (used by wait()).
 * =========================================================================
 */

struct lua_event_flag
{
	bool        value;
	epicsMutex  mutex;
	epicsEvent  signal;

	lua_event_flag() : value(false) {}
};


/* Global registry for named flags */
static std::map<std::string, lua_event_flag*> named_flags;
static epicsMutex namedFlagsMutex;


/*
 * Lua userdata: holds a pointer to the flag and whether this
 * userdata owns it (anonymous) or references a shared one (named).
 */
typedef struct
{
	lua_event_flag* flag;
	bool            owned;
	char            name[64];
} EventFlagUD;

static const char* EVENT_FLAG_META = "lua_event_flag";

static EventFlagUD* check_flag(lua_State* state, int idx)
{
	EventFlagUD* ud = (EventFlagUD*) luaL_checkudata(state, idx, EVENT_FLAG_META);
	if (!ud || !ud->flag)    { luaL_error(state, "Invalid event flag"); }
	return ud;
}


/*
 * =========================================================================
 * Flag methods
 * =========================================================================
 */

static int l_flag_set(lua_State* state)
{
	EventFlagUD* ud = check_flag(state, 1);

	{
		epicsGuard<epicsMutex> guard(ud->flag->mutex);
		ud->flag->value = true;
	}

	ud->flag->signal.signal();

	return 0;
}

static int l_flag_clear(lua_State* state)
{
	EventFlagUD* ud = check_flag(state, 1);

	epicsGuard<epicsMutex> guard(ud->flag->mutex);
	ud->flag->value = false;

	return 0;
}

static int l_flag_test(lua_State* state)
{
	EventFlagUD* ud = check_flag(state, 1);

	epicsGuard<epicsMutex> guard(ud->flag->mutex);
	lua_pushboolean(state, ud->flag->value);

	return 1;
}

static int l_flag_testAndClear(lua_State* state)
{
	EventFlagUD* ud = check_flag(state, 1);

	epicsGuard<epicsMutex> guard(ud->flag->mutex);
	bool was_set = ud->flag->value;
	ud->flag->value = false;
	lua_pushboolean(state, was_set);

	return 1;
}

static int l_flag_wait(lua_State* state)
{
	EventFlagUD* ud = check_flag(state, 1);
	double timeout = luaL_checknumber(state, 2);

	/*
	 * Poll loop: check the flag, then wait on the signal.
	 * The signal is triggered by set(), so we wake up promptly.
	 * A negative timeout means wait indefinitely.
	 */

	/* Quick check first */
	{
		epicsGuard<epicsMutex> guard(ud->flag->mutex);
		if (ud->flag->value)
		{
			lua_pushboolean(state, 1);
			return 1;
		}
	}

	if (timeout == 0.0)
	{
		lua_pushboolean(state, 0);
		return 1;
	}

	if (timeout < 0.0)
	{
		/* Infinite wait */
		while (1)
		{
			ud->flag->signal.wait();

			epicsGuard<epicsMutex> guard(ud->flag->mutex);
			if (ud->flag->value)
			{
				lua_pushboolean(state, 1);
				return 1;
			}
		}
	}

	/* Timed wait: loop with remaining time */
	epicsTime deadline = epicsTime::getCurrent() + timeout;

	while (1)
	{
		double remaining = deadline - epicsTime::getCurrent();

		if (remaining <= 0.0)
		{
			lua_pushboolean(state, 0);
			return 1;
		}

		ud->flag->signal.wait(remaining);

		epicsGuard<epicsMutex> guard(ud->flag->mutex);
		if (ud->flag->value)
		{
			lua_pushboolean(state, 1);
			return 1;
		}
	}
}


/*
 * =========================================================================
 * Metamethods
 * =========================================================================
 */

static int l_flag_gc(lua_State* state)
{
	EventFlagUD* ud = (EventFlagUD*) lua_touserdata(state, 1);

	if (ud && ud->flag && ud->owned)
	{
		delete ud->flag;
		ud->flag = NULL;
	}

	return 0;
}

static int l_flag_tostring(lua_State* state)
{
	EventFlagUD* ud = (EventFlagUD*) luaL_checkudata(state, 1, EVENT_FLAG_META);

	if (ud->name[0])
	{
		lua_pushfstring(state, "event.flag: %s", ud->name);
	}
	else
	{
		lua_pushliteral(state, "event.flag: (anonymous)");
	}

	return 1;
}

static int l_flag_index(lua_State* state)
{
	check_flag(state, 1);
	const char* key = luaL_checkstring(state, 2);

	if (strcmp(key, "set") == 0)           { lua_pushcfunction(state, l_flag_set); return 1; }
	if (strcmp(key, "clear") == 0)         { lua_pushcfunction(state, l_flag_clear); return 1; }
	if (strcmp(key, "test") == 0)          { lua_pushcfunction(state, l_flag_test); return 1; }
	if (strcmp(key, "testAndClear") == 0)  { lua_pushcfunction(state, l_flag_testAndClear); return 1; }
	if (strcmp(key, "wait") == 0)          { lua_pushcfunction(state, l_flag_wait); return 1; }

	return 0;
}


/*
 * =========================================================================
 * Constructor: event.flag([name])
 * =========================================================================
 */

static int l_createflag(lua_State* state)
{
	const char* name = lua_tostring(state, 1);  /* NULL if no argument */

	EventFlagUD* ud = (EventFlagUD*) lua_newuserdata(state, sizeof(EventFlagUD));
	ud->flag = NULL;
	ud->owned = false;
	ud->name[0] = '\0';

	if (name && name[0])
	{
		/* Named flag: look up or create in global registry */
		strncpy(ud->name, name, sizeof(ud->name) - 1);
		ud->name[sizeof(ud->name) - 1] = '\0';

		epicsGuard<epicsMutex> guard(namedFlagsMutex);

		std::map<std::string, lua_event_flag*>::iterator it = named_flags.find(std::string(name));

		if (it != named_flags.end())
		{
			ud->flag = it->second;
		}
		else
		{
			ud->flag = new lua_event_flag();
			named_flags[std::string(name)] = ud->flag;
		}

		ud->owned = false;  /* named flags are never owned by userdata */
	}
	else
	{
		/* Anonymous flag: create a new one, owned by this userdata */
		ud->flag = new lua_event_flag();
		ud->owned = true;
	}

	luaL_setmetatable(state, EVENT_FLAG_META);

	return 1;
}


/*
 * =========================================================================
 * Module registration
 * =========================================================================
 */

int luaopen_event(lua_State* L)
{
	/* Register the event flag metatable */
	if (luaL_newmetatable(L, EVENT_FLAG_META))
	{
		lua_pushcfunction(L, l_flag_index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, l_flag_gc);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, l_flag_tostring);
		lua_setfield(L, -2, "__tostring");
		lua_pushstring(L, "event.flag");
		lua_setfield(L, -2, "__name");

		/* Documentation for info(flag_object) */
		lua_newtable(L);
		lua_pushstring(L, ":set()              -- set the flag"); lua_rawseti(L, -2, 1);
		lua_pushstring(L, ":clear()            -- clear the flag"); lua_rawseti(L, -2, 2);
		lua_pushstring(L, ":test()             -- true if set (does not clear)"); lua_rawseti(L, -2, 3);
		lua_pushstring(L, ":testAndClear()     -- true if was set, clears atomically"); lua_rawseti(L, -2, 4);
		lua_pushstring(L, ":wait(timeout)      -- block until set or timeout (-1 = infinite)"); lua_rawseti(L, -2, 5);
		lua_setfield(L, -2, "_doc");
	}
	lua_pop(L, 1);

	/* Create the module table */
	static const luaL_Reg mylib[] = {
		{"flag", l_createflag},
		{NULL, NULL}
	};

	luaL_newlib(L, mylib);

	/* Documentation for info(event) */
	lua_newtable(L);
	lua_pushstring(L, "Event synchronization library"); lua_rawseti(L, -2, 1);
	lua_pushstring(L, ".flag([name]) -- create or find an event flag"); lua_rawseti(L, -2, 2);
	lua_setfield(L, -2, "_doc");

	return 1;
}

static void libeventRegister(void)    { luaRegisterLibrary("event", luaopen_event); }

extern "C"
{
	epicsExportRegistrar(libeventRegister);
}
