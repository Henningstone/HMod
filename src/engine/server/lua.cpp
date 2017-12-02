#include <lua.hpp>

#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/lua_modules/luafilesystem/lfs.h>

#include <base/system.h>

#include <engine/storage.h>
#include <engine/console.h>
#include <engine/server.h>
#include <engine/shared/config.h>

#include "lua.h"
#include "luabinding.h"

CLua * CLua::ms_Self = NULL;


CLua::CLua()
{
	m_pLuaState = NULL;
}

void CLua::Init()
{
	CLua::ms_Self = this;
	CLuaBinding::StaticInit(this);

	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pGameServer = Kernel()->RequestInterface<IGameServer>();

	// basic lua initialization
	m_pLuaState = luaL_newstate();
	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	luaL_openlibs(m_pLuaState);
	luaopen_lfs(m_pLuaState);

	RegisterLuaCallbacks();
}

bool CLua::LoadGametype()
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "lua/%s/module.lua", g_Config.m_SvGametype);

	// check if the file exists and retrieve its full path
	char aFullPath[512];
	IOHANDLE f = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL, aFullPath, sizeof(aFullPath));
	if(!f)
	{
		Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "luaserver", "Failed to load gametype '%s': lua init file '%s' does not exist!", g_Config.m_SvGametype, aBuf);
		return false;
	}
	io_close(f);

	// load the init file, it will handle the rest
	luaL_dofile(m_pLuaState, aFullPath);

	return true;
}

// low level error handling (errors not thrown as an exception)
int CLua::ErrorFunc(lua_State *L)
{
	if (!lua_isstring(L, -1))
		CLua::ms_Self->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "lua/error", "unknown error");
	else
	{
		CLua::ms_Self->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "lua/error", "%s", lua_tostring(ms_Self->m_pLuaState, -1));
		lua_pop(L, 1); // remove error message
	}

	return 0;
}

// for errors in unprotected calls
int CLua::Panic(lua_State *L)
{
	if(g_Config.m_Debug)
		CLua::ms_Self->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "lua/fatal", "error in unprotected call resulted in panic, throwing an exception:");
	throw luabridge::LuaException(L, 0);
}

ILua *CreateLua() { return new CLua; }
