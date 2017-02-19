// editdlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "editdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditDlg dialog

BEGIN_MESSAGE_MAP(CEditDlg, CDialog)
	//{{AFX_MSG_MAP(CEditDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

CEditDlg::CEditDlg(UINT uFlags,int iTyp,CWnd* pParent /*=NULL*/)
	: CDialog(CEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditDlg)
	m_bAutoSeq = FALSE;
	//}}AFX_DATA_INIT
	m_iTyp=iTyp;
	if(iTyp>0) {
		iTyp--;
		m_nWidth=CMainFrame::m_EditWidth[iTyp]&~E_SIZECHG;
		m_nHeight=CMainFrame::m_EditHeight[iTyp];
	}
	m_uFlags=uFlags;
    m_bTabLines=(uFlags&E_TABLINES)!=0;
    m_bGridLines=(uFlags&E_GRIDLINES)!=0;
    m_bEnterTabs=(uFlags&E_ENTERTABS)!=0;
    m_nTabInterval=(uFlags>>8);
    m_bAutoSeq=(uFlags&E_AUTOSEQ)!=0;
}

BOOL CEditDlg::OnInitDialog()
{ 
	static LPCSTR pEditTTL[3]=
	{
	  "Current File",
	  "Non-Survey Files",
	  "Survey Files"
	};
	CDialog::OnInitDialog();
	SetWndTitle(this,IDS_TITLE_EDITOPT,pEditTTL[m_iTyp]);
	CenterWindow();
	if(m_iTyp==2) {
	  GetDlgItem(IDC_AUTOSEQ)->ShowWindow(SW_SHOW);
	  //((CButton *)GetDlgItem(IDC_AUTOSEQ))->SetCheck(m_bAutoSeq);
	}
	if(m_iTyp<=0) {
		GetDlgItem(IDC_EDIT_WIDTH)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_HEIGHT)->EnableWindow(FALSE);
	}
	return TRUE;
}

void CEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditDlg)
	DDX_Check(pDX, IDC_GRIDLINES, m_bGridLines);
	DDX_Check(pDX, IDC_TABLINES, m_bTabLines);
	DDX_Check(pDX, IDC_ENTERTABS, m_bEnterTabs);
	DDX_Text(pDX, IDC_TABINTERVAL, m_nTabInterval);
	DDV_MinMaxUInt(pDX, m_nTabInterval, 2, 20);
	DDX_Check(pDX, IDC_AUTOSEQ, m_bAutoSeq);
	if(m_iTyp>0) {
		DDX_Text(pDX, IDC_EDIT_HEIGHT, m_nHeight);
		DDV_MinMaxUInt(pDX, m_nHeight,20,200);
		DDX_Text(pDX, IDC_EDIT_WIDTH, m_nWidth);
		DDV_MinMaxUInt(pDX, m_nWidth, 20,200);
	}
	//}}AFX_DATA_MAP

	if(pDX->m_bSaveAndValidate) {
		m_uFlags=
			m_bGridLines*E_GRIDLINES+
			m_bTabLines*E_TABLINES+
			m_bEnterTabs*E_ENTERTABS+
			m_bAutoSeq*E_AUTOSEQ+
			(m_nTabInterval<<8);

		if(m_iTyp>0) {
			int i=m_iTyp-1;
			if(m_nHeight!=CMainFrame::m_EditHeight[i] ||
					m_nWidth!=(CMainFrame::m_EditWidth[i]&~E_SIZECHG)) {
				CMainFrame::m_EditHeight[i]=m_nHeight;
				CMainFrame::m_EditWidth[i]=(m_nWidth|E_SIZECHG);
			}
		}
	}
}

LRESULT CEditDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(17,HELP_CONTEXT);
	return TRUE;
}


