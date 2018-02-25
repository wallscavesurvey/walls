// wedview.h : interface of the CWedView class
//
// Class CLineView implements a generic row-based scroll view.
// This derived class, CWedView, implements the details specific
// to the WEDIT application.
/////////////////////////////////////////////////////////////////////////////

#ifndef LINEDOC_H
#include "linedoc.h"
#endif

#include "tabedit.h"

class CWedView : public CLineView
{
   DECLARE_DYNCREATE(CWedView)
public:
   enum {SIZ_EDITWINPOS=8};
   enum {LHINT_GRID=2};
   enum {SIZ_TGTMSG=44,SIZ_TGTBUF=128};
   enum {INITIAL_WINDOW_COLS=80,MIN_WINDOW_LINES=20,MAX_WINDOW_LINES=80};

   CWedView();
   CLineDoc* GetDocument();

// Attributes
public:
    static CFindTabDlg *m_pSearchDlg; //Used by CMainFrame
    static CWedView *m_pFocusView;
    static char *m_pTgtBuf;
	static WORD m_wTgtBufFlags;
    static PRJFONT m_EditFont;
    static void ChangeFont();
    static void InitCharWidthTable();
	static BOOL AllocTgtBuf();
    static BOOL m_bFindOnly;
protected:
    static BOOL m_bSearchDown;
    static BOOL m_bReplace;
    static BOOL m_bReplaceAll;
	static LINENO m_nFindLine;
	static int m_nEditWinPos[SIZ_EDITWINPOS];

    static BOOL m_bSelectTrailingWS; //Used in OnLButtonDblClk()
    static CPen m_hPenGrid;
    static LPINT m_CharWidthTable;
    static int m_nMaxWidth;
    static int m_nAveWidth;
    static int m_nSpcWidth;
    static int m_nTabWidth;
    
	int m_nWinOffset;
    int m_nTabCols;
    int m_nTabStop;

private:
    static bool m_bClassRegistered;
    bool m_bCaretShown;
    UINT m_uFlags;
    
    BOOL TabLines() {return (m_uFlags&E_TABLINES)!=0;}
    BOOL GridLines() {return (m_uFlags&E_GRIDLINES)!=0;}
    BOOL EnterTabs() {return (m_uFlags&E_ENTERTABS)!=0;}
    BOOL AutoSeq() {return (m_uFlags&E_AUTOSEQ)!=0;}
    
    int NextTabPos(LPBYTE& pLine,int& len,int x);
    void ProcessCtrlN();
    void SetNextTabPosition(BOOL bPrev);
	int  CaretWidth() {return CMainFrame::m_bIns?2:8;}
    void DoFindReplaceDialog(BOOL bFindOnly);
    BOOL InitTgtBuf(BOOL bUseSelection);
	void DoRegExFindReplace();
    void DoFind(BOOL bSearchDown);
	BOOL IncFindLine();
	CPrjListNode * GetFileNode();
	BOOL GetFirstTwoWords(char *buf);
	BOOL LocateVector();
	int RegExReplaceWith(int *ovector,int numsubstr);

public:
    static void Initialize();
    static void Terminate();

	//Called by CMainFrame::OnFindReplace() --
	void OnSearch();
	static void OnTerminateSearch() {StatusClear();}
	void AdjustCaret(bool bShow);


protected:
// Document update functions --
	BOOL IsLocateEnabled();
	BOOL IsReadOnly() {return GetDocument()->m_bReadOnly;}
	BOOL IsSrvFile() {return GetDocument()->IsSrvFile();}
	const CString & GetPathName() {return GetDocument()->GetPathName();}

    void InsertEOL()
    {
      UpdateDoc(CLineDoc::UPD_INSERT,NULL,0);
    }

    void InsertChar(UINT nChar)
    {
      UpdateDoc(CMainFrame::m_bIns?CLineDoc::UPD_INSERT:CLineDoc::UPD_REPLACE,
        (LPSTR)&nChar,1);
    }

    void DeleteChar()
    {
      UpdateDoc(CLineDoc::UPD_DELETE,NULL,1);
    }
    
    void EraseLn(BOOL bEOL);
    void EraseWord();
    void AutoSequence();
	void OpenProjectFile();
	static CPrjListNode *FindName(CPrjDoc **ppDoc,LPCSTR name);

// Required overrides of CLineView --
    virtual CFont * SelectTextFont(CDC* pDC);
    virtual void GetLineMetrics(int& nLineMinWidth,int& nLineMaxWidth,
                 int& nLineHeight,int& nHdrHeight,int& nMargin);
    virtual void OnDrawLine(CDC* pDC,LINENO nLine,CRect rect);
    virtual void OnDrawHdr(CDC* pDC);
	virtual void PositionCaret();
	virtual void DisplayCaret(bool bOn);
    virtual void GetNearestPosition(LINENO nLine,int& nChr,long& xPos);
    virtual int GetPosition(LINENO nLine,int nChr,long& xPos);
	virtual BOOL ProcessVKey(UINT nChar,BOOL bShift,BOOL bControl);

protected:
// Overrides of CLineView
	virtual void OnUpdate(CView* pSender,LPARAM lHint=0L,CObject* pHint=NULL);

//Standard MFC Overrides --
	void OnInitialUpdate();
	BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual ~CWedView();

// Generated message map functions
protected:
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditFind();
	afx_msg void OnEditReplace();
	afx_msg void OnUpdateEditReplace(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnGotoline();
	afx_msg void OnFindNext();
	afx_msg void OnFindPrev();
	afx_msg void OnEditAutoSeq();
	afx_msg void OnUpdateEditAutoSeq(CCmdUI* pCmdUI);
	afx_msg void OnEditReverse();
	afx_msg void OnUpdateEditReverse(CCmdUI* pCmdUI);
	afx_msg void OnZipBackup();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEditProperties();
	afx_msg void OnLocateInGeom();
	//afx_msg void OnUpdateLocateInGeom(CCmdUI* pCmdUI);
	afx_msg void OnLocateOnMap();
	afx_msg void OnViewSegment();
	afx_msg void OnVectorInfo();
	afx_msg void OnUpdateFindNext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSel(CCmdUI* pCCmdUI);
	afx_msg void OnUpdateCol(CCmdUI* pCCmdUI);
	afx_msg void OnUpdateLine(CCmdUI* pCCmdUI);
	afx_msg void OnUpdatePfxNam(CCmdUI* pCmdUI);
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg void OnPopupWriteprotect();
	afx_msg void OnUpdatePopupWriteprotect(CCmdUI *pCmdUI);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#ifndef _DEBUG  // debug version in wedview.cpp
inline CLineDoc* CWedView::GetDocument() { return (CLineDoc*) m_pDocument; }
#endif
