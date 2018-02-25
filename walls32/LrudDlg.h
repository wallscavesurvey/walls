#if !defined(AFX_LRUDDLG_H__04C0F481_7A2B_4E63_A739_4177A88A8E62__INCLUDED_)
#define AFX_LRUDDLG_H__04C0F481_7A2B_4E63_A739_4177A88A8E62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LrudDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLrudDlg dialog

class CSegView;

class CLrudDlg : public CDialog
{
// Construction
public:
	CLrudDlg(CPrjDoc *pDoc, CSegView *pSV, CWnd* pParent = NULL);   // standard constructor

	enum { IDD = IDD_LRUDSTYLE };

public:
	int		m_iLrudStyle;
	BOOL	m_bSVGUseIfAvail;
	CString	m_ThickPenPts;
	CString m_LrudExag;
	double  m_fThickPenPts;
	int m_iEnlarge;
	CSegView *m_pSV;
	CPrjDoc *m_pDoc;

	BOOL m_bMapsActive;
	void EnableApply(BOOL bEnable) {GetDlgItem(IDC_APPLY)->EnableWindow(bEnable);}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();


	BOOL HasChanged();

    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedSVGUse();
	afx_msg void OnBnClickedLRUD(UINT id);
	afx_msg void OnEditChgEnlarge();
	afx_msg void OnEditChgExag();
	afx_msg void OnEditThickPenPts();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LRUDDLG_H__04C0F481_7A2B_4E63_A739_4177A88A8E62__INCLUDED_)
