#include "a__trx.h"

int TRXAPI trx_SizRec(TRX_NO trx)
{
  return _GETTRXT?_TRXT->SizRec:0;
}
