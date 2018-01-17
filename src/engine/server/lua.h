#ifndef ENGINE_SERVER_LUA_H
#define ENGINE_SERVER_LUA_H

#include <lua.h>
#include <engine/lua.h>

class CLua : public ILua
{
	friend class CLuaBinding;

	class IStorage *m_pStorage;
	class IConsole *m_pConsole;
	class IServer *m_pServer;
	class IGameServer *m_pGameServer;

	IStorage *Storage() { return m_pStorage; }
	IConsole *Console() { return m_pConsole; }
	IServer *Server() { return m_pServer; }
	IGameServer *GameServer() { return m_pGameServer; }

	lua_State *m_pLuaState;
	void RegisterLuaCallbacks();

public:
	CLua();
	void Init();
	bool LoadGametype();

	static int ErrorFunc(lua_State *L);
	static int Panic(lua_State *L);

private:
	static CLua *ms_pSelf;
};

#endif
