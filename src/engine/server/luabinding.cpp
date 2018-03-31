#include <lua.hpp>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/storage.h>
#include <base/math.h>

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

int CLuaBinding::LuaListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user)
{
	lua_State *L = (lua_State*)user;
	lua_pushvalue(L, lua_gettop(L)); // duplicate the callback function which is on top of the stack
	lua_pushstring(L, name); // push arg 1 (element name)
	lua_pushstring(L, full_path); // push arg 2 (element path)
	lua_pushboolean(L, is_dir); // push arg 3 (bool indicating whether element is a folder)
	lua_call(L, 3, 1); // no pcall so that errors get propagated upwards
	int ret = 0;
	if(lua_isnumber(L, -1))
		ret = round_to_int((float)lua_tonumber(L, -1));
	lua_pop(L, 1); // pop return
	return ret;
}

/* lua call: Listdir(<string> foldername, <string/function> callback) */
int CLuaBinding::LuaListdir(lua_State *L)
{
	int nargs = lua_gettop(L);
	if(nargs != 2)
		return luaL_error(L, "Listdir expects 2 arguments");

	argcheck(lua_isstring(L, 1), 1, "string"); // path
	argcheck(lua_isstring(L, 2) || lua_isfunction(L, 2), 2, "string (function name) or function"); // callback function

	// convert the function name into the actual function
	if(lua_isstring(L, 2))
	{
		lua_getglobal(L, lua_tostring(L, 2)); // check if the given callback function actually exists / retrieve the function
		argcheck(lua_isfunction(L, -1), 2, "function name (does the given function exist?)");
	}

	char aSandboxedPath[512];
	str_copyb(aSandboxedPath, lua_tostring(L, 1)); // arg1
	const char *pDir = CLua::Lua()->Storage()->SandboxPathMod(aSandboxedPath, sizeof(aSandboxedPath), g_Config.m_SvGametype, true);
	lua_Number ret = (lua_Number)fs_listdir_verbose(pDir, LuaListdirCallback, IStorage::TYPE_SAVE, L);
	lua_pushnumber(L, ret);
	return 1;
}

int CLuaBinding::LuaIO_Open(lua_State *L)
{
	int nargs = lua_gettop(L);
	if(nargs < 1 || nargs > 3)
		return luaL_error(L, "io.open expects between 1 and 3 arguments");

	argcheck(lua_isstring(L, 1), 1, "string"); // path
	if(nargs >= 2)
		argcheck(lua_isstring(L, 2), 2, "string"); // mode
	if(nargs == 3)
		argcheck(lua_isboolean(L, 3), 3, "bool"); // 'shared' flag

	const char *pFilename = lua_tostring(L, 1);
	const char *pOpenMode = luaL_optstring(L, 2, "r");
	bool Shared = false;
	if(nargs == 3)
	{
		if(lua_toboolean(L, 3))
			Shared = true;
	}

	char aFilename[512];
	str_copyb(aFilename, pFilename);
	pFilename = CLua::Lua()->Storage()->SandboxPathMod(aFilename, sizeof(aFilename), Shared ? "_shared" : g_Config.m_SvGametype);

	char aFullPath[512];
	CLua::Lua()->Storage()->GetCompletePath(IStorage::TYPE_SAVE, pFilename, aFullPath, sizeof(aFullPath));
	if(str_find_nocase(pOpenMode, "w"))
	{
		if(fs_makedir_rec_for(aFullPath) != 0)
			return luaL_error(L, "Failed to create path for file '%s'", aFullPath);
	}

	// now we make a call to the builtin function 'io.open'
	lua_pop(L, nargs); // pop our args

	// receive the original io.open that we have saved in the registry
	lua_getregistry(L);
	lua_getfield(L, -1, LUA_REGINDEX_IO_OPEN);

	// push the arguments
	lua_pushstring(L, aFullPath); // he needs the full path
	lua_pushstring(L, pOpenMode);

	// fire it off
	// this pops 3 things (function + args) and pushes 1 (resulting file handle)
	lua_call(L, 2, 1);

	// push an additional argument for the scripter
	lua_pushstring(L, pFilename);
	return 2; // we'll leave cleaning the stack up to lua
}
