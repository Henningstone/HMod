#ifndef ENGINE_LUA_H
#define ENGINE_LUA_H

#include "kernel.h"

class ILua : public IInterface
{
	MACRO_INTERFACE("lua", 0)
public:
	virtual void Init() = 0;
	virtual bool LoadGametype() = 0;
};

extern ILua *CreateLua();

#endif
