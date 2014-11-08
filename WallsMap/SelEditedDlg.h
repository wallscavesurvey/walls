#pragma once


// CSelEditedDlg dialog

class CWallsMapView;

class CSelEditedDlg : public CDialog
{
	DECLARE_DYNAMIC(CSelEditedDlg)

public:
	CSelEditedDlg(CWallsMapView *pView,CWnd* pParent = NULL);   // standard constructor
	virtual ~CSelEditedDlg();

// Dialog Data
	enum { IDD = IDD_SELEDITED };

	UINT m_uFlags;
	BOOL m_bAddToSel;
	BOOL m_bViewOnly;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
	CWallsMapView *m_pView;
	BOOL m_bEditAdd;
	BOOL m_bEditDBF;
	BOOL m_bEditDel;
	BOOL m_bEditLoc;
	bool m_bAddEnabled;
public:
	virtual BOOL OnInitDialog();
};
