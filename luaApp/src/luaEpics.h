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

/*
 * Options-string infrastructure (shared by iocsh/Lua-facing run/load
 * entry points). luaParseOptions parses a "key=value,key=value" string
 * into a table left on top of the stack, using the same coercion rules
 * as macros; the caller owns and must pop the table. luaOptionBool
 * reads a boolean option out of that table with a default. See the
 * .cpp for full semantics.
 */
epicsShareFunc void luaParseOptions(lua_State* state, const char* option_list);
epicsShareFunc int  luaOptionBool(lua_State* state, int index, const char* key, int default_value);

epicsShareFunc int luaLoadLibrary(lua_State* state, const char* lib_name);

epicsShareFunc void luaPushScope(lua_State* state);
epicsShareFunc void luaPopScope(lua_State* state);

epicsShareFunc void luaRegisterFunction(const char* function_name, lua_CFunction function);
epicsShareFunc void luaRegisterLibrary(const char* library_name, lua_CFunction load_func);

/*
 * Emit a one-time-per-name deprecation warning (via errlogPrintf)
 * naming the replacement. Called by deprecated aliases; warns at most
 * once per process for each old name.
 */
epicsShareFunc void luaDeprecated(const char* old_name, const char* replacement);
epicsShareFunc void luaLoadRegistered(lua_State* state);

epicsShareFunc lua_State* luaCreateState();

/*
 * State naming family. luaGetState is get-or-create; luaFindState is
 * lookup-only; luaNameState binds an existing state to a name;
 * luaStateIsNamed is the predicate. "Register" is reserved for
 * extension registration (luaRegisterFunction / luaRegisterLibrary).
 */
epicsShareFunc lua_State* luaGetState(const char* name);
epicsShareFunc lua_State* luaFindState(const char* name);
epicsShareFunc void luaNameState(lua_State* state, const char* name);
epicsShareFunc int  luaStateIsNamed(lua_State* state);

/* Deprecated aliases (deprecation warnings: Stage 7). */
epicsShareFunc lua_State* luaNamedState(const char* name);
epicsShareFunc lua_State* luaFindNamedState(const char* name);
epicsShareFunc void luaRegisterState(lua_State* state, const char* name);
epicsShareFunc int  luaStateIsRegistered(lua_State* state);

epicsShareFunc void luaAddPath(const char* directory);
epicsShareFunc void luaAddModule(const char* module_top);

epicsShareFunc void luaStateRef(lua_State* state);
epicsShareFunc void luaStateUnref(lua_State* state);

epicsShareFunc void luaLockState(lua_State* state);
epicsShareFunc void luaUnlockState(lua_State* state);

/*
 * Line-compilation helpers shared by the shell and the luascript
 * record (both derived from the standard Lua interpreter).
 *
 * luaAddReturn: with a line on the stack, try to compile it as
 *   "return <line>"; on success the compiled chunk replaces the line,
 *   otherwise the original line is left on the stack.
 * luaIncomplete: returns non-zero if 'status' is a syntax error whose
 *   message indicates an incomplete statement (ends with <eof>).
 */
epicsShareFunc int luaAddReturn(lua_State* state);
epicsShareFunc int luaIncomplete(lua_State* state, int status);

#ifdef __cplusplus
}

int l_luaAddPath(lua_State* state);
int l_luaAddModule(lua_State* state);

/*
 * RAII guard for the per-state lock. Locks the state on construction
 * and unlocks on destruction, so every return/exception path releases
 * the lock. Safe on unmanaged/NULL states (luaLockState is a no-op
 * for those).
 */
class LuaStateGuard
{
public:
	LuaStateGuard(lua_State* s) : state(s) { luaLockState(state); }
	~LuaStateGuard() { luaUnlockState(state); }

private:
	lua_State* state;

	LuaStateGuard(const LuaStateGuard&);
	LuaStateGuard& operator=(const LuaStateGuard&);
};

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
