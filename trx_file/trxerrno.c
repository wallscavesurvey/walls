#include <a__trx.h>

int TRXAPI trx_Errno(TRX_NO trx)
{
	if (!_GETTRXP) return TRX_ErrArg;
	return _TRXP->Cf.Errno;
}
