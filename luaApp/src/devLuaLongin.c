#include "devUtil.h"

#include "lua.h"

#include <longinRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static void pushRecord(struct longinRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct longinRecord* record)
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
				long val = lua_tointeger(proto->state, -1);
				record->val = val;
			
				lua_pop(proto->state, 1);
				return 2;
			}
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
	longinRecord* longin = (longinRecord*) record;
	
	longin->dpvt = parseINPOUT(&longin->inp);
	
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
} devLuaLongin = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaLongin);
