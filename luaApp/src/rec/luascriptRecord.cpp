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
#include <vector>
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

/* mark in error messages for incomplete statements */
#define EOFMARK		"<eof>"
#define marklen		(sizeof(EOFMARK)/sizeof(char) - 1)

#include "epicsVersion.h"
#ifdef VERSION_INT

# if EPICS_VERSION_INT < VERSION_INT(3,16,0,2)
#  define RECSUPFUN_CAST (RECSUPFUN)
# else
#  define RECSUPFUN_CAST
# endif

# if EPICS_VERSION_INT < VERSION_INT(3,16,0,1)
#   define dbLinkIsConstant(lnk) ((lnk)->type == CONSTANT)
#   define dbLinkIsVolatile(lnk) ((lnk)->type == CA_LINK)
# endif

#else
# define RECSUPFUN_CAST (RECSUPFUN)
# define dbLinkIsConstant(lnk) ((lnk)->type == CONSTANT)
# define dbLinkIsVolatile(lnk) ((lnk)->type == CA_LINK)
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
	bool        my_state;
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

	errlogPrintf("Calling %s resulted in error: %s\n", record->call, err.c_str());

	strcpy(record->err, err.c_str());
	db_post_events(record, &record->err, DBE_VALUE);
}

/*
** Try to compile line on the stack as 'return <line>'; on return, stack
** has either compiled chunk or original line (if compilation failed).
*/
static int addreturn (lua_State *L)
{
	int status;
	size_t len; const char *line;
	lua_pushliteral(L, "return ");
	lua_pushvalue(L, -2);  /* duplicate line */
	lua_concat(L, 2);  /* new line is "return ..." */
	line = lua_tolstring(L, -1, &len);

	if ((status = luaL_loadbuffer(L, line, len, "=stdin")) == LUA_OK)
	{
		lua_remove(L, -3);  /* remove original line */
	}
	else
	{
		lua_pop(L, 2);  /* remove result from 'luaL_loadbuffer' and new line */
	}

	return status;
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
	if (input.empty() || input.at(0) != '@')    { return std::make_pair("", input); }

	/* A space separates the name of the file and the function call */
	size_t split = input.find(" ");

	/* No function found */
	if (split == std::string::npos)      { return std::make_pair(input, ""); }

	std::string filename = input.substr(1, split - 1);
	std::string function = input.substr(split + 1, std::string::npos);

	return std::make_pair(filename, function);
}


/*
 * Checks if a given name is either a filename or a name of an
 * already created state. Creates a new state from a found
 * file, or loads the named state.
 */
static int getState(luascriptRecord* record, std::string name)
{
	if (! name.empty() && luaLocateFile(name).empty())
	{
		record->state = luaNamedState(name.c_str());
		((rpvtStruct*) record->rpvt)->my_state = false;
		return 0;
	}

	record->state = luaCreateState();
	((rpvtStruct*) record->rpvt)->my_state = true;

	if (name.empty())    { return 0; }

	long status = luaLoadScript((lua_State*) record->state, name.c_str());

	if (status)
	{
		printf("Error loading filename: %s, see ERR field\n", name.c_str());
		logError(record);
		return -1;
	}

	return 0;
}


/*
 * Initializes/Reinitializes Lua state according to the CODE field
 */
static int initState(luascriptRecord* record)
{
	/* Clear existing errors */
	memset(record->err, 0, 200);
	db_post_events(record, &record->err, DBE_VALUE);

	std::string code(record->code);
	std::string pcode(record->pcode);

	/* Parse for filenames and functions */
	std::pair<std::string, std::string> curr = parseCode(code);
	std::pair<std::string, std::string> prev = parseCode(pcode);

	strcpy(record->call, curr.second.c_str());
	strcpy(record->pcode, record->code);

	if (record->relo == luascriptRELO_NewFile)
	{
		if (curr.first == prev.first && record->state != NULL)    { return 0; }
	}

	if (((rpvtStruct*) record->rpvt)->my_state == true)   { lua_close((lua_State*) record->state); }

	return getState(record, curr.first);
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


/*
 * Pull elements number of values from a multi-element link,
 * put them into a lua table, and then put the table onto
 * the stack.
 */
template <typename T>
static int createTable(lua_State* state, DBLINK* field, short field_type, long* elements, bool integers)
{
	T *data = new T[*elements];
	int status = dbGetLink(field, field_type, data, 0, elements);

	if (status) { return status; }

	lua_createtable(state, *elements, 0);

	for (int elem = 0; elem < *elements; elem += 1)
	{
		if (integers)    { lua_pushinteger(state, data[elem]); }
		else             { lua_pushnumber(state, data[elem]); }

		lua_rawseti(state, -2, elem + 1);
	}

	delete data;
	return 0;
}


/*
 * Converts a lua table into a generated array of basic elements
 * so that it can be written to an epics PV. The type of the
 * returned array is based on the type of the first element of
 * the array in lua.
 */
static void* convertTable(lua_State* state, int* generated_size, epicsEnum16* arraytype)
{
	// Get the length of the array
	lua_len(state, -1);
	int array_size = lua_tonumber(state, -1);
	lua_pop(state, 1);

	// Get type of first value
	lua_geti(state, -1, 1);
	int is_integer = lua_isinteger(state, -1);
	int data_type = lua_type(state, -1);
	lua_pop(state, 1);

	switch (data_type)
	{
		case LUA_TNUMBER:
		{
			if (! is_integer)
			{
				double* output = new double[array_size];
				*generated_size = sizeof(double) * array_size;
				*arraytype = luascriptAVALType_Double;

				for (int index = 0; index < array_size; index += 1)
				{
					lua_geti(state, -1, index + 1);
					output[index] = lua_tonumber(state, -1);
					lua_pop(state, 1);
				}

				return output;
			}

			//Intentional Fall-through for integers
		}

		case LUA_TBOOLEAN:
		{
			int* output = new int[array_size];
			*generated_size = sizeof(int) * array_size;
			*arraytype = luascriptAVALType_Integer;

			for (int index = 0; index < array_size; index += 1)
			{
				lua_geti(state, -1, index + 1);
				output[index] = lua_tonumber(state, -1);
				lua_pop(state, 1);
			}

			return output;
		}

		case LUA_TSTRING:
		{
			char* output = new char[array_size];
			*generated_size = sizeof(char) * array_size;
			*arraytype = luascriptAVALType_Char;

			for (int index = 0; index < array_size; index += 1)
			{
				lua_geti(state, -1, index + 1);
				output[index] = lua_tostring(state, -1)[0];
				lua_pop(state, 1);
			}

			return output;
		}

		default:
			*generated_size = 0;
			return NULL;
	}
}

static long loadStrings(luascriptRecord* record)
{
	lua_State* state = (lua_State*) record->state;

	DBLINK* field = &record->inaa;

	char*   strvalue = (char*) record->aa;
	char**  prev_str = (char**) &record->paa;

	long status = 0;

	for (unsigned index = 0; index < STR_ARGS; index += 1)
	{
		strncpy(*prev_str, strvalue, STRING_SIZE);

		short field_type = 0;
		long elements = 1;

		status = getFieldInfo(field, &field_type, &elements);

		if (status)    { return status; }

		switch(field_type)
		{
			case DBF_CHAR:
				elements = std::min(elements, (long) STRING_SIZE - 1);
				//Intentional fall-through

			case DBF_ENUM:
			case DBF_STRING:
			{
				char tempstr[STRING_SIZE] = { '\0' };

				// Use AA .. JJ if INAA..INJJ are empty
				if ( field->type == CONSTANT )
				{
					strncpy(tempstr, strvalue, STRING_SIZE);
 					lua_pushstring(state, tempstr);
 					break;
				}

				if (field_type == DBF_ENUM)
				{
					status = dbGetLink(field, DBF_STRING, tempstr, 0, &elements);
				}
				else
				{
					status = dbGetLink(field, field_type, tempstr, 0, &elements);
				}

				if (status)    { break; }

				strncpy(strvalue, tempstr, STRING_SIZE);

				lua_pushstring(state, tempstr);

				break;
			}

			case DBF_UCHAR:
				status = createTable<epicsUInt8>(state, field, field_type, &elements, true);
				break;

			case DBF_SHORT:
				status = createTable<epicsInt16>(state, field, field_type, &elements, true);
				break;

			case DBF_USHORT:
				status = createTable<epicsUInt16>(state, field, field_type, &elements, true);
				break;

			case DBF_LONG:
				status = createTable<epicsInt32>(state, field, field_type, &elements, true);
				break;

			case DBF_ULONG:
				status = createTable<epicsUInt32>(state, field, field_type, &elements, true);
				break;

			case DBF_FLOAT:
				status = createTable<epicsFloat32>(state, field, field_type, &elements, false);
				break;

			case DBF_DOUBLE:
				status = createTable<epicsFloat64>(state, field, field_type, &elements, false);
				break;

			default:
				status = -1;
				break;
		}

		if (! status)    { lua_setglobal(state, STR_NAMES[index]); }

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
		if (dbLinkIsConstant(field))
		{
			if (index < NUM_ARGS)
			{
				recGblInitConstantLink(field, DBF_DOUBLE, value);
				db_post_events(record, value, DBE_VALUE);
			}

			*valid = luascriptINAV_CON;
		}
		else if (dbLinkIsVolatile(field))
		{
			if (dbIsLinkConnected(field))
			{
				*valid = luascriptINAV_EXT; }
			else
			{
				*valid = luascriptINAV_EXT_NC;
				pvt->caLinkStat = CA_LINKS_NOT_OK;
			}
		}
		else
		{
			*valid = luascriptINAV_LOC;
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
		record->call = (char *) calloc(121, sizeof(char));
		record->rpvt = (void *) calloc(1, sizeof(struct rpvtStruct));

		int index;
		char** init_str = (char**) &record->paa;

		for (index = 0; index < STR_ARGS; index += 1)
		{
			*init_str  = (char *) calloc(STRING_SIZE, sizeof(char));
			init_str++;
		}

		if (initState(record))    { return -1; }

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

static bool checkAvalUpdate(luascriptRecord* record)
{
	if (record->aval == NULL) { return false; }

	bool definitely_changed = (record->pavl == NULL) || (record->pasz != record->asiz);

	switch (record->oopt)
	{
		case luascriptOOPT_Every_Time:
			return true;

		case luascriptOOPT_On_Change:
			return (definitely_changed || memcmp(record->pavl, record->aval, record->asiz));

		case luascriptOOPT_When_Zero:
			return (record->asiz == 0);

		case luascriptOOPT_When_Non_zero:
			return (record->asiz != 0);

		case luascriptOOPT_Transition_To_Zero:
			return ((record->pasz != 0) && (record->asiz == 0));

		case luascriptOOPT_Transition_To_Non_zero:
			return ((record->pasz == 0) && (record->asiz != 0));

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


static int incomplete(lua_State* L, int status)
{
	if (status == LUA_ERRSYNTAX)
	{
		size_t lmsg;
		const char *msg = lua_tolstring(L, -1, &lmsg);

		if (lmsg >= marklen && strcmp(msg + lmsg - marklen, EOFMARK) == 0)
		{
			lua_pop(L, 1);
			return 1;
		}
	}

	return 0;
}

static void processCallback(void* data)
{
	luascriptRecord* record = (luascriptRecord*) data;

	lua_State* state = (lua_State*) record->state;

	lua_pushstring(state, (const char*) record->call);
	int status = addreturn(state);

	if (status != LUA_OK)
	{
		size_t len;
		const char *buffer = lua_tolstring(state, 1, &len);  /* get what it has */
		status = luaL_loadbuffer(state, buffer, len, "=stdin");  /* try it */

		if (incomplete(state, status))
		{
			record->pact = FALSE;
			logError(record);
			return;
		}
	}

	lua_remove(state, 1);
	status = lua_pcall(state, 0, 1, 0);

	if (status)
	{
		logError(record);
		record->pact = FALSE;
		return;
	}

	// Process the returned values
	int top = lua_gettop(state);

	if (top > 0)
	{
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
		else if (rettype == LUA_TTABLE)
		{
			if (record->pavl != NULL)
			{
				if (record->patp == luascriptAVALType_Integer)       { delete [] ((int*) record->pavl); }
				else if (record->patp == luascriptAVALType_Double)   { delete [] ((double*) record->pavl); }
				else if (record->patp == luascriptAVALType_Char)     { delete [] ((char*) record->pavl); }
			}

			record->pavl = record->aval;
			record->pasz = record->asiz;
			record->patp = record->atyp;

			record->aval = convertTable(state, &record->asiz, &record->atyp);

			if (checkAvalUpdate(record))
			{
				record->pact = FALSE;
				writeValue(record);
				record->pact = TRUE;
				db_post_events(record, &record->aval, DBE_VALUE);
			}
		}

		lua_pop(state, 1);
	}

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

	/* No need to process if there's no code */
	if (record->code[0] == '\0')    { return 0; }

	long status;

	/* Clear any existing error message */
	memset(record->err, 0, 200);
	db_post_events(record, &record->err, DBE_VALUE);

	record->pact = TRUE;

	/* luascriptRELO_Always indicates a state reload on every process */
	if (record->relo == luascriptRELO_Always)
	{
		memset(record->pcode, 0, 121);

		status = initState(record);

		if (status) { record->pact = FALSE; return status; }
	}

	status = loadNumbers(record);

	if (status)    { record->pact = FALSE; return status; }

	status = loadStrings(record);

	if (status)    { record->pact = FALSE; return status; }

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
		initState(record);
	}
	else if (field_index == luascriptRecordFRLD && record->frld)
	{
		memset(record->pcode, 0, 121);
		initState(record);
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
