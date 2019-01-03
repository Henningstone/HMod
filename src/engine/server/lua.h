#ifndef ENGINE_SERVER_LUA_H
#define ENGINE_SERVER_LUA_H

#include <map>
#include <vector>
#include <string>
#include <engine/lua_include.h>

#include <base/tl/array.h>
#include <engine/lua.h>
#include <engine/server/luaresman.h>


/** USEFUL MACROS TO INVOKE LUA
 *
 * Explanation of the suffixes:
 *  - '_V': will define a `bool _LUA_EVENT_HANDLED' in the current scope and set it's value depending on whether invoking lua was possible
 *  //- '_C': lets you define the name of the lua class in the macro arguments list
 *  - Where these suffixes are combined, their meanings are as well.
 *  - No suffix means that the lua class name is automatically determined, the variable _LUA_EVENT_HANDLED will not be defined.
 */

#define MACRO_LUA_CALLBACK_RESULT_V(FUNCNAME, RESOP, ...) \
bool _LUA_EVENT_HANDLED = false; \
{ \
\
	using namespace luabridge; \
	LuaRef ClassTable = getGlobal(CLua::Lua()->L(), GetLuaClassName()); \
	if(ClassTable.isTable()) \
	{ \
		lua_State *L = ClassTable.state(); \
		LuaRef Func = ClassTable[FUNCNAME]; \
		if(Func.isFunction()) \
		{ \
			/*lua_State *L = Func.state();*/ \
\
			/* make sure we don't end up in infinite recursion */ \
			lua_getregistry(L); \
			lua_rawgetp(L, -1, this); \
			bool FromLua = lua_isnumber(L, -1) && lua_tonumber(L, -1) == __LINE__; \
			lua_pop(L, 2); /* pop result and registry */ \
			if(!FromLua) \
			{ \
				/* set from-lua marker */ \
				lua_getregistry(L); \
				lua_pushlightuserdata(L, this); \
				lua_pushnumber(L, __LINE__); \
				lua_rawset(L, -3); \
				lua_pop(L, 1); /* pop the registry table */ \
\
				/* prepare */ \
				LuaRef Self = CLua::GetSelfTable(L, this); \
\
				/* store the execution environment */ \
				LuaRef PrevSelf = getGlobal(L, "self"); \
				LuaRef PrevThis = getGlobal(L, "this"); \
				setGlobal(L, Self, "self"); \
				setGlobal(L, this, "this"); \
				try { RESOP Func(__VA_ARGS__); } catch(LuaException& e) { CLua::HandleException(e); } \
				/* restore previous environment */ \
				setGlobal(L, PrevSelf, "self"); \
				setGlobal(L, PrevThis, "this"); \
				_LUA_EVENT_HANDLED = true; \
\
				/* unset from-lua marker */ \
				lua_getregistry(L); \
				lua_pushlightuserdata(L, this); \
				lua_pushnil(L); \
				lua_rawset(L, -3); \
				lua_pop(L, 1); /* pop the registry table */ \
			} \
		} \
	} \
\
}

/**
 * MACRO_LUA_CALLBACK_RESULT
 *
 * Invoke a lua callback and use RESOP to store its result.
 */
//#define MACRO_LUA_CALLBACK_RESULT_C(CLASSNAME, FUNCNAME, RESOP, ...) { MACRO_LUA_CALLBACK_RESULT_CV(CLASSNAME, FUNCNAME, RESOP, __VA_ARGS__) }
#define MACRO_LUA_CALLBACK_RESULT(FUNCNAME, RESOP, ...) { MACRO_LUA_CALLBACK_RESULT_V(FUNCNAME, RESOP, __VA_ARGS__) }
#define MACRO_LUA_CALLBACK_RESULT_REF(FUNCNAME, RESVAR, ...) luabridge::LuaRef RESVAR = luabridge::LuaRef(CLua::Lua()->L()); { MACRO_LUA_CALLBACK_RESULT_V(FUNCNAME, RESVAR=, __VA_ARGS__) }
#define MACRO_LUA_CALLBACK_RESULT_V_REF(FUNCNAME, RESVAR, ...) luabridge::LuaRef RESVAR = luabridge::LuaRef(CLua::Lua()->L()); MACRO_LUA_CALLBACK_RESULT_V(FUNCNAME, RESVAR=, __VA_ARGS__)

/**
 * MACRO_LUA_CALLBACK
 *
 * Invoke a lua function contained in a lua class, if it exists.
 */
//#define MACRO_LUA_CALLBACK_CV(CLASSNAME, FUNCNAME, ...) MACRO_LUA_CALLBACK_RESULT_CV(CLASSNAME, FUNCNAME, /* ignored */, __VA_ARGS__)
//#define MACRO_LUA_CALLBACK_C(CLASSNAME, FUNCNAME, ...) MACRO_LUA_CALLBACK_RESULT_C(CLASSNAME, FUNCNAME, /* ignored */, __VA_ARGS__)
#define MACRO_LUA_CALLBACK_V(FUNCNAME, ...) MACRO_LUA_CALLBACK_RESULT_V(FUNCNAME, /* RESOP ignored */, __VA_ARGS__)
#define MACRO_LUA_CALLBACK(FUNCNAME, ...) MACRO_LUA_CALLBACK_RESULT(FUNCNAME, /* RESOP ignored */, __VA_ARGS__)

/**
 * MACRO_LUA_EVENT
 *
 * If the Lua Class contains a function with the same name as the current function,
 * the current function will be replaced with the lua function.
 * The lua function can still use this:<FuncName>() to invoke the original method.
 *
 * Note: Events will automatically return from the current function if the event was handles, '_V' versions don't make sense.
 */
//#define MACRO_LUA_EVENT_C(CLASSNAME, ...) { MACRO_LUA_CALLBACK_CV(CLASSNAME, __func__, __VA_ARGS__) if(_LUA_EVENT_HANDLED) return; }
#define MACRO_LUA_EVENT(...) { MACRO_LUA_CALLBACK_V(__func__, __VA_ARGS__) if(_LUA_EVENT_HANDLED) return; }

#define MACRO_LUA_RESULT_BOOL(VAR, TRUTH, DEFAULT) ((VAR).isBoolean() == !(DEFAULT) || ((VAR).isBoolean() && (VAR).cast<bool>() == (TRUTH)))


using luabridge::LuaRef;

#define LUACLASS_MT_TYPE "_type"
//#define LUACLASS_MT_OBJS "_objects"


class CLua : public ILua
{
	friend class CLuaBinding;
	friend class CLuaRessourceMgr;
	friend class CLuaSql;

public:
	enum
	{
		OBJ_ID_EVERYTHING = -1
	};

private:
	class IStorage *m_pStorage;
	class IConsole *m_pConsole;
	class IServer *m_pServer;
	class CGameContext *m_pGameServer;
	class CMapConverter *m_pMapConverter;

	IStorage *Storage() { return m_pStorage; }
	IConsole *Console() { return m_pConsole; }
	IServer *Server() { return m_pServer; }
	CGameContext *GameServer() { return m_pGameServer; }
	CMapConverter *MapConverter() { return m_pMapConverter; }

	lua_State *m_pLuaState;

	bool CleanLaunchLua();

	void OpenLua();
	void InitializeLuaState();
	void RegisterLuaCallbacks();
	void InjectOverrides();
	void ReseedRandomizer();

	bool LoadGametype();
	bool RegisterScript(const char *pFullPath, const char *pObjName, bool Reloading = false);
	static int ListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user);
	bool LoadLuaFile(const char *pFilePath);

	struct LuaClass
	{
		LuaClass(const std::string& path, const std::string& name) : name(name), path(path) {}

		std::string name;
		std::string path;

		std::string GetIdent() const { return std::string(name + ":" + path); }
	};
	std::vector<LuaClass> m_lLuaClasses;

	CLuaRessourceMgr m_ResMan;

	// for debugging
	int m_NumLuaObjects;

public:
	CLua();
	lua_State *L() { return m_pLuaState; }
	CLuaRessourceMgr *GetResMan() { return &m_ResMan; }

	void FirstInit();
	bool InitAndStartGametype();
	void ReloadSingleObject(int ObjectID);
	void OnMapLoaded();

	int NumLoadedClasses() const { return (int)m_lLuaClasses.size(); }
	std::string GetObjectIdentifier(int ID) const { return m_lLuaClasses[ID].GetIdent(); }
	const char *GetObjectName(int ID) const { return m_lLuaClasses[ID].name.c_str(); }

	static luabridge::LuaRef GetSelfTable(lua_State *L, const class CLuaClass *pLC);
	static void FreeSelfTable(lua_State *L, const class CLuaClass *pLC);
	int NumLuaObjects() const { return m_NumLuaObjects; }

	static void HandleException(luabridge::LuaException& e);
	static int ErrorFunc(lua_State *L);
	static int Panic(lua_State *L);

	static int LuaHook_NewIndex(lua_State *L);
	static int LuaHook_Index(lua_State *L);
	static int LuaHook_ToString(lua_State *L);

	/**
	 * This function is meant to be invoked after registering a lua class
	 * Must be called as a C-function via the lua enviroment
	 * - Expected arguments:
	 *     - first: the object table
	 *     - second: a string telling the class name or object identifier
	 * - pops both arguments off the stack
	 * - does not return anything on the lua stack
	 */
	static int RegisterMeta(lua_State *L);

	/**
	 * Deep-copy a table (with self-reference detection)
	 * @param Src Table to copy
	 * @param pSeenTables used internally; ignore it.
	 * @return A complete deep-copy of the given table
	 */
	static LuaRef CopyTable(const LuaRef& Src, LuaRef *pSeenTables = NULL);

private:
	static CLua *ms_pSelf;

	static void DbgPrintLuaTable(const LuaRef& Table, int Indent = 1);

public:
	static CLua *Lua() { return ms_pSelf; }
	static void DbgPrintLuaStack(lua_State *L, const char *pNote, bool Deep = false);
};

#endif
