/*****************************************************************************/
// File: kdws_properties.h [scope = APPS/WINSHOW]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: Mr David McKenzie
// License number: 00590
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
  Defines the `kdws_properties' class, which provides a dialog for
displaying JPEG2000 codestream properties in a browser window.
******************************************************************************/
#ifndef KDWS_PROPERTIES_H
#define KDWS_PROPERTIES_H

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#if !(defined _MSC_VER && (_MSC_VER >= 1300)) // If not .NET
#  define DWORD_PTR DWORD
#  define UINT_PTR kdu_uint32
#  define INT_PTR kdu_int32
#endif // Not .NET

#include "resource.h"       // main symbols
#include "kdu_compressed.h"

// Declared here:
class kdws_properties_help;
class kdws_properties;

/*****************************************************************************/
/*                            kdws_properties_help                           */
/*****************************************************************************/

class kdws_properties_help: public CDialog {
public:
  kdws_properties_help(const char *string, kdu_coords placement,
                       CWnd* pParent = NULL);
	enum { IDD = IDD_PROPERTYHELP };
protected:
	virtual BOOL OnInitDialog();
private:
  CEdit *get_edit() { return (CEdit *) GetDlgItem(IDC_MESSAGE); }
private:
  const char *string;
  kdu_coords placement;
protected:
	DECLARE_MESSAGE_MAP()
};


/*****************************************************************************/
/*                              kdws_properties                              */
/*****************************************************************************/

class kdws_properties : public CDialog {
public:
  kdws_properties(kdu_codestream codestream, const char *text,
                  CWnd* pParent=NULL);
	enum { IDD = IDD_PROPERTIESBOX };
protected:
  BOOL OnInitDialog();
	afx_msg void OnDblclkPropertiesList();
private:
  CListBox *get_list() { return (CListBox *) GetDlgItem(IDC_PROPERTIES_LIST); }
private: // Data
  kdu_codestream codestream;
  const char *text;
protected:
	DECLARE_MESSAGE_MAP()
};

#endif // KDWS_PROPERTIES_H
