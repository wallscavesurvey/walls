// SegHier.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "seghier.h"
#include "segview.h"
#include "plotview.h"
#include "compile.h"
#include "statdlg.h"
#include "wall_srv.h"
#include "dragdrop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif 

/////////////////////////////////////////////////////////////////////////////
// CSegListNode

#ifdef _DEBUG
IMPLEMENT_DYNAMIC(CSegListNode, CHierListNode)
#endif

styletyp CSegListNode::m_StyleBlack = { 0,0,0,0,0,0 };

static struct OFF_INFO {
	CDC *pDC;
	int indent;
	int offset;
} off_info;

CSegListNode::~CSegListNode()
{
	free(m_pSeg);
}

BOOL CSegListNode::IsParentOf(CSegListNode *pNode)
{
	while (pNode && !pNode->m_bFloating) {
		pNode = pNode->Parent();
		if (pNode == this) return TRUE;
	}
	return FALSE;
}

BOOL CSegListNode::IsLinesVisible()
{
	if (!IsVisible()) return FALSE;
	CSegListNode *pNode = FirstChild();
	while (pNode) {
		if (pNode->IsLinesVisible()) return TRUE;
		pNode = pNode->NextSibling();
	}
	return LineIdx() != CSegView::PS_NULL_IDX;
}

int CSegListNode::Compare(CHierListNode *pNode)
{
	//Virtual function used when attaching a node to an existing tree --
	LPSTR p1 = Title();
	LPSTR p2 = ((CSegListNode *)pNode)->Title();
	if (*p1 != *p2) return (*p1 > *p2) ? 1 : -1; //May have high bit set!
	int e = CompareNumericLabels(p1 + 1, p2 + 1);
	if (!e) e++; //Allow matches!
	return e;
}

void CSegListNode::SetBranchTitle()
{
	if (*Name() & 0x80) m_pSeg->m_pTitle = NULL;
	else {
		ASSERT(CPrjDoc::m_pReviewNode);
		CPrjListNode *pNode = CPrjDoc::m_pReviewNode->FindName(Name());
		m_pSeg->m_pTitle = pNode ? pNode->Title() : "";
	}
}

void CSegListNode::SetBranchVisibility()
{
	m_bVisible = Parent()->IsVisible() && !IsFloating();

	CSegListNode *pNode = FirstChild();
	while (pNode) {
		pNode->SetBranchVisibility();
		pNode = pNode->NextSibling();
	}
}

HPEN CSegListNode::PenCreate()
{
	int idx = OwnLineIdx();
	return ::CreatePen(CSegView::m_PenStyle[idx], (idx == CSegView::PS_HEAVY_IDX) ? 2 : 1,
		OwnLineColor());
}

void CSegListNode::SetStyleParents(CSegListNode *pStyleParent)
{
	if (IsUsingOwn()) pStyleParent = this;
	m_pStyleParent = pStyleParent;

	CSegListNode *pNode = FirstChild();
	while (pNode) {
		pNode->SetStyleParents(pStyleParent);
		pNode = pNode->NextSibling();
	}
}

void CSegListNode::InitSegStats(SEGSTATS *st)
{
	if (NumVecs()) {
		st->ttlength = Length();
		st->hzlength = HzLength();
		st->vtlength = VtLength();
		st->highpoint = m_pSeg->m_fHigh;
		st->lowpoint = m_pSeg->m_fLow;
		st->idhigh = m_pSeg->m_idHigh;
		st->idlow = m_pSeg->m_idLow;
		st->numvecs = NumVecs();
	}
}

void CSegListNode::AddSegStats(SEGSTATS *st, BOOL bFlt)
{
	if (NumVecs()) {
		if (bFlt) {
			st->exlength += Length();
		}
		else {
			st->ttlength += Length();
			st->hzlength += HzLength();
			st->vtlength += VtLength();
			st->numvecs += NumVecs();
			if (st->highpoint < m_pSeg->m_fHigh) {
				st->highpoint = m_pSeg->m_fHigh;
				st->idhigh = m_pSeg->m_idHigh;
			}
			if (st->lowpoint > m_pSeg->m_fLow) {
				st->lowpoint = m_pSeg->m_fLow;
				st->idlow = m_pSeg->m_idLow;
			}
		}
	}
	CSegListNode *pNode = FirstChild();
	while (pNode) {
		pNode->AddSegStats(st, bFlt || pNode->IsFloating());
		pNode = pNode->NextSibling();
	}
}

bool CSegListNode::HasVecs()
{
	if (m_pSeg) {
		if (NumVecs()) return true;
		CSegListNode *pNode = FirstChild();
		while (pNode) {
			if (pNode->HasVecs()) return true;
			pNode = pNode->NextSibling();
		}
	}
	return false;
}

void CSegListNode::AddStats(double &fLength, UINT &uNumVecs)
{
	CSegListNode *pNode = FirstChild();
	while (pNode) {
		if (!pNode->IsFloating()) pNode->AddStats(fLength, uNumVecs);
		pNode = pNode->NextSibling();
	}
	fLength += Length();
	uNumVecs += NumVecs();
}

void CSegListNode::FlagVisible(UINT mask)
{
	if (mask & 1) m_bVisible &= mask;
	else m_bVisible |= mask;

	CSegListNode *pNode = FirstChild();
	while (pNode) {
		if (!pNode->IsFloating()) pNode->FlagVisible(mask);
		pNode = pNode->NextSibling();
	}
}

void CSegListNode::InitHorzExtent()
{
	char *pTitle = TitlePtr();

	if (!pTitle) {
		pTitle = Name();
		::SelectObject(off_info.pDC->m_hDC, CPrjView::m_SurveyFont.cf.GetSafeHandle());
	}

	m_uExtent = off_info.pDC->GetTextExtent(pTitle, strlen(pTitle)).cx +
		off_info.indent + off_info.offset*Level();

	if (!TitlePtr()) ::SelectObject(off_info.pDC->m_hDC, CPrjView::m_BookFont.cf.GetSafeHandle());

	CSegListNode *pNode = FirstChild();
	while (pNode) {
		pNode->InitHorzExtent();
		pNode = pNode->NextSibling();
	}
}

int CSegListNode::GetHorzExtent(int extent)
{
	if (m_uExtent > extent) extent = m_uExtent;
	if (IsOpen()) {
		CSegListNode *pNode = FirstChild();
		while (pNode) {
			extent = pNode->GetHorzExtent(extent);
			pNode = pNode->NextSibling();
		}
	}
	return extent;
}

//////////////////////////////////////////////////////////////////////////////
// CSegList

BEGIN_MESSAGE_MAP(CSegList, CHierListBox)
	//{{AFX_MSG_MAP(CSegList)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CSegList::CSegList() : CHierListBox(TRUE, 18, 4)
{
}

/////////////////////////////////////////////////////////////////////////////
// CSegList message handlers

BOOL CSegList::RootDetached(void)
{
	CSegListNode *pNext = (CSegListNode *)m_pRoot->FirstChild();
	while (pNext && pNext->IsFloating()) pNext = pNext->NextSibling();
	return pNext == NULL;
}

void CSegList::DrawItemEx(CListBoxExDrawStruct& ds)
{
	// ds.m_Rect is the rectangle to be updated. Its left edge is just right
	// of the node's horizontal connecting line to its parent, which is already
	// visible and has length m_Offset/2 if the node is not floating. The + or -
	// flag still needs to be drawn left of the rectangle if the node has children.
	// The rectangle's background (highlighted or not) is already painted.

	CSegListNode* pNode = (CSegListNode*)ds.m_ItemData;
	ASSERT_SGNODE(pNode);
	CRect rect(&ds.m_Rect);

	//y-position of horizontal connecting line --
	int y = rect.top + ds.m_pResources->BitmapHeight() / 2;

	HDC hDC = ds.m_pDC->m_hDC;

	if (!pNode->Level()) {
		//Draw a black filled triangle to represent the root --
		POINT p[3];
		int x = rect.left + m_Offset / 2;
		p[0].x = x - 2;
		p[1].x = x + 3;
		p[2].x = x;
		p[0].y = p[1].y = y;
		p[2].y = y + 5;
		::SelectObject(hDC, ::GetStockObject(BLACK_BRUSH));
		::Polygon(hDC, p, 3);

		if (!RootDetached()) {
			//Draw start of vertical line to connected children, if there are any -- 
			::MoveToEx(hDC, x, y + 5, NULL);
			::LineTo(hDC, x, rect.bottom);
		}
		return;
	}

	//Width of round rectangle --
	int width = BulletWidth();

	//Flag open/closed state with plus or minus sign --
	if (pNode->Level() && pNode->FirstChild()) {
		int x = rect.left - m_Offset + 2;
		::MoveToEx(hDC, x - 2, y, NULL);
		::LineTo(hDC, x + 3, y);
		if (!pNode->IsOpen()) {
			::MoveToEx(hDC, x, y - 2, NULL);
			::LineTo(hDC, x, y + 3);
		}
	}

	if (pNode->IsUsingOwn()) {
		rect.right = rect.left + width;
		y = (rect.top + rect.bottom) / 2;
		rect.InflateRect(-1, -2);

		HBRUSH hBrushOld = (HBRUSH)::SelectObject(hDC, CSegView::m_hBrushBkg);

		//Draw rounded rectangle filled with the map's background color --
		::RoundRect(hDC, rect.left, rect.top, rect.right, rect.bottom, width / 3, width / 3);

		//Bisect rectangle with a line of the appropriate style and color --
		if (pNode->OwnStyle()->GradIdx()) {
			int w = (pNode->OwnStyle()->LineIdx() == 1) ? 1 : 0;
			CRect crect(rect.left + 1, y - w, rect.right - 1, y + 1);
			pSV->m_Gradient[pNode->OwnStyle()->GradIdx() - 1]->FillRect(ds.m_pDC, &crect);
		}
		else {
			HPEN hPenOld = (HPEN)::SelectObject(hDC, pNode->PenCreate());
			::SetBkMode(hDC, TRANSPARENT);
			::MoveToEx(hDC, rect.left + 1, y, NULL);
			::LineTo(hDC, rect.right - 1, y);
			::DeleteObject(::SelectObject(hDC, hPenOld));
		}

		::SelectObject(hDC, hBrushOld);

		rect.InflateRect(1, 2);
		rect.right = ds.m_Rect.right;
	}
	else {
		//Just draw a plain line segment instead of a rectangle --
		::MoveToEx(hDC, rect.left + (!pNode->IsFloating() ? 0 : (m_Offset / 2)), y, NULL);
		::LineTo(hDC, rect.left + (4 * width) / 5, y);
		if (pNode->IsOpen()) {
			//Draw start of vertical line to connected children, if there are any -- 
			CSegListNode *pNext = pNode->FirstChild();
			while (pNext && pNext->IsFloating()) pNext = pNext->NextSibling();
			if (pNext) {
				::MoveToEx(hDC, rect.left + m_Offset / 2, y, NULL);
				::LineTo(hDC, rect.left + m_Offset / 2, rect.bottom);
			}
		}
	}

	rect.left += width + 1; //Draw text fairly close to rounded rectangle

	HFONT hFontOld;
	char buf[SRV_SIZ_SEGNAMBUF];
	char *pTitle;

	COLORREF txtClr = RGB_BLACK;

	if (pNode->Level() <= 1) txtClr = RGB_DKRED;
	else if (!pNode->HasVecs()) {
		txtClr = RGB_GRAY;
	}

	if (pNode->TitlePtr()) {
		pTitle = pNode->TitlePtr();
		hFontOld = (HFONT)::SelectObject(hDC, CPrjView::m_BookFont.cf.GetSafeHandle());
	}
	else {
		strcpy(buf, pNode->Name());
		pTitle = buf;
		*buf &= 0x7F;
		hFontOld = (HFONT)::SelectObject(hDC, CPrjView::m_SurveyFont.cf.GetSafeHandle());
	}

	::SetTextColor(hDC, txtClr);
	::TextOut(hDC, rect.left, rect.top, pTitle, strlen(pTitle));
	::SelectObject(hDC, hFontOld);
	::SetTextColor(hDC, RGB_BLACK);
}

void CSegList::RefreshBranchOwners(CSegListNode *pNode)
{
	if (pNode->IsUsingOwn()) InvalidateNode(pNode);
	if (!pNode->IsOpen()) return;
	pNode = pNode->FirstChild();
	while (pNode) {
		RefreshBranchOwners(pNode);
		pNode = pNode->NextSibling();
	}
}

void CSegList::InitHorzExtent()
{
	CDC dc;
	dc.CreateCompatibleDC(NULL);
	HFONT hFontOld = (HFONT)::SelectObject(dc.m_hDC, CPrjView::m_BookFont.cf.GetSafeHandle());

	off_info.pDC = &dc;
	off_info.offset = m_Offset;
	off_info.indent = m_RootOffset + BulletWidth() + 1;
	//Call recursive node fcn..
	pSV->m_pSegments->InitHorzExtent();
	::SelectObject(dc.m_hDC, hFontOld);
}

void CSegList::CheckExtentChange()
{
	int index = pSV->m_pSegments->GetHorzExtent(0);
	if (index > m_uExtent || index < m_uExtent) {
		SetHorizontalExtent(m_uExtent = index);
		Invalidate();
	}
}

void CSegList::OpenNode(CHierListNode* pNode, int index)
{
	if (pNode->Level() > 0) {
		if (pNode->FirstChild()) {
			CHierListBox::OpenNode(pNode, index);
			//The node's open state affects the displayed statistics -- 
			pSV->UpdateStats((CSegListNode *)pNode);
			CheckExtentChange();
		}
	}
	else {
		CHierListNode *pNext = pNode->FirstChild();
		int lvl = 16;
		BOOL someClosed = FALSE;
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
			Invalidate();
		}
		CheckExtentChange();
	}
}

int CSegList::SetCurSel(int sel)
{
	int iRet = CHierListBox::SetCurSel(sel);
	if (iRet >= 0) {
		ASSERT(pSV && pSV->IsKindOf(RUNTIME_CLASS(CSegView)));
		pSV->UpdateControls(); //Buttons may need refreshing 
	}
	return iRet;
}

void CSegList::OpenParentNode(CSegListNode *pNode)
{
	CSegListNode *pParent = pNode->Parent();
	if (pParent && !pParent->IsOpen()) {
		OpenParentNode(pParent);
	}
	OpenNode(pNode, pNode->GetIndex());
}

void CSegList::SelectSegListNode(CSegListNode *pNode)
{
	CSegListNode *pParent = pNode->Parent();
	if (!pParent->IsOpen()) OpenParentNode(pNode);
	SetCurSel(pNode->GetIndex());
}

void CSegList::OnRButtonUp(UINT nFlags, CPoint point)
{
	int index = ItemFromPoint(point);

	if (index == LB_ERR) return;
	SetCurSel(index);

	CSegListNode *pNode = (CSegListNode *)GET_HLNODE(index);

	if (pNode >= pSV->m_pSegments) {
		CMenu menu;
		if (menu.LoadMenu(IDR_SEG_CONTEXT))
		{
			CMenu* pPopup = menu.GetSubMenu(0);
			ASSERT(pPopup != NULL);
			ClientToScreen(&point);
			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, GetParentFrame());
		}
	}
}

CPrjListNode *CSegList::GetSelectedPrjListNode()
{
	CSegListNode *pNode = GetSelectedNode();
	if (pNode->TitlePtr() && pNode >= pSV->m_pSegments) {
		return CPrjDoc::m_pReviewNode->FindName(pNode->Name());
	}
	return NULL;
}


void CSegList::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	int index = ItemFromPoint(point);

	if (index >= 3) {
		CHierListNode* n = GET_HLNODE(index);
		if (n->m_pFirstChild) {
			int off = 9 + (n->m_Level - 1) * 18;
			if (point.x <= off && point.x > off - 7) {
				OpenNode(n, index);
			}
		}
	}
	CHierListBox::OnLButtonDown(nFlags, point);
}

void CSegList::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int index = ItemFromPoint(point);

	if (index == 1) {
		//Invoke grid options --
		pPV->OnGridOptions();
	}
	else if (index == 2) {
		//Passage options --
		pSV->OnLrudStyle();
	}
	else if (index >= 0) {
		CHierListNode* n = GET_HLNODE(index);
		ASSERT_NODE(n);
		OpenNode(n, index);
	}
}

