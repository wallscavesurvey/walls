#include "afxwin.h"
#if !defined(AFX_SVGADV_H__FDA613F1_DCFA_4FC0_A306_D692F6B61A52__INCLUDED_)
#define AFX_SVGADV_H__FDA613F1_DCFA_4FC0_A306_D692F6B61A52__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SvgAdv.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSvgAdv dialog

class CSvgAdv : public CDialog
{
	// Construction
public:
	CSvgAdv(SVG_ADV_PARAMS *pAdv, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSvgAdv)
	enum { IDD = IDD_SVGADVANCED };
	CString	m_decimals;
	CString	m_distance_exp;
	CString	m_distance_off;
	CString	m_length_exp;
	CString	m_vector_cnt;
	CString	m_minveclen;
	CString	m_maxCoordChg;
	BOOL	m_bUseLineWidths;
	BOOL	m_bUseLabelPos;
	BOOL	m_bLrudBars;
	//}}AFX_DATA
	BOOL m_bLrudBoxes;


	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CSvgAdv)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

public:
	static void LoadAdvParams(SVG_ADV_PARAMS *pAdv, BOOL bMerging);
	static void SaveAdvParams(SVG_ADV_PARAMS *pAdv);

protected:

	SVG_ADV_PARAMS *m_pAdv;

	UINT CheckData();
	// Generated message map functions
	//{{AFX_MSG(CSvgAdv)
	afx_msg void OnDefaults();
	//}}AFX_MSG
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLrudbars();
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVGADV_H__FDA613F1_DCFA_4FC0_A306_D692F6B61A52__INCLUDED_)
