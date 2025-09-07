// mapdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapDlg dialog
#ifndef __SEGHIER_H
#include "seghier.h"
#endif

class CPlotView;

class CMapDlg : public CDialog
{
	DECLARE_DYNAMIC(CMapDlg)

public:
	CMapDlg(BOOL bPrinter, CWnd* pParent = NULL);	// standard constructor
	~CMapDlg();
	enum { IDD = IDD_MAPFORMAT };

	CString	m_FrameWidthInches;
	CString	m_FrameThick;
	CString	m_MarkerSize;
	CString	m_MarkerThick;
	CString	m_VectorThick;
	CString	m_OffLabelX;
	CString	m_OffLabelY;
	CString	m_TickLength;
	CString	m_TickThick;
	CString	m_FrameHeightInches;
	CString	m_LabelSpace;
	CString	m_FlagSize;
	CString	m_FlagThick;
	CString	m_VectorThin;
	CString	m_LabelInc;
	BOOL	m_bUsePageSize;
	int		m_iLabelType;
	int		m_nLblPrefix;

	MAPFORMAT *m_pMF;

protected:
	MAPFORMAT m_MF[2];
	static BOOL m_bConfirmedReset;
	BOOL m_bSwapping;
	BOOL m_bExport;
	BOOL m_bFlagChange;
	BOOL m_bMapsActive, m_bLineChg, m_bLabelChg, m_bApplyActive;

	CFont m_boldFont;

	UINT CheckData();
	void CMapDlg::InitData();
	void RefreshFrameSize();
	void InitControls();
	void SetSolidText();
	void Enable(int id, BOOL bEnable)
	{
		GetDlgItem(id)->EnableWindow(bEnable);
	}
	void SetText(UINT id, const char *text)
	{
		GetDlgItem(id)->SetWindowText(text);
	}
	void SetCheck(int id, BOOL bChecked)
	{
		((CButton *)GetDlgItem(id))->SetCheck(bChecked);
	}
	BOOL GetCheck(int id)
	{
		return ((CButton *)GetDlgItem(id))->GetCheck();
	}

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnFrameFont();
	afx_msg void OnLabelFont();
	afx_msg void OnNoteFont();
	afx_msg void OnSwapSets();
	afx_msg void OnUsePageSize();
	afx_msg void OnFlagSolid();

	afx_msg void OnLabelChg();
	afx_msg void OnLabelTypeChg(UINT id);
	afx_msg void OnLineChg();
	afx_msg void OnReset();
	afx_msg void OnApplyLbl();

	DECLARE_MESSAGE_MAP()
};
