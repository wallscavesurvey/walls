#ifndef _DBGRIDDLG_H
#define _DBGRIDDLG_H 

// dbfgridDlg.h : header file
//

#pragma once
#include "resource.h"
//#include "afxcmn.h"
#include "ResizeDlg.h"
#include "QuickList.h"
#include "FileDropTarget.h"

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif

#include <trxfile.h>

struct BADLINK
{
	BADLINK(LPCSTR pMemo,LPCSTR pLink,UINT szLink) : sBrok(pLink,szLink),nBrokOff(pLink-pMemo) {}
	CString sBrok;
	CString sRepl;
	UINT nBrokOff;
};

typedef std::vector<BADLINK> VEC_BADLINK;

struct LINK_PARAMS {
	DWORD nRec;
	UINT nRow;
	UINT nFld;
	UINT nLinkPos;
	UINT nNew;
	UINT vBLszMemo;
	VEC_BADLINK vBL;
};

class CFindDlgTbl;
class CCopySelectedDlg;

class CDBGridDlg : public CResizeDlg
{
// Construction
public:
	CDBGridDlg(CShpLayer *pShp,VEC_DWORD *pvRec,CWnd* pParent = NULL);	// standard constructor

	~CDBGridDlg();

	static CTRXFile m_trx;
	//static bool m_bNotAllowingEdits;

	void ReInitGrid(VEC_DWORD *pvRec);
	void InitialSort();

	void EnableAllowEdits();
	BOOL BadLinkSearch(int fcn, LINK_PARAMS &lp);

	DWORD NumRecs() {return m_nNumRecs;}
	UINT m_nSelCount;

	bool m_bRestoreLayout;
	bool m_bExpandMemos;
	enum {MEMO_EXPAND_WIDTH=120};

	void Destroy() {/*GetWindowRect(&m_rectSaved);*/ DestroyWindow();}

// Dialog Data
	enum { IDD = IDD_DBFGRID_DIALOG };

	struct FldInfo
	{
		FldInfo() : pnam(NULL)
			,foff(0)
			,flen(0)
			,ftyp(0)
			,fdec(0)
			,col_order(0)
			//,col_width(LVSCW_AUTOSIZE_USEHEADER)
			//,col_width_org(LVSCW_AUTOSIZE_USEHEADER)
		{}
		LPCSTR pnam;
		WORD foff;
		WORD flen;
		BYTE ftyp;
		BYTE fdec;
		short col_order;
		//short col_width;
		//short col_width_org;
	};

	typedef std::vector<FldInfo> VEC_FLD;
	typedef VEC_FLD::iterator IT_VEC_FLD;

	CShpLayer *m_pShp;

	void RefreshTable(DWORD rec=0);
	void InvalidateTable() {m_list.Invalidate(0);}
	void SaveShpColumns(); //called from CShpLayer::InitShpDef()
	void CopySelected(CShpLayer *pLayer,LPBYTE pSrcFlds,BOOL bConfirm);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

#ifdef _USE_THEMES
	//CTheme m_themeManager;
#endif

private:
	CQuickList m_list;
	CShpDBF *m_pdb;
	SHP_DBFILE *m_pdbfile;
	VEC_PXCOMBO *m_pvxc;
	WCHAR	*m_pwchTip;
	ULONG	m_wchTipSiz;
	CFltRect m_extPolyOrg;
	DWORD m_uPromptRec;
	CString m_csCopyMsg;
	LPBYTE m_sort_pfldbuf;
	WORD m_sort_lenfldbuf;
	CCopySelectedDlg *m_pCopySelectedDlg;

	bool m_bRedrawOff;
	bool m_bAllowPolyDel;

	CFileDropTarget m_dropTarget;

	int ColAdvance(int iCol);
	static void XC_AdjustWidth(CWnd *pWnd,int nFld);

	void InitRecno();
	void CountDeletes();
	void AllowPolyDel(bool bEnable);
	void InitColumns();
	void InitFldInfo();
	void InitDialogSize();
	UINT FlagSelectedItems(bool bUnselect=true);
	void UnflagSelectedItems(UINT cnt);
	void UpdateSelCount(UINT nCount);
	UINT GetSelectedRecs(VEC_DWORD &vRec);
	UINT GetSelectedShpRecs(VEC_SHPREC &vRec,bool bMerging);
	void AddToSelected(bool bReplace);
	
	void SetListRedraw(BOOL bDraw)
	{
		m_list.SetRedraw(bDraw);
		m_bRedrawOff=(bDraw==FALSE);
	}

	DWORD GetFirstSelectedRecNo()
	{
		int rec=GetFirstSelectedItem();
		return (rec>=0)?GetRecNo(rec):0;
	}

	UINT NumSelected()
	{
		return m_list.GetSelectedCount();
	}

	bool IsFldReadonly(int nFld)
	{
		return m_pvxc && (*m_pvxc)[--nFld] && ((*m_pvxc)[nFld]->wFlgs&XC_READONLY);
	}

	int GetFirstSelectedItem()
	{
		POSITION pos = m_list.GetFirstSelectedItemPosition();
		return pos?m_list.GetNextSelectedItem(pos):-1;
	}

	DWORD GetRecNo(int item)
	{
		ASSERT((DWORD)item<m_nNumRecs);
		if(m_iColSorted==0 && !m_bAscending) item=m_nNumRecs-item-1;
		return m_vRecno[item];
	}

	int GetRecIndex(int item)
	{
		return (m_iColSorted==0 && !m_bAscending)?(m_nNumRecs-item-1):item;
	}

	bool IsPtSelected(DWORD rec)
	{
		return m_pdbfile->vdbe.size()>=rec && (m_pdbfile->vdbe[rec-1]&SHP_SELECTED)!=0;
	}

	bool IsPtOpen(DWORD rec);
	void DeleteRecords(bool bDelete);
	
	int m_nSubset; //0=undeleted, 1=deleted, 2=both
	int m_nNumFlds;
	int m_nNumCols;
	DWORD m_nNumRecs,m_uDelCount;
	int m_iColSort,m_iColSorted;
	WORD m_wDigitWidth,m_wLinkTestFld;
	bool m_bReloaded;
	bool m_bAscending;
	bool m_bAllowEdits;
	CFltRect m_extOrg;
	CFindDlgTbl *m_pFindDlg;
	VEC_FLD m_vFld;
	WORD m_wFlags;
	int m_iLastFind;
	CString m_csFind;

	std::vector<DWORD> m_vRecno;  //used for access when m_iColSorted!=0

	bool IsSortable(int nCol) {return true;}
	void SortColumn(bool bAscending);
	void OnChkcolumns();
	void OpenMemo(DWORD rec, UINT fld);
	void CenterView(BOOL bZoom);
	void SelectFlaggedItems(int iStart,int iCount,int iCol);

	// Generated message map functions
	virtual BOOL OnInitDialog();

	afx_msg LRESULT OnGetToolTip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetListItem(WPARAM wParam, LPARAM lParam);
	//afx_msg LRESULT OnNavigationTest(WPARAM wParam, LPARAM lParam);
	//afx_msg LRESULT OnNavigateTo(WPARAM wParam, LPARAM lParam);

	#ifdef	_USE_THEMES
	afx_msg LRESULT OnThemeChanged(WPARAM, LPARAM);
	#endif

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();


	afx_msg void OnCopySelected(UINT id);
	afx_msg void OnClose();
	afx_msg void OnReload();
	afx_msg void OnOK();
	afx_msg void OnEditFind();
	afx_msg LRESULT OnFindDlgMessage(WPARAM wParam, LPARAM lParam);

	afx_msg void OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnBeginEditField(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndEditField(NMHDR* pNMHDR, LRESULT* pResult); 
	afx_msg LRESULT OnListClick(WPARAM wParam, LPARAM lParam);
	//afx_msg LRESULT OnListDblClick(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnListContextMenu(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSortascending();
	afx_msg void OnSortdescending();
	afx_msg void OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemStateChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

	//Table context menu handlers --
private:
	afx_msg void OnAllowEdits();
	afx_msg void OnShowCenter();
	afx_msg void OnZoomView();
	afx_msg void OnAddToSelected();
	afx_msg void OnReplaceSelected();
	afx_msg void OnLaunchWebMap();
	afx_msg void OnOptionsWebMap();
	afx_msg void OnLaunchGE();
	afx_msg void OnOptionsGE();
	afx_msg void OnSelectAll();
	afx_msg void OnRemoveFromTable();
	afx_msg void OnMovetoTop();
	afx_msg void OnExportShapes();
	afx_msg void OnExportXLS();
	afx_msg void OnInvertselection();
	afx_msg void OnLinkTest();
public:
	afx_msg void OnDeleteRecords();
	afx_msg void OnUndeleteRecords();
};
#endif
