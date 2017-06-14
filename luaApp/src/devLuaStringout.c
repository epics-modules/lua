#include "devUtil.h"

#include <stringoutRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static void pushRecord(struct stringoutRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long writeData(struct stringoutRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);
	
	lua_pop(proto->state, 1);
	return 0;
}


static long initRecord (dbCommon* record)
{
	stringoutRecord* stringout = (stringoutRecord*) record;
	
	stringout->dpvt = parseINPOUT(&stringout->out);
	
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
} devLuaStringout = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData,
    NULL
};

epicsExportAddress(dset, devLuaStringout);
