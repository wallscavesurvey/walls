// vecdlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "vecdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVecDlg dialog


CVecDlg::CVecDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVecDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVecDlg)
	m_Azimuth = "";
	m_Distance = "";
	m_VertAngle = "";
	//}}AFX_DATA_INIT
}

void CVecDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVecDlg)
	DDX_Text(pDX, IDC_AZIMUTH, m_Azimuth);
	DDV_MaxChars(pDX, m_Azimuth, 20);
	DDX_Text(pDX, IDC_DISTANCE, m_Distance);
	DDV_MaxChars(pDX, m_Distance, 20);
	DDX_Text(pDX, IDC_VERTANGLE, m_VertAngle);
	DDV_MaxChars(pDX, m_VertAngle, 20);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVecDlg, CDialog)
	//{{AFX_MSG_MAP(CVecDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CVecDlg message handlers

BOOL CVecDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CenterWindow();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}
