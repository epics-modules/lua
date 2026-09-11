#ifndef INC_LUASHELL_H
#define INC_LUASHELL_H

#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Canonical run/load family. All share the (target, macros, options)
 * shape; macros and options are key=value strings (see luaEpics.h /
 * luaParseOptions). See luaShell.cpp for full semantics.
 *
 *   luaRunString - run a code string as one chunk, print final result
 *   luaRunFile   - run a file as one chunk; option "async=true" runs it
 *                  in a background thread, otherwise synchronously
 *   luaShell     - line-by-line REPL execution (NULL pathname = interactive)
 */
epicsShareFunc int epicsShareAPI luaRunString(const char* code, const char* macros, const char* options);
epicsShareFunc int epicsShareAPI luaRunFile(const char* filename, const char* macros, const char* options);
epicsShareFunc int epicsShareAPI luaShell(const char* pathname, const char* macros);

/* Deprecated names retained as aliases (deprecation warnings: Stage 7). */
epicsShareFunc int epicsShareAPI luash(const char* pathname);
epicsShareFunc int epicsShareAPI luashLoad(const char* pathname, const char* macros);
epicsShareFunc int epicsShareAPI luaCmd(const char* command, const char* macros);
epicsShareFunc int epicsShareAPI luaSpawn(const char* pathname, const char* macros);
epicsShareFunc int epicsShareAPI luaLoadFile(const char* filename, const char* macros);

epicsShareFunc void epicsShareAPI luashSetCommonState(const char* name);

#ifdef __cplusplus

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

}

epicsShareFunc int epicsShareAPI luash(lua_State* state, const char* pathname);
epicsShareFunc int epicsShareAPI luash(lua_State* state, const char* pathname, const char* macros);

epicsShareFunc int epicsShareAPI luaCmd(lua_State* state, const char* command, const char* macros);

/* Canonical Lua-callable wrappers */
int l_luaRunString(lua_State* state);
int l_luaRunFile(lua_State* state);
int l_luaShell(lua_State* state);

/* Deprecated Lua-callable wrappers */
int l_luaSpawn(lua_State* state);
int l_luash(lua_State* state);
int l_luaCmd(lua_State* state);
int l_luaLoadFile(lua_State* state);
#endif


#endif
