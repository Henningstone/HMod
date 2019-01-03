/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/math.h>
#include <base/system.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/engine.h>
#include <engine/map.h>
#include <engine/masterserver.h>
#include <engine/server.h>
#include <engine/storage.h>

#include <engine/shared/compression.h>
#include <engine/shared/config.h>
#include <engine/shared/datafile.h>
#include <engine/shared/demo.h>
#include <engine/shared/econ.h>
#include <engine/shared/filecollection.h>
#include <engine/shared/mapchecker.h>
#include <engine/shared/netban.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/snapshot.h>

#include <mastersrv/mastersrv.h>
#include <engine/lua.h>
#include <functional>
#include <game/generated/protocol.h>

#include "register.h"
#include "server.h"
#include "luabinding.h"

#if defined(CONF_FAMILY_WINDOWS)
	#define _WIN32_WINNT 0x0501
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

static const char *StrLtrim(const char *pStr)
{
	while(*pStr && *pStr >= 0 && *pStr <= 32)
		pStr++;
	return pStr;
}

static void StrRtrim(char *pStr)
{
	int i = str_length(pStr);
	while(i >= 0)
	{
		if(pStr[i] < 0 || pStr[i] > 32)
			break;
		pStr[i] = 0;
		i--;
	}
}


CSnapIDPool::CSnapIDPool()
{
	Reset();
}

void CSnapIDPool::Reset()
{
	for(int i = 0; i < MAX_IDS; i++)
	{
		m_aIDs[i].m_Next = i+1;
		m_aIDs[i].m_State = 0;
	}

	m_aIDs[MAX_IDS-1].m_Next = -1;
	m_FirstFree = 0;
	m_FirstTimed = -1;
	m_LastTimed = -1;
	m_Usage = 0;
	m_InUsage = 0;
}


void CSnapIDPool::RemoveFirstTimeout()
{
	int NextTimed = m_aIDs[m_FirstTimed].m_Next;

	// add it to the free list
	m_aIDs[m_FirstTimed].m_Next = m_FirstFree;
	m_aIDs[m_FirstTimed].m_State = 0;
	m_FirstFree = m_FirstTimed;

	// remove it from the timed list
	m_FirstTimed = NextTimed;
	if(m_FirstTimed == -1)
		m_LastTimed = -1;

	m_Usage--;
}

int CSnapIDPool::NewID()
{
	int64 Now = time_get();

	// process timed ids
	while(m_FirstTimed != -1 && m_aIDs[m_FirstTimed].m_Timeout < Now)
		RemoveFirstTimeout();

	int ID = m_FirstFree;
	dbg_assert(ID != -1, "id error");
	if(ID == -1)
		return ID;
	m_FirstFree = m_aIDs[m_FirstFree].m_Next;
	m_aIDs[ID].m_State = 1;
	m_Usage++;
	m_InUsage++;
	return ID;
}

void CSnapIDPool::TimeoutIDs()
{
	// process timed ids
	while(m_FirstTimed != -1)
		RemoveFirstTimeout();
}

void CSnapIDPool::FreeID(int ID)
{
	if(ID < 0)
		return;
	dbg_assert(m_aIDs[ID].m_State == 1, "id is not alloced");

	m_InUsage--;
	m_aIDs[ID].m_State = 2;
	m_aIDs[ID].m_Timeout = time_get()+time_freq()*5;
	m_aIDs[ID].m_Next = -1;

	if(m_LastTimed != -1)
	{
		m_aIDs[m_LastTimed].m_Next = ID;
		m_LastTimed = ID;
	}
	else
	{
		m_FirstTimed = ID;
		m_LastTimed = ID;
	}
}


void CServerBan::InitServerBan(IConsole *pConsole, IStorage *pStorage, CServer* pServer)
{
	CNetBan::Init(pConsole, pStorage);

	m_pServer = pServer;

	// overwrites base command, todo: improve this
	Console()->Register("ban", "s?ir", CFGFLAG_SERVER|CFGFLAG_STORE, ConBanExt, this, "Ban player with ip/client id for x minutes for any reason");
}

template<class T>
int CServerBan::BanExt(T *pBanPool, const typename T::CDataType *pData, int Seconds, const char *pReason)
{
	// validate address
	if(Server()->m_RconExecClientID >= 0 && Server()->m_RconExecClientID < MAX_CLIENTS &&
		Server()->m_aClients[Server()->m_RconExecClientID].m_State != CServer::CClient::STATE_EMPTY)
	{
		if(NetMatch(pData, Server()->m_NetServer.ClientAddr(Server()->m_RconExecClientID)))
		{
			Console()->PrintTo(Server()->m_RconExecClientID, "net_ban", "ban error (you can't ban yourself)");
			return -1;
		}

		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(i == Server()->m_RconExecClientID || Server()->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY)
				continue;

			if(Server()->m_aClients[i].m_Authed >= Server()->m_aClients[Server()->m_RconExecClientID].m_Authed &&
			   NetMatch(pData, Server()->m_NetServer.ClientAddr(i)))
			{
				Console()->PrintTo(Server()->m_RconExecClientID, "net_ban", "ban error (command denied)");
				return -1;
			}
		}
	}
	else if(Server()->m_RconExecClientID == IServer::RCON_CID_VOTE) // vote ban
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(Server()->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY)
				continue;

			if((Server()->m_aClients[i].m_Authed == CServer::AUTHED_ADMIN || Server()->m_aClients[i].m_Authed == CServer::AUTHED_MOD) &&
			   NetMatch(pData, Server()->m_NetServer.ClientAddr(i)))
			{
				Console()->PrintTo(Server()->m_RconExecClientID, "net_ban", "ban error (command denied)");
				return -1;
			}
		}
	}

	int Result = Ban(pBanPool, pData, Seconds, pReason);
	if(Result != 0)
		return Result;

	// drop banned clients
	typename T::CDataType Data = *pData;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(Server()->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY)
			continue;

		if(NetMatch(&Data, Server()->m_NetServer.ClientAddr(i)))
		{
			CNetHash NetHash(&Data);
			char aBuf[256];
			MakeBanInfo(pBanPool->Find(&Data, &NetHash), aBuf, sizeof(aBuf), MSGTYPE_PLAYER);
			Server()->m_NetServer.Drop(i, aBuf);
		}
	}

	return Result;
}

int CServerBan::BanAddr(const NETADDR *pAddr, int Seconds, const char *pReason)
{
	return BanExt(&m_BanAddrPool, pAddr, Seconds, pReason);
}

int CServerBan::BanRange(const CNetRange *pRange, int Seconds, const char *pReason)
{
	if(pRange->IsValid())
		return BanExt(&m_BanRangePool, pRange, Seconds, pReason);

	Console()->PrintTo(Server()->m_RconExecClientID, "net_ban", "ban failed (invalid range)");
	return -1;
}

void CServerBan::ConBanExt(IConsole::IResult *pResult, void *pUser)
{
	CServerBan *pThis = static_cast<CServerBan *>(pUser);

	const char *pStr = pResult->GetString(0);
	int Minutes = pResult->NumArguments()>1 ? clamp(pResult->GetInteger(1), 0, 44640) : 30;
	const char *pReason = pResult->NumArguments()>2 ? pResult->GetString(2) : "No reason given";

	if(StrAllnum(pStr))
	{
		int ClientID = str_toint(pStr);
		if(ClientID < 0 || ClientID >= MAX_CLIENTS || pThis->Server()->m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
			pThis->Console()->PrintTo(pThis->Server()->m_RconExecClientID, "net_ban", "ban error (invalid client id)");
		else
			pThis->BanAddr(pThis->Server()->m_NetServer.ClientAddr(ClientID), Minutes*60, pReason);
	}
	else
		ConBan(pResult, pUser);
}


void CServer::CClient::Reset()
{
	// reset input
	for(int i = 0; i < 200; i++)
		m_aInputs[i].m_GameTick = -1;
	m_CurrentInput = 0;
	mem_zero(&m_LatestInput, sizeof(m_LatestInput));

	m_Snapshots.PurgeAll();
	m_LastAckedSnapshot = -1;
	m_LastInputTick = -1;
	m_SnapRate = CClient::SNAPRATE_INIT;
	m_Score = 0;
}

CServer::CServer() : m_DemoRecorder(&m_SnapshotDelta)
{
	m_TickSpeed = SERVER_TICK_SPEED;

	m_pGameServer = 0;

	m_CurrentGameTick = 0;
	m_RunServer = SERVER_RUNNING;
	str_copyb(m_aShutdownReason, "Server shutdown");

	m_pCurrentMapData = 0;
	m_CurrentMapSize = 0;

	m_MapReload = 0;
	m_LuaReinit = 0;

	m_RconExecClientID = IServer::RCON_CID_SERV;

	Init();
}


int CServer::TrySetClientName(int ClientID, const char *pName)
{
	char aTrimmedName[64];

	// trim the name
	str_copy(aTrimmedName, StrLtrim(pName), sizeof(aTrimmedName));
	StrRtrim(aTrimmedName);

	// check for empty names
	if(!aTrimmedName[0])
		return -1;

	// check if new and old name are the same
	if(m_aClients[ClientID].m_aName[0] && str_comp(m_aClients[ClientID].m_aName, aTrimmedName) == 0)
		return 0;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "'%s' -> '%s'", pName, aTrimmedName);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
	pName = aTrimmedName;

	// make sure that two clients doesn't have the same name
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(i != ClientID && m_aClients[i].m_State >= CClient::STATE_READY)
		{
			if(str_comp(pName, m_aClients[i].m_aName) == 0)
				return -1;
		}

	// set the client name
	str_copy(m_aClients[ClientID].m_aName, pName, MAX_NAME_LENGTH);
	return 0;
}



void CServer::SetClientName(int ClientID, const char *pName)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;

	if(!pName)
		return;

	char aCleanName[MAX_NAME_LENGTH];
	str_copy(aCleanName, pName, sizeof(aCleanName));

	// clear name
	for(char *p = aCleanName; *p; ++p)
	{
		if(*p < 32)
			*p = ' ';
	}

	if(TrySetClientName(ClientID, aCleanName))
	{
		// auto rename
		for(int i = 1;; i++)
		{
			char aNameTry[MAX_NAME_LENGTH];
			str_format(aNameTry, sizeof(aCleanName), "(%d)%s", i, aCleanName);
			if(TrySetClientName(ClientID, aNameTry) == 0)
				break;
		}
	}
}

void CServer::SetClientClan(int ClientID, const char *pClan)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY || !pClan)
		return;

	str_copy(m_aClients[ClientID].m_aClan, pClan, MAX_CLAN_LENGTH);
}

void CServer::SetClientCountry(int ClientID, int Country)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;

	m_aClients[ClientID].m_Country = Country;
}

void CServer::SetClientScore(int ClientID, int Score)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;
	m_aClients[ClientID].m_Score = Score;
}

void CServer::SetClientAccessLevel(int ClientID, int AccessLevel, bool SendRconCmds)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State < CClient::STATE_READY)
		return;

	int PrevAccessLevel = m_aClients[ClientID].m_AccessLevel;

	// nothing to change
	if(PrevAccessLevel == AccessLevel)
		return;

	// check if they've already got an access level or we want to remove it
	if(m_aClients[ClientID].m_Authed || (m_aClients[ClientID].m_Authed && AccessLevel < 0))
	{
		CMsgPacker Msg(proto06::NETMSG_RCON_AUTH_STATUS, true);
		Msg.AddInt(0);    //authed
		Msg.AddInt(0);    //cmdlist
		SendMsg(&Msg, MSGFLAG_VITAL, ClientID);

		m_aClients[ClientID].m_Authed = AUTHED_NO;
		m_aClients[ClientID].m_AuthTries = 0;
		m_aClients[ClientID].m_AccessLevel = IConsole::ACCESS_LEVEL_WORST;
		m_aClients[ClientID].m_pRconCmdToSend = 0;

		if(AccessLevel < 0)
		{
			SendRconLine(ClientID, "Console Access revoked.");
			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "ClientID=%d force-logged out", ClientID);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

			return;
		}
	}

	// tell them that they're now authed
	if(Is07(ClientID))
	{
		CMsgPacker Msg(proto07::NETMSG_RCON_AUTH_ON, true);
		SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}
	else
	{
		CMsgPacker Msg(proto06::NETMSG_RCON_AUTH_STATUS, true);
		Msg.AddInt(1);	/*authed*/
		Msg.AddInt(1);	/*cmdlist*/
		SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
	}

	m_aClients[ClientID].m_Authed = AccessLevel == IConsole::ACCESS_LEVEL_ADMIN
									? AUTHED_ADMIN
									: AccessLevel == IConsole::ACCESS_LEVEL_MOD
									  ? AUTHED_MOD
									  : AUTHED_CUSTOM;
	m_aClients[ClientID].m_AccessLevel = AccessLevel;
	if(SendRconCmds)
		m_aClients[ClientID].m_pRconCmdToSend = Console()->FirstCommandInfo(AccessLevel, CFGFLAG_SERVER);

	if(PrevAccessLevel == IConsole::ACCESS_LEVEL_WORST)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Welcome! Access level %i granted, selected commands available.", AccessLevel);
		SendRconLine(ClientID, aBuf);

		str_format(aBuf, sizeof(aBuf), "ClientID=%d was granted access level %i", ClientID, AccessLevel);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
	else
	{
		char aBuf[256];
		str_formatb(aBuf, "Your access level was changed from %i to %i.", PrevAccessLevel, AccessLevel);
		SendRconLine(ClientID, aBuf);

		str_format(aBuf, sizeof(aBuf), "ClientID=%d was changed from access level %i to %i", ClientID, PrevAccessLevel, AccessLevel);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	}
}

void CServer::Kick(int ClientID, const char *pReason)
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CClient::STATE_EMPTY)
	{
		Console()->PrintTo(m_RconExecClientID, "server", "invalid client id to kick");
		return;
	}
	else if(m_RconExecClientID == ClientID)
	{
		Console()->PrintTo(m_RconExecClientID, "server", "you can't kick yourself");
 		return;
	}
	else if(m_aClients[m_RconExecClientID].m_Authed < m_aClients[ClientID].m_Authed)
	{
		Console()->PrintTo(m_RconExecClientID, "server", "kick command denied");
 		return;
	}

	m_NetServer.Drop(ClientID, pReason);
}

/*int CServer::Tick()
{
	return m_CurrentGameTick;
}*/

int64 CServer::TickStartTime(int Tick)
{
	return m_GameStartTime + (time_freq()*Tick)/SERVER_TICK_SPEED;
}

/*int CServer::TickSpeed()
{
	return SERVER_TICK_SPEED;
}*/

int CServer::Init()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aClients[i].m_State = CClient::STATE_EMPTY;
		m_aClients[i].m_aName[0] = 0;
		m_aClients[i].m_aClan[0] = 0;
		m_aClients[i].m_Country = -1;
		m_aClients[i].m_ClientSupportFlags = 0;
		m_aClients[i].m_Snapshots.Init();
	}

	m_CurrentGameTick = 0;

	return 0;
}

void CServer::SetRconCID(int ClientID)
{
	m_RconExecClientID = ClientID;
}

bool CServer::IsAuthed(int ClientID) const
{
	dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "client_id is not valid");
	return m_aClients[ClientID].m_Authed != AUTHED_NO;
}

bool CServer::HasAccess(int ClientID, int AccessLevel) const
{
	dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "client_id is not valid");
	return m_aClients[ClientID].m_AccessLevel <= AccessLevel;
}

int CServer::GetClientInfo(int ClientID, CClientInfo *pInfo) const
{
	dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "client_id is not valid");
	dbg_assert(pInfo != 0, "info can not be null");

	if(m_aClients[ClientID].m_State == CClient::STATE_INGAME)
	{
		pInfo->m_pName = m_aClients[ClientID].m_aName;
		pInfo->m_Latency = m_aClients[ClientID].m_Latency;
		pInfo->m_Is64 = m_aClients[ClientID].Supports(CClient::SUPPORTS_64P);
		pInfo->m_Is128 = m_aClients[ClientID].Supports(CClient::SUPPORTS_128P);
		return 1;
	}
	return 0;
}

void CServer::GetClientAddr(int ClientID, char *pAddrStr, int Size) const
{
	if(ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State == CClient::STATE_INGAME)
		net_addr_str(m_NetServer.ClientAddr(ClientID), pAddrStr, Size, false);
}


const char *CServer::ClientName(int ClientID) const
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return "(invalid)";
	if(m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME)
		return m_aClients[ClientID].m_aName;
	else if(m_aClients[ClientID].m_State == CServer::CClient::STATE_DUMMY)
	{
		static char aID[16];
		str_formatb(aID, "Dummy %i", ClientID);
		return aID;
	}
	else
		return "(connecting)";

}

const char *CServer::ClientClan(int ClientID) const
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return "";
	if(m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME)
		return m_aClients[ClientID].m_aClan;
	else
		return "";
}

int CServer::ClientCountry(int ClientID) const
{
	if(ClientID < 0 || ClientID >= MAX_CLIENTS || m_aClients[ClientID].m_State == CServer::CClient::STATE_EMPTY)
		return -1;
	if(m_aClients[ClientID].m_State == CServer::CClient::STATE_INGAME)
		return m_aClients[ClientID].m_Country;
	else
		return -1;
}

bool CServer::ClientIngame(int ClientID) const
{
	return ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State >= CServer::CClient::STATE_INGAME;
}

bool CServer::ClientIsDummy(int ClientID) const
{
	return ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State == CServer::CClient::STATE_DUMMY;
}

int CServer::MaxClients() const
{
	return m_NetServer.MaxClients();
}

IDMapT *CServer::GetIdMap(int ClientID)
{
	return m_aaIDMap[ClientID];
}

bool IServer::IDTranslate(int *pTarget, int ForClientID)
{
	CClientInfo Info;
	GetClientInfo(ForClientID, &Info);
	if(ForClientID > DDNET_MAX_CLIENTS-1 && Info.m_Is128)
		return true;
	if(ForClientID > VANILLA_MAX_CLIENTS-1 && (Info.m_Is64 || Info.m_Is128))
		return true;

	// we need to translate
	const IDMapT *aMap = GetIdMap(ForClientID);
	bool Found = false;
	for(int i = 0; i < (Info.m_Is64 ? DDNET_MAX_CLIENTS : VANILLA_MAX_CLIENTS); i++)
	{
		if(*pTarget == aMap[i])
		{
			*pTarget = i;
			Found = true;
			break;
		}
	}

	return Found;
}

bool IServer::IDTranslateReverse(int *pTarget, int ForClientID)
{
	CClientInfo Info;
	GetClientInfo(ForClientID, &Info);
	if(Info.m_Is64)
		return true;

	IDMapT *aMap = GetIdMap(ForClientID);
	if (aMap[*pTarget] == -1)
		return false;

	*pTarget = aMap[*pTarget];

	return true;
}

int CServer::SendMsg(CMsgPacker *pMsg, int Flags, int ClientID)
{
	CNetChunk Packet;
	if(!pMsg)
		return -1;

#ifdef CONF_DEBUG
	if(g_Config.m_DbgDummies)
	{
		if(ClientID >= MAX_CLIENTS-g_Config.m_DbgDummies)
			return 1;
	}
#endif

	mem_zero(&Packet, sizeof(CNetChunk));

	Packet.m_ClientID = ClientID;
	Packet.m_pData = pMsg->Data();
	Packet.m_DataSize = pMsg->Size();

	if(Flags&MSGFLAG_VITAL)
		Packet.m_Flags |= NETSENDFLAG_VITAL;
	if(Flags&MSGFLAG_FLUSH)
		Packet.m_Flags |= NETSENDFLAG_FLUSH;

	// write message to demo recorder
	if(!(Flags&MSGFLAG_NORECORD))
		m_DemoRecorder.RecordMessage(pMsg->Data(), pMsg->Size());

	if(!(Flags&MSGFLAG_NOSEND))
	{
		if(ClientID == -1)
		{
			// broadcast
			int i;
			for(i = 0; i < MAX_CLIENTS; i++)
				if(m_aClients[i].m_State == CClient::STATE_INGAME)
				{
					Packet.m_ClientID = i;
					m_NetServer.Send(&Packet);
				}
		}
		else
			m_NetServer.Send(&Packet);
	}
	return 0;
}

void CServer::DoSnapshot()
{
	GameServer()->OnPreSnap();

	// create snapshot for demo recording
	if(m_DemoRecorder.IsRecording())
	{
		char aData[CSnapshot::MAX_SIZE];
		int SnapshotSize;

		// build snap and possibly add some messages
		m_SnapshotBuilder.Init();
		GameServer()->OnSnap(-1);
		SnapshotSize = m_SnapshotBuilder.Finish(aData);

		// write snapshot
		m_DemoRecorder.RecordSnapshot(Tick(), aData, SnapshotSize);
	}

	// create snapshots for all clients
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// client must be ingame to recive snapshots
		if(m_aClients[i].m_State != CClient::STATE_INGAME)
			continue;

		// this client is trying to recover, don't spam snapshots
		if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_RECOVER && (Tick()%50) != 0)
			continue;

		// this client is trying to recover, don't spam snapshots
		if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_INIT && (Tick()%10) != 0)
			continue;

		{
			char aData[CSnapshot::MAX_SIZE];
			CSnapshot *pData = (CSnapshot*)aData;	// Fix compiler warning for strict-aliasing
			char aDeltaData[CSnapshot::MAX_SIZE];
			char aCompData[CSnapshot::MAX_SIZE];
			int SnapshotSize;
			int Crc;
			static CSnapshot EmptySnap;
			CSnapshot *pDeltashot = &EmptySnap;
			int DeltashotSize;
			int DeltaTick = -1;
			int DeltaSize;

			m_SnapshotBuilder.Init();

			GameServer()->OnSnap(i);

			// finish snapshot
			SnapshotSize = m_SnapshotBuilder.Finish(pData);
			Crc = pData->Crc();

			// remove old snapshos
			// keep 3 seconds worth of snapshots
			m_aClients[i].m_Snapshots.PurgeUntil(m_CurrentGameTick-SERVER_TICK_SPEED*3);

			// save it the snapshot
			m_aClients[i].m_Snapshots.Add(m_CurrentGameTick, time_get(), SnapshotSize, pData, 0);

			// find snapshot that we can preform delta against
			EmptySnap.Clear();

			{
				DeltashotSize = m_aClients[i].m_Snapshots.Get(m_aClients[i].m_LastAckedSnapshot, 0, &pDeltashot, 0);
				if(DeltashotSize >= 0)
					DeltaTick = m_aClients[i].m_LastAckedSnapshot;
				else
				{
					// no acked package found, force client to recover rate
					if(m_aClients[i].m_SnapRate == CClient::SNAPRATE_FULL)
						m_aClients[i].m_SnapRate = CClient::SNAPRATE_RECOVER;
				}
			}

			// create delta
			DeltaSize = m_SnapshotDelta.CreateDelta(pDeltashot, pData, aDeltaData);

			if(DeltaSize)
			{
				// compress it
				int SnapshotSize;
				const int MaxSize = MAX_SNAPSHOT_PACKSIZE;
				int NumPackets;

				SnapshotSize = CVariableInt::Compress(aDeltaData, DeltaSize, aCompData, sizeof(aCompData));
				NumPackets = (SnapshotSize+MaxSize-1)/MaxSize;

				for(int n = 0, Left = SnapshotSize; Left > 0; n++)
				{
					int Chunk = Left < MaxSize ? Left : MaxSize;
					Left -= Chunk;

					if(NumPackets == 1)
					{
						CMsgPacker Msg(NETMSG_SNAPSINGLE, true);
						Msg.AddInt(m_CurrentGameTick);
						Msg.AddInt(m_CurrentGameTick-DeltaTick);
						Msg.AddInt(Crc);
						Msg.AddInt(Chunk);
						Msg.AddRaw(&aCompData[n*MaxSize], Chunk);
						SendMsg(&Msg, MSGFLAG_FLUSH, i);
					}
					else
					{
						CMsgPacker Msg(NETMSG_SNAP, true);
						Msg.AddInt(m_CurrentGameTick);
						Msg.AddInt(m_CurrentGameTick-DeltaTick);
						Msg.AddInt(NumPackets);
						Msg.AddInt(n);
						Msg.AddInt(Crc);
						Msg.AddInt(Chunk);
						Msg.AddRaw(&aCompData[n*MaxSize], Chunk);
						SendMsg(&Msg, MSGFLAG_FLUSH, i);
					}
				}
			}
			else
			{
				CMsgPacker Msg(NETMSG_SNAPEMPTY, true);
				Msg.AddInt(m_CurrentGameTick);
				Msg.AddInt(m_CurrentGameTick-DeltaTick);
				SendMsg(&Msg, MSGFLAG_FLUSH, i);
			}
		}
	}

	GameServer()->OnPostSnap();
}


int CServer::NewClientCallback(int ClientID, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	pThis->m_aClients[ClientID].m_State = CClient::STATE_AUTH;
	pThis->m_aClients[ClientID].m_aName[0] = 0;
	pThis->m_aClients[ClientID].m_aClan[0] = 0;
	pThis->m_aClients[ClientID].m_Country = -1;
	pThis->m_aClients[ClientID].m_Authed = AUTHED_NO;
	pThis->m_aClients[ClientID].m_AuthTries = 0;
	pThis->m_aClients[ClientID].m_AccessLevel = IConsole::ACCESS_LEVEL_WORST;
	pThis->m_aClients[ClientID].m_pRconCmdToSend = NULL;
	pThis->m_aClients[ClientID].m_ClientSupportFlags = 0;
	pThis->m_aClients[ClientID].Reset();
	return 0;
}

int CServer::DelClientCallback(int ClientID, const char *pReason, void *pUser)
{
	CServer *pThis = (CServer *)pUser;

	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(pThis->m_NetServer.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "client dropped. cid=%d addr=%s reason='%s'", ClientID, aAddrStr,	pReason);
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);

	// notify the mod about the drop
	if(pThis->m_aClients[ClientID].m_State >= CClient::STATE_READY)
		pThis->GameServer()->OnClientDrop(ClientID, pReason);

	pThis->m_aClients[ClientID].m_State = CClient::STATE_EMPTY;
	pThis->m_aClients[ClientID].m_aName[0] = 0;
	pThis->m_aClients[ClientID].m_aClan[0] = 0;
	pThis->m_aClients[ClientID].m_Country = -1;
	pThis->m_aClients[ClientID].m_Authed = AUTHED_NO;
	pThis->m_aClients[ClientID].m_AuthTries = 0;
	pThis->m_aClients[ClientID].m_AccessLevel = IConsole::ACCESS_LEVEL_WORST;
	pThis->m_aClients[ClientID].m_pRconCmdToSend = 0;
	pThis->m_aClients[ClientID].m_Snapshots.PurgeAll();
	return 0;
}

void CServer::InitDummy(int ClientID)
{
	m_aClients[ClientID].m_State = CClient::STATE_DUMMY;
}

void CServer::PurgeDummy(int ClientID)
{
	m_aClients[ClientID].m_State = CClient::STATE_EMPTY;
}

void CServer::SendMap(int ClientID)
{
	CMsgPacker Msg(PROTO_S(NETMSG_MAP_CHANGE), true);
	Msg.AddString(GetMapName(), 0);
	Msg.AddInt(m_CurrentMapCrc);
	Msg.AddInt(m_CurrentMapSize);
	if(Is07(ClientID))
	{
		Msg.AddInt(m_MapChunksPerRequest);
		Msg.AddInt(MAP_CHUNK_SIZE);
	}
	SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
}

void CServer::SendConnectionReady(int ClientID)
{
	CMsgPacker Msg(NETMSG_CON_READY, true);
	SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
}

void CServer::SendRconLine(int ClientID, const char *pLine)
{
	CMsgPacker Msg(NETMSG_RCON_LINE, true);
	Msg.AddString(pLine, 512);
	SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CServer::SendRconLineAuthed(const char *pLine, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	static volatile int ReentryGuard = 0;
	int i;

	if(ReentryGuard) return;
	ReentryGuard++;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(pThis->m_aClients[i].m_State != CClient::STATE_EMPTY &&
		   (pThis->m_aClients[i].m_Authed == AUTHED_ADMIN || pThis->m_aClients[i].m_Authed == AUTHED_MOD))
			pThis->SendRconLine(i, pLine);
	}

	ReentryGuard--;
}

void CServer::SendRconLineTo(int To, const char *pLine, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	static volatile int ReentryGuard = 0;

	if(ReentryGuard) return;
	ReentryGuard++;

	if(To < 0 || To > MAX_CLIENTS)
		SendRconLineAuthed(pLine, pUser);
	else if(pThis->m_aClients[To].m_State != CClient::STATE_EMPTY)
		pThis->SendRconLine(To, pLine);

	ReentryGuard--;
}

void CServer::SendRconCmdAdd(const IConsole::CCommandInfo *pCommandInfo, int ClientID)
{
	CMsgPacker Msg(NETMSG_RCON_CMD_ADD, true);
	Msg.AddString(pCommandInfo->m_pName, IConsole::TEMPCMD_NAME_LENGTH);
	Msg.AddString(pCommandInfo->m_pHelp, IConsole::TEMPCMD_HELP_LENGTH);
	Msg.AddString(pCommandInfo->m_pParams, IConsole::TEMPCMD_PARAMS_LENGTH);
	SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CServer::SendRconCmdRem(const IConsole::CCommandInfo *pCommandInfo, int ClientID)
{
	CMsgPacker Msg(NETMSG_RCON_CMD_REM, true);
	Msg.AddString(pCommandInfo->m_pName, 256);
	SendMsg(&Msg, MSGFLAG_VITAL, ClientID);
}

void CServer::UpdateClientRconCommands()
{
	int ClientID = Tick() % MAX_CLIENTS;

	if(m_aClients[ClientID].m_State != CClient::STATE_EMPTY)
	{
		int ConsoleAccessLevel = m_aClients[ClientID].m_Authed == AUTHED_ADMIN
								 ? IConsole::ACCESS_LEVEL_ADMIN
								 : m_aClients[ClientID].m_Authed == AUTHED_MOD
								   ? IConsole::ACCESS_LEVEL_MOD
								   : m_aClients[ClientID].m_AccessLevel;
		for(int i = 0; i < MAX_RCONCMD_SEND && m_aClients[ClientID].m_pRconCmdToSend; ++i)
		{
			SendRconCmdAdd(m_aClients[ClientID].m_pRconCmdToSend, ClientID);
			m_aClients[ClientID].m_pRconCmdToSend = m_aClients[ClientID].m_pRconCmdToSend->NextCommandInfo(ConsoleAccessLevel, CFGFLAG_SERVER);
		}
	}
}

void CServer::ProcessClientPacket(CNetChunk *pPacket)
{
	int ClientID = pPacket->m_ClientID;
	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		// system message
		if(Msg == NETMSG_INFO)
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State == CClient::STATE_AUTH)
			{
				const char *pVersion = Unpacker.GetString(CUnpacker::SANITIZE_CC);
				bool Client07 = str_comp_num(pVersion, "0.7", 3) == 0;

				if(str_comp(pVersion, GameServer()->NetVersion(Client07)) != 0)
				{
					// wrong version
					char aReason[256];
					str_format(aReason, sizeof(aReason), "Wrong version. Server is running '%s' and client '%s'", GameServer()->NetVersion(Client07), pVersion);
					m_NetServer.Drop(ClientID, aReason);
					return;
				}

				const char *pPassword = Unpacker.GetString(CUnpacker::SANITIZE_CC);
				if(g_Config.m_Password[0] != 0 && str_comp(g_Config.m_Password, pPassword) != 0)
				{
					// wrong password
					m_NetServer.Drop(ClientID, "Wrong password");
					return;
				}

				// 0.7
				if(Client07)
				{
					m_aClients[ClientID].m_Version = Unpacker.GetInt();
					m_aClients[ClientID].m_ClientSupportFlags |= CClient::SUPPORTS_TW07;
				}

				m_aClients[ClientID].m_State = CClient::STATE_CONNECTING;
				SendMap(ClientID);
			}
		}
		else if(Msg == PROTO_S(NETMSG_REQUEST_MAP_DATA))
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) == 0 || m_aClients[ClientID].m_State != CClient::STATE_CONNECTING)
				return;

			if(Is07(ClientID))
			{
				// 0.7 map download
				int ChunkSize = MAP_CHUNK_SIZE;

				// send map chunks
				for(int i = 0; i < m_MapChunksPerRequest && m_aClients[ClientID].m_MapChunk >= 0; ++i)
				{
					int Chunk = m_aClients[ClientID].m_MapChunk;
					int Offset = Chunk * ChunkSize;

					// check for last part
					if(Offset+ChunkSize >= m_CurrentMapSize)
					{
						ChunkSize = m_CurrentMapSize-Offset;
						m_aClients[ClientID].m_MapChunk = -1;
					}
					else
						m_aClients[ClientID].m_MapChunk++;

					CMsgPacker Msg(proto07::NETMSG_MAP_DATA, true);
					Msg.AddRaw(&m_pCurrentMapData[Offset], ChunkSize);
					SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);

					if(g_Config.m_Debug)
					{
						char aBuf[64];
						str_format(aBuf, sizeof(aBuf), "sending 0.7 chunk %d with size %d", Chunk, ChunkSize);
						Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
					}
				}
			}
			else
			{
				// 0.6 map download
				unsigned int ChunkSize = 1024-128;

				int Chunk = Unpacker.GetInt();
				unsigned int Offset = Chunk * ChunkSize;
				int Last = 0;

				// drop faulty map data requests
				if(Chunk < 0 || Offset > m_CurrentMapSize)
					return;

				if(Offset+ChunkSize >= m_CurrentMapSize)
				{
					ChunkSize = m_CurrentMapSize-Offset;
					if(ChunkSize < 0)
						ChunkSize = 0;
					Last = 1;
				}

				CMsgPacker Msg(proto06::NETMSG_MAP_DATA, true);
				Msg.AddInt(Last);
				Msg.AddInt(m_CurrentMapCrc);
				Msg.AddInt(Chunk);
				Msg.AddInt(ChunkSize);
				Msg.AddRaw(&m_pCurrentMapData[Offset], ChunkSize);
				SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);

				if(g_Config.m_Debug)
				{
					char aBuf[64];
					str_format(aBuf, sizeof(aBuf), "sending 0.6 chunk %d with size %d", Chunk, ChunkSize);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
				}
			}
		}
		else if(Msg == PROTO_S(NETMSG_READY))
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State == CClient::STATE_CONNECTING)
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(m_NetServer.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);

				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "player is ready. ClientID=%x addr=%s", ClientID, aAddrStr);
				Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
				m_aClients[ClientID].m_State = CClient::STATE_READY;
				GameServer()->OnClientConnected(ClientID);
				SendConnectionReady(ClientID);
			}
		}
		else if(Msg == PROTO_S(NETMSG_ENTERGAME))
		{
			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State == CClient::STATE_READY && GameServer()->IsClientReady(ClientID))
			{
				char aAddrStr[NETADDR_MAXSTRSIZE];
				net_addr_str(m_NetServer.ClientAddr(ClientID), aAddrStr, sizeof(aAddrStr), true);

				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "player has entered the game. ClientID=%x addr=%s", ClientID, aAddrStr);
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
				m_aClients[ClientID].m_State = CClient::STATE_INGAME;
				SendServerInfo(m_NetServer.ClientAddr(ClientID), -1);
				GameServer()->OnClientEnter(ClientID);
			}
		}
		else if(Msg == PROTO_S(NETMSG_INPUT))
		{
			CClient::CInput *pInput;
			int64 TagTime;

			m_aClients[ClientID].m_LastAckedSnapshot = Unpacker.GetInt();
			int IntendedTick = Unpacker.GetInt();
			int Size = Unpacker.GetInt();

			// check for errors
			if(Unpacker.Error() || Size/4 > MAX_INPUT_SIZE)
				return;

			if(m_aClients[ClientID].m_LastAckedSnapshot > 0)
				m_aClients[ClientID].m_SnapRate = CClient::SNAPRATE_FULL;

			if(m_aClients[ClientID].m_Snapshots.Get(m_aClients[ClientID].m_LastAckedSnapshot, &TagTime, 0, 0) >= 0)
				m_aClients[ClientID].m_Latency = (int)(((time_get()-TagTime)*1000)/time_freq());

			// add message to report the input timing
			// skip packets that are old
			if(IntendedTick > m_aClients[ClientID].m_LastInputTick)
			{
				int TimeLeft = ((TickStartTime(IntendedTick)-time_get())*1000) / time_freq();

				CMsgPacker Msg(NETMSG_INPUTTIMING);
				Msg.AddInt(IntendedTick);
				Msg.AddInt(TimeLeft);
				SendMsgEx(&Msg, 0, ClientID, true);
			}

			m_aClients[ClientID].m_LastInputTick = IntendedTick;

			pInput = &m_aClients[ClientID].m_aInputs[m_aClients[ClientID].m_CurrentInput];

			if(IntendedTick <= Tick())
				IntendedTick = Tick()+1;

			pInput->m_GameTick = IntendedTick;

			for(int i = 0; i < Size/4; i++)
				pInput->m_aData[i] = Unpacker.GetInt();

			mem_copy(m_aClients[ClientID].m_LatestInput.m_aData, pInput->m_aData, MAX_INPUT_SIZE*sizeof(int));

			m_aClients[ClientID].m_CurrentInput++;
			m_aClients[ClientID].m_CurrentInput %= 200;

			// call the mod with the fresh input data
			if(m_aClients[ClientID].m_State == CClient::STATE_INGAME)
				GameServer()->OnClientDirectInput(ClientID, m_aClients[ClientID].m_LatestInput.m_aData);
		}
		else if(Msg == NETMSG_RCON_CMD)
		{
			const char *pCmd = Unpacker.GetString();

			if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Unpacker.Error() == 0 )
			{
				if(m_aClients[ClientID].m_Authed)
				{
					MACRO_LUA_CALLBACK_RESULT_REF("OnRconCommand", Result, ClientID, pCmd)

					if(MACRO_LUA_RESULT_BOOL(Result, ! true, true))
					{
						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "ClientID=%d rcon='%s'", ClientID, pCmd);
						m_RconExecClientID = ClientID;
						Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBuf);
						Console()->SetAccessLevel(m_aClients[ClientID].m_Authed == AUTHED_ADMIN
												  ? IConsole::ACCESS_LEVEL_ADMIN
												  : m_aClients[ClientID].m_Authed == AUTHED_MOD
													? IConsole::ACCESS_LEVEL_MOD
													: m_aClients[ClientID].m_AccessLevel);
						Console()->ExecuteLineFlag(pCmd, CFGFLAG_SERVER, ClientID);
						Console()->SetAccessLevel(IConsole::ACCESS_LEVEL_ADMIN);
						m_RconExecClientID = IServer::RCON_CID_SERV;
					}
				}
				else
				{
					if(str_comp_nocase(pCmd, "crashmeplx") == 0)
						m_aClients[ClientID].m_ClientSupportFlags |= CClient::SUPPORTS_64P;
					if(str_comp_nocase(pCmd, "crashmeharder") == 0)
						m_aClients[ClientID].m_ClientSupportFlags |= CClient::SUPPORTS_128P;
					if(str_comp_nocase(pCmd, "fancyunicorn") == 0)
						m_aClients[ClientID].m_ClientSupportFlags |= CClient::SUPPORTS_NETGUI;
				}
			}
		}
		else if(Msg == NETMSG_RCON_AUTH)
		{
			const char *pPw;
			Unpacker.GetString(); // login name, not used
			pPw = Unpacker.GetString(CUnpacker::SANITIZE_CC);

			#define SEND_CLIENT_AUTHED() \
					if(Is07(ClientID)) \
					{ \
						CMsgPacker Msg(proto07::NETMSG_RCON_AUTH_ON, true); \
						SendMsg(&Msg, MSGFLAG_VITAL, ClientID); \
					} \
					else \
					{ \
						CMsgPacker Msg(proto06::NETMSG_RCON_AUTH_STATUS, true); \
						Msg.AddInt(1);	/*authed*/ \
						Msg.AddInt(1);	/*cmdlist*/ \
						SendMsg(&Msg, MSGFLAG_VITAL, ClientID); \
					}

			#define ACCEPT_AUTH_ADMIN() \
					SEND_CLIENT_AUTHED() \
					m_aClients[ClientID].m_Authed = AUTHED_ADMIN; \
					m_aClients[ClientID].m_AccessLevel = IConsole::ACCESS_LEVEL_ADMIN; \
					int SendRconCmds = Is07(ClientID) ? true : Unpacker.GetInt(); \
					if(Unpacker.Error() == 0 && SendRconCmds) \
						m_aClients[ClientID].m_pRconCmdToSend = Console()->FirstCommandInfo(IConsole::ACCESS_LEVEL_ADMIN, CFGFLAG_SERVER); \
					SendRconLine(ClientID, "Admin authentication successful. Full remote console access granted."); \
					Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "server", "ClientID=%d authed (admin)", ClientID);


			#define ACCEPT_AUTH_MOD() \
					SEND_CLIENT_AUTHED() \
					m_aClients[ClientID].m_Authed = AUTHED_MOD; \
					m_aClients[ClientID].m_AccessLevel = IConsole::ACCESS_LEVEL_MOD; \
					int SendRconCmds = Is07(ClientID) ? true : Unpacker.GetInt(); \
					if(Unpacker.Error() == 0 && SendRconCmds) \
						m_aClients[ClientID].m_pRconCmdToSend = Console()->FirstCommandInfo(IConsole::ACCESS_LEVEL_MOD, CFGFLAG_SERVER); \
					SendRconLine(ClientID, "Moderator authentication successful. Limited remote console access granted."); \
					Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "server", "ClientID=%d authed (moderator)", ClientID);


			#define REJECT_AUTH() \
					m_aClients[ClientID].m_AuthTries++; \
					char aBuf[128]; \
					str_format(aBuf, sizeof(aBuf), "Wrong password %d/%d.", m_aClients[ClientID].m_AuthTries, g_Config.m_SvRconMaxTries); \
					SendRconLine(ClientID, aBuf); \
					if(m_aClients[ClientID].m_AuthTries >= g_Config.m_SvRconMaxTries) \
					{ \
						if(!g_Config.m_SvRconBantime) \
							m_NetServer.Drop(ClientID, "Too many remote console authentication tries"); \
						else \
							m_ServerBan.BanAddr(m_NetServer.ClientAddr(ClientID), g_Config.m_SvRconBantime*60, "Too many remote console authentication tries"); \
					}


			if(Unpacker.Error() == 0 && (pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0)
			{
				MACRO_LUA_CALLBACK_RESULT_V_REF("OnRconAuth", Result, ClientID, pPw);
				if(_LUA_EVENT_HANDLED)
				{
					if(Result.isBoolean())
					{
						// true / false = everything xor nothing = admin xor reject
						if(Result.cast<bool>())
						{
							ACCEPT_AUTH_ADMIN()
						}
						else
						{
							// suppose wrong password if the auth try should be rejected
							if(g_Config.m_SvRconMaxTries)
							{
								REJECT_AUTH()
							}
							else
							{
								SendRconLine(ClientID, "Access denied.");
							}
						}
					}
					else if(Result.isNumber())
					{
						// if the callback returns a number, we set the auth level to that number
						int AccessLevel = (int)Result;

						if(AccessLevel == IConsole::ACCESS_LEVEL_ADMIN)
						{
							ACCEPT_AUTH_ADMIN()
						}
						else if(AccessLevel == IConsole::ACCESS_LEVEL_MOD)
						{
							ACCEPT_AUTH_MOD()
						}
						else
						{
							bool SendRconCmds = Unpacker.GetInt() != 0;
							SendRconCmds = SendRconCmds && !Unpacker.Error();
							SetClientAccessLevel(ClientID, AccessLevel, SendRconCmds);
						}
					}
				}
				else if(g_Config.m_SvRconPassword[0] == 0 && g_Config.m_SvRconModPassword[0] == 0)
				{
					SendRconLine(ClientID, "No rcon password set on server. Set sv_rcon_password and/or sv_rcon_mod_password to enable the remote console.");
				}
				else if(g_Config.m_SvRconPassword[0] && str_comp(pPw, g_Config.m_SvRconPassword) == 0)
				{
					ACCEPT_AUTH_ADMIN()
				}
				else if(g_Config.m_SvRconModPassword[0] && str_comp(pPw, g_Config.m_SvRconModPassword) == 0)
				{
					ACCEPT_AUTH_MOD()
				}
				else if(g_Config.m_SvRconMaxTries)
				{
					REJECT_AUTH()
				}
				else
				{
					SendRconLine(ClientID, "Wrong password.");
				}
			}

		}
		else if(Msg == PROTO_S(NETMSG_PING))
		{
			CMsgPacker Msg(PROTO_S(NETMSG_PING_REPLY), true);
			SendMsg(&Msg, 0, ClientID);
		}
		else
		{
			if(g_Config.m_Debug)
			{
				char aHex[] = "0123456789ABCDEF";
				char aBuf[512];

				for(int b = 0; b < pPacket->m_DataSize && b < 32; b++)
				{
					aBuf[b*3] = aHex[((const unsigned char *)pPacket->m_pData)[b]>>4];
					aBuf[b*3+1] = aHex[((const unsigned char *)pPacket->m_pData)[b]&0xf];
					aBuf[b*3+2] = ' ';
					aBuf[b*3+3] = 0;
				}

				char aBufMsg[256];
				str_format(aBufMsg, sizeof(aBufMsg), "strange message ClientID=%d msg=%d data_size=%d", ClientID, Msg, pPacket->m_DataSize);
				Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBufMsg);
				Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);
			}
		}
	}
	else
	{
		// game message
		if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && m_aClients[ClientID].m_State >= CClient::STATE_READY)
			GameServer()->OnMessage(Msg, &Unpacker, ClientID);
	}
}

void CServer::GenerateServerInfo07(CPacker *pPacker, int Token)
{
	// count the players
	int PlayerCount = 0, ClientCount = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			if(GameServer()->IsClientPlayer(i))
				PlayerCount++;

			ClientCount++;
		}
	}

	if(Token != -1)
	{
		pPacker->Reset();
		pPacker->AddRaw(SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO));
		pPacker->AddInt(Token);
	}

	pPacker->AddString(GameServer()->Version(), 32);
	pPacker->AddString(g_Config.m_SvName, 64);
	pPacker->AddString(g_Config.m_SvHostname, 128);
	pPacker->AddString(GetMapName(), 32);

	// gametype
	pPacker->AddString(GameServer()->GameType(), 16);

	// flags
	int Flag = g_Config.m_Password[0] ? proto07::SERVERINFO_FLAG_PASSWORD : 0;	// password set
	pPacker->AddInt(Flag);

	pPacker->AddInt(g_Config.m_SvSkillLevel);	// server skill level
	pPacker->AddInt(PlayerCount); // num players
//	pPacker->AddInt(g_Config.m_SvPlayerSlots); // max players
	pPacker->AddInt(g_Config.m_SvMaxClients - g_Config.m_SvSpectatorSlots); // max players
	pPacker->AddInt(ClientCount); // num clients
	pPacker->AddInt(m_NetServer.MaxClients()); // max clients

	if(Token != -1)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aClients[i].m_State != CClient::STATE_EMPTY)
			{
				pPacker->AddString(ClientName(i), MAX_NAME_LENGTH); // client name
				pPacker->AddString(ClientClan(i), MAX_CLAN_LENGTH); // client clan
				pPacker->AddInt(m_aClients[i].m_Country); // client country
				pPacker->AddInt(m_aClients[i].m_Score); // client score
				pPacker->AddInt(GameServer()->IsClientPlayer(i)?0:1); // flag spectator=1, bot=2 (player=0)
			}
		}
	}
}

void CServer::SendServerInfo07(int ClientID)
{
	CMsgPacker Msg(proto07::NETMSG_SERVERINFO, true);
	GenerateServerInfo07(&Msg, -1);
	if(ClientID == -1)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aClients[i].m_State != CClient::STATE_EMPTY)
				SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, i);
		}
	}
	else if(ClientID >= 0 && ClientID < MAX_CLIENTS && m_aClients[ClientID].m_State != CClient::STATE_EMPTY)
		SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH, ClientID);
}

void CServer::SendServerInfo06(const NETADDR *pAddr, int Token, int InfoType, int Offset)
{
	CNetChunk Packet;
	CPacker p;
	char aBuf[128];

	// count the players
	int PlayerCount = 0, ClientCount = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			if(GameServer()->IsClientPlayer(i))
				PlayerCount++;

			ClientCount++;
		}
	}

	p.Reset();

	switch(InfoType)
	{
		case SRVINFO_VANILLA_TW06: p.AddRaw(SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)); break;
		case SRVINFO_EXTENDED_64: p.AddRaw(SERVERBROWSE_INFO64, sizeof(SERVERBROWSE_INFO64)); break;
		case SRVINFO_EXTENDED_128: p.AddRaw(SERVERBROWSE_INFO128, sizeof(SERVERBROWSE_INFO128)); break;
		default: dbg_assert(false, "CServer::SendServerInfo invalid value for 'InfoType'");
	}

	str_format(aBuf, sizeof(aBuf), "%d", Token);
	p.AddString(aBuf, 6);

	// version
	p.AddString(GameServer()->Version(), 32);

	// server name
	if(InfoType == SRVINFO_EXTENDED_128)
		p.AddString(g_Config.m_SvName, 256);
	else if(InfoType == SRVINFO_EXTENDED_64 && ClientCount < DDNET_MAX_CLIENTS)
		p.AddString(g_Config.m_SvName, 256);
	else if(InfoType == SRVINFO_VANILLA_TW06 && ClientCount < VANILLA_MAX_CLIENTS)
		p.AddString(g_Config.m_SvName, 64);
	else
	{
		str_formatb(aBuf, "%s   (%i/%i)", g_Config.m_SvName, ClientCount, m_NetServer.MaxClients());
		p.AddString(aBuf, InfoType == SRVINFO_EXTENDED_64 ? 256 : 64);
	}

	// map name
	p.AddString(GetMapName(), 32);

	// gametype
	p.AddString(GameServer()->GameType(), 16);

	// flags
	int i = 0;
	if(g_Config.m_Password[0]) // password set
		i |= proto06::SERVER_FLAG_PASSWORD;
	str_format(aBuf, sizeof(aBuf), "%d", i);
	p.AddString(aBuf, 2);

	int MaxClients = m_NetServer.MaxClients();
	if(InfoType == SRVINFO_VANILLA_TW06)
	{
		if(ClientCount >= VANILLA_MAX_CLIENTS)
		{
			if(ClientCount < MaxClients)
				ClientCount = VANILLA_MAX_CLIENTS - 1;
			else
				ClientCount = VANILLA_MAX_CLIENTS;
		}

		if(MaxClients > VANILLA_MAX_CLIENTS)
			MaxClients = VANILLA_MAX_CLIENTS;
	}
	else if(InfoType == SRVINFO_EXTENDED_64)
	{
		if(ClientCount >= DDNET_MAX_CLIENTS)
		{
			if(ClientCount < MaxClients)
				ClientCount = DDNET_MAX_CLIENTS - 1;
			else
				ClientCount = DDNET_MAX_CLIENTS;
		}

		if(MaxClients > DDNET_MAX_CLIENTS)
			MaxClients = DDNET_MAX_CLIENTS;
	}

	if(PlayerCount > ClientCount)
		PlayerCount = ClientCount;

	str_format(aBuf, sizeof(aBuf), "%d", PlayerCount); p.AddString(aBuf, 3); // num players
	str_format(aBuf, sizeof(aBuf), "%d", MaxClients-g_Config.m_SvSpectatorSlots); p.AddString(aBuf, 3); // max players
	str_format(aBuf, sizeof(aBuf), "%d", ClientCount); p.AddString(aBuf, 3); // num clients
	str_format(aBuf, sizeof(aBuf), "%d", MaxClients); p.AddString(aBuf, 3); // max clients

	if(InfoType != SRVINFO_VANILLA_TW06)
		p.AddInt(Offset);


	int ClientsPerPacket = InfoType != SRVINFO_VANILLA_TW06 ? 24 : VANILLA_MAX_CLIENTS;
	int Skip = Offset;
	int Take = ClientsPerPacket;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			if (Skip-- > 0)
				continue;
			if (--Take < 0)
				break;

			p.AddString(ClientName(i), MAX_NAME_LENGTH); // client name
			p.AddString(ClientClan(i), MAX_CLAN_LENGTH); // client clan
			str_format(aBuf, sizeof(aBuf), "%d", m_aClients[i].m_Country); p.AddString(aBuf, 6); // client country
			str_format(aBuf, sizeof(aBuf), "%d", m_aClients[i].m_Score); p.AddString(aBuf, 6); // client score
			str_format(aBuf, sizeof(aBuf), "%d", GameServer()->IsClientPlayer(i)?1:0); p.AddString(aBuf, 2); // is player?
		}
	}

	Packet.m_ClientID = -1;
	Packet.m_Address = *pAddr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = p.Size();
	Packet.m_pData = p.Data();
	m_NetServer.Send(&Packet);

	if(InfoType != SRVINFO_VANILLA_TW06 && Take < 0)
		SendServerInfo06(pAddr, Token, InfoType, Offset + ClientsPerPacket);
}

void CServer::UpdateServerInfo()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			if(Is07(i))
				SendServerInfo07(i);
			else
				SendServerInfo06(m_NetServer.ClientAddr(i), -1,
							   m_aClients[i].Supports(CClient::SUPPORTS_64P)
							   ? SRVINFO_EXTENDED_64
							   : m_aClients[i].Supports(CClient::SUPPORTS_128P)
								 ? SRVINFO_EXTENDED_128
								 : SRVINFO_VANILLA_TW06
			);
		}

	}
}


void CServer::PumpNetwork()
{
	CNetChunk Packet;
	TOKEN ResponseToken;

	m_NetServer.Update();

	// process packets
	while(m_NetServer.Recv(&Packet, &ResponseToken))
	{
		if(Packet.m_ClientID == -1)
		{
			// stateless
			if(!m_Register.RegisterProcessPacket(&Packet))
			{
				if(Packet.m_DataSize >= sizeof(SERVERBROWSE_GETINFO) &&
						mem_comp(Packet.m_pData, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO)) == 0)
				{
					// we've got no clue whether this is a 0.6 or 0.7 client so let's just send both
					SendServerInfo06(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO)]);

					// send 0.7
					CUnpacker Unpacker;
					Unpacker.Reset((unsigned char*)Packet.m_pData+sizeof(SERVERBROWSE_GETINFO), Packet.m_DataSize-sizeof(SERVERBROWSE_GETINFO));
					int SrvBrwsToken = Unpacker.GetInt();
					if(Unpacker.Error())
						continue;

					CPacker Packer;
					CNetChunk Response;

					GenerateServerInfo07(&Packer, SrvBrwsToken);

					Response.m_ClientID = -1;
					Response.m_Address = Packet.m_Address;
					Response.m_Flags = NETSENDFLAG_CONNLESS;
					Response.m_pData = Packer.Data();
					Response.m_DataSize = Packer.Size();
					m_NetServer.Send(&Response, ResponseToken);

				}
				else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO64)+1 &&
						mem_comp(Packet.m_pData, SERVERBROWSE_GETINFO64, sizeof(SERVERBROWSE_GETINFO64)) == 0)
				{
					SendServerInfo06(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO64)], SRVINFO_EXTENDED_64);
				}
				else if(Packet.m_DataSize == sizeof(SERVERBROWSE_GETINFO128)+1 &&
						mem_comp(Packet.m_pData, SERVERBROWSE_GETINFO128, sizeof(SERVERBROWSE_GETINFO128)) == 0)
				{
					SendServerInfo06(&Packet.m_Address, ((unsigned char *)Packet.m_pData)[sizeof(SERVERBROWSE_GETINFO128)], SRVINFO_EXTENDED_128);
				}
			}
		}
		else
			ProcessClientPacket(&Packet);
	}

	m_ServerBan.Update();
	m_Econ.Update();
}

char *CServer::GetMapName()
{
	// get the name of the map without his path
	char *pMapShortName = &g_Config.m_SvMap[0];
	for(int i = 0; i < str_length(g_Config.m_SvMap)-1; i++)
	{
		if(g_Config.m_SvMap[i] == '/' || g_Config.m_SvMap[i] == '\\')
			pMapShortName = &g_Config.m_SvMap[i+1];
	}
	return pMapShortName;
}

int CServer::LoadMap(const char *pMapName)
{
	//DATAFILE *df;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "maps/%s.map", pMapName);

	/*df = datafile_load(buf);
	if(!df)
		return 0;*/

	// check for valid standard map
	if(!m_MapChecker.ReadAndValidateMap(Storage(), aBuf, IStorage::TYPE_ALL))
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mapchecker", "invalid standard map");
		return 0;
	}

	if(!m_pMap->Load(aBuf))
		return 0;

	// stop recording when we change map
	m_DemoRecorder.Stop();

	// reinit snapshot ids
	m_IDPool.TimeoutIDs();

	// get the crc of the map
	m_CurrentMapCrc = m_pMap->Crc();
	char aBufMsg[256];
	str_format(aBufMsg, sizeof(aBufMsg), "%s crc is %08x", aBuf, m_CurrentMapCrc);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "server", aBufMsg);

	str_copy(m_aCurrentMap, pMapName, sizeof(m_aCurrentMap));
	//map_set(df);

	// load complete map into memory for download
	{
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
		m_CurrentMapSize = (unsigned)io_length(File);
		if(m_pCurrentMapData)
			mem_free(m_pCurrentMapData);
		m_pCurrentMapData = (unsigned char *)mem_alloc(m_CurrentMapSize, 1);
		io_read(File, m_pCurrentMapData, m_CurrentMapSize);
		io_close(File);
	}
	return 1;
}

void CServer::InitRegister(CNetServer *pNetServer, IEngineMasterServer *pMasterServer, IConsole *pConsole)
{
	m_Register.Init(pNetServer, pMasterServer, pConsole);
}

int CServer::Run()
{
	//
	m_PrintCBIndex = Console()->RegisterPrintCallback(g_Config.m_ConsoleOutputLevel, SendRconLineAuthed, this);
	m_PrintToCBIndex = Console()->RegisterPrintToCallback(SendRconLineTo, this);

	// load map
	if(!LoadMap(g_Config.m_SvMap))
	{
		dbg_msg("server", "failed to load map. mapname='%s'", g_Config.m_SvMap);
		return 0;
	}

	m_MapChunksPerRequest = g_Config.m_SvMapDownloadSpeed;


	// start server
	NETADDR BindAddr;
	if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
	{
		// sweet!
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = g_Config.m_SvPort;
	}
	else
	{
		mem_zero(&BindAddr, sizeof(BindAddr));
		BindAddr.type = NETTYPE_ALL;
		BindAddr.port = g_Config.m_SvPort;
	}

	if(!m_NetServer.Open(BindAddr, &m_ServerBan, g_Config.m_SvMaxClients, g_Config.m_SvMaxClientsPerIP, 0))
	{
		dbg_msg("server", "couldn't open socket. port %d might already be in use", g_Config.m_SvPort);
		return 0;
	}

	m_NetServer.SetCallbacks(NewClientCallback, DelClientCallback, this);

	m_Econ.Init(Console(), &m_ServerBan);

	if(!GameServer()->OnInit())
		return 0;

	Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "server", "server name is '%s'", g_Config.m_SvName);
	Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "server", "version %s", GameServer()->NetVersion());

	// process pending commands
	m_pConsole->StoreCommands(false);

	// start game
	{
		int64 ReportTime = time_get();
		int ReportInterval = 3;

		m_Lastheartbeat = 0;
		m_GameStartTime = time_get();

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "baseline memory usage %dk", mem_stats()->allocated/1024);
		Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "server", aBuf);

		while(m_RunServer == SERVER_RUNNING)
		{
			int64 t = time_get();
			int NewTicks = 0;

			// load new map
			if(m_MapReload)
			{
				m_MapReload = 0;

				// load map
				if(LoadMap(g_Config.m_SvMap))
				{
					// new map loaded
					GameServer()->OnShutdown();

					for(int c = 0; c < MAX_CLIENTS; c++)
					{
						if(m_aClients[c].m_State <= CClient::STATE_AUTH)
							continue;

						SendMap(c);
						m_aClients[c].Reset();
						m_aClients[c].m_State = CClient::STATE_CONNECTING;
					}

					m_GameStartTime = time_get();
					m_CurrentGameTick = 0;
					Kernel()->ReregisterInterface(GameServer());
					GameServer()->OnInit();
					UpdateServerInfo();
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "failed to load map. mapname='%s'", g_Config.m_SvMap);
					Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
					str_copy(g_Config.m_SvMap, m_aCurrentMap, sizeof(g_Config.m_SvMap));
				}
			}

			if(m_LuaReinit)
			{
				int ID = m_LuaReinit-1;
				m_LuaReinit = 0;
				CLua::Lua()->ReloadSingleObject(ID);
			}

			// main loop
			while(t > TickStartTime(m_CurrentGameTick+1))
			{
				m_CurrentGameTick++;
				NewTicks++;

				// apply new input
				for(int c = 0; c < MAX_CLIENTS; c++)
				{
					if(m_aClients[c].m_State == CClient::STATE_EMPTY)
						continue;
					for(int i = 0; i < 200; i++)
					{
						if(m_aClients[c].m_aInputs[i].m_GameTick == Tick())
						{
							if(m_aClients[c].m_State == CClient::STATE_INGAME)
								GameServer()->OnClientPredictedInput(c, m_aClients[c].m_aInputs[i].m_aData);
							break;
						}
					}
				}

				GameServer()->OnTick();
			}

			// snap game
			if(NewTicks)
			{
				if(g_Config.m_SvHighBandwidth || (m_CurrentGameTick%2) == 0)
					DoSnapshot();

				UpdateClientRconCommands();
			}

			// master server stuff
			m_Register.RegisterUpdate(m_NetServer.NetType());

			PumpNetwork();

			if(ReportTime < time_get())
			{
				if(g_Config.m_Debug)
				{
					/*
					static NETSTATS prev_stats;
					NETSTATS stats;
					netserver_stats(net, &stats);

					perf_next();

					if(config.dbg_pref)
						perf_dump(&rootscope);

					dbg_msg("server", "send=%8d recv=%8d",
						(stats.send_bytes - prev_stats.send_bytes)/reportinterval,
						(stats.recv_bytes - prev_stats.recv_bytes)/reportinterval);

					prev_stats = stats;
					*/
				}

				ReportTime += time_freq()*ReportInterval;
			}

			// wait for incomming data
			net_socket_read_wait(m_NetServer.Socket(), 5);
		}
	}

	if(m_RunServer == SERVER_SHUTDOWN)
	{
		// disconnect all clients on shutdown
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(m_aClients[i].m_State != CClient::STATE_EMPTY)
				m_NetServer.Drop(i, m_aShutdownReason);
		}
	}
	else if(m_RunServer == SERVER_REBOOT)
	{
		// reconnect all clients on shutdown
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(m_aClients[i].m_State != CClient::STATE_EMPTY)
				SendMap(i);
		}
	}

	m_NetServer.Close();

	m_Econ.Shutdown();

	GameServer()->OnShutdown();
	m_pMap->Unload();

	if(m_pCurrentMapData)
		mem_free(m_pCurrentMapData);

	return m_RunServer == SERVER_REBOOT ? 1 : 0;
}

void CServer::ConKick(IConsole::IResult *pResult, void *pUser)
{
	if(pResult->NumArguments() > 1)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Kicked (%s)", pResult->GetString(1));
		((CServer *)pUser)->Kick(pResult->GetInteger(0), aBuf);
	}
	else
		((CServer *)pUser)->Kick(pResult->GetInteger(0), "Kicked by console");
}

void CServer::ConStatus(IConsole::IResult *pResult, void *pUser)
{
	char aBuf[1024];
	char aAddrStr[NETADDR_MAXSTRSIZE];
	CServer* pThis = static_cast<CServer *>(pUser);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(pThis->m_aClients[i].m_State != CClient::STATE_EMPTY)
		{
			net_addr_str(pThis->m_NetServer.ClientAddr(i), aAddrStr, sizeof(aAddrStr), true);
			if(pThis->m_aClients[i].m_State == CClient::STATE_INGAME)
			{
				char aAuthStr[32];
				if(pThis->m_aClients[i].m_Authed == CServer::AUTHED_ADMIN)
					str_copyb(aAuthStr, "(Admin)");
				else if(pThis->m_aClients[i].m_Authed == CServer::AUTHED_MOD)
					str_copyb(aAuthStr, "(Mod)");
				else
					str_formatb(aAuthStr, "(Level %i)", pThis->m_aClients[i].m_AccessLevel);
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%s name='%s' score=%d %s", i, aAddrStr,
						   pThis->m_aClients[i].m_aName, pThis->m_aClients[i].m_Score, aAuthStr);
			}
			else
				str_format(aBuf, sizeof(aBuf), "id=%d addr=%s connecting", i, aAddrStr);
			pThis->Console()->PrintTo(pThis->m_RconExecClientID, "Server", aBuf);
		}
	}
}

void CServer::ConShutdown(IConsole::IResult *pResult, void *pUser)
{
	CServer *pSelf = ((CServer *)pUser);

	pSelf->m_RunServer = SERVER_SHUTDOWN;
	if(pResult->NumArguments() > 0)
	{
		str_appendb(pSelf->m_aShutdownReason, ": ");
		str_appendb(pSelf->m_aShutdownReason, pResult->GetString(0));
	}
}

void CServer::ConReboot(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_RunServer = SERVER_REBOOT;
}

void CServer::DemoRecorder_HandleAutoStart()
{
	if(g_Config.m_SvAutoDemoRecord)
	{
		m_DemoRecorder.Stop();
		char aFilename[128];
		char aDate[20];
		str_timestamp(aDate, sizeof(aDate));
		str_format(aFilename, sizeof(aFilename), "demos/%s_%s.demo", "auto/autorecord", aDate);
		m_DemoRecorder.Start(Storage(), m_pConsole, aFilename, GameServer()->NetVersion(), m_aCurrentMap, m_CurrentMapCrc, "server");
		if(g_Config.m_SvAutoDemoMax)
		{
			// clean up auto recorded demos
			CFileCollection AutoDemos;
			AutoDemos.Init(Storage(), "demos/server", "autorecord", ".demo", g_Config.m_SvAutoDemoMax);
		}
	}
}

bool CServer::DemoRecorder_IsRecording()
{
	return m_DemoRecorder.IsRecording();
}

void CServer::ConRecord(IConsole::IResult *pResult, void *pUser)
{
	CServer* pServer = (CServer *)pUser;
	char aFilename[128];

	if(pResult->NumArguments())
		str_format(aFilename, sizeof(aFilename), "demos/%s.demo", pResult->GetString(0));
	else
	{
		char aDate[20];
		str_timestamp(aDate, sizeof(aDate));
		str_format(aFilename, sizeof(aFilename), "demos/demo_%s.demo", aDate);
	}
	pServer->m_DemoRecorder.Start(pServer->Storage(), pServer->Console(), aFilename, pServer->GameServer()->NetVersion(), pServer->m_aCurrentMap, pServer->m_CurrentMapCrc, "server");
}

void CServer::ConStopRecord(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_DemoRecorder.Stop();
}

void CServer::ConMapReload(IConsole::IResult *pResult, void *pUser)
{
	((CServer *)pUser)->m_MapReload = 1;
}

int CServer::LuaConLuaDoStringPrintOverride(lua_State *L)
{
	CServer *pSelf = (CServer *)lua_touserdata(L, lua_upvalueindex(1));

	int nargs = lua_gettop(L);
	if(!nargs)
		return luaL_error(L, "print expects at least 1 argument");

	for(int i = 1; i <= nargs; i++)
	{
		LuaRef Val = luabridge::LuaRef::fromStack(L, i);
		pSelf->Console()->PrintfTo(pSelf->m_RconExecClientID, "lua", "(print %i/%i):  %s", i, nargs, Val.tostring().c_str());
	}

	return 0;
}

void CServer::ConLuaDoString(IConsole::IResult *pResult, void *pUser)
{
	CServer *pSelf = ((CServer *)pUser);
	lua_State *L = CLua::Lua()->L();

	// store the original print function
/*	lua_getregistry(L);
	lua_pushstring(L, "luaserver:print-backup");
	lua_getglobal(L, "print");
	lua_rawset(L, -3);*/
	/* registry is still on the stack top */

	// override the print function with our own (passing the pointer to our CServer as an upvalue)
	lua_pushlightuserdata(L, pSelf);
	lua_pushcclosure(L, CServer::LuaConLuaDoStringPrintOverride, 1);
	lua_setglobal(L, "print");

	// execute!
	pSelf->Console()->PrintfTo(pResult->GetCID(), "lua", "> %s", pResult->GetString(0));
	int oldtop = lua_gettop(L);
	int Success = luaL_dostring(L, pResult->GetString(0)) == 0;
	int newtop = lua_gettop(L);
	if(Success)
	{
		for(int i = oldtop+1; i <= newtop; i++)
		{
			LuaRef Val = luabridge::LuaRef::fromStack(L, i);
			pSelf->Console()->PrintfTo(pResult->GetCID(), "lua", "<- %2i:  %s", i - oldtop, Val.tostring().c_str());
		}
	}
	else
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua", "ERROR: %s", lua_tostring(L, -1));
	lua_pop(L, newtop-oldtop);

	// restore the old print
	lua_register(L, "print", CLuaBinding::Print);
}

void CServer::ConLuaStatus(IConsole::IResult *pResult, void *pUser)
{
	CServer *pSelf = ((CServer *)pUser);
	lua_State *L = CLua::Lua()->L();

	struct StatusCommand
	{
		const char * const pCommand;
		const char * const pArgList;
		const char * const pHelp;
		std::function<void()> pfnCallback;
	} aCommands[] = {
			{
				"help",
				"",
				"shows this help",
				[&](){
					if(pResult->NumArguments() == 1)
					{
						pSelf->Console()->PrintTo(pResult->GetCID(), "lua_status/help", "Available commands: help, gc");
						pSelf->Console()->PrintTo(pResult->GetCID(), "lua_status/help", "ONLY USE FOR DEBUGGING AND IF YOU KNOW WHAT YOU ARE DOING");
					}
					else
					{
						// help to a specific command
						for(int i = 0; i < 2 /* XXX  increase this number when more commands are added */; i++)
						{
							if(str_comp_nocase(pResult->GetString(1), aCommands[i].pCommand) == 0)
							{
								pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status/help", "Command '%s': %s (possible arguments: %s)", aCommands[i].pCommand, aCommands[i].pHelp, aCommands[i].pArgList[0] != '\0' ? aCommands[i].pArgList : "<none>");
								return;
							}
						}
						pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status/help", "Command '%s' not found", pResult->GetString(1));
					}
				}
			},
			{
				"gc",
				"stop / restart / collect / step",
				"controls the garbage collector",
				[&](){
					if(str_comp_nocase(pResult->GetString(1), "stop") == 0)
					{
						lua_gc(L, LUA_GCSTOP, 0);
						pSelf->Console()->PrintTo(pResult->GetCID(), "lua_status/gc", "garbage collector stopped!");
					}
					else if(str_comp_nocase(pResult->GetString(1), "restart") == 0)
					{
						lua_gc(L, LUA_GCRESTART, 0);
						pSelf->Console()->PrintTo(pResult->GetCID(), "lua_status/gc", "garbage collector restarted!");
					}
					else if(str_comp_nocase(pResult->GetString(1), "collect") == 0)
					{
						int MemBefore = lua_gc(L, LUA_GCCOUNT, 0)*1024 + lua_gc(L, LUA_GCCOUNTB, 0);
						lua_gc(L, LUA_GCCOLLECT, 0);
						int MemAfter = lua_gc(L, LUA_GCCOUNT, 0)*1024 + lua_gc(L, LUA_GCCOUNTB, 0);
						pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status/gc", "full cycle performed; %i bytes collected, %i bytes (%i KB) in use", MemBefore-MemAfter, MemAfter, MemAfter/1024);
					}
					else if(str_comp_nocase(pResult->GetString(1), "step") == 0)
					{
						int MemBefore = lua_gc(L, LUA_GCCOUNT, 0)*1024 + lua_gc(L, LUA_GCCOUNTB, 0);
						bool CycleFinished = lua_gc(L, LUA_GCSTEP, 1) == 1;
						int MemAfter = lua_gc(L, LUA_GCCOUNT, 0)*1024 + lua_gc(L, LUA_GCCOUNTB, 0);
						pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status/gc", "did one gc step%s; %i bytes collected, %i bytes in use",
												   CycleFinished ? " which finished the current cycle" : "",
												   MemBefore-MemAfter, MemAfter);
					}
					else
					{
						if(pResult->GetString(1)[0] == '\0')
							pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status/gc", "the command 'gc' needs an argument; see 'lua_status help gc'", pResult->GetString(1));
						else
							pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status/gc", "invalid argument '%s'", pResult->GetString(1));
					}
				}
			}
	};

	if(pResult->NumArguments() > 0)
	{
		// debug command given
		bool Found = false;
		for(unsigned i = 0; i < sizeof(aCommands)/sizeof(aCommands[0]); i++)
		{
			if(str_comp_nocase(pResult->GetString(0), aCommands[i].pCommand) == 0)
			{
				aCommands[i].pfnCallback();
				Found = true;
				break;
			}
		}

		if(!Found)
			pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "Command '%s' not found", pResult->GetString(0));
	}
	else
	{
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "--- Lua engine status ---");
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "Loaded user classes: %i", CLua::Lua()->NumLoadedClasses());
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "Alive lua objects: %i", CLua::Lua()->NumLuaObjects());
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "Registered resources: %i", CLua::Lua()->GetResMan()->GetCount());
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "VM Memory Usage: %i,%i KB+b", lua_gc(L, LUA_GCCOUNT, 0), lua_gc(L, LUA_GCCOUNTB, 0));
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "Unprotected stack size: %i", lua_gettop(L));
		luaL_dostring(L, "count = 0 for k,v in next,_G do count = count+1 end return count");
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_status", "State-global variables: %i", lua_tointeger(L, -1));
		lua_pop(L, 1);
	}

}

void CServer::ConLuaReinit(IConsole::IResult *pResult, void *pUser)
{
	CServer *pSelf = ((CServer *)pUser);

	int What = CLua::OBJ_ID_EVERYTHING; // everything
	if(pResult->NumArguments())
		What = pResult->GetInteger(0);

	if(What <= CLua::Lua()->NumLoadedClasses())
		pSelf->m_LuaReinit = What;
	else
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_reload", "ID out of range (choose 1..%i)", CLua::Lua()->NumLoadedClasses());
}

void CServer::ConLuaReinitQuick(IConsole::IResult *pResult, void *pUser)
{
	CServer *pSelf = ((CServer *)pUser);

	if(pResult->NumArguments() == 0)
	{
		pSelf->m_LuaReinit = CLua::OBJ_ID_EVERYTHING;
		return;
	}

	const char *pGivenHint = pResult->GetString(0);

	std::vector<int> Candidates;
	int LongestPrefixMatch = 1;

	// do everything nocase aswell, just in case we don't find anything using casecmp
	int LongestPrefixMatchNoCase = 1;
	std::vector<int> CandidatesNoCase;

	int Num = CLua::Lua()->NumLoadedClasses();
	for(int i = 1; i <= Num; i++)
	{
		const char *pObjName = CLua::Lua()->GetObjectName(i-1);
		if(str_comp(pObjName, pGivenHint) == 0)
		{
			// exact match, execute immediately
			pSelf->m_LuaReinit = i;
			if(g_Config.m_Debug)
				pSelf->Console()->PrintfTo(pResult->GetCID(), "lr", "exact match '%s' found", pObjName);
			return;
		}
		else if(int ThisPrefixMatch = str_comp_match_len(pObjName, pGivenHint) >= LongestPrefixMatch)
		{
			if(ThisPrefixMatch > LongestPrefixMatch)
			{
				// found a new longest prefix match; discard all the shorter candidates
				Candidates.clear();
				LongestPrefixMatch = ThisPrefixMatch;
			}

			Candidates.push_back(i);
		}
		else if(int ThisPrefixMatchNoCase = str_comp_match_len_nocase(pObjName, pGivenHint) >= LongestPrefixMatchNoCase)
		{
			if(ThisPrefixMatchNoCase > LongestPrefixMatchNoCase)
			{
				// found a new longest prefix match; discard all the shorter candidates
				CandidatesNoCase.clear();
				LongestPrefixMatchNoCase = ThisPrefixMatchNoCase;
			}

			CandidatesNoCase.push_back(i);
		}
	}

	// evaluate what we've got
	std::vector<int> *pUsedCandidates = &Candidates;
	if(pUsedCandidates->empty())
		pUsedCandidates = &CandidatesNoCase;

	if(pUsedCandidates->empty())
	{
		// got nothing at all
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lr", "no matches found for hint '%s'", pGivenHint);
		return;
	}
	else if(pUsedCandidates->size() == 1)
	{
		// got exactly one match, perfect!
		if(g_Config.m_Debug)
			pSelf->Console()->PrintfTo(pResult->GetCID(), "lr", "match found '%s' for given hint '%s'",
									   CLua::Lua()->GetObjectName((*pUsedCandidates)[0]-1), pGivenHint);
		pSelf->m_LuaReinit = (*pUsedCandidates)[0];
		return;
	}
	else
	{
		// got multiple matches; ask for further specification
		char aMatches[1024];
		aMatches[0] = '\0';
		std::vector<int>& UsedCandidates = *pUsedCandidates;
		for(std::vector<int>::iterator it = UsedCandidates.begin(); it != UsedCandidates.end(); ++it)
		{
			const char *pObjName = CLua::Lua()->GetObjectName(*it-1);
			str_append(aMatches, pObjName, sizeof(aMatches));
			str_append(aMatches, ", ", sizeof(aMatches));
		}
		aMatches[str_length(aMatches)-1 - 1] = '\0';
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lr", "found multiple candidates for hint '%s': %s", pGivenHint, aMatches);
		return;
	}

}

void CServer::ConLuaListClasses(IConsole::IResult *pResult, void *pUser)
{
	CServer *pSelf = ((CServer *)pUser);
	int ClientID = pResult->GetCID();

	pSelf->Console()->PrintTo(ClientID, "lua_listclasses", "----- Begin Loaded User Classes -----");

	int Num = CLua::Lua()->NumLoadedClasses();
	for(int i = 0; i < Num; i++)
	{
		std::string ObjIdent = CLua::Lua()->GetObjectIdentifier(i);
		pSelf->Console()->PrintfTo(pResult->GetCID(), "lua_listclasses", "%i: %s", i+1, ObjIdent.c_str());
	}

	pSelf->Console()->PrintTo(ClientID, "lua_listclasses", "-----  End Loaded User Classes  -----");
}

void CServer::ConDbgDumpIDMap(IConsole::IResult *pResult, void *pUser)
{
	CServer *pServer = (CServer *)pUser;
	int ClientID = pResult->GetCID();
	int MapOfID = pResult->NumArguments() > 0 ? pResult->GetInteger(0) : ClientID;

	IDMapT *aIDMap = pServer->GetIdMap(MapOfID);
	pServer->Console()->PrintfTo(ClientID, "debug", "------------------[ ID MAP OF %i ]-----------------------", MapOfID);
	for(int i = 0; i < DDNET_MAX_CLIENTS/2; i++)
	{
		pServer->Console()->PrintfTo(ClientID, "debug", "  %3i -> %2i        %3i -> %2i", aIDMap[i],i , aIDMap[i+DDNET_MAX_CLIENTS/2],i+DDNET_MAX_CLIENTS/2);
	}
	pServer->Console()->PrintTo(ClientID, "debug", "end ID map");
}

void CServer::ConLogout(IConsole::IResult *pResult, void *pUser)
{
	CServer *pServer = (CServer *)pUser;
	int ClientID = pResult->GetCID();

	if(ClientID >= 0 && ClientID < MAX_CLIENTS &&
		pServer->m_aClients[ClientID].m_State != CServer::CClient::STATE_EMPTY)
	{
		CMsgPacker Msg(NETMSG_RCON_AUTH_STATUS);
		Msg.AddInt(0);	//authed
		Msg.AddInt(0);	//cmdlist
		pServer->SendMsgEx(&Msg, MSGFLAG_VITAL, ClientID, true);

		pServer->m_aClients[ClientID].m_Authed = AUTHED_NO;
		pServer->m_aClients[ClientID].m_AuthTries = 0;
		pServer->m_aClients[ClientID].m_AccessLevel = IConsole::ACCESS_LEVEL_WORST;
		pServer->m_aClients[ClientID].m_pRconCmdToSend = 0;
		pServer->SendRconLine(ClientID, "Logout successful.");
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "ClientID=%d logged out", ClientID);
		pServer->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);
	}
}

void CServer::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->UpdateServerInfo();
}

void CServer::ConchainMapChange(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->m_MapReload = 1;
}

void CServer::ConchainMaxclientsperipUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CServer *)pUserData)->m_NetServer.SetMaxClientsPerIP(pResult->GetInteger(0));
}

void CServer::ConchainModCommandUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	if(pResult->NumArguments() == 2)
	{
		CServer *pThis = static_cast<CServer *>(pUserData);
		const IConsole::CCommandInfo *pInfo = pThis->Console()->GetCommandInfo(pResult->GetString(0), CFGFLAG_SERVER, false);
		int OldAccessLevel = 0;
		if(pInfo)
			OldAccessLevel = pInfo->GetAccessLevel();
		pfnCallback(pResult, pCallbackUserData);
		if(pInfo && OldAccessLevel != pInfo->GetAccessLevel())
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				if(pThis->m_aClients[i].m_State == CServer::CClient::STATE_EMPTY || pThis->m_aClients[i].m_Authed != CServer::AUTHED_MOD ||
					(pThis->m_aClients[i].m_pRconCmdToSend && str_comp(pResult->GetString(0), pThis->m_aClients[i].m_pRconCmdToSend->m_pName) >= 0))
					continue;

				if(OldAccessLevel == IConsole::ACCESS_LEVEL_ADMIN)
					pThis->SendRconCmdAdd(pInfo, i);
				else
					pThis->SendRconCmdRem(pInfo, i);
			}
		}
	}
	else
		pfnCallback(pResult, pCallbackUserData);
}

void CServer::ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 1)
	{
		CServer *pThis = static_cast<CServer *>(pUserData);
		pThis->Console()->SetPrintOutputLevel(pThis->m_PrintCBIndex, pResult->GetInteger(0));
	}
}

void CServer::RegisterCommands()
{
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pGameServer = Kernel()->RequestInterface<IGameServer>();
	m_pMap = Kernel()->RequestInterface<IEngineMap>();
	m_pStorage = Kernel()->RequestInterface<IStorage>();

	// register console commands
	Console()->Register("kick", "i?r", CFGFLAG_SERVER, ConKick, this, "Kick player with specified id for any reason");
	Console()->Register("status", "", CFGFLAG_SERVER, ConStatus, this, "List players");
	Console()->Register("shutdown", "?r", CFGFLAG_SERVER, ConShutdown, this, "Shut down");
	Console()->Register("reboot", "", CFGFLAG_SERVER, ConReboot, this, "Restart the server");
	Console()->Register("logout", "", CFGFLAG_SERVER, ConLogout, this, "Logout of rcon");

	Console()->Register("record", "?s", CFGFLAG_SERVER|CFGFLAG_STORE, ConRecord, this, "Record to a file");
	Console()->Register("stoprecord", "", CFGFLAG_SERVER, ConStopRecord, this, "Stop recording");

	Console()->Register("reload", "", CFGFLAG_SERVER, ConMapReload, this, "Reload the map");

	// lua
	Console()->Register("lua", "r", CFGFLAG_SERVER, ConLuaDoString, this, "Execute the given line of lua code");
	Console()->Register("lua_status", "?s?r", CFGFLAG_SERVER, ConLuaStatus, this, "Get status information about the lua engine or execute debug commands (try 'lua_status help')");
	Console()->Register("lua_reload", "?i", CFGFLAG_SERVER, ConLuaReinit, this, "Reload the specified lua class by ID (or all if none given) - see lua_listclasses");
	Console()->Register("lr", "?r", CFGFLAG_SERVER, ConLuaReinitQuick, this, "Reload the specified lua class (via partial string matching algorithm)");
	Console()->Register("lua_listclasses", "", CFGFLAG_SERVER, ConLuaListClasses, this, "View all loaded lua classes with their IDs");
	Console()->Register("lsc", "", CFGFLAG_SERVER, ConLuaListClasses, this, "View all loaded lua classes with their IDs (short version of lua_listclasses) ");

	// debug
	Console()->Register("debug_dump_id_map", "?i", CFGFLAG_SERVER, ConDbgDumpIDMap, this, "");

	Console()->Chain("sv_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("password", ConchainSpecialInfoupdate, this);

	Console()->Chain("sv_map", ConchainMapChange, this);
	Console()->Chain("sv_gametype", ConchainMapChange, this); // map change also reloads lua and everything

	Console()->Chain("sv_max_clients_per_ip", ConchainMaxclientsperipUpdate, this);
	Console()->Chain("mod_command", ConchainModCommandUpdate, this);
	Console()->Chain("console_output_level", ConchainConsoleOutputLevelUpdate, this);

	// register console commands in sub parts
	m_ServerBan.InitServerBan(Console(), Storage(), this);
	m_pGameServer->OnConsoleInit();
}


int CServer::SnapNewID()
{
	return m_IDPool.NewID();
}

void CServer::SnapFreeID(int ID)
{
	m_IDPool.FreeID(ID);
}


void *CServer::SnapNewItem(int Type, int ID, int Size)
{
	dbg_assert(Type >= 0 && Type <=0xffff, "incorrect type");
	dbg_assert(ID >= 0 && ID <=0xffff, "incorrect id");
	return ID < 0 ? 0 : m_SnapshotBuilder.NewItem(Type, ID, Size);
}

void CServer::SnapSetStaticsize(int ItemType, int Size)
{
	m_SnapshotDelta.SetStaticsize(ItemType, Size);
}

static CServer *CreateServer() { return new CServer(); }

int main(int argc, const char **argv) // ignore_convention
{
#if defined(CONF_FAMILY_WINDOWS)
	for(int i = 1; i < argc; i++) // ignore_convention
	{
		if(str_comp("-s", argv[i]) == 0 || str_comp("--silent", argv[i]) == 0) // ignore_convention
		{
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			break;
		}
	}
#endif

	CServer *pServer = CreateServer();
	IKernel *pKernel = IKernel::Create();

	// create the components
	IEngine *pEngine = CreateEngine("Teeworlds");
	IEngineMap *pEngineMap = CreateEngineMap();
	IGameServer *pGameServer = CreateGameServer();
	IConsole *pConsole = CreateConsole(CFGFLAG_SERVER|CFGFLAG_ECON);
	IEngineMasterServer *pEngineMasterServer = CreateEngineMasterServer();
	IStorage *pStorage = CreateStorage("TeeworldsLuaSrv", IStorage::STORAGETYPE_SERVER, argc, argv); // ignore_convention
	IConfig *pConfig = CreateConfig();
	ILua *pLua = CreateLua();

	pServer->InitRegister(&pServer->m_NetServer, pEngineMasterServer, pConsole);

	{
		bool RegisterFail = false;

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pServer); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pEngine);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMap*>(pEngineMap)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMap*>(pEngineMap));
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pGameServer);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConsole);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pStorage);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConfig);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pLua);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMasterServer*>(pEngineMasterServer)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMasterServer*>(pEngineMasterServer));

		if(RegisterFail)
			return -1;
	}

	pEngine->Init();
	pConfig->Init();
	pEngineMasterServer->Init();
	pEngineMasterServer->Load();

	// register all console commands
	pServer->RegisterCommands();

	// init lua
	pLua->FirstInit();

	// execute autoexec file
	pConsole->ExecuteFile("autoexec.cfg");

	// parse the command line arguments
	if(argc > 1) // ignore_convention
		pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention

	// restore empty config strings to their defaults
	pConfig->RestoreStrings();

	pEngine->InitLogfile();

	// run the server
	dbg_msg("server", "starting...");
	bool Restart = (bool)pServer->Run();

	// free
	delete pServer;
	delete pKernel;
	delete pEngineMap;
	delete pGameServer;
	delete pConsole;
	delete pEngineMasterServer;
	delete pStorage;
	delete pConfig;

	if(Restart)
		shell_execute(argv[0]);

	return 0;
}

