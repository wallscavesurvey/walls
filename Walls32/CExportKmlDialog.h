#pragma once

// CExportKmlDialog dialog

#include "stdafx.h"
#include "resource.h"

class CExportKmlDialog : public CDialog
{
public:
	CExportKmlDialog(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CExportKmlDialog();

	BOOL m_bFlags;
	BOOL m_bNotes;
	BOOL m_bStations;
	BOOL m_bVectors;
	BOOL m_bWalls;
	CString m_pathname;

	// Dialog Data
		//{{AFX_DATA(CExportKmlDialog)
	enum { IDD = IDD_EXPORT_KML_DIALOG };
	//}}AFX_DATA

		//{{AFX_VIRTUAL(CExpSvgDlg)
protected:
	UINT m_uTypes;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	void Browse(UINT id);

	//{{AFX_MSG(CExportKmlDialog)
	afx_msg void OnBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
