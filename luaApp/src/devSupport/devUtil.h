#ifndef INC_DEVUTIL_H
#define INC_DEVUTIL_H

#include "lepicslib.h"
#include <link.h>
#include <lua.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Protocol
{
	lua_State*  state;
	char filename[256];
	char function_name[256];
	char portname[256];
	char param_list[256];
} Protocol;

Protocol* parseINPOUT(const struct link* inpout);

int runFunction(Protocol* proto);

#ifdef __cplusplus
}
#endif

#endif
