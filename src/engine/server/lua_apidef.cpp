#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>
#include "luabinding.h"
#include "lua.h"

using namespace luabridge;

void CLua::RegisterLuaCallbacks()
{
	lua_State *L = m_pLuaState;

	lua_register(L, "print", CLuaBinding::Print);
	lua_register(L, "throw", CLuaBinding::Throw); // adds an exception, but doesn't jump out like 'error' does
	lua_register(L, "script_name", CLuaBinding::ScriptName);

	getGlobalNamespace(L)
		.beginClass<IGameController>("IGameController")
		.endClass()

		.beginClass<CGameContext>("CGameContext")
		.endClass()

		.beginClass<CCharacter>("CCharacter")
		.endClass()

	; // end global namespace
}
