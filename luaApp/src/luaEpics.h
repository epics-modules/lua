#ifndef INC_LUAEPICS_H
#define INC_LUAEPICS_H

#include <shareLib.h>
#include <iocsh.h>
#include <epicsVersion.h>
#include <assert.h>

#ifdef __cplusplus

#ifdef CXX11_SUPPORTED
	#include "luaaa.hpp"
	using namespace luaaa;
#endif

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

#ifdef __cplusplus
}
#endif

#endif
