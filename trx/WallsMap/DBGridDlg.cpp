
// DBGridDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ShpLayer.h"
#include "mainfrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ColumnDlg.h"
#include "DBGridDlg.h"
#include "DbtEditDlg.h"
#include "LayerSetSheet.h"
#include "TabCombo.h"
#include "ShowShapeDlg.h"
#include "ImageViewDlg.h"
#include "LinkTestDlg.h"
#include "ReloadDlg.h"
#include "MsgCheck.h"
#include "KmlDef.h"
#include "DlgExportXLS.h"
#include "WebMapFmtDlg.h"
#include "CopySelectedDlg.h"
#include "TableFillDlg.h"
#include "TableReplDlg.h"

CDBGridDlg::CDBGridDlg(CShpLayer *pShp,VEC_DWORD *pvRec,CWnd* pParent /*=NULL*/)
	: CResizeDlg(CDBGridDlg::IDD, pParent)
	, m_pShp(pShp)
	, m_pdbfile(pShp->m_pdbfile)
	, m_pdb(NULL)
	, m_iColSorted(-1)
	, m_nSelCount(0)
	, m_uDelCount(0)
	, m_uPromptRec(0)
	, m_bRedrawOff(false)
	, m_pFindDlg(NULL)
	, m_wFlags(0) //should be saved for pShp?
	, m_iLastFind(-1)
	, m_bReloaded(false)
	, m_bAllowEdits(false)
	, m_pvxc(NULL)
	, m_nSubset(0)
	, m_pwchTip(NULL)
	, m_bRestoreLayout(false)
	, m_bAllowPolyDel(false)
	, m_bExpandMemos(true) //just for now
	, m_sort_pfldbuf(NULL)
	, m_sort_lenfldbuf(0)
{
	if(pShp->ShpType()!=CShpLayer::SHP_POINT)
		VERIFY(pShp->Init_ppoly());

	if(pvRec) {
		m_vRecno.swap(*pvRec);
		VERIFY(m_nNumRecs=m_vRecno.size());
		CountDeletes();
	}
	else InitRecno(); //sets m_bReloaded=true
	Create(CDBGridDlg::IDD,pParent);
}

CTRXFile CDBGridDlg::m_trx;
//bool CDBGridDlg::m_bNotAllowingEdits=false;

CDBGridDlg::~CDBGridDlg()
{
	AllowPolyDel(false);
	m_pdbfile->pDBGridDlg=NULL;
	delete m_pvxc;
	free(m_pwchTip);
	free(m_sort_pfldbuf);
}

void CDBGridDlg::OnClose() 
{
	//Cancel, "X", and m_pdbfile->pDBGridDlg->Destroy() all come here for now.

	SaveShpColumns();

	VERIFY(DestroyWindow()); //this invokes PostNcDestroy()
}

void CDBGridDlg::OnOK() 
{
	//OK, will always save for now --

	SaveShpColumns();

	VERIFY(DestroyWindow()); //this invokes PostNcDestroy()
}

void CDBGridDlg::PostNcDestroy() 
{
	ASSERT(m_pdbfile->pDBGridDlg==this);
	delete this;
}

BOOL CDBGridDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN) {
		if(pMsg->wParam==VK_ESCAPE) {
				return TRUE;
		}
	}
	return CResizeDlg::PreTranslateMessage(pMsg);
}

void CDBGridDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DBFLIST, m_list);
}

BEGIN_MESSAGE_MAP(CDBGridDlg, CResizeDlg)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, OnOK)
	ON_BN_CLICKED(IDC_ALLOW_EDITS, OnAllowEdits)
	ON_BN_CLICKED(IDC_RELOAD, OnReload)
	ON_COMMAND(ID_CHKCOLUMNS, &CDBGridDlg::OnChkcolumns)
    ON_MESSAGE(WM_QUICKLIST_GETLISTITEMDATA, CDBGridDlg::OnGetListItem) 
    ON_MESSAGE(WM_QUICKLIST_GETTOOLTIP, CDBGridDlg::OnGetToolTip) 
    //ON_MESSAGE(WM_QUICKLIST_NAVIGATIONTEST, CDBGridDlg::OnNavigationTest)
    ON_MESSAGE(WM_QUICKLIST_CONTEXTMENU, CDBGridDlg::OnListContextMenu)
	ON_COMMAND(ID_SORTASCENDING, &CDBGridDlg::OnSortascending)
	ON_COMMAND(ID_SORTDESCENDING, &CDBGridDlg::OnSortdescending)

	//ON_MESSAGE(WM_QUICKLIST_NAVIGATETO, CDBGridDlg::OnNavigateTo)
	ON_NOTIFY(LVN_COLUMNCLICK,IDC_DBFLIST,OnHeaderClick)	// Column Click
	ON_NOTIFY(LVN_ENDLABELEDIT,IDC_DBFLIST,OnEndEditField)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_DBFLIST, OnItemChangedList)
	ON_NOTIFY(LVN_ODSTATECHANGED, IDC_DBFLIST, OnItemStateChanged)
	ON_NOTIFY(LVN_BEGINLABELEDIT,IDC_DBFLIST,OnBeginEditField)
	ON_MESSAGE(WM_QUICKLIST_CLICK, OnListClick)
	//ON_MESSAGE(WM_QUICKLIST_DBLCLICK, OnListDblClick)
#ifdef _USE_THEMES
	ON_MESSAGE(WM_THEMECHANGED, OnThemeChanged)	
#endif
	ON_COMMAND(ID_SHOW_CENTER, &CDBGridDlg::OnShowCenter)
	ON_COMMAND(ID_ZOOM_VIEW, &CDBGridDlg::OnZoomView)
	ON_COMMAND(ID_ADDTO_SELECTED, &CDBGridDlg::OnAddToSelected)
	ON_COMMAND(ID_REPLACE_SELECTED, &CDBGridDlg::OnReplaceSelected)
	ON_COMMAND(ID_LAUNCH_WEBMAP, OnLaunchWebMap)
	ON_COMMAND(ID_OPTIONS_WEBMAP, OnOptionsWebMap)
	ON_COMMAND(ID_LAUNCH_GE, &CDBGridDlg::OnLaunchGE)
	ON_COMMAND(ID_OPTIONS_GE, &CDBGridDlg::OnOptionsGE)
	ON_COMMAND(ID_SELECT_ALL, &CDBGridDlg::OnSelectAll)
	ON_COMMAND(ID_REMOVEFROMTABLE,&CDBGridDlg::OnRemoveFromTable)
	ON_COMMAND(ID_MOVETO_TOP, &CDBGridDlg::OnMovetoTop)
	ON_COMMAND(ID_EXPORT_SHAPES, &CDBGridDlg::OnExportShapes)
#ifdef _XLS
	ON_COMMAND(ID_EXPORT_XLS, &CDBGridDlg::OnExportXLS)
#endif
	ON_COMMAND(ID_INVERTSELECTION, &CDBGridDlg::OnInvertselection)
	ON_COMMAND(ID_EDIT_FIND,OnEditFind)
    ON_REGISTERED_MESSAGE(WM_FINDDLGMESSAGE,OnFindDlgMessage)
	ON_COMMAND(ID_LINKTEST, &CDBGridDlg::OnLinkTest)
	ON_COMMAND(ID_DELETE_RECORDS, &CDBGridDlg::OnDeleteRecords)
	ON_COMMAND(ID_UNDELETE_RECORDS, &CDBGridDlg::OnUndeleteRecords)
	ON_COMMAND_RANGE(ID_APPENDLAYER_0,ID_APPENDLAYER_N,OnCopySelected)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
	ON_COMMAND(ID_TABLE_FILL, OnTableFill)
	ON_COMMAND(ID_TABLE_REPL, OnTableRepl)
END_MESSAGE_MAP()


#ifdef _USE_THEMES
LRESULT CDBGridDlg::OnThemeChanged(WPARAM, LPARAM)
{
	if(wOSVersion>=0x0600) {
		SetListRedraw(0);
		m_list.SubclassHeader(); //subclasses or unsubclasses custom header
		m_list.InitHeaderStyle();  //Initializes and sets fonts as required, no tooltips here
		for(int nCol=m_nNumFlds;nCol>=0;nCol--) {
			VERIFY(m_list.DeleteColumn(nCol));
		}
		InitColumns();
		SetListRedraw(1);
		m_list.Invalidate();
		m_iColSort=-1;
		PostMessage(WM_COMMAND,m_bAscending?ID_SORTASCENDING:ID_SORTDESCENDING);
	}
	return 1;
}
#endif

void CDBGridDlg::CountDeletes()
{
	VERIFY(m_nNumRecs=m_vRecno.size());
	m_uDelCount=0;
	if(m_pShp->ShpType()!=CShpLayer::SHP_POINT) {
		SHP_POLY_DATA *ppoly=m_pdbfile->ppoly;
		if(!ppoly) {
			ASSERT(0);
			return;
		}
		for(VEC_DWORD::iterator it=m_vRecno.begin();it!=m_vRecno.end();it++) {
			if(ppoly[*it-1].IsDeleted()) m_uDelCount++;
		}
	}
	else {
		LPBYTE pv=&(*m_pShp->m_vdbe)[0];
		for(VEC_DWORD::iterator it=m_vRecno.begin();it!=m_vRecno.end();it++) {
			if((pv[*it-1]&SHP_EDITDEL)!=0) m_uDelCount++;
		}
	}
}

void CDBGridDlg::InitRecno()
{
	ASSERT(!m_vRecno.size());

	m_nNumRecs=m_pShp->m_nNumRecs;
	m_uDelCount=0;

	if(m_nNumRecs && m_pShp->IsAdditionPending())
		m_nNumRecs--;

	if(m_nNumRecs) {
		m_vRecno.reserve(m_nNumRecs);

		if(m_pShp->ShpType()!=CShpLayer::SHP_POINT) {
			SHP_POLY_DATA *ppoly=m_pdbfile->ppoly;
			ASSERT(ppoly);
			for(DWORD rec=1;rec<=m_nNumRecs;rec++,ppoly++) {
				bool bDel=ppoly->IsDeleted();
				if(m_nSubset==2 || (!bDel && m_nSubset!=1) || (bDel && m_nSubset==1)) {
					if(bDel) m_uDelCount++;
					m_vRecno.push_back(rec);
				}
			}
		}
		else {
			if(m_pShp->m_vdbe->size()) {
				LPBYTE pv=&(*m_pShp->m_vdbe)[0];
				for(DWORD rec=1;rec<=m_nNumRecs;rec++,pv++) {
					bool bDel=(*pv&SHP_EDITDEL)!=0;
					if(m_nSubset==2 || (!bDel && m_nSubset!=1) || (bDel && m_nSubset==1)) {
						if(bDel) m_uDelCount++;
						m_vRecno.push_back(rec);
					}
				}
			}
			else {
				ASSERT(0); //?
				//for(DWORD rec=1;rec<=m_nNumRecs;rec++) {
					//m_vRecno.push_back(rec);
				//}
			}
		}
		m_nNumRecs=m_vRecno.size();
	}
	m_bReloaded=true;
}

void CDBGridDlg::InitFldInfo()
{
	//Change this for WallsMap usage!

	//m_vFld.clear();
	m_vFld.resize(m_nNumFlds);

	IT_VEC_FLD it=m_vFld.begin();

	for(int nFld=1;nFld<=m_nNumFlds;nFld++,it++) {
		it->pnam=m_pdb->FldNamPtr(nFld);
		it->foff=m_pdb->FldOffset(nFld);
		it->flen=m_pdb->FldLen(nFld);
		it->ftyp=m_pdb->FldTyp(nFld);
		it->fdec=m_pdb->FldDec(nFld);
		it->col_order=nFld; //saved in CShpLayer for WallsMap

		//done in constructor. Will be saved in CShpLayer for WallsMap
		//it->col_width=LVSCW_AUTOSIZE_USEHEADER;
		//it->col_width_org=LVSCW_AUTOSIZE_USEHEADER;
	}
}

void CDBGridDlg::SaveShpColumns()
{
	SHP_COLUMNS &col=m_pdbfile->col;
	if(col.order.size()!=m_nNumCols) {
		col.order.resize(m_nNumCols);
		col.width.resize(m_nNumCols);
	}
	VERIFY(m_list.GetColumnOrderArray(&col.order[0],m_nNumCols));
	for(int nCol=0;nCol<m_nNumCols;nCol++) {
		col.width[nCol]=m_list.GetColumnWidth(nCol);
	}
}

void CDBGridDlg::InitDialogSize()
{
	//Called once from InitGrid() --

	SHP_COLUMNS &col=m_pdbfile->col;
	for(int nCol=1;nCol<m_nNumCols;nCol++) {
		//Examines only rows in view!
		if(col.width[nCol]) {
			m_list.ResizeColumn(nCol);
			col.width[nCol]=m_list.GetColumnWidth(nCol);
		}
	}
	
	//Set dialog size to fit an integral number of rows and columns --

	int wScrollV=0,wScrollH=0;

	//Determine number of rows initially displayed based on initial dialog height --
	UINT count=m_list.GetCountPerPage();
	if(count>m_nNumRecs) 
		count=max(m_nNumRecs,2);
	else if(count<m_nNumRecs)
		wScrollV=GetSystemMetrics(SM_CXVSCROLL); //Make dialog this much wider

	CRect rect;
	m_list.GetItemRect(0,&rect,LVIR_BOUNDS);
	int rw=rect.Width(); //max width to extend list's client window (assuming no vert scroll bar)
	int rh=count*rect.Height()+m_list.GetHeaderHeight(); //desired client height (assuming no horz scroll bar)

	m_list.GetClientRect(&rect); //does not consider horz and vert scroll bars?

	if(rect.right<rw) {
		//recalculate width to hold a whole number of columns if feasible --
		int i=0;
		for(int w=0;i<m_nNumCols;i++) {
			w+=col.width[i];
			if(w>=rect.right) {
				rw=3*GetSystemMetrics(SM_CXFULLSCREEN)/4; //don't make too wide!
				if(rw>w) rw=w;
				i++;
				break;
			}
		}
		if(i<m_nNumCols)
			wScrollH=GetSystemMetrics(SM_CYHSCROLL); //Make dialog this much taller
	}

	//(rw,rh) desired size of client
	rw+=wScrollV-rect.right; //desired increase
	rh+=wScrollH-rect.bottom;
	GetWindowRect(&rect);
	rect.bottom+=rh;
	rect.right+=rw;
	Reposition(rect);
}

void CDBGridDlg::InitColumns()
{
	SHP_COLUMNS &col=m_pdbfile->col;
	if(!col.order.size()) {
		col.order.resize(m_nNumCols);
		col.width.resize(m_nNumCols);
		col.width[0]=20+m_wDigitWidth*((UINT)log10((double)m_nNumRecs));
		col.order[0]=0;
		for(int nCol=1;nCol<m_nNumCols;nCol++) {
			col.order[nCol]=nCol;
			col.width[nCol]=LVSCW_AUTOSIZE;
		}
	}

	m_list.InsertColumn(0,"",LVCFMT_LEFT,col.width[0]);

	IT_VEC_FLD it=m_vFld.begin();

	for(int nFld=1;nFld<=m_nNumFlds;nFld++,it++) {
		int fmt=LVCFMT_LEFT;
		if(it->ftyp=='N' || it->ftyp=='F') fmt=LVCFMT_RIGHT;
		else if(it->ftyp=='M') fmt=LVCFMT_CENTER;
		m_list.InsertColumn(nFld,it->pnam,fmt,col.width[nFld]);
	}
	m_list.EnableHeaderEx();

	m_list.SetColumnOrderArray(m_nNumCols,&col.order[0]);

	int w=strlen((--it)->pnam);
	if(w<it->flen) w=it->flen;
	m_list.m_iLastColWidth=(w+2)*m_wDigitWidth; //insure containment
}

void CDBGridDlg::InitialSort()
{
	if(m_nNumRecs>0) {
		if(!m_bReloaded) {
			//must be a selected branch --
			ASSERT(app_pShowDlg && app_pShowDlg->m_pSelLayer==m_pShp);
			m_iColSort=m_pShp->m_nLblFld;
			m_iColSorted=-1;
			SortColumn(m_bAscending=true); //should be a multisort!
		}
		else {
			m_iColSorted=0;
			m_list.SetSortArrow(0,m_bAscending=true);
		}
	}
	else {
		m_list.SetSortArrow(m_iColSorted=-1,false);
	}
}

void CDBGridDlg::ReInitGrid(VEC_DWORD *pvRec)
{
	//replacing table with new list from same shapefile.
	//if pvRec==NULL we are inserting entire shapefile
	//otherwise only a selected subset, in which case it's a point
	//shapefile with a label field.
	ASSERT(m_pShp && m_pdb==m_pShp->m_pdb && m_pShp->m_pdbfile==m_pdbfile && m_pdbfile->pDBGridDlg==this);

	SetListRedraw(0);

	if(pvRec) {
		ASSERT(m_pShp->ShpType()==CShpLayer::SHP_POINT);
		m_vRecno.swap(*pvRec);
		VERIFY(m_nNumRecs=m_vRecno.size());
		CountDeletes();
		m_bReloaded=false;
	}
	else {
		VEC_DWORD().swap(m_vRecno); //should free memory prior to next statement
		InitRecno(); //orders by rec no. and sets m_bReloaded=true;
	}

	VERIFY(m_list.DeleteAllItems()); //necessary?
	m_list.SetItemCount(0);

	InitialSort();

	m_list.SetItemCount(m_nNumRecs);

	if(m_bRestoreLayout) {
		int nCols = m_list.GetHeaderCtrl()->GetItemCount();
		int* pOrderArray = new int[nCols];
		*pOrderArray=0;
		for(int i=1;i<nCols;i++) {
			pOrderArray[i]=i;
			if(m_list.GetColumnWidth(i)<=0) {
				//turn column on --
				m_list.ResizeColumn(i); //calls XC_AdjustWidth()
			}
		}
		VERIFY(m_list.SetColumnOrderArray(nCols,pOrderArray));
		delete [] pOrderArray;
	}

	SetListRedraw(1);

	m_list.Invalidate(0);
	m_list.UpdateWindow();

	if(m_nNumRecs>0) {
		m_list.SetSelected(0);
		m_list.SetNavigationColumn(0);
		m_list.SetFocus();
	}
	UpdateSelCount(m_nNumRecs?1:0);
	BringWindowToTop();
}

// CDBGridDlg message handlers
BOOL CDBGridDlg::OnInitDialog()
{
	m_list.m_bHdrToolTip=m_pShp->HasXDFlds();

	CQuickList::UseHeaderEx();

	CResizeDlg::OnInitDialog();

#ifdef _USE_THEMES
	ASSERT(wOSVersion>=0x0600 || m_list.IsUsingHeaderEx());
#else
	ASSERT(m_list.IsUsingHeaderEx());
#endif

	//------------------------------
	//ResizeDlg -- adjustment

	m_xMin=500; m_yMin=174; //minimal dialog size -- fix if contols are added/rearranged

	AddControl(IDC_DBFLIST, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE, 1);
	//AddControl(IDC_ST_SELECTED, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_ST_SELCOUNT, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_RELOAD, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDOK, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_ALLOW_EDITS, CST_REPOS, CST_REPOS, CST_REPOS, CST_REPOS, 0);
	//------------------------------

	ASSERT(!m_list.GetHeaderCtrl()->GetItemCount());
	DWORD dwStyle=m_list.GetExtendedStyle();
	dwStyle &=~LVS_SHOWSELALWAYS;
	dwStyle |= (LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP |	LVS_EX_GRIDLINES);

#ifdef _USE_THEMES
	//dwStyle|=LVS_EX_TRACKSELECT; //(LVS_EX_ONECLICKACTIVATE|LVS_EX_UNDERLINEHOT);
#endif
	//m_list.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle); 
	m_list.SetExtendedStyle(dwStyle); //same effect

	{
		CFont *pfOld;
		CDC dc;
		dc.CreateCompatibleDC(NULL);
		VERIFY(pfOld=dc.SelectObject(m_list.GetFont()));
		m_wDigitWidth=(WORD)dc.GetTextExtent("8",1).cx;
		dc.SelectObject(pfOld);
	}

	//EnableAllowEdits();

	m_pShp->InitDlgTitle(this);

	m_list.InitHeaderStyle(); //Initializes and sets fonts as required
	EnableAllowEdits(); //calls the following 3 fcns --
	//m_pdb=m_pShp->m_pdb;
	//m_nNumCols=(m_nNumFlds=m_pdb->NumFlds())+1;
	//InitFldInfo();
	InitColumns(); //After column insertion, calls m_list.EnableHeaderEx()

	if(m_pShp->HasXCFlds()) {
		m_pvxc=new VEC_PXCOMBO(m_nNumFlds);
		PVEC_XCOMBO pvxc=m_pdbfile->pvxc;
		for(VEC_XCOMBO::iterator it=pvxc->begin();it!=pvxc->end();it++) {
			ASSERT(!(*m_pvxc)[it->nFld-1]);
			(*m_pvxc)[it->nFld-1]=&*it;
			if(it->wFlgs&XC_HASLIST)
				m_list.SetAdjustWidthCallback(XC_AdjustWidth);
		}
	}

	if(m_pShp->HasMemos())
		m_list.SetAdjustWidthCallback(XC_AdjustWidth);

	InitialSort();

	m_list.SetItemCount(m_nNumRecs);

	InitDialogSize();

	if(m_nNumRecs>0) {
		m_list.SetSelected(0);
		m_list.SetNavigationColumn(0);
		m_list.SetFocus();
	}
	UpdateSelCount(m_nNumRecs?1:0);

	VERIFY(m_dropTarget.Register(this));

	if(!m_bExpandMemos && m_pdb->HasMemos())
		m_list.EnableToolTips(TRUE);

	return (m_nNumRecs==0);
}

LRESULT CDBGridDlg::OnGetToolTip(WPARAM wParam, LPARAM lParam)
{
    ASSERT( (HWND)wParam == m_list.GetSafeHwnd() );
	QUICKLIST_TTINFO *tt=(QUICKLIST_TTINFO *)lParam;
	int nFld=tt->nCol;
	if(nFld<1) return FALSE;

	if(tt->nRow<0) {
		ASSERT(m_pShp->HasXDFlds());
		LPCSTR pDesc=m_pShp->XC_GetXDesc(nFld);
		if(!pDesc || !*pDesc) return FALSE;
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

	ASSERT(m_bExpandMemos);
	return FALSE;
/*
	if(m_bExpandMemos || m_vFld[nFld-1].ftyp!='M') return FALSE;

	//memo field tip --

	int dbtrec=CDBTFile::RecNo((LPSTR)m_pdb->FldPtr(GetRecNo(tt->nRow),nFld));
	if(!dbtrec) return FALSE;

	return m_pdbfile->dbt.GetToolTip(tt->pvText80,80,tt->bW,dbtrec)>0;
*/
}

LRESULT CDBGridDlg::OnGetListItem(WPARAM wParam, LPARAM lParam)
{
    //wParam is a handler to the list

    //Make sure message comes from list box

    ASSERT( (HWND)wParam == m_list.GetSafeHwnd() );

    //lParam is a pointer to the data that 

    //is needed for the element

	if(!m_pdb) {
		ASSERT(0);
		return 0;
	}

    CQuickList::CListItemData* data = 
        (CQuickList::CListItemData*) lParam;

    //Get which record and field that is asked for.
    int nRec = (GetRecNo(data->GetItem())&0x7FFFFFFF);
    int nFld = data->GetSubItem(); //record number if zero
	ASSERT(nFld>=0);

	bool bHotItem=(nFld==m_list.GetNavigationColumn());

	data->m_colors.m_navigatedBackColor=RGB(165,202,242); //RGB(164,195,235); //::GetSysColor(COLOR_HOTLIGHT);
	data->m_colors.m_selectedBackColor=RGB(215,228,242);

	if(IsPtSelected(nRec))
		data->m_colors.m_backColor=RGB(255,255,204); //::GetSysColor(COLOR_WINDOW); //default

	data->m_colors.m_selectedBackColorNoFocus=
		(m_list.m_edit || m_pFindDlg)?(bHotItem?RGB(165,202,242):RGB(215,228,242)):(bHotItem?RGB(192,192,192):RGB(210,210,210));
	
	//data->m_colors.m_textColor=RGB(0,0,0); //default
	//data->m_colors.m_hotTextColor=RGB(255,0,0);
	data->m_colors.m_selectedTextColor=
		data->m_colors.m_navigatedTextColor=RGB(0,0,0);

	if(nFld==0) {
		//data->m_textStyle.m_hFont=m_fixedFont;
		data->m_text.Format("%u%s",nRec,m_pShp->IsRecDeleted(nRec)?"*":"");
		data->m_colors.m_backColor=::GetSysColor(COLOR_BTNFACE); //COLOR_BTNSHADOW //COLOR_BTNFACE
		data->m_allowEdit=false;
	}
	else {
		LPCSTR pFld=(LPCSTR)m_pdb->RecPtr(nRec);
		bool bRecOpen=(app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp,nRec)) || m_pShp->IsRecDeleted(nRec);
		bool bAllowEdits=m_bAllowEdits && !bRecOpen && (bIgnoreEditTS || !IsFldReadonly(nFld));

		FldInfo &fld=m_vFld[nFld-1];
		pFld+=fld.foff;
		if((data->m_cTyp=fld.ftyp)=='M') {
			data->m_button.m_center=!m_bExpandMemos;
			if(*pFld!=' ' || (bHotItem && bAllowEdits && data->IsSelected())) {
				//data->m_text=""; default
				//data->m_button.m_pText=NULL; default
				//data->m_textStyle.m_uFldWidth=0; default

				if(m_bExpandMemos && *pFld!=' ') {
					UINT dbtrec=CDBTFile::RecNo(pFld);
					if(dbtrec) {
						UINT bufSiz=1023;
						LPCSTR p=m_pdbfile->dbt.GetText(&bufSiz,dbtrec);
						ASSERT(bufSiz);
						if(bufSiz) {
							p=dbt_GetMemoHdr(p,80);
							if(*p!='~') p+=2;
							data->m_button.m_pText=p;
						}
					}
				}
				data->m_button.m_draw=true;
				data->m_button.m_width=2;
				data->m_button.m_style=DFCS_BUTTONPUSH;
				data->m_button.m_noSelection=false; //dflt==true
			}
			data->m_allowEdit=*pFld!=' ' || bAllowEdits; //m_pShp->IsEditable();
		}
		else if(data->m_cTyp=='L') {
				data->m_button.m_draw=true;
				data->m_textStyle.m_uFldWidth=0;
				data->m_allowEdit=bAllowEdits;
				data->m_button.m_noSelection=false;

				ASSERT(data->m_button.m_style==DFCS_BUTTONCHECK);
				data->m_button.m_style=(*pFld=='Y' || *pFld=='T')?
					(DFCS_BUTTONCHECK|DFCS_CHECKED):DFCS_BUTTONCHECK;
				//data->m_button.m_center=true; //dflt
				//data->m_button.m_width=0; //dflt
		}
		else {
			data->m_text.SetString(pFld,fld.flen);
			data->m_text.Trim();

			int nTyp=m_pShp->GetLocFldTyp(nFld);
			if(nTyp && bIgnoreEditTS && (nTyp==LF_CREATED+1 || nTyp==LF_UPDATED+1)) {
				nTyp=0;
			}
	
			if(nTyp || bRecOpen || !strchr("CDNLF",fld.ftyp) || !bIgnoreEditTS && IsFldReadonly(nFld))
			{
				data->m_allowEdit=false;
				data->m_colors.m_selectedTextColor=data->m_colors.m_navigatedTextColor=
					data->m_colors.m_textColor=::GetSysColor(COLOR_GRAYTEXT);
			}
			else {
				if(bAllowEdits && m_pvxc && bHotItem && data->IsSelected()) {
					PXCOMBODATA pxc=(*m_pvxc)[nFld-1];
					if(pxc && (pxc->wFlgs&XC_HASLIST)) {
						data->m_pXCMB=pxc;
					}
				}
				data->m_allowEdit=bAllowEdits;
				data->m_cTyp=fld.ftyp;
				data->m_editBorder=true;
				data->m_textStyle.m_uFldWidth=fld.flen;
			}
		}
	}
    return 0;
}

int CDBGridDlg::ColAdvance(int iCol)
{
	if(iCol>0) {
		while(++iCol<m_nNumCols) {
			if(m_list.GetColumnWidth(iCol)>0) return iCol;
		}
	}
	else {
		ASSERT(iCol<0);
		iCol=-iCol;
		while(--iCol>0) {
			if(m_list.GetColumnWidth(iCol)>0) return iCol;
		}
	}
	return 0;
}

void CDBGridDlg::OnEndEditField(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	int nRow=pDispInfo->item.iItem;
	int nCol=pDispInfo->item.iSubItem;
	ASSERT(nCol>=1);

    *pResult = 0; //unnec

    // If pszText is NULL, editing was canceled or there was no change --
    if(pDispInfo->item.pszText != NULL)
    {
		ASSERT(m_bAllowEdits);
		DWORD rec=GetRecNo(nRow);
		VERIFY(!m_pdb->Go(rec));
		m_pdb->StoreTrimmedFldData(m_pdb->RecPtr(),pDispInfo->item.pszText,nCol);
		if(m_pShp->m_pdbfile->HasTimestamp(1)) {
			ASSERT(SHP_DBFILE::bEditorPrompted);
			m_pShp->UpdateTimestamp(m_pdb->RecPtr(),FALSE);
		}
		if(m_pdb->FlushRec()) {
			AfxMessageBox("Error updating DBF. Can't write to disk!");
			return;
		}
		if(m_pShp->ShpTypeOrg()==CShpLayer::SHP_POINT) {
			m_pShp->ChkEditDBF(rec);
			//Above may NOT have set m_pSelLayer->m_bEditFlags!

			if(nCol==m_pShp->m_nLblFld && m_pShp->IsRecSelected(rec)) {
				ASSERT(app_pShowDlg);
				if(app_pShowDlg) app_pShowDlg->InvalidateTree();
			}
		}

		if(nCol==m_pShp->m_nLblFld) {
			if(app_pDbtEditDlg && app_pDbtEditDlg->IsRecOpen(m_pShp,rec)) {
				app_pDbtEditDlg->RefreshTitle();
			}
			if(app_pImageDlg && app_pImageDlg->IsRecOpen(m_pShp,rec)) {
				app_pImageDlg->RefreshTitle();
			}
			if(m_pShp->IsVisible() && m_pShp->Visibility(rec)&2) {
				m_pShp->m_pDoc->RefreshViews();
			}
		}
    }

	//If edit OK, navigate depending on exit key --

	UINT key=m_list.GetLastEndEditKey();

	//key==-1 if lost focus

	if(key==VK_RETURN || key==VK_DOWN) {
		if(nRow<m_list.GetItemCount()-1) {
			m_list.SetItemState(nRow,0,LVIS_SELECTED|LVIS_FOCUSED);
			++nRow;
			m_list.SetSelected(nRow);
			//m_list.EditSubItem(nRow,nCol); //for now, don't do this
		}
		else if(nCol=ColAdvance(nCol)) {
			m_list.SetNavigationColumn(nCol);
			//m_list.EditSubItem(nRow,nCol);
		}
	}
	else if(key==VK_UP) {
		if(nRow>0) {
			m_list.SetItemState(nRow,0,LVIS_SELECTED|LVIS_FOCUSED);
			--nRow;
			m_list.SetSelected(nRow);
			//m_list.EditSubItem(nRow,nCol);
		}
		else if(nCol=ColAdvance(-nCol)) {
			m_list.SetNavigationColumn(nCol);
			//m_list.EditSubItem(nRow,nCol);
		}
	}
}

void CDBGridDlg::OnBeginEditField(NMHDR* pNMHDR, LRESULT* pResult) 
{
    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	DWORD nRec=GetRecNo(pDispInfo->item.iItem);
	ASSERT(nRec>0 && nRec<=m_pShp->m_nNumRecs);
	int nFld=pDispInfo->item.iSubItem;
	ASSERT(nFld>0 && nFld<=m_nNumFlds);
	if(m_vFld[nFld-1].ftyp=='M') {
		//PostMessage?
		OpenMemo(nRec,nFld);
		*pResult=TRUE;
		return;
	}

	*pResult=FALSE; //allow normal editing (if not checkbox!)
}

LRESULT CDBGridDlg::OnListClick(WPARAM wParam, LPARAM lParam)
{
	//User clicked on list. Toggle checkbox/image if hit
	//Make sure message comes from list box
	ASSERT( (HWND)wParam == m_list.GetSafeHwnd() );
	if(!m_bAllowEdits) return 0;

	CQuickList::CListHitInfo *hit= (CQuickList::CListHitInfo*) lParam;

	//TRACE(_T("User hit item %d, subitem %d "), hit->m_item, hit->m_subitem);

	if(hit && hit->m_onButton)
	{
		int nFld=hit->m_subitem;
		ASSERT(nFld>0);
		FldInfo &fld=m_vFld[nFld-1];

		if(fld.ftyp=='L') {
			DWORD nRec=GetRecNo(hit->m_item);

			if(app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp,nRec))
				return 0;

			VERIFY(!m_pdb->Go(nRec));

			if(m_pShp->IsRecDeleted(nRec)) return 0; //for now

			LPSTR pFld=(LPSTR)m_pdb->FldPtr(nFld);
			*pFld=(*pFld=='Y' || *pFld=='T')?'N':'Y';
			m_list.RedrawCheckBoxs(hit->m_item, hit->m_item, hit->m_subitem);

			if(m_pShp->m_pdbfile->HasTimestamp(1)) {
				ASSERT(SHP_DBFILE::bEditorPrompted);
				m_pShp->UpdateTimestamp(m_pdb->RecPtr(),FALSE);
			}
			if(m_pdb->FlushRec()) {
				AfxMessageBox("Error updating DBF. Can't write to disk!");
				return 0;
			}
			if(m_pShp->ShpTypeOrg()==CShpLayer::SHP_POINT) {
				m_pShp->ChkEditDBF(nRec);
				//Above may NOT have set m_pSelLayer->m_bEditFlags!
			}
		}
	}
	return 0;
}

/*
LRESULT CDBGridDlg::OnListDblClick(WPARAM wParam, LPARAM lParam)
{
	//User double-clicked on list. Open memo editor if column is memo typ --
	//Make sure message comes from list box
	ASSERT( (HWND)wParam == m_list.GetSafeHwnd() );

	CQuickList::CListHitInfo *hit= (CQuickList::CListHitInfo*) lParam;

	if(hit && hit->m_subitem>0 && m_vFld[hit->m_subitem-1].ftyp=='M')
	{
		OpenMemo(GetRecNo(hit->m_item),hit->m_subitem);
	}
	return 0;
}
*/

static CShpDBF *sort_pdb;
static int sort_fldoff;
static char sort_fldtyp;
static bool sort_ascending;
static WORD sort_fldlen;
static LPBYTE sort_pfldbuf;

static int __inline IsNumericPfx(LPCSTR p)
{
	int n=0;
	bool bDec=false;
	while(n<sort_fldlen && *p==' ') {n++;p++;}
	if(n<sort_fldlen && *p=='-') {n++;p++;}
	if(n<sort_fldlen && *p=='.') {n++;p++;bDec=true;}
	if(n==sort_fldlen || *p<'0' || *p>'9') return 0;

	for(;n<sort_fldlen && *p>='.' && *p<='9';n++,p++) {
		if(*p=='.') {
			if(bDec) break;
			bDec=true;
		}
		else if(*p=='/') break;
	}
	//return (n==sort_fldlen || *p==' ' || )?n:0; // require space separator
	return (n<32)?n:0;
}

static bool sort_compare(DWORD w0,DWORD w1)
{
	w0 &= 0x7FFFFFFF; w1 &= 0x7FFFFFFF;

	LPCSTR p0=(LPCSTR)sort_pdb->RecPtr(w0)+sort_fldoff;
	if(!sort_pdb->IsMapped()) p0=(LPCSTR)memcpy(sort_pfldbuf,p0,sort_fldlen);

	LPCSTR p1=(LPCSTR)sort_pdb->RecPtr(w1)+sort_fldoff;

	char fbuf[32];
	double f0,f1;

	if(sort_fldtyp=='C') {
		int n0,n1;
		if((n0=IsNumericPfx(p0))!=0 && (n1=IsNumericPfx(p1))!=0) {
			memcpy(fbuf,p0,n0); fbuf[n0]=0;
			f0=atof(fbuf);
			memcpy(fbuf,p1,n1); fbuf[n1]=0;
			f1=atof(fbuf);
			if(f0!=f1) return sort_ascending==(f0<f1);
			int i=sort_fldlen-max(n0,n1); //length of smallest character suffix
			if(i && (i=_memicmp(p0+n0, p1+n1, i)))
				return sort_ascending==(i<0);
			return sort_ascending?(n0>n1):(n1<n0);
		}
		n0=_memicmp(p0,p1,sort_fldlen);
		return sort_ascending?(n0<0):(n0>0);
	}

	if(sort_fldtyp=='L') {
		bool b0=*p0=='Y'||*p0=='T';
		bool b1=*p1=='Y'||*p1=='T';
		return sort_ascending?(b1 && !b0):(b0 && !b1);
	}

	if(sort_fldtyp=='M') {
		return sort_ascending?((*p0!=' ')&&(*p1==' ')):((*p1!=' ')&&(*p0==' '));
	}

	//dBase N flds have max length of 19, F flds 20!
	ASSERT(sort_fldlen<32);
	fbuf[sort_fldlen]=0;

	memcpy(fbuf, p0, sort_fldlen); f0=atof(fbuf);
	memcpy(fbuf, p1, sort_fldlen); f1=atof(fbuf);

	return sort_ascending?(f0<f1):(f1<f0);
}

static bool sort_compare_recno(DWORD w0,DWORD w1)
{
	return (w0 & 0x7FFFFFFF) < (w1 & 0x7FFFFFFF);
}

static bool sort_compare_seltop(DWORD w0,DWORD w1)
{
	return (w0 & 0x80000000) && !(w1 & 0x80000000);
}

static LPDWORD pwFirst=NULL,pwLast=NULL;
UINT CDBGridDlg::FlagSelectedItems(bool bUnselect /*=true*/)
{
	ASSERT(m_bRedrawOff==true);
	UINT cnt=0;
	pwFirst=pwLast=NULL;
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)	{
	  int nItem = m_list.GetNextSelectedItem(pos);
	  if(bUnselect) m_list.SetItemState(nItem,0,LVIS_SELECTED);
	  if(!m_iColSorted && !m_bAscending) {
		  nItem=m_nNumRecs-nItem-1;
	  }
	  pwLast=&m_vRecno[nItem];
	  *pwLast |= 0x80000000;
	  if(!pwFirst) pwFirst=pwLast;
	  cnt++;
	}
	if(!m_iColSorted && !m_bAscending)
		std::swap(pwLast,pwFirst);
	return cnt;
}

/*
#ifdef _DEBUG
static bool IsFlagged(const VEC_DWORD &vRecno)
{
	for(VEC_DWORD::const_iterator it=vRecno.begin();it!=vRecno.end();it++)
		if((*it)&0x80000000) return true;
	return false;
}
#endif
*/

void CDBGridDlg::UnflagSelectedItems(UINT cnt)
{
	ASSERT(m_bRedrawOff==true);
	ASSERT(cnt && m_vRecno.size()==m_nNumRecs);
	for(UINT i=0;i<m_nNumRecs;i++) {
		if(m_vRecno[i]&0x80000000) {
		  m_list.SetItemState((!m_iColSorted && !m_bAscending)?(m_nNumRecs-i-1):i,LVIS_SELECTED,LVIS_SELECTED);
		  m_vRecno[i]&=0x7FFFFFFF;
		  if(!--cnt) break;
		}
	}
}

UINT CDBGridDlg::GetSelectedRecs(VEC_DWORD &vRec)
{
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)	{
	  int nItem = m_list.GetNextSelectedItem(pos);
	  if(!m_iColSorted && !m_bAscending) {
		  nItem=m_nNumRecs-nItem-1;
	  }
	  vRec.push_back(m_vRecno[nItem]);
	}
	return vRec.size();
}

UINT CDBGridDlg::GetSelectedShpRecs(VEC_SHPREC &vRec,bool bMerging)
{
	UINT nAdded=0;
	SHPREC shprec(m_pShp,0);
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)	{
	  int nItem = m_list.GetNextSelectedItem(pos);
	  if(!m_iColSorted && !m_bAscending) {
		  nItem=m_nNumRecs-nItem-1;
	  }
	  if(!bMerging || !m_pShp->IsRecSelected(m_vRecno[nItem])) {
		  shprec.rec=m_vRecno[nItem];
		  vRec.push_back(shprec);
		  nAdded++;
	  }
	}
	return nAdded;
}

void CDBGridDlg::SortColumn(bool bAscending)
{
	//m_list.ClearLastKeyedItem();

	if(m_iColSorted==m_iColSort && m_bAscending==bAscending) return;

	SetListRedraw(0);

	if(m_iColSort<0) {
		if(m_iColSorted>=0)
			m_list.SetSortArrow(m_iColSorted,m_bAscending);
		SetListRedraw(1);
		return;
	}

	BeginWaitCursor();

	UINT numSel=FlagSelectedItems();

#ifdef _USE_GRTIMER
	CLR_NTI_TM(3);
	START_NTI_TM(3)
#endif

    if(m_iColSort==0) {
		//sort by record record number --
		if(m_iColSorted!=0) {
			std::sort(m_vRecno.begin(),m_vRecno.end(),sort_compare_recno);
		}
	}
	else {
		//perform a merge sort --
		sort_ascending=bAscending;
		//make this global --
		FldInfo &fld=m_vFld[m_iColSort-1];
		sort_pdb=m_pdb;
		sort_fldoff=fld.foff;
		sort_fldlen=fld.flen;
		sort_fldtyp=fld.ftyp;

		ASSERT(sort_fldoff && sort_fldlen<256);
		if(sort_fldtyp=='N' || sort_fldtyp=='F') {
			if(sort_fldlen>=SIZ_NFLD_BUF){
				ASSERT(FALSE);
				sort_fldlen=SIZ_NFLD_BUF-1;
			}
			if(fld.fdec!=0 || fld.flen>=9 || sort_fldtyp=='F') sort_fldtyp=0; //use atof()
		}
		else if(sort_fldtyp!='M') sort_fldtyp='C';

		if(!sort_pdb->IsMapped()) {
		   if(m_sort_lenfldbuf<sort_fldlen)
			   VERIFY(m_sort_pfldbuf=(LPBYTE)realloc(m_sort_pfldbuf, m_sort_lenfldbuf=sort_fldlen));
		   sort_pfldbuf=m_sort_pfldbuf;
		}

		std::stable_sort(m_vRecno.begin(),m_vRecno.end(),sort_compare);
	}

	m_iColSorted=m_iColSort;
	m_bAscending=bAscending;

#ifdef _USE_GRTIMER
	STOP_NTI_TM(3);
#endif

	//if(m_iColSorted>0) m_list.SetColumnWidth(m_iColSorted,LVSCW_AUTOSIZE_USEHEADER);

	m_list.SetSortArrow(m_iColSorted,m_bAscending);

	if(numSel) UnflagSelectedItems(numSel);

	EndWaitCursor();

	//m_bActive=true;
	m_list.EnsureVisible(0,FALSE);
	SetListRedraw(1);
	m_list.Invalidate(0); //necessary!

#ifdef _USE_GRTIMER
	CMsgBox("Elapsed Time: %.2f secs",NTI_SECS(3));
#endif
}

void CDBGridDlg::OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLISTVIEW* pLV = reinterpret_cast<NMLISTVIEW*>(pNMHDR);

	if( m_list.GetFocus() != &m_list )
		m_list.SetFocus();	// Force focus to finish editing (necessary?)

	if(!pLV) {
		m_iColSort=m_iColSorted;
		SortColumn(m_bAscending);
		return;
	}

	m_iColSort = pLV->iSubItem;
	if (IsSortable(m_iColSort))
		SortColumn((m_iColSorted==m_iColSort)?(!m_bAscending):true);
}

void CDBGridDlg::OnSortascending()
{
	SortColumn(true);
}

void CDBGridDlg::OnSortdescending()
{
	SortColumn(false);
}

void CDBGridDlg::XC_AdjustWidth(CWnd *pWnd,int nFld)
{
	CDBGridDlg *pDlg=(CDBGridDlg *)pWnd;
	if(pDlg->m_bExpandMemos && pDlg->m_pdb->FldTyp(nFld)=='M') {
		pDlg->m_list.SetColumnWidth(nFld,MEMO_EXPAND_WIDTH);
	}
	else if(pDlg->m_pvxc)
	{
		PXCOMBODATA pxc=(*pDlg->m_pvxc)[nFld-1];
		if(pxc && (pxc->wFlgs&XC_HASLIST)) {
			int w=pDlg->m_list.GetColumnWidth(nFld);
			CRect rect;
			if(pDlg->m_list.GetItemRect(0,&rect,LVIR_BOUNDS))
				pDlg->m_list.SetColumnWidth(nFld,w+rect.Height());
		}
	}
}

void CDBGridDlg::OnChkcolumns()
{
	CHeaderCtrl *pHdr=m_list.GetHeaderCtrl();
	int nCols = pHdr->GetItemCount();
	int* pOrderArray = new int[nCols];

	VERIFY(m_list.GetColumnOrderArray(pOrderArray, nCols));

	CDBField *pFld = new CDBField[nCols-1];

	ASSERT(sizeof(CDBField)==8);

	CDBField *pF=pFld;
	for(int i=1;i<nCols;i++,pF++) {
		int f=pOrderArray[i];
		ASSERT(f>=1);
		pF->pInfo=&m_vFld[f-1];
		pF->wFld=(WORD)f;
		pF->bVis=m_list.GetColumnWidth(f)>0;
	}

	CColumnDlg dlg(pFld,nCols-1);

	if(IDOK==dlg.DoModal()) {
		SetListRedraw(0);
		pF=pFld;
		for(int i=1;i<nCols;i++,pF++) {
			int f=pF->wFld;
			pOrderArray[i]=f;
			bool bVisPrev=m_list.GetColumnWidth(f)>0;
			if(bVisPrev!=pF->bVis) {
				if(pF->bVis) {
					//turn column on --
					m_list.ResizeColumn(f);
				}
				else {
					VERIFY(m_list.SetColumnWidth(f,0)); //blocked by CGridListCtrlEx when !m_Visible
				}
			}
		}
		VERIFY(m_list.SetColumnOrderArray(nCols,pOrderArray));
		SetListRedraw(1);
		Invalidate(0);
	}
	delete [] pFld;
	delete [] pOrderArray;
}

LRESULT CDBGridDlg::OnListContextMenu(WPARAM wParam, LPARAM lParam)
{

	CWnd *pWnd=(CWnd *)wParam;
	CPoint point=*(CPoint *)lParam;

	if(pWnd==m_list.GetHeaderCtrl()) {
		HDHITTESTINFO hInfo;
		hInfo.pt=point;
		pWnd->ScreenToClient(&hInfo.pt);
		CMenu menu;
		if(menu.LoadMenu(IDR_SORTMENU)) {
			CMenu* pPopup = menu.GetSubMenu(0);
			ASSERT(pPopup != NULL);
			m_wLinkTestFld=m_wTableFillFld=0;
			m_iColSort=((CHeaderCtrl *)pWnd)->HitTest(&hInfo);
			if(m_iColSort>=0) {
				if(m_iColSort==m_iColSorted) {
					pPopup->CheckMenuItem(ID_SORTASCENDING,m_bAscending?(MF_CHECKED|MF_BYCOMMAND):(MF_UNCHECKED|MF_BYCOMMAND));
					pPopup->CheckMenuItem(ID_SORTDESCENDING,(!m_bAscending)?(MF_CHECKED|MF_BYCOMMAND):(MF_UNCHECKED|MF_BYCOMMAND));
				}
				if(m_iColSort) {
					if(m_pdb->FldTyp(m_iColSort)=='M') {
						m_wTableFillFld=m_wLinkTestFld=(WORD)m_iColSort;
					}
					else if(m_pdb->FldTyp(m_iColSort)=='C' || m_pdb->FldTyp(m_iColSort)=='L') m_wTableFillFld=(WORD)m_iColSort;
				}
				else
					pPopup->ModifyMenu(ID_EDIT_FIND,MF_BYCOMMAND,ID_EDIT_FIND,"Find deleted...");
			}
			if(!m_wLinkTestFld) {
				pPopup->DeleteMenu(6, MF_BYPOSITION);
				pPopup->DeleteMenu(ID_LINKTEST, MF_BYCOMMAND);
			}

			if(!m_wTableFillFld) {
				pPopup->DeleteMenu(ID_TABLE_REPL, MF_BYCOMMAND);
				pPopup->DeleteMenu(ID_TABLE_FILL, MF_BYCOMMAND);
			}
			else {
				if((!m_bAllowEdits || (!bIgnoreEditTS && IsFldReadonly(m_wTableFillFld)))) {
					pPopup->EnableMenuItem(ID_TABLE_REPL, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
					pPopup->EnableMenuItem(ID_TABLE_FILL, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}
				//only support repl for C and M fields for now --
				if(m_pdb->FldTyp(m_iColSort)=='L') pPopup->DeleteMenu(ID_TABLE_REPL, MF_BYCOMMAND);
			}

			VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this));
		}
	}
	else
	{
		CPoint pt(point);
		m_list.ScreenToClient(&pt);
		int nRow, nCol;
		m_list.CellHitTest(pt, nRow, nCol);
		if(nRow!=-1) {
			DWORD nCount=NumSelected();
			//CMsgBox("Row=%d Col=%d Selected: %u",nRow,nCol,nCount);
			CMenu menu;
			if(menu.LoadMenu(IDR_TABLE_CONTEXT)) {
				CMenu* pPopup = menu.GetSubMenu(0);
				ASSERT(pPopup != NULL);
				if(m_pShp->ShpType()!=CShpLayer::SHP_POINT) {
					pPopup->EnableMenuItem(ID_ADDTO_SELECTED,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
					pPopup->EnableMenuItem(ID_REPLACE_SELECTED,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}
				else if(!app_pShowDlg || app_pShowDlg->m_pDoc!=m_pShp->m_pDoc) {
					pPopup->EnableMenuItem(ID_ADDTO_SELECTED,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}

				if(!GE_IsInstalled())
					pPopup->ModifyMenu(ID_LAUNCH_GE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED,0,"GE not installed");
				else {
					if(!m_pShp->IsGeoRefSupported() || (nCount>1 && m_pShp->ShpType()!=CShpLayer::SHP_POINT))
						pPopup->EnableMenuItem(2,MF_BYPOSITION|MF_DISABLED|MF_GRAYED); //All GE options disabled
					else if(m_pShp->ShpType()!=CShpLayer::SHP_POINT)
						pPopup->EnableMenuItem(ID_OPTIONS_GE,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}

				if(nCount>1 || !m_pShp->IsGeoRefSupported())
						pPopup->DeleteMenu(3,MF_BYPOSITION); //Remove webmap options
				else {
					CString s;
					if(GetMF()->GetWebMapName(s))
						pPopup->ModifyMenu(ID_LAUNCH_WEBMAP,MF_BYCOMMAND|MF_STRING,ID_LAUNCH_WEBMAP,s);
				}

				{
					byte bd=0,bu=0;
					if(!(m_pShp->m_uFlags&NTL_FLG_EDITABLE) || m_pShp->ShpTypeOrg()!=CShpLayer::SHP_POINT && !m_bAllowPolyDel) {
						bd=bu=1;
					}
					else if(nCount==1) {
						DWORD rec=GetFirstSelectedRecNo();
						if(rec) {
							if(m_pShp->IsRecDeleted(rec)) bd=1;
							else bu=1;
						}
					}
					else if(m_uDelCount==0) bu=1;
					else if(m_nNumRecs==m_uDelCount) bd=1;
					if(bd) pPopup->EnableMenuItem(ID_DELETE_RECORDS,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
					if(bu) pPopup->EnableMenuItem(ID_UNDELETE_RECORDS,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
				}

				HMENU hPopup=NULL;
				if(m_pShp->ShpTypeOrg()==CShpLayer::SHP_POINT) {
					hPopup=m_pShp->m_pDoc->AppendCopyMenu(m_pShp,pPopup,"selection");
				}

				VERIFY(pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x,point.y,this));

				if(hPopup) DestroyMenu(hPopup);
			}
		}
	}
	return 0;
}

void CDBGridDlg::OpenMemo(DWORD nRec, UINT nFld)
{
	//We've clicked a memo field's button --

	static bool bNopenPrompt;

	if(m_pShp->IsMemoOpen(nRec,nFld)) return;

	if(app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp,nRec)) {
		if(m_uPromptRec!=nRec && !m_pShp->m_bDupEditPrompt && m_pShp->IsEditable() && !m_pShp->IsFldReadOnly(nFld)) {
			CString s;
			if(MsgCheckDlg(m_hWnd,MB_OK,
				"NOTE: This record is also the highlighted item in the Selected Points window.\n"
				"While that's the case, any changes you make to a memo field are provisional and\n"
				"won't affect the table display until edits made to the record are committed.",
				m_pShp->GetTitleLabel(s,nRec),
				"Don't display this again for this shapefile.")) m_pShp->m_bDupEditPrompt=true;

			m_uPromptRec=nRec;
		}
		app_pShowDlg->OpenMemo(nFld);
		return;
	}

	bool bViewMode=!CQuickList::m_bCtrl;
	EDITED_MEMO memo;
	memo.uRec=nRec;
	memo.fld=nFld;
	memo.recNo=CDBTFile::RecNo((LPSTR)m_pdb->FldPtr(nRec,nFld));

	if(bViewMode && memo.recNo) {
		char buf[256];
		bViewMode=CDBTData::IsTextIMG(m_pdbfile->dbt.GetTextSample(buf,256,memo.recNo));
	}
	else bViewMode=false;
	//m_bNotAllowingEdits=!m_bAllowEdits;
	m_pShp->OpenMemo(this,memo,bViewMode);
	//m_bNotAllowingEdits=false;
}

void CDBGridDlg::UpdateSelCount(UINT nCount)
{
	if((int)nCount<0) nCount=NumSelected();
	char strSelCount[64];
	_snprintf(strSelCount,63,"Selected: %u of %u (%u deleted)",m_nSelCount=nCount,m_nNumRecs,m_uDelCount);
	strSelCount[63]=0;
	GetDlgItem(IDC_ST_SELCOUNT)->SetWindowText(strSelCount);
}

void CDBGridDlg::OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//*pResult=0;
	if(m_bRedrawOff) return;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView && (pNMListView->uOldState&LVIS_SELECTED) != (pNMListView->uNewState&LVIS_SELECTED))
	{
		DWORD nCount=NumSelected();
		if(nCount!=m_nSelCount)	UpdateSelCount(nCount);
	}
}

void CDBGridDlg::OnItemStateChanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//*pResult=0;
	if(m_bRedrawOff) return;
	LPNMLVODSTATECHANGE lpStateChange = (LPNMLVODSTATECHANGE) pNMHDR;

	if (lpStateChange && (lpStateChange->uOldState&LVIS_SELECTED) != (lpStateChange->uNewState&LVIS_SELECTED))
	{
		UINT nCount=NumSelected();
		if(nCount!=m_nSelCount)	UpdateSelCount(nCount);
	}
}

void CDBGridDlg::OnMovetoTop()
{
	if(m_nNumRecs<2 || m_nSelCount<1) return;

	SetListRedraw(0);
	BeginWaitCursor();
	UINT numSel=FlagSelectedItems();

	std::stable_sort(m_vRecno.begin(),m_vRecno.end(),sort_compare_seltop);

	//Clear sort arrow --
	m_iColSorted=-1;
	m_list.SetSortArrow(m_iColSorted,m_bAscending);

	if(numSel) UnflagSelectedItems(numSel);

	EndWaitCursor();

	m_list.EnsureVisible(0,FALSE);
	SetListRedraw(1);
	m_list.Invalidate(0); //necessary!

}

void CDBGridDlg::OnLaunchWebMap()
{
	int i=m_list.GetSelectionMark();
	if(i>=0) m_pShp->OnLaunchWebMap(GetRecNo(i));
}

void CDBGridDlg::OnOptionsWebMap()
{
	CString s(CMainFrame::m_csWebMapFormat);
	int idx=CMainFrame::m_idxWebMapFormat;
	BOOL bRet=GetMF()->GetWebMapFormatURL(TRUE);
	if(bRet) {
		int i=m_list.GetSelectionMark();
		if(i>=0) {
			m_pShp->OnLaunchWebMap(GetRecNo(i));
			if(bRet>1) {
				CMainFrame::m_csWebMapFormat=s;
				CMainFrame::m_idxWebMapFormat=idx;
			}
		}
	}
}

void CDBGridDlg::OnLaunchGE()
{
	VEC_DWORD vRec;
	if(GetSelectedRecs(vRec)) {
		UINT oldRange=CShpLayer::m_uKmlRange;
		if(m_pShp->ShpType()!=CShpLayer::SHP_POINT) {
			ASSERT(vRec.size()==1);
			CShpLayer::m_uKmlRange=m_pShp->GetPolyRange(vRec[0]);
		}
		m_pShp->OnLaunchGE(vRec);
		CShpLayer::m_uKmlRange=oldRange;
	}
}

void CDBGridDlg::OnOptionsGE()
{
	VEC_DWORD vRec;
	if(GetSelectedRecs(vRec))
		m_pShp->OnOptionsGE(vRec);
}

void CDBGridDlg::AddToSelected(bool bReplace)
{
	CWallsMapDoc *pDoc=m_pShp->m_pDoc;
	ASSERT(bReplace==true || app_pShowDlg && app_pShowDlg->m_pDoc==pDoc); 
	UINT nSelected=NumSelected();
	ASSERT(nSelected && !CLayerSet::vec_shprec_srch.size());

	CWallsMapDoc::m_bSelectionLimited=false;
	if(pDoc->SelectionLimited(nSelected))
		return;

	if(!CheckShowDlg()) return;

	bool bMerging=(!bReplace && app_pShowDlg && app_pShowDlg->m_pDoc==pDoc);

	nSelected=GetSelectedShpRecs(CLayerSet::vec_shprec_srch,bMerging);

	if(!nSelected) {
		AfxMessageBox("The selection is already represented in the Selected Points window.");
		return;
	}

	if(bMerging) {
		nSelected+=app_pShowDlg->NumSelected();
		if(pDoc->SelectionLimited(nSelected)) {
			CLayerSet::Empty_shprec_srch();
			return;
		}

		CLayerSet::vec_shprec_srch.reserve(nSelected);
		pDoc->StoreSelectedRecs();
	}

	if(nSelected>1) {
		pDoc->InitLayerIndices();
		CShpLayer::Sort_Vec_ShpRec(CLayerSet::vec_shprec_srch);
	}

	pDoc->ReplaceVecShprec();

	//The above has reset selected record flags and refreshed table.

	if(!app_pShowDlg)
		app_pShowDlg=new CShowShapeDlg(pDoc);
	else //if(!bMerging || bMerged || app_pShowDlg->m_pDoc!=pDoc)
		app_pShowDlg->ReInit(pDoc);
}

void CDBGridDlg::OnReplaceSelected()
{
	AddToSelected(true);
}

void CDBGridDlg::OnAddToSelected()
{
	AddToSelected(false);
}

void CDBGridDlg::CenterView(BOOL bZoom)
{
	ASSERT(!CShpLayer::m_pGridShp && CShpLayer::m_vGridRecs.empty());
	CShpLayer::m_vGridRecs.clear(); //just in case
	if(GetSelectedRecs(CShpLayer::m_vGridRecs)) {
		CShpLayer::m_pGridShp=m_pShp;
		m_pShp->FitExtent(CShpLayer::m_vGridRecs,bZoom);
		VEC_DWORD().swap(CShpLayer::m_vGridRecs);
		CShpLayer::m_pGridShp=NULL;
	}
}

void CDBGridDlg::OnZoomView()
{
	CenterView(TRUE);
}

void CDBGridDlg::OnShowCenter()
{
	CenterView(FALSE);
}

void CDBGridDlg::OnSelectAll()
{
	m_list.SetItemState(-1,LVIS_SELECTED,LVIS_SELECTED);
}

void CDBGridDlg::OnRemoveFromTable()
{
	SetListRedraw(0);
	UINT numSel=FlagSelectedItems();
	if(!numSel) {
		SetListRedraw(1);
		return;
	}

	ASSERT(m_nNumRecs==m_vRecno.size());

#ifdef _DEBUG
	UINT count=numSel;
#endif

	if(!m_iColSorted && !m_bAscending) {
		//Delete items from top of m_Recno
		int i=m_nNumRecs-(pwFirst-&m_vRecno[0])-1;
		for(LPDWORD pw=pwFirst;pw<=pwLast;pw++,i--) {
			if(*pw&0x80000000) {
				m_list.DeleteItem(i);
				ASSERT(count--);
			}
		}
		ASSERT(!count);
	}
	else {
		//Delete items from bottom of m_Recno
		int i=pwLast-&m_vRecno[0];
		for(LPDWORD pw=pwLast;pw>=pwFirst;pw--,i--) {
			if(*pw&0x80000000) {
				m_list.DeleteItem(i);
				ASSERT(count--);
			}
		}
		ASSERT(!count);
	}

	m_nNumRecs-=numSel;
	pwLast++;

	for(LPDWORD pw=pwFirst;pw<pwLast;pw++) {
		if(*pw&0x80000000) {
			numSel--;
		}
		else *pwFirst++=*pw;
	}
	ASSERT(!numSel);
	numSel=(&m_vRecno[0]+m_vRecno.size())-pwLast;
	if(numSel) memcpy(pwFirst,pwLast,numSel*sizeof(DWORD));

	m_vRecno.resize(m_nNumRecs);

	SetListRedraw(1);
	CountDeletes();
	UpdateSelCount(0);
}

void CDBGridDlg::OnExportShapes()
{
	VEC_DWORD vRec;
	if(GetSelectedRecs(vRec))
		m_pShp->ExportRecords(vRec,true); //true if selected (affects title only)
}

#ifdef _XLS
void CDBGridDlg::OnExportXLS()
{
	VEC_DWORD vRec;
	if(GetSelectedRecs(vRec))
		m_pShp->ExportRecordsXLS(vRec); //true if selected (affects title only)
}
#endif

void CDBGridDlg::OnInvertselection()
{
	UINT nSel=NumSelected();

	if(!nSel || nSel==m_nNumRecs) {
		m_list.SetItemState(-1,nSel?0:LVIS_SELECTED,LVIS_SELECTED);
		return;
	}

	SetListRedraw(0);

	bool bRev=(!m_iColSorted && !m_bAscending);
	VERIFY(FlagSelectedItems(false));

	if(nSel<=m_nNumRecs/2) {
		m_list.SetItemState(-1,LVIS_SELECTED,LVIS_SELECTED); //select all
		//unselect the items that were flagged
		int i=pwFirst-&m_vRecno[0];
		int inc=1;
		if(bRev) {
			i=m_nNumRecs-i-1;
			inc=-1;
		}
		for(LPDWORD pw=pwFirst;pw<=pwLast;pw++,i+=inc) {
			if((*pw)&0x80000000) {
				(*pw)&=0x7FFFFFFF;
				m_list.SetItemState(i,0,LVIS_SELECTED);
			}
		}
	}
	else {
		m_list.SetItemState(-1,0,LVIS_SELECTED); //deselect all
		//select the items that were unflagged
		for(UINT i=0;i<m_nNumRecs;i++) {
			if(m_vRecno[i]&0x80000000) {
				m_vRecno[i]&=0x7FFFFFFF;
			}
			else {
			    m_list.SetItemState(bRev?(m_nNumRecs-i-1):i,LVIS_SELECTED,LVIS_SELECTED);
			}
		}
	}
	SetListRedraw(1);
	UpdateSelCount(-1);
}

void CDBGridDlg::OnEditFind()
{
	//if(m_iColSort<=0) m_iColSort=m_pShp->m_nLblFld;
	LPCSTR pnam="";
	BYTE ftyp=0;
	if(m_iColSort>0) {
		pnam=m_vFld[m_iColSort-1].pnam;
		ftyp=m_vFld[m_iColSort-1].ftyp;
	}

	if(m_pFindDlg) {
		if(m_iColSort>0 && !m_pFindDlg->m_iCol)
			m_pFindDlg->GetDlgItem(1152)->SetWindowText(m_csFind);
		m_pFindDlg->SetColumnData(pnam,m_iColSort,ftyp);
		m_pFindDlg->BringWindowToTop();
		return;
	}

	CFindDlgTbl *pDlg = new CFindDlgTbl(pnam,m_iColSort,ftyp);

	pDlg->m_fr.lpTemplateName=MAKEINTRESOURCE(IDD_FINDDLGTBL);

	DWORD dwFlags=FR_ENABLETEMPLATE|FR_DOWN;
	if(m_wFlags&F_MATCHCASE) dwFlags|=FR_MATCHCASE;
	if(m_wFlags&F_MATCHWHOLEWORD) dwFlags|=FR_WHOLEWORD;

	pDlg->m_bHasMemos=m_pdb->HasMemos();
		
	if(pDlg->Create(TRUE,m_csFind,NULL,dwFlags,this)) {
		CString s;
		s.Format("Find in %s",m_pShp->Title());
		pDlg->SetWindowText(s);
		m_pFindDlg=pDlg;
	}
	ASSERT(m_pFindDlg);
}

void CDBGridDlg::SelectFlaggedItems(int iStart,int iCount,int iCol)
{
	m_list.SetFocus();
	m_list.SetItemState(-1,0,LVIS_SELECTED); //clear selection
	if(iStart<0) {
		UnflagSelectedItems(iCount); //select flagged items while clearing flags
		//ASSERT(!IsFlagged(m_vRecno));
		iStart=GetFirstSelectedItem();
	}
	else {
		m_list.SetItemState(iStart,LVIS_SELECTED,LVIS_SELECTED);
	}
	m_list.SetSelectionMark(m_iLastFind=iStart);
	//int nTop=m_list.GetTopIndex();
	//int w=m_list.GetCountPerPage();
	//if(iStart<nTop || iStart>=nTop+w) {
	m_list.EnsureVisible(iStart,0);
	{
		int nTop=m_list.GetTopIndex();
		int nBot=nTop+m_list.GetCountPerPage()-1;
		if(nTop && nTop==iStart || nBot==iStart && (UINT)nBot<m_nNumRecs-1) {
			CRect rect;
			m_list.GetItemRect(nTop, &rect, LVIR_BOUNDS);
			nBot=rect.Height();
			m_list.Scroll(CSize(0,(nTop==iStart)?-nBot:nBot));
			m_list.Invalidate();
		}
	}
	m_list.SetNavigationColumn(iCol);
	m_list.MakeColumnVisible(iCol);
	UpdateSelCount(iCount);
}

LRESULT CDBGridDlg::OnFindDlgMessage(WPARAM wParam, LPARAM lParam)
{
    ASSERT(m_pFindDlg && m_pFindDlg==(CFindDlgTbl *)CFindReplaceDialog::GetNotifier(lParam));

    // If the FR_DIALOGTERM flag is set,
    // invalidate the handle identifying the dialog box.
    if (m_pFindDlg->IsTerminating())
    {
		//m_pFindDlg->DestroyWindow(); //??
        m_pFindDlg = NULL;
        return 0;
    }
    // If the FR_FINDNEXT flag is set,
    // call the application-defined search routine
    // to search for the requested string.

    if(m_pFindDlg->FindNext())
    {
		if(!m_nNumRecs) return 0;

		bool bDel=(m_pFindDlg->m_iCol==0);

		if(!bDel) {
			m_csFind=m_pFindDlg->GetFindString();
			ASSERT(!m_csFind.IsEmpty());
		}

		bool bSelectAll=m_pFindDlg->m_bSelectAll;
		VEC_BYTE v_sflds;
		int iCol=0;

		if(m_pFindDlg->SearchDown()) m_wFlags&=~F_SEARCHUP;
		else m_wFlags|=F_SEARCHUP;

		if(m_pFindDlg->m_bSearchRTF) m_wFlags|=F_MATCHRTF;
		else m_wFlags&=~F_MATCHRTF;

		if(!bDel) {
 			if(m_pFindDlg->MatchCase()) m_wFlags|=F_MATCHCASE;
			else m_wFlags&=~F_MATCHCASE;
			if(m_pFindDlg->MatchWholeWord()) m_wFlags|=F_MATCHWHOLEWORD;
			else m_wFlags&=~F_MATCHWHOLEWORD;
	
			if(m_pFindDlg->GetColumnSel()!=1) iCol=m_pFindDlg->m_iCol;

			if(!iCol) {
				SaveShpColumns();
				for(int nCol=1;nCol<m_nNumCols;nCol++) {
					if(m_pdbfile->col.width[nCol]>0 && (m_vFld[nCol-1].ftyp!='M' || m_pFindDlg->m_bSearchMemos))
						v_sflds.push_back((BYTE)nCol);
				}
			}
			else {
				if(m_pdbfile->col.width[iCol]>0)
					v_sflds.push_back((BYTE)iCol);
			}

			if(!v_sflds.size()) {
				AfxMessageBox("No searchable columns currently visible!");
				return 0;
			}
		}

		int iInc=1,iStart=0;
		if(!bSelectAll) {
			if(m_wFlags&F_SEARCHUP) iInc=-1;
			iStart=m_list.GetSelectionMark();
			ASSERT((UINT)iStart<m_nNumRecs);
			if(m_iLastFind==iStart) {
				iStart+=iInc;
				if(iStart<0) iStart=m_nNumRecs-1;
				else if((UINT)iStart>=m_nNumRecs) iStart=0;
			}
		}

		int lenTarget;
		char buf[256];
		LPCSTR pTarget=0;

		if(!bDel) {
			lenTarget=m_csFind.GetLength();
			if(lenTarget>=sizeof(buf)) {
				m_csFind.Truncate(lenTarget=sizeof(buf)-1);
			}
			pTarget=m_csFind;
			if(!(m_wFlags&F_MATCHCASE)) {
				SetLowerNoAccents(strcpy(buf,pTarget));
				pTarget=buf;
			}
		}

		UINT iCount=0;

		SetListRedraw(0);

		for(UINT i=0;i<m_nNumRecs;i++,iStart+=iInc) {
			if((UINT)iStart>=m_nNumRecs) {
				iStart=(iStart<0)?(m_nNumRecs-1):0;
				//alert that wrap occurred
				MessageBeep(MB_OK);
			}
			UINT iFld=GetRecNo(iStart);
			ASSERT(iFld<=m_pdb->NumRecs());
			VERIFY(!m_pdb->Go(iFld));
			if(bDel) {
				iFld=m_pShp->IsRecDeleted(iFld)?1:0;
			}
			else {
				iFld=m_pShp->FindInRecord(&v_sflds,pTarget,lenTarget,m_wFlags);
			}
			if(iFld) {
				iCount++;
				if(!bSelectAll) {
					iCol=iFld;
					break;
				}
				m_vRecno[GetRecIndex(iStart)]|=0x80000000;
			}
		}

		if(!iCount) {
			if(bDel)
				CMsgBox("No deleted records found");
			else
				CMsgBox("String '%s' not found.",(LPCSTR)m_csFind);

			m_pFindDlg->SetFocus();
		}
		else 
			SelectFlaggedItems(bSelectAll?-1:iStart,iCount,iCol);

		SetListRedraw(1);
    }

    return 0;
}

void CDBGridDlg::OnReload() 
{
	CReloadDlg dlg(m_pShp->Title(),m_nSubset,m_pShp->m_uDelCount,m_bRestoreLayout==true);
	if(dlg.DoModal()==IDOK) {
		m_nSubset=dlg.m_nSubset;
		m_bRestoreLayout=dlg.m_bRestoreLayout==TRUE;
		ReInitGrid(NULL);
	}
}

void CDBGridDlg::EnableAllowEdits()
{
	m_pdb=m_pShp->m_pdb;
	m_nNumCols=(m_nNumFlds=m_pdb->NumFlds())+1;
	InitFldInfo();
	BOOL bEnable=(m_pShp->IsEditable()==true);
	if(!bEnable && m_bAllowEdits) {
		CheckDlgButton(IDC_ALLOW_EDITS,BST_UNCHECKED);
		m_bAllowEdits=false;
		m_list.RedrawFocusItem();
	}
	GetDlgItem(IDC_ALLOW_EDITS)->EnableWindow(bEnable);
}

void CDBGridDlg::AllowPolyDel(bool bEnable)
{
	if(m_bAllowPolyDel==bEnable) return;
	CFileMap *pf=m_pdbfile->pfShp;
	ASSERT(pf && pf->IsOpen());

	if(bEnable) {
		m_extPolyOrg=m_pShp->Extent();
	}
	else if(!(m_extPolyOrg==m_pShp->Extent())) {
		//file is readwrite so update shp header's XY extent
		try {
			if(m_pShp->m_bConverted)
				m_pShp->ConvertPointsTo(&m_extPolyOrg.tl,m_pShp->m_iNadOrg,m_pShp->m_iZoneOrg,2);
			pf->Seek(9*4,CFile::begin); 	//offset in SHP_MAIN_HDR: double Xmin,Ymin,Xmax,Ymax = l,t,r,b
			std::swap(m_extPolyOrg.t,m_extPolyOrg.b);
			pf->Write(&m_extPolyOrg,sizeof(CFltRect));
			//Z and or M exents not used but calculated and preserved when exported
		}
		catch(...) {}
	}

	CString csPath=pf->GetFilePath();
	pf->Close();
	UINT flag=bEnable?CFile::modeReadWrite:CFile::modeRead;
	if(!pf->Open(csPath,flag|CFile::shareDenyWrite|CFile::osSequentialScan,0)) {
		ASSERT(bEnable);
		if(bEnable)
			VERIFY(pf->Open(csPath,CFile::modeRead|CFile::shareDenyWrite|CFile::osSequentialScan,0));
		return;
	}
	m_bAllowPolyDel=bEnable;
}

void CDBGridDlg::OnAllowEdits()
{
#if 0
#ifdef _DEBUG
	extern char debug_msg[];
	CMsgBox(debug_msg);
#endif
#endif

	m_bAllowEdits=(IsDlgButtonChecked(IDC_ALLOW_EDITS)!=BST_UNCHECKED);
	if(m_bAllowEdits) {
		ASSERT(m_pShp->IsEditable());
		if(!m_pShp->SaveAllowed(1)) {
			CheckDlgButton(IDC_ALLOW_EDITS,BST_UNCHECKED);
			m_bAllowEdits=false;
		}
		else if(m_pdbfile->pfShp && !m_pdbfile->bTypeZ) {
			//allow deletion of 2D polylines and polygons //***BUG!!
			//ASSERT(m_pdbfile->shpTypeOrg==CShpLayer::SHP_POLYGON || m_pdbfile->shpTypeOrg==CShpLayer::SHP_POLYLINE);
			AllowPolyDel(true);
		}
	}
	else AllowPolyDel(false); //updates header
	m_list.RedrawFocusItem();
}

void CDBGridDlg::RefreshTable(DWORD rec /*=0*/)
{
	if(rec) {
		//redraw only the row
		//determine indrx of rec
		int iTop=m_list.GetTopIndex();
		if(iTop<0) return; //LB_ERR
		int iBot=iTop+m_list.GetCountPerPage();
		if((UINT)iBot>m_nNumRecs) iBot=m_nNumRecs;
		for(;iTop<iBot;iTop++) {
			if(rec==GetRecNo(iTop)) {
				m_list.RedrawItems(iTop,iTop);
				m_list.UpdateWindow();
				break;
			}
		}
		return;
	}
	m_list.Invalidate();
	m_list.UpdateWindow();
}
#ifdef _DEBUG
static bool CheckExistence(CShpLayer *pShp,VEC_BADLINK &vBL)
{
	//return false if the full replacement path is bad OR (the replacement path is good AND the broken path is good)!
	char fullpath[MAX_PATH];
	bool bRet=true;
	for(VEC_BADLINK::const_iterator it=vBL.begin();it!=vBL.end();it++)
	{
		pShp->GetFullFromRelative(fullpath,it->sBrok);
		if(!_access(fullpath,0)) {
			bRet=false;
		}
		pShp->GetFullFromRelative(fullpath,it->sRepl);
		if(_access(fullpath,0)) {
			bRet=false;
		}
	}
	return bRet;
}
#endif

static UINT GetBadPathnames(int fcn,LINK_PARAMS &lp,LPCSTR pShpPath,LPCSTR pMemo)
{   
	//Count or retrieve bad links in string pMemo.
	//Called only from BadLinkSearch(int fcn, LINK_PARAMS &lp) --
	//
	//if(!fcn) return count only;
	//if(fcn==-2) index for log file only
	//else push_back BADLINKs in lp.vBL

	//lp.nLinkPos incremented for each link found, return count of bad links

	bool bLocal=!CDBTData::IsTextIMG(pMemo);
	char fpath[_MAX_PATH];
	UINT count=0;
	LPCSTR pEnd;
	LPCSTR pStart=pMemo;

	try {
		while(true) {
			if(bLocal) {
				if(!(pStart=strstr(pStart,CShpLayer::lpstrFile_pfx))) {
					break;
				}
				if(!(pEnd=strchr(pStart+=8,'>'))) {
					break;
				}
			}
			else {
				if(!(pStart=(LPSTR)CDBTData::NextImage(pStart)) || !(pStart=strstr(pStart+3,"\r\n")))
					break;
				pStart+=2;
				while(xisspace(*pStart)) pStart++; //start of relative path name
				pEnd=strstr(pStart,"\r\n");
				if(!pEnd) pEnd=pStart+strlen(pStart);
			}
			if(pEnd-pStart<_MAX_PATH) {
				//Link was found --
				lp.nLinkPos++; //total links
				CString path;
				path.SetString(pStart,pEnd-pStart);
				path.Replace('/','\\');
				if(!GetFullFromRelative(fpath,path,pShpPath) || _access(fpath,0)) {
				    //Bad link found --
					count++;
					if(fcn) {
						if(fcn==-2) {
							//update log index --
							char keybuf[256];
							int e=CDBGridDlg::m_trx.InsertKey(trx_Stncp(keybuf,path,256));
							ASSERT(!e || e==CTRXFile::ErrDup);
						}
						else {
							//save path as stored in memo (normally a relative path),
							//along with its relative position in memo  --
							ASSERT(fcn==1 || fcn==-1);
							lp.vBL.push_back(BADLINK(pMemo,pStart,pEnd-pStart));
						}
					}
					//else only counting
				}
			}

			if(!*pEnd) break;
			pStart=pEnd+1;
		}
		return count;
	}
	catch(...) {
		ASSERT(0);
	}
	return 0;
}

BOOL CDBGridDlg::BadLinkSearch(int fcn, LINK_PARAMS &lp)
{
	//fcn=-1 - lp.nRow=0, called when Select All button pressed
	//fcn=-2 - lp.nRow=0, called from OnBnClickedCreateLog()
	//fcn=0  - lp.nRow=0, return total broken links (lp.nNew will have link total, lp.nRow=row of first bad link)
	//fcn=1  - lp.nRow++,  find next row with bad links and return count
	//fcn=2  - lp.nNew=links to update, should return TRUE, lp.nNew=0 after call

	LPCSTR pMemo;
	UINT dbtrec;

	if(fcn<2) {
		lp.vBL.clear();
		lp.nLinkPos=0;
		lp.nNew=0;

		UINT badcount=0;
		for(UINT r=lp.nRow;r<m_nNumRecs;r++) {
			VERIFY(!m_pdb->Go(lp.nRec=GetRecNo(r)));
			dbtrec=CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(lp.nFld));

			UINT uDataLen=0; //get complete memo
			if(!dbtrec || !(pMemo=m_pdbfile->dbt.GetText(&uDataLen,dbtrec))) continue;

			if(!fcn) lp.nNew++; //count of total records with links

			UINT nBad=GetBadPathnames(fcn,lp,m_pShp->PathName(),pMemo);
			if(nBad) {
				if(fcn>0) {
					lp.nRow=r;
					lp.nLinkPos=0;
					lp.vBLszMemo=uDataLen;
					return nBad;
				}
				if(!badcount) lp.nRow=r;
				if(!fcn) badcount+=nBad;
				else {
					badcount++;
					m_vRecno[GetRecIndex(r)]|=0x80000000;
				}
			}
		}
		ASSERT(!badcount || fcn<=0);
		return badcount;
	}

	ASSERT(fcn==2);

	#ifdef _DEBUG
	if(!CheckExistence(m_pShp,lp.vBL)) {
		return FALSE;
	}
	#endif

	ASSERT(lp.nNew && lp.nRec==m_pdb->Position());
	dbtrec=CDBTFile::RecNo((LPCSTR)m_pdb->FldPtr(lp.nFld));
	UINT uDataLen=0; //get complete memo
	if(!dbtrec || !(pMemo=m_pdbfile->dbt.GetText(&uDataLen,dbtrec))) {
		ASSERT(0); //should be impossible
		return FALSE;
	}
	CString sMemo,sPart;
	LPCSTR p0=pMemo;
	for(VEC_BADLINK::const_iterator it=lp.vBL.begin();it!=lp.vBL.end();it++) {
		if(it->sRepl.IsEmpty()) continue;
		sPart.SetString(p0,pMemo+it->nBrokOff-p0);
		sMemo+=sPart;
		sMemo+=it->sRepl;
		p0=pMemo+it->nBrokOff+it->sBrok.GetLength();
	}
	sMemo+=p0;
	VERIFY(!m_pdb->Go(lp.nRec));
	EDITED_MEMO memo;
	memo.bChg=1;
	memo.recNo=dbtrec;
	memo.recCnt=EDITED_MEMO::GetRecCnt(lp.vBLszMemo); //originally set to zero with comment "not used"!
	memo.pData=(LPSTR)(LPCSTR)sMemo;
	CDBTFile::SetRecNo((LPSTR)m_pdb->FldPtr(lp.nFld),m_pdbfile->dbt.PutText(memo));
	return m_pdbfile->dbt.FlushHdr(); //error msg if false
}

void CDBGridDlg::OnLinkTest()
{
	ASSERT(m_wLinkTestFld);
	CLinkTestDlg dlg(this,m_wLinkTestFld);

	if(IDOK!=dlg.DoModal())
		return;

	if(dlg.m_lp.nRec) {
		SetListRedraw(0);
		SelectFlaggedItems(-1,dlg.m_lp.nRec,m_wLinkTestFld);
		//ASSERT(!IsFlagged(m_vRecno));
		SetListRedraw(1);
	}
}

bool CDBGridDlg::IsPtOpen(DWORD rec)
{
	return app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp,rec) ||
			app_pDbtEditDlg && app_pDbtEditDlg->IsRecOpen(m_pShp,rec) ||
			app_pImageDlg && app_pImageDlg->IsRecOpen(m_pShp,rec);
}

void CDBGridDlg::DeleteRecords(bool bDelete)
{
	ASSERT(m_nSelCount);

	if(bDelete && m_nSelCount>=50) {
		if(IDOK!=CMsgBox(MB_OKCANCEL,"Select OK to confirm deletion of %u records.\n\n"
			"Deleted records, being restorable, occupy space in the files. If it's your intention to remove a significant portion of a shapefile it "
			"can be more efficient to export the records you would like to keep.",m_nSelCount)) return;
	}

	UINT nDel=0,nLock=0,nSel=0;
	CWallsMapView::m_bTestPoint=false; //to check if edited pt is within view
	CSyncHint hint;

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)	{
	  int nItem = m_list.GetNextSelectedItem(pos);
	  if(!m_iColSorted && !m_bAscending) {
		  nItem=m_nNumRecs-nItem-1;
	  }
	  DWORD rec=m_vRecno[nItem];
	  if(bDelete==m_pShp->IsRecDeleted(rec) || !m_pdbfile->ppoly && IsPtOpen(rec) && ++nLock) continue;

	  if(!m_pShp->ToggleDelete(rec)) continue;
	  nDel++;

	  RefreshTable(rec);

	  if(!m_pdbfile->ppoly) {
		  hint.fpt=&m_pShp->m_fpt[rec-1];
		  m_pShp->m_pDoc->UpdateAllViews(NULL,LHINT_TESTPOINT,&hint);
		  if(IsPtSelected(rec)) nSel++;
	  }
	}

	if(bDelete) m_uDelCount+=nDel;
	else m_uDelCount-=nDel;
	UpdateSelCount(m_nSelCount); //shows m_uDelCount, selection shouldn't have changed
	
	if(nDel && m_pdbfile->ppoly) {
		m_pdbfile->pfShp->Flush();
		m_pdb->Flush();
		m_pShp->UpdateSizeStr();
		if(hPropHook && m_pShp->m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(m_pShp);

		CFltRect extOld(m_pShp->Extent());
		m_pShp->UpdatePolyExtent();

		if(!(extOld==m_pShp->Extent())) {
			m_pShp->m_pDoc->LayerSet().UpdateExtent();
		}

		if(m_pShp->IsVisible())
			m_pShp->m_pDoc->RefreshViews();
		return;
	}

	LPCSTR pTyp=bDelete?" deleted":" restored";

	if(nDel) {
		m_pdb->Flush();
		m_pShp->UpdateShpExtent();
		m_pShp->UpdateSizeStr();
		if(hPropHook && m_pShp->m_pDoc==pLayerSheet->GetDoc())
				pLayerSheet->RefreshListItem(m_pShp);

		m_pShp->m_bEditFlags|=SHP_EDITDEL;
		m_pShp->SaveShp();

		if(CWallsMapView::m_bTestPoint) {
			CWallsMapView::m_bTestPoint=false;
			m_pShp->m_pDoc->RefreshViews();
		}

		if(nSel) {
			ASSERT(app_pShowDlg);
			if(app_pShowDlg) app_pShowDlg->InvalidateTree();
		}
		if(nLock)
			CMsgBox("NOTE: %u of %u selected items were successfully%s. "
			"%u record(s) couldn't be%s due to being open in another window.",
			nDel,m_nSelCount,pTyp,nLock,pTyp);
	}
	else {
		CWallsMapView::m_bTestPoint=false;
		CMsgBox("No records were%s. The selected items are either already%s or are open in another window.",pTyp,pTyp);
	}
}

void CDBGridDlg::OnDeleteRecords()
{
	DeleteRecords(true);
}

void CDBGridDlg::OnUndeleteRecords()
{
	DeleteRecords(false);
}

void CDBGridDlg::CopySelected(CShpLayer *pLayer,LPBYTE pSrcFlds,BOOL bConfirm)
{
	BeginWaitCursor();
	bool bUpdateLocFlds=pLayer->HasLocFlds() &&
		(bConfirm>1 || !pLayer->IsSameProjection(m_pShp));

	UINT nCopied=0;
	TRUNC_DATA td;
	m_csCopyMsg.Empty();

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while(pos)	{
		int nItem = m_list.GetNextSelectedItem(pos);
		if(!m_iColSorted && !m_bAscending) {
			nItem=m_nNumRecs-nItem-1;
		}
		DWORD rec=m_vRecno[nItem];
		rec=pLayer->CopyShpRec(m_pShp,rec,pSrcFlds,bUpdateLocFlds,td);
		if(!rec) {
			ASSERT(0);
			break; //error
		}
		ASSERT(rec==pLayer->m_nNumRecs && rec==pLayer->m_pdb->NumRecs());
		nCopied++;
		if(!(nCopied%100) && m_pCopySelectedDlg) {
			if(!m_pCopySelectedDlg->ShowCopyProgress(nCopied)) break;
		}
	}
	VERIFY(!pLayer->m_pdb->Flush());

	if(pLayer->HasMemos())
	  VERIFY(pLayer->m_pdbfile->dbt.FlushHdr());
	pLayer->m_bEditFlags|=SHP_EDITADD;
	pLayer->SaveShp();
	pLayer->UpdateSizeStr();
	if(hPropHook && pLayer->m_pDoc==pLayerSheet->GetDoc())
		pLayerSheet->RefreshListItem(pLayer);
	pLayer->m_pDoc->LayerSet().UpdateExtent();
	pLayer->m_pDoc->RefreshViews(); //required for new points to be selectable

	//Refresh other document views that contain this layer unconditionally  --
	if(pLayer->m_pdbfile->nUsers>1)
		pLayer->UpdateDocsWithPoint(pLayer->m_nNumRecs);

	EndWaitCursor();

	m_csCopyMsg.Format("%u item%s successfully copied to %s.",nCopied,(nCopied==1)?"":"s",pLayer->Title());
	if(td.count) {
		m_csCopyMsg.AppendFormat("\n\nCAUTION: %u field values were truncated, the first being field %s of record %u.",
			td.count,pLayer->m_pdb->FldNamPtr(td.fld),td.rec);
	}
}

void CDBGridDlg::OnCopySelected(UINT id)
{
	ASSERT(id<ID_APPENDLAYER_N);

	CShpLayer *pLayer=(CShpLayer *)CLayerSet::vAppendLayers[id-ID_APPENDLAYER_0];
	ASSERT(pLayer);

	if(pLayer->m_bDragExportPrompt) {
		CString msg;
		msg.Format("Are you sure you want to append %u record%s to %s?",m_nSelCount,(m_nSelCount>1)?"s":"",pLayer->Title());
		int i=MsgCheckDlg(m_hWnd,MB_OKCANCEL,msg,m_pShp->Title(),"Don't ask me this again for this target");
		if(!i) return;
		pLayer->m_bDragExportPrompt=(i==1); //if 2, don't ask again
	}

	ASSERT(pLayer->CanAppendShpLayer(m_pShp));

	BYTE bSrcFlds[256];
	LPBYTE pSrcFlds=NULL;
	BOOL bConfirm=0; //0,1,2
	if(!pLayer->m_pdb->FieldsMatch(m_pdb)) {
		if(!(bConfirm=pLayer->ConfirmAppendShp(m_pShp,pSrcFlds=bSrcFlds)))
			return;
	}

	if(m_nSelCount>=5000) {
		CCopySelectedDlg dlg(pLayer,pSrcFlds,bConfirm,m_nSelCount,this);
		m_pCopySelectedDlg=&dlg;
		dlg.DoModal();
	}
	else {
		m_pCopySelectedDlg=NULL;
		CopySelected(pLayer, pSrcFlds, bConfirm);
	}
	if(!m_csCopyMsg.IsEmpty()) AfxMessageBox(m_csCopyMsg);
}

bool CDBGridDlg::ConfirmFillRepl()
{
	if(m_pShp->m_bFillPrompt) {
		CString msg;
		msg.Format("Confirm that you want to revise field %s in %u%s records without prompting.",
			m_pdb->FldNamPtr(m_wTableFillFld),m_nSelCount,(m_nSelCount>=50)?" (!)":"");
		if(m_pShp->m_pdbfile->HasTimestamp(1))
			msg.AppendFormat("\nThis operation will %s revise the UPDATED timestamps.", bIgnoreEditTS?"NOT":"also");

		int i=MsgCheckDlg(m_hWnd,MB_OKCANCEL,msg,m_pShp->Title(),"Don't require confirmation again with this shapefile");
		if(!i) return false;
		m_pShp->m_bFillPrompt=(i==1); //if 2, don't prompt again
	}
	return true;
}

void CDBGridDlg::OnTableFill()
{
	CString msg;
	ASSERT(m_wTableFillFld);
	CTableFillDlg dlg(m_wTableFillFld,this);

	if(IDOK!=dlg.DoModal() || !ConfirmFillRepl()) return;

	BeginWaitCursor();
	char fldbuf[256];
	UINT nFilled=0,uOpenRec=0;
	char fTyp=m_pdb->FldTyp(m_wTableFillFld);
	WORD fLen=m_pdb->FldLen(m_wTableFillFld);
	UINT fLenData=dlg.m_csFillText.GetLength();
	BYTE fillBool=(fTyp=='L' && dlg.m_bYesNo==0)?'Y':'N';
	if(fTyp=='C') {
		memset(fldbuf,' ',fLen);
		memcpy(fldbuf,dlg.m_csFillText,fLenData);
	}

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)	{
	  int nItem=m_list.GetNextSelectedItem(pos);
	  if(!m_iColSorted && !m_bAscending) {
		  nItem=m_nNumRecs-nItem-1;
	  }
	  UINT rec=m_vRecno[nItem];
	  if(app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp, rec)) {
		  uOpenRec++;
		  continue;
	  }
	  if(fTyp=='M') {
		  if(app_pDbtEditDlg && app_pDbtEditDlg->IsMemoOpen(m_pShp, rec, m_wTableFillFld) ||
			  app_pImageDlg && app_pImageDlg->IsMemoOpen(m_pShp, rec, m_wTableFillFld)) {
			  uOpenRec++;
			  continue;
		  }
	  }
	  if(!m_pdb->Go(rec)) {
	      bool bEdited=false;
		  LPBYTE p=m_pdb->FldPtr(m_wTableFillFld);
		  switch(fTyp) {
			  case 'L' :
				  if(*p!=fillBool) {
					  *p=fillBool;
					  bEdited=true;
				  }
				  break;
			  case 'C' :
				  if(memcmp(fldbuf, p, fLen)) {
					  memcpy(p,fldbuf,fLen);
					  bEdited=true;
				  }
			      break;
			  case 'M':
				  {
				    EDITED_MEMO memo;
					memo.recNo=CDBTFile::RecNo((LPCSTR)p);
					if(memo.recNo) {
					    //need to compare dlg.m_csFillText with existing memo
						UINT oldLen=0; //get complete memo
						// oldLen needed for recCnt even if fLenData==0 (will add to free blocks in dbt) --
						LPSTR pMemo=m_pdbfile->dbt.GetText(&oldLen,memo.recNo);
						if(!(pMemo)) {
							ASSERT(0); //should be impossible
							break;
						}
						if(oldLen==fLenData && !memcmp(pMemo,dlg.m_csFillText,fLenData))
						   break;
						memo.recCnt=EDITED_MEMO::GetRecCnt(oldLen);
					}
					else if(!fLenData)
						break;
					memo.pData=(LPSTR)(LPCSTR)dlg.m_csFillText;
					CDBTFile::SetRecNo((LPSTR)p,m_pdbfile->dbt.PutText(memo));
					m_pdbfile->dbt.FlushHdr();
					bEdited=true;
				  }
				  break;
		  }
		  if(bEdited) {
			  nFilled++;
			  m_pdb->FlushRec();
			  m_pShp->ChkEditDBF(rec);
			  if(m_pShp->m_pdbfile->HasTimestamp(1)) {
				  ASSERT(SHP_DBFILE::bEditorPrompted);
				  m_pShp->UpdateTimestamp(m_pdb->RecPtr(),FALSE);
			  }
		  }
	  }
	} //pos

	if(nFilled) {
		m_list.Invalidate(0);
	}

	msg.Format("%u of %u selected %s revised.",nFilled,m_nSelCount,
		(fTyp=='M')?"memo fields successfully":"records");
	if(uOpenRec)
		msg.AppendFormat("\n\nNOTE: %u records open in other dialogs were skipped.",uOpenRec);

	EndWaitCursor();
	//AfxMessageBox(msg);
	MsgInfo(m_hWnd,msg,m_pShp->Title());
	m_list.SetFocus();

}

static bool __inline isdelim(char c)
{
	return strchr(WORD_SEPARATORS,c)!=NULL;
}

static int ReplaceStr(CString &s, LPCSTR psu, LPCSTR pOld, LPCSTR pNew, BOOL bMatchWord,int lenMax)
{
	//If psu!=0, it points to an upper case version of s that we must scan for matches,
	//otherwise if !bMatchWord we can perform a simple replacement and check the length;

	int nRepl=0;
	int lenS=s.GetLength();
	int lenOld=strlen(pOld);
	int lenNew=strlen(pNew);
	int incLen=lenNew-lenOld;
	VEC_INT vOff;

	if(!psu && !bMatchWord) {
		nRepl=s.Replace(pOld,pNew);
		return (lenS+nRepl*incLen>lenMax)?0:nRepl;
	}

	if(!psu) {
		//Don't ignore case and operate directly on string.
		//First, save offsets of valid matches assuming bulding from left --
		for(int i=0; (i=s.Find(pOld, i))>=0;i+=lenOld) {
			if((!i || isdelim(s[i-1])) && ((lenS==i+lenOld) || isdelim(s[i+lenOld]))) {
				vOff.push_back(i);
				nRepl++;
			}
		}
	}
	else {
		//ignoring case - determine replacement offsets using uppercase version
		for(LPCSTR p=psu; p=strstr(p, pOld); p+=lenOld) {
			if(!bMatchWord || ((p==psu || isdelim(p[-1])) && (!p[lenOld] || isdelim(p[lenOld])))) {
				vOff.push_back(p-psu);
				nRepl++;
			}
		}
	}

	if(!nRepl) return 0;
	if(lenS+nRepl*incLen>lenMax) return -1;

	LPSTR pDst0=s.GetBuffer(lenS+((incLen>0)?(nRepl*incLen):0)+1);
	LPSTR pSrc0=pDst0;

	if(incLen>0) {
		//one data shift to right is required
		pSrc0=(LPSTR)memmove(pSrc0+nRepl*incLen,pSrc0,lenS+1);
	}

	nRepl=0;
	for(it_int it=vOff.begin();it!=vOff.end();it++,nRepl++) {
		if(incLen) {
			//first, restore gap following last replacement
			int lenGap=nRepl?(*it-*(it-1)-lenOld):*it;
			LPSTR pSrc=pSrc0+*it-lenGap;
			LPSTR pDst=pDst0+*it-lenGap+nRepl*incLen;
			memcpy(pDst,pSrc,lenGap);
			memcpy(pDst+lenGap,pNew,lenNew);
			if(it+1==vOff.end()) {
				strcpy(pDst+lenGap+lenNew,pSrc+lenGap+lenOld);
			}
		}
		else {
			memcpy(pDst0+*it, pNew, lenNew);
		};
	}

	s.ReleaseBuffer();

	return nRepl;
}

void CDBGridDlg::OnTableRepl()
{
	CString s;
	ASSERT(m_wTableFillFld);
	CTableReplDlg dlg(m_wTableFillFld,this);

	if(IDOK!=dlg.DoModal() || !ConfirmFillRepl()) return;
	
	BeginWaitCursor();

	UINT nRecUpdates=0,nReplacements=0,nOpenRec=0,nOverflow=0;
	char fTyp=m_pdb->FldTyp(m_wTableFillFld);
	WORD fLen=m_pdb->FldLen(m_wTableFillFld);

	LPCSTR pOldStr=(LPCSTR)dlg.m_csFindWhat;
	LPCSTR pNewStr=(LPCSTR)dlg.m_csReplWith;
	LPCSTR psu=NULL; //pointer to uppercase copy of field value if needed

	UINT lenNew=strlen(pNewStr);
	UINT lenOld=strlen(pOldStr);

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)	{
	  int nItem=m_list.GetNextSelectedItem(pos);
	  if(!m_iColSorted && !m_bAscending) {
		  nItem=m_nNumRecs-nItem-1;
	  }
	  UINT rec=m_vRecno[nItem];
	  if(app_pShowDlg && app_pShowDlg->IsRecOpen(m_pShp, rec)) {
		  nOpenRec++;
		  continue;
	  }
	  if(fTyp=='M') {
		  if(app_pDbtEditDlg && app_pDbtEditDlg->IsMemoOpen(m_pShp, rec, m_wTableFillFld) ||
			  app_pImageDlg && app_pImageDlg->IsMemoOpen(m_pShp, rec, m_wTableFillFld)) {
			  nOpenRec++;
			  continue;
		  }
	  }
	  if(!m_pdb->Go(rec)) {
		  int nRepl=0;
		  LPSTR p=(LPSTR)m_pdb->FldPtr(m_wTableFillFld);
		  switch(fTyp) {
			  case 'C' :
				  s.SetString(p,fLen);
				  s.TrimRight();
				  if(!lenOld) {
					  if(!s.IsEmpty()) continue;
					  memcpy(p,pNewStr,lenNew);
					  nRepl=1;
				  }
				  else {
					  CString su;
					  if((int)lenOld>s.GetLength())
						  continue; //no possible match
					  if(!dlg.m_bMatchCase) {
						  su=s;
						  su.MakeUpper();
						  psu=(LPCSTR)su;
					  }
					  nRepl=ReplaceStr(s,psu,pOldStr,pNewStr,dlg.m_bMatchWord,fLen);
					  if(nRepl<=0) {
						  if(nRepl<0) nOverflow++;
						  continue;
					  }
					  memset(p,' ',fLen);
					  memcpy(p,(LPCSTR)s,s.GetLength());
				  }
			      break;

			  case 'M':
			  {
					EDITED_MEMO memo;
					memo.recNo=CDBTFile::RecNo((LPCSTR)p);
					if(memo.recNo) {
						if(!lenOld) continue; //only modify empty memos if "find what" empty
						UINT lenData=0; //get complete memo
						psu=m_pdbfile->dbt.GetText(&lenData, memo.recNo);
						if(lenOld>lenData)
							continue;
						s=psu;
						if(!dlg.m_bMatchCase) _strupr((LPSTR)psu);
						else psu=NULL;
						nRepl=ReplaceStr(s, psu, pOldStr, pNewStr, dlg.m_bMatchWord, lenData+64*1024);
						if(nRepl<=0) {
							if(nRepl<0) nOverflow++;
							continue;
						}
						memo.recCnt=EDITED_MEMO::GetRecCnt(lenData);
						memo.pData=(LPSTR)(LPCSTR)s;
					}
					else {
						//existing memo is empty - modify only if lenOld==0 
						if(lenOld) continue;
						ASSERT(lenNew);
						memo.pData=(LPSTR)pNewStr;
						nRepl=1;
					}
					
					CDBTFile::SetRecNo((LPSTR)p,m_pdbfile->dbt.PutText(memo));
					m_pdbfile->dbt.FlushHdr();
					break;
			  }
		  } //switch(fTyp)

		  if(nRepl>0) {
			  nRecUpdates++;
			  nReplacements+=nRepl;
			  m_pdb->FlushRec();
			  m_pShp->ChkEditDBF(rec);
			  if(m_pShp->m_pdbfile->HasTimestamp(1)) {
				  ASSERT(SHP_DBFILE::bEditorPrompted);
				  m_pShp->UpdateTimestamp(m_pdb->RecPtr(),FALSE);
			  }
		  }
	  } //Go()
	} //while(pos)

	if(nRecUpdates) {
		m_list.Invalidate(0);
	}

	s.Format("%u of %u selected records revised ", nRecUpdates, m_nSelCount);
	if(nRecUpdates) s.AppendFormat("with %u replacements.",nReplacements);
	else s+="(No matches found.)";

	if(nOpenRec || nOverflow) {
		s+="\n\nCAUTION: ";
		if(nOpenRec) s.AppendFormat("%u records open in other dialogs were not examined. ", nOpenRec);
		if(nOverflow) {
			s.AppendFormat("%u records were left unchanged to avoid exceeding the field length.");
		}
	}

	EndWaitCursor();
	MsgInfo(m_hWnd,s,m_pShp->Title());
	m_list.SetFocus();

}

LRESULT CDBGridDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1016);
	return TRUE;
}
