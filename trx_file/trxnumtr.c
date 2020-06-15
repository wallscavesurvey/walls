#include "a__trx.h"

int TRXAPI trx_NumTrees(TRX_NO trx)
{
	return _GETTRXP ? _TRXP->Fd.NumTrees : 0;
}
