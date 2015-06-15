#ifndef INC_SCRIPTUTIL_H
#define INC_SCRIPTUTIL_H

#include "scriptRecord.h"
#include <dbAccess.h>

#define NUM_ARGS 10
#define STR_ARGS 10
#define NUM_OUT  10

#define STRING_SIZE 40

#define VAL_CHANGE  1
#define SVAL_CHANGE 2


#ifdef __cplusplus
extern "C"
{
#endif
	void logError(scriptRecord* record);
	long initState(scriptRecord* record);
	long setLinks(scriptRecord* record);
	
	void loadNumbers(scriptRecord* record);
	void loadStrings(scriptRecord* record);
	
	long runCode(scriptRecord* record);
	void checkValUpdate(scriptRecord* record, double* val, double* pval, unsigned char* update);
	void checkSvalUpdate(scriptRecord* record, char* sval, char** psvl, unsigned char* update);
	long speci(dbAddr	*paddr, int after);
	void monitor(scriptRecord* record);
	
	void writeValue(scriptRecord* record);
#ifdef __cplusplus
}
#endif

#endif
