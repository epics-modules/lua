#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <map>

#if defined(__vxworks) || defined(vxWorks)
	#include <symLib.h>
	#include <sysSymTbl.h>
#endif

#include <macLib.h>
#include <iocsh.h>
#include <epicsStdio.h>
#include <epicsFindSymbol.h>
#include <epicsExport.h>

#define epicsExportSharedSymbols
#include "luaEpics.h"


typedef std::vector<std::pair<const char*, lua_CFunction> >::iterator reg_iter;

static std::vector<std::pair<const char*, lua_CFunction> > registered_libs;
static std::vector<std::pair<const char*, lua_CFunction> > registered_funcs;

static std::map<std::string, lua_State*> named_states;

static FILE* temp_help = tmpfile();

/* Hook Routines */

epicsShareDef LUA_LIBRARY_LOAD_HOOK_ROUTINE luaLoadLibraryHook = NULL;
epicsShareDef LUA_FUNCTION_LOAD_HOOK_ROUTINE luaLoadFunctionHook = NULL;

/*
 * Attempts to find a given filename within the folders listed
 * in the environment variable "LUA_SCRIPT_PATH"
 */
epicsShareFunc std::string luaLocateFile(std::string filename)
{
	/* Check if the filename is an absolute path */
	if (filename.at(0) == '/' && std::ifstream(filename.c_str()).good())    { return filename; }

	/* Otherwise, see if the file exists in the script path */
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

	std::stringstream path;

	if   (env_path)
	{
		path << env_path;
		path << ":";
	}

	path << ".";

	std::string segment;

	while (std::getline(path, segment, ':'))
	{
		std::string fullpath = segment + "/" + filename;

		/* Check if file exists. If so, return the full filepath */
		if (std::ifstream(fullpath.c_str()).good())    { return fullpath; }
	}

	return "";
}


/*
 * Finds the given file, loads it as bytecode, and runs it. Returns
 * any erros that occur in this process.
 */
epicsShareFunc int luaLoadScript(lua_State* state, const char* script_file)
{
	std::string found = luaLocateFile(std::string(script_file));

	if (found.empty())    { return -1; }

	int status = luaL_loadfile(state, found.c_str());

	if (status)    { return status; }

	/* Run the script so that functions are in the global state */
	return lua_pcall(state, 0, 0, 0);

}


/*
 * Helper wrapper around luaL_loadstring that returns an error on
 * an empty string rather than a success code. Cleans up some
 * other code so that it doesn't have to check for empty strings.
 */
epicsShareFunc int luaLoadString(lua_State* state, const char* lua_code)
{
	if (std::string(lua_code).empty())    { return -1; }

	return luaL_loadstring(state, lua_code);
}


/*
 * Converts a string into a lua value and pushes it to the stack.
 * Runs the string in a sandbox environment and parses the output
 * type to determine what to push to the stack. Any values that aren't
 * recognized or aren't parsed as a number, string, or boolean are
 * treated like strings in order to allow for unquoted strings.
 */
static void strtolua(lua_State* state, std::string text)
{
	size_t trim_front = text.find_first_not_of(" ");
	size_t trim_back  = text.find_last_not_of(" ");

	text = text.substr(trim_front, trim_back - trim_front + 1);

	std::stringstream convert;
	convert << "return " << text;

	lua_State* sandbox = luaL_newstate();
	int err = luaL_dostring(sandbox, convert.str().c_str());

	int type = lua_type(sandbox, -1);

	if      (err)                  { lua_pushstring(state, text.c_str()); }
	else if (type == LUA_TNUMBER)  { lua_pushnumber(state, lua_tonumber(sandbox, -1)); }
	else if (type == LUA_TSTRING)  { lua_pushstring(state, lua_tostring(sandbox, -1)); }
	else if (type == LUA_TBOOLEAN) { lua_pushboolean(state, lua_toboolean(sandbox, -1)); }
	else                           { lua_pushstring(state, text.c_str()); }
}


/*
 * Takes a set of comma-separated values, parses them, and pushes
 * the equivalent lua values to the stack. Returns the number of
 * values in the list. Useful for allowing epics strings to contain
 * text that looks like a function call.
 */
epicsShareFunc int luaLoadParams(lua_State* state, const char* param_list)
{
	std::stringstream parse(param_list);
	std::string param;

	int num_params = 0;

	while (std::getline(parse, param, ','))
	{
		strtolua(state, param);
		num_params += 1;
	}

	return num_params;
}


/*
 * Takes a set of comma-separated macro definitions in the form of
 * key-value pairs. Adds a scope of global variables according to
 * the keys parsed. Variables can be popped off with luaPopScope
 */
epicsShareFunc void luaLoadMacros(lua_State* state, const char* macro_list)
{
	char** pairs;

	if (macro_list)
	{
		lua_newtable(state);

		macParseDefns(NULL, macro_list, &pairs);

		for ( ; pairs && pairs[0]; pairs += 2)
		{
			std::string param(pairs[1]);

			strtolua(state, param);

			lua_setfield(state, -2, pairs[0]);
		}

		lua_pushvalue(state, -1);
		lua_setfield(state, -2, "__index");
		luaPushScope(state);
	}
}


/**
 * Attempts to load a given library name using the 'require' function
 */

epicsShareFunc int luaLoadLibrary(lua_State* state, const char* lib_name)
{
	lua_getglobal(state, "require");
	lua_pushstring(state, lib_name);
	int status = lua_pcall(state, 1, 1, 0);
	
	if (status == LUA_OK)    { lua_setglobal(state, lib_name); }
	
	return status;
}


/*
 * Assumes that there is a table with a defined __index field
 * at the top of the stack. Creates a scope of global variables
 * that can be removed with luaPopScope.
 */
epicsShareFunc void luaPushScope(lua_State* state)
{
	lua_getglobal(state, "_G");

	/*
	 * If there's an existing scope, push it down
	 * as the metatable of the table being added.
	 */
	if( lua_getmetatable(state, -1) )
	{
		lua_setmetatable(state, -3);
	}

	lua_pushvalue(state, -2);
	lua_setmetatable(state, -2);
	lua_pop(state, 2);
}

epicsShareFunc void luaPopScope(lua_State* state)
{
	lua_getglobal(state, "_G");
	lua_getmetatable(state, -1);
	lua_getmetatable(state, -1);
	lua_setmetatable(state, -2);
	lua_pop(state, 1);
}


/*
 * Takes a library name and a function that pushes a table of functions to the stack.
 * When a library is called to be included in a lua script, the name is attempted to
 * be matched against the given library name. Used to allow libraries to be registered
 * during the ioc registrar calls.
 */
epicsShareFunc void luaRegisterLibrary(const char* library_name, lua_CFunction library_func)
{
	std::pair<const char*, lua_CFunction> temp(library_name, library_func);

	registered_libs.push_back(temp);

	if (luaLoadLibraryHook)    { luaLoadLibraryHook(library_name, library_func); }
}


/*
 * Binds a given lua function to the given name in every state created with the
 * luaCreateState function.
 */
epicsShareFunc void luaRegisterFunction(const char* function_name, lua_CFunction function)
{
	std::pair<const char*, lua_CFunction> temp(function_name, function);

	registered_funcs.push_back(temp);

	if (luaLoadFunctionHook)    { luaLoadFunctionHook(function_name, function); }
}


/*
 * Search function that gets added to lua. Gets called when "require" is used.
 * Searches for the named library in the list of registered libraries.
 */
static int luaCheckLibrary(lua_State* state)
{
	std::string libname(lua_tostring(state, 1));

	for (reg_iter index = registered_libs.begin(); index != registered_libs.end(); index++)
	{
		if (libname == std::string(index->first))
		{
			lua_pushcfunction(state, index->second);
			lua_pushnil(state);
			return 2;
		}
	}

	std::stringstream funcname;
	funcname << "\n\tno library registered '" << libname << "'";

	lua_pushstring(state, funcname.str().c_str());
	return 1;
}


/*
 * Called on every state created with luaCreateState. Adds the above
 * function into the list of lua package searchers and then registers
 * all the functions within the registered_funcs list.
 */
epicsShareFunc void luaLoadRegistered(lua_State* state)
{
	/**
	 * Add luaCheckLibrary as an additional function for
	 * lua to use to find libraries when using 'require'
	 */
	lua_getglobal(state, "package");
	lua_getfield(state, -1, "searchers");
	lua_len(state, -1);
	int num_searchers = lua_tonumber(state, -1);
	lua_pop(state, 1);
	lua_pushcfunction(state, luaCheckLibrary);
	lua_seti(state, -2, num_searchers + 1);
	lua_pop(state, 2);

	for (reg_iter index = registered_funcs.begin(); index != registered_funcs.end(); index++)
	{
		lua_register( state, index->first, index->second);
	}
}


/*
 * Function to be run when an ioc shell object is called.
 * Constructs a line of iocsh code and then uses iocshCmd
 * to run the line. Necessary due to epics base removal of
 * access to the ioc function registrar.
 */
static int l_call(lua_State* state)
{
	lua_getfield(state, 1, "function_name");
	const char* function_name = lua_tostring(state, lua_gettop(state));
	lua_pop(state, 1);

	int given_args = lua_gettop(state) - 1;

    std::stringstream parameters;

	for(int index = 0; index < given_args; index += 1)
	{
		if (index > 0)    { parameters << ", "; }

		switch (lua_type(state, index + 2))
		{
			case LUA_TNIL:
				parameters << "0";
				break;

			case LUA_TNUMBER:
				parameters << lua_tostring(state, index + 2);
				break;

			case LUA_TSTRING:
				parameters << '"';
				parameters << lua_tostring(state, index + 2);
				parameters << '"';
				break;

			case LUA_TBOOLEAN:
				parameters << (lua_toboolean(state, index + 2) ? "1" : "0");
				break;

			case LUA_TLIGHTUSERDATA:
			{
				void* userdata = lua_touserdata(state, index + 2);

				lua_getglobal(state, "pdbbase");
				void* pdb = lua_touserdata(state, -1);
				lua_pop(state, 1);

				if (userdata == pdb)    { parameters << "pdbbase"; }
				else                    { parameters << "\"\""; }

				break;
			}

			default:
				parameters << "\"\"";
				break;
		}
	}

	std::stringstream cmd;
	cmd << function_name;
	cmd << "(";
	cmd << parameters.str();
	cmd << ")";

	iocshCmd(cmd.str().c_str());

	return 0;
}


/*
 * Parses the output of the iocsh help command to try
 * and find a given function name. Necessary due to the
 * removal of epics base access to the ioc function registrar.
 * Returns true/false if it finds a matching name.
 */
static bool parseHelp(const char* func_name)
{
	// If there was an issue with initial tmpfile, attempt retry
	if (temp_help == NULL)
	{
		temp_help = tmpfile();

		/*
		 * If things still don't work, then default to assuming
		 * everything is a potential iocsh function
		 */
		if (temp_help == NULL) { return true; }
	}

	FILE* prev = epicsGetThreadStdout();

	epicsSetThreadStdout(temp_help);
	iocshCmd("help()");
	epicsSetThreadStdout(prev);

	long size = ftell(temp_help);
	rewind(temp_help);
	char* buffer = (char*) malloc(sizeof(char) * size);

	fread(buffer, 1, size, temp_help);
	std::stringstream help_str;

	help_str.str(buffer);
	free(buffer);

	rewind(temp_help);

	std::string line;
	std::string element;

	int skipped_first_line = 0;

	while (std::getline(help_str, line, '\n'))
	{
		if (! skipped_first_line)
		{
			skipped_first_line = 1;
			continue;
		}

		std::stringstream check_line;
		check_line.str(line);

		while (std::getline(check_line, element, ' '))
		{
			if(! element.empty() && element == func_name)
			{
				return true;
			}
		}
	}

	return false;
}


/*
 * Replaces the basic index call function within lua in order
 * to recognize ioc shell functions. Any unknown variable
 * name is checked against the list of functions registered
 * in the ioc shell, if it exists, return a function object,
 * otherwise return Nil as normal.
 */
static int l_iocindex(lua_State* state)
{
	const char* symbol_name = lua_tostring(state, 2);

	if (std::string(symbol_name) == "exit")
	{
		lua_pushlightuserdata(state, NULL);
		return lua_error(state);
	}

	std::stringstream environ_check;

	environ_check << "return (os.getenv('";
	environ_check << symbol_name;
	environ_check << "'))";

	luaL_dostring(state, environ_check.str().c_str());

	if (! lua_isnil(state, -1)) { return 1; }

	lua_pop(state, 1);

	if (! parseHelp(symbol_name)) { return 0; }

	static const luaL_Reg func_meta[] = {
		{"__call", l_call},
		{NULL, NULL}
	};

	luaL_newmetatable(state, "func_meta");
	luaL_setfuncs(state, func_meta, 0);
	lua_pop(state, 1);

	lua_newtable(state);
	luaL_setmetatable(state, "func_meta");

	lua_pushstring(state, symbol_name);
	lua_setfield(state, -2, "function_name");

	return 1;
}


/*
 * Put as a size-of function to trick lua into allowing
 * #ENABLE_HASH_COMMENTS, adds a global variable that
 * gets used by the lua shell to ignore lines that start
 * with '#' to enable some backwards compatibility.
 */
static int l_iochash_enable(lua_State* state)
{
	lua_pushstring(state, "YES");
	lua_setglobal(state, "LEPICS_HASH_COMMENTS");

	lua_pushstring(state, "Accepting iocsh-style comments");

	return 1;
}



/*
 * Replacement print function to be used instead
 * of the normal lua print. Used so that epics
 * stdout redirection will affect the lua print
 * statements.
 */
static int l_replaceprint(lua_State* state)
{
	int num_args = lua_gettop(state);  /* number of arguments */
	lua_getglobal(state, "tostring");

	for (int index = 1; index <= num_args; index += 1)
	{
		const char *s;
		size_t l;

		lua_pushvalue(state, -1);  /* function to be called */
		lua_pushvalue(state, index);   /* value to print */
		lua_call(state, 1, 1);

		s = lua_tolstring(state, -1, &l);  /* get result */

		if (s == NULL)
		{
			return luaL_error(state, "'tostring' must return a string to 'print'");
		}

		if (index > 1) { epicsStdoutPrintf("\t"); }

		epicsStdoutPrintf("%s", s);
		lua_pop(state, 1);  /* pop result */
	}

	epicsStdoutPrintf("\n");
	return 0;
}


int luaopen_iocsh (lua_State* state)
{
	static const luaL_Reg iocsh_meta[] = {
		{"__index", l_iocindex},
		{NULL, NULL}  /* sentinel */
	};

	static const luaL_Reg iocsh_funcs[] = {
		{NULL, NULL}
	};

	static const luaL_Reg hash_meta[] = {
		{"__len", l_iochash_enable},
		{NULL, NULL}
	};

	luaL_newmetatable(state, "iocsh_meta");
	luaL_setfuncs(state, iocsh_meta, 0);
	lua_pop(state, 1);

	luaL_newmetatable(state, "hash_enable_meta");
	luaL_setfuncs(state, hash_meta, 0);
	lua_pop(state, 1);

	luaL_newlib(state, iocsh_funcs);
	luaL_setmetatable(state, "iocsh_meta");

	lua_newtable(state);
	luaL_setmetatable(state, "hash_enable_meta");
	lua_setglobal(state, "ENABLE_HASH_COMMENTS");

	return 1;
}


/*
 * Generates a new lua state for the caller,
 * binds in the defined epics libraries and
 * functions.
 */
epicsShareFunc lua_State* luaCreateState()
{
	lua_State* output = luaL_newstate();
	luaL_openlibs(output);
	luaLoadRegistered(output);

	lua_register(output, "print", l_replaceprint);

	luaL_requiref(output, "iocsh", luaopen_iocsh, 1);
	lua_pop(output, 1);

	return output;
}


/*
 * Creates a new named lua_State or returns an existing
 * one. Used to allow data to be shared between different
 * epics uses of lua. Most notably, allowing variables from
 * startup scripts able to continue into another call of
 * luash.
 */
epicsShareFunc lua_State* luaNamedState(const char* name)
{
	if (! name) { return NULL; }

	std::string state_name(name);

	if (named_states.find(state_name) != named_states.end())
	{
		return named_states[state_name];
	}

	lua_State* output = luaCreateState();

	named_states[state_name] = output;

	return output;
}
