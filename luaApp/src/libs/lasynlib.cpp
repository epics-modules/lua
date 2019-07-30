#include <asynPortDriver.h>
#include <asynPortClient.h>
#include "stdio.h"
#include <cstring>
#include <string>
#include <epicsExport.h>
#include "luaEpics.h"

#if defined(WIN32) || defined(_WIN32)
#include <string.h>
#define strcasecmp(x,y) strcmpi(x,y)
#else
#include <strings.h>
#endif

extern "C" {
	void luaGenerateDriver(lua_State* state, const char* port_name);
	void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param);
}

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

static int asyn_write(lua_State* state, asynOctetClient* port, const char* data)
{
	try
	{
		size_t numwrite;
		port->write(data, strlen(data), &numwrite);
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception while writing\n"); }
	
	return 0;
}

static int asyn_writeread(lua_State* state, asynOctetClient* client, const char* data)
{
	try
	{
		std::string output;
		size_t numwrite, numread;
		char buffer[256];
		int eomReason;

		client->writeRead(data, strlen(data), buffer, sizeof(buffer), &numwrite, &numread, &eomReason);
		output += std::string(buffer, numread);
		
		do
		{
			client->read(buffer, sizeof(buffer), &numread, &eomReason);
			output += std::string(buffer, numread);
		} while (eomReason & ASYN_EOM_CNT);
		
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
	
	asynStandardInterfaces* interfaces = port->getAsynStdInterfaces();
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data;
			asynInt32* inter = (asynInt32*) interfaces->int32.pinterface;
			inter->read(port, pasynuser, &data);
			lua_pushinteger(state, data);
			return 1;
		}
		
		case asynParamFloat64:
		{
			epicsFloat64 data;
			asynFloat64* inter = (asynFloat64*) interfaces->float64.pinterface;
			inter->read(port, pasynuser, &data);
			lua_pushnumber(state, data);
			return 1;
		}
		
		case asynParamOctet:
		{
			char buffer[256] = { '\0' };
			
			size_t num_trans;
			int eomreason;
			
			asynOctet* inter = (asynOctet*) interfaces->octet.pinterface;
			inter->read(port, pasynuser, buffer, 256, &num_trans, &eomreason);
			lua_pushstring(state, buffer);
			return 1;
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

static void asyn_settrace(lua_State* state, const char* portname, int addr, const char* key, bool val)
{
	int mask = 0;
	
	if      (strcasecmp(key, "error") == 0)     { mask = 0x0001; }
	else if (strcasecmp(key, "device") == 0)    { mask = 0x0002; }
	else if (strcasecmp(key, "filter") == 0)    { mask = 0x0004; }
	else if (strcasecmp(key, "driver") == 0)    { mask = 0x0008; }
	else if (strcasecmp(key, "flow") == 0)      { mask = 0x0010; }
	else if (strcasecmp(key, "warning") == 0)   { mask = 0x0020; }
		
	asynUser *pasynUser=NULL;
    asynStatus status;

	if (portname && (strlen(portname) > 0))
	{
		pasynUser = pasynManager->createAsynUser(0,0);
		status = pasynManager->connectDevice(pasynUser,portname,addr);
		
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

static void asyn_settraceio(lua_State* state, const char* portname, int addr, const char* key, bool val)
{
	int mask = 0;
	
	if      (strcasecmp(key, "nodata") == 0)   { mask = 0x0001; }
	else if (strcasecmp(key, "ascii") == 0)    { mask = 0x0002; }
	else if (strcasecmp(key, "escape") == 0)   { mask = 0x0004; }
	else if (strcasecmp(key, "hex") == 0)      { mask = 0x0008; }
		
	asynUser *pasynUser=NULL;
    asynStatus status;

	if (portname && (strlen(portname) > 0))
	{
		pasynUser = pasynManager->createAsynUser(0,0);
		status = pasynManager->connectDevice(pasynUser,portname,addr);
		
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
	
	const char* portname = luaL_checkstring(state, 1);
	
	int addr = 0;
	
	if (lua_isnumber(state, 2))     { addr = luaL_checkinteger(state, 2); }
	
	if (lua_type(state, num_ops) != LUA_TTABLE)
	{
		const char* key = luaL_checkstring(state, num_ops - 1);
		bool val = lua_toboolean(state, num_ops);
		
		asyn_settrace(state, portname, addr, key, val);
	}
	else
	{
		lua_pushnil(state);
		
		while(lua_next(state, num_ops))
		{
			lua_pushvalue(state, -2);
			const char* key = lua_tostring(state, -1);
			bool val = lua_toboolean(state, -2);
			asyn_settrace(state, portname, addr, key, val);
			
			lua_pop(state, 2);
		}
	}
	
	return 0;
}

static int l_setTraceIO(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);
	
	const char* portname = luaL_checkstring(state, 1);
	
	int addr = 0;
	
	if (lua_isnumber(state, 2))     { addr = luaL_checkinteger(state, 2); }
	
	if (lua_type(state, num_ops) != LUA_TTABLE)
	{
		const char* key = luaL_checkstring(state, num_ops - 1);
		bool val = lua_toboolean(state, num_ops);
		
		asyn_settraceio(state, portname, addr, key, val);
	}
	else
	{
		lua_pushnil(state);
		
		while(lua_next(state, num_ops))
		{
			lua_pushvalue(state, -2);
			const char* key = lua_tostring(state, -1);
			bool val = lua_toboolean(state, -2);
			asyn_settraceio(state, portname, addr, key, val);
			
			lua_pop(state, 2);
		}
	}
	
	return 0;
}


static int l_portread(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "ReadTimeout");
	if (! lua_isnil(state, -1))    { port->setTimeout(lua_tonumber(state, -1)); }
	lua_pop(state, 1);
	
	return asyn_read(state, port);
}

static int l_portwrite(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	const char* output = luaL_checkstring(state, 2);

	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "WriteTimeout");
	if (! lua_isnil(state, -1))    { port->setTimeout(lua_tonumber(state, -1)); }
	lua_pop(state, 1);
	
	return asyn_write(state, port, output);
}

static int l_portwriteread(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	const char* output = luaL_checkstring(state, 2);

	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "WriteReadTimeout");
	if (! lua_isnil(state, -1))    { port->setTimeout(lua_tonumber(state, -1)); }
	lua_pop(state, 1);
	
	return asyn_writeread(state, port, output);
}

static int l_porttrace(lua_State* state)
{
	lua_settop(state, 3);
	
	lua_getfield(state, 1, "name");
	const char* portname = lua_tostring(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "addr");
	int addr = lua_tointeger(state, -1);
	lua_pop(state, 1);
	
	luaL_checktype(state, 1, LUA_TTABLE);
	
	if (lua_type(state, 2) != LUA_TTABLE)
	{
		bool setval = true;
		if (! lua_isnil(state, 3))    { setval = lua_toboolean(state, 3); }
		
		asyn_settrace(state, portname, addr, luaL_checkstring(state, 2), setval);
	}
	else
	{
		lua_pushnil(state);
		
		while(lua_next(state, 2))
		{
			lua_pushvalue(state, -2);
			const char* key = lua_tostring(state, -1);
			bool val = lua_toboolean(state, -2);
			asyn_settrace(state, portname, addr, key, val);
			
			lua_pop(state, 2);
		}
	}
	
	return 0;
}

static int l_porttraceio(lua_State* state)
{
	lua_settop(state, 3);
	
	lua_getfield(state, 1, "name");
	const char* portname = lua_tostring(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "addr");
	int addr = lua_tointeger(state, -1);
	lua_pop(state, 1);
	
	luaL_checktype(state, 1, LUA_TTABLE);
	
	if (lua_type(state, 2) != LUA_TTABLE)
	{	
		bool setval = true;
		if (! lua_isnil(state, 3))    { setval = lua_toboolean(state, 3); }
		
		asyn_settraceio(state, portname, addr, luaL_checkstring(state, 2), setval);
	}
	else
	{
		lua_pushnil(state);
		
		while(lua_next(state, 2))
		{
			lua_pushvalue(state, -2);
			const char* key = lua_tostring(state, -1);
			bool val = lua_toboolean(state, -2);
			asyn_settraceio(state, portname, addr, key, val);
			
			lua_pop(state, 2);
		}
	}
	
	return 0;
}

static int l_portindex(lua_State* state)
{
	std::string fieldname = std::string(lua_tostring(state, 2));
	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	if (fieldname == "OutTerminator")
	{
		char out_term[20] = {'\0'};
		int actual;
		
		port->getOutputEos(out_term, 20, &actual);
	
		lua_pushstring(state, out_term);
		return 1;
	}
	else if (fieldname == "InTerminator")
	{
		char in_term[20] = {'\0'};
		int actual;
		
		port->getInputEos(in_term, 20, &actual);
	
		lua_pushstring(state, in_term);
		return 1;
	}
	
	return 0;
}

static int l_portnewindex(lua_State* state)
{
	std::string fieldname = std::string(lua_tostring(state, 2));
	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	if (fieldname == "OutTerminator" && ! lua_isnil(state, 3))
	{
		const char* out_term = lua_tostring(state, 3);
		lua_len(state, 3);
		int len = lua_tointeger(state, -1);
		lua_pop(state, 1);
		
		port->setOutputEos(out_term, len);
	}
	else if (fieldname == "InTerminator" && ! lua_isnil(state, 3))
	{
		const char* in_term = lua_tostring(state, 3);
		lua_len(state, 3);
		int len = lua_tointeger(state, -1);
		lua_pop(state, 1);
		
		port->setInputEos(in_term, len);
	}
	
	return 0;
}

static int l_portgc(lua_State* state)
{
	lua_getfield(state, 1, "client");
	delete (asynOctetClient*) lua_touserdata(state, -1);
	return 0;
}

static int l_drivercallbacks(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);
	
	lua_getfield(state, 1, "driver");
	asynPortDriver* port = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	port->callParamCallbacks();
	return 0;
}

static int l_driverindex(lua_State* state)
{
	lua_getfield(state, 1, "driver");
	asynPortDriver* port = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	if (lua_isstring(state, 2))
	{
		std::string fieldname = std::string(lua_tostring(state, 2));
		
		if      (fieldname == "portName")    { lua_pushstring(state, port->portName); }
		else if (fieldname == "maxAddr")     { lua_pushinteger(state, port->maxAddr); }
		else
		{
			lua_getfield(state, 1, "addr");
			int addr = lua_tointeger(state, -1);
			lua_pop(state, 1);
			
			return asyn_getparam(state, port, addr, fieldname.c_str());
		}
	}
	else if (lua_isinteger(state, 2))
	{
		if (lua_tointeger(state, 2) < port->maxAddr)
		{
			luaGenerateDriver(state, port->portName);
			lua_pushvalue(state, 2);
			lua_setfield(state, -2, "addr");
		}
	}
	
	return 1;
}

static int l_driverread(lua_State* state)
{
	lua_getfield(state, 1, "driver");
	asynPortDriver* port = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	std::string fieldname = std::string(luaL_checkstring(state, 2));
		
	lua_getfield(state, 1, "addr");
	int addr = lua_tointeger(state, -1);
	lua_pop(state, 1);
			
	return asyn_readparam(state, port, addr, fieldname.c_str());
}

static int l_drivernewindex(lua_State* state)
{
	std::string fieldname = std::string(lua_tostring(state, 2));
	lua_getfield(state, 1, "driver");
	asynPortDriver* port = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	if      (fieldname == "portName")  { return 0; }
	else if (fieldname == "maxAddr")   { return 0; }
	else
	{
		lua_getfield(state, 1, "addr");
		int addr = lua_tointeger(state, -1);
		lua_pop(state, 1);
		
		return asyn_setparam(state, port, addr, fieldname.c_str(), 3);
	}
}

static int l_driverwrite(lua_State* state)
{
	std::string fieldname = std::string(luaL_checkstring(state, 2));
	
	lua_getfield(state, 1, "driver");
	asynPortDriver* port = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "addr");
	int addr = lua_tointeger(state, -1);
	lua_pop(state, 1);
		
	return asyn_writeparam(state, port, addr, fieldname.c_str(), 3);
}

extern "C"
{
	void luaGenerateDriver(lua_State* state, const char* port_name)
	{
		asynPortDriver* port = find_driver(state, port_name);
		
		static const luaL_Reg driver_meta[] = {
			{"__index", l_driverindex},
			{"__newindex", l_drivernewindex},
			{NULL, NULL}
		};
		
		static const luaL_Reg driver_funcs[] = {
			{"readParam", l_driverread},
			{"writeParam", l_driverwrite},
			{"callParamCallbacks", l_drivercallbacks},
			{NULL, NULL}
		};
		
		luaL_newmetatable(state, "driver_meta");
		luaL_setfuncs(state, driver_meta, 0);
		lua_pop(state, 1);
		
		lua_newtable(state);
		luaL_setfuncs(state, driver_funcs, 0);
		
		lua_pushlightuserdata(state, port);
		lua_setfield(state, -2, "driver");
		
		lua_pushinteger(state, 0);
		lua_setfield(state, -2, "addr");
		
		luaL_setmetatable(state, "driver_meta");
	}
	
	void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param)
	{
		static const luaL_Reg port_meta[] = {
			{"__gc", l_portgc},
			{"__index", l_portindex},
			{"__newindex", l_portnewindex},
			{NULL, NULL}
		};
		
		static const luaL_Reg port_funcs[] = {
			{"read", l_portread},
			{"write", l_portwrite},
			{"writeread", l_portwriteread},
			{"trace", l_porttrace},
			{"traceio", l_porttraceio},
			{NULL, NULL}
		};

		luaL_newmetatable(state, "port_meta");
		luaL_setfuncs(state, port_meta, 0);
		lua_pop(state, 1);

		lua_newtable(state);
		luaL_setfuncs(state, port_funcs, 0);

		lua_pushstring(state, port_name);
		lua_setfield(state, -2, "name");

		lua_pushstring(state, param);
		lua_setfield(state, -2, "param");

		lua_pushnumber(state, addr);
		lua_setfield(state, -2, "addr");
		
		asynOctetClient* port = new asynOctetClient(port_name, addr, param);
		port->flush();
		
		lua_pushlightuserdata(state, port);
		lua_setfield(state, -2, "client");

		luaL_setmetatable(state, "port_meta");
		
		lua_getglobal(state, "OutTerminator");
		lua_setfield(state, -2, "OutTerminator");

		lua_getglobal(state, "InTerminator");
		lua_setfield(state, -2, "InTerminator");
		
		lua_getglobal(state, "WriteTimeout");
		lua_setfield(state, -2, "WriteTimeout");
		
		lua_getglobal(state, "ReadTimeout");
		lua_setfield(state, -2, "ReadTimeout");
		
		lua_getglobal(state, "WriteReadTimeout");
		lua_setfield(state, -2, "WriteReadTimeout");
	}
}


static int l_createport(lua_State* state)
{
	lua_settop(state, 3);

	const char* port = luaL_checkstring(state, 1);
	int addr = (int) lua_tonumber(state, 2);
	const char* param = lua_tostring(state, 3);

	luaGeneratePort(state, port, addr, param);

	return 1;
}

static int l_createdriver(lua_State* state)
{
	lua_settop(state, 1);
	luaGenerateDriver(state, luaL_checkstring(state, 1));

	return 1;
}

int luaopen_asyn (lua_State *L)
{
	static const luaL_Reg mylib[] = {
		{"read", l_read},
		{"write", l_write},
		{"writeread", l_writeread},
		{"setOutTerminator", l_setOutTerminator},
		{"getOutTerminator", l_getOutTerminator},
		{"setInTerminator", l_setInTerminator},
		{"getInTerminator", l_getInTerminator},
		{"setWriteTimeout", l_setWriteTimeout},
		{"getWriteTimeout", l_getWriteTimeout},
		{"setReadTimeout", l_setReadTimeout},
		{"getReadTimeout", l_getReadTimeout},
		{"setWriteReadTimeout", l_setWriteReadTimeout},
		{"getWriteReadTimeout", l_getWriteReadTimeout},
		{"setIntegerParam", l_setIntegerParam},
		{"setDoubleParam", l_setDoubleParam},
		{"setStringParam", l_setStringParam},
		{"setParam", l_setParam},
		{"writeParam", l_writeParam},
		{"getIntegerParam", l_getIntegerParam},
		{"getDoubleParam", l_getDoubleParam},
		{"getStringParam", l_getStringParam},
		{"getParam", l_getParam},
		{"readParam", l_readParam},
		{"callParamCallbacks", l_callParamCallbacks},
		{"setTrace", l_setTrace},
		{"setTraceIO", l_setTraceIO},
		{"client", l_createport},
		{"driver", l_createdriver},
		{NULL, NULL}  /* sentinel */
	};

	luaL_newlib(L, mylib);
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
