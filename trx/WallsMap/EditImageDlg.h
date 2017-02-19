#pragma once

#include "RulerRichEditCtrl.h"
#include "EditLabel.h"

// CEditImageDlg dialog

class CEditImageDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditImageDlg)

public:
	CEditImageDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditImageDlg();

// Dialog Data
	enum { IDD = IDD_EDITIMAGE };

private:
	CEditLabel m_ceTitle;
	CStatic	m_placement;
	CRulerRichEditCtrl m_rtf;

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//afx_msg void OnBnClickedBrowse();

	void SetRO(UINT id) {((CEdit *)GetDlgItem(id))->SetReadOnly();}

	DECLARE_MESSAGE_MAP()

public:
	BOOL m_bIsEditable;
	CString m_csTitle;
	CString m_csDesc;
	CString m_csPath;
	CString m_csSize;

	afx_msg LRESULT OnLinkClicked(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};
