#ifndef __SHPDBF_H
#define __SHPDBF_H
#include <dbfile_shp.h>

//Used when sorting N and F fields (the dBase length limits are 19 and 20.
#define SIZ_NFLD_BUF 24

#ifndef __A__DBF_H
#pragma pack(1)
typedef struct DBF_sHDR {
  /*Updatable portion (16-bytes) --*/
  BYTE  FileID;           /*03h if dBase-III, 83h or 8Bh if .DBT exists*/
  DBF_HDRDATE Date;       /*Date of last close of updated file*/
  DWORD NumRecs;          /*Total number of records in file*/
  WORD  SizHdr;           /*File offset of first record*/
  WORD  SizRec;           /*Record size*/
  WORD  Res1;             /*Reserved*/
  BYTE  Incomplete;       /*1 if disk version is not complete*/
  BYTE  Encrypted;        /*1 if encrypted (not used here)*/

  /*The remaining 16-byte portion is currently unused but will
    be available for application to examine and/or modify --*/

  BYTE  Res2[12];         /*Reserved for LAN use*/
  BYTE  Mdx;              /*1 if production .MDX*/
  BYTE  Res3[3];          /*Reserved*/
} DBF_HDR;
#pragma pack()
#endif

class CShpDBF :	public CDBFile
{

public:
	CShpDBF(void) : m_hFM(NULL), m_ptr(NULL), m_recptr(NULL), m_rec(0), m_pFld(NULL) {}

	~CShpDBF(void)
	{
		UnMapFile();
	}

private:
	bool Map();
	void UnMapFile();
	bool ChkAppend();
	bool MapFile(UINT initrecs=0,UINT increcs=50);

public:
	void SetRecIncrement(UINT nRecs) {m_increcs=nRecs;}
	UINT GetRecIncrement(UINT nRecs) {return m_increcs;}

	int Close();
	int CloseDel()
	{
		UnMapFile();
		return CDBFile::CloseDel();
	}

	int Open(const char* pszFileName,UINT mode=0);
	int Create(const char* pszFileName,UINT numflds,DBF_FLDDEF *pFld,UINT initRecs);

	bool IsMapped() {return m_ptr!=NULL;}
	bool IsReadOnly() {return !m_bRW;}
    bool IsDeleted() {return *(LPCSTR)RecPtr()=='*';}

	LPBYTE RecPtr()
	{
		ASSERT(m_recptr);
		return m_recptr;
	}

	UINT FldOffset(UINT nFld)
	{
		return m_pFld[--nFld].F_Off;
	}

	LPBYTE FldPtr(UINT nFld)
	{
		ASSERT(m_recptr);
		return m_recptr+FldOffset(nFld);
	}

	LPBYTE RecPtr(UINT rec)
	{
		ASSERT(m_tableptr);
		return m_tableptr+(--rec)*m_sizrec;
	}

	LPBYTE FldPtr(UINT rec,UINT nFld)
	{
		return RecPtr(rec)+FldOffset(nFld);
	}

	UINT    FldNum(LPCSTR pFldNam) {return CDBFile::FldNum(pFldNam);}
	LPCSTR  FldNamPtr(UINT nFld) {return m_pFld[--nFld].F_Nam;} 
	BYTE    FldDec(UINT nFld) {return m_pFld[--nFld].F_Dec;}
	BYTE    FldLen(UINT nFld) {return m_pFld[--nFld].F_Len;}
	char    FldTyp(UINT nFld) {return m_pFld[--nFld].F_Typ;}

	int StoreTrimmedFldData(LPBYTE recPtr,LPCSTR text,int nFld);

	int Flush(LPCVOID lpBase=0,DWORD dwBytes=0)
	{
		ASSERT(m_bRW && m_dbfp);
		m_dbfp->FileChg=TRUE;
		return (FlushViewOfFile(lpBase?lpBase:m_ptr,dwBytes) &&
				FlushFileBuffers(m_handle))?0:ErrWrite;
	}

	int FlushRec()
	{
		ASSERT(m_bRW && m_rec && m_dbfp);
		m_dbfp->FileChg=TRUE;
		return FlushViewOfFile(m_recptr,m_sizrec)?0:ErrWrite;
	}

	UINT SizHdr()
	{
		return m_sizhdr;
	}

	UINT SizRec()
	{
		return m_sizrec;
	}

	DWORD NumRecs()
	{
		return m_numrecs;
	}

	UINT NumFlds()
	{
		return m_numflds;
	}

	bool HasMemos()
	{
		return FileID()==DBF_FILEID_MEMO;
	}

   // Positioning functions. If unmapped will first commit to the file (or cache)
   // the record at the current position (if revised) before repositioning
   // and loading the user's buffer with a copy of the new current record.

	int Go(DWORD rec)
	{
		if(!rec || rec>m_numrecs) return rec?ErrEOF:ErrArg;
		m_recptr=m_tableptr+(rec-1)*m_sizrec;
		m_rec=rec;
		return 0;
	}

   int First() {return Go(1);}
   int Last() {return Go(NumRecs());}
   int Next() {return Go(m_rec+1);}
   int Prev() {return Go(m_rec-1);}
   int Skip(long len) {return Go(m_rec+len);}

   bool FieldsMatch(CShpDBF *pdb)
   {
		return m_numflds==pdb->m_numflds && 
			!memcmp(m_pFld,pdb->m_pFld,m_numflds*sizeof(DBF_FLDDEF));
   }

   int FieldsCompatible(CShpDBF *pdb);

   DWORD Position() {return m_rec;}

   //Functions that add a new file record. The new record
   //becomes the user's current record --

   int AppendZero() {ASSERT(0); return ErrFormat;}

   int AppendBlank()
   {
	   if(!ChkAppend()) return ErrFormat;
	   memset(m_recptr,' ',m_sizrec);
	   return 0; 
   }
   int AppendRec(LPVOID pRec)
   {
	   if(!ChkAppend()) return ErrFormat;
	   memcpy(m_recptr,pRec,m_sizrec);
	   return 0;
   }

   int   PutRec(LPVOID pBuf)
   {
	   memcpy(m_recptr,pBuf,m_sizrec);
	   return 0;
   }

   // Data retrieving functions -- implement as needed
	LPCSTR GetTrimmedFldStr(LPBYTE pRec,CString &s,UINT nFld);
 
	int GetTrimmedFldStr(CString &s,UINT nFld)
	{
		return (m_recptr && GetTrimmedFldStr(m_recptr,s,nFld))?0:ErrArg;
	}

	int GetBufferFld(LPBYTE recBuf,UINT nFld,LPSTR pDest,UINT szDest);

	int GetRecordFld(UINT nFld,LPSTR pDest,UINT szDest)
	{
		return GetBufferFld(m_recptr,nFld,pDest,szDest);
	}

	void GetRec(LPVOID pBuf)
	{
		memcpy(pBuf,m_recptr,m_sizrec);
	}

	DBF_pFLDDEF m_pFld;

	static bool IsNumericType(char c)
	{
		return (c=='N' || c=='F');
	}
	static bool TypesCompatible(char cSrc, char cDst);

private:
	DBF_pFILE m_dbfp;
	LPBYTE m_recptr;
	UINT m_rec;
	HANDLE m_hFM;
	LPBYTE m_ptr,m_tableptr;
	UINT m_sizrec,m_sizhdr;
	UINT m_numrecs,m_numflds,m_maxrecs,m_increcs;
	bool m_bRW;
	HANDLE m_handle;
};
#endif //__SHPDBF_H
