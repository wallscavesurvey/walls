// ExpandNTI.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "mainfrm.h"
#include "MapLayer.h"
#include "NtiLayer.h"
#include "ExpandNTI.h"
#include ".\expandnti.h"


// CExpandNTI dialog

IMPLEMENT_DYNAMIC(CExpandNTI, CDialog)
CExpandNTI::CExpandNTI(CNtiLayer *pLayer, CWnd* pParent /*=NULL*/)
	: CDialog(CExpandNTI::IDD, pParent), m_pLayer(pLayer), m_csTitle(pLayer->FileName()), m_bProcessing(FALSE)
{
	theApp.m_bBackground = FALSE;
	m_job.SetLayer(pLayer);
	m_job.SetExpanding(TRUE);
	m_uFirstRec = pLayer->GetNtiFile().LvlRec(1) - 1;
}

CExpandNTI::~CExpandNTI()
{
	m_job.nti.NullifyHandle();
}

void CExpandNTI::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, m_Progress);
}

BOOL CExpandNTI::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWndTitle(this, 0, m_pLayer->FileName());

	if (CNtiLayer::m_bExpanding > 1) {
		//if((CMainFrame *)theApp.m_pMainWnd->IsIconic()) {
			//(CMainFrame *)theApp.m_pMainWnd->ShowWindow(SW_MINIMIZE);
			//theApp.m_bBackground=TRUE;
			//VERIFY(::SetPriorityClass(::GetCurrentProcess(),IDLE_PRIORITY_CLASS));
			//m_job.Begin(this, WM_MYPROGRESS);	 // Start the job
		//}
		OnBnClickedOk();
	}

	return TRUE;
}


BEGIN_MESSAGE_MAP(CExpandNTI, CDialog)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_MESSAGE(WM_MYPROGRESS, OnProgress)
END_MESSAGE_MAP()

// CExpandNTI message handlers

void CExpandNTI::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	if (m_bProcessing) {
		m_job.Abort();
	}
	else OnCancel();
}

void CExpandNTI::OnBnClickedOk()
{
	if (m_bProcessing) {
		(CMainFrame *)theApp.m_pMainWnd->ShowWindow(SW_MINIMIZE);
		theApp.m_bBackground = TRUE;
		VERIFY(::SetPriorityClass(::GetCurrentProcess(), IDLE_PRIORITY_CLASS));
		//m_job.SetPriority(THREAD_PRIORITY_LOWEST);
		return;
	}

	SetWndTitle(GetDlgItem(IDC_ST_MESSAGE), IDS_NTI_EXPAND);
	SetText(IDOK, "Minimize");
	SetText(IDCANCEL, "Abort");
	Show(IDC_ST_PERCENT);

	m_bProcessing = TRUE;
	m_job.Begin(this, WM_MYPROGRESS);	 // Start the job
}

//////////////////
// Handle progress notification from thread.
//    wp = 0   ==> done
//	  p = -1  ==> aborted
//	  else wp  =  percentage of work completed.
//
LRESULT CExpandNTI::OnProgress(WPARAM wp, LPARAM lp)
{
	if ((CMainFrame *)theApp.m_pMainWnd->IsIconic()) {
		if (wp <= 0 || !theApp.m_bBackground) {
			VERIFY(::SetPriorityClass(::GetCurrentProcess(), NORMAL_PRIORITY_CLASS));
			//m_job.SetPriority(THREAD_PRIORITY_NORMAL);
			GetMF()->UpdateFrameTitleForDocument(m_csTitle);
			(CMainFrame *)theApp.m_pMainWnd->ShowWindow(SW_RESTORE);
		}
	}

	if ((int)wp < 0) {
		theApp.m_bBackground = FALSE;
		m_job.Abort();
		OnCancel();
		return 0;
	}

	if (wp) {
		wp -= m_uFirstRec;
		lp -= m_uFirstRec;
		CString s;
		if (theApp.m_bBackground) {
			s.Format("%.0f%%", (100.0*wp) / lp);
			GetMF()->UpdateFrameTitleForDocument(s);
			return 0;
		}
		s.Format("Records: %u of %u", wp, lp);
		SetText(IDC_ST_PERCENT, s);
		m_Progress.SetPos((100 * wp) / lp);
		return 0;
	}

	OnOK();
	return 0;
}

