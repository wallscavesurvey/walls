#if !defined(AFX_MARKERDLG_H__5773D1B7_44E4_4109_8585_57704C0F64EB__INCLUDED_)
#define AFX_MARKERDLG_H__5773D1B7_44E4_4109_8585_57704C0F64EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MarkerDlg.h : header file
//

#include "colorbutton.h"

/////////////////////////////////////////////////////////////////////////////
// CMarkerDlg dialog
#pragma pack(1)
typedef struct {
	WORD flgidx; //m_pFS[m_pFS[i].flgidx].lstidx=i
	WORD lstidx; //index in listbox (priority)
	DWORD color; //initially position in ixSEG
	WORD style;  //seg_id in NTA_NOTEREC
	WORD count;
} MK_FLAGSTYLE;

#pragma pack()

class CMarkerDlg : public CDialog
{
// Construction
public:
	CMarkerDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMarkerDlg)
	enum { IDD = IDD_MARKERDLG };
	CStatic	m_rectSym;
	BOOL	m_bNoOverlap;
	BOOL	m_bHideNotes;
	//}}AFX_DATA

	COLORREF m_bkgcolor,m_markcolor;
	HBRUSH m_hbkbr;
	BOOL m_bChanged;
	WORD m_markstyle;

private:
	MK_FLAGSTYLE *m_pFS;
	CToolTipCtrl m_ToolTips;
	CColorCombo m_colorbtn;
	CColorCombo m_bkgndbtn;

	CBitmap m_cBmp;   // Bitmap with symbol
	CDC m_dcBmp;	  // Compatible Memory DC, with m_cBmp already selected
    CRect m_rectBox;  // Bitmap's client-relative rectangle
	CSize m_sizeBox;  // Size (width and height) of bitmap in pixels
	HBITMAP m_hBmpOld;    // Handle of old bitmap to save
	HBRUSH m_hBrushOld;   // Handle of old brush to save
	int m_lastCaret;

	void DrawSymbol(void);
	void InitFlagNames(void);
	void UpdateFlagStyles(void);
	BOOL SetFlagStyle(int iVal,int type);
	void ChangePriority(int dir);
	BOOL CheckSizeCtrl();
	void EnableButtons(int sel);
	//void UpdateDisableButton(int i);
	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void DeselectAllBut(int p,int s);
	void SelectRange(int i,int j);

	BOOL IsListItemEnabled(int i)
	{
		return !M_ISHIDDEN(m_pFS[m_pFS[i].flgidx].style);
	}

	void EnableListItem(int i,BOOL bEnable)
	{
		WORD *pStyle=&m_pFS[m_pFS[i].flgidx].style;
		M_SETHIDDEN(*pStyle,!bEnable);
	}

	BOOL IsMarkerEnabled(int i)
	{
		return !M_ISHIDDEN(m_pFS[i].style);
	}

	void EnableMarker(int i,BOOL bEnable)
	{
		WORD *pStyle=&m_pFS[i].style;
		M_SETHIDDEN(*pStyle,!bEnable);
	}

	COLORREF ListItemRGB(int i)
	{
		return M_COLORRGB(m_pFS[m_pFS[i].flgidx].color); 
	}

	void SetListItemColor(int i,COLORREF rgb);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMarkerDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
    afx_msg void OnFlagSelChg();
	afx_msg LRESULT OnChgColor(WPARAM,LPARAM);
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CMarkerDlg)
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnPriorUp();
	afx_msg void OnPriorDown();
	afx_msg void OnSolid();
	afx_msg void OnSelShape();
	afx_msg void OnOpaque();
	afx_msg void OnClear();
	afx_msg void OnDeltaposSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillfocusSize();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnDisable();
	afx_msg void OnTransparent();
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	afx_msg void OnEnable();
	afx_msg void OnSetSelected();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MARKERDLG_H__5773D1B7_44E4_4109_8585_57704C0F64EB__INCLUDED_)
