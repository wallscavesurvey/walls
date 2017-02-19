#include "a__trx.h"

DWORD TRXAPI trx_NumRecs(TRX_NO trx)
{
  return _GETTRXT?_TRXT->NumKeys:0L;
}
