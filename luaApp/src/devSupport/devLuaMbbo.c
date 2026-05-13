#include "devUtil.h"

#include <mbboRecord.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recGbl.h>
#include <alarm.h>
#include <epicsExport.h>

static void pushRecord(struct mbboRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long writeData(struct mbboRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	
	if (!proto)
	{
		recGblSetSevr((dbCommon*) record, WRITE_ALARM, INVALID_ALARM);
		return -1;
	}
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);

	lua_pop(proto->state, 1);
	return 0;
}


static long initRecord (dbCommon* record)
{
	mbboRecord* mbbo = (mbboRecord*) record;
	
	mbbo->dpvt = parseINPOUT(&mbbo->out);
	
	if (!mbbo->dpvt)
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
    DEVSUPFUN write;
} devLuaMbbo = {
    5,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData
};

epicsExportAddress(dset, devLuaMbbo);
