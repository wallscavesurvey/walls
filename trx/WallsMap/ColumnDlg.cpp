// ColumnDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ColumnDlg.h"

// CColumnDlg dialog

IMPLEMENT_DYNAMIC(CColumnDlg, CResizeDlg)

CColumnDlg::CColumnDlg(CDBField *pFld,int nFlds,CWnd* pParent /*=NULL*/)
	: m_pFld(pFld)
	, m_nFlds(nFlds)
	, CResizeDlg(CColumnDlg::IDD, pParent)
{
}

CColumnDlg::~CColumnDlg()
{
}

void CColumnDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTCOLS, m_List);
	if(pDX->m_bSaveAndValidate) {
		int nCount = m_List.GetItemCount();
		CDBField *pFld=m_pFld;
		for(int nItem = 0; nItem < nCount; nItem++,pFld++)
		{
			pFld->bVis=ListView_GetCheckState(m_List.m_hWnd, nItem)!=0;
			pFld->wFld=(WORD)m_List.GetItemData(nItem);
		}
	}
}


BEGIN_MESSAGE_MAP(CColumnDlg, CResizeDlg)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTCOLS, OnItemChanged)
	ON_REGISTERED_MESSAGE(urm_MOVE_ITEM, OnMoveItem)
	ON_BN_CLICKED(IDC_RESETROWS, &CColumnDlg::OnBnClickedResetrows)
	ON_MESSAGE(WM_COMMANDHELP,OnCommandHelp)
END_MESSAGE_MAP()


// CColumnDlg message handlers

BOOL CColumnDlg::OnInitDialog()
{
	CResizeDlg::OnInitDialog();

	m_List.SetExtendedStyle(
		m_List.GetExtendedStyle() |
		LVS_EX_CHECKBOXES |
		LVS_EX_FULLROWSELECT |
		LVS_EX_GRIDLINES
		// neither work: LVS_EX_UNDERLINEHOT or LVS_EX_ONECLICKACTIVATE
	);

	AllowSizing(CST_NONE,CST_RESIZE);
	AddControl(IDC_LISTCOLS, CST_NONE, CST_NONE, CST_RESIZE, CST_RESIZE, 0);
	AddControl(IDC_RESETROWS, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDCANCEL, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDOK, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);
	AddControl(IDC_ST_DRAGNOTE, CST_NONE, CST_NONE, CST_REPOS, CST_REPOS, 0);

	//VERIFY(0==m_List.InsertColumn(0, "Include all", LVCFMT_LEFT | LVCFMT_FIXED_WIDTH, 147));
	//VERIFY(1==m_List.InsertColumn(1, "",LVCFMT_LEFT, 100));

	VERIFY(0==m_List.InsertColumn(0, "Name", LVCFMT_LEFT /*| LVCFMT_FIXED_WIDTH*/, 98));
	VERIFY(1==m_List.InsertColumn(1, "Typ",LVCFMT_CENTER /*| LVCFMT_FIXED_WIDTH*/, 30));
	VERIFY(2==m_List.InsertColumn(2, "Len",LVCFMT_LEFT   /*| LVCFMT_FIXED_WIDTH*/, 42));

	//Set up header checkbox
	m_List.InitHdrCheck();

	//m_List.GetHeaderCtrl()->EnableWindow(0);

	{
		//Fill listbox --
		CDBField *pFld=m_pFld;
		CString csLen,csDec;
		char cTyp[2]={0};
		for(int i=0;i<m_nFlds;i++,pFld++) {
			VERIFY(i==m_List.InsertItem(i,pFld->pInfo->pnam,0));
			*cTyp=pFld->pInfo->ftyp;
			VERIFY(m_List.SetItem(i,1,LVIF_TEXT,cTyp,0,0,0,0));
			csLen.Format("%u",pFld->pInfo->flen);
			if((*cTyp=='N' || *cTyp=='F') && pFld->pInfo->fdec) {
				csDec.Format(".%u",pFld->pInfo->fdec);
				csLen+=csDec;
			}
			VERIFY(m_List.SetItem(i,2,LVIF_TEXT,csLen,0,0,0,0));
			VERIFY(m_List.SetItemData(i,(DWORD_PTR)pFld->wFld));
			ListView_SetCheckState(m_List.m_hWnd,i,pFld->bVis);
		}
	}

	{
		//Set resizing range based on number of fields --
		int count=m_List.GetCountPerPage();
		CRect rect;
		m_List.GetItemRect(0,&rect,LVIR_BOUNDS);
		int htCell=rect.Height();
		if(count>m_nFlds) {
			int w=(count-m_nFlds)*htCell; //shorten by this amount
			GetWindowRect(&rect);
			rect.bottom-=w;
			m_yMax=(m_yMin-=w);
			Reposition(rect);
			count=m_nFlds;
		}
		else {
			m_yMax=m_yMin+(m_nFlds-count)*htCell;
		}
		if(count>2) m_yMin-=(count-2)*htCell;
	}

	m_List.SetItemState(0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);  //required!
	m_List.SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
}

void CColumnDlg::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLISTVIEW* pNMLV = (NMLISTVIEW*)pNMHDR;
	*pResult = 0;

	if ( m_List.m_bHdrInit && LVIF_STATE == pNMLV->uChanged)
	{
		bool blAllChecked = true;
		int nCount = m_List.GetItemCount();
		for(int nItem = 0; nItem < nCount; nItem++)
		{
			if ( !ListView_GetCheckState(m_List.m_hWnd, nItem) )
			{
				blAllChecked = false;
				break;
			}
		}
		
		HDITEM hdItem;
		hdItem.mask = HDI_IMAGE;
		if (blAllChecked)
			hdItem.iImage = 2;
		else
			hdItem.iImage = 1;
		VERIFY(m_List.m_checkHeadCtrl.SetItem(0, &hdItem) );
	}
}

LRESULT CColumnDlg::OnMoveItem(WPARAM iDragItem, LPARAM iDropItem)
{
	if(iDropItem>(LPARAM)iDragItem) iDropItem--; 
	//iDragIndex=old index, iDropItem=new index after deletion and reinsertion
	ASSERT((int)iDragItem!=(int)iDropItem);
	int i=(int)iDragItem;
	ASSERT(i>=0 && i<m_List.GetItemCount());
	CString s0(m_List.GetItemText(i,0));
	CString s1(m_List.GetItemText(i,1));
	CString s2(m_List.GetItemText(i,2));
	BOOL bChk=ListView_GetCheckState(m_List.m_hWnd,i);
	DWORD_PTR data=m_List.GetItemData(i);
	VERIFY(m_List.DeleteItem(i));
	i=(int)iDropItem;
	ASSERT(i>=0 && i<=m_List.GetItemCount());
	VERIFY(i==m_List.InsertItem(i,s0,0));
	VERIFY(m_List.SetItemData(i,data));
	VERIFY(m_List.SetItemText(i,1,s1));
	VERIFY(m_List.SetItemText(i,2,s2));
	ListView_SetCheckState(m_List.m_hWnd,i,bChk);
	m_List.SetItemState(i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED); //required!
    m_List.EnsureVisible(iDropItem,FALSE); //partial not OK
	return TRUE;
}


void CColumnDlg::OnBnClickedResetrows()
{
	//Insertion sort --
	char sTop[16],sBot[16];
	for(int iTop=1;iTop<m_nFlds;iTop++) {
		int nfTop=(int)m_List.GetItemData(iTop-1);
		if(nfTop==iTop) continue;
		int nfBot,iBot=iTop+1;
		for(;iBot<=m_nFlds;iBot++) {
			nfBot=(int)m_List.GetItemData(iBot-1);
			if(nfBot==iTop) break;
		}
		ASSERT(iBot<=m_nFlds);
		VERIFY(m_List.SetItemData(iTop-1,(DWORD_PTR)nfBot));
		VERIFY(m_List.SetItemData(iBot-1,(DWORD_PTR)nfTop));
		BOOL bChkTop=ListView_GetCheckState(m_List.m_hWnd,iTop-1);
		BOOL bChkBot=ListView_GetCheckState(m_List.m_hWnd,iBot-1);
		ListView_SetCheckState(m_List.m_hWnd,iTop-1,bChkBot);
		ListView_SetCheckState(m_List.m_hWnd,iBot-1,bChkTop);
		for(nfTop=0;nfTop<3;nfTop++) {
			m_List.GetItemText(iBot-1,nfTop,sBot,16);
			m_List.GetItemText(iTop-1,nfTop,sTop,16);
			VERIFY(m_List.SetItemText(iBot-1,nfTop,sTop));
			VERIFY(m_List.SetItemText(iTop-1,nfTop,sBot));
		}
	}
}
LRESULT CColumnDlg::OnCommandHelp(WPARAM wNone, LPARAM lParam)
{
	AfxGetApp()->WinHelp(0);
	return TRUE;
}
