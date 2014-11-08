// ShowShapeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "WallsMap.h"
#include "Mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "LayerSetSheet.h"
#include "ShpLayer.h"
#include "ShowShapeDlg.h"
#include "UpdatedDlg.h"
#include "dbteditdlg.h"
#include "ImageViewDlg.h"
#include "dlgsymbols.h"
#include "EditLabel.h"
#include "kmldef.h"
#include "DBGridDlg.h"
#include "MsgCheck.h"
#include "AdvSearchDlg.h"
#include "CopySelectedDlg.h"

//Globals --
CShowShapeDlg * app_pShowDlg=NULL;

CRect CShowShapeDlg::m_rectSaved;
bool CShowShapeDlg::m_bLaunchGE_from_Selelected=false;

//static CRect rectSaved(0,0,0,0);

#define MAX_LEN_TREELABEL 80
#define SIZ_FLDVALUE 512
static char fldvalue[SIZ_FLDVALUE];
static UINT fldvalueLen;
#define MIN_FLDVALUELEN 10
#define MIN_COL1WIDTH 316

bool CheckShowDlg()
{
	if(app_pShowDlg && app_pShowDlg->CancelSelChange()) {
		ASSERT(app_pShowDlg);
		if(app_pShowDlg) app_pShowDlg->BringWindowToTop();
	    return false;
	}
	return true;
}

// CShowShapeDlg dialog

IMPLEMENT_DYNAMIC(CShowShapeDlg, CResizeDlg)
CShowShapeDlg::CShowShapeDlg(CWallsMapDoc *pDoc,CWnd* pParent /*=NULL*/)
	: CResizeDlg(CShowShapeDlg::IDD, pParent)
	, m_vec_shprec(&pDoc->m_vec_shprec)
	, m_pDoc(pDoc)
	, m_pwchTip(NULL)
{
	m_bNadToggle=true;
	m_bEditDBF=0;
	m_bEditShp=m_bInitFlag=m_bConflict=m_bClosing=m_bDroppingItem=m_bFieldsInitialized=false;
	m_hSelRoot=m_hSelItem=NULL;
	m_pSelLayer=NULL;
	m_pEditBuf=NULL;
	m_uSelRec=m_uLayerTotal=0;
	m_pNewShpRec=NULL;
	//m_iMemoFld=0;
	
	app_pShowDlg=this;

	Create(CShowShapeDlg::IDD,pParent);
}

CShowShapeDlg::~CShowShapeDlg()
{
	app_pShowDlg=NULL;
	free(m_pwchTip);
}

void CShowShapeDlg::OnClose() 
{
	if(!CancelSelChange() || !m_vec_shprec->size()) {
		if(m_vec_shprec->size() /*&& m_pDoc->m_bMarkSelected*/) { //bug fixed 10/16/2009
			m_bClosing=true;
		}
		Destroy();
	}
	else m_bDlgOpened=false;
}

void CShowShapeDlg::PostNcDestroy() 
{
	ASSERT(app_pShowDlg==this);
	if(m_bClosing) {
		//pDoc->ClearVecShprec() and repaint views --
		GetMF()->PostMessage(WM_PROPVIEWDOC,1,(LPARAM)m_pDoc);
	}
	delete this;
}

void CShowShapeDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_POINT_TREE, m_PointTree);
	DDX_Control(pDX, IDC_FIELD_LIST, m_FieldList);
	DDX_Control(pDX, IDC_EDIT_LABEL, m_ceLabel);
	DDX_Control(pDX, IDC_EDIT_EAST, m_ceEast);
	DDX_Control(pDX, IDC_EDIT_NORTH, m_ceNorth);
}

BEGIN_MESSAGE_MAP(CShowShapeDlg, CResizeDlg)
	ON_WM_CLOSE()
	ON_WM_MEASUREITEM()
	//ON_NOTIFY(NM_LDOWN, IDC_POINT_TREE, OnLclickTree)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_UTMZONE, OnSpinUtm)
	ON_NOTIFY(NM_RCLICK, IDC_POINT_TREE, OnRclickTree)
	ON_NOTIFY(NM_DBLCLK, IDC_POINT_TREE, OnDblClkTree)
	ON_NOTIFY(TVN_SELCHANGING, IDC_POINT_TREE, OnSelChangingTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_POINT_TREE, OnSelChangedTree)
	ON_NOTIFY(TVN_GETDISPINFO, IDC_POINT_TREE,OnTreeDispInfo)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_POINT_TREE, OnCustomDrawTree)

	ON_REGISTERED_MESSAGE(WM_ADVANCED_SRCH,OnAdvancedSrch)

	ON_MESSAGE(WM_QUICKLIST_GETLISTITEMDATA, OnGetListItem)
	ON_MESSAGE(WM_QUICKLIST_NAVIGATIONTEST, OnListNavTest)
	ON_MESSAGE(WM_QUICKLIST_ENDEDIT,OnEndEditField)
	ON_MESSAGE(WM_QUICKLIST_BEGINEDIT,OnBeginEditField)
	ON_MESSAGE(WM_QUICKLIST_CLICK, OnListClick)
	ON_MESSAGE(WM_QUICKLIST_GETTOOLTIP, OnGetToolTip) 

	ON_MESSAGE(WM_QUICKLIST_EDITCHANGE,OnEditChange)
	ON_EN_CHANGE(IDC_EDIT_NORTH, OnEnChangeEditNorth)
	ON_EN_CHANGE(IDC_EDIT_EAST, OnEnChangeEditEast)
	ON_EN_CHANGE(IDC_UTMZONE, OnEnChangeEditZone)
	ON_EN_CHANGE(IDC_EDIT_LABEL, OnEnChangeEditLabel)
	ON_BN_CLICKED(IDOK, OnClose)
	ON_BN_CLICKED(IDC_RESTORE, OnBnClickedUndoEdit)
	ON_BN_CLICKED(IDC_RELOCATE, OnBnClickedRelocate)
	ON_BN_CLICKED(IDC_ENTERKEY, OnBnClickedEnterkey)
	ON_COMMAND(ID_SHOW_CENTER, OnShowCenter)
	ON_COMMAND(ID_SHOW_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_UNDORELOCATE, OnBnClickedUndoRelocate)
	ON_COMMAND(ID_SHOW_COPYLOC, OnShowCopyloc)
	ON_COMMAND(ID_SHOW_PASTELOC, OnShowPasteloc)
	ON_BN_CLICKED(IDC_FINDBYLABEL, OnBnClickedFindbylabel)
#if 0
	ON_WM_SIZE()
	ON_WM_SIZING()
#endif
	//ON_BN_CLICKED(IDC_SAVEATTR, OnBnClickedSaveattr)
	ON_BN_CLICKED(IDC_RESETATTR, OnBnClickedResetattr)
	ON_COMMAND(ID_SHOW_CLEAREDITED, OnClearEdited)
	ON_COMMAND(ID_SHOW_UNSELECT, OnUnselect)
	ON_UPDATE_COMMAND_UI(ID_SHOW_UNSELECT, OnUpdateUnselect)
	ON_COMMAND(ID_EXPORT_SHAPES, OnExportShapes)
	ON_BN_CLICKED(IDC_TYPEGEO, OnBnClickedTypegeo)
	ON_BN_CLICKED(IDC_TYPEUTM, OnBnClickedTypegeo)
	ON_BN_CLICKED(IDC_DATUM1, OnBnClickedDatum)
	ON_BN_CLICKED(IDC_DATUM2, OnBnClickedDatum)
	ON_COMMAND(ID_ZOOM_VIEW, OnZoomView)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_MARKSELECTED, OnBnClickedMarkselected)
	ON_COMMAND(ID_BRANCHSYMBOLS, OnBranchsymbols)
	ON_COMMAND(ID_SET_EDITABLE, OnSetEditable)
	ON_COMMAND(ID_LAUNCH_WEBMAP, OnLaunchWebMap)
	ON_COMMAND(ID_OPTIONS_WEBMAP, OnOptionsWebMap)
	ON_COMMAND(ID_LAUNCH_GE, OnLaunchGE)
	ON_COMMAND(ID_OPTIONS_GE, OnOptionsGE)
	//ON_UPDATE_COMMAND_UI(ID_LAUNCH_GE, OnUpdateLaunchGe)
	ON_COMMAND(ID_OPEN_TABLE,OnOpenTable)
	ON_COMMAND_RANGE(ID_APPENDLAYER_0,ID_APPENDLAYER_N,OnExportTreeItems)
    ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()

// CShowShapeDlg message handlers

LPSTR CShowShapeDlg::GetTreeLabel(UINT i)
{
	if(i>=m_vec_shprec->size()) {
		//i=m_vec_shprec->size();
		*fldvalue=0;
		return fldvalue;
	}

	it_shprec it=m_vec_shprec->begin()+i;
	CShpDBF *pdb=it->pShp->m_pdb;

	if(it->rec>pdb->NumRecs()) {
		ASSERT(it->rec==pdb->NumRecs()+1);
		strcpy(fldvalue,CShpLayer::new_point_str);
		fldvalueLen=LEN_NEW_POINT_STR;

	}
	else {
		if(!pdb->Go(it->rec)) {
			i=pdb->GetRecordFld(it->pShp->m_nLblFld,fldvalue,sizeof(fldvalue));
		}
		else fldvalue[i=0]=0;
		fldvalueLen=i;
	}
	return fldvalue;
}

void CShowShapeDlg::FillPointTree()
{
	CString title;
	title.Format("%s: Selected Points",(LPCSTR)m_pDoc->GetTitle());
	SetWindowText(title);

	CShpLayer *pShp=NULL;

	m_hSelRoot=NULL;
	m_pSelLayer=NULL;
	m_uSelRec=m_uLayerTotal=0;
	m_bDlgOpened=false;

	m_PointTree.SetRedraw(FALSE);

	ClearTree();
	m_vec_relocate.clear();

	m_PointTree.SetImageList(m_pDoc->LayerSet().m_pImageList,TVSIL_NORMAL);

	TV_INSERTSTRUCT tvi;
	memset(&tvi,0,sizeof(tvi));
	tvi.hInsertAfter=TVI_LAST;
	tvi.item.mask=TVIF_DI_SETITEM|TVIF_PARAM|TVIF_TEXT|TVIF_SELECTEDIMAGE|TVIF_IMAGE;

	for(it_shprec it=m_vec_shprec->begin();it!=m_vec_shprec->end();it++) {
		if(pShp!=it->pShp) {
			CString cs;
			pShp=it->pShp;
			tvi.hParent=TVI_ROOT;
			//tvi.item.mask|=TVIF_STATE;
			tvi.item.pszText=(LPSTR)pShp->GetPrefixedTitle(cs);
			tvi.item.lParam=(LPARAM)pShp;
			//tvi.item.stateMask=TVIS_STATEIMAGEMASK;
			tvi.item.iImage=tvi.item.iSelectedImage=pShp->m_nImage;
			//tvi.item.state=INDEXTOSTATEIMAGEMASK(tvi.item.iImage);
			VERIFY(tvi.hParent=m_PointTree.InsertItem(&tvi));
			if(!m_hSelRoot) m_hSelRoot=tvi.hParent;
			m_uLayerTotal++;
			//Set common params for children --
			//tvi.item.mask&=~TVIF_STATE;
			tvi.item.pszText=LPSTR_TEXTCALLBACK;
			tvi.item.iImage=tvi.item.iSelectedImage=0;
		}

		tvi.item.lParam=(LPARAM)(it-m_vec_shprec->begin());
		VERIFY(m_PointTree.InsertItem(&tvi));
	}

	if(m_hSelRoot) {
		VERIFY(m_PointTree.Expand(m_hSelRoot,TVE_EXPAND));
		{
			HTREEITEM h=m_hSelRoot;
			while(h=m_PointTree.GetNextSiblingItem(h))
				m_PointTree.Expand(h,TVE_EXPAND);
		}
		m_PointTree.EnsureVisible(m_hSelRoot);
		m_PointTree.SelectItem(m_PointTree.GetChildItem(m_hSelRoot));
		m_PointTree.SetScrollPos(SB_HORZ,0);
		m_PointTree.SetFocus();
	}
	else {
		m_FieldList.SetItemCount(0);
		if(m_bNadToggle) 
			SetText(IDC_UTMZONE,"");
		ClearText(IDC_EDIT_EAST);
		ClearText(IDC_EDIT_NORTH);
		m_hSelItem=NULL;
		m_pSelLayer=NULL;
		m_pEditBuf=NULL;
		m_uSelRec=0;
		m_ceLabel.SetFocus();
	}
	m_PointTree.SetRedraw(TRUE);
	m_PointTree.Invalidate();

	UpdateTotal();
	if(m_pDoc->m_bMarkSelected) m_pDoc->RepaintViews();

	Disable(IDC_RESTORE);
	Disable(IDC_RELOCATE);
	Disable(IDC_RESETATTR);
	ClearLabel();
}

void CShowShapeDlg::ReInit(CWallsMapDoc *pDoc)
{
	if(m_uSelRec && m_pSelLayer->IsGridDlgOpen()) { 
		DWORD rec=m_uSelRec;
		m_uSelRec=0;
		m_pSelLayer->m_pdbfile->pDBGridDlg->RefreshTable(rec);
	}

	if(m_pDoc!=pDoc) {
		if(m_pDoc->m_bMarkSelected && m_vec_shprec->size())
			m_pDoc->RepaintViews();

		m_pDoc->ClearVecShprec();

		m_pDoc=pDoc;
		m_vec_shprec=&pDoc->m_vec_shprec;
		SetCheck(IDC_MARKSELECTED,pDoc->m_bMarkSelected);
	}
	SetCoordFormat();
	FillPointTree();
	m_FieldList.Invalidate();
}

BOOL CShowShapeDlg::OnInitDialog()
{
	CResizeDlg::OnInitDialog();

	m_xMin -=13;	// x minimum resize
	m_yMin -=50;	// y minimum resize

	CRect rect;
	CWnd* pWnd = GetDlgItem(IDC_STATIC_SPLIT_PANE);
	pWnd->GetWindowRect(&rect);
	ScreenToClient(&rect);
	pWnd->DestroyWindow();

	m_SplitterPane.Create(
		WS_CHILD|WS_VISIBLE/*|WS_BORDER*/|WS_CLIPCHILDREN|SS_VERT,		 
		this,		// the parent of the splitter pane
		&m_PointTree,	// left pane
		&m_FieldList,// right pane
		IDC_VERT_DLG,// this ID is used for saving/restoring splitter
					// position and therefore it must be unique !
		rect,	// dimensions of the splitter pane
		83,	// left constraint for splitter position (pane width is twice this width)
		140 // right constraint for splitter position
	);

	m_PointTree.ModifyStyle(0, TVS_LINESATROOT | TVS_HASBUTTONS
		| TVS_SHOWSELALWAYS | TVS_HASLINES );

	    // Remove owner drawn style set in resources, we already tricked the framework into thinking that
    // we have an owner draw control so that we get a call to OnMeasureItem.
    m_FieldList.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0 );


	ASSERT(!m_FieldList.GetHeaderCtrl()->GetItemCount());
	m_FieldList.SetBkColor(::GetSysColor(COLOR_BTNFACE));

	DWORD dwStyle=m_FieldList.GetExtendedStyle();

	m_FieldList.SetExtendedStyle(
		dwStyle |
		LVS_EX_FULLROWSELECT |
		LVS_EX_GRIDLINES
	);
	// Add columns to header -- we'll change the widths below.
	m_FieldList.InsertColumn(0, "", LVCFMT_RIGHT, 75);
	m_FieldList.InsertColumn(1, "", LVCFMT_LEFT, MIN_COL1WIDTH);
	m_FieldList.SetTooltipDelays(200,32000,100); //dflt for now (500,32000,100)
	m_FieldList.EnableToolTips(TRUE);

	VERIFY(m_PointTree.SetWindowPos(GetDlgItem(IDC_UNDORELOCATE),0,0,0,0,SWP_NOMOVE|SWP_NOSIZE));
	//cwPrev = m_PointTree.GetNextWindow(GW_HWNDPREV);
	VERIFY(m_FieldList.SetWindowPos(&m_PointTree,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE));
	//VERIFY(csLbl->SetWindowPos(&m_FieldList,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE));

	m_FieldList.SetNavigationColumn(1);
	m_FieldList.m_bWrap=true;
	m_FieldList.m_iGridMargin=2;

	m_ceLabel.SetLimitText(127);
	m_ceLabel.SetCustomCmd(WM_ADVANCED_SRCH,"Text Search dialog...",this);

	//CenterWindow(CWnd::GetDesktopWindow());

	AddControl(IDOK, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_RESETATTR, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_RELOCATE, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_UNDORELOCATE, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_RECNO, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);

	AddControl(IDC_ST_TOTAL, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_ST_FINDBYLABEL, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_EDIT_LABEL, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_FINDBYLABEL, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_MARKSELECTED, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_VERT_DLG, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE, 0);

	if(!m_rectSaved.IsRectEmpty()) {
		Reposition(m_rectSaved);
	}
	SetCoordFormat();

	SetCheck(IDC_MARKSELECTED,m_pDoc->m_bMarkSelected);
	FillPointTree();

	VERIFY(m_dropTarget.Register(this));

	return FALSE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

HTREEITEM CShowShapeDlg::IsLayerSelected(CShpLayer *pShp) const
{
	HTREEITEM h=m_PointTree.GetRootItem();
	while(h) {
		if(pShp==(CShpLayer *)m_PointTree.GetItemData(h)) {
			break;
		}
		h=m_PointTree.GetNextSiblingItem(h);
	}
	return h;
}

void CShowShapeDlg::UpdateLayerTitle(CShpLayer *pShp)
{
	HTREEITEM h=IsLayerSelected(pShp);
	if(h) m_PointTree.SetItemText(h,pShp->Title()); 
}

int CShowShapeDlg::GetRelocateStr(CString &msg)
{
	ASSERT(m_hSelItem);
	ASSERT(m_pSelLayer && m_pSelLayer->IsShpEditable());

	int iRet;
	if(m_hSelRoot) {
		if(SelDeleted()) {
			ASSERT(!IsAddingRec());
			msg.Format("Add point similar to %s",GetTreeLabel(m_hSelItem));
			iRet=2;
		}
		else { 
			msg.Format("Relocate %s",GetTreeLabel(m_hSelItem));
			iRet=IsAddingRec()?4:3;
		}
	}
	else {
		msg.Format("Add point to %s",m_pSelLayer->Title());
		iRet=1;
	}
	return iRet;
}

void CShowShapeDlg::MoveRestore()
{
	CRect cr;
	GetDlgItem(IDC_RESTORE)->GetWindowRect(&cr); //screen coordinates
	ScreenToClient(&cr);
	int w=3*cr.Width()/2;
	if(!m_bNadToggle) w=-w;
	GetDlgItem(IDC_RESTORE)->SetWindowPos(NULL,cr.left+w,cr.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
}

void CShowShapeDlg::RefreshCoordinates(CFltPoint &fpt)
{
	//fpt are coordinates in document format.
	//Display them according to current settings 

	char buf[32];
	buf[31]=0;

	if(m_bNadToggle) {
		//Document with known datum and either UTM or Lat/Long --

		if((m_bUtmDisplayed!=(m_iZoneDoc!=0)) || m_iZoneSel!=m_iZoneDoc || m_iNadSel!=m_iNadDoc) {

			VERIFY(GetConvertedPoint(fpt,m_iNadSel,m_bUtmDisplayed?&m_iZoneSel:NULL,m_iNadDoc,m_iZoneDoc));
			//int iZone=m_pSelLayer->ConvertPointsTo(&fpt,m_iNadSel,m_bUtmDisplayed?m_iZoneSel:0,1);
			//ASSERT(iZone==(m_bUtmDisplayed?m_iZoneSel:0));
		}
		EnableNadToggle();
		Show(IDC_SPIN_UTMZONE,m_bUtmDisplayed);
		Enable(IDC_UTMZONE,m_bUtmDisplayed);
		SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel));
	}

	m_bInitFlag=true;
	_snprintf(buf,31,m_bUtmDisplayed?"%.2f":"%.7f",m_bUtmDisplayed?fpt.x:fpt.y);
	Enable(IDC_EDIT_EAST); //In case moving away from a root
	SetText(IDC_EDIT_EAST,buf);
	_snprintf(buf,31,m_bUtmDisplayed?"%.2f":"%.7f",m_bUtmDisplayed?fpt.y:fpt.x);
	Enable(IDC_EDIT_NORTH);
	SetText(IDC_EDIT_NORTH,buf);
	m_bInitFlag=false;
}

void CShowShapeDlg::RefreshSelCoordinates()
{
	CFltPoint fpt=m_pSelLayer->m_fpt[m_uSelRec-1];
	m_iZoneSel=m_iZoneDoc;
	ASSERT(m_iZoneSel==m_pSelLayer->Zone());

	if(m_bNadToggle && !m_iZoneSel)
		m_iZoneSel=GetZone(fpt); //for display only

	m_bValidEast=m_bValidNorth=m_bValidZone=true;

	RefreshCoordinates(fpt);
	BOOL bRO=!m_pSelLayer->IsShpEditable() || SelDeleted();
	SetRO(IDC_EDIT_EAST,bRO);
	SetRO(IDC_EDIT_NORTH,bRO);
	SetRO(IDC_UTMZONE,bRO || !m_bUtmDisplayed);
	Show(IDC_SPIN_UTMZONE,!bRO && m_bUtmDisplayed && m_bNadToggle);
	m_bEditShp=false;
}

void CShowShapeDlg::SetCoordLabels()
{
	SetText(IDC_ST_EASTING,m_bUtmDisplayed?"Easting":"Latitude");
	SetText(IDC_ST_NORTHING,m_bUtmDisplayed?"Northing":"Longitude");
}

void CShowShapeDlg::SetCoordFormat()
{
	//Called when document changes (from OnInitDialog() or ReInit())

	CString datum;
	m_pDoc->GetDatumName(datum);
	
	m_iNadDoc=m_pDoc->LayerSet().m_iNad;
	m_iZoneDoc=m_pDoc->LayerSet().m_iZone;
	if(!m_pDoc->IsGeoRefSupported()) {
		m_bUtmDisplayed=m_pDoc->IsProjected(); //Easting Northing
		if(m_bNadToggle) {
			m_bNadToggle=false; //sets direction in MoveRestore()
			MoveRestore();
			m_iNadSel=m_iNadDoc;
			Hide(IDC_DATUM1);
			Hide(IDC_DATUM2);
			Hide(IDC_TYPEGEO);
			Hide(IDC_TYPEUTM);
			Hide(IDC_UTMZONE);
			Hide(IDC_SPIN_UTMZONE);
			Show(IDC_ST_DATUM);
			SetText(IDC_ST_DATUM,datum);
		}
	}
	else {
		m_iNadSel=m_pDoc->m_bShowAltNad?(3-m_iNadDoc):m_iNadDoc;
		if(m_pDoc->m_bShowAltGeo)
			m_bUtmDisplayed=(m_iZoneDoc==0);
		else
			m_bUtmDisplayed=(m_iZoneDoc!=0);

		if(!m_bNadToggle) {
			m_bNadToggle=true;
			MoveRestore();
			Show(IDC_DATUM1);
			Show(IDC_DATUM2);
			Show(IDC_TYPEGEO);
			Show(IDC_TYPEUTM);
			SetText(IDC_UTMZONE,"");
			Show(IDC_UTMZONE);
			Show(IDC_SPIN_UTMZONE);
			Hide(IDC_ST_DATUM);
		}
		CheckRadioButton(IDC_DATUM1,IDC_DATUM2,(m_iNadSel==1)?IDC_DATUM1:IDC_DATUM2);
		CheckRadioButton(IDC_TYPEGEO,IDC_TYPEUTM,m_bUtmDisplayed?IDC_TYPEUTM:IDC_TYPEGEO);
	}
	SetCoordLabels();
}

void CShowShapeDlg::OnSelChangingTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = m_bDroppingItem==false && m_pSelLayer && CancelSelChange();
}

void CShowShapeDlg::OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	if(m_bDroppingItem) return;

	bool bSelectingNew=false;

	CShpLayer *pLastLayer=m_uSelRec?m_pSelLayer:NULL;
	DWORD uLastRec=m_uSelRec;

	int nTopIndex= uLastRec?m_FieldList.GetTopIndex():0;

	VERIFY(m_hSelItem=m_PointTree.GetSelectedItem());
	m_hSelRoot=m_PointTree.GetParentItem(m_hSelItem);

	if(m_hSelRoot) {
		SHPREC &shprec=(*m_vec_shprec)[m_PointTree.GetItemData(m_hSelItem)];
		bSelectingNew=m_pNewShpRec==&shprec;
		bool bNewLayer=m_pSelLayer!=shprec.pShp;

		m_pSelLayer=shprec.pShp;
		m_uSelRec=shprec.rec;

		ASSERT(!bSelectingNew || m_uSelRec==m_pSelLayer->m_pdb->NumRecs()+1);

		UINT nFlds=m_pSelLayer->m_pdb->NumFlds();
		if(bNewLayer) {
			UINT maxLen=0;
			for(UINT n=1;n<=nFlds;n++) {
				UINT l=m_pSelLayer->m_pdb->FldLen(n);
				if(l>maxLen) maxLen=l;
			}
			HDITEM hdi;
			hdi.mask=HDI_WIDTH;
			hdi.cxy=(maxLen+1)*CShpLayer::m_nFieldListCharWidth;
			if(hdi.cxy<MIN_COL1WIDTH) hdi.cxy=MIN_COL1WIDTH;
			ClearFields();
			VERIFY(m_FieldList.GetHeaderCtrl()->SetItem(1,&hdi));
		}

		m_FieldList.SetItemCount(nFlds); //Also redraws items

		if(nTopIndex && (pLastLayer==m_pSelLayer ||
			pLastLayer->m_pdb->FieldsMatch(m_pSelLayer->m_pdb))) {
			//restore top index --
			m_FieldList.EnsureVisible(m_FieldList.GetItemCount()-1,0); // Scroll down to the bottom
			m_FieldList.EnsureVisible(nTopIndex,0); // scroll back up just enough to show said item on top
		}

		RefreshSelCoordinates();
		RefreshRecNo(m_uSelRec);
		m_vdbeBegin=m_pSelLayer->IsEditable()?SelEditFlag():0;
		m_FieldList.SetSelected(0);
	}
	else {
		ClearFields();
		m_pEditBuf=NULL;
		VERIFY(m_pSelLayer=(CShpLayer *)m_PointTree.GetItemData(m_hSelItem));
		m_uSelRec=0;
		ClearText(IDC_EDIT_EAST);
		ClearText(IDC_EDIT_NORTH);
		SetText(IDC_UTMZONE,"");
		Disable(IDC_UTMZONE);
		Hide(IDC_SPIN_UTMZONE);
		RefreshRecNo(0);
	}
	//m_FieldList.RedrawItems(0,m_FieldList.GetItemCount()-1);

	//Enable datum and projection switching?
	if(m_bNadToggle) EnableNadToggle();

	Disable(IDC_RELOCATE);
	Disable(IDC_RESTORE);
	Disable(IDC_UNDORELOCATE);
	m_vec_relocate.clear();
	ASSERT(!m_vMemo.size());
	m_bEditShp=m_bFieldsInitialized=false;
	m_bEditDBF=0;
	if(bSelectingNew) {
		ASSERT(m_hSelRoot && m_uSelRec==m_pSelLayer->m_pdb->NumRecs()+1);
		VERIFY(m_pEditBuf=m_pSelLayer->m_pdbfile->pbuf);
		Enable(IDC_RESETATTR);
		SetText(IDC_RESETATTR,"Cancel Add");
		m_pSelLayer->UpdateShpExtent();
		m_pDoc->RefreshViews();
		PostMessage(WM_QUICKLIST_CLICK,(WPARAM)m_FieldList.GetSafeHwnd());
	}
	else {
		m_pNewShpRec=NULL;
		Disable(IDC_RESETATTR);
		if(m_pDoc->m_bMarkSelected) m_pDoc->RepaintViews();
	}

	if(pLastLayer && pLastLayer->IsGridDlgOpen())
		pLastLayer->m_pdbfile->pDBGridDlg->RefreshTable(uLastRec);
	if(m_uSelRec && m_pSelLayer->IsGridDlgOpen()) 
		m_pSelLayer->m_pdbfile->pDBGridDlg->RefreshTable(m_uSelRec);
}

void CShowShapeDlg::OnEnChangeEditCoord(bool bValid,bool bValidOther)
{
	if(m_bNadToggle && (bValid && bValidOther) != (m_bValidEast && m_bValidNorth && m_bValidZone)) {
		Enable(IDC_DATUM1,bValid);
		Enable(IDC_DATUM2,bValid);
		Enable(IDC_TYPEGEO,bValid);
		Enable(IDC_TYPEUTM,bValid);
		if(m_bNadToggle)
			Show(IDC_SPIN_UTMZONE,bValid);
	}
	bValid=(bValid && bValidOther);
	if(bValid!=(IsEnabled(IDC_RELOCATE)==TRUE))
		Enable(IDC_RELOCATE,bValid);

	if(!m_bEditShp) {
		Enable(IDC_RESTORE);
		m_bEditShp=true;
	}
}

void CShowShapeDlg::OnEnChangeEditEast()
{
	if(m_bInitFlag) return;

	CString s;
	double f;
	GetText(IDC_EDIT_EAST,s);
	bool bValid=GetCoordinate(s,f,true,m_bUtmDisplayed);
	OnEnChangeEditCoord(bValid,m_bValidNorth && m_bValidZone);
	m_bValidEast=bValid;
}

void CShowShapeDlg::OnEnChangeEditNorth()
{
	if(m_bInitFlag) return;

	CString s;
	double f;
	GetText(IDC_EDIT_NORTH,s);
	bool bValid=GetCoordinate(s,f,false,m_bUtmDisplayed);
	OnEnChangeEditCoord(bValid,m_bValidEast && m_bValidZone);
	m_bValidNorth=bValid;
}

void CShowShapeDlg::OnEnChangeEditZone()
{
	char buf[32];
	if(m_bInitFlag) return;

	ASSERT(m_bUtmDisplayed);
	GetText(IDC_UTMZONE,buf,32);
	int i=(IsNumeric(buf) && !strchr(buf,'.'))?atoi(buf):0;
	if(abs(i)>60) i=0;
	OnEnChangeEditCoord(i!=0,m_bValidEast && m_bValidNorth);
	m_bValidZone=(i!=0);
}

void CShowShapeDlg::OnBnClickedUndoEdit()
{
	ASSERT(m_hSelRoot && m_uSelRec);
	if(m_hSelRoot) {
		ASSERT(m_uSelRec && m_pSelLayer && m_pSelLayer->m_fpt.size()>=m_uSelRec);
		RefreshSelCoordinates();
		m_PointTree.SetFocus();
		Disable(IDC_RESTORE);
		Disable(IDC_RELOCATE);
	}
}

BOOL CShowShapeDlg::GetEditedPoint(CFltPoint &fpt,bool bChkRange)
{
	//Set fpt to edited value after converting to m_pDoc's format if necessary --
	ASSERT(m_bEditShp && m_hSelItem && m_pSelLayer->m_fpt.size()>=m_uSelRec);
	ASSERT(IsLocationValid());

	int iZone;
	CString s;
	double f0,f1;

	//Dialog:
	//Easting		Northing
	//Latitude		Longitude
	//IDC_EDIT_EAST	IDC_EDIT_NORTH

	GetText(IDC_EDIT_EAST,s);
	VERIFY(GetCoordinate(s,f0,true,m_bUtmDisplayed));
	GetText(IDC_EDIT_NORTH,s);
	VERIFY(GetCoordinate(s,f1,false,m_bUtmDisplayed));

	if(m_bUtmDisplayed) {
		fpt.x=f0;
		fpt.y=f1;
		if(m_bNadToggle) {
			GetText(IDC_UTMZONE,s);
			m_iZoneSel=atoi(s);
		}
	}
	else {
		fpt.x=f1;
		fpt.y=f0;
		if(m_bNadToggle) {
			iZone=GetZone(fpt);
			if(iZone!=m_iZoneSel)
				SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel=iZone));
		}
	}

	{
		CFltPoint fptSave(fpt);

		//If necessary, convert displayed coordinates to doc's (or view's) CRS --
		if(m_bNadToggle && ((m_iNadSel!=m_iNadDoc) || ((m_iZoneDoc==0)==m_bUtmDisplayed) ||
			 (m_iZoneDoc && m_iZoneSel!=m_iZoneDoc))) {
				//m_pSelLayer->ConvertPointFrom(fpt,m_iNadSel,m_bUtmDisplayed?m_iZoneSel:0);
			if(!GetConvertedPoint(fpt,m_iNadDoc,m_iZoneDoc?&m_iZoneDoc:NULL,m_iNadSel,m_bUtmDisplayed?m_iZoneSel:0))
				goto _fail;
		}

		if(bChkRange) {
			//Check if new location is out of view --
			CWallsMapView::m_bTestPoint=false;
			CSyncHint hint;
			hint.fpt=&fpt;
			m_pDoc->UpdateAllViews(NULL,LHINT_TESTPOINT,&hint);

			if(m_bNadToggle && m_pSelLayer->m_iZoneOrg && abs(m_pSelLayer->m_iZoneOrg)<=60) {
				if(!m_iZoneDoc) {
					iZone=GetZone(fpt);
				}
				else if(m_bUtmDisplayed) {
					//displayed (edited) easting and northing might have been out-of-zone
					geo_UTM2LatLon(fptSave,m_iZoneSel,m_iNadSel);
					iZone=GetZone(fptSave); //not necessarily = m_iZoneSel
				}
				else {
					//Lat/long is displayed, in which case we've already obtained m_iZoneSel=iZone via GetZone().
					//For this test we ignore the possibility that m_iNadDoc!=m_iNadSel
					ASSERT(iZone==m_iZoneSel);
				}

				if(abs(iZone)-abs(m_pSelLayer->m_iZoneOrg)>1) {
					m_pSelLayer->OutOfZoneMsg(iZone);
					return 0;
				}
			}

			if(!CWallsMapView::m_bTestPoint) {
				if(IDOK!=AfxMessageBox("CAUTION: The proposed location is not currently within view. Select OK to apply the change anyway."
					,MB_OKCANCEL)) return 0;
			}
		}
	}

	return TRUE;

_fail:
	AfxMessageBox("The coordinates are outside the range of the map view's projection.");
	return 0;
}

void CShowShapeDlg::OnBnClickedEnterkey()
{
	CWnd *pWnd=GetFocus();
	UINT id=pWnd?(pWnd->GetDlgCtrlID()):0;
	if(id==IDOK) OnClose();
}

void CShowShapeDlg::OnDblClkTree(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult=CDRF_DODEFAULT; //0==normal handling
	CPoint curPoint;
	GetCursorPos(&curPoint);
	m_PointTree.ScreenToClient(&curPoint);
	HTREEITEM hItem=m_PointTree.HitTest(curPoint);
	if(!hItem || !m_PointTree.GetParentItem(hItem)) return;

	if(!m_pSelLayer->IsShpEditable() || (m_bValidNorth && m_bValidEast && m_bValidZone)) {
		OnShowCenter();
		*pResult=TRUE;
	}
}

void CShowShapeDlg::OnRclickTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CPoint curPoint;
	GetCursorPos(&curPoint);
	m_PointTree.ScreenToClient(&curPoint);
	HTREEITEM hItem=m_PointTree.HitTest(curPoint);

	*pResult=CDRF_DODEFAULT;
	if(!hItem || !m_PointTree.SelectItem(hItem) || !m_pSelLayer) return;

	HMENU hPopup=NULL;
	CMenu menu;
	CMenu* pPopup;

	if(m_PointTree.GetParentItem(hItem)) {
		ASSERT(m_uSelRec && m_hSelRoot);
		if(!menu.LoadMenu(IDR_SHOW_CONTEXT)) return;
		pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		if(!m_bNadToggle) {
			pPopup->DeleteMenu(4,MF_BYPOSITION); //separator
			pPopup->DeleteMenu(4,MF_BYPOSITION); //GE submenu
			pPopup->DeleteMenu(4,MF_BYPOSITION); //Web map submenu
		}
		else {
			CString s;
			if(GetMF()->GetWebMapName(s)) {
				pPopup->ModifyMenu(ID_LAUNCH_WEBMAP,MF_BYCOMMAND|MF_STRING,ID_LAUNCH_WEBMAP,s);
			}
			if(!GE_IsInstalled()) {
				pPopup->ModifyMenu(ID_LAUNCH_GE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED,0,"GE not installed");
			}
		}
		if(!m_pSelLayer->IsShpEditable()) {
			pPopup->DeleteMenu(ID_SHOW_DELETE,MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_SHOW_PASTELOC,MF_BYCOMMAND); //Copy OK
			pPopup->DeleteMenu(ID_SHOW_CLEAREDITED,MF_BYCOMMAND);
		}
		else {
			{
				CString sPaste;
				bool butm=m_bUtmDisplayed; //paste can cause switch from utm to lat/long --
				if(SelDeleted() || !ClipboardPaste(sPaste,42) || !CheckCoordinatePair(sPaste,butm))
					pPopup->EnableMenuItem(ID_SHOW_PASTELOC,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			}

			if(!IsLocationValid()) pPopup->EnableMenuItem(ID_SHOW_COPYLOC,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

			if(!(SelEditFlag()&~SHP_EDITDEL) || IsAddingRec())
				pPopup->DeleteMenu(ID_SHOW_CLEAREDITED,MF_BYCOMMAND);

			if(IsAddingRec()) {
				pPopup->DeleteMenu(pPopup->GetMenuItemCount()-1,MF_BYPOSITION);
				pPopup->EnableMenuItem(ID_SHOW_UNSELECT,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
			}
			else {
				if(SelDeleted()) {
					pPopup->ModifyMenu(ID_SHOW_DELETE,MF_BYCOMMAND|MF_STRING,ID_SHOW_DELETE,"&Undelete feature");
				}
			}
		}
	}
	else {
		ASSERT(m_hSelItem && !m_hSelRoot);
		if(!m_hSelItem) return;
		ASSERT(m_pSelLayer==(CShpLayer *)m_PointTree.GetItemData(m_hSelItem));
		if(!menu.LoadMenu(IDR_SHOWROOT_CONTEXT)) {
			ASSERT(0);
			return;
		}
		pPopup = menu.GetSubMenu(0);
		//do not insert ID_LAUNCH_WEBMAP for branches either for now
		if(!m_bNadToggle) {
			pPopup->DeleteMenu(7,MF_BYPOSITION);
		}
		else if(!GE_IsInstalled()) {
			pPopup->ModifyMenu(ID_LAUNCH_GE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED,0,"GE not installed");
		}
		pPopup->CheckMenuItem(ID_SET_EDITABLE,m_pSelLayer->IsEditable()?(MF_CHECKED|MF_BYCOMMAND):(MF_UNCHECKED|MF_BYCOMMAND));
		if(m_pSelLayer->m_pdbfile->pShpEditor && m_pSelLayer->m_pdbfile->pShpEditor!=m_pSelLayer
			|| m_pSelLayer->HasEditedMemo() /*app_pDbtEditDlg && app_pDbtEditDlg->m_pShp==m_pSelLayer*/
		)
			pPopup->EnableMenuItem(ID_SET_EDITABLE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	}

	ASSERT(m_pSelLayer);
	if(!IsAddingRec())
		hPopup=m_pDoc->AppendCopyMenu(m_pSelLayer,pPopup,m_hSelRoot?"point":"branch items");

	m_PointTree.ClientToScreen(&curPoint);
	VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,curPoint.x,curPoint.y,this));
	if(hPopup) DestroyMenu(hPopup);
}

LPBYTE CShowShapeDlg::LoadEditBuf()
{
	if(!m_pEditBuf) {
		VERIFY(!m_pSelLayer->m_pdb->Go(m_uSelRec));
		VERIFY(m_pEditBuf=m_pSelLayer->m_pdbfile->pbuf);
		m_pSelLayer->m_pdb->GetRec(m_pEditBuf);
	}
	return m_pEditBuf;
}

static void UpdateErrorMsg(CShpLayer *pLayer)
{
	CMsgBox("Failure updating %s.dbf. Disk may be full!",pLayer->Title());
}

void CShowShapeDlg::FlushEdits()
{
	ASSERT(m_hSelRoot && m_pSelLayer && m_pSelLayer->IsEditable());
	ASSERT(m_bEditDBF && m_pEditBuf || m_pNewShpRec || HasNewLocFlds());

	CShpDBF *pdb=m_pSelLayer->m_pdb;
	bool bDlgOpened=false;

	if(HasNewLocFlds()) {
		ASSERT(m_pSelLayer->IsShpEditable());
		LoadEditBuf();
		m_pSelLayer->UpdateLocFlds(m_uSelRec,m_pEditBuf);
		Disable(IDC_UNDORELOCATE);
		m_vec_relocate.clear();
	}

	if(m_pSelLayer->m_pdbfile->HasTimestamp(2)) {
		ASSERT(SHP_DBFILE::bEditorPrompted && m_pEditBuf);
		m_pSelLayer->UpdateTimestamp(m_pEditBuf,m_pNewShpRec!=NULL);
	}

	if(m_vMemo.size()) {
		ASSERT(m_pEditBuf);
		CDBTFile &dbt=m_pSelLayer->m_pdbfile->dbt;
		for(it_vMemo it=m_vMemo.begin();it!=m_vMemo.end();it++) {
			CDBTFile::SetRecNo((LPSTR)GetFldPtr(it->fld),dbt.PutText(*it));
		}
		dbt.FlushHdr();
		ClearVecMemo();
	}
	int e=0;

	if(m_pNewShpRec) {
		ASSERT(m_uSelRec==pdb->NumRecs()+1);
		ASSERT((m_vdbeBegin&(SHP_EDITSHP|SHP_EDITDBF|SHP_EDITADD))==(SHP_EDITSHP|SHP_EDITDBF|SHP_EDITADD));
		m_bEditDBF|=2; //label has changed;
		m_pSelLayer->m_bEditFlags|=m_vdbeBegin;
		if((e=pdb->AppendRec(m_pEditBuf)) || (e=pdb->FlushRec()) || !m_pSelLayer->SaveShp()) {
			bDlgOpened=true;
			if(e) UpdateErrorMsg(m_pSelLayer);
			//Cancel addition, etc!!!
		}
		else {
			ASSERT(m_uSelRec==pdb->NumRecs());
			m_pSelLayer->UpdateSizeStr();
			if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(m_pSelLayer);
		}
		SetText(IDC_RESETATTR,"Reset Attr");
		m_pNewShpRec=NULL;
	}
	else {
		ASSERT(m_uSelRec<=pdb->NumRecs());
		ASSERT(m_pEditBuf || (SelEditFlag()&SHP_EDITSHP)!=0);
		if(m_pEditBuf) {
			if(!(e=pdb->Go(m_uSelRec)) && !(e=pdb->PutRec(m_pEditBuf)) && !(e=pdb->FlushRec())) {
				m_pSelLayer->ChkEditDBF(m_uSelRec); 
				//Above may NOT have set m_pSelLayer->m_bEditFlags!
			}
			else {
				bDlgOpened=true;
				UpdateErrorMsg(m_pSelLayer);
			}
		}
		//may need to rewrite DBE --
		if(m_pSelLayer->m_bEditFlags)
			m_pSelLayer->SaveShp();
	}

	m_pEditBuf=NULL;
	m_PointTree.Invalidate();

	if( (m_bEditDBF&2) && m_pSelLayer->IsVisible() && (m_pSelLayer->Visibility(m_uSelRec)&2)) {
		m_pDoc->RefreshViews();
	}
	if(bDlgOpened) m_bDlgOpened=true;
	m_bFieldsInitialized=false;
	m_bEditDBF=0;
}

bool CShowShapeDlg::IsDBFEdited()
{
	return m_bEditDBF || (m_FieldList.m_edit && m_FieldList.m_edit->IsChanged());
}

bool CShowShapeDlg::IsFieldEmpty(UINT nFld)
{
	CShpDBF *pdb=m_pSelLayer->m_pdb;
	return CShpDBF::IsFldBlank(m_pSelLayer->m_pdbfile->pbuf+pdb->FldOffset(nFld),pdb->FldLen(nFld));
}

bool CShowShapeDlg::DiscardShpEditsOK()
{
	bool bret=(IDOK==CMsgBox(MB_OKCANCEL,"You've directly edited the coordinates displayed for %s. "
				"To revise the location you must press the Relocate button before unselecting or leaving that item.\n"
				"\nPress OK discard edits and leave, or CANCEL to continue editing.",GetTreeLabel(m_hSelItem)));
	if(bret) OnBnClickedUndoEdit();
	return bret;
}

BOOL CShowShapeDlg::CancelSelChange()
{
	ASSERT(m_pSelLayer || !m_bEditShp && !m_pNewShpRec && !m_pEditBuf && !IsDBFEdited() && !m_bEditDBF);
	if(!m_pSelLayer) return FALSE;

	bool bDlgOpened=false,bCancel=false;

	if(m_bEditShp) {
		bDlgOpened=true;
		if(!DiscardShpEditsOK()) return TRUE;
		bCancel =true;
	}

	if(m_pNewShpRec && m_pEditBuf) {
		ASSERT(m_uSelRec==m_pNewShpRec->rec);
		//m_pEditbuf won't be set until after the first selection!
		m_PointTree.SetFocus(); //commit current edits
		bool bContinue=false;

		//if(!IsDBFEdited()) bCancel=true;
		if(!bCancel && IsFieldEmpty(m_pSelLayer->m_nLblFld)) {  //(!IsDBFEdited() || !IsLabelEdited())) {
			bDlgOpened=true;
			CString s;
			s.Format("A new point record is pending but the label field, %s, is empty. Save record anyway?",
				m_pSelLayer->m_pdb->FldNamPtr(m_pSelLayer->m_nLblFld));

			int idret=MsgYesNoCancelDlg(m_hWnd, s, m_pSelLayer->m_csTitle,"Save", "Discard", "Continue Editing");

			if(idret==IDNO) bCancel=true;
			else if(idret==IDCANCEL) m_bConflict=bContinue=true;
		}

		if(bCancel) {
			if(app_pDbtEditDlg && app_pDbtEditDlg->IsRecOpen(m_pSelLayer,m_uSelRec)) {
				app_pDbtEditDlg->Destroy();
			}
			if(app_pImageDlg && app_pImageDlg->IsRecOpen(m_pSelLayer,m_uSelRec)) {
				app_pImageDlg->Destroy();
			}
			CancelAddition();
			return FALSE; //Allow selection change!
		}

		if(bContinue) {
			PostMessage(WM_QUICKLIST_CLICK,(WPARAM)m_FieldList.GetSafeHwnd());
			return TRUE;
		}
	}

	if(IsDBFEdited() && !m_bEditDBF) {
			//This should commit field edit and set m_bEditDBF=true --
			m_PointTree.SetFocus();
	}

	if(m_bEditDBF || IsAddingRec() || HasNewLocFlds()) {
		bool bLabelEdited = m_pNewShpRec!=NULL;
		if(!bLabelEdited && m_pEditBuf) {
			CShpDBF *pdb=m_pSelLayer->m_pdb;
			if(!pdb->Go(m_uSelRec)) {
				UINT nFld=m_pSelLayer->m_nLblFld;
				UINT fldOff=pdb->FldOffset(nFld);
				bLabelEdited=memcmp(m_pEditBuf+fldOff, (LPBYTE)pdb->RecPtr()+fldOff, pdb->FldLen(nFld))!=0;
			}
		}
		FlushEdits();
		if(bLabelEdited) {
			if(app_pDbtEditDlg && app_pDbtEditDlg->IsRecOpen(m_pSelLayer,m_uSelRec)) {
				app_pDbtEditDlg->RefreshTitle();
			}
			if(app_pImageDlg && app_pImageDlg->IsRecOpen(m_pSelLayer,m_uSelRec)) {
				app_pImageDlg->RefreshTitle();
			}
		}
	}

	m_bEditShp=FALSE;

	if(bDlgOpened) m_bDlgOpened=true;

	return FALSE;
}

void CShowShapeDlg::OnTreeDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTVDISPINFO lptvdi = (LPNMTVDISPINFO) pNMHDR; 
	if (lptvdi->item.mask&TVIF_TEXT) {
		UINT i=(UINT)lptvdi->item.lParam;
		UINT sz=(*m_vec_shprec).size();
		SHPREC *pShpRec=(sz || i<sz)?&(*m_vec_shprec)[i]:NULL;
		if(!pShpRec || !pShpRec->rec || pShpRec->rec>pShpRec->pShp->m_nNumRecs) {
			*pResult=TRUE;
			return;
		}

		CString label(GetTreeLabel(i));
		if(pShpRec->pShp->IsPtRecDeleted(pShpRec->rec))
			label.Insert(0,'*');
		label.Replace("&","&&"); //otherwise & causes underline of next char!
		fldvalueLen=label.GetLength();
		if(fldvalueLen>=MAX_LEN_TREELABEL) {
			label.Truncate(MAX_LEN_TREELABEL-4);
			label+="...";
			fldvalueLen=MAX_LEN_TREELABEL-1;
		}

		lptvdi->item.pszText=(LPSTR)memcpy(fldvalue,label,fldvalueLen);

		while(fldvalueLen<MIN_FLDVALUELEN) fldvalue[fldvalueLen++]=' ';
		fldvalue[fldvalueLen]=0;
		*pResult = 0;
	}
	else {
		ASSERT(FALSE);
		*pResult=TRUE;
	}
}

void CShowShapeDlg::OnCustomDrawTree( NMHDR* pNMHDR, LRESULT* pResult )
{
	static int iLevel=0;
	static COLORREF clrText;

	LPNMTVCUSTOMDRAW pCustomDraw = (LPNMTVCUSTOMDRAW)pNMHDR;
	*pResult = CDRF_DODEFAULT;

	if(m_bInitFlag)
		return;

	switch (pCustomDraw->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			// Need to process this case and set pResult to CDRF_NOTIFYITEMDRAW,
			// otherwise parent will never receive CDDS_ITEMPREPAINT notification. (GGH)
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			// Unless selected, painting all 0-level items dark read if editable
			// and all 1-level items red if edited --
			{
				HTREEITEM hItem=(HTREEITEM)pCustomDraw->nmcd.dwItemSpec;
				iLevel=0;
				if(hItem) {
					DWORD data=m_PointTree.GetItemData(hItem);
					iLevel=pCustomDraw->iLevel;
					if(iLevel>0) {
						SHPREC &shprec=(*m_vec_shprec)[data];
						if(!shprec.rec || /*shprec.pShp->IsEditable() &&*/ shprec.pShp->IsRecEdited(shprec.rec)) {
							pCustomDraw->clrText=
								(pCustomDraw->nmcd.uItemState==(CDIS_FOCUS|CDIS_SELECTED))?RGB(255,192,203):RGB(139,0,0);
						}
						clrText=pCustomDraw->clrText;
						// so you don't see the default drawing of the text
						pCustomDraw->clrTextBk=pCustomDraw->clrText=::GetSysColor( COLOR_WINDOW );
					}
					else if((pCustomDraw->nmcd.uItemState&(CDIS_FOCUS|CDIS_SELECTED))==CDIS_SELECTED)
						pCustomDraw->clrTextBk=RGB(227,227,227);

				}
				*pResult = CDRF_NOTIFYPOSTPAINT;
				break;
			}

		case CDDS_ITEMPOSTPAINT:
			{
				HTREEITEM hItem=(HTREEITEM)pCustomDraw->nmcd.dwItemSpec;
				if(hItem && iLevel>0) {
					int i=pCustomDraw->nmcd.uItemState;
					COLORREF bkColor;
					if(i&CDIS_FOCUS) bkColor=::GetSysColor(COLOR_HIGHLIGHT);
					else if(i&CDIS_SELECTED) bkColor=RGB(227,227,227);
					else bkColor=::GetSysColor(COLOR_WINDOW);

					RECT rc;
					TreeView_GetItemRect(pNMHDR->hwndFrom, (HTREEITEM)pCustomDraw->nmcd.dwItemSpec, &rc, 1);
					rc.left-=28;
					CDC::FromHandle(pCustomDraw->nmcd.hdc)->FillSolidRect(&rc,bkColor);
					rc.left+=2;
                    i=SetTextColor(pCustomDraw->nmcd.hdc, clrText);
					int oldBkMode = SetBkMode(pCustomDraw->nmcd.hdc,TRANSPARENT);
					DrawText(pCustomDraw->nmcd.hdc, fldvalue, -1, &rc, DT_LEFT);
					SetBkMode(pCustomDraw->nmcd.hdc, oldBkMode);
					SetTextColor(pCustomDraw->nmcd.hdc,i);
				}
			}
	}
}

LRESULT CShowShapeDlg::OnGetListItem(WPARAM wParam, LPARAM lParam)
{
    //wParam is a handler to the list
    //Make sure message comes from list box
    ASSERT( (HWND)wParam == m_FieldList.GetSafeHwnd() );

    //lParam is a pointer to the data that 
    //is needed for the element
    CQuickList::CListItemData* data = 
        (CQuickList::CListItemData*) lParam;

	data->m_textStyle.m_textPosition = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	data->m_colors.m_textColor = RGB(0,0,0);

	if(!m_pSelLayer) {
		//data->m_text.Empty(); default
		return 0;
	}

    //Get which item and subitem that is asked for.
    UINT fld = data->GetItem()+1;
	CShpDBF *pdb=m_pSelLayer->m_pdb;
	ASSERT(fld<=pdb->NumFlds());

	ASSERT(!data->m_allowEdit); //should have been reset?
	data->m_allowEdit = false;

	if(!data->GetSubItem()) {
		data->m_text=pdb->FldNamPtr(fld);
		data->m_noSelection = true;
		data->m_colors.m_selectedBackColor=data->m_colors.m_selectedBackColorNoFocus=
			data->m_colors.m_backColor=::GetSysColor(COLOR_BTNFACE); //RGB(212,208,200);
        data->m_colors.m_selectedTextColor=RGB(0,0,0);
	}
	else {
		int typ=m_pSelLayer->GetLocFldTyp(fld);
		bool bSpecial=(typ!=0) && (!bIgnoreEditTS || (typ!=LF_CREATED+1 && typ!=LF_UPDATED+1));
		bool bReadOnly=bSpecial || !m_pSelLayer->IsEditable() || SelDeleted();
		bool bHotItem=(data->GetSubItem()==m_FieldList.GetNavigationColumn());

		data->m_colors.m_backColor = RGB(255,255,255);
		data->m_colors.m_navigatedBackColor=RGB(51,153,255); //same as tree

		// 192,192,192 (button shadow) is slightly darker than 240,240,240 (buttonface) which is listbox background --
		data->m_colors.m_selectedBackColorNoFocus=RGB(227,227,227); //RGB(192,192,192) (too dark); 

		if(m_pEditBuf || !pdb->Go(m_uSelRec)) {
			data->m_textStyle.m_hFont=CShpLayer::m_hfFieldList;
			WORD cw=CShpLayer::m_nFieldListCharWidth;
			data->m_textStyle.m_wCharWidth=cw;
			PXCOMBODATA pXCMB=bReadOnly?NULL:m_pSelLayer->XC_GetXComboData(fld);

			bReadOnly=bReadOnly || !bIgnoreEditTS && pXCMB && (pXCMB->wFlgs&XC_READONLY);

			if((data->m_cTyp=pdb->FldTyp(fld))=='M') {

				int memo_chr=0;
				LPCSTR p=NULL;

				if(m_pEditBuf) {
					//Check if memo has been edited --
					it_vMemo it=Get_vMemo_it(fld);
					if(it!=m_vMemo.end()) {
						if(it->pData && *it->pData) {
							p=dbt_GetMemoHdr(it->pData,LEN_MEMO_HDR);
							memo_chr=*p; p+=2;
						}
						else memo_chr=' ';
					}
				}

				if(!memo_chr && !m_pNewShpRec && !pdb->Go(m_uSelRec)) {
					UINT dbtrec=CDBTFile::RecNo((LPCSTR)GetFldPtr(fld));
					if(dbtrec) {
						UINT uDataLen=SIZ_FLDVALUE-1;
						p=dbt_GetMemoHdr(m_pSelLayer->m_pdbfile->dbt.GetText(&uDataLen,dbtrec),LEN_MEMO_HDR);
						memo_chr=*p; p+=2;
					}
				}
				if(!memo_chr) memo_chr=' ';
				//data->m_text=""; default
				data->m_text.AppendChar(memo_chr);
				data->m_button.m_pText=p;
				data->m_button.m_draw=true;
				data->m_button.m_center=false;
				data->m_button.m_width=2;
				data->m_button.m_style=(bReadOnly && memo_chr==' ')?(DFCS_BUTTONPUSH|DFCS_INACTIVE):DFCS_BUTTONPUSH;
				//if(m_iMemoFld==fld)
					//data->m_button.m_style|=DFCS_PUSHED;
				data->m_textStyle.m_uFldWidth=1; //necessary for memo checkbox
			}
			else {
				if(!bReadOnly && bHotItem && data->IsSelected() && pXCMB && pXCMB->wFlgs&XC_HASLIST)
					data->m_pXCMB=pXCMB;

				data->m_allowEdit=!bReadOnly;

				if(bSpecial || bReadOnly) data->m_colors.m_textColor=RGB(128,128,128); 
				GetTrimmedFld(fld,fldvalue,SHP_SIZ_TREELABEL); //rtns length of trimmed data

				if(data->m_cTyp=='L') {
					data->m_button.m_draw=true;
					data->m_button.m_noSelection=false;
					data->m_textStyle.m_uFldWidth=0;
					data->m_allowEdit=!bReadOnly;

					if(bHotItem)
						data->m_colors.m_selectedBackColorNoFocus=data->m_colors.m_navigatedBackColor;


					ASSERT(data->m_button.m_style==DFCS_BUTTONCHECK);
					data->m_button.m_style=(*fldvalue=='Y' || *fldvalue=='T')?
						(DFCS_BUTTONCHECK|DFCS_CHECKED):DFCS_BUTTONCHECK;
					data->m_button.m_center=false;
				}
				else {
					data->m_text.SetString(fldvalue);
					data->m_textStyle.m_uFldWidth=pdb->FldLen(fld);
				}
			}
		}
		else {
			ASSERT(FALSE);
			data->m_text.Empty();
		}
	}

    return 0;
}

LRESULT CShowShapeDlg::OnListNavTest(WPARAM wParam, LPARAM lParam)
{
    //Make sure message comes from list box
    ASSERT( (HWND)wParam == m_FieldList.GetSafeHwnd() );

    CQuickList::CNavigationTest* test = 
        (CQuickList::CNavigationTest*) lParam;

    //The previous column is in test->m_previousColumn.

    //Don't allow navigation to column 2
    if(test->m_newColumn == 0)
        test->m_allowChange = false;

    return 0;
}

BOOL CShowShapeDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN) {
		if(pMsg->wParam==VK_ESCAPE) {
				return TRUE;
		}
	}
	return CResizeDlg::PreTranslateMessage(pMsg);
}

void CShowShapeDlg::UpdateFld(int fnum,LPCSTR text)
{
	LoadEditBuf();

	m_pSelLayer->m_pdb->StoreTrimmedFldData(m_pEditBuf,text,fnum);

	if(!m_bEditDBF) {
		Enable(IDC_RESETATTR);
	}
	m_bEditDBF|=((fnum==m_pSelLayer->m_nLblFld)?2:1);
}

LRESULT CShowShapeDlg::OnBeginEditField(WPARAM wParam, LPARAM lParam)
{
	ASSERT(m_pSelLayer);
	if(!m_bEditDBF && !m_pSelLayer->SaveAllowed(1)) return TRUE; //prevent editing
	return FALSE;
}

LRESULT CShowShapeDlg::OnEndEditField(WPARAM wParam, LPARAM lParam) 
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)wParam;

	int item=pDispInfo->item.iItem;
	ASSERT(pDispInfo->item.iSubItem==1);

    // If pszText is NULL, editing was canceled or there was no change --
    if(pDispInfo->item.pszText != NULL)
    {
		UpdateFld(item+1,pDispInfo->item.pszText);
    }

	UINT key=m_FieldList.GetLastEndEditKey();

	//key==-1 if lost focus

	if(key==VK_RETURN || key==VK_DOWN) {
		if(item<m_FieldList.GetItemCount()-1) {
			m_FieldList.SetSelected(++item);
			//m_FieldList.EditSubItem(item,1);
		}
		else {
			m_FieldList.SetSelected(0);
			//m_FieldList.EditSubItem(0,1);
		}
	}
	else if(key==VK_UP) {
		if(item>0) {
			m_FieldList.SetSelected(--item);
			//m_FieldList.EditSubItem(item,1);
		}
		else {
			item=m_FieldList.GetItemCount()-1;
			m_FieldList.SetSelected(item);
			//m_FieldList.EditSubItem(item,1);
		}
	}

    *(LRESULT *)lParam = 0;
	return 0;
}

void CShowShapeDlg::OnShowCopyloc()
{
	ASSERT(IsLocationValid());
	CString sEast;
	GetDlgItem(IDC_EDIT_EAST)->GetWindowText(sEast);
	sEast.Trim();
	sEast.AppendChar('\t');
	CString sNorth;
	GetDlgItem(IDC_EDIT_NORTH)->GetWindowText(sNorth);
	sNorth.Trim();
	sEast+=sNorth;
	ClipboardCopy(sEast);
}

void CShowShapeDlg::OnShowPasteloc()
{
	CString csPaste;
	if(ClipboardPaste(csPaste,42)) {
		CString s(csPaste);
		bool butm=m_bUtmDisplayed;
		double f0,f1;
		int ix=CheckCoordinatePair(s,butm,&f0,&f1);
		ASSERT(ix);
		cfg_quotes=FALSE;
		int n=cfg_GetArgv((LPSTR)(LPCSTR)csPaste,CFG_PARSE_ALL);
		cfg_quotes=TRUE;
		ASSERT(n>=2 && n<=8 && (n&1)==0);
		n>>=1;
		s=cfg_argv[0];
		for(int i=1;i<n;i++) {
			s+=' '; s+=cfg_argv[i];
		}
		SetText((ix>0)?IDC_EDIT_EAST:IDC_EDIT_NORTH,s);
		s=cfg_argv[n];
		for(int i=n+1;i<2*n;i++) {
			s+=' '; s+=cfg_argv[i];
		}
		SetText((ix>0)?IDC_EDIT_NORTH:IDC_EDIT_EAST,s);

		if(!butm) {
			m_iZoneSel=GetZone(f0,f1);
			SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel));
			if(m_bUtmDisplayed) {
				//auto switch to lat/long!
				m_bUtmDisplayed=false;
				SetRO(IDC_UTMZONE,TRUE);
				Show(IDC_SPIN_UTMZONE,FALSE);
				CheckRadioButton(IDC_TYPEGEO,IDC_TYPEUTM,IDC_TYPEGEO);
				SetCoordLabels();
			}
		}
		else {
			if(!m_bUtmDisplayed) {
				//auto switch to utm!
				m_bUtmDisplayed=true;
				ASSERT(m_iZoneSel);
				Show(IDC_SPIN_UTMZONE,TRUE);
				CheckRadioButton(IDC_TYPEGEO,IDC_TYPEUTM,IDC_TYPEUTM);
				SetCoordLabels();
			}
		}

		m_bEditShp=m_bValidEast=m_bValidNorth=m_bValidZone=true;
		Enable(IDC_RELOCATE);
		Enable(IDC_RESTORE);
	}
}

void CShowShapeDlg::NoSearchableMsg()
{
	CMsgBox(MB_ICONINFORMATION,
		"%s --\nThere are currently no visible layers with searchable attributes. If point shapefiles "
		"still exist you can toggle on their visibility via the Layers/Properties dialog.",
		(LPCSTR)m_pDoc->GetTitle());
}

it_vMemo CShowShapeDlg::Get_vMemo_it(int fld)
{
	it_vMemo it=m_vMemo.begin();
	for(;it!=m_vMemo.end();it++) {
		if(it->fld==fld) break;
	}
	return it;
}

BOOL CShowShapeDlg::UpdateMemo(CShpLayer *pShp,EDITED_MEMO &memo)
{

	if(pShp!=m_pSelLayer || memo.uRec!=m_uSelRec) return FALSE;
	LoadEditBuf();
	it_vMemo it=Get_vMemo_it(memo.fld);

	if(it!=m_vMemo.end()) {
		free(it->pData);
		it->pData=NULL;
	}
	else {
		m_vMemo.push_back(memo);
		it=m_vMemo.end()-1;
	}

	ASSERT(memo.pData);

	UINT uDataLen=0;
	if(*memo.pData) {
		uDataLen=strlen(it->pData=_strdup(memo.pData));
	}
	else it->pData=NULL;
	m_bEditDBF|=1;

	CDBTFile::SetRecNo((LPSTR)GetFldPtr(memo.fld),uDataLen?INT_MAX:0);
	m_FieldList.RedrawSubitems(memo.fld-1,memo.fld-1, 1);
	Enable(IDC_RESETATTR);
	return TRUE;
}

UINT CShowShapeDlg::GetSelectedFieldValue(CString &s,int nFld)
{
	//Called from modal dialog CDlgOptionsGE --
	ASSERT(m_bLaunchGE_from_Selelected);

	UINT len=0;
	LPCSTR pRet="";

	CShpDBF *pdb=m_pSelLayer->m_pdb;

	if(pdb->FldTyp(nFld)=='M') {
		if(m_pEditBuf) {
			it_vMemo it=Get_vMemo_it(nFld);
			if(it!=m_vMemo.end()) {
				//memo has been edited but not saved --
				if(it->pData) {
					pRet=it->pData;
					len=strlen(pRet);
				}
				goto _ret;
			}
		}
		pRet=m_pSelLayer->m_pdbfile->dbt.GetText(&len,GetDbtRec(nFld));
	}
	else {
		if(m_pEditBuf) {
			pRet=(LPCSTR)m_pEditBuf+pdb->FldOffset(nFld);
		}
		else {
			if(pdb->Go(m_uSelRec)) goto _ret;
			pRet=(LPCSTR)pdb->FldPtr(nFld);
		}
		LPCSTR p=pRet+pdb->FldLen(nFld);
		while(p>pRet && xisspace(p[-1])) p--;
		len=p-pRet;
	}
_ret:
	s.SetString(pRet,len);
	return len;
}

void CShowShapeDlg::OpenMemo(UINT nFld)
{
	//Can also be called from CDBGridDlg!

	CShpDBF *pdb=m_pSelLayer->m_pdb;
	if(m_pEditBuf || !pdb->Go(m_uSelRec)) {
		UINT uDbtRec=0;
		UINT uDataLen=0;
		LPCSTR pDataPtr=NULL;
		it_vMemo it=m_vMemo.end();

		if(m_pEditBuf) {
			//This record is being edited --
			ASSERT(m_pSelLayer->IsEditable() && !m_pSelLayer->IsFldReadOnly(nFld));
			if((it=Get_vMemo_it(nFld))!=m_vMemo.end()) {
				//memo has been edited before
				uDbtRec=it->recNo;
				if(pDataPtr=it->pData) uDataLen=strlen(pDataPtr);
				else pDataPtr="";
			}
		}

		bool bViewMode=!CQuickList::m_bCtrl;
		char testbuf[256];
		LPCSTR pTestData=NULL;

		if(!pDataPtr) {
			uDbtRec=GetDbtRec(nFld);
			if(bViewMode && uDbtRec) {
			   ASSERT(uDbtRec!=INT_MAX);
			   pTestData=m_pSelLayer->m_pdbfile->dbt.GetTextSample(testbuf,256,uDbtRec);
			}
			else bViewMode=false;
		}
		else if(bViewMode) {
			pTestData=pDataPtr;
		}

		if(bViewMode) bViewMode=CDBTData::IsTextIMG(pTestData);

		EDITED_MEMO memo;
		if(it!=m_vMemo.end()) {
			memo=*it;
			if(!memo.pData) memo.pData="";
		}
		else {
			memo.uRec=m_uSelRec;
			memo.recNo=uDbtRec;
			memo.fld=nFld;
			memo.recCnt=EDITED_MEMO::GetRecCnt(uDataLen); //original record count in DBT
			memo.pData=(LPSTR)pDataPtr;
			//if memo.pData is NULL, CShpLayer::OpenMemo will initialize it before calling CMemoEditDlg::ReInit
		}
		m_pSelLayer->OpenMemo(this,memo,bViewMode);
	}
}

//User clicked on list. Toggle checkbox/image if hit
LRESULT CShowShapeDlg::OnListClick(WPARAM wParam, LPARAM lParam)
{
	//Called after releasing left mouse button when any list item is clicked
	//Make sure message comes from list box
	ASSERT( (HWND)wParam == m_FieldList.GetSafeHwnd() );

	CQuickList::CListHitInfo *hit= (CQuickList::CListHitInfo*) lParam;

	if(!hit) {
		ASSERT(m_pNewShpRec && m_uSelRec && m_pSelLayer && m_uSelRec==m_pSelLayer->m_pdb->NumRecs()+1);
		ASSERT(m_pNewShpRec->rec==m_uSelRec && m_pNewShpRec->pShp==m_pSelLayer);
		m_FieldList.EditSubItem(m_bConflict?(m_pSelLayer->m_nLblFld-1):0,1);
		m_bConflict=false;
		return 0;
	}

	//TRACE(_T("User hit item %d, subitem %d "), hit->m_item, hit->m_subitem);

	if(hit->m_onButton)
	{
		//We've clicked either a checkbox or a memo field's button --
		UINT nFld=hit->m_item+1;
		if(m_pSelLayer->m_pdb->FldTyp(nFld)=='L') {
			if(!m_pSelLayer->IsEditable() || SelDeleted() ||
				!m_bEditDBF && !m_pSelLayer->SaveAllowed(1)) return 0; //prevent editing

			if(LoadEditBuf()) {
				LPBYTE p=m_pEditBuf+m_pSelLayer->m_pdb->FldOffset(nFld);
				*p=(*p=='Y' || *p=='T')?'N':'Y';
				m_FieldList.RedrawCheckBoxs(hit->m_item, hit->m_item, hit->m_subitem);
				if(!m_bEditDBF)
					GetDlgItem(IDC_RESETATTR)->EnableWindow();
				m_bEditDBF|=1; //ordinary field edit
			}
		}
		else {
			if(m_pSelLayer->IsMemoOpen(m_uSelRec,nFld)) return 0;
			OpenMemo(nFld);
		}
	}
	return 0;
}

void CShowShapeDlg::CancelAddition()
{
	ASSERT(m_pNewShpRec && m_pSelLayer && m_uSelRec && m_uSelRec==m_pSelLayer->m_pdb->NumRecs()+1);

	m_uSelRec=0;
	m_pNewShpRec=NULL;
	m_bEditDBF=0;
	m_bEditShp=false;
	m_pEditBuf=NULL;
	m_vec_relocate.clear();
	ClearVecMemo();
	m_pSelLayer->m_vdbe->resize(--m_pSelLayer->m_nNumRecs);
	m_pSelLayer->m_fpt.resize(m_pSelLayer->m_nNumRecs);
	m_pSelLayer->UpdateShpExtent();
	SetText(IDC_RESETATTR,"Reset Attr");
	Disable(IDC_RESETATTR);
	RemoveSelection();
	m_pDoc->RefreshViews();
}

void CShowShapeDlg::OnBnClickedResetattr()
{
	ASSERT(m_pEditBuf && m_hSelRoot && m_pSelLayer && m_pSelLayer->IsEditable());

	bool bAdding=m_uSelRec>m_pSelLayer->m_pdb->NumRecs();
	bool bEditing=app_pDbtEditDlg && app_pDbtEditDlg->IsRecOpen(m_pSelLayer,m_uSelRec);
	bool bViewing=app_pImageDlg && app_pImageDlg->IsRecOpen(m_pSelLayer,m_uSelRec);
	bool bEditedMemoPending=bEditing && app_pDbtEditDlg->HasEditedText() ||
		 bViewing && app_pImageDlg->HasEditedText();

	if(!bAdding) {
		ASSERT(!m_pNewShpRec);
		if((bEditedMemoPending || m_vMemo.size() || m_bFieldsInitialized) &&
			IDOK!=CMsgBox(MB_OKCANCEL,
			"CAUTION: Resetting attributes will discard all field edits except for any done automatically due to relocation. "
			"Point relocations and memo edits that are still pending in the editor or viewer will not be affected.\n\n"
			"Select OK to proceed."))
				return;

		SelEditFlag()=m_vdbeBegin;
		ClearVecMemo();
		if(m_bFieldsInitialized) {
			//Unfortunally we need to commit the changes made to initialized fields --
			ASSERT(m_uSelRec && m_pSelLayer->HasXCFlds());
			m_pSelLayer->XC_FlushFields(m_uSelRec);
			m_bFieldsInitialized=false;
		}
		m_pEditBuf=NULL;
		m_bEditDBF=0;
		m_FieldList.Invalidate();
		m_PointTree.Invalidate();
		m_PointTree.SetFocus();
		Disable(IDC_RESETATTR);
	}
	else {
		ASSERT(m_pNewShpRec && m_pNewShpRec->rec==m_uSelRec);
		ASSERT(m_uSelRec==m_pSelLayer->m_nNumRecs);
		if(!(bEditedMemoPending || m_vMemo.size()) ||
			IDYES==AfxMessageBox("Select YES to confirm you want to cancel addition of this point.\n"
				"CAUTION: You've added text to at least one memo field.",MB_YESNO)) {
			if(bEditing) app_pDbtEditDlg->Destroy();
			if(bViewing) app_pImageDlg->Destroy();
			CancelAddition();
			if(!m_uLayerTotal) {
				Destroy();
			}
			return;
		}
		PostMessage(WM_QUICKLIST_CLICK,(WPARAM)m_FieldList.GetSafeHwnd());
	}
}

LRESULT CShowShapeDlg::OnEditChange(WPARAM wParam, LPARAM lParam) 
{
	if(!m_bEditDBF) {
		//GetDlgItem(IDC_SAVEATTR)->EnableWindow(wParam);
		GetDlgItem(IDC_RESETATTR)->EnableWindow(wParam);
	}
	return TRUE; //rtn value ignored for now
}

void CShowShapeDlg::OnClearEdited()
{
	ASSERT(m_uSelRec && m_pSelLayer->IsEditable());
	if(m_bEditDBF) {
		if(IDOK!=CMsgBox(MB_OKCANCEL,
			"Record edits are pending for %s.\nPress OK to save the changes before clearing the edited flag, "
			"or CANCEL to leave the flag set.\n\nNote: You can use the Reset Record button to undo the current edits.",
			GetTreeLabel(m_hSelItem)))
			return;
	}
	
	if(m_bEditDBF || HasNewLocFlds()) {
		FlushEdits();
	}

	m_pSelLayer->m_bEditFlags|=SHP_EDITCLR; //Indicates only that we've done some flag clearing
	//SelEditFlag()=(m_vdbeBegin&=SHP_EDITDEL);
	SelEditFlag()&=~(SHP_EDITADD|SHP_EDITLOC|SHP_EDITDBF);
	m_PointTree.Invalidate();
}

BOOL CShowShapeDlg::DeleteTreeItem(HTREEITEM h0)
{
	SHPREC &shprec=(*m_vec_shprec)[m_PointTree.GetItemData(h0)];
	VERIFY(shprec.rec);
	if(shprec.rec && shprec.rec<=shprec.pShp->m_nNumRecs)
		shprec.pShp->SetRecUnselected(shprec.rec);
	shprec.pShp->m_bSelChanged=true;
	shprec.rec=0; //No longer need this
	return m_PointTree.DeleteItem(h0);
}

void CShowShapeDlg::RemoveSelection()
{
	ASSERT(m_hSelItem);
	HTREEITEM h0,h=m_hSelItem;

	m_pDoc->ClearSelChangedFlags();
	if(m_hSelRoot && !m_PointTree.GetNextSiblingItem(m_hSelItem)) {
		//We are deleting the bottom item of the m_hSelRoot branch
		h=m_PointTree.GetPrevSiblingItem(m_hSelItem);
		if(h) {
			//There is a sibling item above the one we're about to delete,
			//so select it first --
			h0=m_hSelItem;
			m_PointTree.SetRedraw(FALSE);
			m_PointTree.SelectItem(h);
			VERIFY(DeleteTreeItem(h0)); //bug -- was below deleteitem!
			m_PointTree.SetRedraw(TRUE);
			UpdateTotal();
			if(m_pDoc->m_bMarkSelected) m_pDoc->RepaintViews();
			m_pDoc->RefreshTables();
			return;
		}
	}
	
	m_PointTree.SetRedraw(FALSE);

	if(m_hSelRoot) {
		//There is either a next sibling beneath m_hSelItem (h==m_hSelItem),
		//or there is no previous sibling (h==NULL), in which case m_hSelItem
		//is the only child item --
		if(!h) {
			//We we want to delete m_hSelRoot also. Let's change the selection first --
			if(!(h=m_PointTree.GetNextSiblingItem(m_hSelRoot)))
				h=m_PointTree.GetPrevSiblingItem(m_hSelRoot);

			h0=m_hSelItem; //item to delete
			if(h) {
				VERIFY(m_PointTree.SelectItem(m_PointTree.GetNextItem(h,TVGN_CHILD)));
			}
			VERIFY(h=m_PointTree.GetParentItem(h0));
			VERIFY(DeleteTreeItem(h0));
			VERIFY(m_PointTree.DeleteItem(h));
			m_uLayerTotal--;
		}
		else {
			//There exists a sibling beneath the item (h==m_hSelItem) we're about to remove --
			ASSERT(h==m_hSelItem);
			VERIFY(DeleteTreeItem(h));
		}
	}
	else {
		//h==m_hSelItem is a root.
		if((h0=m_PointTree.GetNextSiblingItem(h)) ||
					(h0=m_PointTree.GetPrevSiblingItem(h)))
			VERIFY(m_PointTree.SelectItem(m_PointTree.GetNextItem(h0,TVGN_CHILD)));

		while((h0=m_PointTree.GetNextItem(h,TVGN_CHILD))) {
			VERIFY(DeleteTreeItem(h0));
		}
		m_uLayerTotal--;
		VERIFY(m_PointTree.DeleteItem(h));
	}

	m_PointTree.SetRedraw(TRUE);

	if(!m_PointTree.GetCount()) {
		VEC_SHPREC().swap(*m_vec_shprec);
		m_hSelRoot=m_hSelItem=NULL;
		m_pSelLayer=NULL;
		m_uSelRec=0;
		m_pEditBuf=NULL;
		m_pNewShpRec=NULL;
		m_ceLabel.SetFocus();
	}
	UpdateTotal();
	if(m_pDoc->m_bMarkSelected) m_pDoc->RepaintViews();
	m_pDoc->RefreshTables();
}

void CShowShapeDlg::OnUnselect()
{
	if(m_bEditShp && !DiscardShpEditsOK()) 
		return;

	if(m_pEditBuf && IDCANCEL==CMsgBox(MB_OKCANCEL,
			"Record edits are pending for %s.\n"
			"Press OK to save the changes before unselecting this item, or CANCEL to leave it selected.\n\n"
			"Note: You can use the Reset Attr button to undo the current edits.",		
			GetTreeLabel(m_hSelItem))) 
		return;

	if(m_pEditBuf || HasNewLocFlds()) {
		FlushEdits();
	}

	ASSERT(!m_vMemo.size());
	RemoveSelection();
	if(!m_uLayerTotal) {

		ASSERT(!m_PointTree.GetCount());
		//m_pDoc->RefreshTables();

		OnClose();
	}
}

void CShowShapeDlg::OnUpdateUnselect(CCmdUI *pCmdUI)
{
	ASSERT((m_pNewShpRec==NULL)==((m_pEditBuf==NULL&&m_uSelRec==0) || m_uSelRec<=m_pSelLayer->m_pdb->NumRecs()));
	pCmdUI->Enable(!m_pEditBuf || m_uSelRec<=m_pSelLayer->m_pdb->NumRecs());
}

void CShowShapeDlg::OnDelete()
{
	ASSERT(m_pSelLayer->IsShpEditable());

	bool bLocked=!IsSelMovable();
	if(bLocked || (app_pDbtEditDlg && app_pDbtEditDlg->IsRecOpen(m_pSelLayer,m_uSelRec) ||
			app_pImageDlg && app_pImageDlg->IsRecOpen(m_pSelLayer,m_uSelRec))) {
		LPCSTR p=bLocked?"edits are pending":"a memo field is open";
		CMsgBox("%s --\n\nThis record's delete status can't be changed while %s.",GetTreeLabel(m_hSelItem),p);
		return;
	}

	if(!m_pSelLayer->ToggleDelete(m_uSelRec)) return;
	m_pSelLayer->m_pdb->FlushRec();

	m_pSelLayer->UpdateShpExtent();
	m_pSelLayer->m_bEditFlags|=SHP_EDITDEL;
	m_pSelLayer->SaveShp();
	m_pSelLayer->UpdateSizeStr();
	if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(m_pSelLayer);

	if(m_pSelLayer->IsGridDlgOpen()) 
		m_pSelLayer->m_pdbfile->pDBGridDlg->RefreshTable(m_uSelRec);

	//Check if point is in a view that will require updating --
	ASSERT(!CWallsMapView::m_bTestPoint);
	{
		CSyncHint hint;
		hint.fpt=&m_pSelLayer->m_fpt[m_uSelRec-1];
		m_pDoc->UpdateAllViews(NULL,LHINT_TESTPOINT,&hint);
		m_pDoc->RefreshViews();
		CWallsMapView::m_bTestPoint=false;
	}

	bool bDel=SelDeleted();
	SetRO(IDC_EDIT_EAST,bDel);
	SetRO(IDC_EDIT_NORTH,bDel);
	m_PointTree.Invalidate();
	m_FieldList.Invalidate();
}

UINT CShowShapeDlg::GetDbtRec(int nFld)
{
	LPSTR p=0;
	CShpDBF *pdb=m_pSelLayer->m_pdb;
	if(m_pEditBuf) {
		p=(LPSTR)m_pEditBuf+pdb->FldOffset(nFld);
	}
	else {
		p=(LPSTR)pdb->FldPtr(m_uSelRec,nFld);
	}
	return p?CDBTFile::RecNo(p):0;
}

void CShowShapeDlg::ApplyChangedPoint(const CFltPoint &fpt)
{
	CFltPoint fptOld=m_pSelLayer->m_fpt[m_uSelRec-1]; //needed if initializing based on location
	m_pSelLayer->m_fpt[m_uSelRec-1]=fpt;

	m_pSelLayer->m_bEditFlags|=SHP_EDITSHP;
	SelEditFlag()|=(SHP_EDITSHP|SHP_EDITLOC);

	m_pSelLayer->UpdateShpExtent();
	m_pDoc->RefreshViews();
	CWallsMapView::m_bTestPoint=false;

	//Refresh other document views that contain this layer unconditionally  --
	if(m_pSelLayer->m_pdbfile->nUsers>1)
		m_pSelLayer->UpdateDocsWithPoint(m_uSelRec);

	if(m_pSelLayer->HasXCFlds()) {
		if(m_pSelLayer->XC_RefreshInitFlds(GetRecPtr(),fpt,2,&fptOld)) {
			if(!m_pNewShpRec) {
				m_bFieldsInitialized=true;
				if(!m_bEditDBF) {
					Enable(IDC_RESETATTR);
				}
				m_bEditDBF|=1;
			}
			m_FieldList.Invalidate();
		}
	}
	Disable(IDC_RESTORE);
	Disable(IDC_RELOCATE);
	m_PointTree.SetFocus();
}

void CShowShapeDlg::Relocate(CFltPoint &fpt)
{
	if(!m_pSelLayer->SaveAllowed(1)) return;
	m_vec_relocate.push_back(m_pSelLayer->m_fpt[m_uSelRec-1]);
	ApplyChangedPoint(fpt);
	RefreshSelCoordinates();
	Enable(IDC_UNDORELOCATE);
}

void CShowShapeDlg::OnBnClickedRelocate()
{

	if(!m_pSelLayer->SaveAllowed(1)) return;

	CFltPoint fpt;
	if(!GetEditedPoint(fpt,true)) return;

/*
	CString s;
	s.Format("%.*f",m_bUtmDisplayed?2:7,fpt.x);
	SetText(m_bUtmDisplayed?IDC_EDIT_EAST:IDC_EDIT_NORTH,s);
	s.Format("%.*f",m_bUtmDisplayed?2:7,fpt.y);
	SetText(m_bUtmDisplayed?IDC_EDIT_NORTH:IDC_EDIT_EAST,s);
*/
	m_vec_relocate.push_back(m_pSelLayer->m_fpt[m_uSelRec-1]);
	ApplyChangedPoint(fpt);

	RefreshCoordinates(fpt);

	Enable(IDC_UNDORELOCATE);

	m_bEditShp=false;
}

void CShowShapeDlg::OnBnClickedUndoRelocate()
{
	ASSERT(m_vec_relocate.size());
	CFltPoint &fpt=m_vec_relocate.back();
	m_vec_relocate.pop_back();
	//m_pSelLayer->m_fpt[m_uSelRec-1]=fpt;
	ApplyChangedPoint(fpt);
	if(!m_vec_relocate.size()) {
		m_PointTree.SetFocus();
		Disable(IDC_UNDORELOCATE);
		if(!(m_vdbeBegin&SHP_EDITLOC)) {
			//point wasn't previously relocated --
			SelEditFlag()&=~SHP_EDITLOC;
			m_PointTree.Invalidate();
		}
	}
	RefreshSelCoordinates();
}

void CShowShapeDlg::AddShape(CFltPoint &fpt,CShpLayer *pLayer)
{
	//If pLayer is null we are adding a point immediately beneath the
	//currently selected item, possibly a root item. Otherwise the dialog
	//should be empty of data and freshly initialized.

	m_pNewShpRec=NULL;

	try {
		m_vec_shprec->push_back(SHPREC(pLayer,0));
	}
	catch(...) {
		return;
	}

	it_shprec it=m_vec_shprec->end()-1;
	m_pNewShpRec=&(*it);

	if(!pLayer) {
		//Adding a similar point beneath the selected m_PointTree item --
		ASSERT(m_pSelLayer && m_pSelLayer->IsShpEditable());

		TV_INSERTSTRUCT tvi;

		if(m_hSelRoot) {
			ASSERT(m_uSelRec);
			//Creates similar record with provisional label
			if(!(it->rec=m_pSelLayer->InitEditBuf(fpt,/*pLayer?0:*/m_uSelRec))) goto _resize;
			tvi.hParent=m_hSelRoot;
			tvi.hInsertAfter=m_hSelItem;
		}
		else {
			ASSERT(!m_uSelRec);
			//Creates empty record with provisional label
			if(!(it->rec=m_pSelLayer->InitEditBuf(fpt,0))) goto _resize;
			tvi.hParent=m_hSelItem;
			tvi.hInsertAfter=TVI_FIRST;
		}
		it->pShp=m_pSelLayer;
		tvi.item.mask=TVIF_DI_SETITEM|TVIF_PARAM|TVIF_TEXT|TVIF_SELECTEDIMAGE|TVIF_IMAGE;
		tvi.item.pszText=LPSTR_TEXTCALLBACK;
		tvi.item.lParam=m_vec_shprec->size()-1;
		tvi.item.iSelectedImage=tvi.item.iImage=0;
		//tvi.item.stateMask=tvi.item.state=TVIS_SELECTED;
		VERIFY(m_PointTree.SelectItem(m_PointTree.InsertItem(&tvi)));
		UpdateTotal();
		if(m_pDoc->m_bMarkSelected) m_pDoc->RepaintViews();
	}
	else {
		//Caller has just created dialog or called ReInit() with empty m_vec_shprec
		ASSERT(!m_hSelItem && !m_pSelLayer);
		ASSERT(m_vec_shprec->size()==1);
		if(!(it->rec=pLayer->InitEditBuf(fpt,0)))
			goto _resize;
		FillPointTree();
	}

	Enable(IDC_RESETATTR);
	SetText(IDC_RESETATTR,"Cancel Add");
	return;

_resize:
	//memory error -- should alert and close dialog
	AfxMessageBox("Could not add record -- out of memory!");
	m_vec_shprec->erase(it);
	m_pNewShpRec=NULL;
	if(pLayer) Destroy();
}

bool CShowShapeDlg::ConfirmFlyToEdited(CFltPoint &fpt,bool &bCross)
{
	//Edits to this point's location are pending.
	//Select YES to center on proposed location instead of current one.
	int iret=AfxMessageBox(IDS_MSG_SHOWCENTER,MB_YESNOCANCEL);
	if(iret==IDCANCEL) return false;
	if(iret==IDYES) {
		if(!GetEditedPoint(fpt)) {
			ASSERT(0);
			return false;
		}
		if(!bCross && !m_pDoc->Extent().IsPtInside(fpt)) {
			//Operation canceled. The proposed location must be within the layerset's total extent.
			AfxMessageBox(IDS_ERR_SHOWCENTER,MB_OK);
			return false;
		}
		bCross=true;
	}
	else bCross=false;
	return true;
}

void CShowShapeDlg::ZoomToPoint(double fZoom)
{
	ASSERT(m_uSelRec);
	CFltPoint fpt=m_pSelLayer->m_fpt[m_uSelRec-1];
	bool bCross=false;

	if(m_bEditShp && IsLocationValid()) {
		if(!ConfirmFlyToEdited(fpt,bCross)) return;
	}
	else if(!m_pSelLayer->IsVisible()) bCross=true; 

#ifdef _DEBUG
	HTREEITEM hItem=m_PointTree.GetSelectedItem();
	ASSERT(hItem && m_PointTree.GetParentItem(hItem));
#endif
	POSITION pos = m_pDoc->GetFirstViewPosition();
	CWallsMapView *pView=(CWallsMapView *)m_pDoc->GetNextView(pos);
	if(pView) {
		CWallsMapView::m_bMarkCenter=bCross;
		pView->CenterOnGeoPoint(fpt,fZoom);
	}
}

void CShowShapeDlg::OnShowCenter()
{
	ZoomToPoint(1.0);
}

void CShowShapeDlg::OnZoomView()
{
	if(m_hSelRoot) ZoomToPoint(4.0);
	else {
		VEC_DWORD vRec;
		GetBranchRecs(vRec);
		m_pSelLayer->FitExtent(vRec,TRUE);
	}
}

void CShowShapeDlg::OnTimer(UINT nIDEvent)
{
	if(nIDEvent==0x64) {
		Hide(IDC_ST_NOMATCHES);
        KillTimer(0x64);
    }
}

void CShowShapeDlg::GetBranchRecs(VEC_DWORD &vRec)
{
	//Build a vector of record numbers for a CShpLayer operation --
	ASSERT(m_hSelItem && !m_hSelRoot);
	ASSERT(m_pSelLayer==(CShpLayer *)m_PointTree.GetItemData(m_hSelItem));

	HTREEITEM h=m_PointTree.GetNextItem(m_hSelItem,TVGN_CHILD);

	while(h) {
		DWORD rec=m_PointTree.GetItemData(h);
		ASSERT(rec<m_vec_shprec->size() && (*m_vec_shprec)[rec].pShp==m_pSelLayer);
		if(rec=(*m_vec_shprec)[rec].rec)
			vRec.push_back(rec);
		h=m_PointTree.GetNextSiblingItem(h);
	}
	ASSERT(vRec.size());
}

void CShowShapeDlg::OnExportShapes()
{
	VEC_DWORD vRec;
	GetBranchRecs(vRec);
	m_pSelLayer->ExportRecords(vRec,true); //true if selected (affects title only)
}

void CShowShapeDlg::OnBnClickedMarkselected()
{
	m_pDoc->m_bMarkSelected=IsChecked(IDC_MARKSELECTED);
	if(m_vec_shprec->size())
		m_pDoc->RepaintViews();
}

void CShowShapeDlg::OnBranchsymbols()
{
	CDlgSymbols dlg(m_pDoc,(CShpLayer *)m_PointTree.GetItemData(m_hSelItem));
	if(IDOK==dlg.DoModal()) {
		//m_PointTree.RedrawWindow(); //not needed apparently
	}
}

void CShowShapeDlg::RefreshBranchTitle(HTREEITEM h)
{
	if(h) {
		CShpLayer *pShp=(CShpLayer *)m_PointTree.GetItemData(h);
		ASSERT(pShp);
		CString cs;
		m_PointTree.SetItemText(h,pShp->GetPrefixedTitle(cs));
		m_PointTree.RedrawWindow();
	}
}

void CShowShapeDlg::OnSetEditable()
{
	//Toggle editable state
	ASSERT(m_hSelItem && !m_hSelRoot);
	CShpLayer *pShp=(CShpLayer *)m_PointTree.GetItemData(m_hSelItem);
	if(pShp->SetEditable()) {
		m_PointTree.SetFocus();
		RefreshBranchTitle(m_hSelItem);
		if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
			pLayerSheet->RefreshListItem(pShp);
		m_pDoc->SetChanged();
	}
}

void CShowShapeDlg::FlyToWeb(bool bOptions)
{
	CString s(CMainFrame::m_csWebMapFormat);
	int idx=CMainFrame::m_idxWebMapFormat;
	BOOL bRet=1;
	bool bCross=false;
	CFltPoint fpt;

	if(bOptions) {
		if(!(bRet=GetMF()->GetWebMapFormatURL(TRUE)))
			return;
	}
	if(m_bEditShp && IsLocationValid()) {
		bCross=true;
		if(!ConfirmFlyToEdited(fpt,bCross)) {
			ASSERT(0);
			return;
		}
	}
	m_pSelLayer->OnLaunchWebMap(m_uSelRec,bCross?&fpt:NULL);
	if(bRet>1) {
		CMainFrame::m_csWebMapFormat=s;
		CMainFrame::m_idxWebMapFormat=idx;
	}
}

void CShowShapeDlg::OnLaunchWebMap()
{
	ASSERT(m_hSelItem && m_hSelRoot && m_bNadToggle && m_uSelRec);
	FlyToWeb(false);
}

void CShowShapeDlg::OnOptionsWebMap()
{
	FlyToWeb(true);
}

void CShowShapeDlg::GE_Export(bool bOptions)
{
	ASSERT(m_bNadToggle);
	VEC_DWORD vRec;
	CFltPoint fpt;
	bool bCross=false;

	if(m_hSelRoot) {
		if(m_bEditShp && IsLocationValid()) {
			bCross=true;
			if(!ConfirmFlyToEdited(fpt,bCross)) {
				ASSERT(0);
				return;
			}
		}
		vRec.push_back(m_uSelRec);
	}
	else {
		GetBranchRecs(vRec);
	}

	if(bOptions) {
		m_bLaunchGE_from_Selelected=m_hSelRoot!=NULL;
		m_pSelLayer->OnOptionsGE(vRec,bCross?&fpt:NULL);
		m_bLaunchGE_from_Selelected=false;
	}
	else {
		ASSERT(!m_bLaunchGE_from_Selelected);
		m_pSelLayer->OnLaunchGE(vRec,bCross?&fpt:NULL);
	}
}

void CShowShapeDlg::OnLaunchGE()
{
	ASSERT(GE_IsInstalled());
	GE_Export(false);
}

void CShowShapeDlg::OnOptionsGE()
{
	GE_Export(true);
}

int CShowShapeDlg::CopySelected(CShpLayer *pDropLayer,LPBYTE pSrcFlds,BOOL bConfirm,HTREEITEM *phDropItem /*=NULL*/)
{
	BeginWaitCursor();

	UINT uRec=0,nCopied=0;

	#ifdef _DEBUG
	UINT nToCopy=m_hSelRoot?1:GetChildCount(m_hSelItem);
	UINT nDropChildCount=0;
	HTREEITEM hDropParent=NULL;
	if(phDropItem) {
		hDropParent=m_PointTree.GetParentItem(*phDropItem);
		if(!hDropParent) hDropParent=*phDropItem;
		nDropChildCount=GetChildCount(hDropParent);
	}
	#endif

	HTREEITEM hDragItem=m_hSelItem;
	if(!m_hSelRoot)
		VERIFY(hDragItem=m_PointTree.GetChildItem(hDragItem));

	//Copy selected item (if not the root) or copy each child of a branch --

	bool bUpdateLocFlds=pDropLayer->HasLocFlds() && 
		(bConfirm>1 || !pDropLayer->IsSameProjection(m_pSelLayer));

	TRUNC_DATA td;
	m_csCopyMsg.Empty();

	while(TRUE) {
		UINT uRecOld=(*m_vec_shprec)[m_PointTree.GetItemData(hDragItem)].rec;
		ASSERT(!m_hSelRoot || uRecOld==m_uSelRec);
		uRec=pDropLayer->CopyShpRec(m_pSelLayer,uRecOld,pSrcFlds,bUpdateLocFlds,td);
		if(!uRec) {
			ASSERT(0);
			break; //error
		}
		ASSERT(uRec==pDropLayer->m_nNumRecs && uRec==pDropLayer->m_pdb->NumRecs());
		if(phDropItem) {
			nCopied++; //avoid assert only
			m_pSelLayer->SetRecUnselected(uRecOld);
			pDropLayer->SetRecSelected(m_uSelRec=uRec);
			SHPREC &shprec=(*m_vec_shprec)[m_PointTree.GetItemData(hDragItem)];
			shprec.rec=uRec;
			shprec.pShp=pDropLayer;
			*phDropItem=m_PointTree.MoveItem(hDragItem,*phDropItem);
			if(m_hSelRoot || !(hDragItem=m_PointTree.GetChildItem(m_hSelItem)))
				break;
		}
		else {
			nCopied++;
			if(!(nCopied%100) && m_pCopySelectedDlg) {
				if(!m_pCopySelectedDlg->ShowCopyProgress(nCopied)) break;
			}
			if(m_hSelRoot || !(hDragItem=m_PointTree.GetNextItem(hDragItem, TVGN_NEXT)))
				break;
		}
	}

	ASSERT(nCopied==nToCopy);
	ASSERT(!nDropChildCount || GetChildCount(hDropParent)==nCopied+nDropChildCount);

	VERIFY(!pDropLayer->m_pdb->Flush());

	if(pDropLayer->HasMemos())
	  VERIFY(pDropLayer->m_pdbfile->dbt.FlushHdr());

	pDropLayer->m_bEditFlags|=SHP_EDITADD;
	pDropLayer->SaveShp();
	pDropLayer->UpdateSizeStr();
	if(hPropHook && m_pDoc==pLayerSheet->GetDoc())
		pLayerSheet->RefreshListItem(pDropLayer);
	m_pDoc->LayerSet().UpdateExtent();
	m_pDoc->RefreshViews(); //required for new points to be selectable

	//Refresh other document views that contain this layer unconditionally  --
	if(pDropLayer->m_pdbfile->nUsers>1)
		pDropLayer->UpdateDocsWithPoint(pDropLayer->m_nNumRecs);

	EndWaitCursor();

	if(!phDropItem)
		m_csCopyMsg.Format("%u item%s successfully copied to %s.",nCopied,(nCopied==1)?"":"s",pDropLayer->Title());

	if(td.count) {
		m_csCopyMsg.AppendFormat("CAUTION: %u field values were truncated, the first being field %s of record %u.",
			td.count,pDropLayer->m_pdb->FldNamPtr(td.fld),td.rec);
	}
	return phDropItem?uRec:nCopied;
}

UINT CShowShapeDlg::CopySelectedItem(CShpLayer *pDropLayer,HTREEITEM *phDropItem)
{
	//phDropItem!=NULL we'll be rearranging the tree by changing the parent shapefile of
	//the dragged item.

	ASSERT(pDropLayer && pDropLayer->IsProjected()==(pDropLayer->Zone()!=0));

	if(!pDropLayer) return 0;

	int count=m_hSelRoot?1:GetChildCount(m_hSelItem);

	if(pDropLayer->m_bDragExportPrompt) {
		CString msg;
		if(count==1) {
			HTREEITEM h=m_hSelRoot?m_hSelItem:m_PointTree.GetChildItem(m_hSelItem);
			ASSERT(h);
			msg.Format("%s\nAppend this record",GetTreeLabel(h));
		}
		else {
			msg.Format("Append %u record%s in %s", count, (count>1)?"s":"", m_pSelLayer->Title());
		}
		msg.AppendFormat(" to %s?\n\nNOTE: The copied record%s will ",pDropLayer->Title(),(count>1)?"s":"");

		if(phDropItem) msg+="be removed from the selected set but not deleted.";
		else {
			msg.AppendFormat("stay selected so you can delete %s to complete a move.",(count>1)?"them":"it");
		}
		int i=MsgCheckDlg(m_hWnd,MB_OKCANCEL,(LPCSTR)msg,m_pSelLayer->Title(),"Don't ask this again for this target");
		if(!i) return -1;
		pDropLayer->m_bDragExportPrompt=(i==1);
	}

	ASSERT(pDropLayer->CanAppendShpLayer(m_pSelLayer));

	BYTE bSrcFlds[256];
	LPBYTE pSrcFlds=NULL;
	BOOL bConfirm=0; //0,1,2
	if(!pDropLayer->m_pdb->FieldsMatch(m_pSelLayer->m_pdb)) {
		if(!(bConfirm=pDropLayer->ConfirmAppendShp(m_pSelLayer,pSrcFlds=bSrcFlds)))
			return -1;
	}
	if(phDropItem) {
		m_PointTree.SetRedraw(FALSE);
		m_bDroppingItem=true;
	}

	if(count>=5000) {
		CCopySelectedDlg dlg(pDropLayer,pSrcFlds,bConfirm,count,this);
		m_pCopySelectedDlg=&dlg;
		dlg.DoModal();
	}
	else {
		m_pCopySelectedDlg=NULL;
		count=CopySelected(pDropLayer, pSrcFlds, bConfirm, phDropItem);
	}
	if(!m_csCopyMsg.IsEmpty()) AfxMessageBox(m_csCopyMsg);

	return count;
}

void CShowShapeDlg::OnExportTreeItems(UINT id)
{
	if(CancelSelChange())
		return;

	ASSERT(id<ID_APPENDLAYER_N);

	CShpLayer *pLayer=(CShpLayer *)CLayerSet::vAppendLayers[id-ID_APPENDLAYER_0];
	ASSERT(pLayer);

	CopySelectedItem(pLayer,NULL);
}

void CShowShapeDlg::OnDropTreeItem(HTREEITEM hDropItem)
{
	ASSERT(IsSelMovable() && IsSelDroppable(hDropItem));

	#ifdef _DEBUG
	UINT numSelected=NumSelected(); //shouldn't change
	ASSERT(numSelected>1);
	#endif

	HTREEITEM hDropRoot=m_PointTree.GetParentItem(hDropItem);

	bool bUpdateLocFlds=false;
	UINT uRec;

	if(!m_hSelRoot || m_hSelRoot!=hDropRoot && m_hSelRoot!=hDropItem) {

		if(!m_hSelRoot) {
			if(hDropRoot) hDropItem=hDropRoot;
		}
		if(!hDropRoot) hDropRoot=hDropItem;

		CShpLayer *pDropLayer=(CShpLayer *)m_PointTree.GetItemData(hDropRoot);

		uRec=CopySelectedItem(pDropLayer,&hDropItem);

		if((int)uRec<=0)
			return;

		if(!m_hSelRoot || !m_PointTree.GetChildItem(m_hSelRoot)) {
			VERIFY(m_PointTree.DeleteItem(m_hSelRoot?m_hSelRoot:m_hSelItem));
			m_uLayerTotal--; //bug fixed -- this was omitted (10/16/2014)
		}

		//The selected layer now has fewer items selected --
		if(m_pSelLayer->IsGridDlgOpen())
			m_pSelLayer->m_pdbfile->pDBGridDlg->InvalidateTable();

		if(m_hSelRoot) {
			m_hSelItem=hDropItem;
			m_hSelRoot=hDropRoot;
			m_pSelLayer=pDropLayer;
		}
		else {
			m_uSelRec=0;
			m_hSelItem=hDropRoot;
			m_pSelLayer=pDropLayer;
		}
	}
	else {
		//Update position in branch only --
		m_PointTree.SetRedraw(FALSE);
		m_bDroppingItem=true;
		m_hSelItem=m_PointTree.MoveItem(m_hSelItem,hDropItem);
		//m_pSelLayer, m_uRelRec, m_hSelRoot unchanged
	}

	ASSERT(numSelected==NumSelected());

	// select the last item we inserted
	VERIFY(m_PointTree.SelectItem(m_hSelItem));
	m_PointTree.SetRedraw(TRUE);
	m_PointTree.Invalidate();
	if(bUpdateLocFlds) m_FieldList.Invalidate();
	m_bDroppingItem=false;

}

LRESULT CShowShapeDlg::OnAdvancedSrch(WPARAM fcn,LPARAM)
{
	if(fcn==2) {
		OnBnClickedFindbylabel();
		return TRUE;
	}
	if(CancelSelChange()) {
		m_bDlgOpened=false;
		return TRUE;
	}
	m_bDlgOpened=false;

	if(!m_pDoc->HasSearchableLayers()) {
		NoSearchableMsg();
		return TRUE;
	}
	CAdvSearchDlg dlg(m_pDoc,AfxGetMainWnd());

	CString text;
	if(GetEditLabel(text))
		dlg.m_FindString=text;

	if(IDOK==dlg.DoModal()) {
		ReInit(m_pDoc);
	}
	return TRUE;
}

void CShowShapeDlg::ShowNoMatches()
{
	SetText(IDC_ST_NOMATCHES,CWallsMapDoc::m_bSelectionLimited?"Aborted!":"No matches!");
	Show(IDC_ST_NOMATCHES);
	SetTimer(0x64,1000,NULL); //One second display
}

void CShowShapeDlg::OnBnClickedFindbylabel()
{
	CString text;
	if(!GetEditLabel(text)) {
		ClearLabel();
		return;
	}

	if(CancelSelChange()) {
		m_bDlgOpened=false;
		return;
	}
	m_bDlgOpened=false;

	if(!m_pDoc->IsSetSearchable(FALSE)) {
		if(!m_pDoc->HasSearchableLayers()) NoSearchableMsg();
		else {
			UINT id=CMsgBox(MB_OKCANCEL,
				"%s --\nVisible point shapefiles are present but they're currently either flagged "
				"non-searchable or have all their fields excluded from searches.\n\n"
				"Select OK to open the Text Search dialog where you can search-enable the "
				"appropriate shapefiles and/or fields.",(LPCSTR)m_pDoc->GetTitle());

			if(id==IDOK) OnAdvancedSrch(0,0);
		}
		return;
	}

	BeginWaitCursor();
	m_pDoc->InitVecShpopt(false);
	if(!m_pDoc->FindByLabel(text,0)) {
		m_pDoc->ReplaceVecShprec();
		EndWaitCursor();
		ShowNoMatches();
	}
	else EndWaitCursor();
	ClearVecShpopt();
	FillPointTree(); //will call ClearLabel()
	Enable(IDC_FINDBYLABEL);
	SetLabel(text);
	m_ceLabel.SetFocus();
}

void CShowShapeDlg::OnEnChangeEditLabel()
{
	CString text;
	Enable(IDC_FINDBYLABEL,GetEditLabel(text)!=0);
}

void CShowShapeDlg::OnOpenTable()
{
	VEC_DWORD vRec;
	GetBranchRecs(vRec);
	m_pSelLayer->OpenTable(&vRec);
}

void CShowShapeDlg::OnBnClickedTypegeo()
{
	ASSERT(m_bNadToggle);
	CFltPoint fpt;

	if(m_uSelRec) {
		ASSERT(m_bValidNorth && m_bValidEast && m_bValidZone);
		if(m_bEditShp) {
			//if dispaying Lat/Long, the resulting UTM zone could be out of range
			VERIFY(GetEditedPoint(fpt));
		}
		else {
		  fpt=m_pSelLayer->m_fpt[m_uSelRec-1];
		}
	}

	m_bUtmDisplayed=!IsChecked(IDC_TYPEGEO);
	m_pDoc->m_bShowAltGeo=(m_bUtmDisplayed != (m_iZoneDoc!=0));

	SetCoordLabels();

	if(m_uSelRec)
		RefreshCoordinates(fpt);
}

void CShowShapeDlg::OnBnClickedDatum()
{
	ASSERT(m_bNadToggle && (!m_uSelRec || m_bValidNorth && m_bValidEast && m_bValidZone));
	CFltPoint fpt;

	if(m_uSelRec) {
		if(m_bEditShp)
			VERIFY(GetEditedPoint(fpt));
		else
		  fpt=m_pSelLayer->m_fpt[m_uSelRec-1];
	}

	int iNad=IsChecked(IDC_DATUM1)?1:2;
	m_pDoc->m_bShowAltNad=(iNad!=m_iNadDoc);
	m_iNadSel=iNad;

	if(m_uSelRec)
		RefreshCoordinates(fpt);
}

void CShowShapeDlg::OnSpinUtm(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ASSERT(m_bNadToggle);
	ASSERT(m_bValidZone);

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	char buf[12];
	GetText(IDC_UTMZONE,buf,12);
	int i=atoi(buf);
	ASSERT(i && i>=-60 && i<=60);
	i-=pNMUpDown->iDelta;
	if(!i) i-=pNMUpDown->iDelta;
	else if(i>60) i=-60;
	else if(i<-60) i=60;
	SetText(IDC_UTMZONE,GetIntStr(m_iZoneSel=i));

	if(!(IsEnabled(IDC_RELOCATE)))
		Enable(IDC_RELOCATE);

	if(!m_bEditShp) {
		Enable(IDC_RESTORE);
		m_bEditShp=true;
	}

	*pResult = 0;
}

LRESULT CShowShapeDlg::OnGetToolTip(WPARAM wParam, LPARAM lParam)
{
    ASSERT( (HWND)wParam == m_FieldList.GetSafeHwnd() );
	QUICKLIST_TTINFO *tt=(QUICKLIST_TTINFO *)lParam;
	ASSERT(tt->nCol==0);
	//tt->nRow+1 is field number

	LPCSTR pDesc;

	if(!m_hSelRoot || !m_pSelLayer || !m_pSelLayer->HasXDFlds() ||
		!(pDesc=m_pSelLayer->XC_GetXDesc(tt->nRow+1))) return FALSE;

	if(!tt->bW) {
		ASSERT(0); //is this possible?
		tt->pvText80=(LPVOID)pDesc;
		return TRUE;
	}
	UINT len=strlen(pDesc)+1;
	if(!m_pwchTip || m_wchTipSiz<len) {
		m_wchTipSiz=80*(1+(len-1)/80);
		m_pwchTip=(WCHAR *)realloc(m_pwchTip,m_wchTipSiz*sizeof(WCHAR));
	}
	if(m_pwchTip) { 
		_mbstowcsz(m_pwchTip,pDesc,len);
		ASSERT(m_pwchTip[len-1]==(WCHAR)0);
		tt->pvText80=(LPVOID)m_pwchTip;
		return TRUE;
	}
	return FALSE;
}

LRESULT CShowShapeDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1004);
	return TRUE;
}
void CShowShapeDlg::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
	//We come here before LVS_OWNERDRAWFIXED is turned off in OnInitDialog() --
	if(wOSVersion>=0x600 && nIDCtl==IDC_FIELD_LIST) {
		lpMIS->itemHeight-=2;
	}
}



