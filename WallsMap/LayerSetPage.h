#pragma once

#include "ResizeDlg.h"
#include "FileDropTarget.h"
#include "ColumnTreeCtrl.h"
#include "LayerSetSheet.h"

extern BOOL hPropHook;
extern const UINT WM_PROPVIEWDOC;

class CLayerSetSheet;
class CWallsMapDoc;

class CLayerSetPage : public CPropertyPage
{
public:
	CLayerSetPage(UINT nIDTemplate) : CPropertyPage(nIDTemplate)
	{
		m_xSt = CST_RESIZE;
		m_xSt = CST_RESIZE;
		m_xMin = m_yMin = 32;
		m_bNeedInit=TRUE;
	}

	~CLayerSetPage() {}

	CLayerSetSheet *Sheet();
	CWallsMapDoc *GetDoc();

	virtual void UpdateLayerData(CWallsMapDoc *pDoc)=0;

	//For resizing ..
	std::vector<CItemCtrl>	m_Items;           // array of controlled items
	CRect					m_cltRect, m_cltR0;
	int						m_xMin, m_yMin;
	int						m_xSt,  m_ySt;

	CWallsMapDoc			*m_pDoc;

	BOOL					m_bDocChanged;
	BOOL					m_bNeedInit;

	void Enable(UINT id,BOOL bEnable) {GetDlgItem(id)->EnableWindow(bEnable);}
	void SetText(UINT id,LPCSTR pText) {GetDlgItem(id)->SetWindowText(pText);}

    void Show(UINT idc,BOOL bShow)
    {
    	GetDlgItem(idc)->ShowWindow(bShow?SW_SHOW:SW_HIDE);
    }

// Overrides

protected:
	void 					AddControl( UINT nID, int xl, int xr, int yt, int yb, int bFlickerFree = 0, 
									    double xRatio = -1.0, double cxRatio = -1.0,
									    double yRatio = -1.0, double cyRatio = -1.0 );
	virtual BOOL			OnInitDialog();
	virtual BOOL			OnSetActive();
	//virtual BOOL			OnKillActive();

	UINT m_idLastFocus;

	afx_msg void			OnSize(UINT nType, int cx, int cy);
	afx_msg void			OnGetMinMaxInfo(MINMAXINFO *pmmi);
	afx_msg BOOL			OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};



