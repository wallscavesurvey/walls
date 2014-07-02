// GPSOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "GPSOptionsDlg.h"
#include "afxdialogex.h"


// CGPSOptionsDlg dialog

IMPLEMENT_DYNAMIC(CGPSOptionsDlg, CDialog)

CGPSOptionsDlg::CGPSOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGPSOptionsDlg::IDD, pParent)
	, m_iConfirmCnt(2)
{
}

CGPSOptionsDlg::~CGPSOptionsDlg()
{
}

void CGPSOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_AUTORECORDSTART, m_bAutoRecordStart);
	DDX_Radio(pDX, IDC_RADIO1, m_bCenterPercent);
	DDX_Control(pDX, IDC_CONFIRMCNT, m_cbConfirmCnt);
	DDX_CBIndex(pDX, IDC_CONFIRMCNT, m_iConfirmCnt);
}

BEGIN_MESSAGE_MAP(CGPSOptionsDlg, CDialog)
END_MESSAGE_MAP()


// CGPSOptionsDlg message handlers
