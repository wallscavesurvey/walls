// GradientCtrlImpl.cpp: implementation of the CGradientCtrlImpl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GradientCtrl.h"
#include "Gradient.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

const TCHAR ampersandtag[] = _T("&&");
const TCHAR postag[] = _T("&SELPOS");
const TCHAR valtag[] = _T("&SELVAL");
const TCHAR rtag[] = _T("&R");
const TCHAR gtag[] = _T("&G");
const TCHAR btag[] = _T("&B");
const TCHAR hexrtag[] = _T("&HEXR");
const TCHAR hexgtag[] = _T("&HEXG");
const TCHAR hexbtag[] = _T("&HEXB");
const TCHAR floatrtag[] = _T("&FLOATR");
const TCHAR floatgtag[] = _T("&FLOATG");
const TCHAR floatbtag[] = _T("&FLOATB");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGradientCtrlImpl::CGradientCtrlImpl(CGradientCtrl *owner)
{
	m_Owner = owner;
	m_wndToolTip = NULL;
	m_RectCount = m_HistWidth = 0;
	m_RightUpSide = true;
	m_LeftDownSide = false;
	m_pHistValue = NULL;
	m_GradFrame.SetRectEmpty();
}

CGradientCtrlImpl::~CGradientCtrlImpl()
{
	if(m_wndToolTip) DestroyTooltip();
	free(m_pHistValue);
}

void CGradientCtrlImpl::GetGradientFrame()
{
	int w;
	CRect clientrect;
	m_Owner->GetClientRect(&clientrect);

	if(IsVertical())
	{
		w=((m_Owner->m_Width==GCW_AUTO)?clientrect.right:m_Owner->m_Width);
		m_GradFrame.left	= m_LeftDownSide ? 23 : 4,
		m_GradFrame.top	= 4,
		m_GradFrame.right	= w - (m_RightUpSide ? 23 : 4);
		m_GradFrame.bottom = clientrect.bottom-4;
		m_HistWidth = clientrect.right-w;
		m_HistLength = m_GradFrame.Height();
	}
	else
	{
		w=((m_Owner->m_Width==GCW_AUTO)?clientrect.bottom:m_Owner->m_Width);
		m_GradFrame.left	= 4,
		m_GradFrame.top	= clientrect.bottom - w + (m_RightUpSide ? 23 : 4);
		m_GradFrame.right	= clientrect.right-4;
		m_GradFrame.bottom	= clientrect.bottom-(m_LeftDownSide?23:4);
		m_HistWidth = clientrect.bottom-w;
		m_HistLength = m_GradFrame.Width();
	}
	ASSERT(!m_pHistValue && m_HistWidth>1);
	m_pHistValue=(float *)malloc(m_HistLength*sizeof(float)+(m_HistLength+1)*sizeof(WORD));
	m_pHistArray=(WORD *)(m_pHistValue+m_HistLength);
}

void CGradientCtrlImpl::Draw(CDC *dc)
{
	//----- Draw the Palette -----//
	DrawGradient(dc);

	//----- Draw the marker arrows -----//
	DrawEndPegs(dc);
	DrawPegs(dc);

	//The order is important -
    //The function DrawSelPeg is called in the function DrawPegs
	//so if we then draw the end peg we will be drawing over the inverse rectangle!

	if(::GetFocus()==m_Owner->m_hWnd) {
		DrawSelPeg(dc, m_Owner->m_Selected);
	}

	CBrush brush;
	brush.CreateStockObject(BLACK_BRUSH);

	//----- Draw a box around the palette -----//
	dc->FrameRect(m_GradFrame,&brush);

	//Draw histogram box - m_HistWidth is the width inside the box (Width()-2) --

	if(m_pHistValue) DrawHistogram(dc);
}

void CGradientCtrlImpl::DrawHistogram(CDC *dc)
{
	//Note: right and bottom coordinates of rectangle are not painted
	//The Width() of a rectangle is right-left, the actual painted width

	ASSERT(!IsVertical()); //support only horizontal version for now
	CRect rect(m_GradFrame);

	//Clear any existing histogram --
	rect.bottom=rect.top;
	rect.top-=m_HistWidth;
	::FillRect(dc->m_hDC, &rect,(HBRUSH)(COLOR_BTNFACE+1));

	CPen pen,*oldpen;
	VERIFY(pen.CreatePen(PS_SOLID,1,(COLORREF)0));
	VERIFY(oldpen=dc->SelectObject(&pen));
	//CPen *oldpen = (CPen*)dc->SelectStockObject(BLACK_PEN);
	int y,x=rect.left;
	int ybase=rect.bottom-1;

	for(int i=1;i<=m_HistLength;i++,x++) {
		if(m_pHistArray[i]) {
			//Paint a maximum of m_HistWidth pixels 
			y=(m_HistWidth*m_pHistArray[i])/m_pHistArray[0];
			if(!y) y++;
			dc->MoveTo(x,ybase);
			dc->LineTo(x,ybase-y); //paint max(1,y) pixels
		}
	}
	dc->SelectObject(oldpen);
}

BOOL CGradientCtrlImpl::GetNearbyValueStr(int x,CString &valstr)
{
	#define MAX_DELTA 2
	int delta;

    x++; //at least 1
	ASSERT(x>0 && x<=m_HistLength);
	for(delta=0;delta<=MAX_DELTA;delta++) {
		x+=delta;
		if(x<=m_HistLength && m_pHistArray[x]) break;
		x-=2*delta;
		if(x>0 && m_pHistArray[x]) break;
		x+=delta;
	}
	if(delta>MAX_DELTA) return FALSE;
	m_Owner->GetGradient().ValueStr(valstr,m_pHistValue[x-1]);
	return TRUE;
}


void CGradientCtrlImpl::DrawGradient(CDC *dc)
{
	CRect rect(m_GradFrame);
	rect.InflateRect(-1,-1,-1,-1);
	m_Owner->m_Gradient.FillRect(dc,&rect,IsVertical());
}

void CGradientCtrlImpl::DrawPegs(CDC *dc)
{
	CPeg peg;
	CRect clientrect;
	
	m_Owner->GetClientRect(&clientrect);

	// No stupid selection
	if(m_Owner->m_Selected > m_Owner->m_Gradient.GetPegCount())
		m_Owner->m_Selected = -1;

	int pegindent = 0;
	
	for(int i = 0; i < m_Owner->m_Gradient.GetPegCount(); i++)
	{
		peg = m_Owner->m_Gradient.GetPeg(i);

		if(m_RightUpSide)
		{
			//Indent if close
			pegindent = GetPegIndent(i)*11 + GetDrawWidth() - 23;

			//Obvious really
			if(IsVertical())
				DrawPeg(dc, CPoint(pegindent, PointFromPos(peg.position)), peg.color, 0);
			else
				DrawPeg(dc, CPoint(PointFromPos(peg.position), clientrect.bottom - pegindent - 1), peg.color, 1);
		}

		if(m_LeftDownSide)
		{
			//Indent if close
			pegindent = 23 - GetPegIndent(i)*11;

			//Obvious really
			if(IsVertical())
				DrawPeg(dc, CPoint(pegindent, PointFromPos(peg.position)), peg.color, 2);
			else
				DrawPeg(dc, CPoint(PointFromPos(peg.position), clientrect.bottom - pegindent - 1), peg.color, 3);

		}
	}
}

void CGradientCtrlImpl::DrawSelPeg(CDC *dc, CPoint point, int direction)
{
	POINT points[3];

	//Select The Color
	CPen *oldpen = (CPen*)dc->SelectStockObject(BLACK_PEN);
	CBrush *oldbrush = (CBrush*)dc->SelectStockObject(NULL_BRUSH);
	int oldrop = dc->SetROP2(R2_NOT);

	//Prepare the coodrdinates
	switch(direction)
	{
	case 0:
		points[0].x = 8+point.x;
		points[0].y = point.y-2;
		points[1].x = 2+point.x;
		points[1].y = point.y+1;
		points[2].x = 8+point.x;
		points[2].y = point.y+4;
		break;
	case 1:
		points[0].x = point.x-2;
		points[0].y = point.y-8;
		points[1].x = point.x+1;
		points[1].y = point.y-2;
		points[2].x = point.x+4;
		points[2].y = point.y-8;
		break;
	case 2:
		points[0].x = point.x-9,	points[0].y = point.y-2;
		points[1].x = point.x-3,	points[1].y = point.y+1;
		points[2].x = point.x-9,	points[2].y = point.y+4;
		break;
	default:
		points[0].x = point.x-2;
		points[0].y = point.y+8;
		points[1].x = point.x+1;
		points[1].y = point.y+2;
		points[2].x = point.x+4;
		points[2].y = point.y+8;
		break;
	}
	dc->Polygon(points, 3);

	//Restore the old brush and pen
	dc->SetROP2(oldrop);
	dc->SelectObject(oldbrush);
	dc->SelectObject(oldpen);
}

void CGradientCtrlImpl::DrawSelPeg(CDC *dc, int peg)
{
	CBrush *oldbrush;
	CPen *oldpen;
	CRect clientrect;
	int drawwidth = GetDrawWidth()-23;
	bool vertical = IsVertical();

	m_Owner->GetClientRect(&clientrect);
	
	//"Select objects"//
	oldpen = (CPen*)dc->SelectStockObject(BLACK_PEN);
	oldbrush = (CBrush*)dc->SelectStockObject(NULL_BRUSH);
	int oldrop = dc->SetROP2(R2_NOT);

	if(peg == PEG_START)
	{
		if(m_RightUpSide)
			if(vertical) dc->Rectangle(drawwidth+9, 5, drawwidth+14, 10);
			else dc->Rectangle(5, clientrect.bottom-drawwidth-9,
					10,	clientrect.bottom-drawwidth-14);
		
		if(m_LeftDownSide)
			if(vertical) dc->Rectangle(9, 5, 14, 10);
			else dc->Rectangle(5, clientrect.bottom-9, 10, clientrect.bottom-14);
	}
	else if(peg == PEG_END)
	{
		if(m_RightUpSide)
			if(vertical) dc->Rectangle(drawwidth+9, clientrect.bottom-10, drawwidth+14, clientrect.bottom-5);
			else dc->Rectangle(clientrect.right-10, clientrect.bottom-drawwidth-9,
					clientrect.right-5, clientrect.bottom-drawwidth-14);
		
		if(m_LeftDownSide)
			if(vertical) dc->Rectangle(9, clientrect.bottom-5, 14, clientrect.bottom-10);
			else dc->Rectangle(clientrect.right-5, clientrect.bottom-9,
				clientrect.right-10, clientrect.bottom-14);
	}
	
	dc->SelectObject(oldbrush);
	dc->SetROP2(oldrop);
	dc->SelectObject(oldpen);

	if(peg<0) return;

	if(m_Owner->m_Gradient.GetPegCount())
	{
		CPeg mypeg = m_Owner->m_Gradient.GetPeg(peg);
		int pegindent = GetPegIndent(peg)*11;

		if(IsVertical())
		{
			if(m_RightUpSide)
				DrawSelPeg(dc, CPoint(pegindent + drawwidth, PointFromPos(mypeg.position)), 0);
			if(m_LeftDownSide)
				DrawSelPeg(dc, CPoint(23-pegindent, PointFromPos(mypeg.position)), 2);
		}
		else
		{
			if(m_RightUpSide)
				DrawSelPeg(dc, CPoint(PointFromPos(mypeg.position), clientrect.bottom-pegindent-drawwidth-1), 1);
			if(m_LeftDownSide)
				DrawSelPeg(dc, CPoint(PointFromPos(mypeg.position), clientrect.bottom-23+pegindent), 3);
		}
	}
}

void CGradientCtrlImpl::DrawEndPegs(CDC *dc)
{
	CRect clientrect;
	bool vertical = IsVertical();
	int drawwidth = GetDrawWidth();

	m_Owner->GetClientRect(&clientrect);
	CPen *oldpen = (CPen*)dc->SelectStockObject(BLACK_PEN);

	//----- Draw the first marker -----//
	CBrush brush(m_Owner->m_Gradient.GetStartPegColor());
	CBrush *oldbrush = dc->SelectObject(&brush); //select the brush
	if(m_RightUpSide)
	{
		if(vertical)
			dc->Rectangle(drawwidth-15, 4, drawwidth-8, 11); //draw the rectangle
		else
			dc->Rectangle(4, clientrect.bottom-drawwidth+8,
				11, clientrect.bottom-drawwidth+15); //draw the rectangle
	}
	
	if(m_LeftDownSide)
	{
		if(vertical)
			dc->Rectangle(8, 4, 15, 11); //draw the rectangle
		else
			dc->Rectangle(4, clientrect.bottom-15,
				11, clientrect.bottom-8); //draw the rectangle
	}

	dc->SelectObject(oldbrush); // restore the old brush
	brush.DeleteObject();

	//----- Draw the second one -----//
	brush.CreateSolidBrush(m_Owner->m_Gradient.GetEndPegColor());
	oldbrush = dc->SelectObject(&brush); //select the brush
	
	if(m_LeftDownSide)
	{
		if(vertical)
			dc->Rectangle(8, clientrect.bottom-4, 15,
				clientrect.bottom-11);
		else
			dc->Rectangle(clientrect.right-4, clientrect.bottom-15,
				clientrect.right-11, clientrect.bottom-8);
	}
	
	if(m_RightUpSide)
	{
		if(vertical)
			dc->Rectangle(drawwidth-15, clientrect.bottom-4, drawwidth-8,
				clientrect.bottom-11);
		else
			dc->Rectangle(clientrect.right-4, clientrect.bottom-drawwidth+8,
				clientrect.right-11, clientrect.bottom-drawwidth+15);
	}

	dc->SelectObject(oldbrush); //restore the old brush
	dc->SelectObject(oldpen);
}

void CGradientCtrlImpl::DrawPeg(CDC *dc, CPoint point, COLORREF color, int direction)
{
	CBrush brush, *oldbrush;
	POINT points[3];

	//Select The Color
	brush.CreateSolidBrush(color);
	CPen *oldpen = (CPen*)dc->SelectStockObject(NULL_PEN);
	oldbrush = dc->SelectObject(&brush);
	
	//Prepare the coodrdinates
	switch(direction)
	{
	case 0:
		points[0].x = point.x;
		points[0].y = point.y+1;
		points[1].x = point.x+9;
		points[1].y = point.y-3;
		points[2].x = point.x+9;
		points[2].y = point.y+5;
		break;
	case 1:
		points[0].x = point.x+1;
		points[0].y = point.y;
		points[1].x = point.x-3;
		points[1].y = point.y-9;
		points[2].x = point.x+5;
		points[2].y = point.y-9;
		break;
	case 2:
		points[0].x = point.x-1;
		points[0].y = point.y+1;
		points[1].x = point.x-10;
		points[1].y = point.y-3;
		points[2].x = point.x-10;
		points[2].y = point.y+5;
		break;
	default:
		points[0].x = point.x+1;
		points[0].y = point.y+1;
		points[1].x = point.x-3;
		points[1].y = point.y+10;
		points[2].x = point.x+5;
		points[2].y = point.y+10;
		break;
	}
	dc->Polygon(points, 3);
	
	//Restore the old brush and pen
	dc->SelectObject(oldbrush);
	dc->SelectObject(oldpen);
	
	//----- Draw lines manually in the right directions ------//
	CPen outlinepen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT));
	oldpen = dc->SelectObject(&outlinepen);
	
	dc->MoveTo(points[0]);
	dc->LineTo(points[1]);
	dc->LineTo(points[2]);
	dc->LineTo(points[0]);
	
	dc->SelectObject(oldpen);

	brush.DeleteObject();
}

CString CGradientCtrlImpl::ExtractLine(CString source, int line)
{
	int start = 0, end;
	CString textline;

	if(source == _T(""))
		return _T("");

	while(line > 0)
	{
		start = source.Find(_T('\n'), start);
		if(start == -1)
			return _T("");
		start++;
		line--;
	}

	end = source.Find(_T('\n'), start);
	if(end == -1)
		end = source.GetLength();

	textline = source;
	CString op = textline.Mid(start, end-start);

	return textline.Mid(start, end-start);
}

void CGradientCtrlImpl::ParseToolTipLine(CString &tiptext,CPeg peg)
{
	CString str;

	tiptext=ExtractLine(m_Owner->m_ToolTipFormat,1);
	tiptext.Replace(ampersandtag,_T("&"));
	tiptext.Replace(valtag,m_Owner->GetGradient().ValueStrFromPos(str,peg.position));

	LPCSTR fmt=_T("%2.2X");

	str.Format(fmt, GetRValue(peg.color));
	tiptext.Replace(hexrtag, str);

	str.Format(fmt, GetGValue(peg.color));
	tiptext.Replace(hexgtag, str);

	str.Format(fmt, GetBValue(peg.color));
	tiptext.Replace(hexbtag, str);

	fmt=_T("%0.3f");

	str.Format(fmt, peg.position);
	tiptext.Replace(postag, str);

	str.Format(fmt, ((double)GetRValue(peg.color))/255.0);
	tiptext.Replace(floatrtag, str);

	str.Format(fmt, ((double)GetGValue(peg.color))/255.0);
	tiptext.Replace(floatgtag, str);

	str.Format(fmt, ((double)GetBValue(peg.color))/255.0);
	tiptext.Replace(floatbtag, str);
	
	fmt=_T("%u");
	str.Format(fmt, GetRValue(peg.color));
	tiptext.Replace(rtag, str);

	str.Format(fmt, GetGValue(peg.color));
	tiptext.Replace(gtag, str);

	str.Format(fmt, GetBValue(peg.color));
	tiptext.Replace(btag, str);
}

void CGradientCtrlImpl::SynchronizeTooltips()
{
	CString tiptext;
	CRect pegrect, clientrect;
	int drawwidth = GetDrawWidth();

	CClientDC dc(m_Owner);

	m_Owner->GetClientRect(&clientrect);

	if(!m_ToolTipCtrl.GetSafeHwnd())
		m_ToolTipCtrl.Create(m_Owner);
	
	//----- Out with the old -----//
	for(int i = 0; i < m_RectCount+1; i++) m_ToolTipCtrl.DelTool(m_Owner, i);

	m_RectCount = m_Owner->m_Gradient.GetPegCount();

	if(m_Owner->m_ToolTipFormat == _T("")) return;

	//----- Add the main pegs -----//
	for(int i = 0; i < m_RectCount; i++)
	{
		if(m_LeftDownSide)
		{
			GetPegRect(i, &pegrect, false);
			ParseToolTipLine(tiptext, m_Owner->m_Gradient.GetPeg(i));
			m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, i+6);
		}

		if(m_RightUpSide)
		{
			GetPegRect(i, &pegrect, true);
			ParseToolTipLine(tiptext, m_Owner->m_Gradient.GetPeg(i));
			m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, i+6);
		}
	}

	//----- Add ones for the end pegs -----//
	ParseToolTipLine(tiptext, m_Owner->m_Gradient.GetPeg(PEG_START));
	if(tiptext != _T(""))
	{
		if(IsVertical())
		{	
			if(m_RightUpSide)
			{
				pegrect.SetRect(drawwidth-15, 4, drawwidth-8, 11);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 1);
			}

			if(m_LeftDownSide)
			{
				pegrect.SetRect(8, 4, 15, 11);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 2);
			}
		}
		else
		{	
			if(m_RightUpSide)
			{
				pegrect.SetRect(4, clientrect.bottom-drawwidth+8, 11,
					clientrect.bottom-drawwidth+15);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 1);
			}

			if(m_LeftDownSide)
			{
				pegrect = CRect(4, clientrect.bottom-15, 11, clientrect.bottom-8);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 2);
			}
		}
	}

	ParseToolTipLine(tiptext, m_Owner->m_Gradient.GetPeg(PEG_END));
	if(tiptext != _T(""))
	{
		if(IsVertical())
		{		
			if(m_RightUpSide)
			{	
				pegrect.SetRect(drawwidth-15, clientrect.bottom-11, drawwidth-8,
					clientrect.bottom-4);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 3);
			}

			if(m_LeftDownSide)
			{
				pegrect.SetRect(8, clientrect.bottom-11, 15,
					clientrect.bottom-4);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 4);
			}
		}
		else
		{		
			if(m_RightUpSide)
			{	
				pegrect.SetRect(clientrect.right-11, clientrect.bottom-drawwidth+8,
					clientrect.right-4, clientrect.bottom-drawwidth+15);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 3);
			}

			if(m_LeftDownSide)
			{
				pegrect.SetRect(clientrect.right-11, clientrect.bottom-15,
					clientrect.right-4, clientrect.bottom-8);
				m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 4);
			}
		}
	}

	//----- Add one for the gradient -----//
	if(IsVertical())
		pegrect.SetRect(m_LeftDownSide?23:4, 4,
			drawwidth-(m_RightUpSide?23:4),
			(clientrect.bottom>10)?clientrect.bottom-4:clientrect.bottom);
	else
		pegrect.SetRect(4, clientrect.bottom-drawwidth+(m_RightUpSide?23:4),
			(clientrect.right>10)?clientrect.right-4:clientrect.right,
			clientrect.bottom-(m_LeftDownSide?23:4));

	tiptext = ExtractLine(m_Owner->m_ToolTipFormat, 2);
	if(tiptext != _T(""))
	{
		m_ToolTipCtrl.AddTool(m_Owner, tiptext, pegrect, 5);
	}
}

void CGradientCtrlImpl::ShowTooltip(CPoint point, CString text)
{
	unsigned int uid = 0;       // for ti initialization

	if(text == _T(""))
		return;

	// CREATE A TOOLTIP WINDOW
	if(m_wndToolTip == NULL)
	{
		m_wndToolTip = CreateWindowEx(WS_EX_TOPMOST,
			TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,		
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, NULL, NULL);

		ASSERT(m_wndToolTip);
		if(!m_wndToolTip) return;
		// INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE
		m_ToolInfo.cbSize = sizeof(TOOLINFO);
		m_ToolInfo.uFlags = TTF_TRACK;
		m_ToolInfo.hwnd = NULL;
		m_ToolInfo.hinst = NULL;
		m_ToolInfo.uId = uid;
		m_ToolInfo.lpszText = (LPTSTR)(LPCTSTR) text;		
		// ToolTip control will cover the whole window
		m_ToolInfo.rect.left = 0;
		m_ToolInfo.rect.top = 0;
		m_ToolInfo.rect.right = 0;
		m_ToolInfo.rect.bottom = 0;

		// SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW
		::SendMessage(m_wndToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &m_ToolInfo);
		TrackMouse(m_Owner->m_hWnd);
	}

	::SendMessage(m_wndToolTip, TTM_TRACKPOSITION, 0, (LPARAM)(DWORD) MAKELONG(point.x, point.y));
	::SendMessage(m_wndToolTip, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
}

void CGradientCtrlImpl::SetTooltipText(CString text)
{	
	if(m_wndToolTip != NULL)
	{
		m_ToolInfo.lpszText = (LPTSTR)(LPCTSTR) text;	
		::SendMessage(m_wndToolTip, TTM_SETTOOLINFO, 0, (LPARAM)(LPTOOLINFO) &m_ToolInfo);   	
	}
}

void CGradientCtrlImpl::DestroyTooltip()
{
	::DestroyWindow(m_wndToolTip);
	m_wndToolTip = NULL;
}

bool CGradientCtrlImpl::IsVertical()
{
	if(m_Owner->m_Orientation == CGradientCtrl::ForceVertical)
		return true;
	else if(m_Owner->m_Orientation == CGradientCtrl::ForceHorizontal)
		return false;
	else
	{
		CRect clientrect;
		m_Owner->GetClientRect(&clientrect);
		return (clientrect.right <= clientrect.bottom);
	}
}

int CGradientCtrlImpl::GetDrawWidth()
{
	CRect clientrect;
	int dw;

	m_Owner->GetClientRect(&clientrect);

	dw = (m_Owner->m_Width == GCW_AUTO) ? (IsVertical() ? clientrect.right :
		clientrect.bottom) : m_Owner->m_Width;
	return dw;
}

int CGradientCtrlImpl::PointFromPos(double pos)
{
	CRect clientrect;
	m_Owner->GetClientRect(clientrect);
	double length = IsVertical() ? ((double)clientrect.Height() - 10.0)
		: ((double)clientrect.Width() - 10.0);

	return (int)(pos*length) + 4;
}

double CGradientCtrlImpl::PosFromPoint(int point)
{	
	CRect clientrect;
	m_Owner->GetClientRect(clientrect);
	int length = IsVertical() ? (clientrect.bottom -  9) : (clientrect.right -  9);
	int x = point-5;
	double val;
	
	val = (double)x/(double)length;

	if(val < 0) val = 0;
	else if(val > 1) val = 1;

	return val;
}

int CGradientCtrlImpl::GetPegIndent(int index)
{
	int lastpegpos = -1, pegindent = 0;

	for(int i = 0; i <= index; i++)
	{
		const CPeg &peg = m_Owner->m_Gradient.GetPeg(i);
		if(lastpegpos != -1 && lastpegpos >= PointFromPos(peg.position)-10)
			pegindent += 1;
		else pegindent = 0;

		lastpegpos = PointFromPos(peg.position);
	}
	
	return pegindent%2;
}

int CGradientCtrlImpl::PtInPeg(CPoint point)
{
	CPeg peg;
	CRect pegrect, clientrect;
	int drawwidth = GetDrawWidth();
	
	//----- Check if the point is on a marker peg -----//
	for(int i = 0; i < m_Owner->m_Gradient.GetPegCount(); i++)
	{
		peg = m_Owner->m_Gradient.GetPeg(i);
		
		if(m_LeftDownSide)
		{
			GetPegRect(i, &pegrect, false);
			if(pegrect.PtInRect(point))
				return i;
		}
		
		if(m_RightUpSide)
		{
			GetPegRect(i, &pegrect, true);
			if(pegrect.PtInRect(point))
				return i;
		}
	}

	//----- Check if the point is on an  end peg -----//
	m_Owner->GetClientRect(&clientrect);

	pegrect.left = drawwidth+8, pegrect.top = 4;
	pegrect.right = drawwidth+15, pegrect.bottom = 11;
	if(pegrect.PtInRect(point))
		return PEG_START;

	pegrect.left = drawwidth+8, pegrect.top = clientrect.bottom-11;
	pegrect.right = drawwidth+15, pegrect.bottom = clientrect.bottom-4;
	if(pegrect.PtInRect(point))
		return PEG_END;

	return -1;
}

void CGradientCtrlImpl::GetPegRect(int index, CRect *rect, bool right)
{
	CRect clientrect;
	int drawwidth = GetDrawWidth();
	bool vertical = IsVertical();
	m_Owner->GetClientRect(&clientrect);
	
	if(index == PEG_START)
	{
		if(right)
		{	
			if(vertical)
			{
				rect->left = drawwidth-15, rect->top = 4;
				rect->right = drawwidth-8, rect->bottom = 11;
			}
			else
			{
				rect->left = 4, rect->top = clientrect.bottom-drawwidth+8;
				rect->right = 11, rect->bottom = clientrect.bottom-drawwidth+15;
			}
		}
		else
		{
			if(vertical)
			{
				rect->left = 8, rect->top = 4;
				rect->right = 15, rect->bottom = 11;
			}
			else
			{
				rect->left = 4, rect->top = clientrect.bottom-15;
				rect->right = 11, rect->bottom = clientrect.bottom-8;
			}
		}

		return;
	}

	if(index == PEG_END)
	{
		if(right)
		{	
			if(vertical)
			{
				rect->left = drawwidth-15, rect->top = clientrect.bottom-11;
				rect->right = drawwidth-8, rect->bottom = clientrect.bottom-4;
			}
			else
			{
				rect->left = clientrect.right-11, rect->top = clientrect.bottom-drawwidth+8;
				rect->right = clientrect.right-4, rect->bottom = clientrect.bottom-drawwidth+15;
			}
		}
		else
		{
			if(vertical)
			{
				rect->left = 8, rect->top = clientrect.bottom-11;
				rect->right = 15, rect->bottom = clientrect.bottom-4;
			}
			else
			{
				rect->left = clientrect.right-11, rect->top = clientrect.bottom-15;
				rect->right = clientrect.right-4, rect->bottom = clientrect.bottom-8;
			}
		}

		return;
	}
	
	CPeg peg = m_Owner->m_Gradient.GetPeg(index);
	int p = PointFromPos(peg.position);
	int indent = GetPegIndent(index)*11;

	if(right)
	{
		if(vertical)
		{
			rect->top = p - 3;
			rect->bottom = p + 6;
			rect->left = drawwidth+indent-23;
			rect->right = drawwidth+indent-13;
		}
		else
		{
			rect->top = clientrect.bottom-drawwidth-indent+13;
			rect->bottom = clientrect.bottom-drawwidth-indent+23;
			rect->left = p - 3;
			rect->right = p + 6;
		}
	}
	else
	{
		if(vertical)
		{
			rect->top = p - 3;
			rect->bottom = p + 6;
			rect->left = 13-indent;
			rect->right = 23-indent;
		}
		else
		{
			rect->top = clientrect.bottom+indent-23;
			rect->bottom = clientrect.bottom+indent-13;
			rect->left = p - 3;
			rect->right = p + 6;
		}
	}
}
