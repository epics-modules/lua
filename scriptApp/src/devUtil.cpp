#include <string>
#include <cstdlib>

#include "link.h"

#include "luaEpics.h"

class Protocol
{
	public:
		Protocol(const char* inpout);
		long run();

		std::string filename;
		std::string function_name;
		std::string portname;
		std::string param_list;
		
	private:
		lua_State* state;
		
		int parseParams();
};


extern "C"
{
	void* parseINPOUT(const struct link* inpout)
	{
		return (void*) new Protocol(inpout->text);
	}
	
	long runScript(void* dpvt)
	{
		Protocol* proto = (Protocol*) dpvt;
		
		return proto->run();
	}
}

Protocol::Protocol(const char* inpout)
{	
	printf("%s\n", inpout);

	this->state = luaL_newstate();
	
	std::string code(inpout);
	
	size_t first_split = code.find_first_of(" ");
	size_t last_split  = code.find_last_of(" ");
	
	if (code.empty() or 
	    (code[0] != '@') or
		(first_split == last_split) or
		(last_split == std::string::npos))
	{
		printf("Error parsing INP string, format is '@filename function portname'\n");
		return;
	}
	
	this->filename = code.substr(1, first_split - 1);
	this->portname = code.substr(last_split + 1, code.length() - last_split - 1);
	
	std::string function = code.substr(first_split + 1, last_split - first_split - 1);
	
	size_t start_params = function.find_first_of("(");
	size_t end_params   = function.find_last_of(")");
	
	bool has_start = start_params != std::string::npos;
	bool has_end   = end_params   != std::string::npos;
	
	if ((has_start and not has_end) or (has_end and not has_start))
	{
		printf("Error parsing function parameters, format is 'function_name(param1,param2,...)'\n");
		return;
	}
	
	this->function_name = function.substr(0, start_params);
	this->param_list    = function.substr(start_params + 1, end_params - start_params - 1);
	
	if (luaLoadScript(this->state, filename.c_str()))
	{
		printf("Error loading file: %s\n", filename.c_str());
	}
	
	lua_pushstring(this->state, portname.c_str());
	lua_setglobal(this->state, "PORT");
}

long Protocol::run()
{
	lua_getglobal(this->state, this->function_name.c_str());
	
	int params = this->parseParams();
	
	long status = lua_pcall(this->state, params, 1, 0);
	
	if (status)
	{
		//log error
	}
	
	return status;
}

int Protocol::parseParams()
{
	if (this->param_list.empty())    { return 0; }

	int num_params = 0;

	size_t curr = 0;
	size_t next;

	do
	{
		/* Get the next parameter */
		next = this->param_list.find(",", curr);

		std::string param;

		/* If we find the end of the string, just take everything */
		if   (next == std::string::npos)    { param = this->param_list.substr(curr); }
		else                                { param = this->param_list.substr(curr, next - curr); }

		int trim_front = param.find_first_not_of(" ");
		int trim_back  = param.find_last_not_of(" ");

		param = param.substr(trim_front, trim_back - trim_front + 1);
		
		/* Treat anything with quotes as a string, anything else as a number */
		if (param.at(0) == '"' or param.at(0) == '\'')
		{
			lua_pushstring(this->state, param.substr(1, param.length() - 2).c_str());
		}
		else
		{
			lua_pushnumber(this->state, strtod(param.c_str(), NULL));
		}

		num_params += 1;
		curr = next + 1;
	} while (curr);

	return num_params;
}
