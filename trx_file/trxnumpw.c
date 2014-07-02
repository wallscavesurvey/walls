#include "a__trx.h"

UINT TRXAPI trx_NumPreWrite(TRX_NO trx)
{
  return _GETTRXP?_TRXP->NumPreWrite:0;
}
