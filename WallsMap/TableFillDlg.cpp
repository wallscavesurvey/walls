// TableFillDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MainFrm.h"
#include "WallsMap.h"
#include "DBGridDlg.h"
#include "TableFillDlg.h"

// CTableFillDlg dialog

IMPLEMENT_DYNAMIC(CTableFillDlg, CResizeDlg)

CTableFillDlg::CTableFillDlg(WORD nFld,CWnd* pParent /*=NULL*/)
	: CResizeDlg(CTableFillDlg::IDD, pParent)
	, m_nFld(nFld)
	, m_csFillText(_T(""))
	, m_pGDlg((CDBGridDlg *)pParent)
	, m_bYesNo(FALSE)
	, m_bMemo(false)
{
	m_fTyp=m_pGDlg->m_pShp->m_pdb->FldTyp(m_nFld);
}

CTableFillDlg::~CTableFillDlg()
{
}

HBRUSH CTableFillDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if(pWnd->GetDlgCtrlID()==IDC_ST_FIELD) {
		pDC->SetTextColor(RGB(0x80,0,0));
    }
	return hbr;
}

void CTableFillDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PLACEMENT, m_placement);
	DDX_Radio(pDX, IDC_YES, m_bYesNo);

	if(pDX->m_bSaveAndValidate) {
		if(m_fTyp!='L') {
			m_rtf.GetText(m_csFillText);
			m_csFillText.Trim();;
			if(m_fTyp=='C') {
				UINT fMax=m_csFillText.GetLength();
				UINT fLen=m_pGDlg->m_pShp->m_pdb->FldLen(m_nFld);
				if(fLen<fMax) {
					CMsgBox("Text entered is %u characters too long for a field of length %u.", fMax-fLen, fLen);
					pDX->m_idLastControl=IDC_FILLTEXT;
					pDX->m_bEditLastControl=TRUE;
					pDX->Fail();
					return;
				}
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CTableFillDlg, CResizeDlg)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_EDIT_PASTE, &CTableFillDlg::OnBnClickedEditPaste)
	ON_BN_CLICKED(IDC_SELECT_ALL, &CTableFillDlg::OnBnClickedSelectAll)
END_MESSAGE_MAP()


// CTableFillDlg message handlers


BOOL CTableFillDlg::OnInitDialog()
{
	CResizeDlg::OnInitDialog();

	CString s;
	s.Format("Fill Field %s in Selected Records", m_pGDlg->m_pShp->m_pdb->FldNamPtr(m_nFld));
	SetWindowText(s);
	s.Format("%u", m_pGDlg->m_nSelCount);
	GetDlgItem(IDC_ST_FIELD)->SetWindowText(s);

	if(m_pGDlg->m_nSelCount<=1 && m_pGDlg->NumRecs()>1) GetDlgItem(IDC_SELECT_ALL)->ShowWindow(SW_SHOW);

	if(m_fTyp=='L') {
		GetDlgItem(IDC_ST_CAUTION)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_PASTE)->ShowWindow(SW_HIDE);
		m_placement.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_YES)->SetFocus();
	}
	else {
		s.Format("Enter text to replace all existing values of %s.  Leave box empty to clear field.",m_pGDlg->m_pShp->m_pdb->FldNamPtr(m_nFld));
		GetDlgItem(IDC_ST_CAUTION)->SetWindowText(s);
		m_bMemo=(m_fTyp=='M');
		GetDlgItem(IDC_ST_SETLOGICAL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_YES)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_FALSE)->ShowWindow(SW_HIDE);

		CRect rect;
		m_placement.GetWindowRect( rect );
		ScreenToClient( rect );
		m_rtf.Create( WS_TABSTOP | WS_CHILD | WS_VISIBLE, rect, this, 200, &CMainFrame::m_fEditFont,CMainFrame::m_clrTxt);

		m_xMin = 534; // x minimum resize
		m_yMin = 262; // y minimum resize
		AddControl(200, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE,1); //flickerfree
		//AddControl(IDC_ST_FIELD, CST_NONE, CST_NONE, CST_NONE, CST_NONE, 0);
		//AddControl(IDC_EDIT_PASTE, CST_NONE, CST_NONE, CST_REPOS, CST_NONE, 0);
		AddControl(IDCANCEL, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
		AddControl(IDOK, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);

		GetDlgItem(IDC_EDIT_PASTE)->EnableWindow(m_rtf.CanPaste() && ::IsClipboardFormatAvailable(CF_TEXT));
		m_rtf.SetWordWrap(TRUE);
		m_rtf.SetBackgroundColor(m_bMemo?CMainFrame::m_clrBkg:RGB(255,255,255));
		m_rtf.GetRichEditCtrl().LimitText(128*1024);
		m_rtf.m_pTableFillDlg=this;
		m_rtf.GetRichEditCtrl().SetFocus();
	}

	// TODO:  Add extra initialization here

	return FALSE;  // return TRUE unless you set the focus to a control

}

void CTableFillDlg::OnBnClickedEditPaste()
{
	if(m_rtf.CanPaste()) m_rtf.OnEditPaste();
	m_rtf.GetRichEditCtrl().SetFocus();
}

void CTableFillDlg::OnBnClickedSelectAll()
{
	m_pGDlg->OnSelectAll();
	GetDlgItem(IDC_SELECT_ALL)->ShowWindow(SW_HIDE);
	CString s;
	s.Format("%u", m_pGDlg->m_nSelCount);
	GetDlgItem(IDC_ST_FIELD)->SetWindowText(s);
	m_rtf.GetRichEditCtrl().SetFocus();
}
