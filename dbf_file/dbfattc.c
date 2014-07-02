#include "a__dbf.h"

int DBFAPI dbf_AttachCache(DBF_NO dbf,TRX_CSH csh)
{
  UINT sizblk,sizrec,sizbuf;
  DWORD sizdata;

  if(!csh || !_GETDBFP) return DBF_ErrArg;

  /*Cache not allowed for shared file --*/
  if(_PDBFU->U_Mode&DBF_Share) return DBF_ErrFileShared;

  sizbuf=((CSH_NO)csh)->Bufsiz;
  sizrec=_DBFP->H.SizRec;
  for(sizblk=0;sizblk+sizrec<=sizbuf;sizblk+=sizrec);
  if(!sizblk) return dbf_errno=DBF_ErrRecsiz;
  if((dbf_errno=_csf_AttachCache((CSF_NO)_DBFP,(CSH_NO)csh))==0) {
    _DBFP->BufWritten=FALSE;
    _DBFP->Cf.Recsiz=sizblk;
    if(_DBFP->H.NumRecs) {
      sizdata=_DBFP->H.NumRecs*sizrec;
      _DBFP->Cf.LastDataLen=
        (UINT)(sizdata-sizblk*(_DBFP->Cf.LastRecno=(sizdata-1)/sizblk));
    }
    else {
      _DBFP->Cf.LastDataLen=sizblk;
      _DBFP->Cf.LastRecno=(DWORD)(-1);
    }
  }
  return dbf_errno;
}
