// prjview.cpp : implementation of the CPrjView class
//
#include "stdafx.h"
#include "walls.h"
#include "mainfrm.h"
#include "prjframe.h"
#include "prjdoc.h"
#include "prjview.h"
#include "lineview.h"
#include "wedview.h"
#include "tabview.h"
#include "prjfind.h"
#include "filebuf.h"
#include "itemprop.h"
#include "expsefd.h"
#include "zipdlg.h"
#include "dialogs.h"

extern BOOL hPropHook;
extern CItemProp *pItemProp;
extern CPrjView *pPropView;

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern BOOL hPropHook;
extern CItemProp *pItemProp;

/////////////////////////////////////////////////////////////////////////////
// CPrjView

IMPLEMENT_DYNCREATE(CPrjView, CView)

BEGIN_MESSAGE_MAP(CPrjView, CView)
	//{{AFX_MSG_MAP(CPrjView)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_COMMAND(ID_ENABLETITLES, OnEnableTitles)
	ON_UPDATE_COMMAND_UI(ID_ENABLETITLES, OnUpdateEnableTitles)
	ON_COMMAND(ID_ENABLENAMES, OnEnableNames)
	ON_UPDATE_COMMAND_UI(ID_ENABLENAMES, OnUpdateEnableNames)
	ON_COMMAND(ID_PRJ_DETACH, OnDetachBranch)
	ON_UPDATE_COMMAND_UI(ID_PRJ_DETACH, OnUpdateDetachBranch)
	ON_COMMAND(ID_PRJ_NEWITEM, OnNewItem)
	ON_UPDATE_COMMAND_UI(ID_PRJ_NEWITEM, OnUpdateNewItem)
	ON_COMMAND(ID_PRJ_EDITITEM, OnEditItem)
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_PURGEITEM, OnPurgeItem)
	ON_COMMAND(ID_PRJ_COMPILE, OnCompile)
	ON_UPDATE_COMMAND_UI(ID_PRJ_COMPILEITEM, OnUpdateCompileItem)
	ON_COMMAND(ID_ENABLEBOTH, OnEnableBoth)
	ON_UPDATE_COMMAND_UI(ID_ENABLEBOTH, OnUpdateEnableBoth)
	ON_COMMAND(ID_PURGEBRANCH, OnPurgeBranch)
	ON_COMMAND(ID_RECOMPILE, OnRecompile)
	ON_UPDATE_COMMAND_UI(ID_RECOMPILE, OnUpdateRecompile)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_SEGMENTCLR, OnSegmentClr)
	ON_COMMAND(ID_SEGMENTSET, OnSegmentSet)
	ON_COMMAND(ID_FEETSET, OnFeetSet)
	ON_COMMAND(ID_FEETCLR, OnFeetClr)
	ON_COMMAND(ID_LAUNCH3D, OnLaunch3d)
	ON_COMMAND(ID_EDIT_DELETE, OnDelitems)
	ON_COMMAND(ID_EDIT_LEAF, OnEditLeaf)
	ON_UPDATE_COMMAND_UI(ID_EDIT_LEAF, OnUpdateEditLeaf)
	ON_COMMAND(ID_EXPORTSEF, OnExportSef)
	ON_UPDATE_COMMAND_UI(ID_EXPORTSEF, OnUpdateExportSef)
	ON_COMMAND(ID_PRJ_COMPILEITEM, OnCompileItem)
	ON_COMMAND(ID_PRJ_EXPORTITEM, OnExportItem)
	ON_UPDATE_COMMAND_UI(ID_PRJ_EXPORTITEM, OnUpdateExportItem)
	ON_UPDATE_COMMAND_UI(ID_PRJ_EDITITEM, OnUpdatePrjEditItem)
	ON_COMMAND(ID_REFRESHBRANCH, OnRefreshBranch)
	ON_COMMAND(ID_LAUNCHLOG, OnLaunchLog)
	ON_MESSAGE(WM_CTLCOLORLISTBOX, OnCtlColorListBox)
	//ON_WM_CTLCOLOR()
	ON_COMMAND(ID_REVIEWLAST, OnReviewLast)
	ON_UPDATE_COMMAND_UI(ID_REVIEWLAST, OnUpdateReviewLast)
	ON_COMMAND(ID_LAUNCH2D, OnLaunch2d)
	ON_COMMAND(ID_SORTBRANCH, OnSortBranch)
	ON_UPDATE_COMMAND_UI(ID_SORTBRANCH, OnUpdateSortBranch)
	ON_COMMAND(ID_CLEARCHECKS, OnClearChecks)
	ON_UPDATE_COMMAND_UI(ID_CLEARCHECKS, OnUpdateClearChecks)
	ON_COMMAND(ID_SORTBRANCHREV, OnSortBranchRev)
	ON_UPDATE_COMMAND_UI(ID_LAUNCHLOG, OnUpdateLaunchLogLst)
	ON_UPDATE_COMMAND_UI(ID_LAUNCH2D, OnUpdateLaunch)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFindNext)
	ON_COMMAND(ID_ZIPBACKUP, OnZipBackup)
	ON_UPDATE_COMMAND_UI(ID_LAUNCH3D, OnUpdateLaunch)
	ON_COMMAND(ID_FIND_PREV, OnFindPrev)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREV, OnUpdateFindNext)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_SORTBRANCHREV, OnUpdateSortBranch)
	ON_LBN_SELCHANGE(1,OnSelChanged)
	ON_REGISTERED_MESSAGE(WM_PROPVIEWLB,OnPropViewLB)
	ON_COMMAND(ID_REMOVEPROTECTION, OnRemoveprotection)
	ON_COMMAND(ID_WRITEPROTECT, OnWriteprotect)
#ifdef _USE_FILETIMER
	ON_REGISTERED_MESSAGE(WM_TIMERREFRESH,OnTimerRefresh)
#endif
	//ON_UPDATE_COMMAND_UI(ID_REMOVEPROTECTION, OnUpdateWriteprotect)
	//ON_UPDATE_COMMAND_UI(ID_WRITEPROTECT, OnUpdateWriteprotect)
	ON_COMMAND(ID_PRJ_OPENFOLDER, &CPrjView::OnPrjOpenfolder)
	ON_UPDATE_COMMAND_UI(ID_PRJ_OPENFOLDER, &CPrjView::OnUpdatePrjOpenfolder)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Static initialization/termination

PRJFONT CPrjView::m_BookFont;
PRJFONT CPrjView::m_SurveyFont;
int     CPrjView::m_LineHeight;
int 	CPrjView::m_nPrjWinPos[SIZ_PRJWINPOS];

static char BASED_CODE szSurveyFont[] = "Survey Titles";
static char BASED_CODE szBookFont[] = "Book Titles";

void CPrjView::ReviseLineHeight()
{
    int h=((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_ListBoxRes1.BitmapHeight();
    if(h<m_BookFont.LineHeight) h=m_BookFont.LineHeight;
    if(h<m_SurveyFont.LineHeight) h=m_SurveyFont.LineHeight;
	m_LineHeight=h;
}

void CPrjView::Initialize()
{
	CLogFont lf;
	lf.Init("MS Sans Serif",80,700,34,FALSE,FALSE);
    m_BookFont.Load(szBookFont,FALSE,&lf);
    lf.lfWeight=400;
    lf.lfItalic=FALSE;
    lf.lfUnderline=TRUE;
    m_SurveyFont.Load(szSurveyFont,FALSE,&lf);
    ReviseLineHeight();
}

void CPrjView::Terminate()
{
    m_BookFont.Save();
    m_SurveyFont.Save();
}

void CPrjView::ChangeFont(PRJFONT *pf)
{
   if(pf->Change(CF_EFFECTS|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT)) {
	    CPrjDoc *pd=((CMainFrame *)AfxGetMainWnd())->m_pFirstPrjDoc;
	    while(pd) {
	      ReviseLineHeight();
	      pd->UpdateAllViews(NULL,CPrjDoc::LHINT_FONT,NULL);
	      pd=pd->m_pNextPrjDoc;
	    }
   }
}

BOOL CPrjView::PreCreateWindow( CREATESTRUCT& cs )
{
	//No need of complete refresh with size chg --
	return CView::PreCreateWindow(cs);
	cs.style &= ~(CS_HREDRAW|CS_VREDRAW|WS_BORDER);
    cs.dwExStyle&=~(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);
}

/////////////////////////////////////////////////////////////////////////////
// CPrjView drawing

void CPrjView::OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint)
{
  if(lHint==CPrjDoc::LHINT_FONT) m_PrjList.ChangeTextHeight(m_LineHeight);
  else {
    Invalidate(FALSE);  //CPrjDoc::OnFileSaveAs calls UpdateAllViews()
  }
}

void CPrjView::OnDraw(CDC* pDC)
{
	
	//Kyle Marsh's version fills client area with the button color.
	//This affects painting of the area surrounding the listbox
	//which is a 4-pixel "border" in his version. Without this the border
	//would be transparent to previous colors when window is resized.
	//Unfortuanately, this invalidates the entire listbox whenever the
	//view receives a WM_PAINT message!
	
	CRect r;
	GetClientRect(&r);
	pDC->SetBkColor(::GetSysColor(COLOR_BTNFACE));
	pDC->ExtTextOut(0,0,ETO_OPAQUE,&r,0,0,0);
}

/////////////////////////////////////////////////////////////////////////////
// CPrjView diagnostics

#ifdef _DEBUG
void CPrjView::AssertValid() const
{
	CView::AssertValid();
}

void CPrjView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CPrjDoc* CPrjView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPrjDoc)));
	return (CPrjDoc*) m_pDocument;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPrjView message handlers

int CPrjView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    //Note: This is a response to a message sent after actual window creation
    //(and OnPreCreateWindow()), but before the window is made visible.
    
	if (CView::OnCreate(lpCreateStruct) == -1) return -1;

	CPrjFrame *pf=(CPrjFrame *)GetParentFrame();
	ASSERT(pf && pf->IsKindOf(RUNTIME_CLASS(CPrjFrame)));
	m_pPrjDlgBar=&pf->m_PrjDlgBar;
	
	CMainFrame* mf = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	ASSERT(mf && mf->IsKindOf(RUNTIME_CLASS(CMainFrame)));

    //Assign a (pointer to) structure of listbox attributes which is already an initialized
    //member of the application's CMainFrame. This function also invalidates the listbox window
    //if it had already been created. (Not so in this case.)
    
	m_PrjList.AttachResources(&mf->m_ListBoxRes1);
 	m_PrjList.SetTextHeight(m_LineHeight);

    //Although made visible (0 size), the window will be resized in OnSize().
    //NOTE: MFC does not suggest WS_BORDER for a CListBox but it seems necessary
    //to avoid appearance of horizontal lines when scrolling vertically.
    //We would need to call CListBox::SetHorizontalExtent(width) to have
    //horizontal scroll bars. We don't need them here.
    
    //Contrary to docs, LBS_MULTIPLESEL treats each left button click as
    //a ctrl-click. Right button toggles select off.
    //LBS_EXTENDEDSEL uses standard ctl-shift mult selection (overrides above).
    //Right button still toggles selection.
    //QUESTION: Why is the last parameter, nID, set at 1? Probably irrelevant.
     
#ifdef _USE_BORDER
	if( m_PrjList.Create(
		WS_CHILD|WS_VISIBLE|WS_BORDER| \
		LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|WS_VSCROLL| \
		LBS_OWNERDRAWVARIABLE,\
		CRect(0,0,0,0), this, 1) == FALSE )
		return FALSE;
#else
	if( m_PrjList.Create(
		WS_CHILD|WS_VISIBLE| /*WS_BORDER|*/ \
		LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|WS_VSCROLL| \
		LBS_OWNERDRAWVARIABLE,\
		CRect(0,0,0,0), this, 1) == FALSE )
		return FALSE;
#endif

	((CPrjFrame *)GetParent())->m_pPrjList=&m_PrjList;
	
	if (!pf->m_PrjDlgBar.Create(pf,IDD_PRJSELECT,CBRS_TOP,IDD_PRJSELECT))
	{
		TRACE("Failed to create DlgBar\n");
		return FALSE;      // fail to create
	}

    //The CButtonBar will always restore focus to the listbox --
    m_pPrjDlgBar->SetFocusWindow(&m_PrjList);
    
    UpdateButtons(); //Reset flags to avoid unnecessary updating of the dialog bar
    
	return 0;
}

void CPrjView::OnInitialUpdate()
{
    int ierr;

    //Get rid of 3D border --
	ModifyStyleEx(WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE,0);
    
    if((ierr=m_PrjList.CHierListBox::InsertNode(GetDocument()->m_pRoot,-1))<0)
      CPrjList::PrjErrMsg(ierr);
    m_PrjList.SetCurSel(0);
    
    CView::OnInitialUpdate();
    
    //Lets set the initial window size and position based on the size
    //of the document. We actually do this by resizing the parent frame
    //which has not yet been activated or made visible. (This will be done
    //by ActivateFrame(), which is one of the last operations of the
    //template's OpenDocumentFile()). 
    
	CRect rect(0,0,BUTTON_COUNT*BUTTON_WIDTH,BUTTON_HEIGHT);
	
	CPrjFrame *pPrjFrame=(CPrjFrame *)GetParentFrame();
	ASSERT(pPrjFrame && pPrjFrame->IsKindOf(RUNTIME_CLASS(CPrjFrame)));
	
    //We can't do this since CDialogBar is not derived from CDialog!
    //m_pPrjDlgBar->MapDialogRect(&rect);
    //This works --
	::MapDialogRect(m_pPrjDlgBar->m_hWnd,&rect);
	
	CSize size(rect.right,rect.bottom);
	
    size.cx+=2*GetSystemMetrics(SM_CXFRAME);
	size.cy+=2*(GetSystemMetrics(SM_CYFRAME)+BorderHeight+m_LineHeight);
	size.cy+=GetDocument()->GetDataWidth()+BorderHeight;
	
	int offset=GetSystemMetrics(SM_CYCAPTION);
	
	pPrjFrame->GetParent()->GetClientRect(&rect);
	
	m_nWinOffset=GetWinPos(m_nPrjWinPos,SIZ_PRJWINPOS,rect,size,offset);
	
	pPrjFrame->SetWindowPos(NULL,rect.left+offset,rect.top+offset,size.cx,size.cy,
	 SWP_HIDEWINDOW|SWP_NOACTIVATE);

	LPSTR pStr=GetDocument()->m_pRoot->Title();
	pPrjFrame->SetWindowText(pStr);
}

void CPrjView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType,cx,cy);
    //HDWP h = ::BeginDeferWindowPos(1);
    if(cx>0 && cy>0)
      m_PrjList.MoveWindow(BorderHeight,BorderHeight,
                           cx-2*BorderHeight,cy-2*BorderHeight);
    //::EndDeferWindowPos(h);
}

void CPrjView::OnEnableNames()
{
	m_PrjList.m_bShowNames=TRUE;
	m_PrjList.m_bShowTitles=FALSE;
	m_PrjList.Invalidate();	
}

void CPrjView::OnUpdateEnableNames(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!m_PrjList.m_bShowTitles);	
}

void CPrjView::OnEnableTitles()
{
	m_PrjList.m_bShowTitles=TRUE;
	m_PrjList.m_bShowNames=FALSE;	
	m_PrjList.Invalidate();	
}

void CPrjView::OnUpdateEnableTitles(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!m_PrjList.m_bShowNames);	
}

void CPrjView::OnEnableBoth()
{
	m_PrjList.m_bShowTitles=m_PrjList.m_bShowNames=TRUE;	
	m_PrjList.Invalidate();	
}

void CPrjView::OnUpdateEnableBoth(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_PrjList.m_bShowNames==m_PrjList.m_bShowTitles);	
}

BOOL CPrjView::OnEraseBkgnd(CDC* pDC)
{
    //Response to WM_ERASEBKGND.
    //Disable background repainting since listbox fills view --
	return 1; //CView::OnEraseBkgnd(pDC);
    /*
	// Set brush to desired background color
	CBrush* pOldBrush = pDC->SelectObject(CBrush::FromHandle((HBRUSH)::GetStockObject(LTGRAY_BRUSH)));

	CRect rect;
	pDC->GetClipBox(&rect);     // Erase the area needed

	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
		PATCOPY);
	pDC->SelectObject(pOldBrush);
	return TRUE;
	*/
}

void CPrjView::OnDelitems()
{
    CPrjListNode *pNode=GetSelectedNode();
	CDeleteDlg dlg(pNode->Title());
	UINT id=dlg.DoModal();

	if(id==IDCANCEL) return;

	if(id==IDYES) {
		if(id=GetDocument()->CountBranchReadOnly(pNode)) {
			if(IDOK!=CMsgBox(MB_OKCANCEL,IDS_DELETE_LOCKED1,id)) return;
			GetDocument()->SetBranchReadOnly(pNode,FALSE);
		}
		id=IDYES;
	}

	BeginWaitCursor();
	if(id==IDYES) {
		id=GetDocument()->DeleteBranchSurveys(pNode);
	}
	else id=0;
	//ignores attached status and deletes NTA's --
	if(!pNode->NameEmpty()) PurgeAllWorkFiles(pNode,TRUE,TRUE);
	if(pNode->IsRoot()) {
		m_PrjList.SetCurSel(1);
	    while(m_PrjList.GetCurSel()>0) m_PrjList.DeleteSelectedNode();
	}
	else m_PrjList.DeleteSelectedNode();
	GetDocument()->SaveProject();
	EndWaitCursor();
	if(id) {
		StatusMsg(IDS_DELETE_COUNT1,id);
	}
}

void CPrjView::OnPurgeItem()
{
    CPrjListNode *pNode=GetSelectedNode();
    ASSERT(pNode);
    //Include NTA file --
	PurgeWorkFiles(pNode,TRUE);
	m_PrjList.RefreshNode(pNode);
}

void CPrjView::OnPurgeBranch()
{
    CPrjListNode *pNode=GetSelectedNode();
    ASSERT(pNode);
    //Ignore attached status and include NTA --
	PurgeAllWorkFiles(pNode,TRUE,TRUE);
	RefreshBranch(pNode,TRUE); //Refreshes all attached branch items
}

void CPrjView::PurgeDependentFiles(CPrjListNode *pNode)
{
   if(pNode->m_dwFileChk) {
		while(!pNode->m_bFloating) {
			VERIFY(pNode=pNode->Parent());
			if(pNode->m_dwWorkChk) PurgeWorkFiles(pNode,FALSE);
		}
   }
}

void CPrjView::OnDetachBranch()
{
	CPrjListNode *pNode =
	  (CPrjListNode *)m_PrjList.FloatSelection(!m_PrjList.IsSelectionFloating());

	if(pNode) {
	  //Whether newly attached or detached, we need to update the m_dwFileChk
	  //and m_nEditsPending values of the parent --
	  
      ASSERT(pNode->Parent());
	  BOOL bFloating=pNode->m_bFloating; //Save the new attached/detached state
	  
	  pNode->m_bFloating=FALSE;  		 //Insure parent is affected
	  
      pNode->FixParentChecksums();       //XOR operation (reverse effect of node)
	  
      int nChg=pNode->m_nEditsPending;
      
      if(nChg) pNode->Parent()->IncEditsPending(bFloating?-nChg:nChg);
      
      m_PrjList.RefreshDependentItems(pNode);
	  pNode->m_bFloating=bFloating;      //Restore new attached/detached state of branch
	  
	  GetDocument()->MarkProject(TRUE/*,pNode*/);
	  
	  UpdateButtons();
	}
}

void CPrjView::OnUpdateDetachBranch(CCmdUI* pCmdUI)
{
    pCmdUI->SetText(m_PrjList.IsSelectionFloating()?"&Attach":"&Detach");
    pCmdUI->Enable(m_PrjList.GetCurSel()>0);
}

void CPrjView::OnNewItem()
{
	CPrjListNode *pParent;
	CPrjListNode *pNode;

	if(!CloseItemProp()) return;

	int index=m_PrjList.GetSelectedNode(pParent);
	
	ASSERT(index>=0 && !pParent->m_bLeaf);
	
	TRY {
		pNode=new CPrjListNode(TRUE); //Initialize a default survey node
	}
    CATCH(CMemoryException,e) {
    	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
    	return;
    }
	END_CATCH
    
    //By default, new nodes establish a segment prefix --
	pNode->m_uFlags|= FLG_DEFINESEG;
	
	//Don't attach yet, but allow dialog to check for Name() compatibility
	//with the existing tree --
    pNode->SetParent(pParent);
    if(pParent->IsFeetUnits()) pNode->m_uFlags|=FLG_FEETUNITS;
    
	CItemProp *pDlg;
	TRY {
		pDlg=new CItemProp(GetDocument(),pNode,1);
	}
    CATCH(CMemoryException,e) {
		delete pNode;
    	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
    	return;
    }
	END_CATCH

	if(pDlg->DoModal()==IDOK) {
		CItemGeneral *pG=pDlg->GetGenDlg();
#ifdef _DEBUG
		CItemOptions *pO=pDlg->GetOptDlg();
		//NOTE: CString exceptions possible here!
		ASSERT(!strcmp(pNode->m_Title,pG->m_szTitle));
		ASSERT(!strcmp(pNode->m_Name,pG->m_szName));
		ASSERT(!strcmp(pNode->m_Options,pDlg->GetOptDlg()->m_szOptions));
		ASSERT(pNode->m_uFlags==pDlg->m_uFlags);
		ASSERT(((pNode->m_uFlags&FLG_REFUSECHK)!=0)==(pNode->m_pRef!=NULL));
#endif
/*
		//NOTE: CString exceptions possible here!
		TRY {
			pNode->SetTitle(pG->m_szTitle);
			pNode->SetName(pG->m_szName);
			pNode->SetPath(pG->m_szPath);
			pNode->SetOptions(pO->m_szOptions);
			if(flags & FLG_REFUSECHK) {
				ASSERT(!pNode->m_pRef);
				pNode->m_pRef=new PRJREF;
				*pNode->m_pRef=pR->m_Ref;
			}
		}
	    CATCH(CMemoryException,e) {
	    	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
	    	goto _exitErr;
	    }
		END_CATCH
*/		
		UINT flags=pDlg->m_uFlags;
		if((pNode->m_bLeaf=(pG->m_bSurvey||pG->m_bOther))) flags&=~FLG_SVG_PROCESS;
		
		pNode->m_uFlags=flags;
		
		//Conceivably, we will be reanabling an existing survey file and/or workfile --
		if(pDlg->GetGenDlg()->m_bSurvey) {
			GetDocument()->BranchFileChk(pNode);
			GetDocument()->BranchEditsPending(pNode);
		}
		
		//Insert the node and update dependent parents -- 
		index=m_PrjList.InsertNode(pNode,index);
		
		if(index>0) {
		  GetDocument()->SaveProject();
		  if(pNode->IsLeaf()) {
			  if(!pG->m_bClickedReadonly || !GetDocument()->SetReadOnly(pNode,pG->m_bReadOnly)) {
				CLineDoc *pDoc=GetDocument()->GetOpenLineDoc(pNode);
				if(pDoc) {
					pDoc->UpdateOpenFileTitle(pNode->Title(),pNode->Name());
					pDoc->CheckTitleMark(pDoc->IsModified()||pDoc->m_bActiveChange);
				}
			  }
		  }
		  OnRefreshBranch();
		}
		else
			CPrjList::PrjErrMsg(index);
	}

	delete pDlg;
}

void CPrjView::OnUpdateNewItem(CCmdUI* pCmdUI)
{
    CPrjListNode *pNode=GetSelectedNode();
    pCmdUI->Enable(pNode && !pNode->m_bLeaf);
}

void CPrjView::OnUpdatePrjEditItem(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(hPropHook==FALSE);
}

void CPrjView::OnEditItem()
{
	CPrjListNode *pNode;
	m_PrjList.GetSelectedNode(pNode);
	EditItem(pNode);
}

void CPrjView::UpdateProperties(CItemProp *pDlg)
{		
	enum e_upd {UPD_SAVE=1,UPD_DEP=2,UPD_BR=4,UPD_REF=8,UPD_OPT=16,UPD_NAME=32,
		UPD_PATH=64,UPD_OTHER=128};
	CPrjListNode *pNode=pDlg->m_pNode;

	CItemGeneral *pG=pDlg->GetGenDlg();
	CItemOptions *pO=pDlg->GetOptDlg();
	CItemReference *pR=pDlg->GetRefDlg();
	UINT flags=pDlg->m_uFlags;
	int bChg=0;
	BOOL bPurged=FALSE;

	if((flags&FLG_REFMASK)==(pNode->m_uFlags&FLG_REFMASK)) {
		if(flags&FLG_REFUSECHK) {
			//ASSERT(pNode->m_pRef); //failed when setting new project's georef
			/****/ 
			if(!pNode->m_pRef) {
			  pNode->m_uFlags&=~FLG_REFMASK;
			  bChg=UPD_REF;
			}
			else
			/****/
			if(memcmp(pNode->m_pRef,&pR->m_Ref,sizeof(PRJREF))) {
				*pNode->m_pRef=pR->m_Ref;
				bChg=UPD_SAVE;
			}
		}
		else {
			ASSERT(!pNode->m_pRef);
		}

		//??if((flags&FLG_GRIDMASK)!=(pNode->m_uFlags&FLG_GRIDMASK)) {
		//}
	}
	else bChg=UPD_REF;

	if(pNode->m_Options.Compare(pO->m_szOptions)) bChg|=UPD_OPT;
	
	//NOTE: CString exceptions possible here!
	
	if(bChg>UPD_SAVE) {
		TRY {
			if(bChg&UPD_OPT) pNode->SetOptions(pO->m_szOptions);

			if(bChg&UPD_REF) {
				//Ref flags are different --
				if(flags&FLG_REFUSECHK) {
					ASSERT(!pNode->m_pRef);
					pNode->m_pRef=new PRJREF;
					*pNode->m_pRef=pR->m_Ref;
				}
				else {
					//new ref is inherited or turned off --
					if(pNode->m_uFlags&FLG_REFUSECHK) {
						ASSERT(pNode->m_pRef);
						delete pNode->m_pRef;
						pNode->m_pRef=NULL;
					}
					else {
						ASSERT(!pNode->m_pRef);
					}
				}
			}
		}
		CATCH(CMemoryException,e) {
		   CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
		   return;
		}
		END_CATCH
	}
	
	if(bChg) bChg=(UPD_SAVE|UPD_DEP|UPD_BR);

	if(pNode->m_Name.Compare(pG->m_szName)) bChg|=UPD_NAME;
	if(pNode->m_Path.Compare(pG->m_szPath)) bChg|=UPD_PATH;
	if(pG->m_bOther!=pNode->IsOther()) bChg|=(UPD_OTHER|UPD_SAVE);

	if(bChg) {
		PurgeDependentFiles(pNode);
		PurgeAllWorkFiles(pNode,FALSE,TRUE); //ignore floating
	    bPurged=TRUE;
	}

	//This only affects display --
	if(pNode->m_Title.Compare(pG->m_szTitle)) {
		TRY {
		   pNode->SetTitle(pG->m_szTitle);
		   bChg|=UPD_SAVE;
		}
		CATCH(CMemoryException,e) {
		   CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
		   return;
		}
		END_CATCH
		if(m_PrjList.m_bShowTitles) m_PrjList.InvalidateNode(pNode);
		//Update the window caption (see CPrjFrame::OnSetText()) --
		if(!pNode->m_pParent) {
			CPrjFrame *parent=(CPrjFrame *)GetParent();
			ASSERT(parent->IsKindOf(RUNTIME_CLASS(CPrjFrame)));
			if(parent->GetActiveDocument())
				parent->SetWindowText(pNode->Title());
		}
		else if(pNode->IsLeaf() && !(bChg&UPD_NAME)) {
		   CLineDoc *pLineDoc=GetDocument()->GetOpenLineDoc(pNode);
		   if(pLineDoc) pLineDoc->UpdateOpenFileTitle(pNode->Title(),pNode->Name());
		}
	}

	if(!pG->m_bBook) flags&=~FLG_SVG_PROCESS;
	
	//Check for additional flag changes --
	if(!bPurged && flags!=pNode->m_uFlags) {
		bChg|=UPD_SAVE;
		/*
		if(if(pNode==CPrjDoc::m_pReviewNode && (flags&FLG_VIEWMASK)!=(pNode->m_uFlags&FLG_VIEWMASK)) {
			<default button enable?>
		}
		*/

		if((flags&FLG_MASK_DEPBR)!=(pNode->m_uFlags&FLG_MASK_DEPBR)) {
			bChg|=(UPD_DEP|UPD_BR);
			PurgeDependentFiles(pNode);
			PurgeAllWorkFiles(pNode,FALSE,TRUE); //ignore floating
		}
		else if((flags&FLG_MASK_DEP)!=(pNode->m_uFlags&FLG_MASK_DEP)) {
			PurgeDependentFiles(pNode);
			bChg|=UPD_DEP;
		}
		else if((flags&FLG_MASK_BR)!=(pNode->m_uFlags&FLG_MASK_BR)) {
			PurgeAllWorkFiles(pNode,FALSE,TRUE); //ignore floating
			//Also must clear default views!
			GetDocument()->ClearDefaultViews(pNode);
			bChg|=UPD_BR;
		}
	}

	pNode->m_uFlags=flags;
	
    //Next, make tree revisions due to pathname change only --
	if(bChg&(UPD_NAME|UPD_PATH)) {
		TRY {
		   if(bChg&UPD_NAME) {
			   pNode->SetName(pG->m_szName);
			   bChg|=(UPD_SAVE|UPD_DEP);
		   }
		   if(bChg&UPD_PATH) {
			   pNode->SetPath(pG->m_szPath);
			   bChg|=(UPD_SAVE|UPD_DEP|UPD_BR);
		   }
		}
		CATCH(CMemoryException,e) {
		   CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
		   return;
		}
		END_CATCH

		GetDocument()->RefreshBranch(pNode);
		if(pG->m_bFilesHaveMoved) CPrjDoc::RefreshOtherDocs(GetDocument());
		if(m_PrjList.m_bShowNames) m_PrjList.InvalidateNode(pNode);
	}
      
	//Make tree revisions due to change in leaf status --
	if(pNode->m_bLeaf==pG->m_bBook || (bChg&UPD_OTHER)) {
		bChg=(UPD_SAVE|UPD_DEP);
		ASSERT(pNode->m_bLeaf || !pNode->FirstChild());
		if(pNode->m_dwWorkChk) {
			PurgeWorkFiles(pNode,TRUE); //zeroes m_dwWorkChk
		}
		pNode->FixParentEditsAndSums(-1);
		if(pNode->m_bLeaf==pG->m_bBook) pNode->m_bLeaf=!pG->m_bBook; 
		if(pNode->m_bLeaf && !pNode->IsOther()) {
			GetDocument()->BranchFileChk(pNode);
			GetDocument()->BranchEditsPending(pNode);
			pNode->FixParentEditsAndSums(1);
		}
		else {
			pNode->m_nEditsPending=0;
			pNode->m_dwFileChk=0;
		}
		m_PrjList.InvalidateNode(pNode);
	}
    
	if(pG->m_bClickedReadonly && pNode->IsLeaf()) {
		PostMessage(WM_PROPVIEWLB,(WPARAM)pNode,(LPARAM)pG->m_bReadOnly);
		//GetDocument()->SetReadOnly(pNode,pG->m_bReadOnly);
	}

	if(bChg) {
		if(bChg&UPD_DEP) m_PrjList.RefreshDependentItems(pNode);
		if(bChg&UPD_BR) RefreshBranch(pNode,TRUE); //Ignore float status
		if(!pDlg->m_bNew)
			GetDocument()->SaveProject(); //otherwise new project
    }
}

void CPrjView::EditItem(CPrjListNode *pNode)
{
	CItemProp *pDlg;
	TRY {
		pDlg=new CItemProp(GetDocument(),pNode,0);
	}
    CATCH(CMemoryException,e) {
    	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
    	return;
    }
	END_CATCH

    pDlg->Create(AfxGetApp()->m_pMainWnd);
}

void CPrjView::OpenLeaf(CPrjListNode *pNode)
{
	if(pNode->NameEmpty() || (pNode->m_uFlags&(FLG_SURVEYNOT+FLG_LAUNCHOPEN+FLG_LAUNCHEDIT))==FLG_SURVEYNOT) {
		if((pNode->m_uFlags&(FLG_SURVEYNOT+FLG_LAUNCHOPEN+FLG_LAUNCHEDIT))==FLG_LAUNCHOPEN+FLG_SURVEYNOT)
			goto _launch;
		if(CloseItemProp()) EditItem(pNode); //modeless dialog
		return;
	}

	if(!pNode->IsOther()) {
	  DWORD dwChk=pNode->m_dwFileChk;
	  
	  //Recalculate pNode->m_dwFileChk for the leaf --
	  GetDocument()->BranchFileChk(pNode);
	  
	  if(dwChk!=pNode->m_dwFileChk) {
	    //The file's disk status was changed by an external program!
	    pNode->m_dwFileChk=dwChk;
	    GetDocument()->FixAllFileChecksums(GetDocument()->SurveyPath(pNode)); //hits the disk again
	  }
	}
	
	//At this point, the checksum is current with pNode->Name() nonempty.
	//The file exists if and only if the checksum is nonzero.
    //A nonzero checksum indicates that the file exists -- 
    if(!pNode->IsOther() || (pNode->m_uFlags&FLG_LAUNCHEDIT)) {
		if(pNode->IsChecked()) {
			//If the node is checked, assume that the user would prefer positioning
			//at the matched line --
			CLineView::m_nLineStart=pNode->CheckLine();
			CLineView::m_nCharStart=0;
			strcpy(CWedView::m_pTgtBuf,GetDocument()->m_pFindString);
			CWedView::m_wTgtBufFlags=GetDocument()->m_wFindStringFlags;
		}
		
		GetDocument()->OpenSurveyFile(pNode);
		//Assume TRUE returned. If not, an error msg was displayed.
		return;
	}
_launch:
	//Open Other category --
	LPCSTR p;
	int e=(int)::ShellExecute(NULL,"open",p=GetDocument()->SurveyPath(pNode), NULL, NULL, SW_SHOWNORMAL);
	if(e<=32) {
		switch(e) {
			case SE_ERR_ACCESSDENIED : e=IDS_ERR_LAUNCH_ACC ; break;
			case SE_ERR_FNF : e=IDS_ERR_LAUNCH_FNF; break;
			case SE_ERR_NOASSOC : e=IDS_ERR_LAUNCH_ASSOC; break;
			default : e=IDS_ERR_LAUNCH; break;
		}
		CMsgBox(MB_ICONEXCLAMATION,e,p);
	}
}

void CPrjView::SaveAndClose(CPrjListNode *pNode)
{
    CLineDoc::m_bSaveAndCloseError=FALSE;
 	if(pNode->IsReviewable()<=1) CLineDoc::SaveAllOpenFiles();
}

void CPrjView::OnCompileItem()
{
    //For now we unconditionally save all active (modified) CLineDoc files.
    //CLineDoc::OnSaveDocument() may set m_bSaveAndCloseError=TRUE.
    //OnCompile() will check it before proceeding.
    //Alternatively, we could also close the open files to free more memory.
    //Later we can make SaveAndClose(TRUE) a user option.
    //NOTE: For SaveAndClose(FALSE), we could check each project's
    //m_pRoot->m_nEditsPending count before posting the message. The
    //speed advantage of this is probably not worth the extra code.
    
	if(!CloseItemProp()) return;
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode);
	SaveAndClose(pNode);
	PostMessage(WM_COMMAND,ID_PRJ_COMPILE,0);
}

void CPrjView::OnCompile()
{
    //We arrive here only via a PostMessage() in OnCompileItem() above --
    Pause(); //Wait for focus return from button bar
    
    //In case any of the pending edits were not successfully saved --
    if(CLineDoc::m_bSaveAndCloseError) return;

	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode);
	
	//Compile or review branch --
	
	int e=GetDocument()->Compile(pNode);
	//Return values:
	//   0 - Successful compilation (tree needs refreshing)
	//  -1 - Successful review OR compile error displayed (no action required)
	//  -2 - Empty workfile name (edit properties)
	//  -3 - Error occured at specific line number in leaf item m_pErrorNode

	if(!e) {
		m_PrjList.RefreshNode(pNode);
	}
	else if(e==-2) {
		GetDocument()->m_nActivePage=0;
		EditItem(pNode); //Edit properties
	}
	else if(e==-3) {
		//The WALL-SRV.DLL detected an error during compilation.
		//We must open or reactivate a survey file view window with the
		//caret positioned at the error. At this point, Compile() has
		//already initialized CPrjDoc::m_pErrorNode, CPrjDoc::m_nLineStart,
		//and CPrjDoc::m_nCharStart.
	  
	  	GetDocument()->OpenSurveyFile(NULL);
	}
}

void CPrjView::OnUpdateCompileItem(CCmdUI* pCmdUI)
{
    CPrjListNode *pNode=GetSelectedNode();
    int bRev=pNode?pNode->IsReviewable():0;
    if(!bRev) pCmdUI->SetText("No Files");
    else pCmdUI->SetText((bRev>1)?"&Review":"&Compile");
    pCmdUI->Enable(bRev!=0 && !CPrjDoc::m_bComputing);
}

void CPrjView::OnSelChanged()
{
    UpdateButtons();
	if(hPropHook)
		pItemProp->PostMessage(WM_PROPVIEWLB,(WPARAM)GetDocument());
}

void CPrjView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_PrjList.SetFocus();
}

void CPrjView::OnRecompile()
{
	if(!CloseItemProp()) return;
    CPrjListNode *pNode=GetSelectedNode();
    if(pNode && pNode->IsReviewable()) {
      PurgeWorkFiles(pNode,FALSE);
	  m_PrjList.RefreshNode(pNode);
	}
    OnCompileItem();
}

BOOL CPrjView::SetBranchCheckmarks(CPrjListNode *pNode,WORD findflags)
{
	BOOL bMatched=FALSE;
	
	if(pNode->IsLeaf()) {
	  if(pNode->NameEmpty() || (pNode->IsOther() && !(pNode->m_uFlags&FLG_LAUNCHEDIT))) return FALSE;
	  if(pNode->m_nCheckLine=GetDocument()->FindString(pNode,findflags)) {
	    m_PrjList.InvalidateNode(pNode);
	    bMatched=TRUE;
	  }
	}
	else {
	  CPrjListNode *pChild=pNode->FirstChild();
	  while(pChild) {
	    if((!pChild->IsFloating()||(findflags&F_SEARCHDETACHED)) &&
	      SetBranchCheckmarks(pChild,findflags)) bMatched=TRUE;
	    pChild=pChild->NextSibling();
	  }
	  if(bMatched && pNode->Parent()) {
	    pNode->m_nCheckLine=1;   //Nonzero is all that matters
	    m_PrjList.InvalidateNode(pNode);
	  }
	}
	return bMatched;
}

void CPrjView::OnEditFind()
{
	CPrjFindDlg dlg(this);
	CPrjDoc *pDoc=GetDocument();
	
	if(!dlg.m_FindString.IsEmpty()) {
	   dlg.m_nNumChecked=m_PrjList.GetNumChecked();
	}
	int iResult=dlg.DoModal();
	if(iResult==IDCANCEL) return;
	
	//Clear all node checkmark flags in project, invalidating as required --
	m_PrjList.ClearAllCheckmarks();
	
	if(iResult==IDOK && !dlg.m_FindString.IsEmpty()) {
	  if(!CWedView::AllocTgtBuf() ||
	     !pDoc->m_pFindString && !(pDoc->m_pFindString=(char *)malloc(2*PRJ_SIZ_FINDBUF)))
	     goto _errmem;
	     
	  strcpy(pDoc->m_pFindString,dlg.m_FindString);
	  pDoc->m_wFindStringFlags=dlg.m_wFlags;

	  if(!dlg.m_bUseRegEx && !dlg.m_bMatchCase) {
			_strupr(strcpy(pDoc->m_pFindString+PRJ_SIZ_FINDBUF,dlg.m_FindString));
	  }

	  {
		  CPrjListNode *pNode=GetSelectedNode();
		  CFileBuffer file(0x800);
		  if(!file.BufferSize()) goto _errmem;
		  if(dlg.m_bUseRegEx) {
			VERIFY(pDoc->m_pRegEx=GetRegEx(dlg.m_FindString,dlg.m_wFlags));
		  }
		  pDoc->m_pFileBuffer=&file;
		  BeginWaitCursor();
 		  if(SetBranchCheckmarks(pNode,dlg.m_wFlags)) {
 		    iResult=m_PrjList.GetBranchNumChecked(pNode);
 		    if(iResult==1)
				//IDS_PRJ_FINDONE1=="One file contains ""%s""."
			    StatusMsg(IDS_PRJ_FINDONE1,pDoc->m_pFindString);
			else
				//IDS_PRJ_FINDCOUNT2=="There are %u files containing ""%s""."
			    StatusMsg(IDS_PRJ_FINDCOUNT2,iResult,pDoc->m_pFindString);

			strcpy(CWedView::m_pTgtBuf,pDoc->m_pFindString);
			CWedView::m_wTgtBufFlags=pDoc->m_wFindStringFlags;
 		  }
		  else 
			  //IDS_PRJ_FINDCOUNT1=="No files contain  ""%s""."
			  StatusMsg(IDS_PRJ_FINDCOUNT1,pDoc->m_pFindString);

 		  EndWaitCursor();
		  if(dlg.m_bUseRegEx) {
			  free(pDoc->m_pRegEx);
			  pDoc->m_pRegEx=NULL;			
		  }
	  }
	}
	else if(pDoc->m_pFindString) *pDoc->m_pFindString=0;
	return;
	
_errmem:	
	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_MEMORY);
}

void CPrjView::OnSegmentClr()
{
    if(GetSelectedNode()->SetChildFlags(~FLG_DEFINESEG,TRUE))
      GetDocument()->MarkProject();
}

void CPrjView::OnSegmentSet()
{
    if(GetSelectedNode()->SetChildFlags(FLG_DEFINESEG,TRUE))
      GetDocument()->MarkProject();
}

void CPrjView::OnFeetSet()
{
	CPrjListNode *pNode=GetSelectedNode();
	BOOL bSet=pNode->SetItemFlags(FLG_FEETUNITS);
    if(pNode->SetChildFlags(FLG_FEETUNITS,FALSE) || bSet)
      GetDocument()->MarkProject();
}

void CPrjView::OnFeetClr()
{
	CPrjListNode *pNode=GetSelectedNode();
	BOOL bSet=pNode->SetItemFlags(~FLG_FEETUNITS);
    if(pNode->SetChildFlags(~FLG_FEETUNITS,FALSE) || bSet)
      GetDocument()->MarkProject();
}

void CPrjView::OnEditLeaf() 
{
    CPrjListNode *pNode=GetSelectedNode();
	if(pNode->m_bLeaf) OpenLeaf(pNode);
}

void CPrjView::OnUpdateEditLeaf(CCmdUI* pCmdUI) 
{
    CPrjListNode *pNode=GetSelectedNode();
    pCmdUI->Enable(pNode && pNode->m_bLeaf);
}

void CPrjView::OnUpdateRecompile(CCmdUI* pCmdUI) 
{
    CPrjListNode *pNode=GetSelectedNode();
    int bRev=pNode?pNode->IsReviewable():0;
    pCmdUI->Enable(bRev!=0 && !CPrjDoc::m_bComputing);
}

void CPrjView::OnExportSef() 
{
 	CPrjDoc *pDoc=GetDocument();
	CExpSefDlg dlg;

	dlg.m_pDoc=pDoc;
	dlg.m_pNode=GetSelectedNode();
	dlg.m_pathname=pDoc->WorkPath(NULL, 0); //workpath folder path only, folder name should be wpj file name
	if(!dlg.m_pNode->IsRoot()) {
	    CString sefName(dlg.m_pNode->Title());
		FixFilename(sefName);
	    int i=dlg.m_pathname.ReverseFind('\\');
		if(i) dlg.m_pathname.Truncate(i+1);
		else dlg.m_pathname+='\\';
		dlg.m_pathname+=sefName;
	}
	dlg.m_pathname+=".SEF";
	if(dlg.DoModal()==IDOK && pDoc->m_pErrorNode) pDoc->OpenSurveyFile(NULL);
}

void CPrjView::OnUpdateExportSef(CCmdUI* pCmdUI)
{
	CPrjListNode *pNode=GetSelectedNode();
	int bRev=pNode?(!pNode->IsOther()):0;
	pCmdUI->Enable(bRev!=0 && !CPrjDoc::m_bComputing);
}

void CPrjView::OnExportItem()
{
    //This code similar to OnCompileItem() --
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode);
	SaveAndClose(pNode);
	PostMessage(WM_COMMAND,ID_EXPORTSEF,0);
}

void CPrjView::OnUpdateExportItem(CCmdUI* pCmdUI)
{
    CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode);
    pCmdUI->Enable(!pNode->IsOther() && !CPrjDoc::m_bComputing);
}

void CPrjView::OnRefreshBranch() 
{
	GetDocument()->RefreshBranch(GetSelectedNode());
}

LRESULT CPrjView::OnCtlColorListBox(WPARAM wParam, LPARAM lParam)
{
	return (LRESULT)::GetStockObject(LTGRAY_BRUSH);
}

void CPrjView::OnReviewLast() 
{
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode && pNode->m_dwWorkChk && !CPrjDoc::m_bComputing);
	GetDocument()->Review(pNode);
}

void CPrjView::OnUpdateReviewLast(CCmdUI* pCmdUI) 
{
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode);
    pCmdUI->Enable(pNode->m_dwWorkChk && !CPrjDoc::m_bComputing);
}

void CPrjView::OnPrjOpenfolder()
{
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode && pNode->IsLeaf());
	if(OpenContainingFolder(m_PrjList.m_hWnd,GetDocument()->SurveyPath(pNode)))
		((CMainFrame *)AfxGetMainWnd())->SetHierRefresh();
}

void CPrjView::OnUpdatePrjOpenfolder(CCmdUI *pCmdUI)
{
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode);
	pCmdUI->Enable(pNode->IsLeaf());
}

void CPrjView::OnZipBackup() 
{
	CZipDlg dlg(GetDocument());
	dlg.DoModal();
	if(dlg.m_bOpenZIP) {
		CZipCreateDlg cdlg;
		cdlg.m_pMessage=(LPCSTR)dlg.m_Message;
		cdlg.m_Title.Format("Archive: %s",(LPCSTR)dlg.m_pathname);
		cdlg.m_bFcn=0;
		cdlg.DoModal();
		if(cdlg.m_bFcn==1) {
			::ShellExecute(NULL, "open", dlg.m_pathname, NULL, NULL, SW_SHOWNORMAL);
		}
		else if(cdlg.m_bFcn==2) {
			GetDocument()->SendAsEmail(dlg.m_pathname);
		}
	}
}

void CPrjView::SortBranch(BOOL bNameRev)
{
	CPrjListNode *pNode=GetSelectedNode();
	ASSERT(pNode && pNode->FirstChild() && pNode->IsOpen());
	if(pNode->SortBranch(m_PrjList.m_bShowNames|bNameRev)) { //Recursive function
		//Tree was modified --
		VERIFY(pNode->FirstChild()->FixChildLevels());
		m_PrjList.DeepRefresh(pNode,pNode->GetIndex());
		GetDocument()->MarkProject();
	}
}

void CPrjView::OnSortBranch() 
{
	SortBranch(0);
}

void CPrjView::OnSortBranchRev() 
{
	SortBranch(2);
}

void CPrjView::OnUpdateSortBranch(CCmdUI* pCmdUI) 
{
	CPrjListNode *pNode=GetSelectedNode();
	pCmdUI->Enable(pNode && pNode->IsOpen());
}

void CPrjView::OnClearChecks() 
{
	m_PrjList.ClearAllCheckmarks();
}

void CPrjView::OnUpdateClearChecks(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_PrjList.GetNumChecked()!=0);

}

BOOL CPrjView::IsExportedAvail(UINT typ)
{
	CPrjListNode *pNode=GetSelectedNode();
	return !pNode->IsOther() || pNode->IsType(typ);
}

void CPrjView::OnLaunch2d() 
{
    GetDocument()->LaunchFile(GetSelectedNode(),CPrjDoc::TYP_2D);
}

void CPrjView::OnLaunch3d()
{
    GetDocument()->LaunchFile(GetSelectedNode(),CPrjDoc::TYP_3D);
}

void CPrjView::OnUpdateLaunch(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CPrjView::OnLaunchLog() 
{
    GetDocument()->LaunchFile(GetSelectedNode(),CPrjDoc::TYP_LOG);
}

void CPrjView::OnUpdateLaunchLogLst(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!GetSelectedNode()->IsOther());
}

void CPrjView::OnFindNext() 
{
	CPrjListNode *pNode=GetSelectedNode();
	if(pNode->IsLeaf() ||
		!(pNode=pNode->GetFirstBranchCheck()) && (pNode=GetSelectedNode()))
		pNode=pNode->GetNextTreeCheck();
	if(!pNode) pNode=m_PrjList.Root()->GetFirstBranchCheck();
	if(pNode) m_PrjList.SelectNode(pNode);
}

void CPrjView::OnUpdateFindNext(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_PrjList.GetNumChecked());
}

void CPrjView::OnFindPrev() 
{
	CPrjListNode *pLast=NULL;
	m_PrjList.Root()->GetLastCheckBefore(&pLast,GetSelectedNode());
	if(!pLast) m_PrjList.Root()->GetLastCheckBefore(&pLast,NULL);
	if(pLast) m_PrjList.SelectNode(pLast);
}

LRESULT CPrjView::OnPropViewLB(WPARAM wParam,LPARAM lParam)
{
	GetDocument()->SetReadOnly((CPrjListNode *)wParam,(BOOL)lParam);
	return TRUE;
}

void CPrjView::SetReadOnly(BOOL bReadOnly)
{
	CPrjListNode *pNode=GetSelectedNode();
	int nFiles;
	if(bReadOnly && (nFiles=GetDocument()->BranchEditsPending(pNode))) {
		if(IDOK!=CMsgBox(MB_OKCANCEL,IDS_PRJ_OPENCHMODN1,nFiles)) return;
	}
	LPCSTR pType=bReadOnly?"enabl":"remov";
	if((nFiles=GetDocument()->SetBranchReadOnly(pNode,bReadOnly))) {
		if(pNode->IsLeaf() && hPropHook) {
			pItemProp->UpdateReadOnly(bReadOnly);
		}
		StatusMsg(IDS_PRJ_CHMODCOUNT2,pType,nFiles);
	}
	else StatusMsg(IDS_PRJ_CHMODCOUNT1,pType);
}

void CPrjView::OnRemoveprotection()
{
	SetReadOnly(FALSE);
}

void CPrjView::OnWriteprotect()
{
	SetReadOnly(TRUE);
}

#ifdef _USE_FILETIMER
LRESULT CPrjView::OnTimerRefresh(WPARAM,LPARAM)
{
	GetDocument()->TimerRefresh();
	//MessageBeep(0xFFFFFFFF);   // Beep
	return TRUE;
}
#endif
