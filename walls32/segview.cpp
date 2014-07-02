//segview.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "prjview.h"
#include "compview.h"
#include "segview.h"
#include "plotview.h"
#include "compile.h"
#include "dialogs.h"
#include "listdlg.h"
#include "expavdlg.h"
#include "filebuf.h"
#include "wall_wrl.h"
#include "wall_shp.h"
#include "MarkerDlg.h"
#include "lruddlg.h"
#include "ExpSvgDlg.h"
#include "graddlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#undef _SEGNC

/////////////////////////////////////////////////////////////////////////////
// CSegView

IMPLEMENT_DYNCREATE(CSegView, CPanelView)

void CSegView::DoDataExchange(CDataExchange* pDX)
{
	CPanelView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSegView)
	DDX_Control(pDX, IDC_SEGFRM, m_SegFrm);
	//}}AFX_DATA_MAP
	DDX_Control(pDX,IDC_LINECOLOR,m_colorLines);
	DDX_Control(pDX,IDC_LABELCOLOR,m_colorLabels);
	DDX_Control(pDX,IDC_NOTECOLOR,m_colorNotes);
	DDX_Control(pDX,IDC_MARKERCOLOR,m_colorMarkers);
	DDX_Control(pDX,IDC_BKGCOLOR,m_colorBackgnd);
	DDX_Control(pDX,IDC_FLOORCOLOR,m_colorFloor);
}

BEGIN_MESSAGE_MAP(CSegView, CPanelView)
	ON_CBN_SELCHANGE(IDC_LINESTYLE,OnLineIdxChg)
	ON_LBN_SELCHANGE(2,OnSegmentChg)
    //ON_MESSAGE(CPN_SELENDOK,     OnSelEndOK)
    //ON_MESSAGE(CPN_SELENDCANCEL, OnSelEndCancel)
    ON_MESSAGE(CPN_SELCHANGE,    OnChgColor)
    //ON_MESSAGE(CPN_CLOSEUP,      OnCloseUp)
    //ON_MESSAGE(CPN_DROPDOWN,     OnDropDown)
    ON_MESSAGE(CPN_GETCUSTOM,  OnGetGradient)
    ON_MESSAGE(CPN_DRAWCUSTOM,  OnDrawGradient)
	//{{AFX_MSG_MAP(CSegView)
	ON_WM_SETFOCUS()
	ON_BN_CLICKED(IDC_SEGDETACH, OnSegDetach)
	ON_BN_CLICKED(IDC_NAMES, OnNames)
	ON_BN_CLICKED(IDC_APPLYTOALL, OnApplyToAll)
	ON_BN_CLICKED(IDC_SEGDISABLE, OnSegDisable)
	ON_WM_DRAWITEM()
	ON_WM_MEASUREITEM()
	ON_BN_CLICKED(IDC_DISPLAY, OnDisplay)
	//ON_BN_CLICKED(IDC_EXTERNUPDATE, OnExternUpdate)
	ON_WM_CHAR()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_MARKERS, OnMarks)
	ON_COMMAND(ID_EDIT_LEAF, OnEditLeaf)
	ON_UPDATE_COMMAND_UI(ID_EDIT_LEAF, OnUpdateEditLeaf)
	ON_COMMAND(ID_EDIT_PROPERTIES, OnEditProperties)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PROPERTIES, OnUpdateEditProperties)
	ON_BN_CLICKED(IDC_LRUDSTYLE, OnLrudStyle)
	ON_BN_CLICKED(IDC_DETAILS, OnDetails)
	ON_COMMAND(ID_MAP_EXPORT, OnMapExport)
	ON_BN_CLICKED(IDC_APPLY, OnApply)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_ENTERKEY, OnEnterKey)
	ON_BN_CLICKED(IDC_SYMBOLS, OnSymbols)
    ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, OnFilePrintPreview)
    ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Static initialization/termination

int     CSegView::m_LineHeight;
HBRUSH  CSegView::m_hBrushBkg;
BOOL	CSegView::m_bChanged;
BOOL    CSegView::m_bNewVisibility;
BOOL	CSegView::m_bNoPromptStatus;
GRIDFORMAT * CSegView::m_pGridFormat;
int CSegView::m_iLastFlag=0;
double CSegView::m_fThickPenPts;

static char szTravTitle[2][18] = {{"Floated Traverses"},{"Selected Traverse"}};
static char *szThickPenPts="SvgThickPenPts";

styletyp * CSegView::m_StyleHdr;
int     CSegView::m_PenStyle[SEG_NUM_PENSTYLES]=
          {PS_SOLID,PS_SOLID,PS_DASH,PS_DOT,PS_DASHDOT,PS_DASHDOTDOT,PS_NULL};
          
CSegView::CSegView()
	: CPanelView(CSegView::IDD)
{
	//{{AFX_DATA_INIT(CSegView)
	//}}AFX_DATA_INIT
	m_pRoot=NULL;
	m_hBrushBkg=NULL;
	m_bChanged=m_bGradientChanged=FALSE;
	m_iLastFlag=0;
}

CSegView::~CSegView()
{
	ResetContents();
}

void CSegView::Initialize()
{
	CString str=AfxGetApp()->GetProfileString(CPlotView::szMapFormat,szThickPenPts,"1");
	CSegView::m_fThickPenPts=atof(str);
	if(CSegView::m_fThickPenPts<=0.0 || CSegView::m_fThickPenPts>20.0) CSegView::m_fThickPenPts=1.0;  
}

void CSegView::ReviseLineHeight()
{
    int h=((CMainFrame *)AfxGetApp()->m_pMainWnd)->m_ListBoxRes1.BitmapHeight()+4;
    if(h<CPrjView::m_LineHeight) h=CPrjView::m_LineHeight;
	m_LineHeight=h;
}

///////////////////////////////////////////////////////////////////////////// 
void CSegView::InitOutlinesHdr(SEGHDR *sh)
{
	sh->style[HDR_OUTLINES].SetLineColor(RGB_WHITE);
	sh->style[HDR_OUTLINES].SetLineIdx(PS_SOLID_IDX);
	sh->style[HDR_OUTLINES].SetFloating(TRUE);
}

void CSegView::CopySegHdr_7P(SEGHDR *sh,SEGHDR_7P *sh7)
{
	ASSERT(sh7->version=0x0305);
	memcpy(sh,sh7,4+HDR_OUTLINES*sizeof(styletyp));
	sh->version=NTA_VERSION;
	InitOutlinesHdr(sh);
	sh->gf=sh7->gf;
	sh->vf[0]=sh7->vf[0];
	sh->vf[1]=sh7->vf[1];
}

void CSegView::InitSegHdr(SEGHDR *sh)
{
	//Called by CPrjDoc::Compile() to initialize default segment styles
	//in NTA header when there is no preexisting NTA file (or it's obsolete) --
	sh->version=NTA_VERSION;
	memset(&sh->style,0,SEG_NUM_HDRSTYLES*sizeof(styletyp));
	//sh->style[HDR_BKGND]=SEG_STYLE_BLACK;  //same as 0
	
	sh->style[HDR_GRIDLINES].SetFloating(TRUE);
	sh->style[HDR_GRIDLINES].SetLineIdx(PS_DOT_IDX);
	sh->style[HDR_GRIDLINES].SetLineColor(RGB_GRAY);

	InitOutlinesHdr(sh);

	sh->style[HDR_NOTES].SetLineColor(RGB_WHITE);
	sh->style[HDR_LABELS].SetLineColor(RGB_WHITE);
	sh->style[HDR_FLOOR].SetLineColor(RGB_GRAY);
	sh->style[HDR_FLOOR].SetLrudEnlarge(5);
	sh->style[HDR_FLOOR].SetLrudStyleIdx(0);

	//Leaves char idx==0 --
	sh->style[HDR_MARKERS].SetLineColor(RGB_WHITE);
	sh->style[HDR_MARKERS].SetMarkerSize(CPlotView::m_mfFrame.iMarkerSize);
	sh->style[HDR_MARKERS].SetMarkerShade(FST_SOLID);
	sh->style[HDR_MARKERS].SetMarkerShape(FST_PLUSES);
	
	sh->style[HDR_TRAVERSE].SetFloating(TRUE);
	sh->style[HDR_TRAVERSE].SetLineColor(SRV_PEN_TRAVSELCOLOR);
	sh->style[HDR_RAWTRAVERSE].SetLineColor(SRV_PEN_TRAVRAWCOLOR);
	
	sh->style[HDR_FLOATED].SetUsingOwn(TRUE);
	sh->style[HDR_FLOATED].SetFloating(TRUE);
	sh->style[HDR_FLOATED].SetLineColor(SRV_PEN_FLOATEDCOLOR);
	sh->style[HDR_RAWFLOATED].SetUsingOwn(TRUE);
	sh->style[HDR_RAWFLOATED].SetFloating(TRUE);
	sh->style[HDR_RAWFLOATED].SetLineColor(SRV_PEN_RAWFLOATEDCOLOR);
	
	sh->gf=CPlotView::m_GridFormat;
	if(LEN_ISFEET()) {
		sh->gf.fEastInterval=LEN_INVSCALE(sh->gf.fEastInterval);
		sh->gf.fVertInterval=LEN_INVSCALE(sh->gf.fVertInterval);
		sh->gf.fTickInterval=LEN_INVSCALE(sh->gf.fTickInterval);
	}
	double fWidth,fHeight;
	CWallsApp::GetDefaultFrameSize(fWidth,fHeight,FALSE);
	sh->vf[0].fFrameWidth=sh->vf[1].fFrameWidth=fWidth;
	sh->vf[0].fFrameHeight=sh->vf[1].fFrameHeight=fHeight;
	sh->vf[0].iComponent=sh->vf[1].iComponent=0;

	sh->vf[0].bFlags=sh->vf[1].bFlags=LEN_ISFEET();
	sh->vf[0].bFlags|=(VF_VIEWFLAGS+VF_VIEWNOTES);

	sh->vf[0].fPageOffsetX=sh->vf[1].fPageOffsetX=
		sh->vf[0].fPageOffsetY=sh->vf[1].fPageOffsetY=FLT_MIN;
	sh->vf[0].fOverlap=sh->vf[1].fOverlap=0.0;
}

void CSegView::InitDfltStyle(styletyp *st,SEGHDR *sh)
{
	//Called by CPrjDoc::Compile() to set styles of newly
	//encountered segments. At this point we can assume
	//all bits are zero. We should observe the default background
	//color in the current NTA file header --
	if(sh->style[HDR_BKGND].LineColor()==0L)
	  st->SetLineColor(RGB_BLUEGREEN);   //Bright bluegreen
}

void CSegView::OnInitialUpdate()
{
    static int boldctls[]={IDC_SEGLENGTH,IDC_SEGVECTORS,
       IDC_ST_BRANCH,IDC_ST_SEGMENT,0}; //List of ctls with unchanged fonts
      
    CPanelView::OnInitialUpdate();

    //Set fonts of certain controls to a bold type --
    
    GetReView()->SetFontsBold(GetSafeHwnd(),boldctls);
      
    //Assign a (pointer to) structure of listbox attributes which is already an initialized
    //member of the application's CMainFrame. This function also invalidates the listbox window
    //if it had already been created. (Not so in this case.)
	CMainFrame* mf = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	ASSERT(mf && mf->IsKindOf(RUNTIME_CLASS(CMainFrame)));
	m_SegList.AttachResources(&mf->m_ListBoxRes1);
    ReviseLineHeight();
    m_SegList.SetTextHeight(m_LineHeight);
	
    //NOTE: MFC does not suggest WS_BORDER for a CListBox but it seems necessary
    //to avoid appearance of horizontal lines when scrolling vertically.
    //We will need to call CListBox::SetHorizontalExtent(width) to have
    //horizontal scroll bars.
    
    //Contrary to docs, LBS_MULTIPLESEL treats each left button click as
    //a ctrl-click. Right button toggles select off.
    //LBS_EXTENDEDSEL uses standard ctl-shift mult selection (overrides above).
    //Right button still toggles selection.
    //QUESTION: Why is the last parameter, nID, set at 1? Probably irrelevant.
     
    CRect cr;
    m_SegFrm.GetClientRect(&cr);
    m_SegFrm.ClientToScreen(&cr);
    ScreenToClient(&cr);
    cr.InflateRect(-2,-2);
	SetClassLong(m_SegFrm.m_hWnd,GCL_HBRBACKGROUND,(long)::GetStockObject(GRAY_BRUSH));

    
    //Additional spacing from top to improve appearance of root's title --
    //cr.top+=6;
    
	if( m_SegList.Create(
		WS_CHILD|WS_VISIBLE|/*WS_BORDER|*/
		LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|WS_VSCROLL|WS_TABSTOP|WS_HSCROLL|
		LBS_OWNERDRAWVARIABLE,
		cr, this, 2) == FALSE )	ASSERT(FALSE);
	
	{
	    CComboBox *pList=CBStyle();
	    for(int i=0;i<SEG_NUM_PENSTYLES;i++) pList->AddString(NULL);
    }

	//m_SegList.SetHorizontalExtent(450);
    
    ASSERT(!m_pRoot && !m_hBrushBkg);
    //Enable(IDC_EXTERNUPDATE,CPlotView::m_pMapFrame!=NULL);
	Enable(IDC_APPLY,CPrjDoc::IsActiveFrame());

	m_ToolTips.Create(this);
	m_colorNotes.AddToolTip(&m_ToolTips);
	m_colorLabels.AddToolTip(&m_ToolTips);
	m_colorMarkers.AddToolTip(&m_ToolTips);
	m_colorBackgnd.AddToolTip(&m_ToolTips);
	m_colorFloor.AddToolTip(&m_ToolTips);
	m_colorLines.AddToolTip(&m_ToolTips);
}

void CSegView::OnUpdate(CView *pSender,LPARAM lHint,CObject *pHint)
{
  if(lHint==CPrjDoc::LHINT_FONT) {
     ReviseLineHeight();
     m_SegList.ChangeTextHeight(m_LineHeight);
  }
  //else if(lHint!=CTabView::hintSwitchTo && lHint!=CTabView::hintSwitchFrom)
  //Invalidate();
}

static int SaveSegBranch(CSegListNode *pNode,int iLen)
{
	//Prepare key for this node and set iLen to its length plus 1
	//(SegBuf offset for the next segment name) --
    int e=0;
    
	if(!iLen) {
	  iLen=1;
	  *SegBuf=0;
	}
	else {
	  int len=strlen(pNode->Name());
	  if(iLen>1) SegBuf[iLen++]=SRV_SEG_DELIM;
	  memcpy(SegBuf+iLen,pNode->Name(),len);
	  *SegBuf=(iLen+=len)-1;
	}
	  
	if(pNode->IsChanged() && !(e=ixSEG.Find(SegBuf))) {
	  pNode->SetUnchanged();
	  e=ixSEG.PutRec(pNode->m_pSeg);
	}
	
	if(!e) {
		pNode=pNode->FirstChild();
		while(pNode) {
		   if(e=SaveSegBranch(pNode,iLen)) break;
		   pNode=pNode->NextSibling();
		}
	}
	return e;   
}

void CSegView::SaveTravStyles()
{
	int i=HDR_FLOATED-2*m_bTravSelected;
	m_StyleHdr[i]=m_pTraverse->m_pSeg->style;
	m_StyleHdr[i+1]=m_pRawTraverse->m_pSeg->style;
}

void CSegView::SetTravSelected(BOOL bTravSelected)
{
	  //Called from --
	  //CReview::SwitchTab() when homing,
	  //CSegView::OnSegDisable() when "Change Type" clicked,
	  //CPlotView::OnTravSelected(), and
	  //CPlotView::OnTravFloated()
	  
	  //NOTE: No updating of other views is done here. The segment tree is changed
	  //      but not invalidated.
	  
	  if(bTravSelected==m_bTravSelected) return;
	  
	  SetNewVisibility(TRUE);		  
			  
	  SaveTravStyles();
	  m_bTravSelected=bTravSelected;
	  
	  //Fix node title --
	  m_pTraverse->m_pSeg->m_pTitle=szTravTitle[bTravSelected];
	  
	  int i=HDR_FLOATED-2*bTravSelected;
	  
	  BOOL bStyleFloated=m_StyleHdr[i].IsFloating();
	  
	  m_pTraverse->m_pSeg->style=m_StyleHdr[i];
	  
	  if(m_pTraverse->m_bFloating!=bStyleFloated) {
	  	  m_SegList.FloatNode(2,bStyleFloated);
	  }
	  
	  //m_pTraverse->m_bFloating=(m_pTraverse->m_pSeg->style=m_StyleHdr[i]).IsFloating();
	  m_pRawTraverse->m_bFloating=(m_pRawTraverse->m_pSeg->style=m_StyleHdr[i+1]).IsFloating();
	  
	  m_pTraverse->m_bVisible=!m_pTraverse->m_bFloating;
	  m_pRawTraverse->m_bVisible=m_pTraverse->m_bVisible && !m_pRawTraverse->IsFloating();
	  
	  CSegListNode *pNode=m_SegList.GetSelectedNode();
	  if(pNode==m_pTraverse || pNode==m_pRawTraverse) UpdateDetachButton(pNode);
}

void CSegView::SaveSegments()
{
    int e;
    
    m_bChanged=FALSE;
	if(ixSEG.Opened()) {
	   
      m_StyleHdr[HDR_GRIDLINES]=m_pGridlines->m_pSeg->style;
      m_StyleHdr[HDR_OUTLINES]=m_pOutlines->m_pSeg->style;
     
      SaveTravStyles();
      
	  for(e=0;e<SEG_NUM_HDRSTYLES;e++) m_StyleHdr[e].SetUnchanged();
	  //m_pGridFormat->bChanged=FALSE;
	  
	  ixSEG.MarkExtra();
	      
	  //require exact matches for Find()==0
	  ixSEG.SetExact();
	  if(e=SaveSegBranch(m_pSegments,0)) {
		  ASSERT(0);
	  }
	  if(!e && (e=ixSEG.Flush())) {
		  ASSERT(0);
	  }

	  if(!e) return;
	}
	else e=TRX_ErrOpen;
	   
    CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_TRXSAVE1,ixSEG.Errstr(e));
}

void CSegView::ResetContents()
{
    //Save and free segment tree --
    if(m_pRoot) {
		m_SegList.ResetContent();
		pPV->SaveViews();
		if(m_bChanged) SaveSegments();
		if(m_bGradientChanged) SaveGradients();
		delete [] m_pRoot;
		m_pRoot=NULL;
		for(int n=m_Gradient.GetSize();n>0;) delete m_Gradient[--n];
		m_Gradient.RemoveAll();
    }
    if(m_hBrushBkg) {
      ::DeleteObject(m_hBrushBkg);
      m_hBrushBkg=NULL;
    }
    m_bNoPromptStatus=FALSE;
	m_iLastFlag=0;
	m_bGradientChanged=FALSE;
}

static char *GetSegName(char *name)
{
	//Retrieve terminal name from path in SegBuf[], which is a length
	//prefix string. Truncate name from SegBuf in preparation for GetSegParent --
	int len=SegBuf[0];
	while(len && SegBuf[len]!=SRV_SEG_DELIM) len--;
	len=SegBuf[0]-len;
	if(len) {
	  char *pName=SegBuf+SegBuf[0]-len+1;
	  if(len>=SRV_SIZ_NAMBUF) len=SRV_SIZ_NAMBUF-1;
	  memcpy(name,pName,len);
	  if(pName>SegBuf+1) pName--; //pName==SRV_SEG_DELIM or pName==SegBuf+1
	  SegBuf[0]=pName-SegBuf-1;
	}
	name[len]=0;
	return name;
}

static CSegListNode *GetSegParent(CSegListNode *pNode)
{
	if(!*SegBuf) return pNode;
	
	char *p=trx_Stxpc(SegBuf);
	char *pn;
	
	while(p) {
	  pNode=pNode->FirstChild();
	  if(pn=strchr(p,SRV_SEG_DELIM)) *pn++=0;
	  while(pNode) {
	     if(!strcmp(pNode->Name(),p)) break;
	     pNode=pNode->NextSibling();
	  }
	  if(!pNode) {
	     ASSERT(FALSE); //File corrupted
	     pNode=NULL;
	     break;
	  }
	  p=pn;
    }
    return pNode;
}

CSegListNode *CSegView::AttachHdrNode(int idParent,int idThis,
			styletyp *pStyle,char *pTitle,BOOL bIsSibling)
{
    CSegListNode *pNode=m_pRoot+idThis;
    if(!(pNode->m_pSeg=(segnodptr)malloc(sizeof(segnod)))) return FALSE;
	pNode->Attach(m_pRoot+idParent,bIsSibling); //No error possible here
	//ASSERT(pNode->m_Level==1);
	pNode->m_pSeg->m_pTitle=pTitle;
	pNode->m_pSeg->style=*pStyle;
	pNode->m_pStyleParent=pNode;
	pNode->m_bOpen=FALSE;
	//Traverse segment always starts detached --
	pNode->m_bFloating=(idThis==TREEID_TRAVERSE)?TRUE:pStyle->IsFloating();
	pNode->m_pSeg->style.SetUsingOwn(TRUE);
	pNode->m_bVisible=!pNode->m_bFloating;
    return pNode;
}  

static void PASCAL GetLengths(segnodptr s,double *pF,double *pT)
{
	double s0,s1,s2;
	s0=*pT-*pF;
	s1=pT[1]-pF[1];
	s2=pT[2]-pF[2];
	s0=s0*s0+s1*s1;
	s->m_fHzLength+=sqrt(s0);
	s->m_fVtLength+=fabs(s2);
	s->m_fTtLength+=sqrt(s0+s2*s2);
}	

static void PASCAL ChkRange(segnodptr s,UINT rec,double *pXYZ)
{ 
    pXYZ+=2;
	if(*pXYZ>s->m_fHigh) {
	  s->m_fHigh=*pXYZ;
	  s->m_idHigh=rec;
	}
	if(*pXYZ<s->m_fLow) {
	  s->m_fLow=*pXYZ;
	  s->m_idLow=rec;
	}
}

void CSegView::InitSegStats()
{
    //CMsgBox(MB_ICONEXCLAMATION,"Inside InitSegStats");
    
	BeginWaitCursor();
	int max_id=(int)ixSEG.NumRecs();
	CSegListNode *pNode=m_pSegments;
	segnodptr s;
	
	for(int id=max_id;id;id--,pNode++) {
	    s=pNode->m_pSeg;
	    s->m_fTtLength=s->m_fHzLength=s->m_fVtLength=0.0;
	    s->m_nNumVecs=0;
	    s->m_fHigh=-DBL_MAX;
	    s->m_fLow=DBL_MAX;
	}

	//Assume we are already positioned in dbNTN --
	ASSERT(dbNTN.Opened());
	ASSERT(dbNTP.Opened());
	
    //CMsgBox(MB_ICONEXCLAMATION,"Before Position");
    
	m_recNTN=(UINT)dbNTN.Position();
	
	int lastseg=-2;
	int lastid=0;
	UINT lastrec=(UINT)pNTN->plt_id2;
	double xyz[3];
	
	for(UINT rec=(UINT)pNTN->plt_id1;rec<=lastrec;rec++) {
	  if(dbNTP.Go(rec)) break;
	  if(pPLT->vec_id) {
	     int seg_id=pPLT->seg_id;
	     if(seg_id<max_id) {
		     pNode=m_pSegments+seg_id;
		     pNode->m_pSeg->m_nNumVecs++;
		     if(lastid && pPLT->end_id) GetLengths(pNode->m_pSeg,xyz,pPLT->xyz);
			 if(lastseg!=seg_id) {
			     ASSERT(lastseg>-2);
			     if(lastid) ChkRange(pNode->m_pSeg,rec-1,xyz);
		         lastseg=seg_id;
		     }
		     if(pPLT->end_id) ChkRange(pNode->m_pSeg,rec,pPLT->xyz);
	     }
	     else lastseg=-1;
	  }
	  else lastseg=-1;
	  if(lastid=pPLT->end_id) memcpy(xyz,pPLT->xyz,sizeof(xyz));
	}
 	EndWaitCursor();
    //CMsgBox(MB_ICONEXCLAMATION,"Leaving InitSegStats");
}

void CSegView::SaveGradients()
{
	char *pathName=CPrjDoc::m_pReviewDoc->WorkPath(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_NTAC);
	_strlwr(trx_Stpnam(pathName));

	CFile file;

	if(file.Open(pathName,CFile::modeWrite|CFile::modeCreate|CFile::shareDenyWrite)) {
		CArchive ar(&file,CArchive::store);
		TRY {
			m_Gradient.Serialize(ar);
			return;
		}
		CATCH(CException, pEx) {
			//Show warning message here --
		}
		END_CATCH

		TRY {
			CFile::Remove(pathName);
		}
		CATCH(CException, pEx) {
			//Show warning message here --
		}
		END_CATCH
		return;
	}
	//Show warning message here --
}

BOOL CSegView::LoadGradients()
{
	//Load gradient workfile (.NTAC). If not present, create defaults and
	//intialize both value limits to zero --

	char *pathName=CPrjDoc::m_pReviewDoc->WorkPath(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_NTAC);

	CFile file;

	if(file.Open(pathName,CFile::modeRead|CFile::shareDenyWrite)) {
		CArchive ar(&file, CArchive::load);
		//Check file size here --
		TRY {
			m_Gradient.Serialize(ar);
			file.Close();
			if(m_Gradient.GetSize()==SEG_NUM_GRADIENTS) return TRUE;
		}
		CATCH(CException, pEx) {
			//file.Close(); file handle no longer valid!
			//Show warning message here --
		}
		END_CATCH
	}
    TRY {
		m_Gradient.SetSize(SEG_NUM_GRADIENTS);
		CGradient *pGrad;

		for(int n=0;n<SEG_NUM_GRADIENTS;n++) {
			pGrad = new CGradient;
			CPrjDoc::InitGradient(pGrad,n);
			m_Gradient.SetAt(n,pGrad);
		}
	}
	CATCH(CException, pEx) {
		return FALSE;
	}
	END_CATCH

	return TRUE;
}

BOOL CSegView::InitSegTree()
{
	if(m_pRoot) {
	  if(m_recNTN!=(UINT)dbNTN.Position()) {
	    InitSegStats();
	    UpdateStats(m_SegList.GetSelectedNode());
	  }
	  return TRUE;
	}
	m_recNTN=(UINT)-1;
	
	CSegListNode *pNode;
	char buf[SRV_SIZ_NAMBUF];
	UINT numsegs=0;

	m_pGridlines=m_pOutlines=m_pTraverse=m_pSegments=m_pRawTraverse=NULL;

	ASSERT(ixSEG.Opened() && !m_bChanged);
	{
		SEGHDR *sh=(SEGHDR *)ixSEG.ExtraPtr();
		m_StyleHdr=sh->style;
		m_pGridFormat=&sh->gf;
	}
	UINT max_id=(UINT)ixSEG.NumRecs();
	
	TRY {
	  m_pRoot=new CSegListNode[max_id+SEGNODE_OFFSET];
	}
    CATCH(CMemoryException,e) {
      m_pRoot=NULL;
      goto _errMem;
    }
	END_CATCH

	if(!LoadGradients()) goto _errMem;
	m_Gradient[0]->SetUseFeet(LEN_ISFEET());
	m_Gradient[0]->SetUseLengthUnits(TRUE);
	m_Gradient[1]->SetUseIntValue(TRUE);
	
	if(!(m_pGridlines=AttachHdrNode(TREEID_ROOT,TREEID_GRID,
	       m_StyleHdr+HDR_GRIDLINES,"Grid Lines",FALSE)) ||
	   !(m_pOutlines=AttachHdrNode(TREEID_GRID,TREEID_OUTLINES,
	       m_StyleHdr+HDR_OUTLINES,"Passage Outlines",TRUE)) ||
	   !(m_pTraverse=AttachHdrNode(TREEID_OUTLINES,TREEID_TRAVERSE,
	       m_StyleHdr+HDR_TRAVERSE,szTravTitle[m_bTravSelected=TRUE],TRUE)) ||
	   !(m_pRawTraverse=AttachHdrNode(TREEID_TRAVERSE,TREEID_RAWTRAVERSE,
	       m_StyleHdr+HDR_RAWTRAVERSE,"Unadjusted",FALSE))
	   )
	   goto _errMem;
	
	if(ixSEG.First()) {
		goto _errWork;
	}
	
	do {
	     int e=0;
		 if(ixSEG.GetRec(&SegRec,sizeof(SegRec)) && (e=1) || ixSEG.GetKey(&SegBuf) && (e=2) ||
		   (SegRec.seg_id>=max_id) && (e=3) || (pNode=SegNode(SegRec.seg_id))->m_pSeg && (e=4)) 
		 {
		    goto _errWork;
		 }
		   
		 GetSegName(buf);
			 
		 if(!(pNode->m_pSeg=(segnodptr)malloc(sizeof(segnod)+strlen(buf)))) {
		   goto _errWork;
		 }
		 
		 numsegs++;
			 
		 memcpy(pNode->m_pSeg,&SegRec,sizeof(styletyp));
		 strcpy(pNode->Name(),buf);
			 
		 if(m_pSegments) {
			CSegListNode *pParent=GetSegParent(m_pSegments);
			pNode->SetBranchTitle();
			if(!pParent || pNode->Attach(pParent,FALSE)) {
			  goto _errWork;
			}
		 }
		 else {
		    ASSERT(pNode==SegNode(0));
  			pNode->m_pSeg->m_pTitle=CPrjDoc::m_pReviewNode->Title();
		    pNode->m_pSeg->style.SetUsingOwn(TRUE); //allways true
			if(pNode->Attach(m_pTraverse,TRUE)) {
			  goto _errWork;
			}
		    m_pSegments=pNode;
		 }
			 
		 pNode->m_bOpen=FALSE;
		 pNode->m_bFloating=pNode->m_pSeg->style.IsFloating();
	}
	while(!ixSEG.Next());
	
	if(numsegs!=max_id) {
		goto _errWork;
	}
	
	//Let's initialize component stats --
	InitSegStats();
	    
	SetNewVisibility(TRUE); //Forces reinitialization if pNamTyp[] (see graphics.cpp)
	
	ASSERT(!m_hBrushBkg);
	
	if(!(m_hBrushBkg=::CreateSolidBrush(m_StyleHdr[HDR_BKGND].LineColor()))) {
	   //Why bother?
	   m_hBrushBkg=(HBRUSH)::GetStockObject(WHITE_BRUSH);
	   m_StyleHdr[HDR_BKGND].SetLineColor(RGB_WHITE);
	   m_bChanged=TRUE;
	}
	    
	m_SegList.SetRedraw(FALSE);
	m_SegList.m_uExtent=0;
	    
	//Traverse segment tree, setting m_pStyleParent, m_pTitle, etc. --
	m_pRoot->m_bVisible=TRUE;
	m_pSegments->SetStyleParents(NULL);
	m_pSegments->SetBranchVisibility();
	m_pTraverse->SetStyleParents(NULL);
	m_pRawTraverse->SetBranchVisibility();
	
	m_pRoot->m_bOpen=TRUE;
			  
	if(m_SegList.CHierListBox::InsertNode(m_pRoot,-1)<0) {
	  goto _errWork;
	}
	
	m_SegList.SetCurSel(4); //Also updates buttons
	m_SegList.SetRedraw(TRUE);
    
	//May not need these?
	pPV->SetCheck(IDC_TRAVSELECTED,FALSE);
	pPV->SetCheck(IDC_TRAVFLOATED,FALSE);
	
	pPV->SetCheck(IDC_EXTERNGRIDLINES,IsGridVisible());
	pPV->SetCheck(IDC_WALLS,IsOutlinesVisible());

	m_colorNotes.SetColor(m_StyleHdr[HDR_NOTES].NoteColor());
	m_colorLabels.SetColor(m_StyleHdr[HDR_LABELS].LineColor());
	m_colorMarkers.SetColor(m_StyleHdr[HDR_MARKERS].MarkerColor());
	m_colorBackgnd.SetColor(m_StyleHdr[HDR_BKGND].LineColor());
	m_colorFloor.SetCustomText(5);
	m_colorFloor.SetColor(m_StyleHdr[HDR_FLOOR].LineColorGrad());

	m_SegList.InitHorzExtent();
	
	return TRUE;
	
_errMem:
    ResetContents();
    CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_WORKSPACE);
    return FALSE;
    
_errWork:
    ResetContents();
	CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_BADWORK);
    return FALSE;
}

void CSegView::OnSetFocus(CWnd* pOldWnd)
{
	CPanelView::OnSetFocus(pOldWnd);
	m_SegList.SetFocus();
} 

void CSegView::HdrSegEnable(CSegListNode *pNode,BOOL bEnable)
{
	//Helper for SetTravVisible(), SetGridVisible(), SetOutlinesVisible(), which are invoked
	//from CPlotView(), or CReview::SwitchTab() while the listbox is not displayed --
	
	if(bEnable!=pNode->IsVisible()) {
	
	    BOOL bIsTraverse=pNode==m_pTraverse;
	    
	    ASSERT(pNode->IsVisible()!=pNode->IsFloating());
		pNode->OwnStyle()->SetFloating(!bEnable);
		m_SegList.FloatNode(bIsTraverse?3:((pNode==m_pOutlines)+1),!bEnable);
		pNode->m_bVisible=bEnable;
		if(bIsTraverse) {
		  SetNewVisibility(TRUE);
		  m_pRawTraverse->m_bVisible=bEnable && !m_pRawTraverse->IsFloating();
		}
		MarkSeg(pNode);
	    if(pNode==m_SegList.GetSelectedNode()) UpdateControls(TYP_DETACH); 
	}
}

void CSegView::OnSegDetach()
{
	//Attach/detachment of the selected node -- 
    //Does nothing and returns NULL if this is a level 0 node, otherwise invalidates
    //listbox items whose vertical lines are affected  --
    
    BOOL bVisible=m_SegList.RootDetached(); //To test root detachment change
    
	CSegListNode *pNode =
	  (CSegListNode *)m_SegList.FloatSelection(!m_SegList.IsSelectionFloating());

	if(pNode) {
	  
	  //Refresh the root icon if necessary --
	  if(pNode->Level()==1 && bVisible!=m_SegList.RootDetached())
	    m_SegList.InvalidateItem(0);
	
	  bVisible=!pNode->IsFloating();
	  
	  pNode->OwnStyle()->SetFloating(!bVisible);
	  
	  if(pNode==m_pGridlines) {
	     pPV->SetCheck(IDC_EXTERNGRIDLINES,bVisible);
	     pNode->m_bVisible=bVisible;
	  }
	  else if(pNode==m_pOutlines) {
	     pPV->SetCheck(IDC_WALLS,bVisible);
	     pNode->m_bVisible=bVisible;
	  }
	  else if(pNode==m_pTraverse) {
	     pPV->SetTravChecks(m_bTravSelected,bVisible);
	     pNode->m_bVisible=bVisible;
	     m_pRawTraverse->m_bVisible=bVisible && !m_pRawTraverse->IsFloating();
	     SetNewVisibility(TRUE);
	  }
	  else {
		  //If this node is the first child of a node that is not "using own", then
		  //refresh it also to repair the connecting line --
		  if(pNode!=m_pSegments && pNode->NonOwnersFirstChild()) m_SegList.InvalidateNode(pNode->Parent());
		  
	      if(pNode->Parent()->IsVisible()) {
	        //Map visibility of vectors are affected. The visibility of adjacent
	        //stations will have to be recomputed in CPrjDoc::PlotFrame() --
	        SetNewVisibility(TRUE);
		    pNode->SetBranchVisibility();
		  }
	  }
	  if(pNode!=m_pTraverse) MarkSeg(pNode);
	  UpdateControls(TYP_DETACH);  //not a selection change
	}
}

void CSegView::UpdateStats(CSegListNode *pNode)
{
    double fLength;
    UINT uNumVecs;
    
    //CMsgBox(MB_ICONEXCLAMATION,"pNode=%p m_pSegments=%p",pNode,m_pSegments);
    
    if(pNode<m_pSegments) {
    	uNumVecs=(UINT)-1;
    }
    else if(pNode->IsOpen()) {
	    fLength=pNode->Length();
	    uNumVecs=pNode->NumVecs(); 
	}
	else {
	    fLength=0.0;
	    uNumVecs=0;
        //CMsgBox(MB_ICONEXCLAMATION,"Before Addstats");
	    pNode->AddStats(fLength,uNumVecs);
	}
	
    //CMsgBox(MB_ICONEXCLAMATION,"After AddStats");
    
	if(uNumVecs!=(UINT)-1) {
		char buf[30];
		sprintf(buf,"%u",uNumVecs);
		SetText(IDC_SEGVECTORS,buf); 
		sprintf(buf,"%.1f",LEN_SCALE(fLength));
		SetText(IDC_SEGLENGTH,buf);
		if(!uNumVecs) uNumVecs--;
	}
	else {
		SetText(IDC_SEGVECTORS,0); 
		SetText(IDC_SEGLENGTH,0);
	}
}

void CSegView::EnableControls(CSegListNode *pNode)
{
    BOOL bEnable=pNode!=m_pRoot && pNode->IsUsingOwn();

	if(pNode==m_pSegments || pNode->Level()>1) {
		//Enable(IDC_SEGDISABLE,TRUE);
		SetText(IDC_SEGDISABLE,(bEnable?"Inherit":"Use Own"));
		Enable(IDC_SEGDISABLE,pNode!=m_pSegments);
	}
	else {
		//Enable(IDC_SEGDISABLE,TRUE);
		SetText(IDC_SEGDISABLE,"Change Type");
		Enable(IDC_SEGDISABLE,pNode==m_pTraverse);
	}

	Enable(IDC_DETAILS,pNode>=m_pSegments);
	Enable(IDC_LINESTYLE,bEnable);
	Enable(IDC_LINECOLOR,bEnable);
	if(bEnable) {
		m_colorLines.SetCustomText(pNode>=m_pSegments?5:2);
		m_colorLines.SetColor(pNode->OwnStyle()->LineColorGrad());
		m_colorLines.Invalidate(NULL);
	}
	bEnable=(bEnable && pNode!=m_pGridlines && pNode!=m_pOutlines);
	Enable(IDC_MARKERS,bEnable);
	Enable(IDC_NAMES,bEnable);
	Enable(IDC_APPLYTOALL,pNode!=m_pRoot && !pNode->TitlePtr());
}

void CSegView::UpdateDetachButton(CSegListNode *pNode)
{
	SetText(IDC_SEGDETACH,pNode->IsFloating()?"Attach":"Detach");
}

void CSegView::UpdateControls(UINT uType)
{
 	//Called when selection changes in listbox --
	CSegListNode *pNode=m_SegList.GetSelectedNode();
	if(pNode) {
	    if(uType==TYP_SEL) {
	       if(pNode!=m_pRoot) {
			   styletyp *pStyle=pNode->IsUsingOwn()?pNode->OwnStyle():pNode->Style();
		       SetCheck(IDC_MARKERS,pStyle->IsNoMark());
		       SetCheck(IDC_NAMES,pStyle->IsNoName());
		       //SetCheck(IDC_FLAGS,!pNode->OwnStyle()->IsNoFlag());
		       //SetCheck(IDC_NOTES,!pNode->OwnStyle()->IsNoNote());
			   CBStyle()->SetCurSel(pNode->IsUsingOwn()?pNode->OwnLineIdx():pNode->LineIdx());
		   }
		   else {
		       SetCheck(IDC_MARKERS,FALSE);
		       SetCheck(IDC_NAMES,FALSE);
		       //SetCheck(IDC_NOTES,FALSE);
		       //SetCheck(IDC_FLAGS,FALSE);
		       CBStyle()->SetCurSel(-1);
		   }
	       EnableControls(pNode);
	       
           //CMsgBox(MB_ICONEXCLAMATION,"Before UpdateStats");
	       
	       UpdateStats(pNode);
 	    }
	    //else uType==TYP_DETACH
	    UpdateDetachButton(pNode);
	    Enable(IDC_SEGDETACH,pNode!=m_pRoot);
	}
}

void CSegView::OnSegmentChg()
{
	UpdateControls(TYP_SEL);
}

void CSegView::OnLineIdxChg()
{
    int i=CBStyle()->GetCurSel();
    if(i<0 || i>SEG_NUM_PENSTYLES) return;
	CSegListNode *pNode=m_SegList.GetSelectedNode();
	if(pNode) {
       ASSERT(pNode->IsUsingOwn());
       pNode->m_pSeg->style.SetLineIdx(i);
       MarkSeg(pNode);
       m_SegList.InvalidateNode(pNode);
    }
}

CSegListNode * CSegView::UpdateStyle(UINT flag,BOOL bTyp)
{
	CSegListNode *pNode=m_SegList.GetSelectedNode();
	if(pNode) {
	    WORD FAR *pFlags=pNode->OwnFlags();
	    if(bTyp!=((*pFlags&flag)!=0)) {
	       if(bTyp) *pFlags|=flag;
	       else *pFlags&=~flag;
	       MarkSeg(pNode);
	       SetNewVisibility(TRUE);
	    }
	    else {
	       ASSERT(FALSE);
	    }
	 }
	 return pNode;
}

void CSegView::ApplyBranchStyle(CSegListNode *pNode,CSegListNode *pNodeTgt)
{
    if(pNode!=pNodeTgt && !pNode->TitlePtr() &&
      !strcmp(pNode->Name(),pNodeTgt->Name())) {
      
      //Save original IsUsingOwn status -- 
      BOOL bUsingOwn=pNode->IsUsingOwn();
      
      //Unconditionally replace the style and flag the segment as changed --
	  {
		  WORD seg_id=pNode->m_pSeg->style.seg_id;
		  pNode->m_pSeg->style=pNodeTgt->m_pSeg->style;
		  pNode->m_pSeg->style.seg_id=seg_id;
	  }

      MarkSeg(pNode);
      
      //Revise style parent pointers for this branch
      if(bUsingOwn!=pNode->IsUsingOwn()) {
         pNode->SetStyleParents(bUsingOwn?pNode->Parent()->StyleParent():NULL);
      }
      
      //Get listbox position based on ancestor's open status --
      int index=pNode->GetIndex(); //-1 if parent is closed
      
      if(pNode->IsFloating()!=pNodeTgt->IsFloating()) {
        //We are changing the attached/detached status of this node --
        BOOL bFloating=!pNode->IsFloating();
        if(index>=0) {
            ASSERT(index); //This cannot be the root
	        CRect itemRect;
	        m_SegList.GetItemRect(index,&itemRect);
	        int sibWidth=pNode->SetFloating(bFloating);
	        //If the "last child" status of the previous sibling was changed,
	        //we must also invalidate the displayed lines of that sibling's branch.
	        //Redraw parent to repair connecting line --
	        if(!sibWidth && pNode->NonOwnersFirstChild()) sibWidth++;
	        if(sibWidth) itemRect.top-=itemRect.Height()*sibWidth;
	        m_SegList.InvalidateRect(&itemRect,TRUE); //erase bknd 
	     }
	     else {
	        //Parent node closed (no listbox updating required) --
	        pNode->m_bFloating=bFloating;
	     }
	     pNode->SetBranchVisibility();
      }
      else if(index>=0) m_SegList.InvalidateNode(pNode);
    }
    
    //Now handle changes for children --
    pNode=pNode->FirstChild();
    while(pNode) {
      ApplyBranchStyle(pNode,pNodeTgt);
      pNode=pNode->NextSibling();
    }
}

void CSegView::OnApplyToAll()
{
	CSegListNode *pNode=m_SegList.GetSelectedNode();
	ASSERT(pNode && !pNode->TitlePtr());
	
	if(!m_bNoPromptStatus) {
		char buf[10+SRV_SIZ_NAMBUF];
		_snprintf(buf,10+SRV_SIZ_NAMBUF,"Apply to: %Fs",pNode->Name());
#ifdef _SEGNC
		char c=buf[10]&0x7F;
		if(c) {
		  buf[10]=toupper(c);
	      _strlwr(buf+11);
	    }
#else
		buf[10]&=0x7F;
#endif		
		CApplyDlg dlg(IDD_APPLYTOALL,&m_bNoPromptStatus,buf);
		
		if(dlg.DoModal()!=IDOK) return;
	}

	ApplyBranchStyle(m_pSegments,pNode);
	//For now, don't bother to check if any visibility
	//attributes actually changed --
	SetNewVisibility(TRUE);
}

void CSegView::OnSegDisable()
{
   //Toggles IsUsingOwn() state --
	CSegListNode *pNode=m_SegList.GetSelectedNode();
	if(pNode && (pNode->Level()>1 || pNode==m_pTraverse)) {
		
		if(pNode==m_pTraverse) {
		    //We are changing traverse type --
		    BOOL bDetached=m_SegList.RootDetached();
			SetTravSelected(!m_bTravSelected);
			if(bDetached!=m_SegList.RootDetached()) m_SegList.InvalidateItem(0);
			//Update controls in CPlotView --
			pPV->SetTravChecks(m_bTravSelected,!pNode->m_bFloating);
			if(pNode->IsOpen()) m_SegList.InvalidateNode(m_pRawTraverse);
		}
		else {
			if(!pNode->IsUsingOwn()) {
				ASSERT(pNode->StyleParent());
				//pNode->CopyStyle(pNode->StyleParent());
				pNode->SetUsingOwn(TRUE);
			}
		    else pNode->SetUsingOwn(FALSE);
		    MarkSeg(pNode);
	        pNode->SetStyleParents(pNode->Parent()->StyleParent());
	        //EnableControls(pNode);
			UpdateControls(TYP_SEL);
        }
	    SetNewVisibility(TRUE);
	    
        //Update tree display --
        m_SegList.InvalidateNode(pNode);
   }
}

void CSegView::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    if(nIDCtl!=IDC_LINESTYLE) {
      CPanelView::OnDrawItem(nIDCtl,lpDIS);
      return;
    }
    
	//listbox will always have items --
	int penidx=lpDIS->itemID; 
	if(penidx == (UINT)-1) return;
	
	ASSERT(penidx<SEG_NUM_PENSTYLES);
	
	int selChange   = lpDIS->itemAction&ODA_SELECT;
	int drawEntire  = lpDIS->itemAction&ODA_DRAWENTIRE;

	if(drawEntire) CBStyle()->SetTopIndex(0);

	if(selChange || drawEntire) {
	
		CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	    CRect rect(&lpDIS->rcItem);
	    COLORREF rgb;
	    
	    if(!(lpDIS->itemState&ODS_SELECTED)) rgb=GetSysColor(COLOR_BTNFACE);
	    else rgb=(lpDIS->itemState&ODS_DISABLED)?GetSysColor(COLOR_BTNSHADOW):RGB_SKYBLUE;

	    pDC->SetBkColor(rgb);
	    pDC->ExtTextOut(rect.left,rect.top,ETO_OPAQUE,&rect,"",0,NULL);
  
		if(penidx!=PS_NULL_IDX) {
			#ifdef _USE_MFC_ 
			CPen pen(m_PenStyle[penidx],penidx==PS_HEAVY_IDX?2:1,RGB_DKRED);
			CPen *pOldPen=pDC->SelectObject(&pen);
			int y=(rect.bottom+rect.top)/2;
			pDC->MoveTo(rect.left,y);
			pDC->LineTo(rect.right,y);
			pDC->SelectObject(pOldPen);
			#else
			HPEN pen=::CreatePen(m_PenStyle[penidx],penidx==PS_HEAVY_IDX?2:1,RGB_DKRED);
			HPEN OldPen=(HPEN)::SelectObject(pDC->m_hDC,pen);
			int y=(rect.bottom+rect.top)/2;
			::MoveToEx(pDC->m_hDC,rect.left,y,0);
			::LineTo(pDC->m_hDC,rect.right,y);
			::DeleteObject(::SelectObject(pDC->m_hDC,OldPen));
			#endif
		}
		else {
			pDC->SetTextColor(RGB_DKRED);
			pDC->ExtTextOut(rect.left,rect.top,ETO_OPAQUE,&rect," No lines",9,NULL);
		}
 	}
}

void CSegView::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	if(nIDCtl==IDC_LINESTYLE) {
	    CRect cr;
	    CBStyle()->GetDroppedControlRect(&cr);
	    int itemht=cr.Height()/(SEG_NUM_PENSTYLES+1);
	    lpMIS->itemHeight=itemht;
	}
	else CPanelView::OnMeasureItem(nIDCtl,lpMIS);
}

CPlotView * CSegView::InitPlotView()
{
	if(pPV->m_recNTN!=(int)dbNTN.Position()) pCV->PlotTraverse(0);
	return pPV;
}

void CSegView::OnDisplay()
{
	InitPlotView()->OnExternFrame();	
}

/*
void CSegView::OnExternUpdate()
{
	pPV->OnUpdateFrame();
}
*/

void CSegView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{ 
    if(nFlags>=0 && nChar==VK_RETURN) {
		return;
	}
	
	CPanelView::OnChar(nChar, nRepCnt, nFlags);
}

void CSegView::CountNotes(SEGSTATS *st,UINT segid,BOOL bChkBranch)
{
	NTA_NOTETREE_REC rec;
	int iret=0;
	
	ixSEG.UseTree(NTA_NOTETREE);
	if(ixSEG.NumRecs() && (iret=CPrjDoc::InitNamTyp()) && !ixSEG.First()) {
	  do {
		VERIFY(!ixSEG.GetRec(&rec,sizeof(rec)));
	    if(!M_ISFLAGNAME(rec.seg_id) && (CCompView::m_pNamTyp[rec.id]&M_COMPONENT)) {
	      UINT seg=rec.seg_id&M_SEGMASK;
	      if(seg==segid || bChkBranch && seg>segid &&
	        SegNode(segid)->IsParentOf(SegNode(seg))) { 
		      UINT flags=(rec.seg_id&~M_SEGMASK)>>M_NOTESHIFT;
		      if(flags&M_NOTES) st->numnotes++;
		      else if(flags&M_FLAGS) st->numflags++;
	      }
	    }
	  }
	  while(!ixSEG.Next());
	}
	ixSEG.UseTree(NTA_SEGTREE);
	if(iret>0) SetNewVisibility(TRUE);
}

CSegListNode *CSegView::GetSegStats(SEGSTATS &st)
{
   CSegListNode *pNode=m_SegList.GetSelectedNode();
   if(pNode<m_pSegments) return NULL;
    
	memset(&st,0,sizeof(st));
    if(pNode->TitlePtr()) strcpy(st.title,pNode->TitlePtr());
    else {
       strcpy(st.title,pNode->Name());
       *st.title&=0x7F;
#ifdef _SEGNC
       *st.title=toupper(*st.title);
#endif
    }
	
    if(pNode->IsLeaf() || pNode->IsOpen()) {
      pNode->InitSegStats(&st);
      CountNotes(&st,SegID(pNode),FALSE);
    }
    else {
      strcat(st.title,"/...");
	  st.highpoint=-DBL_MAX;
	  st.lowpoint=DBL_MAX;
      pNode->AddSegStats(&st);
      CountNotes(&st,SegID(pNode),TRUE);
    }
    
    strcpy(st.refstation,"Ref:  ");
	CPrjDoc::GetNetNameOfOrigin(st.refstation+6);
	sprintf(st.component,"Component %u of %d",(UINT)dbNTN.Position(),pCV->NumNetworks());
	if(st.numvecs) {
		//Adjust highpoint and lowpoint --
		if(!dbNTP.Go(st.idlow))
		  GetStrFromFld(st.lowstation,pPLT->name,NET_SIZNAME);
		if(!dbNTP.Go(st.idhigh))
		  GetStrFromFld(st.highstation,pPLT->name,NET_SIZNAME);
	}
	return pNode;
}

void CSegView::OnDetails()
{
	SEGSTATS st;
	CSegListNode *pNode=GetSegStats(st);

	if(!pNode) return;
		
	CStatDlg dlg(&st);
	UINT id=dlg.DoModal();
	
	if(!st.numvecs) return;
	
	if(id==IDC_VECTORLIST) OnLaunchLst(&st,pNode);
	else if(id==IDC_ARCVIEW) {
		if(!CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK)) {
			CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_NONUTMSHP);
			return;
		}
		/*
		else if(LEN_ISFEET()) {
		  CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_GRIDFEET," shapefile");
		  return;
		}
		*/
		CExpavDlg expavdlg(pNode,&st);
	    expavdlg.DoModal();
	}
}

void CSegView::OnFilePrint()
{
	InitPlotView()->OnFilePrint();
}

void CSegView::OnFilePrintPreview()
{
	InitPlotView()->OnFilePrintPreview();
}

void CSegView::OnFileExport()
{
	InitPlotView()->OnFileExport();
}

void CSegView::OnMarks()
{
   UpdateStyle(SEG_NOMARK,GetCheck(IDC_MARKERS)!=0);
}

void CSegView::OnNames()
{
   UpdateStyle(SEG_NONAME,GetCheck(IDC_NAMES)!=0);	
}

HBRUSH CSegView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

    if(nCtlColor==CTLCOLOR_STATIC) {
      switch(pWnd->GetDlgCtrlID()) {
        case IDC_SEGVECTORS:
        case IDC_SEGLENGTH: nCtlColor=CTLCOLOR_EDIT;
      }
    }
    else if(nCtlColor==CTLCOLOR_LISTBOX) {
	    if(pWnd->GetDlgCtrlID()==IDC_LINESTYLE) pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)::GetStockObject(LTGRAY_BRUSH);
    }
	return CPanelView::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CSegView::OnMapExport() 
{
	InitPlotView()->OnMapExport();
}

void CSegView::ApplyClrToFrames(COLORREF clr,UINT id)
{
	COLORREF clrSaved;
	styletyp *pStyle;

	if(id==IDC_LINECOLOR) {
		CSegListNode *pNode=m_SegList.GetSelectedNode();
		if(!pNode || !pNode->IsUsingOwn()) {
			ASSERT(FALSE);
			return;
		}
		pStyle=pNode->OwnStyle();
	}
	else if(id==IDC_FLOORCOLOR) {
		pStyle=&m_StyleHdr[HDR_FLOOR];
	}
	else {
		ASSERT(FALSE);
		return;
	}
	clrSaved=pStyle->LineColorGrad();
	pStyle->SetLineColorGrad(clr);
	CPrjDoc::RefreshActiveFrames();
	pStyle->SetLineColorGrad(clrSaved);
}

LRESULT CSegView::OnChgColor(WPARAM clr,LPARAM id)
{
	styletyp *pStyle;

	ASSERT(id==IDC_LINECOLOR || id==IDC_FLOORCOLOR || !IS_CUSTOM(clr));

	switch(id) {
		case IDC_LINECOLOR:
			{
				CSegListNode *pNode=m_SegList.GetSelectedNode();
				ASSERT(pNode && pNode->IsUsingOwn());
				pStyle=pNode->OwnStyle();
				pStyle->SetLineColorGrad(clr);
				m_SegList.InvalidateNode(pNode);
			}
			break;

		case IDC_FLOORCOLOR:
			pStyle=&m_StyleHdr[HDR_FLOOR];
			pStyle->SetLineColorGrad(clr);
			break;

		case IDC_LABELCOLOR:
			pStyle=&m_StyleHdr[HDR_LABELS];
			pStyle->SetLineColor(clr);
			break;

		case IDC_MARKERCOLOR:
			pStyle=&m_StyleHdr[HDR_MARKERS];
			pStyle->SetMarkerColor(clr);
			break;

		case IDC_BKGCOLOR:
			pStyle=&m_StyleHdr[HDR_BKGND];
			pStyle->SetLineColor(clr);
			if(m_hBrushBkg) ::DeleteObject(m_hBrushBkg);
			m_hBrushBkg=::CreateSolidBrush(clr);
			m_SegList.InvalidateNode(m_pGridlines);
			m_SegList.InvalidateNode(m_pOutlines);
			m_SegList.RefreshBranchOwners(m_pTraverse);
			m_SegList.RefreshBranchOwners(m_pSegments);
			break;

		case IDC_NOTECOLOR:
			pStyle=&m_StyleHdr[HDR_NOTES];
			pStyle->SetNoteColorIdx(clr);
			break;

		default: return TRUE;
	}

	pStyle->SetChanged(); //not relevant for header styles
	m_bChanged=TRUE;
    return TRUE;
}

void CSegView::OnSymbols()
{
	CMarkerDlg dlg;

	dlg.m_bNoOverlap=IsNoOverlap();
	dlg.m_bHideNotes=IsHideNotes();

	if(IDOK==dlg.DoModal()) {
		if(dlg.m_bChanged) SetNewVisibility(TRUE);

		if(dlg.m_bkgcolor!=BackColor()) {
			OnChgColor(dlg.m_bkgcolor,IDC_BKGCOLOR);
			m_colorBackgnd.SetColor(dlg.m_bkgcolor);
		    m_colorBackgnd.Invalidate(NULL);
		}
		if(dlg.m_markcolor!=MarkerColor()) {
			OnChgColor(dlg.m_markcolor,IDC_MARKERCOLOR);
			m_colorMarkers.SetColor(dlg.m_markcolor);
		    m_colorMarkers.Invalidate(NULL);
		}
		if(dlg.m_markstyle!=MarkerStyle()) {
			SetMarkerStyle(dlg.m_markstyle);
			m_bChanged=TRUE;
		}
		if(dlg.m_bNoOverlap!=IsNoOverlap()) {
			SetNoOverlap(dlg.m_bNoOverlap);
			m_bChanged=TRUE;
			SetNewVisibility(TRUE);
		}
		if(dlg.m_bHideNotes!=IsHideNotes()) {
			SetHideNotes(dlg.m_bHideNotes);
			m_bChanged=TRUE;
		}
	}
}

void CSegView::OnLrudStyle() 
{
   double fThickPenPts=m_fThickPenPts;

   CLrudDlg dlg(this,&fThickPenPts);
   if(dlg.DoModal()==IDOK) {
		if(fThickPenPts!=m_fThickPenPts) {
		  AfxGetApp()->WriteProfileString(CPlotView::szMapFormat,szThickPenPts,dlg.m_ThickPenPts);
		  m_fThickPenPts=fThickPenPts;
		}
		if(dlg.m_iLrudStyle!=LrudStyleIdx() || dlg.m_iEnlarge!=LrudEnlarge() ||
			dlg.m_bSVGUseIfAvail!=IsSVGUseIfAvail()) {
		  SetLrudStyleIdx(dlg.m_iLrudStyle);
		  SetLrudEnlarge(dlg.m_iEnlarge);
		  SetSVGUseIfAvail(dlg.m_bSVGUseIfAvail);
		  m_bChanged=TRUE;
		}
   }
}

COLORREF CSegView::SafeColor(COLORREF clr)
{
	if(clr==BackColor()) {
		if(clr!=RGB_BLACK) clr=RGB_BLACK;
		else clr=RGB_WHITE;
	}
	return clr;
}

static void maxDiffStr(CString &sd,double f,LPCSTR idstr,SVG_VIEWFORMAT &svg)
{
	LPCSTR p="";
	CString s;
	if(f<0.0) {
		f=svg.advParams.maxCoordChg;
		p="<";
	}
	else s.Format("  Vector id: %s",idstr);
	sd.Format("%s%.1f m%s",p,f,(LPCSTR)s);
}

void CSegView::OnSvgExport() 
{
    VIEWFORMAT vf;
	SVG_VIEWFORMAT svg;
	BOOL bView;
	
	if(InitPlotView()->m_bProfile) {
		AfxMessageBox(IDS_ERR_SVGPROFILE);
		return;
	}
	/*
	if(LEN_ISFEET() && CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK)) {
		CMsgBox(MB_ICONEXCLAMATION,IDS_ERR_GRIDFEET,"n SVG");
		return;
	}
	*/
	pPV->ClearTracker();
	CPrjDoc::InitViewFormat(&vf);

	/*Custom project-specific symbols? --*/
	//CString sympath=pSV->GetDocument()->ProjectPath();
	//sympath.Replace(PRJ_PROJECT_EXT,"_sym.svgz");
	//svg.sympath=sympath;

	svg.backColor=BackColor();
	svg.passageColor=FloorColor();
	svg.lrudColor=LrudColor();
	svg.noteColor=SafeColor(NoteColor());
	svg.markerColor=SafeColor(MarkerColor());
	svg.labelColor=SafeColor(LabelColor());
	svg.gridColor=m_pGridlines->LineColor();
	svg.vectorColor=m_pSegments->LineColor();
	//Pointsize is actually pt sixe x 10 --
	svg.noteSize=(float)CPlotView::m_mfPrint.pfNoteFont->PointSize;
	svg.labelSize=(float)CPlotView::m_mfPrint.pfLabelFont->PointSize;
	svg.offLabelX=(float)CPlotView::m_mfPrint.iOffLabelX;
	svg.offLabelY=(float)CPlotView::m_mfPrint.iOffLabelY;
	svg.titleSize=(float)CPlotView::m_mfPrint.pfFrameFont->PointSize;
	svg.frameThick=(float)CPlotView::m_mfPrint.iFrameThick;
	svg.fMaxDiff=svg.fMaxDiff2=-1.0f;
	//Other flags are initialized in dialog constructor for now --
	svg.flags=SVG_FEET*LEN_ISFEET()+
		((CPlotView::m_mfPrint.bLblFlags&(2+4+8))!=0)*SVG_SHOWPREFIX;

	CExpSvgDlg svgdlg(&vf,&svg);
	//Get default pathname --
	svgdlg.m_bView=bView=CMainFrame::GetViewerStatus(FALSE);
	svgdlg.m_pathname=CMainFrame::Get3DPathName(CPrjDoc::m_pReviewNode,FALSE);
	CMainFrame::GetMrgPathNames(svgdlg.m_MergePath,svgdlg.m_MergePath2);
	svgdlg.m_bUseMerge2=!svgdlg.m_MergePath2.IsEmpty();

	svgdlg.m_title=CPrjDoc::m_pReviewNode->Title();
	
	if(IDOK==svgdlg.DoModal()) {
		if(bView!=svgdlg.m_bView) CMainFrame::SaveViewerStatus(svgdlg.m_bView,FALSE);
		if(svg.flags&SVG_MERGEDCONTENT) {
			if(svgdlg.m_bUseMerge2) {
				CString sd,sd2;
				maxDiffStr(sd,svg.fMaxDiff,svg.max_diff_idstr,svg);
				maxDiffStr(sd2,svg.fMaxDiff2,svg.max_diff_idstr2,svg);
				CMsgBox(MB_ICONASTERISK,
					"Successfully created SVG with merged content from two files:\n%s\n"
					"\n"
					"1st merged file: %s --\n"
					"Max coord adjustment: %s\n"
					"Total path length of Walls layer: %.0f m (%u pts)\n"
					"Total path length of Detail layer: %.0f m (%u pts)\n"
					"\n"
					"2nd merged file: %s --\n"
					"Max coord adjustment: %s\n"
					"Total path length of Walls layer: %.0f m (%u pts)\n"
					"Total path length of Detail layer: %.0f m (%u pts)",
					svg.svgpath,
					trx_Stpnam(svg.mrgpath),(LPCSTR)sd,svg.flength_w,svg.tpoints_w,svg.flength_d,svg.tpoints_d,
					trx_Stpnam(svg.mrgpath2),(LPCSTR)sd2,svg.flength_w2,svg.tpoints_w2,svg.flength_d2,svg.tpoints_d2);
			}
			else {
				if(svg.fMaxDiff>0.0f)
					CMsgBox(MB_ICONASTERISK,
						"Successfully created SVG with merged content: %s\n\n"
						"A feature adjustment was required since a station's coordinate changed by "
						"%.1f meters, the vector id being %s.\n\n" 
						"Total path length of Walls layer: %.0f m (%u pts)\nTotal path length of Detail layer: %.0f m (%u pts)",
					svgdlg.m_pathname,svg.fMaxDiff,svg.max_diff_idstr,svg.flength_w,svg.tpoints_w,svg.flength_d,svg.tpoints_d);
				else CMsgBox(MB_ICONASTERISK,
						"Successfully created SVG file: %s.\n\n"
						"A feature adjustment wasn't required since no station's coordinate changed by more than %.1f meters.\n"
						"Total path length of Walls layer: %.0f m (%u pts)\nTotal path length of Detail layer: %.0f m (%u pts)",
						svgdlg.m_pathname,svg.advParams.maxCoordChg,
					svg.flength_w,svg.tpoints_w,svg.flength_d,svg.tpoints_d);
			}
		}
		else if(!svgdlg.m_bView) {
			CMsgBox(MB_ICONASTERISK,IDS_SVG_NONMERGED2,(svg.flags&SVG_ADJUSTABLE)?"":"non-",svgdlg.m_pathname);
		}
		if(svgdlg.m_bView) CMainFrame::Launch3D(svgdlg.m_pathname,FALSE);
	}
}

void CSegView::OnLaunchLst(SEGSTATS *pst,CSegListNode *pNode /*=NULL*/)
{
	SEGSTATS st;

	if(!pst) {
		ASSERT(!pNode);
		if(!(pNode=GetSegStats(st)) || !st.numvecs) {
			//No vectors to report --
			return;
		}
		pst=&st;
	}
	ASSERT(pNode && pst->numvecs);
	CListDlg lstdlg(GetDocument()->WorkPath(CPrjDoc::m_pReviewNode,CPrjDoc::TYP_LST),pst->numvecs);
	if(CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK)) lstdlg.m_uFlags|=LFLG_LATLONGOK;
	else lstdlg.m_uFlags&=~LFLG_LATLONGOK;

	int e=lstdlg.DoModal();
	if(e==IDOK) {
		if(lstdlg.m_uFlags&LFLG_OPENEXISTING) {
			theApp.OpenDocumentFile(lstdlg.m_pathname);
		}
		else WriteVectorList(pst,lstdlg.m_pathname,pNode,lstdlg.m_uFlags,lstdlg.m_uDecimals);
	}
}

void CSegView::On3DExport()
{
	//Called from CMainframe --
	CExpDlg exdlg;
	BOOL bView;

	//Get default pathname --
	exdlg.m_bView=bView=CMainFrame::GetViewerStatus(TRUE);
	exdlg.m_pathname=CMainFrame::Get3DPathName(CPrjDoc::m_pReviewNode,TRUE);
	exdlg.m_dScaleVert=1.0;
	
	if(exdlg.DoModal()==IDOK) {
		m_pSegments->FlagVisible(LST_INCLUDE);
		UINT flags=exdlg.m_bElevGrid*WRL_F_ELEVGRID+exdlg.m_bLRUD*WRL_F_LRUD+exdlg.m_bLRUDPT*WRL_F_LRUDPT+WRL_F_COLORS+WRL_F_GRID;
		if(exdlg.m_uDimGrid<1) flags&=~WRL_F_GRID;
		if(ExportWRL(exdlg.m_pathname,m_pSegments->Title(),flags,exdlg.m_uDimGrid,(float)exdlg.m_dScaleVert)) {
			if(bView!=exdlg.m_bView) CMainFrame::SaveViewerStatus(exdlg.m_bView,TRUE);
			if(!exdlg.m_bView || !CMainFrame::Launch3D(exdlg.m_pathname,TRUE))
			CMsgBox(MB_ICONASTERISK,IDS_VRML_NOTIFY1,(const char *)exdlg.m_pathname);
		}
		UnFlagVisible(m_pRoot);
	}
}

void CSegView::OnOK()
{
	ASSERT(FALSE);
}
void CSegView::OnEnterKey() 
{
	ASSERT(FALSE);
}

void CSegView::OnEditLeaf() 
{
	CPrjListNode *pPN=m_SegList.GetSelectedPrjListNode();
	if(pPN && pPN->IsLeaf()) {
		CPrjDoc::m_pReviewDoc->GetPrjView()->OpenLeaf(pPN);
	}
}

void CSegView::OnUpdateEditLeaf(CCmdUI* pCmdUI) 
{
	CPrjListNode *pPN=m_SegList.GetSelectedPrjListNode();
	pCmdUI->Enable(pPN && pPN->IsLeaf());
}

void CSegView::OnEditProperties() 
{
	extern BOOL hPropHook;

	CPrjListNode *pPN=m_SegList.GetSelectedPrjListNode();
	if(pPN) {
		CPrjView *pView=CPrjDoc::m_pReviewDoc->GetPrjView();
		pView->m_PrjList.SelectNode(pPN);
		if(!hPropHook) pView->OpenProperties();
	}
}

void CSegView::OnUpdateEditProperties(CCmdUI* pCmdUI) 
{
	CPrjListNode *pPN=m_SegList.GetSelectedPrjListNode();
	pCmdUI->Enable(pPN!=NULL);
}

void CSegView::RefreshGradients()
{
	m_bGradientChanged=TRUE;
	m_colorFloor.Invalidate();
	m_colorLines.Invalidate();
	//Other color gradient nodes in the tree may need to be repainted -- 
	m_SegList.Invalidate();
}

LONG CSegView::OnGetGradient(UINT wParam,LONG lParam)
{
	COLORREF clr=*(COLORREF *)wParam;
	int idx=CUSTOM_COLOR_IDX(clr);

	ASSERT((UINT)lParam==IDC_LINECOLOR || (UINT)lParam==IDC_FLOORCOLOR);
	ASSERT(IS_CUSTOM(clr) && idx>=0 && idx<SEG_NUM_GRADIENTS && m_Gradient[idx]);

	CGradientDlg dlg((UINT)lParam,clr);

	if(IDOK==dlg.DoModal()) {
		*m_Gradient[idx]=dlg.m_wndGradientCtrl.GetGradient();
		if(dlg.m_bChanged) RefreshGradients();
		return TRUE;
	}
	return FALSE;
}

LONG CSegView::OnDrawGradient(UINT wParam, LONG /*lParam*/)
{
	CUSTOM_COLOR *pCC=(CUSTOM_COLOR *)wParam;
	ASSERT(pCC->pDC && IS_CUSTOM(*pCC->pClr));
	int idx=CUSTOM_COLOR_IDX(*pCC->pClr);
	ASSERT(idx>=0 && idx<SEG_NUM_GRADIENTS && m_Gradient[idx]);
	m_Gradient[idx]->FillRect(pCC->pDC,(CRect *)pCC->pRect);
	return TRUE;
}

void CSegView::OnApply() 
{
	CPrjDoc::RefreshActiveFrames();	
}

BOOL CSegView::HasFloors()
{
	BOOL bRet=TRUE;
	if(!IsSVGAvail() || !IsSVGUseIfAvail()) {
		ixSEG.UseTree(NTA_LRUDTREE);
		if(!ixSEG.NumRecs()) bRet=FALSE;
		ixSEG.UseTree(NTA_SEGTREE);
	}
	return bRet;
}
