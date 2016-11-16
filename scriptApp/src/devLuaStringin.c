#include "devUtil.h"

#include <stringinRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long readData(struct stringinRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	stringinRecord* stringin = (stringinRecord*) record;
	
	stringin->dpvt = parseINPOUT(&stringin->inp);
	
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
} devLuaStringin = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaStringin);
