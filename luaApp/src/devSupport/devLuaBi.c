#include "devUtil.h"

#include "lua.h"

#include <biRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>
#include <string.h>

static void pushRecord(struct biRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct biRecord* record)
{
	int type;
	Protocol* proto = (Protocol*) record->dpvt;
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);
	
	type = lua_type(proto->state, -1);
	
	switch (type)
	{		
		case LUA_TNUMBER:
		{
			if (! lua_isinteger(proto->state, -1))
			{
				lua_pop(proto->state, 1);
				return -1;
			}
			else
			{
			    int val = lua_tointeger(proto->state, -1);
			
			    if (record->mask) val &= record->mask;
			
			    record->rval = val;
			
			    lua_pop(proto->state, 1);
			    return 2;
			}
		}
		
		case LUA_TSTRING:
		{
			const char* buffer = lua_tostring(proto->state, -1);
			lua_pop(proto->state, 1);
			
			if (strcmp(record->znam, buffer) == 0)
			{
				record->val = 0;
				return 2;
			}
			
			if (strcmp(record->onam, buffer) == 0)
			{
				record->val = 1;
				return 2;
			}
			
			return -1;
		}
		
		case LUA_TNIL:
			lua_pop(proto->state, 1);
			return 0;
		
		default:
			lua_pop(proto->state, 1);
			return -1;
	}
	
	return 0;
}


static long initRecord (dbCommon* record)
{
	biRecord* bi = (biRecord*) record;
	
	bi->dpvt = parseINPOUT(&bi->inp);
	
	return 0;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
    DEVSUPFUN special_linconv;
} devLuaBi = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaBi);
