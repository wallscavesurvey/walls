#pragma once


// CPercentDlg dialog

#include "EditLabel.h"

class CPercentDlg : public CDialog
{
	DECLARE_DYNAMIC(CPercentDlg)

	// Dialog Data
	enum { IDD = IDD_PERCENT_ZOOM };

	double m_fPercent;
	CString m_csPercent;
	CEditLabel m_cePercent;

public:
	CPercentDlg(double percent, CWnd* pParent = NULL);   // standard constructor
	virtual ~CPercentDlg();


protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
