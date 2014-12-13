#pragma once

#ifndef _SHPLAYER_H
#include "ShpLayer.h"
#endif
#include "colorbutton.h"
#include "NiceSlider.h"
#include "afxwin.h"

class CShpLayer;
class CWallsMapDoc;
class CPageLayers;

class CDlgSymbols : public CDialog
{
	DECLARE_DYNAMIC(CDlgSymbols)

	CShpLayer *m_pShp;

	CColorCombo m_BtnMrk;
	CColorCombo m_BtnBkg;
	CColorCombo m_BtnBkgnd; //Preview background
	CColorCombo m_BtnLbl;
	//CToolTipCtrl m_ToolTips;
	CWallsMapDoc *m_pDoc;

	UINT m_nLblFld;
	COLORREF m_crLbl,m_clrPrev;
	CLogFont m_font;
	SHP_MRK_STYLE m_mstyle;
	BOOL m_bDotCenter;

	CNiceSliderCtrl m_sliderSym;

	CBitmap m_cBmp;   // Bitmap with symbol
	CDC m_dcBmp;	 // Compatible Memory DC, with m_cBmp already selected
	HBITMAP m_hBmpOld;    // Handle of old bitmap to save

    CRect m_rectBox;  // Bitmap's client-relative rectangle
	CSize m_sizeBox;  // Size (width and height) of bitmap in pixels
	CPoint m_ptCtrBox; //Center of box where symbol is drawn
	CStatic	m_rectSym;
	CSize m_szLabel;

	ID2D1DCRenderTarget *m_pRT; //will be bound to m_dcBmp

	void UpdateMap();
	bool UpdateSrchFlds();
	void GetLabelSize();
	void DrawSymbol();
	void InitLabelFields();

	void UpdateOpacityLabel();

public:
	CDlgSymbols(CWallsMapDoc *pDoc,CShpLayer *pLayer,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSymbols();

// Dialog Data
	enum { IDD = IDD_SYMBOLS };

private:

	bool m_bInitFlag;
	std::vector<BYTE> m_vSrchFlds;
	BOOL m_bSelectable;
	BOOL m_bSelectableSrch;
	BOOL m_bShowLabels;
	BOOL m_bShowMarkers;
	BOOL m_bTestMemos;

	CListBox m_list1;
	CListBox m_list2;

	virtual BOOL OnInitDialog();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void UpdateMarkerSize(int inc0);
	afx_msg void OnMarkerSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeMarkerSize();

	void UpdateMarkerLineWidth(int inc0);
	afx_msg void OnMarkerLineWidth(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeMarkerLineWidth();

	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedDotCenter();
	afx_msg void OnSelShape();
	afx_msg void OnSelField();
	afx_msg void OnFilled();
	afx_msg void OnClear();
	afx_msg void OnLblFont();

	afx_msg LRESULT OnChgColor(WPARAM,LPARAM);
	afx_msg LRESULT OnColorListbox(WPARAM wParam,LPARAM lParam);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnBnClickedMoveleft();
	afx_msg void OnBnClickedMoveright();
	afx_msg void OnListDblClk();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
