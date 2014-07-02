#include <a__trx.h>

NPSTR TRXAPI trx_FileName(TRX_NO trx)
{
  return _GETTRXP?dos_FileName(_TRXP->Cf.Handle):"";
}
