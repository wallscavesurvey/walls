//NEWFILE.CPP -- Implementation of class CNewFile
//
#include <stdafx.h>
#include <malloc.h>
#include "newfile.h"

CNewFile::CNewFile(const char* pszPathname,int len)
{
  m_hFile=_lcreat(pszPathname,0); //Truncate any preexisting file 
  if(m_hFile==HFILE_ERROR) {
    m_nError=ERR_OPEN;
    m_bOpen=FALSE;
    m_pLinBuf=NULL;
  }
  else {
    m_bOpen=TRUE;
    m_pLinBuf=(BYTE *)malloc(len);
    m_pLinPtr=m_pLinBuf;
    m_pLinLimit=m_pLinBuf+len;
    m_nError=m_pLinBuf?0:ERR_ALLOC;
  }
}
   
BOOL CNewFile::Write(LPBYTE pData,int nLen)
{
	 if(!m_nError) {
	   if(m_pLinPtr+nLen>m_pLinLimit) {
	     nLen-=Copy(pData,m_pLinLimit-m_pLinPtr);
	     if(Flush()) nLen=0;
	   }
	   if(nLen) Copy(pData,nLen);
	 }
	 return m_nError==0;  
}
     
int CNewFile::Flush()
{
   if(!m_nError && m_pLinPtr>m_pLinBuf) {
       UINT len=m_pLinPtr-m_pLinBuf;
       if(len!=_lwrite(m_hFile,m_pLinBuf,len)) m_nError=ERR_WRITE;
       m_pLinPtr=m_pLinBuf;
   }
   return m_nError;
}
