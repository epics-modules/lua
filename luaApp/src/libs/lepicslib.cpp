#include <cadef.h>
#include <string>
#include <stdio.h>
#include <epicsExport.h>
#include "lepicslib.h"


/*
 * CA context management -- cached per Lua state.
 *
 * A sentinel userdata is stored in the Lua registry. Its __gc
 * destroys the CA context when the Lua state is closed. The
 * context is created once on first use and reused for all
 * subsequent epics.get/put/pv calls in that state.
 */
#define LEPICS_CA_CONTEXT_KEY "LEPICS_CA_CONTEXT"

static int l_ca_context_gc(lua_State* L)
{
	if (ca_current_context())    { ca_context_destroy(); }
	return 0;
}

static void ensure_ca_context(lua_State* L)
{
	if (ca_current_context())    { return; }

	ca_context_create(ca_enable_preemptive_callback);

	/* Create a sentinel userdata with __gc to destroy the context */
	lua_newuserdata(L, 1);

	if (luaL_newmetatable(L, "lepics_ca_gc"))
	{
		lua_pushcfunction(L, l_ca_context_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	lua_setfield(L, LUA_REGISTRYINDEX, LEPICS_CA_CONTEXT_KEY);
}


static int epics_get(lua_State* state, const char* pv_name, double timeout)
{
	if (pv_name == NULL)
	{
		lua_pushnil(state);
		lua_pushstring(state, "PV name is nil");
		return 2;
	}

	ensure_ca_context(state);

	chid id;

	int status = ca_create_channel(pv_name, NULL, NULL, 0, &id);

	if (status != ECA_NORMAL)
	{
		lua_pushnil(state);
		lua_pushfstring(state, "Failed to create channel for '%s'", pv_name);
		return 2;
	}

	status = ca_pend_io(timeout);

	if (status != ECA_NORMAL)
	{
		ca_clear_channel(id);
		lua_pushnil(state);
		lua_pushfstring(state, "Timeout connecting to '%s'", pv_name);
		return 2;
	}

	int result = 0;

	switch (ca_field_type(id))
	{
		case -1:
		case DBF_NO_ACCESS:
			break;

		case DBF_STRING:
		{
			struct dbr_time_string val;
			status = ca_get(DBR_TIME_STRING, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushstring(state, val.value); result = 1; }
			break;
		}

		case DBF_ENUM:
		{
			struct dbr_time_enum val;
			status = ca_get(DBR_TIME_ENUM, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
			break;
		}

		case DBF_CHAR:
		{
			struct dbr_time_char val;
			status = ca_get(DBR_TIME_CHAR, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
			break;
		}

		case DBF_SHORT:
		{
			struct dbr_time_short val;
			status = ca_get(DBR_TIME_SHORT, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
			break;
		}

		case DBF_LONG:
		{
			struct dbr_time_long val;
			status = ca_get(DBR_TIME_LONG, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
			break;
		}

		case DBF_FLOAT:
		{
			struct dbr_time_float val;
			status = ca_get(DBR_TIME_FLOAT, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
			break;
		}

		case DBF_DOUBLE:
		{
			struct dbr_time_double val;
			status = ca_get(DBR_TIME_DOUBLE, id, &val);
			status = ca_pend_io(timeout);
			if (status == ECA_NORMAL) { lua_pushnumber(state, val.value); result = 1; }
			break;
		}
	}

	ca_clear_channel(id);

	if (result == 0)
	{
		lua_pushnil(state);
		lua_pushfstring(state, "Failed to read '%s'", pv_name);
		return 2;
	}

	return result;
}

static int epics_put(lua_State* state, const char* pv_name, int offset, double timeout)
{
	if (pv_name == NULL)
	{
		lua_pushnil(state);
		lua_pushstring(state, "PV name is nil");
		return 2;
	}

	ensure_ca_context(state);

	chid id;

	int status = ca_create_channel(pv_name, NULL, NULL, 0, &id);

	if (status != ECA_NORMAL)
	{
		lua_pushnil(state);
		lua_pushfstring(state, "Failed to create channel for '%s'", pv_name);
		return 2;
	}

	status = ca_pend_io(timeout);

	if (status != ECA_NORMAL)
	{
		ca_clear_channel(id);
		lua_pushnil(state);
		lua_pushfstring(state, "Timeout connecting to '%s'", pv_name);
		return 2;
	}

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

		default:
		{
			ca_clear_channel(id);
			lua_pushnil(state);
			lua_pushfstring(state, "Unsupported value type for put to '%s'", pv_name);
			return 2;
		}
	}

	if (status != ECA_NORMAL)
	{
		ca_clear_channel(id);
		lua_pushnil(state);
		lua_pushfstring(state, "Failed to put value to '%s'", pv_name);
		return 2;
	}

	ca_pend_io(timeout);
	ca_clear_channel(id);

	lua_pushboolean(state, 1);
	return 1;
}


static int l_caget(lua_State* state)
{
	int num_ops = lua_gettop(state);

	const char* pv_name = luaL_checkstring(state, 1);
	double timeout = 1.0;

	if (num_ops == 2)    { timeout = luaL_checknumber(state, 2); }

	return epics_get(state, pv_name, timeout);
}

static int l_caput(lua_State* state)
{
	const char* pv_name = luaL_checkstring(state, 1);
	double timeout = 1.0;

	if (lua_gettop(state) >= 3)    { timeout = luaL_checknumber(state, 3); }

	return epics_put(state, pv_name, 2, timeout);
}

static int l_pvgetval(lua_State* state)
{
	lua_getfield(state, 1, "pv_name");
	const char* pv_name = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);

	const char* field_name = lua_tostring(state, 2);

	if (!pv_name || !field_name)    { return 0; }

	std::string full_name(pv_name);
	full_name.append(".");
	full_name.append(field_name);

	return epics_get(state, full_name.c_str(), 1.0);
}

static int l_pvsetval(lua_State* state)
{
	lua_getfield(state, 1, "pv_name");
	const char* pv_name = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);

	const char* field_name = lua_tostring(state, 2);

	if (!pv_name || !field_name)    { return 0; }

	std::string full_name(pv_name);
	full_name.append(".");
	full_name.append(field_name);

	return epics_put(state, full_name.c_str(), 3, 1.0);
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
		{"get",   l_caget},
		{"put",   l_caput},
		{"pv",    l_createpv},
		{NULL, NULL}
	};

	luaL_newlib(L, mylib);
	return 1;
}

static void libepicsRegister(void)    { luaRegisterLibrary("epics", luaopen_epics); }

extern "C"
{
	epicsExportRegistrar(libepicsRegister);
}
