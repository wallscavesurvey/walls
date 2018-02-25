#include <stdafx.h>
#include "ShpDBF.h"

bool CShpDBF::TypesCompatible(char cSrc, char cDst)
{
	return cSrc==cDst || (cSrc=='C' && cDst=='M') || (cSrc=='M' && cDst=='C') || (IsNumericType(cSrc) && cDst=='C') ||
	    (cSrc=='C' && IsNumericType(cDst)) || (IsNumericType(cSrc) && IsNumericType(cDst));
}


int CShpDBF::FieldsCompatible(CShpDBF *pdb)
{
	//identically named fields must have same type or both either 'M' or 'C'
	//return count of matching fields
	int n=0;
	for(BYTE f=1;f<=NumFlds();f++) {
		BYTE fsrc=pdb->FldNum(FldNamPtr(f));
		if(fsrc) {
			if(!CShpDBF::TypesCompatible(FldTyp(f),pdb->FldTyp(fsrc))) return -1;
			/*
			if((FldTyp(f)!=pdb->FldTyp(fsrc)) && (!strchr("CM",pdb->FldTyp(fsrc)) || !strchr("CM",FldTyp(f)))) {
				return -1;
			}
			*/
			n++;
		}
	}
	return n;
}

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
		0				//dwNumberOfBytesToMap (0 for entire file file mapping)
	);

	if(!m_ptr) {
		CloseHandle(m_hFM);
		m_hFM=NULL;
		return false;
	}
	m_tableptr=m_ptr+m_sizhdr;
	return true;
}

void CShpDBF::UnMapFile()
{
	if(m_ptr) {
		if(m_bRW) VERIFY(FlushViewOfFile(m_ptr,0));
		VERIFY(UnmapViewOfFile(m_ptr));
		m_tableptr=m_ptr=NULL;
	}
	if(m_hFM) {
		VERIFY(CloseHandle(m_hFM));
		m_hFM=NULL;
	}
	if(m_maxrecs>m_numrecs) {
		ASSERT(m_bRW);
		SetFilePointer(m_handle,m_sizhdr+m_numrecs*m_sizrec,0,FILE_BEGIN);
		SetEndOfFile(m_handle);
	}
	m_maxrecs=0;
}

bool CShpDBF::ChkAppend()
{
	if(!IsMapped()) {
		if(dbf_AppendBlank(m_dbfno)) return false;
		dbf_Mark(m_dbfno);
		m_recptr=(LPBYTE)dbf_RecPtr(m_dbfno); //should't change (at rec buf in dbf hdr)
		m_rec=m_numrecs=dbf_NumRecs(m_dbfno);
		return true;
	}

	if(!m_bRW || !m_ptr) return false;
	if(m_maxrecs==m_numrecs) {
	   UnMapFile();
	   m_maxrecs=m_numrecs+m_increcs;
	   if(!Map()) return false;
	}
	DWORD *pdw=&((DBF_HDR *)m_ptr)->NumRecs; //ptr to numrecs variable in mappped version of header
	ASSERT(m_numrecs==*pdw);
	*pdw=++m_numrecs;
	ASSERT(m_dbfp->H.NumRecs==m_numrecs-1);
	m_dbfp->H.NumRecs++;
	m_dbfp->FileChg=TRUE;
	m_recptr=RecPtr(m_rec=m_numrecs); //set at appended record
	memset(m_recptr,' ',m_sizrec);
	return true;
}

int CShpDBF::MapFile(BOOL bUseMap)
{
	//initrecs>0 when creating for export
	VERIFY(m_handle=CDBFile::Handle());
	ASSERT(!IsMapped());

	m_dbfp=_dbf_GetDbfP(m_dbfno);
	m_sizhdr=CDBFile::SizHdr();
	m_bRW=(CDBFile::OpenFlags()&ReadOnly)==0;
	m_sizrec=CDBFile::SizRec();
	m_numrecs=CDBFile::NumRecs();
	m_numflds=CDBFile::NumFlds();
	VERIFY(m_pFld=CDBFile::FldDef(1)); //Allocate field definitions

	m_tableptr=NULL;
	m_rec=0;
	m_recptr=NULL;
	m_maxrecs=0; //initially map whole file with no extra slots

	if(!bUseMap) return 0; //create

	ULONGLONG filesize = m_sizhdr+(ULONGLONG)m_sizrec*m_numrecs;

	if(filesize>ULONG_MAX || m_numrecs && dbf_Go(m_dbfno,m_numrecs)) {
		return ErrFormat;
	}

	if(filesize <= MAX_MAPVIEW) {
		if(Map()) {
			m_maxrecs=m_numrecs; //remap with extra slots on first append
			m_increcs=NUM_MAPINCRECS; //by default, extend length by this many records on each remap
		}
	}

	if(!IsMapped() && m_sizrec <= SIZ_CACHEBUF) {
		//speeds table sorting - 1/3 the time
		if(!dbf_defCache && csh_Alloc(&dbf_defCache, SIZ_CACHEBUF, NUM_CACHEBUF)) return ErrMem; 
		return dbf_AttachCache(m_dbfno,dbf_defCache); //last file will delete cache upon close
	}

	return 0;
}

int CShpDBF::Close()
{
	if(!m_dbfno) return ErrArg;
	if(IsMapped())
	{
		UnMapFile();
	}
	return CDBFile::Close();
}

int CShpDBF::Open(const char* pszFileName,UINT mode)
{
	TRX_CSH cshSave=dbf_defCache;
	dbf_defCache=0;
	//dont assign cache, except possibly in MapFile() if file too big
	int iRet=CDBFile::Open(pszFileName,mode);
	dbf_defCache=cshSave;

	if(!iRet && (iRet=MapFile(TRUE))) { //create map view if file small enough
		Close();
	}
	return iRet;
}

int CShpDBF::Create(const char* pszFileName,UINT numflds,DBF_FLDDEF *pFld)
{
	TRX_CSH cshSave=dbf_defCache;
	dbf_defCache=NULL;
	//don't attach the cache used for unmapped large files
	int iRet=CDBFile::Create(pszFileName,numflds,pFld);
	dbf_defCache=cshSave;

	if(!iRet) iRet=MapFile(FALSE); //don't create actual map view -- no random access of created files in this program
	return iRet;
}

int CShpDBF::StoreTrimmedFldData(LPBYTE recPtr,LPCSTR text,int nFld)
{
	int iRet=0;
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
								iRet=1;
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
					return 1;
	}
	if(len>fldLen) {
		len=fldLen;
		iRet=1;
	}
	if(len) memcpy(pFld,fldData,len);
	if(len<fldLen) memset(pFld+len,' ',fldLen-len);
	return iRet;
}
