#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>

#if defined(__vxworks) || defined(vxWorks)
	#include <symLib.h>
	#include <sysSymTbl.h>
#endif

#include <macLib.h>

#define epicsExportSharedSymbols
#include "luaEpics.h"

static std::vector<std::pair<const char*, lua_CFunction> > registered;

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

epicsShareFunc void luaRegisterLibrary(const char* library_name, lua_CFunction library_func)
{
	std::pair<const char*, lua_CFunction> temp(library_name, library_func);
	
	registered.push_back(temp);
}


epicsShareFunc void luaLoadRegistered(lua_State* state)
{
	for (std::size_t index = 0; index < registered.size(); index += 1)
	{
		std::pair<const char* , lua_CFunction> temp = registered[index];
		
		luaL_requiref( state, temp.first, temp.second, 1 );
	}
}


#ifdef LUA_COMPAT_IOCSH
static int l_call(lua_State* state)
{	
	lua_getfield(state, 1, "function_definition");
	iocshFuncDef const* func_def = (iocshFuncDef const*) lua_touserdata(state, lua_gettop(state));
	lua_pop(state, 1);
	
	lua_getfield(state, 1, "function_pointer");
	iocshCallFunc to_call = (iocshCallFunc) lua_touserdata(state, lua_gettop(state));
	lua_pop(state, 1);
	
	int given_args = lua_gettop(state) - 1;
	int needed_args = func_def->nargs;
		
	char** argv = (char**) malloc(given_args * sizeof(char*));
		
	iocshArgBuf *argBuf = (iocshArgBuf*) malloc(sizeof(iocshArgBuf));
	int argBufCapacity = 0;
	
	for(int index = 0; ; )
	{
		if (index >= needed_args)    { break; }
		if (index >= argBufCapacity) 
		{
			void *np;
			
			argBufCapacity += 20;
			np = realloc (argBuf, argBufCapacity * sizeof *argBuf);
			
			if (np == NULL) 
			{
				printf("Out of memory!\n");
				argBufCapacity -= 20;
				break;
			}
			
			argBuf = (iocshArgBuf *)np;
		}
		
		int needed_type = func_def->arg[index]->type;
		
		if (needed_type == iocshArgArgv)
		{
			argBuf[index].aval.ac = given_args - index + 1;
			
			argv[0] = (char*) func_def->name;
			
			for (int i = index; i < needed_args ; i += 1)
			{
				const char* val = (i < given_args) ? lua_tostring(state, i + 2) : NULL;
				
				argv[i - index + 1] = (char*) val;
			}
			
			argBuf[index].aval.av = argv;
			index = needed_args;
		}
		else if(needed_type == iocshArgInt)
		{
			int val = (int) (index < given_args) ? lua_tonumber(state, index + 2) : 0;
		
			argBuf[index].ival = val;
		}
		else if (needed_type == iocshArgDouble)
		{
			double val = (index < given_args) ? lua_tonumber(state, index + 2) : 0.0;
		
			argBuf[index].dval = val;
		}
		else if (needed_type == iocshArgString || needed_type == iocshArgPersistentString)
		{
			const char* val = (index < given_args) ? lua_tostring(state, index + 2) : NULL;
		
			argBuf[index].sval = (char *) val;
		}
		else if (needed_type == iocshArgPdbbase)
		{
			void* val = (index < given_args) ? lua_touserdata(state, index + 2) : NULL;
			
			argBuf[index].vval = val;
		}
		
		index += 1;
	}
			
	(*to_call)(argBuf);
	free(argv);
	free(argBuf);
	
	return 0;
}


static int l_index(lua_State* state)
{
	static const luaL_Reg func_meta[] = {
		{"__call", l_call},
		{NULL, NULL}
	};
	
	
	const char* temp = lua_tostring(state, 2);
	std::string func_name(temp);
	
	lua_getfield(state, 1, "iocshCmdDef");
	iocshCmdDef* cmds = (iocshCmdDef*) lua_touserdata(state, lua_gettop(state));
	lua_pop(state, 1);
	
	while (cmds->pFuncDef != NULL && cmds->func != NULL)
	{
		if (func_name == cmds->pFuncDef->name)
		{
			luaL_newmetatable(state, "func_meta");
			luaL_setfuncs(state, func_meta, 0);
			lua_pop(state, 1);
			
			lua_newtable(state);
			luaL_setmetatable(state, "func_meta");
			
			lua_pushlightuserdata(state, (void*) cmds->pFuncDef);
			lua_setfield(state, -2, "function_definition");
			
			lua_pushlightuserdata(state, (void*) cmds->func);
			lua_setfield(state, -2, "function_pointer");
			
			return 1;
		}
		
		cmds++;
	}
	
	lua_pushnil(state);
	return 1;
}


epicsShareFunc void luaEpicsLibrary(lua_State* state, const iocshCmdDef* cmds)
{
	static const luaL_Reg epics_meta[] = {
		{"__index", l_index},
		{NULL, NULL}  /* sentinel */
	};
	
	static const luaL_Reg epics_funcs[] = {
		{NULL, NULL}
	};
	
	luaL_newmetatable(state, "epics_meta");
	luaL_setfuncs(state, epics_meta, 0);
	lua_pop(state, 1);
	
	luaL_newlib(state, epics_funcs);
	luaL_setmetatable(state, "epics_meta");
	
	lua_pushlightuserdata(state, (void*) cmds);
	lua_setfield(state, -2, "iocshCmdDef");
}

epicsShareFunc lua_State* luaCreateState()
{
	lua_State* output = luaL_newstate();
	luaL_openlibs(output);
	luaLoadRegistered(output);
	
	return output;
}
#endif
