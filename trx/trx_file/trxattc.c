#include <a__trx.h>

int TRXAPI trx_AttachCache(TRX_NO trx,TRX_CSH csh)
{
  if(!_GETTRXP) return TRX_ErrArg;
  if(csh && _TRXP->Cf.Recsiz>((CSH_NO)csh)->Bufsiz) return trx_errno=TRX_ErrSizNode;
  return trx_errno=_csf_AttachCache((CSF_NO)_TRXP,(CSH_NO)csh);
}
