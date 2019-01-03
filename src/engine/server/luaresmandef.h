#ifndef ENGINE_CLIENT_LUARESMANDEF_H
#define ENGINE_CLIENT_LUARESMANDEF_H
#undef ENGINE_CLIENT_LUARESMANDEF_H // this file is included multiple times

// this is to make IDE's inspection work
#ifndef REGISTER_RESSOURCE
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) ;;
	#if !defined(DO_NOT_COMPILE_THIS_CODE)
	#error included luaresmandef.h without defining REGISTER_RESSOURCE
	#endif
#endif


// the actual definitions

REGISTER_RESSOURCE(CLuaSqlite *, LuaSqlite,
				   delete ELEM;
)

REGISTER_RESSOURCE(std::string, ConsoleCommand,
				   CLua::Lua()->Console()->DeregisterTemp(ELEM.c_str());
)

#endif
