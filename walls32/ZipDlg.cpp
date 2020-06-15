// ZipDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "compview.h"
#include "segview.h"
#include "compile.h"
#include "ZipDlg.h"
#include "filecfg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char * zip_ErrMsg(int code);
int zip_Export(LPCSTR pathname, char **pFile, int iFileCnt, UINT flags);

BOOL CZipDlg::m_bIncludeNTA = TRUE;

/////////////////////////////////////////////////////////////////////////////
// CZipDlg dialog


CZipDlg::CZipDlg(CPrjDoc *pDoc, CWnd* pParent /*=NULL*/)
	: CDialog(CZipDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CZipDlg)
	m_bOpenZIP = 0;
	//}}AFX_DATA_INIT
	m_pDoc = pDoc;
	m_pathname = pDoc->ProjectPath();
	strcpy(trx_Stpext(m_pathname), ".zip");
}

void CZipDlg::freeFileArray()
{
	while (m_iFileCnt) free(m_pFile[--m_iFileCnt]);
	free(m_pFile);
	m_pFile = NULL;
}

BOOL CZipDlg::addFile(LPCSTR file)
{
	DWORD len = strlen(file) + 1;

	if (m_iFileCnt == m_iFileLen) {
		void *pTemp = (char **)realloc(m_pFile, (m_iFileLen += 500) * sizeof(char *));
		if (!pTemp) goto _errmem;
		m_pFile = (char **)pTemp;
	}
	if (!(m_pFile[m_iFileCnt] = (char *)malloc(len))) goto _errmem;
	strcpy(m_pFile[m_iFileCnt++], file);
	return TRUE;

_errmem:
	CMsgBox(MB_ICONEXCLAMATION, IDS_ERR_MEMORY);
	return FALSE;
}

BOOL CZipDlg::addSavedGrads(LPCSTR fspec)
{
	WIN32_FIND_DATA fd;

	HANDLE h = ::FindFirstFile(fspec, &fd);
	if (h == INVALID_HANDLE_VALUE) return TRUE;

	char pathName[MAX_PATH];
	strcpy(pathName, fspec);
	fspec = trx_Stpnam(pathName);
	do {
		strcpy((char *)fspec, fd.cFileName);
		if (!addFile(pathName)) {
			::FindClose(h);
			return FALSE;
		}
	} while (::FindNextFile(h, &fd));
	::FindClose(h);
	return TRUE;
}

BOOL CZipDlg::addBranchFiles(CPrjListNode *pNode)
{
	if (m_bIncludeNTA && !pNode->NameEmpty() && !pNode->IsOther()) {
		char *wpath = m_pDoc->WorkPath(pNode, CPrjDoc::TYP_NTAC);
		if (!_access(wpath, 4) && !addFile(wpath)) return FALSE;
		wpath[strlen(wpath) - 1] = 0;
		if (!_access(wpath, 4) && !addFile(wpath)) return FALSE;
		if (pNode == m_pDoc->m_pRoot) {
			char *p = trx_Stpnam(wpath);
			p = strcpy(p, "*.ntae") + 5;
			if (!addSavedGrads(wpath)) return FALSE;
			*p = 'd'; if (!addSavedGrads(wpath)) return FALSE;
			*p = 's'; if (!addSavedGrads(wpath)) return FALSE;
		}
	}

	if (pNode->IsLeaf()) {
		return (pNode->NameEmpty() || addFile(m_pDoc->SurveyPath(pNode)));
	}

	pNode = pNode->FirstChild();
	while (pNode) {
		if (!addBranchFiles(pNode)) return FALSE;
		pNode = pNode->NextSibling();
	}
	return TRUE;
}

void CZipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CZipDlg)
	DDX_Text(pDX, IDC_PATHNAME, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 256);
	DDX_Check(pDX, IDC_INCLUDENTA, m_bIncludeNTA);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate) {
		//Now let's see if the directory needs to be created --
		if (CheckDirectory(m_pathname)) {
			pDX->m_idLastControl = IDC_PATHNAME;
			pDX->m_bEditLastControl = TRUE;
			pDX->Fail();
			return;
		}

		if (!CloseItemProp()) return;
		CPrjView::SaveAndClose(m_pDoc->m_pRoot);
		if (!m_pDoc->SaveModified()) return;
		m_iFileCnt = m_iFileLen = 0;
		m_pFile = NULL;

		if (addFile(m_pDoc->ProjectPath()) && addBranchFiles(m_pDoc->m_pRoot)) {
			int e;
			BOOL bReview = FALSE;
			if (m_pDoc == CPrjDoc::m_pReviewDoc && ixSEG.Opened()) {
				bReview = TRUE;
				if (CSegView::m_bChanged) pSV->SaveSegments();
				ixSEG.Close();
			}
			BeginWaitCursor();
			e = zip_Export(m_pathname, m_pFile, m_iFileCnt, 0);
			EndWaitCursor();
			if (bReview) {
				VERIFY(!m_pDoc->OpenSegment(&ixSEG, CPrjDoc::m_pReviewNode, CTRXFile::ReadWrite));
			}
			if (e) CMsgBox(MB_ICONEXCLAMATION, "Function aborted: %s", zip_ErrMsg(e));
			else {
				m_Message.Format("Archive successfully written --%s", zip_ErrMsg(0));
				m_bOpenZIP = TRUE;
			}
		}
		freeFileArray();
	}
}

BEGIN_MESSAGE_MAP(CZipDlg, CDialog)
	//{{AFX_MSG_MAP(CZipDlg)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CZipDlg message handlers

BOOL CZipDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this, IDS_ZIPDLGFMT, m_pDoc->Title());

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CZipDlg::OnBrowse()
{
	BrowseFiles((CEdit *)GetDlgItem(IDC_PATHNAME), m_pathname, ".zip", IDS_ZIP_FILES, IDS_ZIP_BROWSE);
}


LRESULT CZipDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(26, HELP_CONTEXT);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CZipCreateDlg dialog


CZipCreateDlg::CZipCreateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CZipCreateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CZipCreateDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(CZipCreateDlg, CDialog)
	//{{AFX_MSG_MAP(CZipCreateDlg)
	ON_BN_CLICKED(IDC_EMAILZIP, OnEmailZip)
	ON_BN_CLICKED(IDC_OPENZIP, OnOpenZip)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CZipCreateDlg message handlers

BOOL CZipCreateDlg::OnInitDialog()
{
	static BOOL bIsMailAvail = (BOOL)-1;
	CDialog::OnInitDialog();

	SetWindowText(m_Title);
	GetDlgItem(IDC_ST_MESSAGE)->SetWindowText(m_pMessage);

	if (bIsMailAvail == (BOOL)-1)
	{
		bIsMailAvail = ::GetProfileInt(_T("MAIL"), _T("MAPI"), 0) != 0 &&
			SearchPath(NULL, _T("MAPI32.DLL"), NULL, 0, NULL, NULL) != 0;
	}
	GetDlgItem(IDC_EMAILZIP)->EnableWindow(bIsMailAvail);

	return TRUE;
}

void CZipCreateDlg::OnEmailZip()
{
	m_bFcn = 2;
	EndDialog(IDCANCEL);
}

void CZipCreateDlg::OnOpenZip()
{
	m_bFcn = 1;
	EndDialog(IDCANCEL);
}

