#if !defined(AFX_EXPSEFD_H__7AB9E932_E347_11D1_AFB3_000000000000__INCLUDED_)
#define AFX_EXPSEFD_H__7AB9E932_E347_11D1_AFB3_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// expsefd.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CExpSefDlg dialog
class CPrjDoc;

class CExpSefDlg : public CDialog
{
	// Construction
public:
	CExpSefDlg(CWnd* pParent = NULL);   // standard constructor

	enum { IDD = IDD_EXPORTSEF };

	CString	m_pathname;

	static BOOL	m_bNoConvertPfx;
	static BOOL	m_bUseVdist;
	static BOOL	m_bUseAllSef;
	CPrjDoc *m_pDoc;
	CPrjListNode *m_pNode;

	// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	// Implementation
protected:
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnBrowse();
	afx_msg void OnBnClickedUseAllsef();
	afx_msg void OnBnClickedUseVdist();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPSEFD_H__7AB9E932_E347_11D1_AFB3_000000000000__INCLUDED_)
