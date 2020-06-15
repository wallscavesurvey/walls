#pragma once


// CCompareFcnDlg dialog

class CCompareFcnDlg : public CDialog
{
	DECLARE_DYNAMIC(CCompareFcnDlg)

public:
	CCompareFcnDlg(LPCSTR pPath, LPCSTR pArgs, CWnd* pParent = NULL);   // standard constructor
	virtual ~CCompareFcnDlg();
	CString m_PathName;
	CString m_Arguments;

	// Dialog Data
	enum { IDD = IDD_COMPARE_FCN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedBrowse();
	DECLARE_MESSAGE_MAP()
};
