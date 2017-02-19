/* TRXNEW.C - Add new node to TRX_FILE index -- */
#include "a__trx.h"

static HNode _trx_maxNode[2]={0x0000FFFFL,0x00FFFFFFL};

apfcn_i _trx_IncPreWrite(UINT numnew)
{
  CSH_HDRP hp;
  TRX_pFILE trxp=_trx_pFile;
  HNode recno;
  int sizp;

  /*If MinPreWrite>0 and the number of available written file nodes is
    less than numnew, guarantee file space for numnew+MinPreWrite-1
    tree nodes. A write error while extending implies a disk full
    condition, which causes trx_errno=TRX_ErrSpace to be returned.*/

  if(trxp->MinPreWrite &&
      (DWORD)numnew>(DWORD)trxp->NumPreWrite+trxp->NumFreeNodes) {

    numnew -= (UINT)(trxp->NumFreeNodes +
                (DWORD)(1+trxp->NumPreWrite-trxp->MinPreWrite));
    
    /*We want to allocate numnew additional file nodes.
      The total number of file nodes is currently --*/

    recno=trxp->NumNodes+trxp->NumPreWrite;

    /*Make sure the link size is adequate to hold a node pointer --*/
    if((sizp=trxp->Fd.SizNodePtr)<4 && recno+numnew>_trx_maxNode[sizp-2])
      return trx_errno=TRX_ErrNodLimit;

    /*Lack of a cache is not fatal to file integrity. A write error
      during a new buffer's allocation IS fatal --*/

    if((hp=_csf_GetRecord((CSF_NO)trxp,recno,CSH_Updated|CSH_New))==0) {
      if((trx_errno=trxp->Cf.Errno)==TRX_ErrNoCache) trxp->Cf.Errno=0;
      return trx_errno;
    }

    /*Prewritten nodes are "free" nodes but not linked --*/
    _Node(hp)->N_Tree=0xFF;
    _Node(hp)->N_This=0L;

    for(;numnew;numnew--) {
      if(_csh_XfrRecord(hp,1)) {
        /*Failure to prewrite should not corrupt the file --*/
        trxp->Cf.Errno=0;
        break;
      }
      trxp->NumPreWrite++;
      trxp->HdrChg=TRUE;
      _Recno(hp)++;
    }
     /*Restore header record number for purge*/
    _Recno(hp)=recno;
    _csh_PurgeHdr(hp);
    if(numnew) {
      /*Not enough nodes were prewritten. If in auto flush mode,
        we must still flush the header if it was revised --*/
      if(!_trx_ErrFlushHdr()) trx_errno=TRX_ErrSpace;
      /*else trx_errno was set to TRX_ErrWrite*/
      return trx_errno;
    }
  }
  return 0;
}

CSH_HDRP NEAR PASCAL _trx_NewNode(int lvlno)
{
  TRX_pFILE trxp=_trx_pFile;
  int sizp=trxp->Fd.SizNodePtr;
  HNode recno;
  CSH_HDRP hp;
  TRX_pNODE node;
  UINT get_flags=CSH_Updated;

  if(trxp->NumFreeNodes) recno=trxp->FirstFreeNode-1L;
  else {
    recno=trxp->NumNodes;
    get_flags=CSH_Updated|CSH_New;
    if(sizp<4 && !trxp->NumPreWrite && recno>=_trx_maxNode[sizp-2]) {
      /*This must be considered fatal to file integrity since
        the user did not choose to prewrite!*/
      trxp->Cf.Errno=TRX_ErrNodLimit;
      goto _err;
    }
  }

  if((hp=_csf_GetRecord((CSF_NO)trxp,recno,get_flags))==0) goto _err;
  node=_Node(hp);

  /*If adding a leaf --*/
  if(!lvlno) sizp=_trx_pTreeVar->SizRec;

  if(trxp->NumFreeNodes) {
    trxp->NumFreeNodes--;
    trxp->FirstFreeNode=node->N_Rlink;
  }
  else {
    if(trxp->NumPreWrite) trxp->NumPreWrite--;
    node->N_This=++trxp->NumNodes;
  }
  node->N_SizLink=-sizp;
  node->N_FstLink=trxp->Fd.SizNode;
  node->N_NumKeys=0;
  node->N_Flags=lvlno;
  node->N_Tree=(BYTE)_trx_tree;
  node->N_Llink=node->N_Rlink=0L;
  ++_trx_pTreeVar->NumNodes;
  trxp->HdrChg=TRUE;
  _csh_MarkHdr(hp);
  return hp;

_err:
  trx_errno=trxp->Cf.Errno;
  return 0;
}
