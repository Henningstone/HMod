#ifndef ENGINE_SERVER_CONFIG_H
#define ENGINE_SERVER_CONFIG_H

#include <engine/shared/config.h>


struct CConfigProperties
{
#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) \
		static std::string GetConfig_##Name() { if(!((Save)&CFGFLAG_SERVER)) throw "invalid config type (this is not a server variable)"; return g_Config.m_##Name; } \

#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) \
		static int GetConfig_##Name() { if(!((Save)&CFGFLAG_SERVER)) throw "invalid config type (this is not a server variable)"; return g_Config.m_##Name; } \

#include <engine/shared/config_variables.h>

#undef MACRO_CONFIG_STR
#undef MACRO_CONFIG_INT
};

#endif
