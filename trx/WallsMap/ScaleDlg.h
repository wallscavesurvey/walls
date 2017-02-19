#pragma once
#include "EditLabel.h"

class CWallsMapDoc;
class CMapLayer;

// CScaleDlg dialog

class CScaleDlg : public CDialog
{
	DECLARE_DYNAMIC(CScaleDlg)

public:
	CScaleDlg(CWallsMapDoc *pDoc,CWnd* pParent = NULL);   // standard constructor
	virtual ~CScaleDlg();

// Dialog Data
	enum { IDD = IDD_SCALERANGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	PTL m_pFolder;

	virtual BOOL OnInitDialog();
	bool IsChecked(UINT id)
	{
		return ((CButton *)GetDlgItem(id))->GetCheck()==BST_CHECKED;
	}
	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void EnableLayerRange();
	void EnableLabelRange();
	bool SetChildRanges(PML ptl,CScaleRange &range);

	CMapLayer *m_pLayer;
	CWallsMapDoc *m_pDoc;
	BOOL m_bLabelShowAll;
	BOOL m_bLayerShowAll;
	CString m_csLabelZoomIn;
	CString m_csLabelZoomOut;
	CString m_csLayerZoomIn;
	CString m_csLayerZoomOut;
	CEditLabel m_ceLabelZoomIn;
	CEditLabel m_ceLabelZoomOut;
	CEditLabel m_ceLayerZoomIn;
	CEditLabel m_ceLayerZoomOut;
public:
	afx_msg void OnBnClickedLabelNoshow();
	afx_msg void OnBnClickedLayerNoshow();
	afx_msg	LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};
