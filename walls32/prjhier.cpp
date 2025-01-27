// PrjHier.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "dialogs.h"
#include "copyfile.h"
#include "dragdrop.h"
#include "itemprop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern BOOL hPropHook;
extern CItemProp *pItemProp;
extern CPrjView *pPropView;

BOOL PropOK(void)
{
	BOOL bRet = TRUE;
	if (hPropHook) {
		CItemPage *page = pItemProp->GetActivePage();
		TRACE0("In PropOK\n");
		if (!page->UpdateData(TRUE) || !page->ValidateData()) {
			page->PostMessage(WM_RETFOCUS);
			bRet = FALSE;
		}
		else bRet = hPropHook;
		hPropHook = TRUE;
	}
	return bRet;
}

BOOL CloseItemProp(void)
{
	if (!hPropHook) return TRUE;
	TRACE0("In CloseItemProp\n");
	if (PropOK() == TRUE) {
		pItemProp->EndItemProp(IDOK);
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CPrjListNode

#ifdef _DEBUG
IMPLEMENT_DYNAMIC(CPrjListNode, CHierListNode)
#endif

//Destructor will destroy CStrings --
CPrjListNode::~CPrjListNode()
{
	if (m_pRef) delete m_pRef;
}

char * CPrjListNode::BaseName()
{
	//Return the filename component in a static buffer --
	static char name[LEN_BASENAME + 2]; //ASSUME ZERO INIT
#ifdef _DEBUG
	if (IsOther()) {
		ASSERT(FALSE);
	}
#endif
	strncpy(name, m_Name, LEN_BASENAME);
	*trx_Stpext(name) = 0;  //Trim extension
	return name;
}

int CPrjListNode::CompareLabels(CPrjListNode *pNode, BOOL bNameRev)
{
	//Is this node less (-1), equal (0), or greater (1) than pMode?
	int iCompare;

	if (IsLeaf() != pNode->IsLeaf()) iCompare = IsLeaf() ? 1 : -1;
	else {
		if (bNameRev & 1) {
			if (NameEmpty())	iCompare = pNode->NameEmpty() ? CompareNumericLabels(m_Title, pNode->m_Title) : -1;
			else iCompare = CompareNumericLabels(m_Name, pNode->m_Name);
		}
		else iCompare = CompareNumericLabels(m_Title, pNode->m_Title);
		if (bNameRev & 2) iCompare = -iCompare;
	}
	return iCompare;
}

BOOL CPrjListNode::SortBranch(BOOL bNameRev)
{
	//This is an open node --
	CPrjListNode *pChild = FirstChild();

	ASSERT(pChild && IsOpen());

	if (!pChild) return FALSE;

	//Children present --
	CPrjListNode *pNextSib, *pLastSib, *pHighChild, *pHighPrev, *pChildPrev = NULL;
	BOOL bChanged = FALSE;

	while (pChild) {
		//At this point, pChild is the first of a set of siblings from
		//which the highest, pHighChild is to be determined.
		//pChildPrev, if not NULL, is the sibling above it, which is already
		//correctly located.
		pHighPrev = pChildPrev;
		pHighChild = pLastSib = pChild;

		//Scan siblings starting with pNextSib, comparing them with
		//pHighChild. After the scan, the highest will be pHighChild, which is
		//the next sibling of pHighPrev --
		while (pNextSib = pLastSib->NextSibling()) {
			if (pNextSib->CompareLabels(pHighChild, bNameRev) < 0) {
				pHighPrev = pLastSib;
				pHighChild = pNextSib;
			}
			pLastSib = pNextSib;
		}

		if (pHighChild != pChild) {
			bChanged = TRUE;
			ASSERT(pHighPrev && pHighChild == pHighPrev->NextSibling());
			pHighPrev->m_pNextSibling = pHighChild->NextSibling();
			pHighChild->m_pNextSibling = pChild;
			if (pChildPrev) pChildPrev->m_pNextSibling = pHighChild;
			else m_pFirstChild = pHighChild;
		}

		if (pHighChild->IsOpen() && pHighChild->SortBranch(bNameRev)) bChanged = TRUE;
		pChildPrev = pHighChild;
		pChild = pHighChild->NextSibling();
	}
	return bChanged;
}

void CPrjListNode::GetBranchPathName(CString &path)
{
	if (!m_pParent) return;
	Parent()->GetBranchPathName(path);
	if (NameEmpty()) return;
	if (!path.IsEmpty()) path += " / ";
	path += IsLeaf() ? Title() : Name();
}

CPrjListNode * CPrjListNode::FindName(const char FAR *pName)
{
	//Does this node and its children have unique base names with 
	//respect to base name pName? If so, return NULL.
	//Otherwise return the first node with a matching name.

	if (!*pName || IsOther()) return NULL;
	if (!m_bLeaf) {
		if (!_stricmp(m_Name, pName)) return this;

		CPrjListNode *pNext = FirstChild();
		while (pNext) {
			CPrjListNode *p = pNext->FindName(pName);
			if (p) return p;
			pNext = pNext->NextSibling();
		}
		return NULL;
	}

	return _stricmp(pName, BaseName()) ? NULL : this;
}

CPrjListNode * CPrjListNode::FindFirstSVG(CPrjListNode *pFirst /*=NULL*/)
{
	if (IsLeaf()) {
		if ((this == pFirst) || !IsOther() || IsFloating()) return NULL;
		char *p = trx_Stpext(Name());
		ASSERT(!IsFloating());
		return (!_stricmp(p, ".svg") || !_stricmp(p, ".svgz")) ? this : NULL;
	}
	CPrjListNode *pNode, *pNext = FirstChild();
	while (pNext) {
		if (!pNext->IsFloating() && (pNode = pNext->FindFirstSVG(pFirst))) {
			ASSERT(!pNode->IsFloating());
			return pNode;
		}
		pNext = pNext->NextSibling();
	}
	return NULL;
}


char * CPrjListNode::GetPathPrefix(char *prefix)
{
	if (IsAbsolutePath(m_Path)) {
		strcpy(prefix, m_Path);
	}
	else {
		if (Parent()) Parent()->GetPathPrefix(prefix);
		if (!m_Path.IsEmpty()) strcat(prefix, m_Path);
	}
	ASSERT(*prefix);
	if (prefix[strlen(prefix) - 1] != '\\') strcat(prefix, "\\");
	return prefix;
}

BOOL CPrjListNode::HasDependentPaths(CPrjDoc *pDoc)
{
	if (IsLeaf()) return !NameEmpty() && (0 == _access(pDoc->SurveyPath(this), 0));

	CPrjListNode *pNext = FirstChild();
	while (pNext) {
		if (!IsAbsolutePath(pNext->m_Path) && pNext->HasDependentPaths(pDoc)) return TRUE;
		pNext = pNext->NextSibling();
	}
	return FALSE;
}

BOOL CPrjListNode::IsPathCompatible(CPrjListNode *pNode)
{
	//Does this node and its children have unique base names with 
	//respect to the base name contained in pNode?

	char fname[LEN_BASENAME + 2];
	return !FindName(strcpy(fname, pNode->BaseName()));
}

BOOL CPrjListNode::IsNodeCompatible(CPrjListNode *pNode)
{
	//Does this node and its children have unique base names with respect
	//to pNode and its children?

	if (!m_bLeaf) {
		if (pNode->FindName(m_Name)) return FALSE;
		CPrjListNode *pNext = FirstChild();
		while (pNext) {
			if (!pNext->IsNodeCompatible(pNode)) return FALSE;
			pNext = pNext->NextSibling();
		}
		return TRUE;
	}
	if (IsOther()) return TRUE;
	return pNode->IsPathCompatible(this);
}

BOOL CPrjListNode::IsCompatible(CHierListNode *pNode)
{
	//Is this node and its children compatible with the tree containing pNode?
	//That is, can we attach to pNode? Currently. our only requirement is that
	//the base names all be unique.

	if (IsOther()) return TRUE;
	while (pNode->Parent()) pNode = pNode->Parent();
	return IsNodeCompatible(PLNODE(pNode));
}

BOOL CPrjListNode::InheritedState(UINT mask)
{
	UINT bits = (m_uFlags&mask);
	if (bits || !Parent()) return bits > (bits^mask);
	return Parent()->InheritedState(mask);
}

UINT CPrjListNode::InheritedView()
{
	if (Parent() && !(m_uFlags&FLG_VIEWMASK)) return Parent()->InheritedView();
	UINT v = ((m_uFlags&FLG_VIEWMASK) >> FLG_VIEWSHIFT);
	return Parent() ? (v - 1) : v;
}

CPrjListNode *CPrjListNode::FindFileNode(CPrjDoc *pDoc, const char *pathname, BOOL bFindOther)
{
	//Return the first branch node whose SurveyPath() matches pathname.

	if (m_bLeaf) {
		return ((!bFindOther&&IsOther()) || _stricmp(pDoc->SurveyPath(this), pathname)) ? NULL : this;
	}

	CPrjListNode *pNode = FirstChild();
	while (pNode) {
		CPrjListNode *pFound = pNode->FindFileNode(pDoc, pathname, bFindOther);
		if (pFound) return pFound;
		pNode = pNode->NextSibling();
	}
	return NULL;
}

int CPrjListNode::Attach(CHierListNode *pPrev, BOOL bIsSibling)
{
	int code = CHierListNode::Attach(pPrev, bIsSibling);
	if (!code) FixParentEditsAndSums(1);
	return code;
}

void CPrjListNode::Detach(CHierListNode *pNode)
{
	FixParentEditsAndSums(-1);
	CHierListNode::Detach(pNode);
}

CPrjListNode * CPrjListNode::Duplicate()
{
	CPrjListNode *pNewParent;

	TRY{
	  pNewParent = new CPrjListNode(m_bLeaf,m_Title,m_Name,m_Options);
	  pNewParent->m_Path = m_Path;
	  if (m_pRef) {
		  pNewParent->m_pRef = new PRJREF;
		  *pNewParent->m_pRef = *m_pRef;
	  }
	}
		CATCH(CMemoryException, e) {
		return NULL;
	}
	END_CATCH

		pNewParent->m_bOpen = m_bOpen;
	pNewParent->m_bFloating = m_bFloating;
	pNewParent->m_uFlags = m_uFlags;

	//Assume effective survey pathnames and workfiles are invalidated!
	pNewParent->m_dwFileChk = pNewParent->m_dwWorkChk = 0;
	pNewParent->m_nEditsPending = 0;

	pNewParent->m_bDrawnReviewable = 0; //Will cause invalidation when attached.

	//We can ignore m_Level and m_FirstChild flags since they will be
	//set during an attachment --

	if (!m_bLeaf && FirstChild()) {
		CPrjListNode *pNewChild = pNewParent;
		CPrjListNode *pChild = FirstChild();
		while (pChild) {
			pNewChild->SetNextSibling(pChild->Duplicate());
			pNewChild = pNewChild->NextSibling();
			if (pChild == FirstChild()) {
				pNewParent->SetFirstChild(pNewChild);
				pNewParent->SetNextSibling(NULL);
			}
			if (!pNewChild) {
				pNewParent->DeleteChildren();
				delete pNewParent;
				return NULL;
			}
			pNewChild->SetParent(pNewParent);
			pChild = pChild->NextSibling();
		}
	}

	return pNewParent;
}

void CPrjListNode::FixParentChecksums()
{
	DWORD chk = m_dwFileChk;
	if (chk) {
		CPrjListNode *pNode = this;
		while (pNode->Parent()) {
			if (pNode->m_bFloating) break;
			pNode = pNode->Parent();
			pNode->m_dwFileChk ^= chk;
		}
	}
}

void CPrjListNode::IncEditsPending(int nChg)
{
	CPrjListNode *pNode = this;
	m_nEditsPending += nChg;
	while (pNode->Parent() && !pNode->m_bFloating) {
		pNode = pNode->Parent();
		pNode->m_nEditsPending += nChg;
	}
}

void CPrjListNode::FixParentEditsAndSums(int dir)
{
	if (!m_bFloating) {
		if (m_dwFileChk) FixParentChecksums();
		if (m_nEditsPending)
			Parent()->IncEditsPending(dir*m_nEditsPending);
	}
}

CPrjListNode * CPrjListNode::ChangedAncestor(CPrjListNode *pDestNode)
{
	CPrjListNode *pNode = this;
	CPrjListNode *pDest;
	BOOL bDestVisible;
	BOOL bThisVisible = TRUE;

	if (pDestNode->m_bLeaf) pDestNode = pDestNode->Parent();

	while (TRUE) {
		bDestVisible = TRUE;
		for (pDest = pDestNode; pDest && pDest != pNode->Parent(); pDest = pDest->Parent()) {
			if (pDest->m_bFloating
#ifdef _GRAYABLE
				|| pDest->m_bGrayed
#endif
				) bDestVisible = FALSE;
		}
		if (pDest) return (bThisVisible != bDestVisible) ? pNode : NULL;
		VERIFY(pNode = pNode->Parent());
		if (pNode->m_bFloating) return NULL;
#ifdef _GRAYABLE
		if (pNode->m_bGrayed) bThisVisible = FALSE;
#endif
	}
}

BOOL CPrjListNode::SetItemFlags(int bFlag)
{
	int iMask = (bFlag < 0) ? ~bFlag : bFlag;
	if (((m_uFlags&iMask) == 0) != (bFlag < 0)) {
		m_uFlags ^= iMask; //assumes single flag
		return TRUE;
	}
	return FALSE;
}

BOOL CPrjListNode::SetChildFlags(int bFlag, BOOL bChkAttach)
{
	//Revise flags of all attached children --
	BOOL bChanged = FALSE;
	CPrjListNode *pNext = FirstChild();

	while (pNext) {
		if (!bChkAttach || !pNext->IsFloating()) {
			if (pNext->SetItemFlags(bFlag)) bChanged = TRUE;
			if (!pNext->IsLeaf() && pNext->SetChildFlags(bFlag, bChkAttach)) bChanged = TRUE;
		}
		pNext = pNext->NextSibling();
	}
	return bChanged;
}

CPrjListNode * CPrjListNode::GetFirstBranchCheck()
{
	if (IsLeaf()) {
		return IsChecked() ? this : NULL;
	}
	CPrjListNode *pChild = FirstChild();
	while (pChild) {
		CPrjListNode *pNode = pChild->GetFirstBranchCheck();
		if (pNode) return pNode;
		pChild = pChild->NextSibling();
	}
	return NULL;
}

CPrjListNode * CPrjListNode::GetNextTreeCheck()
{
	//Find next checked leaf beginning with next sibling;
	CPrjListNode *pSibling = NextSibling();
	while (pSibling) {
		CPrjListNode *pNode = pSibling->GetFirstBranchCheck();
		if (pNode) return pNode;
		pSibling = pSibling->NextSibling();
	}
	return Parent() ? Parent()->GetNextTreeCheck() : NULL;
}

BOOL CPrjListNode::GetLastCheckBefore(CPrjListNode **ppLast, CPrjListNode *pLimit)
{
	if (pLimit == this) return FALSE;
	if (IsLeaf()) {
		if (IsChecked()) *ppLast = this;
		return TRUE;
	}
	CPrjListNode *pChild = FirstChild();
	while (pChild) {
		if (!pChild->GetLastCheckBefore(ppLast, pLimit)) return FALSE;
		pChild = pChild->NextSibling();
	}
	return TRUE;
}

CPrjListNode * CPrjListNode::GetRefParent()
{
	CPrjListNode *pNode = this;
	while (!(pNode->m_uFlags&FLG_REFMASK)) {
		if (!pNode->Parent()) {
			//ASSERT(FALSE);
			break;
		}
		pNode = pNode->Parent();
	}
	return pNode;
}

PRJREF * CPrjListNode::GetDefinedRef()
{
	CPrjListNode *pNode = this;
	while (pNode && !pNode->m_pRef) pNode = pNode->Parent();
	if (!pNode || !pNode->m_pRef) return &CMainFrame::m_reflast;
	ASSERT(pNode->m_uFlags&FLG_REFUSECHK);
	return pNode->m_pRef;
}

PRJREF * CPrjListNode::GetAssignedRef()
{
	CPrjListNode *pNode = this;
	while (pNode && !(pNode->m_uFlags&FLG_REFMASK)) pNode = pNode->Parent();
	if (!pNode || !pNode->m_pRef) {
		ASSERT(!pNode || (pNode->m_uFlags&FLG_REFUSEUNCHK));
		return NULL;
	}
	ASSERT(pNode->m_uFlags&FLG_REFUSECHK);
	return pNode->m_pRef;
}

void CPrjListNode::RefZoneStr(CString &s)
{
	PRJREF *pRef = GetAssignedRef();
	if (pRef) {
		int zone = pRef->zone;
		if (abs(zone) == 61) s = "UPS";
		else s.Format("%02d%c", abs(zone), (zone < 0) ? 'S' : 'N');
	}
}

BOOL CPrjListNode::IsType(UINT typ)
{
	LPCSTR pExt = trx_Stpext(Name());
	return strlen(pExt) > 3 && !memicmp(pExt + 1, CPrjDoc::WorkTyp[typ], 3);
}

/////////////////////////////////////////////////////////////////////////////
// CPrjList

BEGIN_MESSAGE_MAP(CPrjList, CHierListBox)
	//{{AFX_MSG_MAP(CPrjList)
	ON_REGISTERED_MESSAGE(WM_DROP, OnDrop)
	ON_REGISTERED_MESSAGE(WM_CANDROP, OnCanDrop)
	ON_WM_ERASEBKGND()
	ON_WM_SYSCHAR()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPrjList::CPrjList(BOOL ShowNames, BOOL ShowTitles) : CHierListBox(TRUE, 18, 4)
{
	EnableDragDrop(TRUE);
	hCopy = AfxGetApp()->LoadCursor(IDC_COPY);
	hMove = AfxGetApp()->LoadCursor(IDC_MOVE);
	m_bShowNames = ShowNames;
	m_bShowTitles = ShowTitles;
	m_FocusItem = -1;
}

/////////////////////////////////////////////////////////////////////////////
// CPrjList message handlers

void CPrjList::DrawItemEx(CListBoxExDrawStruct& ds)
{
	// All that needs to be done is to BitBlt the correct bitmap from the
	// resources at the start of ds.m_Rect and then use TextOut to draw
	// the text. The background colour and indents are all taken care of.

	CPrjListNode* pNode = (CPrjListNode*)ds.m_ItemData;
	ASSERT_PLNODE(pNode);

	// naughty cast - should BitBlt take a const CDC* ?
	CDC* pBmpDC = (CDC*)&ds.m_pResources->DcBitMap();
	int bmh = ds.m_pResources->BitmapHeight();
	int bmw = ds.m_pResources->BitmapWidth();

	// Select bitmap from resource. Depends on selected, open, and leaf status.

	// Compute x,y (bm_w,bm_h) coordinates of portion of resource to be copied --
	int bm_h = (ds.m_Sel) ? 0 : bmh;
	int bm_w;
	BOOL bRev = pNode->IsReviewable();

	//Avoids unnecessary refreshes (see RefreshNode()) --
	pNode->m_bDrawnReviewable = bRev + 1;

	//Bitmap columns 0-11 contain the following symbols --
	//	0  - Yellow closed folder (empty project)
	//	1  - Yellow open folder   (nonempty project - always open)
	//  2  - Blue closed folder   {not used)
	//  3  - Blue open folder     {reviewable project)
	//  4  - Gray closed book     (empty book - no children)
	//  5  - White page           (empty survey - no file)
	//  6  - Red closed book      (compilable closed book)
	//  7  - Red open book        (compilable open book)
	//  8  - White lined page     (compilable survey)
	//  9  - Blue closed book     (reviewable closed book)
	// 10  - Blue open book       (reviewable open book)
	// 11  - Blue lined page      (reviewable survey)
	// 12  - Gray lined page      (item of type Other)

	if (pNode->m_bLeaf) {
		if (pNode->IsOther()) bm_w = 12;
		else bm_w = (bRev > 1) ? 11 : (bRev ? 8 : 5);
	}
	else if (!pNode->Parent()) {
		bm_w = pNode->m_bOpen ? 1 : 0;
		if (bRev > 1) bm_w += 2;
	}
	else if (!pNode->FirstChild()) bm_w = 4;
	else {
		bm_w = (bRev > 1) ? 9 : 6;
		if (pNode->m_bOpen) bm_w++;
	}
	bm_w *= bmw;

	ds.m_pDC->BitBlt(ds.m_Rect.left + 1, ds.m_Rect.top, bmw, bmh, pBmpDC, bm_w, bm_h, SRCCOPY);

	if (!pNode->Parent()) return;

	if (pNode->IsChecked()) {
		ds.m_pDC->BitBlt(ds.m_Rect.left - bmw - 4, ds.m_Rect.top, 12, bmh,
			pBmpDC, 2 * bmw, 0, SRCCOPY);
	}

	//Unique names are required, but need not be shown --
	BOOL bShowTitle = m_bShowTitles && pNode->Title() != NULL;

	if (bShowTitle || m_bShowNames) {
		char buf[128];

		CFont *p = (CFont *)ds.m_pDC->SelectObject(
			pNode->m_bLeaf ? &CPrjView::m_SurveyFont.cf : &CPrjView::m_BookFont.cf);
		bmw += ds.m_Rect.left + 3;

		if (bShowTitle && m_bShowNames)
			_snprintf(buf, sizeof(buf), "%s: %s", pNode->Name(), pNode->Title());
		else
			_snprintf(buf, sizeof(buf), "%s", bShowTitle ? pNode->Title() : pNode->Name());

		ds.m_pDC->TextOut(bmw, ds.m_Rect.top, buf);

		ds.m_pDC->SelectObject(p);
	}
}

void CPrjList::OpenParentOf(CPrjListNode *pNode)
{
	CPrjListNode *pParent = pNode->Parent();
	if (!pParent || pParent->IsOpen()) return;
	OpenParentOf(pParent);
	OpenToLevel(pParent->GetIndex(), 1);
}

int CPrjList::SelectNode(CPrjListNode *pNode)
{
	int index;

	OpenParentOf(pNode);
	VERIFY((index = pNode->GetIndex()) >= 0);
	VERIFY(SetCurSel(index) != LB_ERR);
	return index;
}

void CPrjList::RefreshNode(CPrjListNode *pNode)
{
	if (pNode->m_bDrawnReviewable != pNode->IsReviewable() + 1) InvalidateNode(pNode);
}

void CPrjList::RefreshDependentItems(CPrjListNode *pNode)
{
	//We need to invalidate the listbox lines corresponding to pNode and
	//its chain of dependent parents --
	do {
		RefreshNode(pNode);
	} while (!pNode->m_bFloating && (pNode = pNode->Parent()) != NULL);
}

void CPrjList::RefreshBranch(CPrjListNode *pNode, BOOL bIgnoreFloating)
{
	RefreshNode(pNode);
	pNode = pNode->FirstChild();
	while (pNode) {
		if (bIgnoreFloating || !pNode->m_bFloating) RefreshBranch(pNode, bIgnoreFloating);
		pNode = pNode->NextSibling();
	}
}

int CPrjList::InsertNode(CHierListNode* pNode, int PrevIndex, BOOL bPrevIsSibling /*=FALSE*/)
{
	int e = CHierListBox::InsertNode(pNode, PrevIndex, bPrevIsSibling);
	if (!e) {
		if (pNode->Parent() && !pNode->m_bFloating) RefreshDependentItems(PLNODE(pNode)->Parent());
	}
	return e;
}

void CPrjList::OpenNode(CHierListNode* pNode, int index)
{
	if (!PLNODE(pNode)->m_bLeaf) {
		BOOL someClosed = FALSE;
		if (pNode->Parent()) {
			if (pNode->FirstChild()) {
				CHierListBox::OpenNode(pNode, index);
				someClosed = TRUE;
			}
		}
		else {
			CHierListNode *pNext = pNode->FirstChild();
			int lvl = 16;
			while (pNext) {
				if (pNext->m_bOpen) {
					lvl = 1;
					someClosed = TRUE;
					break;
				}
				if (!someClosed && pNext->FirstChild()) someClosed = TRUE;
				pNext = pNext->NextSibling();
			}
			if (someClosed) {
				SetRedraw(FALSE);
				OpenToLevel(index, lvl);
				SetRedraw(TRUE);
				Invalidate(FALSE);
			}
		}
		if (someClosed) GetDocument()->MarkProject();
	}
	else {
		//This is a leaf (survey file node).

		GetView()->OpenLeaf(PLNODE(pNode));
	}

}

void CPrjList::DeleteSelectedNode()
{
	// single selection so --
	int sel = GetCurSel();
	if (sel > 0) {
		if (hPropHook) pItemProp->EndItemProp(IDCANCEL);
		//Would it be worth trying a selective invalidation of rows?
		//(Something about listboxes makes that infeasible.)
		SetRedraw(FALSE);
		DeleteNode(sel);
		if (sel >= GetCount()) sel--;
		SetCurSel(sel);
		SetRedraw(TRUE);
		Invalidate();
	}
}

void CPrjList::ReviseFocusRect(int nextItem)
{
	if (nextItem != m_FocusItem) {
		CClientDC dc(this);
		CRect itemRect;
		if (m_FocusItem >= 0) {
			GetItemRect(m_FocusItem, &itemRect);
			dc.DrawFocusRect(&itemRect);
		}
		if (nextItem >= 0) {
			GetItemRect(nextItem, &itemRect);
			dc.DrawFocusRect(&itemRect);
		}
		m_FocusItem = nextItem;
	}
}

//Drag-drop client fcns --
void CPrjList::DropScroll()
{
	int topitem = GetTopIndex();
	int focusitem;
	ReviseFocusRect(-1);
	if (m_iDropScroll < 0) {
		if (topitem > 0) SetTopIndex(--topitem);
		focusitem = topitem;
	}
	else {
		focusitem = GetBotIndex();
		if (focusitem < GetCount() - 1) {
			SetTopIndex(topitem + 1);
			focusitem++;
		}
	}
	ReviseFocusRect(focusitem);
}

LRESULT CPrjList::OnCanDrop(WPARAM bScroll, LPARAM point)
{
	//WPARAM contains bTimer -- true if timer invoked.
	//LPARAM points to the cursor position in this window's client area,
	//  or NULL if tracking is either being cancelled, or the cursor
	//  is being moved out of the client area --

	if (!point) {
		if (bScroll) {
			if (m_iDropScroll) DropScroll();
		}
		else ReviseFocusRect(-1);
	}
	else {
		CPoint cp(*(CPoint FAR *)(LPVOID)point);
		int item = ItemFromPoint(cp);
		m_iDropScroll = 0;

		if (m_bTracking1 && item == m_MouseDownItem && m_pDropWnd == this) {
			ReviseFocusRect(-1);
			return LRESULT(TRUE);
		}

		ReviseFocusRect(item);
		if (item != LB_ERR) {
			//Determine m_iDropScroll --
			if (item > 0 && item <= GetTopIndex()) m_iDropScroll = -1;
			else if (item >= GetBotIndex()) m_iDropScroll = 1;
		}
	}

	return LRESULT(TRUE);
}

void CPrjList::PrjErrMsg(int seleIndex)
{
	UINT msg = IDS_PRJ_ERR_COPYMEM; //selindex=-3 (HN_ERR_Memory)
	switch (seleIndex) {
	case LB_ERRSPACE: msg = IDS_PRJ_ERR_LBSPACE; break;
	case HN_ERR_Compare: msg = IDS_PRJ_ERR_COMPARE; break;
	case HN_ERR_MaxLevels: msg = IDS_PRJ_ERR_MAXLEVELS; break;
	}
	AfxMessageBox(msg);
}

CHierListNode *CPrjList::RemoveNode(int index)
{
	CPrjListNode *pOldParent = GET_PLNODE(index)->Parent();

	CHierListNode *pNode = CHierListBox::RemoveNode(index);

	if (pOldParent) RefreshDependentItems(pOldParent);
	return pNode;
}

static BOOL CheckBranchData(CPrjDoc *srcDoc, CPrjListNode *srcNode, CPrjDoc *dstDoc, CPrjListNode *dstNode, BOOL bRemove)
{
	char pathbuf[_MAX_PATH];

	if (dstNode->IsLeaf()) dstNode = dstNode->Parent();
	*trx_Stpnam(strcpy(pathbuf, srcDoc->SurveyPath(srcNode))) = 0; //old path prefix

	BOOL bOldAbs = IsAbsolutePath(srcNode->m_Path);

	dstDoc->GetRelativePathName(pathbuf, dstNode);
	if (!IsAbsolutePath(pathbuf)) {
		//No need to move survey files since all are either linked to or
		//in directories relative to the new parent's path.
		//For now, if bOldAbs, transform to relative without prompting --
		if (*pathbuf) pathbuf[strlen(pathbuf) - 1] = 0;
		srcNode->m_Path = pathbuf;
		return TRUE;
	}

	if (bOldAbs) return TRUE; //File paths stay the same

	//Path of moved or copied node will be different. Will any survey file paths actually change?
	if (srcNode->HasDependentPaths(srcDoc)) {
		int e = 0;
		if (moveBranch_id == -1) {
			CMoveDlg dlg(!bRemove);
			e = dlg.DoModal();
			if (e == IDCANCEL) return FALSE;
		}
		else if (moveBranch_id&MV_FILES) e = IDOK;
		if (e == IDOK) {
			AfxGetApp()->BeginWaitCursor();
			strcpy(pathbuf, dstDoc->SurveyPath(dstNode)); //new path prefix
			if (!srcNode->PathEmpty()) {
				strcat(pathbuf, srcNode->m_Path);
				strcat(pathbuf, "\\");
			}
			//Be sure to close all open files before moving them!
			CLineDoc::SaveAllOpenFiles();
			bMoveFile = bRemove;
			srcDoc->CopyBranchSurveys(srcNode, pathbuf, 1);
			AfxGetApp()->EndWaitCursor();
		}
		else {
			//Use an absolute link --
			if (pathbuf[1]) pathbuf[strlen(pathbuf) - 1] = 0;
			srcNode->m_Path = pathbuf;
		}
	}
	return TRUE;
}

LRESULT CPrjList::OnDrop(WPARAM NumItems, LPARAM dragData)
{
	static BOOL bPromptMove = FALSE;

	//Currently, we support only drags from CPrjList windows (NumItems==0) --
	if (NumItems != 0) return TRUE;

	CDragStruct* pDrag = (CDragStruct*)dragData;
	ASSERT(pDrag->IsKindOf(RUNTIME_CLASS(CDragStruct)));

	int dropIndex = ItemFromPoint(pDrag->m_ClientPt);  //CListBoxEx function
	int srcIndex = (int)pDrag->m_DragType;

	if (dropIndex < 0) return TRUE;

	CPrjListNode *srcNode = PLNODE(pDrag->m_DragItem);
	CPrjListNode *dstNode = GET_PLNODE(dropIndex);
	CPrjListNode *pNode;

	if (dstNode == PLNODE(LB_ERR)) return TRUE;
	ASSERT_PLNODE(dstNode);

	//If the destination node is a leaf, make the dragged item a sibling
	//of this leaf --
	BOOL isSibling = dstNode->m_bLeaf;

	CPrjDoc *dstDoc = GetDocument();
	int selIndex = 0;

	moveBranch_SameList = pDrag->m_pServerWnd == this;
	moveBranch_id = -1;

	bool isOther = srcNode->IsOther();
	bool bMenu = (pDrag->m_bDragCopy == 2);

	if (bMenu) {
		CMenu menu;
		if (menu.LoadMenu(IDR_MOV_CONTEXT))
		{
			CMenu* pPopup = menu.GetSubMenu(0);
			ASSERT(pPopup != NULL);
			ClientToScreen(&pDrag->m_ClientPt);
			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
				pDrag->m_ClientPt.x, pDrag->m_ClientPt.y,//-100,
				AfxGetMainWnd()); // use main window for cmds
			Pause();
			if (moveBranch_id == -1) return TRUE;
			pDrag->m_bDragCopy = (moveBranch_id&MV_COPY) != 0;
		}
		else return TRUE;
	}

	if (moveBranch_SameList) {
		//Disallow copies (not moves) within the same list --
		if (pDrag->m_bDragCopy) {
			AfxMessageBox(IDS_PRJ_ERR_COPYTOSELF);
			return TRUE;
		}

		//For now, assume we are not maintaining a sort order for inserted items.
		//(See CHierListNode::Attach()). We can make this optional later.
		//We therefore ignore moves that would have no effect, these being
		//moves of srcNode 1) to itself, 2) to an immediately previous leaf
		//sibling, or 3) to its parent, if it is the first child --

		if (srcIndex == dropIndex ||
			(isSibling && srcNode == dstNode->NextSibling()) ||
			(!isSibling && srcNode == dstNode->FirstChild())) return TRUE;

		//The source node cannot be a parent of the destination node --
		for (pNode = dstNode; pNode; pNode = pNode->Parent())
			if (pNode == srcNode) {
				AfxMessageBox(IDS_PRJ_ERR_DROPTOSELF);
				return TRUE;
			}

		//Valid move requested -- Will the survey file paths change?
		if (!CheckBranchData(dstDoc, srcNode, dstDoc, dstNode, TRUE)) return TRUE;

		//Will the visibility within the first common ancestor change?
		//If so, purge the dependent workfiles --
		if (!srcNode->m_bFloating &&
#ifdef _GRAYABLE
			!srcNode->m_bGrayed &&
#endif
			(dstNode = srcNode->ChangedAncestor(dstNode)))
			GetView()->PurgeDependentFiles(dstNode);

		//We must first remove the source node before attempting to reinsert it --
		pNode = srcNode->Parent(); //Save old parent
		RemoveNode(srcIndex);
		//Adjust destination listbox index for the removed branch --
		if (dropIndex > srcIndex) dropIndex -= srcNode->GetWidth();

		//Insert srcNode beneath the item at dropIndex, either as its first child
		//or as its next sibling (if isSibling==TRUE) --
		if ((selIndex = InsertNode(srcNode, dropIndex, isSibling)) < 0) {
			//This shouldn't happen. Rather than reinserting it, let's simply delete it --
			srcNode->DeleteChildren();
			delete srcNode;
		}
		else if (pNode != srcNode->Parent())
			GetDocument()->PurgeAllWorkFiles(srcNode, FALSE, TRUE);
	}
	else {
		//We are moving or copying from a different project.

		if (!bMenu && !pDrag->m_bDragCopy) {
			CApplyDlg dlg(IDD_MOVE_CONFIRM, &bPromptMove, "Moving Branches");
			if (dlg.DoModal() != IDOK) return TRUE;
		}

		//If the source node is the root, we want to copy each of its child nodes
		//in reverse order --
		if (!srcIndex) {
			if (dstNode->IsRoot() && !dstNode->FirstChild()) {
				//We are copying to an empty project. Let's grab compile options and reference --
				dstNode->m_Options = srcNode->m_Options;
				dstNode->m_uFlags = srcNode->m_uFlags;
				if (srcNode->m_pRef) {
					if (!dstNode->m_pRef) dstNode->m_pRef = new PRJREF;
					*dstNode->m_pRef = *srcNode->m_pRef;
				}
			}
			srcNode = (CPrjListNode *)srcNode->LastChild();
			if (!srcNode) return TRUE;
			NumItems++;
		}

		CPrjDoc *srcDoc = ((CPrjList *)pDrag->m_pServerWnd)->GetDocument();
		ASSERT(srcDoc != dstDoc && srcDoc->IsKindOf(RUNTIME_CLASS(CPrjDoc)));

		do {
			//Check if names are unique before any files are copied!
			if (!srcNode->IsNodeCompatible(dstDoc->m_pRoot)) {
				selIndex = HN_ERR_Compare;
				break;
			}

			CPrjListNode *pTemp = srcNode->Duplicate();
			if (pTemp == NULL) {
				selIndex = HN_ERR_Memory;
				break;
			}

			pTemp->m_pParent = srcNode->Parent();
			if (!CheckBranchData(srcDoc, pTemp, dstDoc, dstNode, !pDrag->m_bDragCopy) ||
				(selIndex = InsertNode(pTemp, dropIndex, isSibling)) < 0) {
				pTemp->DeleteChildren();
				delete pTemp;
				if (!NumItems && !selIndex) return TRUE;
				break;
			}

			srcIndex = srcNode->GetIndex(); //Index of last node copied

			srcNode = (CPrjListNode *)srcNode->PrevSibling();

			if (!pDrag->m_bDragCopy) ((CPrjList *)pDrag->m_pServerWnd)->DeleteNode(srcIndex);
		} while (NumItems && srcNode);

		if (!pDrag->m_bDragCopy) {
			CPrjList *pSrcList = (CPrjList *)pDrag->m_pServerWnd;
			((CHierListBox *)pSrcList)->SetCurSel(srcIndex - 1);
			CPrjDoc *srcDoc = pSrcList->GetDocument();
			//Recompute survey and workfile checksums for all tree nodes --
			srcDoc->BranchFileChk(srcDoc->m_pRoot);
			srcDoc->BranchWorkChk(srcDoc->m_pRoot);
			srcDoc->BranchEditsPending(srcDoc->m_pRoot);
			srcDoc->SaveProject();
		}
		((CFrameWnd *)GetView()->GetParent())->ActivateFrame();
	} //!moveBranch_SameList

	if (selIndex < 0) {
		PrjErrMsg(selIndex);
		selIndex = 0;
	}

	if (hPropHook) {
		ASSERT(pItemProp->m_pNode == PLNODE(pDrag->m_DragItem));
		pItemProp->m_pNode = NULL; //prevent update of deleted node
	}

	if (!isOther) {
		//Recompute survey and workfile checksums for all tree nodes --
		dstDoc->BranchFileChk(dstDoc->m_pRoot);
		dstDoc->BranchWorkChk(dstDoc->m_pRoot);
		dstDoc->BranchEditsPending(dstDoc->m_pRoot);
	}

	SetCurSel(selIndex);
	GetDocument()->SaveProject();

	return TRUE;
}

// Drag-drop server fcns --
HCURSOR CPrjList::GetCursor(eLbExCursor e) const
{
	switch (e)
	{
	case LbExCopyMultiple:
	case LbExCopySingle:
		return hCopy;
	case LbExMoveSingle:
	case LbExMoveMultiple:
		return hMove;
	}
	return HCURSOR(NULL);
}

BOOL CPrjList::CanDrop(CWnd* DropWnd, CPoint *p, BOOL bCopy)
{
	//Called from CListBoxEx::OnMouseMove() --
	return (BOOL)DropWnd->SendMessage(WM_CANDROP, (WPARAM)bCopy, (LPARAM)p);
}

void CPrjList::DroppedItemsOnWindowAt(CWnd* DropWnd, const CPoint& DropWndPt, BOOL bCopy)
{
	int SrcIndex = GetCurSel();
	CPrjListNode *SrcNode = GET_PLNODE(SrcIndex);
	ASSERT_PLNODE(SrcNode);

	CDragStruct drag(this,                     //m_pServerWnd
		DropWndPt,                //m_ClientPt
		bCopy,                    //m_bDragCopy (0,1,or 2)
		(WORD)SrcIndex,           //m_DragType
		(DWORD)(LPVOID)SrcNode);  //m_DragItem

//ReleaseCapture(); why?

// Use lparam to pass drop info, wparm for number of items dropped.
// In this case we pass NumItems=0 to indicate we are dragging from
// a CPrjList listbox --
	DropWnd->SendMessage(WM_DROP, 0, (LPARAM)(LPVOID)&drag);
}

#if 0
//BOOL CPrjList::OnEraseBkgnd(CDC* pDC)
{
	//return CHierListBox::OnEraseBkgnd(pDC); //white bkgrnd
	//return CListBoxEx::OnEraseBkgnd(pDC); //white bkgrnd
	//return CListBox::OnEraseBkgnd(pDC); //white bkgrnd

	/* not required here
	RECT rect;
	GetWindowRect(&rect); //Screen coords
	rect.right-=rect.left;
	rect.bottom-=rect.top;
	rect.left=rect.top=0;
	CBrush *cb=(CBrush *)CBrush::FromHandle((HBRUSH)::GetStockObject(LTGRAY_BRUSH));
	pDC->FillRect(&rect, cb);
	return 1;
	*/
#ifdef _USE_BORDER
	RECT rect;
	CWindowDC wDC(this);
	GetWindowRect(&rect); //Screen coords 
	rect.right -= rect.left;
	rect.bottom -= rect.top;
	rect.left = rect.top = 0;
	CBrush *cb = (CBrush *)CBrush::FromHandle((HBRUSH)::GetStockObject(WHITE_BRUSH));
	wDC.FrameRect(&rect, cb);
	//CPen pen(PS_INSIDEFRAME,1,RGB_BLACK);
	wDC.SelectStockObject(BLACK_PEN);
	wDC.MoveTo(0, rect.bottom);
	wDC.LineTo(0, 0);
	wDC.LineTo(rect.right, 0);
	//  These do not seem to work like FrameRect() above --
	//	CPen pen2(PS_INSIDEFRAME,1,RGB_WHITE);
	//	wDC.SelectObject(&pen2);
	//	wDC.MoveTo(rect.right,0);
	//	wDC.LineTo(rect.right,rect.bottom);
	//	wDC.LineTo(0,rect.bottom);
#endif
	//return bRet;
}
#endif

//Used by CPrjView::OnEditFind() --
void CPrjList::ClearBranchCheckmarks(CPrjListNode *pNode)
{
	if (pNode->IsChecked()) {
		pNode->m_nCheckLine = 0;
		InvalidateNode(pNode);
	}
	pNode = pNode->FirstChild();
	while (pNode) {
		ClearBranchCheckmarks(pNode);
		pNode = pNode->NextSibling();
	}
}

int CPrjList::GetBranchNumChecked(CPrjListNode *pNode)
{
	if (pNode->IsLeaf()) return pNode->IsChecked();

	int count = 0;
	pNode = pNode->FirstChild();
	while (pNode) {
		count += GetBranchNumChecked(pNode);
		pNode = pNode->NextSibling();
	}
	return count;
}

void CPrjList::OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nFlags >= 0) {
		//Alt-Key is being pressed --
		CPrjView *pView = GetView();
		CPrjListNode *pNode = GetSelectedNode();
		ASSERT(pNode);
		if (!pNode) return;
		int c = toupper((int)nChar);
		switch (c) {
		case 'R':
		case 'C': if (!(nRepCnt = pNode->IsReviewable()) ||
			(nRepCnt > 1) != (c == 'R')) break;
			pView->OnCompileItem();
			return;
		case 'P':	pView->OnEditItem();
			return;
		case 'N': if (pNode->IsLeaf()) break;
			pView->OnNewItem();
			return;
		case 'D':
		case 'A': if (pNode->Level() < 1 || pNode->IsFloating() != (c == 'A')) break;
			pView->OnDetachBranch();
			return;
		}
	}
	CHierListBox::OnSysChar(nChar, nRepCnt, nFlags);
}

void CPrjList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (!(nChar == VK_UP || nChar == VK_DOWN) || PropOK() == TRUE) {
		CHierListBox::OnKeyDown(nChar, nRepCnt, nFlags);
		//if(hPropHook) {
			//pItemProp->PostMessage(WM_PROPVIEWLB,(WPARAM)GetDocument());
		//}
	}
}

int CPrjList::SetCurSel(int sel)
{
	int iret = CHierListBox::SetCurSel(sel);
	if (iret != LB_ERR && hPropHook) {
		pItemProp->PostMessage(WM_PROPVIEWLB, (WPARAM)GetDocument());
	}
	return iret;
}

void CPrjList::OnLButtonDown(UINT nFlags, CPoint point)
{
	BOOL bRet = PropOK();

	if (bRet != TRUE) {
		m_bRtTracking = FALSE;
		if (!bRet) return;
	}

	int index = ItemFromPoint(point);
	if (index >= 0) {
		SetCurSel(index); //****Changed Post to Send 11/13/02
		if (hPropHook) pItemProp->SendMessage(WM_PROPVIEWLB, (WPARAM)GetDocument());
	}

	if (m_bTracking) {
		CancelTracking();
		return;
	}

	if (bRet == TRUE && index >= 0 && !hPropHook) InitTracking(point);
}

void CPrjList::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (!hPropHook) m_bRtTracking = TRUE;
	OnLButtonDown(nFlags, point);
}

void CPrjList::OnRButtonUp(UINT nFlags, CPoint point)
{

	if (m_bTracking1) {
		CListBoxEx::OnRButtonUp(nFlags, point);
		return;
	}
	if (m_bTracking) CancelTracking();

	int index = ItemFromPoint(point);
	if (index >= 0) {
		if (PropOK() == TRUE) {
			SetCurSel(index);
			GetView()->UpdateButtons();

			CMenu menu;
			if (menu.LoadMenu(IDR_PRJ_CONTEXT))
			{
				CMenu* pPopup = menu.GetSubMenu(0);
				ASSERT(pPopup != NULL);
				ClientToScreen(&point);
				pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					point.x, point.y,//-100,
					AfxGetMainWnd()); // use main window for cmds
			}
		}
	}
}

int CPrjList::DeepRefresh(CPrjListNode *pParent, int iParentIndex)
{
	//Reinsert all visible descendant nodes in listbox --
	CPrjListNode *pNode = pParent->FirstChild();
	ASSERT(pNode);
	while (pNode) {
		DeleteString(++iParentIndex);
		VERIFY(InsertString(iParentIndex, (LPCSTR)pNode) >= 0);
		InvalidateItem(iParentIndex);
		if (pNode->IsOpen()) iParentIndex = DeepRefresh(pNode, iParentIndex);
		pNode = pNode->NextSibling();
	}
	return iParentIndex;
}
