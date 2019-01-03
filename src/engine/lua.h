#ifndef ENGINE_LUA_H
#define ENGINE_LUA_H

#include "kernel.h"

class ILua : public IInterface
{
	MACRO_INTERFACE("lua", 0)
public:
	virtual void FirstInit() = 0;
	virtual void OnMapLoaded() = 0;
};

extern ILua *CreateLua();

#endif
