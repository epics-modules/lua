#include <asynPortDriver.h>
#include <asynPortClient.h>
#include "stdio.h"
#include <cstring>
#include <string>

#ifdef _MSC_VER

    #include <string.h>
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp

#else

    #include <strings.h>

#endif

#include <epicsExport.h>
#include "luaPortDriver.h"
#include "luaEpics.h"
#include "lasynlib.h"

/*
 * __call metatable function on the datatable once the
 * name has been defined.
 *
 * Takes in a multi-line block of code for the parameter
 * to run when read or written. Builds the code into lua
 * bytecode and then transfers all the stored parts of
 * the datatable into a single storage area named _params
 * for the asyn port driver to read.
 */
static int l_call(lua_State* state)
{
	lua_getfield(state, 1, "name");
	const char* param_name = lua_tostring(state, -1);
	lua_pop(state, 1);

	lua_getfield(state, 1, "direction");
	std::string direction(lua_tostring(state, -1));
	lua_pop(state, 1);

	std::string data(luaL_checkstring(state, 2));

	lua_getfield(state, LUA_REGISTRYINDEX, "LPORTDRIVER_PARAMS");
	lua_getfield(state, -1, param_name);

	int status = 0;

	if (direction == "read")
	{
		data = "return function(self) " + data + " end";
		status = luaL_dostring(state, data.c_str());
		if (!status) { lua_setfield(state, -2, "read_bind"); }
	}
	else if (direction == "write")
	{
		data = "return function(value, self) " + data + " end";
		status = luaL_dostring(state, data.c_str());
		if (!status) { lua_setfield(state, -2, "write_bind"); }
	}

	if (status)    { return luaL_error(state, lua_tostring(state, -1)); }

	lua_setfield(state, -2, param_name);
	lua_pop(state, 1);

	return 0;
}

/*
 * __call metatable function on the datatable once the parameter
 * type declaration has finished.
 *
 * Because lua has a syntactic sugar construct where a
 * single argument function can be called without parentheses,
 * we can abuse this a bit by making it look like a type
 * being applied to a variable. So, what is technically
 * 'paramtype("name")' ends up being written as 'paramtype name'.
 *
 * This function adds the name parameter to the running datatable
 * and then switches the __call metatable function to l_call.
 */
static int l_addname(lua_State* state)
{
	static const luaL_Reg data_set[] = {
		{"__call", l_call},
		{NULL, NULL}
	};

	if (luaL_newmetatable(state, "data_set")) { luaL_setfuncs(state, data_set, 0); }
	lua_pop(state, 1);

	const char* param_name = luaL_checkstring(state, 2);

	lua_pushvalue(state, 1);
	luaL_setmetatable(state, "data_set");

	lua_pushstring(state, param_name);
	lua_setfield(state, -2, "name");


	/*
	 * Add name to list of parameters here so that
	 * Basic parameters can be created without need
	 * to bind code to them.
	 */
	lua_getfield(state, LUA_REGISTRYINDEX, "LPORTDRIVER_PARAMS");
	lua_getfield(state, -1, param_name);

	if (lua_isnil(state, -1))
	{
		lua_pop(state, 1);
		lua_newtable(state);
	}

	lua_pushstring(state, param_name);
	lua_setfield(state, -2, "name");

	lua_getfield(state, 1, "type");
	lua_setfield(state, -2, "type");

	lua_setfield(state, -2, param_name);
	lua_pop(state, 1);

	return 1;
}

/*
 * __index metatable function on the datatable after the asyn
 * parameter type is specified.
 *
 * Recognizes the indices "read" and "write" and stores them
 * in the datatable. Then sets the __call metatable function
 * to l_addname.
 */
static int l_readwrite(lua_State* state)
{
	const char* readwrite = luaL_checkstring(state, 2);

	lua_pushvalue(state, 1);

	if      (strcasecmp(readwrite, "read") == 0)   { lua_pushstring(state, "read"); }
	else if (strcasecmp(readwrite, "write") == 0)  { lua_pushstring(state, "write"); }
	else                                           { lua_pop(state, 1); return 0;  }

	lua_setfield(state, -2, "direction");

	return 1;
}

/*
 * __index metatable function for the datatable param
 *
 * This is syntactic sugar to allow for complex data
 * type definitions. Users will be able to type:
 *
 *    param.<param_type>.<read/write>
 *
 * and lua will transform it into
 *
 *    param.__index(<param_type>).__index(<read/write)
 *
 * Which allows us to store data in the param table
 * about the values that are coming in and keep them
 * until the entire parameter definition is finished.
 * At that point, the data can be cleaned up and stored
 * in a better form in a different table for the port
 * driver to use.
 *
 * Stores the given data type into the table if it matches
 * with a list of potential data types, then sets the
 * table's __index metamethod to l_readwrite.
 */
static int l_index(lua_State* state)
{
	const char* param_type = luaL_checkstring(state, 2);

	lua_newtable(state);

	if      (strcasecmp(param_type, "int32") == 0)         { lua_pushinteger(state, asynParamInt32); }
	else if (strcasecmp(param_type, "uint32digital") == 0) { lua_pushinteger(state, 2); }
	else if (strcasecmp(param_type, "float64") == 0)       { lua_pushinteger(state, asynParamFloat64); }
	else if (strcasecmp(param_type, "string") == 0)        { lua_pushinteger(state, asynParamOctet); }
	else if (strcasecmp(param_type, "octet") == 0)         { lua_pushinteger(state, asynParamOctet); }
	else                                                   { lua_pushinteger(state, 0); }

	lua_setfield(state, -2, "type");

	static const luaL_Reg in_out[] = {
		{"__index", l_readwrite},
		{"__call", l_addname},
		{NULL, NULL}
	};

	if (luaL_newmetatable(state, "in_out"))    { luaL_setfuncs(state, in_out, 0); }
	lua_pop(state, 1);

	luaL_setmetatable(state, "in_out");

	return 1;
}


luaPortDriver::~luaPortDriver()   { lua_close(this->state); }

luaPortDriver::luaPortDriver(const char* port_name, const char* lua_filepath, const char* lua_macros)
	:asynPortDriver(port_name, 1,
		asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask,
		asynInt32Mask | asynFloat64Mask | asynOctetMask,
		0, 1, 0, 0)
{
	static const luaL_Reg param_get[] = {
		{"__index", l_index},
		{NULL, NULL}
	};

	this->state = luaCreateState();
	luaLoadLibrary(this->state, "asyn");

	lua_pushstring(this->state, port_name);
	lua_setglobal(this->state, "PORT");

	luaL_newmetatable(this->state, "param_get");
	luaL_setfuncs(this->state, param_get, 0);
	lua_pop(this->state, 1);

	lua_newtable(this->state);
	luaL_setmetatable(this->state, "param_get");
	lua_setglobal(this->state, "param");

	lua_newtable(this->state);
	lua_setfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_PARAMS");

	luaLoadMacros(this->state, lua_macros);

	int status = luaLoadScript(this->state, lua_filepath);

	if (status)
	{
		const char* msg = lua_tostring(state, -1);
		printf("%s\n", msg);
		return;
	}

	lua_pushnil(this->state);
	lua_setglobal(this->state, "param");

	lua_newtable(this->state);
	lua_setfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_FUNCTIONS");

	lua_getfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_PARAMS");
	lua_pushnil(this->state);

	int index = 0;

	while(lua_next(state, -2))
	{
		lua_getfield(this->state, -1, "name");
		const char* param_name = lua_tostring(this->state, -1);
		lua_pop(this->state, 1);

		lua_getfield(this->state, -1, "type");
		int param_type = luaL_optinteger(this->state, -1, 1);
		lua_pop(this->state, 1);

		this->createParam(param_name, (asynParamType) param_type, &index);

		lua_getfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_FUNCTIONS");
		lua_newtable(this->state);

		lua_getfield(this->state, -3, "read_bind");
		lua_setfield(this->state, -2, "read");

		lua_getfield(this->state, -3, "write_bind");
		lua_setfield(this->state, -2, "write");

		lua_seti(this->state, -2, index);
		lua_pop(this->state, 2);
	}

	lua_pop(this->state, 1);

	//Clear _params, not needed anymore
	lua_pushnil(this->state);
	lua_setfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_PARAMS");
}

/*
 * Finds the defined read function according to the
 * asynuser reason and leaves it as the only addition
 * to the stack.
 */
void luaPortDriver::getReadFunction(int index)
{
	lua_getfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_FUNCTIONS");
	lua_geti(this->state, -1, index);
	lua_remove(this->state, -2);
	lua_getfield(this->state, -1, "read");
	lua_remove(this->state, -2);
}

/*
 * Pushes a lua asyn driver object onto the
 * stack for the parameter 'self'. Then
 * runs the function that's currently on the
 * stack.
 */
int luaPortDriver::callReadFunction()
{	
	luaGenerateDriver(this->state, this->portName);

	int status = lua_pcall(this->state, 1, 1, 0);

	if (status)
	{
		const char* msg = lua_tostring(this->state, -1);
		printf("%s\n", msg);
	}

	return status;
}


/*
 * Finds the defined write function according to the
 * asynuser reason and leaves it as the only addition
 * to the stack.
 */
void luaPortDriver::getWriteFunction(int index)
{
	lua_getfield(this->state, LUA_REGISTRYINDEX, "LPORTDRIVER_FUNCTIONS");
	lua_geti(this->state, -1, index);
	lua_getfield(this->state, -1, "write");
	lua_remove(this->state, -3);
	lua_remove(this->state, -2);
}


/*
 * Pushes a lua asyn driver object onto the
 * stack for the parameter 'self'. This function
 * assumes that the caller has already pushed the
 * value coming in from asyn onto the stack to be
 * used for the parameter 'value'. Then, it calls
 * the function on the stack.
 */
int luaPortDriver::callWriteFunction()
{
	luaGenerateDriver(this->state, this->portName);
		
	int status = lua_pcall(this->state, 2, 0, 0);

	if (status)
	{
		const char* msg = lua_tostring(state, -1);
		printf("%s\n", msg);
	}

	return status;
}

asynStatus luaPortDriver::writeOctet(asynUser* pasynuser, const char* value, size_t maxChars, size_t* actual)
{
	this->getWriteFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, -1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::writeOctet(pasynuser, value, maxChars, actual);
	}

	if (value != NULL)
	{
		std::string output(value);
		output.erase(maxChars);

		lua_pushstring(this->state, output.c_str());
		if (this->callWriteFunction())   { return asynError; }
	}

	return asynSuccess;
}


asynStatus luaPortDriver::writeFloat64(asynUser* pasynuser, epicsFloat64 value)
{
	this->getWriteFunction(pasynuser->reason);

	if (lua_isnil(this->state, -1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::writeFloat64(pasynuser, value);
	}

	lua_pushnumber(this->state, value);
	if (this->callWriteFunction())    { return asynError; }

	return asynSuccess;
}

asynStatus luaPortDriver::writeInt32(asynUser* pasynuser, epicsInt32 value)
{	
	this->getWriteFunction(pasynuser->reason);
	
	if (lua_isnil(this->state, -1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::writeInt32(pasynuser, value);
	}

	lua_pushinteger(this->state, value);
	if (this->callWriteFunction())    { return asynError; }
	
	return asynSuccess;
}

asynStatus luaPortDriver::readOctet(asynUser* pasynuser, char* value, size_t maxChars, size_t* actual, int* eomReason)
{
	this->getReadFunction(pasynuser->reason);

	if (lua_isnil(this->state, 1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::readOctet(pasynuser, value, maxChars, actual, eomReason);
	}

	if (this->callReadFunction())    { return asynError; }

	if (! lua_isnil(this->state, -1))
	{
		const char* retval = luaL_checklstring(this->state, -1, actual);

		if (retval != NULL)
		{
			std::string output(retval);
			output.copy(value, maxChars);
		}

		lua_pop(this->state, 1);
	}

	return asynSuccess;
}

asynStatus luaPortDriver::readFloat64(asynUser* pasynuser, epicsFloat64* value)
{
	this->getReadFunction(pasynuser->reason);

	if (lua_isnil(this->state, 1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::readFloat64(pasynuser, value);
	}

	if(this->callReadFunction())    { return asynError; }

	if (! lua_isnil(this->state, -1))
	{
		epicsFloat64 retval = lua_tonumber(this->state, -1);
		lua_pop(this->state, 1);

		*value = retval;
	}

	return asynSuccess;
}


asynStatus luaPortDriver::readInt32(asynUser* pasynuser, epicsInt32* value)
{	
	this->getReadFunction(pasynuser->reason);

	if (lua_isnil(this->state, 1))
	{
		lua_pop(this->state, 1);
		return asynPortDriver::readInt32(pasynuser, value);
	}

	if (this->callReadFunction())    { return asynError; }

	if (! lua_isnil(this->state, -1))
	{
		epicsInt32 retval = lua_tointeger(this->state, -1);
		lua_pop(this->state, -1);

		*value = retval;
	}

	return asynSuccess;
}

int lnewdriver(lua_State* state)
{
	lua_settop(state, 3);
	const char* port_name = luaL_checkstring(state, 1);
	const char* filepath = luaL_checkstring(state, 2);
	const char* macros = lua_tostring(state, 3);

	new luaPortDriver(port_name, filepath, macros);

	return 0;
}

static const iocshArg newdriverCmdArg0 = { "asyn port name", iocshArgString };
static const iocshArg newdriverCmdArg1 = { "filename of defintion file", iocshArgString };
static const iocshArg newdriverCmdArg2 = { "macro definitions", iocshArgString };
static const iocshArg* newdriverCmdArgs[3] = {&newdriverCmdArg0, &newdriverCmdArg1, &newdriverCmdArg2};
static const iocshFuncDef newdriverFuncDef = {"luaPortDriver", 3, newdriverCmdArgs};

static void newdriverCallFunc(const iocshArgBuf* args)
{
	new luaPortDriver(args[0].sval, args[1].sval, args[2].sval);
}

static void portDriverRegister(void)
{
	luaRegisterFunction("luaPortDriver", lnewdriver);
	iocshRegister(&newdriverFuncDef, newdriverCallFunc);
}

extern "C"
{
	epicsExportRegistrar(portDriverRegister);
}
