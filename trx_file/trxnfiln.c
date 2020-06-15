#include "a__trx.h"

DWORD TRXAPI trx_NumFileNodes(TRX_NO trx)
{
	return _GETTRXP ? _TRXP->NumNodes : 0L;
}
