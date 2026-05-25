/*
 * LPeg library registration for EPICS Lua module.
 *
 * Registers the LPeg pattern matching library (luaopen_lpeg)
 * so it is available via require("lpeg") in all Lua states.
 *
 * LPeg is Copyright (c) 2007-2023 Lua.org, PUC-Rio.
 * See luaApp/src/lpeg/LICENSE for the full license text.
 */

#include <epicsExport.h>
#include "luaEpics.h"

extern "C" {
	int luaopen_lpeg(lua_State* L);
}

static int luaopen_lpeg_wrapper(lua_State* L)
{
	int result = luaopen_lpeg(L);

	/* Add _doc table for info(lpeg) */
	lua_newtable(L);
	lua_pushstring(L, ".P(value) -- literal string, char count, or grammar"); lua_rawseti(L, -2, 1);
	lua_pushstring(L, ".R(range) -- character range, e.g. R('az')"); lua_rawseti(L, -2, 2);
	lua_pushstring(L, ".S(string) -- character set, e.g. S('+-*/')"); lua_rawseti(L, -2, 3);
	lua_pushstring(L, ".V(name) -- grammar non-terminal (variable)"); lua_rawseti(L, -2, 4);
	lua_pushstring(L, ".B(patt) -- match behind current position"); lua_rawseti(L, -2, 5);
	lua_pushstring(L, ".utfR(cp1, cp2) -- UTF-8 code point range"); lua_rawseti(L, -2, 6);
	lua_pushstring(L, ".locale([table]) -- character class patterns (alpha, digit, etc.)"); lua_rawseti(L, -2, 7);
	lua_pushstring(L, ".C(patt) -- simple capture"); lua_rawseti(L, -2, 8);
	lua_pushstring(L, ".Carg(n) -- argument capture"); lua_rawseti(L, -2, 9);
	lua_pushstring(L, ".Cb(key) -- back capture"); lua_rawseti(L, -2, 10);
	lua_pushstring(L, ".Cc(values) -- constant capture"); lua_rawseti(L, -2, 11);
	lua_pushstring(L, ".Cg(patt [, key]) -- group capture"); lua_rawseti(L, -2, 12);
	lua_pushstring(L, ".Cp() -- position capture"); lua_rawseti(L, -2, 13);
	lua_pushstring(L, ".Cs(patt) -- substitution capture"); lua_rawseti(L, -2, 14);
	lua_pushstring(L, ".Ct(patt) -- table capture"); lua_rawseti(L, -2, 15);
	lua_pushstring(L, ".Cmt(patt, func) -- match-time capture"); lua_rawseti(L, -2, 16);
	lua_pushstring(L, ".match(patt, subject [, init]) -- match pattern against string"); lua_rawseti(L, -2, 17);
	lua_pushstring(L, ".type(value) -- returns 'pattern' if value is a pattern"); lua_rawseti(L, -2, 18);
	lua_pushstring(L, ".version -- LPeg version string"); lua_rawseti(L, -2, 19);
	lua_pushstring(L, ".setmaxstack(max) -- set backtrack stack limit (default 400)"); lua_rawseti(L, -2, 20);
	lua_pushstring(L, "Operators: patt1 * patt2 (seq), patt1 + patt2 (choice), patt1 - patt2 (diff)"); lua_rawseti(L, -2, 21);
	lua_pushstring(L, "           -patt (not), #patt (and), patt^n (rep), patt / val (capture)"); lua_rawseti(L, -2, 22);
	lua_setfield(L, -2, "_doc");

	return result;
}

static void liblpegRegister(void)
{
	luaRegisterLibrary("lpeg", luaopen_lpeg_wrapper);
}

extern "C"
{
	epicsExportRegistrar(liblpegRegister);
}
