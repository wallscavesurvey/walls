// reselectDoc.cpp : implementation of the CReselectDoc class
//

#include "stdafx.h"
#include "reselect.h"

#include "reselectDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReselectDoc

IMPLEMENT_DYNCREATE(CReselectDoc, CDocument)

BEGIN_MESSAGE_MAP(CReselectDoc, CDocument)
	//{{AFX_MSG_MAP(CReselectDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReselectDoc construction/destruction

CReselectDoc::CReselectDoc()
{
	// TODO: add one-time construction code here

}

CReselectDoc::~CReselectDoc()
{
}

BOOL CReselectDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CReselectDoc serialization

void CReselectDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CReselectDoc diagnostics

#ifdef _DEBUG
void CReselectDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CReselectDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CReselectDoc commands
