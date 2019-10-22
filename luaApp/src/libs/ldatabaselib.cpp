#include <dbStaticLib.h>
#include <epicsExport.h>
#include "luaEpics.h"

int l_record(lua_State* state)
{
	lua_settop(state, 2);

	const char* type = luaL_checkstring(state, 1);
	const char* name = luaL_checkstring(state, 2);
	
	lua_getglobal(state, "pdbbase");
	DBBASE** pdbbase = (DBBASE**) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	DBENTRY* entry = dbAllocEntry(*pdbbase);
	
	int status = dbFindRecordType(entry, type);
	
	if (status) { printf("Error in FindRecordType: %d\n", status); return 0; }
	
	status = dbCreateRecord(entry, name);
	
	if (status) { printf("Error in CreateRecord: %d\n", status); return 0; }
	
	dbFreeEntry(entry);
	
	return 0;
}



int luaopen_database (lua_State *L)
{
	static const luaL_Reg mylib[] = {
		{"addRecord", l_record},
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
