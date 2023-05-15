#ifndef INC_LEPICSLIB_H
#define INC_LEPICSLIB_H

#import "luaEpics.h"

extern "C"
{
	void luaGeneratePV(lua_State* state, const char* pv_name);
}


#endif
