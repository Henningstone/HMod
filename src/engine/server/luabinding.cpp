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
	int result = CLua::m_pClient->Lua()->HandleException(aMsg, pLF);

	// pop argument
	lua_pop(L, nargs);

	return 0;
}
