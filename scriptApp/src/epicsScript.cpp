#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>

#if defined(__vxworks) || defined(vxWorks)
	#include <symLib.h>
	#include <sysSymTbl.h>
#endif


#include <macLib.h>

#include "epicsScript.h"

static std::string locateFile(std::string filename)
{
	char* env_path = std::getenv("LUA_SCRIPT_PATH");
	
	#if defined(__vxworks) || defined(vxWorks)
	/* For compatibility reasons look for global symbols */
	if (!env_path)
	{
		char* symbol;
		SYM_TYPE type;

		if (symFindByName(sysSymTbl, "LUA_SCRIPT_PATH", &symbol, &type) == OK)
		{
			env_path = *(char**) symbol;
		}
	}
	#endif
	
	std::string path;
	
	if   (env_path)    { path = std::string(env_path); }
	else               { path = "."; }
	
	size_t start = 0;
	size_t next;
	
	do
	{
		next = path.find(":", start);
		
		std::string test = path.substr(start, next - start)  + "/" + filename;
		
		/* Check if file exists. If so, return the full filepath */
		if (std::ifstream(test.c_str()).good())    { return test; }
		
		start = next + 1;
	} while (start);
	
	return "";
}


int luaLoadScript(lua_State* state, const char* script_file)
{	
	std::string found = locateFile(std::string(script_file));
	
	if (! found.empty())
	{
		luaL_openlibs(state);
	
		int status = luaL_loadfile(state, found.c_str());
		
		if (status)    { return status; }
		
		/* Run the script so that functions are in the global state */
		status = lua_pcall(state, 0, 0, 0);
		
		if (status)    { return status; }
		
		return 0;
	}
	
	return -1;
}

int luaLoadString(lua_State* state, const char* lua_code)
{
	std::string code(lua_code);
	
	if (! code.empty())
	{
		luaL_openlibs(state);
	
		int status = luaL_loadstring(state, lua_code);
		
		if (status)    { return status; }
		
		return 0;
	}
	
	return -1;
}

void luaLoadParams(lua_State* state, const char* param_list)
{
	char** pairs;
	
	if (param_list)
	{
		macParseDefns(NULL, param_list, &pairs);
		
		for ( ; pairs && pairs[0]; pairs += 2)
		{
			std::string param(pairs[1]);
			
			double test;
			std::istringstream convert(param);
			
			/* Attempt to convert the parameter into a number */
			if (convert >> test)    { lua_pushnumber(state, test); }
			else
			{
				/* Othewise it's a string, remove quotes if necessary */
				if (param.at(0) == '"' or param.at(0) == '\'')
				{
					lua_pushstring(state, param.substr(1, param.length() - 2).c_str());
				}
				else
				{
					lua_pushstring(state, param.c_str());
				}
			}
			
			lua_setglobal(state, pairs[0]);
		}
	}
}
