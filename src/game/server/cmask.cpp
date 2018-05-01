#include "cmask.h"

bool CmaskIsSet(const Cmask& Mask, int ClientID)
{
	CmaskOne MaskOne(ClientID);
	if(ClientID > 63)
		return (Mask.mask[1] & MaskOne.mask[1]) != 0;
	else
		return (Mask.mask[0] & MaskOne.mask[0]) != 0;
};
