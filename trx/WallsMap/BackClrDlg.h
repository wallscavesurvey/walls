#pragma once

#include "colorbutton.h"

// CBackClrDlg dialog

class CBackClrDlg : public CDialog
{
	DECLARE_DYNAMIC(CBackClrDlg)

public:
	CBackClrDlg(LPCSTR pTitle,COLORREF cr,CWnd* pParent = NULL);
	virtual ~CBackClrDlg();

// Dialog Data
	enum { IDD = IDD_BACKCOLOR };

	CColorCombo m_BtnBkg;
	CToolTipCtrl m_ToolTips;
	COLORREF m_cr;
	LPCSTR m_pTitle;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnChgColor(WPARAM,LPARAM);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
