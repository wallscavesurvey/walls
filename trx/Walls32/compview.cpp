// compview.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "lineview.h"
#include "revframe.h"
#include "compview.h"
#include "plotview.h"
#include "traview.h"
#include "segview.h"
#include "netfind.h"
#include "compile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CCompView

IMPLEMENT_DYNCREATE(CCompView, CPanelView)

BYTE * CCompView::m_pNamTyp=NULL;
NTA_FLAGSTYLE * CCompView::m_pFlagStyle=NULL;
POINT * CCompView::m_pNTWpnt=NULL;
NTW_POLYGON * CCompView::m_pNTWply=NULL;
NTW_HEADER CCompView::m_NTWhdr;

CCompView::CCompView()
	: CPanelView(CCompView::IDD)
{
	m_pNamTyp=NULL;
	m_pFlagStyle=NULL;
	m_pNTWpnt=NULL;
	m_nLastNetwork=-2;
	iFindStr=0;
	m_pOtherPanel=NULL;
	m_pBaseName=NULL;
	m_iBaseNameCount=0;
	m_bBaseName_filled=FALSE;
}

CCompView::~CCompView()
{
	if(pCV==this) {
	  ASSERT(CPrjDoc::m_pReviewDoc==GetDocument());
	  if(pSV) pSV->ResetContents(); //Saves segment changes
	  CPrjDoc::m_pReviewDoc->CloseWorkFiles();
	  CPrjDoc::m_pReviewDoc=NULL;
	  pCV=NULL;
	  CPrjDoc::m_bComputing=FALSE;
	  CPrjDoc::m_pReviewNode=NULL;
	}
	free(m_pNamTyp);
	free(m_pFlagStyle);
	free(m_pNTWpnt);
	free(m_pBaseName);
}

BEGIN_MESSAGE_MAP(CCompView, CPanelView)
	ON_WM_SYSCOMMAND()
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_WM_CONTEXTMENU()

	ON_LBN_DBLCLK(IDC_SURVEYS,OnSurveyDblClk)
	ON_LBN_DBLCLK(IDC_SEGMENTS,OnTravDblClk)
	ON_LBN_SELCHANGE(IDC_SYSTEMS,OnSysChgSel)
	ON_LBN_SELCHANGE(IDC_NETWORKS,OnNetChg)
	ON_LBN_SELCHANGE(IDC_ISOLATED,OnIsolChgSel)
	ON_LBN_SELCHANGE(IDC_SEGMENTS,OnTravChgSel)
	
	ON_BN_CLICKED(IDCANCEL, OnFinished)
	ON_BN_CLICKED(ID_COMPLETED, OnCompleted)
	ON_BN_CLICKED(IDC_HOMETRAV, OnHomeTrav)
	ON_BN_CLICKED(IDC_FLOATH, OnFloatH)
	ON_BN_CLICKED(IDC_FLOATV, OnFloatV)

	ON_COMMAND(ID_SURVEYDBLCLK, OnSurveyDblClk)
	ON_COMMAND(ID_TRAVDBLCLK, OnTravDblClk)
	ON_COMMAND(ID_MAP_EXPORT, OnMapExport)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_UPDATE_COMMAND_UI(ID_FIND_NEXT, OnUpdateFindNext)
	ON_COMMAND(ID_FIND_PREV, OnFindPrev)
	ON_UPDATE_COMMAND_UI(ID_FIND_PREV, OnUpdateFindPrev)
	ON_COMMAND(ID_TRAV_TOGGLE, OnTravToggle)
	ON_UPDATE_COMMAND_UI(ID_TRAV_TOGGLE, OnUpdateTravToggle)
END_MESSAGE_MAP()

void CCompView::UpdateFont()
{
	static UINT fixedTextCtls[]={
		IDC_SURVEYS,IDC_SURVEYS,IDC_TOTALS,IDC_NETWORKS,
		IDC_SYSTEMS,IDC_SEGMENTS,IDC_SYSTTL,IDC_LOOPS,
		IDC_LOOPSH,IDC_LOOPSV,IDC_ISOLATED,
		IDC_HDR_COMPONENTS,IDC_HDR_SYSTEMS,IDC_HDR_TRAVERSES};

	//We come here before CTraView::UpdateFont() is called --
	if(!CReView::m_bFontScaled) ScaleReviewFont();

	CFont *pFont=&CReView::m_ReviewFont.cf;

	for(int i=0;i<sizeof(fixedTextCtls)/sizeof(UINT);i++)
		GetDlgItem(fixedTextCtls[i])->SetFont(pFont);

	Invalidate();
}

void CCompView::ScaleReviewFont()
{
	CRect rc( 0, 0, 129, 8 );
	VERIFY(::MapDialogRect(m_hWnd,&rc));

	LOGFONT lf;
	PRJFONT *pf=&CReView::m_ReviewFont;
	pf->cf.GetLogFont(&lf);
	//Let's scale the font so that at least 27 chars will
	//fit in the Traverses listbox's rectangle
	lf.lfHeight=(pf->LineHeight*rc.right)/(pf->AveCharWidth*27); //Maintain aspect ratio
	pf->cf.DeleteObject();
	VERIFY(pf->cf.CreateFontIndirect(&lf));
	pf->CalculateTextSize();

#if 0
	{
		//Due to an undocumented quirk, we must recreate the font with a
		//*negative* variant of lfHeight in order for the CHOOSEFONT dialog
		//to initialize properly!
		CDC dc;
		TEXTMETRIC tm;
		dc.CreateCompatibleDC(NULL);
		CFont *pOldFont=dc.SelectObject(&pf->cf);
		dc.GetTextMetrics(&tm);
		dc.SelectObject(pOldFont);
		lf.lfHeight=-(lf.lfHeight-tm.tmInternalLeading);
		pf->LineHeight=tm.tmHeight;
		pf->AveCharWidth=tm.tmAveCharWidth;
		pf->cf.DeleteObject();
		VERIFY(pf->cf.CreateFontIndirect(&lf));
	}
#endif
	CReView::m_bFontScaled=TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCompView message handlers
void CCompView::OnInitialUpdate()
{
    static int boldctls[]={IDC_ST_COMPONENTS,IDC_ST_SYSTEMS,IDC_ST_SEGMENTS,
       IDC_ST_TOTALS,IDC_ST_SURVEYS,IDC_ST_ISOLATED,IDC_ST_UVE,IDC_ST_LOOPS,0};

    CPanelView::OnInitialUpdate();
  
    pCV=this;
	m_nLastNetwork=-2;

    //Lets set the initial window size and position based on the size
    //of the document. We actually do this by resizing the parent frame
    //which has not yet been activated or made visible. (This will be done
    //by ActivateFrame(), which is one of the last operations of the
    //template's OpenDocumentFile()). 
    
	ResizeParentToFit(FALSE);  //FALSE arg will avoid shrinking view
	
	CRevFrame *pRevFrame=(CRevFrame *)GetParentFrame();
	ASSERT(pRevFrame && pRevFrame->IsKindOf(RUNTIME_CLASS(CRevFrame)));
	
	{
	  CRect rect;
	
	  pRevFrame->GetWindowRect(&rect);
	  pRevFrame->m_ptFrameSize.x=rect.Width();
	  pRevFrame->m_ptFrameSize.y=rect.Height();
	}
	  
    UpdateFont();
    GetReView()->SetFontsBold(GetSafeHwnd(),boldctls);

    m_bLoopsShown=FALSE;
    ResetView();
    pRevFrame->CenterWindow();
}

void CCompView::SetTotals(UINT n,double d)
{
    char buf[80];
    strcpy(buf,"Name    Vectors   Length(m)");
    if(LEN_ISFEET()) strcpy(buf+25,"ft)");
    GetDlgItem(IDC_HDR_COMPONENTS)->SetWindowText(buf);
    sprintf(buf,"%8u%10.1f",n,LEN_SCALE(d));
    GetDlgItem(IDC_TOTALS)->SetWindowText(buf);
}

void CCompView::GetBaseNames()
{
	//GetBaseNames from listbox!
	if(m_pBaseName) {
		ASSERT(m_bBaseName_filled);
		ASSERT(pLB(IDC_NETWORKS)->GetCount()==m_iBaseNameCount);
		return;
	}
	ASSERT(!m_bBaseName_filled);
	m_bBaseName_filled=FALSE;
	ASSERT(pLB(IDC_NETWORKS)->GetCount()==m_iBaseNameCount);
	m_pBaseName=(BASENAME *)calloc(m_iBaseNameCount, sizeof(BASENAME));
	ASSERT(m_pBaseName);
	CString csText;
	BASENAME *pName=m_pBaseName;
	for(int i=0; i<m_iBaseNameCount; i++,pName++) {
		pLB(IDC_NETWORKS)->GetText(i,csText);
		memcpy(pName->baseName,(LPCSTR)csText, NET_SIZNAME);
	}
	m_bBaseName_filled=TRUE;
}

void CCompView::SetLoopCount(int nch,int ncv)
{
    char buf[10];
    sprintf(buf,"%u",nch);
    GetDlgItem(IDC_LOOPSH)->SetWindowText(buf);
    sprintf(buf,"%u",ncv);
    GetDlgItem(IDC_LOOPSV)->SetWindowText(buf);
}

void CCompView::GetNetworkBaseName(CString &name)
{
	CString cs;
	int i=pLB(IDC_NETWORKS)->GetCurSel();
	if(i>=0) {
		cs.SetString(m_pBaseName[i].baseName,NET_SIZNAME);
		cs.Truncate(NET_SIZNAME);
		cs.TrimRight();
		if(cs.Compare("<REF>")) {
			name.Format(" (%s)",(LPCSTR)cs);
		}
	}
}

void CCompView::NetMsg(NET_WORK *pNet,int bSurvey)
{
    char buf[80];
    memcpy(buf,pNet->name,NET_SIZNAME);
    //Extend string 6 chars to insure text background goes past right edge of listbox --
    sprintf(buf+NET_SIZNAME,"%7u%10.1f      ",pNet->vectors,LEN_SCALE(pNet->length));
    pLB(bSurvey?IDC_SURVEYS:IDC_NETWORKS)->AddString(buf);
	if(!bSurvey) m_iBaseNameCount++;
}

void CCompView::StatMsg(char *msg)
{
	if(m_bLoopsShown) {
	  Show(IDC_ST_LOOPS,FALSE);
	  Show(IDC_LOOPS,FALSE);
	  Show(IDC_LOOPSH,FALSE);
	  Show(IDC_LOOPSV,FALSE);
	  Show(IDC_STATMSG,TRUE);
	  m_bLoopsShown=FALSE;
	}
    GetDlgItem(IDC_STATMSG)->SetWindowText(msg);
}

void CCompView::AddString(char *msg,UINT idc)
{
    //Extend string 6 chars to insure text background goes past right edge of listbox --
    pLB(idc)->AddString(strcat(msg,"      "));
    if(idc==IDC_ISOLATED) m_bIsol++;
    else if(idc==IDC_SYSTEMS) m_bSys++;
    else if(idc==IDC_SEGMENTS) m_bSeg++;
}

void CCompView::SysTTL(char *msg)
{
	Show(IDC_ST_UVE,*msg!=0);
    GetDlgItem(IDC_SYSTTL)->SetWindowText(msg);
}

void CCompView::LoopTTL(int nch,int ncv)
{
	if(nch+nch<=0) {
		if(!nch) {
		  StatMsg("No loops present.");
		  return;
		}
	}
	else {
		char buf[16];
		sprintf(buf,"%6u%6u",nch,ncv);
		GetDlgItem(IDC_LOOPS)->SetWindowText(buf);
	}
	if(!m_bLoopsShown) {
	  Show(IDC_STATMSG,FALSE);
	  Show(IDC_ST_LOOPS,TRUE);
	  Show(IDC_LOOPS,TRUE);
	  Show(IDC_LOOPSH,TRUE);
	  Show(IDC_LOOPSV,TRUE);
	  m_bLoopsShown=TRUE;
	}
}

void CCompView::DisableFloat()
{
	Show(IDC_LOOPSH,FALSE);
	Show(IDC_LOOPSV,FALSE);
    Show(IDC_FLOATH,FALSE);
    Show(IDC_FLOATV,FALSE);
    Show(IDC_HOMETRAV,FALSE);
    m_iStrFlag=-1;
}

void CCompView::ResetView()
{
	pREV->switchTab(0);
    GetParentFrame()->SetWindowText(NULL);
    m_nLastNetwork=-2;
    m_nLastSystem=m_nLastTraverse=-1;
    m_nFileno=-1;
    m_bIsol=m_bSys=m_bSeg=0;
    ClearListBox(IDC_NETWORKS);
    ClearListBox(IDC_SURVEYS);
    ClearListBox(IDC_SYSTEMS);
    ClearListBox(IDC_ISOLATED);
    ClearListBox(IDC_SEGMENTS);
    SetTotals(0,0.0);
    GetDlgItem(IDC_SYSTTL)->SetWindowText("");
    StatMsg("Processing..");
    Show(IDCANCEL,FALSE);
    DisableFloat();
    m_bHomeTrav=FALSE;
    CPrjDoc::m_bComputing=TRUE;
	if(m_iBaseNameCount || m_pBaseName) {
		free(m_pBaseName);
		m_pBaseName=NULL;
		m_iBaseNameCount=0;
		m_bBaseName_filled=FALSE;
	}
}

void CCompView::ResetContents()
{
	if(m_pNamTyp) {
	  free(m_pNamTyp);
	  m_pNamTyp=NULL;
	}
	if(m_pFlagStyle) {
	  free(m_pFlagStyle);
	  m_pFlagStyle=NULL;
	}
	if(m_pNTWpnt) {
	  free(m_pNTWpnt);
	  m_pNTWpnt=NULL;
	}
	ResetView();
}

void CCompView::BringToTop()
{
	CRevFrame *pFrame=(CRevFrame *)GetParentFrame();
	if(pFrame->IsIconic()) pFrame->ShowWindow(SW_RESTORE);
	else pFrame->BringWindowToTop();
}

void CCompView::OnSurveyDblClk()
{
    CString name;
    int nFileno=pLB(IDC_SURVEYS)->GetCurSel();
    
    if(nFileno<0) return;
	pLB(IDC_SURVEYS)->GetText(nFileno,name);
	if(name.GetLength()>8) name.Truncate(8);
	name.TrimRight();
    ASSERT(CPrjDoc::m_pReviewNode && CPrjDoc::m_pReviewDoc);
	CPrjListNode *pNode=CPrjDoc::m_pReviewNode->FindName(name);
    if(pNode) {
      if(nFileno==m_nFileno) {
	    CLineView::m_nLineStart=CPrjDoc::GetNetLineno();
	    CLineView::m_nCharStart=0;
      }
      CPrjDoc::m_pReviewDoc->OpenSurveyFile(pNode);
    }
}

void CCompView::OnTravDblClk()
{
    pREV->switchTab(TAB_TRAV);
}

BOOL CCompView::SetTraverse()
{
    int i=pLB(IDC_SEGMENTS)->GetCurSel();
    ASSERT(i>=0);
    //CPrjDoc::SetTraverse(-1,i) selects an isolated loop --
    return i>=0 && CPrjDoc::SetTraverse(pLB(IDC_SYSTEMS)->GetCurSel(),i);
}

void CCompView::UpdateFloat(int iSys,int iTrav)
{
   int flg=CPrjDoc::GetStrFlag(iSys,iTrav);
   if(flg!=m_iStrFlag) {
	   bool bNewSys=m_iStrFlag<0;

       //if(bNewSys) m_iStrFlag=~flg; //Update everything
       
	   if(flg!=-1) {
			if(bNewSys || (flg&NET_STR_FLTMSKH)!=(m_iStrFlag&NET_STR_FLTMSKH)) {
				GetDlgItem(IDC_FLOATH)->SetWindowText((flg&NET_STR_FLTMSKH)?"UnfloatH":"FloatH");
				if(bNewSys) Show(IDC_FLOATH,TRUE);
			}
			if(bNewSys || (flg&NET_STR_FLTMSKV)!=(m_iStrFlag&NET_STR_FLTMSKV)) {
				GetDlgItem(IDC_FLOATV)->SetWindowText((flg&NET_STR_FLTMSKV)?"UnfloatV":"FloatV");
				if(bNewSys) Show(IDC_FLOATV,TRUE);
			}

			//Enable floating/unfloating if this is anything but a "hard bridge" --
			Enable(IDC_FLOATH,(flg&NET_STR_BRGFLTH)!=NET_STR_BRIDGEH);
			Enable(IDC_FLOATV,(flg&NET_STR_BRGFLTV)!=NET_STR_BRIDGEV);
	   }
	   else if(bNewSys) {
			Show(IDC_FLOATH,FALSE);
			Show(IDC_FLOATV,FALSE);
	   }
	   m_iStrFlag=bNewSys?-2:flg; //Revised 11/12/04 to fix bug! Selected traverse not drawn correctly.
   }
}

void CCompView::OnTravChg()
{
    UpdateFloat(pLB(IDC_SYSTEMS)->GetCurSel(),m_nLastTraverse=pLB(IDC_SEGMENTS)->GetCurSel());
}

void CCompView::OnTravChgSel()
{
	if(m_nLastTraverse!=pLB(IDC_SEGMENTS)->GetCurSel()) {
		OnTravChg();  //update Float button and m_iStrFlag flags
		//refresh map views where *selected* traverses are shown --
		GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT); //update preview map prior to first refresh
	}
}

void CCompView::PlotTraverse(int iZoom)
{
	int s,t;
	s=pLB(IDC_SYSTEMS)->GetCurSel();
	t=pLB(IDC_SEGMENTS)->GetCurSel();
    CPrjDoc::PlotTraverse(s,t,iZoom);
}

void CCompView::TravToggle()
{
    m_iStrFlag=CPrjDoc::TravToggle(pLB(IDC_SYSTEMS)->GetCurSel(),pLB(IDC_SEGMENTS)->GetCurSel());
    ASSERT(m_iStrFlag&NET_STR_FLTMSK);
}

void CCompView::ShowAbort(BOOL bShow)
{
    if(bShow) {
		Enable(IDCANCEL,TRUE);
	    Show(IDCANCEL,TRUE);
	    GetDlgItem(IDCANCEL)->SetFocus();
    }
    else {
	    GetDlgItem(IDC_NETWORKS)->SetFocus();
		Enable(IDCANCEL,FALSE);
	    Show(IDCANCEL,FALSE);
    }
}

void CCompView::OnCompleted()
{
    //At this point, the workfiles are open, and the IDC_SURVEYS and
    //IDC_NETWORK listboxes have been filled. We need to initialize the other
    //controls with statistics for the first connected network --

    ShowAbort(FALSE);
	pLB(IDC_NETWORKS)->SetCurSel(-1);

	if(m_pBaseName) {
		free(m_pBaseName);
	}
	m_pBaseName=NULL;
	m_bBaseName_filled=FALSE;

	OnNetChg();
    CPrjDoc::m_bComputing=FALSE;


	//See if an SVG is attached as a descendant item. If so, call the SVG DLL to
	//generate an NTW workfile. (The NTW consists of a PolyDraw array representing the
	//w2d_Mask layer.) If successful, flag the NTA to indicate a valid NTW is present.
	if(!CPrjDoc::m_bReviewing) GetDocument()->ProcessSVG();
    
	if(CPrjDoc::m_bSwitchToMap) {
		if(!CPrjDoc::m_bReviewing) {
		  UpdateWindow();
		  StatMsg("Drawing map...");
		  pREV->switchTab(TAB_PLOT);
		  LoopTTL((m_bSys||m_bIsol)?-1:0);
		}
		else pREV->switchTab(TAB_PLOT);
	}
	else {
		VERIFY(GetSegView()->InitSegTree());
	}

	if(CPrjDoc::m_bReviewing) GetParentFrame()->ShowWindow(SW_NORMAL);
}

void CCompView::OnFinished()
{
	GetParentFrame()->SendMessage(WM_SYSCOMMAND,SC_CLOSE);
}

void CCompView::PostClose()
{
	GetParentFrame()->PostMessage(WM_SYSCOMMAND,SC_CLOSE);
}

void CCompView::ClearListBox(UINT idc)
{
    pLB(idc)->EnableWindow(TRUE);
    pLB(idc)->ResetContent();
}

void CCompView::OnNetChg()
{
    int i=pLB(IDC_NETWORKS)->GetCurSel();
    if(i==m_nLastNetwork) return;

	GetBaseNames();

    ClearListBox(IDC_ISOLATED);
    GetDlgItem(IDC_SYSTTL)->SetWindowText("");
    m_bIsol=m_bSys=0;
    
    //Minimize flashing of the System Listbox --
    pLB(IDC_SYSTEMS)->SetRedraw(FALSE); //Works with disabled window?
    ClearListBox(IDC_SYSTEMS);
    
    //If nothing selected, i==-1, in which case the first
    //network with loops will be selected --
    if((m_nLastNetwork=CPrjDoc::SetNetwork(i+1))>=0) {     
      pLB(IDC_NETWORKS)->SetCurSel(m_nLastNetwork);
      pLB(IDC_SURVEYS)->SetCurSel(m_nFileno=CPrjDoc::GetNetFileno());
    }
    else {
      PostClose();
      return;
    }

    pLB(IDC_SYSTEMS)->SetRedraw(TRUE);
    pLB(IDC_SYSTEMS)->Invalidate();
    
    m_nLastSystem=-1;
    if(m_bSys) {
      pLB(IDC_SYSTEMS)->SetCurSel(0);
      OnSysChg();
	  m_nLastTraverse=0;
    }
    else pLB(IDC_SYSTEMS)->EnableWindow(FALSE);
    
    if(m_bIsol) {
      if(!m_bSys) {
        pLB(IDC_ISOLATED)->SetCurSel(0);
        OnIsolChg();
		m_nLastTraverse=0;
	  }
    }
    else {
      if(!m_bSys) {
        m_bSeg=0;
        pLB(IDC_SEGMENTS)->ResetContent();
        pLB(IDC_SEGMENTS)->EnableWindow(FALSE);
		DisableFloat();
      } 
      pLB(IDC_ISOLATED)->EnableWindow(FALSE);
    }
    
    pREV->enableView(TAB_TRAV,m_bSys || m_bIsol);

	((CRevFrame *)GetParentFrame())->SendMessage(WM_SETTEXT,0);

	VERIFY(pSV->InitSegTree());
	PlotTraverse(0);

	GetDocument()->RefreshMaps();

	/*
	if(m_bSys || m_bIsol) {
		GetDocument()->RefreshMapTraverses(TRV_SEL);
	}
	*/

	pSV->SelectSegListNode(0);
}

void CCompView::SetSystem(int i)
{
    CListBox *plb=pLB(IDC_SEGMENTS);
    plb->SetRedraw(FALSE);
    ClearListBox(IDC_SEGMENTS);
    m_bSeg=0;
    CPrjDoc::SetSystem(i);
    if(m_bSeg) {
	    plb->SetCurSel(0);
	    plb->Invalidate();
	}
    plb->SetRedraw(TRUE);
    
    if(!m_bSeg) {
      plb->EnableWindow(FALSE);
	  DisableFloat();
    }
    else {
      Show(IDC_HOMETRAV,TRUE);
      UpdateFloat(i,0);
    }
}

BOOL CCompView::OnSysChg()
{
	//Called by listbox selection, or from OnNetChg(), GoToVector(), OnFloat()??, pPV->IncSys()
    int i=pLB(IDC_SYSTEMS)->GetCurSel();
    if(i<0 || i==m_nLastSystem) return 0;
    SetSystem(m_nLastSystem=i);
    m_nLastTraverse=0;
	pLB(IDC_ISOLATED)->SetCurSel(-1);
	return 1;
}

void CCompView::OnSysChgSel()
{
	if(OnSysChg())
		GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT);
}

BOOL CCompView::OnIsolChg()
{
    ASSERT(pLB(IDC_ISOLATED)->GetCurSel()==0);
    if(m_nLastSystem==-2) return 0;
    m_nLastSystem=-2;
    SetSystem(-1);
	pLB(IDC_SYSTEMS)->SetCurSel(-1);
	return 1;
}

void CCompView::OnIsolChgSel()
{
	if(OnIsolChg())
		GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT);
}

void CCompView::OnSysCommand(UINT nChar,LONG lParam)
{
	//Handle keyboard here --
	
	CPanelView::OnSysCommand(nChar,lParam); //Calls CTabView doSysCommand()
}

void CCompView::OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint)
{
	//No updating from document for now
}

void CCompView::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    if(nIDCtl!=IDC_NETWORKS && nIDCtl!=IDC_SURVEYS && nIDCtl!=IDC_ISOLATED &&
		nIDCtl!=IDC_SYSTEMS && nIDCtl!=IDC_NETWORKS && nIDCtl!=IDC_SEGMENTS) {
      CPanelView::OnDrawItem(nIDCtl,lpDIS);
      return;
    }
    
	if(lpDIS->itemID == (UINT)-1) return;

	if((lpDIS->itemAction&ODA_FOCUS) || (lpDIS->itemAction&ODA_SELECT) ||
	  (lpDIS->itemAction&ODA_DRAWENTIRE)) {
		CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	    CRect rect(&lpDIS->rcItem);

		if(nIDCtl==IDC_SEGMENTS) {
			if(lpDIS->itemID==2) {
				int i=2;
			}
		}
		COLORREF bkclr=SRV_DLG_BACKCOLOR;

		if(lpDIS->itemState&ODS_SELECTED) bkclr=SRV_DLG_BACKHIGHLIGHT;
		else if(nIDCtl==IDC_SEGMENTS) {
			if(nLNKS && piFRG[lpDIS->itemID+nSYS_Offset]) bkclr=RGB(0xe0,0xe0,0xe0);
		}

		pDC->SetBkColor(bkclr);

	    pDC->SetTextColor(
			(nIDCtl==IDC_SURVEYS && lpDIS->itemID==(UINT)m_nFileno)?SRV_DLG_TEXTLOWLIGHT:SRV_DLG_TEXTCOLOR);
	      
	    char buf[80];
		int cnt;

	    if((cnt=pLB(nIDCtl)->GetText(lpDIS->itemID,buf))<0) cnt=0;
	    pDC->ExtTextOut(rect.left,rect.top,ETO_OPAQUE,&rect,buf,cnt,NULL);
	    if(lpDIS->itemState&ODS_FOCUS) pDC->DrawFocusRect(&rect);
 	}
}

void CCompView::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	//ASSERT(nIDCtl==IDC_SURVEYS);
	//We come here before m_ReviewFont is used anywhere else.
	if(!CReView::m_bFontScaled) ScaleReviewFont();
	lpMIS->itemHeight = CReView::m_ReviewFont.LineHeight+2;
}

void CCompView::OnHomeTrav()
{
	m_bHomeTrav=TRUE;
    pREV->switchTab(TAB_PLOT);
    m_bHomeTrav=FALSE;
	if(!pSV->IsTravSelected() || !pSV->IsTravVisible()) {
		pSV->SetTravSelected(TRUE); //View selected vs floated
		pSV->SetTravVisible(TRUE);  //Attach Traverse bullet
		pSV->SetRawTravVisible(TRUE);  //Attach raw raverse bullet
		pPV->SetCheck(IDC_TRAVSELECTED, TRUE);
		pPV->SetCheck(IDC_TRAVFLOATED, FALSE);
	}
	else if(!pSV->IsRawTravVisible()) {
		pSV->SetRawTravVisible(TRUE);  //Attach raw raverse bullet
	}
	else return;
	//refresh all map views --
	GetDocument()->RefreshMapTraverses(0);
}

void CCompView::OnFloat(int i)
{
	//i==0 - float horiz
	//i==1 - float vert

	int iSys=pLB(IDC_SYSTEMS)->GetCurSel();
	int iTrav=pLB(IDC_SEGMENTS)->GetCurSel();
	int iTopTrav=pLB(IDC_SEGMENTS)->GetTopIndex();
	
	pSV->m_recNTN=(UINT)-1;  //Nullify stats
	
	int e=GetDocument()->FloatStr(iSys,iTrav,i);
	if(e) {
	  if(e<0) Close();
	  return;
	}
    e=pLB(IDC_NETWORKS)->GetCurSel();
    
    ClearListBox(IDC_ISOLATED);
    GetDlgItem(IDC_SYSTTL)->SetWindowText("");
    m_bIsol=m_bSys=0;
    
    //Minimize flashing of the System Listbox --
    pLB(IDC_SYSTEMS)->SetRedraw(FALSE); //Works with disabled window?
    ClearListBox(IDC_SYSTEMS);
    
    ASSERT(e>=0);
    
    if((e=CPrjDoc::SetNetwork(e+1))<0) {
      Close();
      return;
    }
    
    ASSERT(e==m_nLastNetwork);     
    
    pLB(IDC_SYSTEMS)->SetRedraw(TRUE);
    pLB(IDC_SYSTEMS)->Invalidate();
    
    m_nLastSystem=-3;
    
    if(iSys>=0) {
       ASSERT(iSys<m_bSys);
       pLB(IDC_SYSTEMS)->SetCurSel(iSys);
       OnSysChg();
    }
    else {
      ASSERT(m_bIsol);
      pLB(IDC_ISOLATED)->SetCurSel(0);
      OnIsolChg();
    }

    pLB(IDC_SEGMENTS)->SetTopIndex(iTopTrav);
    pLB(IDC_SEGMENTS)->SetCurSel(iTrav);

    OnTravChg(); //update Float button and m_iStrFlag flags
    
	pTV->ClearContents();
    
	m_iStrFlag=-2; //When the map panel is next selected, the map will be replotted

	if(!GetSegView()->InitSegTree()) return;
	PlotTraverse(0);
	GetDocument()->RefreshMaps();
}

void CCompView::OnFloatH()
{
	OnFloat(0);
}

void CCompView::OnFloatV()
{
	OnFloat(1);
}

BOOL CCompView::GoToComponent()
{
	int e=0;
	UINT prec=(UINT)dbNTP.Position();
	int nrec=(int)dbNTN.Position();

	ASSERT(nrec==m_nLastNetwork+1);
	ASSERT(prec>0 && nrec>0);
	if(prec>pNTN->plt_id2) 
	  while(!(e=dbNTN.Go(++nrec)) && pNTN->flag!=NET_SURVEY_FLAG && prec>pNTN->plt_id2);
	else if(prec<pNTN->plt_id1)
	  while(!(e=dbNTN.Go(--nrec)) && prec<pNTN->plt_id1);
	if(e || pNTN->flag==NET_SURVEY_FLAG) {
		dbNTN.Go(m_nLastNetwork+1);
		ASSERT(FALSE);
		return FALSE;
	}
	ASSERT(nrec>0);
	if(--nrec!=m_nLastNetwork) {
		pLB(IDC_NETWORKS)->SetCurSel(nrec);
		OnNetChg();
		return 2;
	}
	ASSERT(prec==(UINT)dbNTP.Position());
	return TRUE;
}

void CCompView::GoToVector()
{
	if(!GoToComponent()) return;

	UINT prec;
	int nrec;
	int e=0;

	CPrjDoc::SetVector();

	//Now adjust system and traverse --
	VERIFY(prec=pPLT->str_id);
	if(!dbNTS.Go(prec)) {
	  for(nrec=0;nrec<=nSYS;nrec++) {
		if(piSYS[nrec]>pSTR->str_id) {
		  e=pSTR->str_id-(nrec?piSYS[nrec-1]:0);
		  break;
		}
	  }
	  ASSERT(nrec<=nSYS);
	  if(nrec>=nSYS) nrec=-2;
	  if(nrec!=m_nLastSystem) {
		if(nrec<0) {
		  pLB(IDC_ISOLATED)->SetCurSel(0);
		  OnIsolChg();
		}
		else {
		  pLB(IDC_SYSTEMS)->SetCurSel(nrec);
		  OnSysChg();
		}
	  }
	  if(e!=pLB(IDC_SEGMENTS)->GetCurSel()) {
		int cnt=pLB(IDC_SEGMENTS)->GetCount();
		int sel=pLB(IDC_SEGMENTS)->GetCurSel();
		ASSERT(e>=0 && e<pLB(IDC_SEGMENTS)->GetCount());
		pLB(IDC_SEGMENTS)->SetCurSel(e);
		OnTravChg(); //update Float button and m_iStrFlag flags
	  }
	} //if Go NTS
}

void CCompView::OnEditFind()
{
	CNetFindDlg dlg;

	if(dlg.DoModal()==IDOK) {
		if(pPLT->str_id) {
			DWORD rec=dbNTP.Position();
			GoToComponent(); //may change component (pCV->OnNetChg), update preview map, and refresh map frames!
			VERIFY(!dbNTP.Go(rec));
			CPrjDoc::PositionReview();
		}
		else {
			ASSERT(abs(CPrjDoc::m_iFindVectorIncr)==2);
			//Locate TO station on map --
			CPrjDoc::LocateOnMap();
		}
	}
}

void CCompView::OnFindPrevNext(int iDir)
{
	ASSERT(CPrjDoc::m_iFindVector!=0);
	if(CPrjDoc::m_iFindVectorMax==1) {
		VERIFY(!dbNTP.Go(CPrjDoc::m_iFindVector));
	}
	else {
		char names[SIZ_EDIT2NAMEBUF]; //Max len in dialog is 40
		strcpy(names,hist_netfind.GetFirstString());
		ASSERT(cfg_quotes==0);
		cfg_quotes=0;
		if(cfg_GetArgv(names,CFG_PARSE_ALL)<2 ||
			!CPrjDoc::FindVector(CNetFindDlg::m_bMatchCase,iDir)) {
			ASSERT(FALSE);
			return;
		}
	}
	ASSERT(pPLT->str_id);
	if(pPLT->str_id) {
		ASSERT(abs(CPrjDoc::m_iFindVectorIncr)==1);
		DWORD rec=dbNTP.Position();
		GoToComponent(); //may change component (pCV->OnNetChg), update preview map, and refresh map frames!
		VERIFY(!dbNTP.Go(rec));
		CPrjDoc::PositionReview();
	}
}

void CCompView::OnFindNext()
{
	OnFindPrevNext(1);
}

void CCompView::OnFindPrev() 
{
	OnFindPrevNext(-1);
}

void CCompView::OnUpdateFindNext(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(CPrjDoc::m_iFindVector!=0 && abs(CPrjDoc::m_iFindVectorIncr)<2);
}

void CCompView::OnMapExport() 
{
	pSV->OnMapExport();
}

void CCompView::OnUpdateFindPrev(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CPrjDoc::m_iFindVector!=0 && CPrjDoc::m_iFindVectorMax>1 &&
		abs(CPrjDoc::m_iFindVectorIncr)<2);
}

void CCompView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu menu;
	if(menu.LoadMenu(IDR_GEOM_CONTEXT))
	{
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		int i=pWnd->GetDlgCtrlID();
		if(i==IDC_SURVEYS || i==IDC_SEGMENTS) {
			CPoint pt(point);
			pWnd->ScreenToClient(&pt);
			BOOL bOutside;
			int pos=((CListBox *)pWnd)->ItemFromPoint(pt,bOutside);
			if(!bOutside) {
				pPopup->InsertMenu(0,MF_BYPOSITION|MF_SEPARATOR);
				VERIFY(((CListBox *)pWnd)->SetCurSel(pos)!=LB_ERR);
				if(i==IDC_SURVEYS) {
					pPopup->InsertMenu(0,MF_BYPOSITION|MF_STRING,ID_SURVEYDBLCLK,"Edit survey data\tDblClk");
				}
				else {
					pPopup->InsertMenu(0,MF_BYPOSITION|MF_STRING,ID_TRAVDBLCLK,"Traverse details\tDblClk");
				}
			}
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,pWnd->GetParentFrame());
	}
}

void CCompView::OnTravToggle()
{
	TravToggle();
	GetDocument()->RefreshMapTraverses(TRV_SEL+TRV_PLT);
}

void CCompView::OnUpdateTravToggle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iStrFlag>0 && (m_iStrFlag&(NET_STR_FLOATV|NET_STR_FLOATH))!=0);
}
