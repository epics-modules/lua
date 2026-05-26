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
#include <errlog.h>
#include <epicsStdio.h>
#include <epicsFindSymbol.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#include <epicsExport.h>

#define epicsExportSharedSymbols
#include "luaEpics.h"
#include "luaShell.h"


typedef std::vector<std::pair<const char*, lua_CFunction> >::iterator reg_iter;

static std::vector<std::pair<const char*, lua_CFunction> > registered_libs;
static std::vector<std::pair<const char*, lua_CFunction> > registered_funcs;
static std::vector<std::string> registered_paths;

static std::map<std::string, lua_State*> named_states;

static epicsMutex registryMutex;
static epicsMutex namedStatesMutex;

/* Forward declaration: defined in path management section below */
static void rebuildPaths(lua_State* state);

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
	if (filename.empty())    { return std::string(""); }

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

	/* Search LUA_SCRIPT_PATH directories */
	if (env_path)
	{
		std::stringstream path;
		path << env_path;

		std::string segment;
		while (std::getline(path, segment, ':'))
		{
			std::string fullpath = segment + "/" + filename;
			if (std::ifstream(fullpath.c_str()).good())    { return fullpath; }
		}
	}

	/* Search directories registered via luaAddPath */
	{
		epicsGuard<epicsMutex> guard(registryMutex);

		for (size_t i = 0; i < registered_paths.size(); i++)
		{
			std::string fullpath = registered_paths[i] + "/" + filename;
			if (std::ifstream(fullpath.c_str()).good())    { return fullpath; }
		}
	}

	/* Search current directory */
	{
		std::string fullpath = std::string("./") + filename;
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
	if (!lua_code || lua_code[0] == '\0')    { return -1; }

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

	lua_close(sandbox);
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

		char** original_pairs = pairs;

		for ( ; pairs && pairs[0]; pairs += 2)
		{
			std::string param(pairs[1]);

			strtolua(state, param);

			lua_setfield(state, -2, pairs[0]);
		}

		free(original_pairs);

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
	else                     { lua_pop(state, 1); }
	
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
	lua_getglobal(state, "_G");                 /* stack: [_G] */

	if (!lua_getmetatable(state, -1))           /* stack: [_G, mt] or [_G] */
	{
		lua_pop(state, 1);                      /* no scope to pop */
		return;
	}

	if (lua_getmetatable(state, -1))            /* stack: [_G, mt, parent_mt] */
	{
		lua_setmetatable(state, -3);            /* _G.metatable = parent_mt */
	}
	else                                        /* stack: [_G, mt] */
	{
		lua_pushnil(state);                     /* stack: [_G, mt, nil] */
		lua_setmetatable(state, -3);            /* _G.metatable = nil (clear scope) */
	}

	lua_pop(state, 2);                          /* pop mt + _G */
}


/*
 * Converts a Lua table at the given stack index to a comma-separated
 * "key=value,key=value" macro string suitable for dbLoadRecords,
 * luaSpawn, luash, etc. Only string keys are included.
 */
epicsShareFunc std::string luaMacrosFromTable(lua_State* state, int index)
{
	std::string result;

	lua_pushnil(state);
	while (lua_next(state, index))
	{
		if (lua_type(state, -2) == LUA_TSTRING)
		{
			const char* key = lua_tostring(state, -2);

			/* Duplicate the value before converting -- lua_tostring on a
			 * non-string value modifies it in-place, which corrupts lua_next */
			lua_pushvalue(state, -1);
			const char* val = lua_tostring(state, -1);
			lua_pop(state, 1);

			if (val)
			{
				if (!result.empty())    { result += ","; }
				result += std::string(key) + "=" + std::string(val);
			}
		}
		lua_pop(state, 1);
	}

	return result;
}


/*
 * Takes a library name and a function that pushes a table of functions to the stack.
 * When a library is called to be included in a lua script, the name is attempted to
 * be matched against the given library name. Used to allow libraries to be registered
 * during the ioc registrar calls.
 */
epicsShareFunc void luaRegisterLibrary(const char* library_name, lua_CFunction library_func)
{
	epicsGuard<epicsMutex> guard(registryMutex);

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
	epicsGuard<epicsMutex> guard(registryMutex);

	std::pair<const char*, lua_CFunction> temp(function_name, function);

	registered_funcs.push_back(temp);

	if (luaLoadFunctionHook)    { luaLoadFunctionHook(function_name, function); }
}


/*
 * Fallback searcher for libraries registered after the Lua state was
 * created. This handles the case where a state is created early
 * (e.g., the "shell" state in luashMain) before the IOC registrars
 * have fired. When require() is called later, the preload table may
 * be missing libraries that were registered in the meantime.
 */
static int luaCheckRegistered(lua_State* state)
{
	epicsGuard<epicsMutex> guard(registryMutex);

	std::string libname(lua_tostring(state, 1));

	for (reg_iter index = registered_libs.begin(); index != registered_libs.end(); index++)
	{
		if (libname == std::string(index->first))
		{
			/* Cache in package.preload for future calls */
			luaL_getsubtable(state, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
			lua_pushcfunction(state, index->second);
			lua_setfield(state, -2, index->first);
			lua_pop(state, 1);

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
 * Called on every state created with luaCreateState. Adds registered
 * libraries to package.preload so they are found by require(), and
 * adds a fallback searcher for libraries registered after state
 * creation. Also registers all standalone functions as globals.
 */
epicsShareFunc void luaLoadRegistered(lua_State* state)
{
	epicsGuard<epicsMutex> guard(registryMutex);

	/* Populate package.preload with currently registered libs */
	luaL_getsubtable(state, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);

	for (reg_iter index = registered_libs.begin(); index != registered_libs.end(); index++)
	{
		lua_pushcfunction(state, index->second);
		lua_setfield(state, -2, index->first);
	}

	lua_pop(state, 1);

	/* Add a fallback searcher for libs registered after state creation */
	lua_getglobal(state, "package");
	lua_getfield(state, -1, "searchers");
	lua_len(state, -1);
	int num_searchers = lua_tonumber(state, -1);
	lua_pop(state, 1);
	lua_pushcfunction(state, luaCheckRegistered);
	lua_seti(state, -2, num_searchers + 1);
	lua_pop(state, 2);

	for (reg_iter index = registered_funcs.begin(); index != registered_funcs.end(); index++)
	{
		lua_register(state, index->first, index->second);
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

	if (size <= 0)    { rewind(temp_help); return false; }

	rewind(temp_help);
	char* buffer = (char*) malloc(sizeof(char) * (size + 1));

	fread(buffer, 1, size, temp_help);
	buffer[size] = '\0';
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


static int l_registerState(lua_State* state);

/*
 * info(library_or_object)
 *
 * Displays available functions/methods for a library table or
 * userdata object. Documentation is stored as a _doc field
 * (integer-indexed array of signature strings) on the table
 * itself or on the userdata's metatable.
 */
static int l_info(lua_State* L)
{
	if (lua_gettop(L) == 0)
	{
		printf("Usage:\n");
		printf("  info(library)  -- list functions in a library (e.g. info(epics))\n");
		printf("  info(object)   -- list methods/properties of an object\n");
		return 0;
	}

	if (lua_isnil(L, 1))
	{
		printf("Input is nil.\n");
		return 0;
	}

	/* Find the _doc table -- on the table itself or on its metatable */
	int found_doc = 0;

	if (lua_istable(L, 1))
	{
		lua_getfield(L, 1, "_doc");
		if (lua_istable(L, -1))    { found_doc = 1; }
		else                       { lua_pop(L, 1); }
	}

	if (!found_doc && lua_isuserdata(L, 1))
	{
		if (lua_getmetatable(L, 1))
		{
			lua_getfield(L, -1, "_doc");
			if (lua_istable(L, -1))    { found_doc = 1; }
			else                       { lua_pop(L, 1); }
			lua_remove(L, -2);
		}
	}

	if (found_doc)
	{
		int len = (int) luaL_len(L, -1);
		int i;
		for (i = 1; i <= len; i++)
		{
			lua_geti(L, -1, i);
			printf("  %s\n", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		return 0;
	}

	/* Fallback for tables without _doc -- list keys */
	if (lua_istable(L, 1))
	{
		lua_pushnil(L);
		while (lua_next(L, 1))
		{
			if (lua_type(L, -2) == LUA_TSTRING)
			{
				const char* key = lua_tostring(L, -2);
				if (key[0] != '_')
					printf("  .%s\n", key);
			}
			lua_pop(L, 1);
		}
		return 0;
	}

	printf("No documentation available.\n");
	return 0;
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

	/* Apply registered paths to package.path and package.cpath */
	{
		epicsGuard<epicsMutex> guard(registryMutex);
		rebuildPaths(output);
	}

	lua_register(output, "print", l_replaceprint);
	lua_register(output, "info", l_info);
	lua_register(output, "luaRegisterState", l_registerState);
	lua_register(output, "luaSpawn", l_luaSpawn);
	lua_register(output, "luash", l_luash);
	lua_register(output, "luaCmd", l_luaCmd);
	lua_register(output, "luaLoadFile", l_luaLoadFile);
	lua_register(output, "luaAddPath", l_luaAddPath);
	lua_register(output, "luaAddModule", l_luaAddModule);

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

	epicsGuard<epicsMutex> guard(namedStatesMutex);

	std::string state_name(name);

	if (named_states.find(state_name) != named_states.end())
	{
		return named_states[state_name];
	}

	lua_State* output = luaCreateState();

	named_states[state_name] = output;

	return output;
}


/*
 * Looks up a named lua_State without creating one.
 * Returns NULL if no state is registered under the given name.
 */
epicsShareFunc lua_State* luaFindNamedState(const char* name)
{
	if (! name) { return NULL; }

	epicsGuard<epicsMutex> guard(namedStatesMutex);

	std::string state_name(name);

	std::map<std::string, lua_State*>::iterator it = named_states.find(state_name);

	if (it != named_states.end()) { return it->second; }

	return NULL;
}


/*
 * Registers an existing lua_State under a name so that
 * it can be retrieved later with luaNamedState. This allows
 * luascript records to reference the calling state via
 * CODE = "@statename function()"
 */
epicsShareFunc void luaRegisterState(lua_State* state, const char* name)
{
	if (! state || ! name) { return; }

	epicsGuard<epicsMutex> guard(namedStatesMutex);

	named_states[std::string(name)] = state;
}

/*
 * Returns non-zero if the given lua_State is registered
 * in the named_states map (i.e., it should not be closed
 * by the caller).
 */
epicsShareFunc int luaStateIsRegistered(lua_State* state)
{
	if (! state) { return 0; }

	epicsGuard<epicsMutex> guard(namedStatesMutex);

	for (std::map<std::string, lua_State*>::iterator it = named_states.begin(); it != named_states.end(); it++)
	{
		if (it->second == state) { return 1; }
	}

	return 0;
}

/*
 * Lua-callable wrapper for luaRegisterState.
 * Registers the calling Lua state under the given name.
 *
 *   luaRegisterState("mydriver")
 */
static int l_registerState(lua_State* state)
{
	const char* name = luaL_checkstring(state, 1);

	luaRegisterState(state, name);

	return 0;
}


/*
 * =========================================================================
 * Path management: luaAddPath / luaAddModule
 * =========================================================================
 *
 * luaAddPath registers a directory so that:
 *   - require() finds .lua files via package.path
 *   - require() finds .so/.dll files via package.cpath
 *   - luaLocateFile finds scripts (luaLoadFile, @file, luaSpawn, etc.)
 *
 * luaAddModule is a convenience wrapper that constructs lib/<arch>/
 * and bin/<arch>/ paths from a module's top directory.
 *
 * Paths registered via luaAddPath are applied to:
 *   - The calling Lua state (when called from Lua)
 *   - All future states created via luaCreateState
 *
 * Duplicate paths are silently ignored.
 */

/*
 * Save the pristine package.path and package.cpath values into
 * the Lua registry. Called once per state, before any paths are
 * modified. Subsequent calls are no-ops.
 */
static void ensureDefaultPaths(lua_State* state)
{
	lua_getfield(state, LUA_REGISTRYINDEX, "_lua_default_path");

	if (lua_isnil(state, -1))
	{
		lua_pop(state, 1);

		lua_getglobal(state, "package");

		lua_getfield(state, -1, "path");
		lua_setfield(state, LUA_REGISTRYINDEX, "_lua_default_path");

		lua_getfield(state, -1, "cpath");
		lua_setfield(state, LUA_REGISTRYINDEX, "_lua_default_cpath");

		lua_pop(state, 1);  /* pop package */
	}
	else
	{
		lua_pop(state, 1);
	}
}

/*
 * Rebuild package.path and package.cpath on a state from the
 * full registered_paths list. Caller must hold registryMutex.
 */
static void rebuildPaths(lua_State* state)
{
	ensureDefaultPaths(state);

	/* Build prefix strings from registered_paths in order */
	std::string path_prefix;
	std::string cpath_prefix;

	for (size_t i = 0; i < registered_paths.size(); i++)
	{
		const std::string& dir = registered_paths[i];

		path_prefix += dir + "/?.lua;";
		path_prefix += dir + "/?/init.lua;";

		#ifdef _WIN32
		cpath_prefix += dir + "/?.dll;";
		#else
		cpath_prefix += dir + "/?.so;";
		#endif
	}

	/* Retrieve saved defaults */
	lua_getfield(state, LUA_REGISTRYINDEX, "_lua_default_path");
	const char* def_path = lua_tostring(state, -1);

	lua_getfield(state, LUA_REGISTRYINDEX, "_lua_default_cpath");
	const char* def_cpath = lua_tostring(state, -1);

	std::string new_path  = path_prefix  + (def_path  ? def_path  : "");
	std::string new_cpath = cpath_prefix + (def_cpath ? def_cpath : "");

	lua_pop(state, 2);

	/* Set the rebuilt paths */
	lua_getglobal(state, "package");

	lua_pushstring(state, new_path.c_str());
	lua_setfield(state, -2, "path");

	lua_pushstring(state, new_cpath.c_str());
	lua_setfield(state, -2, "cpath");

	lua_pop(state, 1);  /* pop package */
}


/*
 * Add a directory to the global path registry. The directory is
 * searched by require() (via package.path/cpath) and by luaLocateFile
 * (for luaLoadFile, @file, luaSpawn, etc.) on all states created
 * after this call.
 *
 * Duplicate directories are silently ignored.
 */
epicsShareFunc void luaAddPath(const char* directory)
{
	if (! directory || ! directory[0])    { return; }

	epicsGuard<epicsMutex> guard(registryMutex);

	/* Duplicate check */
	std::string dir(directory);

	for (size_t i = 0; i < registered_paths.size(); i++)
	{
		if (registered_paths[i] == dir)    { return; }
	}

	registered_paths.push_back(dir);
}


/*
 * Add an EPICS module's lib/<arch>/ and bin/<arch>/ directories
 * to the path registry. Reads EPICS_HOST_ARCH from the environment
 * to determine the architecture subdirectory.
 */
epicsShareFunc void luaAddModule(const char* module_top)
{
	if (! module_top || ! module_top[0])    { return; }

	const char* arch = std::getenv("EPICS_HOST_ARCH");

	if (! arch || ! arch[0])
	{
		errlogPrintf("luaAddModule: EPICS_HOST_ARCH not set\n");
		return;
	}

	std::string top(module_top);
	std::string archstr(arch);

	luaAddPath((top + "/lib/" + archstr).c_str());
	luaAddPath((top + "/bin/" + archstr).c_str());
}


/*
 * Lua-callable wrapper for luaAddPath.
 * Registers the path globally and applies it to the calling state.
 *
 *   luaAddPath("/path/to/libs")
 */
int l_luaAddPath(lua_State* state)
{
	const char* dir = luaL_checkstring(state, 1);

	{
		/* Global registration (also used by luaLocateFile) */
		luaAddPath(dir);

		/* Rebuild package.path/cpath on calling state */
		epicsGuard<epicsMutex> guard(registryMutex);
		rebuildPaths(state);
	}

	return 0;
}


/*
 * Lua-callable wrapper for luaAddModule.
 * Adds lib/<arch>/ and bin/<arch>/ for the given module top directory.
 *
 *   luaAddModule("/path/to/module")
 *   luaAddModule("../..")
 */
int l_luaAddModule(lua_State* state)
{
	const char* top = luaL_checkstring(state, 1);

	{
		/* Global registration */
		luaAddModule(top);

		/* Rebuild package.path/cpath on calling state */
		epicsGuard<epicsMutex> guard(registryMutex);
		rebuildPaths(state);
	}

	return 0;
}
