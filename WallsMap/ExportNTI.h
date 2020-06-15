#pragma once
#include "ExportJob.h"
#include "EditLabel.h"

class CGdalLayer;

class CExportNTI : public CDialog
{
	DECLARE_DYNAMIC(CExportNTI)

public:
	CExportNTI(CImageLayer *pGLayer, CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportNTI();

	CString m_pathName;
	int m_iHint;
	static BOOL m_bReplWithExported, m_bAddExported, m_bOpenExported;

	// Dialog Data
	enum { IDD = IDD_EXPORT_NTI };

private:
	CExportJob m_job;					 // worker job (thread)
	CWallsMapDoc *m_pDoc;
	CImageLayer *m_pGLayer;
	BOOL m_bProcessing;
	BOOL m_bAddToProject;
	CRect m_crWindow;
	CRect crWindow;
	CEditLabel m_cePathName;
	void Show(UINT id) { GetDlgItem(id)->ShowWindow(SW_SHOW); }
	void Hide(UINT id) { GetDlgItem(id)->ShowWindow(SW_HIDE); }
	void Disable(UINT id) { GetDlgItem(id)->EnableWindow(FALSE); }
	void Enable(UINT id) { GetDlgItem(id)->EnableWindow(TRUE); }
	void SetCheck(UINT id, BOOL bChk) { ((CButton *)GetDlgItem(id))->SetCheck(bChk); }
	void SetText(UINT id, LPCSTR text) { GetDlgItem(id)->SetWindowText(text); }
	void SetUintText(UINT id, UINT uVal);
	void ShowResolution(BOOL bExtent);
	UINT CheckInt(UINT id, int *pi, int iMin, int iMax);
	void UpdateAdvSettings();
	UINT Get_crWindow();

protected:
	afx_msg void OnBrowse();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();

	CProgressCtrl m_Progress;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedExport();
	afx_msg LRESULT OnProgress(WPARAM wp, LPARAM lp);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAdvanced();
	BOOL m_bExtentWindow;
	afx_msg void OnBnClickedExtentEntire();
	afx_msg void OnBnClickedExtentWindow();
	afx_msg void OnBnClickedExtentCustom();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnBnClickedAddExported();
	afx_msg void OnBnClickedReplWithExported();
	afx_msg void OnBnClickedOpenExported();
};
