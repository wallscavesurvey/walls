#pragma once
#include "EditLabel.h"

// CUpdatedDlg dialog

class CUpdatedDlg : public CDialog
{
	DECLARE_DYNAMIC(CUpdatedDlg)

public:
	CUpdatedDlg(SHP_DBFILE *pdbfile,BOOL bUpdating,CWnd* pParent = NULL);   // standard constructor
	virtual ~CUpdatedDlg();

private:
	BOOL m_bIgnoreEditTS;
	BOOL m_bUpdating;
	BOOL m_bEditorPreserved;
	CString m_EditorName;
	CEditLabel m_ceEditorName;
	SHP_DBFILE *m_pdbfile;

// Dialog Data
	enum { IDD = IDD_UPDATED };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg LRESULT OnCommandHelp(WPARAM,LPARAM);
};
