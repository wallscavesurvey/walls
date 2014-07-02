// ComboBoxNS.cpp : implementation file
//

#include "stdafx.h"
#include "WallsMap.h"
#include "ComboBoxNS.h"

// CComboBoxNS

IMPLEMENT_DYNAMIC(CComboBoxNS, CComboBox)

CComboBoxNS::CComboBoxNS()
{

}

CComboBoxNS::~CComboBoxNS()
{
}

BOOL CComboBoxNS::PreTranslateMessage(MSG* pMsg)
{
      if (pMsg->message == WM_VSCROLL || pMsg->message == WM_MOUSEWHEEL)
      {
            return TRUE;
      }
      return CComboBox::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CComboBoxNS, CComboBox)
END_MESSAGE_MAP()
