/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/shared/config.h>
#include <algorithm>
#include "gameworld.h"
#include "entity.h"
#include "entities/character.h"
#include "entities/flag.h"
#include "entities/laser.h"
#include "entities/pickup.h"
#include "entities/projectile.h"
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
		m_apFirstEntityTypes[i] = NULL;
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

// for lua

CCharacter *CGameWorld::cast_CCharacter(CEntity *pEnt)
{
	return dynamic_cast<CCharacter *>(pEnt);
}

CFlag *CGameWorld::cast_CFlag(CEntity *pEnt)
{
	return dynamic_cast<CFlag *>(pEnt);
}

CLaser *CGameWorld::cast_CLaser(CEntity *pEnt)
{
	return dynamic_cast<CLaser *>(pEnt);
}

CPickup *CGameWorld::cast_CPickup(CEntity *pEnt)
{
	return dynamic_cast<CPickup *>(pEnt);
}

CProjectile *CGameWorld::cast_CProjectile(CEntity *pEnt)
{
	return dynamic_cast<CProjectile *>(pEnt);
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

	UpdatePlayerMappings();
}


// TODO: should be more general
CCharacter *CGameWorld::IntersectCharacter(vec2 Pos0, vec2 Pos1, float Radius, vec2& NewPos, CEntity *pNotThis)
{
	// Find other players
	float ClosestLen = distance(Pos0, Pos1) * 100.0f;
	CCharacter *pClosest = 0;

	CCharacter *p = dynamic_cast<CCharacter *>(FindFirst(ENTTYPE_CHARACTER));
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

	CCharacter *p = dynamic_cast<CCharacter *>(GameServer()->m_World.FindFirst(ENTTYPE_CHARACTER));
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

struct SClientDist{
	float Dist;
	int ToCID;
};

static int distCompare(const void *pa, const void *pb)
{
	const SClientDist& a = *static_cast<const SClientDist*>(pa);
	const SClientDist& b = *static_cast<const SClientDist*>(pb);
	return a.Dist < b.Dist ? -1 : a.Dist > b.Dist ? 1 : 0;
}

void CGameWorld::UpdatePlayerMappings()
{
	if(Server()->Tick() % g_Config.m_SvIDMapUpdateRate != 0)
		return;

	// calculate for everyone
	for(int FromCID = 0; FromCID < MAX_CLIENTS; FromCID++)
	{
		if(!Server()->ClientIngame(FromCID) || GameServer()->m_apPlayers[FromCID] == NULL)
			continue;

		SClientDist aDists[MAX_CLIENTS];
		const float DIST_INVALID = 1e10;
		const float DIST_DEAD = 1e9;
		const float DIST_OUTOFRANGE = 1e8;

		// compute distances
		for(int ToCID = 0; ToCID < MAX_CLIENTS; ToCID++)
		{
			aDists[ToCID].ToCID = ToCID;
			aDists[ToCID].Dist = 0.0f;

			if(ToCID == FromCID)
				continue;

			if(!Server()->ClientIngame(ToCID) || !GameServer()->m_apPlayers[ToCID])
			{
				aDists[ToCID].Dist = DIST_INVALID;
				continue;
			}

			CCharacter *pChr = GameServer()->m_apPlayers[ToCID]->GetCharacter();
			if(!pChr)
			{
				aDists[ToCID].Dist = DIST_DEAD;
				continue;
			}

			// copypasted chunk from character.cpp Snap() follows
			CPlayer *pPlayer = GameServer()->m_apPlayers[FromCID];
			CCharacter *pSnapChar = GameServer()->GetPlayerChar(FromCID);
			if(pSnapChar && pPlayer->GetTeam() != -1 /*&&
				!pPlayer->IsPaused() && !pSnapChar->m_Super &&
				!pChr->CanCollide(i) &&
				(!pPlayer ||
					pPlayer->m_ClientVersion == VERSION_VANILLA ||
					(pPlayer->m_ClientVersion >= VERSION_DDRACE &&
					!pPlayer->m_ShowOthers
					)
				)*/
					)
				aDists[ToCID].Dist = DIST_OUTOFRANGE;
			else
				aDists[ToCID].Dist = 0.0f;

			aDists[ToCID].Dist = distance(GameServer()->m_apPlayers[FromCID]->m_ViewPos, GameServer()->m_apPlayers[ToCID]->GetCharacter()->m_Pos);
		}

		// always send the player himself
		aDists[FromCID].Dist = 0;


		// sort and assign
		std::qsort(aDists, MAX_CLIENTS, sizeof(SClientDist), distCompare);

		IDMapT *aMap = Server()->GetIdMap(FromCID);
		for(int i = 0; i < DDNET_MAX_CLIENTS; i++)
		{
			aMap[i] = aDists[i].ToCID;
		}
	}
}
