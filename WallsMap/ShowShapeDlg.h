#pragma once

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif

#include "SSplitter.h"
#include "PointTree.h"

#include "FileDropTarget.h"
#include "QuickList.h"
#include "ResizeDlg.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "EditLabel.h"

// CShowShapeDlg dialog

typedef std::vector<EDITED_MEMO> VecMemo;
typedef VecMemo::iterator it_vMemo;

class CCopySelectedDlg;

class CShowShapeDlg : public CResizeDlg
{
	DECLARE_DYNAMIC(CShowShapeDlg)

	enum { IDD = IDD_SHOW_SHAPE };

	CSSplitter	m_SplitterPane;
	CWallsMapDoc *m_pDoc;
	CCopySelectedDlg *m_pCopySelectedDlg;


	VEC_SHPREC	*m_vec_shprec;
	SHPREC		*m_pNewShpRec;

	CPointTree	m_PointTree;
	CQuickList	m_FieldList;
	CEditLabel m_ceEast;
	CEditLabel m_ceNorth;

	HTREEITEM	m_hSelRoot,m_hSelItem;
	CShpLayer	*m_pSelLayer;
	UINT		m_uSelRec;
	LPBYTE		m_pEditBuf;
	CString m_csCopyMsg;

	BYTE m_vdbeBegin;
	bool m_bEditShp;
	bool m_bClosing;
	BYTE m_bEditDBF;
	bool m_bInitFlag;
	bool m_bConflict;
	bool m_bDlgOpened;
	bool m_bFieldsInitialized;

	//int m_iMemoFld;

	VecMemo m_vMemo;
	std::vector<CFltPoint> m_vec_relocate;

public:
	static bool m_bLaunchGE_from_Selelected;
	CShowShapeDlg(CWallsMapDoc *pDoc,CWnd* pParent = NULL);   // standard constructor
	virtual ~CShowShapeDlg();
	void Destroy() {GetWindowRect(&m_rectSaved); DestroyWindow();}
	void ReturnFocus() { m_PointTree.SetFocus(); }

	CShpLayer * SelectedLayer() {return m_pSelLayer;}
	UINT GetSelectedFieldValue(CString &s,int nFld);
	void InvalidateTree() {m_PointTree.Invalidate();}

	void ReInit(CWallsMapDoc *pDoc);
	int  GetRelocateStr(CString &s);
	void Relocate(CFltPoint &fpt);
	void AddShape(CFltPoint &fpt,CShpLayer *pLayer);
	BOOL UpdateMemo(CShpLayer *pShp,EDITED_MEMO &memo);
	BOOL CancelSelChange();
	UINT GetDbtRec(int nFld);
	bool IsEmpty() {return m_hSelItem==NULL;}
	void OpenMemo(UINT nFld);
	int CopySelected(CShpLayer *pLayer,LPBYTE pSrcFlds,BOOL bConfirm,HTREEITEM *phDropItem=NULL);

	HTREEITEM IsLayerSelected(CShpLayer *pShp) const;
	void UpdateLayerTitle(CShpLayer *pShp);
	
	bool IsRecOpen(CShpLayer *pShp,DWORD nRec) const
	{
		return nRec==m_uSelRec && m_pSelLayer->m_pdb==pShp->m_pdb;
	}

	bool IsRecOpen(CShpLayer *pShp) const
	{
		return m_uSelRec && m_pSelLayer->m_pdb==pShp->m_pdb;
	}

	bool HasEditedRec(CShpLayer *pShp) const
	{
		return IsRecOpen(pShp) && (m_bEditDBF || IsAddingRec());
	}

	void RefreshBranchTitle(CShpLayer *pShp)
	{
		RefreshBranchTitle(IsLayerSelected(pShp));
	}

	afx_msg void OnExportTreeItems(UINT id);
	void OnDropTreeItem(HTREEITEM hPrev);
	UINT CopySelectedItem(CShpLayer *pDropLayer,HTREEITEM *phDropItem);

	bool IsRecHighlighted(CShpLayer *pShp,UINT rec)
	{
		return rec==m_uSelRec && pShp==m_pSelLayer;
	}

	bool IsAddingRec() const
	{
		return m_pNewShpRec && m_pNewShpRec->rec==m_uSelRec;
	}

	bool IsSelMovable()
	{
		//No kinds of edits can be pending!
		if(m_bDlgOpened) {
			return m_bDlgOpened=false;
		}
		return !m_bEditShp && !IsDBFEdited() && !HasNewLocFlds();
	}

	bool IsSelDroppable(HTREEITEM hDropItem)
	{
		HTREEITEM hDropRoot=m_PointTree.GetParentItem(hDropItem);
		if(!hDropRoot) hDropRoot=hDropItem;
		CShpLayer *pDropLayer=(CShpLayer *)m_PointTree.GetItemData(hDropRoot);
		return pDropLayer==m_pSelLayer || pDropLayer->CanAppendShpLayer(m_pSelLayer);
	}

	int GetChildCount(HTREEITEM hItem)
	{
	   int count=0;
	   hItem = m_PointTree.GetChildItem(hItem);
	   while (hItem != NULL)
	   {
		  count++;
		  hItem = m_PointTree.GetNextItem(hItem, TVGN_NEXT);
	   }
	   return count;
	}

	void ClearVecMemo()
	{
		for(it_vMemo it=m_vMemo.begin();it!=m_vMemo.end();it++) {
			free(it->pData);
		}
		m_vMemo.clear();
	}

	UINT NumSelected() {
		return m_PointTree.GetCount()-m_uLayerTotal;
	}

	void ShowNoMatches();
	void FillPointTree();

	LPBYTE LoadEditBuf();

	LPSTR GetSelTreeLabel()
	{
		ASSERT(m_PointTree.GetParentItem(m_hSelItem));
		return GetTreeLabel((UINT)m_PointTree.GetItemData(m_hSelItem));
	}

private:

	CFileDropTarget m_dropTarget;

	WCHAR	*m_pwchTip;
	ULONG	m_wchTipSiz;

	static CRect m_rectSaved;
	bool IsFieldEmpty(UINT nFld);
	void NoSearchableMsg();
	void GE_Export(bool bOptions);
	bool IsLocationValid() {return m_uSelRec && m_bValidNorth && m_bValidEast && m_bValidZone;}
	bool DiscardShpEditsOK();
	
	BYTE & SelEditFlag() {return (*m_pSelLayer->m_vdbe)[m_uSelRec-1];}
	bool SelDeleted()
	{
#ifdef _DEBUG
		bool bret=(SelEditFlag()&SHP_EDITDEL)!=0;
		LPBYTE pData=m_pSelLayer->m_pdb->RecPtr(m_uSelRec);
		//ASSERT(bret==(*pData=='*')); //could be a bad shapefile, deleted recs indicated in shp component
		return bret;
#else
		return (SelEditFlag()&SHP_EDITDEL)!=0;
#endif
	}
	void GetBranchRecs(VEC_DWORD &vRec);
	void RefreshBranchTitle(HTREEITEM m_hSelItem);

	it_vMemo Get_vMemo_it(int fld);

	void ClearFields()
	{
		m_bInitFlag=true;
		m_FieldList.DeleteAllItems(); //Clears items
		m_bInitFlag=false;
	}

	void ClearTree()
	{
		m_bInitFlag=true;
		m_PointTree.DeleteAllItems(); //Clears items
		m_bInitFlag=false;
	}

	BOOL DeleteTreeItem(HTREEITEM h0);

	void UpdateFld(int fnum,LPCSTR text);
	void CancelAddition();
	void FlagSelection(bool bUndelete);

	bool HasNewLocFlds()
	{
		ASSERT(!m_vec_relocate.size() || !m_uSelRec || (SelEditFlag()&SHP_EDITSHP));

		if(!m_pSelLayer || !m_uSelRec || !IsAddingRec() && !m_vec_relocate.size() && !(SelEditFlag()&SHP_EDITSHP))
			return false;
		return m_pSelLayer->HasLocFlds();
	}

	void MoveRestore();
	void RefreshSelCoordinates();
	void RefreshEditCoordinates();
	void RefreshCoordinates(CFltPoint &fpt);
	void ApplyChangedPoint(const CFltPoint &fpt);
	bool ConfirmFlyToEdited(CFltPoint &fpt,bool &bCross);
	void FlyToWeb(bool bOptions);

	LPBYTE GetRecPtr()
	{
		ASSERT(m_pEditBuf || m_uSelRec);
		return m_pEditBuf?m_pEditBuf:(LPBYTE)m_pSelLayer->m_pdb->RecPtr(m_uSelRec);
	}

	int GetTrimmedFld(UINT nFld,LPSTR pDest,UINT szDest)
	{
		return m_pSelLayer->m_pdb->GetBufferFld(GetRecPtr(),nFld,pDest,szDest);
	}

	LPBYTE GetFldPtr(UINT nFld)
	{
		return GetRecPtr()+m_pSelLayer->m_pdb->FldOffset(nFld);
	}

	void FlushEdits();
	void RemoveSelection();
	bool IsDBFEdited();
	void Hide(UINT id) {GetDlgItem(id)->ShowWindow(SW_HIDE);}
	void Show(UINT id) {GetDlgItem(id)->ShowWindow(SW_SHOW);}
	void Show(UINT id,bool bShow) {GetDlgItem(id)->ShowWindow(bShow?SW_SHOW:SW_HIDE);}
	void SetCoordLabels();
	void ZoomToPoint(double fZoom);
	void OnEnChangeEditCoord(bool bValid,bool bValidOther);

	void SetCheck(UINT id,BOOL bChk)
	{
		((CButton *)GetDlgItem(id))->SetCheck(bChk);
	}

	bool IsChecked(UINT id)
	{
		return ((CButton *)GetDlgItem(id))->GetCheck()==BST_CHECKED;
	}

	UINT GetEditLabel(CString &text)
	{
		GetDlgItem(IDC_EDIT_LABEL)->GetWindowText(text);
		text.Trim();
		return text.GetLength();
	}
	BOOL IsEnabled(UINT id) {return GetDlgItem(id)->IsWindowEnabled();}
	void Disable(UINT id) {GetDlgItem(id)->EnableWindow(FALSE);}
	void Enable(UINT id) {GetDlgItem(id)->EnableWindow(TRUE);}
	void Enable(UINT id,bool bEnable) {GetDlgItem(id)->EnableWindow(bEnable==true);}
	void SetFocus(UINT id) {GetDlgItem(id)->SetFocus();}
	void SetText(UINT id,LPCSTR text) {m_bInitFlag=true;GetDlgItem(id)->SetWindowText(text);m_bInitFlag=false;}
	void ClearLabel() {m_ceLabel.SetWindowText(""); Disable(IDC_FINDBYLABEL);}
	void SetLabel(LPCSTR pText) {m_ceLabel.SetWindowText(pText);}

	void EnableNadToggle()
	{
		if(!IsEnabled(IDC_DATUM1)) {
			Enable(IDC_DATUM1);
			Enable(IDC_DATUM2);
			Enable(IDC_TYPEGEO);
			Enable(IDC_TYPEUTM);
		}
	}

	void GetText(UINT id,CString &s)
	{
		GetDlgItem(id)->GetWindowText(s);
	}
	void GetText(UINT id,LPSTR buf,UINT sizBuf)
	{
		GetDlgItem(id)->GetWindowText(buf,sizBuf);
	}

	void SetRO(UINT id,BOOL bRO)
	{
		((CEdit *)GetDlgItem(id))->SetReadOnly(bRO);
	}
	void ClearText(UINT id)
	{
		SetText(id,"");
		Disable(id);
	}
	void SetCoordFormat();
	BOOL GetEditedPoint(CFltPoint &fpt,bool bChkRange=false);
	LPSTR GetTreeLabel(UINT i);
	LPSTR GetTreeLabel(HTREEITEM h)
	{
		ASSERT(m_PointTree.GetParentItem(h));
		return GetTreeLabel((UINT)m_PointTree.GetItemData(h));
	}

	void UpdateTotal()
	{
		CString s;
		s.Format("%u",NumSelected());
		SetText(IDC_ST_TOTAL,s);
	}

	void RefreshRecNo(UINT rec) {
		CString s;
		if(rec) s.Format("#%u%s",rec,m_pSelLayer->IsEditable()?"":" Locked");
		SetText(IDC_RECNO,s);
	}

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnSelChangedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLclickTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChangingTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomDrawTree( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnSpinUtm(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTreeDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnEndEditField(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBeginEditField(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetListItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnListNavTest(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnListClick(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetToolTip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEditChange(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedFindbylabel();
	afx_msg void OnEnChangeEditNorth();
	afx_msg void OnEnChangeEditEast();
	afx_msg void OnEnChangeEditZone();
	afx_msg void OnEnChangeEditLabel();
	afx_msg void OnBnClickedUndoEdit();
	afx_msg void OnBnClickedRelocate();
	afx_msg void OnBnClickedEnterkey();
	afx_msg void OnShowCenter();
	afx_msg void OnDelete();
	afx_msg void OnExportShapes();
	afx_msg void OnBnClickedUndoRelocate();
	afx_msg void OnShowCopyloc();
	afx_msg void OnShowPasteloc();
	afx_msg void OnBnClickedTypegeo();
	afx_msg void OnBnClickedDatum();

private:
#if 0
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
#endif

	CEditLabel m_ceLabel;
	//afx_msg void OnBnClickedSaveattr();
	afx_msg void OnBnClickedResetattr();
	afx_msg void OnClearEdited();
	afx_msg void OnUpdateUnselect(CCmdUI *pCmdUI);
	afx_msg void OnUnselect();
	afx_msg void OnOpenTable();

	UINT m_uLayerTotal;
	int m_iZoneSel;	//Zone of selection (0 if Lat/Long)
	int m_iZoneDoc; //Zone of view (m_Points)
	INT8 m_iNadSel; //Datum setting of selection (0 if unsupported)
	INT8 m_iNadDoc; //Datum setting of view (m_points)
	bool m_bNadToggle,m_bValidEast,m_bValidNorth,m_bValidZone,m_bDroppingItem,m_bUtmDisplayed;

	afx_msg void OnZoomView();
	afx_msg void OnBnClickedMarkselected();
	afx_msg void OnBranchsymbols();
	afx_msg void OnSetEditable();
	afx_msg void OnLaunchWebMap();
	afx_msg void OnOptionsWebMap();
	afx_msg void OnLaunchGE();
	afx_msg void OnOptionsGE();
	//afx_msg void OnUpdateLaunchGe(CCmdUI *pCmdUI);
	afx_msg LRESULT OnAdvancedSrch(WPARAM,LPARAM);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};

extern CShowShapeDlg * app_pShowDlg;
extern bool CheckShowDlg();

