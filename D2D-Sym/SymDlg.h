#pragma once

#include "resource.h"
#include "colorbutton.h"
//#include "afxcmn.h"
#include "NiceSlider.h"
//#include "afxwin.h"

// CSymDlg dialog

class CD2DSymView;

class CSymDlg : public CDialog
{
	DECLARE_DYNAMIC(CSymDlg)

public:
	CSymDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSymDlg();

	// Dialog Data
	enum { IDD = IDD_SYMBOLOGY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnChgColor(WPARAM, LPARAM);
	afx_msg void OnSymSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOutSize(NMHDR* pNMHDR, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()


public:
	CD2DSymView *m_pView;
	int m_iSymTyp;
	BOOL m_bDotCenter;
	BOOL m_bAntialias;
	BOOL m_bBackSym;
	int m_iSymSize;
	double m_fLineSize;
	double m_fVecSize;
	CColorCombo m_BtnSym;
	CColorCombo m_BtnLine;
	CColorCombo m_BtnVec;
	CColorCombo m_BtnBack;
	COLORREF m_clrSym;
	COLORREF m_clrLine;
	COLORREF m_clrVec;
	COLORREF m_clrBack;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedApply();
	CNiceSliderCtrl m_sliderSym;
	CNiceSliderCtrl m_sliderVec;
	int m_iOpacitySym;
	BOOL m_bGDI;
	int m_iVecCountIdx;
	int m_iOpacityVec;

private:
	CSpinButtonCtrl m_Spin_Outline;
	CSpinButtonCtrl m_Spin_Vecsize;
	CComboBox m_cbVecCount;

	afx_msg void OnDestroy();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
public:
	afx_msg void OnCbnSelchangeVecCount();
};
