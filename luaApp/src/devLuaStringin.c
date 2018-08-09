#include "devUtil.h"

#include "lua.h"

#include <stringinRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>
#include <string.h>

static void pushRecord(struct stringinRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct stringinRecord* record)
{
	int type;
	Protocol* proto = (Protocol*) record->dpvt;
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);
	
	type = lua_type(proto->state, -1);
	
	switch (type)
	{		
		case LUA_TSTRING:
		{
			const char* temp = lua_tostring(proto->state, -1);
			lua_pop(proto->state, 1);
			
			strcpy(record->val, temp);
			return 2;
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
	stringinRecord* stringin = (stringinRecord*) record;
	
	stringin->dpvt = parseINPOUT(&stringin->inp);
	
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
} devLuaStringin = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaStringin);
