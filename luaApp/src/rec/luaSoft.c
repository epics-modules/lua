#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <callback.h>
#include <dbCa.h>
#include <errlog.h>

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
	dbCaGetNelements(out, &n_elements);

	if (devLuaSoftDebug)
		{ printf("write_Script: field_type=%d\n", field_type); }

	int bytes_per_elem = 1;
	
	if (record->atyp == luascriptAVALType_Integer)   { bytes_per_elem = sizeof(int); }
	if (record->atyp == luascriptAVALType_Double)    { bytes_per_elem = sizeof(double); }
	if (record->atyp == luascriptAVALType_Char)      { bytes_per_elem = sizeof(char); }
	
	
	int array_len = (int) record->asiz / bytes_per_elem;

	
	if (n_elements > 1)
	{
		int index;
		
		if (array_len > n_elements) 
		{
			errlogPrintf("%s: Array too large to write to output\n", record->name);
			return 0;
		}

		switch (field_type)
		{
			case DBF_CHAR:
			case DBF_UCHAR:
			{				
				if (record->atyp == luascriptAVALType_Char)
				{
					return dbCaPutLinkCallback(out, DBF_CHAR, record->aval, array_len, (dbCaCallback) dbCaCallbackProcess, out);

				}
				else if (record->atyp == luascriptAVALType_Integer)
				{
					#if defined(_MSC_VER)
						char buffer[1024];
					#else
						char buffer[array_len];
					#endif

					for (index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = (char) ((int*) record->aval)[index];
					}

					return dbCaPutLinkCallback(out, DBF_CHAR, buffer, array_len, (dbCaCallback) dbCaCallbackProcess, out);
				}

				return 0;
			}

			case DBF_DOUBLE:
			{
				if (record->atyp == luascriptAVALType_Double)
				{
					return dbCaPutLinkCallback(out, DBF_DOUBLE, record->aval, array_len, (dbCaCallback) dbCaCallbackProcess, out);
				}
				else if (record->atyp == luascriptAVALType_Integer)
				{
					#if defined(_MSC_VER)
						double buffer[1024];
					#else
						double buffer[array_len];
					#endif

					for (index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = ((int*) record->aval)[index];
					}

					return dbCaPutLinkCallback(out, DBF_DOUBLE, buffer, array_len, (dbCaCallback) dbCaCallbackProcess, out);
				}

				return 0;
			}

			case DBF_FLOAT:
			{
				#if defined(_MSC_VER)
					float buffer[1024];
				#else
					float buffer[array_len];
				#endif

				if (record->atyp == luascriptAVALType_Double)
				{
					for(index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = (float) ((double*) record->aval)[index];
					}

					return dbCaPutLinkCallback(out, DBF_FLOAT, buffer, array_len, (dbCaCallback) dbCaCallbackProcess, out);
				}
				else if(record->atyp == luascriptAVALType_Integer)
				{
					for(index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = (float) ((int*) record->aval)[index];
					}

					return dbCaPutLinkCallback(out, DBF_FLOAT, buffer, array_len, (dbCaCallback) dbCaCallbackProcess, out);
				}

				return 0;
			}

			default:
			{
				if (record->atyp == luascriptAVALType_Integer)
				{
					dbCaPutLinkCallback(out, DBF_LONG, record->aval, array_len, (dbCaCallback) dbCaCallbackProcess, out);
				}

				return 0;
			}
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
	
	int bytes_per_elem = 1;
	
	if (record->atyp == luascriptAVALType_Integer)   { bytes_per_elem = sizeof(int); }
	if (record->atyp == luascriptAVALType_Double)    { bytes_per_elem = sizeof(double); }
	if (record->atyp == luascriptAVALType_Char)      { bytes_per_elem = sizeof(char); }
	
	
	int array_len = (int) record->asiz / bytes_per_elem;
	
	if (n_elements > 1)
	{
		int index;
		
		if (array_len > n_elements) 
		{
			errlogPrintf("%s: Array too large to write to output\n", record->name);
			return 0;
		}
		
		switch (field_type)
		{
			case DBF_CHAR:
			case DBF_UCHAR:
			{				
				if (record->atyp == luascriptAVALType_Char)
				{
					return dbPutLink(out, DBF_CHAR, record->aval, array_len);
				}
				else if (record->atyp == luascriptAVALType_Integer)
				{
					#if defined(_MSC_VER)
						char buffer[1024];
					#else
						char buffer[array_len];
					#endif

					for (index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = (char) ((int*) record->aval)[index];
					}

					return dbPutLink(out, DBF_CHAR, buffer, array_len);
				}

				return 0;
			}

			case DBF_DOUBLE:
			{
				if (record->atyp == luascriptAVALType_Double)
				{
					return dbPutLink(out, DBF_DOUBLE, record->aval, array_len);
				}
				else if (record->atyp == luascriptAVALType_Integer)
				{
					#if defined(_MSC_VER)
						double buffer[1024];
					#else
						double buffer[array_len];
					#endif

					for (index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = ((int*) record->aval)[index];
					}

					return dbPutLink(out, DBF_DOUBLE, buffer, array_len);
				}

				return 0;
			}

			case DBF_FLOAT:
			{
				#if defined(_MSC_VER)
					float buffer[1024];
				#else
					float buffer[array_len];
				#endif

				if (record->atyp == luascriptAVALType_Double)
				{
					for(index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = (float) ((double*) record->aval)[index];
					}

					return dbPutLink(out, DBF_FLOAT, buffer, array_len);
				}
				else if(record->atyp == luascriptAVALType_Integer)
				{
					for(index = 0; index < n_elements && index < array_len; index += 1)
					{
						buffer[index] = (float) ((int*) record->aval)[index];
					}

					return dbPutLink(out, DBF_FLOAT, buffer, array_len);
				}

				return 0;
			}

			default:
			{
				if (record->atyp == luascriptAVALType_Integer)
				{
					dbPutLink(out, DBF_LONG, record->aval, array_len);
				}

				return 0;
			}
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
			return dbPutLink(out, DBR_DOUBLE, val, 1);
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
