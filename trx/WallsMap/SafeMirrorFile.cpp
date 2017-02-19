//SafeMirrorFile.cpp --

#include "stdafx.h"
//#include <afximpl.h>

#include "SafeMirrorFile.h"

/*
	Exception opening an MFC document off a CD-Rom in VS2003
	Noel Borthwick, 2005-06-08, 4:02 am

	This bug in VS.NET 2003 has been reported a few times before in earlier
	threads. I'm posting a workaround to this problem that I implemented
	that someone might find useful.
	An earlier thread on this topic can be found via google here:
	http://groups-beta.google.com/group...1029aa3d67a0e4d
	The exception is thrown by some code in CFile::GetStatus that does
	CTime conversions without checking for time validity. This bug fixed in
	VS2005 but if you need your VS2003 application to work this is the way
	to fix this problem now:

	<Code for CSafeMirrorFile -- see below>
*/

/*
	CMirrorFile
	Structure (AfxPriv.h - DocCore.cpp)

	The class CMirrorFile is derived from CFile. It overrides the base class implementation of Open(), Abort()
	and Close(). No functions are added.

	Imagine that you have created a new file on disk, but the name is already being used by another file. The
	older file will be overwritten by the new one. There is no way to abort this action and reconstruct the
	original file. If writing to the new file fails for some reason you might end up losing both the original
	and the new file, which can often be a disaster.

	The CMirrorFile provides a safe file save support. When modeCreate is specified in Open() and enough free
	disk space is available, you will work on a temporary mirror file (with another name). When you later
	Close() the file, the original file will be deleted and the mirrored file will be renamed to the original
	file name. When Abort() is called, the mirrored file will be deleted and the original files will still
	safely exist on disk.
	
	Interface:
	The class CMirrorFile only overrides functions of its base class and does not add new functions.
	These are:

	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags,CFileException* pError = NULL);
	virtual void Abort();
	virtual void Close();

	Remarks:
	The class CDocument has two overridable functions called:

	virtual CFile* GetFile(LPCTSTR lpszFileName, UINT nOpenFlags,CFileException* pError);
	virtual void ReleaseFile(CFile* pFile, BOOL bAbort);

	When a new file should be opened, the first function is called. It supplies the file name and the open
	flags and has the possibility to return an exception. These are the same parameters as those for
	CFile::Open(). The function should create a new CFile derived object, open it and return a pointer to it.
	When any of these operations fails NULL should be returned. The actual class that is used can be chosen by
	the implementation of GetFile(). The function ReleaseFile() should close the file object and delete it.
	The parameter bAbort specifies whether the file should to be released using either Close() or Abort().

	The default implementation of CDocument::GetFile() creates a CMirrorFile and opens it. ReleaseFile()
	closes or aborts the file and deletes it.
*/

/////////////////////////////////////////////////////////////////////////////
// CFile override to call our custom SafeGetStatus and prevent an exception
// being thrown when the file resides on a CD device

CString CSafeMirrorFile::GetFilePath() const
{
	ASSERT_VALID(this);

	CFileStatus status;
	SafeGetStatus(status);
	return status.m_szFullName;
}

/////////////////////////////////////////////////////////////////////////////
// custom version of CFile::GetStatus with fix from VC.NET 2005
// SafeGetStatus only invokes CTime conversions when the time read from the file is valid.

BOOL CSafeMirrorFile::SafeGetStatus(CFileStatus& rStatus) const
{
	ASSERT_VALID(this);

	memset(&rStatus, 0, sizeof(CFileStatus));

	// copy file name from cached m_strFileName
	lstrcpyn(rStatus.m_szFullName, m_strFileName,_countof(rStatus.m_szFullName));

	if (m_hFile != hFileNull)
	{
		// get time current file size
		FILETIME ftCreate, ftAccess, ftModify;
		if (!::GetFileTime(m_hFile, &ftCreate, &ftAccess, &ftModify))
			return FALSE;

		if ((rStatus.m_size = ::GetFileSize(m_hFile, NULL)) == (DWORD)-1L)
			return FALSE;

		if (m_strFileName.IsEmpty())
			rStatus.m_attribute = 0;
		else
		{
			DWORD dwAttribute = ::GetFileAttributes(m_strFileName);

			// don't return an error for this because previous versions of MFC didn't
			if (dwAttribute == 0xFFFFFFFF)
				rStatus.m_attribute = 0;
			else
			{
				rStatus.m_attribute = (BYTE) dwAttribute;
				#ifdef _DEBUG
					// MFC BUG: m_attribute is only a BYTE wide
					if (dwAttribute & ~0xFF)
					TRACE("Warning: CFile::GetStatus() returns m_attribute without high-order flags.\n");
				#endif
			}
		}

		// convert times as appropriate
		// some file systems may not record file creation time, file access	time etc
		if ( IsValidFILETIME(ftCreate) )
		{
			rStatus.m_ctime = CTime(ftCreate);
		}
		else //invalid create time
		{
			rStatus.m_ctime = CTime();
		}

		if ( IsValidFILETIME(ftAccess) )
		{
			rStatus.m_atime = CTime(ftAccess);
		}
		else //invalid access time
		{
			rStatus.m_atime = CTime();
		}

		if ( IsValidFILETIME(ftModify) )
		{
			rStatus.m_mtime = CTime(ftModify);
		}
		else //invalid modify time
		{
			rStatus.m_mtime = CTime();
		}

		if (rStatus.m_ctime.GetTime() == 0)
			rStatus.m_ctime = rStatus.m_mtime;

		if (rStatus.m_atime.GetTime() == 0)
			rStatus.m_atime = rStatus.m_mtime;
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Copy of CTime::IsValidFILETIME from VC.NET 2005

BOOL WINAPI CSafeMirrorFile::IsValidFILETIME(const FILETIME& fileTime) const
{
	FILETIME localTime;
	if (!FileTimeToLocalFileTime(&fileTime, &localTime))
	{
		return FALSE;
	}

	// then convert that time to system time
	SYSTEMTIME sysTime;
	if (!FileTimeToSystemTime(&localTime, &sysTime))
	{
		return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// This is a copy of CDocument::GetFile in order to use a customized CMirrorFile.
// We override this method to fix a bug in the MFC shipped with VC.NET2003,
// where CFile::GetStatus would result in an exception being thrown whenever a file
// was opened off a CD device.
/*
CFile* CApplicationDoc::GetFile(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
	CSafeMirrorFile* pFile = new CSafeMirrorFile;
	ASSERT(pFile != NULL);
	if (!pFile->Open(lpszFileName, nOpenFlags, pError))
	{
		delete pFile;
		pFile = NULL;
	}
	return pFile;
}
*/