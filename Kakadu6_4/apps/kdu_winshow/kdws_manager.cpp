/*****************************************************************************/
// File: kdws_manager.cpp [scope = APPS/WINSHOW]
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
   Implements the application object of the interactive JPEG2000 viewer,
"kdu_show".  Menu processing and idle-time decompressor processing are all
controlled from here.  The "kdu_show" application demonstrates some of the
support offered by the Kakadu framework for interactive applications,
including persistence and incremental region-based decompression.
******************************************************************************/

#include "stdafx.h"
#include "kdws_manager.h"
#include "kdws_window.h"
#include "kdws_renderer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

kdws_manager theApp; // There can only be one application object.

const char *jpip_channel_types[4] = {"http","http-tcp","none",NULL};


/* ========================================================================= */
/*                           Internal Functions                              */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                presentation_thread_startup                         */
/*****************************************************************************/

static UINT presentation_thread_startup(void *param)
{
  kdws_manager *app = (kdws_manager *) param;
  app->presentation_thread_entry();
  return 0;
}

/*****************************************************************************/
/* STATIC                     register_protocols                             */
/*****************************************************************************/

static void
  register_protocols(const char *executable_path)
  /* Sets up the registry entries required to register the JPIP protocol
     variants which are currently supported.  URL's of the form
     <protocol name>:... will be passed into a newly launched instance of
     the present application when clicked within the iExplorer application. */
{
  DWORD disposition;
  HKEY root_key, icon_key, shell_key, open_key, command_key;

  kdws_string executable(executable_path);
  kdws_string description("JPIP Interactive Imaging Protocol");
  kdws_string command_string((int) strlen(executable_path) + 4);
  strcpy(command_string,executable_path);
  strcat(command_string," %1");

  LONG result;
  if ((result =
       RegCreateKeyEx(HKEY_CLASSES_ROOT,_T("jpip"),0,REG_NONE,
                      REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                      &root_key,&disposition)) == ERROR_SUCCESS)
    {
      RegSetValueEx(root_key,NULL,0,REG_SZ,
                    (kdu_byte *)((const _TCHAR *)description),
                    (int)(sizeof(_TCHAR)*(strlen(description)+1)));
      RegSetValueEx(root_key,_T("URL Protocol"),0,REG_SZ,
                    (kdu_byte *)(_T("")),1);
      if (RegCreateKeyEx(root_key,_T("DefaultIcon"),0,REG_NONE,
                         REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                         &icon_key,&disposition) == ERROR_SUCCESS)
        {
          RegSetValueEx(icon_key,NULL,0,REG_SZ,
                        (kdu_byte *)((const _TCHAR *) executable),
                        (int)(sizeof(_TCHAR)*(strlen(executable)+1)));
          RegCloseKey(icon_key);
        }
      if (RegCreateKeyEx(root_key,_T("shell"),0,REG_NONE,
                         REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                         &shell_key,&disposition) == ERROR_SUCCESS)
        {
          if (RegCreateKeyEx(shell_key,_T("open"),0,REG_NONE,
                             REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                             &open_key,&disposition) == ERROR_SUCCESS)
            {
              if (RegCreateKeyEx(open_key,_T("command"),0,REG_NONE,
                                 REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                                 &command_key,&disposition) == ERROR_SUCCESS)
                {
                  RegSetValueEx(command_key,NULL,0,REG_SZ,(kdu_byte *)
                                ((const _TCHAR *) command_string),
                                (int)(sizeof(_TCHAR) *
                                      (strlen(command_string)+1)));
                  RegCloseKey(command_key);
                }
              RegCloseKey(open_key);
            }
          RegCloseKey(shell_key);
        }
      RegCloseKey(root_key);
    }
}


/* ========================================================================= */
/*                            External Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN                 kdws_compare_file_pathnames                        */
/*****************************************************************************/

bool
  kdws_compare_file_pathnames(const char *fname1, const char *fname2)
{
  if ((fname1 == NULL) || (fname2 == NULL))
    return false;
  for (; (*fname1 != '\0') && (*fname2 != '\0'); fname1++, fname2++)
    {
      if (toupper(*fname1) == toupper(*fname2))
        continue;
      if (((*fname1 != '/') && (*fname1 != '\\')) ||
          ((*fname2 != '/') && (*fname2 != '\\')))
        break;
    }
  return (*fname1 == '\0') && (*fname2 == '\0');
}


/* ========================================================================= */
/*                               CAboutDlg Class                             */
/* ========================================================================= */

class CAboutDlg : public CDialog {
  public:
    CAboutDlg();
	enum { IDD = IDD_ABOUTBOX };
protected:
	DECLARE_MESSAGE_MAP()
  };

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                             CAboutDlg::CAboutDlg                          */
/*****************************************************************************/

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

/* ========================================================================= */
/*                    Error and Warning Message Handlers                     */
/* ========================================================================= */

/*****************************************************************************/
/* CLASS                        core_messages_dlg                            */
/*****************************************************************************/

class core_messages_dlg : public CDialog
{
  public:
    core_messages_dlg(const char *utf8_src, CWnd* pParent = NULL)
      : CDialog(core_messages_dlg::IDD, pParent)
      {
        while (*utf8_src == '\n')
          utf8_src++; // skip leading empty lines, if any.
        kdws_string tmp_string(utf8_src);
        int num_chars;
        const WCHAR *sp, *src=tmp_string;
        for (num_chars=0, sp=src; *sp != L'\0'; sp++, num_chars++)
          if ((*sp == L'&') || (*sp == L'\\'))
            num_chars++; // Need to replicate these special characters
        string = new kdws_string(num_chars);
        WCHAR *dp = *string;
        for (sp=src; *sp != L'\0'; sp++)
          {
            *(dp++) = *sp;
            if ((*sp == L'&') || (*sp == L'\\'))
              *(dp++) = *sp;
          }
      }
    ~core_messages_dlg()
      { delete string; }
	enum { IDD = IDD_CORE_MESSAGES };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
private:
  CStatic *get_static()
    { return (CStatic *) GetDlgItem(IDC_MESSAGE); }
private:
  kdws_string *string;
protected:
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
};

/*****************************************************************************/
/*                     core_messages_dlg::DoDataExchange                     */
/*****************************************************************************/

void
  core_messages_dlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(core_messages_dlg, CDialog)
END_MESSAGE_MAP()

/*****************************************************************************/
/*                      core_messages_dlg::OnInitDialog                      */
/*****************************************************************************/

BOOL core_messages_dlg::OnInitDialog() 
{
  CDialog::OnInitDialog();
  get_static()->SetWindowText(*string);

  // Find the height of the displayed text
  int text_height = 0;
  SIZE text_size;
  CDC *dc = get_static()->GetDC();
  const _TCHAR *scan = *string;
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
  get_static()->ReleaseDC(dc);

  // Resize windows to fit the text height

  WINDOWPLACEMENT dialog_placement, static_placement;
  GetWindowPlacement(&dialog_placement);
  get_static()->GetWindowPlacement(&static_placement);
  int dialog_width = dialog_placement.rcNormalPosition.right -
    dialog_placement.rcNormalPosition.left;
  int static_width = static_placement.rcNormalPosition.right -
    static_placement.rcNormalPosition.left;
  int dialog_height = dialog_placement.rcNormalPosition.bottom -
    dialog_placement.rcNormalPosition.top;
  int static_height = static_placement.rcNormalPosition.bottom -
    static_placement.rcNormalPosition.top;

  get_static()->SetWindowPos(NULL,0,0,static_width,text_height,
                             SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
  SetWindowPos(NULL,0,0,dialog_width,text_height+6+dialog_height-static_height,
               SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);  
  return TRUE;
}

/*****************************************************************************/
/* CLASS                     kd_core_message_queue                           */
/*****************************************************************************/

class kd_core_message_queue : public kdu_message_queue {
  /* This object is used to implement error and warning message services
     which are registered globally for use by all threads which may
     use `kdu_error' or `kdu_warning'.  The object is based on the
     functionality offered by the versatile Kakadu core message handling
     class `kdu_message_queue', providing the required application specific
     override for `kdu_message_queue::pop_message' so that all message display
     can be handled in the main window management thread. */
  public: // Member functions
    kd_core_message_queue(bool for_errors)
      {
        this->configure(16,true,for_errors,0);
        main_thread_id = 0; main_thread_msg = 0;
        in_modal_display = false;
      }
    void configure_posting(UINT msg, DWORD thread_id)
      { /* This function should be called when the application is started,
           supplying a `msg' number, to be posted to the message loop of the
           thread with the indicated `thread_id', in the event that an
           error/warning message should arrive on another thread.  */
        this->main_thread_msg = msg;
        this->main_thread_id = thread_id;
        main_thread.set_to_self();
        assert(main_thread_id == GetCurrentThreadId());
      }
    const char *pop_message()
      {
        kdu_thread self;  self.set_to_self();
        if (self != main_thread)
          {
            PostThreadMessage(main_thread_id,main_thread_msg,0,0);
            return NULL;
          }
        const char *text = NULL;
        if ((!in_modal_display) &&
            ((text = kdu_message_queue::pop_message()) != NULL))
          {
            in_modal_display = true;
            core_messages_dlg messages(text,NULL);
            messages.DoModal();
            in_modal_display = false;
          }
        return text;
      }
  private: // Data
    bool in_modal_display; // Prevent recursive message window creation
    DWORD main_thread_id; // Used to post thread messages
    UINT main_thread_msg;
    kdu_thread main_thread;
  };

static kd_core_message_queue warn_message_queue(false);
static kd_core_message_queue err_message_queue(true);
static kdu_message_formatter warn_formatter(&warn_message_queue,50);
static kdu_message_formatter err_formatter(&err_message_queue,50);
    // Still need to call `warn_message_queue.configure_display_wnd' and
    // `err_message_queue.configure_display_wnd' once a frame window is
    // available.


/* ========================================================================= */
/*                                kdws_string                                */
/* ========================================================================= */

/*****************************************************************************/
/*                    kdws_string::kdws_string (max_chars)                   */
/*****************************************************************************/

kdws_string::kdws_string(int max_chars)
{
  wide_buf_len = max_chars+1;
  utf8_buf_len = max_chars*3 + 1; // Safe upper bound
  wide_buf = NULL;  utf8_buf = NULL;
  wide_buf = new WCHAR[wide_buf_len];
  utf8_buf = new char[utf8_buf_len];
  utf8_buf_valid = wide_buf_valid = false;
}

/*****************************************************************************/
/*                       kdws_string::kdws_string (UTF8)                     */
/*****************************************************************************/

kdws_string::kdws_string(const char *cp)
{
  utf8_buf_len = (int)(::strlen(cp)+1);
  wide_buf_len = 0;
  utf8_buf = new char[utf8_buf_len];
  wide_buf = NULL;
  utf8_buf_valid = true; wide_buf_valid = false;
  memcpy(utf8_buf,cp,(size_t)utf8_buf_len);
}

/*****************************************************************************/
/*                     kdws_string::kdws_string (unicode)                    */
/*****************************************************************************/

kdws_string::kdws_string(const WCHAR *cp)
{
  wide_buf_len = (int)(wcslen(cp)+1);
  utf8_buf_len = 0;
  wide_buf = new WCHAR[wide_buf_len];
  utf8_buf = NULL;
  wide_buf_valid = true; utf8_buf_valid = false;
  memcpy(wide_buf,cp,sizeof(WCHAR)*(size_t)wide_buf_len);
}

/*****************************************************************************/
/*                          kdws_string::set_strlen                          */
/*****************************************************************************/

void kdws_string::set_strlen(int len)
{
  if (wide_buf_valid)
    {
      assert(len < wide_buf_len);
      if (len >= wide_buf_len)
        len = wide_buf_len-1;
      if (len >= 0)
        wide_buf[len] = L'\0';
    }
  if (utf8_buf_valid)
    {
      assert(len < utf8_buf_len);
      if (len >= utf8_buf_len)
        len = utf8_buf_len-1;
      if (len >= 0)
        utf8_buf[len] = '\0';
    }
}

/*****************************************************************************/
/*                            kdws_string::strlen                            */
/*****************************************************************************/

int kdws_string::strlen()
{
  int length = 0;
  if (wide_buf_valid)
    {
      WCHAR *wp = wide_buf;
      while (*(wp++) != L'\0')
        length++;
    }
  else if (utf8_buf_valid)
    {
      char tmp;
      const char *cp = utf8_buf;
      const char *lim_cp = utf8_buf + utf8_buf_len;
      while ((tmp = *cp) != '\0')
        {
          if (tmp < 128)
            cp++;
          else if (tmp < 224)
            cp += 2;
          else if (tmp < 240)
            cp += 3;
          else if (tmp < 248)
            cp += 4;
          else if (tmp < 252)
            cp += 5;
          else
            cp += 6;
          if (cp >= lim_cp)
            break;
          length++;;
        }
    }
  return length;
}

/*****************************************************************************/
/*                       kdws_string::validate_utf8_buf                      */
/*****************************************************************************/

void kdws_string::validate_utf8_buf()
{
  if (utf8_buf_valid)
    return;
  int count_chars = 0;
  if (wide_buf_valid)
    {
      count_chars = wide_buf_len-1;
      while ((count_chars > 0) && (wide_buf[count_chars-1] == L'\0'))
        count_chars--;
    }
  if (utf8_buf == NULL)
    {
      if (utf8_buf_len == 0)
        utf8_buf_len = 3*count_chars+1;
      if (utf8_buf_len < 1)
        utf8_buf_len = 1; // Just in case of overflow
      utf8_buf = new char[utf8_buf_len];
    }
  memset(utf8_buf,0,(size_t)utf8_buf_len);
  char *dp = utf8_buf;
  if (wide_buf_valid)
    {
      const WCHAR *sp=wide_buf;
      char *dp_lim = utf8_buf + utf8_buf_len - 1;
      for (; (count_chars > 0) && (dp < dp_lim); count_chars--, sp++)
        {
          WCHAR val = *sp;
          if (val < 0x80)
            *(dp++) = (char) val;
          else if (val < 0x800)
            {
              if (dp >= (dp_lim+1))
                break;
              *(dp++) = (char) (0xC0 | (val>>6));
              *(dp++) = (char) (0x80 | (val & 0x3F));
            }
          else
            {
              if (dp >= (dp_lim+2))
                break;
              *(dp++) = (char) (0xE0 | (val>>12));
              *(dp++) = (char) (0x80 | ((val>>6) & 0x3F));
              *(dp++) = (char) (0x80 | (val & 0x3F));
            }
        }
    }
  *dp = '\0';
  utf8_buf_valid = true;
}

/*****************************************************************************/
/*                       kdws_string::validate_wide_buf                      */
/*****************************************************************************/

void kdws_string::validate_wide_buf()
{
  if (wide_buf_valid)
    return;
  int count_bytes = 0;
  if (utf8_buf_valid)
    {
      count_bytes = utf8_buf_len-1;
      while ((count_bytes > 0) && (utf8_buf[count_bytes-1] == '\0'))
        count_bytes--;
    }
  if (wide_buf == NULL)
    {
      if (wide_buf_len == 0)
        wide_buf_len = count_bytes+1;
      wide_buf = new WCHAR[wide_buf_len];
    }
  memset(wide_buf,0,sizeof(WCHAR)*(size_t)wide_buf_len);
  WCHAR *dp = wide_buf;
  if (utf8_buf_valid)
    {
      const char *sp = utf8_buf;
      WCHAR *dp_lim = wide_buf + wide_buf_len - 1;
      while ((count_bytes > 0) && (dp < dp_lim))
        {
          int extra_chars = 0;
          WCHAR val, tmp=((WCHAR)(*(sp++))) & 0x00FF;
          if (tmp < 128)
            val = tmp;
          else if  (tmp < 224)
            { val = (tmp-192); extra_chars = 1; }
          else if (tmp < 240)
            { val = (tmp-224); extra_chars = 2; }
          else if (tmp < 248)
            { val = (tmp-240); extra_chars = 3; }
          else if (tmp < 252)
            { val = (tmp-240); extra_chars = 4; }
          else
            { val = (tmp-252); extra_chars = 5; }
          count_bytes -= (1+extra_chars);
          if (count_bytes < 0)
            break;
          for (; extra_chars > 0; extra_chars--)
            {
              val <<= 6;
              val |= (((kdu_uint16)(*(sp++))) & 0x00FF) - 128;
            }
          *(dp++) = val;
        }
    }
  *dp = L'\0';
  wide_buf_valid = true;
}


/* ========================================================================= */
/*                               kdws_settings                               */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdws_settings::kdws_settings                       */
/*****************************************************************************/

kdws_settings::kdws_settings()
{
  open_save_dir = NULL; open_idx = save_idx = 1;
  jpip_server = jpip_proxy = jpip_cache = NULL;
  jpip_request = jpip_channel = NULL;
  set_jpip_channel_type(jpip_channel_types[0]);
  jpip_client_mode = KDU_CLIENT_MODE_INTERACTIVE;
}

/*****************************************************************************/
/*                       kdws_settings::~kdws_settings                       */
/*****************************************************************************/

kdws_settings::~kdws_settings()
{
  if (open_save_dir != NULL)
    delete[] open_save_dir;
  if (jpip_server != NULL)
    delete[] jpip_server;
  if (jpip_proxy != NULL)
    delete[] jpip_proxy;
  if (jpip_cache != NULL)
    delete[] jpip_cache;
  if (jpip_request != NULL)
    delete[] jpip_request;
  if (jpip_channel != NULL)
    delete[] jpip_channel;
}

/*****************************************************************************/
/*                      kdws_settings::save_to_registry                      */
/*****************************************************************************/

void
  kdws_settings::save_to_registry(CWinApp *app)
{
  app->WriteProfileString(_T("files"),_T("directory"),
                          kdws_string(get_open_save_dir()));
  app->WriteProfileInt(_T("files"),_T("open-index"),open_idx);
  app->WriteProfileInt(_T("files"),_T("save-index"),save_idx);
  app->WriteProfileString(_T("jpip"),_T("cache-dir"),
                          kdws_string(get_jpip_cache()));
  app->WriteProfileString(_T("jpip"),_T("server"),
                          kdws_string(get_jpip_server()));
  app->WriteProfileString(_T("jpip"),_T("request"),
                          kdws_string(get_jpip_request()));
  app->WriteProfileString(_T("jpip"),_T("channel-type"),
                          kdws_string(get_jpip_channel_type()));
  const _TCHAR *mode_string = _T("interactive");
  switch (jpip_client_mode) {
    case KDU_CLIENT_MODE_AUTO: mode_string=_T("auto");
         break;
    case KDU_CLIENT_MODE_INTERACTIVE: mode_string=_T("interactive");
         break;
    case KDU_CLIENT_MODE_NON_INTERACTIVE: mode_string=_T("non-interactive");
         break;
    default: break;
  }
  app->WriteProfileString(_T("jpip"),_T("client-mode"),mode_string);
}

/*****************************************************************************/
/*                     kdws_settings::load_from_registry                     */
/*****************************************************************************/

void
  kdws_settings::load_from_registry(CWinApp *app)
{
  set_open_save_dir(
    kdws_string((const _TCHAR *)
                app->GetProfileString(_T("files"),_T("directory"))));
  open_idx = app->GetProfileInt(_T("files"),_T("open-index"),1);
  save_idx = app->GetProfileInt(_T("files"),_T("save-index"),1);
  CString missing_cache_dir = _T("***");
  CString existing_cache_dir =
    app->GetProfileString(_T("jpip"),_T("cache-dir"),missing_cache_dir);
  if (existing_cache_dir.Compare(missing_cache_dir) == 0)
    {
      kdws_string temp_path(MAX_PATH);
      int len = GetTempPath(MAX_PATH,temp_path);
      if (len > 0)
        set_jpip_cache(temp_path);
    }
  else
    set_jpip_cache(kdws_string((const _TCHAR *) existing_cache_dir));
  set_jpip_server(
    kdws_string((const _TCHAR *)
                app->GetProfileString(_T("jpip"),_T("server"))));
  set_jpip_request(
    kdws_string((const _TCHAR *)
                app->GetProfileString(_T("jpip"),_T("request"))));
  kdws_string c_string((const _TCHAR *)
                       app->GetProfileString(_T("jpip"),_T("channel-type")));
  if (c_string.is_empty())
    set_jpip_channel_type(jpip_channel_types[0]);
  else
    set_jpip_channel_type(c_string);

  kdws_string mode_string((const _TCHAR *)
                          app->GetProfileString(_T("jpip"),_T("client-mode")));
  const char *string = (const char *) mode_string;
  jpip_client_mode = KDU_CLIENT_MODE_INTERACTIVE;
  if (strcmp(string,"auto") == 0)
    jpip_client_mode = KDU_CLIENT_MODE_AUTO;
  else if (strcmp(string,"non-interactive") == 0)
    jpip_client_mode = KDU_CLIENT_MODE_NON_INTERACTIVE;
}


/*===========================================================================*/
/*                           kdws_frame_presenter                            */
/*===========================================================================*/

/*****************************************************************************/
/*                kdws_frame_presenter::kdws_frame_presenter                 */
/*****************************************************************************/

kdws_frame_presenter::kdws_frame_presenter(kdws_manager *application,
                                           kdws_frame_window *wnd,
                                           int min_drawing_interval)
{
  this->application = application;
  this->window = wnd;
  this->min_drawing_interval = min_drawing_interval;
  this->drawing_downcounter = 1; // Can draw immediately
  drawing_mutex.create();
  painter = NULL;
  frame_to_paint = last_frame_painted = (1<<31);
  wake_app_once_painting_completes = false;
}

/*****************************************************************************/
/*                kdws_frame_presenter::~kdws_frame_presenter                */
/*****************************************************************************/

kdws_frame_presenter::~kdws_frame_presenter()
{
  assert(painter == NULL); // `disable' should have been called already
  drawing_mutex.destroy();
}

/*****************************************************************************/
/*                      kdws_frame_presenter::disable                        */
/*****************************************************************************/

bool kdws_frame_presenter::disable()
{
  wake_app_once_painting_completes = false; // Only the app thread can write
            // this flag, so we should cancel any previously installed request
            // for wakeup, before going any further.
  if (painter == NULL)
    return true; // Already disabled
  wake_app_once_painting_completes = true; // Set the flag to true, to ensure
        // safe behaviour in the event of race conditions.
  if (drawing_mutex.try_lock())
    {
      wake_app_once_painting_completes = false;
      painter = NULL;
      drawing_mutex.unlock();
    }
  return (painter == NULL);
}

/*****************************************************************************/
/*                       kdws_frame_presenter::reset                         */
/*****************************************************************************/

void kdws_frame_presenter::reset()
{
  drawing_mutex.lock();
  painter = NULL;
  frame_to_paint = last_frame_painted = (1<<31);
  wake_app_once_painting_completes = false;
  drawing_mutex.unlock();
}

/*****************************************************************************/
/*                 kdws_frame_presenter::draw_pending_frame                  */
/*****************************************************************************/

bool kdws_frame_presenter::draw_pending_frame(DWORD main_thread_id)
{
  if (drawing_downcounter > 1)
    {
      drawing_downcounter--;
      return false;
    }
  kdws_renderer *target = painter;
  if ((target == NULL) || (last_frame_painted == frame_to_paint))
    {
      if (wake_app_once_painting_completes)
        ::PostThreadMessage(main_thread_id,WM_NULL,0,0); // Previous wakeup
                              // might have arrived before app went to sleep.
      return false;
    }
  drawing_mutex.lock();
  target = painter; // Check again to make sure we have something to paint
  if (target != NULL)
    {
      last_frame_painted = frame_to_paint;
      target->paint_view_from_presentation_thread();

         /* Note: The implementation here paints the frame without waiting
            for vertical blanking.  This means that tearing can occur on the
            display.  A better implementation would use DirectDraw to paint
            the surface within the frame presenter, specifying that the draw
            should be synchronized with the vertical blanking period.  This
            means that the painting operation may be held up, but we have
            no problem with this, because the presentation thread is running
            asynchronously from the rest of the application.  In the worst
            case, we hold onto the mutex for quite a while, but the main
            thread calls `disable' when it needs pop frames from the
            compositor, and that function in turn invokes `kdu_mutex.try_lock'
            which does not block the caller.  As a result, it is fine for the
            presentation thread to hold a lock on the `drawing_mutex' for the
            best part of an entire display refresh period.  The only reason
            we don't implement display tear avoidance in this application is
            that it requires DirectX, which is a huge download from
            Microsoft that might prevent licensees from compiling this
            application immediately.  Video is only one small part of the
            "kdu_show" functionality. */

      drawing_downcounter = min_drawing_interval;
      if (wake_app_once_painting_completes)
        ::PostThreadMessage(main_thread_id,WM_NULL,0,0);
    }
  drawing_mutex.unlock();
  return true;
}



/*===========================================================================*/
/*                            kdws_client_notifier                           */
/*===========================================================================*/

/*****************************************************************************/
/*                        kdws_client_notifier::notify                       */
/*****************************************************************************/

void kdws_client_notifier::notify()
{
  if (have_unhandled_notification) return;
  have_unhandled_notification = true;
  PostThreadMessage(main_thread_id,main_thread_msg,0,
                    (LPARAM) client_identifier);
}


/* ========================================================================== */
/*                       kdws_manager Class Implementation                    */
/* ========================================================================== */

/******************************************************************************/
/*                         kdws_manager::kdws_manager                         */
/******************************************************************************/

kdws_manager::kdws_manager()
{
  in_idle = false;
  main_thread_id = 0; // Set the correct value in InitInstance

  windows = next_idle_window = NULL;
  next_window_identifier = 1;
  last_known_key_wnd = NULL;
  broadcast_actions_once = false;
  broadcast_actions_indefinitely = false;

  wakeup_timer_id = 0;
  next_window_to_wake = NULL;
  will_check_best_window_to_wake = false;
  
  window_list_change_mutex.create();
  open_file_list = NULL;
  next_jpip_client_identifier = 0;
  
  reset_placement_engine();

  min_presentation_drawing_interval = 1;
  presentation_thread = NULL;
}

/******************************************************************************/
/*                        kdws_manager::~kdws_manager                         */
/******************************************************************************/

kdws_manager::~kdws_manager()
{
  if (presentation_thread != NULL)
    delete presentation_thread;
  while ((next_idle_window = windows) != NULL)
    {
      windows = next_idle_window->next;
      delete next_idle_window;
    }
  kdws_open_file_record *frec;
  while ((frec = open_file_list) != NULL)
    {
      open_file_list = frec->next;
      delete frec;
    }
  window_list_change_mutex.destroy();
}

/******************************************************************************/
/*                  kdws_manager::presentation_thread_entry                   */
/******************************************************************************/

void kdws_manager::presentation_thread_entry()
{
  // Find screen refresh parameters
  DEVMODE dev_mode;
  memset(&dev_mode,0,sizeof(dev_mode));
  dev_mode.dmSize = sizeof(dev_mode);
  EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dev_mode);
  int refresh_rate = (int) dev_mode.dmDisplayFrequency;
  if (refresh_rate == 0)
    refresh_rate = 60; // Can't find true value; assume standard LCD rate

  min_presentation_drawing_interval = (int)(refresh_rate/60);
                       // Allow for screen to be updated at least 60fps
  if (min_presentation_drawing_interval < 1)
    min_presentation_drawing_interval = 1;

  // Set up refresh timer
  UINT interval_msecs = 1000 / refresh_rate;
  if (interval_msecs < 1)
    interval_msecs = 1;

  UINT_PTR refresh_timer_id = ::SetTimer(NULL,0,interval_msecs,NULL);

  // Run message loop to catch timer events
  MSG msg;
  while (::GetMessage(&msg,NULL,0,0))
    {
      if (msg.message != WM_TIMER)
        continue; // We only really expect timer messages, but there might be
                  // some other extraneous messages I guess, perhaps on startup
      window_list_change_mutex.lock();
      kdws_window_list *elt;
      for (elt=windows; elt != NULL; elt=elt->next)
        { // Give every window a chance to present a frame in the presentation
          // thread.
          elt->frame_presenter->draw_pending_frame(main_thread_id);
        }
      window_list_change_mutex.unlock();
    }
  ::KillTimer(NULL,refresh_timer_id);
}

/******************************************************************************/
/*                  kdws_manager::application_can_terminate                   */
/******************************************************************************/

bool kdws_manager::application_can_terminate()
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (!elt->wnd->application_can_terminate())
      return false;
  return true;
}

/******************************************************************************/
/*            kdws_manager::send_application_terminating_messages             */
/******************************************************************************/

void kdws_manager::send_application_terminating_messages()
{
  kdws_window_list *elt, *next;
  for (elt=windows; elt != NULL; elt=next)
    {
      next = elt->next;
      elt->wnd->application_terminating();
    }
  for (elt=windows; elt != NULL; elt=next)
    {
      next = elt->next;
      elt->wnd->DestroyWindow();
    }
}

/******************************************************************************/
/*                          kdws_manager::add_window                          */
/******************************************************************************/

void kdws_manager::add_window(kdws_frame_window *wnd)
{
  kdws_window_list *elt, *tail=NULL;
  for (elt=windows; elt != NULL; tail=elt, elt=elt->next)
    if (elt->wnd == wnd)
      return;
  elt = new kdws_window_list;
  elt->wnd = wnd;
  elt->window_identifier = next_window_identifier++;
  elt->file_or_url_name = NULL; // Until we have one
  elt->wakeup_time = -1.0;
  elt->window_empty = true;
  elt->window_placed = false;
  elt->frame_presenter =
    new kdws_frame_presenter(this,wnd,min_presentation_drawing_interval);
  window_list_change_mutex.lock();
  elt->next = NULL;
  if ((elt->prev = tail) == NULL)
    windows = elt;
  else
    tail->next = elt;
  window_list_change_mutex.unlock();
  if (m_pMainWnd == NULL)
    m_pMainWnd = wnd;
}

/******************************************************************************/
/*                        kdws_manager::remove_window                         */
/******************************************************************************/

void kdws_manager::remove_window(kdws_frame_window *wnd)
{
  if (wnd == last_known_key_wnd)
    last_known_key_wnd = NULL;
  
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  assert(elt != NULL);
  if (elt == NULL)
    return;
  if (elt == next_idle_window)
    next_idle_window = NULL; // Start scanning list again for idle processing

  window_list_change_mutex.lock();

  if (elt->frame_presenter != NULL)
    {
      elt->frame_presenter->reset(); // Just in case
      delete elt->frame_presenter;
      elt->frame_presenter = NULL;
    }
  
  if (elt->prev == NULL)
    windows = elt->next;
  else
    elt->prev->next = elt->next;
  if (elt->next != NULL)
    elt->next->prev = elt->prev;
  window_list_change_mutex.unlock();
  
  delete elt;
  if ((m_pMainWnd == wnd) && (windows != NULL))
    m_pMainWnd = windows->wnd;
      // If there are no more windows, `m_pMainWnd' will remain NULL,
      // which will cause the main run-loop to terminate the application,
      // passing through `ExitInstance'.  If we really want to keep the
      // application alive beyond this point, we have to create a new
      // empty window.
  
  if (windows == NULL)
    reset_placement_engine();
  if (elt == next_window_to_wake)
    {
      next_window_to_wake = NULL;
      install_next_scheduled_wakeup();
    }
}

/*****************************************************************************/
/*                       kdws_manager::get_access_idx                        */
/*****************************************************************************/

int kdws_manager::get_access_idx(kdws_frame_window *wnd)
{
  kdws_window_list *elt;
  int idx = 0;
  for (elt=windows; (elt != NULL) && (elt->wnd != wnd); idx++, elt=elt->next);
  return (elt==NULL)?-1:idx;
}

/*****************************************************************************/
/*                        kdws_manager::access_window                        */
/*****************************************************************************/

kdws_frame_window *kdws_manager::access_window(int idx)
{
  kdws_window_list *elt;
  for (elt=windows; (elt != NULL) && (idx > 0); idx--, elt=elt->next);
  return ((elt==NULL) || (idx != 0))?NULL:elt->wnd;
}

/*****************************************************************************/
/*                    kdws_manager::get_window_identifier                    */
/*****************************************************************************/

int kdws_manager::get_window_identifier(kdws_frame_window *wnd)
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      return elt->window_identifier;
  return 0;
}

/*****************************************************************************/
/*                   kdws_manager::reset_placement_engine                    */
/*****************************************************************************/

void kdws_manager::reset_placement_engine()
{
  cycle_origin = next_window_pos = kdu_coords(10,10);
  next_window_row = cycle_origin.y;  
}

/*****************************************************************************/
/*                    kdws_manager::declare_window_empty                     */
/*****************************************************************************/

void kdws_manager::declare_window_empty(kdws_frame_window *wnd, bool empty)
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      elt->window_empty = empty;
}

/*****************************************************************************/
/*                       kdws_manager::find_empty_window                     */
/*****************************************************************************/

kdws_frame_window *kdws_manager::find_empty_window()
{
  kdws_frame_window *result = NULL;
  kdws_window_list *best_elt = NULL;
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->window_empty &&
        ((best_elt == NULL) ||
         (best_elt->window_identifier < elt->window_identifier)))
      {
        best_elt = elt;
        result = elt->wnd;
      }
  return result;
}

/*****************************************************************************/
/*                        kdws_manager::place_window                         */
/*****************************************************************************/

bool kdws_manager::place_window(kdws_frame_window *wnd,
                                kdu_coords frame_size,
                                bool do_not_place_again,
                                bool placing_first_empty_window)
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  if (elt != NULL)
    {
      if (elt->window_placed && do_not_place_again)
        return false;
      if (!placing_first_empty_window)
        elt->window_placed = true;
    }
  kdu_dims screen_rect;
  screen_rect.size.x = GetSystemMetrics(SM_CXSCREEN);
  screen_rect.size.y = GetSystemMetrics(SM_CYSCREEN);
  
  // Determine `frame_rect' as it sits within the `screen_rect', with the
  // origin at the top left hand corner of the screen rectangle.
  kdu_dims frame_rect;
  frame_rect.pos = next_window_pos;
  frame_rect.size = frame_size;
  int h_delta = screen_rect.size.x - frame_rect.size.x; // Hor. placement slack
  if (frame_rect.pos.x > h_delta)
    { // Window does not fit horizontally
      if (frame_rect.pos.x == cycle_origin.x)
        { // This is the first attempt to place a window on this row
          frame_rect.pos.x = h_delta; // Shift left as little as possible
        }
      else
        { // Move down to the next window placement row
          frame_rect.pos.x = cycle_origin.x;
          frame_rect.pos.y = next_window_row;
          if (frame_rect.pos.x > h_delta)
            frame_rect.pos.x = h_delta; // Shift left as little as possible
        }
    }
  int v_delta = screen_rect.size.y-frame_rect.size.y; // Vert. placement slack
  if (frame_rect.pos.y > v_delta)
    { // Window does not fit vertically
      if (frame_rect.pos.y == cycle_origin.y)
        { // This is the first row of windows for the cycle
          frame_rect.pos.y = v_delta; // Shift window up as little as possible
        }
      else
        { // Start a new cycle
          cycle_origin.x += 50;
          cycle_origin.y += 50;
          while (cycle_origin.y > (screen_rect.size.y>>2))
            cycle_origin.y -= (screen_rect.size.y>>2);
          while (cycle_origin.x > (screen_rect.size.x>>2))
            cycle_origin.x -= (screen_rect.size.x>>2);
          frame_rect.pos.y = next_window_row = cycle_origin.y;
          frame_rect.pos.x = cycle_origin.x;
          if (frame_rect.pos.y > v_delta)
            frame_rect.pos.y = v_delta;
          if (frame_rect.pos.x > h_delta)
            frame_rect.pos.x = h_delta;
        }
    }

  if (frame_rect.pos.x < 0)
    frame_rect.pos.x = 0; // Just in case
  if (frame_rect.pos.y < 0)
    frame_rect.pos.y = 0; // Just in case
  
  // Determine the next window placement location
  if (!placing_first_empty_window)
    {
      next_window_pos = frame_rect.pos;
      next_window_pos.x += frame_rect.size.x + 10; // Leave a small gap.
      if ((v_delta =
           (frame_rect.pos.y+frame_rect.size.y+10)-next_window_row) > 0)
        next_window_row += v_delta;
    }
  
  // Actually place and display the window
  wnd->SetWindowPos(NULL,frame_rect.pos.x,frame_rect.pos.y,
                    frame_rect.size.x,frame_rect.size.y,
                    SWP_NOZORDER|SWP_SHOWWINDOW);
  return true;
}

/*****************************************************************************/
/*                   kdws_manager::get_next_action_window                    */
/*****************************************************************************/

kdws_frame_window *kdws_manager::get_next_action_window(kdws_frame_window *wnd)
{
  if (!(broadcast_actions_once || broadcast_actions_indefinitely))
    return NULL;
  if (!last_known_key_wnd)
    {
      if (wnd->is_key_window())
        last_known_key_wnd = wnd;
      else
        { // Initiating menu action from non-key window?? Safest to stop here.
          broadcast_actions_once = false;
          return NULL;
        }
    }

  kdws_window_list *elt = windows;
  for (; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  if (elt == NULL)
    { // Should not happen!  Safest to cancel any action broadcast
      last_known_key_wnd = NULL;
      broadcast_actions_once = false;
      return NULL;
    }
  elt = elt->next;
  if (elt == NULL)
    elt = windows;
  kdws_frame_window *result = elt->wnd;
  
  if (result == last_known_key_wnd)
    { // We have been right around already
      last_known_key_wnd = NULL;
      broadcast_actions_once = false;
      return NULL;
    }
  
  return result;
}

/*****************************************************************************/
/*                   kdws_manager::set_action_broadcasting                   */
/*****************************************************************************/

void kdws_manager::set_action_broadcasting(bool broadcast_once,
                                           bool broadcast_indefinitely)
{
  this->broadcast_actions_once = broadcast_once;
  this->broadcast_actions_indefinitely = broadcast_indefinitely;
  last_known_key_wnd = NULL;
}

/*****************************************************************************/
/*               kdws_manager::broadcast_playclock_adjustment                */
/*****************************************************************************/

void kdws_manager::broadcast_playclock_adjustment(double delta)
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    elt->wnd->adjust_playclock(delta);
}

/*****************************************************************************/
/*                        kdws_manager::schedule_wakeup                      */
/*****************************************************************************/

void
  kdws_manager::schedule_wakeup(kdws_frame_window *wnd, double wakeup_time)
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      break;
  if (elt == NULL)
    return; // Should never happen
  elt->wakeup_time = wakeup_time;
  if (will_check_best_window_to_wake)
    { // We will get around to considering this wakeup when we get back into
      // the ongoing call to `install_next_scheduled_wakeup'.
      assert(next_window_to_wake == NULL);
      return;
    }
  if (wakeup_time < 0.0)
    {
      if (next_window_to_wake == elt)
        {
          next_window_to_wake = NULL;
          install_next_scheduled_wakeup();
        }
    }
  else if ((next_window_to_wake == NULL) ||
           (wakeup_time < next_window_to_wake->wakeup_time))
    {
      next_window_to_wake = elt;
      set_wakeup_time(wakeup_time,-1.0);
    }
}

/*****************************************************************************/
/*                kdws_manager::install_next_scheduled_wakeup                */
/*****************************************************************************/

void kdws_manager::install_next_scheduled_wakeup()
{
  next_window_to_wake = NULL; // Just in case
  will_check_best_window_to_wake = true;
      // Avoids accidental installation of a new scheduled wakeup while we
      // are scanning for one.  This could happen if `schedule_wakeup' is
      // called from within a window's `wakeup' function.
  
  double current_time = -1.0; // Evaluate only if we need it
  kdws_window_list *elt, *earliest_elt = NULL;
  while (earliest_elt == NULL)
    {
      for (elt=windows; elt != NULL; elt=elt->next)
        if (elt->wakeup_time >= 0.0)
          {
            if (earliest_elt == NULL)
              {
                current_time = get_current_time();
                earliest_elt = elt;
              }
            else if (elt->wakeup_time < earliest_elt->wakeup_time)
              earliest_elt = elt;
          }
      if (earliest_elt == NULL)
        { // Nothing left to wakeup
          will_check_best_window_to_wake = false;
          if (wakeup_timer_id != 0)
            {
              KillTimer(NULL,wakeup_timer_id);
              wakeup_timer_id = 0;
            }
          return;
        }
      if (earliest_elt->wakeup_time <= current_time)
        {
          double scheduled_time = earliest_elt->wakeup_time;
          earliest_elt->wakeup_time = -1.0;
          if (scheduled_time >= 0.0)
            {
              earliest_elt->wnd->wakeup(scheduled_time,current_time);
              current_time = -1.0; // Some time may have been consumed
            }
          earliest_elt = NULL;
        }
    }

  // If we get here, we are ready to wait for `earliest_elt'
  next_window_to_wake = earliest_elt;
  set_wakeup_time(earliest_elt->wakeup_time,current_time);
  will_check_best_window_to_wake = false;
}

/*****************************************************************************/
/*                  kdws_manager::process_scheduled_wakeup                   */
/*****************************************************************************/

void kdws_manager::process_scheduled_wakeup()
{
  kdws_window_list *elt = next_window_to_wake;
  if (elt == NULL)
    return;
  double current_time = get_current_time();
  double scheduled_time = elt->wakeup_time;
  elt->wakeup_time = -1.0; // Cancel further wakeups
  next_window_to_wake = NULL;
  will_check_best_window_to_wake = true; // Prevents scheduling a new
      // timer event from within `schedule_wakeup' if it is called from within
      // the following `wakeup' call before getting a chance to choose the
      // best window to time, in the ensuing call to
      // `install_next_scheduled_wakeup'.
  if (scheduled_time >= 0.0)
    elt->wnd->wakeup(scheduled_time,current_time);
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*                       kdws_manager::set_wakeup_time                       */
/*****************************************************************************/

void kdws_manager::set_wakeup_time(double abs_time, double current_time)
{
  if (wakeup_timer_id != 0)
    { KillTimer(NULL,wakeup_timer_id); wakeup_timer_id = 0; }
  if (current_time <= 0.0)
    current_time = get_current_time();
  double wait_secs = abs_time - current_time;
  if (wait_secs <= 0.0)
    ::PostThreadMessage(main_thread_id,WM_TIMER,0,0);
  else
    {
      UINT wait_msecs = (UINT)(wait_secs * 1000.0);
      wait_msecs += 2; // Wait a bit longer (1.5 ms on average) for efficiency
      wakeup_timer_id = ::SetTimer(NULL,wakeup_timer_id,wait_msecs,NULL);
    }
}

/*****************************************************************************/
/*                      kdws_manager::get_frame_presenter                    */
/*****************************************************************************/

kdws_frame_presenter *
  kdws_manager::get_frame_presenter(kdws_frame_window *wnd)
{
  kdws_window_list *elt;
  for (elt=windows; elt != NULL; elt=elt->next)
    if (elt->wnd == wnd)
      return elt->frame_presenter;
  return NULL;
}

/*****************************************************************************/
/*                  kdws_manager::retain_open_file_pathname                  */
/*****************************************************************************/

const char *kdws_manager::retain_open_file_pathname(const char *path,
                                                    kdws_frame_window *wnd)
{  
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (kdws_compare_file_pathnames(scan->open_pathname,path))
      {
        scan->retain_count++;
        break;
      }
  if (scan == NULL)
    {
      scan = new kdws_open_file_record;
      scan->open_pathname = new char[strlen(path)+1];
      strcpy(scan->open_pathname,path);
      scan->next = open_file_list;
      open_file_list = scan;
      scan->retain_count = 1;
    }
  kdws_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      wnd_elt->file_or_url_name = scan->open_pathname;
  return scan->open_pathname;
}

/*****************************************************************************/
/*                  kdws_manager::release_open_file_pathname                 */
/*****************************************************************************/

void kdws_manager::release_open_file_pathname(const char *path,
                                              kdws_frame_window *wnd)
{
  kdws_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      wnd_elt->file_or_url_name = NULL;
  
  kdws_open_file_record *scan, *prev;
  for (prev=NULL, scan=open_file_list; scan!=NULL; prev=scan, scan=scan->next)
    if (kdws_compare_file_pathnames(scan->open_pathname,path))
      break;
  if (scan == NULL)
    return; // Nothing to release
  scan->retain_count--;
  if (scan->retain_count == 0)
    {
      if (prev == NULL)
        open_file_list = scan->next;
      else
        prev->next = scan->next;
      if (scan->save_pathname != NULL)
        MoveFileEx(kdws_string(scan->save_pathname),
                   kdws_string(scan->open_pathname),
                   MOVEFILE_REPLACE_EXISTING);
      delete scan;
    }
}

/*****************************************************************************/
/*                     kdws_manager::retain_jpip_client                      */
/*****************************************************************************/

const char *
  kdws_manager::retain_jpip_client(const char *server, const char *request,
                                   const char *url, kdu_client * &client,
                                   int &request_queue_id,
                                   kdws_frame_window *wnd)
{
  client = NULL; request_queue_id = -1; // Just in case something goes wrong
  if (url != NULL)
    server = request = NULL; // To be sure we don't mix two sources of info
  kdws_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      {
        wnd_elt->file_or_url_name = NULL;
        break;
      }

  kdu_client_mode mode = settings.get_jpip_client_mode();
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    {
      if (scan->jpip_client == NULL)
        continue;
      if (!scan->jpip_client->check_compatible_connection(server,request,
                                                          mode,url))
        continue;
      if (scan->jpip_client->is_one_time_request())
        request_queue_id = 0;
      else
        request_queue_id = scan->jpip_client->add_queue();
      if (request_queue_id < 0)
        continue; // Maybe the client connection was lost
      scan->retain_count++;
      break;
    }
  if (scan == NULL)
    {
      const char *proxy = settings.get_jpip_proxy();
      const char *transport = settings.get_jpip_channel_type();
      const char *cache_dir = settings.get_jpip_cache();
      scan = new kdws_open_file_record;
      scan->jpip_client = new kdu_client;
      try {
        if (url != NULL)
          {
            request_queue_id =
              scan->jpip_client->connect(NULL,proxy,NULL,transport,
                                         cache_dir,mode,url);
            scan->open_url = new char[strlen(url)+1];
            strcpy(scan->open_url,url);
          }
        else
          {
            request_queue_id =
              scan->jpip_client->connect(server,proxy,request,transport,
                                         cache_dir,mode,NULL);
            scan->open_url = new char[strlen(server)+strlen(request)+
                                      strlen("jpip://") + 2];
            strcpy(scan->open_url,"jpip://");
            strcat(scan->open_url,server);
            strcat(scan->open_url,"/");
            strcat(scan->open_url,request);
          }
      }
      catch (int exc) {
        delete scan;       
        throw exc;
      }
      scan->client_identifier = ++next_jpip_client_identifier;
      scan->client_notifier =
        new kdws_client_notifier(scan->client_identifier,
                                 KDWS_CLIENT_NOTIFICATION_MESSAGE);
      scan->jpip_client->install_notifier(scan->client_notifier);
      scan->next = open_file_list;
      open_file_list = scan;
      scan->retain_count = 1;
    }

  if (wnd_elt != NULL)
    wnd_elt->file_or_url_name = scan->open_url;
  client = scan->jpip_client;
  return scan->open_url;
}

/*****************************************************************************/
/*             kdws_manager::use_jpx_translator_with_jpip_client             */
/*****************************************************************************/

void kdws_manager::use_jpx_translator_with_jpip_client(kdu_client *client)
{
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (scan->jpip_client == client)
      {
        if (scan->jpx_client_translator == NULL)
          {
            scan->jpx_client_translator = new kdu_clientx;
            client->install_context_translator(scan->jpx_client_translator);
          }
        break;
      }
}

/*****************************************************************************/
/*                      kdws_manager::release_jpip_client                    */
/*****************************************************************************/

void kdws_manager::release_jpip_client(kdu_client *client,
                                       kdws_frame_window *wnd)
{
  kdws_window_list *wnd_elt = windows;
  for (; wnd_elt != NULL; wnd_elt=wnd_elt->next)
    if (wnd_elt->wnd == wnd)
      wnd_elt->file_or_url_name = NULL;
  
  kdws_open_file_record *scan, *prev;
  for (prev=NULL, scan=open_file_list; scan!=NULL; prev=scan, scan=scan->next)
    if (scan->jpip_client == client)
      break;
  if (scan == NULL)
    return; // Nothing to release
  scan->retain_count--;
  if (scan->retain_count == 0)
    {
      if (prev == NULL)
        open_file_list = scan->next;
      else
        prev->next = scan->next;
      assert(scan->save_pathname == NULL);
      scan->jpip_client->disconnect(false,2000,-1,true);
         // Forcibly disconnect everything after a 2 second timeout.
      scan->jpip_client->close();
      delete scan;
    }
}

/*****************************************************************************/
/*                  kdws_manager::handle_client_notification                 */
/*****************************************************************************/

void kdws_manager::handle_client_notification(int client_identifier)
{
  const char *open_url = NULL;
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (scan->client_identifier == client_identifier)
      {
        open_url = scan->open_url;
        scan->client_notifier->handling_notification();
        break;
      }
  for (kdws_window_list *wnd=windows; wnd != NULL; wnd=wnd->next)
    if ((wnd->file_or_url_name == open_url) && (wnd->wnd != NULL))
      wnd->wnd->client_notification();
}

/*****************************************************************************/
/*                kdws_manager::clear_lost_client_notifications              */
/*****************************************************************************/

void kdws_manager::clear_lost_client_notifications()
{
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if ((scan->client_notifier != NULL) &&
        scan->client_notifier->handling_notification())
      {
        for (kdws_window_list *wnd=windows; wnd != NULL; wnd=wnd->next)
          if ((wnd->file_or_url_name == scan->open_url) && (wnd->wnd != NULL))
            wnd->wnd->client_notification();
      }
}

/*****************************************************************************/
/*                   kdws_manager::get_save_file_pathname                    */
/*****************************************************************************/

const char *kdws_manager::get_save_file_pathname(const char *path)
{
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan!=NULL; scan=scan->next)
    if (kdws_compare_file_pathnames(scan->open_pathname,path))
      {
        if (scan->save_pathname == NULL)
          {
            scan->save_pathname = new char[strlen(path)+2];
            strcpy(scan->save_pathname,path);
            strcat(scan->save_pathname,"~");
          }
        return scan->save_pathname;
      }
  return path;
}

/*****************************************************************************/
/*                 kdws_manager::declare_save_file_invalid                   */
/*****************************************************************************/

void kdws_manager::declare_save_file_invalid(const char *path)
{
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (kdws_compare_file_pathnames(scan->open_pathname,path))
      return; // Safety measure to prevent deleting open file; shouldn't happen
    else if ((scan->save_pathname != NULL) &&
             kdws_compare_file_pathnames(scan->save_pathname,path))
      {
        delete[] scan->save_pathname;
        scan->save_pathname = NULL;
      }
  remove(path);
}

/*****************************************************************************/
/*                  kdws_manager::get_open_file_retain_count                 */
/*****************************************************************************/

int kdws_manager::get_open_file_retain_count(const char *path)
{
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if (kdws_compare_file_pathnames(scan->open_pathname,path))
      return scan->retain_count;
  return 0;
}
  
/*****************************************************************************/
/*                   kdws_manager::check_open_file_replaced                  */
/*****************************************************************************/

bool kdws_manager::check_open_file_replaced(const char *path)
{
  kdws_open_file_record *scan;
  for (scan=open_file_list; scan != NULL; scan=scan->next)
    if ((scan->save_pathname != NULL) &&
        kdws_compare_file_pathnames(scan->open_pathname,path))
      return true;
  return false;
}

/*****************************************************************************/
/*                        kdws_manager::InitInstance                         */
/*****************************************************************************/

BOOL kdws_manager::InitInstance()
{
  // _CrtSetBreakAlloc(1429);

  // Configure the application and load settings
  AfxEnableControlContainer();
#if _MFC_VER < 0x0500
#  ifdef _AFXDLL
     Enable3dControls(); // Call this when using MFC in a shared DLL
#  else
     Enable3dControlsStatic(); // Call this when linking to MFC statically
#  endif
#endif // _MFC_VER
  SetRegistryKey(_T("Kakadu"));
  LoadStdProfileSettings(4);
  settings.load_from_registry(this);
  kdws_string executable_path(_MAX_PATH);
  GetModuleFileName(m_hInstance,executable_path,_MAX_PATH);
  if (!executable_path.is_empty())
    register_protocols(executable_path);
  m_pMainWnd = NULL; // The `add_window' function will set the main window

  // Configure thread-related members
  main_thread_id = GetCurrentThreadId();
  presentation_thread = AfxBeginThread(presentation_thread_startup,this,
                                       THREAD_PRIORITY_NORMAL,0,
                                       CREATE_SUSPENDED,NULL);
  presentation_thread->m_bAutoDelete = FALSE;
  presentation_thread->ResumeThread();

  // Configure thread-safe and GUI-safe Kakadu error/warning handlers
  warn_message_queue.configure_posting(KDWS_CORE_MESSAGE,main_thread_id);
  err_message_queue.configure_posting(KDWS_CORE_MESSAGE,main_thread_id);
  kdu_customize_errors(&err_formatter);
  kdu_customize_warnings(&warn_formatter);

  // Create an initial window to get us started
  kdws_frame_window *window = new kdws_frame_window();
  if (!window->init(this))
    {
      delete window;
      return FALSE;
    }

  // See if there is an initial file to open
  kdws_string filename(m_lpCmdLine);
  char *fname = (char *) filename;
  while ((*fname != '\0') && ((*fname == ' ') || (*fname == '\t')))
    fname++;
  if (*fname != '\0')
    {
      if (*fname == '\"')
        {
          char *ch = strrchr(fname+1,'\"');
          if (ch != NULL)
            {
              fname++;
              *ch = '\0';
            }
        }
      window->open_file(fname);
    }
  return TRUE;
}

/*****************************************************************************/
/*                       kdws_manager::SaveAllModified                       */
/*****************************************************************************/

BOOL kdws_manager::SaveAllModified()
{
  if (!application_can_terminate())
    return FALSE;
  return TRUE;
}

/*****************************************************************************/
/*                        kdws_manager::ExitInstance                         */
/*****************************************************************************/

int kdws_manager::ExitInstance()
{
  send_application_terminating_messages();

  if (presentation_thread != NULL)
    {
      ::PostThreadMessage(presentation_thread->m_nThreadID,WM_QUIT,0,0);
      WaitForSingleObject(presentation_thread->m_hThread,INFINITE);
      delete presentation_thread;
      presentation_thread = NULL;
    }

  if (wakeup_timer_id != 0)
    { KillTimer(NULL,wakeup_timer_id); wakeup_timer_id = 0; }
  next_window_to_wake = NULL;
  last_known_key_wnd = NULL;

  settings.save_to_registry(this);
  return 0;
}

/*****************************************************************************/
/*                     kdws_manager::PreTranslateMessage                     */
/*****************************************************************************/

BOOL kdws_manager::PreTranslateMessage(MSG *pMsg)
{
  if (pMsg->hwnd == NULL)
    { // We provide special processing only for thread messages
      if (pMsg->message == KDWS_CORE_MESSAGE)
        { // Message delivered by a call to `kdu_error' or `kdu_warning' in a
          // different thread, via `kd_core_message_queue::pop_message'.
          while (err_message_queue.pop_message() != NULL);
          while (warn_message_queue.pop_message() != NULL);
          return TRUE;
        }
      else if (pMsg->message == KDWS_CLIENT_NOTIFICATION_MESSAGE)
        {
          this->handle_client_notification((int) pMsg->lParam);
          return TRUE;
        }
      else if (pMsg->message == WM_TIMER)
        {
          process_scheduled_wakeup();
          return TRUE;
        }
    }
  return CWinApp::PreTranslateMessage(pMsg);
}

/*****************************************************************************/
/*                           kdws_manager::OnIdle                            */
/*****************************************************************************/

BOOL kdws_manager::OnIdle(LONG lCount)
{
  // Make sure we didn't lose any important messages that may have been
  // posted to a full thread queue while we were busy.
  while (err_message_queue.pop_message() != NULL);
  while (warn_message_queue.pop_message() != NULL);
  clear_lost_client_notifications();

  while (1)
    {
      if (windows == NULL)
        return FALSE;
      kdws_window_list *elt = next_idle_window;
      if (elt == NULL)
        elt = next_idle_window = windows;
      while (!elt->wnd->on_idle())
        {
          elt = elt->next;
          if (elt == NULL)
            elt = windows;
          if (elt == next_idle_window)
            { // Been right around the entire window list and found no window
              // with any idle processing left to do.  Now we can let the
              // run loop sleep.
              return FALSE;
            }
        }

      // We just did some idle-time processing.  We need to return so
      // that pending user messages can be called, but if there are none,
      // we want to make sure that we will come back into this function to
      // do any remaining idle processing, rather than sleeping until a
      // user event occurs.  We do this by returning TRUE.
      next_idle_window = elt->next;
      break;
    }
  return TRUE;
}

/******************************************************************************/
/*                        MESSAGE MAP for kdws_manager                        */
/******************************************************************************/

BEGIN_MESSAGE_MAP(kdws_manager, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
  ON_COMMAND(ID_APP_EXIT, menu_AppExit)
  ON_COMMAND(ID_WINDOW_NEW, menu_WindowNew)
  ON_COMMAND(ID_WINDOW_ARRANGE, menu_WindowArrange)
  ON_COMMAND(ID_WINDOW_BROADCAST, menu_WindowBroadcast)
  ON_UPDATE_COMMAND_UI(ID_WINDOW_BROADCAST, can_do_WindowBroadcast)
END_MESSAGE_MAP()
  
/*****************************************************************************/
/*                         kdws_manager::OnAppAbout                          */
/*****************************************************************************/

void kdws_manager::OnAppAbout()
{
  CAboutDlg aboutDlg;
  aboutDlg.DoModal();
}

/*****************************************************************************/
/*                        kdws_manager::menu_AppExit                         */
/*****************************************************************************/

void kdws_manager::menu_AppExit()
{
  if (application_can_terminate())
    send_application_terminating_messages();
}

/*****************************************************************************/
/*                      kdws_manager::menu_WindowNew                         */
/*****************************************************************************/

void kdws_manager::menu_WindowNew()
{
  kdws_frame_window *wnd = new kdws_frame_window;
  try {
    if (!wnd->init(this))
      { kdu_error e; e << "Internal error creating new window."; }
  }
  catch (int) {
    delete this;
  }
  CRect wnd_frame;  wnd->GetWindowRect(&wnd_frame);
  kdu_coords frame_size;
  frame_size.x = wnd_frame.Width();
  frame_size.y = wnd_frame.Height();
  this->place_window(wnd,frame_size);
}

/*****************************************************************************/
/*                     kdws_manager::menu_WindowArrange                      */
/*****************************************************************************/

void kdws_manager::menu_WindowArrange()
{
  reset_placement_engine();
  kdws_frame_window *wnd;
  for (int w=0; (wnd = this->access_window(w)) != NULL; w++)
    {
      CRect wnd_frame;  wnd->GetWindowRect(&wnd_frame);
      kdu_coords frame_size;
      frame_size.x = wnd_frame.Width();
      frame_size.y = wnd_frame.Height();
      this->place_window(wnd,frame_size);
    }
}

/*****************************************************************************/
/*                     kdws_manager::menu_WindowBroadcast                    */
/*****************************************************************************/

void kdws_manager::menu_WindowBroadcast()
{
  bool indefinite = this->broadcast_actions_indefinitely;
  this->set_action_broadcasting(false,!indefinite);
}

/*****************************************************************************/
/*                    kdws_manager::can_do_WindowBroadcast                   */
/*****************************************************************************/

void kdws_manager::can_do_WindowBroadcast(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(TRUE);
  pCmdUI->SetCheck(this->broadcast_actions_indefinitely);
}
