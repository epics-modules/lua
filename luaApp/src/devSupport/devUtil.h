#ifndef INC_DEVUTIL_H
#define INC_DEVUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <link.h>
#include <lua.h>

typedef struct Protocol
{
	lua_State*  state;
	char filename[62];
	char function_name[62];
	char portname[62];
	char param_list[62];
} Protocol;

Protocol* parseINPOUT(const struct link* inpout);

void runFunction(Protocol* proto);
void luaGeneratePV(lua_State* state, const char* pv_name);
void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param);

#ifdef __cplusplus
}
#endif

#endif
