#include <a__trx.h>

/*NOTE: Provisional unrestricted access to extra data via pointer.
  Read-only users can modify the contents even though an error
  is returned from trx_MarkExtra(). Non read-only users
  can flush the header.*/

PVOID TRXAPI trx_ExtraPtr(TRX_NO trx)
{
  return (_GETTRXP && _TRXP->Fd.SizExtra) ?
    ((PBYTE)_TRXP+sizeof(TRX_FILE)+_TRXP->Fd.NumTrees*sizeof(TRX_TREEVAR)):0;
}

UINT TRXAPI trx_SizExtra(TRX_NO trx)
{
  return _GETTRXP ? _TRXP->Fd.SizExtra : 0;
}

int TRXAPI trx_MarkExtra(TRX_NO trx)
{
  if(!_GETTRXP) return TRX_ErrArg;
  if((_trx_pFileUsr->UsrMode&TRX_ReadOnly)!=0) return TRX_ErrReadOnly;
  _TRXP->HdrChg=TRUE;
  return _trx_ErrFlushHdr();
}
