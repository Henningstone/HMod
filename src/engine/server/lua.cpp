#include <string>
#include <engine/lua_include.h>

#include <base/system.h>

#include <engine/storage.h>
#include <engine/console.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <base/math.h>

#include <game/server/gamecontext.h>

#include "mapconverter.h"
#include "lua.h"
#include "luabinding.h"

CLua * CLua::ms_pSelf = NULL;


CLua::CLua()
{
	m_pLuaState = NULL;
	m_pMapConverter = NULL;
}

void CLua::FirstInit()
{
	CLua::ms_pSelf = this;
	CLuaBinding::StaticInit(this);
}

bool CLua::InitAndStartGametype()
{
	m_pStorage = Kernel()->RequestInterface<IStorage>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pServer = Kernel()->RequestInterface<IServer>();
	m_pGameServer = dynamic_cast<CGameContext *>(Kernel()->RequestInterface<IGameServer>());

	if(m_pMapConverter)
		delete m_pMapConverter;
	m_pMapConverter = new CMapConverter(Storage(), Kernel()->RequestInterface<IEngineMap>(), Console());

	return CleanLaunchLua();
}

void CLua::OpenLua()
{
	// basic lua initialization
	m_pLuaState = luaL_newstate();
	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	luaL_openlibs(m_pLuaState);

	InitializeLuaState();
	RegisterLuaCallbacks();
	InjectOverrides();
	ReseedRandomizer();
}

void CLua::InitializeLuaState()
{
	luabridge::LuaRef Package = luabridge::getGlobal(m_pLuaState, "package");
	if(!Package.isTable())
	{
		dbg_msg("lua/WARN", "failed to initialize module loader: 'package' is not a table");
		return;
	}

	luabridge::LuaRef Path = Package["path"];
	if(!Path.isString())
		Package["path"] = std::string("./?.lua");

	char aBuf[1024];
	// add some more locations to the package.path
	char aCompletePath[512];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, "lua", aCompletePath, sizeof(aCompletePath));
	str_formatb(aBuf, "%s;./lua/?.lua"
			";./lua/modules/?.lua"
			";./lua/modules/?/init.lua"
			";./gametypes/include/?.lua"
			";./gametypes/include/?/init.lua"
			";%s/?.lua"
			";%s/?/init.lua", Path.tostring().c_str(), aCompletePath, aCompletePath);

	Package["path"] = std::string(aBuf);
}

void CLua::InjectOverrides()
{
	// store the original io.open function somewhere secretly
	// we must store it IN lua, otherwise it won't work anymore :/
	lua_getregistry(m_pLuaState);                           // STACK: +1
	lua_getglobal(m_pLuaState, "io");                       // STACK: 2+
	lua_getfield(m_pLuaState, -1, "open");                  // STACK: 3+
	lua_setfield(m_pLuaState, -3, LUA_REGINDEX_IO_OPEN);    // STACK: 2-
	lua_pop(m_pLuaState, 2);                                // STACK: 0-

	// now override it with our own function
	luaL_dostring(m_pLuaState, "io.open = _io_open");
}

void CLua::ReseedRandomizer()
{
	// lazy but works
	char aLine[128];
	str_formatb(aLine, "math.randomseed(%i)", rand()); // our randomizer is already seeded
	luaL_dostring(m_pLuaState, aLine);
}

bool CLua::CleanLaunchLua()
{
	GetResMan()->FreeAll();
	if(m_pLuaState)
		lua_close(m_pLuaState);
	m_lLuaClasses.clear();
	OpenLua();

	return LoadGametype();
}

bool CLua::RegisterScript(const char *pFullPath, const char *pObjName, bool Reloading)
{
	// make sure we don't have duplicate classes
	for(std::vector<LuaClass>::iterator it = m_lLuaClasses.begin(); it != m_lLuaClasses.end(); ++it)
	{
		if(it->name == pObjName && it->path != pFullPath)
		{
			dbg_msg("lua", "ERROR: duplicate declaration of class '%s' in '%s' and '%s'", pObjName, pFullPath, it->path.c_str());
			return false;
		}
	}

	luabridge::setGlobal(m_pLuaState, luabridge::newTable(m_pLuaState), pObjName);

	if(!LoadLuaFile(pFullPath))
		return false;

	if(!Reloading)
	{
		m_lLuaClasses.emplace_back(pFullPath, pObjName);

		dbg_msg("lua/objectmgr", "loaded object class '%s' from file '%s'", pObjName, pFullPath);
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

	if(!is_dir && str_comp_nocase_num(name+str_length(name)-4, ".lua", 4) != 0)
	{
		dbg_msg("lua/loader", "WARN: ignoring non-lua file '%s'", name);
		return 0;
	}

	if(str_comp_filenames(name, "init.lua") == 0)
		return 0;

	CLua *pSelf = static_cast<CLua *>(user);
	if(is_dir && str_comp_nocase(full_path, "include") != 0)
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
			RegisterScript(m_lLuaClasses[i].path.c_str(), m_lLuaClasses[i].name.c_str(), true);
			Console()->Printf(0, "luaserver", "reloading %s", m_lLuaClasses[i].GetIdent().c_str());
		}
	}
	else
	{
		RegisterScript(m_lLuaClasses[ObjectID].path.c_str(), m_lLuaClasses[ObjectID].name.c_str(), true);
		Console()->Printf(0, "luaserver", "reloading %s", m_lLuaClasses[ObjectID].GetIdent().c_str());
	}
}

void CLua::OnMapLoaded()
{
	LuaRef CallbackFunc = LuaRef::getGlobal(m_pLuaState, "OnMapLoaded");
	if(CallbackFunc.isFunction())
	{
		try
		{
			CallbackFunc();
		} catch (luabridge::LuaException& e) {
			HandleException(e);
		}
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

int CLua::LuaHook_NewIndex(lua_State *L)
{
	/// args: table, key, value

	const char *pKey = lua_tostring(L, 2);
	if(pKey && pKey[0] == '_')
		return luaL_error(L, "cannot create static attribute '%s' on an object", pKey);

	lua_rawset(L, 1);
	return 0;
}

int CLua::LuaHook_Index(lua_State *L)
{
	/// args: table, key

	const char *pKey = lua_tostring(L, 2);

	lua_rawget(L, 1);
	return 1;
}

int CLua::LuaHook_ToString(lua_State *L)
{
	if(lua_getmetatable(L, 1) == 0)
		return luaL_error(L, "interal error: failed to getmetatable @ LuaHook_ToString");

	lua_pushstring(L, LUACLASS_MT_TYPE);
	lua_rawget(L, -2);
	const char *pClassName = lua_tostring(L, -1);

	lua_pushfstring(L, "%s", pClassName);
	return 1;
}

int CLua::RegisterMeta(lua_State *L)
{
	/// args: object table, identifier string

	// only do something if we haven't done it yet
	if(lua_getmetatable(L, 1) != 0)
	{
		lua_pop(L, 1); // pop the metatable again
		return 0;
	}

	// create the new metatable for it
	lua_newtable(L); // t (-3)

	// assign tostring event
	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, CLua::LuaHook_ToString);
	lua_rawset(L, -3);

	// assign newindex event
	lua_pushstring(L, "__newindex");
	lua_pushcfunction(L, CLua::LuaHook_NewIndex);
	lua_rawset(L, -3);

	// assign index event
/*	lua_pushstring(L, "__index");
	lua_pushcfunction(L, CLua::LuaHook_Index);
	lua_rawset(L, -3);*/

	// create object storage
/*	lua_pushstring(L, LUACLASS_MT_OBJS);
	lua_newtable(L);
	lua_rawset(L, -3);*/

	// hide metatables
	#if defined(CONF_RELEASE)
	lua_pushstring(L, "__metatable");
	lua_pushboolean(L, true);
	lua_rawset(L, -3);
	#endif

	// remember what type this lua class is of
	char aTypename[128];
	str_formatb(aTypename, "LC:%s", lua_tostring(L, 2));
	lua_pushstring(L, LUACLASS_MT_TYPE); // k (-2)
	lua_pushstring(L, aTypename); // v (-1)
	lua_rawset(L, -3); // pops 2 (k,v) - metatable and object-table still remain

	lua_setmetatable(L, 1); // our class table is on top of the stack (this pops the new metatable)

	lua_pop(L, 2); // pop the args

	return 0;
}

luabridge::LuaRef CLua::GetSelfTable(lua_State *L, const CLuaClass *pLC)
{
	using namespace luabridge;

	char aSelfVarName[64];
	str_format(aSelfVarName, sizeof(aSelfVarName), "__xData%p", pLC);
	LuaRef Self = getGlobal(L, aSelfVarName);
	if(!Self.isTable())
	{
		// create an "object" from the lua-class
		const char *pClassName = pLC->GetLuaClassName();
		LuaRef ClassTable = getGlobal(L, pClassName);
		dbg_assert_legacy(ClassTable.isTable(), "Trying to get self-table of class that doesn't exist?!");
		Self = CLua::CopyTable(ClassTable);
		Self["__dbgId"] = LuaRef(L, std::string(aSelfVarName));
		Self["__dbgLC"] = LuaRef(L, pLC);
		setGlobal(L, Self, aSelfVarName);

		// register metatables for this object
		lua_pushcfunction(L, CLua::RegisterMeta);
		Self.push(L);
		lua_pushfstring(L, "%s->%p", pClassName, pLC);
		lua_call(L, 2, 0);

		CLua::Lua()->m_NumLuaObjects++;
	}

	return Self;
}

void CLua::FreeSelfTable(lua_State *L, const CLuaClass *pLC)
{
	char aSelfVarName[64];
	str_format(aSelfVarName, sizeof(aSelfVarName), "__xData%p", pLC);
	lua_pushnil(L);
	lua_setglobal(L, aSelfVarName);

	CLua::Lua()->m_NumLuaObjects--;
}

void CLua::HandleException(luabridge::LuaException& e)
{
	// pop the error message if there is any
	if(lua_gettop(CLua::Lua()->L()) > 0)
		lua_pop(CLua::Lua()->L(), 1);

	static int s_SameErrorCount = 0;
	static int64 s_LastErrorTime = 0;
	static unsigned int s_LastErrorHash = 0;
	unsigned int ThisErrorHash = str_quickhash(e.what());

	// don't spam
	if(ThisErrorHash == s_LastErrorHash)
	{
		s_SameErrorCount++;
		if(s_SameErrorCount >= 4)
		{
			// throttle
			if(s_SameErrorCount == 4)
				CLua::Lua()->Console()->Print(0, "lua/ERROR", "  (throttling further errors of this kind)");

			int64 Now = time_get();
			if(Now < s_LastErrorTime + time_freq())
				return;
			else
				s_LastErrorTime = Now;
		}
	}
	else
		s_SameErrorCount = 0;
	s_LastErrorHash = ThisErrorHash;

	CLua::DbgPrintLuaStack(CLua::Lua()->L(), "asdasdasd");
	// print the error so that you won't miss it in all the console output mess
	dbg_msg("XXX", "XXX");
	CLua::Lua()->Console()->Print(0, "lua/ERROR", e.what());
	dbg_msg("XXX", "XXX");
}

LuaRef CLua::CopyTable(const LuaRef& Src, LuaRef *pSeenTables)
{
	lua_State *L = Src.state();
	if(!Src.isTable())
		luaL_error(L, "given variable is not a table @ CopyTable (got type: %s)", lua_typename(L, Src.type()));

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
