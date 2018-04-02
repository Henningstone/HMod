#ifndef GAME_SERVER_DUMMY_H
#define GAME_SERVER_DUMMY_H

//#include ""

class CPlayerDummy
{
	class CPlayer *m_pPlayer;

public:
	CPlayerDummy(CPlayer *pPlayer);

private:
	class CGameContext *GameServer();

};

#endif
