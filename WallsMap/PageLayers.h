#pragma once

#include "ResizeDlg.h"
#include "LayerSetPage.h"
#include "FileDropTarget.h"
#include "ColumnTreeCtrl.h"

//using namespace ListCtrlEx;

class CLayerSetSheet;
class CWallsMapDoc;

extern BOOL hPropHook;
extern CLayerSetSheet *pLayerSheet;
extern const UINT WM_PROPVIEWDOC;

//==========================================
// CPageLayers dialog

class CPageLayers : public CLayerSetPage
{
	//DECLARE_DYNCREATE(CPageLayers)
	friend CLayerSetSheet;

public:
	CPageLayers();
	virtual ~CPageLayers();
	virtual void UpdateLayerData(CWallsMapDoc *pDoc);

	void RefreshListItem(PTL ptl);
	void RedrawList();
    void OnDrop();
	bool SaveColumnWidths();

// Dialog Data
	enum { IDD = IDD_PAGE_LAYERS };

private:
#ifdef _DEBUG
	BOOL CheckTree();
#endif
	void AdvDlgCtrl(int n)	{while(n--) CDialog::NextDlgCtrl();}

	PTL GetPTL(HTREEITEM hItem)
	{
		return (PTL)m_Tree.GetItemData(hItem);
	}
	void SelectDocTreePosition();
	void InsertFolder(int pos);
	void UpdateTree();
	void InitColWidths();
	void UpdateButtons();
	//void SaveLayerStatus();
	void OnClose();
	void OnEditPaste(); 
	void OnOpenfolder();
	int EditTitle(CString &s);

	CFileDropTarget m_dropTarget;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//afx_msg void GetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnCustomdrawMyList ( NMHDR* pNMHDR, LRESULT* pResult );
	//afx_msg void OnItemChangedList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnClickList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnDblClickList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnRtClickList(NMHDR* pNMHDR, LRESULT* pResult);
	//afx_msg void OnKeydownList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMoveLayerToTop();
	afx_msg void OnMoveLayerToBottom();
	afx_msg LRESULT OnRetFocus(WPARAM,LPARAM);
	afx_msg void OnBnClickedLayerAdd();
	afx_msg void OnBnClickedLayerConv();
	afx_msg void OnBnClickedScaleRange();
	afx_msg void OnBnClickedLayerDel();
	afx_msg void OnCloseAndDelete();
	afx_msg void OnOpenXC();
	/*
	afx_msg void OnBnClickedLayerDown();
	afx_msg void OnBnClickedLayerUp();
	*/
	//afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnCompareShp();
	afx_msg void OnZoomtolayer();
	afx_msg void OnBnClickedSymbology();
	afx_msg void OnEditTitle();
	afx_msg void OnBnClickedOpenTable();
	afx_msg LRESULT OnMoveItem(WPARAM iDragItem, LPARAM iDropItem);
	afx_msg LRESULT OnCheckbox(WPARAM item, LPARAM bNotify);
	afx_msg LRESULT OnDblClickItem(WPARAM item, LPARAM lParam);
	afx_msg LRESULT OnRClickItem(WPARAM item, LPARAM lParam);
	afx_msg LRESULT OnSelChange(WPARAM item, LPARAM lParam);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	//virtual BOOL OnKillActive();

	CColumnTreeCtrl m_LayerTree;
	CCustomTreeChildCtrl &m_Tree;

	afx_msg void OnClearEditflags();
	afx_msg void OnSetEditable();
	afx_msg void OnFolderAbove();
	afx_msg void OnFolderBelow();
	afx_msg void OnFolderChild();
	afx_msg void OnFolderParent();
	afx_msg void OnLayersAbove();
	afx_msg void OnLayersBelow();
	afx_msg void OnLayersWithin();
};


