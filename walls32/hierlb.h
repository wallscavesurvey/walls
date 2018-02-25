#if !defined(HIERLB_H)
#define HIERLB_H

#if !defined(LSTBOXEX_H)
#include "lstboxex.h"
#endif

#define HN_ERR_MaxLevels (LB_ERRSPACE-1)
#define HN_ERR_Compare (LB_ERRSPACE-2)
#define HN_ERR_Memory (LB_ERRSPACE-3)

#define GET_HLNODE(i) ((CHierListNode *)GetItemData(i))
#define HLNODE(i) ((CHierListNode *)(i))

class CHierListNode 
#if defined(_DEBUG)
: public CObject
#endif
{
friend class CHierListBox;
#if defined(_DEBUG)
	DECLARE_DYNAMIC(CHierListNode) // this really only needed for debug
#endif
public:
	int m_Level;                   // Levels 0..16 allowed
	BOOL  m_bOpen;                 // Are children displayed?
	BOOL  m_bFloating;             // True if no connecting line to parent is visible
#ifdef _GRAYABLE
	BOOL  m_bGrayed;			   // True if connecting line to parent is grayed
#endif
	CHierListNode* m_pParent;
	CHierListNode* m_pFirstChild;
	CHierListNode* m_pNextSibling;
	
private:
	WORD  m_LastChild; // allows 16 levels of lines - use a DWORD for 32
	
public:
	CHierListNode();
	virtual ~CHierListNode();

//Functions of use only to members of this class and friend CHierListBox --
private:
    BOOL GetLastChildBit() const { return m_LastChild & (0x01 << (m_Level-1)); }
    WORD GetLastChildMask() const { return m_LastChild; }
	//CHierListNode * Ancestor(int level); //Get ancestor at level>=0;
	
    void SetOpenFlags(int level); //Set subtree m_bOpen flags level deep, reset those deeper.
    CHierListNode *PrevLastChild(); //Get first "next-to-last child" sibling
    
public:
    BOOL FixChildLevels();        //Recomputes m_Level of children. Rtns FALSE if any level exceeds 16.
    //To avoid excessive casting in derived class implementation --
    void SetParent(CHierListNode *pParent) {m_pParent=pParent;}
    void SetFirstChild(CHierListNode *pChild) {m_pFirstChild=pChild;}
    void SetNextSibling(CHierListNode *pSibling) {m_pNextSibling=pSibling;}
    CHierListNode *Parent() {return m_pParent;}
    CHierListNode *FirstChild() {return m_pFirstChild;}
    CHierListNode *NextSibling() {return m_pNextSibling;}
	CHierListNode *LastChild();
	CHierListNode *PrevSibling();
    BOOL IsFloating() {return m_bFloating;}
    BOOL IsOpen() {return m_bOpen;}
    BOOL IsRoot() {return m_pParent==NULL;}
    int Level() {return m_Level;}

    int  GetWidth(BOOL bIgnoreFlags=FALSE);  //No. of lines used by subtree. Assume expanded if bIgnoreFlags==TRUE.
    int  GetMaxWidth();           //No. of lines used by subtree ignoring m_bopen flags.
    int  GetIndex();              //Get 0-based line number, or -1 if parent not open.
    void DeleteChildren();        //Delete all children and clear m_bOpen.
    int SetFloating(BOOL bFloating); //Hide line to parent.

    virtual int Compare(CHierListNode *pNode); //Default: Insert as first sibling (-1)
    virtual BOOL IsCompatible(CHierListNode *pNode) {return TRUE;}
	//Attaches node to tree. Rtns 0 or code<0;
	virtual int  Attach(CHierListNode *pPrev,BOOL bIsSibling=FALSE);
    virtual void Detach(CHierListNode *pPrev=NULL); //Reverse of Attach().
    
};

#if defined(_DEBUG)
  #define ASSERT_NODE(n) ASSERT(n && n->IsKindOf(RUNTIME_CLASS(CHierListNode)))
#else
 inline void ASSERT_NODE(CHierListNode* n) { n=n; }
#endif

/////////////////////////////////////////////////////////////////////////////
// CHierListBox declarations --

class CHierListBox : public CListBoxEx
{
public:
    CHierListNode *m_pRoot;
private:
	BOOL m_bLines; // do we want connecting lines
protected:
	WORD m_Offset;
	WORD m_RootOffset;
	
// Construction
public:
	CHierListBox(BOOL lines=TRUE,WORD offset=16,WORD rootOffset=0);
	virtual ~CHierListBox();
	BOOL m_bRefresh; //used only when refreshing ater open containing folder

private:
	void DrawLines(CHierListNode*, CDC*, LPCRECT );
	
protected:
	// inherited from CListBox
	virtual void DrawItem( LPDRAWITEMSTRUCT );

	void MakeChildrenVisible(int,int);
	int GetBotIndex();
	 
    int  HideChildren(int index); //Remove children from listbox, preserve m_bOpen flags.
    int  DisplayNode(CHierListNode *pNode,int index); //Display a hidden node.

// Operations
public:
    void InvalidateItem(int index)
    {
        CRect itemRect;
        GetItemRect(index,&itemRect);
        InvalidateRect(&itemRect,TRUE); //erase bknd
    }
    
	void InvalidateNode(CHierListNode *pNode)
	{
		int index=pNode->GetIndex();
		if(index>=0) InvalidateItem(index);
	}

    //Attach and display an unattached node. Returns node's index in listbox --
    virtual int InsertNode(CHierListNode* pNode,int PrevIndex,BOOL bIsPrevSibling=FALSE);
    virtual CHierListNode * RemoveNode(int index);  //Reverse of InsertNode().
    
    //Set m_bFloating status of selected node. Rtn node pointer if changed --
    CHierListNode * FloatSelection(BOOL floating);    
	CHierListNode * FloatNode(int index,BOOL floating); //Called by above fcn
    												
    BOOL IsSelectionFloating();
    
    void DeleteNode(int index);             //RemoveNode() and delete subtree nodes.
    int  OpenToLevel(int index,int level);  //Display to to relative level
    
    //Function called by OnLButtonDblClk() --                                                      
	virtual void OpenNode(CHierListNode *n,int index);      //OpenToLevel(index,0 or 1) with invalidate.
	
	void EnableLines(BOOL);
	BOOL LinesEnabled() const { return m_bLines; }
	
	// Generated message map functions
protected:
	//{{AFX_MSG(CHierListBox)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // HIERLB_H
