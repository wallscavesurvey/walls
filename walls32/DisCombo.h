// DisabledCombo.h : header file
//

class CDisabledCombo;

/////////////////////////////////////////////////////////////////////////////
// CListBoxInsideComboBox window

class CListBoxInsideComboBox : public CWnd
{
	// Construction
public:
	CListBoxInsideComboBox();

	// Attributes
public:
	CDisabledCombo *m_Parent;
	void SetParent(CDisabledCombo *);

	// Operations
public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CListBoxInsideComboBox)
		//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CListBoxInsideComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CListBoxInsideComboBox)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CDisabledCombo window

class CDisabledCombo : public CComboBox
{
	// Construction
public:
	CDisabledCombo();

	// Attributes
public:
	// default implementation uses LSB of item data; override to get other behaviour
	virtual BOOL IsItemEnabled(UINT) const;

protected:
	CString m_strSavedText;	// saves text between OnSelendok and OnRealSelEndOK calls
	CListBoxInsideComboBox m_ListBox;

	// Operations
public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CDisabledCombo)
public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
protected:
	virtual void PreSubclassWindow();
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDisabledCombo();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDisabledCombo)
	afx_msg int OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex);
	afx_msg void OnSelendok();
	//}}AFX_MSG
	afx_msg void RecalcDropWidth();

	LRESULT OnCtlColor(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnRealSelEndOK(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
