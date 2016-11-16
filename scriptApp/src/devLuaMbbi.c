#include "devUtil.h"

#include <mbbiRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static long readData(struct mbbiRecord* record)
{
	return runScript(record->dpvt);
}


static long initRecord (dbCommon* record)
{
	mbbiRecord* mbbi = (mbbiRecord*) record;
	
	mbbi->dpvt = parseINPOUT(&mbbi->inp);
	
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
} devLuaMbbi = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    readData,
    NULL
};

epicsExportAddress(dset, devLuaMbbi);
