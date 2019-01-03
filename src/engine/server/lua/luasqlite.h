#ifndef ENGINE_CLIENT_LUA_LUASQL_H
#define ENGINE_CLIENT_LUA_LUASQL_H

#include <engine/lua_include.h>
#include <engine/shared/db_sqlite3.h>

using luabridge::LuaRef;

// passed to lua, handles the db (essentially a wrapper for CSql)
class CLuaSqlite
{
	CSql *m_pDb;
	char m_aPath[512];

public:
	CLuaSqlite(const char *pFilename)
	{
		m_pDb = new CSql(pFilename, false);
		str_copyb(m_aPath, pFilename);
	}

	~CLuaSqlite();
	void Execute(const char *pStatement, LuaRef Callback, lua_State *L);
	unsigned int Work() { return m_pDb->Work(); }
	void Flush() { m_pDb->Flush(); }
	void Clear() { m_pDb->Clear(); }

	const char *GetDatabasePath() const { return m_aPath; }
};

// invisible to lua
class CLuaSqlQuery : public CQuery
{
	const luabridge::LuaRef m_CbFunc;
	const bool m_GotCb;

public:
	CLuaSqlQuery(char *pQueryBuf, const LuaRef& CbFunc)
			: CQuery(pQueryBuf),
			  m_CbFunc(CbFunc),
			  m_GotCb(CbFunc.isFunction())
	{
	}

private:
	void OnData();
};


// global namespace

class CLuaSql
{
public:
	static CLuaSqlite *Open(const char *pFilename, lua_State *L);
	static void Flush(CLuaSqlite& Db) { Db.Flush(); }
	static void Clear(CLuaSqlite& Db) { Db.Clear(); }
};

#endif
