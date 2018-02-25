// DBFILE.H -- C++ Wrapper for DBF_FILE.LIB --

#ifndef __DBFILE_H
#define __DBFILE_H

#ifndef __DBF_FILE_H
#include "a__dbf.h"
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
   virtual ~CDBFile() { if(m_dbfno) dbf_Close(m_dbfno);}
   
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

   int     Errno() {return dbf_Errno(m_dbfno);}
   static  LPSTR Errstr(UINT e) {return dbf_Errstr(e);}
   LPSTR   Errstr() {return dbf_Errstr(dbf_Errno(m_dbfno));}
   UINT	   FileID() {return dbf_FileID(m_dbfno);}
   DWORD   FileMode() {return dbf_FileMode(m_dbfno);}
   NPSTR   FileName() {return dbf_FileName(m_dbfno);}
   DBF_pFLDDEF FldDef(UINT nFld) {return dbf_FldDef(m_dbfno,nFld);}
   int     GetHdrDate(DBF_HDRDATE FAR *pDate) {
             return dbf_GetHdrDate(m_dbfno,pDate);}
   UINT    FldNum(LPCSTR pFldNam) {return dbf_FldNum(m_dbfno,pFldNam);}
   HANDLE  Handle() {return dbf_Handle(m_dbfno);}
   DWORD   NumRecs() {return dbf_NumRecs(m_dbfno);}
   UINT    NumFlds() {return dbf_NumFlds(m_dbfno);}
   NPSTR   PathName() {return dbf_PathName(m_dbfno);}
   UINT    SizRec() {return dbf_SizRec(m_dbfno);}
   UINT    SizHdr() {return dbf_SizHdr(m_dbfno);}
   UINT    OpenFlags() {return dbf_OpenFlags(m_dbfno);}
 
};
#endif
