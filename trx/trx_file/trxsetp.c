#include "a__trx.h"

int TRXAPI trx_SetPosition(TRX_NO trx,ulong pos)
{
   CSH_HDRP hp;
   UINT cnti;
   DWORD nodi;
   #ifdef _DEBUG
   TRX_pNODE pNode;
   #endif

   if(!_GETTRXP || (trx_errno=_TRXP->Cf.Errno)!=0) return trx_errno;
#if 0
   nodi=(pos-1L)/(cnti=_trx_pFile->Fd.SizNode)+1L;
   cnti=(UINT)(pos-cnti*(nodi-1L));
#else
   nodi=pos/(cnti=_trx_pFile->Fd.SizNode)+1L;
   cnti=(UINT)(pos-cnti*(nodi-1L));
#endif
   if((hp=_trx_GetNode(nodi))==0) return trx_errno;

   #ifdef _DEBUG
   pNode=_Node(hp);
   if(pNode->N_Tree!=(byte)_trx_tree) return trx_errno=TRX_ErrEOF;
   if((pNode->N_Flags&TRX_N_Branch)!=0) return trx_errno=TRX_ErrEOF;
   if(cnti>pNode->N_NumKeys) return trx_errno=TRX_ErrEOF;
   #else

   if(_Node(hp)->N_Tree!=(byte)_trx_tree ||
		(_Node(hp)->N_Flags&TRX_N_Branch)!=0 ||
		cnti>_Node(hp)->N_NumKeys) return trx_errno=TRX_ErrEOF;
   #endif

   _trx_pTreeUsr->NodePos=nodi;
   _trx_pTreeUsr->KeyPos=cnti;
   return 0;
}

DWORD TRXAPI trx_GetPosition(TRX_NO trx)
{
  DWORD pos;
  if(!_trx_GetNodePos(trx,0)) return 0L;
  pos=_trx_pTreeUsr->NodePos-1L;
  pos *= _trx_pFile->Fd.SizNode;
  pos += _trx_pTreeUsr->KeyPos;
  return pos;

  //return (_trx_pTreeUsr->NodePos-1L) * _trx_pFile->Fd.SizNode +
  //  _trx_pTreeUsr->KeyPos;
}
