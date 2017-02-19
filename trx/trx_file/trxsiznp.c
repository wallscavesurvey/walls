#include "a__trx.h"

int TRXAPI trx_SizNodePtr(TRX_NO trx)
{
  return _GETTRXP?_TRXP->Fd.SizNodePtr:0;
}
