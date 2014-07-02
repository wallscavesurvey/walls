/* TRXSKIP.C */
#include "a__trx.h"

DWORD TRXAPI trx_RunLength(TRX_NO trx)
{
  /* Returns the number of records with keys matching the one at the
    the current position, beginning with the current key. The current
	position is unchanged. If zero is returned, trx_errno contains an
	error code -- normally TRX_ErrEOF.*/

  CSH_HDRP hp;
  UINT n;
  DWORD len;
  nptr p;

  if(!_trx_GetTrx(trx) || (hp=_trx_GetNodePos(trx,0))==0) return 0;

  len=1;

  for(n=_trx_pTreeUsr->KeyPos;n>1;n--) {
	  if(!(p=_trx_Btkeyp(hp,n-1))) {
		  trx_errno=TRX_ErrFormat;
		  return 0;
	  }
	  if(*p) break;
	  len++;
  }

  if(n==1) {
	  while(_Node(hp)->N_Flags&TRX_N_DupCutRight) {
		   if((hp=_trx_GetNode(_Node(hp)->N_Rlink))==0) return 0;
		   len+=_Node(hp)->N_NumKeys;
	  }
  }
  return len;
}
