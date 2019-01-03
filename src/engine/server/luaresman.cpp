#include <string>
#include "engine/server/lua/luasqlite.h"
#include <engine/console.h>

#include "luaresman.h"


void CLuaRessourceMgr::FreeAll()
{
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) \
			while(!m_##VARNAME.empty()) \
			{ \
				unsigned SizeBefore = (unsigned)m_##VARNAME.size(); \
				TYPE ELEM = m_##VARNAME.front(); \
				DELETION; \
				dbg_assert_strict((unsigned)m_##VARNAME.size() < SizeBefore, "deletion function of " #TYPE " " #VARNAME " did not deregister the object"); \
			} \
			m_##VARNAME.clear();
	#include "luaresmandef.h"
	#undef REGISTER_RESSOURCE
}

int CLuaRessourceMgr::GetCount() const
{
	int Count = 0;

	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) \
			Count += (int)m_##VARNAME.size();

	#include "luaresmandef.h"
	#undef REGISTER_RESSOURCE

	return Count;
}
