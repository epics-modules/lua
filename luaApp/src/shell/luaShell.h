#ifndef INC_LUASHELL_H
#define INC_LUASHELL_H

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif


epicsShareFunc int epicsShareAPI luash(const char* pathname);
epicsShareFunc int epicsShareAPI luashLoad(const char* pathname, const char* macros);
epicsShareFunc int epicsShareAPI luaCmd(const char* command, const char* macros);
epicsShareFunc int epicsShareAPI luaSpawn(const char* pathname, const char* macros);

epicsShareFunc void epicsShareAPI luashSetCommonState(const char* name);

#ifdef __cplusplus

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

}

epicsShareFunc int epicsShareAPI luash(lua_State* state, const char* pathname);
epicsShareFunc int epicsShareAPI luash(lua_State* state, const char* pathname, const char* macros);

epicsShareFunc int epicsShareAPI luaCmd(lua_State* state, const char* command, const char* macros);
#endif


#endif
