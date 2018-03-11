/*****************************************************************************/
// File: kdws_url_dialog.cpp [scope = APPS/WINSHOW]
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
  Implements the `kdws_url_dialog' class, which provides a dialog for
entering JPIP URL's and associated parameters for remote browsing.
******************************************************************************/

#include "stdafx.h"
#include "kdu_utils.h"
#include "kdcs_comms.h"
#include "kdws_url_dialog.h"
#include "kdws_window.h"


/* ========================================================================= */
/*                              kdws_url_dialog                              */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdws_url_dialog::kdws_url_dialog                      */
/*****************************************************************************/

kdws_url_dialog::kdws_url_dialog(kdws_settings *settings,
                                 kdws_frame_window *window)
	: CDialog(kdws_url_dialog::IDD, window)
{
  this->window = window;
  request_string = NULL;
  server_name = NULL;
  url = NULL;
  this->settings = settings;
}

/*****************************************************************************/
/*                        kdws_url_dialog::OnInitDialog                      */
/*****************************************************************************/

BOOL
  kdws_url_dialog::OnInitDialog()
{
  kdws_string server_name(settings->get_jpip_server());
  kdws_string proxy_name(settings->get_jpip_proxy());
  kdws_string request(settings->get_jpip_request());
  kdws_string channel_type(settings->get_jpip_channel_type());
  kdws_string cache_dir_name(settings->get_jpip_cache());
  kdu_client_mode mode = settings->get_jpip_client_mode();

  get_server()->SetWindowText(server_name);
  get_proxy()->SetWindowText(proxy_name);
  get_request()->SetWindowText(request);
  get_cache_dir()->SetWindowText(cache_dir_name);
  force_interactive_button()->SetCheck(
    (mode == KDU_CLIENT_MODE_INTERACTIVE)?BST_CHECKED:BST_UNCHECKED);
  const char **scan;
  for (scan=jpip_channel_types; *scan != NULL; scan++)
    get_channel_type()->AddString(kdws_string(*scan));
  scan = jpip_channel_types;
  if (channel_type.is_empty())
    get_channel_type()->SelectString(-1,kdws_string(*scan));
  else
    get_channel_type()->SelectString(-1,channel_type);
  if (server_name.is_empty())
    GotoDlgCtrl(GetDlgItem(IDC_JPIP_SERVER));
  else
    GotoDlgCtrl(GetDlgItem(IDC_JPIP_REQUEST));
  EnableToolTips(TRUE);
  return 0;
}

/*****************************************************************************/
/*                          kdws_url_dialog::OnOK                            */
/*****************************************************************************/

void
  kdws_url_dialog::OnOK()
{
  int len, max_string_len = 10;
  if ((len = get_server()->LineLength()) > max_string_len)
    max_string_len = len;
  else if (len < 1)
    {
      window->interrogate_user("You must enter a server name or IP address!",
                               "Kdu_show: Dialog entry error",
                               MB_OK|MB_ICONERROR);
      GotoDlgCtrl(GetDlgItem(IDC_JPIP_SERVER));
      return;
    }

  if ((len = get_proxy()->LineLength()) > max_string_len)
    max_string_len = len;

  if ((len = get_request()->LineLength()) > max_string_len)
    max_string_len = len;
  else if (len < 1)
    {
      window->interrogate_user("You must enter an object (e.g., file name) "
                               "to be served!",
                               "Kdu_show: Dialog entry error",
                               MB_OK|MB_ICONERROR);
      GotoDlgCtrl(GetDlgItem(IDC_JPIP_REQUEST));
      return;
    }

  if ((len = get_cache_dir()->LineLength()) > max_string_len)
    max_string_len = len;

  int cb_idx = get_channel_type()->GetCurSel();
  if ((cb_idx == CB_ERR) ||
      ((len = get_channel_type()->GetLBTextLen(cb_idx)) < 1))
    {
      window->interrogate_user("You must select a channel type\n"
                               "(or \"none\" for stateless communications)!",
                               "Kdu_show: Dialog entry error",
                               MB_OK|MB_ICONERROR);
      GotoDlgCtrl(GetDlgItem(IDC_JPIP_CHANNEL_TYPE));
      return;
    }
  else if (len > max_string_len)
    max_string_len = len;
  request_string = new kdws_string(max_string_len);
  server_name = new kdws_string(max_string_len);

  len = get_server()->GetLine(0,*server_name,max_string_len);
  server_name->set_strlen(len);
  settings->set_jpip_server(*server_name);

  len = get_request()->GetLine(0,*request_string,max_string_len);
  request_string->set_strlen(len);
  settings->set_jpip_request(*request_string);

  kdws_string tmp_string(max_string_len);

  get_channel_type()->GetLBText(cb_idx,tmp_string);
  settings->set_jpip_channel_type(tmp_string);
  tmp_string.clear();

  len = get_proxy()->GetLine(0,tmp_string,max_string_len);
  tmp_string.set_strlen(len);
  settings->set_jpip_proxy(tmp_string);
  tmp_string.clear();

  len = get_cache_dir()->GetLine(0,tmp_string,max_string_len);
  tmp_string.set_strlen(len);
  settings->set_jpip_cache(tmp_string);
  tmp_string.clear();

  kdu_client_mode mode = KDU_CLIENT_MODE_AUTO;
  if (force_interactive_button()->GetCheck() == BST_CHECKED)
    mode = KDU_CLIENT_MODE_INTERACTIVE;
  settings->set_jpip_client_mode(mode);

  EndDialog(IDOK);
}

/*****************************************************************************/
/*                        kdws_url_dialog::get_url                           */
/*****************************************************************************/

const char *kdws_url_dialog::get_url()
{
  if (url != NULL)
    return url;
  if ((request_string == NULL) || (server_name == NULL))
    return NULL;

  const char *name = (const char *)(*server_name);
  char *tmp_name = new char[1+strlen(name)];
  const char *port_suffix = strrchr(name,':');
  if ((name[0] == '[') && (port_suffix != NULL))
    { // Check whether this is really a port suffix or not
      port_suffix = strchr(name,']');
      if ((port_suffix != NULL) && (port_suffix[1] == ':'))
        port_suffix++;
      else if ((port_suffix != NULL) && (port_suffix[1] == '\0'))
        port_suffix = NULL;
    }
  if (port_suffix != NULL)
    { // Copy everything before the port suffix to `tmp_name', making it `name'
      strcpy(tmp_name,name);
      tmp_name[port_suffix-name] = '\0';
      name = tmp_name;
    }
  else
    port_suffix = "";
  char *enc_server = new char[3+kdu_hex_hex_encode(name,NULL,NULL,"&?=")];
  if ((name[0] == '[') && (name[strlen(name)-1] == ']'))
    strcpy(enc_server,name);
  else if ((name[0] != '[') &&
           kdcs_sockaddr::test_ip_literal(name,KDCS_ADDR_FLAG_IPV6_ONLY))
    { // Add brackets to IPv6 literals
      enc_server[0] = '[';
      strcpy(enc_server+1,name);
      strcat(enc_server,"]");
    }
  else
    kdu_hex_hex_encode(name,enc_server,NULL,"&?=");

  name = (const char *)(*request_string);
  const char *query = strrchr(name,'?');
  if ((query != NULL) && (strchr(query,'=') == NULL))
    query = NULL;
  char *enc_resource = new char[1+kdu_hex_hex_encode(name,NULL,query,"&?=")];
  kdu_hex_hex_encode(name,enc_resource,query,"&?=");

  if (query == NULL)
    {
      url = new char[strlen(enc_server)+strlen(enc_resource)+30];
      sprintf(url,"jpip://%s%s/%s",
              enc_server,port_suffix,enc_resource);
    }
  else
    {
      char *enc_query = new char[1+kdu_hex_hex_encode(query+1,NULL)];
      kdu_hex_hex_encode(query+1,enc_query);
      url = new char[strlen(enc_server)+strlen(enc_resource)+strlen(query)+30];
      sprintf(url,"jpip://%s%s/%s?%s",
              enc_server,port_suffix,enc_resource,enc_query);
      delete[] enc_query;
    }
  delete[] tmp_name;
  delete[] enc_server;
  delete[] enc_resource;
  return url;
}

/*****************************************************************************/
/*                      MESSAGE MAP for kdws_url_dialog                      */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_url_dialog, CDialog)
  ON_NOTIFY(TTN_GETDISPINFO, 0, get_tooltip_dispinfo)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                   kdws_url_dialog::get_tooltip_dispinfo                   */
/*****************************************************************************/

void kdws_url_dialog::get_tooltip_dispinfo(NMHDR* pNMHDR, LRESULT *pResult)
{
  NMTTDISPINFO *pTTT = (NMTTDISPINFO *) pNMHDR;
  UINT_PTR nID =pNMHDR->idFrom;
  if (pTTT->uFlags & TTF_IDISHWND)
    {
        // idFrom is actually the HWND of the tool
        nID = ::GetDlgCtrlID((HWND)nID);
        if(nID)
          {
            int num_chars =
              ::LoadString(AfxGetResourceHandle(),
                           (UINT) nID,this->tooltip_buffer,
                           KD_URL_DIALOG_MAX_TOOLTIP_CHARS);
            if (num_chars > 0)
              {
                pTTT->lpszText = this->tooltip_buffer;
                pTTT->hinst = NULL;
              }
            ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,300);
          }
    }
}
