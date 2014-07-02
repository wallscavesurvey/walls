/* TRXFIND.C - Find key in TRX_FILE index -- */
#include <a__trx.h>

/*This function returns trx_errno=TRX_ErrEOF if the target key is
  greater than all keys in the index. Zero is returned if either the
  target exactly matches an existing key, or if flag TRX_KeyExact is NOT
  set and the target is a prefix of an existing key. Otherwise, if
  no tree access errors occur, TRX_ErrMatch is returned.

  If either zero or TRX_ErrMatch is returned, the current position
  becomes that of the matched key, or rather the first key in the
  index equal to or greater than the target key. The global word
  trx_findResult is then set to zero for an exact match, xx00h for a
  prefix match, or 00xxh if the target has an unmatching suffix.

  If the return value is other than zero or TRX_ErrMatch, the current
  position is unchanged and trx_findResult is set to FFFFh. */

int TRXAPI trx_Find(TRX_NO trx,LPBYTE pKey)
{
  CSH_HDRP hp;
  UINT level;

  trx_findResult=0xFFFF;
  if(!_GETTRXU || (trx_errno=_trx_pFile->Cf.Errno)!=0) return trx_errno;
  if(!_trx_pTreeVar->SizKey) return trx_errno=TRX_ErrTreeType;
  if((hp=_trx_GetNode(_trx_pTreeVar->Root))==0) return trx_errno;

  level=0;
  while(_Node(hp)->N_Flags&TRX_N_Branch) {
    if((hp=_trx_GetNode(_trx_Btlnk(hp,pKey)))==0) return trx_errno;
    if(++level>TRX_MAX_BRANCH_LEVELS) return trx_errno=TRX_ErrLevel;
  }

  /*Assumes all leaf nodes are non-empty --*/
  if(!_trx_Btfnd(hp,pKey)) {
    if((hp=_trx_GetNode(_Node(hp)->N_Rlink))==0) {
      trx_findResult=0xFFFF;
      return trx_errno;
    }
    if(!_trx_Btfnd(hp,pKey)) {
      trx_findResult=0xFFFF;
      return FORMAT_ERROR();
    }
  }

  _TRXU->NodePos=_Node(hp)->N_This;
  _TRXU->KeyPos=_trx_btcnt;

  return (trx_findResult &&
         ((BYTE)trx_findResult || (_TRXU->UsrFlags&TRX_KeyExact)))?
         (trx_errno=TRX_ErrMatch):0;
}
