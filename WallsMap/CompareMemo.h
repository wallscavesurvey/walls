#pragma once


// CCompareMemo dialog

class CCompareMemo : public CDialog
{
	DECLARE_DYNAMIC(CCompareMemo)

public:
	CCompareMemo(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCompareMemo();

// Dialog Data
	enum { IDD = IDD_COMPARE_MEMO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bRTF;
	CString m_PathSource;
	CString m_PathTarget;
private:
    UINT CheckPaths();
	afx_msg void OnClickedBrowseSrc();
	afx_msg void OnClickedBrowseTgt();
	virtual BOOL OnInitDialog();
};
