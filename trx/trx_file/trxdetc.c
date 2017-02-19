#include <a__trx.h>

int TRXAPI trx_DetachCache(TRX_NO trx)
{
  return trx_errno=_csf_DetachCache((CSF_NO)_GETTRXP);
}
