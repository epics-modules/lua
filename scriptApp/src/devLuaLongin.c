#include "devUtil.h"

#include <longinRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long readData(struct longinRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	longinRecord* longin = (longinRecord*) record;
	
	longin->dpvt = parseINPOUT(&longin->inp);
	
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
} devLuaLongin = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaLongin);
