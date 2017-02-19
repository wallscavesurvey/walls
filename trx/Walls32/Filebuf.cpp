//FILEBUF.CPP -- Implementation of class CFileBuffer
//
#include "stdafx.h"
#include "filebuf.h"
#include "resource.h"

#include <stdarg.h>

CFileBuffer::CFileBuffer(UINT bufsize)
{
    m_nBufferSize=0;
	if(m_pBuffer=(BYTE *)malloc(bufsize)) {
	  m_nBufferSize=bufsize;
	  m_pWritePtr=m_pBuffer;
	  m_pReadPtr=m_pReadLimit=m_pBuffer+bufsize;
	}
}

CFileBuffer::~CFileBuffer()
{ 
	free(m_pBuffer);
}

BOOL CFileBuffer::OpenFail(const char *pszPathName,UINT nOpenFlags)
{
	CFileException fe;
	if (!m_nBufferSize || !Open(pszPathName,nOpenFlags,&fe))
	{
		if(!m_nBufferSize) AfxMessageBox(IDS_ERR_FILERES);
		else {
			ReportException(pszPathName,&fe,(nOpenFlags&CFile::modeCreate)!=0,IDS_ERR_FILERES);
		}
		return TRUE;
	}
	return FALSE;
}

int CFileBuffer::ReadLine(LPSTR pBuf,int sizBuf)
{
    //Return length of line read, or -1 if there are no more lines.
    //Either LF, CRLF, EOF, or CREOF may terminate a line. The
    //termination character(s) are not stored in the line.
    //NOTE: If the value returned is sizBuf-1, truncation of the
    //line occurred unless the file version was exactly that length.
    
    LPBYTE p=(LPBYTE)pBuf;
    BYTE *pEOL;
    
    if(!m_nBufferSize) {
      return -1;
    }
    if(sizBuf<=0) return 0;
    
    sizBuf--;
	while(TRUE) {
	  int nRead=m_pReadLimit-m_pReadPtr;
	  if(!nRead) {
		if(m_pReadLimit!=m_pBuffer+m_nBufferSize || (nRead=CFile::Read(m_pReadPtr=m_pBuffer,m_nBufferSize))==0) {
		  if(p==(LPBYTE)pBuf) {
		    return -1;
		  }
		  if(m_pReadPtr==m_pBuffer) m_pReadLimit=m_pBuffer; //Return -1 on next call (BUG fix 3/29/05)
		  break;
		}
	  	m_pReadLimit=m_pBuffer+nRead;
	  }
      pEOL=(BYTE *)memchr(m_pReadPtr,'\n',nRead);
      if(pEOL) nRead=pEOL-m_pReadPtr;
      if(nRead) {
	      if(sizBuf) {
	        int len=sizBuf;
	        if(len>nRead) len=nRead;
	        memcpy(p,m_pReadPtr,len);
	        sizBuf-=len;
	        p+=len;
	      }
	      m_pReadPtr+=nRead;
      }
      if(pEOL) {
        m_pReadPtr++;
        break;
      }
    }
    if(p>(LPBYTE)pBuf && p[-1]=='\r') p--;
    *p=0;
    return (int)(p-(LPBYTE)pBuf);      
}

UINT CFileBuffer::Read(void FAR* lpBuf, UINT nCount)
{
    if(!m_nBufferSize) return CFile::Read(lpBuf,nCount);
    
    UINT nRead=m_pReadLimit-m_pReadPtr;
    if(nRead<nCount) {
		if(nRead) nCount-=ReadCopy((LPBYTE &)lpBuf,nRead);
		if(nCount>=m_nBufferSize)
		  return nRead+CFile::Read(lpBuf,nCount);
		if(m_pReadLimit!=m_pBuffer+m_nBufferSize) return nRead;
		m_pReadLimit=m_pBuffer+CFile::Read(m_pReadPtr=m_pBuffer,m_nBufferSize);
	}
	else nRead=0;
	return nRead+ReadCopy((LPBYTE &)lpBuf,nCount);
}

void CFileBuffer::Write(const void FAR* lpBuf, UINT nCount)
{
    if(!m_nBufferSize) {
      CFile::Write(lpBuf,nCount);
      return;
    }
    UINT nSpace=m_nBufferSize-(m_pWritePtr-m_pBuffer);
    if(nSpace<nCount) {
        if(nSpace<m_nBufferSize) {
			if(nSpace) nCount-=WriteCopy((LPBYTE &)lpBuf,nSpace);
			CFile::Write(m_pWritePtr=m_pBuffer,m_nBufferSize);
		}
	    if(nCount>=m_nBufferSize) {
			CFile::Write(lpBuf,nCount);
			return;
	    }
	}
	WriteCopy((LPBYTE &)lpBuf,nCount);
} 

void CDECL CFileBuffer::WriteArgv(const char *format,...)
{
  char buf[256];
  int len;
  va_list marker;
  va_start(marker,format);
  //NOTE: Does the 2nd argument include the terminating NULL?
  //      The return value definitely does NOT.
  if((len=_vsnprintf(buf,255,format,marker))<0) len=254;
  Write(buf,len);
  va_end(marker);
}

void CDECL CFileBuffer::WriteArgv(UINT idFormat,...)
{
	char format[256];
	if(::LoadString(AfxGetInstanceHandle(),idFormat,format,256)) {
		char buf[256];
		int len;
		va_list marker;
		va_start(marker,idFormat);
		if((len=_vsnprintf(buf,255,format,marker))<0) len=254;
		Write(buf,len);
		va_end(marker);
	}
}

void CFileBuffer::FlushBuffer()
{     
	if(m_nBufferSize) {
		if(m_pWritePtr>m_pBuffer) {
			CFile::Write(m_pBuffer,m_pWritePtr-m_pBuffer);
			m_pWritePtr=m_pBuffer;
		}
		m_pReadPtr=m_pReadLimit=m_pBuffer+m_nBufferSize;
	}
}
     
#ifdef __FILEBUF_SEEK
DWORD CFileBuffer::GetPosition()
{
	DWORD pos=CFile::GetPosition();
	if(m_nBufferSize) {
		if(m_pReadPtr<m_pReadLimit) pos-=(m_pReadLimit-m_pReadPtr)
		else if(m_pWritePtr>m_pBuffer) pos+=(m_pWritePtr-m_pBuffer);
	}
	return pos;
}

LONG CFileBuffer::Seek(LONG lOff, UINT nFrom)
{
	FlushBuffer();
	return CFile::Seek(lOff,nFrom);
}
#endif

void CFileBuffer::Flush()
{
    FlushBuffer();
	CFile::Flush();
}

void CFileBuffer::Close()
{
	 FlushBuffer();
	 CFile::Close();
}

void CFileBuffer::Abort()
{
  if(m_nBufferSize) {
    free(m_pBuffer);
    m_pBuffer=NULL;
  }
  CFile::Abort();
}

void CFileBuffer::ReportException(const char* pszPathName, CFileException* e,
		BOOL bSaving, UINT nIDPDefault)
{
	ASSERT_VALID(e);

	UINT nIDP = nIDPDefault;

	switch (e->m_cause)
	{
		case CFileException::fileNotFound:
		case CFileException::badPath:
			nIDP = AFX_IDP_FAILED_INVALID_PATH;
			break;
		case CFileException::diskFull:
			nIDP = AFX_IDP_FAILED_DISK_FULL;
			break;
		case CFileException::accessDenied:
			nIDP = bSaving ? AFX_IDP_FAILED_ACCESS_WRITE :
					AFX_IDP_FAILED_ACCESS_READ;
			break;
		
		case CFileException::badSeek:
		case CFileException::generic:
		case CFileException::tooManyOpenFiles:
		case CFileException::invalidFile:
		case CFileException::hardIO:
		case CFileException::directoryFull:
			nIDP = bSaving ? AFX_IDP_FAILED_IO_ERROR_WRITE :
					AFX_IDP_FAILED_IO_ERROR_READ;
			break;
		default:
			break;
	}
    TRY {
		CString prompt;
		AfxFormatString1(prompt, nIDP, pszPathName);
		AfxMessageBox(prompt, MB_ICONEXCLAMATION, nIDP);
	}
	CATCH(CMemoryException,e) {
		AfxMessageBox(IDS_ERR_FILERES);
	}
	END_CATCH
}
