#include "devUtil.h"

#include "lua.h"

#include <biRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <string.h>
#include <epicsExport.h>

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
	
	if (!proto)
	{
		recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
		return -1;
	}
	
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
				recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
				return -1;
			}
			else
			{
			    int val = lua_tointeger(proto->state, -1);
			
			    if (record->mask) val &= record->mask;
			
		    record->rval = val;
		    record->udf = FALSE;
		
		    lua_pop(proto->state, 1);
		    return 0;
			}
		}
		
		case LUA_TSTRING:
		{
			const char* buffer = lua_tostring(proto->state, -1);
			lua_pop(proto->state, 1);
			
			if (strcmp(record->znam, buffer) == 0)
			{
				record->val = 0;
				record->udf = FALSE;
				return 2;
			}
			
			if (strcmp(record->onam, buffer) == 0)
			{
				record->val = 1;
				record->udf = FALSE;
				return 2;
			}
			
			recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
			return -1;
		}
		
		case LUA_TNIL:
			lua_pop(proto->state, 1);
			return 0;
		
		default:
			lua_pop(proto->state, 1);
			recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
			return -1;
	}
	
	return 0;
}


static long initRecord (dbCommon* record)
{
	biRecord* bi = (biRecord*) record;
	
	bi->dpvt = parseINPOUT(&bi->inp);
	
	if (!bi->dpvt)
	{
		recGblSetSevr(record, LINK_ALARM, INVALID_ALARM);
		return -1;
	}
	
	return 0;
}

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
} devLuaBi = {
    5,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData
};

epicsExportAddress(dset, devLuaBi);
