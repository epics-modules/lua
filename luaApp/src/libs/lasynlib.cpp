#include <asynPortDriver.h>
#include <asynPortClient.h>
#include "stdio.h"
#include <cstring>
#include <string>
#include <epicsExport.h>
#include "luaEpics.h"

static std::string asyn_read(lua_State* state, asynOctetClient* port)
{
	std::string output;

	try
	{		
		char buffer[128];

		size_t numread;
		int eomReason;

		do
		{
			port->read(buffer, sizeof(buffer), &numread, &eomReason);
			output += std::string(buffer, numread);
		} while (eomReason & ASYN_EOM_CNT);

		return output;
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception while reading\n"); }

	return "";
}

static void asyn_write(lua_State* state, asynOctetClient* port, const char* data)
{
	try
	{
		size_t numwrite;
		port->write(data, strlen(data), &numwrite);
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception while writing\n"); }
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

	std::string response = asyn_read(state, &input);
	
	if (response.empty())    { lua_pushnil(state); }
	else                     { lua_pushstring(state, response.c_str()); }

	return 1;
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
	
	asyn_write(state, &output, data);
	return 0;
}

static std::string asyn_writeread(lua_State* state, asynOctetClient* client, const char* data)
{
	try
	{
		size_t numwrite, numread;
		char buffer[256];
		int eomReason;

		client->writeRead(data, strlen(data), buffer, sizeof(buffer), &numwrite, &numread, &eomReason);

		return std::string(buffer, numread);
	}
	catch (std::runtime_error& e)    { luaL_error(state, "%s\n", e.what()); }
	catch (...)                      { luaL_error(state, "Unexpected exception during writeread\n"); }

	return "";
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

	std::string response = asyn_writeread(state, &client, data);
	
	if (response.empty())    { lua_pushnil(state); }
	else                     { lua_pushstring(state, response.c_str()); }
	return 1;
}

static int l_setOutTerminator(lua_State* state)
{
	if (lua_isstring(state, 1))    { lua_setglobal(state, "OutTerminator"); }

	return 0;
}

static int l_setInTerminator(lua_State* state)
{
	if (lua_isstring(state, 1))    { lua_setglobal(state, "InTerminator"); }

	return 0;
}

static int l_setWriteTimeout(lua_State* state)
{
	if (lua_isnumber(state, 1))    { lua_setglobal(state, "WriteTimeout"); }

	return 0;
}

static int l_setReadTimeout(lua_State* state)
{
	if (lua_isnumber(state, 1))    { lua_setglobal(state, "ReadTimeout"); }

	return 0;
}

static int l_getOutTerminator(lua_State* state)    { return lua_getglobal(state, "OutTerminator"); }
static int l_getInTerminator(lua_State* state)     { return lua_getglobal(state, "InTerminator"); }
static int l_getWriteTimeout(lua_State* state)     { return lua_getglobal(state, "WriteTimeout"); }
static int l_getReadTimeout(lua_State* state)      { return lua_getglobal(state, "ReadTimeout"); }


static int l_setIntegerParam(lua_State* state)
{
	int num_ops = lua_gettop(state);

	if (num_ops < 3 || num_ops > 4)    { return 0; }

	if (! lua_isstring(state, 1))      { return 0; }

	if (num_ops == 3 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops == 4 && ! lua_isnumber(state, 2))    { return 0; }

	if (num_ops == 3 && ! lua_isnumber(state, 3))    { return 0; }
	if (num_ops == 4 && ! lua_isstring(state, 3))    { return 0; }

	if (num_ops == 4 && ! lua_isnumber(state, 4))    { return 0; }

	const char* port = lua_tostring(state, 1);
	const char* param = lua_tostring(state, num_ops - 1);

	int addr = 0;

	if (num_ops == 4)    { addr = (int) lua_tonumber(state, 2); }

	int value = (int) lua_tonumber(state, num_ops);

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	int index;

	driver->findParam(addr, param, &index);
	driver->setIntegerParam(addr, index, value);

	return 0;
}

static int l_getIntegerParam(lua_State* state)
{
	int num_ops = lua_gettop(state);

	if (num_ops < 2 || num_ops > 3)    { return 0; }

	if (! lua_isstring(state, 1))      { return 0; }

	if (num_ops == 2 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops == 3 && ! lua_isnumber(state, 2))    { return 0; }

	if (num_ops == 3 && ! lua_isstring(state, 3))    { return 0; }

	const char* port = lua_tostring(state, 1);
	const char* param = lua_tostring(state, num_ops);

	int addr = 0;

	if (num_ops == 3)    { addr = (int) lua_tonumber(state, 2); }

	int value;

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	int index;

	driver->findParam(addr, param, &index);
	driver->getIntegerParam(addr, index, &value);

	lua_pushnumber(state, value);
	return 1;
}


static int l_setDoubleParam(lua_State* state)
{
	int num_ops = lua_gettop(state);

	if (num_ops < 3 || num_ops > 4)    { return 0; }

	if (! lua_isstring(state, 1))      { return 0; }

	if (num_ops == 3 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops == 4 && ! lua_isnumber(state, 2))    { return 0; }

	if (num_ops == 3 && ! lua_isnumber(state, 3))    { return 0; }
	if (num_ops == 4 && ! lua_isstring(state, 3))    { return 0; }

	if (num_ops == 4 && ! lua_isnumber(state, 4))    { return 0; }

	const char* port = lua_tostring(state, 1);
	const char* param = lua_tostring(state, num_ops - 1);

	int addr = 0;

	if (num_ops == 4)    { addr = (int) lua_tonumber(state, 2); }

	double value = lua_tonumber(state, num_ops);

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	int index;

	driver->findParam(addr, param, &index);
	driver->setDoubleParam(addr, index, value);

	return 0;
}

static int l_getDoubleParam(lua_State* state)
{
	int num_ops = lua_gettop(state);

	if (num_ops < 2 || num_ops > 3)    { return 0; }

	if (! lua_isstring(state, 1))      { return 0; }

	if (num_ops == 2 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops == 3 && ! lua_isnumber(state, 2))    { return 0; }

	if (num_ops == 3 && ! lua_isstring(state, 3))    { return 0; }

	const char* port = lua_tostring(state, 1);
	const char* param = lua_tostring(state, num_ops);

	int addr = 0;

	if (num_ops == 3)    { addr = (int) lua_tonumber(state, 2); }

	double value;

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	int index;

	driver->findParam(addr, param, &index);
	driver->getDoubleParam(addr, index, &value);

	lua_pushnumber(state, value);
	return 1;
}

static int l_setStringParam(lua_State* state)
{
	int num_ops = lua_gettop(state);

	if (num_ops < 3 || num_ops > 4)    { return 0; }

	if (! lua_isstring(state, 1))      { return 0; }

	if (num_ops == 3 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops == 4 && ! lua_isnumber(state, 2))    { return 0; }

	if (! lua_isstring(state, 3))      { return 0; }

	if (num_ops == 4 && ! lua_isstring(state, 4))    { return 0; }

	const char* port = lua_tostring(state, 1);
	const char* param = lua_tostring(state, num_ops - 1);

	int addr = 0;

	if (num_ops == 4)    { addr = (int) lua_tonumber(state, 2); }

	const char* value = lua_tostring(state, num_ops);

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	int index;

	driver->findParam(addr, param, &index);
	driver->setStringParam(addr, index, value);

	return 0;
}

static int l_getStringParam(lua_State* state)
{	
	int num_ops = lua_gettop(state);

	if (num_ops < 2 || num_ops > 3)    { return 0; }

	if (! lua_isstring(state, 1))      { return 0; }

	if (num_ops == 2 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops == 3 && ! lua_isnumber(state, 2))    { return 0; }

	if (num_ops == 3 && ! lua_isstring(state, 3))    { return 0; }

	const char* port = lua_tostring(state, 1);
	const char* param = lua_tostring(state, num_ops);

	int addr = 0;

	if (num_ops == 3)    { addr = (int) lua_tonumber(state, 2); }

	char value[255];

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	int index;

	driver->findParam(addr, param, &index);
	driver->getStringParam(addr, index, 255, value);

	std::string temp(value);

	lua_pushstring(state, temp.c_str());
	return 1;
}

static int l_callParamCallbacks(lua_State* state)
{
	lua_settop(state, 2);

	const char* port = lua_tostring(state, 1);

	if (port == NULL)    { return 0; }

	int addr = (int) lua_tonumber(state, 2);

	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);

	if (driver == NULL)    { return luaL_error(state, "No driver found with name: %s\n", port); }

	driver->callParamCallbacks(addr);

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
	
	std::string response = asyn_read(state, port);

	if (response.empty())    { lua_pushnil(state); }
	else                     { lua_pushstring(state, response.c_str()); }
	return 1;
}

static int l_portwrite(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	if (! lua_isstring(state, 2))    { return 0; }

	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "WriteTimeout");
	if (! lua_isnil(state, -1))    { port->setTimeout(lua_tonumber(state, -1)); }
	lua_pop(state, 1);
	
	const char* output = lua_tostring(state, 2);
	lua_len(state, 2);
	int data_len = lua_tointeger(state, -1);
	lua_pop(state, 1);
	
	asyn_write(state, port, output);
	
	return 0;
}

static int l_portwriteread(lua_State* state)
{
	luaL_checktype(state, 1, LUA_TTABLE);

	if (! lua_isstring(state, 2))    { return 0; }

	lua_getfield(state, 1, "client");
	asynOctetClient* port = (asynOctetClient*) lua_touserdata(state, -1);
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "WriteReadTimeout");
	if (! lua_isnil(state, -1))    { port->setTimeout(lua_tonumber(state, -1)); }
	lua_pop(state, 1);
	
	const char* output = lua_tostring(state, 2);
	lua_len(state, 2);
	int data_len = lua_tointeger(state, -1);
	lua_pop(state, 1);
	
	std::string response = asyn_writeread(state, port, output);

	if (response.empty())    { lua_pushnil(state); }
	else                     { lua_pushstring(state, response.c_str()); }
	return 1;
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

extern "C"
{
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
		{"setIntegerParam", l_setIntegerParam},
		{"setDoubleParam", l_setDoubleParam},
		{"setStringParam", l_setStringParam},
		{"getIntegerParam", l_getIntegerParam},
		{"getDoubleParam", l_getDoubleParam},
		{"getStringParam", l_getStringParam},
		{"callParamCallbacks", l_callParamCallbacks},
		{"client", l_createport},
		{NULL, NULL}  /* sentinel */
	};

	luaL_newlib(L, mylib);
	return 1;
}


static void libasynRegister(void)    { luaRegisterLibrary("asyn", luaopen_asyn); }

extern "C"
{
	epicsExportRegistrar(libasynRegister);
}
