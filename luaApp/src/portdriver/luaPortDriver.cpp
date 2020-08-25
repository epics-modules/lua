#include <asynPortDriver.h>
#include <asynPortClient.h>
#include "stdio.h"
#include <cstring>
#include <string>
#include <strings.h>
#include <epicsExport.h>
#include "luaPortDriver.h"
#include "luaEpics.h"

static int l_call(lua_State* state)
{
	lua_getfield(state, 1, "name");
	const char* param_name = lua_tostring(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "direction");
	std::string direction(lua_tostring(state, -1));
	lua_pop(state, 1);
	
	std::string data(luaL_checkstring(state, 2));
	
	lua_getglobal(state, "_params");
	lua_getfield(state, -1, param_name);
	
	if (lua_isnil(state, -1))
	{
		lua_pop(state, 1);
		lua_newtable(state);
	}
	
	lua_pushstring(state, param_name);
	lua_setfield(state, -2, "name");
	
	int status;
	
	if (direction == "read")
	{
		data = "return function(self) " + data + " end";
		status = luaL_dostring(state, data.c_str());
		if (!status) { lua_setfield(state, -2, "read_bind"); }
	}
	else
	{ 
		data = "return function(self, value) " + data + " end";
		status = luaL_dostring(state, data.c_str());
		if (!status) { lua_setfield(state, -2, "write_bind"); }
	}
	
	if (status)
	{
		return luaL_error(state, lua_tostring(state, -1));
	}
	
	lua_getfield(state, 1, "type");
	lua_setfield(state, -2, "type");
	
	lua_setfield(state, -2, param_name);
	lua_pop(state, 1);
	
	return 0;
}

static int l_addname(lua_State* state)
{
	static const luaL_Reg data_set[] = {
		{"__call", l_call},
		{NULL, NULL}
	};
	
	const char* param_name = luaL_checkstring(state, 2);
	
	lua_pushvalue(state, 1);
	lua_pushstring(state, param_name);
	lua_setfield(state, -2, "name");
	
	luaL_newmetatable(state, "data_set");
	luaL_setfuncs(state, data_set, 0);
	lua_pop(state, 1);
	
	luaL_setmetatable(state, "data_set");
	
	return 1;
}

static int l_readwrite(lua_State* state)
{
	const char* readwrite = luaL_checkstring(state, 2);
	
	lua_pushvalue(state, 1);
	
	if      (strcasecmp(readwrite, "read") == 0)   { lua_pushstring(state, "read"); }
	else if (strcasecmp(readwrite, "write") == 0)  { lua_pushstring(state, "write"); }
	else                                           { lua_pop(state, 1); return 0;  }
	
	lua_setfield(state, -2, "direction");
	
	static const luaL_Reg param_call[] = {
		{"__call", l_addname},
		{NULL, NULL}
	};
	
	luaL_newmetatable(state, "param_call");
	luaL_setfuncs(state, param_call, 0);
	lua_pop(state, 1);
	
	luaL_setmetatable(state, "param_call");
	
	return 1;
}

static int l_index(lua_State* state)
{
	const char* param_type = luaL_checkstring(state, 2);
	
	lua_newtable(state);
	
	if      (strcasecmp(param_type, "int32") == 0)         { lua_pushinteger(state, 1); }
	else if (strcasecmp(param_type, "uint32digital") == 0) { lua_pushinteger(state, 2); }
	else if (strcasecmp(param_type, "float64") == 0)       { lua_pushinteger(state, 3); }
	else if (strcasecmp(param_type, "string") == 0)        { lua_pushinteger(state, 4); }
	else                                                   { lua_pushinteger(state, 0); }
	
	lua_setfield(state, -2, "type");
	
	static const luaL_Reg in_out[] = {
		{"__index", l_readwrite},
		{NULL, NULL}
	};
	
	luaL_newmetatable(state, "in_out");
	luaL_setfuncs(state, in_out, 0);
	lua_pop(state, 1);
	
	luaL_setmetatable(state, "in_out");
	
	return 1;
}

luaPortDriver::luaPortDriver(const char* port_name, const char* lua_filepath, const char* lua_macros)
	:asynPortDriver(port_name, 1, 
		asynInt32Mask | asynFloat64Mask, 
		asynInt32Mask | asynFloat64Mask, 
		0, 1, 0, 0)
{
	static const luaL_Reg param_get[] = {
		{"__index", l_index},
		{NULL, NULL}
	};
	
	this->state = luaCreateState();
	luaL_dostring(this->state, "asyn = require('asyn')");
	
	std::string line = "self = asyn.driver '";
	line += port_name;
	line += "'";
	luaL_dostring(this->state, line.c_str());
	
	
	luaL_newmetatable(this->state, "param_get");
	luaL_setfuncs(this->state, param_get, 0);
	lua_pop(this->state, 1);
	
	lua_newtable(this->state);
	luaL_setmetatable(this->state, "param_get");
	lua_setglobal(this->state, "param");
	
	lua_newtable(state);
	lua_setglobal(this->state, "_params");
	
	luaLoadMacros(this->state, lua_macros);
	
	int status = luaLoadScript(this->state, lua_filepath);
	
	if (status) 
	{ 
		const char* msg = lua_tostring(state, -1); 
		printf("%s\n", msg);
		return;
	}
	
	lua_newtable(this->state);
	lua_setglobal(this->state, "_functions");
	
	lua_getglobal(this->state, "_params");
	lua_pushnil(this->state);
	
	int index;
	
	while(lua_next(state, -2))
	{
		lua_getfield(this->state, -1, "name");
		const char* param_name = lua_tostring(this->state, -1);
		lua_pop(this->state, 1);
		
		lua_getfield(this->state, -1, "type");
		int param_type = luaL_optinteger(this->state, -1, 1);
		lua_pop(this->state, 1);
		
		this->createParam(param_name, (asynParamType) param_type, &index);
		
		lua_getglobal(this->state, "_functions");
		lua_newtable(this->state);
		
		lua_getfield(this->state, -3, "read_bind");
		lua_setfield(this->state, -2, "read");
		
		lua_getfield(this->state, -3, "write_bind");
		lua_setfield(this->state, -2, "write");
		
		lua_seti(this->state, -2, index);
		
		lua_pop(this->state, 2);
	}
	
	lua_pop(this->state, 1);
}

void luaPortDriver::getReadFunction(int index)
{
	lua_getglobal(this->state, "_functions");
	lua_geti(this->state, -1, index);
	lua_remove(this->state, -2);
	
	lua_getfield(this->state, -1, "read");
	lua_remove(this->state, -2);
}

int luaPortDriver::callReadFunction()
{
	lua_getglobal(this->state, "self");
	int status = lua_pcall(this->state, 1, 1, 0);
	
	if (status)
	{
		const char* msg = lua_tostring(this->state, -1);
		printf("%s\n", msg);
	}
	
	return status;
}

void luaPortDriver::getWriteFunction(int index)
{
	lua_getglobal(this->state, "_functions");
	lua_geti(this->state, -1, index);
	lua_getfield(this->state, -1, "write");
	lua_remove(this->state, -3);
	lua_remove(this->state, -2);
}

int luaPortDriver::callWriteFunction()
{
	lua_getglobal(this->state, "self");
	lua_pushvalue(this->state, -2);
	lua_remove(this->state, -3);
	
	int status = lua_pcall(this->state, 2, 0, 0);
	
	if (status) 
	{ 
		const char* msg = lua_tostring(state, -1); 
		printf("%s\n", msg);
	}
	
	return status;
}

asynStatus luaPortDriver::writeOctet(asynUser* pasynuser, const char* value, size_t maxChars, size_t* actual)
{
	this->getWriteFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, -1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::writeOctet(pasynuser, value, maxChars, actual);
	}
	
	if (value != NULL)
	{
		std::string output(value);
		output.erase(maxChars);
		
		lua_pushstring(this->state, output.c_str());
		if (this->callWriteFunction())   { return asynError; }
	}
	
	return asynSuccess;
}
		

asynStatus luaPortDriver::writeFloat64(asynUser* pasynuser, epicsFloat64 value)
{
	this->getWriteFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, -1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::writeFloat64(pasynuser, value);
	}
	
	lua_pushnumber(this->state, value);
	if (this->callWriteFunction())    { return asynError; }
	
	return asynSuccess;
}

asynStatus luaPortDriver::writeInt32(asynUser* pasynuser, epicsInt32 value)
{
	this->getWriteFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, -1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::writeInt32(pasynuser, value);
	}
	
	lua_pushinteger(this->state, value);
	if (this->callWriteFunction())    { return asynError; }
	
	return asynSuccess;
}

asynStatus luaPortDriver::readOctet(asynUser* pasynuser, char* value, size_t maxChars, size_t* actual, int* eomReason)
{
	this->getReadFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, 1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::readOctet(pasynuser, value, maxChars, actual, eomReason);
	}
	
	if (this->callReadFunction())    { return asynError; }
	
	if (! lua_isnil(this->state, -1))
	{
		const char* retval = luaL_checklstring(this->state, -1, actual);
		
		if (retval != NULL)
		{
			std::string output(retval);
			output.copy(value, maxChars);
		}
		
		lua_pop(this->state, 1);
	}
	
	return asynSuccess;
}

asynStatus luaPortDriver::readFloat64(asynUser* pasynuser, epicsFloat64* value)
{
	this->getReadFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, 1))
	{	
		lua_pop(this->state, 1);
		return asynPortDriver::readFloat64(pasynuser, value);
	}
	
	if(this->callReadFunction())    { return asynError; }
	
	if (! lua_isnil(this->state, -1))
	{
		epicsFloat64 retval = lua_tonumber(this->state, -1);
		lua_pop(this->state, 1);
		
		*value = retval;
	}
	
	return asynSuccess;
}
	

asynStatus luaPortDriver::readInt32(asynUser* pasynuser, epicsInt32* value)
{	
	this->getReadFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, 1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::readInt32(pasynuser, value);
	}
	
	if (this->callReadFunction())    { return asynError; }
	
	if (! lua_isnil(this->state, -1))
	{
		epicsInt32 retval = lua_tointeger(this->state, -1);
		lua_pop(this->state, -1);
		
		*value = retval;
	}
	
	return asynSuccess;
}

luaPortDriver::~luaPortDriver()
{
	lua_close(this->state);
}

int lnewdriver(lua_State* state)
{
	lua_settop(state, 3);
	const char* port_name = luaL_checkstring(state, 1);
	const char* filepath = luaL_checkstring(state, 2);
	const char* macros = lua_tostring(state, 3);
	
	new luaPortDriver(port_name, filepath, macros);
	
	return 0;
}

static void portDriverRegister(void)
{ 
	luaRegisterFunction("luaPortDriver", lnewdriver);
}

extern "C"
{
	epicsExportRegistrar(portDriverRegister);
}
