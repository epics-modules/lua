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
	char filename[62];
	char function_name[62];
	char portname[62];
	char param_list[62];
} Protocol;

Protocol* parseINPOUT(const struct link* inpout);

void runFunction(Protocol* proto);

#ifdef __cplusplus
}
#endif

#endif
