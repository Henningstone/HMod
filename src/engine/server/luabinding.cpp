#include <lua.hpp>
#include <engine/shared/config.h>
#include <base/system.h>

#include "luabinding.h"
#include "lua.h"

CLua * CLuaBinding::ms_pLua = NULL;


int CLuaBinding::ScriptName(lua_State *L)
{
	// receive the current script file name
	lua_Debug ar;
	lua_getinfo(L, "S", &ar);
	lua_pushstring(L, ar.source);
	lua_pushstring(L, ar.short_src);

	return 2;
}

int CLuaBinding::Print(lua_State *L)
{
	int nargs = lua_gettop(L);
	if(nargs < 1)
		return luaL_error(L, "print expects 1 argument or more");

	// construct the message from all arguments
	char aMsg[512] = {0};
	for(int i = 1; i <= nargs; i++)
	{
		argcheck(lua_isstring(L, i) || lua_isnumber(L, i), i, "string or number");
		str_append(aMsg, lua_tostring(L, i), sizeof(aMsg));
		str_append(aMsg, "\t", sizeof(aMsg));
	}
	aMsg[str_length(aMsg)-1] = '\0'; // remove the last tab character

	// pop all to clean up the stack
	lua_pop(L, nargs);

	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "Sl", &ar);
	char aSource[128];
	str_format(aSource, sizeof(aSource), "lua:%s:%i", ar.short_src, ar.currentline);
	dbg_msg(aSource, "%s", aMsg);

	return 0;
}

int CLuaBinding::Throw(lua_State *L)
{
	int nargs = lua_gettop(L);
	if(nargs != 1)
		return luaL_error(L, "throw expects exactly 1 argument");

	argcheck(lua_isstring(L, nargs) || lua_isnumber(L, nargs), nargs, "string or number");

	// receive the current line
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "Sl", &ar);

	// add the exception
	char aMsg[512];
	str_format(aMsg, sizeof(aMsg), "%s:%i: Custom Exception: %s", ar.short_src, ar.currentline, lua_tostring(L, nargs));
	lua_pushstring(L, aMsg);
	int result = CLua::ErrorFunc(L);

	// pop argument
	lua_pop(L, nargs);

	return result;
}


/**
 * opens all given libraries
 */
void luaX_openlibs(lua_State *L, const luaL_Reg *lualibs)
{
	/* (following code copied from https://www.lua.org/source/5.1/linit.c.html)
	 * This is the only correct way to load standard libs!
	 * For everything non-standard (custom) use luaL_register.
	 */
	const luaL_Reg *lib = lualibs;
	for (; lib->func; lib++) {
		lua_pushcfunction(L, lib->func);
		lua_pushstring(L, lib->name);
		lua_call(L, 1, 0);
	}
}

/**
 * opens a single library
 */
void luaX_openlib(lua_State *L, const char *name, lua_CFunction func)
{
	const luaL_Reg single_lib[] = {
			{name, func},
			{NULL, NULL}
	};
	luaX_openlibs(L, single_lib);
}

/**
 * Publishes the module with given name in the global scope under the same name
 */
void luaX_register_module(lua_State *L, const char *name)
{
	/* adapted from https://www.lua.org/source/5.1/loadlib.c.html function ll_require */
	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
	lua_getfield(L, -1, name);
	if(!dbg_assert_strict(lua_toboolean(L, -1), "trying to register a lib that has not been opened yet!")) {  /* is it there? */
		// got whatever 'require' would return on top of the stack now
		lua_setglobal(L, name);
	}
}
