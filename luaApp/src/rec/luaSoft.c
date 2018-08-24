#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <callback.h>
#include <dbCa.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "link.h"
#include "special.h"
#include "luascriptRecord.h"
#include "epicsExport.h"
#include <dbEvent.h>

static long write_Script(luascriptRecord *record);

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write;
} devLuaSoft = {
	5,
	NULL,
	NULL,
	NULL,
	NULL,
	write_Script
};

epicsExportAddress(dset,devLuaSoft);

volatile int devLuaSoftDebug = 0;
epicsExportAddress(int, devLuaSoftDebug);

static long asyncWrite(luascriptRecord* record, double* val, char* sval, struct link* out)
{	
	long    status;
	long    n_elements = 1;
	short   field_type = 0;
	
	field_type = dbCaGetLinkDBFtype(out);
	
	if (devLuaSoftDebug)
		{ printf("write_Script: field_type=%d\n", field_type); }
	
	switch (field_type)
	{
		case DBF_STRING: 
		case DBF_ENUM: 
		case DBF_MENU: 
		case DBF_DEVICE:
		case DBF_INLINK: 
		case DBF_OUTLINK: 
		case DBF_FWDLINK: 
			if (devLuaSoftDebug)
				{ printf("write_Script: calling dbCPLCB..DBR_STRING\n"); }
			
			status = dbCaPutLinkCallback(out, DBR_STRING, sval, 1, (dbCaCallback) dbCaCallbackProcess, out);
				
			break;
			
		default:
			dbCaGetNelements(out, &n_elements);
			
			if (n_elements>sizeof(sval))    { n_elements = sizeof(sval); }
			
			if (((field_type==DBF_CHAR) || (field_type==DBF_UCHAR)) && (n_elements>1)) 
			{
				if (devLuaSoftDebug)
					{ printf("write_Script: dbCaPutLinkCallback %ld characters\n", n_elements); }
				
				status = dbCaPutLinkCallback(out, DBF_CHAR, sval, n_elements, (dbCaCallback) dbCaCallbackProcess, out);
			} 
			else 
			{
				if (devLuaSoftDebug)
					{ printf("write_Script: calling dbCPLCB..DBR_DOUBLE\n"); }
				
				status = dbCaPutLinkCallback(out, DBR_DOUBLE, val, 1, (dbCaCallback)dbCaCallbackProcess, out);
			}
			
			break;
	}
	
	if (status) 
	{
		if (devLuaSoftDebug)    { printf("write_Script: dbCPLCB returned error\n"); }
			
		recGblSetSevr(record, LINK_ALARM, INVALID_ALARM);
		
		return status;
	}
	
	record->pact = TRUE;
	
	return status;
}

static long syncWrite(luascriptRecord* record, double* val, char* sval, struct link* out)
{
	long    n_elements = 1;
	short   field_type = 0;
	dbAddr  Addr;
	dbAddr* pAddr = &Addr;
	
	if (out->type == CA_LINK)
	{
		field_type = dbCaGetLinkDBFtype(out);
		dbCaGetNelements(out, &n_elements);
	}
	else if (out->type == DB_LINK)
	{
		if (!dbNameToAddr(out->value.pv_link.pvname, pAddr))
		{
			field_type = pAddr->field_type;
			n_elements = pAddr->no_elements;
		}
	}

	switch (field_type)
	{
		case DBF_STRING: 
		case DBF_ENUM: 
		case DBF_MENU: 
		case DBF_DEVICE:
		case DBF_INLINK: 
		case DBF_OUTLINK: 
		case DBF_FWDLINK:
			return dbPutLink(out, DBR_STRING, sval, 1);
			
		default:
			if (n_elements > sizeof(sval))    { n_elements = sizeof(sval); }
			
			if (((field_type == DBF_CHAR) || (field_type == DBF_UCHAR)) && (n_elements > 1))
			{
				if (devLuaSoftDebug) 
					{ printf("write_Script: dbPutLink %ld characters\n", n_elements); }
				
				return dbPutLink(out, DBF_CHAR, sval, n_elements);
			} 
			else 
			{
				return dbPutLink(out, DBR_DOUBLE, val, 1);
			}
	}
}

static long write_Script(luascriptRecord* record)
{	
	long status = 0;
	
	double* val  = &record->val;
	char*   sval = (char*) &record->sval;
	
	struct link* out = &record->out;

	if (devLuaSoftDebug)    { printf("write_Script: pact=%d\n", record->pact); }
	
	if (record->pact)    { return 0; }
	
	if ((out->type == CA_LINK) && (record->wait))
	{
		status = asyncWrite(record, val, sval, out);
		if (status)    { return status; }
	}
	else
	{
		status = syncWrite(record, val, sval, out);
		if (status)    { return status; }
	}
	
	return 0;
}
