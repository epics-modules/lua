#include "devUtil.h"
#include "luaEpics.h"

#include "lua.h"

#include <waveformRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExport.h>

static void pushRecord(struct waveformRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct waveformRecord* record)
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
		case LUA_TTABLE:
		{
			long count = luaTableToBuffer(proto->state, -1, record->ftvl,
			                              record->bptr, record->nelm);
			lua_pop(proto->state, 1);
			
			if (count < 0)
			{
				recGblSetSevr((dbCommon*) record, READ_ALARM, INVALID_ALARM);
				return -1;
			}
			
			record->nord = (epicsUInt32) count;
			record->udf = FALSE;
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
	waveformRecord* wf = (waveformRecord*) record;
	
	wf->dpvt = parseINPOUT(&wf->inp);
	
	if (!wf->dpvt)
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
    DEVSUPFUN read_wf;
} devLuaWaveform = {
    5,
    NULL,
    NULL,
    DEVSUPFUN_CAST initRecord,
    NULL,
    DEVSUPFUN_CAST readData
};

epicsExportAddress(dset, devLuaWaveform);

}
