#include "devUtil.h"
#include "luaEpics.h"

#include "lua.h"

#include <string.h>

#include <lsiRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExport.h>

static void pushRecord(struct lsiRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct lsiRecord* record)
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
		case LUA_TSTRING:
		{
			size_t len;
			const char* buffer = lua_tolstring(proto->state, -1, &len);
			
			/* Copy up to sizv-1 bytes, always null-terminate. */
			size_t maxcopy = (record->sizv > 0) ? (size_t)(record->sizv - 1) : 0;
			if (len > maxcopy)    { len = maxcopy; }
			
			memcpy(record->val, buffer, len);
			record->val[len] = '\0';
			record->len = (epicsUInt32)(len + 1);
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
	lsiRecord* lsi = (lsiRecord*) record;
	
	lsi->dpvt = parseINPOUT(&lsi->inp);
	
	if (!lsi->dpvt)
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
    DEVSUPFUN read_string;
} devLuaLsi = {
    5,
    NULL,
    NULL,
    DEVSUPFUN_CAST initRecord,
    NULL,
    DEVSUPFUN_CAST readData
};

epicsExportAddress(dset, devLuaLsi);

}
