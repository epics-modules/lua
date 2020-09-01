#include <dbStaticLib.h>
#include <epicsExport.h>
#include <iocsh.h>
#include "luaEpics.h"

int l_setfield(lua_State* state)
{
	DBENTRY* entry = dbAllocEntry(*iocshPpdbbase);
	
	lua_getfield(state, 1, "name");
	const char* record_name = lua_tostring(state, -1);
	lua_pop(state, 1);
	
	lua_pushnil(state);
	
	while (lua_next(state, 2))
	{
		lua_pushvalue(state, -2);
		const char* field_name = lua_tostring(state, -1);
		const char* val = lua_tostring(state, -2);
		lua_pop(state, 2);
		
		if (dbFindRecord(entry, record_name))
		{
			dbFreeEntry(entry);
			luaL_error(state, "Unable to find record: %s\n", record_name);
		}
		
		if (dbFindField(entry, field_name))
		{
			dbFreeEntry(entry);
			luaL_error(state, "Unable to find %s field for record: %s\n", field_name, record_name);
		}
		
		if (dbPutString(entry, val))
		{
			dbFreeEntry(entry);
			luaL_error(state, "Error setting %s field on record %s to %s\n", field_name, record_name, val);
		}
	}
	
	dbFreeEntry(entry);
	return 0;
}

int l_record(lua_State* state)
{
	lua_settop(state, 2);

	const char* type = luaL_checkstring(state, 1);
	const char* name = luaL_checkstring(state, 2);

	printf("Called with: %s, %s\n", type, name);
	
	if (! iocshPpdbbase)   { luaL_error(state, "No database definition found.\n"); }
	
	DBENTRY* entry = dbAllocEntry(*iocshPpdbbase);
	
	int status = dbFindRecordType(entry, type);
	
	if (status)
	{ 
		dbFreeEntry(entry);
		luaL_error(state, "Error (%d) finding record type: %s\n", status, type);
	}
	
	status = dbCreateRecord(entry, name);
	
	if (status)
	{
		dbFreeEntry(entry);
		luaL_error(state, "Error (%d) creating record: %s\n", status, name);
	}
	
	dbFreeEntry(entry);
	
	lua_newtable(state);
	lua_pushstring(state, name);
	lua_setfield(state, -2, "name");
	
	luaL_setmetatable(state, "rec_meta");
	
	return 1;
}



int luaopen_database (lua_State *L)
{
	static const luaL_Reg rec_meta[] = {
		{"__call", l_setfield},
		{NULL, NULL}
	};
	
	luaL_newmetatable(L, "rec_meta");
	luaL_setfuncs(L, rec_meta, 0);
	lua_pop(L, 1);
	
	static const luaL_Reg mylib[] = {
		{"record", l_record},
		{NULL, NULL}  /* sentinel */
	};

	luaL_newlib(L, mylib);
	return 1;
}

static void libdatabaseRegister(void)    { luaRegisterLibrary("db", luaopen_database); }

extern "C"
{
	epicsExportRegistrar(libdatabaseRegister);
}
