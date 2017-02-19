// dbrcdoc.cpp : implementation of the CDBRecDoc and
//                CDBRecHint classes

#include "stdafx.h"
#include "resource.h"
#include "dbrcdoc.h"
#include "utility.h"
#include <memory.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CDBRecHint, CObject)
IMPLEMENT_DYNAMIC(CDBRecDoc, CDocument)

BEGIN_MESSAGE_MAP(CDBRecDoc,CDocument)
	//{{AFX_MSG_MAP(CDBRecDoc)
	ON_COMMAND(ID_NEXT_REC, OnNextRec)
	ON_UPDATE_COMMAND_UI(ID_NEXT_REC, OnUpdateNextRec)
	ON_COMMAND(ID_PREV_REC, OnPrevRec)
	ON_UPDATE_COMMAND_UI(ID_PREV_REC, OnUpdatePrevRec)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////
BOOL CDBRecDoc::m_bInitialOpen=FALSE;

CDBRecHint::CDBRecHint()
{
    m_recUpdated=FALSE;
}

CDBRecDoc::CDBRecDoc()
{
	m_nActiveRecNo = 0;
	m_pFldDef=NULL;
	m_pRec=NULL;
}

CDBRecDoc::~CDBRecDoc()
{
}

///////////////////////////////////////////////////////////////////////
// CDocument overrides

BOOL CDBRecDoc::OnOpenDocument(const char* pszPathName)
{ 
    int e;

	UINT mode=CDBFile::Share|CDBFile::ReadOnly|CDBFile::Sequential;
    
	// The standard CDocument::OnOpenDocument() opens the document,
	// deserializes it, and then closes the document.
	// We will avoid the serialization mechanism entirely!
	
	// The following is a modification of CDocument::OnOpenDocument --

#ifdef _DEBUG
	if (IsModified())
		TRACE0("Warning: OnOpenDocument replaces an unsaved document\n");
#endif

 	DeleteContents();

_retry:

	if ((e=m_dbf.Open(pszPathName,mode))==0)
	{
       if((m_pFldDef=m_dbf.FldDef(1))==NULL) {
         e=m_dbf.Errno();
         m_dbf.Close();
       }
       else {
         m_nNumFlds=m_dbf.NumFlds();
         m_pRec=(PBYTE)m_dbf.RecPtr();
         if(m_dbf.NumRecs()) m_nActiveRecNo=1L;
         SetPathName(pszPathName);
	     SetModifiedFlag(FALSE);     // start off with unmodified
	     return TRUE;
       }
	}
	if(!m_bInitialOpen) {
		if(e==CDBFile::ErrNotClosed && mode==(CDBFile::ReadOnly|CDBFile::Sequential)) {
			mode|=CDBFile::ForceOpen;
			CMsgBox(MB_ICONEXCLAMATION,"NOTE: Database %s is not in a closed form. "
				" The ForceOpen flag will be used in an attempt to open the file.",
				pszPathName);
			goto _retry;
		}
		CMsgBox(MB_ICONEXCLAMATION,"Cannot open %s.\n%s",pszPathName,m_dbf.Errstr(e));
	}
	return FALSE;
}

BOOL CDBRecDoc::OnSaveDocument(const char*)
{
    if(IsModified()) {
	  //BeginWaitCursor();
      int e=m_dbf.Flush();
	  SetModifiedFlag(FALSE); 
	  //EndWaitCursor();
	  if(e) {
	    CMsgBox(MB_ICONEXCLAMATION,"Save error: %Fs",m_dbf.Errstr(e));
	    return FALSE;
      }
	}
	return TRUE;        // success
}

void CDBRecDoc::DeleteContents()
{
	// We close the file when the framework wants to
	// clear out the CDocument object for potential reuse.  

    int e=m_dbf.Opened()?m_dbf.Close():0;
	if(e)
	  CMsgBox(MB_ICONEXCLAMATION,"Close error: %Fs",m_dbf.Errstr(e));
	m_nActiveRecNo = 0L;
}

///////////////////////////////////////////////////////////////////////
// Operations

BOOL CDBRecDoc::CreateNewRecord()
{
	// CreateNewRecord is called by the view to create a new fixed
	// length record.  Increment the record count and update the
	// the header to reflect the addition of a new record in the file.
	// Notify all views about the new record.

	int e;
	    
	if((e=m_dbf.AppendBlank())!=0) {
	  CMsgBox(MB_ICONEXCLAMATION,"Cannot add record: %Fs",m_dbf.Errstr(e));
	  return FALSE;
	}
    FillNewRecord(m_pRec);
	m_dbf.Mark();
	m_hint.m_recUpdated=TRUE;
	UpdateAllViews(NULL,m_nActiveRecNo=(LONG)m_dbf.Position(),&m_hint);
	m_hint.m_recUpdated=FALSE;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// Implementation

void * CDBRecDoc::RecPtr(LONG nRecNo)
{
	int e=m_dbf.Go((DWORD)nRecNo);
	if(e) {
	  CMsgBox(MB_ICONEXCLAMATION,"Cannot access record: %Fs",m_dbf.Errstr(e));
	  return NULL;
	}
	return m_pRec;
}

BOOL CDBRecDoc::GetRecord(LONG nRecNo,void *pRecord)
{
	if(RecPtr(nRecNo)) {
	  memcpy(pRecord,m_pRec,m_dbf.SizRec());
	  return TRUE;
	}
	return FALSE;
}

void CDBRecDoc::UpdateRecord(CView* pSourceView,LONG nRecNo,void* pRecord) 
{
	int e=m_dbf.Go((DWORD)nRecNo);
	
	if(e) CMsgBox(MB_ICONEXCLAMATION,"Access error: %Fs",m_dbf.Errstr(e));
	else {
	  m_dbf.PutRec(pRecord);
	  m_dbf.Mark();
	  m_hint.m_recUpdated=TRUE;
	  UpdateAllViews(pSourceView,nRecNo,&m_hint);
	  m_hint.m_recUpdated=FALSE;
	}
}

void CDBRecDoc::ActivateNext(BOOL bNext)
{
	if (bNext)
	{
		if (m_nActiveRecNo < GetRecordCount())
		{
			UpdateAllViews(NULL, ++m_nActiveRecNo,&m_hint);
		}
	}
	else
	{
		if (m_nActiveRecNo > 1L)
		{
			UpdateAllViews(NULL,--m_nActiveRecNo,&m_hint);
		}
	}
}

void CDBRecDoc::Activate(LONG nRecNo)
{
	UpdateAllViews(NULL, m_nActiveRecNo=nRecNo,&m_hint);
}

void CDBRecDoc::OnNextRec()
{
	ActivateNext(TRUE);
}

void CDBRecDoc::OnPrevRec()
{
	ActivateNext(FALSE);
}

void CDBRecDoc::OnUpdateNextRec(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_nActiveRecNo < GetRecordCount());
}

void CDBRecDoc::OnUpdatePrevRec(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_nActiveRecNo > 1L);

}
