// MapExp.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "mapdlg.h"
#include "MapExp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapExpDlg dialog

CMapExpDlg::CMapExpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapExpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapExpDlg)
	m_pathname = _T("");
	//}}AFX_DATA_INIT
}


void CMapExpDlg::FixWMFname()
{
	char *pwmf=".wmf";
	char *p=trx_Stpext(m_pathname);
	if(_stricmp(p,".emf") && _stricmp(p,pwmf)) {
		p=trx_Stpext(m_pathname.GetBuffer(m_pathname.GetLength()+4));
		strcpy(p,pwmf);
	    m_pathname.ReleaseBuffer();
	}
}

void CMapExpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapExpDlg)
	DDX_Text(pDX, IDC_PATHNAME, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 260);
	//}}AFX_DATA_MAP
	
	if(pDX->m_bSaveAndValidate) {
		FixWMFname();
		if(trx_Stpnam(m_pathname)==m_pathname.GetBuffer(0)) { 
			m_pathname=m_defpath+"\\"+m_pathname;
		}
	}
}

BEGIN_MESSAGE_MAP(CMapExpDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CMapExpDlg)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	ON_BN_CLICKED(ID_FORMATOPT, OnFormatOptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapExpDlg message handlers

void CMapExpDlg::OnBrowse() 
{
	CEdit *pw=(CEdit *)GetDlgItem(IDC_PATHNAME);
	CString strFilter;
	CString Path;
	
	pw->GetWindowText(Path);
	
	if(!AddFilter(strFilter,IDS_WMF_FILES) ||
	   !AddFilter(strFilter,IDS_EMF_FILES) ||
	   !AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;
	   
	if(!DoPromptPathName(Path,
	  OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	  3,strFilter,FALSE,
	  IDS_MAPEXP_BROWSE,
	  ".wmf")) return;
	
	m_pathname=Path;
	FixWMFname();
	pw->SetWindowText(m_pathname);
	pw->SetSel(0,-1);
	pw->SetFocus();
}

BOOL CMapExpDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CEdit *pw=(CEdit *)GetDlgItem(IDC_PATHNAME);
	pw->SetSel(0,-1);
	pw->SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
}

void CMapExpDlg::OnFormatOptions() 
{
	CMapDlg dlg(2,this);
	dlg.DoModal();
}

LRESULT CMapExpDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(16);
	return TRUE;
}

