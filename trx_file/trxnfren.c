#include "a__trx.h"

DWORD TRXAPI trx_NumFreeNodes(TRX_NO trx)
{
  return _GETTRXP?_TRXP->NumFreeNodes:0L;
}
