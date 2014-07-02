#include "a__trx.h"

DWORD TRXAPI trx_NumKeys(TRX_NO trx)
{
  if(_GETTRXT && _TRXT->SizKey) return _TRXT->NumKeys;
  if(_TRXT) trx_errno=TRX_ErrTreeType;
  return 0L;
}
