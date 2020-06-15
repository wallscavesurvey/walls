#pragma once


// CHeaderCtrlH

class CHeaderCtrlH : public CHeaderCtrl
{
	DECLARE_DYNAMIC(CHeaderCtrlH)

public:
	CHeaderCtrlH();
	virtual ~CHeaderCtrlH();

protected:
	afx_msg LRESULT OnLayout(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};


