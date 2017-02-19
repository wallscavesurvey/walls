#pragma once

#include "NiceSlider.h"
#include "EditLabel.h"

// CDlgExportImg dialog

class CDlgExportImg : public CDialog
{
	DECLARE_DYNAMIC(CDlgExportImg)

public:
	CDlgExportImg(CWallsMapView *pView,CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgExportImg();

// Dialog Data
	enum { IDD = IDD_EXPORTIMG };

	BOOL m_bOpenImage,m_bIncScale;

private:
	virtual BOOL OnInitDialog();
	CString m_csPathname;
	CEditLabel m_cePathName;
	//CString m_csWidth;
	CWallsMapView *m_pView;
	UINT m_uClientW,m_uClientH;
	CProgressCtrl m_Progress;
	CNiceSliderCtrl m_sliderRatio;
	UINT m_nRatio,m_uWidth;

	CWallsMapDoc *GetDocument() {return m_pView->GetDocument();}
	void Show(UINT id) {GetDlgItem(id)->ShowWindow(SW_SHOW);}
	void Hide(UINT id) {GetDlgItem(id)->ShowWindow(SW_HIDE);}
	void Disable(UINT id) {GetDlgItem(id)->EnableWindow(FALSE);}
	void Enable(UINT id) {GetDlgItem(id)->EnableWindow(TRUE);}
	void SetText(UINT id,LPCSTR text) {GetDlgItem(id)->SetWindowText(text);}
	void GetText(UINT id,CString &text) {GetDlgItem(id)->GetWindowText(text);}
	void InitWidth();
	void CreatePng(const CDIBWrapper &dib);
	void UpdateRatioLabel();
	void UpdateDenomLabel();
	UINT GetImageScaleDenom();

protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnEnChangeWidth();
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
