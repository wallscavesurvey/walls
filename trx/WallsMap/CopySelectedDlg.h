#pragma once

// CCopySelectedDlg dialog

class CResizeDlg;

class CCopySelectedDlg : public CDialog
{
	DECLARE_DYNAMIC(CCopySelectedDlg)

public:
	CCopySelectedDlg(CShpLayer *pLayer,LPBYTE pSrcFld, BOOL bConfirm, UINT nSelCount, CWnd* pParent = NULL);   // standard constructor

	virtual ~CCopySelectedDlg();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();


// Dialog Data
	enum { IDD = IDD_COPYSELECTED };

	BOOL ShowCopyProgress(UINT nCopied);

private:
	CShpLayer *m_pLayer;
	LPBYTE m_pSrcFlds;
	BOOL m_bConfirm;
	CResizeDlg *m_pDlg;
	UINT m_nSelCount;
	bool m_bWindowDisplayed;
	CProgressCtrl m_Progress;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg LONG OnWindowDisplayed(UINT, LONG);
	afx_msg void OnClose();
};
