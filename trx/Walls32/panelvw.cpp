// panelvw.cpp
// Handles the common characteristics of CTabView panel views (CFormView's).

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "compview.h"
#include "segview.h"
#include "panelvw.h"
#include "compile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPanelView

CPanelView::CPanelView(UINT id)
	: CFormView(id)
{
}

CPanelView::~CPanelView()
{
}

IMPLEMENT_DYNAMIC(CPanelView, CFormView)

BEGIN_MESSAGE_MAP(CPanelView, CFormView)
	ON_WM_SYSCOMMAND()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_PFXNAM,OnUpdatePfxNam)
	//{{AFX_MSG_MAP(CPanelView)
	ON_WM_CTLCOLOR()
	ON_COMMAND(ID_DISPLAYOPTIONS, OnDisplayOptions)
	ON_COMMAND(ID_PRINTOPTIONS, OnPrintOptions)
	ON_COMMAND(ID_ZIPBACKUP, OnZipBackup)
	ON_COMMAND(IDC_LRUDSTYLE, OnLrudStyle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTraView message handlers
void CPanelView::OnSysCommand(UINT nChar,LONG lParam)
{
	if(m_pTabView->doSysCommand(nChar,lParam))
		return;
	CFormView::OnSysCommand(nChar,lParam);
}

HBRUSH CPanelView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
    if(nCtlColor==CTLCOLOR_STATIC) {
      switch(pWnd->GetDlgCtrlID()) {
        case IDC_SYSTTL:
        case IDC_LOOPS:
        case IDC_LOOPSH:
        case IDC_LOOPSV:
        case IDC_TOTALS:
        case IDC_HZLENGTH:
        case IDC_VTLENGTH:
        case IDC_CORRECTION:
        case IDC_F_UVEH:
        case IDC_F_UVEV:
		case IDC_PAGES:
		case IDC_ST_PRINTAREA:
		case IDC_ST_FRAMESIZE:
		case IDC_ST_MAPSCALE:
			nCtlColor=CTLCOLOR_EDIT;
      }
    }
    
    if(nCtlColor==CTLCOLOR_EDIT) {
			pDC->SetTextColor(SRV_DLG_TEXTLOWLIGHT);
			pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));
			return ::GetSysColorBrush(COLOR_3DFACE);
    }

    if(nCtlColor==CTLCOLOR_LISTBOX) {
		pDC->SetTextColor(SRV_DLG_TEXTCOLOR); //black
		pDC->SetBkColor(SRV_DLG_BACKCOLOR);   //white
        return ::GetSysColorBrush(COLOR_3DFACE);
	}

    return CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CPanelView::OnDisplayOptions() 
{
	((CMainFrame *)AfxGetMainWnd())->SetOptions(0);
}


void CPanelView::OnPrintOptions() 
{
	((CMainFrame *)AfxGetMainWnd())->SetOptions(1);
}
/*
void CPanelView::OnMarkerStyle() 
{
	if(pCV) {
		if(pSV->InitSegTree()) pSV->OnSymbols();
	}
}
*/
void CPanelView::OnLrudStyle() 
{
	if(pCV) {
		if(pSV->InitSegTree()) pSV->OnLrudStyle();
	}
}

void CPanelView::OnZipBackup() 
{
	GetDocument()->GetPrjView()->OnZipBackup();
}

void CPanelView::OnUpdatePfxNam(CCmdUI* pCmdUI)
{
	CString buf;
	int iTab=m_pTabView->GetTabIndex();

	ASSERT(pCV);

	if(iTab==TAB_PLOT) {
	  if(CPrjDoc::m_iFindStation) iTab=0;
	  else if(CPrjDoc::m_iFindVector) iTab=1;
	  else iTab=-1;
	}
	else if((iTab==TAB_COMP || iTab==TAB_TRAV) && CPrjDoc::m_iFindVector &&
		abs(CPrjDoc::m_iFindVectorIncr)<2 ) iTab=1;
	else iTab=-1;

	if(iTab>=0 && CPrjDoc::GetPrefixedName(buf,iTab)) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetText(buf);
	}
	else pCmdUI->Enable(FALSE);
	CPrjDoc::m_bVectorDisplayed=iTab+1;
}
