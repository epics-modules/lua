#include "devUtil.h"

#include <longoutRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long writeData(struct longoutRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	longoutRecord* longout = (longoutRecord*) record;
	
	longout->dpvt = parseINPOUT(&longout->out);
	
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
} devLuaLongout = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData,
    NULL
};

epicsExportAddress(dset, devLuaLongout);
