#ifndef INC_LUAEPICS_H
#define INC_LUAEPICS_H

extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <string>
#include <shareLib.h>

epicsShareFunc std::string luaLocateFile(std::string filename);
epicsShareFunc int luaLoadScript(lua_State* state, const char* script_file);
epicsShareFunc int luaLoadString(lua_State* state, const char* lua_code);
epicsShareFunc int  luaLoadParams(lua_State* state, const char* param_list);
epicsShareFunc void luaLoadMacros(lua_State* state, const char* macro_list);
epicsShareFunc void luaLoadEnviron(lua_State* state);
#endif
