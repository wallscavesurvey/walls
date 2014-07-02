#include <a__trx.h>

int TRXAPI trx_DeleteCache(TRX_NO trx)
{
  TRX_CSH csh;
  
  if(_GETTRXP) {
    csh=_TRXP->Cf.Csh;
    if(!csh) trx_errno=TRX_ErrNoCache;
    else {
      trx_errno=_csf_DeleteCache((CSF_NO)_TRXP,(CSH_NO *)&trx_defCache);
      if(!trx_errno && csh==trx_defCache) trx_defCache=0;
    }
  }
  return trx_errno;
}
