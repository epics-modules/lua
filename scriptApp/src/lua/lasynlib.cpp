extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <asynPortClient.h>
#include "stdio.h"
#include <cstring>
#include <string>

static const char* try_get_string(lua_State* state, char* name)
{
	lua_getfield(state, -1, name);
	const char* output = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	if (! output)
	{
		lua_getglobal(state, name);
		output = lua_tostring(state, -1);
		lua_remove(state, -1);
	}

	return output;
}

static double try_get_num(lua_State* state, char* name, int* success)
{
	lua_getfield(state, -1, name);
	double output = lua_tonumberx(state, -1, success);
	lua_remove(state, -1);
	
	if (! success)
	{
		lua_getglobal(state, name);
		output = lua_tonumberx(state, -1, success);
		lua_remove(state, -1);
	}
	
	return output;
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
	
	if (port == NULL) { return 0; }
	
	int isnum;
	
	lua_getglobal(state, "ReadTimeout");
	double timeout = lua_tonumberx(state, -1, &isnum);
	lua_remove(state, -1);
	
	lua_getglobal(state, "InTerminator");
	const char* in_term = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	std::string output;
	
	try
	{
		asynOctetClient input(port, addr, param);
		
		if (isnum)    { input.setTimeout(timeout); }
		if (in_term)  { input.setInputEos(in_term, strlen(in_term)); }
		
		char buffer[1000];
				
		size_t numread;
		int eomReason;
		
		do
		{
			input.read(buffer, sizeof(buffer), &numread, &eomReason);
			output += std::string(buffer, numread);
		} while (eomReason == ASYN_EOM_CNT);
		
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
	
	lua_getglobal(state, "WriteTimeout");
	double timeout = lua_tonumberx(state, -1, &isnum);
	lua_remove(state, -1);
	
	lua_getglobal(state, "OutTerminator");
	const char* out_term = lua_tostring(state, -1);
	lua_remove(state, -1);
	
	try
	{
		asynOctetClient output(port, addr, param);
			
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

static const luaL_Reg mylib[] = {
	{"read", l_read},
	{"write", l_write},
	{"setOutTerminator", l_setOutTerminator},
	{"setInTerminator", l_setInTerminator},
	{"setWriteTimeout", l_setWriteTimeout},
	{"setReadTimeout", l_setReadTimeout},
	{NULL, NULL}  /* sentinel */
};


int luaopen_asyn (lua_State *L) 
{
	luaL_newlib(L, mylib);
	return 1;
}
