#if !defined(AFX_EXPSVGDLG_H__B9CF9DBB_17F9_44E5_99B2_B2E53C8E7E54__INCLUDED_)
#define AFX_EXPSVGDLG_H__B9CF9DBB_17F9_44E5_99B2_B2E53C8E7E54__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExpSvgDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExpSvgDlg dialog

class CExpSvgDlg : public CDialog
{
// Construction
public:
	CExpSvgDlg(VIEWFORMAT *pVF,SVG_VIEWFORMAT *pSVG,CWnd* pParent = NULL);   // standard constructor
	VIEWFORMAT *m_pVF;
	SVG_VIEWFORMAT *m_pSVG;
	BOOL m_bExists;
	BOOL m_bView;
	UINT m_uFlags;
// Dialog Data
	//{{AFX_DATA(CExpSvgDlg)
	enum { IDD = IDD_EXPSVGDLG };
	CString	m_pathname;
	CString	m_GridSub;
	CString	m_GridInt;
	CString	m_title;
	BOOL	m_bUseMerge2;
	BOOL	m_bShpFlags;
	BOOL	m_bShpGrid;
	BOOL	m_bShpWalls;
	BOOL	m_bShpNotes;
	BOOL	m_bShpStations;
	BOOL	m_bShpVectors;
	BOOL	m_bAdjustable;
	CString	m_MergePath,m_MergePath2;
	int		m_bShpDetail;
	int		m_bShpLegend;
	BOOL	m_bUseStyles;
	//}}AFX_DATA

	void StatusMsg(char *msg);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpSvgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void EnableMergeWindows(BOOL bChk);
	BOOL init3chk(UINT flag);
	int Checked(UINT id) {return ((CButton *)GetDlgItem(id))->GetCheck();}
	void SetCheck(UINT id,UINT u) {((CButton *)GetDlgItem(id))->SetCheck(u);}
	int CheckData();
	void Browse(UINT id);

	// Generated message map functions
	//{{AFX_MSG(CExpSvgDlg)
	afx_msg void OnBrowse();
	virtual BOOL OnInitDialog();

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnAdvanced();
	afx_msg void OnChkMerge2();
	afx_msg void OnToggleLayer();
	afx_msg void OnBrowse2();
	afx_msg void OnBrowse3();
	afx_msg void OnAdjustable();
	//}}AFX_MSG
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPSVGDLG_H__B9CF9DBB_17F9_44E5_99B2_B2E53C8E7E54__INCLUDED_)
