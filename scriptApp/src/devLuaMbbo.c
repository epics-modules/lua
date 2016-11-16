#include "devUtil.h"

#include <mbboRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long writeData(struct mbboRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	mbboRecord* mbbo = (mbboRecord*) record;
	
	mbbo->dpvt = parseINPOUT(&mbbo->out);
	
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
} devLuaMbbo = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData,
    NULL
};

epicsExportAddress(dset, devLuaMbbo);
