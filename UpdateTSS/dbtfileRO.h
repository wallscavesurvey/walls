#ifndef _DBTFILE_H
#define _DBTFILE_H

#define LEN_MEMO_HDR 50

class CDBTData
{
public:
	CDBTData() : pData(NULL),lenData(0),nUsers(0) {}
	~CDBTData() {
		if(lenData) {
			ASSERT(0);
			free(pData);
			lenData=0;
		}
	}
	BOOL AllocMin(UINT len)
	{
		if(len>lenData) {
			LPSTR pNewData=(LPSTR)realloc(pData,len);
			if(!pNewData) return FALSE;
			pData=pNewData;
			lenData=len;
		}
		return TRUE;
	}

	static bool IsTextRTF(LPCSTR pText)
	{
		return *pText=='{' && pText[1]=='\\' && pText[2]=='r' && pText[3]=='t' && pText[4]=='f';
	}

	static bool IsTextIMG(LPCSTR pText)
	{
		if(IsTextRTF(pText)) return false;
		LPCSTR p=strstr(pText,"[~]");
		return p && (p-pText)<=(255-3);
	}


	LPSTR pData;
	UINT lenData;
	UINT nUsers;
};

struct DBT_FREEREC {
	DBT_FREEREC() : recNo(0),recCnt(0) {}
	DBT_FREEREC(UINT recno,UINT reccnt) : recNo(recno),recCnt(reccnt) {}
	UINT recNo;	 //starting block
	UINT recCnt; //# of contiguous blocks
};

int dbt_GetMemoHdr(LPSTR dst,LPCSTR src);

class CDBTFile : public CFile {
public:
	enum {F_INITFREE=1,F_EXTENDED=2,F_TRUNCATED=4,F_WRITTEN=8};
	CDBTFile() : m_uNextRec(0),m_uFlags(0) {}

	bool IsOpen() {return m_hFile!=CFile::hFileNull;}
	BOOL InitFree();
	static int RecNo(LPCSTR p10);
	static int StripRTF(LPSTR pText,BOOL bKeepPrefix=FALSE);
	LPSTR GetText(UINT *pLen,UINT rec);
	LPSTR GetTextSample(LPSTR buf,UINT bufSiz,UINT recNo);
	virtual BOOL Open(LPSTR pathName,UINT flags,CFileException *pEx=NULL);
	virtual void Close();

	static CDBTData dbt_data;
	DWORD m_uNextRec; //will be >1 after any appends
	UINT m_uFlags;
};
#endif
