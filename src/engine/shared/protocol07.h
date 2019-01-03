/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_PROTOCOL07_H
#define ENGINE_SHARED_PROTOCOL07_H

#include <base/system.h>

/*
	Connection diagram - How the initilization works.

	Client -> INFO -> Server
		Contains version info, name, and some other info.

	Client <- MAP <- Server
		Contains current map.

	Client -> READY -> Server
		The client has loaded the map and is ready to go,
		but the mod needs to send it's information aswell.
		modc_connected is called on the client and
		mods_connected is called on the server.
		The client should call client_entergame when the
		mod has done it's initilization.

	Client -> ENTERGAME -> Server
		Tells the server to start sending snapshots.
		client_entergame and server_client_enter is called.
*/

namespace proto07 {

enum
{
	// sent by server
	NETMSG_MAP_CHANGE,		// sent when client should switch map
	NETMSG_MAP_DATA,		// map transfer, contains a chunk of the map file
	NETMSG_SERVERINFO,
	NETMSG_CON_READY,		// connection is ready, client should send start info
	NETMSG_SNAP,			// normal snapshot, multiple parts
	NETMSG_SNAPEMPTY,		// empty snapshot
	NETMSG_SNAPSINGLE,		// ?
	NETMSG_SNAPSMALL,		//
	NETMSG_INPUTTIMING,		// reports how off the input was
	NETMSG_RCON_AUTH_ON,	// rcon authentication enabled
	NETMSG_RCON_AUTH_OFF,	// rcon authentication disabled
	NETMSG_RCON_LINE,		// line that should be printed to the remote console
	NETMSG_RCON_CMD_ADD,
	NETMSG_RCON_CMD_REM,

	NETMSG_AUTH_CHALLANGE,	//
	NETMSG_AUTH_RESULT,		//

	// sent by client
	NETMSG_READY,			//
	NETMSG_ENTERGAME,
	NETMSG_INPUT,			// contains the inputdata from the client
	NETMSG_RCON_CMD,		//
	NETMSG_RCON_AUTH,		//
	NETMSG_REQUEST_MAP_DATA,//

	NETMSG_AUTH_START,		//
	NETMSG_AUTH_RESPONSE,	//

	// sent by both
	NETMSG_PING,
	NETMSG_PING_REPLY,
	NETMSG_ERROR,
};

// this should be revised
enum
{
	SERVERINFO_FLAG_PASSWORD = 0x1,
	SERVERINFO_LEVEL_MIN=0,
	SERVERINFO_LEVEL_MAX=2,
};

}

#endif
