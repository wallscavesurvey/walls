// LayerSetPropSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "MainFrm.h"
#include "WallsMapDoc.h"
#include "WallsMapView.h"
#include "ShowShapeDlg.h"
#include "DbtEditDlg.h"
#include "ImageViewDlg.h"
#include "DlgSymbols.h"
#include "DlgSymbolsPoly.h"
#include "ScaleDlg.h"
#include "DBGridDlg.h"
#include "MsgCheck.h"
#include "NtiLayer.h"
#include "DlgSymbolsImg.h"
#include "TitleDlg.h"
#include "LayerSetSheet.h"
#include "CompareDlg.h"
#include "UpdZipDlg.h"
#include <direct.h>
#include "PageLayers.h"

#define _H(item) ((HTREEITEM)(item))

/////////////////////////////////////////////////////////////////////////////
CPageLayers::CPageLayers()
	: CLayerSetPage(CPageLayers::IDD), m_Tree(m_LayerTree.GetTreeCtrl())
{
	m_pDoc = NULL;
}

CPageLayers::~CPageLayers()
{
}

BEGIN_MESSAGE_MAP(CPageLayers, CLayerSetPage)
	//ON_NOTIFY(NM_CUSTOMDRAW, IDC_LAYERS_LIST, OnCustomdrawMyList )
	//ON_NOTIFY(LVN_ITEMCHANGED, IDC_LAYERS_LIST, OnItemChangedList)
	//ON_NOTIFY(LVN_GETDISPINFO, IDC_LAYERS_LIST, GetDispInfo)
	//ON_NOTIFY(NM_CLICK, IDC_LAYERS_LIST, OnClickList)
	//ON_NOTIFY(NM_RCLICK, IDC_LAYERS_LIST, OnRtClickList)
	//ON_NOTIFY(NM_DBLCLK, IDC_LAYERS_LIST, OnDblClickList)
	//ON_NOTIFY(LVN_KEYDOWN, IDC_LAYERS_LIST, OnKeydownList)
	ON_BN_CLICKED(IDC_SCALERANGE, OnBnClickedScaleRange)
	ON_BN_CLICKED(IDC_LAYER_ADD, OnBnClickedLayerAdd)
	ON_BN_CLICKED(IDC_LAYER_CONV, OnBnClickedLayerConv)
	ON_BN_CLICKED(IDC_LAYER_DEL, OnBnClickedLayerDel)
	ON_BN_CLICKED(IDCANCEL, OnClose)
	//ON_BN_CLICKED(IDC_LAYER_DOWN, OnBnClickedLayerDown)
	//ON_BN_CLICKED(IDC_LAYER_UP, OnBnClickedLayerUp)
	ON_REGISTERED_MESSAGE(WM_RETFOCUS, OnRetFocus)
	ON_COMMAND(ID_MOVELAYERTOTOP, OnMoveLayerToTop)
	ON_COMMAND(ID_MOVELAYERTOBOTTOM, OnMoveLayerToBottom)
	//ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_ZOOMTOLAYER, OnZoomtolayer)
	ON_BN_CLICKED(IDC_SYMBOLOGY, OnBnClickedSymbology)
	ON_BN_CLICKED(IDC_ATTRIBUTES, OnBnClickedOpenTable)
	ON_COMMAND(ID_OPEN_TABLE, OnBnClickedOpenTable)
	ON_COMMAND(ID_EDITTITLE, OnEditTitle)
	ON_COMMAND(ID_CLEAR_EDITFLAGS, OnClearEditflags)
	ON_COMMAND(ID_SET_EDITABLE, OnSetEditable)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_OPENFOLDER, OnOpenfolder)
	ON_COMMAND(ID_APPENDLAYER_N, OnOpenXC)
	ON_COMMAND(ID_CLOSE_AND_DELETE, OnCloseAndDelete)
	ON_COMMAND(ID_CLOSE_LAYER, OnBnClickedLayerDel)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_CHECKBOX_CLICKED, OnCheckbox)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_END_DRAG, OnMoveItem)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_ITEM_DBLCLCK, OnDblClickItem)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_ITEM_RCLICKED, OnRClickItem)
	ON_REGISTERED_MESSAGE(WM_XHTMLTREE_SELCHANGE, OnSelChange)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_COMMAND(ID_FOLDER_ABOVE, &CPageLayers::OnFolderAbove)
	ON_COMMAND(ID_FOLDER_BELOW, &CPageLayers::OnFolderBelow)
	ON_COMMAND(ID_FOLDER_CHILD, &CPageLayers::OnFolderChild)
	ON_COMMAND(ID_FOLDER_PARENT, &CPageLayers::OnFolderParent)
	ON_COMMAND(ID_LAYERS_ABOVE, &CPageLayers::OnLayersAbove)
	ON_COMMAND(ID_LAYERS_BELOW, &CPageLayers::OnLayersBelow)
	ON_COMMAND(ID_LAYERS_WITHIN, &CPageLayers::OnLayersWithin)
	ON_COMMAND(ID_COMPARE_SHP, &CPageLayers::OnCompareShp)
	ON_COMMAND(ID_TEST_MEMOS, &CPageLayers::OnTestMemos)

END_MESSAGE_MAP()

void CPageLayers::DoDataExchange(CDataExchange* pDX)
{
	CLayerSetPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLUMNTREE, m_LayerTree);
}

BOOL CPageLayers::OnInitDialog()
{
	CLayerSetPage::OnInitDialog();

	AddControl(IDC_LAYER_ADD, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_LAYER_DEL, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_LAYER_CONV, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_ATTRIBUTES, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_SYMBOLOGY, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDC_SCALERANGE, CST_REPOS, CST_REPOS, CST_NONE, CST_NONE, 0);
	AddControl(IDCANCEL, CST_REPOS, CST_REPOS, CST_NONE, CST_REPOS, 0);
	AddControl(IDC_COLUMNTREE, CST_RESIZE, CST_RESIZE, CST_RESIZE, CST_RESIZE, 0);

	m_dropTarget.Register(this, DROPEFFECT_COPY);
	m_dropTarget.m_bAddingLayers = true;
	ASSERT(!m_LayerTree.GetHeaderCtrl().GetItemCount());
	m_Tree.Initialize(); //sets styles, state image list & m_bDestroyingTree=FALSE
	m_LayerTree.SetFirstColumnMinWidth(200);
	UpdateLayerData(Sheet()->GetDoc());
	m_Tree.SetFocus();
	return 0;
}

#if 0
BOOL CPageLayers::OnKillActive()
{
	//ASSERT(m_pDoc);
	//SaveLayerStatus();
	return CPropertyPage::OnKillActive();
}
#endif

// CPageLayers message handlers
LRESULT CPageLayers::OnRetFocus(WPARAM, LPARAM)
{
	//if(m_idLastFocus) GetDlgItem(m_idLastFocus)->SetFocus();
	//else m_idLastFocus=checkid(GetFocus()->GetDlgCtrlID(),getids());
	if (m_pDoc != Sheet()->GetDoc())
		UpdateLayerData(Sheet()->GetDoc());
	m_pDoc->GetPropStatus().nTab = 0;
	m_Tree.SetFocus();
	return TRUE;
}

void CPageLayers::SelectDocTreePosition()
{
	HTREEITEM hItem = m_pDoc->SelectedLayerPtr()->m_hItem;
	HTREEITEM hParent = m_Tree.GetParentItem(hItem);
	if (hParent && !m_Tree.GetImageIndex(hParent)) {
		m_Tree.ExpandItem(hParent, true);
		m_pDoc->SetChanged(1); //no prompt to save just for this
	}
	m_Tree.EnsureVisible(hItem);
	m_Tree.SelectItem(hItem);
}

void CPageLayers::InitColWidths()
{
	CClientDC dc(this);
	int nSave = dc.SaveDC();
	dc.SelectObject(GetFont());

	CLayerSet &lset = m_pDoc->LayerSet();
	CPropStatus &prop = lset.m_PropStatus;

	prop.nColWidth[0] = 200;
	prop.nColWidth[2] = 800;
	int maxw = 40;

	for (it_Layer it = lset.BeginLayerIt(); it != lset.EndLayerIt(); it++) {
		PML pml = (PML)*it;
		if (pml->IsFolder()) continue;
		LPCSTR p = pml->SizeStr();
		int nLength = dc.GetTextExtent(p, strlen(p)).cx + 12; //allow for margin
		if (nLength > maxw) maxw = nLength;
	}
	prop.nColWidth[1] = maxw;

	dc.RestoreDC(nSave);
}


struct COLSTR2
{
	COLSTR2(LPCSTR p0 = "", LPCSTR p1 = "")
	{
		p[0] = p0; p[1] = p1;
	}
	operator LPCSTR *() { return &p[0]; }
	LPCSTR p[2];
};

void CPageLayers::UpdateTree()
{
	CLayerSet &lset = m_pDoc->LayerSet();
	m_Tree.SetRedraw(0);
	m_Tree.m_bDestroyingTree = 1;
	//m_Tree.SelectItem(NULL);
	m_Tree.DeleteAllItems();
	rit_Layer itend = lset.EndLayerRit();
	HTREEITEM hParent = TVI_ROOT;
	UINT uLevel = 0;
	for (rit_Layer it = lset.BeginLayerRit(); it != itend; ++it) {
		PML pml = (PML)*it;
		LPCSTR pTitle = pml->Title();
		CString s("| ");
		if (pml->LayerType() == TYP_SHP && !((CShpLayer *)pml)->IsEditable()) {
			s += pTitle;
			pTitle = s;
		}
		UINT nState = pml->IsVisible() ? CHECKED : (pml->IsTristate() ? TRISTATE : UNCHECKED);
		pml->m_hItem = m_LayerTree.InsertItem(pml, pTitle, pml->m_nImage, nState,
			pml->IsFolder() ? COLSTR2() : COLSTR2(pml->SizeStr(), pml->PathName()),
			pml->parent ? pml->parent->m_hItem : TVI_ROOT, TVI_LAST);
		if (pml->m_nImage == 1) {
			//m_Tree.Expand(pml->m_hItem,TVE_EXPAND); //fails
			m_Tree.SetItemState(pml->m_hItem, TVIS_EXPANDED, TVIS_EXPANDED); //works
		}

		ASSERT(V_F(pml->m_uFlags) != V_FLG);

		pml->m_uLevel = uLevel++;
	}

	m_Tree.m_bDestroyingTree = 0;
	m_Tree.SetRedraw(1);
	m_Tree.UpdateWindow();
}

void CPageLayers::UpdateLayerData(CWallsMapDoc *pDoc)
{
	if (pDoc != m_pDoc) {
		m_pDoc = pDoc;
		m_Tree.m_bDestroyingTree = 1;
		CLayerSet &lset = m_pDoc->LayerSet();
		m_LayerTree.ClearTree();
		CPropStatus &prop = lset.m_PropStatus;
		if (!prop.nColWidth[0]) InitColWidths();
		m_LayerTree.InsertColumn(0, "Title", LVCFMT_LEFT, prop.nColWidth[0]);
		m_LayerTree.InsertColumn(1, "Size", LVCFMT_LEFT, prop.nColWidth[1]);
		m_LayerTree.InsertColumn(2, "Path", LVCFMT_LEFT, prop.nColWidth[2]);
		prop.nTab = 0;
		m_LayerTree.UpdateColumns();
		m_Tree.SetImageList(lset.m_pImageList, TVSIL_NORMAL);
		m_Tree.m_bDestroyingTree = 0;
	}

	UpdateTree();
	SelectDocTreePosition();
#ifdef _DEBUG
	int e = CheckTree();
	ASSERT(!e);
#endif
	if (CLayerSet::m_bAddingNtiLayer)
		UpdateButtons();
}

bool CPageLayers::SaveColumnWidths()
{
	bool bRet = false;
	CPropStatus &prop = m_pDoc->LayerSet().m_PropStatus;
	for (int i = 0; i < 3; i++) {
		int w = m_LayerTree.GetColumnWidth(i);
		if (prop.nColWidth[i] != w && (bRet = true))
			prop.nColWidth[i] = w;
	}
	return bRet;
}

LRESULT CPageLayers::OnSelChange(WPARAM item, LPARAM pL)
{
	PTL ptl = (PTL)pL;
	ASSERT((HTREEITEM)item == ptl->m_hItem);
	m_pDoc->SetSelectedLayerPos(ptl->m_uLevel);
	Sheet()->SetNewLayer(); //causes details page to refresh when selected
	UpdateButtons();
	if (m_pDoc->m_bDrawOutlines > 1) Sheet()->RepaintViews();
	return 0;  //ignored
}

LRESULT CPageLayers::OnDblClickItem(WPARAM item, LPARAM p)
{
	ASSERT(_H(item) == m_Tree.GetSelectedItem());
	if (((PTL)p)->IsFolder()) {
		((PTL)p)->m_nImage = m_Tree.IsExpanded(_H(item)) ? 1 : 0;
		m_pDoc->SetChanged(1); //no prompt to save upon close, will preserve state when project saved for another reason
	}
	else
		OnBnClickedSymbology();

	return 0; //ignored
}

LRESULT CPageLayers::OnRClickItem(WPARAM item, LPARAM point)
{
	PTL ptl = (PTL)m_Tree.GetItemData(_H(item));
	//CMsgBox("Right-clicked on: %s",ptl->Title());
	//Open menu!

	bool bFolder = ptl->IsFolder();
	CMenu menu;
	if (menu.LoadMenu(bFolder ? IDR_FOLDER_CONTEXT : IDR_LYR_CONTEXT))
	{
		int nInsertFolderPos = bFolder ? 0 : 1;
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		if (!m_Tree.GetNextSiblingItem(_H(item))) {
			pPopup->DeleteMenu(ID_MOVELAYERTOBOTTOM, MF_BYCOMMAND);
			//nInsertFolderPos--;
		}
		if (!m_Tree.GetPrevSiblingItem(_H(item))) {
			pPopup->DeleteMenu(ID_MOVELAYERTOTOP, MF_BYCOMMAND);
			//nInsertFolderPos--;
		}

		if (!m_pDoc->IsTransformed())
			pPopup->DeleteMenu(nInsertFolderPos + 1, MF_BYPOSITION);

		if (m_pDoc->HasOneMapLayer()) {
			pPopup->EnableMenuItem(ID_CLOSE_LAYER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			pPopup->EnableMenuItem(ID_CLOSE_AND_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}
		else {
			//Also can't insert new folder if this would exceed maximum depth --
			int nDepth = m_Tree.GetNumAncestors(_H(item)); //ancestors of clicked-on item
			if (nDepth >= MAX_TREEDEPTH) {
				ASSERT(nDepth == MAX_TREEDEPTH && !bFolder);
				//can't add a folder at this level --
				pPopup->DeleteMenu(nInsertFolderPos, MF_BYPOSITION);
			}
			else if (bFolder && nDepth == MAX_TREEDEPTH - 1) {
				//clicked on folder is already at its maximum depth.
				//can't add new folder as its child or parent, so delete these pop-out menu options --
				pPopup->DeleteMenu(ID_FOLDER_CHILD, MF_BYCOMMAND);
				pPopup->DeleteMenu(ID_FOLDER_PARENT, MF_BYCOMMAND);
			}
		}

		{
			//allow pasting as a child or not at top? not for now
			COleDataObject odj;
			if (!odj.AttachClipboard() || !odj.IsDataAvailable(CF_HDROP))
				pPopup->EnableMenuItem(ID_EDIT_PASTE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}

		if (ptl->LayerType() != TYP_SHP) {
			//clicked on item is a folder or image layer
			pPopup->DeleteMenu(ID_OPEN_TABLE, MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_SET_EDITABLE, MF_BYCOMMAND);
			pPopup->DeleteMenu(ID_CLEAR_EDITFLAGS, MF_BYCOMMAND);
			if (ptl->LayerType() != TYP_NTI)
				pPopup->DeleteMenu(ID_CLOSE_AND_DELETE, MF_BYCOMMAND);
		}
		else {
			CShpLayer *pShp = (CShpLayer *)ptl;
			ASSERT(!pShp->IsEditable() || pShp->m_pdbfile->pShpEditor == pShp);

			if (!pShp->m_nNumRecs) {
				pPopup->EnableMenuItem(ID_CLEAR_EDITFLAGS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				pPopup->EnableMenuItem(ID_OPEN_TABLE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			}

			pPopup->CheckMenuItem(ID_SET_EDITABLE, pShp->IsEditable() ? (MF_CHECKED | MF_BYCOMMAND) : (MF_UNCHECKED | MF_BYCOMMAND));

			if (pShp->HasOpenRecord() || pShp->HasEditedMemo() ||
				(!pShp->IsEditable() && pShp->m_pdbfile->pShpEditor)) //open for edits in another project?
			{
				pPopup->EnableMenuItem(ID_CLEAR_EDITFLAGS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				pPopup->EnableMenuItem(ID_SET_EDITABLE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			}

			if (!pShp->IsEditable() || pShp->ShpTypeOrg() != CShpLayer::SHP_POINT)
				pPopup->DeleteMenu(ID_CLEAR_EDITFLAGS, MF_BYCOMMAND);

			if (pShp->HasMemos()) {
				VERIFY(pPopup->AppendMenu(MF_STRING, ID_TEST_MEMOS, "Test memo field integrity"));
			}

			if (pShp->KeyFld() && (!pShp->HasLocFlds() || m_pDoc->LayerSet().IsWgs84())) {
				VERIFY(pPopup->AppendMenu(MF_STRING, ID_COMPARE_SHP, "Compare with other layers..."));
			}

			if (pShp->m_pdbfile->bHaveTmpshp) { //HasXFlds()) {
				CString s;
				s.Format("Open %s", pShp->FileName());
				s.Truncate(s.ReverseFind('.'));
				s += SHP_EXT_TMPSHP;
				VERIFY(pPopup->AppendMenu(MF_STRING, ID_APPENDLAYER_N, (LPCSTR)s));
			}
			if (pShp->m_pdbfile->nUsers > 1 || m_pDoc->HasOneMapLayer())
				pPopup->EnableMenuItem(ID_CLOSE_AND_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		}
		CPoint pt(point);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
			pt.x, pt.y, this);
	}

	return 0;  //ignored
}

void CPageLayers::UpdateButtons()
{
	PTL pLayer = m_pDoc->SelectedLayerPtr();
	CLayerSet &lset = m_pDoc->LayerSet();
	Enable(IDC_LAYER_DEL, pLayer->IsFolder() || lset.InitPML() && lset.NextPML());
	Enable(IDC_LAYER_ADD, m_pDoc->IsTransformed());
	int typ = pLayer->LayerType();
	if (typ != TYP_FOLDER) {
		Enable(IDC_LAYER_CONV, 1);
		SetText(IDC_LAYER_CONV, (typ == TYP_GDAL) ? "Convert..." : "Export...");
		SetText(IDC_LAYER_DEL, "Close Layer");
		Enable(IDC_SCALERANGE, m_pDoc->IsTransformed());
		Enable(IDC_ATTRIBUTES, typ == TYP_SHP && ((CShpLayer *)pLayer)->m_nNumRecs > 0);
		Enable(IDC_SYMBOLOGY, 1);
		SetText(IDC_SYMBOLOGY, (typ == TYP_SHP) ? "Symbols..." : "Opacity");
		if (typ != TYP_SHP && CImageLayer::m_pDlgSymbolsImg) {
			if (CImageLayer::m_pDlgSymbolsImg->m_pLayer != (CImageLayer *)pLayer) {
				CImageLayer::m_pDlgSymbolsImg->NewLayer((CImageLayer *)pLayer);
				SetFocus();
			}
		}
	}
	else {
		SetText(IDC_LAYER_DEL, pLayer->child ? "Ungroup" : "Remove");
		Enable(IDC_LAYER_CONV, 0);
		Enable(IDC_SCALERANGE, (PML)pLayer->FirstChildPML() != 0);
		Enable(IDC_ATTRIBUTES, 0);
		Enable(IDC_SYMBOLOGY, 0);
	}
}

void CPageLayers::RefreshListItem(PTL ptl)
{
	CString text(ptl->Title());
	if (ptl->IsFolder()) text += "\t\t";
	else {
		if (ptl->LayerType() == TYP_SHP && !((PML)ptl)->IsEditable())
			text.Insert(0, "| ");
		text += '\t'; text += ((PML)ptl)->SizeStr();
		text += '\t'; text += ((PML)ptl)->PathName();
	}
	m_Tree.SetItemText(ptl->m_hItem, text);
	CRect rect;
	m_Tree.GetItemRect(ptl->m_hItem, &rect, FALSE);
	m_Tree.InvalidateRect(&rect, FALSE);
}

#ifdef _DEBUG
BOOL CPageLayers::CheckTree()
{
	CLayerSet &lset = m_pDoc->LayerSet();
	UINT pos = 0;
	for (rit_Layer it = lset.BeginLayerRit(); it != lset.EndLayerRit(); it++, pos++)
	{
		PTL p, ptl = *it;
		HTREEITEM h = ptl->m_hItem;

		if (!h || GetPTL(h) != ptl || pos != ptl->m_uLevel)
			return 1;

		if (p = ptl->nextsib) {
			if (m_Tree.GetNextSiblingItem(h) != p->m_hItem)
				return 2;
			if (p->prevsib != ptl || p->parent != ptl->parent)
				return 3;
		}
		else if (m_Tree.GetNextSiblingItem(h))
			return 4;

		if (p = ptl->child) {
			if (ptl->m_nImage > 1) return 5;
			if (m_Tree.GetChildItem(h) != p->m_hItem)
				return 6;
			if (p->prevsib || m_Tree.GetPrevSiblingItem(p->m_hItem))
				return 7;
		}
		else if (m_Tree.GetChildItem(h))
			return 8;

		if (p = ptl->prevsib) {
			if (m_Tree.GetPrevSiblingItem(h) != p->m_hItem)
				return 9;
			if (p->nextsib != ptl || p->parent != ptl->parent)
				return 10;
		}
		else {
			if (m_Tree.GetPrevSiblingItem(h))
				return 11;
			if (p = ptl->parent) {
				if (p->child != ptl || m_Tree.GetChildItem(p->m_hItem) != ptl->m_hItem)
					return 12;
			}
		}

		if (p = ptl->parent) {
			if (m_Tree.GetParentItem(h) != p->m_hItem)
				return 13;
		}
		else if (m_Tree.GetParentItem(h))
			return 14;
	}
	if (pos != lset.NumLayers())
		return 15;
	return 0;
}
#endif

void CPageLayers::RedrawList()
{
	UpdateTree();
	SelectDocTreePosition();
#ifdef _DEBUG
	int e = CheckTree();
	ASSERT(!e);
#endif
	UpdateButtons();
}

static bool bFailedToClose;

void CPageLayers::OnBnClickedLayerDel()
{
	if (GetFocus() == GetDlgItem(IDC_LAYER_DEL)) {
		AdvDlgCtrl(7); //advance focus to tree window, IDC_COLUMNTREE
	}
	PTL ptltop = GetPTL(m_Tree.GetFirstVisibleItem());
	PTL pLayer = m_pDoc->SelectedLayerPtr();

	ASSERT(pLayer);
	bFailedToClose = true;

	bool bNewVisibility = !pLayer->IsFolder() && pLayer->IsVisible();
	if (bNewVisibility) {
		if (pLayer->parent) {
			//May need to set parent checkboxes to tristate --
			m_Tree.SetCheck(pLayer->m_hItem, false);
			bNewVisibility = false;
		}
	}
	else if (pLayer->IsFolder() && !V_PF(pLayer)) {
		//folder checkbox unchecked -- let's try checking it before deleting it.
		//This will revise, if necessary, tree checkboxes and update map
		//to allow folder removal with no such updates necessary.
		m_Tree.SetCheck(pLayer->m_hItem);
	}

	if (pLayer->LayerType() == TYP_SHP && ((PML)pLayer)->IsEditable()) { //Flag was incorrectly set for some image files!

		//Also check if any of those items are selected
		if (app_pShowDlg && m_pDoc == app_pShowDlg->m_pDoc && app_pShowDlg->IsLayerSelected((CShpLayer *)pLayer)) {
			CMsgBox("Layer %s can't be removed while some of its points are selected.", pLayer->Title());
			return;
		}

		//Check if layer can be deleted by descarding or saving changes, otherwise return;
		if (app_pDbtEditDlg && app_pDbtEditDlg->m_pShp == (CShpLayer *)pLayer) {
			if (app_pDbtEditDlg->DenyClose())
				return;
			app_pDbtEditDlg->Destroy();
		}

		if (app_pImageDlg && (CShpLayer *)pLayer == app_pImageDlg->m_pShp) {
			app_pImageDlg->Destroy();
		}

		if (((CShpLayer *)pLayer)->m_bEditFlags) {
			((CShpLayer *)pLayer)->SaveShp();
		}
		//CWallsMapView::SyncStop(); --why?
	}
	m_pDoc->DeleteLayer();
	UpdateTree();
	if (pLayer != ptltop)
		m_Tree.SelectSetFirstVisible(ptltop->m_hItem);
	SelectDocTreePosition();
	m_Tree.SetFocus();
#ifdef _DEBUG
	int e = CheckTree();
	ASSERT(!e);
#endif
	UpdateButtons();
	bFailedToClose = false;
	if (bNewVisibility)
		Sheet()->RefreshViews();
}

void CPageLayers::OnCloseAndDelete()
{
	CString path;
	CTreeLayer *ptl = m_pDoc->SelectedLayerPtr();
	if (ptl->LayerType() == TYP_NTI) {
		path = ((CNtiLayer *)ptl)->PathName();
		bool bHasNte = ((CNtiLayer *)ptl)->HasNTERecord();
		CString msg;
		msg.Format("CAUTION:\n\nAre you sure you want to delete %s after "
			"it's removed as a layer?", trx_Stpnam(path));
		if (bHasNte) msg += "\n(The .nte file component will also be deleted.)";
		if (MsgOkCancel(m_hWnd, msg, "Confirm Deletion of Disk Files")) {
			OnBnClickedLayerDel();
			if (_unlink(path))
				AfxMessageBox("The file is read-only or in use by another process. Although the layer was closed, no files were deleted.");
			else if (bHasNte) {
				VERIFY(!_unlink(ReplacePathExt(path, ".nte")));
			}
		}
	}
	else {
		CShpLayer *pShp = (CShpLayer *)ptl;
		ASSERT(pShp->m_pdbfile->nUsers == 1);

		path.Format("CAUTION:\n\nAre you sure you want to delete the file components of %s after "
			"it's removed as a layer?\n(Components with extensions .shp, .shx, .dbf, .dbt, .prj, "
			".dbe, .tmpshp, .qpj, .sbn, and .sbx will be deleted if they exist.)", trx_Stpnam(pShp->PathName()));

		if (!MsgOkCancel(m_hWnd, path, "Confirm Deletion of Disk Files"))
			return;

		path = pShp->PathName(); //save path
		OnBnClickedLayerDel();
		if (!bFailedToClose) {
			//First try DBF --
			path.Truncate(path.GetLength() - 3);
			path += "dbf";
			if (_unlink(path))
				AfxMessageBox("The shapefile is read-only or in use by another process. Although the layer was closed, no files were deleted.");
			else {
				CShpLayer::DeleteComponents(path);
			}
		}
	}
}

void CPageLayers::OnMoveLayerToTop()
{
	PTL pLayer = m_pDoc->SelectedLayerPtr();
	PTL pTopLayer = pLayer;
	while (pTopLayer->prevsib) pTopLayer = pTopLayer->prevsib;
	if (pLayer != pTopLayer)
		m_Tree.MoveItem(pLayer->m_hItem, pTopLayer->m_hItem, false);
}

void CPageLayers::OnMoveLayerToBottom()
{
	PTL pLayer = m_pDoc->SelectedLayerPtr();
	PTL pBotLayer = pLayer;
	while (pBotLayer->nextsib) pBotLayer = pBotLayer->nextsib;
	if (pLayer != pBotLayer) {
		HTREEITEM hItem = pBotLayer->m_hItem;
		bool bExp = true;
		if (pBotLayer->m_nImage == 1 && pBotLayer->child) {
			m_Tree.ExpandItem(hItem, bExp = false);
		}
		m_Tree.MoveItem(pLayer->m_hItem, hItem, true);
		if (!bExp) m_Tree.ExpandItem(hItem, true);
	}
}

void CPageLayers::OnZoomtolayer()
{
	PTL pLayer = m_pDoc->SelectedLayerPtr();
	ASSERT(!pLayer->IsFolder()); //for now
	if (pLayer->LayerType() == TYP_SHP && ((CShpLayer *)pLayer)->m_nNumRecs == ((CShpLayer *)pLayer)->m_uDelCount) {
		CMsgBox("Layer %s is currently empty.", pLayer->Title());
		return;
	}
	Sheet()->PostMessage(WM_PROPVIEWDOC, (WPARAM)m_pDoc, (LPARAM)LHINT_FITIMAGE);
}

void CPageLayers::OnBnClickedSymbology()
{
	if (GetFocus() == GetDlgItem(IDC_SYMBOLOGY)) AdvDlgCtrl(4);
	CShpLayer *pLayer = (CShpLayer *)m_pDoc->SelectedLayerPtr();

	if (pLayer->LayerType() == TYP_SHP) {
		if (pLayer->ShpType() == CShpLayer::SHP_POINT) {
			CDlgSymbols dlg(m_pDoc, pLayer);
			dlg.DoModal();
		}
		else {
			CDlgSymbolsPoly dlg(m_pDoc, pLayer);
			dlg.DoModal();
		}
	}
	else {
		CImageLayer *pImg = (CImageLayer *)pLayer;
		CDlgSymbolsImg *pDlg = CImageLayer::m_pDlgSymbolsImg;
		if (pDlg) {
			ASSERT(pDlg->m_pLayer == pImg);
			pDlg->BringWindowToTop();
		}
		else {
			pDlg = new CDlgSymbolsImg(pImg, theApp.m_pMainWnd);
			ASSERT(CImageLayer::m_pDlgSymbolsImg);
		}
	}
}

int CPageLayers::EditTitle(CString &title)
{
	CTitleDlg dlg(title);
	if (IDOK == dlg.DoModal()) {
		if (title.Compare(dlg.m_title)) {
			title = dlg.m_title;
			return 1;
		}
		return 0;
	}
	return -1;
}

void CPageLayers::OnEditTitle()
{
	PTL ptl = m_pDoc->SelectedLayerPtr();
	int n = EditTitle(ptl->m_csTitle);
	if (n > 0) {
		m_pDoc->SetChanged();
		RefreshListItem(ptl);
		if (ptl->LayerType() == TYP_SHP)
			((CShpLayer *)ptl)->UpdateTitleDisplay();
	}
}

void CPageLayers::OnClearEditflags()
{
	PTL ptl = m_pDoc->SelectedLayerPtr();
	ASSERT(ptl->LayerType() == TYP_SHP);
	if (IDOK == CMsgBox(MB_OKCANCEL,
		"Select OK to clear the edited status flags (.DBE file component) of %s. "
		"These flags indicate which features have been changed or added since the shapefile "
		"was first accessed by WallsMap, or since the last clearing operation.", ptl->Title()))
	{
		((CShpLayer *)ptl)->ClearDBE();
		if (app_pShowDlg)
			app_pShowDlg->InvalidateTree();
	}
}

void CPageLayers::OnSetEditable()
{
	//Toggle editable state
	CShpLayer *pShp = (CShpLayer *)m_pDoc->SelectedLayerPtr();
	ASSERT(pShp->LayerType() == TYP_SHP);
	if (pShp->SetEditable()) {
		RefreshListItem(pShp);
		m_pDoc->SetChanged();
	}
}

void CPageLayers::OnBnClickedLayerConv()
{
	if (GetFocus() == GetDlgItem(IDC_LAYER_CONV)) AdvDlgCtrl(5);
	CWallsMapView::SyncStop();
	//SaveLayerStatus();
	m_pDoc->ExportNTI();
}

void CPageLayers::OnBnClickedScaleRange()
{
	if (GetFocus() == GetDlgItem(IDC_SCALERANGE)) AdvDlgCtrl(3);
	CScaleDlg dlg(m_pDoc);
	dlg.DoModal();
}

void CPageLayers::OnBnClickedOpenTable()
{
	if (GetFocus() == GetDlgItem(IDC_ATTRIBUTES)) AdvDlgCtrl(6);
	((CShpLayer *)m_pDoc->SelectedLayerPtr())->OpenTable(NULL);
}

static void ToggleChildVisibility(PTL ptl, UINT oldf, UINT newf, bool &bVisible)
{
	ptl->m_uFlags &= ~oldf;
	ptl->m_uFlags |= newf;
	if (!ptl->child) {
		if (!bVisible && !ptl->IsFolder() && ((PML)ptl)->Visibility()) {
			bVisible = true;
		}
	}
	for (ptl = ptl->child; ptl; ptl = ptl->nextsib) {
		if (V_PF(ptl) == oldf) {
			ToggleChildVisibility(ptl, oldf, newf, bVisible);
		}
	}
}

LRESULT CPageLayers::OnCheckbox(WPARAM item, LPARAM state)
{
	m_pDoc->SetChanged();

	//nState = CHECKED, UNCHECKED, or TRISTATE (not yet set for item)
	UINT nState = (UINT)state;
	UINT oState = m_Tree.GetCheckedState(_H(item));
	ASSERT(nState != oState);

	PTL ptl = (PTL)m_Tree.GetItemData(_H(item));
	UINT flg = ptl->m_uFlags;

	ASSERT(ptl && ptl->m_hItem == _H(item));

	if (nState == TRISTATE || (nState == UNCHECKED && oState == TRISTATE)) {
		//no effect on flags of other tree items
		if (nState == TRISTATE) {
			ASSERT(oState == UNCHECKED && V_F(flg) == 0);
			ptl->m_uFlags |= NTL_FLG_TRISTATE;
		}
		else {
			ASSERT(V_F(flg) == NTL_FLG_TRISTATE);
			ptl->m_uFlags &= ~NTL_FLG_TRISTATE;
		}
		return 0;
	}

	bool bVisible = false; //set true if refresh needed

	if (nState == CHECKED) {
		//Switch this item to CHECKED and all TRISTATE children to CHECKED recursively.
		ASSERT(oState == UNCHECKED && V_F(flg) == 0);
		ToggleChildVisibility(ptl, NTL_FLG_TRISTATE, NTL_FLG_VISIBLE, bVisible);
		//Parent flag, if TRISTATE due to no children checked, may have changed to CHECKED
	}
	else {
		//Switch this item to UNCHECKED and all CHECKED children to TRISTATE recursively.
		ASSERT(nState == UNCHECKED && oState == CHECKED && V_F(flg) == NTL_FLG_VISIBLE);
		ToggleChildVisibility(ptl, NTL_FLG_VISIBLE, NTL_FLG_TRISTATE, bVisible);
		ASSERT(V_PF(ptl) == NTL_FLG_TRISTATE);
		ptl->m_uFlags &= ~NTL_FLG_TRISTATE;
		//Parent flag changes to TRISTATE if no siblings of ntl are now CHECKED
	}

	//Adjust ancestor flags based on tree item states --
	while (ptl->parent) {
		ptl = ptl->parent;
		UINT nState = m_Tree.GetCheckedState(ptl->m_hItem);
		ptl->m_uFlags &= ~V_FLG;
		if (nState == CHECKED)
			ptl->m_uFlags |= NTL_FLG_VISIBLE;
		else if (nState == TRISTATE)
			ptl->m_uFlags |= NTL_FLG_TRISTATE;
	}

	if (bVisible) Sheet()->RefreshViews();
	return 0;
}

LRESULT CPageLayers::OnMoveItem(WPARAM itemdrop, LPARAM)
{
	// moved a branch -- fix m_layers and refresh map --
	VEC_TREELAYER vec;
	int n = m_pDoc->NumLayers();
	vec.reserve(n);

	//work from top of tree view to fix visibility and links--
	UINT nState = m_Tree.GetCheckedState(_H(itemdrop)), nLevel = 0;
	PTL ptldrop = GetPTL(_H(itemdrop)); //item dropped
	int vispos = 0, dropvispos = -1;

	bool bNewVisibility = ptldrop->IsVisible() != (nState == CHECKED);

	if (!bNewVisibility && nState == CHECKED) {
		ASSERT(ptldrop->IsVisible());
		CLayerSet &lset = m_pDoc->LayerSet();
		for (rit_Layer rit = lset.BeginLayerRit(); rit != lset.EndLayerRit(); rit++) {
			if (*rit == ptldrop) {
				dropvispos = vispos;
				break;
			}
			if ((*rit)->IsVisible()) vispos++;
		}
		vispos = 0;
	}

	//rebuild layerset from top of tree --

	HTREEITEM h0;
	for (HTREEITEM h = m_Tree.GetRootItem(); h; h = m_Tree.GetNextItem(h)) {
		PTL ptl = GetPTL(h);
		vec.push_back(ptl);
		ptl->m_hItem = h;

		//fix m_uLevel and visibility --
		ptl->m_uLevel = nLevel++;
		nState = m_Tree.GetCheckedState(h);
		ptl->m_uFlags &= ~V_FLG;
		if (nState == CHECKED) {
			if (dropvispos >= 0) {
				if (ptl == ptldrop) {
					ASSERT(!bNewVisibility);
					bNewVisibility = dropvispos != vispos; //has position among visible layers changed!
					dropvispos = -1;
				}
				else vispos++;
			}
			ptl->m_uFlags |= NTL_FLG_VISIBLE;
		}
		else if (nState == TRISTATE) ptl->m_uFlags |= NTL_FLG_TRISTATE;

		// fix links --
		h0 = m_Tree.GetParentItem(h);
		ptl->parent = h0 ? GetPTL(h0) : NULL;
		h0 = m_Tree.GetNextSiblingItem(h);
		ptl->nextsib = h0 ? GetPTL(h0) : NULL;
		h0 = m_Tree.GetPrevSiblingItem(h);
		ptl->prevsib = h0 ? GetPTL(h0) : NULL;
		h0 = m_Tree.GetChildItem(h);
		ptl->child = h0 ? GetPTL(h0) : NULL;
	}
	ASSERT(n == vec.size());
	std::reverse(vec.begin(), vec.end());
	m_pDoc->ReplaceLayerSet(vec);
	m_pDoc->SetChanged();
	m_Tree.SelectItem(_H(itemdrop));
#ifdef _DEBUG
	int e = CheckTree();
	ASSERT(!e);
#endif
	if (bNewVisibility)
		Sheet()->RefreshViews();
	return TRUE;
}

void CPageLayers::OnClose()
{
	m_Tree.m_bDestroyingTree = 1;
	VERIFY(Sheet()->DestroyWindow());
}

void CPageLayers::OnDrop()
{
	int nAdded = 0;
	CNtiLayer::m_bExpanding = FALSE;
	UINT uFlags = NTL_FLG_VISIBLE;
	if (CMainFrame::IsPref(PRF_OPEN_EDITABLE)) uFlags |= NTL_FLG_EDITABLE;

	for (V_FILELIST::iterator it = CFileDropTarget::m_vFiles.begin(); it != CFileDropTarget::m_vFiles.end(); it++) {
		if (m_pDoc->AddLayer(*it, uFlags))
			nAdded++;
	}
	if (nAdded) {
		//CWallsMapView::SyncStop(); //why?
		if (m_Tree.GetCount() == 1) Sheet()->SetPropTitle(); //Pathname may have changed
		UpdateLayerData(m_pDoc);
		m_LayerTree.SetFocus();
		Sheet()->RefreshViews();
	}
}

void CPageLayers::OnEditPaste()
{
	COleDataObject oData;

	if (!oData.AttachClipboard() || !oData.IsDataAvailable(CF_HDROP)) {
		ASSERT(0);
		return;
	}
	if (m_dropTarget.ReadHdropData(&oData)) {
		OnDrop();
		V_FILELIST().swap(CFileDropTarget::m_vFiles);
	}
	else
		CFileDropTarget::FileTypeErrorMsg();
}


void CPageLayers::OnOpenfolder()
{
	PML pLayer = (PML)m_pDoc->SelectedLayerPtr();
	ASSERT(!pLayer->IsFolder());
	if (pLayer) {
		OpenContainingFolder(pLayer->PathName());
	}
}

void CPageLayers::OnOpenXC()
{
	static bool bNopenXCPrompt;
	CShpLayer *pShp = (CShpLayer *)m_pDoc->SelectedLayerPtr();
	ASSERT(pShp->LayerType() == TYP_SHP);
	if (!bNopenXCPrompt) {
		if (MsgCheckDlg(m_hWnd, MB_OK,
			"NOTE: If you make changes to the template file, particularly the\n"
			"expressions \"?(...)\" that control how fields are edited or initialized,\n"
			"you'll need to close and reopen the project before they can take effect.",
			pShp->m_csTitle,
			"Don't display this again during this session.") == 2) bNopenXCPrompt = true;
	}
	CString s(pShp->PathName());
	s.Truncate(s.ReverseFind('.'));
	s += SHP_EXT_TMPSHP;
	OpenFileAsText(s);
}

LRESULT CPageLayers::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(1007);
	return TRUE;
}

void CPageLayers::InsertFolder(int pos)
{
	CString title;
	if (EditTitle(title) < 0) return;
	PTL ptlnew = new CTreeLayer(title);
	ptlnew->m_nImage = 1; //open
	CLayerSet &lset = m_pDoc->LayerSet();
	rit_Layer rit = lset.SelectedLayerRit();
	ASSERT(rit != lset.EndLayerRit());
	PTL ptl = *rit;
	PTL ptltop = GetPTL(m_Tree.GetFirstVisibleItem());
	//fix both tree links and layerset, but no change to map --
	switch (pos) {
	case -2: //Insert as parent --
		ptlnew->parent = ptl->parent;
		if ((ptlnew->prevsib = ptl->prevsib))
			ptl->prevsib->nextsib = ptlnew;
		else if (ptl->parent)
			ptl->parent->child = ptlnew;
		if ((ptlnew->nextsib = ptl->nextsib))
			ptl->nextsib->prevsib = ptlnew;
		ptlnew->child = ptl;
		ptlnew->m_uFlags |= V_PF(ptl);
		ptl->nextsib = ptl->prevsib = 0;
		ptl->parent = ptlnew;
		break;
	case -1: //insert at this position (as prev sibling)
		ptlnew->parent = ptl->parent;
		if ((ptlnew->prevsib = ptl->prevsib)) ptl->prevsib->nextsib = ptlnew;
		else if (ptl->parent) {
			ASSERT(ptl->parent->child == ptl);
			ptl->parent->child = ptlnew;
		}
		ptlnew->nextsib = ptl;
		ptl->prevsib = ptlnew;
		break;
	case  0: //insert as child
		ASSERT(ptl->IsFolder());
		ptl->m_nImage = 1; //open folder
		ptlnew->parent = ptl;
		if ((ptlnew->nextsib = ptl->child)) ptl->child->prevsib = ptlnew;
		ptl->child = ptlnew;
		rit++;
		break;
	case  1: //insert after this position (as next sibling)
		ptlnew->parent = ptl->parent;
		if ((ptlnew->nextsib = ptl->nextsib)) {
			for (; *rit != ptl->nextsib; rit++); //find rit for ptl's next sibling, or point of insertion
			ptl->nextsib->prevsib = ptlnew;
		}
		else {
			rit++;
		}
		ptlnew->prevsib = ptl;
		ptl->nextsib = ptlnew;
		break;
	}
	lset.InsertPTL(rit, ptlnew);
	m_pDoc->SetChanged();
	UpdateTree();
	if (pos >= 0 || ptl != ptltop)
		m_Tree.SelectSetFirstVisible(ptltop->m_hItem);
	m_Tree.SelectItem(ptlnew->m_hItem); //UpdateButtons()?
#ifdef _DEBUG
	int e = CheckTree();
	ASSERT(!e);
#endif
}

void CPageLayers::OnFolderAbove()
{
	InsertFolder(-1);
}


void CPageLayers::OnFolderBelow()
{
	InsertFolder(1);
}


void CPageLayers::OnFolderChild()
{
	InsertFolder(0);
}

void CPageLayers::OnFolderParent()
{
	InsertFolder(-2);
}

void CPageLayers::OnLayersAbove()
{
	//as prev sibling
	m_pDoc->AddFileLayers(true, -1);
}

void CPageLayers::OnLayersBelow()
{
	//as next sibling
	m_pDoc->AddFileLayers(true, 1);
}

void CPageLayers::OnLayersWithin()
{
	//as children
	m_pDoc->AddFileLayers(true, 2);
}

void CPageLayers::OnBnClickedLayerAdd()
{
	if (GetFocus() == GetDlgItem(IDC_LAYER_ADD)) AdvDlgCtrl(8);
	m_pDoc->AddFileLayers(true, -1);
	m_Tree.SetFocus();
}

void CPageLayers::OnCompareShp()
{
	CShpLayer *pShpRef = (CShpLayer *)m_pDoc->SelectedLayerPtr();
	BYTE refKeyFld = pShpRef->KeyFld();
	ASSERT(refKeyFld);

	if (pShpRef->HasPendingEdits()) {
		CMsgBox("Shapefile %s cannot have a record with uncommitted edits when used as a reference. "
			"Please conclude or cancel editing before performing a compare.", pShpRef->Title());
		return;
	}

	if (pShpRef->m_bConverted) {
		/*
		CString csOrg;
		pShpRef->AppendCoordSystemOrg(csOrg);
		if(IDOK!=CMsgBox(MB_OKCANCEL,"%s --\nNOTE: Although a table comparison is possible, the option to create an "
			"update shapefile will be disabled because the original coordinates (%s) have been "
			"transformed to %s to match the view.",pShpRef->Title(),(LPCSTR)csOrg,pShpRef->GetKnownCoordSystemStr())) return;
		*/
		if (IDOK != CMsgBox(MB_OKCANCEL, "%s --\nNOTE: Although a table comparison is possible, the option to create an "
			"update shapefile will be disabled because the original coordinates have been "
			"transformed to match the view.", pShpRef->Title())) return;
	}

	//Initialize vector of available layers
	CLayerSet &lset = m_pDoc->LayerSet();

	CShpLayer *pShpSkipped = NULL;
	VEC_COMPSHP vCompShp;
	LPCSTR pKeyName = pShpRef->m_pdb->FldNamPtr(refKeyFld);
	LPCSTR pfxFldNam = NULL;

	if (pShpRef->m_pKeyGenerator && strstr(pShpRef->m_pKeyGenerator, "::"))
		pfxFldNam = pShpRef->m_pdb->FldNamPtr(pShpRef->m_pdbfile->pfxfld);

	vCompShp.reserve(lset.NumLayers());
	for (PML pml = lset.InitTreePML(); pml; pml = lset.NextTreePML()) {
		CShpLayer *pShp = (CShpLayer *)pml;
		if (pShp->IsVisible() && pShp->LayerType() == TYP_SHP && pShp->ShpType() == CShpLayer::SHP_POINT && pShp != pShpRef) {
			BYTE keyFld = (BYTE)pShp->m_pdb->FldNum(pKeyName);
			if (keyFld && !memcmp(&pShpRef->m_pdb->m_pFld[refKeyFld - 1], &pShp->m_pdb->m_pFld[keyFld - 1], sizeof(DBF_FLDDEF) - 2)) {
				if (pfxFldNam && !pShp->m_pdb->FldNum(pfxFldNam)) {
					continue;
				}
				if (pShp->HasPendingEdits()) pShpSkipped = pShp;
				else vCompShp.push_back(COMPSHP((CShpLayer *)pml, 0));
			}
		}
	}

	CString s;

	if (!vCompShp.size()) {
		if (pShpSkipped) s.Format(" Note that %s was skipped due to having pending edits.", pShpSkipped->Title());
		CMsgBox("For comparing against %s, the project has no visible shapefiles with the same key field.%s",
			pShpRef->Title(), (LPCSTR)s);
		return;
	}
	else if (pShpSkipped) {
		if (IDOK != CMsgBox(MB_OKCANCEL, "CAUTION: Shapefile %s can't be compared against %s due to having pending edits. "
			"Select OK to choose from among those (%u) that qualify.", pShpSkipped->LayerPath(s), pShpRef->Title(), vCompShp.size()))
			return;
	}

	CString pathName;
	WORD *pwFld = NULL;

	CCompareDlg dlg(pShpRef, pathName, s, vCompShp, &pwFld);

	//s contains scan summary and pwFld stores matching field pointers for files in vCompShp.
	//pointers to array pwFld are contained in c_Trxlinks reords.

	if (IDOK == dlg.DoModal() && dlg.m_iFlags >= 0) {
		//pShpRef->CompareShp() ran successfully --
		int flags = dlg.m_iFlags;
		int ir = CMsgBox(flags ? (MB_ICONINFORMATION | MB_YESNO) : MB_ICONINFORMATION, (LPCSTR)s);

		if (flags) {
			//At least a log was generated --
			if (flags & 3) {
				//Summary prompted for proceeding with zip creation --
				if (ir == IDYES) {
					//Open ZIP progress Dialog --
					CUpdZipDlg zdlg(pShpRef, pathName, flags); //will offer to open log at end
					ir = zdlg.DoModal(); //returns IDOK for log display
					if (zdlg.m_bAddShp) {
						//True only if shapefile successfully written! ir can be either IDOK or IDCANCEL to determine log file display.
						ReplacePathExt(pathName, ".shp");
						if (m_pDoc->AddLayer(pathName, NTL_FLG_VISIBLE | NTL_FLG_EDITABLE)) { //editable layer
							CWallsMapView::SyncStop();
							Sheet()->SendMessage(WM_PROPVIEWDOC, (WPARAM)m_pDoc, (LPARAM)LHINT_NEWLAYER);
							Sheet()->RefreshViews();
						}
					}
				}
				else {
					//No to zip creation, so prompt to open log --
					if (IDYES == CMsgBox(MB_ICONQUESTION | MB_YESNO, "Would you like to view the generated log?"))
						ir = IDOK;
				}
			}
			else {
				//Summary prompted to open log --
				if (ir == IDYES) ir = IDOK;
			}

			if (ir == IDOK) {
				ReplacePathExt(pathName, ".txt");
				OpenFileAsText(pathName);
			}
		}
	}

	free(pwFld); pwFld = NULL;
	c_Trxkey.CloseDel();
	c_Trxlinks.CloseDel();
	c_Trxref.CloseDel();
	c_Trx.CloseDel();
}

void CPageLayers::OnTestMemos()
{
	CShpLayer *pShp = (CShpLayer *)m_pDoc->SelectedLayerPtr();
	ASSERT(pShp->LayerType() == TYP_SHP && pShp->HasMemos());
	pShp->TestMemos();
}
