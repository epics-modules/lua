#include <new>
#include <string>
#include <vector>

#include <dbStaticLib.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsThread.h>
#include "luaEpics.h"
#include "epicsVersion.h"
#include "dbAccess.h"
#include "macLib.h"


#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 4, 0)
	#define WITH_DBFTYPE
#endif


static std::vector<lua_State*> hook_states;
typedef std::vector<lua_State*>::iterator state_it;
static epicsMutex state_lock;
static DB_LOAD_RECORDS_HOOK_ROUTINE previousHook=NULL;


/*
 * lua_dbentry -- userdata wrapping a DBENTRY* cursor for static database access.
 * Stored directly in Lua's GC heap; __gc frees the DBENTRY.
 */
typedef struct {
	DBENTRY* entry;
} lua_dbentry;


/*
 * lua_dbrecord -- userdata stored directly in Lua's GC heap.
 * Holds a DBENTRY cursor and cached name/type strings.
 * Replaces the former _record class; all logic is in static C functions.
 */
typedef struct {
	DBENTRY*    entry;
	std::string name;
	std::string type;
} lua_dbrecord;


/*
 * Helper to extract the lua_dbentry userdata from the stack.
 */
static lua_dbentry* check_dbentry(lua_State* L, int idx)
{
	return (lua_dbentry*) luaL_checkudata(L, idx, "lua_dbentry");
}

/*
 * db.entry() -- create a new DBENTRY cursor.
 */
static int l_genEntry(lua_State* L)
{
	if (! iocshPpdbbase)    { return luaL_error(L, "No database definition found."); }
	
	lua_dbentry* ud = (lua_dbentry*) lua_newuserdata(L, sizeof(lua_dbentry));
	ud->entry = dbAllocEntry(*iocshPpdbbase);
	luaL_setmetatable(L, "lua_dbentry");
	
	return 1;
}

/*
 * __gc metamethod for lua_dbentry -- frees the DBENTRY cursor.
 */
static int l_entry_gc(lua_State* L)
{
	lua_dbentry* ud = (lua_dbentry*) lua_touserdata(L, 1);
	
	if (ud && ud->entry)
	{
		dbFreeEntry(ud->entry);
		ud->entry = NULL;
	}
	
	return 0;
}


/* ------------------------------------------------------------------ */
/*  String-returning entry methods                                     */
/* ------------------------------------------------------------------ */

static int l_getFieldName(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	char* output = dbGetFieldName(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getDefault(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	char* output = dbGetDefault(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getRecordTypeName(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	char* output = dbGetRecordTypeName(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getRecordName(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	char* output = dbGetRecordName(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getString(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	char* output = dbGetString(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getPrompt(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	char* output = dbGetPrompt(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getMenuStringFromIndex(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	int index = luaL_checkinteger(L, 2);
	char* output = dbGetMenuStringFromIndex(ud->entry, index);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getInfoName(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* output = dbGetInfoName(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getInfoString(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* output = dbGetInfoString(ud->entry);
	lua_pushstring(L, output ? output : "");
	return 1;
}

static int l_getInfo(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	const char* output = dbGetInfo(ud->entry, name);
	lua_pushstring(L, output ? output : "");
	return 1;
}


/* ------------------------------------------------------------------ */
/*  Bool-returning entry methods (status / mutation)                    */
/* ------------------------------------------------------------------ */

static int l_putRecordAttribute(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* key = luaL_checkstring(L, 2);
	const char* val = luaL_checkstring(L, 3);
	lua_pushboolean(L, dbPutRecordAttribute(ud->entry, key, val) == 0);
	return 1;
}

static int l_putInfo(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* key = luaL_checkstring(L, 2);
	const char* val = luaL_checkstring(L, 3);
	lua_pushboolean(L, dbPutInfo(ud->entry, key, val) == 0);
	return 1;
}

static int l_getRecordAttribute(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* key = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbGetRecordAttribute(ud->entry, key) == 0);
	return 1;
}

static int l_findRecord(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbFindRecord(ud->entry, name) == 0);
	return 1;
}

static int l_createRecord(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbCreateRecord(ud->entry, name) == 0);
	return 1;
}

static int l_createAlias(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbCreateAlias(ud->entry, name) == 0);
	return 1;
}

static int l_deleteAliases(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbDeleteAliases(ud->entry) == 0);
	return 1;
}

static int l_copyRecord(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	int overwrite = luaL_checkinteger(L, 3);
	lua_pushboolean(L, dbCopyRecord(ud->entry, name, overwrite) == 0);
	return 1;
}

static int l_deleteRecord(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbDeleteRecord(ud->entry) == 0);
	return 1;
}

static int l_findRecordType(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbFindRecordType(ud->entry, name) == 0);
	return 1;
}

static int l_findField(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbFindField(ud->entry, name) == 0);
	return 1;
}

static int l_putString(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* value = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbPutString(ud->entry, value) == 0);
	return 1;
}

static int l_putMenuIndex(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	int index = luaL_checkinteger(L, 2);
	lua_pushboolean(L, dbPutMenuIndex(ud->entry, index) == 0);
	return 1;
}

static int l_getLinkField(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	int index = luaL_checkinteger(L, 2);
	lua_pushboolean(L, dbGetLinkField(ud->entry, index) == 0);
	return 1;
}

static int l_findInfo(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbFindInfo(ud->entry, name) == 0);
	return 1;
}

static int l_putInfoString(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* val = luaL_checkstring(L, 2);
	lua_pushboolean(L, dbPutInfoString(ud->entry, val) == 0);
	return 1;
}

static int l_firstRecord(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbFirstRecord(ud->entry) == 0);
	return 1;
}

static int l_nextRecord(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbNextRecord(ud->entry) == 0);
	return 1;
}

static int l_isAlias(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbIsAlias(ud->entry));
	return 1;
}

static int l_firstField(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbFirstField(ud->entry, 0) == 0);
	return 1;
}

static int l_nextField(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbNextField(ud->entry, 0) == 0);
	return 1;
}

static int l_firstRecordType(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbFirstRecordType(ud->entry) == 0);
	return 1;
}

static int l_nextRecordType(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbNextRecordType(ud->entry) == 0);
	return 1;
}

static int l_foundField(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbFoundField(ud->entry));
	return 1;
}

static int l_isDefaultValue(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbIsDefaultValue(ud->entry));
	return 1;
}

static int l_firstInfo(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbFirstInfo(ud->entry) == 0);
	return 1;
}

static int l_nextInfo(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbNextInfo(ud->entry) == 0);
	return 1;
}

static int l_deleteInfo(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushboolean(L, dbDeleteInfo(ud->entry) == 0);
	return 1;
}


/* ------------------------------------------------------------------ */
/*  Int-returning entry methods                                        */
/* ------------------------------------------------------------------ */

static int l_getNFields(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetNFields(ud->entry, 0));
	return 1;
}

static int l_getNRecords(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetNRecords(ud->entry));
	return 1;
}

static int l_getNAliases(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetNAliases(ud->entry));
	return 1;
}

static int l_getNRecordTypes(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetNRecordTypes(ud->entry));
	return 1;
}

static int l_getPromptGroup(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetPromptGroup(ud->entry));
	return 1;
}

static int l_getNMenuChoices(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetNMenuChoices(ud->entry));
	return 1;
}

static int l_getMenuIndex(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetMenuIndex(ud->entry));
	return 1;
}

static int l_getNLinks(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetNLinks(ud->entry));
	return 1;
}

#ifdef WITH_DBFTYPE
static int l_getFieldDbfType(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	lua_pushinteger(L, dbGetFieldDbfType(ud->entry));
	return 1;
}
#endif

static int l_getMenuIndexFromString(lua_State* L)
{
	lua_dbentry* ud = check_dbentry(L, 1);
	const char* name = luaL_checkstring(L, 2);
	lua_pushinteger(L, dbGetMenuIndexFromString(ud->entry, name));
	return 1;
}


/* ------------------------------------------------------------------ */
/*  Method dispatch table for lua_dbentry __index                      */
/* ------------------------------------------------------------------ */

static const luaL_Reg entry_methods[] = {
	/* String-returning */
	{"getFieldName",          l_getFieldName},
	{"getDefault",            l_getDefault},
	{"getRecordTypeName",     l_getRecordTypeName},
	{"getRecordName",         l_getRecordName},
	{"getString",             l_getString},
	{"getPrompt",             l_getPrompt},
	{"getMenuStringFromIndex",l_getMenuStringFromIndex},
	{"getInfoName",           l_getInfoName},
	{"getInfoString",         l_getInfoString},
	{"getInfo",               l_getInfo},
	
	/* Bool-returning (status / mutation) */
	{"putRecordAttribute",    l_putRecordAttribute},
	{"putInfo",               l_putInfo},
	{"getRecordAttribute",    l_getRecordAttribute},
	{"findRecord",            l_findRecord},
	{"createRecord",          l_createRecord},
	{"createAlias",           l_createAlias},
	{"deleteAliases",         l_deleteAliases},
	{"copyRecord",            l_copyRecord},
	{"deleteRecord",          l_deleteRecord},
	{"findRecordType",        l_findRecordType},
	{"findField",             l_findField},
	{"putString",             l_putString},
	{"putMenuIndex",          l_putMenuIndex},
	{"getLinkField",          l_getLinkField},
	{"findInfo",              l_findInfo},
	{"putInfoString",         l_putInfoString},
	{"firstRecord",           l_firstRecord},
	{"nextRecord",            l_nextRecord},
	{"isAlias",               l_isAlias},
	{"firstField",            l_firstField},
	{"nextField",             l_nextField},
	{"firstRecordType",       l_firstRecordType},
	{"nextRecordType",        l_nextRecordType},
	{"foundField",            l_foundField},
	{"isDefaultValue",        l_isDefaultValue},
	{"firstInfo",             l_firstInfo},
	{"nextInfo",              l_nextInfo},
	{"deleteInfo",            l_deleteInfo},
	
	/* Int-returning */
	{"getNFields",            l_getNFields},
	{"getNRecords",           l_getNRecords},
	{"getNAliases",           l_getNAliases},
	{"getNRecordTypes",       l_getNRecordTypes},
	{"getPromptGroup",        l_getPromptGroup},
	{"getNMenuChoices",       l_getNMenuChoices},
	{"getMenuIndex",          l_getMenuIndex},
	{"getNLinks",             l_getNLinks},
#ifdef WITH_DBFTYPE
	{"getFieldDbfType",       l_getFieldDbfType},
#endif
	{"getMenuIndexFromString",l_getMenuIndexFromString},
	
	{NULL, NULL}
};

/*
 * __index metamethod for lua_dbentry. Looks up method name in the
 * entry_methods table and returns the corresponding C function.
 */
static int l_entry_index(lua_State* L)
{
	luaL_checkudata(L, 1, "lua_dbentry");
	const char* key = luaL_checkstring(L, 2);
	
	for (const luaL_Reg* m = entry_methods; m->name != NULL; m++)
	{
		if (strcmp(key, m->name) == 0)
		{
			lua_pushcfunction(L, m->func);
			return 1;
		}
	}
	
	return luaL_error(L, "dbentry has no method '%s'", key);
}


/*
 * Creates or finds a dbrecord instance. Allocates a lua_dbrecord
 * userdata directly in Lua's GC heap with placement-new, and sets
 * the "lua_dbrecord" metatable (which includes __gc for cleanup).
 */
int genRecord(lua_State* state)    
{ 
	if (! iocshPpdbbase)    { return luaL_error(state, "No database definition found.\n"); }
	
	int num_params = lua_gettop(state);
	const char* rec_name;
	const char* rec_type = NULL;
	
	if (num_params == 1)
	{
		rec_name = luaL_checkstring(state, 1);
		
		/* Verify the record exists */
		DBENTRY* test = dbAllocEntry(*iocshPpdbbase);
		if (dbFindRecord(test, rec_name))
		{
			dbFreeEntry(test);
			return luaL_error(state, "Error finding record: %s", rec_name);
		}
		dbFreeEntry(test);
	}
	else
	{
		rec_type = luaL_checkstring(state, 1);
		rec_name = luaL_checkstring(state, 2);
	}
	
	/* Allocate userdata and construct with placement-new */
	lua_dbrecord* ud = (lua_dbrecord*) lua_newuserdata(state, sizeof(lua_dbrecord));
	new (ud) lua_dbrecord();
	
	ud->entry = dbAllocEntry(*iocshPpdbbase);
	ud->name  = std::string(rec_name);
	
	if (num_params == 1)
	{
		/* Find existing record, look up its RTYP */
		dbFindRecord(ud->entry, rec_name);
		dbGetRecordAttribute(ud->entry, "RTYP");
		
		char* type_str = dbGetString(ud->entry);
		ud->type = std::string(type_str ? type_str : "");
		
		dbFindRecord(ud->entry, rec_name);
	}
	else
	{
		ud->type = std::string(rec_type);
		
		if (dbFindRecord(ud->entry, rec_name))
		{
			/* Record doesn't exist, so create it */
			dbFindRecordType(ud->entry, rec_type);
			dbCreateRecord(ud->entry, rec_name);
		}
	}
	
	luaL_setmetatable(state, "lua_dbrecord");
	
	return 1;
}
int all_records(lua_State* state)
{
	if (! iocshPpdbbase)    { luaL_error(state, "No database definition found.\n"); }
	
	lua_newtable(state);
	int index = 1;
	
	DBENTRY* primary = dbAllocEntry(*iocshPpdbbase);
	
	int rtyp_status = dbFirstRecordType(primary);
	
	while (rtyp_status == 0)
	{
		DBENTRY* secondary = dbCopyEntry(primary);
		
		int rec_status = dbFirstRecord(secondary);
		
		while (rec_status == 0)
		{
			const char* rec_name = dbGetRecordName(secondary);
			
			lua_pushcfunction(state, genRecord);
			lua_pushstring(state, rec_name);
			lua_call(state, 1, 1);
			
			lua_seti(state, -2, index++);
			
			rec_status = dbNextRecord(secondary);
		}
		
		dbFreeEntry(secondary);
		rtyp_status = dbNextRecordType(primary);
	}
	
	dbFreeEntry(primary);
	
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

/*
 * rec:field(fieldname, value) -- set a record field by name.
 */
static int l_record_field(lua_State* state)
{
	lua_dbrecord* ud = (lua_dbrecord*) luaL_checkudata(state, 1, "lua_dbrecord");
	const char* fieldname = luaL_checkstring(state, 2);
	const char* value     = luaL_checkstring(state, 3);
	
	if (dbFindField(ud->entry, fieldname))
	{
		dbFindRecord(ud->entry, ud->name.c_str());
		return luaL_error(state, "Unable to find %s field for record: %s", fieldname, ud->name.c_str());
	}
	
	if (dbPutString(ud->entry, value))
	{
		dbFindRecord(ud->entry, ud->name.c_str());
		return luaL_error(state, "Error setting %s field on record %s to %s", fieldname, ud->name.c_str(), value);
	}
	
	dbFindRecord(ud->entry, ud->name.c_str());
	return 0;
}

/*
 * rec:info(name, value) -- add an info field to the record.
 */
static int l_record_info(lua_State* state)
{
	lua_dbrecord* ud = (lua_dbrecord*) luaL_checkudata(state, 1, "lua_dbrecord");
	const char* info_name  = luaL_checkstring(state, 2);
	const char* info_value = luaL_checkstring(state, 3);
	
	if (dbPutInfo(ud->entry, info_name, info_value))
	{
		dbFindRecord(ud->entry, ud->name.c_str());
		return luaL_error(state, "Error adding info to record %s: %s: %s", ud->name.c_str(), info_name, info_value);
	}
	
	dbFindRecord(ud->entry, ud->name.c_str());
	return 0;
}

/*
 * __gc metamethod -- destructs the lua_dbrecord (frees DBENTRY, std::strings).
 */
static int l_record_gc(lua_State* state)
{
	lua_dbrecord* ud = (lua_dbrecord*) lua_touserdata(state, 1);
	
	if (ud)
	{
		if (ud->entry)    { dbFreeEntry(ud->entry); }
		ud->~lua_dbrecord();
	}
	
	return 0;
}

/*
 * __index metamethod for lua_dbrecord. Dispatches known method and
 * property names, then falls back to reading a record field value.
 */
static int l_record_index(lua_State* state)
{
	lua_dbrecord* ud = (lua_dbrecord*) luaL_checkudata(state, 1, "lua_dbrecord");
	const char* key = luaL_checkstring(state, 2);
	
	/* Properties */
	if (strcmp(key, "name") == 0)
	{
		lua_pushstring(state, ud->name.c_str());
		return 1;
	}
	else if (strcmp(key, "type") == 0)
	{
		lua_pushstring(state, ud->type.c_str());
		return 1;
	}
	
	/* Methods */
	else if (strcmp(key, "field") == 0)
	{
		lua_pushcfunction(state, l_record_field);
		return 1;
	}
	else if (strcmp(key, "info") == 0)
	{
		lua_pushcfunction(state, l_record_info);
		return 1;
	}
	
	/* Fallback: treat key as a record field name and read its value */
	if (dbFindField(ud->entry, key))
	{
		dbFindRecord(ud->entry, ud->name.c_str());
		lua_pushstring(state, "");
		return 1;
	}
	
	char* val = dbGetString(ud->entry);
	dbFindRecord(ud->entry, ud->name.c_str());
	lua_pushstring(state, val ? val : "");
	return 1;
}

/*
 * __call metamethod for lua_dbrecord. Accepts a table of field
 * name-value pairs and sets them on the record.
 */
static int l_record_call(lua_State* state)
{
	lua_dbrecord* ud = (lua_dbrecord*) luaL_checkudata(state, 1, "lua_dbrecord");
	
	if (lua_istable(state, 2))
	{
		lua_pushnil(state);
		while (lua_next(state, 2))
		{
			if (lua_type(state, -2) == LUA_TSTRING)
			{
				const char* fieldname = lua_tostring(state, -2);
				const char* value     = lua_tostring(state, -1);
				
				if (fieldname && value)
				{
					if (dbFindField(ud->entry, fieldname))
					{
						dbFindRecord(ud->entry, ud->name.c_str());
						return luaL_error(state, "Unable to find %s field for record: %s", fieldname, ud->name.c_str());
					}
					
					if (dbPutString(ud->entry, value))
					{
						dbFindRecord(ud->entry, ud->name.c_str());
						return luaL_error(state, "Error setting %s field on record %s to %s", fieldname, ud->name.c_str(), value);
					}
					
					dbFindRecord(ud->entry, ud->name.c_str());
				}
			}
			lua_pop(state, 1);
		}
	}
	
	/* Return the record object itself for chaining */
	lua_pushvalue(state, 1);
	return 1;
}

/*
 * __newindex metamethod for lua_dbrecord. Sets a record field value.
 */
static int l_record_newindex(lua_State* state)
{
	lua_dbrecord* ud = (lua_dbrecord*) luaL_checkudata(state, 1, "lua_dbrecord");
	const char* key = luaL_checkstring(state, 2);
	const char* val = luaL_checkstring(state, 3);
	
	if (dbFindField(ud->entry, key))
	{
		dbFindRecord(ud->entry, ud->name.c_str());
		return luaL_error(state, "Unable to find %s field for record: %s", key, ud->name.c_str());
	}
	
	if (dbPutString(ud->entry, val))
	{
		dbFindRecord(ud->entry, ud->name.c_str());
		return luaL_error(state, "Error setting %s field on record %s to %s", key, ud->name.c_str(), val);
	}
	
	dbFindRecord(ud->entry, ud->name.c_str());
	return 0;
}

/*
 * db.loadRecords(filename, macros)
 *
 * Loads a database file with macros specified as a Lua table.
 *   db.loadRecords("motor.db", {P="ioc:", M="m1", PORT="serial1"})
 */
static int l_loadRecords(lua_State* state)
{
	const char* filename = luaL_checkstring(state, 1);
	
	std::string macros;
	
	if (lua_istable(state, 2))
	{
		macros = luaMacrosFromTable(state, 2);
	}
	else if (lua_isstring(state, 2))
	{
		/* Also accept a plain string for compatibility */
		macros = std::string(lua_tostring(state, 2));
	}
	
	int status = dbLoadRecords(filename, macros.empty() ? NULL : macros.c_str());
	
	lua_pushinteger(state, status);
	return 1;
}

/*
 * db.loadTemplate(filename, substitutions)
 *
 * Loads a database file multiple times with different macro sets.
 *
 * Variable style (each entry is a table of key=value macros):
 *   db.loadTemplate("motor.db", {
 *       {P="ioc:", M="m1", ADDR="0"},
 *       {P="ioc:", M="m2", ADDR="1"},
 *   })
 *
 * Pattern style (first entry names the variables, subsequent entries
 * provide positional values):
 *   db.loadTemplate("motor.db", {
 *       pattern = {"P", "M", "ADDR"},
 *       {"ioc:", "m1", "0"},
 *       {"ioc:", "m2", "1"},
 *   })
 *
 * Global macros (merged into every instance):
 *   db.loadTemplate("motor.db", {
 *       global = {P="ioc:", PORT="serial1"},
 *       {M="m1", ADDR="0"},
 *       {M="m2", ADDR="1"},
 *   })
 */
static int l_loadTemplate(lua_State* state)
{
	const char* filename = luaL_checkstring(state, 1);
	luaL_checktype(state, 2, LUA_TTABLE);
	
	/* Check for global macros */
	std::string global_macros;
	lua_getfield(state, 2, "global");
	if (lua_istable(state, -1))
	{
		global_macros = luaMacrosFromTable(state, lua_gettop(state));
	}
	lua_pop(state, 1);
	
	/* Check for pattern variable names */
	int has_pattern = 0;
	lua_getfield(state, 2, "pattern");
	if (lua_istable(state, -1))
	{
		has_pattern = 1;
	}
	int pattern_idx = lua_gettop(state);
	
	/* Iterate array entries (integer-keyed) in the substitutions table */
	int len = luaL_len(state, 2);
	
	for (int i = 1; i <= len; i++)
	{
		lua_geti(state, 2, i);
		
		if (!lua_istable(state, -1))
		{
			lua_pop(state, 1);
			continue;
		}
		
		int entry_idx = lua_gettop(state);
		std::string macros = global_macros;
		
		if (has_pattern)
		{
			/* Pattern style: map positional values to variable names */
			int npattern = luaL_len(state, pattern_idx);
			
			for (int j = 1; j <= npattern; j++)
			{
				lua_geti(state, pattern_idx, j);
				const char* varname = lua_tostring(state, -1);
				lua_pop(state, 1);
				
				lua_geti(state, entry_idx, j);
				const char* val = lua_tostring(state, -1);
				lua_pop(state, 1);
				
				if (varname && val)
				{
					if (!macros.empty())    { macros += ","; }
					macros += std::string(varname) + "=" + std::string(val);
				}
			}
		}
		else
		{
			/* Variable style: entry is a table of key=value pairs */
			std::string entry_macros = luaMacrosFromTable(state, entry_idx);
			
			if (!global_macros.empty() && !entry_macros.empty())
				macros = global_macros + "," + entry_macros;
			else if (!entry_macros.empty())
				macros = entry_macros;
			else
				macros = global_macros;
		}
		
		dbLoadRecords(filename, macros.empty() ? NULL : macros.c_str());
		
		lua_pop(state, 1);  /* pop entry table */
	}
	
	if (has_pattern)    { lua_pop(state, 1); }  /* pop pattern table */
	else                { lua_pop(state, 1); }  /* pop nil from getfield("pattern") */
	
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
	
	/* Register our custom metatable for db.record() return values. */
	if (luaL_newmetatable(L, "lua_dbrecord"))
	{
		lua_pushcfunction(L, l_record_index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, l_record_newindex);
		lua_setfield(L, -2, "__newindex");
		lua_pushcfunction(L, l_record_call);
		lua_setfield(L, -2, "__call");
		lua_pushcfunction(L, l_record_gc);
		lua_setfield(L, -2, "__gc");
		lua_pushstring(L, "dbrecord");
		lua_setfield(L, -2, "__name");
	}
	lua_pop(L, 1);
	
	/* Register our custom metatable for db.entry() return values. */
	if (luaL_newmetatable(L, "lua_dbentry"))
	{
		lua_pushcfunction(L, l_entry_index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, l_entry_gc);
		lua_setfield(L, -2, "__gc");
		lua_pushstring(L, "dbentry");
		lua_setfield(L, -2, "__name");
	}
	lua_pop(L, 1);
	
	/* Register the db module -- high-level functions plus all entry methods */
	static const luaL_Reg db_funcs[] = {
		{"entry",                 l_genEntry},
		{"record",                genRecord},
		{"loadRecords",           l_loadRecords},
		{"loadTemplate",          l_loadTemplate},
		{"list",                  all_records},
		{"registerDatabaseHook",  registerDbHook},
		
		/* Entry methods also available as db.xxx(entry, ...) */
		{"getNRecordTypes",       l_getNRecordTypes},
		{"findRecordType",        l_findRecordType},
		{"firstRecordType",       l_firstRecordType},
		{"nextRecordType",        l_nextRecordType},
		{"getRecordTypeName",     l_getRecordTypeName},
		{"getNFields",            l_getNFields},
		{"firstField",            l_firstField},
		{"nextField",             l_nextField},
		{"getFieldName",          l_getFieldName},
#ifdef WITH_DBFTYPE
		{"getFieldDbfType",       l_getFieldDbfType},
#endif
		{"getDefault",            l_getDefault},
		{"getPrompt",             l_getPrompt},
		{"getPromptGroup",        l_getPromptGroup},
		{"putRecordAttribute",    l_putRecordAttribute},
		{"getRecordAttribute",    l_getRecordAttribute},
		{"getNAliases",           l_getNAliases},
		{"getNRecords",           l_getNRecords},
		{"findRecord",            l_findRecord},
		{"firstRecord",           l_firstRecord},
		{"nextRecord",            l_nextRecord},
		{"getRecordName",         l_getRecordName},
		{"isAlias",               l_isAlias},
		{"createRecord",          l_createRecord},
		{"createAlias",           l_createAlias},
		{"deleteRecord",          l_deleteRecord},
		{"deleteAliases",         l_deleteAliases},
		{"copyRecord",            l_copyRecord},
		{"findField",             l_findField},
		{"foundField",            l_foundField},
		{"getString",             l_getString},
		{"putString",             l_putString},
		{"isDefaultValue",        l_isDefaultValue},
		{"getNMenuChoices",       l_getNMenuChoices},
		{"getMenuIndex",          l_getMenuIndex},
		{"putMenuIndex",          l_putMenuIndex},
		{"getMenuStringFromIndex",l_getMenuStringFromIndex},
		{"getMenuIndexFromString",l_getMenuIndexFromString},
		{"getNLinks",             l_getNLinks},
		{"getLinkField",          l_getLinkField},
		{"firstInfo",             l_firstInfo},
		{"nextInfo",              l_nextInfo},
		{"findInfo",              l_findInfo},
		{"getInfoName",           l_getInfoName},
		{"getInfoString",         l_getInfoString},
		{"putInfoString",         l_putInfoString},
		{"putInfo",               l_putInfo},
		{"deleteInfo",            l_deleteInfo},
		{"getInfo",               l_getInfo},
		{NULL, NULL}
	};
	
	luaL_newlib(L, db_funcs);
	lua_setglobal(L, "db");
	
	lua_getglobal(L, "db");
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
