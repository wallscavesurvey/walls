#if !defined(AFX_GRADDLG_H__1D26758D_6AF4_4493_9124_E4F8E3BBF419__INCLUDED_)
#define AFX_GRADDLG_H__1D26758D_6AF4_4493_9124_E4F8E3BBF419__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GradDlg.h : header file
//

//#include "resource.h"
//#include "ColorPick.h"
#include "GradientCtrl.h"
#include "colorbutton.h"

/////////////////////////////////////////////////////////////////////////////
// CGradientDlg dialog

class CGradientDlg : public CDialog
{
// Construction
public:
	CGradientDlg(UINT id,COLORREF clr,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGradientDlg)
	enum { IDD = IDD_GRADIENT };
	CColorCombo	m_SelPegColor;
	int		m_iInterpMethod;
	double	m_NewPegPosition;
	double	m_SelPegPosition;
	//}}AFX_DATA

	//double m_NewPegPosition;

	CGradientCtrl m_wndGradientCtrl;
	BOOL m_bChanged;

private:
	CGradient *m_pGrad;
	int	m_iTyp;
	COLORREF m_clr;
	UINT m_id;
	int m_SelPegIndex;
	double m_fMinValue;
	double m_fMaxValue;
	BOOL m_bUpdating;
	BOOL m_bSelValid;
	CToolTipCtrl m_ToolTips;

	void Enable(int id,BOOL bEnable)
	{
	   GetDlgItem(id)->EnableWindow(bEnable);
	}
	
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGradientDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	CFileDialog * GetFileDialog(BOOL bSave);

protected:
	void SelectPeg(int idx);
	void AddAndSelectPeg(COLORREF color,double position);
	void UpdateSelPegText(int index);
	void UpdateFloatBox(UINT id,double value,int iTyp=-1);
	void UpdateValueBox(UINT id,double position);
	void ShowRangeError();
	void UpdateHistArray();
	int UpdateRangeEnd(int index,double fVal);

	BOOL ChangePosition(UINT idPos,UINT idVal,double &fNewPos);
	BOOL ChangeValue(UINT idVal,UINT idPos,double &fNewPos,int iIndex);

	// Generated message map functions
    afx_msg LONG OnSelEndOK(UINT lParam, LONG wParam);
	//{{AFX_MSG(CGradientDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeMethodCombo();
	afx_msg void OnAddPeg();
	afx_msg void OnDelPeg();
	afx_msg void OnChangeSelpegPosition();
	afx_msg void OnResetPegs();
	afx_msg void OnChangeNewpegPosition();
	afx_msg void OnChangeNewpegValue();
	afx_msg void OnChangeSelpegValue();
	afx_msg void OnKillfocusNewpegValue();
	afx_msg void OnKillfocusSelpegValue();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDefault();
	afx_msg void OnSaveAs();
	afx_msg void OnOpen();
	afx_msg void OnApply();
	//}}AFX_MSG
    afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
	afx_msg void OnNotifyChangeSelPeg(NMHDR * pNotifyStruct, LRESULT *result);
	afx_msg void OnNotifyPegMove(NMHDR * pNotifyStruct, LRESULT *result);
	afx_msg void OnNotifyPegRemoved(NMHDR * pNotifyStruct, LRESULT *result);
	afx_msg void OnNotifyDoubleClickCreate(NMHDR * pNotifyStruct, LRESULT *result);
	//afx_msg void OnNotifyEditPeg(NMHDR * pNotifyStruct, LRESULT *result);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRADDLG_H__1D26758D_6AF4_4493_9124_E4F8E3BBF419__INCLUDED_)
