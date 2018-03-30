#include <lua.hpp>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

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

const char *CLuaBinding::SandboxPath(char *pInOutBuffer, unsigned BufferSize, lua_State *L, bool MakeFullPath)
{
	const char *pSubdir = g_Config.m_SvGametype;

	char aFullPath[512];
	str_formatb(aFullPath, "mods_storage/%s/%s", pSubdir, CLua::Lua()->Storage()->SandboxPath(pInOutBuffer, BufferSize));
	if(MakeFullPath)
		CLua::Lua()->Storage()->MakeFullPath(aFullPath, sizeof(aFullPath), IStorage::TYPE_SAVE);
	str_copy(pInOutBuffer, aFullPath, BufferSize);

	return pInOutBuffer;
}
