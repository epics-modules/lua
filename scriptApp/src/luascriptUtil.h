#ifndef INC_LUASCRIPTUTIL_H
#define INC_LUASCRIPTUTIL_H

#include "luascriptRecord.h"
#include <dbAccess.h>
#include <callback.h>

#define NUM_ARGS 10
#define STR_ARGS 10
#define NUM_OUT  10

#define STRING_SIZE 40

#define VAL_CHANGE  1
#define SVAL_CHANGE 2

#define NO_CA_LINKS     0
#define CA_LINKS_ALL_OK 1
#define CA_LINKS_NOT_OK 2

typedef struct rpvtStruct {
	CALLBACK	doOutCb;
	CALLBACK	checkLinkCb;
	short		wd_id_1_LOCK;
	short		caLinkStat; /* NO_CA_LINKS,CA_LINKS_ALL_OK,CA_LINKS_NOT_OK */
	short		outlink_field_type;
} rpvtStruct;


#ifdef __cplusplus
extern "C"
{
#endif
	void logError(luascriptRecord* record);
	long initState(luascriptRecord* record, int force_reload);

	void checkLinks(luascriptRecord* record);
	long setLinks(luascriptRecord* record);

	long startProc(luascriptRecord* record);
	long cleanProc(luascriptRecord* record);
	void processCallback(void* data);
	
	long loadNumbers(luascriptRecord* record);
	long loadStrings(luascriptRecord* record);
	
	long runCode(luascriptRecord* record);
	long speci(dbAddr	*paddr, int after);
	void monitor(luascriptRecord* record);
	
	void writeValue(luascriptRecord* record);
#ifdef __cplusplus
}
#endif

#endif
