#if !defined(AFX_VECINFO_H__481C4B3F_F6FD_42BF_8002_414ACE951F1F__INCLUDED_)
#define AFX_VECINFO_H__481C4B3F_F6FD_42BF_8002_414ACE951F1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VecInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVecInfoDlg dialog

class CPrjListNode;

class CVecInfoDlg : public CDialog
{
// Construction
public:
	CVecInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVecInfoDlg)
	enum { IDD = IDD_VECTORINFO };
	//}}AFX_DATA

	CPrjListNode *m_pNode;

private:
	HANDLE m_hMagDlg;
	double m_enu_fr[3],m_enu_to[3];
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVecInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);

// Implementation
protected:
	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void SetText(UINT id,LPCSTR text) {GetDlgItem(id)->SetWindowText(text);}
	void SetCoordinate(UINT id,int i);
	void SetNote(UINT id);
	void SetFlagnames(UINT idf,UINT idm);
	void SetButtonImage(UINT id);

    void Show(UINT idc,BOOL bShow)
    {
    	GetDlgItem(idc)->ShowWindow(bShow?SW_SHOW:SW_HIDE);
    }

    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CVecInfoDlg)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnProperties();
	afx_msg void OnFileEdit();
	afx_msg void OnViewStats();
	afx_msg void OnBnClickedViewseg();
	afx_msg void OnDestroy();
	afx_msg void OnFrMagDlg();
	afx_msg void OnToMagDlg();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VECINFO_H__481C4B3F_F6FD_42BF_8002_414ACE951F1F__INCLUDED_)
