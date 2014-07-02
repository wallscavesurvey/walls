#pragma once

#ifndef _SHPLAYER_H
//#include "ShpLayer.h"
#endif
#include "colorbutton.h"
#include "NiceSlider.h"

class CShpLayer;
class CWallsMapDoc;
class CPageLayers;

// CDlgSymbolsPoly dialog

class CDlgSymbolsPoly : public CDialog
{
	DECLARE_DYNAMIC(CDlgSymbolsPoly)

	CShpLayer *m_pLayer;

	CColorCombo m_BtnMrk;
	CColorCombo m_BtnBkg;
	CColorCombo m_BtnLbl;
	//CToolTipCtrl m_ToolTips;
	CWallsMapDoc *m_pDoc;

	UINT m_nLblFld;
	COLORREF m_crLbl;
	CLogFont m_font;
	SHP_MRK_STYLE m_mstyle;

	CNiceSliderCtrl m_sliderVec;
	CNiceSliderCtrl m_sliderSym;
	int m_iOpacitySym;
	int m_iOpacityVec;
	BOOL m_bAntialias;
	bool m_bInitFlag;
	void UpdateMap();
	void InitLabelFields();


public:
	CDlgSymbolsPoly(CWallsMapDoc *pDoc,CShpLayer *pLayer,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSymbolsPoly();

// Dialog Data
	enum { IDD = IDD_SYMBOLS_POLY };

private:
	BOOL m_bShowLabels;
	BOOL m_bUseIndex;
	UINT m_uIdxDepth;
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void RefreshIdxDepth();
	void UpdateLineSize(int inc0);
	void Hide(UINT id) { GetDlgItem(id)->ShowWindow(SW_HIDE); }
	void Enable(UINT id,BOOL bEnable) {	GetDlgItem(id)->EnableWindow(bEnable); }

	afx_msg void OnLineSize(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnIdxDepth(NMHDR* pNMHDR, LRESULT* pResult) ;
	afx_msg void OnEnChangeLineSize();
	afx_msg void OnBnClickedApply();
	afx_msg void OnSelField();
	afx_msg void OnFilled();
	afx_msg void OnClear();
	afx_msg void OnLblFont();
	afx_msg LRESULT OnChgColor(WPARAM,LPARAM);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedUseIndex();
};
