// Copyright (C) 1992-1997 Microsoft Corporation
// All rights reserved.
//
#if !defined(_WBCGID_H_)
#define _WBCGID_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


//#ifndef CGID_IWebBrowser
// CGID_WebBrowser: {ED016940-BD5B-11cf-BA4E-00C04FD70816}
DEFINE_GUID(CGID_IWebBrowser,0xED016940L,0xBD5B,0x11cf,0xBA,0x4E,0x00,0xC0,0x4F,0xD7,0x08,0x16);
//#endif

//
// Utility class for the WebBrowser Command Group
class CWBCmdGroup
{
public:
   // CommandTarget ids for menu driving
   enum 
   {
      HTMLID_FIND         = 1,
      HTMLID_VIEWSOURCE   = 2,
      HTMLID_OPTIONS      = 3,
   };
};

#endif // _WBCGID_H_
