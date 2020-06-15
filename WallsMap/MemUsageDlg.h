#pragma once


// CMemUsageDlg dialog

class CMemUsageDlg : public CDialog
{
	DECLARE_DYNAMIC(CMemUsageDlg)

public:
	CMemUsageDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMemUsageDlg();

	// Dialog Data
	enum { IDD = IDD_MEMUSAGEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
