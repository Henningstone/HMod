#ifndef HMOD_LUA_CLASS_H
#define HMOD_LUA_CLASS_H

#include <base/system.h>
#include <string>
#include <lua.hpp>


class CLuaClass
{
	std::string m_LuaClass;
	volatile int m_IntegrityCheck;

protected:
	CLuaClass(const char *pClassName)
	{
		m_LuaClass = std::string(pClassName);
		m_IntegrityCheck = 0x539;
	}

	virtual ~CLuaClass(){}

	inline const char *GetLuaClassName() const { dbg_assert_legacy(m_IntegrityCheck == 0x539, "bad mem"); return m_LuaClass.c_str(); }

public:
	inline void LuaBindClass(const char *pClassName) { m_LuaClass = std::string(pClassName); }

	LuaRef GetSelf(lua_State *L)
	{
		char aSelfVarName[64]; \
		str_format(aSelfVarName, sizeof(aSelfVarName), "__xData%p", this);
		LuaRef Self = luabridge::getGlobal(L, aSelfVarName);
		if(!Self.isTable())
		{
			const char *pClassName = GetLuaClassName();
			LuaRef ClassTable = luabridge::getGlobal(CLua::Lua()->L(), pClassName);
			Self = CLua::CopyTable(ClassTable);
			Self["__dbgId"] = LuaRef(L, std::string(aSelfVarName)); \
			setGlobal(L, Self, aSelfVarName); \
		}
		return Self;
	}




	/*
	 * don't mind this function here, but also don't remove it!
	 * It is an essential crash fix... yea don't even ask, I have seriously
	 * NO idea why _not_ having this will cause memory corruption, but it does.
	 *
	 * If I was forced to make a guess, I'd say that luabridge needs this class (CLuaClass)
	 * to be run-time polymorphic (what we archive by having a virtual method in it).
	 */
	virtual void fuckyou()
	{
	}

};

#endif
