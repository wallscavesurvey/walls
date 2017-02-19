#include "a__trx.h"

DWORD TRXAPI trx_NumTreeNodes(TRX_NO trx)
{
  return _GETTRXT?_TRXT->NumNodes:0L;
}
