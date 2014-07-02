// statdlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "statdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatDlg dialog


CStatDlg::CStatDlg(SEGSTATS *st,CWnd* pParent /*=NULL*/)
	: CDialog(CStatDlg::IDD, pParent)
{
	m_st=st;
	//{{AFX_DATA_INIT(CStatDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CStatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CStatDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CStatDlg)
	ON_BN_CLICKED(IDC_VECTORLIST, OnVectorList)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_ARCVIEW, OnArcVew)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStatDlg message handlers

BOOL CStatDlg::OnInitDialog()
{
    char *na="N/A";
	CDialog::OnInitDialog();
	
	SetText(IDC_COMPONENT,m_st->component);
	SetText(IDC_REFSTATION,m_st->refstation);
	SetUint(IDC_NUMVECS,m_st->numvecs);
	SetUint(IDC_NUMFLAGS,m_st->numflags);
	SetUint(IDC_NUMNOTES,m_st->numnotes);

	SetWndTitle(GetDlgItem(IDC_UNITS),0,LEN_ISFEET()?"Feet":"Meters");

	if(m_st->numvecs) {

		if(m_st->ttlength>0.0) SetFloat(IDC_PERCENT,100.0*m_st->vtlength/m_st->ttlength);
		else SetText(IDC_PERCENT,na);
		SetText(IDC_HIGHSTATION,m_st->highstation);
		SetText(IDC_LOWSTATION,m_st->lowstation);
		SetFloat(IDC_HZLENGTH,m_st->hzlength);
		SetFloat(IDC_VTLENGTH,m_st->vtlength);
		SetFloat(IDC_TTLENGTH,m_st->ttlength);
		SetFloat(IDC_HIGHPOINT,m_st->highpoint);
		SetFloat(IDC_LOWPOINT,m_st->lowpoint);
		SetFloat(IDC_DEPTH,m_st->highpoint-m_st->lowpoint);
	}
	else {
		SetText(IDC_HZLENGTH,na);
		SetText(IDC_VTLENGTH,na);
		SetText(IDC_TTLENGTH,na);
		SetText(IDC_HIGHPOINT,na);
		SetText(IDC_LOWPOINT,na);
		SetText(IDC_DEPTH,na);
		GetDlgItem(IDC_ARCVIEW)->EnableWindow(FALSE);
		GetDlgItem(IDC_VECTORLIST)->EnableWindow(FALSE);
	}
	SetWndTitle(this,IDS_STAT_TITLE,m_st->title);
	
	CenterWindow();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CStatDlg::SetFloat(UINT id,double f)
{
	char buf[40];
	if(id==IDC_PERCENT) sprintf(buf,"%.1f%%",f);
	else sprintf(buf,"%.2f",LEN_SCALE(f));
	SetText(id,buf);
}

void CStatDlg::SetUint(UINT id,UINT u)
{
	char buf[20];
	sprintf(buf,"%u",u);
	SetText(id,buf);
}

LRESULT CStatDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(11,HELP_CONTEXT);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CExpDlg dialog

BOOL CExpDlg::m_bLRUD_init=0;
BOOL CExpDlg::m_bLRUDPT_init=0;

CExpDlg::CExpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExpDlg::IDD, pParent)
	, m_bLRUD(m_bLRUD_init)
	, m_bLRUDPT(m_bLRUDPT_init)
	, m_bElevGrid(TRUE)
	, m_DimGrid("20")
{
}

void CExpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	if(!pDX->m_bSaveAndValidate) {
		m_scaleVert=GetFloatStr(m_dScaleVert,2);
	}

	DDX_Check(pDX, IDC_LOADVIEWER, m_bView);
	DDX_Text(pDX, IDC_3DPATH, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 250);
	DDX_Check(pDX, IDC_ELEVGRID, m_bElevGrid);
	DDX_Text(pDX, IDC_DIMGRID, m_DimGrid);
	DDV_MaxChars(pDX, m_DimGrid, 4);
		DDX_Text(pDX,IDC_SCALEVERT,m_scaleVert);
	DDV_MaxChars(pDX,m_scaleVert,10);
	DDX_Check(pDX, IDC_LRUD, m_bLRUD);
	DDX_Check(pDX, IDC_LRUDPT, m_bLRUDPT);

	if(pDX->m_bSaveAndValidate) {
		UINT id=0;
		if(!CheckFlt(m_scaleVert,&m_dScaleVert,0.0,1000.0,2)) id=IDC_SCALEVERT;
		else if(!CheckInt(m_DimGrid,&m_uDimGrid,0,1000)) id=IDC_DIMGRID;
		if(id) {
			pDX->m_idLastControl=id;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
		}
		//Now let's see if the directory needs to be created --
		else if(CheckDirectory(m_pathname)) {
			pDX->m_idLastControl=IDC_3DPATH;
			pDX->m_bEditLastControl=TRUE;
			pDX->Fail();
			return;
		}
		m_bLRUD_init=m_bLRUD;
		m_bLRUDPT_init=m_bLRUDPT;
	} 
}

BEGIN_MESSAGE_MAP(CExpDlg, CDialog)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CExpDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CExpDlg message handlers

BOOL CExpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CenterWindow();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

LRESULT CExpDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(13,HELP_CONTEXT);
	return TRUE;
}

void CExpDlg::OnBrowse() 
{
	BrowseFiles((CEdit *)GetDlgItem(IDC_3DPATH),m_pathname,".wrl",IDS_WRL_FILES,IDS_WRL_BROWSE);
	//	  OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
}

HBRUSH CStatDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    if(nCtlColor==CTLCOLOR_STATIC) {
		switch(pWnd->GetDlgCtrlID()) {
			case IDC_NUMVECS:
			case IDC_NUMNOTES:
			case IDC_NUMFLAGS:
			case IDC_HIGHPOINT:
			case IDC_LOWPOINT:
			case IDC_DEPTH:
			case IDC_HZLENGTH:
			case IDC_VTLENGTH:
			case IDC_TTLENGTH:
			pDC->SetTextColor(RGB_DKRED);
		}
    }
	return hbr;
}

void CStatDlg::OnVectorList()
{
	EndDialog(IDC_VECTORLIST);
}

void CStatDlg::OnArcVew() 
{
	EndDialog(IDC_ARCVIEW);
}
