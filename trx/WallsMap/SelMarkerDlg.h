#pragma once

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif
#include "colorbutton.h"
#include "NiceSlider.h"
#include "afxcmn.h"

// CSelMarkerDlg dialog

class CSelMarkerDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelMarkerDlg)

	CTabCtrl m_tabTypeMark;
	CColorCombo m_BtnMrk;
	CColorCombo m_BtnBkg;
	CColorCombo m_BtnLbl; //actually preview background

	CNiceSliderCtrl m_sliderSym;

	CToolTipCtrl m_ToolTips;

	CBitmap m_cBmp;   // Bitmap with symbol
	CDC m_dcBmp;	 // Compatible Memory DC, with m_cBmp already selected
	HBITMAP m_hBmpOld;    // Handle of old bitmap to save

    CRect m_rectBox;  // Bitmap's client-relative rectangle
	CSize m_sizeBox;  // Size (width and height) of bitmap in pixels
	CPoint m_ptCtrBox1,m_ptCtrBox2; //Center of box where symbol is drawn

	//Used for drawing symbol --
	CStatic	m_rectSym;

	CWallsMapDoc *m_pDoc;

	ID2D1DCRenderTarget *m_pRT; //will be bound to m_dcBmp
	ID2D1SolidColorBrush *m_pBR;

	SHP_MRK_STYLE *m_pmstyle;
	BOOL m_bSelected; //0, 1, or (if m_bGPS) maybe 3
	bool m_bActiveItems;
	bool m_bInitFlag;
	bool m_bGPS;

	void UpdateMap();
	void DrawSymbol();
	void SetTabbedCntrls();
	void UpdateOpacityLabel();

public:
	CSelMarkerDlg(bool bGPS=false,CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelMarkerDlg();

// Dialog Data

	//Accessed by caller: CMainFrame
	COLORREF m_clrPrev;
	SHP_MRK_STYLE m_mstyle_s,*m_pmstyle_s;
	SHP_MRK_STYLE m_mstyle_h,*m_pmstyle_h;
	SHP_MRK_STYLE m_mstyle_t; //gps trackline

	enum { IDD = IDD_SELMARKER };

protected:

	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void Show(UINT id,bool bShow) {GetDlgItem(id)->ShowWindow(bShow?SW_SHOW:SW_HIDE);}
	void SetText(UINT id,LPCSTR s) {GetDlgItem(id)->SetWindowText(s);}

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void UpdateMarkerSize(int inc0);
	afx_msg void OnMarkerSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeMarkerSize();

	void UpdateMarkerLineWidth(int inc0);
	afx_msg void OnMarkerLineWidth(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeMarkerLineWidth();

	afx_msg void OnSelShape();
	afx_msg void OnFilled();
	afx_msg void OnClear();
	afx_msg LRESULT OnChgColor(WPARAM,LPARAM);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnTabChanged(LPNMHDR pHdr, LRESULT* pResult);
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedRestoreDflt();

	DECLARE_MESSAGE_MAP()
};
