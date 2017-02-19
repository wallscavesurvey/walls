/* TRXFILLK.C */
#include "a__trx.h"

int TRXAPI trx_FillKeyBuffer(TRX_NO trx,int offset)
{
  CSH_HDRP hp;
  BYTE *keybuf;

  if((hp=_trx_GetNodePos(trx,offset))!=0) {
    if(_trx_pTreeVar->SizKey==0) trx_errno=TRX_ErrTreeType;
    else if(!(keybuf=_trx_pFileUsr->UsrKeyBuffer)) trx_errno=TRX_ErrNoKeyBuffer;
    else {
      #ifndef _NO_TRXD
        if(io_prec) {
            io_prec->idx=(int)keybuf;
            keybuf=io_prec->key;
        }
      #endif
        trx_errno=_trx_Btkey(hp,_trx_pTreeUsr->KeyPos,keybuf,_trx_pTreeVar->SizKey+1);
    }
  }
  return trx_errno;
}

int TRXAPI trx_SetKeyBuffer(TRX_NO trx,void *keyBuffer)
{
	if(!_GETTRXM) return TRX_ErrArg;
	_TRXM->UsrKeyBuffer=(BYTE *)keyBuffer;
	return 0;
}
