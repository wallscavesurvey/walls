#pragma once


// CReloadDlg dialog

class CReloadDlg : public CDialog
{
	DECLARE_DYNAMIC(CReloadDlg)

public:
	CReloadDlg(LPCSTR pTitle,int nSubset,int nDeleted,BOOL bRestoreLayout,CWnd* pParent = NULL);   // standard constructor
	virtual ~CReloadDlg();

// Dialog Data
	enum { IDD = IDD_RELOAD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	int m_nDeleted;
	LPCSTR m_pTitle;
public:
	int m_nSubset;
	virtual BOOL OnInitDialog();
	BOOL m_bRestoreLayout;
};
