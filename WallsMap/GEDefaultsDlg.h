#pragma once
// CGEDefaultsDlg dialog

#include "EditLabel.h"

class CGEDefaultsDlg : public CDialog
{
	DECLARE_DYNAMIC(CGEDefaultsDlg)

public:
	CGEDefaultsDlg(LPCSTR pgePath, UINT uKmlRange,UINT uLabelSize,UINT uIconSize,CString &csIconSingle,CString &csIconMultiple,CWnd* pParent = NULL);   // standard constructor
	virtual ~CGEDefaultsDlg();

	UINT m_uKmlRange,m_uIconSize,m_uLabelSize;
	CEditLabel m_ceKmlRange;
	CEditLabel m_ceIconSingle,m_ceIconMultiple;
	CString m_csIconSingle,m_csIconMultiple;
	CString m_PathName;

// Dialog Data
	enum { IDD = IDD_GEDEFAULTSDLG };

private:
	void RefreshIconSize();
	void RefreshLabelSize();
	virtual void DoDataExchange(CDataExchange* pDX); 
	afx_msg void OnIconSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLabelSize(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedLoadDflts();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnClickedBrowse();
};
