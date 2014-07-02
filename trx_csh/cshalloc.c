/* CSHALLOC.C -- Cache allocation and buffer access functions */

#include <a__csh.h>
#include <limits.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

int      csh_errno;
BOOL    _csh_newRead; //int

int TRXAPI csh_Alloc(TRX_CSH FAR *pcsh,UINT bufsiz,UINT bufcnt)
{
  UINT i;
  CSH_NO csh;
  CSH_HDRP hp;
  PBYTE pBuf;

  *pcsh=0;

  if(!bufsiz) bufsiz=CSH_DEF_BUFSIZ;
  else {
    if((i=(bufsiz%16))!=0) bufsiz+=(16-i);
    if(bufsiz>CSH_MAX_BUFSIZ)
      return csh_errno=DOS_ErrArg;
	if(bufsiz<CSH_MIN_BUFSIZ) bufsiz=CSH_MIN_BUFSIZ;
  }
  
  if(bufcnt<CSH_MIN_BUFCNT) bufcnt=CSH_MIN_BUFCNT;
  else if(bufcnt>CSH_MAX_BUFCNT) bufcnt=CSH_MAX_BUFCNT;
  //if(maxcnt<bufcnt) maxcnt=bufcnt;
  //else if(maxcnt>CSH_MAX_BUFCNT) maxcnt=CSH_MAX_BUFCNT;

  /*Allocate 24+64 bytes for the CSH_CACHE structure, followed by the
    trailing part of the array of 64-byte CSH_HDR structs --*/
  if((csh=(CSH_NO)dos_Alloc(sizeof(CSH_CACHE)+(bufcnt-1)*sizeof(CSH_HDR)))==0)
    return csh_errno=DOS_ErrMem;

  #ifdef _MSC_VER
  if(NULL==(pBuf=(PBYTE)VirtualAlloc(NULL,bufcnt*bufsiz,
	  MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE))) {
	dos_Free(csh);
	return csh_errno=DOS_ErrMem;
  }
  #else
  if(NULL==(pBuf=(PBYTE)dos_Alloc(bufcnt*bufsiz))) {
	dos_Free(csh);
	return csh_errno=DOS_ErrMem;
  }
  #endif

  hp=csh->HdrOff;
  for(i=0;i<bufcnt;i++,hp++,pBuf+=bufsiz){
	hp->Buffer=pBuf;
    hp->Cache=csh;
    hp->File=NULL;
    hp->HashTable=NULL;
    hp->Status=hp->NumLocks=0;
    hp->PrevMru=hp-1;
	hp->NextMru=hp+1;
  }
  //Make links circular --
  csh->Mru=csh->HdrOff->PrevMru=--hp;
  hp->NextMru=csh->HdrOff;

  csh->Bufsiz=bufsiz;
  csh->Bufcnt=bufcnt;
  //csh->Maxcnt=maxcnt;
  csh->FstFile=NULL;
  csh->HitCount=0;
  *pcsh=csh;
  return csh_errno=0;
}

int TRXAPI csh_Free(TRX_CSH csh)
{
  CSH_HDRP hp;
  int i;

  if(!csh) return csh_errno=DOS_ErrArg;
  csh_errno=0;

  /*Bug free code will never lock-up!*/
  while(_CSH->FstFile!=0)
    if((i=_csf_DetachCache(_CSH->FstFile))!=0) csh_errno=i;

  hp=_CSH->HdrOff;
  for(i=_CSH->Bufcnt;i;i--,hp++) {
    free(hp->Reserved[1]);
    free(hp->Reserved[2]);
  }

  hp=_CSH->HdrOff;
#ifdef _MSC_VER
  VirtualFree(hp->Buffer,0,MEM_RELEASE);
#else
  dos_Free(hp->Buffer);
#endif
  dos_Free(_CSH);
  return csh_errno;
}

/*===========================================================*/
/*Library helper functions -- */

/*===========================================================*/
/*Local CSH_CACHE specific helper functions --*/

static apfcn_hdrp _csh_GetNextMru(CSH_NO csh)
{
  /*Return handle of LRU unlocked buffer, flush if necessary,
    and make it the new csh->Mru. Return zero only if all
    buffers are locked.*/

  CSH_HDRP hp=csh->Mru;

  while(TRUE) {
    //NOTE: Every unlocked buffer is checked for availability in order
    //of least recent use *including*, finally, the current Mru --

    hp=hp->NextMru;
    if(!hp->Status) break;
    if(!hp->NumLocks) {
      /*Sets csf->Errno if write fails! This will be checked later,
        but only for the current file.*/
      if(hp->Status&CSH_Updated) _csh_XfrRecord(hp,1);
      _csh_PurgeHdr(hp); /*Remove file ownership - set status CSH_Free==0*/
      break;
    }
    if(hp==csh->Mru) return 0;
  }
  return(csh->Mru=hp);
}

static apfcn_v _csh_MakeLru(CSH_HDRP hp)
{
  CSH_NO csh=hp->Cache;
  CSH_HDRP mru=csh->Mru;

  if(hp==mru) csh->Mru=hp->PrevMru;
  else
    if(mru->NextMru!=hp) {
      /*unlink hp and reinsert after Mru --*/
      hp->NextMru->PrevMru=hp->PrevMru;
      hp->PrevMru->NextMru=hp->NextMru;
      hp->PrevMru=mru;
      hp->NextMru=mru->NextMru;
      mru->NextMru=hp->NextMru->PrevMru=hp;
    }
}

/*============================================================*/
/*Public CSH_CACHE specific helper functions --*/

apfcn_v _csh_PurgeHdr(CSH_HDRP hp)
{
  /*Unconditionally discard buffer contents*/

  CSH_HDRP hpnext;

  if(hp->File) {
    if((hpnext=hp->NextHash)!=0) {
      hpnext->PrevHash=hp->PrevHash;
      hpnext->HashIndex=hp->HashIndex;
    }
    if(hp->PrevHash) hp->PrevHash->NextHash=hpnext;
    else hp->HashIndex->HashTable=hpnext;
    hp->File=0;
  }
  /*For now, make purged buffer the LRU only if it is
    currently the MRU --*/
  if(hp->Cache->Mru==hp) hp->Cache->Mru=hp->PrevMru;
  hp->Status=hp->NumLocks=0;
}

apfcn_i _csh_FlushHdr(CSH_HDRP hp)
{
  if(hp->Status&CSH_Updated) {
    /*hp->Status &= ~(CSH_Updated+CSH_New); */
    hp->Status=CSH_InUse;  /*Equivalent to above for now */
    return _csh_XfrRecord(hp,1);
  }
  return 0;
}

apfcn_i _csh_XfrRecord(CSH_HDRP hp,int typ)
{
  CSF_NO csf=hp->File;

  if(csf->XfrFcn) return csf->Errno=csf->XfrFcn(hp,typ);

  assert((__int64)hp->Recno*csf->Recsiz+csf->Offset<UINT_MAX);

  if(dos_Transfer(typ,csf->Handle,hp->Buffer,
      csf->Offset+hp->Recno*csf->Recsiz,
      (csf->LastDataLen && hp->Recno==csf->LastRecno)?
      csf->LastDataLen:csf->Recsiz))
    return csf->Errno=DOS_ErrRead+typ;

  if(!typ) _csh_newRead=TRUE;
  return 0;
}

/*============================================================*/
/*Local CSH_FILE specific helper functions --*/

static apfcn_v _csf_InsertFile(CSH_NO csh,CSF_NO csfnew)
{
  /*Insert csfnew in linked list between existing files
  separated by the largest HashBase interval. Initialize
  csfnew->HashBase to split this interval.*/

  /*NOTE: This function should reduce hash table collisions
  somewhat when multiple files share the cache. Otherwise
  identically numbered records would always map to the same
  hash table offset, possibly leaving significant portions of
  the table unused when files are smaller than the cache.*/

  CSF_NO csf,maxcsf;
  UINT width,maxwidth;

  if((csf=csh->FstFile)==0) {
    /*We are adding the first file --*/
    csfnew->NextFile=0;
    csfnew->HashBase=0;
    csh->FstFile=csfnew;
    return;
  }

  maxwidth=0;
  maxcsf=0; /*avoid warning of uninitialized usage*/
  while(csf) {
    /*determine HashBase interval from csf to csf->NextFile --*/
    if(csf->NextFile) width=(csf->NextFile)->HashBase;
    else width=(csh->FstFile)->HashBase+csh->Bufcnt;
    if((width-=csf->HashBase)>=maxwidth) {
      maxwidth=width;
      maxcsf=csf;
    }
    csf=csf->NextFile;
  }

  width=maxcsf->HashBase+width/2;
  if(width>=csh->Bufcnt) {
    /*Possible only if maxcsf was the last file in the chain --*/
    csfnew->NextFile=csh->FstFile;
    csh->FstFile=csfnew;
    width-=csh->Bufcnt;
  }
  else {
    csfnew->NextFile=maxcsf->NextFile;
    maxcsf->NextFile=csfnew;
  }
  csfnew->HashBase=width;
}

/*============================================================*/
/*Public CSH_FILE specific helper functions --*/

apfcn_i _csf_AttachCache(CSF_NO csf,CSH_NO csh)
{
  /*If a cache already exists, it is detached and if a file error
  is pending, the code is returned without the new cache being attached.
  The *next* call to to either _csf_DetachCache() or _csf_AttachCache()
  should clear this code and return success.*/

  if(!csf) return DOS_ErrArg;
  if(_csf_DetachCache(csf)) return csf->Errno;
  csf->Csh=csh;
  /*Insert file in linked list and initialize csf->HashBase --*/
  if(csh) _csf_InsertFile(csh,csf);
  return 0;
}

apfcn_i _csf_DetachCache(CSF_NO csf)
{
  CSF_NO NEAR *pcsf;

  /*If a cache exists, it is detached and any pending error code
    is returned, with csf->Errno NOT reset. Otherwise, csf->Errno
    is reset to zero.*/

  if(!csf) return DOS_ErrArg;
  if(!csf->Csh) return 0;

  _csf_Flush(csf);
  _csf_Purge(csf);
  pcsf=&csf->Csh->FstFile;
  while(*pcsf) {
    if(*pcsf==csf) {
      *pcsf=csf->NextFile;
      break;
    }
    pcsf=&(*pcsf)->NextFile;
  }
  csf->Csh=0;

  return csf->Errno;
}

apfcn_i _csf_DeleteCache(CSF_NO csf,CSH_NO *pCshDef)
{
  int e;
  CSH_NO csh;

  if(!csf || (csh=csf->Csh)==0) e=csf?0:DOS_ErrArg;
  else {
    e=_csf_DetachCache(csf);
    if(!csh->FstFile && pCshDef && *pCshDef==csh) {
      csh_Free((TRX_CSH)csh);
      *pCshDef=0;
    }
  }
  return e;
}

apfcn_v _csf_Purge(CSF_NO csf)
{
  CSH_HDRP hp;
  int i;
  CSH_NO csh;

  if(!csf || (csh=csf->Csh)==0) return;

  hp=csh->HdrOff;
  for(i=csh->Bufcnt;i;i--) {
    if(hp->File==csf) _csh_PurgeHdr(hp);
    hp++;
  }
}

apfcn_i _csf_Flush(CSF_NO csf)
{
  /*NOTE: May set but does NOT reset csf->Errno --*/

  CSH_HDRP hp;
  int i;
  CSH_NO csh;

  if(!csf) return DOS_ErrArg;
  if((csh=csf->Csh)!=0) {
    hp=csh->HdrOff;
    for(i=csh->Bufcnt;i;i--) {
      if(hp->File==csf) _csh_FlushHdr(hp);
      hp++;
    }
  }
  /*else if no cache, nothing to flush*/
  return csf->Errno;
}

apfcn_hdrp _csf_GetRecord(CSF_NO csf,DWORD recno,int status)
{
  /*Returns handle of cache buffer containing record recno. If the
    record is not already buffered, the LRU buffer is flushed if
    necessary and assigned to the file, in which case the record is also
    read from disk unless only a buffer allocation is requested
    ((status&CSH_New)!=0). The status of a newly assigned buffer is
    initialized to status|CSH_InUse. If the record is already buffered,
    the status becomes <old status>|status.

    A zero handle is returned if a disk read error occurs (record out of
    range?), or if a write error occurs when attempting to flush a
    buffer previously owned by the same file (in which case no read is
    attempted). Either type of error will purge the buffer (making it
    the LRU buffer) and set the file's error condition (csf->Errno!=0).
    A zero handle is also returned if no cache is assigned to the file,
    if there are no unlocked buffers, or if the file's error condition
    is already set when the function is called.

    NOTE: A flush error involving a different file will set only that
    file's error condition. It alone will *not* prevent buffer
    reassignment nor this function's successful completion.*/

  CSH_HDRP hp,nexthp,mru;
  CSH_NO csh;
  CSH_HDRP hashindex;

  _csh_newRead=FALSE;    /*Will be set TRUE if disk read occurs*/
  if(!csf || csf->Errno) return 0;
  if((csh=csf->Csh)==0) {
    csf->Errno=DOS_ErrNoCache;
    return 0;
  }
  hashindex=csh->HdrOff+(int)((csf->HashBase+recno)%csh->Bufcnt);

  if((nexthp=hashindex->HashTable)!=0) {
    /*The record may be in the cache. The _NextHash() chain is sorted
      by file+record. Advance to target record, if present, or to the
      next record (or end of chain) --*/

    do {
      hp=nexthp;
      if(csf>hp->File) continue;
      if(csf<hp->File || recno<hp->Recno) break;
      if(recno==hp->Recno) {
        csh->HitCount++;
        _csh_MakeLru(hp);
        csh->Mru=hp;
        /*Do not remove Updated/New flags if set --*/
        if(status) hp->Status |= status;
        return hp;
      }
    }
    while((nexthp=hp->NextHash)!=0);

    /*Hash table collision but record not in cache --*/

    if(hp==csh->Mru->NextMru && !hp->NumLocks) {
      /*Less juggling needed if hp happens to be the LRU buffer handle --*/
      _csh_FlushHdr(hp); /*Sets Errno variables upon write failure*/
      csh->Mru=hp;
    }
    else {
      /* Flush LRU and make it the MRU buffer */
      if((mru=_csh_GetNextMru(csh))==0) {   //.15
        csf->Errno=DOS_ErrLocked;
        return 0;
      }
      if(nexthp) {
        /*Not at end of chain, so insert new mru before current header --*/
        if((mru->PrevHash=hp->PrevHash)!=0) hp->PrevHash->NextHash=mru;
        else {
          hashindex->HashTable=mru;
          mru->HashIndex=hashindex;
        }
        mru->NextHash=hp;
        hp->PrevHash=mru;
      }
      else { /*Insert at end of chain --*/
        mru->NextHash=0;
        mru->PrevHash=hp;
        hp->NextHash=mru;
      }
      hp=mru;
    }
  }
  else {
     /*_HashTable(hashindex)==0*/
     if((hp=_csh_GetNextMru(csh))==0) { //.15
       csf->Errno=DOS_ErrLocked;
       return 0;
     }
    hashindex->HashTable=hp;
    hp->HashIndex=hashindex;
    hp->NextHash=hp->PrevHash=0;
  }

  /*Buffer hp now belongs to file csf, is the new csh->Mru,
  and is available for reading or allocating a new record.*/

  hp->File=csf;
  hp->Recno=recno;

  /*hp->Status is still undefined. We may
  have csf->Errno != 0 if a flush was required.*/

  if(csf->Errno || (!(status&CSH_New) && _csh_XfrRecord(hp,0))) { //.25
    _csh_PurgeHdr(hp);
    return 0;
  }

  hp->Status=status|CSH_InUse;
  return hp;
}

int TRXAPI csh_Flush(TRX_CSH csh)
{
  /*Flush the cache of all unwritten records*/

  CSH_HDRP hp;
  int i;

  if(!csh) return csh_errno=DOS_ErrArg;
  csh_errno=0;
  i=_CSH->Bufcnt;
  hp=_CSH->HdrOff;
  while(i--) {
    if(hp->File && _csh_FlushHdr(hp)) csh_errno=DOS_ErrWrite;
    hp++;
  }
  return csh_errno;
}

int TRXAPI csh_Purge(TRX_CSH csh)
{
  /*Unconditionally empty the cache*/

  CSH_HDRP hp;
  int i;

  if(!csh) return csh_errno=DOS_ErrArg;

  i=_CSH->Bufcnt;
  hp=_CSH->HdrOff;
  while(i--) {
    if(hp->File) _csh_PurgeHdr(hp);
    hp++;
  }
  return csh_errno=0;
}

apfcn_v _csh_MarkHdr(CSH_HDRP hp)
{
  hp->Status |= CSH_Updated;
}

apfcn_v _csh_LockHdr(CSH_HDRP hp)
{
  hp->NumLocks++;
}

apfcn_v _csh_UnLockHdr(CSH_HDRP hp)
{
  if(hp->NumLocks) hp->NumLocks--;
}

UINT TRXAPI csh_NumBuffers(TRX_CSH csh)
{
  return csh?_CSH->Bufcnt:0;
}

UINT TRXAPI csh_SizBuffer(TRX_CSH csh)
{
  return csh?_CSH->Bufsiz:0;
}
#ifdef __cplusplus
}
#endif
