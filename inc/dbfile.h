// DBFILE.H -- C++ Wrapper for DBF_FILE.LIB --

#ifndef __DBFILE_H
#define __DBFILE_H

#ifndef __DBF_FILE_H
#include <dbf_file.h>
#endif

bool __inline xisspace(char c) {return isspace((BYTE)c)!=0;}

class CDBFile {
public:

// File oriented user flags - used with Open() and Create() --
   enum {
      ReadWrite=0,Share=1,DenyWrite=2,ReadOnly=4,
      UserFlush=8,AutoFlush=16,ForceOpen=32,Sequential=64
   };
      
//Basic error codes common to DBF and TRX file types --
   enum {
      OK,
      ErrName,
      ErrFind,
      ErrArg,
      ErrMem,
      ErrRecsiz,
      ErrNoCache,
      ErrLocked,
      ErrRead,
      ErrWrite,
	  ErrCreate,
      ErrOpen,
      ErrClose,
      ErrDelete,
      ErrLimit,
      ErrShare,
      ErrFormat,
      ErrReadOnly,
      ErrSpace,
      ErrFileType,
	  ErrDiskIO,
      ErrLock,
	  ErrFileShared,
	  ErrUsrAbort,
	  ErrRename
     };

// Specific errors for DBF files --

enum {ErrEOF=DBF_ERR_BASE,
      ErrNotClosed,
      ErrFldDef,
     };

// Attributes
protected:
   DBF_NO m_dbfno;
    
// Constructors
public:
   CDBFile() {m_dbfno=0;};
   virtual ~CDBFile() {
	   if(m_dbfno) dbf_Close(m_dbfno);
   }
   
// Operations 
   int Open(const char* pszFileName,UINT mode=0) {
     return dbf_Open(&m_dbfno,(LPSTR)pszFileName,mode);}

   int Create(const char* pszFileName,UINT numflds,DBF_FLDDEF *pFld,UINT mode=0) {
     return dbf_Create(&m_dbfno,(LPSTR)pszFileName,mode,numflds,pFld);}

   BOOL Opened() const {return m_dbfno!=0;}

   DBF_NO GetDBF_NO() const {return m_dbfno;}

   void SwapHandle(CDBFile &db)
   {
	   DBF_NO d=m_dbfno;
	   m_dbfno=db.m_dbfno;
	   db.m_dbfno=d;
   }
   
   int Close() {
     int e=m_dbfno?dbf_Close(m_dbfno):0;
     m_dbfno=0;
     return e;
   }
   int CloseDel() {
	 int e=m_dbfno?dbf_CloseDel(m_dbfno):0;
     m_dbfno=0;
     return e;
   }

   static bool IsFldBlank(const BYTE *pBuf,UINT len)
   {
	  while(len && xisspace(*pBuf++)) len--;
	  return len==0;
   }

   int AllocCache(UINT bufsiz,UINT numbufs,BOOL makedef) {
         return dbf_AllocCache(m_dbfno,bufsiz,numbufs,makedef);}
   int AttachCache(TRX_CSH csh) {return dbf_AttachCache(m_dbfno,csh);}
   int DetachCache() {return dbf_DetachCache(m_dbfno);}
   int DeleteCache() {return dbf_DeleteCache(m_dbfno);} //Delete if last file 
   int FlushCache() {return csh_Flush(Cache());}
   int PurgeCache() {return csh_Purge(Cache());} 
   int Flush() {return dbf_Flush(m_dbfno);}
   
   // Positioning functions. All will first commit to the file (or cache)
   // the record at the current position (if revised) before repositioning
   // and loading the user's buffer with a copy of the new current record.

   int First() {return dbf_First(m_dbfno);}
   int Go(DWORD position) {return dbf_Go(m_dbfno,position);}
   int Last() {return dbf_Last(m_dbfno);}
   int Next() {return dbf_Next(m_dbfno);}
   int Prev() {return dbf_Prev(m_dbfno);}
   int Skip(long len) {return dbf_Skip(m_dbfno,len);}

   //Functions that add a new file record. The new record
   //becomes the user's current record --

   int   AppendBlank() {return dbf_AppendBlank(m_dbfno);}
   int   AppendZero() {return dbf_AppendZero(m_dbfno);}
   int   AppendRec(LPVOID pRec) {return dbf_AppendRec(m_dbfno,pRec);}

   // Record update functions. All but the last two mark the current
   // record (user's buffer copy) as having been modified --

   int   Mark() {return dbf_Mark(m_dbfno);}
   int   PutFld(LPVOID pBuf,UINT nFld) {
           return dbf_PutFld(m_dbfno,pBuf,nFld);}
   int   PutFldStr(LPCSTR pStr,UINT nFld) {
           return dbf_PutFldStr(m_dbfno,pStr,nFld);}
   int   PutRec(LPVOID pBuf) {return dbf_PutRec(m_dbfno,pBuf);}
   int   UnMark() {return dbf_UnMark(m_dbfno);}
   int   MarkHdr() {return dbf_MarkHdr(m_dbfno);}

   // Data retrieving functions --

   int   GetFld(LPVOID pBuf,UINT nFld) {
           return dbf_GetFld(m_dbfno,pBuf,nFld);}
   int   GetFldStr(LPSTR pStr,UINT nFld) {
           return dbf_GetFldStr(m_dbfno,pStr,nFld);}

   int GetFldStr(CString &s,UINT nFld)
   {
	   LPCSTR pFld=(LPCSTR)dbf_FldPtr(m_dbfno,nFld);
	   if(pFld) {
			s.SetString(pFld,dbf_FldLen(m_dbfno,nFld));
			return 0;
	   }
	   return dbf_errno;
   }

   int   GetTrimmedFldKey(LPSTR pStr,UINT nFld)
   {
	   if(dbf_FldKey(m_dbfno,pStr,nFld)) {
		    while(*pStr && pStr[(BYTE)*pStr]==' ') (*pStr)--;
			return 0;
	   }
	   return dbf_errno;
   }

   int GetTrimmedFldStr(LPSTR pStr,UINT nFld)
   {
	   if(dbf_FldStr(m_dbfno,pStr,nFld)) {
		    for(nFld=(UINT)strlen(pStr);nFld && pStr[nFld-1]==' ';nFld--);
			pStr[nFld]=0;
			return 0;
	   }
	   return dbf_errno;
   }

   int GetTrimmedFldStr(CString &s,UINT nFld)
   {
	   LPCSTR pFld=(LPCSTR)dbf_FldPtr(m_dbfno,nFld);
	   if(pFld) {
			LPCSTR p=pFld+dbf_FldLen(m_dbfno,nFld);
			while(p>pFld && xisspace(p[-1])) p--;
			s.SetString(pFld,(int)(p-pFld));
			return 0;
	   }
	   return dbf_errno;
  }

  int GetTrimmedFld(LPSTR buf,UINT nFld,UINT sizbuf)
  {
	LPCSTR pFld=(LPCSTR)dbf_FldPtr(m_dbfno,nFld);
	if(pFld) {
		nFld=dbf_FldLen(m_dbfno,nFld);
		if(nFld>=sizbuf) nFld=sizbuf-1;
		memcpy(buf,pFld,nFld);
		while(nFld && xisspace(buf[nFld-1])) --nFld;
	}
	else nFld=0;
	buf[nFld]=0;
	return nFld;
  }

  int   GetRec(LPVOID pBuf) {return dbf_GetRec(m_dbfno,pBuf);}

/*Data retrieving function returning the buffer argument (0 if error) --*/
   LPVOID Fld(LPVOID pBuf,UINT nFld) {return dbf_Fld(m_dbfno,pBuf,nFld);}
   LPSTR  FldStr(LPSTR pStr,UINT nFld) {return dbf_FldStr(m_dbfno,pStr,nFld);}
   LPVOID Rec(LPVOID pBuf) {return dbf_Rec(m_dbfno,pBuf);}
   LPBYTE HdrRes16() {return dbf_HdrRes16(m_dbfno);}
   LPBYTE HdrRes2() {return dbf_HdrRes2(m_dbfno);}
/* Return file status or description --*/
   static TRX_CSH DefaultCache() {return dbf_defCache;}

   static void FreeDefaultCache()
   {
	   if(dbf_defCache) {
		   csh_Free(dbf_defCache);
		   dbf_defCache=NULL;
	   }
   }

   TRX_CSH Cache() {return dbf_Cache(m_dbfno);}
   bool	   IsDeleted() {return *(LPCSTR)dbf_RecPtr(m_dbfno)=='*';}
   bool	   IsReadOnly() {return (dbf_OpenFlags(m_dbfno)&ReadOnly)!=0;}
   int     Errno() {return dbf_Errno(m_dbfno);}
   static LPSTR Errstr(UINT e) {return dbf_Errstr(e);}
   LPSTR   Errstr() {return dbf_Errstr(dbf_Errno(m_dbfno));}
   UINT	   FileID() {return dbf_FileID(m_dbfno);}
   DWORD   FileMode() {return dbf_FileMode(m_dbfno);}
   NPSTR   FileName() {return dbf_FileName(m_dbfno);}
   DBF_FLDDEF *FldDef(UINT nFld) {return dbf_FldDef(m_dbfno,nFld);}
   int     GetFldDef(DBF_FLDDEF FAR *pFld,UINT nFld,int count) {
             return dbf_GetFldDef(m_dbfno,pFld,nFld,count);}
   int     GetHdrDate(DBF_HDRDATE FAR *pDate) {
             return dbf_GetHdrDate(m_dbfno,pDate);}
   UINT	   FldOffset(UINT nFld)
   {
		DBF_pFLDDEF fp=dbf_FldDef(m_dbfno,nFld);
		return fp?fp->F_Off:0;
   }
   LPVOID  FldPtr(UINT nFld) {return dbf_FldPtr(m_dbfno,nFld);}
   PSTR    FldNam(PSTR pBuf,UINT nFld) {return dbf_FldNam(m_dbfno,pBuf,nFld);}
   LPCSTR  FldNamPtr(UINT nFld) {return dbf_FldNamPtr(m_dbfno,nFld);} 
   UINT    FldNum(LPCSTR pFldNam) {return dbf_FldNum(m_dbfno,pFldNam);}
   BYTE    FldDec(UINT nFld) {return dbf_FldDec(m_dbfno,nFld);}
   BYTE    FldLen(UINT nFld) {return dbf_FldLen(m_dbfno,nFld);}
   char    FldTyp(UINT nFld) {return dbf_FldTyp(m_dbfno,nFld);}
   void    FreeFldDef() {dbf_FreeFldDef(m_dbfno);}
   HANDLE  Handle() {return dbf_Handle(m_dbfno);}
   UINT    NumUsers() {return dbf_NumUsers(m_dbfno);}
   DWORD   NumRecs() {return dbf_NumRecs(m_dbfno);}
   UINT    NumFlds() {return dbf_NumFlds(m_dbfno);}
   NPSTR   PathName() {return dbf_PathName(m_dbfno);}
   DWORD   Position() {return dbf_Position(m_dbfno);}
   LPVOID  RecPtr() {return dbf_RecPtr(m_dbfno);}
   UINT    SizRec() {return dbf_SizRec(m_dbfno);}
   UINT    SizHdr() {return dbf_SizHdr(m_dbfno);}
   UINT    OpenFlags() {return dbf_OpenFlags(m_dbfno);}
   UINT	   Type() {return dbf_Type(m_dbfno);}

};
#endif
