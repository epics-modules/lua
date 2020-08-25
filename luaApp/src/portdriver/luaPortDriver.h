#ifndef INC_LUAPORTDRIVER_H
#define INC_LUAPORTDRIVER_H

#include "asynPortDriver.h"
#include "luaEpics.h"
#include <map>
#include <string>
#include "epicsTypes.h"

class luaPortDriver : public asynPortDriver
{
	public:
		luaPortDriver(const char* port_name, const char* lua_filepath, const char* lua_macros);
		~luaPortDriver();
				
		asynStatus writeInt32(asynUser* pasynuser, epicsInt32 value);
		asynStatus readInt32(asynUser* pasynuser, epicsInt32* value);
		
		asynStatus writeFloat64(asynUser* pasynuser, epicsFloat64 value);
		asynStatus readFloat64(asynUser* pasynuser, epicsFloat64* value);
		
		asynStatus writeOctet(asynUser* pasynuser, const char* value, size_t maxChars, size_t* actual);
		asynStatus readOctet(asynUser* pasynuser, char* value, size_t maxChars, size_t* actual, int* eomReason);
		
	protected:
		void getReadFunction(int index);
		void getWriteFunction(int index);
		
		int callReadFunction();
		int callWriteFunction();
		
		lua_State* state;
};

int lnewdriver(lua_State* state);

#endif
