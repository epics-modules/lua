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


static asynPortDriver* find_driver(const char* port_name)
{
	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port_name);

	if (driver && std::string(driver->portName) == std::string(port_name))
	{
		return driver;
	}
	
	return NULL;
}

static int asyn_read(lua_State* state, asynOctetClient* port)
{
	char errbuf[256] = { '\0' };

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
		catch (std::runtime_error& e)
		{
			strncpy(errbuf, e.what(), sizeof(errbuf) - 1);
		}
		catch (...)
		{
			strncpy(errbuf, "Unexpected exception while reading", sizeof(errbuf) - 1);
		}
	}

	if (errbuf[0])    { return luaL_error(state, "%s", errbuf); }

	lua_pushnil(state);
	return 1;
}

static int asyn_write(lua_State* state, asynOctetClient* port, const char* data, size_t len)
{
	char errbuf[256] = { '\0' };

	try
	{
		size_t numwrite;
		port->write(data, len, &numwrite);
	}
	catch (std::runtime_error& e)
	{
		strncpy(errbuf, e.what(), sizeof(errbuf) - 1);
	}
	catch (...)
	{
		strncpy(errbuf, "Unexpected exception while writing", sizeof(errbuf) - 1);
	}

	if (errbuf[0])    { return luaL_error(state, "%s", errbuf); }

	return 0;
}

static int asyn_writeread(lua_State* state, asynOctetClient* client, const char* data, size_t len)
{
	char errbuf[256] = { '\0' };

	{
		try
		{
			std::string output;
			size_t numwrite, numread;
			char buffer[256];
			int eomReason;

			client->writeRead(data, len, buffer, sizeof(buffer), &numwrite, &numread, &eomReason);
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
		catch (std::runtime_error& e)
		{
			strncpy(errbuf, e.what(), sizeof(errbuf) - 1);
		}
		catch (...)
		{
			strncpy(errbuf, "Unexpected exception during writeread", sizeof(errbuf) - 1);
		}
	}

	if (errbuf[0])    { return luaL_error(state, "%s", errbuf); }

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
	
	return asyn_write(state, &output, data, strlen(data));
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

	return asyn_writeread(state, &client, data, strlen(data));
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
	
	/* Validate the Lua value before allocating asyn resources.
	 * luaL_check* may longjmp on type error -- must happen before
	 * createAsynUser so we don't leak the asynUser on failure. */
	switch (partype)
	{
		case asynParamInt32:   luaL_checkinteger(state, val_index); break;
		case asynParamFloat64: luaL_checknumber(state, val_index);  break;
		case asynParamOctet:   luaL_checkstring(state, val_index);  break;
		default: return 0;
	}
	
	asynUser* pasynuser = pasynManager->createAsynUser(NULL, NULL);
	pasynManager->connectDevice(pasynuser, port->portName, addr);
	
	pasynuser->reason = index;
	
	asynStandardInterfaces* interfaces = port->getAsynStdInterfaces();
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data = lua_tointeger(state, val_index);
			
			asynInt32* inter = (asynInt32*) interfaces->int32.pinterface;
			inter->write(port, pasynuser, data);
			
			break;
		}
		
		case asynParamFloat64:
		{
			epicsFloat64 data = lua_tonumber(state, val_index);
			
			asynFloat64* inter = (asynFloat64*) interfaces->float64.pinterface;
			inter->write(port, pasynuser, data);
			break;
		}
		
		case asynParamOctet:
		{
			const char* data = lua_tostring(state, val_index);
			
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
	
	int result = 0;
	
	switch (partype)
	{
		case asynParamInt32:
		{
			epicsInt32 data;
			asynInt32* inter = (asynInt32*) interfaces->int32.pinterface;
			
			if(asynSuccess == inter->read(port, pasynuser, &data))
			{
				lua_pushinteger(state, data);
				result = 1;
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
				result = 1;
			}
			
			break;
		}
		
		case asynParamOctet:
		{
			char buffer[1024] = { '\0' };
			
			size_t num_trans;
			int eomreason;
			
			asynOctet* inter = (asynOctet*) interfaces->octet.pinterface;
			if (asynSuccess == inter->read(port, pasynuser, buffer, sizeof(buffer), &num_trans, &eomreason))
			{
				lua_pushstring(state, buffer);
				result = 1;
			}
			
			break;
		}
		
		default:
			break;
	}
	
	pasynManager->disconnect(pasynuser);
	pasynManager->freeAsynUser(pasynuser);
	
	return result;
}

/*
 * Core helpers for parameter get/set by index.
 * These are the single source of truth for the Int32/Float64/Octet
 * type dispatch pattern used throughout the asyn library.
 */

/* Read a parameter value by index and push it onto the Lua stack. Returns 1 on success, 0 on failure. */
static int asyn_pushparam(lua_State* state, asynPortDriver* driver, int addr, int index)
{
	asynParamType ptype;
	driver->getParamType(addr, index, &ptype);

	switch (ptype)
	{
		case asynParamInt32:
		{
			epicsInt32 val;
			driver->getIntegerParam(addr, index, &val);
			lua_pushinteger(state, val);
			return 1;
		}
		case asynParamFloat64:
		{
			epicsFloat64 val;
			driver->getDoubleParam(addr, index, &val);
			lua_pushnumber(state, val);
			return 1;
		}
		case asynParamOctet:
		{
			std::string val;
			driver->getStringParam(addr, index, val);
			lua_pushstring(state, val.c_str());
			return 1;
		}
		default:
			return 0;
	}
}

/* Pull a value from the Lua stack and set it on a parameter by index. */
static void asyn_pullparam(lua_State* state, asynPortDriver* driver, int addr, int index, int val_index)
{
	asynParamType ptype;
	driver->getParamType(addr, index, &ptype);

	switch (ptype)
	{
		case asynParamInt32:
			driver->setIntegerParam(addr, index, (epicsInt32) luaL_checkinteger(state, val_index));
			break;
		case asynParamFloat64:
			driver->setDoubleParam(addr, index, luaL_checknumber(state, val_index));
			break;
		case asynParamOctet:
			driver->setStringParam(addr, index, luaL_checkstring(state, val_index));
			break;
		default:
			break;
	}
}

static int asyn_setparam(lua_State* state, asynPortDriver* port, int addr, const char* param, int val_index)
{
	int index;
	if (port->findParam(addr, param, &index) == asynParamNotFound)    { return 0; }
	asyn_pullparam(state, port, addr, index, val_index);
	return 0;
}

static int asyn_getparam(lua_State* state, asynPortDriver* port, int addr, const char* param)
{
	int index;
	if (port->findParam(addr, param, &index) == asynParamNotFound)    { return 0; }
	return asyn_pushparam(state, port, addr, index);
	
	return 0;
}

/* Common helpers for the asyn.setParam/writeParam/getParam/readParam functions.
 * Each pair (set/write, get/read) shares identical argument parsing and differs
 * only in the backend function called. */

typedef int (*ParamSetFunc)(lua_State*, asynPortDriver*, int, const char*, int);
typedef int (*ParamGetFunc)(lua_State*, asynPortDriver*, int, const char*);

static int l_paramset_common(lua_State* state, ParamSetFunc backend)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);

	const char* port = luaL_checkstring(state, 1);

	int addr = 0;
	if (num_ops == 4)    { addr = luaL_checkinteger(state, 2); }

	const char* param = luaL_checkstring(state, num_ops - 1);

	asynPortDriver* driver = find_driver(port);
	if (!driver)    { return luaL_error(state, "No asynPortDriver found with name: %s\n", port); }
	return backend(state, driver, addr, param, num_ops);
}

static int l_paramget_common(lua_State* state, ParamGetFunc backend)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 3);

	const char* port = luaL_checkstring(state, 1);

	int addr = 0;
	if (num_ops == 3)    { addr = luaL_checkinteger(state, 2); }

	const char* param = luaL_checkstring(state, num_ops);

	asynPortDriver* driver = find_driver(port);
	if (!driver)    { return luaL_error(state, "No asynPortDriver found with name: %s\n", port); }
	return backend(state, driver, addr, param);
}

static int l_setParam(lua_State* state)    { return l_paramset_common(state, asyn_setparam); }
static int l_writeParam(lua_State* state)  { return l_paramset_common(state, asyn_writeparam); }
static int l_getParam(lua_State* state)    { return l_paramget_common(state, asyn_getparam); }
static int l_readParam(lua_State* state)   { return l_paramget_common(state, asyn_readparam); }

static int l_setIntegerParam(lua_State* state)
{
	luaL_checkinteger(state, lua_gettop(state));
	return l_setParam(state);
}

static int l_getIntegerParam(lua_State* state)
{
	int ret = l_getParam(state);
	if (ret > 0)    { luaL_checkinteger(state, -1); }
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
	if (ret > 0)    { luaL_checknumber(state, -1); }
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
	if (ret > 0)    { luaL_checktype(state, -1, LUA_TSTRING); }
	return ret;
}

static int l_callParamCallbacks(lua_State* state)
{
	lua_settop(state, 2);

	const char* port = luaL_checkstring(state, 1);
	int addr = lua_tointeger(state, 2);

	asynPortDriver* driver = find_driver(port);
	if (!driver)    { return luaL_error(state, "No asynPortDriver found with name: %s\n", port); }
	driver->callParamCallbacks(addr);
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
	
	std::string portname(luaL_checkstring(state, 1));
	
	int addr = 0;
	if (lua_isnumber(state, 2))    { addr = (int) lua_tointeger(state, 2); }
	
	if (lua_type(state, num_ops) != LUA_TTABLE)
	{
		std::string key(luaL_checkstring(state, num_ops - 1));
		bool val = lua_toboolean(state, num_ops);
		asyn_settrace(state, portname, addr, key, val);
	}
	else
	{
		lua_pushnil(state);
		while (lua_next(state, num_ops))
		{
			if (lua_type(state, -2) == LUA_TSTRING)
			{
				std::string key(lua_tostring(state, -2));
				bool val = lua_toboolean(state, -1);
				asyn_settrace(state, portname, addr, key, val);
			}
			lua_pop(state, 1);
		}
	}
	
	return 0;
}

static int l_setTraceIO(lua_State* state)
{
	int num_ops = lua_gettop(state);
	lua_settop(state, 4);
	
	std::string portname(luaL_checkstring(state, 1));
	
	int addr = 0;
	if (lua_isnumber(state, 2))    { addr = (int) lua_tointeger(state, 2); }
	
	if (lua_type(state, num_ops) != LUA_TTABLE)
	{
		std::string key(luaL_checkstring(state, num_ops - 1));
		bool val = lua_toboolean(state, num_ops);
		asyn_settraceio(state, portname, addr, key, val);
	}
	else
	{
		lua_pushnil(state);
		while (lua_next(state, num_ops))
		{
			if (lua_type(state, -2) == LUA_TSTRING)
			{
				std::string key(lua_tostring(state, -2));
				bool val = lua_toboolean(state, -1);
				asyn_settraceio(state, portname, addr, key, val);
			}
			lua_pop(state, 1);
		}
	}
	
	return 0;
}


// ################################
// # lua asynPortDriver Functions #
// ################################


// #################################
// # asyn.client API                #
// #################################

/*
 * Client userdata: owns an asynOctetClient* and stores port info.
 * Has __gc to clean up the asynOctetClient on collection.
 */
typedef struct {
	asynOctetClient* client;
	char portName[64];
	int addr;
} ClientUD;

static ClientUD* check_clientud(lua_State* state, int idx)
{
	ClientUD* ud = (ClientUD*) luaL_checkudata(state, idx, "lua_asynclient");
	if (!ud || !ud->client)    { luaL_error(state, "Invalid asyn client object"); }
	return ud;
}

static int l_client_gc(lua_State* state)
{
	ClientUD* ud = (ClientUD*) lua_touserdata(state, 1);
	if (ud && ud->client)
	{
		delete ud->client;
		ud->client = NULL;
	}
	return 0;
}

/* Forward declaration */
static void push_clientproxy(lua_State* state, const char* portName, int addr, const char* param);

/* Client methods */

static int l_client_read(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);
	return asyn_read(state, ud->client);
}

static int l_client_write(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);
	const char* data = luaL_checkstring(state, 2);
	return asyn_write(state, ud->client, data, strlen(data));
}

static int l_client_writeread(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);
	const char* data = luaL_checkstring(state, 2);
	return asyn_writeread(state, ud->client, data, strlen(data));
}

static int l_client_flush(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);
	ud->client->flush();
	return 0;
}

static int l_client_trace(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);

	if (lua_type(state, 2) == LUA_TTABLE)
	{
		lua_pushnil(state);
		while (lua_next(state, 2))
		{
			if (lua_type(state, -2) == LUA_TSTRING)
			{
				const char* key = lua_tostring(state, -2);
				bool val = lua_toboolean(state, -1);
				asyn_settrace(state, std::string(ud->portName), ud->addr, std::string(key), val);
			}
			lua_pop(state, 1);
		}
	}
	else
	{
		const char* key = luaL_checkstring(state, 2);
		bool val = true;
		if (!lua_isnil(state, 3))    { val = lua_toboolean(state, 3); }
		asyn_settrace(state, std::string(ud->portName), ud->addr, std::string(key), val);
	}

	return 0;
}

static int l_client_traceio(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);

	if (lua_type(state, 2) == LUA_TTABLE)
	{
		lua_pushnil(state);
		while (lua_next(state, 2))
		{
			if (lua_type(state, -2) == LUA_TSTRING)
			{
				const char* key = lua_tostring(state, -2);
				bool val = lua_toboolean(state, -1);
				asyn_settraceio(state, std::string(ud->portName), ud->addr, std::string(key), val);
			}
			lua_pop(state, 1);
		}
	}
	else
	{
		const char* key = luaL_checkstring(state, 2);
		bool val = true;
		if (!lua_isnil(state, 3))    { val = lua_toboolean(state, 3); }
		asyn_settraceio(state, std::string(ud->portName), ud->addr, std::string(key), val);
	}

	return 0;
}

static int l_client_setoption(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);
	const char* key = luaL_checkstring(state, 2);
	const char* val = luaL_checkstring(state, 3);
	asynSetOption(ud->portName, ud->addr, key, val);
	return 0;
}

/* __index on client proxy */
static int l_client_index(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);

	if (lua_isinteger(state, 2))
	{
		/* Integer index: create new client with same port, different address */
		int newAddr = lua_tointeger(state, 2);
		push_clientproxy(state, ud->portName, newAddr, "");
		return 1;
	}

	const char* key = luaL_checkstring(state, 2);

	/* Methods */
	if (strcmp(key, "read") == 0)       { lua_pushcfunction(state, l_client_read); return 1; }
	if (strcmp(key, "write") == 0)      { lua_pushcfunction(state, l_client_write); return 1; }
	if (strcmp(key, "writeread") == 0)  { lua_pushcfunction(state, l_client_writeread); return 1; }
	if (strcmp(key, "flush") == 0)      { lua_pushcfunction(state, l_client_flush); return 1; }
	if (strcmp(key, "trace") == 0)      { lua_pushcfunction(state, l_client_trace); return 1; }
	if (strcmp(key, "traceio") == 0)    { lua_pushcfunction(state, l_client_traceio); return 1; }
	if (strcmp(key, "setOption") == 0)  { lua_pushcfunction(state, l_client_setoption); return 1; }

	/* Read-only properties */
	if (strcmp(key, "portName") == 0)   { lua_pushstring(state, ud->portName); return 1; }
	if (strcmp(key, "addr") == 0)       { lua_pushinteger(state, ud->addr); return 1; }

	/* Terminators */
	if (strcmp(key, "InTerminator") == 0)
	{
		char eos[20] = {'\0'};
		int actual;
		ud->client->getInputEos(eos, sizeof(eos), &actual);
		lua_pushstring(state, eos);
		return 1;
	}
	if (strcmp(key, "OutTerminator") == 0)
	{
		char eos[20] = {'\0'};
		int actual;
		ud->client->getOutputEos(eos, sizeof(eos), &actual);
		lua_pushstring(state, eos);
		return 1;
	}

	return 0;
}

/* __newindex on client proxy */
static int l_client_newindex(lua_State* state)
{
	ClientUD* ud = check_clientud(state, 1);
	const char* key = luaL_checkstring(state, 2);

	if (strcmp(key, "InTerminator") == 0 && !lua_isnil(state, 3))
	{
		const char* eos = lua_tostring(state, 3);
		ud->client->setInputEos(eos, strlen(eos));
	}
	else if (strcmp(key, "OutTerminator") == 0 && !lua_isnil(state, 3))
	{
		const char* eos = lua_tostring(state, 3);
		ud->client->setOutputEos(eos, strlen(eos));
	}

	return 0;
}

/* Creates a client proxy userdata and pushes it onto the stack */
static void push_clientproxy(lua_State* state, const char* portName, int addr, const char* param)
{
	ClientUD* ud = (ClientUD*) lua_newuserdata(state, sizeof(ClientUD));
	ud->client = NULL;
	strncpy(ud->portName, portName, sizeof(ud->portName) - 1);
	ud->portName[sizeof(ud->portName) - 1] = '\0';
	ud->addr = addr;

	{
		char errbuf[256] = { '\0' };

		try
		{
			ud->client = new asynOctetClient(portName, addr, param ? param : "");
			ud->client->flush();
		}
		catch (std::exception& e)
		{
			ud->client = NULL;
			strncpy(errbuf, e.what(), sizeof(errbuf) - 1);
		}

		if (errbuf[0])
		{
			luaL_error(state, "Failed to create asyn client for port '%s': %s", portName, errbuf);
		}
	}

	if (luaL_newmetatable(state, "lua_asynclient"))
	{
		lua_pushcfunction(state, l_client_index);
		lua_setfield(state, -2, "__index");
		lua_pushcfunction(state, l_client_newindex);
		lua_setfield(state, -2, "__newindex");
		lua_pushcfunction(state, l_client_gc);
		lua_setfield(state, -2, "__gc");
		lua_pushstring(state, "asynOctetClient");
		lua_setfield(state, -2, "__name");

		lua_newtable(state);
		lua_pushstring(state, ".portName                   -- port name (property)"); lua_rawseti(state, -2, 1);
		lua_pushstring(state, ".addr                       -- address (property)"); lua_rawseti(state, -2, 2);
		lua_pushstring(state, ".InTerminator               -- get/set input terminator"); lua_rawseti(state, -2, 3);
		lua_pushstring(state, ".OutTerminator              -- get/set output terminator"); lua_rawseti(state, -2, 4);
		lua_pushstring(state, ":read()"); lua_rawseti(state, -2, 5);
		lua_pushstring(state, ":write(data)"); lua_rawseti(state, -2, 6);
		lua_pushstring(state, ":writeread(data)"); lua_rawseti(state, -2, 7);
		lua_pushstring(state, ":flush()"); lua_rawseti(state, -2, 8);
		lua_pushstring(state, ":trace(mask) -- error=0x1, device=0x2, filter=0x4, driver=0x8, flow=0x10, warning=0x20"); lua_rawseti(state, -2, 9);
		lua_pushstring(state, ":traceio(mask) -- nodata=0x0, ascii=0x1, escape=0x2, hex=0x4"); lua_rawseti(state, -2, 10);
		lua_pushstring(state, ":setOption(key, val)"); lua_rawseti(state, -2, 11);
		lua_pushstring(state, "[addr]                      -- index by address"); lua_rawseti(state, -2, 12);
		lua_setfield(state, -2, "_doc");
	}
	lua_setmetatable(state, -2);
}

/*
 * asyn.client.find(portName [, addr [, param]])
 * asyn.client(portName [, addr [, param]])
 */
static int l_client_find(lua_State* state)
{
	const char* portName = luaL_checkstring(state, 1);
	int addr = luaL_optinteger(state, 2, 0);
	const char* param = luaL_optstring(state, 3, "");

	push_clientproxy(state, portName, addr, param);
	return 1;
}

/* asyn.client(...) shortcut via __call */
static int l_client_call(lua_State* state)
{
	lua_remove(state, 1);  /* remove the client table itself */
	return l_client_find(state);
}


// #############################
// # Generators for C/C++ Code #
// #############################


// ###################################
// # asyn.driver.new API             #
// ###################################

#include <epicsMutex.h>
#include <epicsGuard.h>

/*
 * luaAsynPortDriver: a self-contained asynPortDriver subclass for the
 * asyn.driver.new() API. Read/write callbacks are stored as Lua function
 * references in the registry, keyed by parameter index.
 *
 * This is independent of the luaPortDriver class used by the old
 * param DSL / luaPortDriver() iocsh command.
 */
class luaAsynPortDriver : public asynPortDriver
{
public:
	lua_State* state;
	epicsMutex stateMutex;
	int funcTableRef;
	int selfRef;

	luaAsynPortDriver(const char* portName, lua_State* luaState)
		: asynPortDriver(portName, 1,
			asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask,
			asynInt32Mask | asynFloat64Mask | asynOctetMask,
			0, 1, 0, 0),
		  state(luaState),
		  funcTableRef(LUA_NOREF),
		  selfRef(LUA_NOREF)
	{
		lua_newtable(state);
		funcTableRef = luaL_ref(state, LUA_REGISTRYINDEX);
	}

	~luaAsynPortDriver() {}

private:
	void getFunction(int index, const char* direction)
	{
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, this->funcTableRef);
		lua_geti(this->state, -1, index);
		lua_remove(this->state, -2);
		if (!lua_isnil(this->state, -1))
		{
			lua_getfield(this->state, -1, direction);
			lua_remove(this->state, -2);
		}
	}

	int callRead()
	{
		/* Push self (driver proxy) */
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, this->selfRef);
		int status = lua_pcall(this->state, 1, 1, 0);
		if (status)
		{
			printf("%s\n", lua_tostring(this->state, -1));
			lua_pop(this->state, 1);
		}
		return status;
	}

	int callWrite()
	{
		/* value is already on stack, push self */
		lua_rawgeti(this->state, LUA_REGISTRYINDEX, this->selfRef);
		int status = lua_pcall(this->state, 2, 0, 0);
		if (status)
		{
			printf("%s\n", lua_tostring(this->state, -1));
			lua_pop(this->state, 1);
		}
		return status;
	}

public:
	asynStatus readInt32(asynUser* pasynUser, epicsInt32* value)
	{
		epicsGuard<epicsMutex> guard(this->stateMutex);
		this->getFunction(pasynUser->reason, "read");
		if (lua_isnil(this->state, -1))
		{
			lua_pop(this->state, 1);
			return asynPortDriver::readInt32(pasynUser, value);
		}
		if (this->callRead()) return asynError;
		if (!lua_isnil(this->state, -1))
		{
			*value = lua_tointeger(this->state, -1);
		}
		lua_pop(this->state, 1);
		return asynSuccess;
	}

	asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value)
	{
		epicsGuard<epicsMutex> guard(this->stateMutex);
		this->getFunction(pasynUser->reason, "write");
		if (lua_isnil(this->state, -1))
		{
			lua_pop(this->state, 1);
			return asynPortDriver::writeInt32(pasynUser, value);
		}
		lua_pushinteger(this->state, value);
		if (this->callWrite()) return asynError;
		return asynSuccess;
	}

	asynStatus readFloat64(asynUser* pasynUser, epicsFloat64* value)
	{
		epicsGuard<epicsMutex> guard(this->stateMutex);
		this->getFunction(pasynUser->reason, "read");
		if (lua_isnil(this->state, -1))
		{
			lua_pop(this->state, 1);
			return asynPortDriver::readFloat64(pasynUser, value);
		}
		if (this->callRead()) return asynError;
		if (!lua_isnil(this->state, -1))
		{
			*value = lua_tonumber(this->state, -1);
		}
		lua_pop(this->state, 1);
		return asynSuccess;
	}

	asynStatus writeFloat64(asynUser* pasynUser, epicsFloat64 value)
	{
		epicsGuard<epicsMutex> guard(this->stateMutex);
		this->getFunction(pasynUser->reason, "write");
		if (lua_isnil(this->state, -1))
		{
			lua_pop(this->state, 1);
			return asynPortDriver::writeFloat64(pasynUser, value);
		}
		lua_pushnumber(this->state, value);
		if (this->callWrite()) return asynError;
		return asynSuccess;
	}

	asynStatus readOctet(asynUser* pasynUser, char* value, size_t maxChars, size_t* actual, int* eomReason)
	{
		epicsGuard<epicsMutex> guard(this->stateMutex);
		this->getFunction(pasynUser->reason, "read");
		if (lua_isnil(this->state, -1))
		{
			lua_pop(this->state, 1);
			return asynPortDriver::readOctet(pasynUser, value, maxChars, actual, eomReason);
		}
		if (this->callRead()) return asynError;
		if (!lua_isnil(this->state, -1))
		{
			const char* retval = lua_tolstring(this->state, -1, actual);
			if (retval)
			{
				strncpy(value, retval, maxChars);
			}
		}
		lua_pop(this->state, 1);
		return asynSuccess;
	}

	asynStatus writeOctet(asynUser* pasynUser, const char* value, size_t maxChars, size_t* actual)
	{
		epicsGuard<epicsMutex> guard(this->stateMutex);
		this->getFunction(pasynUser->reason, "write");
		if (lua_isnil(this->state, -1))
		{
			lua_pop(this->state, 1);
			return asynPortDriver::writeOctet(pasynUser, value, maxChars, actual);
		}
		lua_pushstring(this->state, value);
		if (this->callWrite()) return asynError;
		return asynSuccess;
	}
};

/*
 * Type constructors: asyn.Int32, asyn.Float64, asyn.Octet
 *
 * Each returns a param spec table {name=str, type=int} with a
 * __call metamethod for optional default values:
 *   Int32 "PARAM_NAME"         -> {name="PARAM_NAME", type=asynParamInt32}
 *   Int32 "PARAM_NAME" (42)    -> {name="PARAM_NAME", type=asynParamInt32, default=42}
 */

/* __call on param spec table: stores the default value */
static int l_paramspec_setdefault(lua_State* state)
{
	lua_pushvalue(state, 2);
	lua_setfield(state, 1, "default");
	lua_pushvalue(state, 1);
	return 1;
}

static void ensure_paramspec_meta(lua_State* state)
{
	if (luaL_newmetatable(state, "lua_paramspec"))
	{
		lua_pushcfunction(state, l_paramspec_setdefault);
		lua_setfield(state, -2, "__call");
	}
	lua_pop(state, 1);
}

/*
 * Generic type constructor. The asynParamType is stored as an upvalue
 * (closure), so a single function handles Int32, Float64, and Octet.
 *
 *   Int32 "PARAM_NAME"       -> {name="PARAM_NAME", type=asynParamInt32}
 *   Int32 "PARAM_NAME" (42)  -> {name="PARAM_NAME", type=asynParamInt32, default=42}
 */
static int l_type_constructor(lua_State* state)
{
	int paramType = lua_tointeger(state, lua_upvalueindex(1));
	const char* name = luaL_checkstring(state, 1);
	lua_newtable(state);
	lua_pushstring(state, name);
	lua_setfield(state, -2, "name");
	lua_pushinteger(state, paramType);
	lua_setfield(state, -2, "type");
	ensure_paramspec_meta(state);
	luaL_setmetatable(state, "lua_paramspec");
	return 1;
}


/*
 * Parameter proxy object
 *
 * Returned by drv.PARAM_NAME via the driver proxy __index.
 * Supports:
 *   proxy.read  = function(self) ... end    -- bind read callback
 *   proxy.write = function(value, self) ... end  -- bind write callback
 *   proxy.value                             -- read param value
 *   proxy.value = x                         -- write param value + callParamCallbacks
 *   proxy.name                              -- parameter name string
 */

/* __index on param proxy */
static int l_paramproxy_index(lua_State* state)
{
	const char* key = luaL_checkstring(state, 2);

	/* Get the driver pointer and param index from the proxy */
	lua_getfield(state, 1, "_driver");
	asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "_addr");
	int addr = lua_tointeger(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "_index");
	int paramIndex = lua_tointeger(state, -1);
	lua_pop(state, 1);

	if (strcmp(key, "value") == 0)
	{
		return asyn_pushparam(state, driver, addr, paramIndex);
	}
	else if (strcmp(key, "name") == 0)
	{
		lua_getfield(state, 1, "_name");
		return 1;
	}
	else if (strcmp(key, "read") == 0 || strcmp(key, "write") == 0)
	{
		/* Return the stored callback (if any) from the function registry */
		lua_getfield(state, 1, "_state");
		lua_State* drvState = (lua_State*) lua_touserdata(state, -1);
		lua_pop(state, 1);

		if (!drvState)    { lua_pushnil(state); return 1; }

		luaAsynPortDriver* luaDrv = (luaAsynPortDriver*) driver;

		lua_rawgeti(drvState, LUA_REGISTRYINDEX, luaDrv->funcTableRef);
		lua_geti(drvState, -1, paramIndex);
		if (!lua_isnil(drvState, -1))
		{
			lua_getfield(drvState, -1, key);
			lua_xmove(drvState, state, 1);
			lua_pop(drvState, 2);
		}
		else
		{
			lua_pop(drvState, 2);
			lua_pushnil(state);
		}
		return 1;
	}

	lua_pushnil(state);
	return 1;
}

/* __newindex on param proxy */
static int l_paramproxy_newindex(lua_State* state)
{
	const char* key = luaL_checkstring(state, 2);

	lua_getfield(state, 1, "_driver");
	asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "_addr");
	int addr = lua_tointeger(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "_index");
	int paramIndex = lua_tointeger(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "_state");
	lua_State* drvState = (lua_State*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	if (strcmp(key, "value") == 0)
	{
		asyn_pullparam(state, driver, addr, paramIndex, 3);
		driver->callParamCallbacks(addr);
		return 0;
	}
	else if (strcmp(key, "read") == 0 || strcmp(key, "write") == 0)
	{
		/* Only server-created drivers (asyn.driver.new) can bind callbacks */
		if (!drvState)
		{
			return luaL_error(state, "Cannot bind callbacks on a client driver (use asyn.driver.new to create a server driver)");
		}

		/* Store the callback in the driver's function table */
		luaL_checktype(state, 3, LUA_TFUNCTION);

		luaAsynPortDriver* luaDrv = (luaAsynPortDriver*) driver;

		/* Move the function to the driver's Lua state if they differ */
		lua_rawgeti(drvState, LUA_REGISTRYINDEX, luaDrv->funcTableRef);
		lua_geti(drvState, -1, paramIndex);

		if (lua_isnil(drvState, -1))
		{
			lua_pop(drvState, 1);
			lua_newtable(drvState);
		}

		/* Copy the function to the driver state */
		if (state == drvState)
		{
			lua_pushvalue(state, 3);
		}
		else
		{
			lua_pushvalue(state, 3);
			lua_xmove(state, drvState, 1);
		}

		lua_setfield(drvState, -2, key);
		lua_seti(drvState, -2, paramIndex);
		lua_pop(drvState, 1);
		return 0;
	}

	return 0;
}

/* Create a param proxy table for a given parameter */
static void push_paramproxy(lua_State* state, asynPortDriver* driver, lua_State* drvState,
                            int addr, int paramIndex, const char* paramName)
{
	static int meta_registered = 0;

	if (!meta_registered)
	{
		luaL_newmetatable(state, "lua_paramproxy");
		lua_pushcfunction(state, l_paramproxy_index);
		lua_setfield(state, -2, "__index");
		lua_pushcfunction(state, l_paramproxy_newindex);
		lua_setfield(state, -2, "__newindex");

		lua_newtable(state);
		lua_pushstring(state, ".value                      -- get/set parameter value"); lua_rawseti(state, -2, 1);
		lua_pushstring(state, ".name                       -- parameter name (property)"); lua_rawseti(state, -2, 2);
		lua_pushstring(state, ".read = func(self)          -- bind read callback"); lua_rawseti(state, -2, 3);
		lua_pushstring(state, ".write = func(self, value)  -- bind write callback"); lua_rawseti(state, -2, 4);
		lua_setfield(state, -2, "_doc");

		lua_pop(state, 1);
		meta_registered = 1;
	}

	lua_newtable(state);
	lua_pushlightuserdata(state, driver);
	lua_setfield(state, -2, "_driver");
	lua_pushlightuserdata(state, drvState);
	lua_setfield(state, -2, "_state");
	lua_pushinteger(state, addr);
	lua_setfield(state, -2, "_addr");
	lua_pushinteger(state, paramIndex);
	lua_setfield(state, -2, "_index");
	lua_pushstring(state, paramName);
	lua_setfield(state, -2, "_name");
	luaL_setmetatable(state, "lua_paramproxy");
}


/*
 * Driver proxy object
 *
 * Returned by asyn.driver.new(). Supports:
 *   drv.PARAM_NAME           -- returns param proxy (via __index)
 *   drv.internal_var         -- returns internal state value (via __index)
 *   drv.internal_var = val   -- stores internal state (via __newindex)
 */

/*
 * Lua-callable callParamCallbacks for the driver proxy.
 * Usage: drv:callParamCallbacks()
 */
static int l_driverproxy_callParamCallbacks(lua_State* state)
{
	lua_getfield(state, 1, "_driver");
	asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	if (driver)    { driver->callParamCallbacks(); }

	return 0;
}

/*
 * Lua-callable writeParam for the driver proxy.
 * Usage: drv:writeParam("PARAM_NAME", value)
 */
static int l_driverproxy_writeParam(lua_State* state)
{
	lua_getfield(state, 1, "_driver");
	asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	if (!driver)    { return 0; }

	const char* param = luaL_checkstring(state, 2);
	return asyn_setparam(state, driver, 0, param, 3);
}

/*
 * Lua-callable readParam for the driver proxy.
 * Usage: local val = drv:readParam("PARAM_NAME")
 */
static int l_driverproxy_readParam(lua_State* state)
{
	lua_getfield(state, 1, "_driver");
	asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	if (!driver)    { return 0; }

	const char* param = luaL_checkstring(state, 2);
	return asyn_getparam(state, driver, 0, param);
}

/* __index on driver proxy */
static int l_driverproxy_index(lua_State* state)
{
	const char* key = luaL_checkstring(state, 2);

	/* Check for built-in properties */
	if (strcmp(key, "portName") == 0)
	{
		lua_getfield(state, 1, "_driver");
		asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
		lua_pop(state, 1);
		if (driver)    { lua_pushstring(state, driver->portName); }
		else           { lua_pushnil(state); }
		return 1;
	}

	if (strcmp(key, "maxAddr") == 0)
	{
		lua_getfield(state, 1, "_driver");
		asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
		lua_pop(state, 1);
		if (driver)    { lua_pushinteger(state, driver->maxAddr); }
		else           { lua_pushnil(state); }
		return 1;
	}

	if (strcmp(key, "callParamCallbacks") == 0)
	{
		lua_pushcfunction(state, l_driverproxy_callParamCallbacks);
		return 1;
	}

	if (strcmp(key, "writeParam") == 0)
	{
		lua_pushcfunction(state, l_driverproxy_writeParam);
		return 1;
	}

	if (strcmp(key, "readParam") == 0)
	{
		lua_pushcfunction(state, l_driverproxy_readParam);
		return 1;
	}

	/* Check the param proxy cache */
	lua_getfield(state, 1, "_param_proxies");
	lua_getfield(state, -1, key);
	if (!lua_isnil(state, -1))
	{
		lua_remove(state, -2);
		return 1;
	}
	lua_pop(state, 2);

	/* Check internal state table */
	lua_getfield(state, 1, "_state_data");
	lua_getfield(state, -1, key);
	if (!lua_isnil(state, -1))
	{
		lua_remove(state, -2);
		return 1;
	}
	lua_pop(state, 2);

	/* Try on-demand param proxy creation: check if the key is a parameter name */
	lua_getfield(state, 1, "_driver");
	asynPortDriver* driver = (asynPortDriver*) lua_touserdata(state, -1);
	lua_pop(state, 1);

	if (driver)
	{
		int paramIndex;
		asynStatus status = driver->findParam(key, &paramIndex);

		if (status == asynSuccess)
		{
			/* Found a parameter -- create a proxy, cache it, return it */
			lua_getfield(state, 1, "_lua_state");
			lua_State* drvState = (lua_State*) lua_touserdata(state, -1);
			lua_pop(state, 1);

			lua_getfield(state, 1, "_param_proxies");
			push_paramproxy(state, driver, drvState, 0, paramIndex, key);
			lua_pushvalue(state, -1);           /* duplicate for return */
			lua_setfield(state, -3, key);       /* cache in _param_proxies */
			lua_remove(state, -2);              /* remove _param_proxies */
			return 1;
		}
	}

	lua_pushnil(state);
	return 1;
}

/* __newindex on driver proxy */
static int l_driverproxy_newindex(lua_State* state)
{
	const char* key = luaL_checkstring(state, 2);

	/* Don't allow overwriting internal fields */
	if (key[0] == '_') return 0;

	/* Store in the internal state table */
	lua_getfield(state, 1, "_state_data");
	lua_pushvalue(state, 3);
	lua_setfield(state, -2, key);
	lua_pop(state, 1);

	return 0;
}


/*
 * Helper: creates a driver proxy table for the given asynPortDriver.
 * Used by both asyn.driver.find() and asyn.driver.new().
 * Pushes the proxy table onto the stack.
 *
 * drvState is the Lua state where callbacks are stored:
 *   - For server drivers (new): the calling state
 *   - For client drivers (find): NULL (no callback binding)
 */
static void push_driverproxy(lua_State* state, asynPortDriver* driver, lua_State* drvState)
{
	lua_newtable(state);

	lua_newtable(state);
	lua_setfield(state, -2, "_param_proxies");

	lua_newtable(state);
	lua_setfield(state, -2, "_state_data");

	lua_pushlightuserdata(state, driver);
	lua_setfield(state, -2, "_driver");

	if (drvState)
	{
		lua_pushlightuserdata(state, drvState);
	}
	else
	{
		lua_pushnil(state);
	}
	lua_setfield(state, -2, "_lua_state");

	/* Ensure metatable is registered */
	if (luaL_newmetatable(state, "lua_driverproxy"))
	{
		lua_pushcfunction(state, l_driverproxy_index);
		lua_setfield(state, -2, "__index");
		lua_pushcfunction(state, l_driverproxy_newindex);
		lua_setfield(state, -2, "__newindex");

		lua_newtable(state);
		lua_pushstring(state, ".portName                   -- port name (property)"); lua_rawseti(state, -2, 1);
		lua_pushstring(state, ".maxAddr                    -- max address (property)"); lua_rawseti(state, -2, 2);
		lua_pushstring(state, ".PARAM                      -- access param proxy by name"); lua_rawseti(state, -2, 3);
		lua_pushstring(state, ":callParamCallbacks()"); lua_rawseti(state, -2, 4);
		lua_pushstring(state, ":writeParam(name, value)"); lua_rawseti(state, -2, 5);
		lua_pushstring(state, ":readParam(name)"); lua_rawseti(state, -2, 6);
		lua_setfield(state, -2, "_doc");
	}
	lua_setmetatable(state, -2);
}


/*
 * asyn.driver.new(portName, paramTable [, initFunc])
 *
 * Creates a new luaAsynPortDriver with the given parameters.
 */
static int l_driver_new(lua_State* state)
{
	const char* portName = luaL_checkstring(state, 1);
	luaL_checktype(state, 2, LUA_TTABLE);

	/* Create the driver using the calling Lua state */
	luaAsynPortDriver* cppDriver = new luaAsynPortDriver(portName, state);

	/* Create the driver proxy */
	push_driverproxy(state, cppDriver, state);

	int proxyIndex = lua_gettop(state);

	/* Store the driver proxy in the registry for callback self access */
	lua_pushvalue(state, proxyIndex);
	cppDriver->selfRef = luaL_ref(state, LUA_REGISTRYINDEX);

	/* Process the parameter table */
	int paramCount = luaL_len(state, 2);

	for (int i = 1; i <= paramCount; i++)
	{
		lua_geti(state, 2, i);

		if (!lua_istable(state, -1))
		{
			lua_pop(state, 1);
			continue;
		}

		lua_getfield(state, -1, "name");
		const char* paramName = lua_tostring(state, -1);
		lua_pop(state, 1);

		lua_getfield(state, -1, "type");
		int paramType = lua_tointeger(state, -1);
		lua_pop(state, 1);

		if (!paramName) { lua_pop(state, 1); continue; }

		int index;
		cppDriver->createParam(paramName, (asynParamType) paramType, &index);

		/* Set default value if provided */
		lua_getfield(state, -1, "default");
		if (!lua_isnil(state, -1))
		{
			asyn_pullparam(state, cppDriver, 0, index, lua_gettop(state));
		}
		lua_pop(state, 1);

		/* Create param proxy and cache it */
		lua_getfield(state, proxyIndex, "_param_proxies");
		push_paramproxy(state, cppDriver, state, 0, index, paramName);
		lua_setfield(state, -2, paramName);
		lua_pop(state, 1);

		lua_pop(state, 1);  /* param spec table */
	}

	cppDriver->callParamCallbacks();

	/* Call init function if provided */
	if (lua_isfunction(state, 3))
	{
		lua_pushvalue(state, 3);
		lua_pushvalue(state, proxyIndex);
		if (lua_pcall(state, 1, 0, 0) != LUA_OK)
		{
			const char* err = lua_tostring(state, -1);
			printf("Error in driver init: %s\n", err);
			lua_pop(state, 1);
		}
	}

	lua_pushvalue(state, proxyIndex);
	return 1;
}

/*
 * asyn.driver.find(portName)
 *
 * Finds an existing asynPortDriver and returns a unified driver proxy.
 * The proxy supports the same drv.PARAM.value interface as asyn.driver.new(),
 * but without callback binding (read/write assignment is not allowed).
 */
static int l_driver_find(lua_State* state)
{
	const char* portName = luaL_checkstring(state, 1);

	asynPortDriver* driver = find_driver(portName);
	if (!driver)    { return luaL_error(state, "No asynPortDriver found with name: %s\n", portName); }

	push_driverproxy(state, driver, NULL);
	return 1;
}

/*
 * asyn.driver(portName) -- shortcut to asyn.driver.find(portName)
 * via __call metamethod on the driver table
 */
static int l_driver_call(lua_State* state)
{
	/* arg 1 is the driver table itself (from __call), arg 2 is portName */
	lua_remove(state, 1);
	return l_driver_find(state);
}


// #############################

extern "C"
{	
	void luaGenerateDriver(lua_State* state, const char* port_name)
	{
		asynPortDriver* driver = find_driver(port_name);
		
		if (driver)    { push_driverproxy(state, driver, NULL); }
		else           { lua_pushnil(state); }
	}
	
	void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param)
	{
		push_clientproxy(state, port_name, addr, param ? param : "");
	}
}



int luaopen_asyn (lua_State *L)
{
	static const luaL_Reg asynlib[] = {
		{"setTrace", l_setTrace},
		{"setTraceIO", l_setTraceIO},
		{"setOption", l_setOption},
		{"setDoubleParam", l_setDoubleParam},
		{"setIntegerParam", l_setIntegerParam},
		{"setStringParam", l_setStringParam},
		{"setParam", l_setParam},
		{"getDoubleParam", l_getDoubleParam},
		{"getIntegerParam", l_getIntegerParam},
		{"getStringParam", l_getStringParam},
		{"getParam", l_getParam},
		{"readParam", l_readParam},
		{"writeParam", l_writeParam},
		{"callParamCallbacks", l_callParamCallbacks},
		{"read", l_read},
		{"write", l_write},
		{"writeread", l_writeread},
		{"setInTerminator", l_setInTerminator},
		{"setOutTerminator", l_setOutTerminator},
		{"setWriteTimeout", l_setWriteTimeout},
		{"setReadTimeout", l_setReadTimeout},
		{"setWriteReadTimeout", l_setWriteReadTimeout},
		{"getInTerminator", l_getInTerminator},
		{"getOutTerminator", l_getOutTerminator},
		{"getWriteTimeout", l_getWriteTimeout},
		{"getReadTimeout", l_getReadTimeout},
		{"getWriteReadTimeout", l_getWriteReadTimeout},
		{NULL, NULL}
	};

	luaL_newlib(L, asynlib);

	/* The module table is now on top of the stack.
	 * Add type constructors, driver table, and client table to it. */

	lua_pushinteger(L, asynParamInt32);
	lua_pushcclosure(L, l_type_constructor, 1);
	lua_setfield(L, -2, "Int32");

	lua_pushinteger(L, asynParamFloat64);
	lua_pushcclosure(L, l_type_constructor, 1);
	lua_setfield(L, -2, "Float64");

	lua_pushinteger(L, asynParamOctet);
	lua_pushcclosure(L, l_type_constructor, 1);
	lua_setfield(L, -2, "Octet");

	/* Create asyn.driver table with .new, .find, and __call -> find */
	lua_newtable(L);

	lua_pushcfunction(L, l_driver_new);
	lua_setfield(L, -2, "new");

	lua_pushcfunction(L, l_driver_find);
	lua_setfield(L, -2, "find");

	/* Set __call metamethod on the driver table to shortcut to find */
	lua_newtable(L);
	lua_pushcfunction(L, l_driver_call);
	lua_setfield(L, -2, "__call");
	lua_setmetatable(L, -2);

	lua_setfield(L, -2, "driver");

	/* Create asyn.client table with .find and __call -> find */
	lua_newtable(L);

	lua_pushcfunction(L, l_client_find);
	lua_setfield(L, -2, "find");

	lua_newtable(L);
	lua_pushcfunction(L, l_client_call);
	lua_setfield(L, -2, "__call");
	lua_setmetatable(L, -2);

	lua_setfield(L, -2, "client");

	/* Ensure param spec metatable is registered */
	ensure_paramspec_meta(L);

	/* Documentation for info(asyn) */
	lua_newtable(L);
	lua_pushstring(L, ".read(port [, addr [, param]])"); lua_rawseti(L, -2, 1);
	lua_pushstring(L, ".write(data, port [, addr [, param]])"); lua_rawseti(L, -2, 2);
	lua_pushstring(L, ".writeread(data, port [, addr [, param]])"); lua_rawseti(L, -2, 3);
	lua_pushstring(L, ".setOption(port, key, val [, addr])"); lua_rawseti(L, -2, 4);
	lua_pushstring(L, ".setInTerminator(term) / .setOutTerminator(term)"); lua_rawseti(L, -2, 5);
	lua_pushstring(L, ".getInTerminator() / .getOutTerminator()"); lua_rawseti(L, -2, 6);
	lua_pushstring(L, ".setReadTimeout(t) / .setWriteTimeout(t) / .setWriteReadTimeout(t)"); lua_rawseti(L, -2, 7);
	lua_pushstring(L, ".getReadTimeout() / .getWriteTimeout() / .getWriteReadTimeout()"); lua_rawseti(L, -2, 8);
	lua_pushstring(L, ".getParam(port, param [, addr]) / .setParam(port, param, value [, addr])"); lua_rawseti(L, -2, 9);
	lua_pushstring(L, ".getIntegerParam / .getDoubleParam / .getStringParam"); lua_rawseti(L, -2, 10);
	lua_pushstring(L, ".setIntegerParam / .setDoubleParam / .setStringParam"); lua_rawseti(L, -2, 11);
	lua_pushstring(L, ".readParam(port, param [, addr]) / .writeParam(port, param, value [, addr])"); lua_rawseti(L, -2, 12);
	lua_pushstring(L, ".callParamCallbacks(port [, addr])"); lua_rawseti(L, -2, 13);
	lua_pushstring(L, ".setTrace(port, mask) -- error=0x1, device=0x2, filter=0x4, driver=0x8, flow=0x10, warning=0x20"); lua_rawseti(L, -2, 14);
	lua_pushstring(L, ".setTraceIO(port, mask) -- nodata=0x0, ascii=0x1, escape=0x2, hex=0x4"); lua_rawseti(L, -2, 15);
	lua_pushstring(L, ".driver.new(port, params [, initFunc]) -- create asynPortDriver"); lua_rawseti(L, -2, 16);
	lua_pushstring(L, ".driver.find(port) -- find existing asynPortDriver"); lua_rawseti(L, -2, 17);
	lua_pushstring(L, ".client(port [, addr [, param]]) -- create asynOctetClient"); lua_rawseti(L, -2, 18);
	lua_pushstring(L, ".client.find(port [, addr [, param]]) -- find/create client"); lua_rawseti(L, -2, 19);
	lua_pushstring(L, ".Int32(name [, default]) / .Float64(name [, default]) / .Octet(name [, default])"); lua_rawseti(L, -2, 20);
	lua_setfield(L, -2, "_doc");

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
