#include "a__trx.h"

int TRXAPI trx_SizKey(TRX_NO trx)
{
  return _GETTRXT?_TRXT->SizKey:0;
}
