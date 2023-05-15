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
		
	protected:
		asynPortDriver* driver = NULL;
		int addr;
		
	public:
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
		
		int index_get(lua_State* state);
		int index_set(lua_State* state);
		
		int readParam(lua_State* state);
		int writeParam(lua_State* state);
		
		void callParamCallbacks(int addr);
};


class lua_asynOctetClient
{
	private:
		lua_asynOctetClient(std::string port_name, int addr, std::string param);
		~lua_asynOctetClient();
		
	protected:
		asynOctetClient* port = NULL;
		std::string name;
		int addr;
		std::string param;
		
	public:
		static lua_asynOctetClient* find(std::string port_name, int addr, std::string param)
		{
			return new lua_asynOctetClient(port_name, addr, param);
		}
		
		static void destroy(lua_asynOctetClient* instance)    { delete instance; }
		
		int read(lua_State* state);
		int write(lua_State* state);
		int writeread(lua_State* state);
		
		int trace(lua_State* state);
		int traceio(lua_State* state);
		int option(lua_State* state);
		
		int index_get(lua_State* state);
		int index_set(lua_State* state);
};

#endif

#endif

#endif
