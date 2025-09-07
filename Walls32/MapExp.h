#if !defined(AFX_MAPEXP_H__66FCD220_3DA8_11D1_B46B_AF38AF34C820__INCLUDED_)
#define AFX_MAPEXP_H__66FCD220_3DA8_11D1_B46B_AF38AF34C820__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MapExp.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapExpDlg dialog

class CMapExpDlg : public CDialog
{
	// Construction
public:
	CMapExpDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMapExpDlg)
	enum { IDD = IDD_MAPEXPORT };
	CString	m_pathname;
	//}}AFX_DATA

	CString m_defpath;

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CMapExpDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void FixWMFname();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CMapExpDlg)
	afx_msg void OnBrowse();
	virtual BOOL OnInitDialog();
	afx_msg void OnFormatOptions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPEXP_H__66FCD220_3DA8_11D1_B46B_AF38AF34C820__INCLUDED_)
