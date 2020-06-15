#include <a__trx.h>

TRX_CSH TRXAPI trx_Cache(TRX_NO trx)
{
	return _GETTRXP ? (TRX_CSH)_TRXP->Cf.Csh : 0;
}
