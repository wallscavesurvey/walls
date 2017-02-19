#pragma once

#include "ResizeDlg.h"
#include "FileDropTarget.h"
#include "LayerSetPage.h"

class CLayerSetSheet;

extern BOOL hPropHook;
extern CLayerSetSheet *pLayerSheet;
extern const UINT WM_PROPVIEWDOC;

class CWallsMapDoc;

//==========================================
// CPageDetails dialog

class CPageDetails : public CLayerSetPage
{
	//DECLARE_DYNCREATE(CPageDetails)

public:
	CPageDetails();
	virtual ~CPageDetails();
	virtual void UpdateLayerData(CWallsMapDoc *pDoc);
	void SetNewLayer() {m_bNewLayer=true;}

// Dialog Data
	enum { IDD = IDD_PAGE_DETAILS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	void OnClose();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg LRESULT OnRetFocus(WPARAM,LPARAM);
	afx_msg LRESULT OnCommandHelp(WPARAM wNone, LPARAM lParam);

private:
	bool m_bNewLayer;
	int m_nPos;
};
