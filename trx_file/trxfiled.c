#include <a__trx.h>

LPVOID TRXAPI trx_FileDef(TRX_NO trx)
{
  return _GETTRXP?&_TRXP->Fd:NULL;
}
