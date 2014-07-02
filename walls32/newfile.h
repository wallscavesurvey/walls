//NEWFILE.H -- Class CNewFile
//
//Class for saving data to a new disk file, using buffered _lwrite()
//
#ifndef __NEWFILE_H
#define __NEWFILE_H 

class CNewFile {
   BYTE *m_pLinBuf;
   BYTE *m_pLinLimit;
   BYTE *m_pLinPtr;
   HFILE m_hFile;
   int m_nError;
   BOOL m_bOpen;

private:
   int Flush();
   int Copy(LPBYTE &pData,int nLen)
   {
	  _fmemcpy(m_pLinPtr,pData,nLen);
	  m_pLinPtr+=nLen;
	  pData+=nLen;
	  return nLen;
   }
   
public:
   enum {ERR_OPEN=1,ERR_WRITE,ERR_ALLOC};
   
   CNewFile(const char* pszPathName,int len);
   
   ~CNewFile()
   { 
     free(m_pLinBuf);
     if(m_bOpen) _lclose(m_hFile);
   }
   
   int Error() {return m_nError;}

   BOOL Write(LPBYTE pData,int nLen);

   BOOL Close()
   {
     if(!Flush()) {
       m_bOpen=FALSE;
       if(_lclose(m_hFile)) m_nError=ERR_WRITE;
     }
     return m_nError==0;
   }
};
#endif
