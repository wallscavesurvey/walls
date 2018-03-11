/*****************************************************************************/
// File: kdws_url_dialog.h [scope = APPS/WINSHOW]
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
  Defines the `kdws_url_dialog' class, which provides a dialog for
entering JPIP URL's and associated attributes for remote browsing.
******************************************************************************/
#ifndef KDWS_URL_DIALOG_H
#define KDWS_URL_DIALOG_H

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
#include "kdws_manager.h"

// Declared here:
class kdws_url_dialog;

// Declared elsewhere:
class kdws_settings;
class kdws_frame_window;

#define KD_URL_DIALOG_MAX_TOOLTIP_CHARS 4096

/*****************************************************************************/
/*                              kdws_url_dialog                              */
/*****************************************************************************/

class kdws_url_dialog: public CDialog {
  public:
    kdws_url_dialog(kdws_settings *settings, kdws_frame_window *window);
    ~kdws_url_dialog()
      {
        if (request_string != NULL) delete request_string;
        if (server_name != NULL) delete server_name;
        if (url != NULL) delete[] url;
      }
    const char *get_url();
      /* Retrieves a string which holds a complete URL representing the
         information fetched via the dialog.  If the dialog was cancelled
         or has not yet completed, this function may return NULL. */
	  enum { IDD = IDD_JPIP_OPEN };
  private: // Base object overrides
    BOOL OnInitDialog();
    void OnOK();
  private: // Control access
    CEdit *get_server()
      { return (CEdit *) GetDlgItem(IDC_JPIP_SERVER); }
    CEdit *get_request()
      { return (CEdit *) GetDlgItem(IDC_JPIP_REQUEST); }
    CComboBox *get_channel_type()
      { return (CComboBox *) GetDlgItem(IDC_JPIP_CHANNEL_TYPE); }
    CEdit *get_proxy()
      { return (CEdit *) GetDlgItem(IDC_JPIP_PROXY); }
    CEdit *get_cache_dir()
      { return (CEdit *) GetDlgItem(IDC_JPIP_CACHE_DIR); }
    CButton *force_interactive_button()
      { return (CButton *) GetDlgItem(IDC_JPIP_FORCE_INTERACTIVE); }
  private:
    kdws_frame_window *window;
    kdws_string *request_string;
    kdws_string *server_name;
    char *url;
    kdws_settings *settings;
    _TCHAR tooltip_buffer[KD_URL_DIALOG_MAX_TOOLTIP_CHARS+1];
  protected:
    afx_msg void get_tooltip_dispinfo(NMHDR* pNMHDR, LRESULT *pResult);
	  DECLARE_MESSAGE_MAP()
  };

#endif // KDWS_URL_DIALOG_H
