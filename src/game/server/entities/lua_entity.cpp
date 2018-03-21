#include "lua_entity.h"

CLuaEntity::CLuaEntity(CGameWorld *pGameWorld, const char *pLuaClass)
		: CEntity(pGameWorld, CGameWorld::ENTTYPE_CUSTOM, pLuaClass)
{

}

void CLuaEntity::Reset()
{
	MACRO_LUA_EVENT()
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

LuaRef CLuaEntity::GetSelf(lua_State *L)
{
	char aSelfVarName[32]; \
	str_format(aSelfVarName, sizeof(aSelfVarName), "__xData%p", this);
	LuaRef Self = luabridge::getGlobal(L, aSelfVarName);
	if(!Self.isTable())
	{
		LuaRef Table = luabridge::getGlobal(CLua::Lua()->L(), GetLuaClassName());
		Self = CLua::CopyTable(Table);
	}
	return Self;
}
