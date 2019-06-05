#include <alarm.h>
#include <dbEvent.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <errlog.h>
#include <callback.h>

#include <epicsExport.h>

#define GEN_SIZE_OFFSET
#include "luascriptRecord.h"
#undef  GEN_SIZE_OFFSET

#include "luaEpics.h"

#include <sstream>

#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>

#define NUM_ARGS 10
#define STR_ARGS 10
#define NUM_OUT  10

#define STRING_SIZE 40

#define VAL_CHANGE  1
#define SVAL_CHANGE 2

#define NO_CA_LINKS     0
#define CA_LINKS_ALL_OK 1
#define CA_LINKS_NOT_OK 2

#include "epicsVersion.h"
#ifdef VERSION_INT
# if EPICS_VERSION_INT < VERSION_INT(3,16,0,2)
#  define RECSUPFUN_CAST (RECSUPFUN)
# else
#  define RECSUPFUN_CAST
# endif
#else
# define RECSUPFUN_CAST (RECSUPFUN)
#endif

/* Lua variable names for numeric and string fields */
static const char NUM_NAMES[NUM_ARGS][2] = {"A","B","C","D","E","F","G","H","I","J"};
static const char STR_NAMES[STR_ARGS][3] = {"AA","BB","CC","DD","EE","FF","GG","HH","II","JJ"};


static long init_record(dbCommon* common, int pass);
static long process(dbCommon* common);
static long special(dbAddr *paddr, int after);
static long get_precision(const dbAddr* paddr, long* precision);

typedef struct ScriptDSET
{
	long       number;
	DEVSUPFUN  dev_report;
	DEVSUPFUN  init;
	DEVSUPFUN  init_record;
	DEVSUPFUN  get_ioint_info;
	DEVSUPFUN  write;
} ScriptDSET;

typedef struct rpvtStruct {
	CALLBACK	doOutCb;
	CALLBACK	checkLinkCb;
	short		wd_id_1_LOCK;
	short		caLinkStat; /* NO_CA_LINKS,CA_LINKS_ALL_OK,CA_LINKS_NOT_OK */
	short		outlink_field_type;
} rpvtStruct;

extern "C"
{

rset luascriptRSET =
{
	RSETNUMBER,
	NULL,
	NULL,
	RECSUPFUN_CAST (init_record),
	RECSUPFUN_CAST (process),
	RECSUPFUN_CAST (special),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	RECSUPFUN_CAST (get_precision),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

epicsExportAddress(rset, luascriptRSET);

}

static void logError(luascriptRecord* record)
{
	std::string err(lua_tostring((lua_State*) record->state, -1));
	lua_pop((lua_State*) record->state, 1);


	/* Get rid of everything up until the line number */
	size_t split = err.find_first_of(':');

	if (split != std::string::npos)    { err.erase(0, split + 1); }

	strcpy(record->err, err.c_str());
	db_post_events(record, &record->err, DBE_VALUE);
}


static bool isLink(int index)
{
	return (index == luascriptRecordOUT || (index >= luascriptRecordINPA && index <= luascriptRecordINJJ));
}

/*
 * Parses the CODE field for a file name and given function
 * assuming a style of "@filename function()"
 */
static std::pair<std::string, std::string> parseCode(std::string& input)
{
	/* A space separates the name of the file and the function call */
	size_t split = input.find(" ");

	/* Function name goes to either the beginning of parameters, or end of string */
	size_t end = input.find("(");

	/* No function found */
	if (split == std::string::npos)      { return std::make_pair("", ""); }

	std::string filename = input.substr(1, split - 1);
	std::string function;

	if   (end == std::string::npos)    { function = input.substr(split + 1, end); }
	else                               { function = input.substr(split + 1, end - split - 1); }

	return std::make_pair(filename, function);
}

/*
 * Parses the comma separated values in between the parentheses
 * in the CODE field.
 */
static int parseParams(lua_State* state, std::string params)
{
	if (params.at(0) != '@')    { return 0; }

	size_t start = params.find_first_of("(");
	size_t end   = params.find_last_of(")");
	size_t oob   = std::string::npos;

	/* Syntax error */
	if (end == oob || start == oob || start == end - 1) { return 0; }

	return luaLoadParams(state, params.substr(start + 1, end - start - 1).c_str());
}

/*
 * Initializes/Reinitializes Lua state according to the CODE field
 */
static long initState(luascriptRecord* record, int force_reload)
{
	long status = 0;

	/* Clear existing errors */
	memset(record->err, 0, 200);
	db_post_events(record, &record->err, DBE_VALUE);

	lua_State* state = luaCreateState();

	std::string code(record->code);
	std::string pcode(record->pcode);

	if (!code.empty())
	{
		/* @ signifies a call to a function in a file, otherwise treat it as code */
		if (code[0] != '@')
		{
			if (code != pcode || force_reload)    { status = luaLoadString(state, code.c_str()); }
		}
		else
		{
			/* Parse for filenames and functions */
			std::pair<std::string, std::string> curr = parseCode(code);
			std::pair<std::string, std::string> prev = parseCode(pcode);

			/* We'll always need to reload going from code to referencing a file */
			if (pcode[0] == '@' && !force_reload)
			{
				if (curr == prev && record->relo != luascriptRELO_Always)    { return 0; }

				/*
				 * If only the functon name changes and the record is set to only
				 * reload on file changes, all we have to do is pull the function
				 * and put it at the top of the stack.
				 */
				if ((record->relo == luascriptRELO_NewFile) && (curr.first == prev.first))
				{
					lua_getglobal((lua_State*) record->state, curr.second.c_str());
					strcpy(record->pcode, record->code);
					return 0;
				}
			}

			status = luaLoadScript(state, curr.first.c_str());

			if (! status)    { lua_getglobal(state, curr.second.c_str()); }
		}
	}

	if (status == -1)    { printf("Invalid input or could not locate file\n"); status = 0;}

	/* Cleanup any memory from previous state */
	if (record->state != NULL)    { lua_close((lua_State*) record->state); }

	record->state = (void*) state;
	strcpy(record->pcode, record->code);

	return status;
}

static long getFieldInfo(DBLINK* field, short* field_type, long* elements)
{
	dbAddr  Addr;
	dbAddr* pAddr = &Addr;

	if (field->type == CA_LINK)
	{
		*field_type = dbCaGetLinkDBFtype(field);
		dbCaGetNelements(field, elements);
	}
	else if (field->type ==  DB_LINK)
	{
		*field_type = DBR_STRING;

		long status = dbNameToAddr(field->value.pv_link.pvname, pAddr);

		if (status)    { return status; }

		*field_type = pAddr->field_type;
		*elements   = pAddr->no_elements;
	}

	return 0;
}


/*
 * Grabs the new values of every numerical input link and
 * pushes the result to the lua stack.
 */
static long loadNumbers(luascriptRecord* record)
{
	lua_State* state = (lua_State*) record->state;

	DBLINK* field = &record->inpa;
	double* value = &record->a;
	double* prev  = &record->pa;

	long status = 0;

	for (unsigned index = 0; index < NUM_ARGS; index += 1)
	{
		*prev = *value;

		long newStatus = dbGetLink(field, DBR_DOUBLE, value, 0, 0);

		if (!status)    { status = newStatus; }

		lua_pushnumber(state, *value);
		lua_setglobal(state, NUM_NAMES[index]);

		field++;
		prev++;
		value++;
	}

	return status;
}

static long loadStrings(luascriptRecord* record)
{
	lua_State* state = (lua_State*) record->state;

	DBLINK* field = &record->inaa;
	char*   strvalue = (char*) record->aa;
	char**  prev_str = (char**) &record->paa;
	char tempstr[STRING_SIZE];

	long status = 0;

	for (unsigned index = 0; index < STR_ARGS; index += 1)
	{
		strncpy(*prev_str, strvalue, STRING_SIZE);

		short field_type = 0;
		long elements = 1;

		status = getFieldInfo(field, &field_type, &elements);

		if (status)    { return status; }

		elements = std::min(elements, (long) STRING_SIZE - 1);
		std::fill(tempstr, tempstr + elements, '\0');

		status = dbGetLink(field, field_type, tempstr, 0, &elements);

		if (status)    { return status; }

		strncpy(strvalue, tempstr, STRING_SIZE);

		lua_pushstring(state, tempstr);
		lua_setglobal(state, STR_NAMES[index]);

		field++;
		prev_str++;
		strvalue += STRING_SIZE;
	}

	lua_pushstring(state, record->name);
	lua_setglobal(state, "self");

	return status;
}

void checkLinks(luascriptRecord* record)
{
	dbAddr address;
	dbAddr* paddress = &address;

	DBLINK* field = &record->inpa;
	unsigned short* valid = &record->inav;
	double* value = &record->a;

	int isCaLink = 0;
	int isCaLinkNc = 0;
	int isString;
	int linkWorks;

	char tmpstr[100];

	rpvtStruct* pvt = (rpvtStruct*) record->rpvt;

	for (unsigned index = 0; index < NUM_ARGS + STR_ARGS + 1; index += 1)
	{
		if (field->type == CA_LINK)
		{
			isCaLink = 1;
			isString = 0;
			linkWorks = 0;

			if (dbCaIsLinkConnected(field))
			{
				if (index >= NUM_ARGS && index < NUM_ARGS + STR_ARGS)
				{
					isString = 1;
					long status = dbGetLink(field, DBR_STRING, tmpstr, 0, 0);

					if (RTN_SUCCESS(status))
					{
						linkWorks = 1;
					}
				}
			}

			if (dbCaIsLinkConnected(field) && (isString == linkWorks))
			{
				if (*valid == luascriptINAV_EXT_NC)
				{
					if (!dbNameToAddr(field->value.pv_link.pvname, paddress))
					{
						*valid = luascriptINAV_LOC;
					}
					else
					{
						*valid = luascriptINAV_EXT;
					}

					db_post_events(record, valid, DBE_VALUE);
				}

				if (field == &record->out)
				{
					pvt->outlink_field_type = dbCaGetLinkDBFtype(field);
				}
			}
			else
			{
				if (*valid == luascriptINAV_EXT_NC)
				{
					isCaLinkNc = 1;
				}
				else if (*valid == luascriptINAV_EXT)
				{
					*valid = luascriptINAV_EXT_NC;
					db_post_events(record, valid, DBE_VALUE);
					isCaLinkNc = 1;
				}

				if (field == &record->out)    { pvt->outlink_field_type = DBF_NOACCESS; }
			}
		}

		field++;
		valid++;
		value++;
	}

	if      (isCaLinkNc)    { pvt->caLinkStat = CA_LINKS_NOT_OK; }
	else if (isCaLink)      { pvt->caLinkStat = CA_LINKS_ALL_OK; }
	else                    { pvt->caLinkStat = NO_CA_LINKS; }

	if (!pvt->wd_id_1_LOCK && isCaLinkNc)
	{
		pvt->wd_id_1_LOCK = 1;
		callbackRequestDelayed(&pvt->checkLinkCb, .5);
	}
}


static void checkLinksCallback(CALLBACK* callback)
{
	luascriptRecord* record;
	void* temp;

	callbackGetUser(temp, callback);
	record = (luascriptRecord*) temp;

	rpvtStruct* pvt = (rpvtStruct*) record->rpvt;

	if (false)
	{
	}
	else
	{
		dbScanLock((struct dbCommon*) record);
		pvt->wd_id_1_LOCK = 0;
		checkLinks(record);
		dbScanUnlock((struct dbCommon*) record);
	}
}


long setLinks(luascriptRecord* record)
{
	dbAddr address;
	dbAddr* paddress = &address;

	DBLINK* field = &record->inpa;
	unsigned short* valid = &record->inav;
	double* value = &record->a;

	rpvtStruct* pvt = (rpvtStruct*) record->rpvt;

	for (unsigned index = 0; index < NUM_ARGS + STR_ARGS + 1; index += 1)
	{
		if (field == &record->out)    { pvt->outlink_field_type = DBF_NOACCESS; }

		if (field->type == CONSTANT)
		{
			if (index < NUM_ARGS)
			{
				recGblInitConstantLink(field, DBF_DOUBLE, value);
				db_post_events(record, value, DBE_VALUE);
			}

			*valid = luascriptINAV_CON;
		}
		else if (!dbNameToAddr(field->value.pv_link.pvname, paddress))
		{
			*valid = luascriptINAV_LOC;

			if (field == &record->out)
			{
				pvt->outlink_field_type = paddress->field_type;
			}
		}
		else
		{
			*valid = luascriptINAV_EXT_NC;
			pvt->caLinkStat = CA_LINKS_NOT_OK;
		}

		db_post_events(record, valid, DBE_VALUE);

		callbackSetCallback(checkLinksCallback, &pvt->checkLinkCb);
		callbackSetPriority(0, &pvt->checkLinkCb);
		callbackSetUser(record, &pvt->checkLinkCb);
		pvt->wd_id_1_LOCK = 0;

		if (pvt->caLinkStat == CA_LINKS_NOT_OK)
		{
			callbackRequestDelayed(&pvt->checkLinkCb, 1.0);
			pvt->wd_id_1_LOCK = 1;
		}

		field++;
		valid++;
		value++;
	}

	return 0;
}




static long init_record(dbCommon* common, int pass)
{
	luascriptRecord* record = (luascriptRecord*) common;

	if (pass == 0)
	{
		record->pcode = (char *) calloc(121, sizeof(char));
		record->rpvt = (void *) calloc(1, sizeof(struct rpvtStruct));

		int index;
		char** init_str = (char**) &record->paa;

		for (index = 0; index < STR_ARGS; index += 1)
		{
			*init_str  = (char *) calloc(STRING_SIZE, sizeof(char));
			init_str++;
		}

		if (initState(record, 0))
		{
			logError(record);
			return -1;
		}

		return 0;
	}

	setLinks(record);

	return 0;
}

static bool checkValUpdate(luascriptRecord* record)
{
	double val = record->val;
	double pval = record->pval;

	switch (record->oopt)
	{
		case luascriptOOPT_Every_Time:
			return true;

		case luascriptOOPT_On_Change:
			return (fabs(pval - val) > record->mdel);

		case luascriptOOPT_When_Zero:
			return !val;

		case luascriptOOPT_When_Non_zero:
			return (bool) val;

		case luascriptOOPT_Transition_To_Zero:
			return (val == 0 && pval != 0);

		case luascriptOOPT_Transition_To_Non_zero:
			return (val != 0 && pval == 0);

		case luascriptOOPT_Never:
			return false;
	}

	return false;
}


static bool checkSvalUpdate(luascriptRecord* record)
{
	std::string curr(record->sval);

	std::string prev;

	if (record->psvl)    { prev = std::string(record->psvl); }
	else                 { prev = std::string(""); }

	switch (record->oopt)
	{
		case luascriptOOPT_Every_Time:
			return true;

		case luascriptOOPT_On_Change:
			return (prev != curr);

		case luascriptOOPT_When_Zero:
			return curr.empty();

		case luascriptOOPT_When_Non_zero:
			return !curr.empty();

		case luascriptOOPT_Transition_To_Zero:
			return (!prev.empty() && curr.empty());

		case luascriptOOPT_Transition_To_Non_zero:
			return (!prev.empty() && !curr.empty());

		case luascriptOOPT_Never:
			return false;
	}

	return false;
}

static void writeValue(luascriptRecord* record)
{
	ScriptDSET* pluascriptDSET = (ScriptDSET*) record->dset;

	if (!pluascriptDSET || !pluascriptDSET->write)
		{
		errlogPrintf("%s DSET write does not exist\n", record->name);
		recGblSetSevr(record, SOFT_ALARM, INVALID_ALARM);
		record->pact = TRUE;
		return;
	}

	pluascriptDSET->write(record);
}


static void processCallback(void* data)
{
	luascriptRecord* record = (luascriptRecord*) data;

	lua_State* state = (lua_State*) record->state;

	/* Make a copy of the current code chunk */
	lua_pushvalue(state, -1);

	/* Load Params */
	int params = parseParams(state, std::string(record->code));

	/* Call the chunk */
	if (lua_pcall(state, params, 1, 0))
	{
		logError(record);
		record->pact = FALSE;
		return;
	}

	int rettype = lua_type(state, -1);

	if (rettype == LUA_TBOOLEAN || rettype == LUA_TNUMBER)
	{
		record->pval = record->val;
		record->val = lua_tonumber(state, -1);

		if (checkValUpdate(record))
		{
			record->pact = FALSE;
			writeValue(record);
			record->pact = TRUE;
			db_post_events(record, &record->val, DBE_VALUE);
		}
	}
	else if (rettype == LUA_TSTRING)
	{
		strncpy(record->psvl, record->sval, STRING_SIZE);
		strncpy(record->sval, lua_tostring(state, -1), STRING_SIZE);

		if (checkSvalUpdate(record))
		{
			record->pact = FALSE;
			writeValue(record);
			record->pact = TRUE;
			db_post_events(record, &record->sval, DBE_VALUE);
		}
	}

	lua_pop(state, 1);

	double* new_val = &record->a;
	double* old_val = &record->pa;

	for (unsigned index = 0; index < NUM_ARGS; index += 1)
	{
		if (*new_val != *old_val)    { db_post_events(record, new_val, DBE_VALUE); }

		new_val++;
		old_val++;
	}

	char*  new_str = (char*)  &record->aa;
	char** old_str = (char**) &record->paa;

	for (unsigned index = 0; index < STR_ARGS; index += 1)
	{
		if (strcmp(new_str, *old_str))    { db_post_events(record, new_str, DBE_VALUE); }

		old_str++;
		new_str += STRING_SIZE;
	}

	recGblFwdLink(record);
	record->pact = FALSE;
}

static long runCode(luascriptRecord* record)
{
	if (std::string(record->code).empty())    { record->pact = FALSE; return 0; }

	if (record->sync == luascriptSYNC_Synchronous)
	{
		processCallback(record);
	}
	else
	{
		std::stringstream temp_stream;
		std::string threadname;

		temp_stream << "Script Record Process (" << record->name << ")";
		temp_stream >> threadname;

		epicsThreadCreate(threadname.c_str(),
		                  epicsThreadPriorityLow,
		                  epicsThreadGetStackSize(epicsThreadStackMedium),
		                  (EPICSTHREADFUNC)::processCallback, record);
	}

	return 0;
}

static long process(dbCommon* common)
{
	luascriptRecord* record = (luascriptRecord*) common;

	long status;

	/* Clear any existing error message */
	memset(record->err, 0, 200);
	db_post_events(record, &record->err, DBE_VALUE);

	record->pact = TRUE;

	/* luascriptRELO_Always indicates a state reload on every process */
	if (record->relo == luascriptRELO_Always)
	{
		status = initState(record, 1);

		if (status) { return status; }
	}

	status = loadNumbers(record);

	if (status)    { return status; }

	status = loadStrings(record);

	if (status)    { return status; }

	status = runCode(record);

	if (status)
	{
		recGblFwdLink(record);
		record->pact = FALSE;
	}

	return status;
}

static long special(dbAddr* paddr, int after)
{
	if (!after)    { return 0; }

	luascriptRecord* record = (luascriptRecord*) paddr->precord;

	int field_index = dbGetFieldIndex(paddr);

	if (field_index == luascriptRecordCODE)
	{
		if (initState(record, 0))    { logError(record); }
	}
	else if (field_index == luascriptRecordFRLD && record->frld)
	{
		if (initState(record, 1))    { logError(record); }
		record->frld = 0;
	}
	else if (isLink(field_index))
	{
		int offset = field_index - luascriptRecordINPA;

		DBLINK* field = &record->inpa + offset;
		double* value = &record->a + offset;
		unsigned short* valid = &record->inav + offset;
		char* name = field->value.pv_link.pvname;

		dbAddr address;
		dbAddr* paddress = &address;

		rpvtStruct* pvt = (rpvtStruct*) record->rpvt;

		if (field_index == luascriptRecordOUT)    { pvt->outlink_field_type = DBF_NOACCESS; }

		if (field->type == CONSTANT)
		{
			if (field_index <= luascriptRecordINPJ)
			{
				recGblInitConstantLink(field, DBF_DOUBLE, value);
				db_post_events(record, value, DBE_VALUE);
			}

			*valid = luascriptINAV_CON;
		}
		else if (!dbNameToAddr(name, paddress))
		{
			short pvlMask = field->value.pv_link.pvlMask;
			short isCA = pvlMask & (pvlOptCA | pvlOptCP | pvlOptCPP);

			if (field_index <= luascriptRecordINPJ || field_index > luascriptRecordINJJ || !isCA)
			{
				*valid = luascriptINAV_LOC;
			}
			else
			{
				*valid = luascriptINAV_EXT_NC;

				if (!pvt->wd_id_1_LOCK)
				{
					callbackRequestDelayed(&pvt->checkLinkCb, .5);
					pvt->wd_id_1_LOCK = 1;
					pvt->caLinkStat = CA_LINKS_NOT_OK;
				}
			}

			if (field_index == luascriptRecordOUT)
			{
				pvt->outlink_field_type = paddress->field_type;

				if (paddress->field_type >= DBF_INLINK && paddress->field_type <= DBF_FWDLINK)
				{
					if (! (pvlMask & pvlOptCA))
					{
						printf("luascriptRecord(%s):non-CA link to link field\n", name);
					}
				}

				if (record->wait && !(pvlMask & pvlOptCA))
				{
					printf("luascriptRecord(%s):Can't wait with non-CA link attribute\n", name);
				}
			}
		}
		else
		{
			*valid = luascriptINAV_EXT_NC;

			if (!pvt->wd_id_1_LOCK)
			{
				callbackRequestDelayed(&pvt->checkLinkCb,.5);
				pvt->wd_id_1_LOCK = 1;
				pvt->caLinkStat = CA_LINKS_NOT_OK;
			}
		}

		db_post_events(record, valid, DBE_VALUE);
	}

	return 0;
}

static long get_precision(const dbAddr* paddr, long* precision)
{
	luascriptRecord *record = (luascriptRecord *) paddr->precord;
	int index = dbGetFieldIndex(paddr);

	*precision = record->prec;

	if (index == luascriptRecordVAL) return 0;

	recGblGetPrec(paddr, precision);
	return 0;
}
