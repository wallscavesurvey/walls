// TableFillDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "DBGridDlg.h"
#include "TableFillDlg.h"
#include "afxdialogex.h"


// CTableFillDlg dialog

IMPLEMENT_DYNAMIC(CTableFillDlg, CDialog)

CTableFillDlg::CTableFillDlg(WORD nFld,CWnd* pParent /*=NULL*/)
	: CDialog(CTableFillDlg::IDD, pParent)
	, m_nFld(nFld)
	, m_csFillText(_T(""))
	, m_pGDlg((CDBGridDlg *)pParent)
	, m_bYesNo(FALSE)
{

}

CTableFillDlg::~CTableFillDlg()
{
}

void CTableFillDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FILLTEXT, m_csFillText);
	DDX_Radio(pDX, IDC_YES, m_bYesNo);
	if(pDX->m_bSaveAndValidate) {
		char fTyp=m_pGDlg->m_pShp->m_pdb->FldTyp(m_nFld);
		if(fTyp!='L') m_csFillText.Trim();
	    if(fTyp=='C') {
			UINT fMax=m_csFillText.GetLength();
			UINT fLen=m_pGDlg->m_pShp->m_pdb->FldLen(m_nFld);
			if(fLen<fMax) {
				CMsgBox("Text entered is %u characters too long for a field of length %u.",fMax-fLen,fLen);
				pDX->m_idLastControl=IDC_FILLTEXT;
				pDX->m_bEditLastControl=TRUE;
				pDX->Fail();
				return;
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CTableFillDlg, CDialog)
END_MESSAGE_MAP()


// CTableFillDlg message handlers


BOOL CTableFillDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s;
	s.Format("Fill Field %s in Selected Records", m_pGDlg->m_pShp->m_pdb->FldNamPtr(m_nFld));
	SetWindowText(s);
	s.Format("Records selected for revision:  %u", m_pGDlg->m_nSelCount);
	GetDlgItem(IDC_ST_FIELD)->SetWindowText(s);

	bool bYN=(m_pGDlg->m_pShp->m_pdb->FldTyp(m_nFld)=='L');
	if(bYN) {
		GetDlgItem(IDC_ST_CAUTION)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_FILLTEXT)->ShowWindow(SW_HIDE);
	}
	else {
		GetDlgItem(IDC_ST_SETLOGICAL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_YES)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_FALSE)->ShowWindow(SW_HIDE);
	}

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
