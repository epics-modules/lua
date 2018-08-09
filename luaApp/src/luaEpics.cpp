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

#define epicsExportSharedSymbols
#include "luaEpics.h"

epicsShareFunc std::string luaLocateFile(std::string filename)
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


epicsShareFunc int luaLoadScript(lua_State* state, const char* script_file)
{	
	std::string found = luaLocateFile(std::string(script_file));
	
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

epicsShareFunc int luaLoadString(lua_State* state, const char* lua_code)
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

epicsShareFunc int luaLoadParams(lua_State* state, const char* param_list)
{
	std::string parse(param_list);
	
	int num_params = 0;

	size_t curr = 0;
	size_t next;

	do
	{
		/* Get the next parameter */
		next = parse.find(",", curr);

		std::string param;

		/* If we find the end of the string, just take everything */
		if   (next == std::string::npos)    { param = parse.substr(curr); }
		else                                { param = parse.substr(curr, next - curr); }

		size_t trim_front = param.find_first_not_of(" ");
		size_t trim_back  = param.find_last_not_of(" ");

		param = param.substr(trim_front, trim_back - trim_front + 1);
		
		/* Treat anything with quotes as a string, anything else as a number */
		if (param.at(0) == '"' || param.at(0) == '\'')
		{
			lua_pushstring(state, param.substr(1, param.length() - 2).c_str());
		}
		else
		{
			lua_pushnumber(state, strtod(param.c_str(), NULL));
		}

		num_params += 1;
		curr = next + 1;
	} while (curr);
	
	return num_params;
}

epicsShareFunc void luaLoadMacros(lua_State* state, const char* macro_list)
{
	char** pairs;
	
	if (macro_list)
	{
		macParseDefns(NULL, macro_list, &pairs);
		
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
				if (param.at(0) == '"' || param.at(0) == '\'')
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


epicsShareFunc void luaLoadEnviron(lua_State* state)
{
	#if defined(__unix__)
	extern char** environ;
	char** sp;
	
	for (sp = environ; (sp != NULL) && (*sp != NULL); sp++)
	{
		std::string line(*sp);
		
		size_t split_loc = line.find_first_of("=");
		
		std::string var_name = line.substr(0, split_loc);
		std::string var_val  = line.substr(split_loc + 1);
		
		lua_pushstring(state, var_val.c_str());
		lua_setglobal(state, var_name.c_str());
	}
	#endif
}
