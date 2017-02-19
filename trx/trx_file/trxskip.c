/* TRXSKIP.C */
#include "a__trx.h"

int TRXAPI trx_Skip(TRX_NO trx,long n)
{
  /*Adjusts the user's tree position by n, where n is any integer
    (positive, negative, or zero). For example, assuming the tree's
    possible positions are numbered 1 to N, starting with the lowest
    valued position, if X is the user's original position, the new
    position will be X+n provided it is within range (1 <= X+n <= N).
    Upon a successful adjustment, the leaf node corresponding to the new
    position is loaded into to the file's LRU cache.

    TRX_ErrEOF is returned if either a prior position was not
    established or if the new position doesn't exist. If this, or any
    other type of error occurs, the user's original tree position
    remains unchanged.*/


  CSH_HDRP hp;
  int dir;
  UINT old_keypos;
  HNode old_nodepos;

  if(!_trx_GetTrx(trx)) return trx_errno;

  /*Save old tree position in case of error --*/
  old_keypos=_trx_pTreeUsr->KeyPos;
  old_nodepos=_trx_pTreeUsr->NodePos;

  dir=(n>0L)?1:(n?-1:0);
  if((hp=_trx_GetNodePos(trx,dir))==0) return trx_errno;

  trx=(TRX_NO)_trx_pTreeUsr;
  n=(long)(_TRXU->KeyPos+dir)-n;

  /*n=new count position with respect to current node*/

  if(dir>=0) {
    while(n<=0L) {
      if((hp=_trx_GetNode(_Node(hp)->N_Rlink))==0) goto _err;
      n+=(long)_Node(hp)->N_NumKeys;
    }
  }
  else {
    while(n>(long)_Node(hp)->N_NumKeys) {
      n-=(long)_Node(hp)->N_NumKeys;
      if((hp=_trx_GetNode(_Node(hp)->N_Llink))==0) goto _err;
    }
  }

  _TRXU->KeyPos=(UINT)n;
  _TRXU->NodePos=_Node(hp)->N_This;
  return 0;

_err:
  _TRXU->KeyPos=old_keypos;
  _TRXU->NodePos=old_nodepos;
  return trx_errno;

}
