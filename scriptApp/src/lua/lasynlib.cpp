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
	
	if (port == NULL) { return 0; }
	
	int isnum;
	std::string output;
	
	try
	{
		asynOctetClient input(port, addr, param);
		
        lua_getglobal(state, "ReadTimeout");
        double timeout = lua_tonumberx(state, -1, &isnum);
        lua_remove(state, -1);
        
        lua_getglobal(state, "InTerminator");
        const char* in_term = lua_tostring(state, -1);
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
	catch (...)
	{
		return 0;
	}
	
	return 0;
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
	
	int isnum;
	
	try
	{
		asynOctetClient output(port, addr, param);
			
        lua_getglobal(state, "WriteTimeout");
        double timeout = lua_tonumberx(state, -1, &isnum);
        lua_remove(state, -1);
        
        lua_getglobal(state, "OutTerminator");
        const char* out_term = lua_tostring(state, -1);
        lua_remove(state, -1);
        
		if (isnum)       { output.setTimeout(timeout); }
		if (out_term)    { output.setOutputEos(out_term, strlen(out_term)); }
			
		size_t numwrite;       
		output.write(data, strlen(data), &numwrite);
        
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
	
	int isnum;
    
	try
	{
		asynOctetClient client(port, addr, param);
        
        lua_getglobal(state, "WriteReadTimeout");
        double timeout = lua_tonumberx(state, -1, &isnum);
        lua_remove(state, -1);
        
        lua_getglobal(state, "OutTerminator");
        const char* out_term = lua_tostring(state, -1);
        lua_remove(state, -1);
        
        lua_getglobal(state, "InTerminator");
        const char* in_term = lua_tostring(state, -1);
        lua_remove(state, -1);
        
		if (isnum)       { client.setTimeout(timeout); }
		if (out_term)    { client.setOutputEos(out_term, strlen(out_term)); }
		if (in_term)  { client.setInputEos(in_term, strlen(in_term)); }
        
		size_t numwrite, numread;
		char buffer[256];
		int eomReason;
		
        client.writeRead(data, strlen(data), buffer, sizeof(buffer), &numwrite, &numread, &eomReason);
        
        std::string output(buffer, numread);
		
		if (output.empty())    { return 0; }
        
		lua_pushstring(state, output.c_str());
		return 1;      
	}
	catch (...)
	{
		return 0;
	}
	
	return 0;
}

static int l_setOutTerminator(lua_State* state)
{
	if (lua_isstring(state, -1))    { lua_setglobal(state, "OutTerminator"); }
	
	return 0;
}

static int l_setInTerminator(lua_State* state)
{
	if (lua_isstring(state, -1))    { lua_setglobal(state, "InTerminator"); }
	
	return 0;
}

static int l_setWriteTimeout(lua_State* state)
{
	if (lua_isnumber(state, -1))    { lua_setglobal(state, "WriteTimeout"); }
	
	return 0;
}

static int l_setReadTimeout(lua_State* state)
{
	if (lua_isnumber(state, -1))    { lua_setglobal(state, "ReadTimeout"); }
	
	return 0;
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

static const luaL_Reg mylib[] = {
	{"read", l_read},
	{"write", l_write},
    {"writeread", l_writeread},
	{"setOutTerminator", l_setOutTerminator},
	{"setInTerminator", l_setInTerminator},
	{"setWriteTimeout", l_setWriteTimeout},
	{"setReadTimeout", l_setReadTimeout},
	{"setIntegerParam", l_setIntegerParam},
	{"setDoubleParam", l_setDoubleParam},
	{"getIntegerParam", l_getIntegerParam},
	{"getDoubleParam", l_getDoubleParam},
    {"callParamCallbacks", l_callParamCallbacks},
	{NULL, NULL}  /* sentinel */
};


int luaopen_asyn (lua_State *L) 
{
	luaL_newlib(L, mylib);
	return 1;
}
