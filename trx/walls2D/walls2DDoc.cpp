// walls2DDoc.cpp : implementation of the CWalls2DDoc class
//

#include "stdafx.h"
#include "walls2D.h"

#include "walls2DDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWalls2DDoc

IMPLEMENT_DYNCREATE(CWalls2DDoc, CDocument)

BEGIN_MESSAGE_MAP(CWalls2DDoc, CDocument)
	//{{AFX_MSG_MAP(CWalls2DDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWalls2DDoc construction/destruction

CWalls2DDoc::CWalls2DDoc()
{
}

CWalls2DDoc::~CWalls2DDoc()
{
}

/////////////////////////////////////////////////////////////////////////////
// CWalls2DDoc diagnostics

#ifdef _DEBUG
void CWalls2DDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWalls2DDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWalls2DDoc commands

