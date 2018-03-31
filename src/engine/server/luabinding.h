#ifndef ENGINE_SERVER_LUABINDING_H
#define ENGINE_SERVER_LUABINDING_H

#include <lua.hpp>

#define argcheck(cond, narg, expected) \
		if(!(cond)) \
		{ \
			if(g_Config.m_Debug) \
				dbg_msg("Lua/debug", "%s: argcheck: narg=%i expected='%s'", __FUNCTION__, narg, expected); \
			char buf[64]; \
			str_format(buf, sizeof(buf), "expected a %s value, got %s", expected, lua_typename(L, lua_type(L, narg))); \
			return luaL_argerror(L, (narg), (buf)); \
		}


#define LUA_REGINDEX_IO_OPEN "TeeModF:io.open"

class CLuaBinding
{
	static class CLua *ms_pLua;

public:
	static void StaticInit(CLua *pLua) { ms_pLua = pLua; }

	static int LuaListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user);

	static int ScriptName(lua_State *L);
	static int Print(lua_State *L);
	static int Throw(lua_State *L);

	static int LuaListdir(lua_State *L);

	// io override
	static int LuaIO_Open(lua_State *L);

	// helper functions
};

#endif
