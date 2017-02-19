// CopySelectedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
//#include "afxdialogex.h"
#include  "DBGridDlg.h"
#include  "ShowShapeDlg.h"
#include "CopySelectedDlg.h"

// CCopySelectedDlg dialog

IMPLEMENT_DYNAMIC(CCopySelectedDlg, CDialog)

CCopySelectedDlg::CCopySelectedDlg(CShpLayer *pLayer,LPBYTE pSrcFld, BOOL bConfirm, UINT nSelCount, CWnd* pParent /*=NULL*/)
	: m_pLayer(pLayer), m_pSrcFlds(pSrcFld)
	, m_bConfirm(bConfirm)
	, m_nSelCount(nSelCount)
	, m_pDlg((CResizeDlg *)pParent)
	, CDialog(CCopySelectedDlg::IDD, pParent)
	, m_bWindowDisplayed(false)
{
	m_pDlg->IsKindOf(RUNTIME_CLASS(CShowShapeDlg));
}

CCopySelectedDlg::~CCopySelectedDlg()
{
}

void CCopySelectedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
}


BEGIN_MESSAGE_MAP(CCopySelectedDlg, CDialog)
	ON_WM_CLOSE()
	ON_WM_WINDOWPOSCHANGED()
	ON_MESSAGE(WM_APP,OnWindowDisplayed)
END_MESSAGE_MAP()


// CCopySelectedDlg message handlers

void CCopySelectedDlg::OnClose() 
{
	//X icon at top-right
	m_bWindowDisplayed=false;
	//do not destroywindow here, EndDialog() called upon return of m_pDlg->CopySelected()
}

void CCopySelectedDlg::OnCancel() 
{
	OnClose(); //cancel button
}


BOOL CCopySelectedDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString title;
	title.Format("Appending to %s", trx_Stpnam(m_pLayer->PathName()));
	SetWindowText(title);

	return TRUE;  // return TRUE unless you set the focus to a control

}


void CCopySelectedDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanged(lpwndpos);

	if(!m_bWindowDisplayed && (lpwndpos->flags & SWP_SHOWWINDOW)) {
		m_bWindowDisplayed=true;
		PostMessage(WM_APP, 0, 0);
	}
}

BOOL CCopySelectedDlg::ShowCopyProgress(UINT nCopied)
{
	CString s;
	s.Format("%u of %u records appended...",nCopied,m_nSelCount);
	GetDlgItem(IDC_ST_PROGRESS)->SetWindowText(s);
	m_Progress.SetPos((100*nCopied)/m_nSelCount);
	Pause();
	if(!m_bWindowDisplayed) {
		if(nCopied<m_nSelCount &&
			IDYES==CMsgBox(MB_YESNO, "Stop after only %u of %u selected records copied?", nCopied, m_nSelCount))
				return FALSE;
		m_bWindowDisplayed=true;
	}
	return TRUE;
}

LONG CCopySelectedDlg::OnWindowDisplayed(UINT, LONG)
{
	ASSERT(m_bWindowDisplayed);
	//m_Progress.SetPos(0); //needed?
	if(m_pDlg->IsKindOf(RUNTIME_CLASS(CShowShapeDlg)))
		((CShowShapeDlg *)m_pDlg)->CopySelected(m_pLayer, m_pSrcFlds, m_bConfirm);
	else
		((CDBGridDlg *)m_pDlg)->CopySelected(m_pLayer, m_pSrcFlds, m_bConfirm);
	EndDialog(IDOK);
	return 0;
}
