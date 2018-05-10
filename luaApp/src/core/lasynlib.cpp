extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <asynPortDriver.h>
#include <asynPortClient.h>
#include "stdio.h"
#include <cstring>
#include <string>

static int asyn_read(lua_State* state, const char* port, int addr, const char* param, const char* in_term)
{
	std::string output;
	int isnum;
	
	try
	{
		asynOctetClient input(port, addr, param);
		
		lua_getglobal(state, "ReadTimeout");
		double timeout = lua_tonumberx(state, -1, &isnum);
		lua_remove(state, -1);

		if (isnum)    { input.setTimeout(timeout); }
		if (in_term)  { input.setInputEos(in_term, strlen(in_term)); }
		
		char buffer[128];
				
		size_t numread;
		int eomReason;
		
		do
		{
			input.read(buffer, sizeof(buffer), &numread, &eomReason);
			output += std::string(buffer, numread);
		} while (eomReason & ASYN_EOM_CNT);
		
		if (output.empty())    { return 0; }

		lua_pushstring(state, output.c_str());
		return 1;
	}
	catch (std::runtime_error& e)
	{
		printf("asyn.read: %s\n", e.what());
		return 0;
	}
	catch (...)
	{
		return 0;
	}
	
	return 0;
}

static int asyn_write(lua_State* state, const char* data, const char* port, int addr, const char* param, const char* out_term)
{	
	int isnum;
	
	try
	{
		asynOctetClient output(port, addr, param);
			
		lua_getglobal(state, "WriteTimeout");
		double timeout = lua_tonumberx(state, -1, &isnum);
		lua_remove(state, -1);

		if (isnum)       { output.setTimeout(timeout); }
		if (out_term)    { output.setOutputEos(out_term, strlen(out_term)); }
			
		size_t numwrite;       
		output.write(data, strlen(data), &numwrite);

		return 0;
	}
	catch (std::runtime_error& e)
	{
		printf("asyn.write: %s\n", e.what());
		return 0;
	}
	catch (...)
	{
		return 0;
	}
	
	return 0;
}

static int l_read(lua_State* state)
{
	int num_ops = lua_gettop(state);
	
	if (num_ops < 1 || num_ops > 3)    { return 0; }
	
	if (num_ops >= 1 && ! lua_isstring(state, 1))    { return 0; }
	if (num_ops >= 2 && ! lua_isnumber(state, 2))    { return 0; }
	if (num_ops == 3 && ! lua_isstring(state, 3))    { return 0; }
	
	const char* port = lua_tostring(state, 1);
	int addr = (int) lua_tonumber(state, 2);
	const char* param = lua_tostring(state, 3);
	
	if (port == NULL)    { return 0; }
	
	lua_getglobal(state, "InTerminator");
	const char* in_term = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	return asyn_read(state, port, addr, param, in_term);
}


static int l_write(lua_State* state)
{
	int num_ops = lua_gettop(state);
	
	if (num_ops < 2 || num_ops > 4)    { return 0; }
	
	if (num_ops >= 1 && ! lua_isstring(state, 1))    { return 0; }
	if (num_ops >= 2 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops >= 3 && ! lua_isnumber(state, 3))    { return 0; }
	if (num_ops == 4 && ! lua_isstring(state, 4))    { return 0; }
	
	
	const char* data = lua_tostring(state, 1);
	const char* port = lua_tostring(state, 2);
	int addr = (int) lua_tonumber(state, 3);
	const char* param = lua_tostring(state, 4);
	
	if (port == NULL)    { return 0; }
	
	lua_getglobal(state, "OutTerminator");
	const char* out_term = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	return asyn_write(state, data, port, addr, param, out_term);
}

static int asyn_writeread(lua_State* state, const char* data, const char* port, int addr, const char* param, const char* in_term, const char* out_term)
{
	int isnum;

	try
	{
		asynOctetClient client(port, addr, param);

		lua_getglobal(state, "WriteReadTimeout");
		double timeout = lua_tonumberx(state, -1, &isnum);
		lua_remove(state, -1);

		if (isnum)       { client.setTimeout(timeout); }
		if (out_term)    { client.setOutputEos(out_term, strlen(out_term)); }
		if (in_term)     { client.setInputEos(in_term, strlen(in_term)); }

		size_t numwrite, numread;
		char buffer[256];
		int eomReason;
		
		client.writeRead(data, strlen(data), buffer, sizeof(buffer), &numwrite, &numread, &eomReason);
		
		std::string output(buffer, numread);
		
		if (output.empty())    { return 0; }

		lua_pushstring(state, output.c_str());
		return 1;      
	}
	catch (std::runtime_error& e)
	{
		printf("asyn.writeread: %s\n", e.what());
		return 0;
	}
	catch (...)
	{
		return 0;
	}
	
	return 0;
}

static int l_writeread(lua_State* state)
{
	int num_ops = lua_gettop(state);
	
	if (num_ops < 2 || num_ops > 4)    { return 0; }
	
	if (num_ops >= 1 && ! lua_isstring(state, 1))    { return 0; }
	if (num_ops >= 2 && ! lua_isstring(state, 2))    { return 0; }
	if (num_ops >= 3 && ! lua_isnumber(state, 3))    { return 0; }
	if (num_ops == 4 && ! lua_isstring(state, 4))    { return 0; }
	
	const char* data = lua_tostring(state, 1);
	const char* port = lua_tostring(state, 2);
	int addr = (int) lua_tonumber(state, 3);
	const char* param = lua_tostring(state, 4);
	
	if (port == NULL)    { return 0; }
	
	lua_getglobal(state, "OutTerminator");
	const char* out_term = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	lua_getglobal(state, "InTerminator");
	const char* in_term = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	return asyn_writeread(state, data, port, addr, param, in_term, out_term);
}

static int l_setOutTerminator(lua_State* state)
{
	if (lua_isstring(state, -1))    { lua_setglobal(state, "OutTerminator"); }
	
	return 0;
}

static int l_getOutTerminator(lua_State* state)
{
	return lua_getglobal(state, "OutTerminator");
}

static int l_setInTerminator(lua_State* state)
{
	if (lua_isstring(state, -1))    { lua_setglobal(state, "InTerminator"); }
	
	return 0;
}

static int l_getInTerminator(lua_State* state)
{
	return lua_getglobal(state, "InTerminator");
}

static int l_setWriteTimeout(lua_State* state)
{
	if (lua_isnumber(state, -1))    { lua_setglobal(state, "WriteTimeout"); }
	
	return 0;
}

static int l_getWriteTimeout(lua_State* state)
{
	return lua_getglobal(state, "WriteTimeout");
}

static int l_setReadTimeout(lua_State* state)
{
	if (lua_isnumber(state, -1))    { lua_setglobal(state, "ReadTimeout"); }
	
	return 0;
}

static int l_getReadTimeout(lua_State* state)
{
	return lua_getglobal(state, "ReadTimeout");
}

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
	
	int index;
	
	driver->findParam(addr, param, &index);
	driver->getStringParam(addr, index, 255, value);

	std::string temp(value);
	
	lua_pushstring(state, temp.c_str());
	return 1;
}

static int l_callParamCallbacks(lua_State* state)
{
	int num_ops = lua_gettop(state);
	
	if (num_ops < 1 || num_ops > 2)    { return 0; }
	
	if (! lua_isstring(state, 1))      { return 0; }
	
	if (num_ops == 2 && ! lua_isnumber(state, 2))    { return 0; }
	
	const char* port = lua_tostring(state, 1);
	
	int addr = 0;
	
	if (num_ops == 2)    { addr = (int) lua_tonumber(state, 2); }
	
	asynPortDriver* driver = (asynPortDriver*) findAsynPortDriver(port);
	
	driver->callParamCallbacks(addr);
	
	return 0;
}


static int l_portread(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.read' instead of ':read'?)\n");
		return 0;
	}
	
	lua_getfield(state, 1, "port_name");
	const char* port = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "param_name");
	const char* param = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "addr");
	int addr = lua_tonumber(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "in_term");
	const char* in_term = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	return asyn_read(state, port, addr, param, in_term);
}

static int l_portwrite(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.write' instead of ':write'?)\n");
		return 0;
	}
	
	if (! lua_isstring(state, 2))    { return 0; }
	
	const char* output = lua_tostring(state, 2);
	
	lua_getfield(state, 1, "port_name");
	const char* port = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "param_name");
	const char* param = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "addr");
	int addr = lua_tonumber(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "out_term");
	const char* out_term = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	return asyn_write(state, output, port, addr, param, out_term);
}

static int l_portwriteread(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.writeread' instead of ':writeread'?)\n");
		return 0;
	}
	
	if (! lua_isstring(state, 2))    { return 0; }
	
	const char* output = lua_tostring(state, 2);
	
	lua_getfield(state, 1, "port_name");
	const char* port = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "param_name");
	const char* param = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "addr");
	int addr = lua_tonumber(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "in_term");
	const char* in_term = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "out_term");
	const char* out_term = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);

	return asyn_writeread(state, output, port, addr, param, in_term, out_term);
}

static int l_porteosout(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.setOutTerminator' instead of ':setOutTerminator'?)\n");
		return 0;
	}
	
	if (! lua_isstring(state, 2))    { return 0; }
	
	lua_pushvalue(state, 2);
	lua_setfield(state, 1, "out_term");
	
	return 0;
}

static int l_porteosin(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.setInTerminator' instead of ':setInTerminator'?)\n");
		return 0;
	}
	
	if (! lua_isstring(state, 2))    { return 0; }
	
	lua_pushvalue(state, 2);
	lua_setfield(state, 1, "in_term");
	
	return 0;
}

static int l_portoutget(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.getOutTerminator' instead of ':getOutTerminator'?)\n");
		return 0;
	}
	
	lua_getfield(state, 1, "out_term");
	
	return 1;
}

static int l_portinget(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.getInTerminator' instead of ':getInTerminator'?)\n");
		return 0;
	}
	
	lua_getfield(state, 1, "in_term");
	
	return 1;
}

static int l_portnameget(lua_State* state)
{
	if (! lua_istable(state, 1))
	{
		printf("Port reference not given. (Did you use '.getPortName' instead of ':getPortName'?)\n");
		return 0;
	}
	
	lua_getfield(state, 1, "port_name");
	
	return 1;
}


static const luaL_Reg port_funcs[] = {
	{"read", l_portread},
	{"write", l_portwrite},
	{"writeread", l_portwriteread},
	{"getPortName", l_portnameget},
	{"setOutTerminator", l_porteosout},
	{"getOutTerminator", l_portoutget},
	{"setInTerminator", l_porteosin},
	{"getInTerminator", l_portinget},
	{NULL, NULL}
};



extern "C"
{
void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param)
{	
	luaL_newmetatable(state, "port_meta");
	lua_pop(state, 1);
	
	lua_newtable(state);
	luaL_setfuncs(state, port_funcs, 0);
	
	lua_pushstring(state, port_name);
	lua_setfield(state, -2, "port_name");
	
	lua_pushstring(state, param);
	lua_setfield(state, -2, "param_name");
	
	lua_pushnumber(state, addr);
	lua_setfield(state, -2, "addr");

	lua_getglobal(state, "OutTerminator");
	lua_setfield(state, -2, "out_term");
	
	lua_getglobal(state, "InTerminator");
	lua_setfield(state, -2, "in_term");

	luaL_setmetatable(state, "port_meta");
}
}


static int l_createport(lua_State* state)
{
	int num_ops = lua_gettop(state);
	
	if (num_ops < 1 || num_ops > 3)    { return 0; }
	
	if (num_ops >= 1 && ! lua_isstring(state, 1))    { return 0; }
	if (num_ops >= 2 && ! lua_isnumber(state, 2))    { return 0; }
	if (num_ops == 3 && ! lua_isstring(state, 3))    { return 0; }
	
	const char* port = lua_tostring(state, 1);
	int addr = (int) lua_tonumber(state, 2);
	const char* param = lua_tostring(state, 3);
	
	luaGeneratePort(state, port, addr, param);
	
	return 1;
}



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
	{"port", l_createport},
	{NULL, NULL}  /* sentinel */
};


int luaopen_asyn (lua_State *L) 
{
	luaL_newlib(L, mylib);
	return 1;
}
