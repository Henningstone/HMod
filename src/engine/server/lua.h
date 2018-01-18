#ifndef ENGINE_SERVER_LUA_H
#define ENGINE_SERVER_LUA_H

#include <engine/lua.h>
#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <base/tl/array.h>

#define MACRO_LUA_EVENT(CLASSNAME, ...) \
{ \
	bool Handled = false; \
\
	using namespace luabridge; \
	LuaRef Table = getGlobal(CLua::Lua()->L(), CLASSNAME); \
	if(Table.isTable()) \
	{ \
		LuaRef Func = Table[__func__]; \
		if(Func.isFunction()) \
		{ \
			lua_State *L = Func.state(); \
\
			/* make sure we don't end up in infinite recursion */ \
			static int s_Sentinel = 0; \
			lua_getregistry(L); \
			lua_rawgetp(L, -1, &s_Sentinel); \
			bool NotFromLua = lua_isnil(L, -1); \
			lua_pop(L, 1); /* pop the result */ \
			if(NotFromLua) \
			{ \
				/* set from-lua marker */ \
				lua_pushlightuserdata(L, &s_Sentinel); \
				lua_pushboolean(L, 1); \
				lua_rawset(L, -3); \
\
				/* prepare */ \
				setGlobal(L, Table, "self"); \
				setGlobal(L, this, "this"); \
				try { Func(__VA_ARGS__); } catch(luabridge::LuaException& e) { CLua::HandleException(e); } \
				Handled = true; \
\
				/* unset from-lua marker */ \
				lua_pushlightuserdata(L, &s_Sentinel); \
				lua_pushnil(L); \
				lua_rawset(L, -3); \
			} \
			lua_pop(L, 1); /* pop the registry table */ \
		} \
	} \
\
	if(Handled) \
		return; \
}


using luabridge::LuaRef;


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
	bool RegisterScript(const char *pScriptClass, bool Reloading = false);

	array<const char*> m_lpLoadedClasses;

	bool LoadLuaFile(const char *pClassName);

public:
	CLua();
	void Init();
	bool LoadGametype();
	lua_State *L() { return m_pLuaState; }

	void ReloadClass(int i);
	int NumLoadedClasses() const { return m_lpLoadedClasses.size(); }
	const char *GetClassName(int i) const { return m_lpLoadedClasses[i]; }

	static void HandleException(luabridge::LuaException& e);
	static int ErrorFunc(lua_State *L);
	static int Panic(lua_State *L);

private:
	static CLua *ms_pSelf;

	static void DbgPrintLuaTable(LuaRef Table, int Indent = 1);

public:
	static CLua *Lua() { return ms_pSelf; }
	static void DbgPrintLuaStack(lua_State *L, const char *pNote, bool Deep = false);
};

#endif
