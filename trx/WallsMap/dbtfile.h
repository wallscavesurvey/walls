#ifndef _DBTFILE_H
#define _DBTFILE_H

#define LEN_MEMO_HDR 50

#ifdef _DEBUG
	#undef _USE_REPLACE
	#ifdef _USE_REPLACE
		extern UINT _bRepCount,_bRepRecs;
		extern bool _bReplace;
	#endif
#endif

struct EDITED_MEMO
{
	EDITED_MEMO() : uRec(0),fld(0),bChg(0),recNo(0),recCnt(0),pData(NULL) {}
	//EDITED_MEMO(const EDITED_MEMO &memo) : uRec(memo.uRec),fld(memo.fld),recNo(memo.recNo),recCnt(memo.recCnt),pData(memo.pData) {}
	EDITED_MEMO(const EDITED_MEMO &memo)
	{
		*this=memo;
	}

	static UINT GetRecCnt(UINT uDataLen)
	{
		//count of contiguous record in DBT
		//required for a length of data -- allows for single 0x1a termination
		return uDataLen?((uDataLen+512)/512):0;
	}

	UINT uRec; //dbf record number
	WORD fld;
	WORD bChg;
	UINT recNo; //record no. in DBT, or 0 if new 
	UINT recCnt; //original record length in 512 blocks
	LPSTR pData; //actual data length can change
};

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

	void Free()
	{
		free(pData);
		pData=NULL;
		lenData=0;
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

	static LPCSTR NextImage(LPCSTR p)
	{
		return strstr(p,"[~]");
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

typedef std::vector<DBT_FREEREC> VEC_FREE;
typedef VEC_FREE::iterator it_free;

LPCSTR dbt_GetMemoHdr(LPCSTR src,UINT maxHdrLen);

class CDBTFile : public CFile {
public:
	enum {F_INITFREE=1,F_EXTENDED=2,F_TRUNCATED=4,F_WRITTEN=8};
	CDBTFile() : m_uNextRec(0),m_uFlags(0) {}

	BOOL Create(LPCSTR pathName);
	UINT AppendCopy(CDBTFile &dbt,UINT recNo);

	bool IsFlushed() {return !(m_uFlags&(F_EXTENDED|F_TRUNCATED|F_WRITTEN));}
	void AppendFree();
	BOOL FlushHdr();
	bool IsOpen() {return m_hFile!=CFile::hFileNull;}
	BOOL InitFree(BOOL bTesting=0);
	void AddToFree(const DBT_FREEREC &frec);
	BOOL CheckFree();
	static int RecNo(LPCSTR p10);
	static void SetRecNo(LPSTR p10,UINT iVal);
	static int StripRTF(LPSTR pText,BOOL bKeepPrefix=FALSE);
	UINT PutText(EDITED_MEMO &memo);
	UINT PutTextField(LPCSTR text,UINT len);
	LPSTR GetText(UINT *pLen,UINT rec);
	LPSTR GetTextSample(LPSTR buf,UINT bufSiz,UINT recNo);
	int GetToolTip(LPVOID pvText80,UINT siztip,BOOL bWide,UINT recNo);
	virtual BOOL Open(LPSTR pathName,UINT flags,CFileException *pEx=NULL);
	virtual void Close();
	virtual void Abort();
	void CloseDel()
	{
		if(IsOpen()) {
			Abort();
			_unlink(GetFilePath());
		}
	}

	static CDBTData dbt_data;
	DWORD m_uNextRec; //will be >1 after any appends
	UINT m_uFlags;
	VEC_FREE m_vFree;
};
#endif
