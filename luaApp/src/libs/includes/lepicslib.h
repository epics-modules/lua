#ifndef INC_LEPICSLIB_H
#define INC_LEPICSLIB_H

#include "luaEpics.h"

#ifdef CXX11_SUPPORTED

#endif

#ifdef __cplusplus
extern "C"
{
#endif
	
	void luaGeneratePV(lua_State* state, const char* pv_name);
	
#ifdef __cplusplus
}
#endif

#endif
