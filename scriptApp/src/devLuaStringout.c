#include "devUtil.h"

#include <stringoutRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long writeData(struct stringoutRecord* record)
{
	return runScript(record->dpvt);
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
