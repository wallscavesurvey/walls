#include "a__trx.h"

int TRXAPI trx_InitTree(TRX_NO trx,UINT sizRec,UINT sizKey,UINT initFlags)
{
  TRX_pTREEVAR trxt;
  UINT nodespace,minsizkey;

  if(sizKey>TRX_MAX_KEYLEN || !_GETTRXP) goto _err_arg;
  trxt=_trx_pTreeVar;

  if(_trx_pFileUsr->UsrMode&TRX_ReadOnly) return trx_errno=TRX_ErrReadOnly;
  if(trxt->Root!=0L) return trx_errno=TRX_ErrNonEmpty;

  nodespace=_TRXP->Fd.SizNode-sizeof(TRX_NODE);

  /*A branch node must be large enough for two keys w/links.
    A leaf node must be large enough for one key w/record.*/

  if(initFlags&TRX_KeyMaxSize) {
    minsizkey=sizKey;
    sizKey=nodespace/2-(_TRXP->Fd.SizNodePtr+2);
    if(sizKey>TRX_MAX_KEYLEN) sizKey=TRX_MAX_KEYLEN;
    /*sizKey now satisfies the branch node requirement.*/
    if(sizRec>nodespace-sizKey-2) sizKey=nodespace-sizRec-2;
    if(sizRec>=nodespace-2 || minsizkey>sizKey) goto _err_siz;
  }

  if(!sizKey) {
    /*Record-type tree -- NOT YET SUPPORTED --
    if((initFlags&TRX_RecMaxSize) && sizRec<nodespace) sizRec=nodespace;
    else if(!sizRec) goto _err_arg;
    initFlags|=TRX_KeyFixedSize;
    if(nodespace/sizRec==0) */
    goto _err_siz;
  }
  else {
    /*Index-type tree --*/
    if(nodespace<2*(sizKey+_TRXP->Fd.SizNodePtr+2) ||
       sizRec>nodespace-sizKey-2) goto _err_siz;
    if(initFlags&TRX_RecMaxSize) sizRec=nodespace-sizKey-2;
  }
  _trx_pTreeUsr->KeyPos=0; /*Should already be set!*/

  trxt->SizRec=sizRec;
  trxt->SizKey=(BYTE)sizKey;
  trxt->InitFlags=(BYTE)initFlags;
  _TRXP->HdrChg=TRUE;

  return _trx_ErrFlushHdr();

_err_arg:
  return trx_errno=TRX_ErrArg;
_err_siz:
  return trx_errno=TRX_ErrSizNode;
}
