#include "a__trx.h"

NPSTR TRXAPI trx_PathName(TRX_NO trx)
{
	return _GETTRXP ? dos_PathName(_TRXP->Cf.Handle) : "";
}
