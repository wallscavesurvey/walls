/* TRXGETK.C */
#include "a__trx.h"

int TRXAPI trx_GetKey(TRX_NO trx,LPVOID keyBuf,UINT sizKeyBuf)
{
  CSH_HDRP hp;

  if((hp=_trx_GetNodePos(trx,0))!=0) {
    if(_trx_pTreeVar->SizKey==0) {
      *((PBYTE)keyBuf)=0;
      trx_errno=TRX_ErrTreeType;
    }
    else trx_errno=_trx_Btkey(hp,_trx_pTreeUsr->KeyPos,(PBYTE)keyBuf,sizKeyBuf);
  }
  return trx_errno;
}
