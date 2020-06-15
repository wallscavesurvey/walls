#pragma once


// CImageZoomDlg dialog

class CImageZoomDlg : public CDialog
{
	DECLARE_DYNAMIC(CImageZoomDlg)

public:
	CImageZoomDlg(LPCSTR pTitle, CWnd* pParent = NULL);   // standard constructor
	virtual ~CImageZoomDlg();

	// Dialog Data
	enum { IDD = IDD_IMGZOOM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bZoomToAdded;
private:
	LPCSTR m_pTitle;
public:
	afx_msg void OnBnClickedNo();
	afx_msg void OnBnClickedYes();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};
