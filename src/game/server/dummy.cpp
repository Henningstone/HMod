#include "player.h"

#include "dummy.h"


CPlayerDummy::CPlayerDummy(CPlayer *pPlayer) : m_pPlayer(pPlayer)
{
}







CGameContext *CPlayerDummy::GameServer()
{
	return m_pPlayer->GameServer();
}
