#include "devUtil.h"
#include "luaEpics.h"

#include <aaoRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExport.h>

static void pushRecord(struct aaoRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long writeData(struct aaoRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	
	if (!proto)
	{
		recGblSetSevr((dbCommon*) record, WRITE_ALARM, INVALID_ALARM);
		return -1;
	}
	
	LuaStateGuard guard(proto->state);

	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	
	if (runFunction(proto))
	{
		recGblSetSevr((dbCommon*) record, WRITE_ALARM, INVALID_ALARM);
		return -1;
	}
	
	lua_pop(proto->state, 1);
	return 0;
}


static long initRecord (dbCommon* record)
{
	aaoRecord* aao = (aaoRecord*) record;
	
	aao->dpvt = parseINPOUT(&aao->out);
	
	if (!aao->dpvt)
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
    DEVSUPFUN write_aao;
} devLuaAao = {
    5,
    NULL,
    NULL,
    DEVSUPFUN_CAST initRecord,
    NULL,
    DEVSUPFUN_CAST writeData
};

epicsExportAddress(dset, devLuaAao);

}
