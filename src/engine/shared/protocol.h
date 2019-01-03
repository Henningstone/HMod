#ifndef ENGINE_SHARED_PROTOCOL_H
#define ENGINE_SHARED_PROTOCOL_H

#include "protocol06.h"
#include "protocol07.h"


#define PROTO(USE07, WHAT) (USE07) ? proto07::WHAT : proto06::WHAT

enum
{
	NETMSG_NULL=0,

	// the first thing sent by the client
	// contains the version info for the client
	NETMSG_INFO=1,
};

// this should be revised
enum
{
	SERVER_TICK_SPEED=50,

	VANILLA_MAX_CLIENTS=16,
	DDNET_MAX_CLIENTS=64,
	EXTENDED_MAX_CLIENTS=128,
	MAX_CLIENTS=EXTENDED_MAX_CLIENTS,

	MAX_INPUT_SIZE=128,
	MAX_SNAPSHOT_PACKSIZE=900,

	MAX_NAME_LENGTH=16,
	MAX_CLAN_LENGTH=12,

	// message packing
	MSGFLAG_VITAL=1,
	MSGFLAG_FLUSH=2,
	MSGFLAG_NORECORD=4,
	MSGFLAG_RECORD=8,
	MSGFLAG_NOSEND=16
};

#endif
