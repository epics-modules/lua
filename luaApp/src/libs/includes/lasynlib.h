#ifndef INC_LASYNLIB_H
#define INC_LASYNLIB_H

#include "luaEpics.h"

#ifdef __cplusplus

#include <string>
#include <asynPortDriver.h>
#include <asynPortClient.h>

extern "C" {
#endif
	
	void luaGenerateDriver(lua_State* state, const char* port_name);
	void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param);
	
#ifdef __cplusplus
}
#endif

#endif
