#ifndef ENGINE_SERVER_LUA_H
#define ENGINE_SERVER_LUA_H

#include <vector>
#include <string>
#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/lua.h>
#include <base/tl/array.h>
#include <map>

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
			lua_pop(L, 2); /* pop result and registry */ \
			if(NotFromLua) \
			{ \
				/* set from-lua marker */ \
				lua_getregistry(L); \
				lua_pushlightuserdata(L, &s_Sentinel); \
				lua_pushboolean(L, 1); \
				lua_rawset(L, -3); \
				lua_pop(L, 1); /* pop the registry table */ \
\
				/* prepare */ \
				setGlobal(L, Table, "self"); \
				setGlobal(L, this, "this"); \
				try { Func(__VA_ARGS__); } catch(luabridge::LuaException& e) { CLua::HandleException(e); } \
				Handled = true; \
\
				/* unset from-lua marker */ \
				lua_getregistry(L); \
				lua_pushlightuserdata(L, &s_Sentinel); \
				lua_pushnil(L); \
				lua_rawset(L, -3); \
				lua_pop(L, 1); /* pop the registry table */ \
			} \
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

private:
	class IStorage *m_pStorage;
	class IConsole *m_pConsole;
	class IServer *m_pServer;
	class CGameContext *m_pGameServer;

	IStorage *Storage() { return m_pStorage; }
	IConsole *Console() { return m_pConsole; }
	IServer *Server() { return m_pServer; }
	CGameContext *GameServer() { return m_pGameServer; }

	lua_State *m_pLuaState;
	void RegisterLuaCallbacks();
	bool RegisterScript(const char *pFullPath, const char *pObjName, bool Reloading = false);
	static int ListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user);
	bool LoadLuaFile(const char *pFilePath);

	struct LuaObject
	{
		LuaObject(const std::string& path, const std::string& name) : name(name), path(path) {}

		std::string name;
		std::string path;

		std::string GetIdent() const { return std::string(name + ":" + path); }
	};
	std::vector<LuaObject> m_lLuaObjects;

public:
	CLua();
	void Init();
	void OpenLua();
	bool Reload();
	bool LoadGametype();
	lua_State *L() { return m_pLuaState; }

	void ReloadSingleObject(int ObjectID);
	int NumLoadedClasses() const { return (int)m_lLuaObjects.size(); }
	std::string GetObjectIdentifier(int ID) const { return m_lLuaObjects[ID].GetIdent(); }

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
