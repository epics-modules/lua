#ifndef INCEPICSSCRIPT_H
#define INCEPICSSCRIPT_H

extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

int luaLoadScript(lua_State* state, const char* script_file);
int luaLoadString(lua_State* state, const char* lua_code);
void luaLoadParams(lua_State* state, const char* param_list);
#endif
