/* DBFGO.C -- Set current position in DBF file*/

#include <string.h>
#include "a__dbf.h"

static apfcn_i _dbf_GetCshRec(DBF_pFILE dbfp)
{
  DBF_pFILEUSR usrp=_PDBFU;
  CSH_HDRP hp;
  DWORD blkno;

  if((hp=_csf_GetRecord((CSF_NO)dbfp,blkno=usrp->U_Recoff/dbfp->Cf.Recsiz,0))==0)
      return _dbf_pFile->Cf.Errno;
  memcpy(usrp->U_Rec,hp->Buffer+(int)(usrp->U_Recoff-(blkno*dbfp->Cf.Recsiz)),
     dbfp->H.SizRec);
  return 0;
}

apfcn_i _dbf_LoadUsrRec(void)
{
  DBF_pFILE dbfp=_PDBFF;
  DBF_pFILEUSR usrp=_PDBFU;
  int e;

  if(dbfp->Cf.Csh) e=_dbf_GetCshRec(dbfp);
  else {
	  if((usrp->U_Mode&DBF_Share) && 
		 (e=_dbf_LockFilRec(dbfp,DOS_ShareLock))) goto _ret;

	  if((e=dos_Transfer(0,dbfp->Cf.Handle,usrp->U_Rec,
		dbfp->Cf.Offset+usrp->U_Recoff,
		dbfp->H.SizRec))!=0) dbfp->Cf.Errno=e;

	  if(usrp->U_Mode&DBF_Share) {
		  _dbf_LockFilRec(dbfp,DOS_UnLock);

	      if(!(usrp->U_Mode&DBF_ReadOnly))
			memcpy(usrp->U_Rec+dbfp->H.SizRec,usrp->U_Rec,dbfp->H.SizRec);
	  }
  }
_ret:
  if(e) usrp->U_Recno=0;
  return dbf_errno=e;
}

int DBFAPI dbf_Go(DBF_NO dbf,DWORD position)
{
  int e=0;
  if(!_GETDBFU) return DBF_ErrArg;

  if(_PDBFF->H.NumRecs<position) {
	  if(!(_DBFU->U_Mode&DBF_Share) || (e=_dbf_RefreshNumRecs())<(int)position) 
		  return dbf_errno=(e<0)?-e:DBF_ErrEOF;
  }

  /*A move to the current position (or rec 0) will simply commit the
    current record if it was updated. If it was unmarked,
    the current record will be read again into the user's buffer --*/

  if(!position) position=_DBFU->U_Recno;
  if((position==_DBFU->U_Recno) && _DBFU->U_RecChg) return _dbf_CommitUsrRec();
  if(_dbf_CommitUsrRec()!=0) return dbf_errno;

  _DBFU->U_Recno=position;
  _DBFU->U_Recoff=_PDBFF->H.SizRec*(position-1);
  return _dbf_LoadUsrRec();
}
