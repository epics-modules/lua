#include <cadef.h>
#include <string>
#include <stdio.h>
#include <epicsThread.h>
#include <epicsExport.h>
#include "luaEpics.h"

static int epics_get(lua_State* state, const char* pv_name)
{
	if (pv_name == NULL)    { return 0; }

	int status = ca_context_create(ca_enable_preemptive_callback);
	SEVCHK(status, NULL);

	chid id;

	status = ca_create_channel(pv_name, NULL, NULL, 0, &id);
	SEVCHK(status, NULL);

	switch (status)
	{
		case ECA_STRTOBIG:
			printf("PV name too long\n");
			return 0;

		case ECA_ALLOCMEM:
			printf("Cannot allocate memory\n");
			return 0;

		case ECA_BADTYPE:
			printf("Invalid DBR_XXXX type\n");
			return 0;

		default:
			break;
	}

	ca_pend_io(0.1);
	SEVCHK (status, NULL);

	switch (ca_field_type(id))
	{
		case -1:
			return 0;

		case DBF_STRING:
		{
			struct dbr_time_string val;

			status = ca_get(DBR_TIME_STRING, id, &val);

			lua_pushstring(state, val.value);
			break;
		}

		case DBF_ENUM:
		{
			struct dbr_time_enum val;

			status = ca_get(DBR_TIME_ENUM, id, &val);

			lua_pushnumber(state, val.value);
			break;
		}

		case DBF_CHAR:
		{
			struct dbr_time_char val;

			status = ca_get(DBR_TIME_ENUM, id, &val);

			lua_pushnumber(state, val.value);
			break;
		}

		case DBF_SHORT:
		{
			struct dbr_time_short val;

			status = ca_get(DBR_TIME_SHORT, id, &val);

			lua_pushnumber(state, val.value);
			break;
		}

		case DBF_LONG:
		{
			struct dbr_time_long val;

			status = ca_get(DBR_TIME_LONG, id, &val);

			lua_pushnumber(state, val.value);
			break;
		}

		case DBF_FLOAT:
		{
			struct dbr_time_float val;

			status = ca_get(DBR_TIME_FLOAT, id, &val);

			lua_pushnumber(state, val.value);
			break;
		}

		case DBF_DOUBLE:
		{
			struct dbr_time_double val;

			status = ca_get(DBR_TIME_DOUBLE, id, &val);

			lua_pushnumber(state, val.value);
			break;
		}

		case DBF_NO_ACCESS:
			return 0;
	}

	SEVCHK(status, NULL);

	ca_pend_io(0.1);

	ca_clear_channel(id);

	ca_context_destroy();

	return 1;
}

static int epics_put(lua_State* state, const char* pv_name, int offset)
{
	if (pv_name == NULL)    { return 0; }

	int status = ca_context_create(ca_enable_preemptive_callback);
	SEVCHK(status, NULL);

	chid id;

	status = ca_create_channel(pv_name, NULL, NULL, 0, &id);
	SEVCHK(status, NULL);

	switch (status)
	{
		case ECA_STRTOBIG:
			printf("PV name too long\n");
			return 0;

		case ECA_ALLOCMEM:
			printf("Cannot allocate memory\n");
			return 0;

		case ECA_BADTYPE:
			printf("Invalid DBR_XXXX type\n");
			return 0;

		default:
			break;
	}

	status = ca_pend_io(0.1);
	SEVCHK (status, NULL);

	switch (lua_type(state, offset))
	{
		case LUA_TNUMBER:
		{
			double data = lua_tonumber(state, offset);
			status = ca_put(DBR_DOUBLE, id, &data);
			break;
		}

		case LUA_TBOOLEAN:
		{
			short data = lua_toboolean(state, offset);
			status = ca_put(DBR_INT, id, &data);
			break;
		}

		case LUA_TSTRING:
		{
			const char* data = lua_tostring(state, offset);
			status = ca_put(DBR_STRING, id, data);
			break;
		}

		case LUA_TNIL:
			break;

		case LUA_TTABLE:
		case LUA_TFUNCTION:
		case LUA_TUSERDATA:
		case LUA_TTHREAD:
		case LUA_TLIGHTUSERDATA:
			/* Unsupported types */
			break;
	}

	SEVCHK(status, NULL);

	ca_pend_io(0.1);

	ca_clear_channel(id);

	ca_context_destroy();

	return 0;
}


static int l_caget(lua_State* state)
{
	const char* pv_name = lua_tostring(state, 1);

	return epics_get(state, pv_name);
}

static int l_caput(lua_State* state)
{
	const char* pv_name = lua_tostring(state, 1);

	return epics_put(state, pv_name, 2);
}

static int l_epicssleep(lua_State* state)
{
	double seconds = lua_tonumber(state, 1);

	epicsThreadSleep(seconds);

	return 0;
}

static int l_pvgetval(lua_State* state)
{
	lua_getfield(state, 1, "pv_name");
	const char* pv_name = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);

	const char* field_name = lua_tostring(state, 2);

	std::string full_name(pv_name);
	full_name.append(".");
	full_name.append(field_name);

	return epics_get(state, full_name.c_str());
}

static int l_pvsetval(lua_State* state)
{
	lua_getfield(state, 1, "pv_name");
	const char* pv_name = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);

	const char* field_name = lua_tostring(state, 2);

	std::string full_name(pv_name);
	full_name.append(".");
	full_name.append(field_name);

	return epics_put(state, full_name.c_str(), 3);
}

static int l_pvgetname(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("PV reference not given. (Did you use '.getName' instead of ':getName'?)\n");
		return 0;
	}

	lua_getfield(state, -1, "pv_name");

	return 1;
}

extern "C"
{
	void luaGeneratePV(lua_State* state, const char* pv_name)
	{
		static const luaL_Reg pv_meta[] = {
			{"__index", l_pvgetval},
			{"__newindex", l_pvsetval},
			{NULL, NULL}
		};

		static const luaL_Reg pv_funcs[] = {
			{"getName", l_pvgetname},
			{NULL, NULL}
		};

		luaL_newmetatable(state, "pv_meta");
		luaL_setfuncs(state, pv_meta, 0);
		lua_pop(state, 1);

		lua_newtable(state);
		luaL_setfuncs(state, pv_funcs, 0);

		lua_pushstring(state, pv_name);
		lua_setfield(state, -2, "pv_name");

		luaL_setmetatable(state, "pv_meta");
	}
}

static int l_createpv(lua_State* state)
{
	if (! lua_isstring(state, 1))    { return 0; }

	const char* pv_name = lua_tostring(state, 1);

	luaGeneratePV(state, pv_name);

	return 1;
}


int luaopen_epics (lua_State *L)
{
	static const luaL_Reg mylib[] = {
		{"get", l_caget},
		{"put", l_caput},
		{"sleep", l_epicssleep},
		{"pv", l_createpv},
		{NULL, NULL}  /* sentinel */
	};

	luaL_newlib(L, mylib);
	return 1;
}

static void libepicsRegister(void)    { luaRegisterLibrary("epics", luaopen_epics); }

extern "C"
{
	epicsExportRegistrar(libepicsRegister);
}
