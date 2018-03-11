/*****************************************************************************/
// File: kdws_properties.cpp [scope = APPS/WINSHOW]
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
  Implements the `kdws_properties' class, which provides a dialog for
displaying JPEG2000 codestream properties in a browser window.
******************************************************************************/

#include "stdafx.h"
#include "kdws_properties.h"
#include "kdws_manager.h"


/* ========================================================================= */
/*                           kdws_properties_help                            */
/* ========================================================================= */

/*****************************************************************************/
/*                kdws_properties_help::kdws_properties_help                 */
/*****************************************************************************/

kdws_properties_help::kdws_properties_help(const char *string,
                                           kdu_coords placement,
                                           CWnd* pParent)
	: CDialog(kdws_properties_help::IDD, pParent)
{
  this->string = string;
  this->placement = placement;
}

/*****************************************************************************/
/*                    kdws_properties_help::OnInitDialog                     */
/*****************************************************************************/

BOOL kdws_properties_help::OnInitDialog() 
{
  CDialog::OnInitDialog();
  while (*string == '\n')
    string++; // Skip leading empty lines, if any
  const char *sp;
  char *dp;
  int out_string_len;
  for (sp=string, out_string_len=1; *sp != '\0'; sp++, out_string_len++)
    if (*sp == '\n')
      out_string_len++;
  kdws_string out_string(out_string_len);
  for (sp=string, dp=out_string; *sp != '\0'; *(dp++)=*(sp++))
    if (*sp == '\n')
      *(dp++) = '\r';
  *(dp++) = '\0';
  get_edit()->SetWindowText(out_string);

  // Move dialog window to desired position
  SetWindowPos(NULL,placement.x,placement.y,0,0,
               SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);

  // Find the height of the displayed text
  int text_height=0, text_lines=0;
  SIZE text_size;
  CDC *dc = get_edit()->GetDC();
  const _TCHAR *scan = out_string;
  while (*scan != _T('\0'))
    {
      int line_chars = 0;
      while ((scan[line_chars] != _T('\0')) && (scan[line_chars] != _T('\n')))
        line_chars++;
      if (line_chars > 0)
        text_size = dc->GetTextExtent(scan,line_chars);
      text_height += text_size.cy;
      scan += line_chars;
      if (*scan != _T('\0'))
        scan++; 
    }
  get_edit()->ReleaseDC(dc);

  // Resize windows to fit the text height

  WINDOWPLACEMENT dialog_placement, edit_placement;
  GetWindowPlacement(&dialog_placement);
  get_edit()->GetWindowPlacement(&edit_placement);
  int dialog_width = dialog_placement.rcNormalPosition.right -
    dialog_placement.rcNormalPosition.left;
  int edit_width = edit_placement.rcNormalPosition.right -
    edit_placement.rcNormalPosition.left;
  int dialog_height = dialog_placement.rcNormalPosition.bottom -
    dialog_placement.rcNormalPosition.top;
  int edit_height = edit_placement.rcNormalPosition.bottom -
    edit_placement.rcNormalPosition.top;

  get_edit()->SetWindowPos(NULL,0,0,edit_width,text_height+8,
                           SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
  SetWindowPos(NULL,0,0,dialog_width,text_height+8+dialog_height-edit_height,
               SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);

  return TRUE;  // return TRUE unless you set the focus to a control
}

/*****************************************************************************/
/*                    MESSAGE MAP for kdws_properties_help                   */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_properties_help, CDialog)
END_MESSAGE_MAP()


/* ========================================================================= */
/*                             kdws_properties                               */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdws_properties::kdws_properties                      */
/*****************************************************************************/

kdws_properties::kdws_properties(kdu_codestream codestream,
                                 const char *text, CWnd* pParent)
	: CDialog(kdws_properties::IDD, pParent)
{
  assert(text != NULL);
  this->codestream = codestream;
  this->text = text;
}

/*****************************************************************************/
/*                       kdws_properties::OnInitDialog                       */
/*****************************************************************************/

BOOL kdws_properties::OnInitDialog()
{
  CDialog::OnInitDialog();
  const char *string = text;
  int max_width = 0;;
  kdws_string tbuf(256);
  while (*string != '\0')
    {
      char *tbuf_start = tbuf;
      char *tp = tbuf_start;
      SIZE text_size;
      for (; (*string != '\0') && (*string != '\n'); string++)
        if ((tp-tbuf_start) < 255)
          *(tp++) = *string;
      *tp = '\0';
      get_list()->AddString(tbuf);
      CDC *dc = get_list()->GetDC();
      GetTextExtentPoint32(dc->m_hDC,tbuf,(int)(tp-tbuf),&text_size);
      get_list()->ReleaseDC(dc);
      if (text_size.cx > max_width)
        max_width = text_size.cx;
      if (*string == '\n')
        string++; // Skip to next line.
    }
  get_list()->SetCurSel(-1);
  get_list()->SetHorizontalExtent(max_width+10);
  return TRUE;
}

/*****************************************************************************/
/*                  kdws_properties::OnDblclkPropertiesList                  */
/*****************************************************************************/

void kdws_properties::OnDblclkPropertiesList() 
{
  int selection = get_list()->GetCurSel();
  int length = get_list()->GetTextLen(selection);
  kdws_string string(length);
  get_list()->GetText(selection,string);
  const char *attribute_id;
  kdu_params *obj = codestream.access_siz()->find_string(string,attribute_id);
  if (obj != NULL)
    {
      kdu_message_queue collector;
      kdu_message_formatter formatter(&collector,60);
      formatter.start_message();
      obj->describe_attribute(attribute_id,formatter,true);
      formatter.flush(true);
      POINT point;
      GetCursorPos(&point);
      kdws_properties_help help(collector.pop_message(),
                                kdu_coords(point.x,point.y),this);
      help.DoModal();
    }
}

/*****************************************************************************/
/*                      MESSAGE MAP for kdws_properties                      */
/*****************************************************************************/

BEGIN_MESSAGE_MAP(kdws_properties, CDialog)
	ON_LBN_DBLCLK(IDC_PROPERTIES_LIST, OnDblclkPropertiesList)
END_MESSAGE_MAP()
