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

epicsShareFunc void luaAddPath(const char* directory);
epicsShareFunc void luaAddModule(const char* module_top);

epicsShareFunc void luaStateRef(lua_State* state);
epicsShareFunc void luaStateUnref(lua_State* state);

#ifdef __cplusplus
}

int l_luaAddPath(lua_State* state);
int l_luaAddModule(lua_State* state);

epicsShareFunc std::string luaMacrosFromTable(lua_State* state, int index);

/*
 * Database field type mapping.
 *
 * The native field_type values from dbFldTypes.h conflict with the
 * DBF_* macros from db_access.h (included by cadef.h) -- they use
 * different numeric values for the same names. These DB_DBR_*
 * constants use the dbFldTypes.h numbering for dbGetField/dbPutField.
 *
 * Base 7.0 inserted INT64/UINT64 at positions 7-8, shifting
 * FLOAT/DOUBLE/ENUM by 2 relative to 3.15.
 *
 * dbFldTypes.h enum (3.15):
 *   0=STRING, 1=CHAR, 2=UCHAR, 3=SHORT, 4=USHORT,
 *   5=LONG, 6=ULONG, 7=FLOAT, 8=DOUBLE, 9=ENUM,
 *   10=MENU, 11=DEVICE
 *
 * dbFldTypes.h enum (7.0+):
 *   0=STRING, 1=CHAR, 2=UCHAR, 3=SHORT, 4=USHORT,
 *   5=LONG, 6=ULONG, 7=INT64, 8=UINT64, 9=FLOAT,
 *   10=DOUBLE, 11=ENUM, 12=MENU, 13=DEVICE
 */

/* Database-side request type values from dbFldTypes.h */
#define DB_DBR_STRING   0
#define DB_DBR_CHAR     1
#define DB_DBR_LONG     5

#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 0, 0)
#define DB_DBR_DOUBLE  10
#else
#define DB_DBR_DOUBLE   8
#endif

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
 * to a Lua type category. Version-guarded for the INT64/UINT64
 * shift between base 3.15 and 7.0.
 */
static inline enum db_lua_type dbf_to_lua_type(short field_type)
{
	switch (field_type)
	{
		case 0:              return DB_LUA_STRING;   /* STRING */
		case 1:  case 2:     return DB_LUA_CHAR;     /* CHAR, UCHAR */
		case 3:  case 4:     return DB_LUA_INTEGER;  /* SHORT, USHORT */
		case 5:  case 6:     return DB_LUA_INTEGER;  /* LONG, ULONG */
#if EPICS_VERSION_INT >= VERSION_INT(7, 0, 0, 0)
		case 7:  case 8:     return DB_LUA_DOUBLE;   /* INT64, UINT64 */
		case 9:  case 10:    return DB_LUA_DOUBLE;   /* FLOAT, DOUBLE */
		case 11: case 12:
		case 13:             return DB_LUA_ENUM;     /* ENUM, MENU, DEVICE */
#else
		case 7:  case 8:     return DB_LUA_DOUBLE;   /* FLOAT, DOUBLE */
		case 9:  case 10:
		case 11:             return DB_LUA_ENUM;     /* ENUM, MENU, DEVICE */
#endif
		default:             return DB_LUA_UNKNOWN;
	}
}

#endif

#endif
