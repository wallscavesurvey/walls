// FldsIgnoreDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "shplayer.h"
#include "FldsIgnoreDlg.h"

// CFldsIgnoreDlg dialog

IMPLEMENT_DYNAMIC(CFldsIgnoreDlg, CDialog)

CFldsIgnoreDlg::CFldsIgnoreDlg(CShpLayer *pShp, VEC_WORD &vFldsIgnored, CWnd* pParent /*=NULL*/)
	: CDialog(CFldsIgnoreDlg::IDD, pParent)
	, m_pShp(pShp)
	, m_vFldsIgnored(vFldsIgnored)
{

}

CFldsIgnoreDlg::~CFldsIgnoreDlg()
{
}

void CFldsIgnoreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FLDS_COMPARED, m_lbFldsCompared);
	DDX_Control(pDX, IDC_FLDS_IGNORED, m_lbFldsIgnored);
	if (pDX->m_bSaveAndValidate) {
		m_vFldsIgnored.clear();
		int n = m_lbFldsIgnored.GetCount();
		for (int i = 0; i < n; i++) {
			m_vFldsIgnored.push_back((WORD)m_lbFldsIgnored.GetItemData(i));
		}
	}
}

BEGIN_MESSAGE_MAP(CFldsIgnoreDlg, CDialog)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_MOVEALL_LEFT, &CFldsIgnoreDlg::OnBnClickedMoveallLeft)
	ON_BN_CLICKED(IDC_MOVEALL_RIGHT, &CFldsIgnoreDlg::OnBnClickedMoveallRight)
	ON_BN_CLICKED(IDC_MOVELEFT, &CFldsIgnoreDlg::OnBnClickedMoveFldLeft)
	ON_BN_CLICKED(IDC_MOVERIGHT, &CFldsIgnoreDlg::OnBnClickedMoveFldRight)
	ON_LBN_DBLCLK(IDC_FLDS_COMPARED, &CFldsIgnoreDlg::OnBnClickedMoveFldRight)
	ON_LBN_DBLCLK(IDC_FLDS_IGNORED, &CFldsIgnoreDlg::OnBnClickedMoveFldLeft)
END_MESSAGE_MAP()

BOOL CFldsIgnoreDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	DBF_pFLDDEF pfd = m_pShp->m_pdb->m_pFld;
	int numf = m_pShp->m_pdb->NumFlds();
	CListBox *pList;
	for (int f = 1; f <= numf; f++, pfd++) {
		if (f != m_pShp->KeyFld()) {
			if (std::find(m_vFldsIgnored.begin(), m_vFldsIgnored.end(), f) != m_vFldsIgnored.end())
				pList = &m_lbFldsIgnored;
			else  pList = &m_lbFldsCompared;
			pList->SetItemData(pList->AddString(pfd->F_Nam), (DWORD_PTR)f);
		}
	}

	m_lbFldsCompared.SetFocus();
	return FALSE;  // return TRUE unless you set the focus to a control
}

void CFldsIgnoreDlg::UpdateButtons()
{
	BOOL b = m_lbFldsCompared.GetCount() > 0;
	Enable(IDC_MOVERIGHT, b);
	Enable(IDC_MOVEALL_RIGHT, b);

	b = m_lbFldsIgnored.GetCount() > 0;
	Enable(IDC_MOVELEFT, b);
	Enable(IDC_MOVEALL_LEFT, b);
}

void CFldsIgnoreDlg::MoveAllFields(CListBox &list)
{
	m_lbFldsCompared.SetRedraw(0);
	m_lbFldsCompared.ResetContent();
	m_lbFldsIgnored.SetRedraw(0);
	m_lbFldsIgnored.ResetContent();

	DBF_pFLDDEF pfd = m_pShp->m_pdb->m_pFld;
	WORD numf = m_pShp->m_pdb->NumFlds();
	for (int f = 1; f <= numf; f++, pfd++) {
		if (f != m_pShp->KeyFld())
			list.SetItemData(list.AddString(pfd->F_Nam), (DWORD_PTR)f);
	}
	list.SetCurSel(0);
	m_lbFldsCompared.SetRedraw(1);
	m_lbFldsIgnored.SetRedraw(1);
	UpdateButtons();
}

void CFldsIgnoreDlg::OnBnClickedMoveallLeft()
{
	MoveAllFields(m_lbFldsCompared); //exclude all layers
}

void CFldsIgnoreDlg::OnBnClickedMoveallRight()
{
	MoveAllFields(m_lbFldsIgnored); //include all layers
}

void CFldsIgnoreDlg::MoveOne(CListBox &list1, CListBox &list2, int iSel1, BOOL bToRight)
{
	if (iSel1 < 0) return;
	CString s;
	list1.GetText(iSel1, s);
	int iFld = list1.GetItemData(iSel1);
	VERIFY(list1.DeleteString(iSel1) != LB_ERR);
	if (iSel1 == list1.GetCount()) iSel1--;
	list1.SetCurSel(iSel1);
	iSel1 = list2.GetCount();
	for (; iSel1; iSel1--) {
		if (iFld > (int)list2.GetItemData(iSel1 - 1)) break;
	}
	if (iSel1 == list2.GetCount()) iSel1 = -1;
	VERIFY((iSel1 = list2.InsertString(iSel1, s)) >= 0);
	list2.SetItemData(iSel1, iFld);
	list2.SetCurSel(iSel1);
	UpdateButtons();
}

void CFldsIgnoreDlg::OnBnClickedMoveFldLeft()
{
	MoveOne(m_lbFldsIgnored, m_lbFldsCompared, m_lbFldsIgnored.GetCurSel(), FALSE);
}

void CFldsIgnoreDlg::OnBnClickedMoveFldRight()
{
	MoveOne(m_lbFldsCompared, m_lbFldsIgnored, m_lbFldsCompared.GetCurSel(), TRUE);
}
