// traview.cpp : implementation file of Traverse view of CReview tabbed dialog --
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "traview.h"
#include "compview.h"
#include "segview.h"
#include "compile.h"

#include "wall_srv.h" /*For NET_VEC_FIXED*/

#ifndef __NETLIB_H
#include "netlib.h"
#endif

#define wCHAR (CReView::m_ReviewFont.AveCharWidth)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CTraView

IMPLEMENT_DYNCREATE(CTraView, CPanelView)

CTraView::CTraView()
	: CPanelView(CTraView::IDD)
{
	//{{AFX_DATA_INIT(CTraView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CTraView::~CTraView()
{
}

BEGIN_MESSAGE_MAP(CTraView, CPanelView)
	ON_WM_SYSCOMMAND()
	ON_LBN_DBLCLK(IDC_VECLIST,OnVectorDblClk)
	ON_BN_CLICKED(IDC_PREVTRAV, OnPrevTrav)
	ON_BN_CLICKED(IDC_NEXTTRAV, OnNextTrav)
	//{{AFX_MSG_MAP(CTraView)
	ON_WM_SETFOCUS()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_BN_CLICKED(IDC_USEFEET, OnUseFeet)
	ON_COMMAND(ID_MAP_EXPORT, OnMapExport)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFindNext)
	ON_COMMAND(ID_FIND_PREV, OnFindPrev)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREV, OnUpdateFindPrev)
	ON_COMMAND(ID_LOCATEONMAP,OnLocateOnMap)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_VECTORINFO, OnVectoInfo)
	ON_COMMAND(ID_GO_EDIT,OnVectorDblClk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CTraView::m_barCol[NUM_GRD_COLS]={GRD_NAME_COL,GRD_ORIG_COL,GRD_CORR_COL};

void CTraView::UpdateFont()
{
	CFont *pFont=&CReView::m_ReviewFont.cf;
	GetDlgItem(IDC_HDR_TRAVDLG)->SetFont(pFont);
    pLB(IDC_VECLIST)->SetFont(pFont);
    Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CTraView message handlers
void CTraView::OnInitialUpdate()
{
    static int boldctls[]={IDC_ST_UVEH,IDC_ST_UVEV,IDC_ST_ORIGINAL,
       IDC_ST_BEST,IDC_USEFEET,IDC_ST_HZLEN,IDC_ST_VTLEN,IDC_ST_ENU,
	   IDC_ST_PREVNEXT,IDC_F_UVEH,IDC_F_UVEV,IDC_HZLENGTH,IDC_VTLENGTH,
	   IDC_CORRECTION,IDC_NEXTTRAV,IDC_PREVTRAV,0};

    CPanelView::OnInitialUpdate();
    GetReView()->SetFontsBold(GetSafeHwnd(),boldctls);
      
    m_buttonNext.SubclassDlgItem(IDC_NEXTTRAV,this);
    m_buttonPrev.SubclassDlgItem(IDC_PREVTRAV,this);
      
    UpdateFont();
    ResetContents();
}

void CTraView::LabelUnits()
{
    ((CButton *)GetDlgItem(IDC_USEFEET))->SetCheck(m_bFeetUnits);
}

void CTraView::ClearContents()
{
    pLB(IDC_VECLIST)->ResetContent();
    m_recNTS=0;
}

void CTraView::ResetContents()
{
    ClearContents();
    m_bFeetUnits=CPrjDoc::m_pReviewNode->IsFeetUnits();
    LabelUnits();
}

void CTraView::SetF_UVE(char *buf,UINT idc)
{
	SetText(idc,buf);
}

void CTraView::SetTotals(double hlen,double vlen,double *cxyz,int sign)
{
	char buf[64];
	double scale=(m_bFeetUnits?3.28084:1.0);
	sprintf(buf,"%-10.1f",scale*hlen);
	SetText(IDC_HZLENGTH,buf);
	sprintf(buf,"%-10.1f",scale*vlen);
	SetText(IDC_VTLENGTH,buf);
	hlen=scale*sqrt(cxyz[0]*cxyz[0]+cxyz[1]*cxyz[1]);
	scale*=sign;

	_snprintf(buf,64,"%.1f  %.1f  %.1f  (Hz: %.1f)",scale*cxyz[0],scale*cxyz[1],scale*cxyz[2],hlen);
    SetText(IDC_CORRECTION,buf);
}

void CTraView::OnVectorDblClk()
{
    CListBox *plb=pLB(IDC_VECLIST);
    int i=plb->GetCurSel();
    
	if(i>=0) CPrjDoc::LoadVecFile((int)plb->GetItemData(i));
}

void CTraView::OnSetFocus(CWnd* pOldWnd)
{
	GetDlgItem(IDC_VECLIST)->SetFocus();
}

void CTraView::OnSysCommand(UINT nChar,LONG lParam)
{
    //Handle keystrokes here --
    
 	CPanelView::OnSysCommand(nChar,lParam);
}

void CTraView::EnableButtons(int iTrav)
{
	CListBox *plb=(CListBox *)pCV->GetDlgItem(IDC_SEGMENTS);
	GetDlgItem(IDC_PREVTRAV)->EnableWindow(iTrav>0);
	GetDlgItem(IDC_NEXTTRAV)->EnableWindow(plb->GetCount()-1>iTrav);
}

void CTraView::OnPrevTrav()
{
	CListBox *plb=(CListBox *)pCV->GetDlgItem(IDC_SEGMENTS);
	int i=plb->GetCurSel();
	
	if(i>0) {
	  plb->SetCurSel(--i);
	  int iSys=((CListBox *)pCV->GetDlgItem(IDC_SYSTEMS))->GetCurSel();
	  CPrjDoc::SetTraverse(iSys,i);
	  pCV->OnTravChg(); //update Float button and m_iStrFlag flags
	  GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT);
	}
}

void CTraView::OnNextTrav()
{
 	CListBox *plb=(CListBox *)pCV->GetDlgItem(IDC_SEGMENTS);
	int i=plb->GetCurSel();
	if(i<plb->GetCount()-1) {
	  int iSys=((CListBox *)pCV->GetDlgItem(IDC_SYSTEMS))->GetCurSel();
	  plb->SetCurSel(++i);
	  CPrjDoc::SetTraverse(iSys,i);
	  pCV->OnTravChg(); //update Float button and m_iStrFlag flags
	  GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT);
	}
}

static void NEAR PASCAL DrawGrid(CDC *pDC,CRect *pRect)
{
     int x;
     int ybot=pRect->bottom-1;
     
     CPen *oldpen=(CPen *)pDC->SelectObject(&CReView::m_PenGrid);
     
     //First, draw the right-bounding field bars --
     x=pRect->left+wCHAR*CTraView::GRD_LABEL_COL-1;
     for(int i=0;i<CTraView::NUM_GRD_COLS;i++) {
       x+=wCHAR*CTraView::m_barCol[i];
       pDC->MoveTo(x,pRect->top);
       pDC->LineTo(x,ybot);
     }
     
     x=pRect->left+wCHAR*CTraView::GRD_LABEL_COL;
     
     //Draw horizontal line at row's bottom --
     pDC->MoveTo(x,ybot);
     pDC->LineTo(pRect->right,ybot);
     
     //Draw the label's right border and bottom as a darker line --
     pDC->SelectObject(&CReView::m_PenBorder); 
     pDC->MoveTo(x,pRect->top);
     pDC->LineTo(x,ybot);
     pDC->LineTo(pRect->left,ybot);

     pDC->SelectObject(oldpen); 
}

static char * NEAR PASCAL FltStr(char *buf,double d,int len,int dec)
{
	sprintf(buf,"%*.*f",len,dec,d);
	if(strlen(buf)>(size_t)len) strcpy(buf,dec==1?"***.*":"***.**");
	return buf;
}

void CTraView::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(nIDCtl==IDC_VECLIST);
	vecdata vecd;
	char buffer[128];
	double dLen;
	BOOL bFixed;
	
	//listbox will always have items -- 
	if(lpDIS->itemID == (UINT)-1) return;
	
	int selChange   = lpDIS->itemAction&ODA_SELECT;
	int drawEntire  = lpDIS->itemAction&ODA_DRAWENTIRE;

	if(selChange || drawEntire) {
	
		CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	    CRect rect(&lpDIS->rcItem);
	    
		if(drawEntire) {
			if(!CPrjDoc::GetVecData(&vecd,(int)lpDIS->itemData)) return;

			bFixed=vecd.pvec->flag&NET_VEC_FIXED;
		  
		    pDC->SetBkColor(SRV_DLG_BACKCOLOR);
		    pDC->SetTextColor(SRV_DLG_TEXTCOLOR);
		    
		    memcpy(buffer,vecd.pvec->nam[0],NET_SIZNAME);
		    buffer[NET_SIZNAME]=' ';
		    memcpy(buffer+NET_SIZNAME+1,vecd.pvec->nam[1],NET_SIZNAME);
		    
		    int x=rect.left+wCHAR*(GRD_LABEL_COL+1);
		    
		    pDC->ExtTextOut(x,rect.top+1,ETO_OPAQUE,&rect,buffer,2*NET_SIZNAME+1,NULL);
		    
		    x+=wCHAR*(2*NET_SIZNAME+1);
		      
		    for(int i=0;i<3;i++) {
		      dLen=vecd.sp[i];
		      if(!i && m_bFeetUnits) dLen*=3.28084;
		      if(vecd.sp_index==i) pDC->SetTextColor(SRV_DLG_TEXTHIGHLIGHT);
		      pDC->ExtTextOut(x,rect.top+1,0,&rect,FltStr(buffer,dLen,8,2),8,NULL);
		      if(vecd.sp_index==i) pDC->SetTextColor(SRV_DLG_TEXTCOLOR);
		      x+=wCHAR*8;
		    }
		    
		    for(int i=0;i<3;i++) {
		      dLen=vecd.e_sp[i];
		      if((!i || bFixed) && m_bFeetUnits) dLen*=3.28084;
		      if(vecd.sp_index==i) pDC->SetTextColor(SRV_DLG_TEXTHIGHLIGHT);
		      pDC->ExtTextOut(x,rect.top+1,0,&rect,FltStr(buffer,dLen,7,1),7,NULL);
		      if(vecd.sp_index==i) pDC->SetTextColor(SRV_DLG_TEXTCOLOR);
		      x+=wCHAR*7;
		    }
		     
			//Draw label portion (survey name and line number) --
			{
				int rt=rect.right;
				sprintf(buffer,"%*u",GRD_LABEL_COL-1,vecd.pvec->lineno);
				memcpy(buffer,vecd.pvec->filename,8);
				rect.top++;
				rect.right=wCHAR*GRD_LABEL_COL;
		        pDC->SetBkColor(::GetSysColor(COLOR_BTNFACE));
		        pDC->SetTextColor(RGB_BLACK);
		        pDC->ExtTextOut(rect.left,rect.top,ETO_OPAQUE,&rect,buffer,GRD_LABEL_COL-1,NULL);
	    	    rect.top--;
				rect.right=rt;
			} 
		    
			//Draw grid lines --
			DrawGrid(pDC,&rect);
	    }
	    if(selChange || lpDIS->itemState&ODS_SELECTED) {
			//rect.right=wCHAR*GRD_LABEL_COL; ???
			pDC->PatBlt(rect.left,rect.top,wCHAR*GRD_LABEL_COL,rect.Height(),DSTINVERT);
	    }
 	}
}

void CTraView::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	ASSERT(nIDCtl==IDC_VECLIST);
	ASSERT(CReView::m_bFontScaled);
	lpMIS->itemHeight = CReView::m_ReviewFont.LineHeight+2;
}

void CTraView::OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint)
{
	//No updating from document for now
}

void CTraView::OnUseFeet()
{
	m_bFeetUnits=!m_bFeetUnits;
	
	int i=((CListBox *)pCV->GetDlgItem(IDC_SEGMENTS))->GetCurSel();
	if(i>=0) {
	  int iSys=((CListBox *)pCV->GetDlgItem(IDC_SYSTEMS))->GetCurSel();
	  m_recNTS=-1;
	  CPrjDoc::SetTraverse(iSys,i);
	}
	GetDlgItem(IDC_VECLIST)->SetFocus();	
}

void CTraView::OnMapExport() 
{
	if(pSV->InitSegTree()) pSV->OnMapExport();
}

void CTraView::OnEditFind() 
{
	pCV->OnEditFind();
	if(CPrjDoc::m_iFindVector) pCV->SetTraverse();
}

void CTraView::OnFindNext() 
{
	pCV->OnFindNext();
	ASSERT(CPrjDoc::m_iFindVector);
	if(CPrjDoc::m_iFindVector) pCV->SetTraverse();
}

void CTraView::OnUpdateFindNext(CCmdUI* pCmdUI) 
{
	pCV->OnUpdateFindNext(pCmdUI);
}

void CTraView::OnFindPrev() 
{
	pCV->OnFindPrev();
	ASSERT(CPrjDoc::m_iFindVector);
	if(CPrjDoc::m_iFindVector) pCV->SetTraverse();
}

void CTraView::OnUpdateFindPrev(CCmdUI* pCmdUI) 
{
	pCV->OnUpdateFindPrev(pCmdUI);
}

void CTraView::OnLocateOnMap() 
{
    CListBox *plb=pLB(IDC_VECLIST);
    int tgt=plb->GetCurSel();
    
	if(tgt<0) return;
	tgt=(int)plb->GetItemData(tgt);
	if(!dbNTV.Go(abs(tgt))) {
		tgt=CPrjDoc::GetVecPltPosition();
		if(tgt) {
			CPrjDoc::m_iFindStation=0;
			CPrjDoc::m_iFindVector=tgt;
			CPrjDoc::m_iFindVectorCnt=CPrjDoc::m_iFindVectorIncr=CPrjDoc::m_iFindVectorMax=1;
			//If vec_id is odd, it corresponds to the FROM station in the dbNTV record,
			//in which case we'll set m_iFindVectorDir=0.
			//If !m_iFindVectorDir, a Prev() is required for positioning pPLT at TO station.
			CPrjDoc::m_iFindVectorDir=((pPLT->vec_id&1)==0);
			CPrjDoc::LocateOnMap();
		}
	}
}

void CTraView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CListBox *plb=pLB(IDC_VECLIST);
	if(plb!=pWnd) return;
	ASSERT(pWnd->GetDlgCtrlID()==IDC_VECLIST);

	CPoint pt(point);
	plb->ScreenToClient(&pt);

	BOOL bOutside;
	int pos=plb->ItemFromPoint(pt,bOutside);

	if(!bOutside) {
		VERIFY(plb->SetCurSel(pos)!=LB_ERR);

		pos=(int)plb->GetItemData(pos);
		if(dbNTV.Go(abs(pos))) {
			ASSERT(FALSE);
			return;
		}

	}

	CMenu menu;
	if(menu.LoadMenu(bOutside?IDR_GEOM_CONTEXT:IDR_TRAV_CONTEXT))
	{
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		if(!bOutside) {
			CString sTitle("Edit  ");
			CPrjDoc::AppendVecStationNames(sTitle);
			sTitle+="\tDblClk";
			pPopup->ModifyMenu(0,MF_BYPOSITION|MF_STRING,ID_GO_EDIT,sTitle);
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,pWnd->GetParentFrame());
	}
}

void CTraView::OnVectoInfo() 
{
    CListBox *plb=pLB(IDC_VECLIST);
    int tgt=plb->GetCurSel();
    
	if(tgt<0) return;
	tgt=(int)plb->GetItemData(tgt);
	if(!dbNTV.Go(abs(tgt)) && CPrjDoc::GetVecPltPosition()) {
		CPrjDoc::VectorProperties();
	}
}
