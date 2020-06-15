// IgrfExportDlg.h : header file
//

#pragma once

#include "filebuf.h"

// CIgrfExportDlg dialog
class CIgrfExportDlg : public CDialog
{
	// Construction
public:
	CIgrfExportDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TSSTRAVIS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	CDaoDatabase m_database;
	CFileBuffer m_fbLog;
	BOOL m_bLogFailed;
	HICON m_hIcon;

	void CDECL WriteLog(const char *format, ...);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBrowse();
	DECLARE_MESSAGE_MAP()

public:
	UINT m_nLogMsgs;
	CString m_csPathName;
};
