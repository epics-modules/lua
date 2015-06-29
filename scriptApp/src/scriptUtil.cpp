#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <algorithm>

#if defined(__vxworks) || defined(vxWorks)
	#include <symLib.h>
	#include <sysSymTbl.h>
#endif

#include <recGbl.h>
#include <epicsExport.h>
#include <devSup.h>
#include <recSup.h>
#include <errlog.h>
#include <alarm.h>
#include <dbEvent.h>

#include "scriptUtil.h"

extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

std::string locateFile(std::string& filename);
long loadFile(lua_State* state, std::string& filename, std::string& function);
int parseParams(lua_State* state, std::string params);

std::pair<std::string, std::string> parseCode(std::string& input);

typedef struct ScriptDSET {
    long       number;
    DEVSUPFUN  dev_report;
    DEVSUPFUN  init;
    DEVSUPFUN  init_record;
    DEVSUPFUN  get_ioint_info;
    DEVSUPFUN  write;
} ScriptDSET;


/* Lua variable names for numeric and string fields */
static const char NUM_NAMES[NUM_ARGS][2] = {"A","B","C","D","E","F","G","H","I","J"};
static const char STR_NAMES[STR_ARGS][3] = {"AA","BB","CC","DD","EE","FF","GG","HH","II","JJ"};
static const char OUT_NAMES[NUM_OUT][5]  = {"OUT0","OUT1","OUT2","OUT3","OUT4","OUT5","OUT6","OUT7","OUT8","OUT9"};


/*
 * Parses the CODE field for a file name and given function
 * assuming a style of "@filename function()"
 */
std::pair<std::string, std::string> parseCode(std::string& input)
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
 * Initializes/Reinitializes Lua state according to the CODE field
 */
long initState(scriptRecord* record)
{		
	long status = 0;
	
	lua_State* state = luaL_newstate();
	luaL_openlibs(state);
	
	std::string code(record->code);
	std::string pcode(record->pcode);
	
	if (! code.empty())
	{
		/* @ signifies a call to a function in a file, otherwise treat it as code */
		if (code[0] != '@')
		{
			if (code != pcode)    { status = luaL_loadstring(state, record->code); }
		}
		else 
		{
			/* Parse for filenames and functions */
			std::pair<std::string, std::string> curr = parseCode(code);
			std::pair<std::string, std::string> prev = parseCode(pcode);
			
			/* We'll always need to reload going from code to referencing a file */
			if (pcode[0] == '@')
			{
				if (curr == prev && record->relo != scriptRELO_Always)    { return 0; }
				
				/* 
				 * If only the functon name changes and the record is set to only
				 * reload on file changes, all we have to do is pull the function
				 * and put it at the top of the stack.
				 */
				if ((record->relo == scriptRELO_NewFile) && (curr.first == prev.first))
				{
					lua_getglobal((lua_State*) record->state, curr.second.c_str());
					strcpy(record->pcode, record->code);
					return 0;
				}
			}
			
			status = loadFile(state, curr.first, curr.second);
		}
	}
	
	/* Cleanup any memory from previous state */
	if (record->state != NULL)    { lua_close((lua_State*) record->state); }
	
	record->state = (void*) state;
	strcpy(record->pcode, record->code);
	
	return status;
}


std::string locateFile(std::string& filename)
{
	char* env_path = std::getenv("LUA_SCRIPT_PATH");
	
	#if defined(__vxworks) || defined(vxWorks)
    /* For compatibility reasons look for global symbols */
    if (!env_path)
    {
        char* symbol;
        SYM_TYPE type;
		
        if (symFindByName(sysSymTbl, "LUA_SCRIPT_PATH", &symbol, &type) == OK)
        {
            env_path = *(char**) symbol;
        }
    }
	#endif
	
	std::string path;
	
	/* If no environment variable found, default to the current directory. */
	if   (env_path)    { path = std::string(env_path); }
	else               { path = "."; }
	
	size_t start = 0;
	size_t next;
	
	do
	{
		next = path.find(":", start);
		
		std::string test = "/" + filename;
		
		if   (next == std::string::npos)    { test = path.substr(start) + test; }
		else                                { test = path.substr(start, next - start) + test; }
		
		/* Check if file exists. If so, return the full filepath */
		if (std::ifstream(test.c_str()).good())    { return test; }
		
		start = next + 1;
	} while (start);
	
	return filename;
}

long loadFile(lua_State* state, std::string& filename, std::string& function)
{
	int status = 0;
	
	status = luaL_loadfile(state, locateFile(filename).c_str()); 
	if (status)    { return status; }
	
	/* Run the script so functions are in the global state */
	status = lua_pcall(state, 0, 0, 0);
	if (status)    { return status; }
	
	/* Get the named function */
	lua_getglobal(state, function.c_str());
	return 0;
}


long setLinks(scriptRecord* record)
{
	dbAddr address;
	dbAddr* paddress = &address;
	
	DBLINK* field = &record->inpa;
	unsigned short* valid = &record->inav; 
	double* value = &record->a;
	
	for (int index = 0; index < NUM_ARGS + STR_ARGS; index += 1)
	{
		if (field->type == CONSTANT)
		{
			if (index < NUM_ARGS)
			{
				recGblInitConstantLink(field, DBF_DOUBLE, value);
				db_post_events(record, value, DBE_VALUE);
			}
			
			*valid = scriptINAV_CON;
		}
		else if (! dbNameToAddr(field->value.pv_link.pvname, paddress))
		{
			*valid = scriptINAV_LOC;
			
			db_post_events(record, valid, DBE_VALUE);
		}
		
		field++;
		valid++;
		value++;
	}
	
	return 0;
}


/*
 * Grabs the new values of every numerical input link and
 * pushes the result to the lua stack.
 */
void loadNumbers(scriptRecord* record)
{
	lua_State* state = (lua_State*) record->state;
	
	DBLINK* field = &record->inpa;
	double* value = &record->a;
	double* prev  = &record->pa;
	
	for (int index = 0; index < NUM_ARGS; index += 1)
	{
		*prev = *value;
		
		dbGetLink(field, DBR_DOUBLE, value, 0, 0);
		
		lua_pushnumber(state, *value);
		lua_setglobal(state, NUM_NAMES[index]);
		
		field++;
		prev++;
		value++;
	}
}


void loadStrings(scriptRecord* record)
{
	lua_State* state = (lua_State*) record->state;
	
	DBLINK* field = &record->inaa;
	char*   strvalue = (char*) record->aa;
	char**  prev_str = (char**) &record->paa;
	
	dbAddr  Addr;
	dbAddr* pAddr = &Addr;
	
	char tempstr[STRING_SIZE];
	
	for (int index = 0; index < STR_ARGS; index += 1)
	{		
		strncpy(*prev_str, strvalue, STRING_SIZE);
		
		short field_type = 0;
		long elements = 1;

		if (field->type == CA_LINK)
		{
			field_type = dbCaGetLinkDBFtype(field);
			dbCaGetNelements(field, &elements);
		}
		else if (field->type == DB_LINK)
		{
			field_type = DBR_STRING;
		
			if (! dbNameToAddr(field->value.pv_link.pvname, pAddr))
			{
				field_type = pAddr->field_type;
				elements   = pAddr->no_elements;
			}
		}
		
		elements = std::min(elements, (long) STRING_SIZE - 1);
		std::fill(tempstr, tempstr + elements, '\0');
		
		if ((field->type == CA_LINK) || (field->type == DB_LINK))
		{			
			if (((field_type == DBR_CHAR) || (field_type == DBR_UCHAR)) && elements > 1)
			{			
				dbGetLink(field, field_type, tempstr, 0, &elements);
			}
			else
			{
				dbGetLink(field, DBR_STRING, tempstr, 0, 0);
			}
			
			strncpy(strvalue, tempstr, STRING_SIZE);
				
			lua_pushstring(state, tempstr);
			lua_setglobal(state, STR_NAMES[index]);
		}
		
		field++;
		prev_str++;
		strvalue += STRING_SIZE;
	}
}

/*
 * Pulls the output variables from the lua state and
 * puts their values to the correct val/sval field.
 */
void postOutput(scriptRecord* record)
{
	lua_State* state = (lua_State*) record->state;
	
	double* val  = &record->val0;
	double* pval = &record->pvl0;
	
	char*   sval = (char*)  &record->svl0;
	char**  psvl = (char**) &record->psvl0;
	
	unsigned char*  update = (unsigned char*) &record->wrt0;
	
	for (int index = 0; index < NUM_OUT; index += 1)
	{
		int type = lua_getglobal(state, OUT_NAMES[index]);
		
		*pval = *val;
		strncpy(sval, (char*) psvl, STRING_SIZE);
		
		if ((type == LUA_TNUMBER) || (type == LUA_TBOOLEAN))
		{
			*val = lua_tonumber(state, -1);
			
			checkValUpdate(record, val, pval, update);
		}
		else if (type == LUA_TSTRING)
		{
			char* tempstr = (char*) lua_tostring(state, -1);
			strncpy(sval, tempstr, STRING_SIZE);
			
			checkSvalUpdate(record, sval, psvl, update);
		}
		
		lua_pop(state, 1);
		
		val++;
		pval++;
		psvl++;
		update++;
		sval += STRING_SIZE;
	}
}

long runCode(scriptRecord* record)
{	
	if (std::string(record->code).empty())    { return 0; }

	long status;
	
	/* scriptRELO_Always indicates a state reload on every process */
	if (record->relo == scriptRELO_Always)
	{ 
		status = initState(record);
		if (status)    { return status; }
	}
	
	lua_State* state = (lua_State*) record->state;
	
	/* Make a copy of the current code chunk */
	lua_pushvalue(state, -1);
	
	/* Load Params */
	int params = parseParams(state, std::string(record->code));
	
	/* Call the chunk */
	status = lua_pcall(state, params, 0, 0);
	if (status)    { return status; }
	
	postOutput(record);
	
	return 0;
}

void checkValUpdate(scriptRecord* record, double* val, double* pval, unsigned char* update)
{
	switch (record->oopt)
	{
		case scriptOOPT_Every_Time:
			*update = VAL_CHANGE;
			break;
		
		case scriptOOPT_On_Change:
			if (abs(*pval - *val) > record->mdel)    { *update = VAL_CHANGE; }
			break;
			
		case scriptOOPT_When_Zero:
			if (!*val)    { *update = VAL_CHANGE; }
			break;
			
		case scriptOOPT_When_Non_zero:
			if (*val)     { *update = VAL_CHANGE; }
			break;
			
		case scriptOOPT_Transition_To_Zero:
			if (*val == 0 && *pval != 0)    { *update = VAL_CHANGE; }
			break;
			
		case scriptOOPT_Transition_To_Non_zero:
			if (*val != 0 && *pval == 0)    { *update = VAL_CHANGE; }
			break;
			
		case scriptOOPT_Never:
			break;
	}
}


void checkSvalUpdate(scriptRecord* record, char* sval, char** psvl, unsigned char* update)
{
	std::string prev(*psvl);
	std::string curr(sval);
	
	switch (record->oopt)
	{
		case scriptOOPT_Every_Time:
			*update = SVAL_CHANGE;
			break;
		
		case scriptOOPT_On_Change:
			if (prev != curr)    { *update = SVAL_CHANGE; }
			break;
			
		case scriptOOPT_When_Zero:
			if (curr.empty())    { *update = SVAL_CHANGE; }
			break;
			
		case scriptOOPT_When_Non_zero:
			if (!curr.empty())   { *update = SVAL_CHANGE; }
			break;
			
		case scriptOOPT_Transition_To_Zero:
			if (!prev.empty() && curr.empty())    { *update = SVAL_CHANGE; }
			break;
			
		case scriptOOPT_Transition_To_Non_zero:
			if (!prev.empty() && !curr.empty())    { *update = SVAL_CHANGE; }
			break;
			
		case scriptOOPT_Never:
			break;
	}
}

long speci(dbAddr *paddr, int after)
{
	if (!after)    { return 0; }
	
	scriptRecord* record = (scriptRecord*) paddr->precord;
	
	dbAddr address;
	dbAddr* paddress = &address;
	int field_index = dbGetFieldIndex(paddr);
	
	switch (field_index)
	{	
		case(scriptRecordCODE):
			if (initState(record))    { logError(record); }
			break;
	
		case(scriptRecordINPA):
		case(scriptRecordINPB):
		case(scriptRecordINPC):
		case(scriptRecordINPD):
		case(scriptRecordINPE):
		case(scriptRecordINPF):
		case(scriptRecordINPG):
		case(scriptRecordINPH):
		case(scriptRecordINPI):
		case(scriptRecordINPJ):
		case(scriptRecordINAA):
		case(scriptRecordINBB):
		case(scriptRecordINCC):
		case(scriptRecordINDD):
		case(scriptRecordINEE):
		case(scriptRecordINFF):
		case(scriptRecordINGG):
		case(scriptRecordINHH):
		case(scriptRecordINII):
		case(scriptRecordINJJ):
		case(scriptRecordOUT0):
		case(scriptRecordOUT1):
		case(scriptRecordOUT2):
		case(scriptRecordOUT3):
		case(scriptRecordOUT4):
		case(scriptRecordOUT5):
		case(scriptRecordOUT6):
		case(scriptRecordOUT7):
		case(scriptRecordOUT8):
		case(scriptRecordOUT9):
		{
			int offset = field_index - scriptRecordINPA;
			
			DBLINK* field = &record->inpa + offset;
			double* value = &record->a + offset;
			
			if (field->type == CONSTANT)
			{
				if (field_index <= scriptRecordINPJ)
				{
					recGblInitConstantLink(field, DBF_DOUBLE, value);
					db_post_events(record, value, DBE_VALUE);
				}
			}
			else if (! dbNameToAddr(field->value.pv_link.pvname, paddress))
			{
			}
			else
			{
			}
			
			break;
		}
		
		default:
			break;
	}
	
	return 0;
}


/*
 * Parses the comma separated values in between the parentheses
 * in the CODE field.
 */
int parseParams(lua_State* state, std::string params)
{
	if (params.at(0) != '@')    { return 0; }
	
	size_t start = params.find_first_of("(");
	size_t end   = params.find_last_of(")");
	size_t oob   = std::string::npos;	
	
	/* Syntax error */
	if (end == oob || start == oob || start == end - 1) { return 0; }
	
	std::string parse = params.substr(start + 1, end - start - 1);
	
	int num_params = 0;

	size_t curr = 0;
	size_t next;
	
	do
	{		
		/* Get the next parameter */
		next = parse.find(",", curr);
		
		std::string param;
		
		/* If we find the end of the string, just take everything */
		if   (next == oob)    { param = parse.substr(curr); }
		else                  { param = parse.substr(curr, next - curr); }
		
		int trim_front = param.find_first_not_of(" ");
		int trim_back  = param.find_last_not_of(" ");
		
		param = param.substr(trim_front, trim_back - trim_front + 1);
		
		/* Treat anything with quotes as a string, anything else as a number */
		if (param.at(0) == '"' || param.at(0) == '\'')
		{
			lua_pushstring(state, param.substr(1, param.length() - 2).c_str());
		}
		else
		{
			lua_pushnumber(state, strtod(param.c_str(), NULL));
		}
		
		num_params += 1;
		curr = next + 1;
	} while (curr);
	
	return num_params;
}

void logError(scriptRecord* record)
{
	std::string err(lua_tostring((lua_State*) record->state, -1));
	lua_pop((lua_State*) record->state, 1);
	
	printf("%s\n", err.c_str());
	strcpy(record->err, err.c_str());
	db_post_events(record, &record->err, DBE_VALUE);
}

void writeValue(scriptRecord* record)
{
	ScriptDSET* pscriptDSET = (ScriptDSET*) record->dset;

    if (!pscriptDSET || !pscriptDSET->write) {
        errlogPrintf("%s DSET write does not exist\n", record->name);
        recGblSetSevr(record,SOFT_ALARM,INVALID_ALARM);
        record->pact = TRUE;
        return;
    }
	
	pscriptDSET->write(record);
}


/*
 * Update numeric and string inputs.
 */
void monitor(scriptRecord* record)
{
	double* new_val = &record->a;
	double* old_val = &record->pa;
	
	for (int index = 0; index < NUM_ARGS; index += 1)
	{
		if (*new_val != *old_val)    { db_post_events(record, new_val, DBE_VALUE); }
		
		new_val++;
		old_val++;
	}
	
	char*  new_str = (char*)  &record->aa;
	char** old_str = (char**) &record->paa;
	
	for (int index = 0; index < STR_ARGS; index += 1)
	{
		if (strcmp(new_str, *old_str))    { db_post_events(record, new_str, DBE_VALUE); }
		
		old_str++;
		new_str += STRING_SIZE;
	}
}
