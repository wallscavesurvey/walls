// ReadOnlyComboBox.cpp : implementation file
//

#include "stdafx.h"
#include "ROCombo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReadOnlyComboBox

CReadOnlyComboBox::CReadOnlyComboBox()
{
}

CReadOnlyComboBox::~CReadOnlyComboBox()
{
}


BEGIN_MESSAGE_MAP(CReadOnlyComboBox, CComboBox)
	//{{AFX_MSG_MAP(CReadOnlyComboBox)
	ON_WM_ENABLE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReadOnlyComboBox message handlers

void CReadOnlyComboBox::OnEnable(BOOL bEnable) 
{
	CComboBox::OnEnable(bEnable);
	
	CEdit*	pEdit = (CEdit*)GetWindow(GW_CHILD);
	pEdit->EnableWindow(TRUE);
	pEdit->SetReadOnly(!bEnable);
}
