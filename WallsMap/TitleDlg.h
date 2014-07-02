#pragma once

#ifndef _EDITACCENT_H
#include "EditLabel.h"
#endif

// CTitleDlg dialog

class CTitleDlg : public CDialog
{
	DECLARE_DYNAMIC(CTitleDlg)

public:
	CTitleDlg(LPCSTR pTitle,LPCSTR pWinTitle=NULL,CWnd* pParent = NULL);   // standard constructor
	virtual ~CTitleDlg();

// Dialog Data
	enum { IDD = IDD_EDITTITLE };
	CString m_title;
	LPCSTR m_pWinTitle;
	BOOL m_bRO;

private:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	CEditLabel m_cea;
	virtual BOOL OnInitDialog();
};
