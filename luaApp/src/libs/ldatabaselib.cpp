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


class _record
{
	private:
		// Create an instance associated with existing record
		_record(std::string name)
		{
			this->_name = name;
			this->record_entry = dbAllocEntry(*iocshPpdbbase);
			
			dbFindRecord(this->record_entry, name.c_str());
			dbGetRecordAttribute(this->record_entry, "RTYP");
			
			char* type_str = dbGetString(this->record_entry);
			this->_type = std::string(type_str);
			
			dbFindRecord(this->record_entry, name.c_str());
		}
	
		// Try to find an existing record, create a new record if not found
		_record(std::string type, std::string name)    
		{ 
			this->_type = type;
			this->_name = name;
			
			this->record_entry = dbAllocEntry(*iocshPpdbbase);
			
			if (dbFindRecord(this->record_entry, name.c_str()))
			{
				// Record doesn't exist, so we'll make it
				dbFindRecordType(this->record_entry, type.c_str());
				dbCreateRecord(this->record_entry, name.c_str());
			}
		}
		
		~_record()   { dbFreeEntry(this->record_entry); }
		
		void processField(lua_State* state, std::string fieldname, std::string value)
		{
			if (dbFindField(this->record_entry, fieldname.c_str()))
			{
				dbFindRecord(this->record_entry, this->_name.c_str());
				luaL_error(state, "Unable to find %s field for record: %s\n", fieldname.c_str(), this->_name.c_str());
			}
			
			if (dbPutString(this->record_entry, value.c_str()) )
			{
				dbFindRecord(this->record_entry, this->_name.c_str());
				luaL_error(state, "Error setting %s field on record %s to %s\n", fieldname.c_str(), this->_name.c_str(), value.c_str());
			}
		
			dbFindRecord(this->record_entry, this->_name.c_str());
		}
		
		std::string _type;
		std::string _name;
		DBENTRY* record_entry;
		
	public:
		static _record* create(lua_State* state)
		{
			if (! iocshPpdbbase)    { luaL_error(state, "No database definition found.\n"); }
			
			lua_settop(state, 2);
			std::string type = LuaStack<std::string>::get(state, 1);
			std::string name = LuaStack<std::string>::get(state, 2);
			
			DBENTRY* test_ent = dbAllocEntry(*iocshPpdbbase);
			
			int status = dbFindRecordType(test_ent, type.c_str());
			
			dbFreeEntry(test_ent);
			
			if (status)    { luaL_error(state, "Error (%d) finding record type: %s\n", status, type.c_str()); }
			
			return new _record(type, name);
		}
		
		static _record* find(lua_State* state)
		{
			if (! iocshPpdbbase)    { luaL_error(state, "No database definition found.\n"); }
			
			lua_settop(state, 1);
			std::string name = LuaStack<std::string>::get(state, 1);
			
			DBENTRY* test_ent = dbAllocEntry(*iocshPpdbbase);
			
			int status = dbFindRecord(test_ent, name.c_str());
			
			dbFreeEntry(test_ent);
			
			if (status)    { luaL_error(state, "Error (%d) finding record name: %s\n", status, name.c_str()); }
			
			return new _record(name);
		}
		
		static void destroy(_record* instance)    { delete instance; }
		std::string name()    { return this->_name; }
		std::string type()    { return this->_type; }
		
		void setField(lua_State* state)
		{
			lua_settop(state, 3);
			
			std::string fieldname = LuaStack<std::string>::get(state, 2);
			std::string value = LuaStack<std::string>::get(state, 3);
			
			this->processField(state, fieldname, value);
		}
		
		std::string getField(std::string fieldname)
		{
			if (dbFindField(this->record_entry, fieldname.c_str()))
			{
				dbFindRecord(this->record_entry, this->_name.c_str());
				return std::string("");
			}
			
			char* val = dbGetString(this->record_entry);
			dbFindRecord(this->record_entry, this->_name.c_str());
			
			if (val)    { return std::string(val); }
			else        { return std::string(""); }
		}
		
		void setInfo(lua_State* state)
		{
			lua_settop(state, 3);
			
			std::string info = LuaStack<std::string>::get(state, 2);
			std::string value = LuaStack<std::string>::get(state, 3);
			
			
			if (dbPutInfo(this->record_entry, info.c_str(), value.c_str()) )
			{
				dbFindRecord(this->record_entry, this->_name.c_str());
				luaL_error(state, "Error adding info to record %s: %s: %s\n", this->_name.c_str(), info.c_str(), value.c_str());
			}
		
			dbFindRecord(this->record_entry, this->_name.c_str());
		}
		
		void init(lua_State* state)
		{
			auto fields = LuaStack<std::map<std::string, std::string> >::get(state, 2);
			
			for (auto it = fields.begin(); it != fields.end(); it++)
			{
				this->processField(state, it->first, it->second);
			}
		}
};


_entry* genEntry(lua_State* state)      { return _entry::create(state); }


/*
 * Creates or finds a dbrecord instance by calling the luaaa-registered
 * dbrecord.new or dbrecord.find constructor via the Lua C API.
 */
int genRecord(lua_State* state)    
{ 
	int num_params = lua_gettop(state);
	
	lua_getglobal(state, "dbrecord");
	
	if (num_params == 1)
	{
		lua_getfield(state, -1, "find");
		lua_remove(state, -2);
		lua_pushvalue(state, 1);
		lua_call(state, 1, 1);
	}
	else
	{
		lua_getfield(state, -1, "new");
		lua_remove(state, -2);
		lua_pushvalue(state, 1);
		lua_pushvalue(state, 2);
		lua_call(state, 2, 1);
	}
	
	return 1;
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

/*
 * Custom __index for dbrecord objects. First checks for class methods
 * in the metatable (stored by luaaa with '<' prefix), then falls back
 * to reading a record field.
 */
static int l_record_index(lua_State* state)
{
	const char* key = luaL_checkstring(state, 2);
	
	/* Check the metatable for luaaa-registered methods (stored as "<methodname") */
	char method_key[256];
	snprintf(method_key, sizeof(method_key), "<%s", key);
	
	if (lua_getmetatable(state, 1))
	{
		lua_getfield(state, -1, method_key);
		if (lua_isfunction(state, -1))
		{
			lua_remove(state, -2);  /* remove metatable */
			return 1;
		}
		lua_pop(state, 2);  /* pop nil + metatable */
	}
	
	/* Not a method -- treat as a record field name */
	typedef struct { _record* obj; int (*dtor)(void*); void* free_func; } UDD;
	UDD* udd = (UDD*) lua_touserdata(state, 1);
	
	if (!udd || !udd->obj)    { return 0; }
	
	std::string val = udd->obj->getField(std::string(key));
	lua_pushstring(state, val.c_str());
	return 1;
}

/*
 * Custom __newindex for dbrecord objects. Sets a record field value.
 */
static int l_record_newindex(lua_State* state)
{
	const char* key = luaL_checkstring(state, 2);
	const char* val = luaL_checkstring(state, 3);
	
	typedef struct { _record* obj; int (*dtor)(void*); void* free_func; } UDD;
	UDD* udd = (UDD*) lua_touserdata(state, 1);
	
	if (!udd || !udd->obj)    { return luaL_error(state, "Invalid record object"); }
	
	/* Reuse the setField path -- need to set up the stack like setField expects */
	udd->obj->setField(state);
	return 0;
}

/*
 * Helper: converts a Lua table at the given stack index to a
 * comma-separated "key=value,key=value" macro string suitable
 * for dbLoadRecords. If global_macros is non-empty, it is
 * prepended.
 */
static std::string macrosFromTable(lua_State* state, int index, const std::string& global_macros = "")
{
	std::string result = global_macros;
	
	lua_pushnil(state);
	while (lua_next(state, index))
	{
		/* Skip non-string keys (e.g., integer keys from pattern arrays) */
		if (lua_type(state, -2) == LUA_TSTRING)
		{
			const char* key = lua_tostring(state, -2);
			const char* val = lua_tostring(state, -1);
			
			if (val)
			{
				if (!result.empty())    { result += ","; }
				result += std::string(key) + "=" + std::string(val);
			}
		}
		lua_pop(state, 1);
	}
	
	return result;
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
		macros = macrosFromTable(state, 2);
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
		global_macros = macrosFromTable(state, lua_gettop(state));
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
			macros = macrosFromTable(state, entry_idx, global_macros);
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
	
	// Database Record Class
	LuaClass<_record> lua_rw(L, "dbrecord");
	lua_rw.ctor<void, lua_State*>("new", &_record::create, &_record::destroy);
	lua_rw.ctor("find", &_record::find);
	lua_rw.fun("field", &_record::setField);
	lua_rw.fun("info", &_record::setInfo);
	lua_rw.fun("name", &_record::name);
	lua_rw.fun("type", &_record::type);
	lua_rw.fun("__call", &_record::init);
	
	/* Override luaaa's __index/__newindex with custom versions
	 * that fall back to record field access */
	luaL_getmetatable(L, "dbrecord");
	lua_pushcfunction(L, l_record_index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_record_newindex);
	lua_setfield(L, -2, "__newindex");
	lua_pop(L, 1);
	
	// DBENTRY* wrapper and all associated functions
	
	LuaClass<_entry> lua_ew(L, "dbentry");
	lua_ew.ctor<void, lua_State*>("new",&_entry::create, &_entry::destroy);
	
	LuaModule dbmod (L, "db");
	dbmod.fun("entry", genEntry);
	dbmod.fun("record", genRecord);
	dbmod.fun("loadRecords", l_loadRecords);
	dbmod.fun("loadTemplate", l_loadTemplate);
	
	dbmod.fun("list", all_records);
	
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
