#ifndef INC_LEPICSLIB_H
#define INC_LEPICSLIB_H

#include "luaEpics.h"

#ifdef CXX11_SUPPORTED

#endif


extern "C"
{
	void luaGeneratePV(lua_State* state, const char* pv_name);
}


#endif
