#include "a__trx.h"

UINT TRXAPI trx_SizNode(TRX_NO trx)
{
  return _GETTRXP?_TRXP->Fd.SizNode:0;
}
