// ListDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "ListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CListDlg::m_uDecimals=2;
UINT CListDlg::m_uFlags=0;

/////////////////////////////////////////////////////////////////////////////
// CListDlg dialog

CListDlg::CListDlg(LPCSTR pPath,int nVectors,CWnd* pParent /*=NULL*/)
	: CDialog(CListDlg::IDD, pParent)
	, m_pathname(pPath)
	, m_nVectors(nVectors)
{
	//{{AFX_DATA_INIT(CListDlg)
	//}}AFX_DATA_INIT
	m_bGroupByFlag=(m_uFlags&LFLG_GROUPBYFLAG)!=0;
	m_bGroupByLoc=(m_uFlags&LFLG_GROUPBYLOC)!=0;
	m_bNotedOnly=(m_uFlags&LFLG_NOTEONLY)!=0;
	m_bVector=(m_uFlags&LFLG_VECTOR)!=0;
	m_Decimals=GetIntStr(m_uDecimals);
	m_bLatLong=(m_uFlags&LFLG_LATLONG)!=0;
}

void CListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CListDlg)
	DDX_Check(pDX, IDC_NOTEDONLY, m_bNotedOnly);
	DDX_Check(pDX, IDC_GROUPBYFLAG, m_bGroupByFlag);
	DDX_Check(pDX, IDC_GROUPBYLOC, m_bGroupByLoc);
	DDX_Text(pDX, IDC_DECIMALS, m_Decimals);
	DDV_MaxChars(pDX, m_Decimals, 2);
	DDX_Text(pDX, IDC_PATHNAME, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 260);
	DDX_Check(pDX, IDC_LATLONG, m_bLatLong);
	//}}AFX_DATA_MAP

	//DDX_Radio(pDX, IDC_TYPVECTOR, m_bVector); //Fails! Why???

	if(pDX->m_bSaveAndValidate) {
		int e;
		if(!CheckInt(m_Decimals,(int *)&m_uDecimals,0,6)) {
			pDX->m_idLastControl=IDC_DECIMALS;
			//***7.1 pDX->m_hWndLastControl=GetDlgItem(IDC_DECIMALS)->m_hWnd;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
		//Now let's see if the directory needs to be created --
		else if((e=CheckDirectory(m_pathname,TRUE))) {
			if(e==1) {
				pDX->m_idLastControl=IDC_PATHNAME;
				pDX->m_bEditLastControl=TRUE;
				pDX->Fail();
			}
			else m_uFlags|=LFLG_OPENEXISTING;
		}
		else {
			if(m_uFlags&LFLG_LATLONGOK) m_uFlags=m_bLatLong*LFLG_LATLONG;
			else m_uFlags&=LFLG_LATLONG;
			m_uFlags|=((m_bVector*LFLG_VECTOR)+(m_bGroupByFlag*LFLG_GROUPBYFLAG)+
				(m_bGroupByLoc*LFLG_GROUPBYLOC)+(m_bNotedOnly*LFLG_NOTEONLY));
		}
	}
	else {
		m_uFlags &=~LFLG_OPENEXISTING;
		((CButton *)GetDlgItem(IDC_TYPNAME))->SetCheck(!m_bVector);
		((CButton *)GetDlgItem(IDC_TYPVECTOR))->SetCheck(m_bVector);
		GetDlgItem(IDC_LATLONG)->EnableWindow(!m_bVector);	
		GetDlgItem(IDC_NOTEDONLY)->EnableWindow(!m_bVector);	
		GetDlgItem(IDC_GROUPBYFLAG)->EnableWindow(!m_bVector);
		if(!(m_uFlags&LFLG_LATLONGOK)) {
			m_bLatLong=FALSE;
			GetDlgItem(IDC_LATLONG)->ShowWindow(SW_HIDE);
		}
	}
}

BEGIN_MESSAGE_MAP(CListDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CListDlg)
	ON_BN_CLICKED(IDC_TYPNAME, OnTypName)
	ON_BN_CLICKED(IDC_TYPVECTOR, OnTypVector)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDBROWSE, OnBnClickedBrowse)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListDlg message handlers

void CListDlg::OnTypName() 
{
	m_bVector=FALSE;
	GetDlgItem(IDC_LATLONG)->EnableWindow(TRUE);	
	GetDlgItem(IDC_NOTEDONLY)->EnableWindow(TRUE);	
	GetDlgItem(IDC_GROUPBYFLAG)->EnableWindow(TRUE);	
}

void CListDlg::OnTypVector() 
{
	m_bVector=TRUE;
	GetDlgItem(IDC_LATLONG)->EnableWindow(FALSE);	
	GetDlgItem(IDC_NOTEDONLY)->EnableWindow(FALSE);	
	GetDlgItem(IDC_GROUPBYFLAG)->EnableWindow(FALSE);	
}

LRESULT CListDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(20,HELP_CONTEXT);
	return TRUE;
}

void CListDlg::OnBnClickedBrowse()
{
	BrowseFiles((CEdit *)GetDlgItem(IDC_PATHNAME),m_pathname,".lst",IDS_LST_FILES,IDS_LST_BROWSE);
}

BOOL CListDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s;
	s.Format("Report Options (%d vectors selected)",m_nVectors);
	GetDlgItem(IDC_ST_NUMVECS)->SetWindowText(s);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
