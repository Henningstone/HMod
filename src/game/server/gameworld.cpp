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


void CGameWorld::UpdatePlayerMappings()
{
	if(Server()->Tick() % g_Config.m_SvIDMapUpdateRate != 0)
		return;

	// calculate for everyone
	for(int ForCID = 0; ForCID < MAX_CLIENTS; ForCID++)
	{
		// only calculate maps for existing players
		if(GameServer()->m_apPlayers[ForCID] == NULL || !Server()->ClientIngame(ForCID) || Server()->ClientIsDummy(ForCID))
			continue;

		/*
		 * Algorithm:
		 *
		 * OwnID = 0
		 * MappedID = (InternalID % LargestAssignableID) + 1  // InternalID is the server's, LargestAssignableID = (16||64)-1
		 * if MappedID collides:
		 *   decide by considering priority criteria:
		 *     - hooked players (top priority)
		 *     - has character
		 *     - smaller distance
		 *
		 * ID MAP: [0..InternalID] -> [0..LargestAssignableID-1]
		 *
		 */

		IServer::CClientInfo Info;
		Server()->GetClientInfo(ForCID, &Info);

		// the largest ID our 'ForCID' client can handle
		const int LargestAssignableID = (Info.m_Is128 ? MAX_CLIENTS : Info.m_Is64 ? DDNET_MAX_CLIENTS-1 : VANILLA_MAX_CLIENTS-1) - 1; // one space for chat-fakeid

		// calculate mappings
		#define TAKE_SLOT(ID_TAKING_SLOT, CHOSEN_SLOT) \
				Server()->WriteIdMap(ForCID, ID_TAKING_SLOT, CHOSEN_SLOT);

		const IDMapT *aIDMap = Server()->GetIdMap(ForCID);
		const IDMapT *aRevMap = Server()->GetRevMap(ForCID); // RevMap holds the indices of IDMap at InternalID
		for(int InternalID = 0; InternalID < MAX_CLIENTS; InternalID++)
		{
			// reset slot
			if(aRevMap[InternalID] != IDMapT::DEFAULT)
			{
				Server()->ResetIdMapSlotOf(ForCID, InternalID);
			}

			// check if player is local
			if(InternalID == ForCID)
			{
				// OwnID = 0
				TAKE_SLOT(InternalID, 0);
				continue;
			}

			// only calculate mapping for existing players
			if(!Server()->ClientIngame(InternalID) || GameServer()->m_apPlayers[InternalID] == NULL)
				continue;

			// map internal id range onto client's id range uniformly
			int MappedID = (InternalID % LargestAssignableID) + 1; // [1..14] or [1..62], 0 and 15/63 are reserved

			// look for conflicts
			if(aIDMap[MappedID] == IDMapT::DEFAULT) // aIDMap[MappedID] means "who is displayed as MappedID?"
			{
				// slot is still free, take it
				TAKE_SLOT(InternalID, MappedID);
			}
			else
			{
				// try to find an alternative slot
				FindAltSlot(ForCID, LargestAssignableID, InternalID);
			}
		}

		//Server()->DumpIdMap(ForCID);
	}
}

void CGameWorld::FindAltSlot(int ForCID, int LargestAssignableID, int WhoIsSearching)
{
	const IDMapT *aIDMap = Server()->GetIdMap(ForCID);
	const IDMapT *aRevMap = Server()->GetRevMap(ForCID);
	const vec2& OwnPos = GameServer()->m_apPlayers[ForCID]->m_ViewPos;

	// search for the next free slot
	for(int AltSlot = 1; AltSlot <= LargestAssignableID; AltSlot++)
	{
		// if the slot is free, take it
		if(aIDMap[AltSlot] == IDMapT::DEFAULT)
		{
			TAKE_SLOT(WhoIsSearching, AltSlot);
			return;
		}
	}

	// no free slot, decide whom to kick out
	CCharacter *pCurrChr = GameServer()->GetPlayerChar(WhoIsSearching);
	if(!pCurrChr)
	{
		// who doesn't have a character shall not get the slot
		return;
	}

	// kick out who's the farthest from 'ForID'
	{
		float MaxDist = distance(OwnPos, pCurrChr->m_Pos);
		int FarthestID = WhoIsSearching;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!Server()->ClientIngame(i) || GameServer()->m_apPlayers[i] == NULL)
			{
				// there should be no slot taken up by non-existent players
				// DEBUG: verify
				dbg_assert_strict(aRevMap[i] == IDMapT::DEFAULT, "non-existent player DOES take up a slot?!");
				continue;
			}

			if(i == WhoIsSearching)
				continue;

			CCharacter *pChr = GameServer()->GetPlayerChar(i);
			if(!pChr)
				continue;

			// check if he's at all using up a slot
			if(aRevMap[i] == IDMapT::DEFAULT)
				continue;

			// only consider who is actually IN the usable part of the id map
			// DEBUG: if someone is OUTSIDE of the map, that would be a bug then.
			dbg_assert_strict(aRevMap[i] <= LargestAssignableID, "ip map went out of range somehow");

			// calculate and check distance
			float Dist = distance(OwnPos, pChr->GetCore()->m_Pos);
			if(Dist > MaxDist)
			{
				MaxDist = Dist;
				FarthestID = i;
			}
		}

		if(FarthestID == WhoIsSearching)
		{
			// player WhoIsSearching is the farthest, drop him.
			return;
		}
		else
		{
			// take the farthest player's slot, kicking him out
			//dbg_msg("idmap/debug", "CID %i stealing slot %i from CID %i", WhoIsSearching, aRevMap[FarthestID], FarthestID);
			int SlotID = Server()->ResetIdMapSlotOf(ForCID, FarthestID);
			if(SlotID >= 0)
				TAKE_SLOT(WhoIsSearching, SlotID);
		}
	}
}

#undef TAKE_SLOT
