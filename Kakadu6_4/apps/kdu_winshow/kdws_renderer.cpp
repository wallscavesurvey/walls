/*****************************************************************************/
// File: kdu_renderer.cpp [scope = APPS/WINSHOW]
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
   Implements the image management and rendering object, `kdws_renderer'.
This object lies the heart of the "kdu_show" application's functionality,
implementing menu commands, image processing, painting, navigation and JPIP
remote image browsing functionality.
******************************************************************************/

#include "stdafx.h"
#include "kdu_arch.h"
#include "kdws_manager.h"
#include "kdws_renderer.h"
#include "kdws_window.h"
#include "kdws_properties.h"
#include "kdws_metashow.h"
#include "kdws_metadata_editor.h"
#include "kdws_catalog.h"
#include <math.h>

#if defined KDU_X86_INTRINSICS
#  include <emmintrin.h>
#endif // KDU_X86_INTRINSICS

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define KDWS_OVERLAY_DEFAULT_INTENSITY 0.55F
#define KDWS_OVERLAY_FLASHING_STATES 3 // Num distinct flashing states
static float kdws_overlay_flash_factor[KDWS_OVERLAY_FLASHING_STATES] =
  {1.2F,0.6F,-0.6F};
static float kdws_overlay_flash_dwell[KDWS_OVERLAY_FLASHING_STATES] =
  {0.5F,0.5F,1.0F};
  /* Notes: when metadata overlay painting is enabled, they can either be
     displayed statically, using the nominal intensity (starts out with
     the default value given above, but can be adjusted up and down by the
     user), or dynamically (flashing).  In the dynamic case, the current
     value of the nominal intensity (as mentioned, this can be adjusted up
     or down by the user) is additionally scaled by a value which depends
     upon the current flashing state.  The flashing state machine moves
     cyclically through `KDWS_OVERLAY_FLASHING_STATES' states.  Each
     state is characterized by a scaling factor (mentioned above) and a
     dwelling period (measured in seconds); these are stored in the
     `kdws_overlay_flash_factor' and `kdws_overlay_flash_dwell' arrays. */
#define KDWS_OVERLAY_HIGHLIGHT_STATES 5
static float kdws_overlay_highlight_factor[KDWS_OVERLAY_HIGHLIGHT_STATES] =
  {2.0F,-2.0F,2.0F,-2.0F,2.0F};
static float kdws_overlay_highlight_dwell[KDWS_OVERLAY_HIGHLIGHT_STATES] =
  {0.125F,0.125F,0.125F,0.125F,2.0F};
  /* Notes: the above arrays play a similar role to `kdws_overlay_flash_factor'
     and `kdws_overlay_flash_dwell', but they apply to the temporary flashing
     of overlay information for a specific metadata node, in response to
     calls to `kdws_renderer::flash_imagery_for_metanode'. */

#define KDWS_FOCUS_MARGIN 2 // Enough to be absolutely sure we cover the focus
                            // pen.


/*===========================================================================*/
/*                             EXTERNAL FUNCTIONS                            */
/*===========================================================================*/

/*****************************************************************************/
/* EXTERN                     kdws_utf8_to_unicode                           */
/*****************************************************************************/

int kdws_utf8_to_unicode(const char *utf8, kdu_uint16 unicode[], int max_chars)
{
  int length = 0;
  while ((length < max_chars) && (*utf8 != '\0'))
    {
      int extra_chars = 0;
      kdu_uint16 val, tmp=((kdu_uint16)(*(utf8++))) & 0x00FF;
      if (tmp < 128)
        val = tmp;
      else if (tmp < 224)
        { val = (tmp-192); extra_chars = 1; }
      else if (tmp < 240)
        { val = (tmp-224); extra_chars = 2; }
      else if (tmp < 248)
        { val = (tmp-240); extra_chars = 3; }
      else if (tmp < 252)
        { val = (tmp-240); extra_chars = 4; }
      else
        { val = (tmp-252); extra_chars = 5; }
      for (; extra_chars > 0; extra_chars--)
        {
          val <<= 6;
          if (*utf8 != '\0')
            val |= (((kdu_uint16)(*(utf8++))) & 0x00FF) - 128;
        }
      unicode[length++] = val;
    }
  unicode[length] = 0;
  return length;
}


/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                         find_scale                                 */
/*****************************************************************************/

static inline float
  find_scale(int expand, int discard_levels)
  /* Returns `expand'*2^{-`discard_levels'}. */
{
  float scale = (float) expand;
  for (; discard_levels > 0; discard_levels--)
    scale *= 0.5F;
  return scale;
}

/*****************************************************************************/
/* STATIC                          copy_tile                                 */
/*****************************************************************************/

static void
  copy_tile(kdu_tile tile_in, kdu_tile tile_out)
  /* Walks through all code-blocks of the tile in raster scan order, copying
     them from the input to the output tile. */
{
  int c, num_components = tile_out.get_num_components();
  for (c=0; c < num_components; c++)
    {
      kdu_tile_comp comp_in;  comp_in  = tile_in.access_component(c);
      kdu_tile_comp comp_out; comp_out = tile_out.access_component(c);
      int r, num_resolutions = comp_out.get_num_resolutions();
      for (r=0; r < num_resolutions; r++)
        {
          kdu_resolution res_in;  res_in  = comp_in.access_resolution(r);
          kdu_resolution res_out; res_out = comp_out.access_resolution(r);
          int b, min_band;
          int num_bands = res_in.get_valid_band_indices(min_band);
          for (b=min_band; num_bands > 0; num_bands--, b++)
            {
              kdu_subband band_in;  band_in = res_in.access_subband(b);
              kdu_subband band_out; band_out = res_out.access_subband(b);
              kdu_dims blocks_in;  band_in.get_valid_blocks(blocks_in);
              kdu_dims blocks_out; band_out.get_valid_blocks(blocks_out);
              if ((blocks_in.size.x != blocks_out.size.x) ||
                  (blocks_in.size.y != blocks_out.size.y))
                { kdu_error e; e << "Transcoding operation cannot proceed: "
                  "Code-block partitions for the input and output "
                  "code-streams do not agree."; }
              kdu_coords idx;
              for (idx.y=0; idx.y < blocks_out.size.y; idx.y++)
                for (idx.x=0; idx.x < blocks_out.size.x; idx.x++)
                  {
                    kdu_block *in  = band_in.open_block(idx+blocks_in.pos);
                    kdu_block *out = band_out.open_block(idx+blocks_out.pos);
                    if (in->K_max_prime != out->K_max_prime)
                      { kdu_error e;
                        e << "Cannot copy blocks belonging to subbands with "
                             "different quantization parameters."; }
                    if ((in->size.x != out->size.x) ||
                        (in->size.y != out->size.y))  
                      { kdu_error e; e << "Cannot copy code-blocks with "
                        "different dimensions."; }
                    out->missing_msbs = in->missing_msbs;
                    if (out->max_passes < (in->num_passes+2))
                      out->set_max_passes(in->num_passes+2,false);
                    out->num_passes = in->num_passes;
                    int num_bytes = 0;
                    for (int z=0; z < in->num_passes; z++)
                      {
                        num_bytes += (out->pass_lengths[z]=in->pass_lengths[z]);
                        out->pass_slopes[z] = in->pass_slopes[z];
                      }
                    if (out->max_bytes < num_bytes)
                      out->set_max_bytes(num_bytes,false);
                    memcpy(out->byte_buffer,in->byte_buffer,(size_t) num_bytes);
                    band_in.close_block(in);
                    band_out.close_block(out);
                  }
            }
        }
    }
}

/*****************************************************************************/
/* STATIC                write_metadata_box_from_file                        */
/*****************************************************************************/

static void
  write_metadata_box_from_file(jp2_output_box *container, kdws_file *file)
{
  const char *filename = file->get_pathname();
  FILE *fp = fopen(filename,"rb");
  if (fp != NULL)
    {
      kdu_byte buf[512];
      size_t xfer_bytes;
      while ((xfer_bytes = fread(buf,1,512,fp)) > 0)
        container->write(buf,(int)xfer_bytes);
    }
  fclose(fp);
}

/*****************************************************************************/
/* STATIC                    write_metanode_to_jp2                           */
/*****************************************************************************/

static void
  write_metanode_to_jp2(jpx_metanode node, jp2_output_box &tgt,
                        kdws_file_services *file_server)
/* Writes the JP2 box represented by `node' or (if `node' is a numlist) from
   its descendants, to the `tgt' object.  Does not recursively visit
   descendants since this would require the writing of asoc boxes, which are
   not part of the JP2 file format.  Writes only boxes which are JP2
   compatible.  Uses the `file_server' object to resolve boxes whose contents
   have been defined by the metadata editor. */
{
  bool rres;
  int num_cs, num_l;
  int c, num_children = 0;
  node.count_descendants(num_children);
  if (node.get_numlist_info(num_cs,num_l,rres))
    {
      jpx_metanode scan;
      for (c=0; c < num_children; c++)
        if ((scan=node.get_descendant(c)).exists())
          write_metanode_to_jp2(scan,tgt,file_server);
      return;
    }
  if (num_children > 0)
    return; // Don't write nodes with descendants
  
  kdu_uint32 box_type = node.get_box_type();
  if ((box_type != jp2_label_4cc) && (box_type != jp2_xml_4cc) &&
      (box_type != jp2_iprights_4cc) && (box_type != jp2_uuid_4cc) &&
      (box_type != jp2_uuid_info_4cc))
    return; // Not a box we should write to a JP2 file
  
  int num_bytes;
  const char *label = node.get_label();
  if (label != NULL)
    {
      tgt.open_next(jp2_label_4cc);
      tgt.set_target_size((kdu_long)(num_bytes=(int)strlen(label)));
      tgt.write((kdu_byte *)label,num_bytes);
      tgt.close();
      return;
    }
  
  jp2_input_box in_box;
  if (node.open_existing(in_box))
    {
      tgt.open_next(box_type);
      if (in_box.get_remaining_bytes() > 0)
        tgt.set_target_size((kdu_long)(in_box.get_remaining_bytes()));
      kdu_byte buf[512];
      while ((num_bytes = in_box.read(buf,512)) > 0)
        tgt.write(buf,num_bytes);
      tgt.close();
      return;
    }
  
  int i_param;
  void *addr_param;
  kdws_file *file;
  if (node.get_delayed(i_param,addr_param) &&
      (addr_param == (void *)file_server) &&
      ((file = file_server->find_file(i_param)) != NULL))
    {
      tgt.open_next(box_type);
      write_metadata_box_from_file(&tgt,file);
      tgt.close();
    }
}


/*===========================================================================*/
/*                                 kdws_file                                 */
/*===========================================================================*/

/*****************************************************************************/
/*                           kdws_file::kdws_file                            */
/*****************************************************************************/

kdws_file::kdws_file(kdws_file_services *owner)
{
  this->owner = owner;
  unique_id = owner->next_unique_id++;
  retain_count = 0; // Caller needs to explicitly retain it
  pathname = NULL;
  is_temporary = false;
  next = NULL;
  if ((prev = owner->tail) == NULL)
    owner->head = this;
  else
    prev->next = this;
  owner->tail = this;
}

/*****************************************************************************/
/*                           kdws_file::~kdws_file                           */
/*****************************************************************************/

kdws_file::~kdws_file()
{
  if (owner != NULL)
    { // Unlink from owner.
      if (prev == NULL)
        {
          assert(this == owner->head);
          owner->head = next;
        }
      else
        prev->next = next;
      if (next == NULL)
        {
          assert(this == owner->tail);
          owner->tail = prev;
        }
      else
        next->prev = prev;
      prev = next = NULL;
      owner = NULL;
    }
  if (is_temporary && (pathname != NULL))
    remove(pathname);
  if (pathname != NULL)
    delete[] pathname;
}

/*****************************************************************************/
/*                             kdws_file::release                            */
/*****************************************************************************/

void kdws_file::release()
{
  retain_count--;
  if (retain_count == 0)
    delete this;
}

/*****************************************************************************/
/*                       kdws_file::create_if_necessary                      */
/*****************************************************************************/

bool kdws_file::create_if_necessary(const char *initializer)
{
  if ((pathname == NULL) || !is_temporary)
    return false;
  const char *mode = (initializer == NULL)?"rb":"r";
  FILE *file = fopen(pathname,mode);
  if (file != NULL)
    { // File already exists
      fclose(file);
      return false;
    }
  mode = (initializer == NULL)?"wb":"w";
  file = fopen(pathname,mode);
  if (file == NULL)
    return false; // Cannot write to file
  if (initializer != NULL)
    fwrite(initializer,1,strlen(initializer),file);
  fclose(file);
  return true;
}


/*===========================================================================*/
/*                             kdws_file_services                            */
/*===========================================================================*/

/*****************************************************************************/
/*                   kdws_file_services::kdws_file_services                  */
/*****************************************************************************/

kdws_file_services::kdws_file_services(const char *source_pathname)
{
  head = tail = NULL;
  next_unique_id = 1;
  editors = NULL;
  base_pathname_confirmed = false;
  base_pathname = NULL;
  if (source_pathname != NULL)
    {
      base_pathname = new char[strlen(source_pathname)+8];
      strcpy(base_pathname,source_pathname);
      const char *cp = strrchr(base_pathname,'.');
      if (cp == NULL)
        strcat(base_pathname,"_meta_");
      else
        strcpy(base_pathname + (cp-base_pathname),"_meta_");
    }
}

/*****************************************************************************/
/*                   kdws_file_services::~kdws_file_services                 */
/*****************************************************************************/

kdws_file_services::~kdws_file_services()
{
  while (head != NULL)
    delete head;
  if (base_pathname != NULL)
    delete[] base_pathname;
  kdws_file_editor *editor;
  while ((editor=editors) != NULL)
    {
      editors = editor->next;
      delete editor;
    }
}

/*****************************************************************************/
/*                 kdws_file_services::find_new_base_pathname                */
/*****************************************************************************/

bool kdws_file_services::find_new_base_pathname()
{
  if (base_pathname != NULL)
    delete[] base_pathname;
  base_pathname_confirmed = false;
  base_pathname = new char[L_tmpnam+8];
  tmpnam(base_pathname);
  FILE *fp = fopen(base_pathname,"wb");
  if (fp == NULL)
    return false;
  fclose(fp);
  remove(base_pathname);
  base_pathname_confirmed = true;
  strcat(base_pathname,"_meta_");
  return true;
}

/*****************************************************************************/
/*                 kdws_file_services::confirm_base_pathname                 */
/*****************************************************************************/

void kdws_file_services::confirm_base_pathname()
{
  if (base_pathname_confirmed)
    return;
  if (base_pathname != NULL)
    {
      FILE *fp = fopen(base_pathname,"rb");
      if (fp != NULL)
        {
          fclose(fp); // Existing base pathname already exists; can't test it
          // easily for writability; safest to make new base path
          find_new_base_pathname();
        }
      else
        {
          fp = fopen(base_pathname,"wb");
          if (fp != NULL)
            { // All good; can write to this volume
              fclose(fp);
              remove(base_pathname);
              base_pathname_confirmed = true;
            }
          else
            find_new_base_pathname();
        }
    }
  if (base_pathname == NULL)
    find_new_base_pathname();
}

/*****************************************************************************/
/*                    kdws_file_services::retain_known_file                  */
/*****************************************************************************/

kdws_file *kdws_file_services::retain_known_file(const char *pathname)
{
  kdws_file *file;
  for (file=head; file != NULL; file=file->next)
    if (strcmp(file->pathname,pathname) == 0)
      break;
  if (file == NULL)
    {
      file = new kdws_file(this);
      file->pathname = new char[strlen(pathname)+1];
      strcpy(file->pathname,pathname);
    }
  file->is_temporary = false;
  file->retain();
  return file;
}

/*****************************************************************************/
/*                     kdws_file_services::retain_tmp_file                   */
/*****************************************************************************/

kdws_file *kdws_file_services::retain_tmp_file(const char *suffix)
{
  if (!base_pathname_confirmed)
    confirm_base_pathname();
  kdws_file *file = new kdws_file(this);
  file->pathname = new char[strlen(base_pathname)+81];
  strcpy(file->pathname,base_pathname);
  char *cp = file->pathname + strlen(file->pathname);
  int extra_id = 0;
  bool found_new_filename = false;
  while (!found_new_filename)
    {
      if (extra_id == 0)
        sprintf(cp,"_tmp_%d.%s",file->unique_id,suffix);
      else
        sprintf(cp,"_tmp%d_%d.%s",extra_id,file->unique_id,suffix);
      FILE *fp = fopen(file->pathname,"rb");
      if (fp != NULL)
        {
          fclose(fp); // File already exists
          extra_id++;
        }
      else
        found_new_filename = true;
    }
  file->is_temporary = true;
  file->retain();
  return file;
}

/*****************************************************************************/
/*                        kdws_file_services::find_file                      */
/*****************************************************************************/

kdws_file *kdws_file_services::find_file(int identifier)
{
  kdws_file *scan;
  for (scan=head; scan != NULL; scan=scan->next)
    if (scan->unique_id == identifier)
      return scan;
  return NULL;
}

/*****************************************************************************/
/*               kdws_file_services::get_editor_for_doc_type                 */
/*****************************************************************************/

kdws_file_editor *
  kdws_file_services::get_editor_for_doc_type(const char *doc_suffix,
                                              int which)
{
  kdws_file_editor *scan;
  int n;
  
  for (n=0, scan=editors; scan != NULL; scan=scan->next)
    if (strcmp(scan->doc_suffix,doc_suffix) == 0)
      {
        if (n == which)
          return scan;
        n++;
      }
  
  if (n > 0)
    return NULL;
  
  // If we get here, we didn't find any matching editors at all.  Let's see if
  // we can add some.
  kdws_string extension(79);
  char *ext_string = extension;
  ext_string[0] = '.';
  strncpy(ext_string+1,doc_suffix,78);
  DWORD val_type, prog_id_len;
  kdws_string prog_id(255); // Only collect prog-id's with 255 chars or less
  HKEY ext_key;
  if (RegOpenKeyEx(HKEY_CLASSES_ROOT,extension,0,KEY_READ,
                   &ext_key) == ERROR_SUCCESS)
    {
      HKEY openwith_key;
      if (RegOpenKeyEx(ext_key,_T("OpenWithProgids"),0,KEY_READ,
                       &openwith_key) == ERROR_SUCCESS)
        {
          for (n=0; true; n++)
            {
              prog_id_len = 255;
              prog_id.clear();
              if (RegEnumValue(openwith_key,n,prog_id,&prog_id_len,
                               NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
                break;
              add_editor_progid_for_doc_type(doc_suffix,prog_id);
            }
          RegCloseKey(openwith_key);
        }
      kdws_string valname(79);
      for (n=0; true; n++)
        {
          DWORD name_len=79;
          prog_id_len = 255;
          prog_id.clear();
          valname.clear();
          if (RegEnumValue(ext_key,n,valname,&name_len,NULL,
                           &val_type,(LPBYTE)((_TCHAR *) prog_id),
                           &prog_id_len) != ERROR_SUCCESS)
            break;
          if ((prog_id_len > 0) && (val_type == REG_SZ) &&
              valname.is_empty())
            add_editor_progid_for_doc_type(doc_suffix,prog_id);
        }
      RegCloseKey(ext_key);
    }

  // Now go back and try to answer the request again
  for (n=0, scan=editors; scan != NULL; scan=scan->next)
    if (strcmp(scan->doc_suffix,doc_suffix) == 0)
      {
        if (n == which)
          return scan;
        n++;
      }
  
  return NULL;
}

/*****************************************************************************/
/*           kdws_file_services::add_editor_progid_for_doc_type              */
/*****************************************************************************/

void
  kdws_file_services::add_editor_progid_for_doc_type(const char *doc_suffix,
                                                     kdws_string &prog_id)
{
  HKEY prog_key;
  if (RegOpenKeyEx(HKEY_CLASSES_ROOT,prog_id,0,KEY_READ,
                   &prog_key) == ERROR_SUCCESS)
    {
      HKEY shell_key;
      if (RegOpenKeyEx(prog_key,_T("shell"),0,KEY_READ,
                       &shell_key) == ERROR_SUCCESS)
        {
          HKEY action_key;
          for (int action=0; action < 2; action++)
            {
              const _TCHAR *action_verb = (action==1)?_T("edit"):_T("open");
              if (RegOpenKeyEx(shell_key,action_verb,0,KEY_READ,
                               &action_key) != ERROR_SUCCESS)
                continue;
              HKEY command_key;
              if (RegOpenKeyEx(action_key,_T("command"),0,KEY_READ,
                               &command_key) == ERROR_SUCCESS)
                {
                  kdws_string name_string(80);
                  kdws_string command_string(MAX_PATH+80);
                  for (int n=0; true; n++)
                    {
                      DWORD val_type;
                      DWORD name_chars = 80;
                      DWORD command_chars = MAX_PATH+80;
                      if (RegEnumValue(command_key,n,name_string,&name_chars,
                                       NULL,&val_type,
                                       (LPBYTE)((_TCHAR *) command_string),
                                       &command_chars) != ERROR_SUCCESS)
                        break;
                      if (!name_string.is_empty())
                        continue;

                      // Now we have the default value for the "command" key
                      if (val_type == REG_EXPAND_SZ)
                        { // Need to expand environment variables.
                          kdws_string expand_string(MAX_PATH+79);
                          DWORD expand_len =
                            ExpandEnvironmentStrings(command_string,
                                                     expand_string,
                                                     MAX_PATH+79);
                          if ((expand_len == 0) ||
                              (expand_len > (MAX_PATH+79)))
                            break; // Expand not successful
                          command_string.clear();
                          strcpy(command_string,expand_string);
                        }
                      else if (val_type != REG_SZ)
                        break;
                      char *sep, *app_path = command_string;
                      while ((*app_path == ' ') || (*app_path == '\t'))
                        app_path++;
                      bool have_arg = false;
                      for (sep=app_path; *sep != '\0'; sep++)
                        if ((sep[0] == '%') && (sep[1] == '1'))
                        { have_arg = true; break; }
                      if ((sep > app_path) && have_arg && (sep[-1]=='\"'))
                        sep--;
                      while ((sep > command_string) &&
                             ((sep[-1] == ' ') || (sep[-1] == '\t')))
                        sep--;
                      *sep = '\0';
                      if (!have_arg)
                        { // Still OK if "/dde" is found
                          if (((sep-app_path) < 6) ||
                              (sep[-4] != '/') || (sep[-3] != 'd') ||
                              (sep[-2] != 'd') || (sep[-1] != 'e'))
                            break;
                        }
                      if (*app_path != '\0')
                        add_editor_for_doc_type(doc_suffix,app_path);
                      break;
                    }
                  RegCloseKey(command_key);
                }
              RegCloseKey(action_key);
            }
          RegCloseKey(shell_key);
        }
      RegCloseKey(prog_key);
    }
}

/*****************************************************************************/
/*               kdws_file_services::add_editor_for_doc_type                 */
/*****************************************************************************/

kdws_file_editor *
  kdws_file_services::add_editor_for_doc_type(const char *doc_suffix,
                                              const char *app_pathname)
{
  kdws_file_editor *scan, *prev;
  for (scan=editors, prev=NULL; scan != NULL; prev=scan, scan=scan->next)
    if (strcmp(scan->app_pathname,app_pathname) == 0)
      break;
  
  if (scan != NULL)
    { // Found an existing editor
      if (prev == NULL)
        editors = scan->next;
      else
        prev->next = scan->next;
      scan->next = editors;
      editors = scan;
      return scan;
    }
  
  scan = new kdws_file_editor;
  scan->doc_suffix = new char[strlen(doc_suffix)+1];
  strcpy(scan->doc_suffix,doc_suffix);
  scan->app_pathname = new char[strlen(app_pathname)+1];
  strcpy(scan->app_pathname,app_pathname);
  
  const char *cp, *name_start = scan->app_pathname;
  for (cp=name_start; *cp != '\0'; cp++)
    if ((*cp == '\"') && (scan->app_pathname[0] == '\"') && (cp > name_start))
      break;
    else if ((cp[0] == '.') && (toupper(cp[1]) == (int)'E') &&
             (toupper(cp[2]) == (int)'X') && (toupper(cp[3]) == (int)'E'))
      break;
    else
      if (((*cp == '/') || (*cp == '\\') || (*cp == ':')) && (cp[1] != '\0'))
        name_start = cp+1;
  scan->app_name = new char[(cp-name_start)+1];
  memcpy(scan->app_name,name_start,(cp-name_start));
  scan->app_name[cp-name_start] = '\0';

  kdws_file_editor *dup;
  for (dup=editors; dup != NULL; dup=dup->next)
    if (strcmp(dup->app_name,scan->app_name) == 0)
      {
        delete scan;
        return dup;
      }
  scan->next = editors;
  editors = scan;
  return scan;
}


/*===========================================================================*/
/*                            kdws_playmode_stats                            */
/*===========================================================================*/

/*****************************************************************************/
/*                        kdws_playmode_stats::update                        */
/*****************************************************************************/

void kdws_playmode_stats::update(int frame_idx, double start_time,
                                 double end_time, double cur_time)
{
  if ((num_updates_since_reset > 0) && (last_frame_idx == frame_idx))
    return; // Duplicate frame (should not happen, the way this
            // function is currently used)
  if (num_updates_since_reset > 0)
    {
      double interval = cur_time - cur_time_at_last_update;
      if (interval < 0.0001)
        interval = 0.0001;
      if (num_updates_since_reset == 1)
        mean_time_between_frames = interval;
      else
        {
          double update_weight = 0.5 * (mean_time_between_frames + interval);
          if (update_weight > 1.0)
            update_weight = 1.0;
          else if (update_weight < 0.025)
            update_weight = 0.025;
          if ((update_weight*num_updates_since_reset) < 1.0)
            update_weight = 1.0 / (double) num_updates_since_reset;
          mean_time_between_frames = interval*update_weight +
            (mean_time_between_frames*(1.0-update_weight));
        }
    }
  last_frame_idx = frame_idx;
  cur_time_at_last_update = cur_time;
  num_updates_since_reset++;
}


/* ========================================================================= */
/*                               kd_bitmap_buf                               */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_bitmap_buf::create_bitmap                       */
/*****************************************************************************/

void
  kd_bitmap_buf::create_bitmap(kdu_coords size)
{
  if (bitmap != NULL)
    {
      assert(this->size == size);
      return;
    }

  memset(&bitmap_info,0,sizeof(bitmap_info));
  bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info.bmiHeader.biWidth = size.x;
  bitmap_info.bmiHeader.biHeight = -size.y;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = 32;
  bitmap_info.bmiHeader.biCompression = BI_RGB;
  void *buf_addr;
  bitmap = CreateDIBSection(NULL,&bitmap_info,DIB_RGB_COLORS,&buf_addr,NULL,0);
  if (bitmap == NULL)
    { kdu_error e; e << "Unable to allocate sufficient bitmap surfaces "
      "to service `kdu_region_compositor'."; }
  this->buf = (kdu_uint32 *) buf_addr;
  this->row_gap = size.x;
  this->size = size;
}

/*****************************************************************************/
/*                      kd_bitmap_buf::flush_from_cache                      */
/*****************************************************************************/

void
  kd_bitmap_buf::flush_from_cache()
{
#if defined KDU_X86_INTRINSICS
  if (kdu_mmx_level < 2)
    return;
  kdu_uint32 *sp=this->buf, *ep=sp+size.x*size.y-1;
  for (ep-=128; sp < ep; sp+=128)
    {
      _mm_clflush(sp+ 0); _mm_clflush(sp+16);
      _mm_clflush(sp+32); _mm_clflush(sp+48);
      _mm_clflush(sp+64); _mm_clflush(sp+80);
      _mm_clflush(sp+96); _mm_clflush(sp+112);
    }
  for (ep+=128; sp < ep; sp+=16)
    _mm_clflush(sp);
  _mm_clflush(ep);
  _mm_mfence();
#endif // KDU_X86_INTRINSICS
}


/* ========================================================================= */
/*                            kd_bitmap_compositor                           */
/* ========================================================================= */

/*****************************************************************************/
/*                    kd_bitmap_compositor::allocate_buffer                  */
/*****************************************************************************/

kdu_compositor_buf *
  kd_bitmap_compositor::allocate_buffer(kdu_coords min_size,
                                        kdu_coords &actual_size,
                                        bool read_access_required)
{
  if (min_size.x < 1) min_size.x = 1;
  if (min_size.y < 1) min_size.y = 1;
  actual_size = min_size;
  int row_gap = actual_size.x;

  kd_bitmap_buf *prev=NULL, *elt=NULL;

  // Start by trying to find a compatible buffer on the free list.
  for (elt=free_list, prev=NULL; elt != NULL; prev=elt, elt=elt->next)
    if (elt->size == actual_size)
      break;

  bool need_init = false;
  if (elt != NULL)
    { // Remove from the free list
      if (prev == NULL)
        free_list = elt->next;
      else
        prev->next = elt->next;
      elt->next = NULL;
    }
  else
    {
      // Delete the entire free list: one way to avoid accumulating buffers
      // whose sizes are no longer helpful
      while ((elt=free_list) != NULL)
        { free_list = elt->next;  delete elt; }

      // Allocate a new element
      elt = new kd_bitmap_buf;
      need_init = true;
    }
  elt->next = active_list;
  active_list = elt;
  elt->set_read_accessibility(read_access_required);
  if (need_init)
    elt->create_bitmap(actual_size);
  return elt;
}

/*****************************************************************************/
/*                    kd_bitmap_compositor::delete_buffer                    */
/*****************************************************************************/

void
  kd_bitmap_compositor::delete_buffer(kdu_compositor_buf *_buffer)
{
  kd_bitmap_buf *buffer = (kd_bitmap_buf *) _buffer;
  kd_bitmap_buf *scan, *prev;
  for (prev=NULL, scan=active_list; scan != NULL; prev=scan, scan=scan->next)
    if (scan == buffer)
      break;
  assert(scan != NULL);
  if (prev == NULL)
    active_list = scan->next;
  else
    prev->next = scan->next;

  scan->next = free_list;
  free_list = scan;
}


/* ========================================================================= */
/*                               kdws_rel_dims                               */
/* ========================================================================= */

/*****************************************************************************/
/*                             kdws_rel_dims::set                            */
/*****************************************************************************/

bool kdws_rel_dims::set(kdu_dims region, kdu_dims container)
{
  kdu_dims content = region & container;
  centre_x = centre_y = size_x = size_y = 0.0;
  if (!container)
    return false;
  if (!content.is_empty())
    {
      double fact_x = 1.0 / container.size.x;
      double fact_y = 1.0 / container.size.y;
      size_x = fact_x * content.size.x;
      size_y = fact_y * content.size.y;
      centre_x = fact_x * (content.pos.x-container.pos.x) + 0.5*size_x;
      centre_y = fact_y * (content.pos.y-container.pos.y) + 0.5*size_y;
    }
  return (content == region);
}

/*****************************************************************************/
/*                     kdws_rel_dims::apply_to_container                     */
/*****************************************************************************/

kdu_dims kdws_rel_dims::apply_to_container(kdu_dims container)
{
  kdu_dims result;
  result.size.x = (int)(0.5 + size_x * container.size.x);
  result.size.y = (int)(0.5 + size_y * container.size.y);
  result.pos.x = (int)(0.5 + (centre_x-0.5*size_x) * container.size.x);
  result.pos.y = (int)(0.5 + (centre_y-0.5*size_y) * container.size.y);
  result.pos += container.pos;
  return result;
}


/* ========================================================================= */
/*                               kdws_renderer                               */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdws_renderer::kdws_renderer                       */
/*****************************************************************************/

kdws_renderer::kdws_renderer(kdws_frame_window *owner,
                             kdws_image_view *image_view,
                             kdws_manager *manager)
{
  this->window = owner;
  this->image_view = image_view;
  this->manager = manager;
  this->metashow = NULL;
  this->window_identifier = manager->get_window_identifier(owner);
  this->catalog_source = NULL;
  this->catalog_closed = false;
  this->editor = NULL;

  processing_call_to_open_file = false;
  open_file_pathname = NULL;
  open_file_urlname = NULL;
  open_filename = NULL;
  last_save_pathname = NULL;
  save_without_asking = false;
  duplication_src = NULL;

  compositor = NULL;
  num_recommended_threads = kdu_get_num_processors();
  if (num_recommended_threads > 4)
    num_recommended_threads = 4;
  if (num_recommended_threads < 2)
    num_recommended_threads = 0;
  num_threads = num_recommended_threads;
  if (num_threads > 0)
    {
      thread_env.create();
      for (int k=1; k < num_threads; k++)
        if (!thread_env.add_thread())
          num_threads = k;
    }
  in_idle = false;
  processing_suspended = false;
  defer_process_regions_deadline = -1.0;

  file_server = NULL;
  have_unsaved_edits = false;

  jpip_client = NULL;
  client_request_queue_id = -1;
  one_time_jpip_request = false;
  respect_initial_jpip_request = false;
  enable_region_posting = false;
  catalog_has_novel_metareqs = false;
  jpip_client_received_queue_bytes = 0;
  jpip_client_received_total_bytes = 0;
  jpip_client_received_bytes_since_last_refresh = false;
  jpip_refresh_interval = 0.8;

  jpip_client_status = "<no JPIP>";
  jpip_interval_start_bytes = 0;
  jpip_interval_start_time = 0.0;
  jpip_client_data_rate = -1.0;

  allow_seeking = true;
  error_level = 0;
  max_display_layers = 1<<16;
  transpose = vflip = hflip = false;
  min_rendering_scale = -1.0F;
  max_rendering_scale = -1.0F;
  rendering_scale = 1.0F;
  max_components = max_codestreams = max_compositing_layer_idx = -1;
  single_component_idx = single_codestream_idx = single_layer_idx = 0;
  frame_idx = 0;
  frame_start = frame_end = 0.0;
  frame = NULL;
  frame_iteration = 0;
  frame_expander.reset();
  fit_scale_to_window = false;
  configuration_complete = false;
  status_id = KDS_STATUS_LAYER_RES;
  pending_show_metanode = jpx_metanode();

  view_mutex.create();
  view_mutex_locked = false;
  in_playmode = false;
  playmode_repeat = false;
  playmode_reverse = false;
  pushed_last_frame = false;
  use_native_timing = true;
  custom_frame_interval = native_playback_multiplier = 1.0F;
  playclock_offset = 0.0;
  playclock_base = 0.0;
  playmode_frame_queue_size = 3;
  playmode_frame_idx = -1;
  num_expired_frames_on_queue = 0;
  frame_presenter = manager->get_frame_presenter(window);

  last_render_completion_time = -100.0;
  last_status_update_time = -100.0;
  next_render_refresh_time = -1.0;
  next_overlay_flash_time = -1.0;
  next_frame_end_time = -1.0;
  next_status_update_time = -1.0;
  next_scheduled_wakeup_time = -100.0;

  pixel_scale = 1;
  image_dims.pos = image_dims.size = kdu_coords(0,0);
  buffer_dims = view_dims = image_dims;
  view_centre_x = view_centre_y = 0.0;
  view_centre_known = false;

  enable_focus_box = false;
  highlight_focus = true;
  focus_pen_outside.CreatePen(PS_SOLID,1,RGB(153,51,0)); // Brown Pen
  focus_pen_inside.CreatePen(PS_SOLID,1,RGB(255,255,102)); // Yellow Pen

  overlays_enabled = false;
  overlay_flashing_state = 1;
  overlay_highlight_state = 0;
  overlay_nominal_intensity = KDWS_OVERLAY_DEFAULT_INTENSITY;
  overlay_size_threshold = 1;
  kdu_uint32 params[KDWS_NUM_OVERLAY_PARAMS] = KDWS_OVERLAY_PARAMS;
  for (int p=0; p < KDWS_NUM_OVERLAY_PARAMS; p++)
    overlay_params[p] = params[p];
  
  buffer = NULL;
  compatible_dc.CreateCompatibleDC(NULL);
  strip_extent = kdu_coords(0,0);
  stripmap = NULL;
  stripmap_buffer = NULL;
  for (int i=0; i < 256; i++)
    {
      foreground_lut[i] = (kdu_byte)(40 + ((i*216) >> 8));
      background_lut[i] = (kdu_byte)((i*216) >> 8);
    }

  set_key_window_status(window->is_key_window());
}

/*****************************************************************************/
/*                        kdws_renderer::~kdws_renderer                      */
/*****************************************************************************/

kdws_renderer::~kdws_renderer()
{
  frame_presenter = NULL; // Avoid access to potentially non-existent object
  perform_essential_cleanup_steps();

  if (file_server != NULL)
    { delete file_server; file_server = NULL; }
  if (compositor != NULL)
    { delete compositor; compositor = NULL; }
  if (metashow != NULL)
    metashow->close();
  if (editor != NULL)
    {
      editor->close();
      editor = NULL;
    }
  if (thread_env.exists())
    thread_env.destroy();
  file_in.close();
  jpx_in.close();
  mj2_in.close();
  jp2_family_in.close();
  cache_in.close();
  if (jpip_client != NULL)
    {
      jpip_client->disconnect(false,20000,client_request_queue_id,false);
      // Give it a long timeout so as to avoid accidentally breaking any
      // other request queues by forcing a timeout, but we won't wait for
      // closure here.  Forced closure with a shorter timeout can be
      // performed in the `release_jpip_client' call below, if there
      // are no windows left using the client.
      manager->release_jpip_client(jpip_client,window);
      jpip_client = NULL;
    }
  if (stripmap != NULL)
    { DeleteObject(stripmap); stripmap = NULL; stripmap_buffer = NULL; }
  if (last_save_pathname != NULL)
    delete[] last_save_pathname;
  focus_pen_inside.DeleteObject();
  focus_pen_outside.DeleteObject();
  assert(!view_mutex_locked);
  view_mutex.destroy();
}

/*****************************************************************************/
/*             kdws_renderer::perform_essential_cleanup_steps                */
/*****************************************************************************/

void kdws_renderer::perform_essential_cleanup_steps()
{
  if (frame_presenter != NULL)
    frame_presenter->reset(); // Make sure there is no asynchronous painting
  if (file_server != NULL)
    delete file_server;
  file_server = NULL;
  if ((manager != NULL) && (open_file_pathname != NULL))
    {
      if (compositor != NULL)
        { delete compositor; compositor = NULL; }
      file_in.close(); // Unlocks any files
      jp2_family_in.close(); // Unlocks any files
      manager->release_open_file_pathname(open_file_pathname,window);
      open_file_pathname = NULL;
      open_filename = NULL;
    }
}

/*****************************************************************************/
/*                          kdws_renderer::close_file                        */
/*****************************************************************************/

void
  kdws_renderer::close_file()
{
  // Prevent other windows from accessing the data source
  if (catalog_source != NULL)
    catalog_source->deactivate();
  if (metashow != NULL)
    metashow->deactivate();

  perform_essential_cleanup_steps();

  stop_playmode();

  // Cancel any pending wakeups
  last_render_completion_time = -100.0;
  last_status_update_time = -100.0;
  next_render_refresh_time = -1.0;
  next_overlay_flash_time = -1.0;
  next_frame_end_time = -1.0;
  next_status_update_time = -1.0;
  next_scheduled_wakeup_time = -100.0;
  manager->schedule_wakeup(window,-1.0);

  // Close down all information rendering machinery
  if (compositor != NULL)
    { delete compositor; compositor = NULL; }
  if (editor != NULL)
    {
      editor->close();
      assert(editor == NULL);
    }
  file_in.close();
  cache_in.close();
  jpx_in.close();
  mj2_in.close();
  jp2_family_in.close();

  // Reset all file path names and related attributes
  processing_call_to_open_file = false;
  open_file_pathname = NULL;
  open_file_urlname = NULL;
  open_filename = NULL;
  if (last_save_pathname != NULL)
    { delete[] last_save_pathname; last_save_pathname = NULL; }
  save_without_asking = false;

  if (file_server != NULL)
    { delete file_server; file_server = NULL; }
  have_unsaved_edits = false;

  // Shut down JPIP client
  if (jpip_client != NULL)
    {
      jpip_client->disconnect(false,20000,client_request_queue_id,false);
          // Give it a long timeout so as to avoid accidentally breaking any
          // other request queues by forcing a timeout, but we won't wait for
          // closure here.  Forced closure with a shorter timeout can be
          // performed in the `release_jpip_client' call below, if there
          // are no windows left using the client.
      manager->release_jpip_client(jpip_client,window);
      jpip_client = NULL;
    }

  // Reset state variables
  client_roi.init();
  client_request_queue_id = -1;
  one_time_jpip_request = false;
  respect_initial_jpip_request = false;
  enable_region_posting = false;
  catalog_has_novel_metareqs = false;
  jpip_client_received_queue_bytes = 0;
  jpip_client_received_total_bytes = 0;
  jpip_client_received_bytes_since_last_refresh = false;
  jpip_client_status = "<no JPIP>";
  jpip_interval_start_bytes = 0;
  jpip_interval_start_time = 0.0;
  jpip_client_data_rate = -1.0;
  
  composition_rules = jpx_composition(NULL);
  processing_suspended = false;
  defer_process_regions_deadline = -1.0;
  fit_scale_to_window = false;
  frame_idx = 0;
  frame_start = frame_end = 0.0;
  frame = NULL;
  frame_iteration = 0;
  frame_expander.reset();
  configuration_complete = false;
  transpose = vflip = hflip = false;
  pending_show_metanode = jpx_metanode();
  last_show_metanode = jpx_metanode();

  image_dims.pos = image_dims.size = kdu_coords(0,0);
  buffer_dims = view_dims = image_dims;
  view_centre_known = false;
  enable_focus_box = false;
  rel_focus.reset();
  overlays_enabled = false;
  overlay_flashing_state = 1;
  overlay_highlight_state = 0;
  highlight_metanode = jpx_metanode();
  overlay_nominal_intensity = KDWS_OVERLAY_DEFAULT_INTENSITY;
  overlay_restriction = last_selected_overlay_restriction = jpx_metanode();

  buffer = NULL;

  // Update GUI elements to reflect "closed" status
  if ((catalog_source != NULL) && (window != NULL))
    window->remove_metadata_catalog();
  else if (catalog_source != NULL)
    set_metadata_catalog_source(NULL); // Just in case the window went away
  catalog_closed = false;
  char title[80];
  sprintf(title,"kdu_show %d: <no data>",window_identifier);
  window->SetWindowText(kdws_string(title));
  window->remove_progress_bar();
  if (image_view != NULL)
    {
      image_view->set_max_view_size(kdu_coords(20000,20000),pixel_scale);
      if (image_view->GetSafeHwnd() != NULL)
        image_view->Invalidate();
    }
  manager->declare_window_empty(window,true);
}

/*****************************************************************************/
/*                   kdws_renderer::set_key_window_status                    */
/*****************************************************************************/

void kdws_renderer::set_key_window_status(bool is_key_window)
{
  jpip_refresh_interval = (is_key_window)?0.8:4.0;
  if (!is_key_window)
    return;
  if ((jpip_client != NULL) && !jpip_client_received_bytes_since_last_refresh)
    { // Perhaps there are more bytes available anyway -- because of activity
      // in a different window which shares the same cache; better check here,
      // especially because this might augment the metadata catalog
      // significantly, which is very useful for a key window.
      kdu_long new_bytes = jpip_client->get_received_bytes(-1);
      if (new_bytes > jpip_client_received_total_bytes)
        {
          jpip_client_received_total_bytes = new_bytes;
          jpip_client_received_bytes_since_last_refresh = true;
          if ((catalog_source != NULL) && (editor == NULL))
            catalog_source->update_metadata();
        }
    }
  if (jpip_client_received_bytes_since_last_refresh &&
      (compositor != NULL) && compositor->is_processing_complete())
    { // May need to refresh display or schedule/reschedule a refresh
      double cur_time = manager->get_current_time();
      if (cur_time > (last_render_completion_time+jpip_refresh_interval - 0.1))
        {
          next_render_refresh_time = -1.0;
          refresh_display();
        }
      else
        schedule_render_refresh_wakeup(last_render_completion_time +
                                       jpip_refresh_interval);
    }
}

/*****************************************************************************/
/*                     kdws_renderer::open_as_duplicate                      */
/*****************************************************************************/

void kdws_renderer::open_as_duplicate(kdws_renderer *src)
{
  this->duplication_src = src;
  if (src->open_file_pathname != NULL)
    window->open_file(src->open_file_pathname);
       // We could just call `open_file' directly, but the above statement
       // interrogates the user if the file has been modified.
  else if (src->open_file_urlname != NULL)
    open_file(src->open_file_urlname);
  this->duplication_src = NULL; // Because the `src' might disappear
}

/*****************************************************************************/
/*                          kdws_renderer::open_file                         */
/*****************************************************************************/

void
  kdws_renderer::open_file(const char *filename_or_url)
{
  if (processing_call_to_open_file)
    return; // Prevent recursive entry to this function from event handlers
  if (filename_or_url != NULL)
    {
      close_file();
      manager->declare_window_empty(window,false);
    }
  processing_call_to_open_file = true;
  assert(compositor == NULL);
  enable_focus_box = false;
  focus_box.size = kdu_coords(0,0);
  client_roi.init();
  processing_suspended = false;
  enable_region_posting = false;
  bool is_jpip = false;
  try {
      if (filename_or_url != NULL)
        {
          const char *url_host=NULL, *url_request=NULL;
          if ((url_host =
               kdu_client::check_compatible_url(filename_or_url,false,NULL,
                                                &url_request)) != NULL)
            { // Open as an interactive client-server application
              is_jpip = true;
              if (url_request == NULL)
                { kdu_error e;
                  e << "Illegal JPIP request, \"" << filename_or_url
                    << "\".  General form is \"<prot>://<hostname>[:<port>]/"
                       "<resource>[?<query>]\"\nwhere <prot> is \"jpip\" or "
                       "\"http\" and forward slashes may be substituted "
                       "for back slashes if desired.";
                }
              open_file_urlname =
                manager->retain_jpip_client(NULL,NULL,filename_or_url,
                                                jpip_client,
                                                client_request_queue_id,
                                                window);
              one_time_jpip_request = jpip_client->is_one_time_request();
              respect_initial_jpip_request = one_time_jpip_request;
              if ((client_request_queue_id == 0) && (!one_time_jpip_request) &&
                  jpip_client->connect_request_has_non_empty_window())
                respect_initial_jpip_request = true;
              open_filename = jpip_client->get_target_name();
              jpip_client_status =
                jpip_client->get_status(client_request_queue_id);
              status_id = KDS_STATUS_CACHE;
              cache_in.attach_to(jpip_client);
              jp2_family_in.open(&cache_in);
              window->set_progress_bar(0.0);
              if (metashow != NULL)
                metashow->activate(&jp2_family_in,open_filename);
            }
          else
            { // Open as a local file
              status_id = KDS_STATUS_LAYER_RES;
              jp2_family_in.open(filename_or_url,allow_seeking);
              compositor = new kd_bitmap_compositor(&thread_env);
              if (jpx_in.open(&jp2_family_in,true) >= 0)
                { // We have a JP2/JPX-compatible file.
                  compositor->create(&jpx_in);
                }
              else if (mj2_in.open(&jp2_family_in,true) >= 0)
                {
                  compositor->create(&mj2_in);
                }
              else
                { // Incompatible with JP2/JPX or MJ2. Try opening as raw stream
                  jp2_family_in.close();
                  file_in.open(filename_or_url,allow_seeking);
                  compositor->create(&file_in);
                }
              compositor->set_error_level(error_level);
              open_file_pathname =
                manager->retain_open_file_pathname(filename_or_url,
                                                       window);
              const char *cp = open_filename = open_file_pathname;
              for (; *cp != '\0'; cp=cp++)
                if ((*cp == '/') || (*cp == '\\'))
                  open_filename = cp+1;
              if ((metashow != NULL) && jp2_family_in.exists())
                metashow->activate(&jp2_family_in,open_filename);
            }
        }

      if (jpip_client != NULL)
        { // See if the client is ready yet
          assert((compositor == NULL) && jp2_family_in.exists());
          bool bin0_complete = false;
          if (cache_in.get_databin_length(KDU_META_DATABIN,0,0,
                                          &bin0_complete) > 0)
            { // Open as a JP2/JPX family file
              if (metashow != NULL)
                metashow->update_metadata();
              if (jpx_in.open(&jp2_family_in,false))
                {
                  compositor = new kd_bitmap_compositor(&thread_env);
                  compositor->create(&jpx_in);
                }
              manager->use_jpx_translator_with_jpip_client(jpip_client);
            }
          else if (bin0_complete)
            { // Open as a raw file
              assert(!jpx_in.exists());
              if (metashow != NULL)
                metashow->update_metadata();
              bool hdr_complete;
              cache_in.get_databin_length(KDU_MAIN_HEADER_DATABIN,0,0,
                                          &hdr_complete);
              if (hdr_complete)
                {
                  cache_in.set_read_scope(KDU_MAIN_HEADER_DATABIN,0,0);
                  compositor = new kd_bitmap_compositor(&thread_env);
                  compositor->create(&cache_in);
                }
            }
          if (compositor != NULL)
            {
              compositor->set_error_level(error_level);
              client_roi.init();
              enable_region_posting = true;
            }
          else if (!jpip_client->is_alive(client_request_queue_id))
            {
              kdu_error e; e << "Unable to recover sufficient information "
              "from remote server (or a local cache) to open the image.";
            }
        }
    }
  catch (int)
    {
      close_file();
      processing_call_to_open_file = false;
      return;
    }

  if (compositor != NULL)
    {
      max_display_layers = 1<<16;
      transpose = vflip = hflip = false;
      min_rendering_scale = -1.0F;
      max_rendering_scale = -1.0F;
      rendering_scale = 1.0F;
      single_component_idx = -1;
      single_codestream_idx = 0;
      max_components = -1;
      frame_idx = 0;
      frame_start = frame_end = 0.0;
      num_frames = -1;
      frame = NULL;
      composition_rules = jpx_composition(NULL);

      if (jpx_in.exists())
        {
          max_codestreams = -1;  // Unknown as yet
          max_compositing_layer_idx = -1; // Unknown as yet
          single_layer_idx = -1; // Try starting in composite frame mode
        }
      else if (mj2_in.exists())
        {
          single_layer_idx = 0; // Start in composed single layer (track) mode
        }
      else
        {
          max_codestreams = 1;
          max_compositing_layer_idx = 0;
          num_frames = 0;
          single_layer_idx = 0; // Start in composed single layer mode
        }

      overlays_enabled = true; // Default starting position
      overlay_flashing_state = 1;
      overlay_highlight_state = 0;
      overlay_nominal_intensity = KDWS_OVERLAY_DEFAULT_INTENSITY;
      next_overlay_flash_time = -1.0;
      configure_overlays();

      invalidate_configuration();
      fit_scale_to_window = true;
      image_dims = buffer_dims = view_dims = kdu_dims();
      buffer = NULL;
      if (duplication_src != NULL)
        {
          kdws_renderer *src = duplication_src;
          if (src->last_save_pathname != NULL)
            {
              last_save_pathname = new char[strlen(src->last_save_pathname)+1];
              strcpy(last_save_pathname,src->last_save_pathname);
            }
          save_without_asking = src->save_without_asking;
          allow_seeking = src->allow_seeking;
          error_level = src->error_level;
          max_display_layers = src->max_display_layers;
          transpose = src->transpose;
          vflip = src->vflip;
          hflip = src->hflip;
          min_rendering_scale = src->min_rendering_scale;
          max_rendering_scale = src->max_rendering_scale;
          rendering_scale = src->rendering_scale;
          single_component_idx = src->single_component_idx;
          max_components = src->max_components;
          single_codestream_idx = src->single_component_idx;
          max_codestreams = src->max_codestreams;
          single_layer_idx = src->single_layer_idx;
          max_compositing_layer_idx = src->max_compositing_layer_idx;
          frame_idx = src->frame_idx; // Check that `frame' can be recreated
          num_frames = src->num_frames;
          status_id = src->status_id;
          pixel_scale = src->pixel_scale;
          src->calculate_rel_view_and_focus_params();
          view_centre_x = src->view_centre_x;
          view_centre_y = src->view_centre_y;
          view_centre_known = src->view_centre_known;
          enable_focus_box = src->enable_focus_box;
          rel_focus = src->rel_focus;
          src->view_centre_known = false;
          src->rel_focus.reset();
          highlight_focus = src->highlight_focus;
          overlays_enabled = src->overlays_enabled;
          overlay_flashing_state = src->overlay_flashing_state;
          overlay_nominal_intensity = src->overlay_nominal_intensity;
          overlay_size_threshold = src->overlay_size_threshold;
          memcpy(overlay_params,src->overlay_params,
                 sizeof(kdu_uint32)*KDWS_NUM_OVERLAY_PARAMS);
          catalog_closed = true; // Prevents auto opening of duplicate catalog
          if (view_centre_known)
            {
              place_duplicate_window();          
              fit_scale_to_window = false;
            }
        }
      initialize_regions(true);
    }
  if ((filename_or_url != NULL) && (open_filename != NULL))
    { // Being called for the first time; set the title here
      char *title = new char[strlen(open_filename)+40];
      sprintf(title,"kdu_show %d: %s",window_identifier,open_filename);
      window->SetWindowText(kdws_string(title));
      delete[] title;
    }

  if ((duplication_src == NULL) && jpx_in.exists() &&
      jpx_in.access_meta_manager().peek_touched_nodes(jp2_label_4cc).exists())
    window->create_metadata_catalog();
  processing_call_to_open_file = false;
}

/*****************************************************************************/
/*                   kdws_renderer::invalidate_configuration                 */
/*****************************************************************************/

void
  kdws_renderer::invalidate_configuration()
{
  frame_presenter->reset(); // Avoid possible access to an invalid buffer
  configuration_complete = false;
  max_components = -1; // Changed config may have different num image components
  buffer = NULL;

  if (compositor != NULL)
    compositor->remove_ilayer(kdu_ilayer_ref(),false);
}

/*****************************************************************************/
/*                      kdws_renderer::initialize_regions                    */
/*****************************************************************************/

void
  kdws_renderer::initialize_regions(bool perform_full_window_init)
{
  frame_presenter->reset(); // Avoid possible access to invalid buffer
  if (image_view != NULL)
    image_view->cancel_focus_drag();
  if (perform_full_window_init)
    view_dims = kdu_dims();

  bool can_enlarge_window = fit_scale_to_window;

  // Reset the buffer and view dimensions to zero size.
  buffer = NULL;
  frame_expander.reset();
  
  while (!configuration_complete)
    { // Install configuration
      try {
          if (single_component_idx >= 0)
            { // Check for valid codestream index before opening
              if (jpx_in.exists())
                {
                  int count=max_codestreams;
                  if ((count < 0) && !jpx_in.count_codestreams(count))
                    { // Actual number not yet known, but have at least `count'
                      if (single_codestream_idx >= count)
                        return; // Come back later once more data is in cache
                    }
                  else
                    { // Number of codestreams is now known
                      max_codestreams = count;
                      if (single_codestream_idx >= max_codestreams)
                        single_codestream_idx = max_codestreams-1;
                    }
                }
              else if (mj2_in.exists())
                {
                  kdu_uint32 trk;
                  int frm, fld;
                  if (!mj2_in.find_stream(single_codestream_idx,trk,frm,fld))
                    return; // Come back later once more data is in cache
                  if (trk == 0)
                    {
                      int count;
                      if (mj2_in.count_codestreams(count))
                        max_codestreams = count;
                      single_codestream_idx = count-1;
                    }
                }
              else
                { // Raw codestream
                  single_codestream_idx = 0;
                }
              if (!compositor->add_primitive_ilayer(single_codestream_idx,
                                                single_component_idx,
                                                KDU_WANT_CODESTREAM_COMPONENTS,
                                                kdu_dims(),kdu_dims()))
                { // Cannot open codestream yet; waiting for cache
                  if (!respect_initial_jpip_request)
                    update_client_window_of_interest();
                  return;
                }
            
              kdu_istream_ref str;
              str = compositor->get_next_istream(str);
              kdu_codestream codestream = compositor->access_codestream(str);
              codestream.apply_input_restrictions(0,0,0,0,NULL);
              max_components = codestream.get_num_components();
              if (single_component_idx >= max_components)
                single_component_idx = max_components-1;
            }
          else if (single_layer_idx >= 0)
            { // Check for valid compositing layer index before opening
              if (jpx_in.exists())
                {
                  frame_idx = 0;
                  frame_start = frame_end = 0.0;
                  int count=max_compositing_layer_idx+1;
                  if ((count <= 0) && !jpx_in.count_compositing_layers(count))
                    { // Actual number not yet known, but have at least `count'
                      if (single_layer_idx >= count)
                        return; // Come back later once more data is in cache
                    }
                  else
                    { // Number of compositing layers is now known
                      max_compositing_layer_idx = count-1;
                      if (single_layer_idx > max_compositing_layer_idx)
                        single_layer_idx = max_compositing_layer_idx;
                    }
                }
              else if (mj2_in.exists())
                {
                  int track_type;
                  kdu_uint32 track_idx = (kdu_uint32)(single_layer_idx+1);
                  bool loop_check = false;
                  mj2_video_source *track = NULL;
                  while (track == NULL)
                    {
                      track_type = mj2_in.get_track_type(track_idx);
                      if (track_type == MJ2_TRACK_IS_VIDEO)
                        {
                          track = mj2_in.access_video_track(track_idx);
                          if (track == NULL)
                            return; // Come back later once more data in cache
                          num_frames = track->get_num_frames();
                          if (num_frames == 0)
                            { // Skip over track with no frames
                              track_idx = mj2_in.get_next_track(track_idx);
                              continue;
                            }
                        }
                      else if (track_type == MJ2_TRACK_NON_EXISTENT)
                        { // Go back to the first track
                          if (loop_check)
                            { kdu_error e; e << "Motion JPEG2000 source "
                              "has no video tracks with any frames!"; }
                          loop_check = true;
                          track_idx = mj2_in.get_next_track(0);
                        }
                      else if (track_type == MJ2_TRACK_MAY_EXIST)
                        return; // Come back later once more data is in cache
                      else
                        { // Go to the next track
                          track_idx = mj2_in.get_next_track(track_idx);
                        }
                      single_layer_idx = ((int) track_idx) - 1;
                    }
                  if (frame_idx >= num_frames)
                    frame_idx = num_frames-1;
                  change_frame_idx(frame_idx);
                }
              else
                { // Raw codestream
                  single_layer_idx = 0;
                }
              if (!compositor->add_ilayer(single_layer_idx,kdu_dims(),
                                          kdu_dims(),false,false,false,
                                          frame_idx,0))
                { // Cannot open compositing layer yet; waiting for cache
                  if (respect_initial_jpip_request)
                    { // See if request is compatible with opening a layer
                      if (!check_initial_request_compatibility())         
                        continue; // Mode changed to single component
                    }
                  else
                    update_client_window_of_interest();
                  return;
                }
            }
          else
            { // Attempt to open frame
              if (num_frames == 0)
                { // Downgrade to single layer mode
                  single_layer_idx = 0;
                  frame_idx = 0;
                  frame_start = frame_end = 0.0;
                  frame = NULL;
                  continue;
                }
              assert(jpx_in.exists());
              if (frame == NULL)
                {
                  if (!composition_rules)
                    composition_rules = jpx_in.access_composition();
                  if (!composition_rules)
                    return; // Wait for the cache.
                  frame = composition_rules.get_next_frame(NULL);
                  int save_frame_idx = frame_idx;
                  frame_idx = 0;
                  frame_iteration = 0;
                  if (frame == NULL)
                    { // Downgrade to single layer mode
                      single_layer_idx = 0;
                      frame_idx = 0;
                      frame_start = frame_end = 0.0;
                      num_frames = 0;
                      continue;
                    }
                  else if (save_frame_idx != 0)
                    change_frame_idx(save_frame_idx);
                }

              if (!frame_expander.construct(&jpx_in,frame,
                                            frame_iteration,true))
                {
                  if (respect_initial_jpip_request)
                    { // See if request is compatible with opening a frame
                      if (!check_initial_request_compatibility())         
                        continue; // Mode changed to single layer or component
                    }
                  else
                    update_client_window_of_interest();
                  return; // Can't open all frame members yet; wait for cache
                }
                  
              if (frame_expander.get_num_members() == 0)
                { // Requested frame does not exist
                  if ((num_frames < 0) || (num_frames > frame_idx))
                    num_frames = frame_idx;
                  if (frame_idx == 0)
                    { kdu_error e; e << "Cannot render even the first "
                      "composited frame described in the JPX composition "
                      "box due to unavailability of the required compositing "
                      "layers in the original file.  Viewer will fall back "
                      "to single layer rendering mode."; }
                  change_frame_idx(frame_idx-1);
                  continue; // Loop around to try again
                }

              compositor->set_frame(&frame_expander);
            }
          if (fit_scale_to_window && respect_initial_jpip_request &&
              !check_initial_request_compatibility())
            { // Check we are opening the image in the intended manner
              compositor->remove_ilayer(kdu_ilayer_ref(),false);
              continue; // Open again in different mode
            }
          compositor->set_max_quality_layers(max_display_layers);
          compositor->cull_inactive_ilayers(3); // Really an arbitrary amount of
                                               // culling in this demo app.
          configuration_complete = true;
          min_rendering_scale=-1.0F; // Need to discover these
          max_rendering_scale=-1.0F; // from scratch each time
        }
      catch (int) { // Try downgrading to a more primitive viewing mode
          if ((single_component_idx >= 0) ||
              !(jpx_in.exists() || mj2_in.exists()))
            { // Already in single comp mode; nothing more primitive exists
              close_file();
              return;
            }
          else if (single_layer_idx >= 0)
            { // Downgrade from single layer mode to single component mode
              single_component_idx = 0;
              single_codestream_idx = 0;
              single_layer_idx = -1;
              if (enable_region_posting)
                update_client_window_of_interest();
            }
          else
            { // Downgrade from frame animation mode to single layer mode
              frame = NULL;
              num_frames = 0;
              frame_idx = 0;
              frame_start = frame_end = 0.0;
              single_layer_idx = 0;
            }
        }
    }

  // Get size at current scale, possibly adjusting the scale if this is the
  // first time through
  float new_rendering_scale = rendering_scale;
  kdu_dims new_image_dims = image_dims;
  try {
      bool found_valid_scale=false;
      while (fit_scale_to_window || !found_valid_scale)
        {
          if ((min_rendering_scale > 0.0F) &&
              (new_rendering_scale < min_rendering_scale))
            new_rendering_scale = min_rendering_scale;
          if ((max_rendering_scale > 0.0F) &&
              (new_rendering_scale > max_rendering_scale))
            new_rendering_scale = max_rendering_scale;          
          compositor->set_scale(transpose,vflip,hflip,new_rendering_scale);
          found_valid_scale = 
            compositor->get_total_composition_dims(new_image_dims);
          if (!found_valid_scale)
            { // Increase scale systematically before trying again
              int invalid_scale_code = compositor->check_invalid_scale_code();
              if ((invalid_scale_code & KDU_COMPOSITOR_CANNOT_FLIP) &&
                  (vflip || hflip))
                {
                  kdu_warning w; w << "This image cannot be decompressed "
                    "with the requested geometry (horizontally or vertically "
                    "flipped), since it employs a packet wavelet transform "
                    "in which horizontally (resp. vertically) high-pass "
                    "subbands are further decomposed in the horizontal "
                    "(resp. vertical) direction.  Only transposed decoding "
                    "will be permitted.";
                  vflip = hflip = false;
                }
              else if ((invalid_scale_code & KDU_COMPOSITOR_SCALE_TOO_SMALL) &&
                       !(invalid_scale_code & KDU_COMPOSITOR_SCALE_TOO_LARGE))
                {
                  new_rendering_scale =
                    decrease_scale(new_rendering_scale,false);
                    // The above call will not actually decrease the scale; it
                    // will just find the smallest possible scale that can be
                    // supported.
                  assert((min_rendering_scale > 0.0F) &&
                         (new_rendering_scale == min_rendering_scale));
                  if (new_rendering_scale > 1000.0F)
                    {
                      if (single_component_idx >= 0)
                        { kdu_error e;
                          e << "Cannot display the image.  Seems to "
                          "require some non-trivial scaling.";
                        }
                      else
                        {
                          { kdu_warning w; w << "Cannot display the image.  "
                            "Seems to require some non-trivial scaling.  "
                            "Downgrading to single component mode.";
                          }
                          single_component_idx = 0;
                          invalidate_configuration();
                          if (!compositor->add_primitive_ilayer(0,
                                                single_component_idx,
                                                KDU_WANT_CODESTREAM_COMPONENTS,
                                                kdu_dims(),kdu_dims()))
                            return;
                          configuration_complete = true;
                        }
                    }
                }
              else if ((invalid_scale_code & KDU_COMPOSITOR_SCALE_TOO_LARGE) &&
                       !(invalid_scale_code & KDU_COMPOSITOR_SCALE_TOO_SMALL))
                {
                  new_rendering_scale =
                    increase_scale(new_rendering_scale,false);
                    // The above call will not actually increase the scale; it
                    // will just find the maximum possible scale that can be
                    // supported.
                  assert(new_rendering_scale == max_rendering_scale);
                }
              else if (!(invalid_scale_code & KDU_COMPOSITOR_TRY_SCALE_AGAIN))
                {
                  if (single_component_idx >= 0)
                    { kdu_error e;
                      e << "Cannot display the image.  Unexplicable error "
                        "code encountered in call to "
                        "`kdu_region_compositor::get_total_composition_dims'.";
                    }
                  else
                    {
                      { kdu_warning w; w << "Cannot display the image.  "
                        "Seems to require some incompatible set of geometric "
                        "manipulations for the various composed codestreams.";
                      }
                      single_component_idx = 0;
                      invalidate_configuration();
                      if (!compositor->add_primitive_ilayer(0,
                                                single_component_idx,
                                                KDU_WANT_CODESTREAM_COMPONENTS,
                                                kdu_dims(),kdu_dims()))
                        return;
                      configuration_complete = true;
                    }
                }
            }
          else if (fit_scale_to_window)
            {
              kdu_coords max_tgt_size =
                kdu_coords(1000/pixel_scale,1000/pixel_scale);
              if (respect_initial_jpip_request &&
                  jpip_client->get_window_in_progress(&tmp_roi) &&
                  (tmp_roi.resolution.x > 0) && (tmp_roi.resolution.y > 0))
                { // Roughly fit scale to match the resolution
                  max_tgt_size = tmp_roi.resolution;
                  max_tgt_size.x += (3*max_tgt_size.x)/4;
                  max_tgt_size.y += (3*max_tgt_size.y)/4;
                  if (!tmp_roi.region.is_empty())
                    { // Configure view and focus box based on one-time request
                      rel_focus.reset();
                      rel_focus.region.set(tmp_roi.region,tmp_roi.resolution);
                      if (transpose)
                        rel_focus.region.transpose();
                      if (image_dims.pos.x < 0)
                        rel_focus.region.hflip();
                      if (image_dims.pos.y < 0)
                        rel_focus.region.vflip();
                      if (single_component_idx >= 0)
                        { rel_focus.codestream_idx = single_codestream_idx;
                          rel_focus.codestream_region = rel_focus.region; }
                      else if (jpx_in.exists() && (single_layer_idx >= 0))
                        { rel_focus.layer_idx = single_layer_idx;
                          rel_focus.layer_region = rel_focus.region; }
                      else
                        rel_focus.frame_idx = frame_idx;
                      enable_focus_box = rel_focus.region.is_partial();
                      rel_focus.get_centre_if_valid(view_centre_x,
                                                    view_centre_y);
                      view_centre_known = true;
                    }
                }
              float max_x = new_rendering_scale *
                ((float) max_tgt_size.x) / ((float) new_image_dims.size.x);
              float max_y = new_rendering_scale *
                ((float) max_tgt_size.y) / ((float) new_image_dims.size.y);
              while (((min_rendering_scale < 0.0) ||
                      (new_rendering_scale > min_rendering_scale)) &&
                     ((new_rendering_scale > max_x) ||
                      (new_rendering_scale > max_y)))
                {
                  new_rendering_scale =
                    decrease_scale(new_rendering_scale,false);
                  found_valid_scale = false;
                }
              kdu_dims region_basis;
              if (configuration_complete)
                region_basis = (enable_focus_box)?focus_box:view_dims;            
              new_rendering_scale =
              compositor->find_optimal_scale(region_basis,new_rendering_scale,
                                             new_rendering_scale,
                                             new_rendering_scale);              
              fit_scale_to_window = false;
            }
        }
    }
  catch (int) { // Some error occurred while parsing code-streams
      close_file();
      return;
    }

  // Install the dimensioning parameters we just found, checking to see
  // if the window needs to be resized.
  rendering_scale = new_rendering_scale;
  if ((image_dims == new_image_dims) && (!view_dims.is_empty()) &&
      (!buffer_dims.is_empty()))
    { // No need to resize the window
      compositor->set_buffer_surface(buffer_dims);
      buffer = compositor->get_composition_bitmap(buffer_dims);
      if (adjust_rel_focus())
        {
          if (rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
            view_centre_known = true;
          else if (!view_centre_known)
            update_focus_box();
        }
      if (view_centre_known)
        set_view_size(view_dims.size);
    }
  else
    { // Send a message to the image view window, identifying the
      // maximum allowable image dimensions.  We expect to hear back (possibly
      // after some windows message processing) concerning
      // the actual view dimensions via the `set_view_size' function.
      view_dims = buffer_dims = kdu_dims();
      image_dims = new_image_dims;
      if ((duplication_src == NULL) && adjust_rel_focus() &&
          rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
        view_centre_known = true;
      image_view->set_max_view_size(image_dims.size,pixel_scale,
                                    can_enlarge_window);
      compositor->invalidate_rect(view_dims);
    }
  if (!respect_initial_jpip_request)
    update_client_window_of_interest();
  if (in_playmode)
    { // See if we need to turn off play mode.
      if ((single_component_idx >= 0) ||
          ((single_layer_idx >= 0) && jpx_in.exists()))
        stop_playmode();
    }
  if (overlays_enabled && !in_playmode)
    {
      jpx_metanode node;
      if ((catalog_source != NULL) &&
          (node=catalog_source->get_selected_metanode()).exists())
        flash_imagery_for_metanode(node);
      else if (overlay_highlight_state || overlay_flashing_state)
        {
          overlay_highlight_state = 0;
          overlay_flashing_state = 1;
          configure_overlays();
        }
    }
  display_status();
  view_centre_known = false;
  rel_focus.reset(); // Generate relative focus again when needed
}

/*****************************************************************************/
/*                     kdws_renderer::configure_overlays                     */
/*****************************************************************************/

void kdws_renderer::configure_overlays()
{
  if (compositor == NULL)
    return;
  float intensity = overlay_nominal_intensity;
  jpx_metanode restriction = overlay_restriction;
  double next_dwell = 0.0;
  if (overlay_highlight_state)
    { 
      intensity *= kdws_overlay_highlight_factor[overlay_highlight_state-1];
      next_dwell = kdws_overlay_highlight_dwell[overlay_highlight_state-1];
      restriction = highlight_metanode;
    }
  else if (overlay_flashing_state)
    {
      intensity *= kdws_overlay_flash_factor[overlay_flashing_state-1];
      next_dwell = kdws_overlay_flash_dwell[overlay_flashing_state-1];
    }
  compositor->configure_overlays(overlays_enabled,overlay_size_threshold,
                                 intensity,KDWS_OVERLAY_BORDER,
                                 restriction,0,overlay_params,
                                 KDWS_NUM_OVERLAY_PARAMS);
  if (overlays_enabled && compositor->is_processing_complete() &&
      (overlay_highlight_state || overlay_flashing_state))
    schedule_overlay_flash_wakeup(manager->get_current_time() + next_dwell);
  else
    schedule_overlay_flash_wakeup(-1.0);
}

/*****************************************************************************/
/*                    kdws_renderer::place_duplicate_window                  */
/*****************************************************************************/

void kdws_renderer::place_duplicate_window()
{
  assert(view_centre_known);
  kdu_coords view_size = duplication_src->view_dims.size;
  if (rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
    {
      kdu_coords max_view_size = duplication_src->focus_box.size;
      max_view_size.x += max_view_size.x / 3;
      max_view_size.y += max_view_size.y / 3;
      if (view_size.x > max_view_size.x)
        view_size.x = max_view_size.x;
      if (view_size.y > max_view_size.y)
        view_size.y = max_view_size.y;
    }

  // Now we need to estimate the frame margins so that we can size and
  // place the window appropriately.  Easiest way to do this is to use
  // the margins of the window being duplicated, being careful not
  // to count the contribution of any catalog.  Unfortunately, this method
  // cannot account for the possible impact of extra menu rows which might
  // be added if the window is too narrow.  To fix the problem, we'll adjust
  // the size in two phases: horizontal first and then vertical.
  CRect window_frame; duplication_src->window->GetWindowRect(&window_frame);
  CRect window_client; duplication_src->window->GetClientRect(&window_client);
  CRect view_frame; duplication_src->image_view->GetWindowRect(&view_frame);
  CRect view_client; duplication_src->image_view->GetClientRect(&view_client);
  kdu_coords margins;
  margins.x = window_frame.Width() - window_client.Width();
  margins.y = window_frame.Height() - view_frame.Height();
  margins.x += view_frame.Width() - view_client.Width();
  margins.y += view_frame.Height() - view_client.Height();

  kdu_coords target_frame_size;
  target_frame_size.x = view_size.x * pixel_scale + margins.x;
  target_frame_size.y = window_frame.Height();
  window->SetWindowPos(NULL,0,0,target_frame_size.x,target_frame_size.y,
                       SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOMOVE);
  window->GetWindowRect(&window_frame);
  window->GetClientRect(&window_client);
  image_view->GetWindowRect(&view_frame);
  image_view->GetClientRect(&view_client);
  margins.x = window_frame.Width() - window_client.Width();
  margins.y = window_frame.Height() - view_frame.Height();
  margins.x += view_frame.Width() - view_client.Width();
  margins.y += view_frame.Height() - view_client.Height();
  target_frame_size.x = view_size.x * pixel_scale + margins.x;
  target_frame_size.y = view_size.y * pixel_scale + margins.y;
  manager->place_window(window,target_frame_size);
}

/*****************************************************************************/
/*                        kdws_renderer::set_view_size                       */
/*****************************************************************************/

void
  kdws_renderer::set_view_size(kdu_coords size)
{
  if ((compositor == NULL) || !configuration_complete)
    return;
  if (image_view != NULL)
    image_view->cancel_focus_drag();

  // Set view region to the largest subset of the image region consistent with
  // the size of the new viewing region.
  kdu_dims new_view_dims = view_dims;
  new_view_dims.size = size;
  if (view_centre_known)
    {
      new_view_dims.pos.x = image_dims.pos.x - (size.x / 2) +
        (int) floor(0.5 + image_dims.size.x*view_centre_x);
      new_view_dims.pos.y = image_dims.pos.y - (size.y / 2) +
        (int) floor(0.5 + image_dims.size.y*view_centre_y);
      view_centre_known = false;
    }
  if (new_view_dims.pos.x < image_dims.pos.x)
    new_view_dims.pos.x = image_dims.pos.x;
  if (new_view_dims.pos.y < image_dims.pos.y)
    new_view_dims.pos.y = image_dims.pos.y;
  kdu_coords view_lim = new_view_dims.pos + new_view_dims.size;
  kdu_coords image_lim = image_dims.pos + image_dims.size;
  if (view_lim.x > image_lim.x)
    new_view_dims.pos.x -= view_lim.x-image_lim.x;
  if (view_lim.y > image_lim.y)
    new_view_dims.pos.y -= view_lim.y-image_lim.y;
  new_view_dims &= image_dims;
  bool need_redraw = new_view_dims.pos != view_dims.pos;

  lock_view(); // We are about to change parameters which affect painting
  view_dims = new_view_dims;

  // Get preferred minimum dimensions for the new buffer region.
  size = view_dims.size;
  if (!in_playmode)
    { // A small boundary minimizes impact of scrolling
      size.x += (size.x>>4)+100;
      size.y += (size.y>>4)+100;
    }

  // Make sure buffered region is no larger than image
  if (size.x > image_dims.size.x)
    size.x = image_dims.size.x;
  if (size.y > image_dims.size.y)
    size.y = image_dims.size.y;
  kdu_dims new_buffer_dims;
  new_buffer_dims.size = size;
  new_buffer_dims.pos = buffer_dims.pos;

  // Make sure the buffer region is contained within the image
  kdu_coords buffer_lim = new_buffer_dims.pos + new_buffer_dims.size;
  if (buffer_lim.x > image_lim.x)
    new_buffer_dims.pos.x -= buffer_lim.x-image_lim.x;
  if (buffer_lim.y > image_lim.y)
    new_buffer_dims.pos.y -= buffer_lim.y-image_lim.y;
  if (new_buffer_dims.pos.x < image_dims.pos.x)
    new_buffer_dims.pos.x = image_dims.pos.x;
  if (new_buffer_dims.pos.y < image_dims.pos.y)
    new_buffer_dims.pos.y = image_dims.pos.y;
  assert(new_buffer_dims == (new_buffer_dims & image_dims));

  // See if the buffered region includes any new locations at all.
  if ((new_buffer_dims.pos != buffer_dims.pos) ||
      (new_buffer_dims != (new_buffer_dims & buffer_dims)) ||
      (view_dims != (view_dims & new_buffer_dims)))
    { // We will need to reshuffle or resize the buffer anyway, so might
      // as well get the best location for the buffer.
      new_buffer_dims.pos.x = view_dims.pos.x;
      new_buffer_dims.pos.y = view_dims.pos.y;
      new_buffer_dims.pos.x -= (new_buffer_dims.size.x-view_dims.size.x)/2;
      new_buffer_dims.pos.y -= (new_buffer_dims.size.y-view_dims.size.y)/2;
      if (new_buffer_dims.pos.x < image_dims.pos.x)
        new_buffer_dims.pos.x = image_dims.pos.x;
      if (new_buffer_dims.pos.y < image_dims.pos.y)
        new_buffer_dims.pos.y = image_dims.pos.y;
      buffer_lim = new_buffer_dims.pos + new_buffer_dims.size;
      if (buffer_lim.x > image_lim.x)
        new_buffer_dims.pos.x -= buffer_lim.x - image_lim.x;
      if (buffer_lim.y > image_lim.y)
        new_buffer_dims.pos.y -= buffer_lim.y - image_lim.y;
      assert(view_dims == (view_dims & new_buffer_dims));
      assert(new_buffer_dims == (image_dims & new_buffer_dims));
      assert(view_dims == (new_buffer_dims & view_dims));
    }

  // Set surface and get buffer
  compositor->set_buffer_surface(new_buffer_dims);
  buffer = compositor->get_composition_bitmap(buffer_dims);

  unlock_view(); // Finished the sensitive section

  // Now reflect changes in the view size to the appearance of scroll bars.

  SCROLLINFO sc_info; sc_info.cbSize = sizeof(sc_info);
  sc_info.fMask = SIF_DISABLENOSCROLL | SIF_ALL;
  sc_info.nMin = 0;
  sc_info.nMax = image_dims.size.x-1;
  sc_info.nPage = view_dims.size.x;
  sc_info.nPos = view_dims.pos.x - image_dims.pos.x;
  image_view->SetScrollInfo(SB_HORZ,&sc_info);
  sc_info.fMask = SIF_DISABLENOSCROLL | SIF_ALL;
  sc_info.nMin = 0;
  sc_info.nMax = image_dims.size.y-1;
  sc_info.nPage = view_dims.size.y;
  sc_info.nPos = view_dims.pos.y - image_dims.pos.y;
  image_view->SetScrollInfo(SB_VERT,&sc_info);
  kdu_coords step = view_dims.size;
  step.x = (step.x >> 4) + 1;
  step.y = (step.y >> 4) + 1;
  kdu_coords page = view_dims.size - step;
  image_view->set_scroll_metrics(step,page,image_dims.size-view_dims.size);

  // Update related display properties
  if (need_redraw)
    image_view->Invalidate();
  update_focus_box();
}

/*****************************************************************************/
/*                       kdws_renderer::set_hscroll_pos                      */
/*****************************************************************************/

void
  kdws_renderer::set_hscroll_pos(int pos, bool relative_to_last)
{
  if ((compositor == NULL) || (buffer == NULL))
    return;
  if (image_view != NULL)
    image_view->cancel_focus_drag();

  view_centre_known = false;
  if (relative_to_last)
    pos += view_dims.pos.x;
  else
    pos += image_dims.pos.x;
  if (pos < image_dims.pos.x)
    pos = image_dims.pos.x;
  if ((pos+view_dims.size.x) > (image_dims.pos.x+image_dims.size.x))
    pos = image_dims.pos.x+image_dims.size.x-view_dims.size.x;
  lock_view();
  if (pos != view_dims.pos.x)
    {
      RECT update;
      image_view->ScrollWindowEx((view_dims.pos.x-pos)*pixel_scale,0,
                                 NULL,NULL,NULL,&update,0);
      view_dims.pos.x = pos;
      if (view_dims != (view_dims & buffer_dims))
        { // The view is no longer fully contained in the buffered region.
          buffer_dims.pos.x =
            view_dims.pos.x - (buffer_dims.size.x-view_dims.size.x)/2;
          if (buffer_dims.pos.x < image_dims.pos.x)
            buffer_dims.pos.x = image_dims.pos.x;
          int image_lim = image_dims.pos.x+image_dims.size.x;
          int buf_lim = buffer_dims.pos.x + buffer_dims.size.x;
          if (buf_lim > image_lim)
            buffer_dims.pos.x -= (buf_lim-image_lim);
          compositor->set_buffer_surface(buffer_dims);
          buffer = compositor->get_composition_bitmap(buffer_dims);
        }
      // Repaint the erased area -- note that although the scroll function
      // is supposed to be able to invalidate the relevant regions of the
      // window, rendering this code unnecessary, that functionality appears
      // to be able to fail badly under certain extremely fast scrolling
      // sequences. Best to do the job ourselves.
      update.left = update.left / pixel_scale;
      update.right = (update.right+pixel_scale-1) / pixel_scale;
      update.top = update.top / pixel_scale;
      update.bottom = (update.bottom+pixel_scale-1) / pixel_scale;
      kdu_dims update_dims;
      update_dims.pos.x = update.left;
      update_dims.pos.y = update.top;
      update_dims.size.x = update.right-update.left;
      update_dims.size.y = update.bottom-update.top;

      update_dims.pos += view_dims.pos; // Get in the global coordinate system
      paint_region(NULL,update_dims);
    }
  unlock_view();
  image_view->SetScrollPos(SB_HORZ,pos-image_dims.pos.x);
  update_focus_box();
}

/*****************************************************************************/
/*                       kdws_renderer::set_vscroll_pos                      */
/*****************************************************************************/

void
  kdws_renderer::set_vscroll_pos(int pos, bool relative_to_last)
{
  if ((compositor == NULL) || (buffer == NULL))
    return;
  if (image_view != NULL)
    image_view->cancel_focus_drag();

  view_centre_known = false;
  if (relative_to_last)
    pos += view_dims.pos.y;
  else
    pos += image_dims.pos.y;
  if (pos < image_dims.pos.y)
    pos = image_dims.pos.y;
  if ((pos+view_dims.size.y) > (image_dims.pos.y+image_dims.size.y))
    pos = image_dims.pos.y+image_dims.size.y-view_dims.size.y;
  lock_view();
  if (pos != view_dims.pos.y)
    {
      RECT update;
      image_view->ScrollWindowEx(0,(view_dims.pos.y-pos)*pixel_scale,
                                NULL,NULL,NULL,&update,0);
      view_dims.pos.y = pos;
      if (view_dims != (view_dims & buffer_dims))
        { // The view is no longer fully contained in the buffered region.
          buffer_dims.pos.y =
            view_dims.pos.y - (buffer_dims.size.y-view_dims.size.y)/2;
          if (buffer_dims.pos.y < image_dims.pos.y)
            buffer_dims.pos.y = image_dims.pos.y;
          int image_lim = image_dims.pos.y+image_dims.size.y;
          int buf_lim = buffer_dims.pos.y + buffer_dims.size.y;
          if (buf_lim > image_lim)
            buffer_dims.pos.y -= (buf_lim-image_lim);
          compositor->set_buffer_surface(buffer_dims);
          buffer = compositor->get_composition_bitmap(buffer_dims);
        }
      // Repaint the erased area -- note that although the scroll function
      // is supposed to be able to invalidate the relevant regions of the
      // window, rendering this code unnecessary, that functionality appears
      // to be able to fail badly under certain extremely fast scrolling
      // sequences.  Best to do the job ourselves.
      update.left = update.left / pixel_scale;
      update.right = (update.right+pixel_scale-1) / pixel_scale;
      update.top = update.top / pixel_scale;
      update.bottom = (update.bottom+pixel_scale-1) / pixel_scale;
      kdu_dims update_dims;
      update_dims.pos.x = update.left;
      update_dims.pos.y = update.top;
      update_dims.size.x = update.right-update.left;
      update_dims.size.y = update.bottom-update.top;
      update_dims.pos += view_dims.pos; // Get in the global coordinate system
      paint_region(NULL,update_dims);
    }
  unlock_view();
  image_view->SetScrollPos(SB_VERT,pos-image_dims.pos.y);
  update_focus_box();
}

/*****************************************************************************/
/*                       kdws_renderer::set_scroll_pos                       */
/*****************************************************************************/

void
  kdws_renderer::set_scroll_pos(int pos_x, int pos_y, bool relative_to_last)
{
  if ((compositor == NULL) || (buffer == NULL))
    return;
  view_centre_known = false;
  if (relative_to_last)
    {
      pos_y += view_dims.pos.y;
      pos_x += view_dims.pos.x;
    }
  else
    {
      pos_y += image_dims.pos.y;
      pos_x += image_dims.pos.x;
    }
  if (pos_y < image_dims.pos.y)
    pos_y = image_dims.pos.y;
  if ((pos_y+view_dims.size.y) > (image_dims.pos.y+image_dims.size.y))
    pos_y = image_dims.pos.y+image_dims.size.y-view_dims.size.y;
  if (pos_x < image_dims.pos.x)
    pos_x = image_dims.pos.x;
  if ((pos_x+view_dims.size.x) > (image_dims.pos.x+image_dims.size.x))
    pos_x = image_dims.pos.x+image_dims.size.x-view_dims.size.x;

  lock_view();
  if ((pos_y != view_dims.pos.y) || (pos_x != view_dims.pos.x))
    {
      RECT update;
      image_view->ScrollWindowEx((view_dims.pos.x-pos_x)*pixel_scale,
                                 (view_dims.pos.y-pos_y)*pixel_scale,
                                 NULL,NULL,NULL,&update,0);
      view_dims.pos.x = pos_x;
      view_dims.pos.y = pos_y;
      if (view_dims != (view_dims & buffer_dims))
        { // The view is no longer fully contained in the buffered region.
          buffer_dims.pos.x =
            view_dims.pos.x - (buffer_dims.size.x-view_dims.size.x)/2;
          buffer_dims.pos.y =
            view_dims.pos.y - (buffer_dims.size.y-view_dims.size.y)/2;
          if (buffer_dims.pos.x < image_dims.pos.x)
            buffer_dims.pos.x = image_dims.pos.x;
          if (buffer_dims.pos.y < image_dims.pos.y)
            buffer_dims.pos.y = image_dims.pos.y;
          kdu_coords image_lim = image_dims.pos + image_dims.size;
          kdu_coords buf_lim = buffer_dims.pos + buffer_dims.size;
          if (buf_lim.y > image_lim.y)
            buffer_dims.pos.y -= (buf_lim.y - image_lim.y);
          if (buf_lim.x > image_lim.x)
            buffer_dims.pos.x -= (buf_lim.x - image_lim.x);
          compositor->set_buffer_surface(buffer_dims);
          buffer = compositor->get_composition_bitmap(buffer_dims);
        }
      // Repaint the erased area -- note that although the scroll function
      // is supposed to be able to invalidate the relevant regions of the
      // window, rendering this code unnecessary, that functionality appears
      // to be able to fail badly under certain extremely fast scrolling
      // sequences.  Best to do the job ourselves.
      update.left = update.left / pixel_scale;
      update.right = (update.right+pixel_scale-1) / pixel_scale;
      update.top = update.top / pixel_scale;
      update.bottom = (update.bottom+pixel_scale-1) / pixel_scale;
      kdu_dims update_dims;
      update_dims.pos.x = update.left;
      update_dims.pos.y = update.top;
      update_dims.size.x = update.right-update.left;
      update_dims.size.y = update.bottom-update.top;
      update_dims.pos += view_dims.pos; // Get in the global coordinate system
      paint_region(NULL,update_dims);
    }
  unlock_view();
  image_view->SetScrollPos(SB_HORZ,pos_x-image_dims.pos.x);
  image_view->SetScrollPos(SB_VERT,pos_y-image_dims.pos.y);
  update_focus_box();  
}

/*****************************************************************************/
/*                  kdws_renderer::scroll_to_view_anchors                    */
/*****************************************************************************/

void kdws_renderer::scroll_to_view_anchors()
{
  if (!view_centre_known)
    return;
  kdu_coords new_view_origin;
  new_view_origin.x = (int) floor(image_dims.size.x*view_centre_x -
                                  0.5*view_dims.size.x);
  new_view_origin.y = (int) floor(image_dims.size.y*view_centre_y -
                                  0.5*view_dims.size.y);
  if (new_view_origin.x < 0)
    new_view_origin.x = 0;
  if (new_view_origin.y < 0)
    new_view_origin.y = 0;
  view_centre_known = false;
  set_scroll_pos(new_view_origin.x,new_view_origin.y);
}

/*****************************************************************************/
/*                      kdws_renderer::resize_stripmap                       */
/*****************************************************************************/

void
  kdws_renderer::resize_stripmap(int min_width)
{
  if (min_width <= strip_extent.x)
    return;
  strip_extent.x = min_width + 100;
  strip_extent.y = 100;
  memset(&stripmap_info,0,sizeof(stripmap_info));
  stripmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  stripmap_info.bmiHeader.biWidth = strip_extent.x;
  stripmap_info.bmiHeader.biHeight = -strip_extent.y;
  stripmap_info.bmiHeader.biPlanes = 1;
  stripmap_info.bmiHeader.biBitCount = 32;
  stripmap_info.bmiHeader.biCompression = BI_RGB;
  if (stripmap != NULL)
    DeleteObject(stripmap);
  stripmap =
    CreateDIBSection(NULL,&stripmap_info,DIB_RGB_COLORS,
                     (void **) &stripmap_buffer,NULL,0);
}

/*****************************************************************************/
/*                        kdws_renderer::paint_region                        */
/*****************************************************************************/

void
  kdws_renderer::paint_region(CDC *supplied_dc,
                              kdu_dims region, bool is_absolute)
{
  CDC *dc = supplied_dc;
  if (dc == NULL)
    dc = image_view->GetDC();

  int save_dc_state = 0;
  if (supplied_dc != NULL)
    save_dc_state = supplied_dc->SaveDC();

  if (!is_absolute)
    region.pos += view_dims.pos;
  kdu_dims region_on_buffer = region;
  region_on_buffer &= buffer_dims; // Intersect with buffer region
  region_on_buffer.pos -= buffer_dims.pos; // Make relative to buffer region
  if (region_on_buffer.size != region.size)
    { /* Need to erase the region first and then modify it to reflect the
         region we are about to actually paint. */
      dc->BitBlt(region.pos.x*pixel_scale,region.pos.y*pixel_scale,
                 region.size.x*pixel_scale,region.size.y*pixel_scale,
                 NULL,0,0,WHITENESS);
      region = region_on_buffer;
      region.pos += buffer_dims.pos; // Convert to absolute region
    }
  region.pos -= view_dims.pos; // Make relative to curent view
  if ((!region_on_buffer) || (buffer == NULL))
    {
      if (supplied_dc == NULL)
        image_view->ReleaseDC(dc);
      else if (save_dc_state != 0)
        supplied_dc->RestoreDC(save_dc_state);
      return;
    }
  assert(region.size == region_on_buffer.size);

  kdu_coords screen_pos;

  int bitmap_row_gap;
  kdu_uint32 *bitmap_buffer;
  HBITMAP bitmap = buffer->access_bitmap(bitmap_buffer,bitmap_row_gap);
  if (enable_focus_box && highlight_focus)
    { // Paint through intermediate strip buffer
      assert(bitmap != NULL);
      kdu_dims focus_region = focus_box;
      focus_region.pos -= view_dims.pos; // Make relative to view port
      focus_region &= region; // Get portion of focus box inside paint region
      if (!focus_region)
        focus_region.pos = region.pos;
      int focus_above = focus_region.pos.y - region.pos.y;
      int focus_height = focus_region.size.y;
      int focus_left = focus_region.pos.x - region.pos.x;
      int focus_width = focus_region.size.x;
      int focus_right = region.size.x - focus_left - focus_width;
      assert((focus_above >= 0) && (focus_height >= 0) &&
             (focus_left >= 0) && (focus_width >= 0) && (focus_right >= 0));
      if (strip_extent.x < region.size.x)
        resize_stripmap(region.size.x);
      kdu_dims save_region = region;
      while (region.size.y > 0)
        {
          int i, j, xfer_height = region.size.y;
          if (xfer_height > strip_extent.y)
            xfer_height = strip_extent.y;
          kdu_uint32 *sp, *spp = bitmap_buffer +
            (region_on_buffer.pos.x + region_on_buffer.pos.y*bitmap_row_gap);
          kdu_uint32 *dp, *dpp = stripmap_buffer;
          for (j=xfer_height; j > 0; j--,
               spp+=bitmap_row_gap, dpp+=strip_extent.x,
               focus_above--)
            {
              sp = spp; dp = dpp;
              kdu_uint32 val;
              kdu_byte *lut;
              if ((focus_above <= 0) && (focus_height > 0))
                {
                  focus_height--;
                  for (lut=background_lut, i=focus_left; i > 0; i--)
                    {
                      val = *(sp++);
                      *(dp++) = 0xFF000000 +
                        (((kdu_uint32) lut[(val>>16)&0xFF])<<16) +
                        (((kdu_uint32) lut[(val>> 8)&0xFF])<< 8) +
                        (((kdu_uint32) lut[(val    )&0xFF])    );
                    }
                  for (lut=foreground_lut, i=focus_width; i > 0; i--)
                    {
                      val = *(sp++);
                      *(dp++) = 0xFF000000 +
                        (((kdu_uint32) lut[(val>>16)&0xFF])<<16) +
                        (((kdu_uint32) lut[(val>> 8)&0xFF])<< 8) +
                        (((kdu_uint32) lut[(val    )&0xFF])    );
                    }
                  i = focus_right;
                }
              else
                i = region.size.x;
              for (lut=background_lut; i > 0; i--)
                {
                  val = *(sp++);
                  *(dp++) = 0xFF000000 +
                    (((kdu_uint32) lut[(val>>16)&0xFF])<<16) +
                    (((kdu_uint32) lut[(val>> 8)&0xFF])<< 8) +
                    (((kdu_uint32) lut[(val    )&0xFF])    );
                }
            }
          HBITMAP old_bitmap = (HBITMAP)
            SelectObject(compatible_dc.m_hDC,stripmap);
          if (pixel_scale == 1)
            dc->BitBlt(region.pos.x,region.pos.y,region.size.x,xfer_height,
                       &compatible_dc,0,0,SRCCOPY);
          else
            dc->StretchBlt(region.pos.x*pixel_scale,region.pos.y*pixel_scale,
                           region.size.x*pixel_scale,xfer_height*pixel_scale,
                           &compatible_dc,0,0,region.size.x,xfer_height,
                           SRCCOPY);
          SelectObject(compatible_dc.m_hDC,old_bitmap);
          region.pos.y += xfer_height;
          region_on_buffer.pos.y += xfer_height;
          region.size.y -= xfer_height;
        }
      region = save_region;
    }
  else
    { // Paint bitmap directly
      HBITMAP old_bitmap = (HBITMAP) SelectObject(compatible_dc.m_hDC,bitmap);
      if (pixel_scale == 1)
        dc->BitBlt(region.pos.x,region.pos.y,region.size.x,region.size.y,
                   &compatible_dc,region_on_buffer.pos.x,region_on_buffer.pos.y,
                   SRCCOPY);
      else
        dc->StretchBlt(region.pos.x*pixel_scale,region.pos.y*pixel_scale,
                       region.size.x*pixel_scale,region.size.y*pixel_scale,
                       &compatible_dc,
                       region_on_buffer.pos.x,region_on_buffer.pos.y,
                       region.size.x,region.size.y,SRCCOPY);
      SelectObject(compatible_dc.m_hDC,old_bitmap);
    }
  if (enable_focus_box)
    outline_focus_box(dc,region);

  if (supplied_dc == NULL)
    image_view->ReleaseDC(dc);
  else
    supplied_dc->RestoreDC(save_dc_state);
}

/*****************************************************************************/
/*                      kdws_renderer::display_status                        */
/*****************************************************************************/

void kdws_renderer::display_status()
{
  int p;
  char char_buf[300];
  char *panel_text[3] = {char_buf,char_buf+100,char_buf+200};
  for (p=0; p < 3; p++)
    *(panel_text[p]) = '\0'; // Reset all panels to empty strings.
  
  // Find resolution information
  kdu_dims basis_region = (enable_focus_box)?focus_box:view_dims;
  float component_scale_x=-1.0F, component_scale_y=-1.0F;
  if ((compositor != NULL) && configuration_complete)
    {
      kdu_ilayer_ref layer =
        compositor->get_next_visible_ilayer(kdu_ilayer_ref(),basis_region);
      if (layer.exists() &&
          !compositor->get_next_visible_ilayer(layer,basis_region))
        {
          int cs_idx;
          kdu_istream_ref stream = compositor->get_ilayer_stream(layer,0);
          compositor->get_istream_info(stream,cs_idx,NULL,NULL,4,NULL,
                                       &component_scale_x,
                                       &component_scale_y);
          component_scale_x *= rendering_scale;
          component_scale_y *= rendering_scale;
          if (transpose)
            {
              float tmp=component_scale_x;
              component_scale_x=component_scale_y;
              component_scale_y=tmp;
            }
        }
    }

  if ((status_id == KDS_STATUS_CACHE) && (jpip_client == NULL))
    status_id = KDS_STATUS_LAYER_RES;

  // Fill in status pane 0 with resolution information
  if (status_id == KDS_STATUS_CACHE)
    {
      assert(jpip_client_status != NULL);
      if (strlen(jpip_client_status) < 80)
        strcpy(panel_text[0],jpip_client_status);
      else
        {
          strncpy(panel_text[0],jpip_client_status,80);
          strcpy(panel_text[0]+80,"...");
        }
    }
  else if (compositor != NULL)
    {
      float res_percent = 100.0F*rendering_scale;
      float x_percent = 100.0F*component_scale_x;
      float y_percent = 100.0F*component_scale_y;
      if ((x_percent > (0.99F*res_percent)) &&
          (x_percent < (1.01F*res_percent)) &&
          (y_percent > (0.99F*res_percent)) &&
          (y_percent < (1.01F*res_percent)))
        x_percent = y_percent = -1.0F;
      if (x_percent < 0.0F)
        sprintf(panel_text[0],"Res=%1.1f%%",res_percent);
      else if ((x_percent > (0.99F*y_percent)) &&
               (x_percent < (1.01F*y_percent)))
        sprintf(panel_text[0],"Res=%1.1f%% (%1.1f%%)",res_percent,x_percent);
      else
        sprintf(panel_text[0],"Res=%1.1f%% (x:%1.1f%%,y:%1.1f%%)",
                res_percent,x_percent,y_percent);
    }
  
  // Fill in status pane 1 with image/component/frame information
  if (compositor != NULL)
    {
      char *sp = panel_text[1];
      if (single_component_idx >= 0)
        {
          sprintf(sp,"Comp=%d/",single_component_idx+1);
          sp += strlen(sp);
          if (max_components <= 0)
            *(sp++) = '?';
          else
            { sprintf(sp,"%d",max_components); sp += strlen(sp); }
          sprintf(sp,", Stream=%d/",single_codestream_idx+1);
          sp += strlen(sp);
          if (max_codestreams <= 0)
            strcpy(sp,"?");
          else
            sprintf(sp,"%d",max_codestreams);
        }
      else if ((single_layer_idx >= 0) && !mj2_in)
        { // Report compositing layer index
          sprintf(sp,"C.Layer=%d/",single_layer_idx+1);
          sp += strlen(sp);
          if (max_compositing_layer_idx < 0)
            strcpy(sp,"?");
          else
            sprintf(sp,"%d",max_compositing_layer_idx+1);
        }
      else if ((single_layer_idx >= 0) && mj2_in.exists())
        { // Report track and frame indices, rather than layer index
          sprintf(sp,"Trk:Frm=%d:%d",single_layer_idx+1,
                  (playmode_frame_idx<0)?(frame_idx+1):(playmode_frame_idx+1));
        }
      else
        {
          sprintf(sp,"Frame=%d/",
                  (playmode_frame_idx<0)?(frame_idx+1):(playmode_frame_idx+1));
          sp += strlen(sp);
          if (num_frames <= 0)
            strcpy(sp,"?");
          else
            sprintf(sp,"%d",num_frames);
        }   
    }
  
  // Fill in status pane 2
  if ((compositor != NULL) && (status_id == KDS_STATUS_LAYER_RES))
    {
      if (!compositor->is_codestream_processing_complete())
        strcpy(panel_text[2],"WORKING");
      else
        {
          int max_layers = compositor->get_max_available_quality_layers();
          if (max_display_layers >= max_layers)
            sprintf(panel_text[2],"Q.Layers=all ");
          else
            sprintf(panel_text[2],"Q.Layers=%d/%d ",
                    max_display_layers,max_layers);
        }
    }
  else if ((compositor != NULL) && (status_id == KDS_STATUS_DIMS))
    {
      if (!compositor->is_codestream_processing_complete())
        strcpy(panel_text[2],"WORKING");
      else
        sprintf(panel_text[2],"HxW=%dx%d ",
                image_dims.size.y,image_dims.size.x);
    }
  else if ((compositor != NULL) && (status_id == KDS_STATUS_MEM))
    {
      if (!compositor->is_codestream_processing_complete())
        strcpy(panel_text[2],"WORKING");
      else
        {
          kdu_long bytes = 0;
          kdu_istream_ref str;
          while ((str=compositor->get_next_istream(str,false,true)).exists())
            {
              kdu_codestream ifc = compositor->access_codestream(str);
              bytes += ifc.get_compressed_data_memory()
                     + ifc.get_compressed_state_memory();
            }
          if ((jpip_client != NULL) && jpip_client->is_active())
            bytes += jpip_client->get_peak_cache_memory();
          sprintf(panel_text[2],"%5g MB compressed data memory",1.0E-6*bytes);
        }
    }
  else if ((compositor != NULL) && (status_id == KDS_STATUS_PLAYSTATS))
    {
      double avg_frame_rate = playmode_stats.get_avg_frame_rate();
      if (avg_frame_rate > 0.0)
        sprintf(panel_text[2],"%4.2g fps",avg_frame_rate);
    }
  else if ((jpip_client != NULL) && (status_id == KDS_STATUS_CACHE))
    {
      double active_secs = 0.0;
      kdu_long global_bytes = jpip_client->get_received_bytes(-1);
      kdu_long queue_bytes =
        jpip_client->get_received_bytes(client_request_queue_id,&active_secs);
      double interval_seconds = active_secs - jpip_interval_start_time;
      if (queue_bytes <= jpip_interval_start_bytes)
        { // Use last known statistics
          queue_bytes = jpip_interval_start_bytes;
        }
      else if ((interval_seconds > 0.5) && (active_secs > 2.0))
        { // Update statistics for this queue so long as some active reception
          // time has ellapsed since last update and we have been active for
          // a total of at least 2 seconds.
          kdu_long interval_bytes = queue_bytes - jpip_interval_start_bytes;
          jpip_interval_start_bytes = queue_bytes;
          jpip_interval_start_time = active_secs;
          double inst_rate = ((double) interval_bytes) / interval_seconds;
          double rho = 1.0 - pow(0.85,interval_seconds);
          if (jpip_client_data_rate <= 0.0)
            rho = 1.0;
          jpip_client_data_rate += rho * (inst_rate - jpip_client_data_rate);
        }
      if (jpip_client_data_rate > 0.0)
        sprintf(panel_text[2],"%8.1f kB; %7.1f kB/s",
                0.001 * ((double) queue_bytes),0.001 * jpip_client_data_rate);
      else
        sprintf(panel_text[2],"%8.1f kB",(0.001 * (double) queue_bytes));
      if (global_bytes > queue_bytes)
        sprintf(panel_text[2]+strlen(panel_text[2]),
                " (%8.1f kB)",(0.001 * (double) global_bytes));
    }
  
  if ((jpip_client != NULL) && (compositor != NULL) &&
      compositor->is_processing_complete())
    {
      kdu_long samples=0, packet_samples=0, max_packet_samples=0, s, p, m;
      kdu_ilayer_ref lyr;
      while ((compositor != NULL) &&
             ((lyr=compositor->get_next_visible_ilayer(lyr,
                                                  basis_region)).exists()))
        { 
          kdu_istream_ref str;
          int w=0;
          while ((str=compositor->get_ilayer_stream(lyr,w++)).exists())
            {
              compositor->get_codestream_packets(str,basis_region,s,p,m);
            samples += s;  packet_samples += p;  max_packet_samples += m;
            }
        }
      double progress = 0.0;
      if (packet_samples > 0)
        progress = 2.0 + // Make sure the user can see some progress
          98.0 * (((double) packet_samples) / ((double) max_packet_samples));
      window->set_progress_bar(progress);
    }
  window->set_status_strings((const char **)panel_text);
}

/*****************************************************************************/
/*                         kdws_renderer::on_idle                            */
/*****************************************************************************/

bool kdws_renderer::on_idle()
{
  if (processing_suspended)
    return false;

  if (!(configuration_complete && (buffer != NULL)))
    return false; // Nothing to process
  
  bool processed_something = false;
  kdu_dims new_region;
  try {
    int defer_flag = KDU_COMPOSIT_DEFER_REGION;
    if ((!in_playmode) && (defer_process_regions_deadline > 0.0))
      {
        double cur_time = manager->get_current_time();
        if (cur_time > defer_process_regions_deadline)
          { defer_flag=0; defer_process_regions_deadline = cur_time + 0.05; }
      }
    processed_something = compositor->process(128000,new_region,defer_flag);
    if ((!in_playmode) && (defer_process_regions_deadline <= 0.0) &&
        processed_something)
      defer_process_regions_deadline = manager->get_current_time() + 0.05;

    if ((!processed_something) &&
        !compositor->get_total_composition_dims(image_dims))
      { // Must have invalid scale
        frame_presenter->reset(); // Prevents access to invalid buffer
        compositor->flush_composition_queue();
        buffer = NULL;
        initialize_regions(false); // This call will find a satisfactory scale
        return false;
      }
  }
  catch (int) { // Problem occurred.  Only safe thing is to close the file.
    close_file();
    return false;
  }

  if (processed_something)
    next_render_refresh_time = -1.0; // Avoid refreshing the rendering too soon
  
  double cur_time = -1.0; // Set only if we need it
  bool processing_newly_complete =
    processed_something && compositor->is_processing_complete();
  if (processing_newly_complete || in_playmode)
    {
      cur_time = manager->get_current_time();
      if (processing_newly_complete)
        {
          last_render_completion_time = cur_time;
          if (overlays_enabled && overlay_highlight_state)
            { // Schedule return to the regular overlay flashing state
              double dwell =
                kdws_overlay_highlight_dwell[overlay_highlight_state-1];
              schedule_overlay_flash_wakeup(cur_time+dwell);
            }
          else if (overlays_enabled && overlay_flashing_state &&
                   (!in_playmode))
            { // Schedule the next flashing overlay wakeup time
              double dwell=kdws_overlay_flash_dwell[overlay_flashing_state-1];
              schedule_overlay_flash_wakeup(cur_time+dwell);
            }
          if (jpip_client_received_bytes_since_last_refresh)
            schedule_render_refresh_wakeup(cur_time + jpip_refresh_interval);
        }
    }
  if (in_playmode)
    adjust_frame_queue_presentation_and_wakeup(cur_time);
  else if (processed_something)
    { // Paint any newly decompressed region right away.
      new_region &= view_dims; // No point in painting invisible regions.
      if (!new_region.is_empty())
        {
          if (pixel_scale > 1)
            { // Adjust the invalidated region so interpolated painting
              // produces no cracks.
              new_region.pos.x--; new_region.pos.y--;
              new_region.size.x += 2; new_region.size.y += 2;
            }
          paint_region(NULL,new_region);
        }
      if (jpip_client == NULL)
        display_status(); // All processing done, keep status up to date
      else if (processing_newly_complete)
        { // Should call `display_status' now because the progress bar can
          // only be updated when there is no rendering in progress
          assert(cur_time > 0.0);
          last_status_update_time = cur_time;
          display_status();
          next_status_update_time = -1.0;
        }
    }
  return ((compositor != NULL) && !compositor->is_processing_complete());
}

/*****************************************************************************/
/*         kdws_renderer::adjust_frame_queue_presentation_and_wakeup         */
/*****************************************************************************/

void kdws_renderer::adjust_frame_queue_presentation_and_wakeup(double cur_time)
{
  assert(playmode_frame_queue_size >= 2);
  
  // Find out exactly how many frames on the queue have already passed
  // their expiry date
  kdu_long stamp;
  int display_frame_idx; // Will be the frame index of any frame we present
  double display_end_time; // Will be the end-time for any frame we present
  bool have_current_frame_on_queue = false; // Will be true if the queue
          // contains a frame whose end time has not yet been reached.
  while (compositor->inspect_composition_queue(num_expired_frames_on_queue,
                                               &stamp,&display_frame_idx))
    {
      display_end_time = (0.001*stamp) + playclock_base;
      if (display_end_time > (cur_time+0.0011))
        { have_current_frame_on_queue = true; break; }
      num_expired_frames_on_queue++;
    }
  
  // See if playmode should stop
  if (pushed_last_frame && !have_current_frame_on_queue)
    {
      stop_playmode();
      return;
    }
    
  // Now see how many frames we can safely pop from the queue, ensuring that
  // at least one frame remains to be presented.
  bool have_new_frame_to_push =
    compositor->is_processing_complete() && !pushed_last_frame;
  int num_frames_to_pop = num_expired_frames_on_queue;
  if (!(have_current_frame_on_queue || have_new_frame_to_push))
    num_frames_to_pop--;
  
  // Now try to pop these frames if we can
  bool need_to_present_new_frame = false;
  if ((num_frames_to_pop > 0) && frame_presenter->disable())
    {
      for (; num_frames_to_pop > 0;
           num_frames_to_pop--, num_expired_frames_on_queue--)
        compositor->pop_composition_buffer();
      buffer = compositor->get_composition_bitmap(buffer_dims);
      need_to_present_new_frame = true; // Disabled the presenter, so we will
          // need to re-present something, even if we don't have a current
          // frame to present.  In any case, `display_frame_idx' and
          // `display_end_time' are correct for the frame to be presented.
    }
  
  // Now see if we can push any newly generated frame onto the queue
  if (have_new_frame_to_push &&
      !compositor->inspect_composition_queue(playmode_frame_queue_size-1))
    { // Can push a new frame onto the composition queue
      double pushed_frame_end_time = get_absolute_frame_end_time();
      double pushed_frame_start_time = get_absolute_frame_start_time();
      int pushed_frame_idx = frame_idx;
      
      compositor->set_surface_initialization_mode(false);
      compositor->push_composition_buffer(
          (kdu_long)((pushed_frame_end_time-playclock_base)*1000),frame_idx);
      playmode_stats.update(frame_idx,pushed_frame_start_time,
                            pushed_frame_end_time,cur_time);
      
      // Pushing a composition buffer is tantamount to a frame refresh.  This
      // starts the rendering process from scratch, so we should start
      // watching for refresh events due to JPIP cache activity from scratch.
      next_render_refresh_time = -1.0;
      jpip_client_received_bytes_since_last_refresh = false;
      
      // Set up the next frame to render
      if (!set_frame(frame_idx+((playmode_reverse)?-1:1)))
        pushed_last_frame = true;
      if (pushed_last_frame && playmode_repeat)
        {
          if (!playmode_reverse)
            set_frame(0);
          else if (num_frames > 0)
            set_frame(num_frames-1);
          else
            set_frame(100000); // Let `set_frame' find `num_frames'
          playclock_offset +=
            pushed_frame_end_time - get_absolute_frame_start_time();
          pushed_last_frame = false;
        }
      if (!have_current_frame_on_queue)
        { // The newly pushed frame should be presented, unless we were
          // unable to disable the `frame_presenter'
          if (num_expired_frames_on_queue==0) // Else `disable' call failed
            need_to_present_new_frame = true;
          display_end_time = pushed_frame_end_time;
          display_frame_idx = pushed_frame_idx;
          have_current_frame_on_queue = (display_end_time > cur_time);
          if (num_expired_frames_on_queue == 0)
            need_to_present_new_frame = true;
        }
      buffer = compositor->get_composition_bitmap(buffer_dims);
      double overdue = cur_time - pushed_frame_start_time;
      if (overdue > 0.1)
        { // We are running behind by more than 100ms
          manager->broadcast_playclock_adjustment(0.5*overdue);
             // Broadcasting the adjustment to all windows allows multiple
             // video playback windows to keep roughy in sync.
        }
    }
  
  if (need_to_present_new_frame)
    {
      frame_presenter->present_frame(this,display_frame_idx);
      playmode_frame_idx = display_frame_idx;
      if (cur_time > (last_status_update_time+0.25))
        {
          display_status();
          last_status_update_time = cur_time;
          next_status_update_time = -1.0;
        }
    }
  if (have_current_frame_on_queue && compositor->is_processing_complete())
    schedule_frame_end_wakeup(display_end_time);
}

/*****************************************************************************/
/*                          kdws_renderer::wakeup                            */
/*****************************************************************************/

void kdws_renderer::wakeup(double scheduled_time,double current_time)
{
  if (current_time < scheduled_time)
    current_time = scheduled_time; // Just in case; should not happen
  next_scheduled_wakeup_time = -1.0; // Nothing scheduled right now
  
  // Perform all relevant activities which were scheduled
  if ((next_overlay_flash_time >= 0.0) &&
      (next_overlay_flash_time <= current_time))
    {
      next_overlay_flash_time = -1.0;
      if (overlays_enabled && (compositor != NULL) &&
          (overlay_highlight_state || overlay_flashing_state))
        {
          if (overlay_highlight_state)
            { 
              overlay_highlight_state++;
              if (overlay_highlight_state > KDWS_OVERLAY_HIGHLIGHT_STATES)
                overlay_highlight_state = 0;
            }
          else if (overlay_flashing_state)
            { 
              overlay_flashing_state++;
              if (overlay_flashing_state > KDWS_OVERLAY_FLASHING_STATES)
                overlay_flashing_state = 1;
            }
          configure_overlays();
        }
    }
  bool need_display_status = false;
  if ((next_status_update_time >= 0.0) &&
      (next_status_update_time <= current_time))
    {
      next_status_update_time = -1.0;
      need_display_status = true;
    }
  if ((next_frame_end_time >= 0.0) &&
      (next_frame_end_time <= current_time))
    {
      assert(in_playmode);
      next_frame_end_time = -1.0;
      adjust_frame_queue_presentation_and_wakeup(current_time);
    }
  if ((next_render_refresh_time >= 0.0) &&
      (next_render_refresh_time <= current_time))
    {
      next_render_refresh_time = -1.0;
      if ((jpip_client != NULL) && (compositor != NULL) &&
          compositor->is_processing_complete())
        refresh_display();
    }
  if (need_display_status)
    { // We need to do this after any call to `adjust_frame_queue...' since it
      // may affect the displayed frame index, but we needed to reset the
      // `next_display_update_time' member before calling that function so as
      // to be sure that there were no pending wakeup events -- because the
      // `adjust_frame_queue...' function may itself schedule a wakeup.
      if (current_time > (last_status_update_time + 0.01))
        {
          last_status_update_time = current_time;
          display_status();
        }
    }
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*                   kdws_renderer::client_notification                      */
/*****************************************************************************/

void kdws_renderer::client_notification()
{
  if (jpip_client == NULL)
    return;
  bool need_to_update_request = false;
  if (compositor == NULL)
    open_file(NULL); // Try to complete the client source opening operation
  if ((compositor != NULL) && !configuration_complete)
    initialize_regions(false);
  if ((compositor != NULL) && compositor->waiting_for_stream_headers())
    { // See if we can now find the stream headers
      if (refresh_display(true))
        need_to_update_request = true;
    }  
  if (respect_initial_jpip_request && configuration_complete &&
      !one_time_jpip_request)
    respect_initial_jpip_request = false;

  const char *new_status_text = "<NO JPIP>";
  kdu_long new_bytes = jpip_client_received_queue_bytes;
  if (jpip_client != NULL)
    {
      new_status_text = jpip_client->get_status(client_request_queue_id);
      new_bytes = jpip_client->get_received_bytes(client_request_queue_id);
      if (configuration_complete &&
          jpip_client->get_window_in_progress(&tmp_roi,
                                              client_request_queue_id))
        {
          if ((client_roi.region != tmp_roi.region) ||
              (client_roi.resolution != tmp_roi.resolution))
            change_client_focus(tmp_roi.resolution,tmp_roi.region);
          client_roi.copy_from(tmp_roi);
        }
    }
  
  bool status_text_changed = (new_status_text != jpip_client_status);
  bool bytes_changed = (new_bytes != jpip_client_received_queue_bytes);
  jpip_client_status = new_status_text;
  jpip_client_received_queue_bytes = new_bytes;
  double cur_time = -1.0; // Initialize when we need it
  if (status_text_changed)
    {
      if (cur_time < 0.0)
        cur_time = manager->get_current_time();
      last_status_update_time = cur_time;
      display_status();
    }
  if (bytes_changed)
    {
      jpip_client_received_bytes_since_last_refresh = true;
      jpip_client_received_total_bytes = jpip_client->get_received_bytes(-1);
            // By updating the total client bytes only when the queue bytes
            // changes we can be sure that the value reflects any bytes
            // received on the current queue.  This allows us to separately
            // test for changes in the total client bytes at important
            // junctures (such as in `set_key_window_status') and be sure
            // that we only detect bytes received above and beyond those
            // available at the time when the bytes for the current queue
            // last changed.
      
      if ((compositor != NULL) && pending_show_metanode.exists())
        show_imagery_for_metanode(pending_show_metanode);
      
      if ((compositor != NULL) && (next_render_refresh_time < 0.0) &&
          compositor->is_processing_complete())
        { // Should refresh display; no refresh yet scheduled
          if (cur_time < 0.0)
            cur_time = manager->get_current_time();
          if (cur_time > (last_render_completion_time +
                          jpip_refresh_interval - 0.1))
            refresh_display();
          else
            schedule_render_refresh_wakeup(last_render_completion_time +
                                           jpip_refresh_interval);
        }

      if ((!status_text_changed) && (next_status_update_time < 0.0))
        { // Should update status text now or soon; no update yet scheduled
          if (cur_time < 0.0)
            cur_time = manager->get_current_time();
          if (cur_time > (last_status_update_time+0.25))
            {
              last_status_update_time = cur_time;
              display_status();
            }
          else
            schedule_status_update_wakeup(cur_time+0.25);
        }
      
      if (editor == NULL)
        {
          if ((metashow != NULL) || (catalog_source != NULL))
            { // Metashow should be updated
              if (metashow != NULL)
                metashow->update_metadata();
              if (catalog_source != NULL)
                catalog_source->update_metadata();
            }
          if ((compositor != NULL) && (catalog_source == NULL) &&
              (!catalog_closed) && jpx_in.exists())
            {
              jpx_meta_manager meta_manager = jpx_in.access_meta_manager();
              meta_manager.load_matches(-1,NULL,-1,NULL);
              if (meta_manager.peek_touched_nodes(jp2_label_4cc).exists())
                window->create_metadata_catalog();
            }
        }
    }
  if (jpip_client == NULL)
    return;
  if (need_to_update_request)
    update_client_window_of_interest();
  else if (jpip_client->is_idle(client_request_queue_id) &&
           catalog_has_novel_metareqs)
    { // Server has finished responding to all requests, but the catalog source
      // has may now have enough data to make a more precise request for the
      // metadata it actually needs.  We need to keep re-invoking
      // `update_client_window_of_interest' until the client goes idle
      // after a request in which there were no novel metareqs -- a
      // novel metareq is a metadata request which differs in any way
      // from metadata requests posted previously, to which the server's
      // response was "window-done".
      update_client_window_of_interest();
    }
}

/*****************************************************************************/
/*                      kdws_renderer::refresh_display                       */
/*****************************************************************************/

bool kdws_renderer::refresh_display(bool new_imagery_only)
{
  if (compositor == NULL)
    return false;

  bool something_refreshed = configuration_complete;
  bool *check_new_imagery_only = NULL;
  if (new_imagery_only)
    check_new_imagery_only = &something_refreshed;
  if (configuration_complete && !compositor->refresh(check_new_imagery_only))
    { // Can no longer trust buffer surfaces
      if (check_new_imagery_only != NULL)
        assert(something_refreshed);
      frame_presenter->reset(); // Should not be possible to be in playmode
            // if refresh invalidates the display dims, but best to be safe.
      kdu_dims new_image_dims;
      bool have_valid_scale = 
        compositor->get_total_composition_dims(new_image_dims);
      if ((!have_valid_scale) || (new_image_dims != image_dims))
        initialize_regions(false);
      else
        buffer = compositor->get_composition_bitmap(buffer_dims);
    }

  if (something_refreshed)
    { // Refreshing starts the rendering process from scratch, so we start
      // watching for refresh events due to JPIP cache activity from scratch.
      next_render_refresh_time = -1.0;
      jpip_client_received_bytes_since_last_refresh = false;
    }
  
  return something_refreshed;
}

/*****************************************************************************/
/*               kdws_renderer::schedule_overlay_flash_wakeup                */
/*****************************************************************************/

void kdws_renderer::schedule_overlay_flash_wakeup(double when)
{
  next_overlay_flash_time = when;
  if ((next_render_refresh_time >= 0.0) &&
      (next_render_refresh_time > (when-0.1)) &&
      (next_render_refresh_time < (when+0.1)))
    next_render_refresh_time = when;
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*              kdws_renderer::schedule_render_refresh_wakeup                */
/*****************************************************************************/

void kdws_renderer::schedule_render_refresh_wakeup(double when)
{
  if ((next_scheduled_wakeup_time >= 0.0) &&
      (next_scheduled_wakeup_time > (when-0.1)) &&
      (next_scheduled_wakeup_time < (when+0.1)))
    when = next_scheduled_wakeup_time;
  next_render_refresh_time = when;  
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*               kdws_renderer::schedule_status_update_wakeup                */
/*****************************************************************************/

void kdws_renderer::schedule_status_update_wakeup(double when)
{
  if ((next_scheduled_wakeup_time >= 0.0) &&
      (next_scheduled_wakeup_time > (when-0.1)) &&
      (next_scheduled_wakeup_time < (when+0.1)))
    when = next_scheduled_wakeup_time;
  next_status_update_time = when;  
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*               kdws_renderer::install_next_scheduled_wakeup                */
/*****************************************************************************/

void kdws_renderer::install_next_scheduled_wakeup()
{
  double tmp, min_time = next_render_refresh_time;
  if (((tmp=next_overlay_flash_time) >= 0.0) &&
      ((min_time < 0.0) || (min_time > tmp)))
    min_time = tmp;
  if (((tmp=next_frame_end_time) >= 0.0) &&
      ((min_time < 0.0) || (min_time > tmp)))
    min_time = tmp;
  if (((tmp=next_status_update_time) >= 0.0) &&
      ((min_time < 0.0) || (min_time > tmp)))
    min_time = tmp;
  if (min_time < 0.0)
    {
      if (next_scheduled_wakeup_time >= 0.0)
        manager->schedule_wakeup(window,-1.0); // Cancel existing wakeup
      next_scheduled_wakeup_time = -100.0; // Make sure it cannot pass any
        // of the tests in the above functions, which coerce a loose wakeup
        // time to equal the next scheduled wakeup time if it is already close.
    }
  else if (min_time != next_scheduled_wakeup_time)
    {
      next_scheduled_wakeup_time = min_time;
      manager->schedule_wakeup(window,min_time);
    }
}

/*****************************************************************************/
/*                       kdws_renderer::increase_scale                       */
/*****************************************************************************/

float kdws_renderer::increase_scale(float from_scale, bool slightly)
{
  float min_scale = from_scale*1.5F;
  float max_scale = from_scale*3.0F;
  float scale_anchor = from_scale*2.0F;
  if (slightly)
    {
      min_scale = from_scale*1.1F;
      max_scale = from_scale*1.3F;
      scale_anchor = from_scale*1.2F;
    }
  if (max_rendering_scale > 0.0F)
    {
      if (min_scale > max_rendering_scale)
        min_scale = max_rendering_scale;
      if (max_scale > max_rendering_scale)
        max_scale = max_rendering_scale;
    }
  if (compositor == NULL)
    return min_scale;
  kdu_dims region_basis;
  if (configuration_complete)
    region_basis = (enable_focus_box)?focus_box:view_dims;
  float new_scale = compositor->find_optimal_scale(region_basis,scale_anchor,
                                                   min_scale,max_scale);
  if (new_scale < min_scale)
    max_rendering_scale = new_scale; // This is as large as we can go
  return new_scale;  
}

/*****************************************************************************/
/*                       kdws_renderer::decrease_scale                       */
/*****************************************************************************/

float kdws_renderer::decrease_scale(float from_scale, bool slightly)
{
  float max_scale = from_scale/1.5F;
  float min_scale = from_scale/3.0F;
  float scale_anchor = from_scale/2.0F;
  if (slightly)
    {
      max_scale = from_scale/1.1F;
      min_scale = from_scale/1.3F;
      scale_anchor = from_scale/1.2F;      
    }
  if (min_rendering_scale > 0.0F)
    {
      if (min_scale < min_rendering_scale)
        min_scale = min_rendering_scale;
      if (max_scale < min_rendering_scale)
        max_scale = min_rendering_scale;
    }
  if (compositor == NULL)
    return max_scale;
  kdu_dims region_basis;
  if (configuration_complete)
    region_basis = (enable_focus_box)?focus_box:view_dims;
  float new_scale = compositor->find_optimal_scale(region_basis,scale_anchor,
                                                   min_scale,max_scale);
  if (new_scale > max_scale)
    min_rendering_scale = new_scale; // This is as small as we can go
  return new_scale;
}

/*****************************************************************************/
/*                      kdws_renderer::change_frame_idx                      */
/*****************************************************************************/

void kdws_renderer::change_frame_idx(int new_frame_idx)
  /* Note carefully that, on entry to this function, only the `frame_start'
   time can be relied upon.  The function sets `frame_end' from scratch,
   rather than basing the newly calculated value on a previous one. */
{
  if (new_frame_idx < 0)
    new_frame_idx = 0;
  if ((num_frames >= 0) && (new_frame_idx >= (num_frames-1)))
    new_frame_idx = num_frames-1;
  if ((new_frame_idx == frame_idx) && (frame_end > frame_start))
    return; // Nothing to do
  
  if (composition_rules.exists() && (frame != NULL))
    {
      int num_instructions, duration, repeat_count, delta;
      bool is_persistent;
      
      if (new_frame_idx == 0)
        {
          frame = composition_rules.get_next_frame(NULL);
          frame_iteration = 0;
          frame_idx = 0;
          frame_start = 0.0;
          composition_rules.get_frame_info(frame,num_instructions,duration,
                                           repeat_count,is_persistent);
          frame_end = frame_start + 0.001*duration;
        }
      
      while (frame_idx < new_frame_idx)
        {
          composition_rules.get_frame_info(frame,num_instructions,duration,
                                           repeat_count,is_persistent);
          delta = repeat_count - frame_iteration;
          if (delta > 0)
            {
              if ((frame_idx+delta) > new_frame_idx)
                delta = new_frame_idx - frame_idx;
              frame_iteration += delta;
              frame_idx += delta;
              frame_start += delta * 0.001*duration;
              frame_end = frame_start + 0.001*duration;
            }
          else
            {
              jx_frame *new_frame = composition_rules.get_next_frame(frame);
              frame_end = frame_start + 0.001*duration; // Just in case
              if (new_frame == NULL)
                {
                  num_frames = frame_idx+1;
                  break;
                }
              else
                { frame = new_frame; frame_iteration=0; }
              composition_rules.get_frame_info(frame,num_instructions,duration,
                                               repeat_count,is_persistent);
              frame_start = frame_end;
              frame_end += 0.001*duration;
              frame_idx++;
            }
        }
      
      while (frame_idx > new_frame_idx)
        {
          composition_rules.get_frame_info(frame,num_instructions,duration,
                                           repeat_count,is_persistent);
          if (frame_iteration > 0)
            {
              delta = frame_idx - new_frame_idx;
              if (delta > frame_iteration)
                delta = frame_iteration;
              frame_iteration -= delta;
              frame_idx -= delta;
              frame_start -= delta * 0.001*duration;
              frame_end = frame_start + 0.001*duration;
            }
          else
            {
              frame = composition_rules.get_prev_frame(frame);
              assert(frame != NULL);
              composition_rules.get_frame_info(frame,num_instructions,duration,
                                               repeat_count,is_persistent);
              frame_iteration = repeat_count;
              frame_idx--;
              frame_start -= 0.001*duration;
              frame_end = frame_start + 0.001*duration;
            }
        }
    }
  else if (mj2_in.exists() && (single_layer_idx >= 0))
    {
      mj2_video_source *track =
        mj2_in.access_video_track((kdu_uint32)(single_layer_idx+1));
      if (track == NULL)
        return;
      frame_idx = new_frame_idx;
      track->seek_to_frame(new_frame_idx);
      frame_start = ((double) track->get_frame_instant()) /
        ((double) track->get_timescale());
      frame_end = frame_start + ((double) track->get_frame_period()) /
        ((double) track->get_timescale());
    }
}

/*****************************************************************************/
/*                 kdws_renderer::find_compatible_frame_idx                  */
/*****************************************************************************/

int
  kdws_renderer::find_compatible_frame_idx(int num_streams, const int *streams,
                                       int num_layers, const int *layers,
                                       int num_regions, const jpx_roi *regions,
                                       int &compatible_codestream_idx,
                                       int &compatible_layer_idx,
                                       bool match_all_layers,
                                       bool advance_to_next)
{
    if (!composition_rules)
    composition_rules = jpx_in.access_composition();
  if (!composition_rules)
    return -1; // No frames can be accessed.
  
  int scan_frame_idx, scan_frame_iteration;
  jx_frame *scan_frame;
  if (frame != NULL)
    {
      scan_frame_idx = this->frame_idx;
      scan_frame_iteration = this->frame_iteration;
      scan_frame = this->frame;
    }
  else
    {
      scan_frame_idx = 0;
      scan_frame_iteration = 0;
      scan_frame = composition_rules.get_next_frame(NULL);
      advance_to_next = false;
    }
  if (scan_frame == NULL)
    return -1; // Unable to access any frames yet; maybe cache needs more data
  
  bool is_persistent;
  int num_instructions, duration, repeat_count;
  composition_rules.get_frame_info(scan_frame,num_instructions,duration,
                                   repeat_count,is_persistent);
  if (advance_to_next)
    {
      scan_frame_idx++;
      scan_frame_iteration++;
      if (scan_frame_iteration > repeat_count)
        { // Need to access next frame
          scan_frame = composition_rules.get_next_frame(scan_frame);
          if (scan_frame == NULL)
            {
              scan_frame = composition_rules.get_next_frame(NULL);
              scan_frame_idx = 0;
              assert(scan_frame != NULL);
            }
          scan_frame_iteration = 0;
          composition_rules.get_frame_info(scan_frame,num_instructions,
                                           duration,repeat_count,
                                           is_persistent);
        }
    }
  
  if ((num_regions > 0) || (num_streams > 0) || (num_layers < 2))
    match_all_layers = false;
  bool *matched_layers = NULL;
  if (match_all_layers)
    {
      matched_layers = new bool[num_layers];
      for (int n=0; n < num_layers; n++)
        matched_layers[n] = false;
    }
  
  int lim_frame_idx = scan_frame_idx; // Loop ends when we get back here
  do {
    if (num_streams > 0)
      { // Have to match codestream
        int n;
        kdu_dims composition_region;
        for (n=0; n < num_streams; n++)
          {
            int r, l_idx, cs_idx = streams[n];
            if (num_regions > 0)
              { // Have to find visible region
                for (r=0; r < num_regions; r++)
                  { // Have to match
                    l_idx = frame_expander.test_codestream_visibility(
                                     &jpx_in,scan_frame,scan_frame_iteration,
                                     true,cs_idx,composition_region,
                                     regions[r].region);
                    if ((l_idx >= 0) && (num_layers > 0))
                      { // Have to match one of the compositing layers
                        for (int p=0; p < num_layers; p++)
                          if (layers[p] == l_idx)
                            { // Found a match
                              compatible_codestream_idx = cs_idx;
                              compatible_layer_idx = l_idx;
                              assert(matched_layers == NULL);
                              return scan_frame_idx;
                            }
                      }
                    else if (l_idx >= 0)
                      { // Any compositing layer will do
                        compatible_codestream_idx = cs_idx;
                        compatible_layer_idx = l_idx;
                        assert(matched_layers == NULL);
                        return scan_frame_idx;
                      }
                  }
              }
            else
              { // Any visible portion of the codestream will do
                l_idx = frame_expander.test_codestream_visibility(
                                   &jpx_in,scan_frame,scan_frame_iteration,
                                   true,cs_idx,composition_region);
                if ((l_idx >= 0) && (num_layers > 0))
                  { // Have to match one of the compositing layers
                    for (int p=0; p < num_layers; p++)
                      if (layers[p] == l_idx)
                        { // Found a match
                          compatible_codestream_idx = cs_idx;
                          compatible_layer_idx = l_idx;
                          return scan_frame_idx;
                        }
                  }
                else if (l_idx >= 0)
                  { // Any compositing layer will do
                    compatible_codestream_idx = cs_idx;
                    compatible_layer_idx = l_idx;
                    assert(matched_layers == NULL);
                    return scan_frame_idx;
                  }
              }
          }
      }
    else
      { // Only have to match compositing layers
        frame_expander.construct(&jpx_in,scan_frame,scan_frame_iteration,true);
        int m, num_members = frame_expander.get_num_members();
        if ((num_members >= num_layers) || !match_all_layers)
          {
            for (m=0; m < num_members; m++)
              {
                bool covers;
                int n, inst_idx, l_idx;
                kdu_dims src_dims, tgt_dims;
                if (frame_expander.get_member(m,inst_idx,l_idx,covers,
                                              src_dims,tgt_dims) == NULL)
                  continue;
                for (n=0; n < num_layers; n++)
                  if (layers[n] == l_idx)
                    {
                      if (match_all_layers)
                        matched_layers[n] = true;
                      else
                        {
                          frame_expander.reset();
                          compatible_layer_idx = l_idx;
                          return scan_frame_idx;
                        }
                    }
              }
            if (match_all_layers)
              { // Check if all layers matched
                bool all_matched = true;
                for (m=0; m < num_layers; m++)
                  {
                    all_matched &= matched_layers[m];
                    matched_layers[m] = false;
                  }
                if (all_matched)
                  {
                    delete[] matched_layers;
                    frame_expander.reset();
                    compatible_layer_idx = layers[0];
                    return scan_frame_idx;
                  }
              }
          }
      }
    scan_frame_idx++;
    scan_frame_iteration++;
    if (scan_frame_iteration > repeat_count)
      { // Need to access next frame
        scan_frame = composition_rules.get_next_frame(scan_frame);
        if (scan_frame == NULL)
          {
            scan_frame = composition_rules.get_next_frame(NULL);
            scan_frame_idx = 0;
            assert(scan_frame != NULL);
          }
        scan_frame_iteration = 0;
        composition_rules.get_frame_info(scan_frame,num_instructions,
                                         duration,repeat_count,
                                         is_persistent);
      }
  } while (scan_frame_idx != lim_frame_idx);

  if (matched_layers != NULL)
    delete[] matched_layers;
  return -1; // Didn't find any match          
}

/*****************************************************************************/
/*                       kdws_renderer::adjust_rel_focus                     */
/*****************************************************************************/

bool
  kdws_renderer::adjust_rel_focus()
{
  if (image_dims.is_empty() ||
      !(rel_focus.is_valid() && configuration_complete))
    return false;

  kdu_istream_ref istr;
  kdu_ilayer_ref ilyr;
  if (single_component_idx >= 0)
    {
      if (rel_focus.codestream_idx == single_codestream_idx)
        { // We have a focus region expressed directly for this codestream
          if ((rel_focus.frame_idx < 0) && (rel_focus.layer_idx < 0))
            return false; // No change; focus was already on single codestream
          rel_focus.frame_idx = rel_focus.layer_idx = -1;
          rel_focus.region = rel_focus.codestream_region;
        }
    }
  else if (jpx_in.exists() && (single_layer_idx >= 0))
    {
      kdu_dims region, rel_dims;
      if (rel_focus.layer_idx == single_layer_idx)
        { // We have a focus region expressed directly for this layer
          if (rel_focus.frame_idx < 0)
            return false; // No change; focus was already on single layer
          rel_focus.frame_idx = -1;
          rel_focus.region = rel_focus.layer_region;
          return true;
        }
      else if ((rel_focus.codestream_idx >= 0) &&
               ((istr=compositor->get_next_istream(kdu_istream_ref(),
                          true,false,rel_focus.codestream_idx)).exists()) &&
               !(rel_dims=compositor->find_istream_region(istr,
                                                          false)).is_empty())
        { 
          rel_focus.frame_idx = -1;
          region=rel_focus.codestream_region.apply_to_container(rel_dims);
          rel_focus.region.set(region,image_dims);
          return true;
        }
    }
  else
    { // In JPX frame composition (or MJ2 frame display) mode
      kdu_dims region, rel_dims;
      if (rel_focus.frame_idx == frame_idx)
        return false; // Nothing to change
      else if ((rel_focus.layer_idx >= 0) &&
               ((ilyr=compositor->get_next_ilayer(kdu_ilayer_ref(),
                                            rel_focus.layer_idx)).exists()) &&
               !(rel_dims =
                 compositor->find_ilayer_region(ilyr,false)).is_empty())
        {
          region = rel_focus.layer_region.apply_to_container(rel_dims);
          rel_focus.frame_idx = frame_idx;
          rel_focus.region.set(region,image_dims);
          return true;
        }
      else if ((rel_focus.codestream_idx >= 0) &&
               ((istr=compositor->get_next_istream(kdu_istream_ref(),
                            true,false,rel_focus.codestream_idx)).exists()) &&
               !(rel_dims=compositor->find_istream_region(istr,
                                                          false)).is_empty())
        {
          region = rel_focus.codestream_region.apply_to_container(rel_dims);
          rel_focus.frame_idx = frame_idx;
          rel_focus.region.set(region,image_dims);
          return true;
        }
    }

 // If we get here, we can't transform the focus box into anything compatible
  enable_focus_box=false;
  rel_focus.reset();
  return true;
}

/*****************************************************************************/
/*             kdws_renderer::calculate_rel_view_and_focus_params            */
/*****************************************************************************/

void
  kdws_renderer::calculate_rel_view_and_focus_params()
{
  view_centre_known = false; rel_focus.reset();
  if ((buffer == NULL) || !image_dims)
    return;
  view_centre_known = true;
  view_centre_x =
    (double)(view_dims.pos.x + view_dims.size.x/2 - image_dims.pos.x) /
      ((double) image_dims.size.x);
  view_centre_y =
    (double)(view_dims.pos.y + view_dims.size.y/2 - image_dims.pos.y) /
      ((double) image_dims.size.y);
  if (!enable_focus_box)
    return;
  rel_focus.region.set(focus_box,image_dims);
  kdu_coords focus_point = focus_box.pos;
  focus_point.x += (focus_box.size.x >> 1);
  focus_point.y += (focus_box.size.y >> 1);
  if (single_component_idx >= 0)
    {
      rel_focus.codestream_idx = single_codestream_idx;
      rel_focus.codestream_region = rel_focus.region;
    }
  else if (jpx_in.exists() && (single_layer_idx >= 0))
    {
      rel_focus.layer_idx = single_layer_idx;
      rel_focus.layer_region = rel_focus.region;
      kdu_ilayer_ref ilyr = compositor->find_point(focus_point);
      if (ilyr.exists())
        {
          kdu_istream_ref istr = compositor->get_ilayer_stream(ilyr,0);
          kdu_dims cs_dims = compositor->find_istream_region(istr,false);
          if (rel_focus.codestream_region.set(focus_box,cs_dims))
            compositor->get_istream_info(istr,rel_focus.codestream_idx);
        }
    }
  else
    {
      rel_focus.frame_idx = frame_idx;
      kdu_ilayer_ref ilyr = compositor->find_point(focus_point);
      if (ilyr.exists())
        {
          kdu_istream_ref istr = compositor->get_ilayer_stream(ilyr,0);
          kdu_dims cs_dims=compositor->find_istream_region(istr,false);
          if (rel_focus.codestream_region.set(focus_box,cs_dims))
            compositor->get_istream_info(istr,rel_focus.codestream_idx);
          kdu_dims l_dims=compositor->find_ilayer_region(ilyr,false);
          if (rel_focus.layer_region.set(focus_box,l_dims))
            { 
              int cs=-1; bool opq;
              compositor->get_ilayer_info(ilyr,rel_focus.layer_idx,cs,opq);
            }
        }
    }
}

/*****************************************************************************/
/*                      kdws_renderer::get_new_focus_box                     */
/*****************************************************************************/

kdu_dims
  kdws_renderer::get_new_focus_box()
{
  kdu_dims new_focus_box = focus_box;
  if ((!enable_focus_box) || (!focus_box))
    new_focus_box = view_dims;
  if (enable_focus_box && rel_focus.is_valid())
    new_focus_box = rel_focus.region.apply_to_container(image_dims);

  // Adjust box to fit into view port.
  if (new_focus_box.pos.x < view_dims.pos.x)
    new_focus_box.pos.x = view_dims.pos.x;
  if ((new_focus_box.pos.x+new_focus_box.size.x) >
      (view_dims.pos.x+view_dims.size.x))
    new_focus_box.pos.x = view_dims.pos.x+view_dims.size.x-new_focus_box.size.x;
  if (new_focus_box.pos.y < view_dims.pos.y)
    new_focus_box.pos.y = view_dims.pos.y;
  if ((new_focus_box.pos.y+new_focus_box.size.y) >
      (view_dims.pos.y+view_dims.size.y))
    new_focus_box.pos.y = view_dims.pos.y+view_dims.size.y-new_focus_box.size.y;
  new_focus_box &= view_dims;

  if (!new_focus_box)
    new_focus_box = view_dims;
  return new_focus_box;
}

/*****************************************************************************/
/*                       kdws_renderer::set_focus_box                        */
/*****************************************************************************/

void
  kdws_renderer::set_focus_box(kdu_dims new_box)
{
  rel_focus.reset();
  if ((compositor == NULL) || (buffer == NULL))
    return;
  new_box.pos += view_dims.pos;
  kdu_dims previous_box = focus_box;
  bool previous_enable = enable_focus_box;
  if (new_box.area() < (kdu_long) 100)
    enable_focus_box = false;
  else
    { focus_box = new_box; enable_focus_box = true; }
  lock_view();
  focus_box = get_new_focus_box();
  enable_focus_box = (focus_box != view_dims);
  rel_focus.reset();
  unlock_view();
  calculate_rel_view_and_focus_params();
  if (previous_enable != enable_focus_box)
    compositor->invalidate_rect(view_dims);
  else if (enable_focus_box)
    for (int w=0; w < 2; w++)
      {
        kdu_dims box = (w==0)?previous_box:focus_box;
        box.pos.x  -= KDWS_FOCUS_MARGIN;    box.pos.y  -= KDWS_FOCUS_MARGIN;
        box.size.x += 2*KDWS_FOCUS_MARGIN;  box.size.y += 2*KDWS_FOCUS_MARGIN;
        compositor->invalidate_rect(box);
      }
  if (jpip_client != NULL)
    update_client_window_of_interest();
  display_status();
}

/*****************************************************************************/
/*                       kdws_renderer::update_focus_box                     */
/*****************************************************************************/

void
  kdws_renderer::update_focus_box()
{
  kdu_dims new_focus_box = get_new_focus_box();
  bool new_focus_enable = (new_focus_box != view_dims);
  if ((new_focus_box != focus_box) || (new_focus_enable != enable_focus_box))
    {
      bool previous_enable = enable_focus_box;
      kdu_dims previous_box = focus_box;
      lock_view();
      focus_box = new_focus_box;
      enable_focus_box = new_focus_enable;
      unlock_view();
      if (previous_enable != enable_focus_box)
        compositor->invalidate_rect(view_dims);
      else if (enable_focus_box)
        for (int w=0; w < 2; w++)
          {
            kdu_dims box = (w==0)?previous_box:focus_box;
            box.pos.x -=  KDWS_FOCUS_MARGIN;   box.pos.y -=  KDWS_FOCUS_MARGIN;
            box.size.x+=2*KDWS_FOCUS_MARGIN; box.size.y+=2*KDWS_FOCUS_MARGIN;
            compositor->invalidate_rect(box);
          }
    }
  if (enable_region_posting)
    update_client_window_of_interest();
}

/*****************************************************************************/
/*                      kdws_renderer::outline_focus_box                     */
/*****************************************************************************/

void
  kdws_renderer::outline_focus_box(CDC *dc, kdu_dims relative_region)
{
  if ((!enable_focus_box) || (focus_box == view_dims))
    return;
  kdu_dims test_dims = relative_region;
  test_dims.pos.x = test_dims.pos.x*pixel_scale - 3;
  test_dims.size.x = test_dims.size.x*pixel_scale + 6;
  test_dims.pos.y = test_dims.pos.y*pixel_scale - 3;
  test_dims.size.y = test_dims.size.y*pixel_scale + 6;
  kdu_dims box_dims = focus_box;
  box_dims.pos -= view_dims.pos;
  box_dims.pos.x *= pixel_scale; box_dims.pos.y *= pixel_scale;
  box_dims.size.x *= pixel_scale; box_dims.size.y *= pixel_scale;

  if (!box_dims.intersects(test_dims))
    return;
  dc->IntersectClipRect(test_dims.pos.x,test_dims.pos.y,
                        test_dims.pos.x+test_dims.size.x,
                        test_dims.pos.y+test_dims.size.y);
  POINT pt;
  for (int p=0; p < 2; p++)
    {
      if (p == 0)
        dc->SelectObject(focus_pen_inside);
      else
        dc->SelectObject(focus_pen_outside);
      pt.x = box_dims.pos.x-p; pt.y = box_dims.pos.y-p;
      dc->MoveTo(pt);
      pt.y += box_dims.size.y+p+p;
      dc->LineTo(pt);
      pt.x += box_dims.size.x+p+p;
      dc->LineTo(pt);
      pt.y = box_dims.pos.y-p;
      dc->LineTo(pt);
      pt.x = box_dims.pos.x-p;
      dc->LineTo(pt);
    }
}

/*****************************************************************************/
/*            kdws_renderer::check_initial_request_compatibility             */
/*****************************************************************************/

bool kdws_renderer::check_initial_request_compatibility()
{
  assert((jpip_client != NULL) && respect_initial_jpip_request);
  if ((!jpx_in.exists()) || (single_component_idx >= 0))
    return true;
  if (!jpip_client->get_window_in_progress(&tmp_roi,client_request_queue_id))
    return true; // Window information not available yet
  
  int c;
  kdu_sampled_range *rg;
  bool compatible = true;
  if ((single_component_idx < 0) && (single_layer_idx < 0) &&
      (frame_idx == 0) && (frame != NULL))
    { // Check compatibility of default animation frame
      int m, num_members = frame_expander.get_num_members();
      for (m=0; m < num_members; m++)
        {
          int instruction_idx, layer_idx;
          bool covers_composition;
          kdu_dims source_dims, target_dims;
          int remapping_ids[2];
          jx_frame *frame =
          frame_expander.get_member(m,instruction_idx,layer_idx,
                                    covers_composition,source_dims,
                                    target_dims);
          composition_rules.get_original_iset(frame,instruction_idx,
                                              remapping_ids[0],
                                              remapping_ids[1]);
          for (c=0; (rg=tmp_roi.contexts.access_range(c)) != NULL; c++)
            if ((rg->from <= layer_idx) && (rg->to >= layer_idx) &&
                (((layer_idx-rg->from) % rg->step) == 0))
              {
                if (covers_composition &&
                    (rg->remapping_ids[0] < 0) && (rg->remapping_ids[1] < 0))
                  break; // Found compatible layer
                if ((rg->remapping_ids[0] == remapping_ids[0]) &&
                    (rg->remapping_ids[1] == remapping_ids[1]))
                    break; // Found compatible layer
              }
          if (rg == NULL)
            { // No appropriate context request for layer; see if it needs one
              if (covers_composition && (layer_idx == 0) &&
                  jpx_in.access_compatibility().is_jp2_compatible())
                {
                  for (c=0;
                       (rg = tmp_roi.codestreams.access_range(c)) != NULL;
                       c++)
                    if (rg->from == 0)
                      break;
                }
            }
          if (rg == NULL)
            { // No response for this frame layer; must downgrade
              compatible = false;
              frame_idx = 0;
              frame_start = frame_end = 0.0;
              frame = NULL;
              single_layer_idx = 0; // Try this one
              break;
            }
        }
    }
  
  if (compatible)
    return true;
  
  bool have_codestream0 = false;
  for (c=0; (rg=tmp_roi.codestreams.access_range(c)) != NULL; c++)
    {
      if (rg->from == 0)
        have_codestream0 = true;
      if (rg->context_type == KDU_JPIP_CONTEXT_TRANSLATED)
        { // Use the compositing layer translated here
          int range_idx = rg->remapping_ids[0];
          int member_idx = rg->remapping_ids[1];
          rg = tmp_roi.contexts.access_range(range_idx);
          if (rg == NULL)
            continue;
          single_layer_idx = rg->from + rg->step*member_idx;
          if (single_layer_idx > rg->to)
            continue;
          return false; // Found a suitable compositing layer
        }
    }
  
  if (have_codestream0 && jpx_in.access_compatibility().is_jp2_compatible())
    {
      single_layer_idx = 0;
      return false; // Compositing layer 0 is suitable
    }
  
  // If we get here, we could not find a compatible layer
  single_layer_idx = -1;
  single_component_idx = 0;
  if ((rg=tmp_roi.codestreams.access_range(0)) != NULL)
    single_codestream_idx = rg->from;
  else
    single_codestream_idx = 0;

  return compatible;
}

/*****************************************************************************/
/*              kdws_renderer::update_client_window_of_interest              */
/*****************************************************************************/

void kdws_renderer::update_client_window_of_interest()
{
  if ((compositor == NULL) || (jpip_client == NULL) ||
      (!jpip_client->is_alive(client_request_queue_id)) ||
      respect_initial_jpip_request)
    return;
  
  // Now introduce requests for required metadata
  tmp_roi.init();
  catalog_has_novel_metareqs = false;
  if (catalog_source != NULL)
    { // Ask for metadata which may have no connection to the currently
      // requested image window, using KDU_MRQ_ALL.
      if (catalog_source->generate_client_requests(&tmp_roi) > 0)
        { // See if the catalog has issued any metadata requests which could
          // involve the delivery of new data
          if (!client_roi.metareq_contains(tmp_roi))
            catalog_has_novel_metareqs = true;
          else
            {
              int client_status_flags = 0;
              jpip_client->get_window_in_progress(NULL,client_request_queue_id,
                                                  &client_status_flags);
              if (!(client_status_flags & KDU_CLIENT_WINDOW_IS_COMPLETE))
                catalog_has_novel_metareqs = true;
            }
        }
      bool catalog_is_first_responder = !image_view->is_first_responder();
      if (catalog_has_novel_metareqs && catalog_is_first_responder &&
          !pending_show_metanode)
        { // The catalog has new metadata to request and also has priority
          // as first responder for the window; make this a metadata-only
          // request.
          tmp_roi.set_metadata_only(true);
          if (jpip_client->post_window(&tmp_roi,client_request_queue_id,
                                       true,&client_prefs))
            client_roi.copy_from(tmp_roi);
          return;
        }
    }
  
  // Begin by trying to find the location, size and resolution of the
  // view window.  We might have to guess at these parameters here if we
  // do not currently have an open image.
  int round_direction=0;
  kdu_dims res_dims, roi_dims, roi_region;
  
  if (enable_focus_box && !focus_box.is_empty())
    roi_region = focus_box;
  else if (rel_focus.is_valid() && !image_dims.is_empty())
    roi_region = rel_focus.region.apply_to_container(image_dims);
  else if (!view_dims.is_empty())
    roi_region = view_dims;
  if (!compositor->find_compatible_jpip_window(res_dims.size,roi_dims,
                                               round_direction,roi_region))
    { // Need to guess an appropriate resolution for the request
      if (single_component_idx >= 0)
        { // Use size of the codestream, if we can determine it
          if (!jpx_in.exists())
            {
              kdu_istream_ref istr; istr=compositor->get_next_istream(istr);
              if (istr.exists())
                {
                  compositor->access_codestream(istr).get_dims(-1,res_dims);
                  res_dims.pos = kdu_coords();
                }
            }
          else
            {
              jpx_codestream_source stream =
                jpx_in.access_codestream(single_codestream_idx);
              if (stream.exists())
                res_dims.size = stream.access_dimensions().get_size();
            }
        }
      else if (single_layer_idx >= 0)
        { // Use size of compositing layer, if we can evaluate it
          if (!jpx_in.exists())
            { 
              kdu_istream_ref istr; istr=compositor->get_next_istream(istr);
              if (istr.exists())
                {
                  compositor->access_codestream(istr).get_dims(-1,res_dims);
                  res_dims.pos = kdu_coords();
                }
            }
          else
            {
              jpx_layer_source layer = jpx_in.access_layer(single_layer_idx);
              if (layer.exists())
                res_dims.size = layer.get_layer_size();
            }
        }
      else
        { // Use size of composition surface, if we can find it
          if (composition_rules.exists())
            composition_rules.get_global_info(res_dims.size);
        }
      res_dims.pos = kdu_coords(0,0);
      res_dims.size.x = (int) ceil(res_dims.size.x * rendering_scale);
      res_dims.size.y = (int) ceil(res_dims.size.y * rendering_scale);
      
      if (rel_focus.is_valid())
        {
          res_dims.to_apparent(transpose,vflip,hflip);
          kdu_dims roi_dims = rel_focus.region.apply_to_container(res_dims);
          roi_dims.from_apparent(transpose,vflip,hflip);
          res_dims.from_apparent(transpose,vflip,hflip);
        }
      else
        roi_dims = res_dims;
    }

  if (roi_dims.is_empty())
    roi_dims = res_dims;
  roi_dims &= res_dims;
  
  // Now, we have as much information as we can guess concerning the
  // intended view window.  Let's use it to create a window request.
  roi_dims.pos -= res_dims.pos; // Make relative to current resolution
  tmp_roi.resolution = res_dims.size;
  tmp_roi.region = roi_dims;
  tmp_roi.round_direction = round_direction;
  
  if (configuration_complete && (max_display_layers < (1<<16)) &&
      (max_display_layers < compositor->get_max_available_quality_layers()))
    tmp_roi.max_layers = max_display_layers;
  
  if (single_component_idx >= 0)
    {
      tmp_roi.components.add(single_component_idx);
      tmp_roi.codestreams.add(single_codestream_idx);
    }
  else if (single_layer_idx >= 0)
    { // single compositing layer
      if ((single_layer_idx == 0) &&
          ((!jpx_in) || jpx_in.access_compatibility().is_jp2_compatible()))
        tmp_roi.codestreams.add(0); // Maximize chance of success even if
      // server cannot translate contexts
      kdu_sampled_range rg;
      rg.from = rg.to = single_layer_idx;
      rg.context_type = KDU_JPIP_CONTEXT_JPXL;
      tmp_roi.contexts.add(rg);
    }
  else if (frame != NULL)
    { // Request whole animation frame
      kdu_dims scaled_roi_dims;
      if (configuration_complete && (!pending_show_metanode) &&
          !(roi_dims.is_empty() || res_dims.is_empty()))
        {
          kdu_coords comp_size; composition_rules.get_global_info(comp_size);
          double scale_x = ((double) comp_size.x) / ((double) res_dims.size.x);
          double scale_y = ((double) comp_size.y) / ((double) res_dims.size.y);
          kdu_coords min = roi_dims.pos;
          kdu_coords lim = min + roi_dims.size;
          min.x = (int) floor(scale_x * min.x);
          lim.x = (int) ceil(scale_x * lim.x);
          min.y = (int) floor(scale_y * min.y);
          lim.y = (int) ceil(scale_y * lim.y);
          scaled_roi_dims.pos = min;
          scaled_roi_dims.size = lim - min;
        }
      kdu_sampled_range rg;
      frame_expander.construct(&jpx_in,frame,frame_iteration,true,
                               scaled_roi_dims);
      int m, num_members = frame_expander.get_num_members();
      for (m=0; m < num_members; m++)
        {
          int instruction_idx, layer_idx;
          bool covers_composition;
          kdu_dims source_dims, target_dims;
          jx_frame *frame =
          frame_expander.get_member(m,instruction_idx,layer_idx,
                                    covers_composition,source_dims,
                                    target_dims);
          rg.from = rg.to = layer_idx;
          rg.context_type = KDU_JPIP_CONTEXT_JPXL;
          if (covers_composition)
            {
              if ((layer_idx == 0) && (num_members == 1) &&
                  jpx_in.access_compatibility().is_jp2_compatible())
                tmp_roi.codestreams.add(0); // Maximize chance that server will
                      // respond correctly, even if it can't translate contexts
              rg.remapping_ids[0] = rg.remapping_ids[1] = -1;
            }
          else
            {
              composition_rules.get_original_iset(frame,instruction_idx,
                                                  rg.remapping_ids[0],
                                                  rg.remapping_ids[1]);
            }
          tmp_roi.contexts.add(rg);
        }
    }
  
  // Finish up by adding any image window dependent metadata requests
  if (jpx_in.exists())
    {
      tmp_roi.add_metareq(0,KDU_MRQ_WINDOW|KDU_MRQ_STREAM);
        // Requests all region/stream-specific metadata with low priority
      if (!catalog_source)
        { // Ask for the metadata relevant to the requested region & streams
          tmp_roi.add_metareq(jp2_label_4cc,KDU_MRQ_GLOBAL|KDU_MRQ_STREAM,
                              true,INT_MAX,false,0,1);
                // Requests all relevant labels in the top 2 levels of the
                // box hierarchy with high priority.
          tmp_roi.add_metareq(jp2_label_4cc,KDU_MRQ_GLOBAL|KDU_MRQ_STREAM,
                              false,INT_MAX,false,0,INT_MAX);
                // Requests all relevant labels with low priority.
        }
    }  
      
  // Post the window
  if (!enable_focus_box)
    client_prefs.set_pref(KDU_WINDOW_PREF_FULL);
  else
    client_prefs.set_pref(KDU_WINDOW_PREF_PROGRESSIVE);  
  if (jpip_client->post_window(&tmp_roi,client_request_queue_id,
                               true,&client_prefs))
    client_roi.copy_from(tmp_roi);
}

/*****************************************************************************/
/*                     kdws_renderer::change_client_focus                    */
/*****************************************************************************/

void
  kdws_renderer::change_client_focus(kdu_coords actual_resolution,
                                     kdu_dims actual_region)
{
  // Find the relative location and dimensions of the new focus box
  rel_focus.reset();
  rel_focus.region.set(actual_region,actual_resolution);
  if (transpose)
    rel_focus.region.transpose();
  if (image_dims.pos.x < 0)
    rel_focus.region.hflip();
  if (image_dims.pos.y < 0)
    rel_focus.region.vflip();
  if (single_component_idx >= 0)
    { rel_focus.codestream_idx = single_codestream_idx;
      rel_focus.codestream_region = rel_focus.region; }
  else if (jpx_in.exists() && (single_layer_idx >= 0))
    { rel_focus.layer_idx = single_layer_idx;
      rel_focus.layer_region = rel_focus.region; }
  else
    rel_focus.frame_idx = frame_idx;

  // Draw the focus box from its relative coordinates.
  enable_focus_box = true;
  bool posting_state = enable_region_posting;
  enable_region_posting = false;
  update_focus_box();
  enable_region_posting = posting_state;
}

/*****************************************************************************/
/*                        kdws_renderer::set_codestream                      */
/*****************************************************************************/

void
  kdws_renderer::set_codestream(int codestream_idx)
{
  calculate_rel_view_and_focus_params();
  single_component_idx = 0;
  single_codestream_idx = codestream_idx;
  single_layer_idx = -1;
  frame = NULL;
  adjust_rel_focus();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  invalidate_configuration();
  initialize_regions(false);
}

/*****************************************************************************/
/*                    kdws_renderer::set_compositing_layer                   */
/*****************************************************************************/

void
  kdws_renderer::set_compositing_layer(int layer_idx)
{
  calculate_rel_view_and_focus_params();
  single_component_idx = -1;
  single_layer_idx = layer_idx;
  frame = NULL;
  adjust_rel_focus();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  invalidate_configuration();
  initialize_regions(false);
}

/*****************************************************************************/
/*                    kdws_renderer::set_compositing_layer                   */
/*****************************************************************************/

void
  kdws_renderer::set_compositing_layer(kdu_coords point)
{
  if (compositor == NULL)
    return;
  point += view_dims.pos;
  kdu_ilayer_ref ilyr = compositor->find_point(point);
  int layer_idx, cs_idx; bool is_opaque;
  if ((compositor->get_ilayer_info(ilyr,layer_idx,cs_idx,is_opaque) > 0) &&
      (layer_idx >= 0))
    set_compositing_layer(layer_idx);
}

/*****************************************************************************/
/*                         kdws_renderer::set_frame                          */
/*****************************************************************************/

bool
  kdws_renderer::set_frame(int new_frame_idx)
{
  if ((compositor == NULL) ||
      (single_component_idx >= 0) ||
      ((single_layer_idx >= 0) && jpx_in.exists()))
    return false;
  if (new_frame_idx < 0)
    new_frame_idx = 0;
  if ((num_frames > 0) && (new_frame_idx >= num_frames))
    new_frame_idx = num_frames-1;
  int old_frame_idx = this->frame_idx;
  if (new_frame_idx == old_frame_idx)
    return false;
  change_frame_idx(new_frame_idx);
  if (frame_idx == old_frame_idx)
    return false;

  if ((buffer != NULL) && mj2_in.exists())
    {
      kdu_ilayer_ref ilyr=compositor->get_next_ilayer(kdu_ilayer_ref());
      compositor->change_ilayer_frame(ilyr,frame_idx);
      enable_focus_box = false;
    }
  else if ((buffer != NULL) && (frame != NULL) &&
           frame_expander.construct(&jpx_in,frame,frame_iteration,true) &&
           (frame_expander.get_num_members() > 0))
    { // Composit a new JPX frame
      if (enable_focus_box)
        calculate_rel_view_and_focus_params();
      compositor->set_frame(&frame_expander);
      int inactive_layers_to_cache = (in_playmode)?1:3;
      compositor->cull_inactive_ilayers(inactive_layers_to_cache);
      buffer = compositor->get_composition_bitmap(buffer_dims);
      if (enable_focus_box)
        {
          rel_focus.frame_idx = old_frame_idx;
          if (adjust_rel_focus() && enable_focus_box &&
              rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
            {
              view_centre_known = true;
              set_view_size(view_dims.size);
              view_centre_known = false;
            }
          rel_focus.reset();
        }
      if (jpip_client != NULL)
        update_client_window_of_interest();
      if (overlays_enabled && !in_playmode)
        {
          jpx_metanode node;
          if ((catalog_source != NULL) &&
              (node=catalog_source->get_selected_metanode()).exists())
            flash_imagery_for_metanode(node);
          else if (overlay_highlight_state || overlay_flashing_state)
            {
              overlay_highlight_state = 0;
              overlay_flashing_state = 1;
              configure_overlays();
            }
        }
    }
  else
    { // Have to initialize the frame composition from scratch
      calculate_rel_view_and_focus_params();
      rel_focus.frame_idx = old_frame_idx;
      invalidate_configuration();
      initialize_regions(false);
    }
  return true;
}

/*****************************************************************************/
/*                      kdws_renderer::stop_playmode                         */
/*****************************************************************************/

void kdws_renderer::stop_playmode()
{
  if (!in_playmode)
    return;
  if (status_id == KDS_STATUS_PLAYSTATS)
    {
      if (jpip_client == NULL)
        status_id = KDS_STATUS_LAYER_RES;
      else
        status_id = KDS_STATUS_CACHE;
    }
  next_frame_end_time = -1.0;
  frame_presenter->reset(); // Blocking call
  playmode_reverse = false;
  pushed_last_frame = false;
  in_playmode = false;
  assert(!view_mutex_locked);
  compositor->set_surface_initialization_mode(true);
  if (compositor != NULL)
    compositor->flush_composition_queue();
  playmode_frame_idx = -1;
  num_expired_frames_on_queue = 0;
  if (compositor != NULL)
    {
      buffer = compositor->get_composition_bitmap(buffer_dims);
      if (!compositor->is_processing_complete())
        compositor->refresh(); // Make sure the surface is properly initialized
    }
  display_status();
}

/*****************************************************************************/
/*                     kdws_renderer::start_playmode                         */
/*****************************************************************************/

void kdws_renderer::start_playmode(bool reverse)
{
  if (!source_supports_playmode())
    return;
  if ((single_component_idx >= 0) ||
      ((single_layer_idx >= 0) && jpx_in.exists()))
    return;
  
  compositor->flush_composition_queue();
  playmode_frame_idx = -1; // Becomes non-negative with 1st frame presentation
  num_expired_frames_on_queue = 0;

  // Make sure we have valid `frame_start' and `frame_end' times
  if (frame_end <= frame_start)
    change_frame_idx(frame_idx); // Generate new `frame_end' time

  // Start the clock running
  this->playmode_reverse = reverse;
  this->playclock_base = manager->get_current_time();
  this->playclock_offset = 0.0;
  this->playclock_offset = playclock_base - get_absolute_frame_start_time();
  pushed_last_frame = false;
  in_playmode = true;
  last_status_update_time = playclock_base;
  playmode_stats.reset();
  status_id = KDS_STATUS_PLAYSTATS;
  next_overlay_flash_time = -1.0;
  
  // Setting things up to decode from scratch means that the image dimensions
  // may possibly change.  Actually, I don't think this can happen unless
  // the `start_playmode' function is called while we are already in
  // playmode, since then we may have been displaying a different frame.  In
  // any case, it doesn't do any harm to re-initialize the buffer and viewport
  // on entering playback mode.
  calculate_rel_view_and_focus_params();
  view_dims = buffer_dims = kdu_dims();
  buffer = NULL;
  if (adjust_rel_focus())
    rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  image_view->set_max_view_size(image_dims.size,pixel_scale);
  view_centre_known = false;
  rel_focus.reset();
}

/*****************************************************************************/
/*                kdws_renderer::set_metadata_catalog_source                 */
/*****************************************************************************/

void kdws_renderer::set_metadata_catalog_source(kdws_catalog *source)
{
  if ((source == NULL) && (this->catalog_source != NULL))
    catalog_closed = true;
  if (this->catalog_source != source)
    {
      if (this->catalog_source != NULL)
        this->catalog_source->deactivate(); // Best to do this first
    }
  this->catalog_source = source;
  if ((source != NULL) && jpx_in.exists())
    {
      source->update_metadata();
      catalog_closed = false;
    }
}

/*****************************************************************************/
/*                 kdws_renderer::reveal_metadata_at_point                   */
/*****************************************************************************/

void kdws_renderer::reveal_metadata_at_point(kdu_coords point, bool dbl_click)
{
  if ((compositor == NULL) || (!jpx_in) ||
      ((catalog_source == NULL) && (metashow == NULL) && !dbl_click))
    return;
  point += view_dims.pos;

  kdu_ilayer_ref ilyr;
  kdu_istream_ref istr;
  jpx_metanode node;
  if (overlays_enabled)
    node = compositor->search_overlays(point,istr,0.1F);
  bool revealing_roi = node.exists();
  if (!revealing_roi)
    { 
      jpx_meta_manager meta_manager = jpx_in.access_meta_manager();
      int enumerator=0;
      while ((ilyr=compositor->find_point(point,enumerator,0.1F)).exists())
        { // Look for a numlist which is associated with as few other
          // compositing layers as possible and has a descendant in the
          // catalog.  There may be more than one such numlist, so we will
          // look for the one which is related most closely to the
          // most recently selected entry in the catalog, if any.
          int layer_idx, dcs_idx; bool opq;
          if ((compositor->get_ilayer_info(ilyr,layer_idx,dcs_idx,opq) > 0) &&
              (layer_idx >= 0))
            { 
              jpx_metanode scan, nd;
              int dist, min_dist=INT_MAX, min_layers=INT_MAX;
              while ((scan =
                      meta_manager.enumerate_matches(scan,-1,layer_idx,
                                                false,kdu_dims(),0)).exists())
                { 
                  int num_cs, num_layers; bool rres;
                  if (!scan.get_numlist_info(num_cs,num_layers,rres))
                    continue;
                  if (catalog_source != NULL)
                    nd=catalog_source->find_best_catalogued_descendant(scan,
                                                                       dist,
                                                                       true);
                  else
                    { nd=scan; dist = INT_MAX; }
                  if (nd.exists() &&
                      ((!node) || (num_layers < min_layers) ||
                       ((num_layers == min_layers) && (dist < min_dist))))
                    { node = nd; min_layers = num_layers; min_dist = dist; }
                }
            }
          enumerator++;
        }
    }
  if (node.exists())
    {
      if (metashow != NULL)
        metashow->select_matching_metanode(node);
      if (catalog_source != NULL)
        catalog_source->select_matching_metanode(node);
      if (dbl_click)
        {
          if (!revealing_roi)
            last_show_metanode = node; // So we immediately advance to next
                                       // associated image, if any.
          show_imagery_for_metanode(node);
        }
    }
}

/*****************************************************************************/
/*                  kdws_renderer::edit_metadata_at_point                    */
/*****************************************************************************/

void kdws_renderer::edit_metadata_at_point(kdu_coords *point_ref)
{
  if ((compositor == NULL) || !jpx_in)
    return;
  
  kdu_coords point; // Will hold mouse location in `view_dims' coordinates
  if (point_ref != NULL)
    point = *point_ref;
  else
    point = image_view->get_last_mouse_point();
  point += view_dims.pos;
  
  kdu_ilayer_ref ilyr;
  kdu_istream_ref istr;
  jpx_meta_manager manager = jpx_in.access_meta_manager();
  jpx_metanode roi_node;
  if (overlays_enabled)
    roi_node = compositor->search_overlays(point,istr,0.1F);
  kdws_matching_metalist *metalist = new kdws_matching_metalist;
  if (roi_node.exists())
    metalist->append_node_expanding_numlists_and_free_boxes(roi_node);
  else
    {
      int enumerator=0;
      while ((ilyr=compositor->find_point(point,enumerator,0.1F)).exists())
        { 
          int layer_idx=-1, stream_idx; bool opq;
          compositor->get_ilayer_info(ilyr,layer_idx,stream_idx,opq);
          if ((istr=compositor->get_ilayer_stream(ilyr,0)).exists())
            compositor->get_istream_info(istr,stream_idx);
          manager.load_matches((stream_idx >= 0)?1:0,&stream_idx,
                               (layer_idx >= 0)?1:0,&layer_idx);
          metalist->append_image_wide_nodes(manager,layer_idx,stream_idx);
          if (metalist->get_head() != NULL)
            break;
          enumerator++;
        }
    }
  if (metalist->get_head() == NULL)
    { // Nothing to edit; user needs to add metadata
      delete metalist;
      return;
    }
  if (file_server == NULL)
    file_server = new kdws_file_services(open_file_pathname);
  if (editor == NULL)
    {
      RECT wnd_frame;  window->GetWindowRect(&wnd_frame);
      POINT preferred_location;
      preferred_location.x = wnd_frame.right + 10;
      preferred_location.y = wnd_frame.top;
      editor = new kdws_metadata_editor(&jpx_in,file_server,true,
                                        this,preferred_location,
                                        window_identifier,NULL);
  }
  editor->configure(metalist);
}

/*****************************************************************************/
/*                      kdws_renderer::edit_metanode                         */
/*****************************************************************************/

void kdws_renderer::edit_metanode(jpx_metanode node,
                                  bool include_roi_or_numlist)
{
  if ((compositor == NULL) || (!jpx_in) || (!node) || node.is_deleted())
    return;
  kdws_matching_metalist *metalist = new kdws_matching_metalist;
  metalist->append_node_expanding_numlists_and_free_boxes(node);
  if (metalist->get_head() == NULL)
    { // Nothing to edit; user needs to add metadata
      delete metalist;
      return;
    }
  jpx_metanode editor_startup_node = metalist->get_head()->node;
  if (include_roi_or_numlist && (editor_startup_node == node))
    { // See if we want to build the metalist with a different root
      jpx_metanode edit_root = node;
      jpx_metanode scan, last_scan=node;
      for (scan=node.get_parent(); scan.exists();
           last_scan=scan, scan=scan.get_parent())
        {
          bool rres;
          int num_cs, num_l;
          if (scan.get_numlist_info(num_cs,num_l,rres))
            {
              edit_root = last_scan;
              break;
            }
          else if (scan.get_num_regions() > 0)
            {
              edit_root = scan;
              break;
            }
        }
      if (edit_root != node)
        {
          delete metalist;
          metalist = new kdws_matching_metalist;
          metalist->append_node_expanding_numlists_and_free_boxes(edit_root);
        }
    }
  
  if (file_server == NULL)
    file_server = new kdws_file_services(open_file_pathname);
  if (editor == NULL)
    {
      RECT wnd_frame;  window->GetWindowRect(&wnd_frame);
      POINT preferred_location;
      preferred_location.x = wnd_frame.right + 10;
      preferred_location.y = wnd_frame.top;
      editor = new kdws_metadata_editor(&jpx_in,file_server,true,
                                        this,preferred_location,
                                        window_identifier,NULL);
  }
  editor->configure(metalist);
  editor->setActiveNode(editor_startup_node);
}

/*****************************************************************************/
/*                kdws_renderer::flash_imagery_for_metanode                  */
/*****************************************************************************/

bool
  kdws_renderer::flash_imagery_for_metanode(jpx_metanode node)
{
  if (!overlays_enabled)
    return false;
  
  if (node.exists())
    { 
      overlay_highlight_state = 1;
      highlight_metanode = node;
      configure_overlays();
      int total_roi_nodes=0, hidden_roi_nodes=0;
      compositor->get_overlay_info(total_roi_nodes,hidden_roi_nodes);
      if (total_roi_nodes > hidden_roi_nodes)
        { // Something associated with `node' is visible
          jpx_metanode roi=node;
          while (roi.exists() && (roi.get_regions() == NULL))
            roi = roi.get_parent();
          if (roi.exists())
            return true; // Highlighting a single ROI node
          if (hidden_roi_nodes > 0)
            return true; // At least something is hidden
        }
    }
  if (overlay_highlight_state)
    {
      overlay_highlight_state = 0;
      configure_overlays();
    }
  return true;
}

/*****************************************************************************/
/*                kdws_renderer::show_imagery_for_metanode                   */
/*****************************************************************************/

bool kdws_renderer::show_imagery_for_metanode(jpx_metanode node)
{
  bool advance_to_next = (node==last_show_metanode) && !pending_show_metanode;
  last_show_metanode = node; // Assume success until proven otherwise
  pending_show_metanode = node;
  if ((compositor == NULL) || (!jpx_in) || (!node) || !configuration_complete)
    {
      cancel_pending_show();
      return false;
    }
  
  bool rendered_result=false;
  int num_streams=0, num_layers=0, num_regions=0;
  jpx_metanode numlist, roi;
  for (; node.exists() && !(numlist.exists() && roi.exists());
       node=node.get_parent())
    if ((!numlist) &&
        node.get_numlist_info(num_streams,num_layers,rendered_result))
      numlist = node;
    else if ((!roi) && ((num_regions = node.get_num_regions()) > 0))
      roi = node;

  const int *layers = NULL;
  const int *streams = NULL;
  if (numlist.exists())
    {
      streams = numlist.get_numlist_codestreams();
      layers = numlist.get_numlist_layers();
    }
  else if (!roi)
    {
      cancel_pending_show();
      return false; // Nothing to do
    }
  bool can_cancel_pending_show = true;
  
  bool have_unassociated_roi = roi.exists() && (streams == NULL);
  if (have_unassociated_roi)
    { // An ROI must be associated with one or more codestreams, or else it
      // must be associated with nothing other than the rendered result (an
      // unassociated ROI).  This last case is probably rare, but we
      // accommodate it here anyway, by enforcing the lack of association
      // with anything other than the rendered result.
      num_layers = num_streams = 0;
      layers = NULL;
    }
  
  if (!composition_rules)
    composition_rules = jpx_in.access_composition();
  bool match_any_frame = false;
  bool match_any_layer = false;
  if (rendered_result || have_unassociated_roi)
    {
      if (!composition_rules)
        { can_cancel_pending_show = false; match_any_layer = true; }
      else if (composition_rules.get_next_frame(NULL) == NULL)
        match_any_layer = true;
      else
        match_any_frame = true;
    }

  bool have_compatible_layer = false;
  int compatible_codestream=-1; // This is the codestream against which
                                // we will assess the `roi' if there is one
  int compatible_codestream_layer=-1; // This is the compositing layer against
                                // which we should assess the `roi' if any.
  
  int m, n, c, p;
  if (streams == NULL)
    { // Compatible imagery consists either of a rendered result or a
      // single compositing layer only.  This may require us to display
      // a more complex object than is currently being rendered.  Also,
      // because `num_streams' is 0, we only need to match image entitites,
      // not ROI's.
      if ((num_layers == 0) && match_any_frame)
        { // Only a composited frame will be compatible with the metadata
          if (!composition_rules)
            { // Waiting for more data from the cache before navigating
              return false;
            }
          if ((frame == NULL) || (!advance_to_next) || !set_frame(frame_idx+1))
            set_frame(0);
          if ((!configuration_complete) ||
              compositor->waiting_for_stream_headers())
            can_cancel_pending_show = false;
          if (!have_unassociated_roi)
            {
              if (can_cancel_pending_show)
                cancel_pending_show();
              return can_cancel_pending_show;
            }
        }
      else if ((single_component_idx >= 0) && match_any_layer)
        {
          set_compositing_layer(0);
          if ((!configuration_complete) ||
              compositor->waiting_for_stream_headers())
            can_cancel_pending_show = false;
          if (!have_unassociated_roi)
            {
              if (can_cancel_pending_show)
                cancel_pending_show();
              return can_cancel_pending_show;
            }
        }
      else if ((single_component_idx >= 0) && (num_layers > 0))
        { // Need to display a compositing layer.  If possible, display one
          // which contains the currently displayed codestream.
          int layer_idx = -1;
          for (m=0; (m < num_layers) && (layer_idx < 0); m++)
            for (n=0; (c=jpx_in.get_layer_codestream_id(layers[m],n))>=0; n++)
              if (c == single_codestream_idx)
                { layer_idx = layers[m]; break; }
          set_compositing_layer((layer_idx >= 0)?layer_idx:layers[0]);
          if ((!configuration_complete) ||
              compositor->waiting_for_stream_headers())
            can_cancel_pending_show = false;
          if (can_cancel_pending_show)
            cancel_pending_show();
          return can_cancel_pending_show;
        }
    }
  
  if (have_unassociated_roi)
    have_compatible_layer = true; // We have already bumped things up as req'd.
  const jpx_roi *regions = NULL;
  if (roi.exists())
    regions = roi.get_regions();
  
  // Now see what changes are required to match the image entities
  if ((frame != NULL) && !have_compatible_layer)
    { // We are displaying a frame.  Let's try to keep doing that.
      if (match_any_frame && !roi)
        { // Any frame will do, but we may need to advance
          if (advance_to_next && !set_frame(frame_idx+1))
            set_frame(0);
          if ((!configuration_complete) ||
              compositor->waiting_for_stream_headers())
            can_cancel_pending_show = false;
          if (can_cancel_pending_show)
            cancel_pending_show();
          return can_cancel_pending_show;
        }
      
      // Look for the most appropriate matching frame
      int compatible_frame_idx = -1;
      if ((num_regions == 0) && (num_streams == 0) && (num_layers > 1) &&
          (num_layers <= 1024))
        compatible_frame_idx =
          find_compatible_frame_idx(num_streams,streams,num_layers,layers,
                                    num_regions,regions,compatible_codestream,
                                    compatible_codestream_layer,true,
                                    advance_to_next);
      if (compatible_frame_idx < 0)
        compatible_frame_idx =
          find_compatible_frame_idx(num_streams,streams,num_layers,layers,
                                    num_regions,regions,compatible_codestream,
                                    compatible_codestream_layer,false,
                                    advance_to_next);
      if (compatible_frame_idx >= 0)
        {
          have_compatible_layer = true;
          if (compatible_frame_idx != frame_idx)
            {
              set_frame(compatible_frame_idx);
              if ((!configuration_complete) ||
                  compositor->waiting_for_stream_headers())
                can_cancel_pending_show = false;
            }
        }
    }

  if ((single_layer_idx >= 0) && (!have_compatible_layer) && !advance_to_next)
    { // We are already displaying a single compositing layer; try to keep
      // doing this.
      if (match_any_layer)
        have_compatible_layer = true;
      else
        for (n=0; n < num_layers; n++)
          if (layers[n] == single_layer_idx)
            { have_compatible_layer = true; break; }      
      if (have_compatible_layer || (num_layers == 0))
        for (n=0;
             ((c=jpx_in.get_layer_codestream_id(single_layer_idx,n)) >= 0) &&
             (compatible_codestream < 0); n++)
          for (p=0; p < num_streams; p++)
            if (c == streams[p])
              { compatible_codestream=c; have_compatible_layer=true; break; }
      if (roi.exists() && (compatible_codestream < 0))
        have_compatible_layer = false;
    }
  
  int count_layers=1;
  bool all_layers_available =
  jpx_in.count_compositing_layers(count_layers);

  if ((single_layer_idx >= 0) && (!have_compatible_layer) && match_any_layer)
    { // Advance to next available compositing layer
      assert(advance_to_next);
      int new_layer_idx = single_layer_idx+1;
      if (new_layer_idx >= count_layers)
        new_layer_idx = 0;
      if (new_layer_idx != single_layer_idx)
        set_compositing_layer(new_layer_idx);
      have_compatible_layer = true;
      if ((!configuration_complete) ||
          compositor->waiting_for_stream_headers())
        can_cancel_pending_show = false;
    }
  
  if ((single_component_idx < 0) && !have_compatible_layer)
    { // See if we can change to any compositing layer which is compatible
      int layer_idx = -1;
      int first_layer_idx = -1;
      int closest_following_layer_idx = -1;
      for (m=0; (m < num_layers); m++)
        {
          layer_idx = layers[m];
          for (n=0; ((c=jpx_in.get_layer_codestream_id(layer_idx,n)) >= 0) &&
                    (compatible_codestream < 0); n++)
            for (p=0; p < num_streams; p++)
              if (c == streams[p])
                { compatible_codestream=c; break; }
          if ((compatible_codestream >= 0) || !roi)
            { // This layer is compatible
              have_compatible_layer = true;
              if ((first_layer_idx < 0) || (layer_idx < first_layer_idx))
                first_layer_idx = layer_idx;
              if (!advance_to_next)
                break;
              if ((layer_idx > single_layer_idx) &&
                  ((closest_following_layer_idx < 0) ||
                   (closest_following_layer_idx > layer_idx)))
                closest_following_layer_idx = layer_idx;
            }
        }
      if (num_layers == 0)
        {
          for (m=0; (m < count_layers) && !have_compatible_layer; m++)
            {
              layer_idx = m;
              for (n=0; ((c=jpx_in.get_layer_codestream_id(layer_idx,n))>=0) &&
                        (compatible_codestream < 0); n++)
                for (p=0; p < num_streams; p++)
                  if (c == streams[p])
                    { compatible_codestream=c; break; }
              if ((compatible_codestream >= 0) ||
                  (match_any_layer && !roi))
                {
                  have_compatible_layer = true;
                  if ((first_layer_idx < 0) || (layer_idx < first_layer_idx))
                    first_layer_idx = layer_idx;
                  if (!advance_to_next)
                    break;
                  if ((layer_idx > single_layer_idx) &&
                      ((closest_following_layer_idx < 0) ||
                       (closest_following_layer_idx > layer_idx)))
                    closest_following_layer_idx = layer_idx;                  
                }
            }
        }
      if (have_compatible_layer)
        {
          if (closest_following_layer_idx >= 0)
            layer_idx = closest_following_layer_idx;
          else
            layer_idx = first_layer_idx;
          set_compositing_layer(layer_idx);
        }
      else if (!all_layers_available)
        { // Come back later once we have more information in the cache,
          // rather than downgrading now to a lower form of rendered object,
          // which is what happens if we continue.
          return false;
        }
    }
  
  if (!have_compatible_layer)
    { // See if we can display a single codestream which is compatible; it
      // is possible that we are already displaying a single compatible
      // codestream.
      if (!advance_to_next)
        for (n=0; n < num_streams; n++)
          if (streams[n] == single_codestream_idx)
            { compatible_codestream = single_codestream_idx; break; }
      if ((compatible_codestream < 0) && (num_streams > 0))
        {
          int first_codestream_idx = -1;
          int closest_following_codestream_idx=-1;
          for (n=0; n < num_streams; n++)
            {
              if ((first_codestream_idx < 0) ||
                  (streams[n] < first_codestream_idx))
                first_codestream_idx = streams[n];
              if (!advance_to_next)
                break;
              if ((streams[n] > single_codestream_idx) &&
                  ((closest_following_codestream_idx < 0) ||
                   (closest_following_codestream_idx > streams[n])))
                closest_following_codestream_idx = streams[n];
            }
          if (closest_following_codestream_idx >= 0)
            compatible_codestream = closest_following_codestream_idx;
          else
            compatible_codestream = first_codestream_idx;
        }
      if (compatible_codestream >= 0)
        set_codestream(compatible_codestream);
    }

  if ((!configuration_complete) ||
      compositor->waiting_for_stream_headers())
    can_cancel_pending_show = false;

  kdu_dims roi_region;
  kdu_dims mapped_roi_region;
  if (have_compatible_layer && configuration_complete && (num_layers == 1) &&
      (frame != NULL) && (!roi) &&
      (compositor->get_num_ilayers() > 1))
    { // See if we should create a fake region of interest, to identify
      // a compositing layer within its frame.
      kdu_ilayer_ref ilyr; ilyr=compositor->get_next_ilayer(ilyr,layers[0]);
      if (ilyr.exists())
        mapped_roi_region = compositor->find_ilayer_region(ilyr,true);
      mapped_roi_region &= image_dims;
      if (mapped_roi_region.area() == image_dims.area())
        mapped_roi_region = kdu_dims(); // No need to highlight the layer
      else if (enable_focus_box && (focus_box.area() >= 100) &&
               (focus_box == (mapped_roi_region & focus_box)))
        mapped_roi_region = kdu_dims();      
    }
  if ((!mapped_roi_region) &&
      ((!roi) || (!configuration_complete) ||
       ((compatible_codestream < 0) && !have_unassociated_roi)))
    { // Nothing we can do or perhaps need to do.
      if (can_cancel_pending_show)
        cancel_pending_show();
      return can_cancel_pending_show;
    }
  
  // Now we need to find the region of interest on the compatible codestream
  // (or the rendered result if we have an unassociated ROI).
  kdu_istream_ref roi_istr;
  if (roi.exists())
    {
      roi_region = roi.get_bounding_box();
      kdu_ilayer_ref ilyr;
      ilyr = compositor->get_next_ilayer(ilyr,compatible_codestream_layer,-1);
      roi_istr = compositor->get_ilayer_stream(ilyr,-1,compatible_codestream);
      mapped_roi_region = compositor->inverse_map_region(roi_region,roi_istr);
    }
  mapped_roi_region &= image_dims;
  if (!mapped_roi_region)
    { // Region does not appear to intersect with the image surface.
      if (can_cancel_pending_show)
        cancel_pending_show();
      return can_cancel_pending_show;
    }
  while (roi_istr.exists() &&
         ((mapped_roi_region.size.x > view_dims.size.x) ||
          (mapped_roi_region.size.y > view_dims.size.y)))
    {
      if (rendering_scale == min_rendering_scale)
        break;
      rendering_scale = decrease_scale(rendering_scale,false);
      calculate_rel_view_and_focus_params();
      initialize_regions(true);
      mapped_roi_region = compositor->inverse_map_region(roi_region,roi_istr);
    }
  while (roi_istr.exists() &&
         (mapped_roi_region.size.x < (view_dims.size.x >> 1)) &&
         (mapped_roi_region.size.y < (view_dims.size.y >> 1)) &&
         (mapped_roi_region.area() < 400))
    {
      if (rendering_scale == max_rendering_scale)
        break;
      rendering_scale = increase_scale(rendering_scale,false);
      calculate_rel_view_and_focus_params();
      initialize_regions(true);
      mapped_roi_region = compositor->inverse_map_region(roi_region,roi_istr);
    }
  if ((mapped_roi_region & view_dims) != mapped_roi_region)
    { // The ROI region is not completely visible; centre the view over the
      // region then.
      rel_focus.region.set(mapped_roi_region,image_dims);
      view_centre_x = rel_focus.region.centre_x;
      view_centre_y = rel_focus.region.centre_y;
      view_centre_known = true;
      rel_focus.reset();
      scroll_to_view_anchors();
    }
  mapped_roi_region.pos -= view_dims.pos;
  set_focus_box(mapped_roi_region);
  
  if (can_cancel_pending_show)
    cancel_pending_show();
  return can_cancel_pending_show;
}

/*****************************************************************************/
/*                    kdws_renderer::cancel_pending_show                     */
/*****************************************************************************/

void kdws_renderer::cancel_pending_show()
{
  pending_show_metanode = jpx_metanode();
}

/*****************************************************************************/
/*                     kdws_renderer::metadata_changed                       */
/*****************************************************************************/

void kdws_renderer::metadata_changed(bool new_data_only)
{
  if (compositor == NULL)
    return;
  have_unsaved_edits = true;
  if (overlay_restriction.exists() && overlay_restriction.is_deleted())
    {
      overlay_restriction = last_selected_overlay_restriction = jpx_metanode();
      configure_overlays();
    }
  compositor->update_overlays(!new_data_only);
  if (metashow != NULL)
    metashow->update_metadata();
  if ((catalog_source == NULL) && jpx_in.exists() &&
      jpx_in.access_meta_manager().peek_touched_nodes(jp2_label_4cc).exists())
    window->create_metadata_catalog();
  else if (catalog_source != NULL)
    catalog_source->update_metadata();
}

/*****************************************************************************/
/*                         kdws_renderer::save_over                          */
/*****************************************************************************/

bool kdws_renderer::save_over()
{
  const char *save_pathname =
    manager->get_save_file_pathname(open_file_pathname);
  const char *suffix = strrchr(open_filename,'.');
  bool result = true;
  BeginWaitCursor();
  try { // Protect the entire file writing process against exceptions
    if ((suffix != NULL) &&
        (toupper(suffix[1]) == 'J') && (toupper(suffix[2]) == 'P') &&
        ((toupper(suffix[3]) == 'X') || (toupper(suffix[3]) == 'F') ||
         (suffix[3] == '2')))
      { // Generate a JP2 or JPX file.
        if (!(jpx_in.exists() || mj2_in.exists()))
          { kdu_error e; e << "Cannot write JP2-family file if input data "
            "source is a raw codestream.  Too much information must "
            "be invented.  Probably the file you opened had a \".jp2\", "
            "\".jpx\" or \".jpf\" suffix but was actually a raw codestream "
            "not embedded within any JP2 family file structure.  Use "
            "the \"Save As\" option to save it as a codestream again if "
            "you like."; }
        if ((suffix[3] == '2') || !jpx_in)
          save_as_jp2(save_pathname);
        else
          {
            save_as_jpx(save_pathname,true,false);
            have_unsaved_edits = false;
          }
      }
    else
      save_as_raw(save_pathname);
    
    if (last_save_pathname != NULL)
      delete[] last_save_pathname;
    last_save_pathname = new char[strlen(open_file_pathname)+1];
    strcpy(last_save_pathname,open_file_pathname);
  }
  catch (int)
  {
    manager->declare_save_file_invalid(save_pathname);
    result = false;
  }
  EndWaitCursor();
  return result;
}

/*****************************************************************************/
/*                         kdws_renderer::save_as                            */
/*****************************************************************************/

void
  kdws_renderer::save_as(bool preserve_codestream_links,
                        bool force_codestream_links)
{
  if ((compositor == NULL) || in_playmode)
    return;

  // Get the file name
  OPENFILENAME ofn;
  memset(&ofn,0,sizeof(ofn)); ofn.lStructSize = sizeof(ofn);
  kdws_settings *settings = manager->access_persistent_settings();
  kdws_string initial_dir(MAX_PATH);
  strcpy(initial_dir,settings->get_open_save_dir());
  if (initial_dir.is_empty())
    GetCurrentDirectory(MAX_PATH,initial_dir);

  kdws_string filename(MAX_PATH+4);
  if ((last_save_pathname != NULL) &&
      (strlen(last_save_pathname) < MAX_PATH))
    strcpy(filename,last_save_pathname);
  else if ((open_file_pathname != NULL) &&
           (strlen(open_file_pathname) < MAX_PATH))
    strcpy(filename,open_file_pathname);
  if (mj2_in.exists())
    {
      char *suffix = strrchr((char *) filename,'.');
      if (suffix == NULL)
        filename.clear();
      else
        strcpy(suffix,".jp2");
    }
  
  int num_choices = 1;

  ofn.hwndOwner = window->GetSafeHwnd();
  if (jpx_in.exists())
    {
      ofn.lpstrFilter =
        _T("JP2 compatible file (*.jp2, *.jpx, *.jpf)\0*.jp2;*.jpx;*.jpf\0")
        _T("JPX file (*.jpx, *.jpf)\0*.jpx;*.jpf\0")
        _T("JPEG2000 unwrapped code-stream (*.j2c, *.j2k)\0*.j2c;*.j2k\0\0");
      ofn.lpstrDefExt = _T("jpf");
      num_choices = 3;
    }
  else if (mj2_in.exists())
    {
      ofn.lpstrFilter =
        _T("JP2 file (*.jp2)\0*.jp2\0")
        _T("JPEG2000 unwrapped code-stream (*.j2c, *.j2k)\0*.j2c;*.j2k\0\0");
      ofn.lpstrDefExt = _T("jp2");
      num_choices = 2;
    }
  else
    {
      ofn.lpstrFilter =
        _T("JPEG2000 unwrapped code-stream (*.j2c, *.j2k)\0*.j2c;*.j2k\0\0");
      ofn.lpstrDefExt = _T("j2c");
      num_choices = 1;
    }
  ofn.nFilterIndex = settings->get_save_idx();
  if ((ofn.nFilterIndex < 1) || ((int) ofn.nFilterIndex > num_choices))
    ofn.nFilterIndex = 1;
  ofn.lpstrFile = filename;
  ofn.nMaxFile = MAX_PATH;
  
  ofn.lpstrInitialDir = initial_dir;
  ofn.lpstrTitle = _T("Save as JPEG2000 File");
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
  if (!GetSaveFileName(&ofn))
    return;

  // Adjust `settings' based on the outcome before we do anything else
  char *fname_string = filename;
  if ((ofn.nFileOffset > 0) &&
      (fname_string[ofn.nFileOffset-1] == '/') ||
      (fname_string[ofn.nFileOffset-1] == '\\'))
    ofn.nFileOffset--;
  char sep = fname_string[ofn.nFileOffset];
  fname_string[ofn.nFileOffset] = '\0';
  settings->set_open_save_dir(fname_string);
  settings->set_save_idx(ofn.nFilterIndex);
  fname_string[ofn.nFileOffset] = sep;
  
  // Remember the last saved file name
  const char *chosen_pathname = filename;
  const char *suffix = strrchr(chosen_pathname,'.');
  bool jp2_suffix = ((suffix != NULL) &&
                     (toupper(suffix[1]) == 'J') &&
                     (toupper(suffix[2]) == 'P') &&
                     (suffix[3] == '2'));
  bool jpx_suffix = ((suffix != NULL) &&
                     (toupper(suffix[1]) == 'J') &&
                     (toupper(suffix[2]) == 'P') &&
                     ((toupper(suffix[3]) == 'X') ||
                      (toupper(suffix[3]) == 'F')));
  
  // Now save the file
  const char *save_pathname = NULL;
  BeginWaitCursor();
  try { // Protect the entire file writing process against exceptions
    bool may_overwrite_link_source = false;
    if (force_codestream_links && jpx_suffix && (open_file_pathname != NULL))
      if (kdws_compare_file_pathnames(chosen_pathname,open_file_pathname))
        may_overwrite_link_source = true;
    if (jpx_in.exists() && !jp2_family_in.uses_cache())
      {
        jp2_data_references data_refs;
        if (jpx_in.exists())
          data_refs = jpx_in.access_data_references();
        if (data_refs.exists())
          {
            int u;
            const char *url_name;
            for (u=0; (url_name = data_refs.get_url(u)) != NULL; u++)
              if (kdws_compare_file_pathnames(chosen_pathname,url_name))
                may_overwrite_link_source = true;
          }
      }
    if (may_overwrite_link_source)
      { kdu_error e; e << "You are about to overwrite an existing file "
        "which is being linked by (or potentially about to be linked by) "
        "the file you are saving.  This operation is too dangerous to "
        "attempt."; }
    save_pathname = manager->get_save_file_pathname(chosen_pathname);
    if (jpx_suffix || jp2_suffix)
      { // Generate a JP2 or JPX file.
        if (!(jpx_in.exists() || mj2_in.exists()))
          { kdu_error e; e << "Cannot write JP2-family file if input data "
            "source is a raw codestream.  Too much information must "
            "be invented."; }
        if (jp2_suffix || !jpx_in)
          save_as_jp2(save_pathname);
        else
          {
            save_as_jpx(save_pathname,preserve_codestream_links,
                        force_codestream_links);
            have_unsaved_edits = false;
          }
      }
    else
      save_as_raw(save_pathname);
    
    if (last_save_pathname != NULL)
      delete[] last_save_pathname;
    last_save_pathname = new char[strlen(chosen_pathname)+1];
    strcpy(last_save_pathname,chosen_pathname);
  }
  catch (int)
  {
    if (save_pathname != NULL)
      manager->declare_save_file_invalid(save_pathname);
  }
  EndWaitCursor();
}

/*****************************************************************************/
/*                        kdws_renderer::save_as_jp2                         */
/*****************************************************************************/

void
  kdws_renderer::save_as_jp2(const char *filename)
{
  jp2_family_tgt tgt; 
  jp2_target jp2_out;
  jp2_input_box stream_box;
  int jpx_layer_idx=-1, jpx_stream_idx=-1;
  
  if (jpx_in.exists())
    {
      jpx_layer_idx = 0;
      if (single_layer_idx >= 0)
        jpx_layer_idx = single_layer_idx;
      
      jpx_layer_source layer_in = jpx_in.access_layer(jpx_layer_idx);
      if (!(layer_in.exists() && (layer_in.get_num_codestreams() == 1)))
        { kdu_error e;
          e << "Cannot write JP2 file based on the currently active "
          "compositing layer.  The layer either is not yet available, or "
          "else it uses multiple codestreams.  Try saving as a JPX file.";
        }
      jpx_stream_idx = layer_in.get_codestream_id(0);
      jpx_codestream_source stream_in=jpx_in.access_codestream(jpx_stream_idx);
      
      tgt.open(filename);
      jp2_out.open(&tgt);
      jp2_out.access_dimensions().copy(stream_in.access_dimensions(true));
      jp2_out.access_colour().copy(layer_in.access_colour(0));
      jp2_out.access_palette().copy(stream_in.access_palette());
      jp2_out.access_resolution().copy(layer_in.access_resolution());
      
      jp2_channels channels_in = layer_in.access_channels();
      jp2_channels channels_out = jp2_out.access_channels();
      int n, num_colours = channels_in.get_num_colours();
      channels_out.init(num_colours);
      for (n=0; n < num_colours; n++)
        { // Copy channel information, converting only the codestream ID
          int comp_idx, lut_idx, stream_idx;
          channels_in.get_colour_mapping(n,comp_idx,lut_idx,stream_idx);
          channels_out.set_colour_mapping(n,comp_idx,lut_idx,0);
          if (channels_in.get_opacity_mapping(n,comp_idx,lut_idx,stream_idx))
            channels_out.set_opacity_mapping(n,comp_idx,lut_idx,0);
          if (channels_in.get_premult_mapping(n,comp_idx,lut_idx,stream_idx))
            channels_out.set_premult_mapping(n,comp_idx,lut_idx,0);
        }
      
      jp2_locator stream_loc = stream_in.get_stream_loc();
      stream_box.open(&jp2_family_in,stream_loc);
    }
  else
    {
      assert(mj2_in.exists());
      int frm, fld;
      mj2_video_source *track = NULL;
      if (single_component_idx >= 0)
        { // Find layer based on the track to which single component belongs
          kdu_uint32 trk;
          if (mj2_in.find_stream(single_codestream_idx,trk,frm,fld) &&
              (trk > 0))
            track = mj2_in.access_video_track(trk);
        }
      else
        {
          assert(single_layer_idx >= 0);
          track = mj2_in.access_video_track((kdu_uint32)(single_layer_idx+1));
          frm = frame_idx;
          fld = 0;
        }
      if (track == NULL)
        { kdu_error e; e << "Insufficient information available to open "
          "current video track.  Perhaps insufficient information has been "
          "received so far during a JPIP browsing session.";
        }
      
      tgt.open(filename);
      jp2_out.open(&tgt);
      jp2_out.access_dimensions().copy(track->access_dimensions());
      jp2_out.access_colour().copy(track->access_colour());
      jp2_out.access_palette().copy(track->access_palette());
      jp2_out.access_resolution().copy(track->access_resolution());
      jp2_out.access_channels().copy(track->access_channels());
      
      track->seek_to_frame(frm);
      track->open_stream(fld,&stream_box);
      if (!stream_box)
        { kdu_error e; e << "Insufficient information available to open "
          "relevant video frame.  Perhaps insufficient information has been "
          "received so far during a JPIP browsing session.";
        }
    }
  
  jp2_out.write_header();
  
  if (jpx_in.exists())
    { // See if we can copy some metadata across
      jpx_meta_manager meta_manager = jpx_in.access_meta_manager();
      jpx_metanode scan;
      while ((scan=meta_manager.enumerate_matches(scan,-1,jpx_layer_idx,false,
                                                  kdu_dims(),0,true)).exists())
        write_metanode_to_jp2(scan,jp2_out,file_server);
      while ((scan=meta_manager.enumerate_matches(scan,jpx_stream_idx,-1,false,
                                                  kdu_dims(),0,true)).exists())
        write_metanode_to_jp2(scan,jp2_out,file_server);
      if (!jpx_in.access_composition())
        { // In this case, rendered result same as composition layer
          while ((scan=meta_manager.enumerate_matches(scan,-1,-1,true,
                                                      kdu_dims(),0,
                                                      true)).exists())
            write_metanode_to_jp2(scan,jp2_out,file_server);
        }
      while ((scan=meta_manager.enumerate_matches(scan,-1,-1,false,
                                                  kdu_dims(),0,true)).exists())
        write_metanode_to_jp2(scan,jp2_out,file_server);
    }
  
  jp2_out.open_codestream();
  
  if (jp2_family_in.uses_cache())
    { // Need to generate output codestreams from scratch -- transcoding
      kdu_codestream cs_in;
      try { // Protect the `cs_in' object, so we can destroy it
        cs_in.create(&stream_box);
        copy_codestream(&jp2_out,cs_in);
        cs_in.destroy();
      }
      catch (int val)
      {
        if (cs_in.exists())
          cs_in.destroy();
        throw val;
      }
    }
  else
    { // Simply copy the box contents directly
      kdu_byte *buffer = new kdu_byte[1<<20];
      try {
        int xfer_bytes;
        while ((xfer_bytes=stream_box.read(buffer,(1<<20))) > 0)
          jp2_out.write(buffer,xfer_bytes);
      }
      catch (int val)
      {
        delete[] buffer;
        throw val;
      }
      delete[] buffer;
    }
  stream_box.close();
  jp2_out.close();
  tgt.close();
}

/*****************************************************************************/
/*                        kdws_renderer::save_as_jpx                         */
/*****************************************************************************/

void
  kdws_renderer::save_as_jpx(const char *filename,
                            bool preserve_codestream_links,
                            bool force_codestream_links)
{
  assert(jpx_in.exists());
  if ((open_file_pathname == NULL) || jp2_family_in.uses_cache())
    force_codestream_links = preserve_codestream_links = false;
  if (force_codestream_links)
    preserve_codestream_links = true;
  int n, num_codestreams, num_compositing_layers;
  jpx_in.count_codestreams(num_codestreams);
  jpx_in.count_compositing_layers(num_compositing_layers);
  jpx_layer_source default_layer; // To substitute for missing ones
  for (n=0; (n < num_compositing_layers) && !default_layer; n++)
    default_layer = jpx_in.access_layer(n);
  if (!default_layer)
    { kdu_error e; e << "Cannot write JPX file.  Not even one of the source "
      "file's compositing layers is available yet."; }
  jpx_codestream_source default_stream = // To substitute for missing ones
    jpx_in.access_codestream(default_layer.get_codestream_id(0));

  // Create output file
  jp2_family_tgt tgt; tgt.open(filename);
  jpx_target jpx_out; jpx_out.open(&tgt);

  // Allocate local arrays
  jpx_codestream_target *out_streams =
    new jpx_codestream_target[num_codestreams];
  jpx_codestream_source *in_streams =
    new jpx_codestream_source[num_codestreams];
  kdu_byte *buffer = new kdu_byte[1<<20];

  kdu_codestream cs_in;

  try { // Protect local arrays

      // Generate output layers
      for (n=0; n < num_compositing_layers; n++)
        {
          jpx_layer_source layer_in = jpx_in.access_layer(n);
          if (!layer_in)
            layer_in = default_layer;
          jpx_layer_target layer_out = jpx_out.add_layer();
          jp2_colour colour_in, colour_out;
          int k=0;
          while ((colour_in = layer_in.access_colour(k++)).exists())
            {
              colour_out =
                layer_out.add_colour(colour_in.get_precedence(),
                                     colour_in.get_approximation_level());
              colour_out.copy(colour_in);
            }
          layer_out.access_resolution().copy(layer_in.access_resolution());
          layer_out.access_channels().copy(layer_in.access_channels());
          int num_streams = layer_in.get_num_codestreams();
          for (k=0; k < num_streams; k++)
            {
              kdu_coords alignment, sampling, denominator;
              int codestream_idx =
                layer_in.get_codestream_registration(k,alignment,sampling,
                                                     denominator);
              layer_out.set_codestream_registration(codestream_idx,alignment,
                                                    sampling,denominator);
            }
        }

      // Copy composition instructions and compatibility info
      jpx_out.access_compatibility().copy(jpx_in.access_compatibility());
      jpx_out.access_composition().copy(jpx_in.access_composition());

      // Copy codestream headers
      for (n=0; n < num_codestreams; n++)
        {
          in_streams[n] = jpx_in.access_codestream(n);
          if (!in_streams[n])
            in_streams[n] = default_stream;
          out_streams[n] = jpx_out.add_codestream();
          out_streams[n].access_dimensions().copy(
                                in_streams[n].access_dimensions(true));
          out_streams[n].access_palette().copy(
                                in_streams[n].access_palette());
        }

      // Write JPX headers and all the code-streams
      jp2_data_references data_refs_in;
      if (preserve_codestream_links)
        data_refs_in = jpx_in.access_data_references();
      for (n=0; n < num_codestreams; n++)
        {
          jpx_out.write_headers(NULL,NULL,n); // Interleaved header writing
          jpx_input_box box_in;
          jpx_fragment_list frags_in;
          if (preserve_codestream_links)
            frags_in = in_streams[n].access_fragment_list();
          if (!(frags_in.exists() && data_refs_in.exists()))
            in_streams[n].open_stream(&box_in);
          jp2_output_box *box_out = NULL;
          if ((!force_codestream_links) && box_in.exists())
            box_out = out_streams[n].open_stream();
          int xfer_bytes;
          if (jp2_family_in.uses_cache())
            { // Need to generate output codestreams from scratch - transcoding
              assert(box_in.exists() && (box_out != NULL));
              cs_in.create(&box_in);
              copy_codestream(box_out,cs_in);
              cs_in.destroy();
              box_out->close();
            }
          else if (box_out != NULL)
            { // Simply copy the box contents directly
              if (box_in.get_remaining_bytes() > 0)
                box_out->set_target_size(box_in.get_remaining_bytes());
              else
                box_out->write_header_last();
              while ((xfer_bytes=box_in.read(buffer,1<<20)) > 0)
                box_out->write(buffer,xfer_bytes);
              box_out->close();
            }
          else
            { // Writing a fragment list
              kdu_long offset, length;            
              if (box_in.exists())
                { // Generate fragment table from embedded source codestream
                  assert(force_codestream_links);
                  jp2_locator loc = box_in.get_locator();
                  offset = loc.get_file_pos() + box_in.get_box_header_length();
                  length = box_in.get_remaining_bytes();
                  if (length < 0)
                    for (length=0; (xfer_bytes=box_in.read(buffer,1<<20)) > 0;)
                      length += xfer_bytes;
                  const char *sep1 = strrchr(filename,'\\');
                  const char *sep2 = strrchr(open_file_pathname,'\\');
                  const char *source_path = open_file_pathname;
                  if ((sep1 != NULL) && (sep2 != NULL) &&
                      ((sep1-filename) == (sep2-open_file_pathname)) &&
                      (strncmp(filename,open_file_pathname,sep1-filename)==0))
                    source_path = sep2+1;
                  out_streams[n].add_fragment(source_path,offset,length,true);
                }
              else
                { // Copy fragment table from linked source codestream
                  assert(frags_in.exists() && data_refs_in.exists());
                  int frag_idx, num_frags = frags_in.get_num_fragments();
                  for (frag_idx=0; frag_idx < num_frags; frag_idx++)
                    {
                      int url_idx;
                      const char *url_path;
                      if (frags_in.get_fragment(frag_idx,url_idx,
                                                offset,length) &&
                          ((url_path=data_refs_in.get_url(url_idx)) != NULL))
                        out_streams[n].add_fragment(url_path,offset,length);
                    }
                }
              out_streams[n].write_fragment_table();
            }

          box_in.close();
        }

      // Copy JPX metadata
      int i_param=0;
      void *addr_param=NULL;
      jp2_output_box *metadata_container=NULL;
      jpx_meta_manager meta_in = jpx_in.access_meta_manager();
      jpx_meta_manager meta_out = jpx_out.access_meta_manager();
      meta_out.copy(meta_in);
      while ((metadata_container =
              jpx_out.write_metadata(&i_param,&addr_param)) != NULL)
        { // Note that we are only writing headers up to and including
          // those which are required for codestream `n'.
          kdws_file *file;
          if ((file_server != NULL) && (addr_param == (void *)file_server) &&
              ((file = file_server->find_file(i_param)) != NULL))
            write_metadata_box_from_file(metadata_container,file);
        }
    }
  catch (int val)
    {
      delete[] in_streams;
      delete[] out_streams;
      delete[] buffer;
      if (cs_in.exists())
        cs_in.destroy();
      throw val;
    }

  // Clean up
  delete[] in_streams;
  delete[] out_streams;
  delete[] buffer;
  jpx_out.close();
  tgt.close();
}

/*****************************************************************************/
/*                        kdws_renderer::save_as_raw                         */
/*****************************************************************************/

void
  kdws_renderer::save_as_raw(const char *filename)
{
  compositor->halt_processing();
  kdu_ilayer_ref ilyr = compositor->get_next_ilayer(kdu_ilayer_ref());
  kdu_istream_ref istr = compositor->get_ilayer_stream(ilyr,0);
  if (!istr)
    { kdu_error e; e << "No active codestream is available for writing the "
      "requested output file."; }
  kdu_codestream cs_in = compositor->access_codestream(istr);
  kdu_simple_file_target file_out;
  file_out.open(filename);
  copy_codestream(&file_out,cs_in);
  file_out.close();
}




/*****************************************************************************/
/*                      kdws_renderer::copy_codestream                       */
/*****************************************************************************/

void
  kdws_renderer::copy_codestream(kdu_compressed_target *tgt, kdu_codestream src)
{
  src.apply_input_restrictions(0,0,0,0,NULL,KDU_WANT_OUTPUT_COMPONENTS);
  siz_params *siz_in = src.access_siz();
  siz_params init_siz; init_siz.copy_from(siz_in,-1,-1);
  kdu_codestream dst; dst.create(&init_siz,tgt);
  try { // Protect the `dst' codestream
      siz_params *siz_out = dst.access_siz();
      siz_out->copy_from(siz_in,-1,-1);
      siz_out->parse_string("ORGgen_plt=yes");
      siz_out->finalize_all(-1);

      // Now ready to perform the transfer of compressed data between streams
      kdu_dims tile_indices; src.get_valid_tiles(tile_indices);
      kdu_coords idx;
      for (idx.y=0; idx.y < tile_indices.size.y; idx.y++)
        for (idx.x=0; idx.x < tile_indices.size.x; idx.x++)
          {
            kdu_tile tile_in = src.open_tile(idx+tile_indices.pos);
            int tnum = tile_in.get_tnum();
            siz_out->copy_from(siz_in,tnum,tnum);
            siz_out->finalize_all(tnum);
            kdu_tile tile_out = dst.open_tile(idx+tile_indices.pos);
            assert(tnum == tile_out.get_tnum());
            copy_tile(tile_in,tile_out);
            tile_in.close();
            tile_out.close();
          }
      dst.trans_out();
    }
  catch (int val)
    {
      if (dst.exists())
        dst.destroy();
      throw val;
    }
  dst.destroy();
}


/* ========================================================================= */
/*                    COMMAND HANDLERS for kdws_renderer                     */
/* ========================================================================= */

BEGIN_MESSAGE_MAP(kdws_renderer, CCmdTarget)
// File Menu Handlers
	ON_COMMAND(ID_FILE_DISCONNECT, menu_FileDisconnect)
	ON_UPDATE_COMMAND_UI(ID_FILE_DISCONNECT, can_do_FileDisconnect)
	ON_COMMAND(ID_FILE_SAVE_AS, menu_FileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, can_do_FileSaveAs)
  ON_COMMAND(ID_FILE_SAVEASLINKED, menu_FileSaveAsLinked)
  ON_UPDATE_COMMAND_UI(ID_FILE_SAVEASLINKED, can_do_FileSaveAsLinked)
  ON_COMMAND(ID_FILE_SAVEASEMBEDDED, menu_FileSaveAsEmbedded)
  ON_UPDATE_COMMAND_UI(ID_FILE_SAVEASEMBEDDED, can_do_FileSaveAsEmbedded)
  ON_COMMAND(ID_FILE_SAVEANDRELOAD, menu_FileSaveReload)
  ON_UPDATE_COMMAND_UI(ID_FILE_SAVEANDRELOAD, can_do_FileSaveReload)
	ON_COMMAND(ID_FILE_PROPERTIES, menu_FileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, can_do_FileProperties)
// View Menu Handlers (orientation and zoom)
	ON_COMMAND(ID_VIEW_HFLIP, menu_ViewHflip)
	ON_UPDATE_COMMAND_UI(ID_VIEW_HFLIP, can_do_ViewHflip)
	ON_COMMAND(ID_VIEW_VFLIP, menu_ViewVflip)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VFLIP, can_do_ViewVflip)
	ON_COMMAND(ID_VIEW_ROTATE, menu_ViewRotate)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE, can_do_ViewRotate)
	ON_COMMAND(ID_VIEW_COUNTER_ROTATE, menu_ViewCounterRotate)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COUNTER_ROTATE, can_do_ViewCounterRotate)
	ON_COMMAND(ID_VIEW_ZOOM_OUT, menu_ViewZoomOut)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_OUT, can_do_ViewZoomOut)
	ON_COMMAND(ID_VIEW_ZOOM_IN, menu_ViewZoomIn)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_IN, can_do_ViewZoomIn)
	ON_COMMAND(ID_VIEW_ZOOM_OUT_SLIGHTLY, menu_ViewZoomOutSlightly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_OUT_SLIGHTLY, can_do_ViewZoomOut)
	ON_COMMAND(ID_VIEW_ZOOM_IN_SLIGHTLY, menu_ViewZoomInSlightly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_IN_SLIGHTLY, can_do_ViewZoomIn)
  ON_COMMAND(ID_VIEW_OPTIMIZESCALE, menu_ViewOptimizescale)
  ON_UPDATE_COMMAND_UI(ID_VIEW_OPTIMIZESCALE, can_do_ViewOptimizescale)
	ON_COMMAND(ID_VIEW_RESTORE, menu_ViewRestore)
  ON_UPDATE_COMMAND_UI(ID_VIEW_RESTORE, can_do_ViewRestore)
	ON_COMMAND(ID_VIEW_REFRESH, menu_ViewRefresh)
	ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH, can_do_ViewRefresh)
// View Menu Handlers (quality layers)
	ON_COMMAND(ID_LAYERS_LESS, menu_LayersLess)
	ON_UPDATE_COMMAND_UI(ID_LAYERS_LESS, can_do_LayersLess)
	ON_COMMAND(ID_LAYERS_MORE, menu_LayersMore)
	ON_UPDATE_COMMAND_UI(ID_LAYERS_MORE, can_do_LayersMore)
// View Menu Handlers (status display)
	ON_COMMAND(ID_STATUS_TOGGLE, menu_StatusToggle)
	ON_UPDATE_COMMAND_UI(ID_STATUS_TOGGLE, can_do_StatusToggle)
// View Menu Handlers (image window)
	ON_COMMAND(ID_VIEW_WIDEN, menu_ViewWiden)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WIDEN, can_do_ViewWiden)
	ON_COMMAND(ID_VIEW_SHRINK, menu_ViewShrink)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHRINK, can_do_ViewShrink)
  ON_COMMAND(ID_VIEW_SCALE_X2, menu_ViewScaleX2)
  ON_COMMAND(ID_VIEW_SCALE_X1, menu_ViewScaleX1)
// View Menu Handlers (navigation)
	ON_COMMAND(ID_COMPONENT1, menu_NavComponent1)
	ON_UPDATE_COMMAND_UI(ID_COMPONENT1, can_do_NavComponent1)
	ON_COMMAND(ID_COMPONENT_NEXT, menu_NavComponentNext)
	ON_UPDATE_COMMAND_UI(ID_COMPONENT_NEXT, can_do_NavComponentNext)
	ON_COMMAND(ID_COMPONENT_PREV, menu_NavComponentPrev)
	ON_UPDATE_COMMAND_UI(ID_COMPONENT_PREV, can_do_NavComponentPrev)
	ON_COMMAND(ID_COMPOSITING_LAYER, menu_NavCompositingLayer)
	ON_UPDATE_COMMAND_UI(ID_COMPOSITING_LAYER, can_do_NavCompositingLayer)
  ON_COMMAND(ID_MULTI_COMPONENT, menu_NavMultiComponent)
	ON_UPDATE_COMMAND_UI(ID_MULTI_COMPONENT, can_do_NavMultiComponent)
  ON_COMMAND(ID_TRACK_NEXT, menu_NavTrackNext)
  ON_UPDATE_COMMAND_UI(ID_TRACK_NEXT, can_do_NavTrackNext)
	ON_COMMAND(ID_IMAGE_NEXT, menu_NavImageNext)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_NEXT, can_do_NavImageNext)
	ON_COMMAND(ID_IMAGE_PREV, menu_NavImagePrev)
	ON_UPDATE_COMMAND_UI(ID_IMAGE_PREV, can_do_NavImagePrev)
// Focus Menu Handlers
	ON_COMMAND(ID_FOCUS_OFF, menu_FocusOff)
	ON_UPDATE_COMMAND_UI(ID_FOCUS_OFF, can_do_FocusOff)
	ON_COMMAND(ID_FOCUS_HIGHLIGHTING, menu_FocusHighlighting)
	ON_UPDATE_COMMAND_UI(ID_FOCUS_HIGHLIGHTING, can_do_FocusHighlighting)
	ON_COMMAND(ID_FOCUS_WIDEN, menu_FocusWiden)
	ON_COMMAND(ID_FOCUS_SHRINK, menu_FocusShrink)
	ON_COMMAND(ID_FOCUS_LEFT, menu_FocusLeft)
	ON_COMMAND(ID_FOCUS_RIGHT, menu_FocusRight)
	ON_COMMAND(ID_FOCUS_UP, menu_FocusUp)
	ON_COMMAND(ID_FOCUS_DOWN, menu_FocusDown)
// Mode Menu Handlers
  ON_COMMAND(ID_MODE_FAST, menu_ModeFast)
	ON_UPDATE_COMMAND_UI(ID_MODE_FAST, can_do_ModeFast)
	ON_COMMAND(ID_MODE_FUSSY, menu_ModeFussy)
	ON_UPDATE_COMMAND_UI(ID_MODE_FUSSY, can_do_ModeFussy)
	ON_COMMAND(ID_MODE_RESILIENT, menu_ModeResilient)
	ON_UPDATE_COMMAND_UI(ID_MODE_RESILIENT, can_do_ModeResilient)
	ON_COMMAND(ID_MODE_RESILIENT_SOP, menu_ModeResilientSop)
	ON_UPDATE_COMMAND_UI(ID_MODE_RESILIENT_SOP, can_do_ModeResilientSop)
	ON_COMMAND(ID_MODE_SEEKABLE, menu_ModeSeekable)
	ON_UPDATE_COMMAND_UI(ID_MODE_SEEKABLE, can_do_ModeSeekable)
  ON_COMMAND(ID_MODE_SINGLETHREADED, menu_ModeSinglethreaded)
  ON_UPDATE_COMMAND_UI(ID_MODE_SINGLETHREADED, can_do_ModeSinglethreaded)
  ON_COMMAND(ID_MODE_MULTITHREADED, menu_ModeMultithreaded)
  ON_UPDATE_COMMAND_UI(ID_MODE_MULTITHREADED, can_do_ModeMultithreaded)
  ON_COMMAND(ID_MODE_MORETHREADS, menu_ModeMorethreads)
  ON_UPDATE_COMMAND_UI(ID_MODE_MORETHREADS, can_do_ModeMorethreads)
  ON_COMMAND(ID_MODE_LESSTHREADS, menu_ModeLessthreads)
  ON_UPDATE_COMMAND_UI(ID_MODE_LESSTHREADS, can_do_ModeLessthreads)
  ON_COMMAND(ID_MODE_RECOMMENDED_THREADS, menu_ModeRecommendedThreads)
  ON_UPDATE_COMMAND_UI(ID_MODE_RECOMMENDED_THREADS, can_do_ModeRecommendedThreads)
// Metadata Menu Handlers (metadata)
	ON_COMMAND(ID_METADATA_CATALOG, menu_MetadataCatalog)
	ON_UPDATE_COMMAND_UI(ID_METADATA_CATALOG, can_do_MetadataCatalog)
	ON_COMMAND(ID_METADATA_SWAPFOCUS, menu_MetadataSwapFocus)
	ON_UPDATE_COMMAND_UI(ID_METADATA_SWAPFOCUS, can_do_MetadataSwapFocus)
	ON_COMMAND(ID_METADATA_SHOW, menu_MetadataShow)
	ON_UPDATE_COMMAND_UI(ID_METADATA_SHOW, can_do_MetadataShow)
	ON_COMMAND(ID_METADATA_ADD, menu_MetadataAdd)
	ON_UPDATE_COMMAND_UI(ID_METADATA_ADD, can_do_MetadataAdd)
	ON_COMMAND(ID_METADATA_EDIT, menu_MetadataEdit)
	ON_UPDATE_COMMAND_UI(ID_METADATA_EDIT, can_do_MetadataEdit)
	ON_COMMAND(ID_METADATA_DELETE, menu_MetadataDelete)
	ON_UPDATE_COMMAND_UI(ID_METADATA_DELETE, can_do_MetadataDelete)
	ON_COMMAND(ID_METADATA_COPYLABEL, menu_MetadataCopyLabel)
	ON_UPDATE_COMMAND_UI(ID_METADATA_COPYLABEL, can_do_MetadataCopyLabel)
	ON_COMMAND(ID_METADATA_COPYLINK, menu_MetadataCopyLink)
	ON_UPDATE_COMMAND_UI(ID_METADATA_COPYLINK, can_do_MetadataCopyLink)
	ON_COMMAND(ID_METADATA_CUT, menu_MetadataCut)
	ON_UPDATE_COMMAND_UI(ID_METADATA_CUT, can_do_MetadataCut)
	ON_COMMAND(ID_METADATA_PASTENEW, menu_MetadataPasteNew)
	ON_UPDATE_COMMAND_UI(ID_METADATA_PASTENEW, can_do_MetadataPasteNew)
// Metadata Menu Handlers (overlays)
	ON_COMMAND(ID_OVERLAY_ENABLED, menu_OverlayEnable)
	ON_UPDATE_COMMAND_UI(ID_OVERLAY_ENABLED, can_do_OverlayEnable)
	ON_COMMAND(ID_OVERLAY_FLASHING, menu_OverlayFlashing)
	ON_UPDATE_COMMAND_UI(ID_OVERLAY_FLASHING, can_do_OverlayFlashing)
	ON_COMMAND(ID_OVERLAY_TOGGLE, menu_OverlayToggle)
  ON_UPDATE_COMMAND_UI(ID_OVERLAY_TOGGLE, can_do_OverlayToggle)
	ON_COMMAND(ID_OVERLAY_RESTRICT, menu_OverlayRestrict)
  ON_UPDATE_COMMAND_UI(ID_OVERLAY_RESTRICT, can_do_OverlayRestrict)
	ON_COMMAND(ID_OVERLAY_HEAVIER, menu_OverlayHeavier)
	ON_UPDATE_COMMAND_UI(ID_OVERLAY_HEAVIER, can_do_OverlayHeavier)
	ON_COMMAND(ID_OVERLAY_LIGHTER, menu_OverlayLighter)
	ON_UPDATE_COMMAND_UI(ID_OVERLAY_LIGHTER, can_do_OverlayLighter)
  ON_COMMAND(ID_OVERLAY_DOUBLESIZE, menu_OverlayDoubleSize)
	ON_UPDATE_COMMAND_UI(ID_OVERLAY_DOUBLESIZE, can_do_OverlayDoubleSize)
	ON_COMMAND(ID_OVERLAY_HALVESIZE, menu_OverlayHalveSize)
	ON_UPDATE_COMMAND_UI(ID_OVERLAY_HALVESIZE, can_do_OverlayHalveSize)
// Play Menu Handlers
	ON_COMMAND(ID_PLAY_STARTFORWARD, menu_PlayStartForward)
	ON_UPDATE_COMMAND_UI(ID_PLAY_STARTFORWARD, can_do_PlayStartForward)
	ON_COMMAND(ID_PLAY_STARTBACKWARD, menu_PlayStartBackward)
	ON_UPDATE_COMMAND_UI(ID_PLAY_STARTBACKWARD, can_do_PlayStartBackward)
	ON_COMMAND(ID_PLAY_STOP, menu_PlayStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_STOP, can_do_PlayStop)
	ON_COMMAND(ID_PLAY_REPEAT, menu_PlayRepeat)
	ON_UPDATE_COMMAND_UI(ID_PLAY_REPEAT, can_do_PlayRepeat)
	ON_COMMAND(ID_PLAY_NATIVE, menu_PlayNative)
	ON_UPDATE_COMMAND_UI(ID_PLAY_NATIVE, can_do_PlayNative)
	ON_COMMAND(ID_PLAY_CUSTOM, menu_PlayCustom)
	ON_UPDATE_COMMAND_UI(ID_PLAY_CUSTOM, can_do_PlayCustom)
	ON_COMMAND(ID_PLAY_FRAMERATEUP, menu_PlayFrameRateUp)
	ON_UPDATE_COMMAND_UI(ID_PLAY_FRAMERATEUP, can_do_PlayFrameRateUp)
	ON_COMMAND(ID_PLAY_FRAMERATEDOWN, menu_PlayFrameRateDown)
	ON_UPDATE_COMMAND_UI(ID_PLAY_FRAMERATEDOWN, can_do_PlayFrameRateDown)

  END_MESSAGE_MAP()

/* ========================================================================= */
/*                       File Menu Command Handlers                          */
/* ========================================================================= */

/*****************************************************************************/
/*                   kdws_renderer::menu_FileDisconnect                      */
/*****************************************************************************/

void kdws_renderer::menu_FileDisconnect() 
{
  if ((jpip_client != NULL) && jpip_client->is_alive(client_request_queue_id))
    jpip_client->disconnect(false,30000,client_request_queue_id,false);
             // Give it a long timeout so as to avoid accidentally breaking
             // an HTTP request channel which might be shared with other
             // request queues -- but let's not wait around here.
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_FileDisconnect                    */
/*****************************************************************************/

void kdws_renderer::can_do_FileDisconnect(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((jpip_client != NULL) &&
                 jpip_client->is_alive(client_request_queue_id));
}

/*****************************************************************************/
/*                   kdws_renderer::menu_FileSaveAs                          */
/*****************************************************************************/

void
  kdws_renderer::menu_FileSaveAs()
{
  if ((compositor == NULL) || in_playmode)
    return;
  save_as(true,false);
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_FileSaveAs                        */
/*****************************************************************************/

void
  kdws_renderer::can_do_FileSaveAs(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) && !in_playmode);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_FileSaveAsLinked                      */
/*****************************************************************************/

void kdws_renderer::menu_FileSaveAsLinked()
{
  if ((compositor == NULL) || in_playmode)
    return;
  save_as(true,true);
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_FileSaveAsLinked                    */
/*****************************************************************************/

void kdws_renderer::can_do_FileSaveAsLinked(CCmdUI *pCmdUI)
{
  pCmdUI->Enable((compositor != NULL) && (!in_playmode) &&
                 jp2_family_in.exists() && !jp2_family_in.uses_cache());
}

/*****************************************************************************/
/*                 kdws_renderer::menu_FileSaveAsEmbedded                    */
/*****************************************************************************/

void kdws_renderer::menu_FileSaveAsEmbedded()
{
  if ((compositor == NULL) || in_playmode)
    return;
  save_as(false,false);
}

/*****************************************************************************/
/*                kdws_renderer::can_do_FileSaveAsEmbedded                   */
/*****************************************************************************/

void kdws_renderer::can_do_FileSaveAsEmbedded(CCmdUI *pCmdUI)
{
  pCmdUI->Enable((compositor != NULL) && !in_playmode);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_FileSaveReload                      */
/*****************************************************************************/

void kdws_renderer::menu_FileSaveReload()
{
  if ((compositor == NULL) || in_playmode || mj2_in.exists() ||
      (!have_unsaved_edits) || (open_file_pathname == NULL))
    return;
  if (!save_without_asking)
    {
      int response =
        window->interrogate_user(
                           "You are about to save to the file which you are "
                           "currently viewing.  In the process, "
                           "there is a chance that you might lose some "
                           "metadata which could not be internally "
                           "represented.  Do you want to be asked this "
                           "question again in the future?",
                           "Kdu_show: About to overwrite file",
                           MB_YESNOCANCEL | MB_ICONWARNING);
      if (response == IDCANCEL)
        return;
      if (response == IDNO)
        save_without_asking = true;
    }
  if (manager->get_open_file_retain_count(open_file_pathname) != 1)
    {
      window->interrogate_user(
                         "Cannot perform this operation at present.  There "
                         "are other open windows in the application which "
                         "are also using this same file -- you may not "
                         "overwrite and reload it until they are closed.",
                         "Kdu_show: File in use elsewhere",
                         MB_OK | MB_ICONERROR);
      return;
    }

  if (!save_over())
    return;
  char *filename = new char[strlen(open_file_pathname)+1];
  strcpy(filename,open_file_pathname);
  bool save_without_asking = this->save_without_asking;
  open_file(filename);
  this->save_without_asking = save_without_asking;
  delete[] filename;
}

/*****************************************************************************/
/*                  kdws_renderer::can_do_FileSaveReload                     */
/*****************************************************************************/

void kdws_renderer::can_do_FileSaveReload(CCmdUI *pCmdUI)
{
  pCmdUI->Enable((compositor != NULL) && have_unsaved_edits &&
                 (open_file_pathname != NULL) && (!mj2_in.exists()) &&
                 !in_playmode);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_FileProperties                     */
/*****************************************************************************/

void kdws_renderer::menu_FileProperties() 
{
  if (compositor == NULL)
    return;
  compositor->halt_processing(); // Stops any current processing
  kdu_ilayer_ref ilyr = compositor->get_next_ilayer(kdu_ilayer_ref());
  kdu_istream_ref istr = compositor->get_ilayer_stream(ilyr,0);
  if (!istr)
    return;
  int c_idx;
  kdu_codestream codestream = compositor->access_codestream(istr);
  compositor->get_istream_info(istr,c_idx);

  kdu_message_queue collector;
  collector.start_message();
  collector << "Properties for code-stream " << c_idx << "\n\n";

  // Textualize any comments
  kdu_codestream_comment com;
  while ((com = codestream.get_comment(com)).exists())
    {
      int length;
      const char *comtext = com.get_text();
      if (*comtext != '\0')
        collector << ">>>>> Code-stream comment:\n" << comtext << "\n";
      else if ((length = com.get_data(NULL,0,INT_MAX)) > 0)
        collector << ">>>>> Binary code-stream comment ("
                  << length << " bytes)\n";
    }

  // Textualize the main code-stream header
  kdu_params *root = codestream.access_siz();
  collector << "<<<<< Main Header >>>>>\n";
  root->textualize_attributes(collector,-1,-1);

  // Textualize the tile headers
  codestream.apply_input_restrictions(0,0,0,0,NULL);
  codestream.change_appearance(false,false,false);
  kdu_dims valid_tiles; codestream.get_valid_tiles(valid_tiles);
  kdu_coords idx;
  for (idx.y=0; idx.y < valid_tiles.size.y; idx.y++)
    for (idx.x=0; idx.x < valid_tiles.size.x; idx.x++)
      {
        kdu_dims tile_dims;
        codestream.get_tile_dims(valid_tiles.pos+idx,-1,tile_dims);
        int tnum = idx.x + idx.y*valid_tiles.size.x;
        collector << "<<<<< Tile " << tnum << " >>>>>"
                  << " Canvas coordinates: "
                  << "y = " << tile_dims.pos.y
                  << "; x = " << tile_dims.pos.x
                  << "; height = " << tile_dims.size.y
                  << "; width = " << tile_dims.size.x << "\n";

        // Open and close each tile in case the code-stream has not yet been
        // fully parsed
        try {
            kdu_tile tile = codestream.open_tile(valid_tiles.pos+idx);
            tile.close();
          }
        catch (int) { // Error occurred during tile open
            close_file();
            return;
          }

        root->textualize_attributes(collector,tnum,tnum);
      }

  // Display the properties.
  collector.flush(true);
  kdws_properties properties(codestream,collector.pop_message());
  properties.DoModal();
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_FileProperties                    */
/*****************************************************************************/

void kdws_renderer::can_do_FileProperties(CCmdUI* pCmdUI)
{
  pCmdUI->Enable(compositor != NULL);
}

/* ========================================================================= */
/*             View Menu Command Handlers (orientation and zoom)             */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdws_renderer::menu_ViewHflip                        */
/*****************************************************************************/

void kdws_renderer::menu_ViewHflip()
{
  if (buffer == NULL)
    return;
  hflip = !hflip;
  calculate_rel_view_and_focus_params();
  view_centre_x = 1.0 - view_centre_x;
  rel_focus.hflip();
  initialize_regions(true);
}

/*****************************************************************************/
/*                     kdws_renderer::can_do_ViewHflip                       */
/*****************************************************************************/

void kdws_renderer::can_do_ViewHflip(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(buffer != NULL);
}

/*****************************************************************************/
/*                      kdws_renderer::menu_ViewVflip                        */
/*****************************************************************************/

void kdws_renderer::menu_ViewVflip() 
{
  if (buffer == NULL)
    return;
  vflip = !vflip;
  calculate_rel_view_and_focus_params();
  view_centre_y = 1.0 - view_centre_y;
  rel_focus.vflip();
  initialize_regions(true);
}

/*****************************************************************************/
/*                     kdws_renderer::can_do_ViewVflip                      */
/*****************************************************************************/

void kdws_renderer::can_do_ViewVflip(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(buffer != NULL);
}

/*****************************************************************************/
/*                       kdws_renderer::menu_ViewRotate                      */
/*****************************************************************************/

void kdws_renderer::menu_ViewRotate() 
{
  if (buffer == NULL)
    return;
  // Need to effectively add an extra transpose, followed by an extra hflip.
  transpose = !transpose;
  bool tmp = hflip; hflip = vflip; vflip = tmp;
  hflip = !hflip;
  calculate_rel_view_and_focus_params();
  double f_tmp = view_centre_y;
  view_centre_y = view_centre_x;
  view_centre_x = 1.0-f_tmp;
  rel_focus.transpose();
  rel_focus.hflip();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  initialize_regions(true);
}

/*****************************************************************************/
/*                     kdws_renderer::can_do_ViewRotate                      */
/*****************************************************************************/

void kdws_renderer::can_do_ViewRotate(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(buffer != NULL);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_ViewCounterRotate                   */
/*****************************************************************************/

void kdws_renderer::menu_ViewCounterRotate() 
{
  if (buffer == NULL)
    return;
  // Need to effectively add an extra transpose, followed by an extra vflip.
  transpose = !transpose;
  bool tmp = hflip; hflip = vflip; vflip = tmp;
  vflip = !vflip;
  calculate_rel_view_and_focus_params();
  double f_tmp = view_centre_x;
  view_centre_x = view_centre_y;
  view_centre_y = 1.0-f_tmp;
  rel_focus.transpose();
  rel_focus.vflip();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  initialize_regions(true);
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_ViewCounterRotate                   */
/*****************************************************************************/

void kdws_renderer::can_do_ViewCounterRotate(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(buffer != NULL);
}

/*****************************************************************************/
/*                      kdws_renderer::menu_ViewZoomOut                      */
/*****************************************************************************/

void kdws_renderer::menu_ViewZoomOut() 
{
  if (buffer == NULL)
    return;
  if (rendering_scale == min_rendering_scale)
    return;
  rendering_scale = decrease_scale(rendering_scale,false);
  calculate_rel_view_and_focus_params();
  initialize_regions(true);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_ViewZoomOutSlightly                   */
/*****************************************************************************/

void kdws_renderer::menu_ViewZoomOutSlightly() 
{
  if (buffer == NULL)
    return;
  if (rendering_scale == min_rendering_scale)
    return;
  rendering_scale = decrease_scale(rendering_scale,true);
  calculate_rel_view_and_focus_params();
  initialize_regions(true);
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ViewZoomOut                      */
/*****************************************************************************/

void kdws_renderer::can_do_ViewZoomOut(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((buffer != NULL) &&
                 ((min_rendering_scale < 0.0F) ||
                  (rendering_scale > min_rendering_scale)));
}

/*****************************************************************************/
/*                     kdws_renderer::menu_ViewZoomIn                        */
/*****************************************************************************/

void kdws_renderer::menu_ViewZoomIn() 
{
  if (buffer == NULL)
    return;
  if (rendering_scale == max_rendering_scale)
    return;
  rendering_scale = increase_scale(rendering_scale,false);
  calculate_rel_view_and_focus_params();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  initialize_regions(true);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_ViewZoomInSlightly                    */
/*****************************************************************************/

void kdws_renderer::menu_ViewZoomInSlightly() 
{
  if (buffer == NULL)
    return;
  if (rendering_scale == max_rendering_scale)
    return;
  rendering_scale = increase_scale(rendering_scale,true);
  calculate_rel_view_and_focus_params();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  initialize_regions(true);
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ViewZoomIn                       */
/*****************************************************************************/

void kdws_renderer::can_do_ViewZoomIn(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(buffer != NULL);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_ViewOptimizescale                   */
/*****************************************************************************/

void kdws_renderer::menu_ViewOptimizescale()
{
  if ((compositor == NULL) || !configuration_complete)
    return;
  float min_scale = rendering_scale * 0.5F;
  float max_scale = rendering_scale * 2.0F;
  if (min_rendering_scale > 0.0F)
    {
      if (min_scale < min_rendering_scale)
        min_scale = min_rendering_scale;
      if (max_scale < min_rendering_scale)
        max_scale = min_rendering_scale;
    }
  if (max_rendering_scale > 0.0F)
    {
      if (min_scale > max_rendering_scale)
        min_scale = max_rendering_scale;
      if (max_scale > max_rendering_scale)
        max_scale = max_rendering_scale;
    }
  kdu_dims region_basis = (enable_focus_box)?focus_box:view_dims;
  float new_scale =
    compositor->find_optimal_scale(region_basis,rendering_scale,
                                   min_scale,max_scale);
  if (new_scale == rendering_scale)
    return;
  calculate_rel_view_and_focus_params();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  rendering_scale = new_scale;
  initialize_regions(true);
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_ViewOptimizescale                   */
/*****************************************************************************/

void kdws_renderer::can_do_ViewOptimizescale(CCmdUI *pCmdUI)
{
  pCmdUI->Enable((compositor != NULL) && configuration_complete);
}

/*****************************************************************************/
/*                     kdws_renderer::menu_ViewRestore                      */
/*****************************************************************************/

void kdws_renderer::menu_ViewRestore() 
{
  if (buffer == NULL)
    return;
  calculate_rel_view_and_focus_params();
  if (vflip)
    {
      vflip = false;
      view_centre_y = 1.0-view_centre_y;
      rel_focus.vflip();
    }
  if (hflip)
    {
      hflip = false;
      view_centre_x = 1.0-view_centre_x;
      rel_focus.hflip();
    }
  if (transpose)
    {
      transpose = false;
      double tmp = view_centre_y;
      view_centre_y = view_centre_x;
      view_centre_x = tmp;
      rel_focus.transpose();
    }
  initialize_regions(true);
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ViewRestore                      */
/*****************************************************************************/

void kdws_renderer::can_do_ViewRestore(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((buffer != NULL) && (transpose || vflip || hflip));
}

/*****************************************************************************/
/*                     kdws_renderer::menu_ViewRefresh                       */
/*****************************************************************************/

void kdws_renderer::menu_ViewRefresh() 
{
  refresh_display();
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ViewRefresh                      */
/*****************************************************************************/

void kdws_renderer::can_do_ViewRefresh(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(compositor != NULL);
}


/* ========================================================================= */
/*                View Menu Command Handlers (quality layers)                */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdws_renderer::menu_LayersLess                       */
/*****************************************************************************/

void kdws_renderer::menu_LayersLess()
{
  if (compositor == NULL)
    return;
  int max_layers = compositor->get_max_available_quality_layers();
  if (max_display_layers > max_layers)
    max_display_layers = max_layers;
  if (max_display_layers <= 1)
    { max_display_layers = 1; return; }
  max_display_layers--;
  if (enable_region_posting)
    update_client_window_of_interest();
  else
    status_id = KDS_STATUS_LAYER_RES;
  compositor->set_max_quality_layers(max_display_layers);
  refresh_display();
  display_status();
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_LayersLess                        */
/*****************************************************************************/

void kdws_renderer::can_do_LayersLess(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) && (max_display_layers > 1));
}

/*****************************************************************************/
/*                    kdws_renderer::menu_LayersMore                         */
/*****************************************************************************/

void kdws_renderer::menu_LayersMore() 
{
  if (compositor == NULL)
    return;
  int max_layers = compositor->get_max_available_quality_layers();
  bool need_update = (max_display_layers < max_layers);
  max_display_layers++;
  if (max_display_layers >= max_layers)
    max_display_layers = 1<<16;
  if (need_update)
    {
      refresh_display();
      if (enable_region_posting)
        update_client_window_of_interest();
      else
        status_id = KDS_STATUS_LAYER_RES;
      compositor->set_max_quality_layers(max_display_layers);
      display_status();
    }
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_LayersMore                        */
/*****************************************************************************/

void kdws_renderer::can_do_LayersMore(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) &&
                 (max_display_layers <
                  compositor->get_max_available_quality_layers()));
}


/* ========================================================================= */
/*                View Menu Command Handlers (status display)                */
/* ========================================================================= */

/*****************************************************************************/
/*                   kdws_renderer::menu_StatusToggle                        */
/*****************************************************************************/

void kdws_renderer::menu_StatusToggle() 
{
  if (status_id == KDS_STATUS_LAYER_RES)
    status_id = KDS_STATUS_DIMS;
  else if (status_id == KDS_STATUS_DIMS)
    status_id = KDS_STATUS_MEM;
  else if (status_id == KDS_STATUS_MEM)
    status_id = KDS_STATUS_CACHE;
  else if (status_id == KDS_STATUS_CACHE)
    status_id = KDS_STATUS_PLAYSTATS;
  else
    status_id = KDS_STATUS_LAYER_RES;
  display_status();
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_StatusToggle                        */
/*****************************************************************************/

void kdws_renderer::can_do_StatusToggle(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(compositor != NULL);
}


/* ========================================================================= */
/*                 View Menu Command Handlers (image window)                 */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdws_renderer::menu_ViewWiden                         */
/*****************************************************************************/

void kdws_renderer::menu_ViewWiden() 
{
  RECT rect;
  kdu_dims target_dims;
  kdu_coords limits;

  // Find screen dimensions
  CDC *dc = image_view->GetDC();
  limits.x = dc->GetDeviceCaps(HORZRES);
  limits.y = dc->GetDeviceCaps(VERTRES);
  image_view->ReleaseDC(dc);

  // Find current window location and dimensions
  window->GetWindowRect(&rect);
  target_dims.pos.x = rect.left;
  target_dims.pos.y = rect.top;
  target_dims.size.x = rect.right-rect.left;
  target_dims.size.y = rect.bottom-rect.top;

  // Calculate new dimensions
  kdu_coords new_size = target_dims.size;
  new_size.x += new_size.x / 2;
  new_size.y += new_size.y / 2;
  if (new_size.x > limits.x)
    new_size.x = limits.x;
  if (new_size.y > limits.y)
    new_size.y = limits.y;
  if (target_dims.size == new_size)
    return;

  target_dims.pos.x -= (new_size.x - target_dims.size.x) / 2;
  target_dims.pos.y -= (new_size.y - target_dims.size.y) / 2;
  target_dims.size = new_size;

  // Make sure new location fits inside window
  if (target_dims.pos.x < 0)
    target_dims.pos.x = 0;
  if (target_dims.pos.y < 0)
    target_dims.pos.y = 0;
  limits -= target_dims.pos + target_dims.size;
  if (limits.x < 0)
    target_dims.pos.x += limits.x;
  if (limits.y < 0)
    target_dims.pos.y += limits.y;

  // Reposition window
  calculate_rel_view_and_focus_params();
  window->SetWindowPos(NULL,target_dims.pos.x,target_dims.pos.y,
                       target_dims.size.x,target_dims.size.y,
                       SWP_SHOWWINDOW | SWP_NOZORDER);
  scroll_to_view_anchors();
  view_centre_known = false;
  rel_focus.reset();
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ViewWiden                        */
/*****************************************************************************/

void kdws_renderer::can_do_ViewWiden(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((buffer == NULL) ||
                 ((view_dims.size.x < image_dims.size.x) ||
                  (view_dims.size.y < image_dims.size.y)));
}

/*****************************************************************************/
/*                    kdws_renderer::menu_ViewShrink                         */
/*****************************************************************************/

void kdws_renderer::menu_ViewShrink() 
{
  RECT rect;
  kdu_coords target_size;

  calculate_rel_view_and_focus_params();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  window->GetWindowRect(&rect);
  target_size.x = rect.right-rect.left;
  target_size.y = rect.bottom-rect.top;
  target_size.x -= target_size.x / 3;
  target_size.y -= target_size.y / 3;
  if (target_size.x < 100)
    target_size.x = rect.right-rect.left;
  if (target_size.y < 100)
    target_size.y = rect.bottom-rect.top;
  window->SetWindowPos(NULL,0,0,target_size.x,target_size.y,
                       SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOZORDER);
  scroll_to_view_anchors();
  view_centre_known = false;
  rel_focus.reset();
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ViewShrink                       */
/*****************************************************************************/

void kdws_renderer::can_do_ViewShrink(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_ViewScaleX2                         */
/*****************************************************************************/

void kdws_renderer::menu_ViewScaleX2()
{
  if (pixel_scale != 2)
    {
      pixel_scale = 2;
      calculate_rel_view_and_focus_params();
      rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
      image_view->set_max_view_size(image_dims.size,pixel_scale);
    }
}

/*****************************************************************************/
/*                    kdws_renderer::menu_ViewScaleX1                        */
/*****************************************************************************/

void kdws_renderer::menu_ViewScaleX1()
{
  if (pixel_scale != 1)
    {
      pixel_scale = 1;
      image_view->set_max_view_size(image_dims.size,pixel_scale);
    }
}


/* ========================================================================= */
/*                 View Menu Command Handlers (navigation)                   */
/* ========================================================================= */

/*****************************************************************************/
/*                   kdws_renderer::menu_NavComponent1                       */
/*****************************************************************************/

void kdws_renderer::menu_NavComponent1() 
{
  if ((compositor == NULL) || (single_component_idx == 0))
    return;
  if (single_component_idx < 0)
    { // See if we can find a good codestream to go to
      kdu_coords point;
      if (this->enable_focus_box)
        {
          point.x = focus_box.pos.x + (focus_box.size.x>>1);
          point.y = focus_box.pos.y + (focus_box.size.y>>1);
        }
      else if (!view_dims.is_empty())
        {
          point.x = view_dims.pos.x + (view_dims.size.x>>1);
          point.y = view_dims.pos.y + (view_dims.size.y>>1);
        }
      int stream_idx;
      kdu_ilayer_ref ilyr;
      kdu_istream_ref istr;
      if ((ilyr = compositor->find_point(point)).exists() &&
          (istr = compositor->get_ilayer_stream(ilyr,0)).exists())
        compositor->get_istream_info(istr,stream_idx);
      else
        stream_idx = 0;
      set_codestream(stream_idx);
    }
  else
    {
      single_component_idx = 0;
      frame = NULL;
      calculate_rel_view_and_focus_params();
      invalidate_configuration();
      initialize_regions(false);
    }
}

/*****************************************************************************/
/*                  kdws_renderer::can_do_NavComponent1                      */
/*****************************************************************************/

void kdws_renderer::can_do_NavComponent1(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) && (single_component_idx != 0));
  pCmdUI->SetCheck((compositor != NULL) && (single_component_idx == 0));
}

/*****************************************************************************/
/*                  kdws_renderer::menu_NavComponentNext                     */
/*****************************************************************************/

void kdws_renderer::menu_NavComponentNext()
{
  if ((compositor == NULL) ||
      ((max_components >= 0) && (single_component_idx >= (max_components-1))))
    return;
  if (single_component_idx < 0)
    { // See if we can find a good codestream to go to
      kdu_coords point;
      if (this->enable_focus_box)
        {
          point.x = focus_box.pos.x + (focus_box.size.x>>1);
          point.y = focus_box.pos.y + (focus_box.size.y>>1);
        }
      else if (!view_dims.is_empty())
        {
          point.x = view_dims.pos.x + (view_dims.size.x>>1);
          point.y = view_dims.pos.y + (view_dims.size.y>>1);
        }
      int stream_idx;
      kdu_ilayer_ref ilyr;
      kdu_istream_ref istr;
      if ((ilyr = compositor->find_point(point)).exists() &&
          (istr = compositor->get_ilayer_stream(ilyr,0)).exists())
        compositor->get_istream_info(istr,stream_idx);
      else
        stream_idx = 0;
      set_codestream(stream_idx);
    }
  else
    {
      single_component_idx++;
      frame = NULL;
      calculate_rel_view_and_focus_params();
      invalidate_configuration();
      initialize_regions(false);
    }
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_NavComponentNext                    */
/*****************************************************************************/

void kdws_renderer::can_do_NavComponentNext(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) &&
                 ((max_components < 0) ||
                  (single_component_idx < (max_components-1))));
}

/*****************************************************************************/
/*                  kdws_renderer::menu_NavComponentPrev                     */
/*****************************************************************************/

void kdws_renderer::menu_NavComponentPrev() 
{
  if ((compositor == NULL) || (single_component_idx == 0))
    return;
  if (single_component_idx < 0)
    { // See if we can find a good codestream to go to
      kdu_coords point;
      if (this->enable_focus_box)
        {
          point.x = focus_box.pos.x + (focus_box.size.x>>1);
          point.y = focus_box.pos.y + (focus_box.size.y>>1);
        }
      else if (!view_dims.is_empty())
        {
          point.x = view_dims.pos.x + (view_dims.size.x>>1);
          point.y = view_dims.pos.y + (view_dims.size.y>>1);
        }
      int stream_idx;
      kdu_ilayer_ref ilyr;
      kdu_istream_ref istr;
      if ((ilyr = compositor->find_point(point)).exists() &&
          (istr = compositor->get_ilayer_stream(ilyr,0)).exists())
        compositor->get_istream_info(istr,stream_idx);
      else
        stream_idx = 0;
      set_codestream(stream_idx);
    }
  else
    {
      single_component_idx--;
      frame = NULL;
      calculate_rel_view_and_focus_params();
      invalidate_configuration();
      initialize_regions(false);
    }
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_NavComponentPrev                    */
/*****************************************************************************/

void kdws_renderer::can_do_NavComponentPrev(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) && (single_component_idx != 0));
}

/*****************************************************************************/
/*               kdws_renderer::menu_NavCompositingLayer                     */
/*****************************************************************************/

void
 kdws_renderer::menu_NavCompositingLayer() 
{
  if ((compositor == NULL) || (single_layer_idx >= 0))
    return;
  int layer_idx = 0;
  if (frame != NULL)
    { // See if we can find a good layer to go to
      kdu_coords point;
      if (this->enable_focus_box)
        {
          point.x = focus_box.pos.x + (focus_box.size.x>>1);
          point.y = focus_box.pos.y + (focus_box.size.y>>1);
        }
      else if (!view_dims.is_empty())
        {
          point.x = view_dims.pos.x + (view_dims.size.x>>1);
          point.y = view_dims.pos.y + (view_dims.size.y>>1);
        }
      int dcs_idx; bool opq; // Not actually used
      kdu_ilayer_ref ilyr = compositor->find_point(point);
      if (!(compositor->get_ilayer_info(ilyr,layer_idx,dcs_idx,opq) &&
            (layer_idx >- 0)))
        layer_idx = 0;
    }
  else if (mj2_in.exists() && (single_component_idx >= 0))
    { // Find layer based on the track to which the single component belongs
      kdu_uint32 trk;
      int frm, fld;
      if (mj2_in.find_stream(single_codestream_idx,trk,frm,fld) && (trk != 0))
        layer_idx = (int)(trk-1);
    }
  set_compositing_layer(layer_idx);
}

/*****************************************************************************/
/*              kdws_renderer::can_do_NavCompositingLayer                    */
/*****************************************************************************/

void kdws_renderer::can_do_NavCompositingLayer(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) && (single_layer_idx < 0));
  pCmdUI->SetCheck((compositor != NULL) && (single_layer_idx >= 0));
}

/*****************************************************************************/
/*                 kdws_renderer::menu_NavMultiComponent                     */
/*****************************************************************************/

void kdws_renderer::menu_NavMultiComponent() 
{
  if ((compositor == NULL) ||
      ((single_component_idx < 0) &&
       ((single_layer_idx < 0) || (num_frames == 0) || !jpx_in)))
    return;
  if ((num_frames == 0) || !jpx_in)
    menu_NavCompositingLayer();
  else
    {
      calculate_rel_view_and_focus_params();
      single_component_idx = -1;
      single_layer_idx = -1;
      frame_idx = 0;
      frame_start = frame_end = 0.0;
      frame = NULL; // Let `initialize_regions' try to find it.
      frame_iteration = 0;
    }
  invalidate_configuration();
  initialize_regions(false);
}

/*****************************************************************************/
/*                kdws_renderer::can_do_NavMultiComponent                    */
/*****************************************************************************/

void kdws_renderer::can_do_NavMultiComponent(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable((compositor != NULL) &&
                 ((single_component_idx >= 0) ||
                  ((single_layer_idx >= 0) && (num_frames != 0) &&
                    jpx_in.exists())));
  pCmdUI->SetCheck((compositor != NULL) &&
                   (single_component_idx < 0) &&
                   ((single_layer_idx < 0) || (num_frames == 0) || !jpx_in));
}

/*****************************************************************************/
/*                   kdws_renderer::menu_NavTrackNext                        */
/*****************************************************************************/

void kdws_renderer::menu_NavTrackNext()
{
  if ((compositor == NULL) || !mj2_in)
    return;
  if (single_layer_idx < 0)
    menu_NavCompositingLayer();
  else
    {
      single_layer_idx++;
      if ((max_compositing_layer_idx >= 0) &&
          (single_layer_idx > max_compositing_layer_idx))
        single_layer_idx = 0;
      invalidate_configuration();
      initialize_regions(false);
    }
}

/*****************************************************************************/
/*                  kdws_renderer::can_do_NavTrackNext                       */
/*****************************************************************************/

void kdws_renderer::can_do_NavTrackNext(CCmdUI *pCmdUI)
{
  pCmdUI->Enable((compositor != NULL) && mj2_in.exists());
}

/*****************************************************************************/
/*                   kdws_renderer::menu_NavImageNext                        */
/*****************************************************************************/

void
  kdws_renderer::menu_NavImageNext() 
{
  if (compositor == NULL)
    return;
  int current_frame_idx = frame_idx;
  if (single_component_idx >= 0)
    { // Advance to next code-stream
      if ((max_codestreams > 0) &&
          (single_codestream_idx >= (max_codestreams-1)))
        return;
      calculate_rel_view_and_focus_params();
      single_codestream_idx++;
      single_component_idx = 0;
    }
  else if ((single_layer_idx >= 0) && !mj2_in)
    { // Advance to next compositing layer
      if ((max_compositing_layer_idx >= 0) &&
          (single_layer_idx >= max_compositing_layer_idx))
        return;
      calculate_rel_view_and_focus_params();
      single_layer_idx++;
    }
  else if (single_layer_idx >= 0)
    { // Advance to next frame in track
      if ((num_frames > 0) && (frame_idx == (num_frames-1)))
        return;
      calculate_rel_view_and_focus_params();
      change_frame_idx(frame_idx+1);
    }
  else
    { // Advance to next frame in animation
      if (frame == NULL)
        return; // Still have not been able to open the composition box
      if ((num_frames > 0) && (frame_idx >= (num_frames-1)))
        return;
      calculate_rel_view_and_focus_params();
      change_frame_idx(frame_idx+1);
    }
  invalidate_configuration();
  initialize_regions(false);
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_NavImageNext                        */
/*****************************************************************************/

void
  kdws_renderer::can_do_NavImageNext(CCmdUI* pCmdUI) 
{
  if (single_component_idx >= 0)
    pCmdUI->Enable((compositor != NULL) &&
                   ((max_codestreams < 0) ||
                    (single_codestream_idx < (max_codestreams-1))));
  else if ((single_layer_idx >= 0) && !mj2_in)
    pCmdUI->Enable((compositor != NULL) &&
                   ((max_compositing_layer_idx < 0) ||
                    (single_layer_idx < max_compositing_layer_idx)));
  else if (single_layer_idx >= 0)
    pCmdUI->Enable((compositor != NULL) &&
                   ((num_frames < 0) || (frame_idx < (num_frames-1))));
  else
    pCmdUI->Enable((compositor != NULL) && (frame != NULL) &&
                   ((num_frames < 0) ||
                    (frame_idx < (num_frames-1))));
}

/*****************************************************************************/
/*                  kdws_renderer::menu_NavImagePrev                         */
/*****************************************************************************/

void
  kdws_renderer::menu_NavImagePrev() 
{
  if (compositor == NULL)
    return;
  if (single_component_idx >= 0)
    { // Go to previous code-stream
      if (single_codestream_idx == 0)
        return;
      calculate_rel_view_and_focus_params();
      single_codestream_idx--;
      single_component_idx = 0;
    }
  else if ((single_layer_idx >= 0) && !mj2_in)
    { // Go to previous compositing layer
      if (single_layer_idx == 0)
        return;
      calculate_rel_view_and_focus_params();
      single_layer_idx--;
    }
  else if (single_layer_idx >= 0)
    { // Go to previous frame in track
      if (frame_idx == 0)
        return;
      calculate_rel_view_and_focus_params();
      change_frame_idx(frame_idx-1);
    }
  else
    { // Go to previous frame
      if ((frame_idx == 0) || (frame == NULL))
        return;
      calculate_rel_view_and_focus_params();
      change_frame_idx(frame_idx-1);
    }
  invalidate_configuration();
  initialize_regions(false);
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_NavImagePrev                      */
/*****************************************************************************/

void
  kdws_renderer::can_do_NavImagePrev(CCmdUI* pCmdUI) 
{
  if (single_component_idx >= 0)
    pCmdUI->Enable((compositor != NULL) && (single_codestream_idx > 0));
  else if ((single_layer_idx >= 0) && !mj2_in)
    pCmdUI->Enable((compositor != NULL) && (single_layer_idx > 0));
  else if (single_layer_idx >= 0)
    pCmdUI->Enable((compositor != NULL) && (frame_idx > 0));
  else
    pCmdUI->Enable((compositor != NULL) && (frame != NULL) && (frame_idx > 0));
}


/* ========================================================================= */
/*                        Focus Menu Command Handlers                        */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdws_renderer::menu_FocusOff                        */
/*****************************************************************************/

void kdws_renderer::menu_FocusOff() 
{
  if (enable_focus_box)
    {
      kdu_dims empty_box;
      empty_box.size = kdu_coords(0,0);
      set_focus_box(empty_box);
    }
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_FocusOff                         */
/*****************************************************************************/

void kdws_renderer::can_do_FocusOff(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(enable_focus_box);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_FocusHighlighting                     */
/*****************************************************************************/

void kdws_renderer::menu_FocusHighlighting()
{
  highlight_focus = !highlight_focus;
  if (enable_focus_box && (compositor != NULL))
    compositor->invalidate_rect(view_dims);
}

/*****************************************************************************/
/*                kdws_renderer::can_do_FocusHighlighting                    */
/*****************************************************************************/

void kdws_renderer::can_do_FocusHighlighting(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(highlight_focus);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_FocusWiden                         */
/*****************************************************************************/

void kdws_renderer::menu_FocusWiden()
{
  if (!enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.x -= new_box.size.x >> 2;
  new_box.pos.y -= new_box.size.y >> 2;
  new_box.size.x += new_box.size.x >> 1;
  new_box.size.y += new_box.size.y >> 1;
  new_box &= view_dims;
  new_box.pos -= view_dims.pos;
  set_focus_box(new_box);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_FocusShrink                        */
/*****************************************************************************/

void kdws_renderer::menu_FocusShrink()
{
  kdu_dims new_box = focus_box;
  if (!enable_focus_box)
    new_box = view_dims;
  new_box.pos.x += new_box.size.x >> 3;
  new_box.pos.y += new_box.size.y >> 3;
  new_box.size.x -= new_box.size.x >> 2;
  new_box.size.y -= new_box.size.y >> 2;
  new_box.pos -= view_dims.pos;
  set_focus_box(new_box);
}

/*****************************************************************************/
/*                     kdws_renderer::menu_FocusLeft                         */
/*****************************************************************************/

void kdws_renderer::menu_FocusLeft()
{
  if (!enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.x -= 20;
  if (new_box.pos.x < view_dims.pos.x)
    {
      if (new_box.pos.x < image_dims.pos.x)
        new_box.pos.x = image_dims.pos.x;
      if (new_box.pos.x < view_dims.pos.x)
        set_hscroll_pos(new_box.pos.x-view_dims.pos.x,true);
    }
  new_box.pos -= view_dims.pos;
  set_focus_box(new_box);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_FocusRight                         */
/*****************************************************************************/

void kdws_renderer::menu_FocusRight()
{
  if (!enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.x += 20;
  int new_lim = new_box.pos.x + new_box.size.x;
  int view_lim = view_dims.pos.x + view_dims.size.x;
  if (new_lim > view_lim)
    {
      int image_lim = image_dims.pos.x + image_dims.size.x;
      if (new_lim > image_lim)
        {
          new_box.pos.x -= (new_lim-image_lim);
          new_lim = image_lim;
        }
      if (new_lim > view_lim)
        set_hscroll_pos(new_lim-view_lim,true);
    }
  new_box.pos -= view_dims.pos;
  set_focus_box(new_box);
}

/*****************************************************************************/
/*                       kdws_renderer::menu_FocusUp                         */
/*****************************************************************************/

void kdws_renderer::menu_FocusUp()
{
  if (!enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.y -= 20;
  if (new_box.pos.y < view_dims.pos.y)
    {
      if (new_box.pos.y < image_dims.pos.y)
        new_box.pos.y = image_dims.pos.y;
      if (new_box.pos.y < view_dims.pos.y)
        set_vscroll_pos(new_box.pos.y-view_dims.pos.y,true);
    }
  new_box.pos -= view_dims.pos;
  set_focus_box(new_box);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_FocusDown                          */
/*****************************************************************************/

void kdws_renderer::menu_FocusDown()
{
  if (!enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.y += 20;
  int new_lim = new_box.pos.y + new_box.size.y;
  int view_lim = view_dims.pos.y + view_dims.size.y;
  if (new_lim > view_lim)
    {
      int image_lim = image_dims.pos.y + image_dims.size.y;
      if (new_lim > image_lim)
        {
          new_box.pos.y -= (new_lim-image_lim);
          new_lim = image_lim;
        }
      if (new_lim > view_lim)
        set_vscroll_pos(new_lim-view_lim,true);
    }
  new_box.pos -= view_dims.pos;
  set_focus_box(new_box);
}


/* ========================================================================= */
/*                        Mode Menu Command Handlers                         */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdws_renderer::menu_ModeFast                        */
/*****************************************************************************/

void kdws_renderer::menu_ModeFast() 
{
  error_level = 0;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                     kdws_renderer::can_do_ModeFast                        */
/*****************************************************************************/

void kdws_renderer::can_do_ModeFast(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
  pCmdUI->SetCheck(error_level==0);
}

/*****************************************************************************/
/*                     kdws_renderer::menu_ModeFussy                         */
/*****************************************************************************/

void kdws_renderer::menu_ModeFussy() 
{
  error_level = 1;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                    kdws_renderer::can_do_ModeFussy                        */
/*****************************************************************************/

void kdws_renderer::can_do_ModeFussy(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
  pCmdUI->SetCheck(error_level==1);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_ModeResilient                       */
/*****************************************************************************/

void kdws_renderer::menu_ModeResilient() 
{
  error_level = 2;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_ModeResilient                       */
/*****************************************************************************/

void kdws_renderer::can_do_ModeResilient(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
  pCmdUI->SetCheck(error_level==2);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_ModeResilientSop                      */
/*****************************************************************************/

void kdws_renderer::menu_ModeResilientSop() 
{
  error_level = 3;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                kdws_renderer::can_do_ModeResilientSop                     */
/*****************************************************************************/

void kdws_renderer::can_do_ModeResilientSop(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
  pCmdUI->SetCheck(error_level==3);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_ModeSeekable                       */
/*****************************************************************************/

void kdws_renderer::menu_ModeSeekable() 
{
  allow_seeking = !allow_seeking;
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_ModeSeekable                      */
/*****************************************************************************/

void kdws_renderer::can_do_ModeSeekable(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(compositor == NULL); // No changes allowed when loaded
  pCmdUI->SetCheck(allow_seeking);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_ModeSinglethreaded                    */
/*****************************************************************************/

void kdws_renderer::menu_ModeSinglethreaded()
{
  if (thread_env.exists())
    {
      if (compositor != NULL)
        compositor->halt_processing();
      thread_env.destroy();
      if (compositor != NULL)
        compositor->set_thread_env(NULL,NULL);
    }
  num_threads = 0;
}

/*****************************************************************************/
/*                kdws_renderer::can_do_ModeSinglethreaded                   */
/*****************************************************************************/

void kdws_renderer::can_do_ModeSinglethreaded(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(num_threads==0);
}

/*****************************************************************************/
/*                  kdws_renderer::menu_ModeMultithreaded                    */
/*****************************************************************************/

void kdws_renderer::menu_ModeMultithreaded()
{
  if (thread_env.exists())
    return;
  num_threads = 1;
  thread_env.create();
  if (compositor != NULL)
    {
      compositor->halt_processing();
      compositor->set_thread_env(&thread_env,NULL);
    }
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_ModeMultithreaded                   */
/*****************************************************************************/

void kdws_renderer::can_do_ModeMultithreaded(CCmdUI *pCmdUI)
{
  if (pCmdUI->m_pMenu != NULL)
    {
      kdws_string menu_string(80);
      if (num_threads > 0)
        sprintf(menu_string,"Multi-Threaded (%d threads)",num_threads);
      else
        strcpy(menu_string,"Multi-Threaded");
      pCmdUI->m_pMenu->ModifyMenu(pCmdUI->m_nID,MF_BYCOMMAND,pCmdUI->m_nID,
                                  menu_string);
    }
  pCmdUI->SetCheck(num_threads > 0);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_ModeMorethreads                     */
/*****************************************************************************/

void kdws_renderer::menu_ModeMorethreads()
{
  if (compositor != NULL)
    compositor->halt_processing();
  num_threads++;
  if (thread_env.exists())
    thread_env.destroy();
  thread_env.create();
  for (int k=1; k < num_threads; k++)
    if (!thread_env.add_thread())
      num_threads = k;
  if (compositor != NULL)
    compositor->set_thread_env(&thread_env,NULL);
}

/*****************************************************************************/
/*                  kdws_renderer::can_do_ModeMorethreads                    */
/*****************************************************************************/

void kdws_renderer::can_do_ModeMorethreads(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(TRUE);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_ModeLessthreads                    */
/*****************************************************************************/

void kdws_renderer::menu_ModeLessthreads()
{
  if (num_threads == 0)
    return;
  if (compositor != NULL)
    compositor->halt_processing();
  num_threads--;
  if (thread_env.exists())
    thread_env.destroy();
  if (num_threads > 0)
    {
      thread_env.create();
      for (int k=1; k < num_threads; k++)
        if (!thread_env.add_thread())
          num_threads = k;
    }
  if (compositor != NULL)
    compositor->set_thread_env(&thread_env,NULL);
}

/*****************************************************************************/
/*                  kdws_renderer::can_do_ModeLessthreads                    */
/*****************************************************************************/

void kdws_renderer::can_do_ModeLessthreads(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(num_threads > 0);
}

/*****************************************************************************/
/*                 kdws_renderer::menu_ModeRecommendedThreads                */
/*****************************************************************************/

void kdws_renderer::menu_ModeRecommendedThreads()
{
  if (num_threads == num_recommended_threads)
    return;
  if (compositor != NULL)
    compositor->halt_processing();
  num_threads = num_recommended_threads;
  if (thread_env.exists())
    thread_env.destroy();
  if (num_threads > 0)
    {
      thread_env.create();
      for (int k=1; k < num_threads; k++)
        if (!thread_env.add_thread())
          num_threads = k;
    }
  if (compositor != NULL)
    compositor->set_thread_env(&thread_env,NULL);
}

/*****************************************************************************/
/*               kdws_renderer::can_do_ModeRecommendedThreads                */
/*****************************************************************************/

void kdws_renderer::can_do_ModeRecommendedThreads(CCmdUI *pCmdUI)
{
  if (pCmdUI->m_pMenu != NULL)
    {
      kdws_string menu_string(79);
      if (num_recommended_threads > 0)
        sprintf(menu_string,"Recommended: %d threads",num_recommended_threads);
      else
        strcpy(menu_string,"Recommended: single threaded");
      pCmdUI->m_pMenu->ModifyMenu(pCmdUI->m_nID,MF_BYCOMMAND,pCmdUI->m_nID,
                                  menu_string);
    }
}


/* ========================================================================= */
/*               Metadata Menu Command Handlers (metadata)                   */
/* ========================================================================= */

/*****************************************************************************/
/*                 kdws_renderer::menu_MetadataCatalog                       */
/*****************************************************************************/

void kdws_renderer::menu_MetadataCatalog()
{
  if ((catalog_source == NULL) && jpx_in.exists())
    window->create_metadata_catalog();
  else if (catalog_source != NULL)
    window->remove_metadata_catalog();
  if (catalog_source != NULL)
    catalog_source->tree_ctrl.SetFocus();
}

/*****************************************************************************/
/*                kdws_renderer::menu_MetadataSwapFocus                      */
/*****************************************************************************/

void kdws_renderer::menu_MetadataSwapFocus()
{
  if ((catalog_source == NULL) && jpx_in.exists())
    window->create_metadata_catalog();
  else if (catalog_source != NULL)
    {
      if (image_view->is_first_responder())
        catalog_source->tree_ctrl.SetFocus();
      else
        image_view->SetFocus();
    }
}

/*****************************************************************************/
/*                   kdws_renderer::menu_MetadataShow                        */
/*****************************************************************************/

void kdws_renderer::menu_MetadataShow() 
{
  if (metashow != NULL)
    return;
  RECT wnd_frame;  window->GetWindowRect(&wnd_frame);
  POINT preferred_location;
  preferred_location.x = wnd_frame.left;
  preferred_location.y = wnd_frame.bottom + 10;
  metashow = new kdws_metashow(this,preferred_location,window_identifier);
  if (jp2_family_in.exists())
    metashow->activate(&jp2_family_in,open_filename);
}

/*****************************************************************************/
/*                    kdws_renderer::menu_MetadataAdd                        */
/*****************************************************************************/

void
  kdws_renderer::menu_MetadataAdd() 
{
  if ((compositor == NULL) || (!jpx_in) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)))
    return;
  if (file_server == NULL)
    file_server = new kdws_file_services(open_file_pathname);
  if (editor == NULL)
    {
      RECT wnd_frame;  window->GetWindowRect(&wnd_frame);
      POINT preferred_location;
      preferred_location.x = wnd_frame.right + 10;
      preferred_location.y = wnd_frame.top;
      editor = new kdws_metadata_editor(&jpx_in,file_server,true,
                                        this,preferred_location,
                                        window_identifier,NULL);
    }

  kdu_dims initial_region = focus_box;
  kdu_istream_ref istr;
  if (enable_focus_box &&
      (istr=compositor->map_region(initial_region,kdu_istream_ref())).exists())
    { 
      kdu_ilayer_ref ilyr;
      int initial_codestream_idx=0, initial_layer_idx=-1, dcs_idx; bool opq;
      compositor->get_istream_info(istr,initial_codestream_idx,&ilyr);
      compositor->get_ilayer_info(ilyr,initial_layer_idx,dcs_idx,opq);
      editor->configure(initial_region,initial_codestream_idx,
                        initial_layer_idx,false);
    }
  else if (single_component_idx >= 0)
    editor->configure(kdu_dims(),single_codestream_idx,-1,false);
  else if (single_layer_idx >= 0)
    editor->configure(kdu_dims(),-1,single_layer_idx,false);
  else
    editor->configure(kdu_dims(),-1,-1,true);
}

/*****************************************************************************/
/*                   kdws_renderer::menu_MetadataEdit                        */
/*****************************************************************************/

void kdws_renderer::menu_MetadataEdit()
{
  if ((compositor == NULL) || !jpx_in.exists())
    return;
  if ((catalog_source != NULL) && !image_view->is_first_responder())
    {
      jpx_metanode metanode = catalog_source->get_unique_selected_metanode();
      if (metanode.exists())
        edit_metanode(metanode,true);
      else
        MessageBeep(MB_ICONEXCLAMATION);
    }
  else
    edit_metadata_at_point(NULL);
}

/*****************************************************************************/
/*                  kdws_renderer::menu_MetadataDelete                       */
/*****************************************************************************/

void kdws_renderer::menu_MetadataDelete()
{
  if ((catalog_source == NULL) || (editor != NULL) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)) ||
      image_view->is_first_responder())
    return;
  jpx_metanode metanode = catalog_source->get_unique_selected_metanode();
  if (!metanode)
    {
      MessageBeep(MB_ICONEXCLAMATION);
      return;
    }
  if (window->interrogate_user(
                  "Are you sure you want to delete this label/link from "
                  "the file's metadata?  This operation cannot be "
                  "undone.","Kdu_show: Really delete?",
                  MB_OKCANCEL | MB_ICONWARNING) == IDCANCEL)
    return;
  jpx_metanode parent = metanode.get_parent();
  if (parent.exists() && (parent.get_num_regions() > 0))
    {
      int num_siblings=0;
      parent.count_descendants(num_siblings);
      if ((num_siblings == 1) &&
          (window->interrogate_user(
                            "The node you are deleting belongs to a region "
                            "of interest (ROI) with no other descendants.  "
                            "Would you like to delete the region as well?",
                            "Kdu_show: Delete ROI as well?",
                            MB_YESNO | MB_ICONQUESTION) == IDYES))
        {
          metanode = parent;
          parent = jpx_metanode();
        }
    }
      
  metanode.delete_node();
  metadata_changed(false);
  if (parent.exists())
    catalog_source->select_matching_metanode(parent);
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_MetadataDelete                      */
/*****************************************************************************/

void kdws_renderer::can_do_MetadataDelete(CCmdUI* pCmdUI)
{
  pCmdUI->Enable((catalog_source != NULL) && (editor == NULL) &&
                 ((jpip_client == NULL) || !jpip_client->is_alive(-1)) &&
                 catalog_source->get_unique_selected_metanode().exists() &&
                 !image_view->is_first_responder());
}

/*****************************************************************************/
/*                 kdws_renderer::menu_MetadataCopyLabel                     */
/*****************************************************************************/

void kdws_renderer::menu_MetadataCopyLabel()
{
  if ((catalog_source == NULL) || image_view->is_first_responder())
    return;
  jpx_metanode node = catalog_source->get_selected_metanode();
  const char *label = NULL;
  jpx_metanode_link_type link_type;
  if (node.exists() &&
      ((label = node.get_label()) == NULL) &&
      (node = node.get_link(link_type)).exists())
    label = node.get_label();
  if (label == NULL)
    MessageBeep(MB_ICONEXCLAMATION);
  else
    catalog_source->paste_label(label);
}

/*****************************************************************************/
/*                kdws_renderer::can_do_MetadataCopyLabel                    */
/*****************************************************************************/

void kdws_renderer::can_do_MetadataCopyLabel(CCmdUI* pCmdUI)
{
  pCmdUI->Enable((catalog_source != NULL) &&
                 !image_view->is_first_responder());
}

/*****************************************************************************/
/*                 kdws_renderer::menu_MetadataCopyLink                      */
/*****************************************************************************/

void kdws_renderer::menu_MetadataCopyLink()
{
  if ((compositor == NULL) || !jpx_in)
    return;
  if ((catalog_source != NULL) && !image_view->is_first_responder())
    { // Link to something selected within the catalog
      jpx_metanode node = catalog_source->get_selected_metanode();
      if (node.exists())
        catalog_source->paste_link(node);
    }
  else
    { // Link to an image feature.
      kdu_coords point = image_view->get_last_mouse_point() + view_dims.pos;
      kdu_istream_ref istr;
      jpx_metanode roi_node =
        compositor->search_overlays(point,istr,0.1F);
      if (roi_node.exists())
        catalog_source->paste_link(roi_node);
    }
}

/*****************************************************************************/
/*                    kdws_renderer::menu_MetadataCut                        */
/*****************************************************************************/

void kdws_renderer::menu_MetadataCut()
{
  if ((catalog_source == NULL) || image_view->is_first_responder())
    return;
  jpx_metanode node = catalog_source->get_unique_selected_metanode();
  if (node.exists())
    catalog_source->paste_node_to_cut(node);
}

/*****************************************************************************/
/*                   kdws_renderer::can_do_MetadataCut                       */
/*****************************************************************************/

void kdws_renderer::can_do_MetadataCut(CCmdUI* pCmdUI)
{
  pCmdUI->Enable((catalog_source != NULL) &&
                 !image_view->is_first_responder());
}

/*****************************************************************************/
/*                 kdws_renderer::menu_MetadataPasteNew                      */
/*****************************************************************************/

void kdws_renderer::menu_MetadataPasteNew()
{
  if ((compositor == NULL) || (catalog_source == NULL) || (editor != NULL) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)))
    return;
  jpx_metanode parent = catalog_source->get_unique_selected_metanode();
  if (!parent.exists())
    {
      MessageBeep(MB_ICONEXCLAMATION);
      return;
    }
  const char *label = catalog_source->get_pasted_label();
  jpx_metanode link = catalog_source->get_pasted_link();
  jpx_metanode node_to_cut = catalog_source->get_pasted_node_to_cut();
  
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  jpx_metanode_link_type reverse_link_type = JPX_METANODE_LINK_NONE;
  if (link.exists())
    {
      int response =
        window->interrogate_user(
                   "What type of link would you like to add?\n"
                   "  \"Grouping links\" can have their own "
                   "descendants, all of which can be understood as "
                   "members of the same group.\n"
                   "  \"Alternate child\" is the most natural type of "
                   "link -- it suggests that the target of the link "
                   "and all its descendants could be considered "
                   "descendants of the node within which the link is "
                   "found.\n"
                   "  \"Alternate parent\" suggests that the target "
                   "of the link could be considered a parent of the "
                   "node within which the link is found.\n"
                   "  Click \"YES\" for an Alternate Child link, \"NO\" for "
                   "the other choices.\n",
                   "Kdu_show: Alternate Child link?",
                   MB_YESNOCANCEL | MB_ICONQUESTION);
      if (response == IDCANCEL)
        return;
      if (response == IDYES)
        link_type = JPX_ALTERNATE_CHILD_LINK;
      else if ((response =
                window->interrogate_user(
                           "Would you like an Alternate Parent link?  If you "
                           "click \"NO\", a Grouping link will "
                           "be created.",
                           "Kdu_show: Alternate Parent link?",
                           MB_YESNOCANCEL | MB_ICONQUESTION)) == IDCANCEL)
        return;
      else if (response == IDYES)
        link_type = JPX_ALTERNATE_PARENT_LINK;
      else
        link_type = JPX_GROUPING_LINK;

      if (link_type == JPX_ALTERNATE_CHILD_LINK)
        {
          int user_response =
            window->interrogate_user(
                             "Would you like to create an \"Alternate "
                             "parent\" link within the link target to "
                             "provide bi-directional linking for this "
                             "\"Alternate child\" link?",
                             "Kdu_show: Two-way link?",
                             MB_YESNOCANCEL | MB_ICONQUESTION);
          if (user_response == IDCANCEL)
            return;
          if (user_response == IDYES)
            reverse_link_type = JPX_ALTERNATE_PARENT_LINK;
        }
      else if (link_type == JPX_ALTERNATE_PARENT_LINK)
        {
          int user_response =
            window->interrogate_user(
                             "Would you like to create an \"Alternate "
                             "child\" link within the link target to "
                             "provide bi-directional linking for this "
                             "\"Alternate parent\" link?",
                             "Kdu_show: Two-way link?",
                             MB_YESNOCANCEL | MB_ICONQUESTION);
          if (user_response == IDCANCEL)
            return;
          if (user_response == IDYES)
            reverse_link_type = JPX_ALTERNATE_CHILD_LINK;
        }
    }
  
  try {
    jpx_metanode new_node;
    if (label != NULL)
      new_node = parent.add_label(label);
    else if (node_to_cut.exists())
      {
        if (node_to_cut.change_parent(parent))
          new_node = node_to_cut;
      }
    else if (link.exists())
      new_node = parent.add_link(link,link_type);
    if (new_node.exists())
      {
        if (reverse_link_type != JPX_METANODE_LINK_NONE)
          link.add_link(parent,reverse_link_type);
        metadata_changed(true);
        catalog_source->select_matching_metanode(new_node);
      }
  }
  catch (int)
  {}
}

/*****************************************************************************/
/*                kdws_renderer::can_do_MetadataPasteNew                     */
/*****************************************************************************/

void kdws_renderer::can_do_MetadataPasteNew(CCmdUI* pCmdUI)
{
  pCmdUI->Enable((catalog_source != NULL) && (editor == NULL) &&
                 ((jpip_client == NULL) || !jpip_client->is_alive(-1)) &&
                 ((catalog_source->get_pasted_label() != NULL) ||
                  catalog_source->get_pasted_link().exists() ||
                  catalog_source->get_pasted_node_to_cut().exists()));
}

/* ========================================================================= */
/*                 Metadata Menu Command Handlers (overlays)                 */
/* ========================================================================= */

/*****************************************************************************/
/*                   kdws_renderer::menu_OverlayEnable                       */
/*****************************************************************************/

void kdws_renderer::menu_OverlayEnable() 
{
  if ((compositor == NULL) || !jpx_in)
    return;
  overlays_enabled = !overlays_enabled;
  if (!overlays_enabled)
    next_overlay_flash_time = -1.0;
  configure_overlays();
}

/*****************************************************************************/
/*                   kds_renderer::menu_OverlayFlashing                      */
/*****************************************************************************/

void kdws_renderer::menu_OverlayFlashing()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled) || in_playmode)
    return;
  overlay_flashing_state = (overlay_flashing_state)?0:1;
  next_overlay_flash_time = -1.0;
  configure_overlays();
}

/*****************************************************************************/
/*                    kds_renderer::menu_OverlayToggle                       */
/*****************************************************************************/

void kdws_renderer::menu_OverlayToggle()
{
  if ((compositor == NULL) || !jpx_in)
    return;
  if (!overlays_enabled)
    {
      overlays_enabled = true;
      overlay_flashing_state = 1;
    }
  else if (overlay_flashing_state)
    overlay_flashing_state = 0;
  else
    overlays_enabled = false;
  overlay_highlight_state = 0;
  configure_overlays();
}

/*****************************************************************************/
/*                   kds_renderer::menu_OverlayRestrict                      */
/*****************************************************************************/

void kdws_renderer::menu_OverlayRestrict()
{
  jpx_metanode new_restriction;
  if ((catalog_source != NULL) && !image_view->is_first_responder())
    {
      jpx_metanode metanode = catalog_source->get_selected_metanode();
      if (metanode.exists())
        {
          jpx_metanode_link_type link_type;
          jpx_metanode link_target = metanode.get_link(link_type);
          if (link_target.exists())
            new_restriction = link_target;
          else
            new_restriction = metanode;
        }
      if (!new_restriction)
        new_restriction = overlay_restriction;
      else
        {
          if (new_restriction == last_selected_overlay_restriction)
            new_restriction = jpx_metanode(); // Cancel restrictions
          last_selected_overlay_restriction = new_restriction;
        }
    }
  if ((new_restriction != overlay_restriction) || !overlays_enabled)
    {
      overlay_restriction = new_restriction;
      overlays_enabled = true;
      configure_overlays();
    }
}

/*****************************************************************************/
/*                 kdws_renderer::can_do_OverlayRestrict                     */
/*****************************************************************************/

void kdws_renderer::can_do_OverlayRestrict(CCmdUI* pCmdUI)
{
  pCmdUI->SetCheck(overlay_restriction.exists());
  pCmdUI->Enable(overlay_restriction.exists() ||
                 ((catalog_source != NULL) &&
                  !image_view->is_first_responder()));
}

/*****************************************************************************/
/*                    kds_renderer::menu_OverlayLighter                      */
/*****************************************************************************/

void kdws_renderer::menu_OverlayLighter()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled))
    return;
  overlay_nominal_intensity *= 1.0F/1.5F;
  if (overlay_nominal_intensity < 0.25F)
    overlay_nominal_intensity = 0.25F;
  configure_overlays();
}

/*****************************************************************************/
/*                    kds_renderer::menu_OverlayHeavier                      */
/*****************************************************************************/

void kdws_renderer::menu_OverlayHeavier()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled))
    return;
  overlay_nominal_intensity *= 1.5F;
  if (overlay_nominal_intensity > 4.0F)
    overlay_nominal_intensity = 4.0F;
  configure_overlays();
}

/*****************************************************************************/
/*                  kds_renderer::menu_OverlayDoubleSize                     */
/*****************************************************************************/

void kdws_renderer::menu_OverlayDoubleSize()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled) ||
      (overlay_size_threshold & 0x40000000))
    return;
  overlay_size_threshold <<= 1;
  configure_overlays();
}

/*****************************************************************************/
/*                   kds_renderer::menu_OverlayHalveSize                     */
/*****************************************************************************/

void kdws_renderer::menu_OverlayHalveSize()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled) ||
      (overlay_size_threshold == 1))
    return;
  overlay_size_threshold >>= 1;
  configure_overlays();
}

/* ========================================================================= */
/*                        Play Menu Command Handlers                         */
/* ========================================================================= */

/*****************************************************************************/
/*                      kds_renderer::menu_PlayNative                        */
/*****************************************************************************/

void kdws_renderer::menu_PlayNative()
{
  if (!source_supports_playmode())
    return;
  if (use_native_timing)
    return;
  use_native_timing = true;
  if (in_playmode)
    {
      stop_playmode();
      start_playmode(playmode_reverse);
    }
}

/*****************************************************************************/
/*                     kds_renderer::can_do_PlayNative                       */
/*****************************************************************************/

void kdws_renderer::can_do_PlayNative(CCmdUI* pCmdUI)
{
  if (!source_supports_playmode())
    pCmdUI->Enable(false);
  else
    {
      kdws_string menu_string(80);
      if ((native_playback_multiplier > 1.01F) ||
          (native_playback_multiplier < 0.99F))
        sprintf(menu_string,"Native play rate x %5.2g",
                1.0F/native_playback_multiplier);
      else
        strcpy(menu_string,"Native play rate");
      pCmdUI->SetText(menu_string);
      pCmdUI->SetCheck(use_native_timing);
      pCmdUI->Enable(true);
    }
}

/*****************************************************************************/
/*                     kds_renderer::menu_PlayCustom                         */
/*****************************************************************************/

void kdws_renderer::menu_PlayCustom()
{
  if (!source_supports_playmode())
    return;
  if (!use_native_timing)
    return;
  use_native_timing = false;
  if (in_playmode)
    {
      stop_playmode();
      start_playmode(playmode_reverse);
    }
}

/*****************************************************************************/
/*                     kds_renderer::can_do_PlayCustom                       */
/*****************************************************************************/

void kdws_renderer::can_do_PlayCustom(CCmdUI* pCmdUI)
{
  if (!source_supports_playmode())
    pCmdUI->Enable(false);
  else
    {
      kdws_string menu_string(79);
      sprintf(menu_string,"Custom frame rate = %5.2g fps",
              1.0F/custom_frame_interval);
      pCmdUI->SetText(menu_string);
      pCmdUI->SetCheck(!use_native_timing);
      pCmdUI->Enable(true);
    }
}
  
/*****************************************************************************/
/*                  kds_renderer::menu_PlayFrameRateUp                       */
/*****************************************************************************/

void kdws_renderer::menu_PlayFrameRateUp()
{
  if (!source_supports_playmode())
    return;
  if (use_native_timing)
    native_playback_multiplier *= 1.0F / (float) sqrt(2.0);
  else
    {
      float frame_rate = 1.0F / custom_frame_interval;
      if (frame_rate > 4.5F)
        frame_rate += 5.0F;
      else if (frame_rate > 0.9F)
        {
          if ((frame_rate += 1.0F) > 5.0F)
            frame_rate = 5.0F;
        }
      else if ((frame_rate *= (float) sqrt(2.0)) > 1.0F)
        frame_rate = 1.0F;
      custom_frame_interval = 1.0F / frame_rate;
    }
  if (in_playmode)
    {
      stop_playmode();
      start_playmode(playmode_reverse);
    }
}

/*****************************************************************************/
/*                 kds_renderer::menu_PlayFrameRateDown                      */
/*****************************************************************************/

void kdws_renderer::menu_PlayFrameRateDown()
{
  if (!source_supports_playmode())
    return;
  if (use_native_timing)
    native_playback_multiplier *= (float) sqrt(2.0); 
  else
    {
      float frame_rate = 1.0F / custom_frame_interval;
      if (frame_rate < 1.2F)
        frame_rate *= 1.0F / (float) sqrt(2.0);
      else if (frame_rate < 5.5F)
        {
          if ((frame_rate -= 1.0F) < 1.0F)
            frame_rate = 1.0F;
        }
      else if ((frame_rate -= 5.0F) < 5.0F)
        frame_rate = 5.0F;
      custom_frame_interval = 1.0F / frame_rate;
    }
  if (in_playmode)
    {
      stop_playmode();
      start_playmode(playmode_reverse);
    }
}
