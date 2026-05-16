#include "devUtil.h"

#include "lua.h"

#include <stringinRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <string.h>
#include <epicsExport.h>

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
	
	if (!proto)
	{
		recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
		return -1;
	}
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	
	if (runFunction(proto))
	{
		recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
		return -1;
	}
	
	type = lua_type(proto->state, -1);
	
	switch (type)
	{		
		case LUA_TSTRING:
		{
			const char* temp = lua_tostring(proto->state, -1);
			
			strncpy(record->val, temp, sizeof(record->val) - 1);
			record->val[sizeof(record->val) - 1] = '\0';
			record->udf = FALSE;
			lua_pop(proto->state, 1);
			return 0;
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
	stringinRecord* stringin = (stringinRecord*) record;
	
	stringin->dpvt = parseINPOUT(&stringin->inp);
	
	if (!stringin->dpvt)
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
} devLuaStringin = {
    5,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData
};

epicsExportAddress(dset, devLuaStringin);
