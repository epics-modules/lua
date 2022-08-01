#include <functional>
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
static epicsMutex state_lock;
static DB_LOAD_RECORDS_HOOK_ROUTINE previousHook=NULL;


class _entry
{
	private:
		_entry()    { this->entry = dbAllocEntry(*iocshPpdbbase); }
		~_entry()   { if(this->entry) { dbFreeEntry(this->entry); } }
		
	public:
		DBENTRY* entry = NULL;
		
		static _entry* create(lua_State* state)
		{			
			if (! iocshPpdbbase)    { luaL_error(state, "No database definition found.\n"); }
			
			return new _entry();
		}
		
		static void destroy(_entry* instance)    { delete instance; }
};

_entry* genEntry(lua_State* state)
{
	return _entry::create(state);
}


std::string fieldName(_entry* in)
{
	char* output = dbGetFieldName(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string defaultVal(_entry* in)
{
	char* output = dbGetDefault(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string recordTypeName(_entry* in)
{
	char* output = dbGetRecordTypeName(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string recordName(_entry* in)
{
	char* output = dbGetRecordName(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string getString(_entry* in)
{
	char* output = dbGetString(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string getPrompt(_entry* in)
{
	char* output = dbGetPrompt(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string getMenuFromIndex(_entry* in, int index)
{
	char* output = dbGetMenuStringFromIndex(in->entry, index);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string getInfoName(_entry* in)
{
	const char* output = dbGetInfoName(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string getInfoString(_entry* in)
{
	const char* output = dbGetInfoString(in->entry);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

std::string getInfo(_entry* in, const char* name)
{
	const char* output = dbGetInfo(in->entry, name);
	
	if (output)    { return std::string(output); }
	else           { return std::string(""); }
}

bool putAttr(_entry* in, const char* key, const char* val)    { return (dbPutRecordAttribute(in->entry, key, val) == 0); }
bool putInfo(_entry* in, const char* key, const char* val)    { return (dbPutInfo(in->entry, key, val) == 0); }

bool getAttr(_entry* in, const char* key)              { return (dbGetRecordAttribute(in->entry, key) == 0); }
bool findRecord(_entry* in, const char* name)          { return (dbFindRecord(in->entry, name) == 0); }
bool createRecord(_entry* in, const char* name)        { return (dbCreateRecord(in->entry, name) == 0); }
bool createAlias(_entry* in, const char* name)         { return (dbCreateAlias(in->entry, name) == 0); }
bool deleteAliases(_entry* in)                         { return (dbDeleteAliases(in->entry) == 0); }
bool copyRecord(_entry* in, const char* name, int ow)  { return (dbCopyRecord(in->entry, name, ow) == 0); }
bool deleteRecord(_entry* in)                          { return (dbDeleteRecord(in->entry) == 0); }
bool findRecordType(_entry* in, const char* name)      { return (dbFindRecordType(in->entry, name) == 0); }
bool findField(_entry* in, const char* name)           { return dbFindField(in->entry, name); }
bool putString(_entry* in, const char* value)          { return (dbPutString(in->entry, value) == 0); }
bool putIndex(_entry* in, int index)                   { return (dbPutMenuIndex(in->entry, index) == 0); }
bool linkField(_entry* in, int index)                  { return (dbGetLinkField(in->entry, index) == 0); }
bool findInfo(_entry* in, const char* name)            { return (dbFindInfo(in->entry, name) == 0); }
bool putInfoString(_entry* in, const char* val)        { return (dbPutInfoString(in->entry, val) == 0); }

bool firstRecord(_entry* in)      { return (dbFirstRecord(in->entry) == 0); }
bool nextRecord(_entry* in)       { return (dbNextRecord(in->entry) == 0); }
bool isAlias(_entry* in)          { return dbIsAlias(in->entry); }
bool firstField(_entry* in)       { return (dbFirstField(in->entry, 0) == 0); }
bool nextField(_entry* in)        { return (dbNextField(in->entry, 0) == 0); }
bool firstRecordType(_entry* in)  { return (dbFirstRecordType(in->entry) == 0); }
bool nextRecordType(_entry* in)   { return (dbNextRecordType(in->entry) == 0); }
bool foundField(_entry* in)       { return dbFoundField(in->entry); }
bool isDefault(_entry* in)        { return dbIsDefaultValue(in->entry); }
bool firstInfo(_entry* in)        { return (dbFirstInfo(in->entry) == 0); }
bool nextInfo(_entry* in)         { return (dbNextInfo(in->entry) == 0); }
bool deleteInfo(_entry* in)       { return (dbDeleteInfo(in->entry) == 0); }

int numFields(_entry* in)       { return dbGetNFields(in->entry, 0); }
int numRecords(_entry* in)      { return dbGetNRecords(in->entry); }
int numAliases(_entry* in)      { return dbGetNAliases(in->entry); }
int numRecordTypes(_entry* in)  { return dbGetNRecordTypes(in->entry); }
int promptGroup(_entry* in)     { return dbGetPromptGroup(in->entry); }
int numChoices(_entry* in)      { return dbGetNMenuChoices(in->entry); }
int getIndex(_entry* in)        { return dbGetMenuIndex(in->entry); }
int numLinks(_entry* in)        { return dbGetNLinks(in->entry); }

#ifdef WITH_DBFTYPE
int fieldType(_entry* in)       { return dbGetFieldDbfType(in->entry); }
#endif

int getIndexFromMenu(_entry* in, const char* name)    { return dbGetMenuIndexFromString(in->entry, name); }

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
	for (auto it = hook_states.begin(); it != hook_states.end(); it++)
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
	for (auto it = hook_states.begin(); it != hook_states.end(); it++)
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
	
	
	// DBENTRY* wrapper and all associated functions
	
	LuaClass<_entry> lua_ew(L, "dbentry");
	lua_ew.ctor<void, lua_State*>("new",&_entry::create, &_entry::destroy);
	
	LuaModule dbmod (L, "db");
	dbmod.fun("entry", genEntry);
	
	dbmod.fun("getNRecordTypes", numRecordTypes);
	dbmod.fun("findRecordType", findRecordType);
	dbmod.fun("firstRecordType", firstRecordType);
	dbmod.fun("nextRecordType", nextRecordType);
	dbmod.fun("getRecordTypeName", recordTypeName);
	
	dbmod.fun("getNFields", numFields);
	dbmod.fun("firstField", firstField);
	dbmod.fun("nextField", nextField);
	dbmod.fun("getFieldName", fieldName);
	
	#ifdef WITH_DBFTYPE
	dbmod.fun("getFieldDbfType", fieldType);
	#endif

	dbmod.fun("getDefault", defaultVal);
	
	dbmod.fun("getPrompt", getPrompt);
	dbmod.fun("getPromptGroup", promptGroup);
	
	dbmod.fun("putRecordAttribute", putAttr);
	dbmod.fun("getRecordAttribute", getAttr);
	
	dbmod.fun("getNAliases", numAliases);
	
	dbmod.fun("getNRecords", numRecords);
	dbmod.fun("findRecord", findRecord);
	dbmod.fun("firstRecord", firstRecord);
	dbmod.fun("nextRecord", nextRecord);
	dbmod.fun("getRecordName", recordName);
	
	dbmod.fun("isAlias", isAlias);
	
	dbmod.fun("createRecord", createRecord);
	dbmod.fun("createAlias", createAlias);
	dbmod.fun("deleteRecord", deleteRecord);
	dbmod.fun("deleteAliases", deleteAliases);
	
	dbmod.fun("copyRecord", copyRecord);
	
	dbmod.fun("findField", findField);
	dbmod.fun("foundField", foundField);
	
	dbmod.fun("getString", getString);
	dbmod.fun("putString", putString);
	dbmod.fun("isDefaultValue", isDefault);
	
	dbmod.fun("getNMenuChoices", numChoices);
	dbmod.fun("getMenuIndex", getIndex);
	dbmod.fun("putMenuIndex", putIndex);
	dbmod.fun("getMenuStringFromIndex", getMenuFromIndex);
	dbmod.fun("getMenuIndexFromString", getIndexFromMenu);
	
	dbmod.fun("getNLinks", numLinks);
	dbmod.fun("getLinkField", linkField);
	
	dbmod.fun("firstInfo", firstInfo);
	dbmod.fun("nextInfo", nextInfo);
	dbmod.fun("findInfo", findInfo);
	dbmod.fun("getInfoName", getInfoName);
	dbmod.fun("getInfoString", getInfoString);
	dbmod.fun("putInfoString", putInfoString);
	dbmod.fun("putInfo", putInfo);
	dbmod.fun("deleteInfo", deleteInfo);
	dbmod.fun("getInfo", getInfo);
		
	dbmod.fun("registerDatabaseHook", registerDbHook);
	
	
	// record generation functions
	static const luaL_Reg rec_meta[] = {
		{"__call", l_setfield},
		{NULL, NULL}
	};

	luaL_newmetatable(L, "rec_meta");
	luaL_setfuncs(L, rec_meta, 0);
	lua_pop(L, 1);
	
	dbmod.fun("record", l_record);
	
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
