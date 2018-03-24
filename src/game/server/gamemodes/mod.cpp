/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include "mod.h"

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	// Exchange this to a string that identifies your game mode.
	// DM, TDM and CTF are reserved for teeworlds original modes.
	//m_pGameType = "MOD";

	str_copyb(m_aPublicGametype, g_Config.m_SvGametype);
	if(str_comp_nocase(m_aPublicGametype, "dm") == 0 ||
	   str_comp_nocase(m_aPublicGametype, "tdm") == 0 ||
	   str_comp_nocase(m_aPublicGametype, "ctf") == 0)
		str_append(m_aPublicGametype, "-mod", sizeof(m_aPublicGametype));

	m_pGameType = m_aPublicGametype;

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}

void CGameControllerMOD::Tick()
{
	// this is the main part of the gamemode, this function is run every tick

	IGameController::Tick();
}
