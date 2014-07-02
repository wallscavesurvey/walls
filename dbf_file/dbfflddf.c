/* DBFFLDDF.C -- Initializes DBF_FLD array. Called by functions
   requiring field-level access.*/

#include <string.h>
#include "a__dbf.h"

/*=================================================================*/

DBF_pFLDDEF PASCAL _dbf_GetFldDef(UINT nFld)
{
  DBF_pFILE dbfp=_PDBFF;
  DBF_pFLDDEF pfd;
  int n,sizFld,len_ttl;
  DBF_FILEFLD ff;
  DWORD fileoff;
  
  #ifdef _DBF2
  BOOL dbf2_type;
  DBF2_FILEFLD *pff2=(DBF2_FILEFLD *)&ff;
  #endif

  if(--nFld>=dbfp->NumFlds) {
    dbf_errno=DBF_ErrArg;
    return 0;
  }
  if(dbfp->pFldDef) return dbfp->pFldDef+nFld;
  
  if((pfd=(DBF_pFLDDEF)dos_AllocNear(dbfp->NumFlds*sizeof(DBF_FLDDEF)))==0)
  {
    dbf_errno=DBF_ErrMem;
    return 0;
  }
  dbfp->pFldDef=pfd;
  
  #ifdef _DBF2
  if((dbf2_type=(dbfp->H.FileID==DBF2_FILEID))) {
	  fileoff=sizeof(DBF2_HDR);
	  sizFld=sizeof(DBF2_FILEFLD);
  }
  else
  #endif
  {
	  fileoff=sizeof(DBF_HDR);
	  sizFld=sizeof(DBF_FILEFLD);
  }

  if((_PDBFU->U_Mode&DBF_Share) && _dbf_LockHdr(_PDBFF,DOS_ShareLock)) {
	dos_FreeNear(dbfp->pFldDef);
	dbfp->pFldDef=NULL;
	dbf_errno=DBF_ErrLock;
	return 0;
  }
  
  /*Read 32-byte (or 16-byte DBF2) field descriptors one-at-a-time --*/
  len_ttl=1;
  for(n=dbfp->NumFlds;n;n--) {
    if(dos_Transfer(0,dbfp->Cf.Handle,&ff,fileoff,sizFld)) break;
    memcpy(pfd->F_Nam,ff.F_Nam,sizeof(ff.F_Nam)+sizeof(ff.F_Typ));
	pfd->F_Nam[10]=0; //protect against illegal length
    
    #ifdef _DBF2
    if(dbf2_type) {
      pfd->F_Len=pff2->F_Len;
      pfd->F_Dec=pff2->F_Dec;
    }
    else
    #endif
    {
      pfd->F_Len=ff.F_Len;
      pfd->F_Dec=ff.F_Dec;
    }
    pfd->F_Off=len_ttl;
    len_ttl+=pfd->F_Len;
    fileoff+=sizFld;
    pfd++;
  }

  if(_PDBFU->U_Mode&DBF_Share)_dbf_LockHdr(_PDBFF,DOS_UnLock);

  if(n || len_ttl!=(int)dbfp->H.SizRec) {
    dos_FreeNear(dbfp->pFldDef);
    dbfp->pFldDef=0;
    FORMAT_ERROR();
    return 0;
  }
  return dbfp->pFldDef+nFld;
}

DBF_pFLDDEF DBFAPI dbf_FldDef(DBF_NO dbf,UINT nFld)
{
  return _dbf_GetDbfP(dbf)?_dbf_GetFldDef(nFld):0;
}

void DBFAPI dbf_FreeFldDef(DBF_NO dbf)
{
  if(_GETDBFP && _DBFP->pFldDef) {
    dos_Free(_DBFP->pFldDef);
    _DBFP->pFldDef=0;
  }
}
