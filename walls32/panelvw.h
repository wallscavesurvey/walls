// panelvw.h : header file for CFormView of CReview tabbed dialog --
// Handles the common characteristics of CTabView panel views (CFormView's).

#ifndef __PANELVW_H
#define __PANELVW_H

#ifndef __MFX_TABVIEW_H__
#include "tabview.h"
#endif

/////////////////////////////////////////////////////////////////////////////
class CPanelView : public CFormView
{
	DECLARE_DYNAMIC(CPanelView)

public:
	CTabView* m_pTabView;

	//Inlines to improve code readability --
	//void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void Enable(UINT id, BOOL bEnable)
	{
		GetDlgItem(id)->EnableWindow(bEnable);
	}

	void SetText(UINT id, char *pStr)
	{
		GetDlgItem(id)->SetWindowText(pStr ? pStr : "");
	}

	void Show(UINT idc, BOOL bShow)
	{
		GetDlgItem(idc)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	}

	void SetCheck(UINT id, BOOL bChecked) { ((CButton *)GetDlgItem(id))->SetCheck(bChecked); }
	BOOL GetCheck(UINT id) { return ((CButton *)GetDlgItem(id))->GetCheck() != 0; }
	void OnUpdatePfxNam(CCmdUI* pCmdUI);

protected:
	CPrjDoc *GetDocument() { return (CPrjDoc *)CView::GetDocument(); }

	// Implementation
protected:
	CPanelView(UINT id); // protected constructor used by dynamic creation
	virtual ~CPanelView();
	afx_msg void OnSysCommand(UINT nChar, LONG lParam);
	// Generated message map functions
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDisplayOptions();
	afx_msg void OnPrintOptions();
	afx_msg void OnZipBackup();
	afx_msg void OnLrudStyle();

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif
