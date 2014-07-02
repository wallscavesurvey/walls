#include "a__trx.h"

DWORD TRXAPI trx_SizFile(TRX_NO trx)
{
  return _GETTRXP?((_TRXP->NumNodes+_TRXP->NumPreWrite)*
    _TRXP->Fd.SizNode+_TRXP->Cf.Offset):0L;
}
