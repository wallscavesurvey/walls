#include "stdafx.h"
#include <ShpDBF.h>

LPCSTR CShpDBF::GetTrimmedFldStr(LPBYTE recBuf,CString &s,UINT nFld)
{
	LPCSTR pFld=(LPCSTR)recBuf+FldOffset(nFld);
	LPCSTR p=pFld+m_pFld[--nFld].F_Len;
	while(p>pFld && xisspace(*pFld)) pFld++;
	while(p>pFld && xisspace(p[-1])) p--;
	s.SetString(pFld,(int)(p-pFld));
	return s;
}

int CShpDBF::GetBufferFld(LPBYTE recBuf,UINT nFld,LPSTR pDest,UINT szDest)
{
	LPCSTR pFld=(LPCSTR)recBuf+FldOffset(nFld);
	LPCSTR p=pFld+m_pFld[--nFld].F_Len;
	if((UINT)(p-pFld)>szDest) p=pFld+szDest;
	while(p>pFld && xisspace(*pFld)) pFld++;
	while(p>pFld && xisspace(p[-1])) p--;
	if((nFld=p-pFld)==szDest) --nFld;
	memcpy(pDest,pFld,nFld);
	pDest[nFld]=0;
	return nFld;
}

bool CShpDBF::Map()
{
	UINT sizMap=m_maxrecs?(m_sizhdr+m_maxrecs*m_sizrec):0;

	m_hFM=CreateFileMapping(
		m_handle,		//handle
		NULL,			//LPSECURITY_ATTRIBUTES
		m_bRW?PAGE_READWRITE:PAGE_READONLY,	//flProtect
		0,sizMap,			//dwMaximumSizeHigh,dwMaximumSizeLow (leave zero for current file size)
		NULL			//lpName (used for sharing only)
	);
	if(!m_hFM) return false;

	m_ptr=(LPBYTE)MapViewOfFile(
		m_hFM,
		m_bRW?FILE_MAP_ALL_ACCESS:FILE_MAP_READ,  //dwDesiredAccess
		0,0,            //dwFileOffsetHigh, dwFileOffsetLow (start map at file beginning)
		0				//dwNumberOfBytesToMap (0 for entire file)
	);

	if(!m_ptr) {
		CloseHandle(m_hFM);
		m_hFM=NULL;
		return false;
	}
	m_tableptr=m_ptr+m_sizhdr;
	m_rec=0;
	m_recptr=NULL;
	return true;
}

void CShpDBF::UnMapFile()
{
	if(m_ptr) {
		//if(m_bRW) VERIFY(Flush()); //causes assert if no updates!
		VERIFY(UnmapViewOfFile(m_ptr));
		m_ptr=NULL;
	}
	if(m_hFM) {
		VERIFY(CloseHandle(m_hFM));
		m_hFM=NULL;
	}
}

bool CShpDBF::ChkAppend()
{
	if(!m_bRW || !m_ptr) return false;
	if(m_maxrecs==m_numrecs) {
	   UnMapFile();
	   m_maxrecs+=m_increcs;
	   if(!Map()) return false;
	}
	DWORD *pdw=&((DBF_HDR *)m_ptr)->NumRecs;
	m_numrecs=++(*pdw);
	VERIFY(FlushViewOfFile(pdw,sizeof(DWORD)));
	ASSERT(m_dbfp->H.NumRecs==m_numrecs-1);
	m_dbfp->H.NumRecs++;
	m_dbfp->FileChg=TRUE;
	m_recptr=RecPtr(m_rec=m_numrecs);
	return true;
}

bool CShpDBF::MapFile(UINT initrecs,UINT increcs)
{
	if(m_ptr || !Opened()) {
		ASSERT(0);
		return false;
	}

	VERIFY(m_handle=CDBFile::Handle());

	m_dbfp=_dbf_GetDbfP(m_dbfno);
	m_sizhdr=CDBFile::SizHdr();
	m_bRW=(CDBFile::OpenFlags()&ReadOnly)==0;
	m_sizrec=CDBFile::SizRec();
	m_numrecs=CDBFile::NumRecs();

	//if initrecs==0, start by mapping current file length
	m_maxrecs=initrecs;
	if(!Map()) return false;

	VERIFY(m_pFld=CDBFile::FldDef(1)); //Allocate field definitions
	m_numflds=CDBFile::NumFlds();
	if(!initrecs) m_maxrecs=m_numrecs; //remap on first append
	m_increcs=increcs; //extend length by this many records on each remap
	return true;
}

int CShpDBF::Close()
{
	if(!m_dbfno) return ErrArg;
	UnMapFile();
	if(m_maxrecs>m_numrecs) {
		ASSERT(m_bRW);
		SetFilePointer(m_handle,m_sizhdr+m_numrecs*m_sizrec,0,FILE_BEGIN);
		SetEndOfFile(m_handle);
	}
	return CDBFile::Close();
}

int CShpDBF::Open(const char* pszFileName,UINT mode)
{
	int iRet=CDBFile::Open(pszFileName,mode);
	if(!iRet && !MapFile()) {
		Close();
		iRet=ErrMem;
	}
	return iRet;
}

int CShpDBF::Create(const char* pszFileName,UINT numflds,DBF_FLDDEF *pFld,UINT initRecs)
{
	int iRet=CDBFile::Create(pszFileName,numflds,pFld);
	if(!iRet && !(iRet=CDBFile::Close()) && !(iRet=CDBFile::Open(pszFileName))) {
		if(!MapFile(initRecs)) {
			CloseDel();
			iRet=ErrMem;
		}
	}
	return iRet;
}

void CShpDBF::StoreTrimmedFldData(LPBYTE recPtr,LPCSTR text,int nFld)
{
	char fldData[256];
	*fldData=0;
	CString s(text);
	s.Trim();

	LPBYTE pFld=recPtr+FldOffset(nFld);
	int fldLen=FldLen(nFld);
	char typ=FldTyp(nFld);
	int len=0;

	switch(typ) {
		case 'F' :
		case 'N' :  {
						if(s.IsEmpty()) {
							len=0; //allow empty numeric fields
						}
						else {
							BYTE dec=FldDec(nFld);
							len=_snprintf(fldData,fldLen+1,(typ=='N')?"%*.*f":"%*.*e",fldLen,dec,atof(s));
							if(len<0 || len>fldLen) {
								len=_snprintf(fldData,fldLen+1,"%*g",fldLen,atof(s));
								fldData[fldLen]=0;
								if(fldLen==1 && *fldData=='-') len=0;
							}
						}
					}
					break;
		case 'D' :	ASSERT(fldLen==8);
		case 'C' :	strncpy(fldData,s,fldLen);
					fldData[fldLen]=0;
					len=strlen(fldData);
					ASSERT(typ!='D' || len==8);
					break;
		case 'L' :  *fldData=toupper(*text);
					if(*fldData=='T') *fldData='Y';
					else if(*fldData!=' ' && *fldData!='Y') *fldData='N';
					fldData[1]=0;
					len=1;
					break;
		case 'M' :  ASSERT(FALSE);
					return;
	}
	if(len>fldLen) len=fldLen;
	if(len) memcpy(pFld,fldData,len);
	if(len<fldLen) memset(pFld+len,' ',fldLen-len);
}
