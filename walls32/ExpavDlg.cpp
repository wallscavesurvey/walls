// ExpavDlg.cpp : implementation file
//

#include "stdafx.h"
#include "walls.h"
#include "prjdoc.h"
#include "plotview.h"
#include "compile.h"
#include "segview.h"
#include "ExpavDlg.h"
#include "wall_shp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExpavDlg dialog


CExpavDlg::CExpavDlg(CSegListNode *pNode,SEGSTATS *st,CWnd* pParent /*=NULL*/)
	: CDialog(CExpavDlg::IDD, pParent)
{
	CPrjDoc *pDoc=pSV->GetDocument();
	m_uTypes=pDoc->m_uShapeTypes;
	m_b3DType = (m_uTypes&SHP_3D)!=0;
	m_bUTM = (m_uTypes&SHP_UTM)!=0;
	m_bFlags = (m_uTypes&SHP_FLAGS)!=0;
	m_bNotes = (m_uTypes&SHP_NOTES)!=0;
	m_bStations = (m_uTypes&SHP_STATIONS)!=0;
	m_bVectors = (m_uTypes&SHP_VECTORS)!=0;
	m_bWalls = pSV->HasFloors() && (m_uTypes&SHP_WALLS)!=0;

#ifdef _USE_SHPX
	m_bWallshpx_enable=(!LEN_ISFEET() && CPrjDoc::m_pReviewNode->InheritedState(FLG_GRIDMASK));
	m_bWallshpx = m_bWallshpx_enable && (m_uTypes&SHP_WALLSHPX)!=0;
#endif
		//Get default pathname --
	m_pathname=pDoc->ShapeFilePath();
	m_shpbase=pNode->Name();
	if(m_shpbase.IsEmpty()) m_shpbase=CPrjDoc::m_pReviewNode->BaseName();
	m_st=st;
	m_pNode=pNode;
	//{{AFX_DATA_INIT(CExpavDlg)
	//}}AFX_DATA_INIT
}


void CExpavDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExpavDlg)
	DDX_Check(pDX, IDC_SHP_FLAGS, m_bFlags);
	DDX_Check(pDX, IDC_SHP_NOTES, m_bNotes);
	DDX_Check(pDX, IDC_SHP_STATIONS, m_bStations);
	DDX_Check(pDX, IDC_SHP_VECTORS, m_bVectors);
	DDX_Text(pDX, IDC_SHPPATH, m_pathname);
	DDV_MaxChars(pDX, m_pathname, 250);
	DDX_Text(pDX, IDC_SHPBASE, m_shpbase);
	DDV_MaxChars(pDX, m_shpbase, 128);
#ifdef _USE_SHPX
	DDX_Check(pDX, IDC_WALLSHPX, m_bWallshpx);
#endif
	DDX_Check(pDX, IDC_SHP_WALLS, m_bWalls);
	DDX_Check(pDX, IDC_3DTYPE, m_b3DType);
	DDX_Radio(pDX, IDC_LATLONG, m_bUTM);
	//}}AFX_DATA_MAP

	if(pDX->m_bSaveAndValidate) {

		CPrjDoc *pDoc=pSV->GetDocument();

		int i=strlen(m_pathname)-1;
		if(i>=0 && (m_pathname[i]=='\\' || m_pathname[i]=='/')) m_pathname.GetBufferSetLength(i);

		UINT flags=m_uTypes=(m_bFlags*SHP_FLAGS)+(m_bNotes*SHP_NOTES)+
			(m_bStations*SHP_STATIONS)+(m_bVectors*SHP_VECTORS)+(m_bWalls*SHP_WALLS)+
#ifdef _USE_SHPX
			(m_bWallshpx*SHP_WALLSHPX)+
#endif
			(m_b3DType*SHP_3D)+(m_bUTM*SHP_UTM);

		if(!m_st->numnotes) flags&=~SHP_NOTES;
		if(!m_st->numflags) flags&=~SHP_FLAGS;
		if(!(flags&(SHP_STATIONS+SHP_VECTORS+SHP_FLAGS+SHP_NOTES+SHP_WALLS))) {
			AfxMessageBox(IDS_SHP_NOTYPES);
			return;
		}

		//Let's see if the directory needs to be created --
		char buf[MAX_PATH];
		if(!DirCheck(strcat(strcpy(buf,m_pathname),"\\x"),TRUE)) return;

		if(i=pSV->ExportSHP(this,m_pathname,m_shpbase,m_pNode,flags)) {
			pDoc->SetShapeFilePath(m_pathname);
			pDoc->m_uShapeTypes=m_uTypes;
		}

	}
}

BEGIN_MESSAGE_MAP(CExpavDlg, CDialog)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	//{{AFX_MSG_MAP(CExpavDlg)
	ON_BN_CLICKED(IDBROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExpavDlg message handlers

BOOL CExpavDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWndTitle(this,0,m_pNode->Title());
	
	Enable(IDC_SHP_FLAGS,m_st->numflags>0);
	Enable(IDC_SHP_NOTES,m_st->numnotes>0);
#ifdef _USE_SHPX
	Enable(IDC_WALLSHPX,m_bWallshpx_enable);
#endif
	Enable(IDC_SHP_WALLS,pSV->HasFloors());

	{
		PRJREF *pr=CPrjDoc::m_pReviewNode->GetAssignedRef();
		ASSERT(pr);
		if(pr && abs(pr->zone)>60) {
			CString s;
			s.Format("UPS %c Meters",(pr->zone>0)?'N':'S');
			GetDlgItem(IDC_UTM)->SetWindowText(s);
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CExpavDlg::OnBrowse() 
{
	int i;
	CString strFilter;
	CString Path;
	CEdit *pwP=(CEdit *)GetDlgItem(IDC_SHPPATH);
	CEdit *pwB=(CEdit *)GetDlgItem(IDC_SHPBASE);
		
	pwP->GetWindowText(Path);
	pwB->GetWindowText(m_shpbase);
	if((i=Path.GetLength())>0 && Path[i-1]!='\\') Path+="\\";
	Path+=m_shpbase;

	if(!AddFilter(strFilter,IDS_SHP_FILES) ||
	   !AddFilter(strFilter,AFX_IDS_ALLFILTER)) return;
	   
	if(!DoPromptPathName(Path,
	  OFN_HIDEREADONLY,
	  2,strFilter,FALSE,
	  IDS_SHP_BROWSE,
	  ".SHP")) return;
	
	char *pnam=trx_Stpnam(Path.GetBuffer(0));
	*trx_Stpext(pnam)=0;
	if((i=strlen(pnam)-2)>0 && pnam[i]=='_') pnam[i]=0;
	if(*(pnam-1)=='\\') *(pnam-1)=0;
	pwP->SetWindowText(Path);

	pwB->SetWindowText(pnam);
	pwB->SetSel(0,-1);
	pwB->SetFocus();
}

void CExpavDlg::StatusMsg(int veccount)
{
	char buf[40];
	sprintf(buf,"%s exported:%8u",(pExpShp->shp_type==SHP_WALLS)?"Polygons":"Vectors",veccount);
	GetDlgItem(IDC_ST_COUNTER)->SetWindowText(buf);
}

LRESULT CExpavDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(130,HELP_CONTEXT);
	return TRUE;
}

