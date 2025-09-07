// SvgProc.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "wall_shp.h"
#include "SvgProc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSvgProcDlg dialog


CSvgProcDlg::CSvgProcDlg(LPFN_SVG_EXPORT pfnExp, LPFN_ERRMSG pfnErr,
	LPFN_EXPORT_CB pfnCB, CWnd* pParent /*=NULL*/)
	: CDialog(CSvgProcDlg::IDD, pParent)
{
	m_pfnExp = pfnExp;
	m_pfnErr = pfnErr;
	m_pfnCB = pfnCB;
	//{{AFX_DATA_INIT(CSvgProcDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSvgProcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSvgProcDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate) {
		if (!GetDlgItem(IDOK)->IsWindowEnabled()) {
			GetDlgItem(IDC_ST_MESSAGE)->SetWindowText("Processing SVG source...");
			Pause();
			Sleep(500);
			int e = m_pfnExp(SVG_WRITENTW, m_pfnCB);
			if (!e) {
				EndDialog(IDCANCEL);
				pDX->Fail();
			}
			else {
				CString msg;
				msg.Format((e < 0) ? IDS_ERR_NOTENTW1 : IDS_ERR_WRTNTW1, m_pfnErr(e));
				GetDlgItem(IDC_ST_MESSAGE)->SetWindowText(msg);
				GetDlgItem(IDOK)->EnableWindow(TRUE);
				GetDlgItem(IDOK)->ShowWindow(SW_SHOW);
				if (e < 0) m_pfnExp = NULL; //Flag as warning only
				pDX->Fail();
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CSvgProcDlg, CDialog)
	//{{AFX_MSG_MAP(CSvgProcDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSvgProcDlg message handlers

BOOL CSvgProcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	PostMessage(WM_COMMAND, IDOK, 0L);

	return TRUE;
}

void CSvgProcDlg::OnOK()
{
	ShowWindow(SW_SHOW);
	CDialog::OnOK();
}
