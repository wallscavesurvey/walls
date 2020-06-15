#pragma once
// CLinkTestDlg dialog

#ifndef _DBGRIDDLG_H
#include "DBGridDlg.h"
#endif

#include "EditLabel.h"

struct CSTR2
{
	CString s0;
	CString s1;
};

typedef std::vector<CSTR2> VEC_CSTR2;

class CLinkTestDlg : public CDialog
{
	DECLARE_DYNAMIC(CLinkTestDlg)

public:
	CLinkTestDlg(CDBGridDlg *pDBGridDlg, UINT nFld, CWnd* pParent = NULL);   // standard constructor
	virtual ~CLinkTestDlg();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnCancel();

	// Dialog Data
	enum { IDD = IDD_LINKTEST };

	LINK_PARAMS m_lp;

private:
	UINT m_nFld, m_nFixed, m_nTotal, m_nCount;
	CDBGridDlg *m_pDBGridDlg;
	CShpLayer *m_pShp;
	CShpDBF *m_pdb;
	CComboBox m_cbLookIn;
	VEC_CSTR2 m_vcs2;
	CString m_sReplacement, m_sBrokenText;
	CEditLabel m_ceReplacement, m_ceBrokenLink;
	bool m_bInit;
	bool m_bReplaceAll, m_bScanAgain;

	void SetText(UINT id, LPCSTR pText) { GetDlgItem(id)->SetWindowText(pText); }
	void GetText(UINT id, CString &s) { GetDlgItem(id)->GetWindowText(s); }

	void SetMsgText(LPCSTR format, ...);
	void SetBrokenText(LPCSTR pText) { SetText(IDC_BROKEN_LINK, pText); }
	void GetBrokenText(CString &s) { GetText(IDC_BROKEN_LINK, s); }

	void SetLabelMsg();

	void SetReplacementText(LPCSTR pText)
	{
		m_bInit = true;
		SetText(IDC_REPLACEMENT, pText);
		m_bInit = false;
	}
	void GetReplacementText(CString &s) { GetText(IDC_REPLACEMENT, s); }

	void SetCounts();
	void Enable(UINT id, BOOL bEnable = 1) { GetDlgItem(id)->EnableWindow(bEnable); }
	void InitStartMsg();

	bool FindReplacement();
	bool CanAccess(CString &sPathRel);
	void StoreUnmatchedPrefixes(LPCSTR p0, LPCSTR p1);

	virtual void OnOK();

	BOOL Search(int fcn)
	{
		return m_pDBGridDlg->BadLinkSearch(fcn, m_lp);
	}
	afx_msg void OnBnClickedReplaceAll();
	afx_msg void OnBnClickedReplace();
	//afx_msg void OnEnChangeFindWhat();
	afx_msg void OnFindNext();
	afx_msg void OnSelChangeLookIn();
	afx_msg LRESULT CLinkTestDlg::OnPropViewDoc(WPARAM wParam, LPARAM typ);
	afx_msg void OnEnChangeRepl();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedSelectrec();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCreateLog();
};
