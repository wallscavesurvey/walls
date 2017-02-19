#pragma once
#include "ExportJob.h"

// CExpandNTI dialog

class CExpandNTI : public CDialog
{
	DECLARE_DYNAMIC(CExpandNTI)

public:
	CExpandNTI(CNtiLayer *pLayer,CWnd* pParent = NULL);   // standard constructor
	virtual ~CExpandNTI();

// Dialog Data
	enum { IDD = IDD_EXPAND_NTI };

	BOOL OnInitDialog();

private:
	void Show(UINT id) {GetDlgItem(id)->ShowWindow(SW_SHOW);}
	void Hide(UINT id) {GetDlgItem(id)->ShowWindow(SW_HIDE);}
	void Disable(UINT id) {GetDlgItem(id)->EnableWindow(FALSE);}
	void Enable(UINT id) {GetDlgItem(id)->EnableWindow(TRUE);}
	void SetText(UINT id,LPCSTR text) {GetDlgItem(id)->SetWindowText(text);}
	BOOL m_bProcessing;
	CExportJob m_job;					 // worker job (thread)
	CNtiLayer *m_pLayer;
	CProgressCtrl m_Progress;
	UINT m_uFirstRec;
	CString m_csTitle;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnProgress(WPARAM wp, LPARAM lp);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
};
