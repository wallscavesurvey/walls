#if !defined(AFX_SVGPROC_H__CA467285_28E2_421F_B33B_9A11698CD64C__INCLUDED_)
#define AFX_SVGPROC_H__CA467285_28E2_421F_B33B_9A11698CD64C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SvgProc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSvgProcDlg dialog

class CSvgProcDlg : public CDialog
{
	// Construction
public:
	CSvgProcDlg(LPFN_SVG_EXPORT pfnExp, LPFN_ERRMSG pfnErr, LPFN_EXPORT_CB pfnCB, CWnd* pParent = NULL);   // standard constructor
	void StatusMsg(LPCSTR msg) { GetDlgItem(IDC_ST_MESSAGE)->SetWindowText(msg); }

	// Dialog Data
		//{{AFX_DATA(CSvgProcDlg)
	enum { IDD = IDD_SVGPROCESS };
	// NOTE: the ClassWizard will add data members here
//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSvgProcDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	LPFN_SVG_EXPORT m_pfnExp;

protected:
	LPFN_ERRMSG m_pfnErr;
	LPFN_EXPORT_CB m_pfnCB;
	void OnOK();

	// Generated message map functions
	//{{AFX_MSG(CSvgProcDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVGPROC_H__CA467285_28E2_421F_B33B_9A11698CD64C__INCLUDED_)
