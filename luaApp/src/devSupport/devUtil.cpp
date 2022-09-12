#include "devUtil.h"

#include <string>
#include <cstdlib>
#include <cstring>

#include "link.h"

#include <epicsExport.h>

#include "luaEpics.h"

extern "C"
{
	Protocol* parseINPOUT(const struct link* inpout)
	{
		Protocol* output = new Protocol;
		
		output->state = luaCreateState();
		
		std::string code(inpout->value.instio.string);
		
		size_t first_split = code.find_first_of(" ");
		size_t last_split  = code.find_last_of(" ");
		
		if (code.empty())
		{
			printf("Error parsing INP string, format is '@filename function [portname]'\n");
			return NULL;
		}
		
		strcpy(output->filename, code.substr(0, first_split).c_str());
		
		if (last_split != first_split)
		{
			strcpy(output->portname, code.substr(last_split + 1, code.length() - last_split - 1).c_str());
		}
		
		std::string function = code.substr(first_split + 1, last_split - first_split - 1);
		
		size_t start_params = function.find_first_of("(");
		size_t end_params   = function.find_last_of(")");
		
		bool has_start = start_params != std::string::npos;
		bool has_end   = end_params   != std::string::npos;
		
		if ((has_start && !has_end) || (has_end && !has_start))
		{
			printf("Error parsing function parameters, format is 'function_name(param1,param2,...)'\n");
			return NULL;
		}
		
		strcpy(output->function_name, function.substr(0, start_params).c_str());
		
		if (has_start && has_end)
		{
			strcpy(output->param_list,    function.substr(start_params + 1, end_params - start_params - 1).c_str());
		}
		
		if (luaLoadScript(output->state, output->filename))
		{
			printf("Error loading file: %s\n", output->filename);
		}
		
		lua_pushstring(output->state, output->portname);
		lua_setglobal(output->state, "PORT");
		
		return output;
	}
	
	void runFunction(Protocol* proto)
	{
		int params = luaLoadParams(proto->state, proto->param_list);
		
		long status = lua_pcall(proto->state, params + 1, 1, 0);
		
		if (status)
		{
			std::string err(lua_tostring(proto->state, -1));
			
			printf("Calling %s in %s resulted in error: %s\n", proto->function_name, proto->filename, err.c_str());
		}
	}
}
