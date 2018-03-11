//FILEBUF.H -- Class CFileBuffer
//
//Class for reading or writing a file, using buffered I/O
//
#ifndef __FILEBUF_H
#define __FILEBUF_H

//Define if using Seek() or GetPosition() --
#undef __FILEBUF_SEEK

class CFileBuffer : public CFile
{
public:
	UINT m_nBufferSize;          //Check for successful construction
	
protected:
	BYTE *m_pBuffer;
	BYTE *m_pReadLimit;
	BYTE *m_pWritePtr;
	BYTE *m_pReadPtr;
	
	UINT ReadCopy(LPBYTE &pData,UINT nLen)
	{
		memcpy(pData,m_pReadPtr,nLen);
		m_pReadPtr+=nLen;
		pData+=nLen;
		return nLen;
	}
	
	UINT WriteCopy(LPBYTE &pData,UINT nLen)
	{
		memcpy(m_pWritePtr,pData,nLen);
		m_pWritePtr+=nLen;
		pData+=nLen;
		return nLen;
	}
	
	void FlushBuffer();
	
public:
	CFileBuffer(UINT bufsize);
	virtual ~CFileBuffer();
	
#ifdef __FILEBUF_SEEK
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual DWORD GetPosition() const;
#endif

	BOOL IsOpen()
	{
		return (m_hFile != CFile::hFileNull);
	}

	BOOL OpenFail(const char *pszPathName,UINT nOpenFlags);
	int ReadLine(LPSTR pBuf,int sizBuf);
	int BufferSize() {return m_nBufferSize;}
	
	void CDECL WriteArgv(const char *format,...);
	void CDECL WriteArgv(UINT idFormat,...);
	   
	virtual void Flush();
	virtual UINT Read(void FAR* lpBuf, UINT nCount);
	virtual void Write(const void FAR* lpBuf, UINT nCount);
	void Write(LPCSTR s);
	void WriteLn() {Write("\r\n");}

	virtual void Close();
	virtual void Abort();
	
	static void ReportException(const char* pszPathName, CFileException* e,
		BOOL bSaving, UINT nIDPDefault);
};
#endif
