#pragma once


// CResizeFrmDlg dialog

class CResizeFrmDlg : public CDialog
{
	DECLARE_DYNAMIC(CResizeFrmDlg)

public:
	CResizeFrmDlg(double *pWidth,int px_per_in,CWnd* pParent = NULL);   // standard constructor
	virtual ~CResizeFrmDlg();

// Dialog Data
	enum { IDD = IDD_RESIZEFRAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bSave;
	virtual BOOL OnInitDialog();
	CString m_InchWidth;
private:
	double *m_pWidth;
	int m_iPx_per_in;
	void UpdatePixels(char *buf,double f);
public:
	afx_msg void OnEnChangeInchwidth();
	afx_msg void OnDeltaposSpinInches(NMHDR *pNMHDR, LRESULT *pResult);
};
