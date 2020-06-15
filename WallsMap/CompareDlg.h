#pragma once
#include "afxwin.h"
#include "HScrollListBox.h"

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif

#ifndef __TRXFILE_H
#include <trxfile.h>
#endif

// CCompareDlg dialog

class CCompareDlg : public CDialog
{
	DECLARE_DYNAMIC(CCompareDlg)
public:
	CCompareDlg(CShpLayer *pShp, CString &path, CString &summary, VEC_COMPSHP &vCompShp, WORD **ppwFld, CWnd* pParent = NULL);   // standard constructor
	virtual ~CCompareDlg();

	enum { IDD = IDD_COMPARESHP };

	int m_iFlags; //return option flags
	static BOOL bLogOutdated;

private:
	//initialized in CPageLayers before DoModal() --
	VEC_COMPSHP &m_vCompShp;
	CString &m_pathName;
	CString &m_summary;
	WORD **m_ppwFld;

	BYTE m_keylen;
	UINT m_nNumSel;
	BOOL m_bIncludeShp;
	BOOL m_bIncludeLinked;
	BOOL m_bIncludeAll;
	BOOL m_bLogOutdated;

	CShpLayer *m_pShpRef;
	CHScrollListBox m_lbShpCand;
	CHScrollListBox m_lbShpComp;
	CEdit m_cePathName;
	CButton m_cbIncludeShp;
	CButton m_cbIncludeLinked;
	CButton m_cbIncludeAll;
	VEC_WORD m_vFldsIgnored;

private:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void Enable(UINT id, BOOL bEnable) { GetDlgItem(id)->EnableWindow(bEnable); }
	CHScrollListBox *pLB(UINT id) { return (CHScrollListBox *)GetDlgItem(id); }

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

	void UpdateButtons();
	void ShowIgnored();
	void UpdateSelFldChange();
	void MoveAllLayers(BOOL bToRight);
	void MoveOne(CListBox &list1, CListBox &list2, int iSel1, BOOL bToRight);

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedMoveallLeft();
	afx_msg void OnBnClickedMoveallRight();
	afx_msg void OnBnClickedMoveLayerLeft();
	afx_msg void OnBnClickedMoveLayerRight();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedSelIgnored();
	afx_msg void OnBnClickedIncludeAll();
	afx_msg LRESULT OnCommandHelp(WPARAM, LPARAM);
	afx_msg void OnBnClickedIncludeShp();
};
