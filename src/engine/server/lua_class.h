#ifndef HMOD_LUA_CLASS_H
#define HMOD_LUA_CLASS_H

#include <string>
#include <lua.hpp>


class CLuaClass
{
	std::string m_LuaClass;

protected:
	CLuaClass(const char *pClassName)
	{
		m_LuaClass = std::string(pClassName);
	}

	inline const char *GetLuaClassName() const { return m_LuaClass.c_str(); }

public:
	inline void LuaBindClass(const char *pClassName) { m_LuaClass = std::string(pClassName); }

	LuaRef GetSelf(lua_State *L)
	{
		char aSelfVarName[32]; \
		str_format(aSelfVarName, sizeof(aSelfVarName), "__xData%p", this);
		LuaRef Self = luabridge::getGlobal(L, aSelfVarName);
		if(!Self.isTable())
		{
			LuaRef Table = luabridge::getGlobal(CLua::Lua()->L(), GetLuaClassName());
			Self = CLua::CopyTable(Table);
		}
		return Self;
	}
};

#endif
