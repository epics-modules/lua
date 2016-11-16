#include "devUtil.h"

#include <aoRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long writeData(struct aoRecord* record)
{
	return runScript(record->dpvt);
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
