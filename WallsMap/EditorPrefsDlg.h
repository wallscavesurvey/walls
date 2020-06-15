#pragma once
#include "afxwin.h"
#ifndef _UTILITY_H
#include "utility.h"
#endif
#include "colorbutton.h"

// CEditorPrefsDlg dialog

class CEditorPrefsDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditorPrefsDlg)

public:
	CEditorPrefsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditorPrefsDlg();

	// Dialog Data
	enum { IDD = IDD_EDITOR_PREFS };

	BOOL m_bEditRTF;
	BOOL m_bEditToolbar;
	COLORREF m_clrBkg;
	COLORREF m_clrTxt;
	CLogFont m_font;

private:
	CColorCombo m_BtnTxt;
	CColorCombo m_BtnBkg;
	CStatic m_StaticFrame;
	CRect m_rectBox;
	CSize m_sizeBox;  // Size (width and height) of bitmap in pixels
	CSize m_szLabel;
	CBitmap m_cBmp;   // Bitmap with symbol
	CDC m_dcBmp;	 // Compatible Memory DC, with m_cBmp already selected
	HBITMAP m_hBmpOld;    // Handle of old bitmap to save
	HBRUSH m_hBrushOld;

	void GetLabelSize();
	void UpdateBox();
	void RefreshTextCompOptions();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg LRESULT OnChgColor(WPARAM, LPARAM);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnBnClickedFont();
	afx_msg void OnClickedTxtcomp();

	DECLARE_MESSAGE_MAP()
};
