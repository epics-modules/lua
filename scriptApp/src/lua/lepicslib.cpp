extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <cadef.h>
#include <stdio.h>

static int l_caget(lua_State* state)
{
	if (! lua_isstring(state, 1))    { return 0; }

	ca_context_create(ca_enable_preemptive_callback);
	
	const char* pv_name = lua_tostring(state, 1);
	
	chid id;
	
	ca_create_channel(pv_name, NULL, NULL, 0, &id);
	ca_pend_io(0.001);
	
	switch (ca_field_type(id))
	{
		case DBF_STRING:
		{
			struct dbr_time_string val;
			
			ca_get(DBR_TIME_STRING, id, &val);
			
			lua_pushstring(state, val.value);
			break;
		}
		
		case DBF_ENUM:
		{
			struct dbr_time_enum val;
			
			ca_get(DBR_TIME_ENUM, id, &val);
			
			lua_pushnumber(state, val.value);
			break;
		}
		
		case DBF_CHAR:
		{
			struct dbr_time_char val;
			
			ca_get(DBR_TIME_ENUM, id, &val);
			
			lua_pushnumber(state, val.value);
			break;
		}
		
		case DBF_SHORT:
		{
			struct dbr_time_short val;
			
			ca_get(DBR_TIME_SHORT, id, &val);
			
			lua_pushnumber(state, val.value);
			break;
		}
		
		case DBF_LONG:
		{
			struct dbr_time_long val;
			
			ca_get(DBR_TIME_LONG, id, &val);
			
			lua_pushnumber(state, val.value);
			break;
		}
		
		case DBF_FLOAT:
		{
			struct dbr_time_float val;
			
			ca_get(DBR_TIME_FLOAT, id, &val);
			
			lua_pushnumber(state, val.value);
			break;
		}
			
		case DBF_DOUBLE:
		{
			struct dbr_time_double val;
			
			ca_get(DBR_TIME_DOUBLE, id, &val);
			
			lua_pushnumber(state, val.value);
			break;
		}
	}
	
	ca_clear_channel(id);
	
	ca_context_destroy();
	
	return 1;
}

static int l_caput(lua_State* state)
{
	if (! lua_isstring(state, 1))    { return 0; }
	
	ca_context_create(ca_enable_preemptive_callback);
	
	const char* pv_name = lua_tostring(state, 1);
	
	chid id;
	
	ca_create_channel(pv_name, NULL, NULL, 0, &id);
	ca_pend_io(0.001);
	
	switch (lua_type(state, 2))
	{
		case LUA_TNUMBER:
		{
			double data = lua_tonumber(state, 2);
			ca_put(DBR_DOUBLE, id, &data);
			break;
		}
		
		case LUA_TBOOLEAN:
		{
			short data = lua_toboolean(state, 2);
			ca_put(DBR_INT, id, &data);
			break;
		}
		
		case LUA_TSTRING:
		{
			ca_put(DBR_STRING, id, lua_tostring(state, 2));
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
	
	ca_clear_channel(id);
	
	ca_context_destroy();
	
	return 0;
}

static const luaL_Reg mylib[] = {
	{"get", l_caget},
	{"put", l_caput},
	{NULL, NULL}  /* sentinel */
};


int luaopen_epics (lua_State *L) 
{
	luaL_newlib(L, mylib);
	return 1;
}
