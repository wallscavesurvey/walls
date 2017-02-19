#if !defined(_DBFHEADER_H_)
#define _DBFHEADER_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXCMN_H__
#include <afxcmn.h>
#endif
/////////////////////////////////////////////////////////////////////////////
// CDBFHeader window

class CDBFHeader : public CHeaderCtrl
{
// Construction
public:
	CDBFHeader();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBFHeader)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDBFHeader();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDBFHeader)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif