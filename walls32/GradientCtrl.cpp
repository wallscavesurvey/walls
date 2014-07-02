// GradientCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "GradientCtrl.h"
#include "GradientCtrlImpl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define GRADIENTCTRL_CLASSNAME _T("MFCGradientCtrl")

#define _DRAG_END

/////////////////////////////////////////////////////////////////////////////
// CGradientCtrl

CGradientCtrl::CGradientCtrl()
{
	RegisterWindowClass();
	m_Width = GCW_AUTO;
	m_Selected = -1;
	m_ToolTipFormat = _T("&SELPOS\n&SELVAL  Color: RGB( &HEXR,&HEXG,&HEXB )\nDouble-click to add a new peg");
	m_bShowToolTip = true;
	m_Orientation = Auto;
	m_Impl = new CGradientCtrlImpl(this);
}

CGradientCtrl::~CGradientCtrl()
{
	delete m_Impl;
}

BEGIN_MESSAGE_MAP(CGradientCtrl, CWnd)
	//{{AFX_MSG_MAP(CGradientCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_GETDLGCODE()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
END_MESSAGE_MAP()

BOOL CGradientCtrl::RegisterWindowClass()
{
    WNDCLASS wndcls;
    HINSTANCE hInst = AfxGetInstanceHandle();
	HBRUSH background;
	background = ::CreateSolidBrush(0x00FFFFFF);

    if (!(::GetClassInfo(hInst, GRADIENTCTRL_CLASSNAME, &wndcls)))
    {
        // otherwise we need to register a new class
        wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndcls.lpfnWndProc      = ::DefWindowProc;
        wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
        wndcls.hInstance        = hInst;
        wndcls.hIcon            = NULL;
        wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wndcls.hbrBackground    = background;
        wndcls.lpszMenuName     = NULL;
        wndcls.lpszClassName    = GRADIENTCTRL_CLASSNAME;

        if (!AfxRegisterClass(&wndcls))
        {
            AfxThrowResourceException();
            return FALSE;
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGradientCtrl message handlers

BOOL CGradientCtrl::Create(const RECT& rect, CWnd* pParentWnd, UINT nID) 
{
	return CWnd::CreateEx(WS_EX_CLIENTEDGE, GRADIENTCTRL_CLASSNAME, _T(""),
		0x50010000, rect, pParentWnd, nID);
}

void CGradientCtrl::OnPaint()
{
	CPaintDC dc(this);

	m_Impl->Draw(&dc);

	//----- Refresh -----//
	if(m_Selected > m_Gradient.GetPegCount())
		m_Selected = PEG_NONE;
	
	if(m_bShowToolTip) m_Impl->SynchronizeTooltips();
}

BOOL CGradientCtrl::OnEraseBkgnd(CDC* pDC) 
{
	CRgn pegrgn, clientrgn, gradientrgn, erasergn;
	CRect rect;
	BOOL leftdown = m_Impl->m_LeftDownSide;
	BOOL rightup = m_Impl->m_RightUpSide;

	//----- The area of the window -----//
	GetClientRect(&rect);
	clientrgn.CreateRectRgnIndirect(rect);

	rect=m_Impl->m_GradFrame;
	if(m_Impl->m_HistWidth) rect.top-=m_Impl->m_HistWidth+1;

	gradientrgn.CreateRectRgnIndirect(rect);
	
	//----- The area of the pegs -----//
	GetPegRgn(&pegrgn);

	//----- Create a region outside all of these -----//
	erasergn.CreateRectRgn(0, 0, 0, 0); // Create a dummy rgn
	erasergn.CombineRgn(&clientrgn, &pegrgn, RGN_DIFF);
	erasergn.CombineRgn(&erasergn, &gradientrgn, RGN_DIFF);
	
	//----- Fill the result in -----//
	//CBrush background(GetSysColor(COLOR_WINDOW));
	CBrush background(GetSysColor(COLOR_BTNFACE));
	pDC->FillRgn(&erasergn, &background);

	return 1;
}

void CGradientCtrl::PreSubclassWindow() 
{
	CWnd::PreSubclassWindow();
}

void CGradientCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CClientDC dc(this);
	CRect pegrect;
	int index=PEG_NONE;

	//----- Check if the user is selecting a marker peg -----//
	for(int i = 0; i < m_Gradient.GetPegCount(); i++)
	{
		if(m_Impl->m_LeftDownSide)
		{
			m_Impl->GetPegRect(i, &pegrect, false);
			if(pegrect.PtInRect(point))
			{
				index=i;
				break;
			}
		}

		if(m_Impl->m_RightUpSide)
		{
			m_Impl->GetPegRect(i, &pegrect, true);
			if(pegrect.PtInRect(point))
			{
				index=i;
				break;
			}
		}
	}

	//----- Check if the user is trying to select the first or last one -----//

	if(index==PEG_NONE && m_Impl->m_RightUpSide)
	{
		m_Impl->GetPegRect(PEG_START, &pegrect, true);
		if(pegrect.PtInRect(point))	index=PEG_START;
		else {
			m_Impl->GetPegRect(PEG_END, &pegrect, true);
			if(pegrect.PtInRect(point))	index=PEG_END;
		}
	}

	if(index==PEG_NONE && m_Impl->m_LeftDownSide)
	{
		m_Impl->GetPegRect(PEG_START, &pegrect, false);
		if(pegrect.PtInRect(point))	index=PEG_START;
		else {
			m_Impl->GetPegRect(PEG_END, &pegrect, false);
			if(pegrect.PtInRect(point))	index=PEG_END;
		}
	}

	if(index!=PEG_NONE) {
		//----- Just in case the user starts dragging a peg -----//

#ifdef _DRAG_END
		SetCapture();
		m_MouseDown = point;
		if(index!=m_Selected) {
			if(GetFocus()==this) {
				m_Impl->DrawSelPeg(&dc,m_Selected);
				m_Impl->DrawSelPeg(&dc,index);
			}
			m_Selected=index;
			SendSelectedMessage(GC_SELCHANGE);
		}
#else
		if(index>=0) { //bug fixed (dmck)
			SetCapture();
			m_MouseDown = point;
		}
		else if(index!=m_Selected) {
			if(GetFocus()==this) {
				m_Impl->DrawSelPeg(&dc,m_Selected);
				m_Impl->DrawSelPeg(&dc,index);
			}
		}

		if(index!=m_Selected) {
			m_Selected=index;
			SendSelectedMessage(GC_SELCHANGE);
		}
#endif
	}

	SetFocus();

	CWnd::OnLButtonDown(nFlags, point);
}

void CGradientCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if((nFlags&MK_LBUTTON) && GetCapture()==this && point!=m_MouseDown)
	{
		bool vertical = m_Impl->IsVertical();
		int selpegpos;
		double pos;
		
		//----- Checks to see if the mouse is far enough away to "drop out" -----//
		if(vertical)
		{
			if(max(point.x - m_MouseDown.x, m_MouseDown.x - point.x) <= 200)
				selpegpos = point.y;
			else
				selpegpos = m_MouseDown.y;
		}
		else
		{
			if(max(point.y - m_MouseDown.y, m_MouseDown.y - point.y) <= 200)
				selpegpos = point.x;
			else
				selpegpos = m_MouseDown.x;
		}

#ifdef _DRAG_END
		if(m_Selected==PEG_START) {
			if(!vertical) {
				if(selpegpos<=m_MouseDown.x) return;
			}
			else if(selpegpos<=m_MouseDown.y) return;
		}
		else if(m_Selected==PEG_END) {
			if(!vertical) {
				if(selpegpos>=m_MouseDown.x) return;
			}
			else if(selpegpos>=m_MouseDown.y) return;
		}

		if(m_Selected==PEG_START || m_Selected==PEG_END) {
			if((pos=m_Impl->PosFromPoint(selpegpos))<=0.0 || pos>=1.0) return;
			SetSelIndex(GetGradient().AddPeg(GetGradient().GetPeg(m_Selected).color,pos));
			SendSelectedMessage(GC_SELCHANGE);
			return;
		}
#endif
		//----- Prepare -----//
		CRgn oldrgn, newrgn, erasergn;
		CBrush brush;
		CClientDC dc(this);

		//----- A brush for thebackground -----//
		brush.CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

		//----- Get the orginal erase area -----//
		GetPegRgn(&oldrgn);
		
		//----- Continue -----//
		CPeg peg = m_Gradient.GetPeg(m_Selected);
		pos = m_Impl->PosFromPoint(selpegpos);
		//"The id of the selection may change"//
		m_Selected = m_Gradient.SetPeg(m_Selected, peg.color, pos);
		
		//----- Draw the peg -----//
		m_Impl->DrawPegs(&dc);
		m_Impl->DrawEndPegs(&dc);
		m_Impl->DrawSelPeg(&dc, m_Selected);
		m_Impl->DrawGradient(&dc);
		GetPegRgn(&newrgn);
			
		erasergn.CreateRectRgn(0,0,0,0); //Dummy rgn
		erasergn.CombineRgn(&oldrgn, &newrgn, RGN_DIFF);
		
		dc.FillRgn(&erasergn, &brush);

		//----- Free up stuff -----//
		oldrgn.DeleteObject();
		newrgn.DeleteObject();
		erasergn.DeleteObject();

		//----- Send parent messages -----//
		SendSelectedMessage(GC_PEGMOVE);
	}
	else if(m_bShowToolTip) {
		CRect rect(m_Impl->m_GradFrame);
		//Histogram rectangle --
		rect.bottom=rect.top;
		rect.top-=m_Impl->m_HistWidth;
		if(rect.PtInRect(point)) {
			CString tiptext;
			if(m_Impl->GetNearbyValueStr(point.x-rect.left,tiptext)) {
				if(!m_Impl->m_wndToolTip) {
					CPoint pt(point.x+4,rect.top);
					ClientToScreen(&pt);
					m_Impl->ShowTooltip(pt,tiptext);
				}
				else m_Impl->SetTooltipText(tiptext);
			}
			else if(m_Impl->m_wndToolTip) m_Impl->DestroyTooltip();
		}
		else if(m_Impl->m_wndToolTip) m_Impl->DestroyTooltip();
	}
	
	CWnd::OnMouseMove(nFlags, point);
}

void CGradientCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if(GetCapture()==this)
	{
		ReleaseCapture();
		CClientDC dc(this);

		/* Why ?? -- Also buggy!
		CRect rect;
		GetClientRect(&rect);
		if(m_Impl->IsVertical()) rect.left=m_Impl->GetDrawWidth();
		else rect.bottom -= m_Impl->GetDrawWidth();
		dc.FillSolidRect(rect,0x00FFFFFF); //Erase the old ones
		*/

		//----- Update the pegs -----//
		m_Impl->DrawPegs(&dc);
		m_Impl->DrawEndPegs(&dc); //Draw the new ones
		m_Impl->DrawSelPeg(&dc, m_Selected);

		m_Impl->DestroyTooltip();
		m_Impl->SynchronizeTooltips();

		SendSelectedMessage(GC_PEGMOVED);
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CGradientCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch(nChar)
	{
	case VK_TAB:
		if(GetKeyState(VK_SHIFT)<0) ((CDialog *)GetParent())->PrevDlgCtrl();
		else ((CDialog *)GetParent())->NextDlgCtrl();
		break;

	case VK_LEFT:
	case VK_RIGHT:
		if(m_Selected > -4 && m_Selected < m_Gradient.GetPegCount())
		{
			CClientDC dc(this);
			m_Impl->DrawSelPeg(&dc, m_Selected);

			switch(m_Selected)
			{
				case PEG_NONE:
					m_Selected = PEG_START;
					break;

				case PEG_START:
					if(nChar==VK_RIGHT && m_Gradient.GetPegCount()>0) m_Selected=0;
					else m_Selected = PEG_END;
					break;

				case PEG_END:
					if(nChar==VK_LEFT && m_Gradient.GetPegCount()>0) m_Selected=m_Gradient.GetPegCount()-1;
					else m_Selected = PEG_START;
					break;

				default:
					if(nChar==VK_RIGHT) {
						if(++m_Selected >= m_Gradient.GetPegCount()) m_Selected=PEG_END;
					}
					else {
						if(--m_Selected < 0) m_Selected=PEG_START;
					}
			}
			m_Impl->DrawSelPeg(&dc, m_Selected);
			SendSelectedMessage(GC_SELCHANGE);
		}
		break;

	case VK_UP:
		if(m_Selected >= 0 && m_Selected < m_Gradient.GetPegCount())
		{
			CPeg selpeg = GetSelPeg();
			selpeg.position -= 0.025;
			//Make sure that the position does not stray bellow zero
			selpeg.position = (selpeg.position <= 1.0) ?  selpeg.position : 1.0;
			MoveSelected(selpeg.position, true);
			
			//Send parent messages
			SendBasicNotification(GC_PEGMOVED, selpeg, m_Selected);
		}
		break;

	case VK_DOWN:
		if(m_Selected >= 0 && m_Selected < m_Gradient.GetPegCount())
		{
			CPeg selpeg = GetSelPeg();
			selpeg.position += 0.025;
			//Make sure that the position does not stray above 1
			selpeg.position = (selpeg.position <= 1.0) ?  selpeg.position : 1.0;
			MoveSelected(selpeg.position, true);
			
			//Send parent messages
			SendBasicNotification(GC_PEGMOVED, selpeg, m_Selected);
		}

		break;

	case VK_HOME:
		if(m_Selected >= 0 && m_Selected < m_Gradient.GetPegCount())
		{
			CPeg selpeg = GetSelPeg();
			selpeg.position = 0.0;
			MoveSelected(selpeg.position, true);
			//Send parent messages
			SendBasicNotification(GC_PEGMOVED, selpeg, m_Selected);
		}

		break;

	case VK_END:
		if(m_Selected >= 0 && m_Selected < m_Gradient.GetPegCount())
		{
			CPeg selpeg = GetSelPeg();
			selpeg.position = 1.0;
			MoveSelected(selpeg.position, true);
			//Send parent messages
			SendBasicNotification(GC_PEGMOVED, selpeg, m_Selected);
		}

		break;

	case VK_DELETE:
	case VK_BACK:
		if(m_Gradient.GetPegCount()) SendSelectedMessage(GC_PEGREMOVED);
		break;

	case VK_PRIOR: // Shift the peg up a big jump
		if(m_Selected >= 0 && m_Selected < m_Gradient.GetPegCount())
		{
			CPeg selpeg = GetSelPeg();
			selpeg.position -= 0.1;
			//Make sure that the position does not stray bellow zero
			selpeg.position = (selpeg.position >= 0.0) ?  selpeg.position : 0.0;
			MoveSelected(selpeg.position, true);

			//Send parent messages
			SendBasicNotification(GC_PEGMOVED, selpeg, m_Selected);
		}
		
		break;

	case VK_NEXT:
		if(m_Selected >= 0 && m_Selected < m_Gradient.GetPegCount())
		{
			CPeg selpeg = GetSelPeg();
			selpeg.position += 0.1;
			//Make sure that the position does not stray above 1
			selpeg.position = (selpeg.position <= 1.0) ?  selpeg.position : 1.0;
			MoveSelected(selpeg.position, true);
			
			//Send parent message --
			SendBasicNotification(GC_PEGMOVED, selpeg, m_Selected);
		}
		break;

	case VK_RETURN:
	case VK_SPACE:
		if(m_Selected != -1) SendSelectedMessage(GC_EDITPEG);
		break;

	case VK_INSERT:
		if(m_Selected != -1)
		{
			CPeg peg = GetSelPeg();
			CreatePeg(peg.position,peg.color);
		}
		break;
	}
	
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

int CGradientCtrl::SetSelIndex(int iSel)
{
	int oldsel = m_Selected;
	ASSERT(iSel > -4); //Nothing smaller than -4 ok?
	ASSERT(iSel != PEG_BACKGROUND); //You can't select the background
	ASSERT(iSel < m_Gradient.GetPegCount()); //Make sure things are in range

	m_Selected = iSel;

	return oldsel;
}

int CGradientCtrl::MoveSelected(double newpos, BOOL bUpdate)
{
	CPeg peg;

	if(m_Selected < 0)
		return -1;

	peg = GetSelPeg();
	m_Selected = m_Gradient.SetPeg(m_Selected, peg.color, newpos);

	if(bUpdate) Invalidate();
	
	return m_Selected;
}

COLORREF CGradientCtrl::SetColorSelected(COLORREF crNewColor, BOOL bUpdate)
{
	CPeg peg;

	if(m_Selected < 0)
		return 0x00000000;

	peg = GetSelPeg();
	m_Gradient.SetPeg(m_Selected, crNewColor, peg.position);

	if(bUpdate) Invalidate();
	
	return peg.color;
}

void CGradientCtrl::CreatePeg(double pos,COLORREF color)
{
	PegCreateNMHDR nmhdr;
	nmhdr.nmhdr.code = GC_CREATEPEG;
	nmhdr.nmhdr.hwndFrom = GetSafeHwnd();
	nmhdr.nmhdr.idFrom = GetDlgCtrlID();
	nmhdr.position = pos;
	nmhdr.color = color;
	GetParent()->SendMessage(WM_NOTIFY, nmhdr.nmhdr.idFrom, (DWORD)(&nmhdr));
}

void CGradientCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CRect clientrect, pegrect;
	CPeg peg;
	CClientDC dc(this);
	BOOL edit = FALSE;
	int drawwidth = m_Impl->GetDrawWidth();
	
	//----- Check if the user is selecting a marker peg -----//
	for(int i = 0; i < m_Gradient.GetPegCount(); i++)
	{
		peg = m_Gradient.GetPeg(i);
		
		if(m_Impl->m_LeftDownSide)
		{
			m_Impl->GetPegRect(i, &pegrect, false);
			if(pegrect.PtInRect(point))
			{
				SetSelIndex(i);
				edit = true;
				break;
			}
		}

		if(m_Impl->m_RightUpSide)
		{
			m_Impl->GetPegRect(i, &pegrect, true);
			if(pegrect.PtInRect(point))
			{
				SetSelIndex(i);
				edit = true;
				break;
			}
		}
	}

	//----- Check if the user is trying to select the first or last one -----//
	GetClientRect(&clientrect);

	pegrect.left = drawwidth+8, pegrect.top = 4;
	pegrect.right = drawwidth+15, pegrect.bottom = 11;
	if(pegrect.PtInRect(point))
	{
		m_Impl->DrawSelPeg(&dc, m_Selected); //Erase the last m_Selected peg
		m_Selected = PEG_START;
		m_Impl->DrawSelPeg(&dc, m_Selected); //Draw the new one
		edit = true;
	}

	pegrect.left = drawwidth+8, pegrect.top = clientrect.bottom-11;
	pegrect.right = drawwidth+15, pegrect.bottom = clientrect.bottom-4;
	if(pegrect.PtInRect(point))
	{	
		m_Impl->DrawSelPeg(&dc, m_Selected); //Erase the last m_Selected peg
		m_Selected = PEG_END;		
		m_Impl->DrawSelPeg(&dc, m_Selected);
		edit = true;
	}
	
	if(edit) SendSelectedMessage(GC_EDITPEG);
	else if(m_Impl->m_GradFrame.PtInRect(point)) {
		double pos;
		
		if(m_Impl->IsVertical())
			pos = m_Impl->PosFromPoint(point.y);
		else
			pos = m_Impl->PosFromPoint(point.x);

		CreatePeg(pos,m_Gradient.ColorFromPosition(pos));
	}

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CGradientCtrl::GetPegRgn(CRgn *rgn)
{
	CRect clientrect;
	POINT pegpoint;
	int j;
	int drawwidth = m_Impl->GetDrawWidth();
	bool vertical = m_Impl->IsVertical();
	int colcount = (int)m_Impl->m_LeftDownSide+(int)m_Impl->m_RightUpSide;

	if(colcount == 0)
	{
		rgn->CreateRectRgn(0,0,0,0); // A dummy
		return;
	}

	//----- Carefully do some preparation -----//
	int pegcount = m_Gradient.GetPegCount();
	POINT *points = new POINT[pegcount*3*colcount + 8*colcount];
	ASSERT(points);
	int *polycounts = new int[pegcount*colcount + 2*colcount];
	ASSERT(polycounts);
	
	GetClientRect(&clientrect);

	//----- End pegs -----//
	j = 0;
	if(vertical)
	{
		if(m_Impl->m_RightUpSide)
		{
			polycounts[j/4]=4;
			points[j].x = drawwidth-15,	points[j].y = 4;
			j++;
			points[j].x = drawwidth-15,	points[j].y = 11;
			j++;
			points[j].x = drawwidth-8,	points[j].y = 11;
			j++;
			points[j].x = drawwidth-8,	points[j].y = 4;
			j++;

			polycounts[j/4]=4;
			points[j].x = drawwidth-15,	points[j].y = clientrect.bottom-4;
			j++;
			points[j].x = drawwidth-15,	points[j].y = clientrect.bottom-11;
			j++;
			points[j].x = drawwidth-8,	points[j].y = clientrect.bottom-11;
			j++;
			points[j].x = drawwidth-8,	points[j].y = clientrect.bottom-4;
			j++;
		}

		if(m_Impl->m_LeftDownSide)
		{
			polycounts[j/4]=4;
			points[j].x = 8,	points[j].y = 4;
			j++;
			points[j].x = 8,	points[j].y = 11;
			j++;			
			points[j].x = 15,	points[j].y = 11;
			j++;			
			points[j].x = 15,	points[j].y = 4;
			j++;			
			
			polycounts[j/4]=4;
			points[j].x = 8,	points[j].y = clientrect.bottom-4;
			j++;	
			points[j].x = 8,	points[j].y = clientrect.bottom-11;
			j++;	
			points[j].x = 15,	points[j].y = clientrect.bottom-11;
			j++;			
			points[j].x = 15,	points[j].y = clientrect.bottom-4;
			j++;			
		}
	}
	else
	{
		if(m_Impl->m_RightUpSide)
		{
			polycounts[j/4]=4;
			points[j].x = 4,	points[j].y = clientrect.bottom-drawwidth+8;
			j++;
			points[j].x = 11,	points[j].y = clientrect.bottom-drawwidth+8;
			j++;
			points[j].x = 11,	points[j].y = clientrect.bottom-drawwidth+15;
			j++;
			points[j].x = 4,	points[j].y = clientrect.bottom-drawwidth+15;
			j++;
		
			polycounts[j/4]=4;
			points[j].x = clientrect.right-4,	points[j].y = clientrect.bottom-drawwidth+8;
			j++;
			points[j].x = clientrect.right-11,	points[j].y = clientrect.bottom-drawwidth+8;
			j++;
			points[j].x = clientrect.right-11,	points[j].y = clientrect.bottom-drawwidth+15;
			j++;
			points[j].x = clientrect.right-4,	points[j].y = clientrect.bottom-drawwidth+15;
			j++;
		}
		
		if(m_Impl->m_LeftDownSide)
		{
			polycounts[j/4]=4;
			points[j].x = 4,	points[j].y = clientrect.bottom-8;
			j++;
			points[j].x = 11,	points[j].y = clientrect.bottom-8;
			j++;
			points[j].x = 11,	points[j].y = clientrect.bottom-15;
			j++;
			points[j].x = 4,	points[j].y = clientrect.bottom-15;
			j++;
			
			polycounts[j/4]=4;
			points[j].x = clientrect.right-4,	points[j].y = clientrect.bottom-8;
			j++;
			points[j].x = clientrect.right-11,	points[j].y = clientrect.bottom-8;
			j++;
			points[j].x = clientrect.right-11,	points[j].y = clientrect.bottom-15;
			j++;
			points[j].x = clientrect.right-4,	points[j].y = clientrect.bottom-15;
			j++;
		}
	}

	j=0;

	//----- Main pegs -----//
	for(int i = 0; i < pegcount; i++)
	{
		if(vertical)
		{
			pegpoint.y = m_Impl->PointFromPos(m_Gradient.GetPeg(i).position);
			
			if(m_Impl->m_LeftDownSide)
			{
				pegpoint.x = 23 - m_Impl->GetPegIndent(i)*11;

				points[j*3+8*colcount].x = pegpoint.x;
				points[j*3+8*colcount].y = pegpoint.y+1;
				points[j*3+8*colcount+1].x = pegpoint.x-10;
				points[j*3+8*colcount+1].y = pegpoint.y-4;
				points[j*3+8*colcount+2].x = pegpoint.x-10;
				points[j*3+8*colcount+2].y = pegpoint.y+6;
				polycounts[j + 2*colcount] = 3;
				j++;
			}

			if(m_Impl->m_RightUpSide)
			{
				pegpoint.x = m_Impl->GetPegIndent(i)*11 + drawwidth - 23;

				points[j*3+8*colcount].x = pegpoint.x;
				points[j*3+8*colcount].y = pegpoint.y+1;
				points[j*3+8*colcount+1].x = pegpoint.x+10;
				points[j*3+8*colcount+1].y = pegpoint.y-4;
				points[j*3+8*colcount+2].x = pegpoint.x+10;
				points[j*3+8*colcount+2].y = pegpoint.y+6;
				polycounts[j + 2*colcount] = 3;
				j++;
			}
		}
		else
		{
			pegpoint.x = m_Impl->PointFromPos(m_Gradient.GetPeg(i).position);

			if(m_Impl->m_LeftDownSide)
			{
				pegpoint.y = clientrect.bottom - 23 + m_Impl->GetPegIndent(i)*11;
				
				points[j*3+8*colcount].x = pegpoint.x+1;
				points[j*3+8*colcount].y = pegpoint.y-1;
				points[j*3+8*colcount+1].x = pegpoint.x-4;
				points[j*3+8*colcount+1].y = pegpoint.y+10;
				points[j*3+8*colcount+2].x = pegpoint.x+7;
				points[j*3+8*colcount+2].y = pegpoint.y+10;
				polycounts[j + 2*colcount] = 3;
				j++;
			}

			if(m_Impl->m_RightUpSide)
			{
				pegpoint.y = clientrect.bottom - m_Impl->GetPegIndent(i)*11 - drawwidth + 22;
				
				points[j*3+8*colcount].x = pegpoint.x+1;
				points[j*3+8*colcount].y = pegpoint.y+1;
				points[j*3+8*colcount+1].x = pegpoint.x-4;
				points[j*3+8*colcount+1].y = pegpoint.y-9;
				points[j*3+8*colcount+2].x = pegpoint.x+6;
				points[j*3+8*colcount+2].y = pegpoint.y-9;
				polycounts[j + 2*colcount] = 3;
				j++;
			}
		}
	}

	rgn->CreatePolyPolygonRgn(points, polycounts,
		(pegcount+2)*colcount, ALTERNATE);

	delete[] points;
	delete[] polycounts;
}

BOOL CGradientCtrl::PreTranslateMessage(MSG* pMsg) 
{
	m_Impl->m_ToolTipCtrl.RelayEvent(pMsg);

	return CWnd::PreTranslateMessage(pMsg);
}

void CGradientCtrl::ShowTooltips(BOOL bShow)
{
	m_bShowToolTip = bShow;
	if(m_Impl->m_ToolTipCtrl.GetSafeHwnd() != NULL)
		m_Impl->m_ToolTipCtrl.Activate(bShow);
	if(m_bShowToolTip) m_Impl->SynchronizeTooltips();
}

void CGradientCtrl::SetPegSide(BOOL setrightup, BOOL enable)
{
	if(setrightup) m_Impl->m_RightUpSide = enable;
	else m_Impl->m_LeftDownSide = enable;
}

BOOL CGradientCtrl::GetPegSide(BOOL rightup) const 
{
	return rightup?m_Impl->m_RightUpSide:m_Impl->m_LeftDownSide;
}

void CGradientCtrl::SetTooltipFormat(CString format) {m_ToolTipFormat = format;}
CString CGradientCtrl::GetTooltipFormat() const {return m_ToolTipFormat;}

void CGradientCtrl::SendBasicNotification(UINT code, CPeg peg, int index)
{
	//----- Send parent messages -----//
	CWnd *pParent = GetParent();
	if (pParent)
	{
		PegNMHDR nmhdr;
		nmhdr.nmhdr.code = code;
		nmhdr.nmhdr.hwndFrom = GetSafeHwnd();
		nmhdr.nmhdr.idFrom = GetDlgCtrlID();
		nmhdr.peg = peg;
		nmhdr.index = index;
		pParent->SendMessage(WM_NOTIFY, nmhdr.nmhdr.idFrom, (DWORD)(&nmhdr));
	}
}

const CPeg CGradientCtrl::GetSelPeg() const
{
	if(m_Selected == -1) return m_Impl->m_Null;
	if(m_Selected >= -3 && m_Selected < m_Gradient.GetPegCount()) return m_Gradient.GetPeg(m_Selected);
	ASSERT(FALSE); //Some kind of stupid selection error?
	return m_Impl->m_Null;
}

UINT CGradientCtrl::OnGetDlgCode() 
{
	return DLGC_WANTALLKEYS;
}

void CGradientCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	CWnd::OnKillFocus(pNewWnd);
	CClientDC dc(this);
	m_Impl->DrawSelPeg(&dc, m_Selected);
}

void CGradientCtrl::OnSetFocus(CWnd* pOldWnd) 
{
	CWnd::OnSetFocus(pOldWnd);
	CClientDC dc(this);
	if(GetSelIndex()==PEG_NONE) {
		SetSelIndex(PEG_START);
		SendSelectedMessage(GC_SELCHANGE);
	}
	m_Impl->DrawSelPeg(&dc, m_Selected);
}

LRESULT CGradientCtrl::OnMouseLeave(WPARAM wParam,LPARAM strText)
{
	if(m_Impl->m_wndToolTip) m_Impl->DestroyTooltip();
	return FALSE;
}


