#include <string.h>

#include "a__dbf.h"

static apfcn_i _dbf_AddCshRec(DBF_pFILE dbfp)
{
  CSH_HDRP hp;
  DBF_pFILEUSR usrp=_dbf_pFileUsr;
  UINT reclen=dbfp->H.SizRec; //(WORD in header)
  UINT bufoff=dbfp->Cf.LastDataLen;

  /*NOTE: Upon starting a new block of records, or extending a
    non-prewritten block, we will attempt to extend the file to an
    integral number of blocks if any flush mode is in effect. In this
    case a DBF_ErrWrite error would NOT be fatal to file integrity,
    but would cause DBF_ErrSpace to be returned.*/

  if(bufoff==dbfp->Cf.Recsiz) {
    bufoff=0;
    dbfp->Cf.LastRecno++;
  }

  hp=_csf_GetRecord((CSF_NO)dbfp,dbfp->Cf.LastRecno,bufoff?0:CSH_New);

  if(hp) {
    memcpy(hp->Buffer+bufoff,usrp->U_Rec,reclen);
    _csh_MarkHdr(hp);

    if((!bufoff || !dbfp->BufWritten) &&
      (usrp->U_Mode&(DBF_UserFlush+DBF_AutoFlush))!=0) {

      /*Let's fill the remaining portion of the buffer with EOF marks --*/
      if((reclen+=bufoff)<dbfp->Cf.Recsiz)
        memset(hp->Buffer+reclen,DBF_EOFMARK,
          dbfp->Cf.Recsiz-reclen);

      dbfp->Cf.LastDataLen=dbfp->Cf.Recsiz;
      if(_csh_FlushHdr(hp)) {
        /*Failure to extend file by one buffer size is NOT fatal
          to file integrity, but will be returned as a disk full error --*/
        dbfp->Cf.Errno=0;
        if(bufoff) dbfp->Cf.LastDataLen=bufoff;
        else {
          dbfp->Cf.LastRecno--;
          dbfp->Cf.LastDataLen=dbfp->Cf.Recsiz;
        }
        return dbf_errno=DBF_ErrSpace;
      }
      /*From now on, we will flush only the buffer's valid data --*/
      dbfp->Cf.LastDataLen=reclen;
      dbfp->BufWritten=TRUE;
      return 0;
    }

    /*We have extended the valid data in a block. If any
      flush mode is in effect, we must have bufoff>0 and
      dbfp->BufWritten==TRUE. An auto-flush failure, in this
      case, cannot be due to lack of disk space.*/

    dbfp->Cf.LastDataLen=bufoff+reclen;
    if(!(usrp->U_Mode&DBF_AutoFlush) || !_csh_FlushHdr(hp)) return 0;
    dbfp->Cf.LastDataLen=bufoff;
  }
  else if(!bufoff) dbfp->Cf.LastRecno--;

  /*A cache write error occurred during an attempt to allocate
    a new buffer, or to write a record already prewritten.
    We must consider this fatal to file integrity --*/

  return dbf_errno=dbfp->Cf.Errno;
}

static LPVOID _pUsrRec;

static apfcn_i _dbf_AppendUsrRec(DBF_NO dbf,int c)
{
  DBF_pFILEUSR usrp;
  DWORD new_recoff;
  BOOL bShared;
  
  if(!_GETDBFP || _dbf_ErrReadOnly() || _dbf_CommitUsrRec())
    return dbf_errno;

  usrp=_PDBFU;  //Faster? Who knows?

  if(c>=0) memset(usrp->U_Rec,c,_DBFP->H.SizRec);
  else if(_pUsrRec) memcpy(usrp->U_Rec,_pUsrRec,_DBFP->H.SizRec);
  usrp->U_Recno=0;
  
  bShared=(usrp->U_Mode&DBF_Share)!=0;

  if(bShared && (c=_dbf_RefreshHdr(_DBFP,DOS_ExclusiveLock))) return dbf_errno=c;

  new_recoff=_DBFP->H.NumRecs*_DBFP->H.SizRec;

  /*We will destroy any EOF mark in our attempt to write a new record --*/
  _DBFP->FileSizChg=TRUE;

  if(!_DBFP->Cf.Csh) {
	/*
    if(dos_Transfer(1,_DBFP->Cf.Handle,usrp->U_Rec,
      _DBFP->Cf.Offset+new_recoff,_DBFP->H.SizRec)) return dbf_errno=DBF_ErrSpace;
	*/
	if(bShared) memcpy(usrp->U_Rec+_DBFP->H.SizRec,usrp->U_Rec,_DBFP->H.SizRec);
  }
  else if(_dbf_AddCshRec(_DBFP)) return dbf_errno;

  usrp->U_Recoff=new_recoff;
  usrp->U_Recno=++_DBFP->H.NumRecs;
  _DBFP->HdrChg=TRUE;
  return (_PDBFU->U_Mode&DBF_AutoFlush)?_dbf_FlushHdr():0;
}

int DBFAPI dbf_AppendBlank(DBF_NO dbf)
{
 return _dbf_AppendUsrRec(dbf,' ');
}

int DBFAPI dbf_AppendZero(DBF_NO dbf)
{
 return _dbf_AppendUsrRec(dbf,0);
}

int DBFAPI dbf_AppendRec(DBF_NO dbf,LPVOID pRec)
{
 _pUsrRec=pRec;
 return _dbf_AppendUsrRec(dbf,-1);
}

int DBFAPI dbf_AppendRecPtr(DBF_NO dbf)
{
  if(!_dbf_GetDbfU(dbf)) return DBF_ErrArg;
  if(_PDBFU->U_RecChg) return DBF_ErrRecChanged;
 _pUsrRec=0;
 return _dbf_AppendUsrRec(dbf,-1);
}
