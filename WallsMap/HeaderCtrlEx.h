#ifndef _HEADERCTRLEX_H
#define _HEADERCTRLEX_H
// A simple extension to the header class to allow multi-line headers
class CHeaderCtrlEx : public CHeaderCtrl
{
public:
	CHeaderCtrlEx() : m_nSortCol(-1), m_pCellFont(NULL) {}
	virtual ~CHeaderCtrlEx() {}
	CFont *m_pCellFont;
	int m_nSortCol;
	bool m_bSortAsc;
	int SetSortImage(int nCol,bool bAsc);

protected:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//virtual int OnToolHitTest(CPoint point, TOOLINFO * pTI) const;
	//virtual BOOL PreTranslateMessage(MSG* pMsg);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnLayout( WPARAM wParam, LPARAM lParam );
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//afx_msg BOOL OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnTabletQuerySystemGestureStatus(WPARAM,LPARAM);
};
#endif
