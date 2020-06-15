#include "a__trx.h"

UINT TRXAPI trx_InitTreeFlags(TRX_NO trx)
{
	return _GETTRXT ? _TRXT->InitFlags : 0;
}
