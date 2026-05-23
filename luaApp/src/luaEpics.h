#ifndef INC_LUAEPICS_H
#define INC_LUAEPICS_H

#include <shareLib.h>
#include <iocsh.h>
#include <epicsVersion.h>
#include <assert.h>

#ifdef __cplusplus

#include <string>

epicsShareFunc std::string luaLocateFile(std::string filename);



extern "C"
{
	
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


typedef void (*LUA_LIBRARY_LOAD_HOOK_ROUTINE)(const char* library_name, lua_CFunction library_func);
typedef void (*LUA_FUNCTION_LOAD_HOOK_ROUTINE)(const char* function_name, lua_CFunction function);

epicsShareExtern LUA_LIBRARY_LOAD_HOOK_ROUTINE luaLoadLibraryHook;
epicsShareExtern LUA_FUNCTION_LOAD_HOOK_ROUTINE luaLoadFunctionHook;

epicsShareFunc int  luaLoadScript(lua_State* state, const char* script_file);
epicsShareFunc int  luaLoadString(lua_State* state, const char* lua_code);
epicsShareFunc int  luaLoadParams(lua_State* state, const char* param_list);
epicsShareFunc void luaLoadMacros(lua_State* state, const char* macro_list);

epicsShareFunc int luaLoadLibrary(lua_State* state, const char* lib_name);

epicsShareFunc void luaPushScope(lua_State* state);
epicsShareFunc void luaPopScope(lua_State* state);

epicsShareFunc void luaRegisterFunction(const char* function_name, lua_CFunction function);
epicsShareFunc void luaRegisterLibrary(const char* library_name, lua_CFunction load_func);
epicsShareFunc void luaLoadRegistered(lua_State* state);

epicsShareFunc lua_State* luaCreateState();
epicsShareFunc lua_State* luaNamedState(const char* name);
epicsShareFunc lua_State* luaFindNamedState(const char* name);
epicsShareFunc void luaRegisterState(lua_State* state, const char* name);
epicsShareFunc int  luaStateIsRegistered(lua_State* state);

#ifdef __cplusplus
}

epicsShareFunc std::string luaMacrosFromTable(lua_State* state, int index);

/*
 * Database field type mapping.
 *
 * The native field_type values from dbFldTypes.h conflict with the
 * DBF_* macros from db_access.h (included by cadef.h) -- they use
 * different numeric values for the same names. These constants and
 * the dbf_to_lua_type mapping resolve this by using the known numeric
 * values from dbFldTypes.h directly.
 */

/* Database-side DBR request type values from dbFldTypes.h */
#define DB_DBR_STRING   0
#define DB_DBR_CHAR     1
#define DB_DBR_LONG     5
#define DB_DBR_DOUBLE  10

enum db_lua_type {
	DB_LUA_STRING,
	DB_LUA_CHAR,
	DB_LUA_INTEGER,
	DB_LUA_DOUBLE,
	DB_LUA_ENUM,
	DB_LUA_UNKNOWN
};

/*
 * Map the native database field_type (dbFldTypes.h enum values)
 * to a Lua type category.
 *
 * dbFldTypes.h enum:
 *   0=STRING, 1=CHAR, 2=UCHAR, 3=SHORT, 4=USHORT,
 *   5=LONG, 6=ULONG, 7=INT64, 8=UINT64, 9=FLOAT,
 *   10=DOUBLE, 11=ENUM, 12=MENU, 13=DEVICE
 */
static inline enum db_lua_type dbf_to_lua_type(short field_type)
{
	switch (field_type)
	{
		case 0:              return DB_LUA_STRING;
		case 1:  case 2:     return DB_LUA_CHAR;
		case 3:  case 4:     return DB_LUA_INTEGER;
		case 5:  case 6:     return DB_LUA_INTEGER;
		case 7:  case 8:     return DB_LUA_DOUBLE;
		case 9:  case 10:    return DB_LUA_DOUBLE;
		case 11: case 12:
		case 13:             return DB_LUA_ENUM;
		default:             return DB_LUA_UNKNOWN;
	}
}

#endif

#endif
