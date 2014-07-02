////////////////////////////////////////////////////////////////
// 1998 Microsoft Systems Journal. 
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 5.0 on Windows 95
//
// Implementation for CThreadJob, a generic worker thread.
// 
#include "stdafx.h"
#include "ThreadJob.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CThreadJob, CObject)

CThreadJob::CThreadJob() : m_pThread(NULL)
{
}

CThreadJob::~CThreadJob()
{
	if (m_pThread) {
		TRACE("CThreadJob: *** Warning: deleting active thread! ***\n");
		delete m_pThread;
	}
}

//////////////////
// Thread proc calls virtual DoWork function. This converts the
// Windows/C-style thread procedure into an MFC/C++-style virtual function.
// To do the "work" of the thread, implement DoWork and don't worry about
// the thread proc.
//
UINT CThreadJob::ThreadProc(LPVOID pObj)
{
	CThreadJob* pJob = (CThreadJob*)pObj;
	ASSERT_KINDOF(CThreadJob, pJob);
	pJob->m_uErr = pJob->DoWork();		 // call virt fn to do the work
	pJob->m_pThread = NULL;					 // done: clear
	return pJob->m_uErr;						 // ..and return error code to Windows
}

//////////////////
// Begin running the worker thread. Args are owner window and callback
// message ID to use for OnProgress notifications, if any. You could enhance
// this to expose pritority and other AfxBeginThread args.
//
BOOL CThreadJob::Begin(CWnd* pWndOwner, UINT ucbMsg)
{
	m_hWndOwner = pWndOwner->GetSafeHwnd();
	m_ucbMsg = ucbMsg;
	m_bAbort = FALSE;
	m_uErr = 0;
	m_pThread = AfxBeginThread(ThreadProc, this);
	return m_pThread != NULL;
}

//////////////////
// Abort the thread. All this does is set m_bAbort = TRUE.
// It's up to you to check m_bAbort periodically in your DoWork function.
// You can override to use CEvent if you need to.
//
void CThreadJob::Abort(BOOL bAbort)
{
	m_bAbort = bAbort;
}

//////////////////
// Report progress generically in the form of WPARAM/LPARAM.
// Your DoWork function can call this whenever it likes to post a message
// to the owning window. It's up to you what wp/lp mean.
// OnProgress uses PostMessage (instead of SendMessage) in order not to
// wait for the owner to process the message. Contrary to what was printed
// in the article, the message handler will run in the main thread whether
// you use SendMessage or PostMessage.
//
LRESULT CThreadJob::OnProgress(WPARAM wp, LPARAM lp)
{
	if (m_hWndOwner && m_ucbMsg) {
		::PostMessage(m_hWndOwner, m_ucbMsg, wp, lp);
	}
	return 0;
}
