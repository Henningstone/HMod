#include <lua.hpp>

#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/lua_modules/luafilesystem/lfs.h>

#include <base/system.h>

#include <engine/storage.h>
#include <engine/console.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <base/math.h>

#include <game/server/gamecontext.h>

#include "lua.h"
#include "luabinding.h"

CLua * CLua::ms_pSelf = NULL;


CLua::CLua()
{
	m_pLuaState = NULL;
}

void CLua::Init()
{
	CLua::ms_pSelf = this;
	CLuaBinding::StaticInit(this);

	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pGameServer = dynamic_cast<CGameContext *>(Kernel()->RequestInterface<IGameServer>());

	// basic lua initialization
	m_pLuaState = luaL_newstate();
	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	luaL_openlibs(m_pLuaState);
	luaopen_lfs(m_pLuaState);

	RegisterLuaCallbacks();
}

bool CLua::RegisterScript(const char *pObjName, bool Reloading, const char *pRegisterAs)
{
	if(!pRegisterAs)
		pRegisterAs = pObjName;
	luabridge::setGlobal(m_pLuaState, luabridge::newTable(m_pLuaState), pRegisterAs);

	if(!LoadLuaFile(pObjName))
		return false;

	if(!Reloading)
	{
		m_lLoadedClasses[std::string(pRegisterAs)].push_back(LuaObject((int)m_lpAllObjects.size(), std::string(pRegisterAs), std::string(pObjName)));

	/*	TLoadedClassesMap::iterator it;
		for(it = m_lLoadedClasses.begin(); it != m_lLoadedClasses.end(); ++it)
		{
			if(it->first == RegisterAs)
				break;
		}

		// insert new key if not found
		if(it == m_lLoadedClasses.end())
		{
			m_lLoadedClasses.emplace_back(std::make_pair(RegisterAs, std::vector<std::string>()));
			m_lLoadedClasses.back().second.emplace_back(std::string(pObjName));
		}
		else
		{
			it->second.emplace_back(std::string(pObjName));
		}*/

		dbg_msg("lua/objectmgr", "object '%s' registered as instance of class '%s'", pObjName, pRegisterAs);
	}

	return true;
}

bool CLua::LoadLuaFile(const char *pClassName)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "gamemodes/%s/%s.lua", g_Config.m_SvGametype, pClassName);

	// check if the file exists and retrieve its full path
	char aFullPath[512];
	IOHANDLE f = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL, aFullPath, sizeof(aFullPath));
	if(!f)
		return false;

	io_close(f);

	// load the init file, it will handle the rest
	dbg_msg("luasrv", "loading script '%s' for gametype %s", aFullPath, g_Config.m_SvGametype);
	int Status = luaL_dofile(m_pLuaState, aFullPath);
	if(Status != 0)
	{
		dbg_msg("FATAL", "an error was thrown while loading file '%s', not starting!", aFullPath);
		ErrorFunc(m_pLuaState);
		return false;
	}

	return true;
}

bool CLua::LoadGametype()
{
	if(!LoadLuaFile("init"))
	{
		Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "luaserver", "Error while loading init file of gametype %s, aborting!", g_Config.m_SvGametype);
		return false;
	}

	// auto-add standard classes
	RegisterScript("entities/Character",  false, "Character");
	RegisterScript("entities/Projectile", false, "Projectile");
	RegisterScript("entities/Laser",      false, "Laser");
	RegisterScript("entities/Pickup",     false, "Pickup");
	RegisterScript("entities/Flag",       false, "Flag");

	return true;
}

void CLua::ReloadSingleObject(int ObjectID)
{
	if(ObjectID < 0)
	{
		// reload all
		int Num = NumLoadedObjects();
		for(int i = 0; i < Num; i++)
			RegisterScript(m_lpAllObjects[i]->objname.c_str(), true, m_lpAllObjects[i]->classname.c_str());
	}
	else
		RegisterScript(m_lpAllObjects[ObjectID]->objname.c_str(), true, m_lpAllObjects[ObjectID]->classname.c_str());
}

bool CLua::AddClass(const char *pClassPath, const char *pRegisterAs)
{
	return RegisterScript(pClassPath, false, pRegisterAs);
}

const CLua::LuaObject *CLua::FindObject(const std::string& ObjName)
{
	for(std::vector<LuaObject*>::iterator it = m_lpAllObjects.begin(); it != m_lpAllObjects.end(); ++it)
	{
		if((*it)->objname == ObjName)
			return (*it);
	}
	return NULL;
}

// low level error handling (errors not thrown as an exception)
int CLua::ErrorFunc(lua_State *L)
{
	if (!lua_isstring(L, -1))
		CLua::ms_pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "lua/error", "unknown error");
	else
	{
		CLua::ms_pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "lua/error", "%s", lua_tostring(ms_pSelf->m_pLuaState, -1));
		lua_pop(L, 1); // remove error message
	}

	return 0;
}

// for errors in unprotected calls
int CLua::Panic(lua_State *L)
{
	//if(g_Config.m_Debug)
		CLua::ms_pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "lua/fatal", "error in unprotected call resulted in panic, throwing an exception");
	throw luabridge::LuaException(L, 0);
	return 0;
}

void CLua::HandleException(luabridge::LuaException& e)
{
	CLua::Lua()->Console()->Print(0, "lua/ERROR", e.what());
}

void CLua::DbgPrintLuaTable(LuaRef Table, int Indent)
{
	if(Indent > 40 || !Table.isTable())
		return;

	char aIndent[41];
	mem_set(aIndent, ' ', sizeof(aIndent));
	Indent = min(40, Indent*2);
	aIndent[Indent] = '\0';

	for(luabridge::Iterator it(Table); !it.isNil(); ++it)
	{
		const char *pKeyName = it.key().tostring().c_str();
		const LuaRef& Value = it.value();
		char aSpaces[21] = "               ";
		aSpaces[max(0, 20-str_length(pKeyName))] = '\0';
		dbg_msg("lua/debug", "%s> %s%s= %s", aIndent, pKeyName, aSpaces, Value.tostring().c_str());
		// print subtables
		if(Value.isTable())
			DbgPrintLuaTable(it.value(), Indent+1);
	}
}

void CLua::DbgPrintLuaStack(lua_State *L, const char *pNote, bool Deep)
{
	dbg_msg("lua/debug", "--- BEGIN LUA STACK --- %s", pNote ? pNote : "");
	for(int i = 1; i <= lua_gettop(L); i++)
	{
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		const char *pVarName = lua_getlocal(L, &ar, i);
		if(pVarName)
			lua_pop(L, 1);

		dbg_msg("lua/debug", "#%02i    %s    %s    %s", i, luaL_typename(L, i),
				lua_isstring(L, i) || lua_isnumber(L, i) ? lua_tostring(L, i) :
				lua_isboolean(L, i) ? (lua_toboolean(L, i) ? "true" : "false") :
				"?",
				pVarName
		);

		if(Deep && lua_istable(L, i))
			DbgPrintLuaTable(luabridge::Stack<LuaRef>::get(L, i));
	}
	dbg_msg("lua/debug", "---- END LUA STACK ---- %s", pNote ? pNote : "");
}

ILua *CreateLua() { return new CLua; }
