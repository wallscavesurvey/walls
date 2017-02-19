#include "a__trx.h"

UINT TRXAPI trx_MinPreWrite(TRX_NO trx)
{
  return _GETTRXP?_TRXP->MinPreWrite:0;
}
