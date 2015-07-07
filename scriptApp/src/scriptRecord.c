#include <dbAccess.h>
#include <recSup.h>
#include <recGbl.h>

#include <epicsExport.h>

#define GEN_SIZE_OFFSET
#include "scriptRecord.h"
#undef  GEN_SIZE_OFFSET

#include "scriptUtil.h"

#include <stdlib.h>

#include <stdio.h>

static long init_record();
static long process();
static long special();
static long get_precision();

rset scriptRSET =
{
	RSETNUMBER,
	NULL,
	NULL,
	init_record,
	process,
	special,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	get_precision,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

epicsExportAddress(rset, scriptRSET);

static long init_record(void* data, int pass)
{	
	scriptRecord* record = (scriptRecord*) data;
	
	if (pass == 0)
	{
		record->pcode = (char *) calloc(121, sizeof(char));
		
		int index;
		char** init_str = (char**) &record->paa;
		
		for (index = 0; index < STR_ARGS; index += 1)
		{
			*init_str  = (char *) calloc(STRING_SIZE, sizeof(char));
			init_str++;
		}
	
		if (initState(record))
		{
			logError(record);
			return -1;
		}
		
		return 0;
	}

	setLinks(record);

	return 0;
}

static long process(void* data)
{
	scriptRecord* record = (scriptRecord*) data;

	return startProc(record);
}

static long special(dbAddr *paddr, int after)
{
	speci(paddr, after);
	return 0;
}

static long get_precision(dbAddr *paddr, long *precision)
{
	scriptRecord *record = (scriptRecord *) paddr->precord;
	int index = dbGetFieldIndex(paddr);

	*precision = record->prec;
	
	if (index >= scriptRecordVAL0 && index <= scriptRecordVAL9) return 0;
		
	recGblGetPrec(paddr, precision);
	return 0;
}
