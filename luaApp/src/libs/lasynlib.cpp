#include <asynPortDriver.h>
#include <asynPortClient.h>
#include <asynShellCommands.h>
#include "stdio.h"
#include <cstring>
#include <string>
#include <sstream>
#include <epicsExport.h>
#include "lasynlib.h"

#if defined(WIN32) || defined(_WIN32)
#include <string.h>
#define strcasecmp(x,y) strcmpi(x,y)
#else
#include <strings.h>
#endif


static asynPortDriver* find_driver(lua_State* state, const char* port_name)
{
	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port_name);

	if (driver == NULL || std::string(driver->portName) != std::string(port_name) )
	{ 
		luaL_error(state, "No asynPortDriver found with name: %s\n", port_name);
		return NULL;
	}
	
	return driver;
}

static int asyn_read(lua_State* state, asynOctetClient* port)
{
	try
	{
		std::string output;
		char buffer[128];

		size_t numread;
		int eomReason;

		do
		{
			port->read(buffer, sizeof(buffer), &numread, &eomReason);
			output += std::string(buffer, numread);
		} while (eomReason & ASYN_EOM_CNT);

		if (! output.empty())
		{ 
			lua_pushstring(state, output.c_str());
			return 1;
		}
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception while reading\n"); }

	lua_pushnil(state);
	return 1;
}

static int asyn_write(lua_State* state, asynOctetClient* port, std::string data)
{
	try
	{
		size_t numwrite;
		port->write(data.c_str(), data.size(), &numwrite);
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception while writing\n"); }
	
	return 0;
}

static int asyn_writeread(lua_State* state, asynOctetClient* client, std::string data)
{
	try
	{
		std::string output;
		size_t numwrite, numread;
		char buffer[256];
		int eomReason;

		client->writeRead(data.c_str(), data.size(), buffer, sizeof(buffer), &numwrite, &numread, &eomReason);
		output += std::string(buffer, numread);
		
		while (eomReason & ASYN_EOM_CNT)
		{
			client->read(buffer, sizeof(buffer), &numread, &eomReason);
			output += std::string(buffer, numread);
		}
		
		if (! output.empty())
		{ 
			lua_pushstring(state, output.c_str());
			return 1;
		}
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception during writeread\n"); }

	lua_pushnil(state);
	return 1;
}

static int l_read(lua_State* state)
{
	lua_settop(state, 3);

	const char* port = luaL_checkstring(state, 1);
	int addr = (int) lua_tonumber(state, 2);
	const char* param = lua_tostring(state, 3);

	int isnum;
	
	lua_getglobal(state, "InTerminator");
	const char* in_term = lua_tostring(state, -1);
	lua_remove(state, -1);

	lua_getglobal(state, "ReadTimeout");
	double timeout = lua_tonumberx(state, -1, &isnum);
	lua_remove(state, -1);

	asynOctetClient input(port, addr, param);
	
	if (isnum)    { input.setTimeout(timeout); }
	if (in_term)  { input.setInputEos(in_term, strlen(in_term)); }

	return asyn_read(state, &input);
}

static int l_write(lua_State* state)
{
	lua_settop(state, 4);

	const char* data = luaL_checkstring(state, 1);
	const char* port = luaL_checkstring(state, 2);
	int addr = (int) lua_tonumber(state, 3);
	const char* param = lua_tostring(state, 4);
	
	int isnum;
	
	lua_getglobal(state, "OutTerminator");
	const char* out_term = lua_tostring(state, -1);
	lua_remove(state, -1);

	lua_getglobal(state, "WriteTimeout");
	double timeout = lua_tonumberx(state, -1, &isnum);
	lua_remove(state, -1);

	asynOctetClient output(port, addr, param);
	
	if (isnum)       { output.setTimeout(timeout); }
	if (out_term)    { output.setOutputEos(out_term, strlen(out_term)); }
	
	return asyn_write(state, &output, data);
}

static int l_writeread(lua_State* state)
{
	lua_settop(state, 4);

	const char* data = luaL_checkstring(state, 1);
	const char* port = luaL_checkstring(state, 2);
	int addr = (int) lua_tonumber(state, 3);
	const char* param = lua_tostring(state, 4);

	int isnum;
	
	lua_getglobal(state, "OutTerminator");
	const char* out_term = lua_tostring(state, -1);
	lua_remove(state, -1);

	lua_getglobal(state, "InTerminator");
	const char* in_term = lua_tostring(state, -1);
	lua_remove(state, -1);

	lua_getglobal(state, "WriteReadTimeout");
	double timeout = lua_tonumberx(state, -1, &isnum);
	lua_remove(state, -1);
	
	asynOctetClient client(port, addr, param);
	
	if (isnum)       { client.setTimeout(timeout); }
	if (out_term)    { client.setOutputEos(out_term, strlen(out_term)); }
	if (in_term)     { client.setInputEos(in_term, strlen(in_term)); }

	return asyn_writeread(state, &client, data);
}

static int l_setOption(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);

	const char* port = luaL_checkstring(state, 1);
	
	int addr = 0;
	if (num_ops == 4)    { addr = luaL_checkinteger(state, 2); }
	
	const char* key = luaL_checkstring(state, num_ops - 1);
	const char* val = luaL_checkstring(state, num_ops);
	
	int status = asynSetOption(port, addr, key, val);
	
	lua_pushinteger(state, status);
	return 1;
}

static int l_setOutTerminator(lua_State* state)
{
	luaL_checkstring(state, 1);
	lua_setglobal(state, "OutTerminator");
	return 0;
}

static int l_setInTerminator(lua_State* state)
{
	luaL_checkstring(state, 1);
	lua_setglobal(state, "InTerminator");
	return 0;
}

static int l_setWriteTimeout(lua_State* state)
{
	luaL_checknumber(state, 1);
	lua_setglobal(state, "WriteTimeout");
	return 0;
}

static int l_setReadTimeout(lua_State* state)
{
	luaL_checknumber(state, 1);
	lua_setglobal(state, "ReadTimeout");
	return 0;
}

static int l_setWriteReadTimeout(lua_State* state)
{
	luaL_checknumber(state, 1);
	lua_setglobal(state, "WriteReadTimeout");
	return 0;
}

static int l_getOutTerminator(lua_State* state)    { return lua_getglobal(state, "OutTerminator"); }
static int l_getInTerminator(lua_State* state)     { return lua_getglobal(state, "InTerminator"); }
static int l_getWriteTimeout(lua_State* state)     { return lua_getglobal(state, "WriteTimeout"); }
static int l_getReadTimeout(lua_State* state)      { return lua_getglobal(state, "ReadTimeout"); }
static int l_getWriteReadTimeout(lua_State* state) { return lua_getglobal(state, "WriteReadTimeout"); }

static int asyn_writeparam(lua_State* state, asynPortDriver* port, int addr, const char* param, int val_index)
{	
	int index;
		
	asynStatus status = port->findParam(addr, param, &index);
	
	if (status == asynParamNotFound)    { return 0; }
	
	asynParamType partype;
	
	port->getParamType(addr, index, &partype);
	
	asynUser* pasynuser = pasynManager->createAsynUser(NULL, NULL);
	pasynManager->connectDevice(pasynuser, port->portName, addr);
	
	pasynuser->reason = index;
	
	asynStandardInterfaces* interfaces = port->getAsynStdInterfaces();
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data = luaL_checkinteger(state, val_index);
			
			asynInt32* inter = (asynInt32*) interfaces->int32.pinterface;
			inter->write(port, pasynuser, data);
			
			break;
		}
		
		case asynParamFloat64:
		{
			epicsFloat64 data = luaL_checknumber(state, val_index);
			
			asynFloat64* inter = (asynFloat64*) interfaces->float64.pinterface;
			inter->write(port, pasynuser, data);
			break;
		}
		
		case asynParamOctet:
		{
			const char* data = luaL_checkstring(state, val_index);
			
			asynOctet* inter = (asynOctet*) interfaces->octet.pinterface;
			
			size_t num_trans;
			inter->write(port, pasynuser, data, strlen(data), &num_trans);
			break;
		}
		
		default:
			break;
	}
	
	pasynManager->disconnect(pasynuser);
	pasynManager->freeAsynUser(pasynuser);
	
	return 0;
}

static int asyn_readparam(lua_State* state, asynPortDriver* port, int addr, const char* param)
{
	int index;
		
	asynStatus status = port->findParam(addr, param, &index);
	
	if (status == asynParamNotFound)    { return 0; }
	
	asynParamType partype;
	
	port->getParamType(addr, index, &partype);
	
	asynUser* pasynuser = pasynManager->createAsynUser(NULL, NULL);
	pasynManager->connectDevice(pasynuser, port->portName, addr);
	
	pasynuser->reason = index;
	
	asynStandardInterfaces* interfaces = port->getAsynStdInterfaces();
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data;
			asynInt32* inter = (asynInt32*) interfaces->int32.pinterface;
			
			if(asynSuccess == inter->read(port, pasynuser, &data))
			{
				lua_pushinteger(state, data);
				return 1;
			}
			
			break;
		}
		
		case asynParamFloat64:
		{
			epicsFloat64 data;
			asynFloat64* inter = (asynFloat64*) interfaces->float64.pinterface;
			
			if(asynSuccess == inter->read(port, pasynuser, &data))
			{
				lua_pushnumber(state, data);
				return 1;
			}
			
			break;
		}
		
		case asynParamOctet:
		{
			char buffer[256] = { '\0' };
			
			size_t num_trans;
			int eomreason;
			
			asynOctet* inter = (asynOctet*) interfaces->octet.pinterface;
			if (asynSuccess == inter->read(port, pasynuser, buffer, 256, &num_trans, &eomreason))
			{
				lua_pushstring(state, buffer);
				return 1;
			}
			
			break;
		}
		
		default:
			return 0;
	}
	
	return 0;
}

static int asyn_setparam(lua_State* state, asynPortDriver* port, int addr, const char* param, int val_index)
{
	int index;
		
	asynStatus status = port->findParam(addr, param, &index);
	
	if (status == asynParamNotFound)    { return 0; }
	
	asynParamType partype;
	
	port->getParamType(addr, index, &partype);
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data = luaL_checkinteger(state, val_index);
			port->setIntegerParam(addr, index, data);
			return 0;
		}
		
		case asynParamFloat64:
		{
			epicsFloat64 data = luaL_checknumber(state, val_index);
			port->setDoubleParam(addr, index, data);
			return 0;
		}
		
		case asynParamOctet:
		{
			const char* data = luaL_checkstring(state, val_index);
			port->setStringParam(addr, index, data);
			return 0;
		}
		
		default:
			return 0;
	}
	
	return 0;
}

static int asyn_getparam(lua_State* state, asynPortDriver* port, int addr, const char* param)
{
	int index;
		
	asynStatus status = port->findParam(addr, param, &index);
	
	if (status == asynParamNotFound)    { return 0; }
	
	asynParamType partype;
	
	port->getParamType(addr, index, &partype);
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data;
			port->getIntegerParam(addr, index, &data);
			lua_pushinteger(state, data);
			return 1;
		}
		
		case asynParamFloat64:
		{
			epicsFloat64 data;
			port->getDoubleParam(addr, index, &data);
			lua_pushnumber(state, data);
			return 1;
		}
		
		case asynParamOctet:
		{
			std::string data;
			port->getStringParam(addr, index, data);
			lua_pushstring(state, data.c_str());
			return 1;
		}
		
		default:
			return 0;
	}
	
	return 0;
}

static int l_setParam(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);

	const char* port = luaL_checkstring(state, 1);
	
	int addr = 0;
	if (num_ops == 4)    { addr = luaL_checkinteger(state, 2); }
	
	const char* param = luaL_checkstring(state, num_ops - 1);
	
	asynPortDriver* driver = find_driver(state, port);
	return asyn_setparam(state, driver, addr, param, num_ops); 
}

static int l_writeParam(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);

	const char* port = luaL_checkstring(state, 1);
	
	int addr = 0;
	if (num_ops == 4)    { addr = luaL_checkinteger(state, 2); }
	
	const char* param = luaL_checkstring(state, num_ops - 1);
	
	asynPortDriver* driver = find_driver(state, port);
	return asyn_writeparam(state, driver, addr, param, num_ops); 
}

static int l_getParam(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 3);
	
	const char* port = luaL_checkstring(state, 1);

	int addr = 0;
	if (num_ops == 3)    { addr = luaL_checkinteger(state, 2); }
	
	const char* param = luaL_checkstring(state, num_ops);
	
	asynPortDriver* driver = find_driver(state, port);
	return asyn_getparam(state, driver, addr, param);
}

static int l_readParam(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 3);
	
	const char* port = luaL_checkstring(state, 1);

	int addr = 0;
	if (num_ops == 3)    { addr = luaL_checkinteger(state, 2); }
	
	const char* param = luaL_checkstring(state, num_ops);
	
	asynPortDriver* driver = find_driver(state, port);
	return asyn_readparam(state, driver, addr, param);
}

static int l_setIntegerParam(lua_State* state)
{
	luaL_checkinteger(state, lua_gettop(state));
	return l_setParam(state);
}

static int l_getIntegerParam(lua_State* state)
{
	int ret = l_getParam(state);
	luaL_checkinteger(state, -1);
	return ret;
}

static int l_setDoubleParam(lua_State* state)
{
	luaL_checknumber(state, lua_gettop(state));
	return l_setParam(state);
}

static int l_getDoubleParam(lua_State* state)
{
	int ret = l_getParam(state);
	luaL_checknumber(state, -1);
	return ret;
}

static int l_setStringParam(lua_State* state)
{
	luaL_checktype(state, lua_gettop(state), LUA_TSTRING);
	return l_setParam(state);
}

static int l_getStringParam(lua_State* state)
{	
	int ret = l_getParam(state);
	luaL_checktype(state, -1, LUA_TSTRING);
	return ret;
}

static int l_callParamCallbacks(lua_State* state)
{
	lua_settop(state, 2);

	const char* port = luaL_checkstring(state, 1);
	int addr = lua_tointeger(state, 2);

	find_driver(state, port)->callParamCallbacks(addr);
	return 0;
}

static void asyn_settrace(lua_State* state, std::string portname, int addr, std::string key, bool val)
{
	int mask = 0;
	
	if      (key == "error")     { mask = 0x0001; }
	else if (key == "device")    { mask = 0x0002; }
	else if (key == "filter")    { mask = 0x0004; }
	else if (key == "driver")    { mask = 0x0008; }
	else if (key == "flow")      { mask = 0x0010; }
	else if (key == "warning")   { mask = 0x0020; }
		
	asynUser *pasynUser=NULL;
    asynStatus status;

	if (! portname.empty())
	{
		pasynUser = pasynManager->createAsynUser(0,0);
		status = pasynManager->connectDevice(pasynUser,portname.c_str(),addr);
		
		if(status!=asynSuccess) 
		{
			printf("%s\n",pasynUser->errorMessage);
			pasynManager->freeAsynUser(pasynUser);
			return;
		}
	}
	
	int prevmask = pasynTrace->getTraceMask(pasynUser);
	
	if (val)    { status = pasynTrace->setTraceMask(pasynUser, prevmask | mask); }
	else        { status = pasynTrace->setTraceMask(pasynUser, prevmask & ~mask); }
	
	if (status!=asynSuccess)    { printf("%s\n",pasynUser->errorMessage); }
	if (pasynUser)              { pasynManager->freeAsynUser(pasynUser); }
}

static void asyn_settraceio(lua_State* state, std::string portname, int addr, std::string key, bool val)
{
	int mask = 0;
	
	if      (key == "nodata")   { mask = 0x0001; }
	else if (key == "ascii")    { mask = 0x0002; }
	else if (key == "escape")   { mask = 0x0004; }
	else if (key == "hex")      { mask = 0x0008; }
		
	asynUser *pasynUser=NULL;
    asynStatus status;

	if (! portname.empty())
	{
		pasynUser = pasynManager->createAsynUser(0,0);
		status = pasynManager->connectDevice(pasynUser,portname.c_str(),addr);
		
		if(status!=asynSuccess) 
		{
			printf("%s\n",pasynUser->errorMessage);
			pasynManager->freeAsynUser(pasynUser);
			return;
		}
	}
	
	int prevmask = pasynTrace->getTraceIOMask(pasynUser);
	
	if (val)    { status = pasynTrace->setTraceIOMask(pasynUser, prevmask | mask); }
	else        { status = pasynTrace->setTraceIOMask(pasynUser, prevmask & ~mask); }
	
	if (status!=asynSuccess)    { printf("%s\n",pasynUser->errorMessage); }
	if (pasynUser)              { pasynManager->freeAsynUser(pasynUser); }
}

static int l_setTrace(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);
	
	std::string portname = LuaStack<std::string>::get(state, 1);
	
	int addr = 0;
	
	if (lua_isnumber(state, 2))     { addr = LuaStack<int>::get(state, 2); }
	
	if (lua_type(state, num_ops) != LUA_TTABLE)
	{
		std::string key = LuaStack<std::string>::get(state, num_ops - 1);
		bool val = LuaStack<bool>::get(state, num_ops);
		
		asyn_settrace(state, portname, addr, key, val);
	}
	else
	{
		auto dict = LuaStack<std::map<std::string, bool> >::get(state, num_ops);
		
		for (auto it = dict.begin(); it != dict.end(); it++)
		{
			asyn_settrace(state, portname, addr, it->first, it->second);
		}
	}
	
	return 0;
}

static int l_setTraceIO(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);
	
	std::string portname = LuaStack<std::string>::get(state, 1);
	
	int addr = 0;
	
	if (lua_isnumber(state, 2))     { addr = LuaStack<int>::get(state, 2); }
	
	if (lua_type(state, num_ops) != LUA_TTABLE)
	{
		std::string key = LuaStack<std::string>::get(state, num_ops - 1);
		bool val = LuaStack<bool>::get(state, num_ops);
		
		asyn_settraceio(state, portname, addr, key, val);
	}
	else
	{
		auto dict = LuaStack<std::map<std::string, bool> >::get(state, num_ops);
		
		for (auto it = dict.begin(); it != dict.end(); it++)
		{
			asyn_settraceio(state, portname, addr, it->first, it->second);
		}
	}
	
	return 0;
}


// ################################
// # lua asynPortDriver Functions #
// ################################


lua_asynPortDriver::lua_asynPortDriver(std::string driver_name, int addr)
{
	this->driver = (asynPortDriver*) findAsynPortDriver(driver_name.c_str());
	this->addr = addr;
}

static int lapd_readParam(lua_State* state)
{
	lua_settop(state, 2);
	
	lua_asynPortDriver* self = LuaStack<lua_asynPortDriver*>::get(state, 1);
	std::string fieldname = LuaStack<std::string>::get(state, 2);
				
	return asyn_readparam(state, self->driver, self->addr, fieldname.c_str());
}

static int lapd_writeParam(lua_State* state)
{
	lua_settop(state, 3);
	
	lua_asynPortDriver* self = LuaStack<lua_asynPortDriver*>::get(state, 1);
	std::string fieldname = LuaStack<std::string>::get(state, 2);
		
	return asyn_writeparam(state, self->driver, self->addr, fieldname.c_str(), 3);
}

static int lapd_callParamCallbacks(lua_State* state)    
{ 
	lua_settop(state, 1);
	
	lua_asynPortDriver* self = LuaStack<lua_asynPortDriver*>::get(state, 1);
	self->driver->callParamCallbacks(self->addr); 
	
	return 0;
}

static int l_driverindex(lua_State* state)    
{	
	lua_asynPortDriver* self = LuaStack<lua_asynPortDriver*>::get(state, 1);
	
	if (lua_isstring(state, 2))
	{
		std::string fieldname = LuaStack<std::string>::get(state, 2);
		
		// Get class table to check for existing functions
		lua_getmetatable(state, -2);
		lua_getfield(state, -1, fieldname.c_str()); 
		
		if (! lua_isnil(state, -1))
		{		
			// Remove metatable
			lua_remove(state, -2);
			
			return 1;
		}
		
		lua_pop(state, 2);
		
		if      (fieldname == "portName")    { lua_pushstring(state, self->driver->portName); }
		else if (fieldname == "maxAddr")     { lua_pushinteger(state, self->driver->maxAddr); }
		else
		{					
			return asyn_getparam(state, self->driver, self->addr, fieldname.c_str());
		}
		
		return 1;
	}
	else if (lua_isinteger(state, 2))
	{
		int addr = LuaStack<int>::get(state, 2);
	
		std::stringstream code;
		
		code << "return asynPortDriver.new('" << self->driver->portName;
		code << "', " << addr << ")";
		
		luaL_dostring(state, code.str().c_str());
		
		return 1;
	}
	
	return 0;
}

static int l_drivernewindex(lua_State* state) 
{ 
	lua_asynPortDriver* self = LuaStack<lua_asynPortDriver*>::get(state, 1);

	std::string fieldname = std::string(lua_tostring(state, 2));
	
	if      (fieldname == "portName")  { return 0; }
	else if (fieldname == "maxAddr")   { return 0; }
	else
	{
		return asyn_setparam(state, self->driver, self->addr, fieldname.c_str(), 3);
	}
}


// #################################
// # lua_asynOctetClient Functions #
// #################################

lua_asynOctetClient::lua_asynOctetClient(std::string port_name, int addr, std::string param)
{
	this->port = new asynOctetClient(port_name.c_str(), addr, param.c_str());
	this->port->flush();
	
	this->name = port_name;
	this->addr = addr;
	this->param = param;
}

lua_asynOctetClient::~lua_asynOctetClient()    { delete this->port; }

static int laoc_write(lua_State* state)   
{
	lua_asynOctetClient* self = LuaStack<lua_asynOctetClient*>::get(state, 1);
	return asyn_write(state, self->port, LuaStack<std::string>::get(state, 2)); 
}

static int laoc_read(lua_State* state)    
{ 
	lua_asynOctetClient* self = LuaStack<lua_asynOctetClient*>::get(state, 1);

	return asyn_read(state, self->port);
}

static int laoc_writeread(lua_State* state) 
{ 
	lua_asynOctetClient* self = LuaStack<lua_asynOctetClient*>::get(state, 1);
	return asyn_writeread(state, self->port, LuaStack<std::string>::get(state, 2));
}

void lua_asynOctetClient::trace(lua_State* state)
{
	lua_settop(state, 3);
	
	if (lua_type(state, 2) != LUA_TTABLE)
	{
		std::string key = LuaStack<std::string>::get(state, 2);
	
		bool setval = true;
		if (! lua_isnil(state, 3))    { setval = lua_toboolean(state, 3); }
		
		asyn_settrace(state, this->name, this->addr, key, setval);
	}
	else
	{
		auto dict = LuaStack<std::map<std::string, bool> >::get(state, 2);
		
		for (auto it = dict.begin(); it != dict.end(); it++)
		{
			asyn_settrace(state, this->name, this->addr, it->first, it->second);
		}
	}
}

void lua_asynOctetClient::traceio(lua_State* state)
{
	lua_settop(state, 3);
	
	if (lua_type(state, 2) != LUA_TTABLE)
	{
		std::string key = LuaStack<std::string>::get(state, 2);
	
		bool setval = true;
		if (! lua_isnil(state, 3))    { setval = lua_toboolean(state, 3); }
		
		asyn_settraceio(state, this->name, this->addr, key, setval);
	}
	else
	{
		auto dict = LuaStack<std::map<std::string, bool> >::get(state, 2);
		
		for (auto it = dict.begin(); it != dict.end(); it++)
		{
			asyn_settraceio(state, this->name, this->addr, it->first, it->second);
		}
	}
}

int lua_asynOctetClient::option(lua_State* state)
{
	lua_settop(state, 3);
	
	std::string key = LuaStack<std::string>::get(state, 2);
	std::string val = LuaStack<std::string>::get(state, 3);
	
	return asynSetOption(this->name.c_str(), this->addr, key.c_str(), val.c_str());
}

int l_clientindex(lua_State* state)
{	
	lua_asynOctetClient* self = LuaStack<lua_asynOctetClient*>::get(state, 1);
	std::string fieldname = std::string(lua_tostring(state, 2));
		
	// Get class table to check for existing functions
	lua_getmetatable(state, -2);
	lua_getfield(state, -1, fieldname.c_str()); 
	
	if (! lua_isnil(state, -1))
	{		
		// Remove metatable
		lua_remove(state, -2);
		
		return 1;
	}
	
	lua_pop(state, 2);
	
	if (fieldname == "OutTerminator")
	{
		char out_term[20] = {'\0'};
		int actual;
		
		self->port->getOutputEos(out_term, 20, &actual);
	
		lua_pushstring(state, out_term);
		return 1;
	}
	else if (fieldname == "InTerminator")
	{
		char in_term[20] = {'\0'};
		int actual;
		
		self->port->getInputEos(in_term, 20, &actual);
	
		lua_pushstring(state, in_term);
		return 1;
	}
	
	return 0;
}

int l_clientnewindex(lua_State* state)
{
	lua_asynOctetClient* self = LuaStack<lua_asynOctetClient*>::get(state, 1);
	std::string fieldname = std::string(lua_tostring(state, 2));
	
	if (fieldname == "OutTerminator" && ! lua_isnil(state, 3))
	{
		std::string out_term = LuaStack<std::string>::get(state, 3);
		self->port->setOutputEos(out_term.c_str(), out_term.size());
	}
	else if (fieldname == "InTerminator" && ! lua_isnil(state, 3))
	{
		std::string in_term = LuaStack<std::string>::get(state, 3);
		self->port->setInputEos(in_term.c_str(), in_term.size());
	}
	
	return 0;
}


// #############################
// # Generators for C/C++ Code #
// #############################

extern "C"
{	
	void luaGenerateDriver(lua_State* state, const char* port_name)
	{
		std::stringstream code;
		
		code << "return asynPortDriver.find('" << port_name;
		code << "')";
		
		luaL_dostring(state, code.str().c_str());
	}
	
	void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param)
	{		
		std::stringstream code;
		
		code << "return asynOctetClient.find('" << port_name;
		code << "', " << addr << ", " << param << ")";
		
		luaL_dostring(state, code.str().c_str());
	}
}



int luaopen_asyn (lua_State *L)
{
	static const luaL_Reg driver_meta[] = {
		{"__index", l_driverindex},
		{"__newindex", l_drivernewindex},
		{NULL, NULL}
	};	
	
	LuaClass<lua_asynPortDriver> lua_apd(L, "asynPortDriver");
	lua_apd.ctor<void, std::string>("find", &lua_asynPortDriver::find, &lua_asynPortDriver::destroy);
	lua_apd.fun("readParam", lapd_readParam);
	lua_apd.fun("writeParam", lapd_writeParam);
	lua_apd.fun("callParamCallbacks", lapd_callParamCallbacks);
	
	luaL_getmetatable(L, "asynPortDriver");
	luaL_setfuncs(L, driver_meta, 0);
	lua_pop(L, 1);
	

	 static const luaL_Reg client_meta[] = {
		{"__index", l_clientindex},
		{"__newindex", l_clientnewindex},
		{NULL, NULL}
	};	
	
	
	LuaClass<lua_asynOctetClient> lua_aoc(L, "asynOctetClient");
	lua_aoc.ctor<void, std::string, int, std::string>("find", &lua_asynOctetClient::find, &lua_asynOctetClient::destroy);
	lua_aoc.fun("read", laoc_read);
	lua_aoc.fun("write", laoc_write);
	lua_aoc.fun("writeread", laoc_writeread);
	lua_aoc.fun("trace", &lua_asynOctetClient::trace);
	lua_aoc.fun("traceio", &lua_asynOctetClient::traceio);
	lua_aoc.fun("setOption", &lua_asynOctetClient::option);
	
	luaL_getmetatable(L, "asynOctetClient");
	luaL_setfuncs(L, client_meta, 0);
	lua_pop(L, 1);
	
	
	
	LuaModule lua_asyn(L, "asyn");
	lua_asyn.fun("setTrace", l_setTrace);
	lua_asyn.fun("setTraceIO", l_setTraceIO);
	lua_asyn.fun("setOption", l_setOption);
	lua_asyn.fun("setDoubleParam", l_setDoubleParam);
	lua_asyn.fun("setIntegerParam", l_setIntegerParam);
	lua_asyn.fun("setStringParam", l_setStringParam);
	lua_asyn.fun("setParam", l_setParam);
	lua_asyn.fun("getDoubleParam", l_getDoubleParam);
	lua_asyn.fun("getIntegerParam", l_getIntegerParam);
	lua_asyn.fun("getStringParam", l_getStringParam);
	lua_asyn.fun("getParam", l_getParam);
	lua_asyn.fun("readParam", l_readParam);
	lua_asyn.fun("writeParam", l_writeParam);
	lua_asyn.fun("callParamCallbacks", l_callParamCallbacks);
	lua_asyn.fun("read", l_read);
	lua_asyn.fun("write", l_write);
	lua_asyn.fun("writeread", l_writeread);
	lua_asyn.fun("setInTerminator", l_setInTerminator);
	lua_asyn.fun("setOutTerminator", l_setOutTerminator);
	lua_asyn.fun("setWriteTimeout", l_setWriteTimeout);
	lua_asyn.fun("setReadTimeout", l_setReadTimeout);
	lua_asyn.fun("setWriteReadTimeout", l_setWriteReadTimeout);
	lua_asyn.fun("getInTerminator", l_getInTerminator);
	lua_asyn.fun("getOutTerminator", l_getOutTerminator);
	lua_asyn.fun("getWriteTimeout", l_getWriteTimeout);
	lua_asyn.fun("getReadTimeout", l_getReadTimeout);
	lua_asyn.fun("getWriteReadTimeout", l_getWriteReadTimeout);

	lua_getglobal(L, "asyn");
	return 1;
}


static void libasynRegister(void)
{ 
	luaRegisterLibrary("asyn", luaopen_asyn);
}

extern "C"
{
	epicsExportRegistrar(libasynRegister);
}
