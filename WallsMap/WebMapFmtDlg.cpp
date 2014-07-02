// WebMapFmtDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WebMapFmtDlg.h"
#include "afxdialogex.h"

// CWebMapFmtDlg dialog
BOOL CWebMapFmtDlg::m_bSaveStatic=FALSE;
CString CWebMapFmtDlg::m_csUsrDefault[DFLT_URL_COUNT];
LPCSTR CWebMapFmtDlg::m_strPrgDefault[DFLT_URL_COUNT]={
	"mapper.acme.com/?ll=<LAT>,<LON>&z=17&t=H&marker0=<LAT>%2C<LON>%2C<NAM>",
	"www.mappingsupport.com/p/gmap4.php?ll=<LAT>,<LON>&z=17&t=h&icon=ch",
	"maps.google.com/maps?z=17&t=h&q=<LAT>+<LON>+(<NAM>)",
	"www.bing.com/maps/?v=2&lvl=17&sty=h&where1=<LAT>%2C%20<LON>&cp=<LAT>~<LON>", //%20 after %2C required
	"www.openstreetmap.org/?mlat=<LAT>&mlon=<LON>#map=15/<LAT>/<LON>",
	"www.txdot.gov/apps/statewide_mapping/StatewidePlanningMap.html?llmz=<LAT>,<LON>,Highway,4"
};

IMPLEMENT_DYNAMIC(CWebMapFmtDlg, CDialog)

CWebMapFmtDlg::CWebMapFmtDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWebMapFmtDlg::IDD, pParent)
	, m_bSave(m_bSaveStatic)
	, m_bChg(false)
{
}

CWebMapFmtDlg::~CWebMapFmtDlg()
{
}


BEGIN_MESSAGE_MAP(CWebMapFmtDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_BN_CLICKED(IDC_RESET, &CWebMapFmtDlg::OnBnClickedReset)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, OnEditChange)
	ON_CBN_SELCHANGE(IDC_URLCOMBO, OnSelChange)
	ON_BN_CLICKED(IDC_UPDATE, &CWebMapFmtDlg::OnBnClickedUpdate)
	ON_BN_CLICKED(IDOK, &CWebMapFmtDlg::OnBnClickedOk)
END_MESSAGE_MAP()

static LPCSTR CheckURL(CString &csURL)
{
	LPCSTR p=NULL;
	int i=csURL.Find("http://");
	if(i>=0) csURL.Delete(0,i+7);
	csURL.Trim();
	if((i=csURL.Find('/'))<=0 || i>80 || csURL.Find(' ')>=0)
		p="URL is invalid or not properly formated.";
	else {
		i++;
		if(csURL.Find("<LAT>")<i || csURL.Find("<LON>")<i) 
		p="URL is missing either a \"<LAT>\" or \"<LON>\" placeholder.";
	}
	return p;
}

// CWebMapFmtDlg message handlers
void CWebMapFmtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_urlCombo);
	DDX_CBString(pDX, IDC_URLCOMBO, m_csURL);
	DDX_Check(pDX, IDC_SAVEASDFLT, m_bSave);

	if(pDX->m_bSaveAndValidate) {
		LPCSTR p=CheckURL(m_csURL);
		if(p) {
			AfxMessageBox(p);
			pDX->m_idLastControl=IDC_URLCOMBO;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
		if(m_bFlyTo) m_bSaveStatic=m_bSave;
	}
	else {
		//CString s;
		//s.LoadStringA(IDS_WEBMAP_INFO);
		//GetDlgItem(IDC_ST_INFO)->SetWindowText(s);
		if(m_bFlyTo) GetDlgItem(IDOK)->SetWindowText("Go to Site");
		else GetDlgItem(IDC_SAVEASDFLT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RESET)->EnableWindow(m_csUsrDefault[m_idx].Compare(m_strPrgDefault[m_idx])!=0); //restore prog default?
		for(int i=0;i<DFLT_URL_COUNT;i++) {
			m_urlCombo.AddString(m_csUsrDefault[i]);
		}
		m_urlCombo.SetCurSel(m_idx); //doesn't trigger OnSelChange()
		if(m_csURL.Compare(m_csUsrDefault[m_idx])!=0) {
			m_urlCombo.SetWindowText(m_csURL); //replace edit box content if different
			//IDC_UPDATE starts disabled
			GetDlgItem(IDC_UPDATE)->EnableWindow(TRUE); //update user default?
		}
	}
}

LRESULT CWebMapFmtDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1001);
	return TRUE;
}


BOOL CWebMapFmtDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_urlCombo.SetFocus();
	return 0;  // return TRUE unless you set the focus to a control
}

void CWebMapFmtDlg::OnBnClickedReset()
{
	m_urlCombo.SetWindowText(m_strPrgDefault[m_idx]);
	OnBnClickedUpdate();
}

void CWebMapFmtDlg::EnableSave()
{
	if(m_bFlyTo) GetDlgItem(IDC_SAVEASDFLT)->EnableWindow(
		m_idx!=CMainFrame::m_idxWebMapFormat || m_csUsrDefault[m_idx].Compare(CMainFrame::m_csWebMapFormat)!=0
	);
}

void CWebMapFmtDlg::OnSelChange() 
{
	int i=m_urlCombo.GetCurSel();
	ASSERT(i>=0);
	if(i>=0) {
		m_idx=i;
		GetDlgItem(IDC_RESET)->EnableWindow(m_csUsrDefault[i].Compare(m_strPrgDefault[i])!=0); //restore prog default?
		EnableSave();
	}
}

void CWebMapFmtDlg::OnEditChange() 
{
	CString s;
	m_urlCombo.GetWindowTextA(s);
	BOOL bEnable=s.Compare(m_csUsrDefault[m_idx])!=0;
	GetDlgItem(IDC_UPDATE)->EnableWindow(bEnable);
	if(m_bFlyTo) {
		if(!bEnable) EnableSave();
		else GetDlgItem(IDC_SAVEASDFLT)->EnableWindow(TRUE);
	}
}

void CWebMapFmtDlg::OnBnClickedUpdate()
{
	//have clicked Update or filled edit with restored version --
	CString s;
	m_urlCombo.GetWindowTextA(s);
	LPCSTR p=CheckURL(s);
	if(p) {
		AfxMessageBox(p);
		return;
	}
	GetDlgItem(IDC_RESET)->EnableWindow(s.Compare(m_strPrgDefault[m_idx])!=0);
	GetDlgItem(IDC_UPDATE)->EnableWindow(0);
    m_csUsrDefault[m_idx]=s;
	EnableSave();
	m_bChg=true;
	m_urlCombo.ResetContent();
	for(int i=0;i<DFLT_URL_COUNT;i++) {
		m_urlCombo.AddString(m_csUsrDefault[i]);
	}
	m_urlCombo.SetCurSel(m_idx);
	m_urlCombo.SetFocus();
}


void CWebMapFmtDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	CDialog::OnOK();
}
