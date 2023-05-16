#ifndef INC_LASYNLIB_H
#define INC_LASYNLIB_H

#include "luaEpics.h"

#ifdef __cplusplus

#include <string>
#include <asynPortDriver.h>
#include <asynPortClient.h>

#ifdef CXX11_SUPPORTED

class lua_asynPortDriver
{
	private:
		lua_asynPortDriver(std::string driver_name, int addr);

		
	public:
		asynPortDriver* driver = NULL;
		int addr;
		
		static lua_asynPortDriver* find(const std::string port)
		{
			asynPortDriver* try_find = (asynPortDriver*) findAsynPortDriver(port.c_str());
			
			if (try_find == NULL || std::string(try_find->portName) != port )
			{
				return NULL;
			}
			
			return new lua_asynPortDriver(port, 0);
		}
		
		static void destroy(lua_asynPortDriver* instance)    { delete instance; }
};


class lua_asynOctetClient
{
	private:
		lua_asynOctetClient(std::string port_name, int addr, std::string param);
		~lua_asynOctetClient();
		
	public:
		asynOctetClient* port = NULL;
		std::string name;
		int addr;
		std::string param;
		
		static lua_asynOctetClient* find(std::string port_name, int addr, std::string param)
		{
			return new lua_asynOctetClient(port_name, addr, param);
		}
		
		static void destroy(lua_asynOctetClient* instance)    { delete instance; }
				
		void trace(lua_State* state);
		void traceio(lua_State* state);
		int option(lua_State* state);
};

#endif

extern "C" {
#endif
	
	void luaGenerateDriver(lua_State* state, const char* port_name);
	void luaGeneratePort(lua_State* state, const char* port_name, int addr, const char* param);
	
#ifdef __cplusplus
}
#endif

#endif
