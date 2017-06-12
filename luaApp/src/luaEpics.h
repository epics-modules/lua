#ifndef INC_LUAEPICS_H
#define INC_LUAEPICS_H

extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <string>

std::string luaLocateFile(std::string filename);
int luaLoadScript(lua_State* state, const char* script_file);
int luaLoadString(lua_State* state, const char* lua_code);
int  luaLoadParams(lua_State* state, const char* param_list);
void luaLoadMacros(lua_State* state, const char* macro_list);
void luaLoadEnviron(lua_State* state);
#endif
