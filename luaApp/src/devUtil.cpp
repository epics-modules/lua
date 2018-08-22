#include "devUtil.h"

#include <string>
#include <cstdlib>
#include <cstring>

#include "link.h"

#include <epicsExport.h>

#include "luaEpics.h"

static int parseParams(Protocol* proto)
{
	if (strlen(proto->param_list) == 0)    { return 0; }

	std::string params(proto->param_list);
	
	int num_params = 0;

	size_t curr = 0;
	size_t next;

	do
	{
		/* Get the next parameter */
		next = params.find(",", curr);

		std::string a_param;

		/* If we find the end of the string, just take everything */
		if   (next == std::string::npos)    { a_param = params.substr(curr); }
		else                                { a_param = params.substr(curr, next - curr); }

		size_t trim_front = a_param.find_first_not_of(" ");
		size_t trim_back  = a_param.find_last_not_of(" ");

		a_param = a_param.substr(trim_front, trim_back - trim_front + 1);
		
		/* Treat anything with quotes as a string, anything else as a number */
		if (a_param.at(0) == '"' || a_param.at(0) == '\'')
		{
			lua_pushstring(proto->state, a_param.substr(1, a_param.length() - 2).c_str());
		}
		else
		{
			lua_pushnumber(proto->state, strtod(a_param.c_str(), NULL));
		}

		num_params += 1;
		curr = next + 1;
	} while (curr);

	return num_params;
}



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
		int params = parseParams(proto);
		
		long status = lua_pcall(proto->state, params + 1, 1, 0);
		
		if (status)
		{
			//log error
		}
	}
}
