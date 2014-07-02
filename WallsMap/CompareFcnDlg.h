#pragma once


// CCompareFcnDlg dialog

class CCompareFcnDlg : public CDialog
{
	DECLARE_DYNAMIC(CCompareFcnDlg)

public:
	CCompareFcnDlg(LPCSTR pPath, LPCSTR pArgs, CWnd* pParent = NULL);   // standard constructor
	virtual ~CCompareFcnDlg();

// Dialog Data
	enum { IDD = IDD_COMPARE_FCN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	CString m_PathName;
	CString m_Arguments;
	afx_msg void OnClickedBrowse();
};
