#pragma once
#include "afxwin.h"


// CGPSOptionsDlg dialog

#define GPS_NUM_CONFIRMCNTS 6

class CGPSOptionsDlg : public CDialog
{
	DECLARE_DYNAMIC(CGPSOptionsDlg)

public:
	CGPSOptionsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGPSOptionsDlg();

	// Dialog Data
	enum { IDD = IDD_GPS_OPTIONS };

	int m_nConfirmCnts[GPS_NUM_CONFIRMCNTS];
	int m_iConfirmCnt;
	BOOL m_bAutoRecordStart;
	BOOL m_bCenterPercent;
	CComboBox m_cbConfirmCnt;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
