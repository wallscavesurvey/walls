#pragma once
#include "afxwin.h"
#include "EditLabel.h"

class CCenterViewDlg : public CDialog
{
	DECLARE_DYNAMIC(CCenterViewDlg)

// Dialog Data
	enum { IDD = IDD_CENTER_VIEW };

public:
	CCenterViewDlg(const CFltPoint &fpt,CWnd* pParent = NULL);   // standard constructor
	virtual ~CCenterViewDlg();

	BOOL m_bMark;
	bool m_bZoom;
	CFltPoint m_fpt,m_fptOrg;

private:
	CEditLabel m_ceEast;
	CEditLabel m_ceNorth;

	double m_f0,m_f1;

	CWallsMapView *m_pView;
	CWallsMapDoc *m_pDoc;

	int m_iZoneSel;	//Zone of selection (0 if Lat/Long)
	int m_iZoneDoc; //Zone of view
	INT8 m_iNadSel; //Datum setting of selection (0 if unsupported)
	INT8 m_iNadDoc; //Datum setting of view (m_points)
	bool m_bNadToggle,m_bValidEast,m_bValidNorth,m_bValidZone,m_bUtmDisplayed;
	bool m_bInitFlag,m_bOutOfZone;

	BOOL IsEnabled(UINT id) {return GetDlgItem(id)->IsWindowEnabled();}
	void Disable(UINT id) {GetDlgItem(id)->EnableWindow(FALSE);}
	void Enable(UINT id) {GetDlgItem(id)->EnableWindow(TRUE);}
	void Enable(UINT id,bool bEnable) {GetDlgItem(id)->EnableWindow(bEnable==true);}
	void SetFocus(UINT id) {GetDlgItem(id)->SetFocus();}
	void SetText(UINT id,LPCSTR text) {m_bInitFlag=true;GetDlgItem(id)->SetWindowText(text);m_bInitFlag=false;}
	void Hide(UINT id) {GetDlgItem(id)->ShowWindow(SW_HIDE);}
	void Show(UINT id) {GetDlgItem(id)->ShowWindow(SW_SHOW);}
	void Show(UINT id,bool bShow) {GetDlgItem(id)->ShowWindow(bShow?SW_SHOW:SW_HIDE);}

	bool InitFormat();
	bool IsLocationValid() {return m_bValidNorth && m_bValidEast && m_bValidZone;}
	void OnEnChangeEditCoord(bool bValid);
	void SetCoordLabels();
	bool SetCoordFormat();
	void FixZoneMsg(int zone);
	void SetOutOfZone();
	void RefreshCoordinates(const CFltPoint &fpt);
	void GetEditedPoint(CFltPoint &fpt);

	int GetTrueZone();

	void SetCheck(UINT id,BOOL bChk)
	{
		((CButton *)GetDlgItem(id))->SetCheck(bChk);
	}

	bool IsChecked(UINT id)
	{
		return ((CButton *)GetDlgItem(id))->GetCheck()==BST_CHECKED;
	}

	void SetRO(UINT id,BOOL bRO)
	{
		((CEdit *)GetDlgItem(id))->SetReadOnly(bRO);
	}

	void GetText(UINT id,CString &s)
	{
		GetDlgItem(id)->GetWindowText(s);
	}
	void GetText(UINT id,LPSTR buf,UINT sizBuf)
	{
		GetDlgItem(id)->GetWindowText(buf,sizBuf);
	}

//==============================================

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedZoom4x();
	afx_msg void OnEnChangeEditNorth();
	afx_msg void OnEnChangeEditEast();
	afx_msg void OnBnClickedCopyloc();
	afx_msg void OnBnClickedPasteloc();
	afx_msg void OnBnClickedUndoEdit();
	afx_msg void OnBnClickedTypegeo();
	afx_msg void OnBnClickedDatum();
	afx_msg void OnSpinUtm(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);
};
