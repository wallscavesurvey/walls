// GEDefaultsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ShpLayer.h"
#include "GEDefaultsDlg.h"

#define MAX_ICONSIZE 30
#define MAX_LABELSIZE 30

// CGEDefaultsDlg dialog

IMPLEMENT_DYNAMIC(CGEDefaultsDlg, CDialog)

CGEDefaultsDlg::CGEDefaultsDlg(LPCSTR pgePath, UINT uKmlRange,
	      UINT uLabelSize, UINT uIconSize, CString &csIconSingle, CString &csIconMultiple, CWnd* pParent /*=NULL*/)
	: CDialog(CGEDefaultsDlg::IDD, pParent)
	, m_PathName(pgePath)
	, m_uKmlRange(uKmlRange)
	, m_uIconSize(uIconSize)
	, m_uLabelSize(uLabelSize)
	, m_csIconSingle(csIconSingle)
	, m_csIconMultiple(csIconMultiple)
{
}

CGEDefaultsDlg::~CGEDefaultsDlg()
{
}

void CGEDefaultsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHNAME, m_PathName);
	DDV_MaxChars(pDX, m_PathName, 260);
	DDX_Control(pDX, IDC_RANGE, m_ceKmlRange);
	DDX_Text(pDX, IDC_RANGE, m_uKmlRange);
	DDV_MinMaxUInt(pDX, m_uKmlRange, 100, 100000);
	DDX_Control(pDX, IDC_EDIT_SINGLE, m_ceIconSingle);
	DDX_Text(pDX, IDC_EDIT_SINGLE, m_csIconSingle);
	DDX_Control(pDX, IDC_EDIT_MULTIPLE, m_ceIconMultiple);
	DDX_Text(pDX, IDC_EDIT_MULTIPLE, m_csIconMultiple);

	if(pDX->m_bSaveAndValidate) {
	    if(!m_PathName.IsEmpty()) {
			if((m_PathName[1]!=':' || _access(m_PathName, 0))) {
				AfxMessageBox("Program's pathname incomplete or no longer valid.");
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->Fail();
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CGEDefaultsDlg, CDialog)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_MSIZE, OnIconSize)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LSIZE, OnLabelSize)
	ON_BN_CLICKED(IDC_LOAD_DFLTS, &CGEDefaultsDlg::OnBnClickedLoadDflts)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDBROWSE, &CGEDefaultsDlg::OnClickedBrowse)
END_MESSAGE_MAP()

void CGEDefaultsDlg::RefreshIconSize()
{
	CString s;
	s.Format("%.1f",m_uIconSize/10.0);
	GetDlgItem(IDC_MSIZE)->SetWindowText(s);
}

void CGEDefaultsDlg::RefreshLabelSize()
{
	CString s;
	s.Format("%.1f",m_uLabelSize/10.0);
	GetDlgItem(IDC_LSIZE)->SetWindowText(s);
}

// CGEDefaultsDlg message handlers
void CGEDefaultsDlg::OnIconSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int i=m_uIconSize-pNMUpDown->iDelta;
	if(i<1) i=MAX_ICONSIZE;
	else if(i>MAX_ICONSIZE) i=1;
	m_uIconSize=i;
	RefreshIconSize();
	*pResult = TRUE;
}

void CGEDefaultsDlg::OnLabelSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	int i=m_uLabelSize-pNMUpDown->iDelta;
	if(i<1) i=MAX_LABELSIZE;
	else if(i>MAX_LABELSIZE) i=1;
	m_uLabelSize=i;
	RefreshLabelSize();
	*pResult = TRUE;
}

BOOL CGEDefaultsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	RefreshIconSize();
	RefreshLabelSize();

	GetDlgItem(IDC_RANGE)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGEDefaultsDlg::OnBnClickedLoadDflts()
{
	m_uIconSize=10;
	m_uLabelSize=8;
	RefreshLabelSize();
	RefreshIconSize();
	m_uKmlRange=1000;
	m_csIconMultiple=CShpLayer::pIconMultiple;
	m_csIconSingle=CShpLayer::pIconSingle;
	UpdateData(FALSE);
}

void CGEDefaultsDlg::OnClickedBrowse()
{
	CString strFilter;
	if(!AddFilter(strFilter, IDS_EXE_FILES)) return;

	TCHAR pf[MAX_PATH];
	CString path;
	if(SHGetSpecialFolderPath(0, pf, CSIDL_PROGRAM_FILES, FALSE)) {
		strcat(pf, "\\");
		path=pf;
	}
	else if(!m_PathName.IsEmpty()) {
		LPCSTR p=trx_Stpnam(m_PathName);
		path.SetString(p, trx_Stpnam(p)-p);
	}

	if(!DoPromptPathName(path, OFN_FILEMUSTEXIST, 3, strFilter,
		TRUE, IDS_COMPARE_FCN, ".exe")) return;

	GetDlgItem(IDC_PATHNAME)->SetWindowText(m_PathName=path);
}


LRESULT CGEDefaultsDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1002);
	return TRUE;
}
