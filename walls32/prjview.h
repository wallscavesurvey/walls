// prjview.h : interface of the CPrjView class
//
/////////////////////////////////////////////////////////////////////////////
#if !defined(PRJVIEW_H)
#define PRJVIEW_H

#if !defined(PRJHIER_H)
 #include "prjhier.h"
#endif

#ifndef __PRJFONT_H
 #include "prjfont.h"
#endif

#ifndef BUTTONB_H
 #include "buttonb.h"
#endif

class CItemProp;

class CPrjView : public CView
{
friend class CPrjList;

public:
	    CPrjList m_PrjList;       //Constructed after CView()
        static int     m_LineHeight;
#ifdef _USE_BORDER
        enum {BorderHeight=10};
#else
        enum {BorderHeight=0};
#endif
        static PRJFONT m_BookFont;
        static PRJFONT m_SurveyFont;
        
private:
		//These constants are in dialog box size units (obtained from WALLS.RC) --
		enum {BUTTON_COUNT=4,BUTTON_WIDTH=44,BUTTON_HEIGHT=13};
		enum {SIZ_PRJWINPOS=5};
        
        CButtonBar *m_pPrjDlgBar; //Set to parent frame's m_PrjDlgBar in On_Create()

		static int m_nPrjWinPos[SIZ_PRJWINPOS];
		int m_nWinOffset;
        
// create from serialization only
	    CPrjView() {m_nWinOffset=0;}
	    
	    DECLARE_DYNCREATE(CPrjView)

//Overrides --
        virtual void OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint);

// Attributes
public:
        void UpdateButtons() {m_pPrjDlgBar->OnUpdateCmdUI((CFrameWnd *)AfxGetMainWnd(),TRUE);}
        
        //Accessed from CPrjDoc --
        void RefreshDependentItems(CPrjListNode *pNode)
        {
        	m_PrjList.RefreshDependentItems(pNode);
        }

        void RefreshBranch(CPrjListNode *pNode,BOOL bIgnoreFloating)
		{
			m_PrjList.RefreshBranch(pNode,bIgnoreFloating);
		}

		void OpenLeaf(CPrjListNode *pNode);
        void EditItem(CPrjListNode *pNode);
		void OpenProperties() {OnEditItem();}
		void UpdateProperties(CItemProp *pDlg);

		CPrjListNode *GetSelectedNode() { return m_PrjList.GetSelectedNode();}
	    static void Initialize();
        static void Terminate();
		static void ChangeFont(PRJFONT *pf);
		static void SaveAndClose(CPrjListNode *pNode);
        CPrjDoc* GetDocument();
		void OnSearch();
		static void OnTerminateSearch() {StatusClear();}
        
private:
		BOOL IsExportedAvail(UINT typ);

		void PurgeDependentFiles(CPrjListNode *pNode);
		void PurgeWorkFiles(CPrjListNode *pNode,BOOL bIncludeNTA=FALSE)
		{
			GetDocument()->PurgeWorkFiles(pNode,bIncludeNTA);
		}
		void PurgeAllWorkFiles(CPrjListNode *pNode,BOOL bIncludeNTA=FALSE,BOOL bIgnoreFloating=FALSE)
		{
			GetDocument()->PurgeAllWorkFiles(pNode,bIncludeNTA,bIgnoreFloating);
		}
	    static void ReviseLineHeight();
        void SetListFonts();
		BOOL SetBranchCheckmarks(CPrjListNode *pNode,WORD findflags);
		void SortBranch(BOOL bNameRev);
		void SetReadOnly(BOOL bReadOnly);

// Implementation

public:
	virtual ~CPrjView() {m_nPrjWinPos[m_nWinOffset]=0;}
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT&);
	virtual void OnInitialUpdate();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
	afx_msg void OnZipBackup();
protected:
	//{{AFX_MSG(CPrjView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEnableTitles();
	afx_msg void OnUpdateEnableTitles(CCmdUI* pCmdUI);
	afx_msg void OnEnableNames();
	afx_msg void OnUpdateEnableNames(CCmdUI* pCmdUI);
	afx_msg void OnDetachBranch();
	afx_msg void OnUpdateDetachBranch(CCmdUI* pCmdUI);
	afx_msg void OnNewItem();
	afx_msg void OnUpdateNewItem(CCmdUI* pCmdUI);
	afx_msg void OnEditItem();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPurgeItem();
	afx_msg void OnCompile();
	afx_msg void OnUpdateCompileItem(CCmdUI* pCmdUI);
	afx_msg void OnEnableBoth();
	afx_msg void OnUpdateEnableBoth(CCmdUI* pCmdUI);
	afx_msg void Disable(CCmdUI* pCmdUI);
	afx_msg void OnPurgeBranch();
	afx_msg void OnRecompile();
	afx_msg void OnEditFind();
	afx_msg void OnSegmentClr();
	afx_msg void OnSegmentSet();
	afx_msg void OnFeetSet();
	afx_msg void OnFeetClr();
	afx_msg void OnLaunch3d();
	afx_msg void OnDelitems();
	afx_msg void OnEditLeaf();
	afx_msg void OnUpdateEditLeaf(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRecompile(CCmdUI* pCmdUI);
	afx_msg void OnExportSef();
	afx_msg void OnCompileItem();
	afx_msg void OnExportItem();
	afx_msg void OnUpdateExportItem(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePrjEditItem(CCmdUI* pCmdUI);
	afx_msg void OnRefreshBranch();
	afx_msg void OnLaunchLog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnReviewLast();
	afx_msg void OnUpdateReviewLast(CCmdUI* pCmdUI);
	afx_msg void OnLaunch2d();
	afx_msg void OnSortBranch();
	afx_msg void OnUpdateSortBranch(CCmdUI* pCmdUI);
	afx_msg void OnClearChecks();
	afx_msg void OnUpdateClearChecks(CCmdUI* pCmdUI);
	afx_msg void OnSortBranchRev();
	afx_msg void OnUpdateLaunchLogLst(CCmdUI* pCmdUI);
	afx_msg void OnUpdateLaunch(CCmdUI* pCmdUI);
	afx_msg void OnFindNext();
	afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
	afx_msg void OnFindPrev();
#ifdef _USE_FILETIMER
	afx_msg LRESULT OnTimerRefresh(WPARAM,LPARAM);
#endif
	afx_msg void OnPrjOpenfolder();
	afx_msg void OnUpdatePrjOpenfolder(CCmdUI *pCmdUI);
	afx_msg void OnSelChanged();
	afx_msg LRESULT OnPropViewLB(WPARAM wParam,LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRemoveprotection();
	afx_msg void OnWriteprotect();
	//afx_msg void OnUpdateWriteprotect(CCmdUI *pCmdUI);
};

#ifndef _DEBUG	// debug version in hierlvw.cpp
inline CPrjDoc* CPrjView::GetDocument()
   { return (CPrjDoc*) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
#endif
