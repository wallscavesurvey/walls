#pragma once


// CAbtBox dialog

class CAbtBox : public CDialog
{
	DECLARE_DYNAMIC(CAbtBox)

public:
	CAbtBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAbtBox();

// Dialog Data
	enum { IDD = IDD_ABTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
