#pragma once

#ifndef _UTILITY_H
#include "utility.h"
#endif
#include "afxcmn.h"
#include "afxwin.h"
#include "EditLabel.h"

class CShpLayer;

// CDlgExportXLS dialog

class CDlgExportXLS : public CDialog
{
	DECLARE_DYNAMIC(CDlgExportXLS)

	CShpLayer *m_pShp;
	VEC_DWORD &m_vRec;
	CString m_pathName;
	CEditLabel m_cePathName;
	afx_msg void OnBnClickedBrowse();
	LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	virtual BOOL OnInitDialog();

public:
	CDlgExportXLS(CShpLayer *pShp,VEC_DWORD &vRec,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgExportXLS();

// Dialog Data
	enum { IDD = IDD_EXPORT_XLS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void SetText(UINT id,LPCSTR text) {GetDlgItem(id)->SetWindowText(text);}

	DECLARE_MESSAGE_MAP()
public:
	CString m_csMsg;
	BOOL m_bStripRTF;
	UINT m_uMaxChars;
	CProgressCtrl m_Progress;
	void SetStatusMsg(LPCSTR msg);
};
