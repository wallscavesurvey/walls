//FILECFG.CPP -- Implementation of class CFileCfg
//
#include "stdafx.h"
#include "resource.h"
#include "filecfg.h"
#include <malloc.h>

CFileBuffer * CFileCfg::m_pFile=NULL;
BOOL CFileCfg::m_bIgnoreComments=TRUE;

int CALLBACK CfgLineFcn(NPSTR buf,int sizbuf)
{
	if(!CFileCfg::m_pFile || CFileCfg::m_pFile->m_hFile==CFile::hFileNull) return -1;
	return CFileCfg::m_pFile->ReadLine((LPSTR)buf,sizbuf);
}

void CFileCfg::InitCFG()
{
 	ASSERT(!m_pFile && m_pLineBuffer); //Only one CFileCfg can can have buffers active at once
	cfg_LineInit(CfgLineFcn,m_pLineBuffer,m_nBufferSize,m_uPrefix);
	if(m_uPrefix>=CFG_PARSE_KEY) cfg_KeyInit(m_pLineTypes,m_numTypes);
	m_pFile=this; //static -- indicates ownership
}

BOOL CFileCfg::OpenCFG()
{
	ASSERT(m_hFile!=CFile::hFileNull);
	try {
		CFile::SeekToBegin();
	}
	catch(...) {
		ASSERT(0);
		return FALSE;
	}
	InitCFG();
	return TRUE;
}

BOOL CFileCfg::Open(const char* pszFileName,UINT nOpenFlags,CFileException* pException /* = NULL */)
{
	ASSERT(!m_pFile && !m_pLineBuffer);
	if(!(m_pLineBuffer=(PBYTE)malloc(m_nBufferSize))) {
		m_nBufferSize=0; //document not openable -- tell caller this isn't a file status problem
		AfxMessageBox(IDS_ERR_FILERES);
		return FALSE;
	}
	if(CFileBuffer::OpenFail(pszFileName,nOpenFlags,pException)) {
		free(m_pLineBuffer);
		m_pLineBuffer=NULL;
	    return FALSE;
	}
	InitCFG();
	return TRUE;
}
