#if !defined(AFX_MAGDLG_H__31CC7A40_58EE_11D1_B46B_BFAC75366B3B__INCLUDED_)
#define AFX_MAGDLG_H__31CC7A40_58EE_11D1_B46B_BFAC75366B3B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef __WALLMAG_H
#include "wallmag.h"
#endif

#include "linkedit.h"
#include "ResourceButton.h"
#include "afxwin.h"
//#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CMagDlg dialog

class CMagDlg : public CDialog
{
public:
	CMagDlg(CWnd* pParent, bool bVecInit = false);   // standard constructor
	CMagDlg();
	~CMagDlg();
	BOOL upd_UTM(int fcn);
	BOOL upd_DEG(int fcn);
	BOOL upd_ZONE(int fcn);
	BOOL upd_DECL(int fcn);
	char *m_pTitle;
	MAGDATA m_MD;

	BOOL IsDirEnabled() { return m_bDirEnabled; }

	bool ErrorOK(CLinkEdit *pEdit, CWnd* pNewWnd);

	//void UpdateMD(double *enu,DWORD date); //Called from CVecInfoDlg::OnMagDlg()

	// Dialog Data
	//{{AFX_DATA(CMagDlg)
	enum { IDD = IDD_MAGREF };
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagDlg)
protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
private:
	BOOL m_bValidDECL;
	BOOL m_bValidDEG;
	BOOL m_bValidUTM;
	BOOL m_bDisplayDEG;
	BOOL m_bDirEnabled;
	int m_iOutOfZone;

	bool m_bUPS;
	bool m_bModeless, m_bVecInit;

	CButton m_zoneInc, m_zoneDec;
	CResourceButton m_btnUTMUPS;

	CLinkEditI  m_zone, m_elev, m_year, m_month,
		m_latdeg, m_latmin,
		m_londeg, m_lonmin;

	CLinkEditF  m_latdegF, m_londegF,
		m_latminF, m_lonminF,
		m_latsecF, m_lonsecF,
		m_north, m_east;

	CSpinEdit  m_syear, m_smonth,
		m_slatdeg, m_slatmin, m_slatsec,
		m_slondeg, m_slonmin, m_slonsec;

	int m_MF(int f) { m_MD.fcn = f; return mag_GetData(&m_MD); }

	void IncZone(int dir);
	void EnableDIR(BOOL bEnable);
	void DisplayLatDir();
	void ShowZoneLetter();
	void DisplayLonDir();
	void DisplayDECL(BOOL bValid);
	void DisplayUTM(BOOL bValid);
	void DisplayZONE();
	void DisplayDEG(BOOL bValid);
	BOOL CvtD2UTM();
	void SetDegInvalid();
	void SetFormat(UINT id);
	void ToggleFormat(UINT id);

	void ShowErrMsg();
	void Show(UINT id, BOOL bShow) {
		GetDlgItem(id)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
	}
	void Enable(UINT id, BOOL bEnable) {
		GetDlgItem(id)->EnableWindow(bEnable);
	}

	void SetText(UINT id, LPCSTR text) {
		GetDlgItem(id)->SetWindowText(text);
	}

	void SetFlt(UINT id, double f, int dec) {
		SetText(id, GetFloatStr(f, dec));
	}

	void SetInt(UINT id, int i)
	{
		SetText(id, GetIntStr(i));
	}

	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg LRESULT OnCopyCoords(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPasteCoords(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CMagDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	virtual void OnCancel();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnFmtD();
	afx_msg void OnFmtDm();
	afx_msg void OnFmtDms();
	afx_msg void OnSelchangeDatum();
	afx_msg void OnSave();
	afx_msg void OnDiscard();
	afx_msg void OnLonDir();
	afx_msg void OnLatDir();
	afx_msg void OnEnterKey();
	afx_msg void OnBnClickedUtmUps();
	afx_msg void OnZoneInc();
	afx_msg void OnZoneDec();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CMagDlg *app_pMagDlg;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGDLG_H__31CC7A40_58EE_11D1_B46B_BFAC75366B3B__INCLUDED_)
