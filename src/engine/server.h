/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_H
#define ENGINE_SERVER_H
#include "kernel.h"
#include "message.h"
#include <base/system++/system++.h>
#include <base/system++/typewrapper.h>
#include <engine/server/lua.h>
#include <engine/server/lua_class.h>
#include <game/generated/protocol.h>
#include <engine/shared/protocol.h>

//typedef std::map< int, TypeWrapper<int, -1>[VANILLA_MAX_CLIENTS] > IDMapT;
//typedef IDMapT& IDMap;
typedef TypeWrapper<int, -1> IDMapT;

class IServer : public IInterface, protected CLuaClass
{
	MACRO_INTERFACE("server", 0)
protected:
	int m_CurrentGameTick;
	int m_TickSpeed;

public:
	/*
		Structure: CClientInfo
	*/
	struct CClientInfo
	{
		const char *m_pName;
		int m_Latency;

		bool m_Is64;
		bool m_Is128;
	};

	IServer() : CLuaClass("Server") {}

	int Tick() const { return m_CurrentGameTick; }
	int TickSpeed() const { return m_TickSpeed; }

	virtual int MaxClients() const = 0;
	virtual const char *ClientName(int ClientID) = 0;
	virtual const char *ClientClan(int ClientID) = 0;
	virtual int ClientCountry(int ClientID) = 0;
	virtual bool ClientIngame(int ClientID) = 0;
	virtual bool ClientIsDummy(int ClientID) = 0;
	virtual int GetClientInfo(int ClientID, CClientInfo *pInfo) const = 0;
	virtual void GetClientAddr(int ClientID, char *pAddrStr, int Size) = 0;
	virtual std::string GetClientAddrLua(int ClientID) = 0;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID) = 0;
	virtual void SendRconLine(int ClientID, const char *pLine) = 0;

	/*template<class T> int SendPackMsg(T *pMsg, int Flags, int ClientID);
	template<class T> int SendPackMsgTranslate(T *pMsg, int Flags, int ClientID);
	template<class T> int SendPackMsgOne(T *pMsg, int Flags, int ClientID);
	int SendPackMsgTranslate(CNetMsg_Sv_Emoticon *pMsg, int Flags, int ClientID);
	int SendPackMsgTranslate(CNetMsg_Sv_Chat *pMsg, int Flags, int ClientID);
	int SendPackMsgTranslate(CNetMsg_Sv_KillMsg *pMsg, int Flags, int ClientID);*/

	virtual bool IDTranslate(int *pInOutTargetID, int ForClientID) const = 0;
	virtual bool IDTranslateReverse(int *pInOutTargetID, int ForClientID) const = 0;
	virtual int IDTranslateLua(int InternalID, int ForClientID) const = 0;
	virtual int IDTranslateReverseLua(int InternalID, int ForClientID) const = 0;

	virtual const IDMapT *GetIdMap(int ClientID) const = 0;
	virtual const IDMapT *GetRevMap(int ClientID) const = 0;
	virtual void WriteIdMap(int ClientID, int IdTakingSlot, int ChosenSlot) = 0;
	virtual void ResetIdMap(int ClientID) = 0;
	virtual int ResetIdMapSlotOf(int ClientID, int IdTakingSlot) = 0;
	virtual void DumpIdMap(int ClientID) const = 0;

	virtual void SetClientName(int ClientID, char const *pName) = 0;
	virtual void SetClientClan(int ClientID, char const *pClan) = 0;
	virtual void SetClientCountry(int ClientID, int Country) = 0;
	virtual void SetClientScore(int ClientID, int Score) = 0;
	virtual void SetClientAccessLevel(int ClientID, int Level, bool SendRconCmds) = 0;

	virtual void InitDummy(int ClientID) = 0;
	virtual void PurgeDummy(int ClientID) = 0;

	virtual int SnapNewID() = 0;
	virtual void SnapFreeID(int ID) = 0;
	virtual void *SnapNewItem(int Type, int ID, int Size) = 0;

	virtual void SnapSetStaticsize(int ItemType, int Size) = 0;

	enum
	{
		RCON_CID_SERV=-1,
		RCON_CID_VOTE=-2,
	};
	virtual void SetRconCID(int ClientID) = 0;
	virtual bool IsAuthed(int ClientID) = 0;
	virtual bool HasAccess(int ClientID, int AccessLevel) = 0;
	virtual void Kick(int ClientID, const char *pReason) = 0;

	virtual void DemoRecorder_HandleAutoStart() = 0;
	virtual bool DemoRecorder_IsRecording() = 0;

	// smart send function; automatically decides how to handle translation
	template <class T>
	void SendPackMsg(const T *pMsg, int Flags, int ClientID)
	{
		if (ClientID == -1)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(ClientIngame(i))
				{
					T MsgCopy;
					mem_copy(&MsgCopy, pMsg, sizeof(T));
					SendPackMsgTranslate(&MsgCopy, Flags, i);
				}
			}
		}
		else
		{
			T MsgCopy;
			mem_copy(&MsgCopy, pMsg, sizeof(T));
			SendPackMsgTranslate(&MsgCopy, Flags, ClientID);
		}
	}

private:

	// for generic messages that don't require translation
	template<class T>
	int SendPackMsgTranslate(T *pMsg, int Flags, int ClientID)
	{
		return SendPackMsgOne(pMsg, Flags, ClientID);
	}

	// emoticon
	int SendPackMsgTranslate(CNetMsg_Sv_Emoticon *pMsg, int Flags, int ClientID)
	{
		if(!IDTranslate(&pMsg->m_ClientID, ClientID))
			return 0;
		return SendPackMsgOne(pMsg, Flags, ClientID);
	}

	// chat message
	int SendPackMsgTranslate(CNetMsg_Sv_Chat *pMsg, int Flags, int ClientID)
	{
		if (pMsg->m_ClientID >= 0 && !IDTranslate(&pMsg->m_ClientID, ClientID))
		{
			CClientInfo Info;
			GetClientInfo(ClientID, &Info);

			char aMsgBuf[1024];
			str_format(aMsgBuf, sizeof(aMsgBuf), "%s: %s", ClientName(pMsg->m_ClientID), pMsg->m_pMessage);
			pMsg->m_pMessage = aMsgBuf;
			pMsg->m_ClientID = Info.m_Is64 ? FAKE_ID_DDNET : FAKE_ID_VANILLA;
		}
		return SendPackMsgOne(pMsg, Flags, ClientID);
	}

	// kill message
	int SendPackMsgTranslate(CNetMsg_Sv_KillMsg *pMsg, int Flags, int ClientID)
	{
		if(!IDTranslate(&pMsg->m_Victim, ClientID))
			return 0;
		if(!IDTranslate(&pMsg->m_Killer, ClientID))
			pMsg->m_Killer = pMsg->m_Victim;

		return SendPackMsgOne(pMsg, Flags, ClientID);
	}

	// pack a message and send it
	template<class T>
	int SendPackMsgOne(T *pMsg, int Flags, int ClientID)
	{
		CMsgPacker Packer(pMsg->MsgID());
		if(pMsg->Pack(&Packer))
			return -1;
		return SendMsg(&Packer, Flags, ClientID);
	}
};

class IGameServer : public IInterface
{
	MACRO_INTERFACE("gameserver", 0)
protected:
public:
	virtual bool OnInit() = 0;
	virtual void OnConsoleInit() = 0;
	virtual void OnShutdown() = 0;

	virtual void OnTick() = 0;
	virtual void OnPreSnap() = 0;
	virtual void OnSnap(int ClientID) = 0;
	virtual void OnPostSnap() = 0;

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID) = 0;

	virtual void OnClientConnected(int ClientID) = 0;
	virtual void OnClientEnter(int ClientID) = 0;
	virtual void OnClientDrop(int ClientID, const char *pReason) = 0;
	virtual void OnClientDirectInput(int ClientID, void *pInput) = 0;
	virtual void OnClientPredictedInput(int ClientID, void *pInput) = 0;

	virtual bool IsClientReady(int ClientID) = 0;
	virtual bool IsClientPlayer(int ClientID) = 0;

	virtual const char *GameType() = 0;
	virtual const char *Version() = 0;
	virtual const char *NetVersion() = 0;
};

extern IGameServer *CreateGameServer();
#endif
