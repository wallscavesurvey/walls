#pragma once


// CResizeFrmDlg dialog

class CResizeFrmDlg : public CDialog
{
	DECLARE_DYNAMIC(CResizeFrmDlg)

public:
	CResizeFrmDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CResizeFrmDlg();

// Dialog Data
	enum { IDD = IDD_RESIZEFRAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	int m_iPx_per_in,m_iCurrentWidth;
	BOOL m_bSave;
	double m_dfltWidthInches;

};
