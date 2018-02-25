// MarkerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "segview.h"
#include "compile.h"
#include "compview.h"
#include "plotview.h"
#include "MarkerDlg.h"

#ifndef __AFXCMN_H__
#include <afxcmn.h>
#endif


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMarkerDlg dialog


CMarkerDlg::CMarkerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMarkerDlg::IDD, pParent)
{
	m_bHideNotes = FALSE;
	m_hbkbr=NULL;
	m_pFS=NULL;
	m_bMapsActive=CPrjDoc::m_pReviewDoc && CPrjDoc::m_pReviewDoc->UpdateMapViews(TRUE, M_TOGGLEFLAGS);
}

void CMarkerDlg::UpdateFlagStyles(void)
{
	NTA_FLAGTREE_REC rec;
	
	ixSEG.UseTree(NTA_NOTETREE);
	
    if(!ixSEG.First()) {
		 do {
			VERIFY(!ixSEG.GetRec(&rec,sizeof(NTA_FLAGTREE_REC)));
			if(!M_ISFLAGNAME(rec.seg_id)) break;
			rec.color=m_pFS[rec.id].color;
			rec.seg_id=m_pFS[rec.id].style;
			rec.iseq=m_pFS[rec.id].lstidx;
			VERIFY(!ixSEG.PutRec(&rec));
		 }
		 while(!ixSEG.Next());
	}
	VERIFY(!ixSEG.Flush());
	ixSEG.UseTree(NTA_SEGTREE);
}

void CMarkerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLORFRM, m_rectSym);
	DDX_Check(pDX, IDC_OVERLAP, m_bNoOverlap);
	DDX_Check(pDX, IDC_HIDENOTES, m_bHideNotes);
	DDX_Control(pDX,IDC_FGCOLOR,m_colorbtn);
	DDX_Control(pDX,IDC_BKGCOLOR,m_bkgndbtn);

	if(pDX->m_bSaveAndValidate && m_pFS) {
		CSegView::m_iLastFlag=pLB(IDC_LISTFLAGS)->GetCaretIndex();
		UpdateFlagStyles();
		m_markcolor=m_pFS[pixSEGHDR->numFlagNames].color;
		m_markstyle=m_pFS[pixSEGHDR->numFlagNames].style;
	}
}

BEGIN_MESSAGE_MAP(CMarkerDlg, CDialog)
    ON_MESSAGE(CPN_SELCHANGE, OnChgColor)
	ON_LBN_SELCHANGE(IDC_LISTFLAGS,OnFlagSelChg)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_PRIORUP, OnPriorUp)
	ON_BN_CLICKED(IDC_PRIORDOWN, OnPriorDown)
	ON_BN_CLICKED(IDC_MSOLID, OnSolid)
	ON_CBN_SELCHANGE(IDC_MSHAPE, OnSelShape)
	ON_BN_CLICKED(IDC_MOPAQUE, OnOpaque)
	ON_BN_CLICKED(IDC_MCLEAR, OnClear)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_MSIZE, OnDeltaposSize)
	ON_WM_LBUTTONDOWN()
	ON_EN_KILLFOCUS(IDC_MSIZE, OnKillfocusSize)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_DISABLE, OnDisable)
	ON_BN_CLICKED(IDC_MTRANSPARENT, OnTransparent)
	ON_WM_VKEYTOITEM()
	ON_BN_CLICKED(IDC_ENABLE, OnEnable)
	ON_BN_CLICKED(IDC_HIDENOTES, OnHideNotes)
	ON_BN_CLICKED(IDC_SETSELECTED, OnSetSelected)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMarkerDlg message handlers

void CMarkerDlg::InitFlagNames(void)
{
    CListBox *plb=pLB(IDC_LISTFLAGS);
	UINT maxext=0;
	NTA_FLAGTREE_REC rec;
	BYTE key[TRX_MAX_KEYLEN+1];

	
	ASSERT(ixSEG.Opened());
	ixSEG.UseTree(NTA_NOTETREE);
	
	m_pFS=(MK_FLAGSTYLE *)calloc(pixSEGHDR->numFlagNames+1,sizeof(MK_FLAGSTYLE));

	if(m_pFS && pixSEGHDR->numFlagNames>0 && !ixSEG.First()) {
		 UINT ext;
		 CDC *pDC=plb->GetDC();
		 TEXTMETRIC tm;

		 VERIFY(pDC->SelectObject(plb->GetFont()));
		 VERIFY(pDC->GetTextMetrics(&tm));

	     do {
	       VERIFY(!ixSEG.Get(&rec,sizeof(NTA_FLAGTREE_REC),key,sizeof(key)));
	       if(M_ISFLAGNAME(rec.seg_id)) {
	         ASSERT(rec.id<pixSEGHDR->numFlagNames);
	         m_pFS[rec.iseq].flgidx=rec.id;
			 m_pFS[rec.id].lstidx=rec.iseq;
	         m_pFS[rec.id].color=ixSEG.Position();
			 m_pFS[rec.id].style=rec.seg_id;
			 m_pFS[rec.id].count=0;
	       }
		   else {
			   if(key[1]==NTA_CHAR_FLAGPREFIX) {
				 ASSERT((*(WORD *)(key+6))<pixSEGHDR->numFlagNames);
				 m_pFS[*(WORD *)(key+6)].count++;
			   }
			   else break;
		   }
	     }
	     while(!ixSEG.Next());

		 //Fill listbox with flag names --
		 for(int i=0;i<(int)pixSEGHDR->numFlagNames;i++) {
			 if(ixSEG.SetPosition(m_pFS[m_pFS[i].flgidx].color)) {
				 ASSERT(FALSE);
			 }
			 VERIFY(!ixSEG.Get(&rec,sizeof(NTA_FLAGTREE_REC),key,sizeof(key)));
			 m_pFS[m_pFS[i].flgidx].color=rec.color;
			 if(!*key) trx_Stccp((char *)key,"{Unnamed Flag}");
			 //else trx_CapitalizeKey((LPSTR)key);
			 ext=*key+sprintf((char *)key+*key+1," (%d)",m_pFS[m_pFS[i].flgidx].count);
			 ext=pDC->GetTextExtent((LPCTSTR)key+1,ext).cx+tm.tmAveCharWidth;
			 if(ext>maxext) maxext=ext;
			 VERIFY(plb->AddString(FixFlagCase((char *)key+1))>=0);
		 }

		 plb->ReleaseDC(pDC);
	}

	//Now set up style for default markers, also specifying horizontal extent --
	{
	 MK_FLAGSTYLE *pFS=&m_pFS[pixSEGHDR->numFlagNames];
	 pFS->flgidx=pFS->lstidx=pixSEGHDR->numFlagNames;
	 pFS->color=CSegView::m_StyleHdr[HDR_MARKERS].MarkerColor();
	 pFS->style=m_markstyle=CSegView::m_StyleHdr[HDR_MARKERS].seg_id;
	 if(maxext) plb->SetHorizontalExtent(maxext);
	 VERIFY(plb->AddString("{Station Markers}")>=0);
	 ASSERT(plb->GetCount()==pixSEGHDR->numFlagNames+1);
	}

	ixSEG.UseTree(NTA_SEGTREE);
}

BOOL CMarkerDlg::OnInitDialog() 
{
    static int dlgfont_id[]={IDC_ST_FLAGLIST,IDC_ST_MARKERSTYLE,0};
      
 	CDialog::OnInitDialog();

	pCV->GetReView()->SetFontsBold(GetSafeHwnd(),dlgfont_id);

	CListBox *plb=pLB(IDC_LISTFLAGS);

	//First, scan the flagname array --
	InitFlagNames();

	//m_colorbtn.SetCustomText(2); //default
	//m_bkgndbtn.SetCustomText(2); //default
	m_bkgndbtn.SetColor(m_bkgcolor=CSegView::m_StyleHdr[HDR_BKGND].LineColor());

    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_MSIZE))->SetRange(0,255);

    m_rectSym.GetClientRect(&m_rectBox);

    m_sizeBox.cx = m_rectBox.right-2; //Add 2 since we will InflateRect(1,1)
    m_sizeBox.cy = m_rectBox.bottom-2;

    m_rectSym.ClientToScreen(&m_rectBox);
    ScreenToClient(&m_rectBox);
    m_rectBox.InflateRect(-1,-1);
        
    // Get temporary DC for dialog - Will be released in dc destructor
    CClientDC dc(this);
    VERIFY(m_cBmp.CreateCompatibleBitmap(&dc,m_sizeBox.cx,m_sizeBox.cy));
    VERIFY(m_dcBmp.CreateCompatibleDC(&dc));
    VERIFY(m_hBmpOld=(HBITMAP)::SelectObject(m_dcBmp.m_hDC,m_cBmp.GetSafeHandle()));
    VERIFY(m_hBrushOld=(HBRUSH)::SelectObject(m_dcBmp.m_hDC,CSegView::m_hBrushBkg));

	plb->SetCaretIndex(CSegView::m_iLastFlag,0); //Should be last flag selected
	plb->SetSel(m_lastCaret=CSegView::m_iLastFlag);

	OnFlagSelChg();

	m_ToolTips.Create(this);
	m_colorbtn.AddToolTip(&m_ToolTips);
	m_bkgndbtn.AddToolTip(&m_ToolTips);

	plb->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

HBRUSH CMarkerDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
   int id=pWnd->GetDlgCtrlID();
   if(nCtlColor==CTLCOLOR_LISTBOX && id==IDC_LISTFLAGS) {
        pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)::GetSysColorBrush(COLOR_3DFACE);
   }

   return CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CMarkerDlg::OnDestroy() 
{
	CDialog::OnDestroy();
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hBmpOld));
    VERIFY(::SelectObject(m_dcBmp.GetSafeHdc(),m_hBrushOld));
    VERIFY(m_dcBmp.DeleteDC());
	if(m_hbkbr) ::DeleteObject(m_hbkbr);
	if(m_pFS) free(m_pFS);
}

void CMarkerDlg::OnCancel()
{
	//RestoreStyles();
	//pSV->UpdateSymbols(this);
	EndDialog(IDCANCEL);
}

void CMarkerDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	dc.BitBlt(m_rectBox.left,m_rectBox.top,m_rectBox.Width(),m_rectBox.Height(),
	  	&m_dcBmp,0,0,SRCCOPY);
}

void CMarkerDlg::SetListItemColor(int i,COLORREF rgb)
{
	COLORREF *pClr=&m_pFS[m_pFS[i].flgidx].color;
	M_SETCOLOR(*pClr,rgb);
}

LRESULT CMarkerDlg::OnChgColor(WPARAM clr,LPARAM id)
{
	if(id==IDC_BKGCOLOR) {
		if(m_hbkbr) ::DeleteObject(m_hbkbr);
		m_hbkbr=::CreateSolidBrush(clr);
		if(m_hbkbr) {
			VERIFY(::SelectObject(m_dcBmp.m_hDC,m_hbkbr));
			m_bkgcolor=clr;
		}
	}
	else {
		ASSERT(id==IDC_FGCOLOR);
		int i=pLB(IDC_LISTFLAGS)->GetCaretIndex();
		SetListItemColor(i,clr);
		//if(i!=pixSEGHDR->numFlagNames) m_bChanged=TRUE;
	}
	DrawSymbol();
	ApplyChange();
	return TRUE;
}

void CMarkerDlg::DrawSymbol()
{
    int i=pLB(IDC_LISTFLAGS)->GetCaretIndex();

	ASSERT(i!=LB_ERR);
	if(i==LB_ERR) return;

	WORD style=m_pFS[m_pFS[i].flgidx].style;

	m_dcBmp.PatBlt(0,0,m_sizeBox.cx,m_sizeBox.cy,PATCOPY);

	M_SETHIDDEN(style,FALSE);
	
	//Place symbol on bitmap --
    CPrjDoc::PlotFlagSymbol(&m_dcBmp,m_sizeBox,
		i!=pixSEGHDR->numFlagNames,
		ListItemRGB(i),
		m_bkgcolor,
		style);

	Invalidate(FALSE);
}

void CMarkerDlg::EnableButtons(int sel)
{
	CListBox *plb=pLB(IDC_LISTFLAGS);
	BOOL bHideEnable=FALSE,bUnhideEnable=FALSE;

	for(int i=plb->GetCount()-2;i>=0;i--) {
		if(plb->GetSel(i)) {
			if(IsListItemEnabled(i)) bHideEnable=TRUE;
			else bUnhideEnable=TRUE;
		}
	}
	Enable(IDC_SETSELECTED,plb->GetSelCount()>1);
	Enable(IDC_DISABLE,bHideEnable);
	Enable(IDC_ENABLE,bUnhideEnable);
	if(sel>=0) {
		Enable(IDC_PRIORUP,sel>0 && sel<pixSEGHDR->numFlagNames);
		Enable(IDC_PRIORDOWN,sel<pixSEGHDR->numFlagNames-1);
	}
}

void CMarkerDlg::DeselectAllBut(int p,int s)
{
	CListBox *plb=pLB(IDC_LISTFLAGS);
	int i,pIndex=s;

	if(p>s) {
		i=s; s=p; p=i;
	}
	for(i=plb->GetCount()-1;i>=0;i--) {
		plb->SetSel(i,i>=p && i<=s);
	}
	plb->SetCaretIndex(pIndex);
}

void CMarkerDlg::OnFlagSelChg()
{
	CListBox *plb=pLB(IDC_LISTFLAGS);
    int i=plb->GetCaretIndex();
    ASSERT(i>=0 && i<=pixSEGHDR->numFlagNames);

	if(GetKeyState(VK_CONTROL)<0) {
		if(!plb->GetSelCount()) plb->SetSel(i,TRUE);
	}
	else if(GetKeyState(VK_SHIFT)<0) {
		DeselectAllBut(m_lastCaret,i);
	}
	else {
		m_lastCaret=i;
		DeselectAllBut(i,i);
	}

	EnableButtons(i);
	//UpdateDisableButton(i);

	MK_FLAGSTYLE *pFS=&m_pFS[m_pFS[i].flgidx];

	//Init flag color --
	m_colorbtn.SetColor(M_COLORRGB(pFS->color));
	m_colorbtn.Invalidate(FALSE);

	//Init flag shade --
	i=M_SYMBOLSHADE(pFS->style);
	switch(i) {
		case FST_SOLID:  i=IDC_MSOLID; break;
		case FST_OPAQUE: i=IDC_MOPAQUE; break;
		case FST_CLEAR:  i=IDC_MCLEAR; break;
		case FST_TRANSPARENT: i=IDC_MTRANSPARENT; break;
		default:  ASSERT(FALSE);
	}
	::CheckRadioButton(m_hWnd,IDC_MSOLID,IDC_MTRANSPARENT,i);

	//Init flag shape --
	i=M_SYMBOLSHAPE(pFS->style);
	if(i>=M_NUMSHAPES) i=CPlotView::m_mfFrame.iFlagStyle;
    VERIFY(CB_ERR!=((CComboBox *)GetDlgItem(IDC_MSHAPE))->SetCurSel(i));

	//Init flag size --
	i=M_SYMBOLSIZE(pFS->style);
	if(i==M_SIZEMASK) i=CPlotView::m_mfFrame.iFlagSize;
    ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_MSIZE))->SetPos(i);

	DrawSymbol();
}

void CMarkerDlg::ChangePriority(int dir)
{
	CListBox *plb=pLB(IDC_LISTFLAGS);
	int i=plb->GetCaretIndex();


	if((dir<0 && i<=0) || (dir>0 && (i+2)>=plb->GetCount())) return;

	ASSERT(m_pFS[m_pFS[i].flgidx].lstidx==i);
	ASSERT(m_pFS[m_pFS[i+dir].flgidx].lstidx==i+dir);

	m_pFS[m_pFS[i].flgidx].lstidx=i+dir;
	m_pFS[m_pFS[i+dir].flgidx].lstidx=i;

	int idxsav=m_pFS[i].flgidx;
	m_pFS[i].flgidx=m_pFS[i+dir].flgidx;
	m_pFS[i+dir].flgidx=idxsav;

	//m_bChanged=TRUE;

	CString cs;
	plb->GetText(i,cs);
	plb->SetRedraw(FALSE);
	plb->DeleteString(i);
	plb->SetRedraw(TRUE);
	plb->InsertString(i+=dir,cs);
	plb->SetCaretIndex(m_lastCaret=i,0);
	DeselectAllBut(i,i);
	EnableButtons(i);
	plb->SetFocus();
	ApplyChange();
}

void CMarkerDlg::OnPriorUp() 
{
	ChangePriority(-1);	
}

void CMarkerDlg::OnPriorDown() 
{
	ChangePriority(1);	
}

BOOL CMarkerDlg::SetFlagStyle(int iVal,int type)
{
	int i=pLB(IDC_LISTFLAGS)->GetCaretIndex();
	ASSERT(i>=0 && i<=(int)pixSEGHDR->numFlagNames);
	WORD style=m_pFS[m_pFS[i].flgidx].style;

	switch(type) {
		case M_SHADEMASK:
			if(iVal==M_SYMBOLSHADE(style)) return FALSE;
			M_SETSHADE(style,iVal);
			break;
		case M_SHAPEMASK:
			if(iVal==M_SYMBOLSHAPE(style)) return FALSE;
			M_SETSHAPE(style,iVal);
			break;
		case M_SIZEMASK:
			if(iVal==M_SYMBOLSIZE(style)) return FALSE;
			M_SETSIZE(style,iVal);
			break;
	}

	m_pFS[m_pFS[i].flgidx].style=style;
	//if(i!=pixSEGHDR->numFlagNames) m_bChanged=TRUE;
	DrawSymbol();
	ApplyChange();
	return TRUE;
}

void CMarkerDlg::OnSolid() 
{
	SetFlagStyle(FST_SOLID,M_SHADEMASK);
}

void CMarkerDlg::OnOpaque() 
{
	SetFlagStyle(FST_OPAQUE,M_SHADEMASK);
}

void CMarkerDlg::OnClear() 
{
	SetFlagStyle(FST_CLEAR,M_SHADEMASK);
}

void CMarkerDlg::OnTransparent() 
{
	SetFlagStyle(FST_TRANSPARENT,M_SHADEMASK);
}

void CMarkerDlg::OnSelShape() 
{
	int i=((CComboBox *)GetDlgItem(IDC_MSHAPE))->GetCurSel();
	ASSERT(i>=0);
	SetFlagStyle(i,M_SHAPEMASK);
}

void CMarkerDlg::OnDeltaposSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	int i=pNMUpDown->iPos+pNMUpDown->iDelta;
	if(i<0) i=255;
	else if(i>255) i=0;
	SetFlagStyle(i,M_SIZEMASK);
	*pResult = 0;
}

BOOL CMarkerDlg::CheckSizeCtrl(void)
{
	int i=pLB(IDC_LISTFLAGS)->GetCaretIndex();
	ASSERT(i>=0 && i<=(int)pixSEGHDR->numFlagNames);
	WORD style=m_pFS[m_pFS[i].flgidx].style;

	i=((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_MSIZE))->GetPos();
	if(i!=M_SYMBOLSIZE(style)) {
		if(i<0 || i>255) ((CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_MSIZE))->SetPos(i=255);
		return SetFlagStyle(i,M_SIZEMASK);
	}
	return FALSE;
}

void CMarkerDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CheckSizeCtrl(); 
	CDialog::OnLButtonDown(nFlags, point);
}

void CMarkerDlg::OnKillfocusSize() 
{
	CheckSizeCtrl(); 
}

void CMarkerDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS) 
{
    if(nIDCtl!=IDC_LISTFLAGS) {
		CDialog::OnDrawItem(nIDCtl,lpDIS);
		return;
	}
    
	if(lpDIS->itemID == (UINT)-1) return;

	if((lpDIS->itemAction&ODA_FOCUS) || (lpDIS->itemAction&ODA_SELECT) ||
	  (lpDIS->itemAction&ODA_DRAWENTIRE)) {
		CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	    CRect rect(&lpDIS->rcItem);

		pDC->SetBkColor((lpDIS->itemState&ODS_SELECTED)?RGB_SKYBLUE:SRV_DLG_BACKCOLOR);
	    pDC->SetTextColor(IsListItemEnabled(lpDIS->itemID)?SRV_DLG_TEXTCOLOR:SRV_DLG_TEXTDISABLE);

		CString cs;
	    pLB(IDC_LISTFLAGS)->GetText(lpDIS->itemID,cs);
		//FixFlagCase((LPSTR)(LPCSTR)cs);
	      
	    pDC->ExtTextOut(rect.left,rect.top,ETO_OPAQUE,&rect,cs,NULL);
	    if(lpDIS->itemState&ODS_FOCUS) pDC->DrawFocusRect(&rect);
 	}
}

void CMarkerDlg::OnDisable() 
{
    CListBox *plb=pLB(IDC_LISTFLAGS);
	BOOL bChg=FALSE;

	for(int i=plb->GetCount()-1;i>0;) {
		if(plb->GetSel(--i) && IsListItemEnabled(i)) {
			bChg=TRUE;
			EnableListItem(i,FALSE);
		}
	}
	if(bChg) {
		//m_bChanged=TRUE;
		plb->Invalidate(FALSE); //Updates listbox? necessary?
		Enable(IDC_DISABLE,FALSE);
		Enable(IDC_ENABLE,TRUE);
		ApplyChange();
	}

	plb->SetFocus();
}

void CMarkerDlg::OnEnable() 
{
    CListBox *plb=pLB(IDC_LISTFLAGS);
	BOOL bChg=FALSE;

	for(int i=plb->GetCount()-1;i>0;) {
		if(plb->GetSel(--i) && !IsListItemEnabled(i)) {
			bChg=TRUE;
			EnableListItem(i,TRUE);
		}
	}
	if(bChg) {
		//m_bChanged=TRUE;
		plb->Invalidate(FALSE); //Updates listbox? necessary?
		Enable(IDC_DISABLE,TRUE);
		Enable(IDC_ENABLE,FALSE);
		ApplyChange();
	}
	plb->SetFocus();
}

void CMarkerDlg::OnHideNotes()
{
	ApplyChange();
}

BOOL CMarkerDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN) {
		if(pMsg->wParam==VK_SPACE && pMsg->hwnd==::GetDlgItem(m_hWnd,IDC_LISTFLAGS)) {
			CListBox *plb=pLB(IDC_LISTFLAGS);
			int i=plb->GetCaretIndex();
			if(i<plb->GetCount()-1) {
				EnableListItem(i,!IsListItemEnabled(i));
				//m_bChanged=TRUE;
				plb->Invalidate(FALSE); //Updates listbox? necessary?
				EnableButtons(-1);
			}
			return TRUE;
		}
    }
    return CDialog::PreTranslateMessage(pMsg);
	//return FALSE;
}


LRESULT CMarkerDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(142);
	return TRUE;
}

int CMarkerDlg::OnVKeyToItem(UINT nKey, CListBox* pLB, UINT nIndex) 
{
	if(nKey==VK_DOWN || nKey==VK_UP) {
		if(GetKeyState(VK_CONTROL)<0) {
			ChangePriority((nKey==VK_UP)?-1:1);
			return -2;
		}
	}
	return -1; //default action
}

void CMarkerDlg::OnSetSelected() 
{
    CListBox *plb=pLB(IDC_LISTFLAGS);
	int i=plb->GetCaretIndex();
	ASSERT(i>=0 && i<=(int)pixSEGHDR->numFlagNames);
	BOOL bChg=0;

	for(int j=plb->GetCount()-1;j>=0;j--) {
		if(j!=i && plb->GetSel(j)) {
			WORD s=m_pFS[m_pFS[i].flgidx].style;
			M_SETHIDDEN(s,M_ISHIDDEN(m_pFS[m_pFS[j].flgidx].style));
			m_pFS[m_pFS[j].flgidx].style=s;
			m_pFS[m_pFS[j].flgidx].color=m_pFS[m_pFS[i].flgidx].color;
			if(j!=pixSEGHDR->numFlagNames) bChg=1;
		}
	}
	if(bChg) {
		//m_bChanged=TRUE;
		ApplyChange();
	}
}
