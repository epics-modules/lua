#ifndef INC_LUAEPICS_H
#define INC_LUAEPICS_H

#include <shareLib.h>
#include <iocsh.h>
#include <epicsVersion.h>

#ifdef __cplusplus

#include <string>

epicsShareFunc std::string luaLocateFile(std::string filename);



extern "C"
{
	
#endif
	
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

/* EPICS base version test.*/
#ifndef EPICS_VERSION_INT
#define VERSION_INT(V,R,M,P) ( ((V)<<24) | ((R)<<16) | ((M)<<8) | (P))
#define EPICS_VERSION_INT VERSION_INT(EPICS_VERSION, EPICS_REVISION, EPICS_MODIFICATION, EPICS_PATCH_LEVEL)
#endif
#define LT_EPICSBASE(V,R,M,P) (EPICS_VERSION_INT < VERSION_INT((V),(R),(M),(P)))
#define GE_EPICSBASE(V,R,M,P) (EPICS_VERSION_INT >= VERSION_INT((V),(R),(M),(P)))
	
epicsShareFunc int  luaLoadScript(lua_State* state, const char* script_file);
epicsShareFunc int  luaLoadString(lua_State* state, const char* lua_code);
epicsShareFunc int  luaLoadParams(lua_State* state, const char* param_list);
epicsShareFunc void luaLoadMacros(lua_State* state, const char* macro_list);

epicsShareFunc void luaRegisterLibrary(const char* library_name, lua_CFunction load_func);
epicsShareFunc void luaLoadRegistered(lua_State* state);

epicsShareFunc lua_State* luaCreateState();

#if GE_EPICSBASE(3,15,6,0)
epicsShareFunc void luaEpicsLibrary(lua_State* state, const iocshCmdDef*);
#endif

#ifdef __cplusplus
}
#endif

#endif
