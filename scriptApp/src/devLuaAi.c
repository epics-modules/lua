#include "devUtil.h"

#include <aiRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long readData(struct aiRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	aiRecord* ai = (aiRecord*) record;
	
	ai->dpvt = parseINPOUT(&ai->inp);
	
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
} devLuaAi = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaAi);
