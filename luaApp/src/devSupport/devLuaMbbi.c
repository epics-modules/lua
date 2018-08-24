#include "devUtil.h"

#include "lua.h"

#include <mbbiRecord.h>
#include <epicsExport.h>
#include <dbCommon.h>
#include <devSup.h>
#include <string.h>

static void pushRecord(struct mbbiRecord* record)
{
	Protocol* proto = (Protocol*) record->dpvt;
	lua_State* state = proto->state;
	
	luaGeneratePV(state, record->name);
}

static long readData(struct mbbiRecord* record)
{
	int type, index;
	Protocol* proto = (Protocol*) record->dpvt;
	
	lua_getglobal(proto->state, proto->function_name);
	pushRecord(record);
	runFunction(proto);

	type = lua_type(proto->state, -1);
	
	switch (type)
	{		
		case LUA_TNUMBER:
		{
			int val;
			
			if (! lua_isinteger(proto->state, -1))
			{ 
				lua_pop(proto->state, 1);
				return -1;
			}
			
			val = lua_tointeger(proto->state, -1);
			
			if (record->sdef)
			{
				for (index = 0; index < 16; index += 1)
				{
					if ((&record->zrvl)[index])
					{
						if (record->mask)    { val &= record->mask; }
						record->rval = val;
						lua_pop(proto->state, 1);
						return 2;
					}
				}
			}
			
			record->val = (short) val;
			lua_pop(proto->state, 1);
			return 2;
		}
		
		case LUA_TSTRING:
		{
			const char* buffer = lua_tostring(proto->state, -1);
			lua_pop(proto->state, 1);
			for (index = 0; index < 16; index += 1)
			{
				if (strcmp((&record->zrst)[index], buffer) == 0)
				{
					record->val = (short) index;
					return 2;
				}
			}
			
			return -1;
		}
		
		case LUA_TNIL:
			lua_pop(proto->state, 1);
			return 0;
		
		default:
			lua_pop(proto->state, 1);
			return -1;
	}
	
	return 0;
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
