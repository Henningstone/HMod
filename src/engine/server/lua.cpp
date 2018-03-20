#include <lua.hpp>

#include <engine/external/luabridge/LuaBridge.h>

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

	OpenLua();
}

void CLua::OpenLua()
{
	// basic lua initialization
	m_pLuaState = luaL_newstate();
	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	luaL_openlibs(m_pLuaState);

	RegisterLuaCallbacks();
}

bool CLua::Reload()
{
	lua_close(m_pLuaState);
	OpenLua();

	return LoadGametype();
}

bool CLua::RegisterScript(const char *pFullPath, const char *pObjName, bool Reloading)
{
	// make sure we don't have duplicate classes
	for(std::vector<LuaObject>::iterator it = m_lLuaObjects.begin(); it != m_lLuaObjects.end(); ++it)
	{
		if(it->name == pObjName && it->path != pFullPath)
		{
			dbg_msg("lua", "ERROR: duplicate declaraction of class '%s' in '%s' and '%s'", pObjName, pFullPath, it->path.c_str());
			return false;
		}
	}

	luabridge::setGlobal(m_pLuaState, luabridge::newTable(m_pLuaState), pObjName);

	if(!LoadLuaFile(pFullPath))
		return false;

	if(!Reloading)
	{
		m_lLuaObjects.push_back(LuaObject(pFullPath, pObjName));

		dbg_msg("lua/objectmgr", "loaded object '%s' from file '%s'", pObjName, pFullPath);
	}

	return true;
}

bool CLua::LoadLuaFile(const char *pFilePath)
{
	// check if the file exists and retrieve its full path
	char aFullPath[512];
	IOHANDLE f = Storage()->OpenFile(pFilePath, IOFLAG_READ, IStorage::TYPE_ALL, aFullPath, sizeof(aFullPath));
	if(!f)
		return false;

	io_close(f);

	// load the file
	dbg_msg("lua", "loading script '%s' for gametype %s", aFullPath, g_Config.m_SvGametype);
	int Status = luaL_dofile(m_pLuaState, aFullPath);
	if(Status != 0)
	{
		dbg_msg("lua", "FATAL: an error was thrown while loading file '%s', not starting!", aFullPath);
		ErrorFunc(m_pLuaState);
		return false;
	}

	return true;
}

int CLua::ListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user)
{
	if(name[0] == '.')
		return 0;

	if(str_length(name) <= 4)
		return 0;

	if(str_comp_filenames(name, "init.lua") == 0)
		return 0;

	CLua *pSelf = static_cast<CLua *>(user);
	if(is_dir)
		return fs_listdir_verbose(full_path, CLua::ListdirCallback, dir_type, user);
	else
	{
		char aObjName[64];
		str_copyb(aObjName, name);
		aObjName[str_length(name)-4] = '\0';
		return pSelf->RegisterScript(full_path, aObjName) ? 0 : 1;
	}
}

bool CLua::LoadGametype()
{
	char aDir[128];

	// load all the lua files
	str_formatb(aDir, "gamemodes/%s", g_Config.m_SvGametype);
	if(fs_listdir_verbose(aDir, ListdirCallback, 0, this) != 0)
		return false;

	// load the init file
	str_formatb(aDir, "gamemodes/%s/init.lua", g_Config.m_SvGametype);
	if(!LoadLuaFile(aDir))
	{
		Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "luaserver", "Error while loading init file of gametype %s, aborting!", g_Config.m_SvGametype);
		return false;
	}

	return true;
}

void CLua::ReloadSingleObject(int ObjectID)
{
	if(ObjectID < 0)
	{
		// reload all
		int Num = NumLoadedClasses();
		for(int i = 0; i < Num; i++)
		{
			RegisterScript(m_lLuaObjects[i].path.c_str(), m_lLuaObjects[i].name.c_str(), true);
			Console()->Printf(0, "luaserver", "reloading %s", m_lLuaObjects[i].GetIdent().c_str());
		}
	}
	else
	{
		RegisterScript(m_lLuaObjects[ObjectID].path.c_str(), m_lLuaObjects[ObjectID].name.c_str(), true);
		Console()->Printf(0, "luaserver", "reloading %s", m_lLuaObjects[ObjectID].GetIdent().c_str());
	}
}

// low level error handling (errors not thrown as an exception)
int CLua::ErrorFunc(lua_State *L)
{
	if (!lua_isstring(L, -1))
		CLua::ms_pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "lua/error", "unknown error");
	else
	{
		lua_Debug ar;
		lua_getstack(L, 0, &ar);
		lua_getinfo(L, "nSl", &ar);

		CLua::ms_pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "lua/error", "%s:%i: %s", ar.short_src, ar.currentline, lua_tostring(ms_pSelf->m_pLuaState, -1));
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

/// hook for __newindex on lua-classes
int CLua::LuaHook_NewIndex(lua_State *L)
{
	const char *pKey = lua_tostring(L, -2);

	// allow only static-type variables to be assigned to on the class
	if(pKey[0] != '_')
	{
		luaL_error(L, "Can not assign to field '%s': not a static member (hint: prefix field names with an underscore '_' to make them static-compliant)", pKey);
	}

	LuaRef Tbl = luabridge::LuaRef::fromStack(L, 1);
	LuaRef Key = luabridge::LuaRef::fromStack(L, 2);
	LuaRef Val = luabridge::LuaRef::fromStack(L, 3);

	dbg_msg("NEWINDEX HOOK", "%i elements in table; k = %s, v = %s (pKey: %s)", Tbl.length(), Key.tostring().c_str(), Val.tostring().c_str(), pKey);

	return 0;
}

int CLua::LuaHook_Index(lua_State *L)
{
	LuaRef Tbl = luabridge::LuaRef::fromStack(L, 1);
	LuaRef Val = luabridge::LuaRef::fromStack(L, 1);
	dbg_msg("INDEX HOOK", "%i elements in table; v = %s", Tbl.length(), Val.tostring().c_str());

	return 0;
}

int CLua::LuaHook_ToString(lua_State *L)
{
	if(lua_getmetatable(L, 1) == 0)
		return luaL_error(L, "interal error: failed to getmetatable @ LuaHook_ToString");

	lua_pushstring(L, LUACLASS_MT_TYPE);
	lua_rawget(L, -2);
	const char *pClassName = lua_tostring(L, -1);

	lua_pushstring(L, LUACLASS_MT_UID);
	lua_rawget(L, -2);
	const void *pUid = lua_touserdata(L, -1);

	lua_pushfstring(L, "%s[%p]", pClassName, pUid);
	return 1;
}

int CLua::RegisterMeta(lua_State *L)
{
	// only do something if we haven't done it yet
	if(lua_getmetatable(L, 1) != 0)
	{
		lua_pop(L, 1); // pop the metatable again
		return 0;
	}

	// create the new metatable for it
	lua_newtable(L); // t (-3)

	// assign newindex event
	lua_pushstring(L, "__newindex"); // k (-2)
	lua_pushcfunction(L, CLua::LuaHook_NewIndex); // v (-1)
	lua_rawset(L, -3); // pops 2 (k,v) - metatable and classtable remain

	// assign index event
	lua_pushstring(L, "__index");
	lua_pushcfunction(L, CLua::LuaHook_Index);
	lua_rawset(L, -3);

	// assign tostring event
	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, CLua::LuaHook_ToString);
	lua_rawset(L, -3);

	// hide metatables
	lua_pushstring(L, "__metatable");
	lua_pushboolean(L, true);
	lua_rawset(L, -3);

	// remember what type this lua class is of
	char aTypename[128];
	str_formatb(aTypename, "LC:%s", lua_tostring(L, 2));
	lua_pushstring(L, LUACLASS_MT_TYPE); // k (-2)
	lua_pushstring(L, aTypename); // v (-1)
	lua_rawset(L, -3); // pops 2 (k,v) - metatable and classtable still remain

	lua_setmetatable(L, -1); // our class table is on top of the stack (this pops the new metatable)

	lua_pop(L, 2); // the args

	return 0;
}

void CLua::HandleException(luabridge::LuaException& e)
{
	CLua::Lua()->Console()->Print(0, "lua/ERROR", e.what());
}

LuaRef CLua::CopyTable(const LuaRef& Src, LuaRef *pSeenTables)
{
	lua_State *L = Src.state();
	if(!Src.isTable())
		luaL_error(L, "given variable is not a table");

	LuaRef Copy = luabridge::newTable(L);
	LuaRef SeenTables = luabridge::newTable(L);
	if(!pSeenTables)
	{
		pSeenTables = &SeenTables;
		// we are just starting off, initialize with certain things we'll never want to copy
		SeenTables.append(Src);
		SeenTables.append(luabridge::getGlobal(L, "_G"));
	}

	for(luabridge::Iterator it(Src); !it.isNil(); ++it)
	{
		const LuaRef& val = it.value();

		// tables are tricky!
		if(val.isTable())
		{
			// check for self-referencing tables
			bool Found = false;
			for(luabridge::Iterator it2(*pSeenTables); !it2.isNil(); ++it2)
			{
				if(it2.value().rawequal(val))
				{
					Found = true;
					break;
				}
			}

			if(Found)
			{
				// we have already seen this reference, so don't copy the table behind it again. Just assign the reference.
				Copy[it.key()] = val; // think of this as a weak reference - we know there has already been another reference to this
				continue;
			}

			// haven't had this reference so far? copy what's behind it.
			LuaRef valCopy = CopyTable(val, pSeenTables);
			pSeenTables->append(val);
			pSeenTables->append(valCopy);
			Copy[it.key()] = valCopy;

		}
		else // non-table value
		{
			if(it.key().tostring().c_str()[0] == '_')
				/* ignore */; // emulate the behavior of static variables (they are not part of objects!)
			else
				Copy[it.key()] = val; // everything but a table can just be thrown in as is (actually userdata might make problems still, but let's not care.)
		}
	}

	return Copy;
}

void CLua::DbgPrintLuaTable(const LuaRef& Table, int Indent)
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
