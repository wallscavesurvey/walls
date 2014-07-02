#pragma once
#include "afx.h"

class CFileMap : public CFile
{
public:

#ifdef _USE_POLYMAP
	//requires too much ram for sequential access --
	CFileMap(void) : m_hFM(NULL), m_ptr(NULL)
	{
	}

	~CFileMap(void)
	{
		if(m_ptr) {
			UnmapFile();
		}
	}

	virtual void Close()
	{
		if(m_ptr) {
			UnmapFile();
		}
		CFile::Close();
	}

	bool IsOpen() const {return m_hFile!=CFile::hFileNull;} 

	bool MapFile()
	{
		m_hFM=CreateFileMapping(
			m_hFile,		//handle
			NULL,			//LPSECURITY_ATTRIBUTES
			PAGE_READONLY,	//flProtect
			0,0,			//dwMaximumSizeHigh,dwMaximumSizeLow (leave zero for current file size)
			NULL			//lpName (used for sharing only)
		);
		if(!m_hFM) return false;

		m_ptr=(LPBYTE)MapViewOfFile(
			m_hFM,
			FILE_MAP_READ,  //dwDesiredAccess
			0,0,            //dwFileOffsetHigh, dwFileOffsetLow (start map at file beginning)
			0				//dwNumberOfBytesToMap (0 for entire file)
		);

		if(!m_ptr) {
			CloseHandle(m_hFM);
			m_hFM=NULL;
			return false;
		}
		return true;
	}

	void UnmapFile()
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

	LPBYTE m_ptr;

private:

	HANDLE m_hFM;
#else

CFileMap(void) {}
bool IsOpen() const {return m_hFile!=CFile::hFileNull;} 

#endif

};
