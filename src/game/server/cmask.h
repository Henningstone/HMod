#ifndef GAME_SERVER_CMASK_H
#define GAME_SERVER_CMASK_H

#include <base/system.h>

/* Cmask extends:
 * 0:  0 .. 31
 * 1: 32 .. 63
 * 2: 64 .. 95
 * 3: 96 ..127
 *
 * (the same but efficiently spaced out - not used):
 * 0:   0 ..  61
 * 1:  62 .. 123
 * 2: 124 .. 185
 * 3: 186 .. 247
 */

/*
inline uint64 CmaskExtendOne()   { return 0x4000000000000000; }
inline uint64 CmaskExtendTwo()   { return 0x8000000000000000; }
inline uint64 CmaskExtendThree() { return 0xC000000000000000; }
inline uint64 CmaskAll()         { return 0xFFFFffffFFFFffff; }
inline uint64 CmaskOne(int ClientID) {
	if(ClientID < 32)
		return 1ULL << (unsigned)ClientID;
	else if(ClientID < 64)
		return CmaskExtendOne() | 1ULL << (unsigned)ClientID-32;
	else if(ClientID < 96)
		return CmaskExtendTwo() | 1ULL << (unsigned)ClientID-64;
	else if(ClientID < 128)
		return CmaskExtendThree() | 1ULL << (unsigned)ClientID-96;
}
inline uint64 CmaskAllExceptOne(int ClientID) { return 0x7fffffff^CmaskOne(ClientID); }
inline bool CmaskIsSet(uint64 Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
*/
struct Cmask
{
	uint64 mask[2];

	Cmask operator|=(const Cmask other)
	{
		mask[0] |= other.mask[0];
		mask[1] |= other.mask[1];
		return *this;
	}

	Cmask() { mem_zerob(mask); }
};

struct CmaskOne : public Cmask
{
	CmaskOne(int ClientID) : Cmask()
	{
		if(ClientID > 63)
			mask[1] = 1ULL << (unsigned)(ClientID-64);
		else
			mask[0] = 1ULL << (unsigned)ClientID;
	}
};

struct CmaskAll : public Cmask
{
	CmaskAll() : Cmask()
	{
		mask[0] = 0xFFFFffffFFFFffff;
		mask[1] = 0xFFFFffffFFFFffff;
	}
};

struct CmaskAllExceptOne : public CmaskAll
{
	CmaskAllExceptOne(int ClientID) : CmaskAll()
	{
		CmaskOne MaskOne(ClientID);
		if(ClientID > 63)
			mask[1] ^= MaskOne.mask[1];
		else
			mask[0] ^= MaskOne.mask[0];
	}
};

bool CmaskIsSet(const Cmask& Mask, int ClientID);

#endif
