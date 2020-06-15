// wcene.cpp : Windows scene3D
// written in August 1995
// Gerbert Orasche
// copyright: (c) 1994-95
// Institute For Information Processing And Computer Supported New Media (IICM)
// Graz University Of Technology

#include "stdafx.h"
#include "wscene.h"
#include "resource.h"
#include "scenedoc.h"
#include "pcutil/mfcutil.h"

CWinScene3D::CWinScene3D(CSceneDoc* pDoc)
{
	m_pDoc = pDoc;

	// allocate 
	m_pszParseOut = new char[PARSE_BUF];
	*m_pszParseOut = 0;
}

CWinScene3D::~CWinScene3D()
{
	delete[] m_pszParseOut;
}

// sets progress indicator
void CWinScene3D::progress(float fPercent, int iLabel)
{
	GUIControls* pTemp = guiControls();

	//if(pTemp)
	pTemp->setProgIndicator(fPercent);
}

// error message (e.g. during parsing) 
void CWinScene3D::errorMessage(const char* pszError) const
{
	char pszNewError[1024];

	// prevent overflow
	if (strlen(pszError) > 512)
		((char*)pszError)[511] = 0;

	if (strlen(pszError) + strlen(m_pszParseOut) < PARSE_BUF - 1)
	{
		const char* pSrc = pszError;
		char* pDst;
		//int iCount;
		pDst = pszNewError;

		// replace \n with \r\n
		//for(iCount=0;iCount<strlen(pszError);iCount++)
		while (*pSrc != 0)
		{
			if (*pSrc != '\n')
				*pDst = *pSrc;
			else
			{
				*pDst++ = '\015';
				//pDst++;
				*pDst = '\012';
			}
			pDst++;
			pSrc++;
		}
		*pDst = 0;
		strcat(m_pszParseOut, pszNewError);
	}
}

void CWinScene3D::showNumFaces()
{
	unsigned long lPolygons = 0;
	char szBuf[512];

	lPolygons = getNumFaces();
	sprintf(szBuf, "Polygons in scene: %ld\n", lPolygons);
	errorMessage(szBuf);
}

// request all textures
void CWinScene3D::requestTextures()
{

}

// load a texture
void CWinScene3D::loadTextureFile(Material* material, const char* filename)
{

}

// read inline VRML data
int CWinScene3D::readInlineVRMLFile(QvWWWInline* node, const char* filename)
{

	return 1;
}

// read inline VRML data
int CWinScene3D::readInlineVRMLFILE(QvWWWInline* node, FILE* file)
{

	return 1;
}

// navigation hint
void CWinScene3D::navigationHint(int)
{
	// TODO: send message to enable navigation mode...
}


// *** selection ***
// selected object has changed
void CWinScene3D::selectionChanged()
{

}

// define selected object as source anchor
void CWinScene3D::setSourceAnchor()
{

}

// delete a source anchor
void CWinScene3D::deleteSourceAnchor(const SourceAnchor*)
{

}

// define current view as destination anchor
void CWinScene3D::setDestinationAnchor()
{

}

// define the default destination anchor
void CWinScene3D::setDefDestAnchor()
{

}

// *** navigation ***
// history functions
void CWinScene3D::back()
{

}

void CWinScene3D::forward()
{

}

void CWinScene3D::history()
{

}

// hold this viewer
void CWinScene3D::hold()
{

}

void CWinScene3D::docinfo(int)
{

}

// document information
void CWinScene3D::anchorInfo(const SourceAnchor*)
{

}

// *** anchors ***
// activation of a link (view other document)
int CWinScene3D::followLink(const GeometricObject*, const SourceAnchor*)
{

	return 1;
}

// URL-request for inline VRML
void CWinScene3D::requestInlineURL(QvWWWInline* node, const char* url, const char* docurl)
{

}

void CWinScene3D::showResult(void)
{
	CParseDlg* pOutputDlg;

	pOutputDlg = new CParseDlg(m_pszParseOut);
	TRACE("Dialog created\n");
	pOutputDlg->DoModal();
	delete pOutputDlg;
}

/////////////////////////////////////////////////////////////////////////////
// CParseDlg dialog



CParseDlg::CParseDlg(char* pszEditText) : CDialog(CParseDlg::IDD)
{
	m_pszEditText = pszEditText;
}


void CParseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CParseDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CParseDlg, CDialog)
	//{{AFX_MSG_MAP(CParseDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CParseDlg message handlers

BOOL CParseDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_SCENE_EDIT);
	pEdit->SetWindowText(m_pszEditText);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
