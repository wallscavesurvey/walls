// ImageZoomDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ImageZoomDlg.h"


// CImageZoomDlg dialog

IMPLEMENT_DYNAMIC(CImageZoomDlg, CDialog)

CImageZoomDlg::CImageZoomDlg(LPCSTR pTitle,CWnd* pParent /*=NULL*/)
	: CDialog(CImageZoomDlg::IDD, pParent)
	, m_bZoomToAdded(TRUE)
	, m_pTitle(pTitle)
{
}

CImageZoomDlg::~CImageZoomDlg()
{
}

void CImageZoomDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_ZOOMTOADDED, m_bZoomToAdded);
	if(!pDX->m_bSaveAndValidate)
		SetWindowText(m_pTitle);
}


BEGIN_MESSAGE_MAP(CImageZoomDlg, CDialog)
	ON_BN_CLICKED(IDNO, &CImageZoomDlg::OnBnClickedNo)
	ON_BN_CLICKED(IDYES, &CImageZoomDlg::OnBnClickedYes)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


// CImageZoomDlg message handlers

void CImageZoomDlg::OnBnClickedNo()
{
	UpdateData();
	EndDialog(IDNO);
}

void CImageZoomDlg::OnBnClickedYes()
{
	UpdateData();
	EndDialog(IDYES);
}

LRESULT CImageZoomDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}
