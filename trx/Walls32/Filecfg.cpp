//FILECFG.CPP -- Implementation of class CFileCfg
//
#include "stdafx.h"
#include "resource.h"
#include "filecfg.h"

CFileBuffer * CFileCfg::m_pFile=NULL;
BOOL CFileCfg::m_bIgnoreComments=TRUE;

int CALLBACK CfgLineFcn(NPSTR buf,int sizbuf)
{
	if(!CFileCfg::m_pFile || CFileCfg::m_pFile->m_hFile==CFile::hFileNull) return -1;
	return CFileCfg::m_pFile->ReadLine((LPSTR)buf,sizbuf);
}

BOOL CFileCfg::Open(const char* pszFileName,UINT nOpenFlags,
	CFileException* pException /* = NULL */)
{
    PBYTE pLineBuffer;
    
    ASSERT(!m_pFile); //Only one CFileCfg can be open at once
    if(m_pFile || !(pLineBuffer=(PBYTE)malloc(384))) return FALSE;
	cfg_LineInit(CfgLineFcn,pLineBuffer,384,m_uPrefix);
	if(m_uPrefix>=CFG_PARSE_KEY) cfg_KeyInit(m_pLineTypes,m_numTypes);
	if(!CFileBuffer::Open(pszFileName,nOpenFlags,pException)) {
	  free(cfg_linebuf);
	  cfg_linebuf=NULL;
	  return FALSE;
	}
	m_pFile=this;
	return TRUE;
}

void CFileCfg::Close()
{
    ASSERT(m_pFile==this);
	free(cfg_linebuf);
	cfg_linebuf=NULL;
	m_pFile=NULL;
	CFileBuffer::Close();
}

void CFileCfg::Abort()
{
    ASSERT(m_pFile==this);
	free(cfg_linebuf);
	cfg_linebuf=NULL;
	m_pFile=NULL;
	CFileBuffer::Abort();
}
