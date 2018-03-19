#ifndef ENGINE_CLIENT_LUARESMANDEF_H
#define ENGINE_CLIENT_LUARESMANDEF_H
#undef ENGINE_CLIENT_LUARESMANDEF_H // this file is included multiple times

#ifndef REGISTER_RESSOURCE
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) ;;
	#if !defined(DO_NOT_COMPILE_THIS_CODE)
	#error included luaresmandef.h without defining REGISTER_RESSOURCE
	#endif
#endif

REGISTER_RESSOURCE(int, Texture,
				   pKernel->RequestInterface<IGraphics>()->UnloadTexture(ELEM);
)



#endif
