#include "devUtil.h"

#include <longoutRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>

static void pushRecord(struct longoutRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	lua_createtable(state, 0, 3);
	
	lua_pushstring(state, "name");
	lua_pushstring(state, record->name);
	lua_settable(state, -3);
	
	lua_pushstring(state, "desc");
	lua_pushstring(state, record->desc);
	lua_settable(state, -3);
	
	lua_pushstring(state, "val");
	lua_pushnumber(state, record->val);
	lua_settable(state, -3);
}

static long writeData(struct longoutRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);
	
	lua_pop(proto->state, 1);
	return 0;
}


static long initRecord (dbCommon* record)
{
	longoutRecord* longout = (longoutRecord*) record;
	
	longout->dpvt = parseINPOUT(&longout->out);
	
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
} devLuaLongout = {
    6,
    NULL,
    NULL,
    initRecord,
    NULL,
    writeData,
    NULL
};

epicsExportAddress(dset, devLuaLongout);
