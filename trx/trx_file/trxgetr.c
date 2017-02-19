/* TRXGETR.C */
#include "a__trx.h"

int TRXAPI trx_GetRec(TRX_NO trx,LPVOID recBuf,UINT sizRecBuf)
{
  CSH_HDRP hp;

  /*Note: _trx_Btrec() can fail only if the node is corrupted.
    We must set the file error flag to indicate this fatal condition!*/

  if((hp=_trx_GetNodePos(trx,0))!=0) {
    if(_trx_pTreeVar->SizRec==0) trx_errno=TRX_ErrTreeType;
    else
    if((trx_errno=_trx_Btrec(hp,_trx_pTreeUsr->KeyPos,recBuf,sizRecBuf))!=0 && trx_errno!=TRX_ErrTruncate)
      _trx_pFile->Cf.Errno=trx_errno;
  }
  return(trx_errno);
}
