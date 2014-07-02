#pragma once
#include "afxwin.h"

// CWebMapFmtDlg dialog

#define DFLT_URL_COUNT 6

class CWebMapFmtDlg : public CDialog
{
	DECLARE_DYNAMIC(CWebMapFmtDlg)

public:
	CWebMapFmtDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWebMapFmtDlg();

	static BOOL m_bSaveStatic;
	static LPCSTR m_strPrgDefault[DFLT_URL_COUNT];
	static CString m_csUsrDefault[DFLT_URL_COUNT];
	int m_idx;
	CString m_csURL;
	BOOL m_bFlyTo;
	BOOL m_bSave;
	bool m_bChg;

private:
	void EnableSave();

// Dialog Data
	enum { IDD = IDD_WEBMAP_FORMAT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedReset();
	afx_msg void OnEditChange(); 
	afx_msg void OnSelChange(); 
	afx_msg void OnBnClickedUpdate();
	afx_msg void OnBnClickedOk();

	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	CComboBox m_urlCombo;
};
