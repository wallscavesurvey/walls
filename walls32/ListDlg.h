#if !defined(AFX_LISTDLG_H__30ACC5DE_C78B_42DF_8DED_1D3851165955__INCLUDED_)
#define AFX_LISTDLG_H__30ACC5DE_C78B_42DF_8DED_1D3851165955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CListDlg dialog

enum { LFLG_VECTOR = 1, LFLG_NOTEONLY = 2, LFLG_GROUPBYFLAG = 4, LFLG_GROUPBYLOC = 8, LFLG_OPENEXISTING = 16, LFLG_LATLONG = 32, LFLG_LATLONGOK = 64 };

class CListDlg : public CDialog
{
	// Construction
public:
	CListDlg(LPCSTR pPath, int nVectors, CWnd* pParent = NULL);   // standard constructor
	static UINT m_uDecimals;
	static UINT m_uFlags;

	// Dialog Data
		//{{AFX_DATA(CListDlg)
	enum { IDD = IDD_LISTDLG };
	CString	m_Decimals;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
private:
	int m_nVectors;

	// Implementation
protected:
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CListDlg)
	afx_msg void OnTypName();
	afx_msg void OnTypVector();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CString m_pathname;
	BOOL m_bVector;
	BOOL m_bGroupByLoc;
	BOOL m_bNotedOnly;
	BOOL m_bGroupByFlag;
	BOOL m_bLatLong;
	afx_msg void OnBnClickedBrowse();
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTDLG_H__30ACC5DE_C78B_42DF_8DED_1D3851165955__INCLUDED_)
