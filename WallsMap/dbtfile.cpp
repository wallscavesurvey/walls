#include "stdafx.h"
#include "dbtfile.h"
#include <ShpDBF.h>
#include "shplayer.h"

#ifdef _USE_REPLACE
	bool _bReplace;
	UINT _bRepCount,_bRepRecs;
#endif

CDBTData CDBTFile::dbt_data;

BOOL CDBTFile::Open(LPSTR pathName,UINT flags,CFileException *pEx /*=NULL*/)
{
	strcpy(trx_Stpext(pathName),SHP_EXT_DBT);
	CFileException ex;
	if(!CFile::Open(pathName,flags,&ex)) {
		LPCSTR pCause=(ex.m_cause==CFileException::fileNotFound)?"missing": "not accessible";
		CMsgBox("File: %s\nOpen failure: The shapefile has memo fields but the .DBT component is %s.",
			trx_Stpnam(pathName),pCause);
		return FALSE;
	}
	dbt_data.nUsers++;
	m_uFlags=0;
	return TRUE;
}

void CDBTFile::Abort()
{
	if(IsOpen()) {
		ASSERT(dbt_data.nUsers);
		//Free static data structure used with all DBT files for both
		//reading and writing if it's no longer needed --
		if(!--dbt_data.nUsers && dbt_data.lenData)
			dbt_data.Free();
		CFile::Abort(); //should already be flushed
	}
}

void CDBTFile::Close()
{
	if(IsOpen()) {
		ASSERT(dbt_data.nUsers);
		//Free static data structure used with all DBT files for both
		//reading and writing if it's no longer needed --
		if(!--dbt_data.nUsers && dbt_data.lenData)
			dbt_data.Free();
		CFile::Close(); //should already be flushed
	}
}

BOOL CDBTFile::FlushHdr()
{
	BOOL bRet=TRUE;
	//Always called afer a record with edited memo fields is saved,
	//Flushes file alone if m_uFlags==F_WRITTEN --
	if(m_uFlags&(F_EXTENDED|F_TRUNCATED|F_WRITTEN)) {
		ASSERT(IsOpen());
		try {
			ASSERT(m_uNextRec || !(m_uFlags&(F_EXTENDED|F_TRUNCATED)));
			if(m_uFlags&(F_EXTENDED|F_TRUNCATED)) {
				SeekToBegin();
				Write(&m_uNextRec,sizeof(UINT));
			}
			if(m_uFlags&F_TRUNCATED) {
				SetLength(m_uNextRec*512);
			}
			Flush();
		}
		catch(...) {
			CMsgBox("File %s: Update failure at system level!",
				(LPCSTR)CFile::GetFileName());
			bRet=FALSE;
		}
		m_uFlags&=~(F_EXTENDED|F_TRUNCATED|F_WRITTEN);
	}
	return bRet;
}

void CDBTFile::AppendFree()
{
	//Called only when editable shapefile is closed or explicitly saved --
	ASSERT(!(m_uFlags&(F_EXTENDED|F_TRUNCATED|F_WRITTEN)));
	if(!(m_uFlags&F_INITFREE)) return;
	m_uFlags=0;

	if(!m_uNextRec || !IsOpen()) return;

	ASSERT(CFile::GetLength()==m_uNextRec*512UL); //*** triggered after copy to layer /w no source memos, but memos then edited
	UINT nRecs=m_vFree.size();
	if(nRecs) {
		UINT nFreeBlks=((nRecs+1)*sizeof(DBT_FREEREC)+511)/512;
		DBT_FREEREC freeRec(0xFEFEFEFE,nRecs);

		try {
			if(!dbt_data.AllocMin(nFreeBlks*512))
				throw 0;
			memset(dbt_data.pData,0,nFreeBlks*512);
			memcpy(dbt_data.pData,&m_vFree[0],nRecs*sizeof(DBT_FREEREC));
			memcpy(dbt_data.pData+nFreeBlks*512-sizeof(DBT_FREEREC),&freeRec,sizeof(DBT_FREEREC));
			Seek(m_uNextRec*512,CFile::begin);
			Write(dbt_data.pData,nFreeBlks*512);
			m_uNextRec+=nFreeBlks;
			SeekToBegin();
			Write(&m_uNextRec,sizeof(DWORD));
			Flush();
		}
		catch(...) {
			CMsgBox("File %s: System-level failure writing final version of memo file!",
				(LPCSTR)CFile::GetFileName());
		}
		std::vector<DBT_FREEREC>().swap(m_vFree);
	}
}
static bool free_comp(const DBT_FREEREC &frec1,const DBT_FREEREC &frec2)
{
	return frec1.recNo<frec2.recNo;
}

void CDBTFile::AddToFree(const DBT_FREEREC &frec)
{
	ASSERT(frec.recNo && frec.recCnt);

	if(!m_vFree.size()) {
		if(frec.recNo+frec.recCnt==m_uNextRec) {
			//block at end of file --
			m_uNextRec=frec.recNo;
			m_uFlags|=F_TRUNCATED;
		}
		else {
			m_vFree.push_back(frec);
		}
		return;
	}

	it_free itb=m_vFree.begin();
	it_free ite=m_vFree.end();
	it_free it=std::lower_bound(itb,ite,frec,free_comp);
	//if it!=ite, it==position of first block with it->recNo >= frec.recNo (won't be equal)
	ASSERT(it==ite || (it->recNo>frec.recNo && (it==itb || (it-1)->recNo<frec.recNo)));

	if(it>itb) itb=it-1; //will insert at or immediately after itb
	if(it!=itb && itb->recNo+itb->recCnt==frec.recNo) {
		//no insertion -- add records to itb==(it-1)
		itb->recCnt+=frec.recCnt;
		//if contiguous, also add next block's records and errase it --
		if(it!=ite && frec.recNo+frec.recCnt==it->recNo) {
			itb->recCnt+=it->recCnt;
			m_vFree.erase(it);
		}
		it=itb; //new position of this free block
	}
	else if(it!=ite && frec.recNo+frec.recCnt==it->recNo) {
		//the next block is contiguous, so simply increase its size --
		it->recNo=frec.recNo;
		it->recCnt+=frec.recCnt;
	}
	else {
		it=m_vFree.insert(it,frec);
		//it == position of newly-inserted block
	}

	//iterator it now points to the free block that we've either allocated
	//or enlarged. If it represents the last portion of the file as it
	//would currently be written, remove block from list and flag file for
	//resizing in FlushHdr() --

	if(it->recNo+it->recCnt==m_uNextRec) {
		ASSERT(it+1==m_vFree.end());
		m_uNextRec=it->recNo;
		m_vFree.erase(it);
		m_uFlags|=F_TRUNCATED;
	}

#ifdef _DEBUG
	if(!CheckFree()) {
		ASSERT(0);
		UINT rec;
		for(it_free it=m_vFree.begin();it!=m_vFree.end();it++) {
			rec=it->recNo;
			rec=it->recCnt;
		}
	}
#endif
}

UINT CDBTFile::PutText(EDITED_MEMO &memo)
{
	if(!IsOpen()) {
		ASSERT(0);
		return 0;
	}

	UINT newLen=memo.pData?strlen(memo.pData):0;
	UINT recNo=memo.recNo; //original record number

	try {
		if(!newLen) {
			if(recNo) {
				if(InitFree())
					AddToFree(DBT_FREEREC(recNo,memo.recCnt));
			}
			return 0;
		}

		UINT newCnt=(newLen+513)/512; //records required

		if(!dbt_data.AllocMin(newCnt*512)) return 0;

		memcpy(dbt_data.pData,memo.pData,newLen);
		dbt_data.pData[newLen++]=0x1A;
		dbt_data.pData[newLen++]=0x1A;
		memset(dbt_data.pData+newLen,0,newCnt*512-newLen);

		bool bUsingFree=!recNo || newCnt != memo.recCnt;

		if(bUsingFree) {

			//New record position or new blocksize desired -- access the free record list
			if(!InitFree()) return 0; //if bad dbt file

			if(recNo) {
				//if old block large enough, keep recNo and add only unused portion to free list --
				UINT inc=(newCnt<memo.recCnt)?newCnt:0;
				AddToFree(DBT_FREEREC(recNo+inc,memo.recCnt-inc));
				if(!inc) recNo=0;
			}

			if(!recNo && m_vFree.size()) {
				//Look for first smallest free block that's large enough --
				it_free it0=m_vFree.begin();
				if(newCnt!=it0->recCnt) {
					for(it_free it=it0+1;it!=m_vFree.end();it++) {
						if(newCnt==it->recCnt) {
							it0=it;
							break;
						}
						if(newCnt<=it->recCnt && it->recCnt<it0->recCnt) //was <=it0->recCnt
							it0=it;
					}
				}
				//We will use record at it0 if it's large enough --
				if(it0->recCnt>=newCnt) {
					recNo=it0->recNo;
					if(it0->recCnt==newCnt) {
						m_vFree.erase(it0);
					}
					else {
						it0->recCnt-=newCnt;
						it0->recNo+=newCnt;
					}
				}
			}
			if(!recNo)
				recNo=m_uNextRec;
		}

		Seek(recNo*512,CFile::begin);
		m_uFlags|=F_WRITTEN;
		Write(dbt_data.pData,newCnt*512);
		if(bUsingFree && recNo==m_uNextRec) {
			m_uFlags|=F_EXTENDED;
			m_uNextRec+=newCnt;
		}
	}
	catch(...) {
		return 0;
	}

	return recNo;
}

int CDBTFile::RecNo(LPCSTR p10)
{
	if(!p10) return 0;
	char buf[12];
	int n=10;
	for(;n && (*p10==' ' || *p10=='0');n--,p10++);
	if(n) {
		memcpy(buf,p10,n);
		buf[n]=0;
		n=atoi(buf);
	}
	return n;
}

void CDBTFile::SetRecNo(LPSTR p10,UINT iVal)
{
	if(iVal) {
		char buf[12];
		sprintf(buf,"%010u",iVal);
		memcpy(p10,buf,10);
	}
	else memset(p10,' ',10);
}


LPCSTR dbt_GetMemoHdr(LPCSTR src,UINT maxHdrLen)
{
	//Return with "* " or "~ " in first 2 chars of dbt_memoHdr

	static char dbt_memoHdr[514];
	LPSTR p,pDst=dbt_memoHdr;

	*pDst++='*';
	*pDst++=' ';

	UINT len=strlen(src);
	if(len>511) len=511;

	memcpy(pDst,src,len);
	pDst[len]=0;

	bool bFormatRTF=CDBTData::IsTextRTF(pDst);

	if(bFormatRTF) {
		len=CDBTFile::StripRTF(pDst,TRUE); //Keep prefix
		p=pDst+len;
	}
	else if(p=strstr(pDst,"[~]")) {
		*dbt_memoHdr='~';
		*p=0;
		while(p>pDst && xisspace(p[-1])) p--;
		while(p>pDst && !isgraph(p[-1])) p--;
		len=p-pDst;
	}

	bool bTruncated=len>maxHdrLen;

	if(!(p=strchr(pDst,'\r')) && !(p=strchr(pDst,'\n')))
		p=pDst+strlen(pDst);
	while(p>pDst && xisspace(p[-1])) p--;
	*p=0;

	if(bTruncated) {
		len=p-pDst;
		if(len>maxHdrLen) {
			len=bFormatRTF?(maxHdrLen-4):(maxHdrLen-3);
			p=pDst+len;
			//while(p>pDst && isspace(p[-1])) p--;
		}
		strcpy(p,"...");
	}
	for(p=pDst;*p;p++) if(*p=='\t') *p=' ';

	if(bFormatRTF) {
		if(p==pDst) {*p++='{'; *p++=' ';}
		*p++='}';
		*p=0;
	}
	return dbt_memoHdr;
}

/*
int CDBTFile::GetToolTip(LPVOID pvText80,UINT siztip,BOOL bWide,UINT recNo)
{
	char src[256];

	if(!*GetTextSample(src,256,recNo))
		return 0;

	if(CDBTData::IsTextRTF(src))
		CDBTFile::StripRTF(src,FALSE); //Discard prefix

	LPSTR p;

	if(!(p=strchr(src,'\r')) && !(p=strchr(src,'\n')))
		p=src+strlen(src);

	if((UINT)(p-src)>=siztip) {
		p=strcpy(src+(siztip-4),"...");
	}
	else {
		while(p>src && xisspace(p[-1])) p--;
		while(p>src && !isgraph(p[-1])) p--;
		*p=0;
	}
	for(p=src;*p;p++) if(*p=='\t') *p=' ';
	int len=p-src; //chars in string
	if(len) {
		ASSERT(len<80);
		if(bWide) {
			_mbstowcsz((WCHAR *)pvText80,src,len+1);
			ASSERT((((WCHAR *)pvText80))[len]==0);
		}
		else
			memcpy((LPSTR)pvText80,src,len+1);
	}
	return len;
}
*/
#ifdef _USE_REPLACE
void BufReplace(LPSTR buf,LPSTR pOld,LPSTR pNew)
{
	CString s;
	LPSTR p=(LPSTR)memchr(buf,0x1A,512);
	if(!p) return;
	s.SetString(buf,p-buf);
	if(s.GetLength()>=(int)strlen(pOld)) {
		_bRepRecs++;
		_bRepCount+=s.Replace(pOld,pNew);
		memcpy(buf,(LPCSTR)s,s.GetLength());
	}
}
#endif

UINT CDBTFile::AppendCopy(CDBTFile &dbt,UINT recNo)
{
	BYTE buf[512];
#ifdef _USE_REPLACE
	bool bRep=false;
#endif
	try {
		if(!InitFree()) throw 0;
		Seek(m_uNextRec*512,CFile::begin);
		dbt.Seek(512*recNo,CFile::begin);
		recNo=m_uNextRec;
		do {
			if(dbt.Read(buf,512)!=512)
				throw 0;
#ifdef _USE_REPLACE
			if(!bRep && _bReplace) {
				bRep=true;
				BufReplace((LPSTR)buf,".tif\r",".png\r");
			}
#endif
			Write(buf,512);
			m_uNextRec++;
		}
		while(!memchr(buf,0x1A,512));
		m_uFlags|=F_EXTENDED;
		return recNo;
	}
	catch(...) {
	}
	return (UINT)-1;
}

UINT CDBTFile::PutTextField(LPCSTR text,UINT len)
{
	//write a single-record memo --
	BYTE buf[512];
	ASSERT(len<=510);
	if(len>510) len=510;
	while(len && xisspace((BYTE)text[len-1])) len--;
	if(!len) return 0;
	memcpy(buf,text,len);
	buf[len++]=0x1A;
	buf[len++]=0x1A;
	memset(buf+len,0,512-len);
	try {
		Seek(m_uNextRec*512,CFile::begin);
		Write(buf,512);
		m_uFlags|=F_EXTENDED;
		return m_uNextRec++;
	}
	catch(...) {
	}
	return (UINT)-1;
}

LPSTR CDBTFile::GetTextSample(LPSTR buf,UINT bufSiz,UINT recNo)
{
	ASSERT(bufSiz>0 && bufSiz<=512);
	bufSiz--;
	*buf=0;
	if(!recNo || !IsOpen()) {
		return buf;
	}
	try {
		Seek(recNo*512,CFile::begin);
		if(Read(buf,bufSiz)!=bufSiz) throw 0;
		buf[bufSiz]=0;
		LPSTR p=strchr(buf,0x1A);
		if(p) *p=0;
	}
	catch(...) {
		ASSERT(0);
	}
	return buf;
}

LPSTR CDBTFile::GetText(UINT *pLen,UINT recNo)
{
	//Return ptr to 0-terminated string of maximum length *pLen (excluding null).
	//If *pLen==0, return complete record in dbt_data.pData and set *pLen to actual length (excluding null).

	LPSTR pRet="";

	UINT maxLen=*pLen;
	*pLen=0;

	if(!recNo || !IsOpen())
		return pRet;

	try {

		if(!dbt_data.AllocMin(512)) return pRet;

		Seek(recNo*512,CFile::begin);

		UINT len=512;
		while(true) {
			LPSTR pData=dbt_data.pData+(len-512);
			if(Read(pData,512)!=512) {
				ASSERT(0);
				break;
			}
			LPSTR pEnd=(LPSTR)memchr(pData,0x1A,512);
			if(pEnd) {
				*pLen=pEnd-dbt_data.pData;
			}
			if(maxLen && len>=maxLen+1 && (!pEnd || *pLen>maxLen)) {
				*pLen=maxLen;
				pEnd=dbt_data.pData+maxLen;
			}
			if(pEnd) {
				*pEnd=0;
				return dbt_data.pData;
			}
			len+=512;
			if(!dbt_data.AllocMin(len)) {
				ASSERT(0);
				break;
			}
		}
	}
	catch (...) {
	}
	return pRet;
}

BOOL CDBTFile::CheckFree()
{
		//check if values are reasonable -- if not, clear the list --
		if(!m_vFree.size()) return TRUE;

		it_free it0=m_vFree.begin();
		for(it_free it=it0+1;it!=m_vFree.end();it++,it0++) {
			if(!it0->recNo || !it0->recCnt || it0->recNo+it0->recCnt>=it->recNo) {
				return FALSE;
			}
		}
		return (it0->recNo && it0->recCnt && it0->recNo+it0->recCnt<m_uNextRec);
}

BOOL CDBTFile::InitFree()
{
	if(m_uFlags&F_INITFREE)
		return m_uNextRec>0;

	ASSERT(!m_vFree.size() && !(m_uFlags&(F_EXTENDED|F_TRUNCATED)));

	m_uFlags|=F_INITFREE;
	m_uNextRec=0;

	try {

		SeekToBegin();

		if(Read(&m_uNextRec,sizeof(DWORD))<sizeof(DWORD) || !m_uNextRec || m_uNextRec*512>GetLength()) {
			throw 0;
		}
		//Let's see if a free record list exists --
		if(m_uNextRec==1) {
			return TRUE; //file has neither a free list or data records
		}
		DBT_FREEREC freeRec;
		Seek(m_uNextRec*512-sizeof(DBT_FREEREC),CFile::begin);
		Read(&freeRec,sizeof(DBT_FREEREC));

		UINT nFreeRecBlks=0;
		if(freeRec.recNo!=0xFEFEFEFE || !freeRec.recCnt ||
			(nFreeRecBlks=((freeRec.recCnt+1)*sizeof(DBT_FREEREC)+511)/512)>m_uNextRec-1) {
				ASSERT(!nFreeRecBlks);
				return TRUE;
		}
		Seek((m_uNextRec-nFreeRecBlks)*512,CFile::begin);
		m_vFree.assign(freeRec.recCnt,DBT_FREEREC());

		if(CFile::Read(&m_vFree[0],freeRec.recCnt*sizeof(DBT_FREEREC))!=freeRec.recCnt*sizeof(DBT_FREEREC) ||
				!CheckFree())
			throw 0;

		m_uNextRec-=nFreeRecBlks;
		SeekToBegin();
		Write(&m_uNextRec,sizeof(UINT));
		SetLength(m_uNextRec*512);
	}
	catch(CFileException e) {
		//Bad DBT format --
		CMsgBox("CAUTION: File %s doesn't have the correct format. Some memo field updates "
			"will not actually be saved! Repair format by exporting shapefile.",(LPCSTR)GetFileName());
		if(m_vFree.size())
			std::vector<DBT_FREEREC>().swap(m_vFree);
		m_uNextRec=0;
	}

	return m_uNextRec>0;
}

BOOL CDBTFile::Create(LPCSTR pathName)
{
	if(!CFile::Open(pathName,CFile::modeCreate|CFile::modeReadWrite|CFile::shareExclusive))
		return FALSE;

	BYTE dbt_hdr[512];
	memset(dbt_hdr,0,512);

	*(UINT *)dbt_hdr=1;
	dbt_hdr[16]=0x03;
	try {
		CFile::SeekToBegin();
		CFile::Write(dbt_hdr,512);
	}
	catch(...) {
		CFile::Abort();
		return FALSE;
	}
	m_uNextRec=1;
	dbt_data.nUsers++;
	m_uFlags=F_INITFREE;
	return TRUE;
}

//Strip out rtf codes
int CDBTFile::StripRTF(LPSTR pText,BOOL bKeepPrefix/*=FALSE*/)
{
	bool bBrace = false;
	bool bSlash = false;
	bool bFirstLetter = false;
	LPSTR p,pNext=pText;

	for (p=pText;*p; p++)
	{
		char ch = *p;
		if (ch == '\\')
		{
			//if(!bKeepPrefix) {
				if(p[1]=='t' && p[2]=='a' && p[3]=='b') {
					//*pNext++=' ';
					//*pNext++=' ';
					*pNext++=' ';
					p+=3;
				}
			//}
			bSlash = true;
			continue;
		}
		if (ch == '{')
		{
			bBrace = true;
			bSlash = false;
			continue;
		}
		if (ch == '}')
		{
			bSlash = bBrace = false;
			continue;
		}
		if(xisspace(ch))
		{
			bSlash = false;
			//Let it fall through so the space is added
			//if we have found first letter
			if (!bFirstLetter) continue;
		}

		if (!bSlash && !bBrace)
		{
			if (!bFirstLetter)	{
				if(bKeepPrefix) *pNext++='{';
				bFirstLetter = true;
			}
			*pNext++ = ch;
		}
	}
	*pNext=0;

	//Always remove extra spaces --
	pNext=pText;
	bSlash=false;
	for(p=pText;*p;p++) {
		if(*p==' ') {
			if(bSlash) continue;
			bSlash=true;
		}
		else bSlash=false;
		*pNext++=*p;
	}
	*pNext=0;

	return pNext-pText;
}


				

			

