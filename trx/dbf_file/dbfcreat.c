/* DBFCREAT.C -- DBF File Creation*/

#include <string.h>
#include "a__dbf.h"

#ifdef _DBF2
static apfcn_i _Fill_DB2_hdr(DBF_pFILE dbfp)
{
        char buf[DBF2_SIZHDR+1-sizeof(DBF2_HDR)-sizeof(DBF2_FILEFLD)];
	UINT foffset=sizeof(DBF2_HDR)+sizeof(DBF2_FILEFLD)*dbfp->NumFlds;
	   
        memset(buf,0,sizeof(buf));
	buf[0]=0x0D;
        buf[DBF2_SIZHDR-foffset]=0x1A;
        if(dos_Transfer(1,dbfp->Cf.Handle,buf,foffset,DBF2_SIZHDR+1-foffset)) return DBF_ErrWrite;
	return 0;
}
#endif

static apfcn_i _PutFldDef(DBF_pFILE dbfp,DBF_FLDDEF *pFldDef)
{
  int numf;
  DBF_FILEFLD f;
  WORD eof=(((WORD)DBF_EOFMARK)<<8)+0x0D;
  UINT sizFld;
  DWORD fileoff;
  
  #ifdef _DBF2
  BOOL dbf2_type=(dbfp->H.FileID==DBF2_FILEID);
  DBF2_FILEFLD *pf2=(DBF2_FILEFLD *)&f;
  UINT fldoff=1;
  if(dbf2_type) {
    sizFld=sizeof(DBF2_FILEFLD);
    fileoff=sizeof(DBF2_HDR);
  }
  else
  #endif
  {
 	sizFld=sizeof(DBF_FILEFLD);
    fileoff=sizeof(DBF_HDR);
  }
  numf=dbfp->NumFlds;

  while(numf--) {

    /*For safety, we check the DBF_FLDDEF structure for proper
      initialization. For now, we will accept either upper or
      lower case in the name.*/
    if(strlen(pFldDef->F_Nam)>=sizeof(f.F_Nam) || !pFldDef->F_Len) return DBF_ErrFldDef;

    memset(&f,0,sizeof(DBF_FILEFLD));

    strcpy(f.F_Nam,pFldDef->F_Nam);
    f.F_Typ=pFldDef->F_Typ;
    f.F_Len=pFldDef->F_Len;
    f.F_Dec=pFldDef->F_Dec;
    #ifdef _DBF2
    if(dbf2_type) {
		pf2->F_Len=f.F_Len;
		pf2->F_Dec=f.F_Dec;
                pf2->F_Off=fldoff;
                fldoff+=pf2->F_Len;
    }
    #endif
    pFldDef++;

	if(dos_Transfer(1,dbfp->Cf.Handle,&f,fileoff,sizFld)) return DBF_ErrWrite;
	fileoff+=sizFld;
  }
  
  #ifdef _DBF2
  if(dbf2_type) {
     if(_Fill_DB2_hdr(dbfp)) return DBF_ErrWrite;
  }
  else
  #endif
  if(dos_Transfer(1,dbfp->Cf.Handle,&eof,fileoff,2)) return DBF_ErrWrite;
  return 0;
}

int DBFAPI dbf_Create(DBF_NO *pdbf,LPCSTR pathName,UINT mode,
                         UINT numflds,DBF_FLDDEF *pFldDef)
{
  int e;
  NPSTR fullpath;
  int mapIndex;
  DBF_pFILE dbfp;
  #ifdef _DBF2
  BOOL dbf2_type=FALSE;
  #endif
  
  *pdbf=0;

  if(mode&DBF_Share) return DBF_ErrFileShared;

  if(mode&DBF_TypeDBF2) {
    #ifdef _DBF2
    mode&=~DBF_TypeDBF2;
    e=DBF2_MAXFLD;
    dbf2_type=TRUE;
    #else
    e=0; /*Fail if DBF2 not supported*/
    #endif
  }
  else e=DBF_MAXFLD;

  if(pathName==0L || !numflds || numflds>(UINT)e || !pFldDef) {
    e=DBF_ErrArg;
    goto _return_e;
  }

  if(((fullpath=dos_FullPath(pathName,dbf_ext))==0 && (e=DBF_ErrName)!=0) ||
    ((mapIndex=_dbf_FindFile(fullpath))==DBF_MAX_FILES && (e=DBF_ErrLimit)!=0) ||
    /*Can't create it if it's already open --*/
    (_dbf_fileMap[mapIndex]!=0 && (e=DBF_ErrShare)!=0) ||
    ((mode&DBF_ReadOnly) && (e=DBF_ErrReadOnly)!=0)) goto _return_e;

  if((e=_dbf_AllocFile())==0) {
    _dbf_fileMap[mapIndex]=dbfp=_dbf_pFile;
    dbfp->NumFlds=numflds;
    #ifdef _DBF2
    if(dbf2_type) {
	    dbfp->H.FileID=DBF2_FILEID;
	    dbfp->Cf.Offset=dbfp->H.SizHdr=DBF2_SIZHDR;
    }
    else
    #endif
    {
	    dbfp->H.FileID=DBF_FILEID;
		for(e=0;e<(int)numflds;e++)
			if(pFldDef[e].F_Typ=='M') {
				dbfp->H.FileID=DBF_FILEID_MEMO;
				break;
			}
	    dbfp->Cf.Offset=dbfp->H.SizHdr=sizeof(DBF_HDR)+numflds*sizeof(DBF_FILEFLD)+1;
    }
    
    /*Record size is needed for functions below --*/
    dbfp->H.SizRec=(WORD)1;
    for(e=0;(UINT)e<numflds;e++) dbfp->H.SizRec+=pFldDef[e].F_Len;

    if((e=_dbf_AllocUsr(pdbf,dbfp,mode))==0) {
      if(!dbf_defCache || (e=dbf_AttachCache(*pdbf,dbf_defCache))==0) {
        if((e=dos_CreateFile(&dbfp->Cf.Handle,fullpath,(mode&DOS_Sequential)))==0) {
          dbfp->H.Incomplete=(mode&(DBF_UserFlush+DBF_AutoFlush))?0:1;
          dos_GetSysDate(&dbfp->H.Date);
          /*Finish by writing out the complete header, including the
            field descriptors --*/
          if((e=_dbf_WriteHdr(dbfp))==0 &&
             (e=_PutFldDef(dbfp,pFldDef))==0) goto _return_e;

          /*If we cannot write the header, close and delete the file --*/
          dos_CloseDelete(dbfp->Cf.Handle);
        }
        if(dbf_defCache) _csf_DeleteCache((CSF_NO)dbfp,(CSH_NO *)&dbf_defCache);
      }
      _dbf_FreeUsr(_dbf_pFileUsr);
      *pdbf=0;
    }
    _dbf_FreeFile(dbfp);
  }

_return_e:
  return dbf_errno=e;
}
/*End of DBFCREAT.C*/
