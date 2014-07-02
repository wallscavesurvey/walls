/* DBFALLOC.C -- Functions required by DBFCREAT.C and DBFOPEN.C*/

#include "a__dbf.h"

/*Default DBF file extension --*/
LPCSTR dbf_ext=DBF_DFLT_EXT;

/*Global variables referenced by applications --*/
TRX_CSH         dbf_defCache;
UINT            dbf_errno;

/*Globals initialized by _dbf_GetTrx() --*/
DBF_pFILE    _dbf_pFile;
DBF_pFILEUSR _dbf_pFileUsr;

/*Each pointer in _dbf_fileMap[] corresponds to a unique file, or
  handle returned by dos_Openfile(), etc. The pointers are maintained
  so that all valid (nonzero) elements are contiguous at the front
  of the array. */

DBF_pFILE    _dbf_fileMap[DBF_MAX_FILES];

/*Each pointer in _dbf_usrMap[] corresponds to a unique
  file/user connection. Nonzero elements are NOT necessarily
  contiguous. Index in this array is used to construct DBF_NO:
  DBF_NO = index+1. */

DBF_pFILEUSR _dbf_usrMap[DBF_MAX_USERS];

static BOOL bExit_Set=FALSE;

/*=================================================================*/

apfcn_v _dbf_FreeFile(DBF_pFILE dbfp)
{
   int i;

   for(i=0;i<DBF_MAX_FILES;i++)
     if(_dbf_fileMap[i]==dbfp) {
       while(++i<DBF_MAX_FILES)
         if((_dbf_fileMap[i-1]=_dbf_fileMap[i])==0) break;
       if(i==DBF_MAX_FILES) _dbf_fileMap[DBF_MAX_FILES-1]=0;
       break;
     }
   if(dbfp->pFldDef) dos_Free(dbfp->pFldDef);
   dos_Free(dbfp);
}

apfcn_i _dbf_AllocFile(void)
{
  /*All fields are initialized to zero --*/
  if((_dbf_pFile=(DBF_pFILE)dos_Alloc(sizeof(DBF_FILE)))==0)
    return DBF_ErrMem;
  return 0;
}

static void _dbf_Exit(void *f)
{
	FIX_EX(f);
	
	/*The above inserts f!=0 into chain and exits, or runs
	  fcns below after more recently inserted fcns are executed with f=0.*/

	/*Close all open DBFs upon call to arcexit() or _arcexit(0)*/
	dbf_Clear();
	bExit_Set=FALSE;
}

apfcn_i _dbf_AllocUsr(DBF_NO *pdbf,DBF_pFILE dbfp,UINT mode)
{
  int usrIndex;
  DBF_pFILEUSR usrp;
  UINT sizRecbuf=dbfp->H.SizRec;

  for(usrIndex=0;usrIndex<DBF_MAX_USERS;usrIndex++)
    if(_dbf_usrMap[usrIndex]==0) break;
  if(usrIndex==DBF_MAX_USERS) return DBF_ErrLimit;

  if((mode&(DBF_Share+DBF_ReadOnly))==DBF_Share) sizRecbuf*=2;

  if((usrp=(DBF_pFILEUSR)dos_Alloc(sizeof(DBF_FILEUSR)+sizRecbuf-1))==0)
    return DBF_ErrMem;

  usrp->U_pFile=dbfp;
  usrp->U_Index=usrIndex;
  usrp->U_Mode=mode&
    (DBF_Share+DBF_DenyWrite+DBF_ReadOnly+DBF_UserFlush+
    DBF_AutoFlush+DBF_NoCloseDate);
  usrp->U_pNext=dbfp->pFileUsr;
  _dbf_usrMap[usrIndex]=dbfp->pFileUsr=_dbf_pFileUsr=usrp;
  *pdbf=usrIndex+1;
  _dbf_pFileUsr=usrp;

  if(!bExit_Set) {
	  bExit_Set=TRUE;
	  _dos_exit(_dbf_Exit);
  }
  return 0;
}

apfcn_v _dbf_FreeUsr(DBF_pFILEUSR usrp)
{
  DBF_pFILEUSR u=(usrp->U_pFile)->pFileUsr;
  DBF_pFILEUSR uprev=0;

  while(u && u!=usrp) {
    uprev=u;
    u=u->U_pNext;
  }
  if(u) {
    u=usrp->U_pNext;
    if(uprev) uprev->U_pNext=u;
    else (usrp->U_pFile)->pFileUsr=u;
    _dbf_usrMap[usrp->U_Index]=0;
    dos_Free(usrp);
  }
}

apfcn_i _dbf_FindFile(NPSTR name)
{
  int i;
  DBF_pFILE pf;

  for(i=0;i<DBF_MAX_FILES;i++)
    if((pf=_dbf_fileMap[i])==0 || dos_MatchName(pf->Cf.Handle,name)) break;
  return i;
}

/*End of DBFALLOC.C*/
