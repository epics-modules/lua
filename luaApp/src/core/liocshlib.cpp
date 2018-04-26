extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

#include <cstdlib>
#include <cstdio>
#include "stdio.h"
#include <cstring>
#include <vector>
#include "epicsStdio.h"

#include <epicsExport.h>
#include "iocsh.h"

static int l_call(lua_State* state)
{	
	lua_getfield(state, 1, "function_name");
	
	const char* func_name = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);
	
	const iocshCmdDef* found = iocshFindCommand(func_name);
	
	if (found)
	{
		int given_args = lua_gettop(state) - 1;
		int needed_args = found->pFuncDef->nargs;
		
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
			
			int needed_type = found->pFuncDef->arg[index]->type;
			
			if (needed_type == iocshArgArgv)
			{
				argBuf[index].aval.ac = given_args - index + 1;
				
				argv[0] = (char*) func_name;
				
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
				
		(*found->func)(argBuf);
		free(argv);
		free(argBuf);
	}
	else
	{
		printf("Command %s not found.\n", func_name);
	}
	
	return 0;
}


static const luaL_Reg func_meta[] = {
	{"__call", l_call},
	{NULL, NULL}
};

static int l_index(lua_State* state)
{
	const char* func_name = lua_tostring(state, 2);
	
	luaL_newmetatable(state, "func_meta");
	luaL_setfuncs(state, func_meta, 0);
	lua_pop(state, 1);
	
	lua_newtable(state);
	luaL_setmetatable(state, "func_meta");
	
	lua_pushstring(state, func_name);	
	lua_setfield(state, -2, "function_name");
	
	return 1;
}


static const luaL_Reg iocsh_meta[] = {
	{"__index", l_index},
	{NULL, NULL}  /* sentinel */
};

static const luaL_Reg iocsh_funcs[] = {
	{NULL, NULL}
};


int luaopen_iocsh (lua_State* state) 
{	
	luaL_newmetatable(state, "iocsh_meta");
	luaL_setfuncs(state, iocsh_meta, 0);
	lua_pop(state, 1);
	
	luaL_newlib(state, iocsh_funcs);
	luaL_setmetatable(state, "iocsh_meta");
	
	return 1;
}
