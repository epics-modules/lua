#include "devUtil.h"
#include "luaEpics.h"

#include "lua.h"

#include <mbbiDirectRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExport.h>

static void pushRecord(struct mbbiDirectRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct mbbiDirectRecord* record)
{
	int type;
	Protocol* proto = (Protocol*) record->dpvt;
	
	if (!proto)
	{
		recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
		return -1;
	}
	
	LuaStateGuard guard(proto->state);

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
		case LUA_TNUMBER:
		{
			if (! lua_isinteger(proto->state, -1))
			{ 
				lua_pop(proto->state, 1);
				recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
				return -1;
			}
			
			epicsInt32 val = (epicsInt32) lua_tointeger(proto->state, -1);
			record->val = val;
			record->udf = FALSE;
			
			lua_pop(proto->state, 1);
			/* 2 => success, do not convert RVAL->VAL (value set directly) */
			return 2;
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
	mbbiDirectRecord* mbbiDirect = (mbbiDirectRecord*) record;
	
	mbbiDirect->dpvt = parseINPOUT(&mbbiDirect->inp);
	
	if (!mbbiDirect->dpvt)
	{
		recGblSetSevr(record, LINK_ALARM, INVALID_ALARM);
		return -1;
	}
	
	return 0;
}

extern "C"
{

struct {
    long number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read;
} devLuaMbbiDirect = {
    5,
    NULL,
    NULL,
    DEVSUPFUN_CAST initRecord,
    NULL,
    DEVSUPFUN_CAST readData
};

epicsExportAddress(dset, devLuaMbbiDirect);

}
