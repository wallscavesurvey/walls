#if !defined(AFX_PAGEOFFDLG_H__9E421CA3_F18A_4F8F_AE48_7C52E8CE968F__INCLUDED_)
#define AFX_PAGEOFFDLG_H__9E421CA3_F18A_4F8F_AE48_7C52E8CE968F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PageOffDlg.h : header file
//

struct VIEWFORMAT;

/////////////////////////////////////////////////////////////////////////////
// CPageOffDlg dialog

class CPageOffDlg : public CDialog
{
// Construction
public:
	CPageOffDlg(VIEWFORMAT *pVF,double fPW,double fPH,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPageOffDlg)
	enum { IDD = IDD_PAGEOFFDLG };
	BOOL	m_bCenterOfPage;
	CString	m_LeftOffset;
	CString	m_TopOffset;
	CString	m_Overlap;
	BOOL	m_bInsideFrame;
	int		m_iLegendPos;
	//}}AFX_DATA

private:
	VIEWFORMAT *m_pVF;
	double m_fCenterLeft,m_fCenterTop;

	void GetCenterOffsets();
	void Enable(UINT id,BOOL bEnable) {
		GetDlgItem(id)->EnableWindow(bEnable);
	}
	void SetText(UINT id,const char *pText)
	{
		((CEdit *)GetDlgItem(id))->SetWindowText(pText);
	}
	UINT CheckData();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPageOffDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	// Generated message map functions
	//{{AFX_MSG(CPageOffDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCenterOfPage();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEOFFDLG_H__9E421CA3_F18A_4F8F_AE48_7C52E8CE968F__INCLUDED_)
