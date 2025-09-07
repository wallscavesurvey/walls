#ifndef PRJHIER_H
#define PRJHIER_H

/////////////////////////////////////////////////////////////////////////////
// CPrjList window

#ifndef HIERLB_H
#include "hierlb.h"
#endif

#ifndef LINEDOC_H
#include "linedoc.h"
#endif

#undef _USE_BORDER

BOOL PropOK(void);
BOOL CloseItemProp(void);

#define GET_PLNODE(i) ((CPrjListNode *)GetItemData(i))
#define GET_PLVALIDNODE(i) (i>=0?(CPrjListNode *)GetItemData(i):NULL)
#define PLNODE(i) ((CPrjListNode *)(i))

enum { //m_uFlags --
	FLG_DEFINESEG = 1, FLG_FEETUNITS = 2, FLG_WESTWARD = (1 << 2),
	FLG_REFUSEUNCHK = (1 << 3), FLG_REFUSECHK = (1 << 4),
	FLG_DATEUNCHK = (1 << 5), FLG_DATECHK = (1 << 6),
	FLG_GRIDUNCHK = (1 << 7), FLG_GRIDCHK = (1 << 8),
	FLG_VTVERTUNCHK = (1 << 9), FLG_VTVERTCHK = (1 << 10),
	FLG_VTLENUNCHK = (1 << 11), FLG_VTLENCHK = (1 << 12),
	FLG_SURVEYNOT = (1 << 13), FLG_LAUNCHEDIT = (1 << 14), FLG_LAUNCHOPEN = (1 << 15),
	FLG_SVG_PROCESS = (1 << 19)
};

enum {
	FLG_REFMASK = (FLG_REFUSEUNCHK + FLG_REFUSECHK),
	FLG_DATEMASK = (FLG_DATEUNCHK + FLG_DATECHK),
	FLG_GRIDMASK = (FLG_GRIDCHK + FLG_GRIDUNCHK),
	FLG_VTVERTMASK = (FLG_VTVERTUNCHK + FLG_VTVERTCHK),
	FLG_VTLENMASK = (FLG_VTLENUNCHK + FLG_VTLENCHK),
	FLG_VIEWMASK = (7 << 16)
};

#define FLG_VIEWSHIFT 16

enum {
	FLG_MASK_GENERAL = FLG_DEFINESEG + FLG_FEETUNITS + FLG_SURVEYNOT + FLG_LAUNCHEDIT + FLG_LAUNCHOPEN,
	FLG_MASK_OPTIONS = FLG_VTVERTMASK + FLG_VTLENMASK + FLG_VIEWMASK + FLG_SVG_PROCESS,
	FLG_MASK_REFERENCE = FLG_REFMASK + FLG_DATEMASK + FLG_GRIDMASK,

	FLG_MASK_DEPBR = FLG_VTVERTMASK + FLG_VTLENMASK + FLG_DATEMASK,
	FLG_MASK_BR = FLG_GRIDMASK,
	FLG_MASK_DEP = FLG_DEFINESEG,
	FLG_MASK_UNCHK = FLG_REFUSEUNCHK + FLG_DATEUNCHK + FLG_GRIDUNCHK + FLG_VTVERTUNCHK + FLG_VTLENUNCHK
};

class CPrjListNodeConflict;

class CPrjListNode : public CHierListNode
{
	friend class CPrjList;
#if defined(_DEBUG)
	DECLARE_DYNAMIC(CPrjListNode) // this really only needed for debug
#endif

public:
	BOOL m_bLeaf;
	BOOL m_bDrawnReviewable;
	int m_nEditsPending; //No. of open survey files with changes pending
	DWORD m_dwFileChk;   //Checksum identifying the (unordered) set of dated branch surveys
	DWORD m_dwWorkChk;   //Checksum from workfile header (0 if file is absent)
	CString m_Title;     //Title string for branch or survey
	CString m_Name;      //Workfile and/or survey file base name
	CString m_Options;   //Command line to pass to WALL-SRV.DLL
	CString m_Path;
	LINENO m_nCheckLine;
	UINT m_uFlags;
	PRJREF *m_pRef;		 //Geographical reference;

public:
	CPrjListNode(BOOL bLeaf = FALSE,
		const char *szTitle = NULL, const char *szName = NULL, const char *szOptions = NULL) :
		m_bLeaf(bLeaf), m_Title(szTitle), m_Name(szName), m_Options(szOptions)
	{
		m_nEditsPending = 0;
		m_bDrawnReviewable = 0;
		m_nCheckLine = 0;
		m_dwFileChk = m_dwWorkChk = 0;
		m_uFlags = 0;
		m_pRef = NULL;
	}

	virtual ~CPrjListNode();
	virtual BOOL IsCompatible(CHierListNode *pNode);
	BOOL InheritedState(UINT mask);
	UINT InheritedView();

	BOOL IsType(UINT typ);

	//Used by CPrjView to refresh Compile/Review button only when necessary --
	CPrjListNode *m_pNodeReview;

	BOOL IsReviewable()
	{
		//0 - Empty, no edits pending
		//1 - non-empty compilable or edits pending
		//2 - Reviewable
		return m_dwFileChk ? ((m_dwFileChk == m_dwWorkChk && !m_nEditsPending) + 1) : (m_nEditsPending != 0);
	}

	BOOL IsDefineSeg() { return (m_uFlags&FLG_DEFINESEG) != 0; }
	BOOL IsFeetUnits() { return (m_uFlags&FLG_FEETUNITS) != 0; }
	//BOOL IsWestward() {return (m_uFlags&FLG_WESTWARD)!=0;}
	PRJREF * GetAssignedRef();
	PRJREF * GetDefinedRef();
	void RefZoneStr(CString &s);
	CPrjListNode * GetRefParent();
	char *GetPathPrefix(char *prefix);
	BOOL CPrjListNode::HasDependentPaths(CPrjDoc *pDoc);

	//Attaches node to tree. Rtns 0 or code<0 --
	virtual int  Attach(CHierListNode *pPrev, BOOL bIsSibling = FALSE);
	virtual void Detach(CHierListNode *pPrev = NULL); //Reverse of Attach().

	//To avoid excessive casting --
	CPrjListNode *Parent() { return (CPrjListNode *)m_pParent; }
	CPrjListNode *FirstChild() { return (CPrjListNode *)m_pFirstChild; }
	CPrjListNode *NextSibling() { return (CPrjListNode *)m_pNextSibling; }
	CPrjListNode *GetNextTreeCheck();
	CPrjListNode *GetFirstBranchCheck();
	BOOL GetLastCheckBefore(CPrjListNode **ppLast, CPrjListNode *pLimit);

	char *Title() { return (char *)(const char *)m_Title; }
	void SetTitle(const char *szTitle) { m_Title = szTitle; }
	char *Name() { return (char *)(const char *)m_Name; }
	char *Path() { return (char *)(const char *)m_Path; }
	void SetName(const char *szName) { m_Name = szName; }
	void SetPath(const char *szPath) { m_Path = szPath; }
	char *Options() { return (char *)(const char *)m_Options; }
	void SetOptions(const char *szOptions) { m_Options = szOptions; }
	BOOL IsLeaf() { return m_bLeaf; }
	BOOL IsChecked() { return m_nCheckLine != 0; }
	LINENO CheckLine() { return m_nCheckLine; }
	BOOL NameEmpty() { return m_Name.IsEmpty(); }
	int CompareLabels(CPrjListNode *pNode, BOOL bUseNames);
	BOOL IsOther() { return (m_uFlags&FLG_SURVEYNOT) != 0; }
	BOOL IsProcessingSVG() { return (m_uFlags&FLG_SVG_PROCESS) != 0; }
	BOOL PathEmpty() { return m_Path.IsEmpty(); }
	BOOL OptionsEmpty() { return m_Options.IsEmpty(); }
	void FixParentChecksums();
	BOOL SortBranch(BOOL bNameRev);
	BOOL SetItemFlags(int bFlag);
	BOOL SetChildFlags(int bSet, BOOL bChkAttach);
	CPrjListNode *FindFileNode(CPrjDoc *pDoc, const char *pathname, BOOL bFindOther = FALSE);
	void IncEditsPending(int nChg);
	void FixParentEditsAndSums(int dir); //Called by CPrjView::OnEditItem()
	void GetBranchPathName(CString &path);

	CPrjListNode * Duplicate(); //Used by OnDrop() when copying
public:
	//Helpers --
	char *BaseName(); //Uses a static buffer

	//Recursive helper fcns used by IsCompatible() --
	CPrjListNode *FindFirstSVG(CPrjListNode *pNode = NULL);
	CPrjListNode *FindName(const char FAR *pName);
	BOOL IsNodeCompatible(CPrjListNode *pNode);
	BOOL IsPathCompatible(CPrjListNode *pNode);
	/// <summary>Finds conflicting nodes among this node, pNode, and their descendants.
	/// There is only a conflict if the returned CPrjListNodeConflict.HasConflict() is true.
	/// </summary>
	CPrjListNodeConflict FindConflict(CPrjListNode *pNode);
	CPrjListNode *ChangedAncestor(CPrjListNode *pDestNode);
};

class CPrjList : public CHierListBox
{
	friend class CPrjView;
	// Construction --
public:
	CPrjList(BOOL ShowNames = FALSE, BOOL ShowTitles = TRUE);
	virtual ~CPrjList() {}
	static void PrjErrMsg(int seleIndex);

	// Public data members --
	BOOL m_bShowNames;
	BOOL m_bShowTitles;

private:
	//For enabling "drag from" operations --
	HCURSOR hCopy;
	HCURSOR hMove;
	int m_FocusItem;
	int m_iDropScroll;

public:
	//Overridden non-virtual of CListBox --
	int SetCurSel(int sel);

	// Operations not in parent class --
	int DeepRefresh(CPrjListNode *pParent, int iParentIndex);
	void OpenParentOf(CPrjListNode *pNode);
	int SelectNode(CPrjListNode *pNode);

	//Called from CPrjDoc::FixParentChecksums() and FixEditsPending() --
	void RefreshDependentItems(CPrjListNode *pNode);
	void RefreshNode(CPrjListNode *pNode); //Updates listbox item
	void RefreshBranch(CPrjListNode *pNode, BOOL bIgnoreFloating); //Updates *attached* listbox items

	int GetSelectedNode(CPrjListNode * &pNode)
	{
		int index = GetCurSel();
		pNode = GET_PLVALIDNODE(index);
		return index;
	}
	CPrjListNode *GetSelectedNode()
	{
		return GET_PLVALIDNODE(GetCurSel());
	}
	void DeleteSelectedNode();
	CPrjListNode *Root() { return (CPrjListNode *)m_pRoot; }
	CPrjView *GetView() { return (CPrjView *)GetParent(); }
	CPrjDoc *GetDocument() { return (CPrjDoc *)((CView *)GetParent())->GetDocument(); }

private:
	void ClearBranchCheckmarks(CPrjListNode *pNode);
	static int GetBranchNumChecked(CPrjListNode *pNode);
	//Used by CPrjView::OnEditFind() -- 
	void ClearAllCheckmarks() { ClearBranchCheckmarks(Root()); }
	int GetNumChecked() { return GetBranchNumChecked(Root()); }
	void DropScroll();

protected:
	void ReviseFocusRect(int nextItem);

	// Required override (pure virtual in CListBoxEx) --
	virtual void DrawItemEx(CListBoxExDrawStruct&);

	// Optional overrides --
	//Attach and display an unattached node. Returns node's index in listbox --
	virtual int InsertNode(CHierListNode* pNode, int PrevIndex, BOOL bIsPrevSibling = FALSE);
	virtual CHierListNode * RemoveNode(int index);  //Reverse of InsertNode().

	virtual void OpenNode(CHierListNode *node, int index);   //Called when double-clicked

	// Implement these to add drag and drop functionality --
	virtual void DroppedItemsOnWindowAt(CWnd*, const CPoint&, BOOL bCopy = FALSE);
	virtual BOOL CanDrop(CWnd* dropWnd, CPoint* dropWndClientPt, BOOL bTimer = FALSE);
	virtual HCURSOR GetCursor(eLbExCursor) const;

	// Generated message map functions
protected:
	//{{AFX_MSG(CPrjList)
	afx_msg LRESULT OnDrop(WPARAM, LPARAM);
	afx_msg LRESULT OnCanDrop(WPARAM, LPARAM);
	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CPrjListNodeConflict {
public: 
	CPrjListNodeConflict();
	CPrjListNodeConflict(CPrjListNode *srcNode, CPrjListNode *destNode);
	CPrjListNode *m_srcNode;
	CPrjListNode *m_destNode;
	BOOL HasConflict();
};

/////////////////////////////////////////////////////////////////////////////
#if defined(_DEBUG)
//#define ASSERT_PLNODE(n) ASSERT(n && n->IsKindOf(RUNTIME_CLASS(CPrjListNode)))

inline void ASSERT_PLNODE(CHierListNode* n)
{
	ASSERT(n);
	ASSERT(n != HLNODE(LB_ERR));
	ASSERT(n->IsKindOf(RUNTIME_CLASS(CPrjListNode)));
}

#else
inline void ASSERT_PLNODE(CPrjListNode* n) { n = n; }
#endif

#endif // PRJHIER_H
