#include "devUtil.h"

#include <string>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include "link.h"

#include <errlog.h>
#include <epicsExport.h>

#include "luaEpics.h"

/*
 * Quote/paren-aware scanning helpers for INP/OUT parsing.
 *
 * The INP/OUT string has the form:
 *   "filename function(param1, param2, ...) [portname]"
 * where a param may itself contain spaces, commas, or parentheses if
 * quoted ("..."/'...') or nested. The old parser split on the first
 * and last space and the first/last parenthesis, which mis-parsed any
 * quoted space (read as the portname delimiter) or quoted/nested
 * parenthesis. These helpers scan while tracking quote state and
 * paren/bracket/brace depth so structure inside quotes/parens is
 * preserved.
 */
namespace {

/* Advance past one character, updating quote state. Returns the index
 * just after the (possibly escaped) character. */
static size_t scanChar(const std::string& s, size_t i, char& quote)
{
	char c = s[i];

	if (quote)
	{
		if (c == '\\' && i + 1 < s.size())    { return i + 2; }  /* escaped */
		if (c == quote)                       { quote = '\0'; }
		return i + 1;
	}

	if (c == '"' || c == '\'')    { quote = c; }

	return i + 1;
}

/* Find the first top-level (not quoted, depth 0) whitespace character,
 * or npos. */
static size_t findTopLevelSpace(const std::string& s, size_t start)
{
	char quote = '\0';
	int depth = 0;

	for (size_t i = start; i < s.size(); )
	{
		char c = s[i];

		if (!quote)
		{
			if (c == '(' || c == '[' || c == '{')    { depth += 1; }
			else if (c == ')' || c == ']' || c == '}') { if (depth > 0) depth -= 1; }
			else if (depth == 0 && isspace((unsigned char) c)) { return i; }
		}

		i = scanChar(s, i, quote);
	}

	return std::string::npos;
}

/* Find the matching top-level '(' and its matching ')', quote/nesting
 * aware. Sets open/close to their indices; returns true if a balanced
 * top-level (...) was found. */
static bool findParens(const std::string& s, size_t start, size_t& open, size_t& close)
{
	char quote = '\0';
	int depth = 0;
	open = std::string::npos;
	close = std::string::npos;

	for (size_t i = start; i < s.size(); )
	{
		char c = s[i];

		if (!quote)
		{
			if (c == '(')
			{
				if (depth == 0 && open == std::string::npos)    { open = i; }
				depth += 1;
			}
			else if (c == ')')
			{
				depth -= 1;
				if (depth == 0 && open != std::string::npos)    { close = i; return true; }
			}
		}

		i = scanChar(s, i, quote);
	}

	return false;
}

/* Trim leading/trailing ASCII whitespace. */
static std::string trim(const std::string& s)
{
	size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos)    { return std::string(); }
	size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}

}  /* anonymous namespace */

extern "C"
{
	Protocol* parseINPOUT(const struct link* inpout)
	{
		Protocol* output = new Protocol();
		
		std::string code(inpout->value.instio.string);
		
		if (code.empty())
		{
			errlogPrintf("Error parsing INP string, format is '@filename function [portname]'\n");
			delete output;
			return NULL;
		}
		
		/* filename: up to the first top-level whitespace. */
		size_t fn_end = findTopLevelSpace(code, 0);
		std::string filename = (fn_end == std::string::npos) ? code : code.substr(0, fn_end);
		
		strncpy(output->filename, filename.c_str(), sizeof(output->filename) - 1);
		output->filename[sizeof(output->filename) - 1] = '\0';
		
		/* The remainder is "function(params) [portname]". */
		std::string rest = (fn_end == std::string::npos) ? std::string() : trim(code.substr(fn_end + 1));
		
		std::string function;   /* "name(params)" portion */
		
		/* Locate the top-level (...) that delimits the parameter list. */
		size_t open = std::string::npos, close = std::string::npos;
		bool has_parens = findParens(rest, 0, open, close);
		
		/* Detect an unbalanced single paren (has one but not a matched
		 * pair) to preserve the previous error behavior. */
		bool any_open  = rest.find('(') != std::string::npos;
		bool any_close = rest.find(')') != std::string::npos;
		
		if ((any_open || any_close) && !has_parens)
		{
			errlogPrintf("Error parsing function parameters, format is 'function_name(param1,param2,...)'\n");
			delete output;
			return NULL;
		}
		
		if (has_parens)
		{
			/* function name is everything up to the '('; params are
			 * inside; portname is the top-level token after ')'. */
			strncpy(output->function_name, trim(rest.substr(0, open)).c_str(), sizeof(output->function_name) - 1);
			output->function_name[sizeof(output->function_name) - 1] = '\0';
			
			std::string params = rest.substr(open + 1, close - open - 1);
			strncpy(output->param_list, params.c_str(), sizeof(output->param_list) - 1);
			output->param_list[sizeof(output->param_list) - 1] = '\0';
			
			std::string after = trim(rest.substr(close + 1));
			if (!after.empty())
			{
				/* portname is the first top-level token after ')'. */
				size_t sp = findTopLevelSpace(after, 0);
				std::string port = (sp == std::string::npos) ? after : after.substr(0, sp);
				strncpy(output->portname, port.c_str(), sizeof(output->portname) - 1);
				output->portname[sizeof(output->portname) - 1] = '\0';
			}
		}
		else
		{
			/* No parameter list. "function [portname]". */
			size_t sp = findTopLevelSpace(rest, 0);
			std::string fname = (sp == std::string::npos) ? rest : rest.substr(0, sp);
			strncpy(output->function_name, fname.c_str(), sizeof(output->function_name) - 1);
			output->function_name[sizeof(output->function_name) - 1] = '\0';
			
			if (sp != std::string::npos)
			{
				std::string port = trim(rest.substr(sp + 1));
				strncpy(output->portname, port.c_str(), sizeof(output->portname) - 1);
				output->portname[sizeof(output->portname) - 1] = '\0';
			}
		}
		
		/*
		 * If the filename doesn't resolve to a file on disk, treat it
		 * as a named state (same convention as luascriptRecord's CODE
		 * field). This allows DTYP "lua" records to share a Lua state
		 * bound with luaNameState().
		 *
		 * INP/OUT format:
		 *   "@script.lua function(params) [portname]"  -- file-based
		 *   "@statename function(params)"              -- named state
		 */
		std::string located = luaLocateFile(std::string(output->filename));
		
		if (located.empty())
		{
			/* Not a file -- try as a named state */
			lua_State* named = luaFindState(output->filename);
			
			if (named)
			{
				output->state = named;
				return output;
			}
		}
		
		/* File found (or no named state match) -- create a new state and load */
		output->state = luaCreateState();
		
		if (luaLoadScript(output->state, output->filename))
		{
			errlogPrintf("Error loading file: %s\n", output->filename);
			luaStateUnref(output->state);
			delete output;
			return NULL;
		}
		
		lua_pushstring(output->state, output->portname);
		lua_setglobal(output->state, "PORT");
		
		return output;
	}
	
	int runFunction(Protocol* proto)
	{
		int params = luaLoadParams(proto->state, proto->param_list);
		
		int status = lua_pcall(proto->state, params + 1, 1, 0);
		
		if (status)
		{
			std::string err(lua_tostring(proto->state, -1));
			lua_pop(proto->state, 1);
			
			errlogPrintf("Calling %s in %s resulted in error: %s\n", proto->function_name, proto->filename, err.c_str());
		}
		
		return status;
	}
}
