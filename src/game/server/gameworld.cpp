/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/shared/config.h>
#include <algorithm>
#include "gameworld.h"
#include "entity.h"
#include "gamecontext.h"

//////////////////////////////////////////////////
// game world
//////////////////////////////////////////////////
CGameWorld::CGameWorld()
{
	m_pGameServer = 0x0;
	m_pServer = 0x0;

	m_Paused = false;
	m_ResetRequested = false;
	for(int i = 0; i < NUM_ENTTYPES; i++)
		m_apFirstEntityTypes[i] = 0;
}

CGameWorld::~CGameWorld()
{
	// delete all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		while(m_apFirstEntityTypes[i])
			delete m_apFirstEntityTypes[i];
}

void CGameWorld::SetGameServer(CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
}

CEntity *CGameWorld::FindFirst(int Type)
{
	return Type < 0 || Type >= NUM_ENTTYPES ? 0 : m_apFirstEntityTypes[Type];
}

int CGameWorld::FindEntities(vec2 Pos, float Radius, CEntity **ppEnts, int Max, int Type)
{
	if(Type < 0 || Type >= NUM_ENTTYPES)
		return 0;

	int Num = 0;
	for(CEntity *pEnt = m_apFirstEntityTypes[Type];	pEnt; pEnt = pEnt->m_pNextTypeEntity)
	{
		if(distance(pEnt->m_Pos, Pos) < Radius+pEnt->m_ProximityRadius)
		{
			if(ppEnts)
				ppEnts[Num] = pEnt;
			Num++;
			if(Num == Max)
				break;
		}
	}

	return Num;
}

void CGameWorld::InsertEntity(CEntity *pEnt)
{
#ifdef CONF_DEBUG
	for(CEntity *pCur = m_apFirstEntityTypes[pEnt->m_ObjType]; pCur; pCur = pCur->m_pNextTypeEntity)
	{
		if(pCur == pEnt) dbg_msg("world/error", "pEnt = %p, pEnt->m_ObjType = %i", pEnt, pEnt->m_ObjType);
		dbg_assert(pCur != pEnt, "entity inserted twice");
	}
#endif

	// insert it
	if(m_apFirstEntityTypes[pEnt->m_ObjType])
		m_apFirstEntityTypes[pEnt->m_ObjType]->m_pPrevTypeEntity = pEnt;
	pEnt->m_pNextTypeEntity = m_apFirstEntityTypes[pEnt->m_ObjType];
	pEnt->m_pPrevTypeEntity = 0x0;
	m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt;

	pEnt->OnInsert();
}

void CGameWorld::DestroyEntity(CEntity *pEnt)
{
	pEnt->m_MarkedForDestroy = true;
}

void CGameWorld::RemoveEntity(CEntity *pEnt)
{
	// not in the list
	if(!pEnt->m_pNextTypeEntity && !pEnt->m_pPrevTypeEntity && m_apFirstEntityTypes[pEnt->m_ObjType] != pEnt)
		return;

	// remove
	if(pEnt->m_pPrevTypeEntity)
		pEnt->m_pPrevTypeEntity->m_pNextTypeEntity = pEnt->m_pNextTypeEntity;
	else
		m_apFirstEntityTypes[pEnt->m_ObjType] = pEnt->m_pNextTypeEntity;
	if(pEnt->m_pNextTypeEntity)
		pEnt->m_pNextTypeEntity->m_pPrevTypeEntity = pEnt->m_pPrevTypeEntity;

	// keep list traversing valid
	if(m_pNextTraverseEntity == pEnt)
		m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;

	pEnt->m_pNextTypeEntity = 0;
	pEnt->m_pPrevTypeEntity = 0;
}

//
void CGameWorld::Snap(int SnappingClient)
{
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Snap(SnappingClient);
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Reset()
{
	// reset all entities
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			pEnt->Reset();
			pEnt = m_pNextTraverseEntity;
		}
	RemoveEntities();

	GameServer()->m_pController->PostReset();
	RemoveEntities();

	m_ResetRequested = false;
}

void CGameWorld::RemoveEntities()
{
	// destroy objects marked for destruction
	for(int i = 0; i < NUM_ENTTYPES; i++)
		for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
		{
			m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
			if(pEnt->m_MarkedForDestroy)
			{
				RemoveEntity(pEnt);
				pEnt->Destroy();
			}
			pEnt = m_pNextTraverseEntity;
		}
}

void CGameWorld::Tick()
{
	if(m_ResetRequested)
		Reset();

	if(!m_Paused)
	{
		if(GameServer()->m_pController->IsForceBalanced())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Teams have been balanced");
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->Tick();
				pEnt = m_pNextTraverseEntity;
			}

		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickDefered();
				pEnt = m_pNextTraverseEntity;
			}
	}
	else
	{
		// update all objects
		for(int i = 0; i < NUM_ENTTYPES; i++)
			for(CEntity *pEnt = m_apFirstEntityTypes[i]; pEnt; )
			{
				m_pNextTraverseEntity = pEnt->m_pNextTypeEntity;
				pEnt->TickPaused();
				pEnt = m_pNextTraverseEntity;
			}
	}

	RemoveEntities();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		vec2 IntersectPos = closest_point_on_line(Pos0, Pos1, p->m_Pos);
		float Len = distance(p->m_Pos, IntersectPos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			Len = distance(Pos0, IntersectPos);
			if(Len < ClosestLen)
			{
				NewPos = IntersectPos;
				ClosestLen = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}


CCharacter *CGameWorld::ClosestCharacter(vec2 Pos, float Radius, CEntity *pNotThis)
{
	// Find other players
	float ClosestRange = Radius*2;
	CCharacter *pClosest = 0;

	CCharacter *p = (CCharacter *)GameServer()->m_World.FindFirst(ENTTYPE_CHARACTER);
	for(; p; p = (CCharacter *)p->TypeNext())
 	{
		if(p == pNotThis)
			continue;

		float Len = distance(Pos, p->m_Pos);
		if(Len < p->m_ProximityRadius+Radius)
		{
			if(Len < ClosestRange)
			{
				ClosestRange = Len;
				pClosest = p;
			}
		}
	}

	return pClosest;
}

static bool distCompare(const std::pair<float,int>& a, const std::pair<float,int>& b)
{
	return a.first < b.first;
}

void CGameWorld::UpdatePlayerMaps()
{
	if(Server()->Tick() % g_Config.m_SvMapUpdateRate != 0) return;

	std::pair<float,int> dist[MAX_CLIENTS];
	for(int CID = 0; CID < MAX_CLIENTS; CID++)
	{
		if(!Server()->ClientIngame(CID))
			continue;

		int *aMap = Server()->GetIdMap(CID);

		// compute distances
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			dist[i].second = i;
			dist[i].first = 1e10;
			if(!Server()->ClientIngame(i))
				continue;

			CCharacter *pChr = GameServer()->m_apPlayers[i]->GetCharacter();
			if(!pChr)
				continue;

			// copypasted chunk from character.cpp Snap() follows
			int SnappingClient = CID;
			CCharacter *pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
			if(pSnapChar &&
			   GameServer()->m_apPlayers[SnappingClient]->GetTeam() != -1/* &&
				!pChr->CanCollide(SnappingClient) &&
				(!GameServer()->m_apPlayers[SnappingClient]->m_IsUsingDDRaceClient ||
					(GameServer()->m_apPlayers[SnappingClient]->m_IsUsingDDRaceClient &&
					!GameServer()->m_apPlayers[SnappingClient]->m_ShowOthers
                                	)
				)*/
					) continue;

			dist[i].first = distance(GameServer()->m_apPlayers[CID]->m_ViewPos, GameServer()->m_apPlayers[i]->m_ViewPos);
		}

		// always send the player himself
		dist[CID].first = 0;

		// compute reverse map

		int aReverseMap[MAX_CLIENTS];
		for(int i = 0; i < MAX_CLIENTS; i++)
			aReverseMap[i] = -1;

		for(int i = 0; i < DDNET_MAX_CLIENTS; i++)
		{
			if(aMap[i] == -1)
				continue;

			if(dist[aMap[i]].first > 1e9)
				aMap[i] = -1;
			else
				aReverseMap[aMap[i]] = i;
		}

		std::nth_element(&dist[0], &dist[DDNET_MAX_CLIENTS - 1], &dist[MAX_CLIENTS], distCompare);
		std::nth_element(&dist[0], &dist[VANILLA_MAX_CLIENTS - 1], &dist[DDNET_MAX_CLIENTS - 2], distCompare);

		int demand = 0;
		for(int i = 0; i < VANILLA_MAX_CLIENTS - 1; i++)
		{
			int k = dist[i].second;
			if(aReverseMap[k] != -1 || dist[i].first > 1e9)
				continue;

			// search first free slot
			int mapc = 0;
			while(mapc < VANILLA_MAX_CLIENTS && aMap[mapc] != -1)
				mapc++;

			if(mapc < VANILLA_MAX_CLIENTS - 1)
				aMap[mapc] = k;
			else if(dist[i].first < 1300) // dont bother freeing up space for players which are too far to be displayed anyway
				demand++;
		}

		for(int j = DDNET_MAX_CLIENTS - 1; j > VANILLA_MAX_CLIENTS - 2; j--)
		{
			int k = dist[j].second;
			if(aReverseMap[k] != -1 && demand-- > 0)
				aMap[aReverseMap[k]] = -1;
		}
		aMap[VANILLA_MAX_CLIENTS - 1] = -1; // player with empty name to say chat msgs

		// again for ddnet
		demand = 0;
		for(int i = VANILLA_MAX_CLIENTS; i < DDNET_MAX_CLIENTS - 1; i++)
		{
			int k = dist[i].second;
			if(aReverseMap[k] != -1 || dist[i].first > 1e9)
				continue;

			// search first free slot
			int mapc = 0;
			while(mapc < DDNET_MAX_CLIENTS && aMap[mapc] != -1)
				mapc++;

			if(mapc < DDNET_MAX_CLIENTS - 1)
				aMap[mapc] = k;
			else if(dist[i].first < 1300) // dont bother freeing up space for players which are too far to be displayed anyway
				demand++;
		}

		for(int j = MAX_CLIENTS - 1; j > DDNET_MAX_CLIENTS - 2; j--)
		{
			int k = dist[j].second;
			if(aReverseMap[k] != -1 && demand-- > 0)
				aMap[aReverseMap[k]] = -1;
		}
		aMap[DDNET_MAX_CLIENTS - 1] = -1; // player with empty name to say chat msgs
	}
}
