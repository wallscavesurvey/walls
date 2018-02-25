// ResizeFrmDlg.cpp : implementation file
//
#include "stdafx.h"
#include "Walls.h"
#include "plotview.h"
#include "ResizeFrmDlg.h"

// CResizeFrmDlg dialog

IMPLEMENT_DYNAMIC(CResizeFrmDlg, CDialog)
CResizeFrmDlg::CResizeFrmDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CResizeFrmDlg::IDD, pParent)
	, m_bSave(FALSE)
{
}

CResizeFrmDlg::~CResizeFrmDlg()
{
}

void CResizeFrmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SAVEASDEFAULT, m_bSave);
	if(pDX->m_bSaveAndValidate)
		m_dfltWidthInches=atof(GetFloatStr((double)m_iCurrentWidth/m_iPx_per_in, 2));
}

BEGIN_MESSAGE_MAP(CResizeFrmDlg, CDialog)
END_MESSAGE_MAP()

// CResizeFrmDlg message handlers

BOOL CResizeFrmDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s;
	s.Format("%s in. (assuming %u px/in)",GetFloatStr((double)m_iCurrentWidth/m_iPx_per_in,2),m_iPx_per_in);
	GetDlgItem(IDC_ST_CURRENT)->SetWindowText(s);
	s.Format("%s in. (%u px.)", GetFloatStr(m_dfltWidthInches, 2), (int)(m_iPx_per_in*m_dfltWidthInches));
	GetDlgItem(IDC_ST_DEFAULT)->SetWindowText(s);

	return TRUE;  // return TRUE unless you set the focus to a control
}
