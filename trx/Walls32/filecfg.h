//FILECFG.H --
//
//CFileCfg -- Class for reading and parsing a CFG file, using buffered I/O.
//This is a C++ wrapper for CFG.C in TRX_STRM.LIB.
//
#ifndef __FILECFG_H
#define __FILECFG_H

#ifndef __TRX_STR_H
#include <trx_str.h>
#endif
#ifndef __FILEBUF_H
#include "filebuf.h"
#endif

int CALLBACK CfgLineFcn(NPSTR buf,int sizbuf);

class CFileCfg : public CFileBuffer
{
public:
    //All members public for now --
    static CFileBuffer *m_pFile; //Used by callback and to prevent multiple opens*/
	static BOOL m_bIgnoreComments;
    
	CFileCfg::CFileCfg(LPSTR FAR *pLineTypes=NULL,int numTypes=0,UINT uPrefix='.',
		UINT sizFileBuf=1024)
		: 	m_pLineTypes(pLineTypes),
			m_numTypes(numTypes),
			m_uPrefix(uPrefix),
			CFileBuffer(sizFileBuf) {
				ASSERT(TRUE);
			}

	~CFileCfg() {
		free(cfg_linebuf);
		cfg_linebuf=NULL;
	}
	  
	virtual BOOL Open(const char* pszFileName,UINT nOpenFlags,CFileException* pError=NULL);
	virtual void Close();
	virtual void Abort();
	
	static int GetArgv(NPSTR npzBuffer,UINT uPrefix=CFG_PARSE_ALL)
	{
		return cfg_GetArgv(npzBuffer,uPrefix);
	}	
	static int GetLine()
	{
	  int iRet;
	  while(!(iRet=cfg_GetLine()) && m_bIgnoreComments); //Ignore comments and blank lines
	  return iRet;
	}
	static NPSTR *Argv() {return cfg_argv;}
	static NPSTR Argv(int i) {return cfg_argv[i];}
	static int Argc() {return cfg_argc;}
	static int LineCount() {return cfg_linecnt;}
	static NPSTR LineBuffer() {return (NPSTR)cfg_linebuf;}
		
    int m_numTypes;
    UINT m_uPrefix;
    LPSTR FAR *m_pLineTypes;
};
#endif   