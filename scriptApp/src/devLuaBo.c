#include "devUtil.h"

#include <boRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long writeData(struct boRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	boRecord* bo = (boRecord*) record;
	
	bo->dpvt = parseINPOUT(&bo->out);
	
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
} devLuaBo = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData,
    NULL
};

epicsExportAddress(dset, devLuaBo);
