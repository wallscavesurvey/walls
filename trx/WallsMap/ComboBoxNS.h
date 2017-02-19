#pragma once

class CComboBoxNS : public CComboBox
{
	DECLARE_DYNAMIC(CComboBoxNS)

public:
	CComboBoxNS();
	virtual ~CComboBoxNS();

protected:
	BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
};


