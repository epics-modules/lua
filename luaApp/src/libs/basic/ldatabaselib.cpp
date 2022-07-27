#include <vector>

#include <dbStaticLib.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include "luaEpics.h"
#include "dbAccess.h"
#include "macLib.h"

static std::vector<lua_State*> hook_states;
typedef std::vector<lua_State*>::iterator state_it;

static epicsMutex state_lock;
static DB_LOAD_RECORDS_HOOK_ROUTINE previousHook=NULL;



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


static void myDatabaseHook(const char* fname, const char* macro)
{
	char* full_name = macEnvExpand(fname);
	char** pairs;
	
	macParseDefns(NULL, macro, &pairs);
	
	state_lock.lock();
	for (state_it it = hook_states.begin(); it != hook_states.end(); it++)
	{
		lua_getfield(*it, LUA_REGISTRYINDEX, "LDB_HOOKS");
		lua_len(*it, -1);
		
		int length = lua_tointeger(*it, -1);
		
		lua_pop(*it, 1);
		
		for (int i = 1; i <= length; i += 1)
		{
			lua_geti(*it, -1, i);
			lua_pushstring(*it, full_name);
			lua_newtable(*it);
			
			for ( ; pairs && pairs[0]; pairs += 2)
			{
				lua_pushstring(*it, pairs[0]);
				lua_pushstring(*it, pairs[1]);
				lua_settable(*it, -3);
			}
			
			int status = lua_pcall(*it, 2, 1, 0);
			
			if (status)    
			{
				std::string err(lua_tostring(*it, -1));
				
				state_lock.unlock();
				luaL_error(*it, err.c_str());
			}
		}
		
		lua_pop(*it, 1);
	}
	state_lock.unlock();
	
	if (previousHook)    { previousHook(fname, macro); }
}

int registerDbHook(lua_State* state)
{
	if ( lua_type(state, -1) != LUA_TFUNCTION)
	{
		luaL_error(state, "Database hook must be a function\n");
	}
	
	lua_getfield(state, LUA_REGISTRYINDEX, "LDB_HOOKS");
	
	// DB_HOOKS[ length + 1] = new_hook
	lua_len(state, -1);
	
	int next_val = 1 + lua_tointeger(state, -1);
	
	lua_pop(state, 1);
	
	lua_pushvalue(state, -2);
	lua_seti(state, -2, next_val);
	lua_pop(state, 2);
	
	return 0;
}

int l_deregister(lua_State* state)
{
	state_lock.lock();
	for (state_it it = hook_states.begin(); it != hook_states.end(); it++)
	{
		if (*it == state)
		{
			hook_states.erase(it);
			break;
		}
	}
	state_lock.unlock();
	
	return 0;
}




int luaopen_database (lua_State *L)
{
	/*
	 * Add lua_State to list of states that potentially contain dbLoad callbacks.
	 * Then, set up a table to hold the callbacks with a garbage collection
	 * function to remove the lua_State when it is closed.
	 */
	state_lock.lock();
		hook_states.push_back(L);
	state_lock.unlock();
	
	static const luaL_Reg dereg_meta[] = {
		{"__gc", l_deregister},
		{NULL, NULL}
	};
	
	luaL_newmetatable(L, "deregister_meta");
	luaL_setfuncs(L, dereg_meta, 0);
	lua_pop(L, 1);
	
	lua_newtable(L);
	luaL_setmetatable(L, "deregister_meta");
	lua_setfield(L, LUA_REGISTRYINDEX, "LDB_HOOKS");
	
	static const luaL_Reg rec_meta[] = {
		{"__call", l_setfield},
		{NULL, NULL}
	};

	luaL_newmetatable(L, "rec_meta");
	luaL_setfuncs(L, rec_meta, 0);
	lua_pop(L, 1);

	static const luaL_Reg mylib[] = {
		{"record", l_record},
		{"registerDatabaseHook", registerDbHook},
		{NULL, NULL}  /* sentinel */
	};

	luaL_newlib(L, mylib);
	return 1;
}

static void libdatabaseRegister(void)    
{ 
	previousHook = dbLoadRecordsHook;
	dbLoadRecordsHook = myDatabaseHook;
	luaRegisterLibrary("db", luaopen_database); 
}

extern "C"
{
	epicsExportRegistrar(libdatabaseRegister);
}
