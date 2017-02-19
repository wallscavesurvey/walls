#if !defined(AFX_READONLYCOMBOBOX_H__11768645_6CBA_11D4_9754_00508B5ECE69__INCLUDED_)
#define AFX_READONLYCOMBOBOX_H__11768645_6CBA_11D4_9754_00508B5ECE69__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ReadOnlyComboBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReadOnlyComboBox window

class CReadOnlyComboBox : public CComboBox
{
// Construction
public:
	CReadOnlyComboBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReadOnlyComboBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CReadOnlyComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CReadOnlyComboBox)
	afx_msg void OnEnable(BOOL bEnable);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_READONLYCOMBOBOX_H__11768645_6CBA_11D4_9754_00508B5ECE69__INCLUDED_)
