#ifndef GAME_SERVER_ENTITIES_LUA_ENTITY_H
#define GAME_SERVER_ENTITIES_LUA_ENTITY_H

#include <game/server/entity.h>

class CLuaEntity : public CEntity
{
public:
	CLuaEntity(CGameWorld *pGameWorld, const char *pLuaClass);

	void Reset();
	void Destroy();
	void Tick();
	void TickDefered();
	void TickPaused();
	void Snap(int SnappingClient);
};

#endif
