#include "lua_entity.h"

CLuaEntity::CLuaEntity(CGameWorld *pGameWorld, const char *pLuaClass)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_CUSTOM, pLuaClass)
{
}

void CLuaEntity::Reset()
{
	MACRO_LUA_EVENT()
}

void CLuaEntity::Destroy()
{
	MACRO_LUA_CALLBACK(__func__)

	CEntity::Destroy();
}

void CLuaEntity::Tick()
{
	MACRO_LUA_EVENT()
}

void CLuaEntity::TickDefered()
{
	MACRO_LUA_EVENT()
}

void CLuaEntity::TickPaused()
{
	MACRO_LUA_EVENT()
}

void CLuaEntity::Snap(int SnappingClient)
{
	MACRO_LUA_EVENT(SnappingClient)
}

void CLuaEntity::OnInsert()
{
	MACRO_LUA_EVENT()
}
