#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "colorbutton.h"
#include "afxwin.h"

// CDlgSymbolsImg dialog
class CImageLayer;
class CWallsMapDoc;

class CDlgSymbolsImg : public CDialog
{
	DECLARE_DYNAMIC(CDlgSymbolsImg)

public:
	CDlgSymbolsImg(CImageLayer *pLayer, CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSymbolsImg();

	enum { IDD = IDD_SYMBOLS_IMG };

    void NewLayer(CImageLayer *pLayer=NULL);

	CImageLayer *m_pLayer;
   
	DECLARE_MESSAGE_MAP()

private:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	void UpdateOpacityLabel();
	LRESULT OnChgColor(WPARAM clr,LPARAM id);


	BYTE m_bAlphaSav;
	bool m_bUseTransColorSav;
	bool m_bPreview,m_bHiddenChg;
	COLORREF m_crTransColorSav;
	int m_iLastToleranceSav;

	int m_bTolerance;
	CColorCombo m_BtnTransClr;
	CSliderCtrl m_sliderOpacity;
	int m_iOpacity;
	BOOL m_bMatchSimilarColors;
	COLORREF m_crTransColor;
	BOOL m_bUseTransColor;

	void SetHiddenChg()
	{
		if(!m_bHiddenChg) {
			m_bHiddenChg=true;
			GetDlgItem(IDOK)->EnableWindow(1);
		}
	}

	void InitData(CImageLayer *pLayer);
	void RestoreData();
	void UpdateDoc();
	BOOL IsLayerUpdated();
	bool IsDialogUpdated();

	virtual void PostNcDestroy();

	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedAccept();
	afx_msg void OnBnClickedUseTransclr();
	afx_msg void OnBnClickedMakeSimilar();
	afx_msg void OnBnClickedTolerance(UINT id);
	afx_msg void OnBnClickedMaxOpacity();
	afx_msg	LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

public:
	afx_msg void OnClose();
private:
	CButton m_Preview;
public:
	afx_msg void OnBnClickedPreview();
};	
