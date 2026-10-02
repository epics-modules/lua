#ifndef INC_DEVUTIL_H
#define INC_DEVUTIL_H

#include "lepicslib.h"
#include <link.h>
#include <lua.h>

#ifdef __cplusplus
/* Device support is compiled as C++; DEVSUPFUN assignments need an
 * explicit cast (parallels RECSUPFUN_CAST used by the record). */
#include <devSup.h>
#define DEVSUPFUN_CAST (DEVSUPFUN)
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

/*
 * Fill an array record's buffer (bptr) from a Lua table at stack index
 * `idx`, dispatching on the record's FTVL. Reads up to `nelm` elements,
 * stopping at the first nil (standard Lua sequence semantics). Returns
 * the number of elements written (suitable for NORD). Returns -1 if the
 * value at `idx` is not a table.
 */
long luaTableToBuffer(lua_State* state, int idx, short ftvl, void* bptr, long nelm);

/*
 * Push a Lua table (left at the top of the stack) built from `count`
 * elements of an array record's buffer (bptr), dispatching on FTVL.
 */
void luaBufferToTable(lua_State* state, short ftvl, const void* bptr, long count);

#ifdef __cplusplus
}
#endif

#endif
