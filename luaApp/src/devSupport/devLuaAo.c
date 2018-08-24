#include "devUtil.h"

#include "lua.h"

#include <aoRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>


static void pushRecord(struct aoRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long writeData(struct aoRecord* record)
{
	int type;
	double val = record->oval - record->aoff;
	Protocol* proto = (Protocol*) record->dpvt;

	if (record->aslo != 0)    { val /= record->aslo; }
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);
	
	type = lua_type(proto->state, -1);
	
	switch (type)
	{
		case LUA_TNIL:
			lua_pop(proto->state, 1);
			return 2;
			
		default:
			lua_pop(proto->state, 1);
			return -1;
	}
	
	return 0;
}


static long initRecord (dbCommon* record)
{
	aoRecord* ao = (aoRecord*) record;
	
	ao->dpvt = parseINPOUT(&ao->out);
	
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
} devLuaAo = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData,
    NULL
};

epicsExportAddress(dset, devLuaAo);
