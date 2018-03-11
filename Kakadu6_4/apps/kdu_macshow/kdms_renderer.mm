/*****************************************************************************/
// File: kdms_renderer.mm [scope = APPS/MACSHOW]
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
   Implements the main image management and rendering object, `kdms_renderer',
an instance of which is associated with each `kdms_window' object.
******************************************************************************/
#import "kdms_renderer.h"
#import "kdms_window.h"
#import "kdms_properties.h"
#import "kdms_metadata_editor.h"
#import "kdms_catalog.h"
#include "kdu_arch.h"

#ifdef __SSE__
#  ifndef KDU_X86_INTRINSICS
#    define KDU_X86_INTRINSICS
#  endif
#endif

#ifdef KDU_X86_INTRINSICS
#  include <tmmintrin.h>
#endif // KDU_X86_INTRINSICS

#define KDMS_OVERLAY_DEFAULT_INTENSITY 0.7F
#define KDMS_OVERLAY_FLASHING_STATES 3 // Num distinct flashing states
static float kdms_overlay_flash_factor[KDMS_OVERLAY_FLASHING_STATES] =
  {-0.6F,1.2F,0.6F};
static float kdms_overlay_flash_dwell[KDMS_OVERLAY_FLASHING_STATES] =
  {1.0F,0.5F,0.5F};
  /* Notes: when metadata overlay painting is enabled, they can either be
     displayed statically, using the nominal intensity (starts out with
     the default value given above, but can be adjusted up and down by the
     user), or dynamically (flashing).  In the dynamic case, the current
     value of the nominal intensity (as mentioned, this can be adjusted up
     or down by the user) is additionally scaled by a value which depends
     upon the current flashing state.  The flashing state machine moves
     cyclically through `KDMS_OVERLAY_FLASHING_STATES' states.  Each
     state is characterized by a scaling factor (mentioned above) and a
     dwelling period (measured in seconds); these are stored in the
     `kdms_overlay_flash_factor' and `kdms_overlay_flash_dwell' arrays. */
#define KDMS_OVERLAY_HIGHLIGHT_STATES 5
static float kdms_overlay_highlight_factor[KDMS_OVERLAY_HIGHLIGHT_STATES] =
  {2.0F,-2.0F,2.0F,-2.0F,2.0F};
static float kdms_overlay_highlight_dwell[KDMS_OVERLAY_HIGHLIGHT_STATES] =
  {0.125F,0.125F,0.125F,0.125F,2.0F};
  /* Notes: the above arrays play a similar role to `kdms_overlay_flash_factor'
     and `kdms_overlay_flash_dwell', but they apply to the temporary flashing
     of overlay information for a specific metadata node, in response to
     calls to `kdms_renderer::flash_imagery_for_metanode'. */

/*===========================================================================*/
/*                             EXTERNAL FUNCTIONS                            */
/*===========================================================================*/

/*****************************************************************************/
/* EXTERN                     kdms_utf8_to_unicode                           */
/*****************************************************************************/

int kdms_utf8_to_unicode(const char *utf8, kdu_uint16 unicode[], int max_chars)
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


/*===========================================================================*/
/*                             INTERNAL FUNCTIONS                            */
/*===========================================================================*/

/*****************************************************************************/
/* STATIC                         copy_tile                                  */
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
                        num_bytes+=(out->pass_lengths[z]=in->pass_lengths[z]);
                        out->pass_slopes[z] = in->pass_slopes[z];
                      }
                    if (out->max_bytes < num_bytes)
                      out->set_max_bytes(num_bytes,false);
                    memcpy(out->byte_buffer,in->byte_buffer,(size_t)num_bytes);
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
  write_metadata_box_from_file(jp2_output_box *container, kdms_file *file)
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
                        kdms_file_services *file_server)
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
  kdms_file *file;
  if (node.get_delayed(i_param,addr_param) &&
      (addr_param == (void *)file_server) &&
      ((file = file_server->find_file(i_param)) != NULL))
    {
      tgt.open_next(box_type);
      write_metadata_box_from_file(&tgt,file);
      tgt.close();
    }
}

/*****************************************************************************/
/* STATIC                 look_for_catalog_descendants                       */
/*****************************************************************************/

static bool
  look_for_catalog_descendants(jpx_metanode node)
 /* Returns true if `node' or any of its descendants appears in the catalog
    in any form other than as a grouping link. */
{
  if (node.get_label() != NULL)
    return true;
  /*
  jpx_metanode_link_type link_type;
  jpx_metanode target = node.get_link(link_type);
  if (target.exists() && (link_type != JPX_GROUPING_LINK))
    return true;
  */
  
  jpx_metanode child;
  for (int d=0; (child=node.get_descendant(d)).exists(); d++)
    if (look_for_catalog_descendants(child))
      return true;
  return false;
}


/*===========================================================================*/
/*                                 kdms_file                                 */
/*===========================================================================*/

/*****************************************************************************/
/*                           kdms_file::kdms_file                            */
/*****************************************************************************/

kdms_file::kdms_file(kdms_file_services *owner)
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
/*                           kdms_file::~kdms_file                           */
/*****************************************************************************/

kdms_file::~kdms_file()
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
/*                             kdms_file::release                            */
/*****************************************************************************/

void kdms_file::release()
{
  retain_count--;
  if (retain_count == 0)
    delete this;
}

/*****************************************************************************/
/*                       kdms_file::create_if_necessary                      */
/*****************************************************************************/

bool kdms_file::create_if_necessary(const char *initializer)
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
/*                             kdms_file_services                            */
/*===========================================================================*/

/*****************************************************************************/
/*                   kdms_file_services::kdms_file_services                  */
/*****************************************************************************/

kdms_file_services::kdms_file_services(const char *source_pathname)
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
/*                   kdms_file_services::~kdms_file_services                 */
/*****************************************************************************/

kdms_file_services::~kdms_file_services()
{
  while (head != NULL)
    delete head;
  if (base_pathname != NULL)
    delete[] base_pathname;
  kdms_file_editor *editor;
  while ((editor=editors) != NULL)
    {
      editors = editor->next;
      delete editor;
    }
}

/*****************************************************************************/
/*                 kdms_file_services::find_new_base_pathname                */
/*****************************************************************************/

bool kdms_file_services::find_new_base_pathname()
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
/*                 kdms_file_services::confirm_base_pathname                 */
/*****************************************************************************/

void kdms_file_services::confirm_base_pathname()
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
/*                    kdms_file_services::retain_known_file                  */
/*****************************************************************************/

kdms_file *kdms_file_services::retain_known_file(const char *pathname)
{
  kdms_file *file;
  for (file=head; file != NULL; file=file->next)
    if (strcmp(file->pathname,pathname) == 0)
      break;
  if (file == NULL)
    {
      file = new kdms_file(this);
      file->pathname = new char[strlen(pathname)+1];
      strcpy(file->pathname,pathname);
    }
  file->is_temporary = false;
  file->retain();
  return file;
}

/*****************************************************************************/
/*                     kdms_file_services::retain_tmp_file                   */
/*****************************************************************************/

kdms_file *kdms_file_services::retain_tmp_file(const char *suffix)
{
  if (!base_pathname_confirmed)
    confirm_base_pathname();
  kdms_file *file = new kdms_file(this);
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
/*                        kdms_file_services::find_file                      */
/*****************************************************************************/

kdms_file *kdms_file_services::find_file(int identifier)
{
  kdms_file *scan;
  for (scan=head; scan != NULL; scan=scan->next)
    if (scan->unique_id == identifier)
      return scan;
  return NULL;
}

/*****************************************************************************/
/*               kdms_file_services::get_editor_for_doc_type                 */
/*****************************************************************************/

kdms_file_editor *
  kdms_file_services::get_editor_for_doc_type(const char *doc_suffix,
                                              int which)
{
  kdms_file_editor *scan;
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
  // we can add one.
  FSRef fs_ref;
  CFStringRef extension_string =
    CFStringCreateWithCString(NULL,doc_suffix,kCFStringEncodingASCII);
  OSStatus os_status =
    LSGetApplicationForInfo(kLSUnknownType,kLSUnknownCreator,extension_string,
                            kLSRolesAll,&fs_ref,NULL);
  CFRelease(extension_string);
  if (os_status == 0)
    {
      char pathname[512]; *pathname = '\0';
      os_status = FSRefMakePath(&fs_ref,(UInt8 *) pathname,511);
      if ((os_status == 0) && (*pathname != '\0'))
        add_editor_for_doc_type(doc_suffix,pathname);
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
/*               kdms_file_services::add_editor_for_doc_type                 */
/*****************************************************************************/

kdms_file_editor *
  kdms_file_services::add_editor_for_doc_type(const char *doc_suffix,
                                              const char *app_pathname)
{
  kdms_file_editor *scan, *prev;
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
  
  FSRef fs_ref;
  OSStatus os_status = FSPathMakeRef((UInt8 *)app_pathname,&fs_ref,NULL);
  if (os_status == 0)
    {
      scan = new kdms_file_editor;
      scan->doc_suffix = new char[strlen(doc_suffix)+1];
      strcpy(scan->doc_suffix,doc_suffix);
      scan->app_pathname = new char[strlen(app_pathname)+1];
      strcpy(scan->app_pathname,app_pathname);
      if ((scan->app_name = strrchr(scan->app_pathname,'/')) != NULL)
        scan->app_name++;
      else
        scan->app_name = scan->app_pathname;
      scan->fs_ref = fs_ref;
      scan->next = editors;
      editors = scan;
      return scan;
    }
  return NULL;
}


/*===========================================================================*/
/*                           kdms_region_compositor                          */
/*===========================================================================*/

/*****************************************************************************/
/*                  kdms_region_compositor::allocate_buffer                  */
/*****************************************************************************/

kdu_compositor_buf *
  kdms_region_compositor::allocate_buffer(kdu_coords min_size,
                                          kdu_coords &actual_size,
                                          bool read_access_required)
{
  if (min_size.x < 1) min_size.x = 1;
  if (min_size.y < 1) min_size.y = 1;
  actual_size = min_size;
  
  kdms_compositor_buf *prev=NULL, *elt=NULL;
  
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
      elt = new kdms_compositor_buf;
      need_init = true;
    }
  elt->next = active_list;
  active_list = elt;
  elt->set_read_accessibility(read_access_required);
  if (need_init)
    {
      elt->size = actual_size;
      elt->row_gap = actual_size.x + ((-actual_size.x) & 3); // 16-byte align
      size_t buffer_pels = elt->size.y*elt->row_gap + 3; // Allow for realign
      elt->handle = new(std::nothrow) kdu_uint32[buffer_pels];
      if (elt->handle == NULL)
        { // See if we can free up some memory by culling unused layers
          cull_inactive_ilayers(0);
          elt->handle = new kdu_uint32[buffer_pels];
        }
      int align_offset = ((-_addr_to_kdu_int32(elt->handle)) & 0x0F) >> 2;
      elt->buf = elt->handle + align_offset;
    }
  return elt;
}

/*****************************************************************************/
/*                    kdms_region_compositor::delete_buffer                  */
/*****************************************************************************/

void
  kdms_region_compositor::delete_buffer(kdu_compositor_buf *_buffer)
{
  kdms_compositor_buf *buffer = (kdms_compositor_buf *) _buffer;
  kdms_compositor_buf *scan, *prev;
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


/*===========================================================================*/
/*                             kdms_data_provider                            */
/*===========================================================================*/

/*****************************************************************************/
/*                   kdms_data_provider::kdms_data_provider                  */
/*****************************************************************************/

kdms_data_provider::kdms_data_provider()
{
  provider_ref = NULL;
  buf = NULL;
  tgt_size = 0;
  display_with_focus = false;
  rows_above_focus = rows_below_focus = focus_rows = 0;
  cols_left_of_focus = cols_right_of_focus = focus_cols = 0;
  for (int i=0; i < 256; i++)
    {
      foreground_lut[i] = (kdu_byte)(56 + ((i*200) >> 8));
      background_lut[i] = (kdu_byte)((i*200) >> 8);
    }
}

/*****************************************************************************/
/*                  kdms_data_provider::~kdms_data_provider                  */
/*****************************************************************************/

kdms_data_provider::~kdms_data_provider()
{
  if (provider_ref != NULL)
    CGDataProviderRelease(provider_ref);
}

/*****************************************************************************/
/*                          kdms_data_provider::init                         */
/*****************************************************************************/

CGDataProviderRef
  kdms_data_provider::init(kdu_coords size, kdu_uint32 *buf, int row_gap,
                           bool display_with_focus, kdu_dims focus_box)
{
  off_t new_size = (off_t)(size.x*size.y);
  if ((provider_ref == NULL) || (tgt_size != new_size))
    { // Need to allocate a new CGDataProvider resource
      CGDataProviderDirectAccessCallbacks callbacks;
      callbacks.getBytePointer = NULL;
      callbacks.releaseBytePointer = NULL;
      callbacks.releaseProvider = NULL;
      callbacks.getBytes = kdms_data_provider::get_bytes_callback;
      if (provider_ref != NULL)
        CGDataProviderRelease(provider_ref);
      provider_ref =
        CGDataProviderCreateDirectAccess(this,(new_size<<2),&callbacks);
    }
  tgt_size = new_size;
  tgt_width = size.x;
  tgt_height = size.y;
  this->buf = buf;
  this->buf_row_gap = row_gap;
  if (!(this->display_with_focus = display_with_focus))
    return provider_ref;
  
  kdu_dims painted_region; painted_region.size = size;
  focus_box &= painted_region;
  if (focus_box.is_empty())
    { // Region to be painted is entirely in the background
      rows_above_focus = tgt_height; focus_rows = rows_below_focus = 0;
      cols_left_of_focus = tgt_width; focus_cols = cols_right_of_focus = 0;
      return provider_ref;
    }
  rows_above_focus = focus_box.pos.y;
  focus_rows = focus_box.size.y;
  rows_below_focus = tgt_height - rows_above_focus - focus_rows;
  assert((rows_above_focus >= 0) && (focus_rows >= 0) &&
         (rows_below_focus >= 0));
  cols_left_of_focus = focus_box.pos.x;
  focus_cols = focus_box.size.x;
  cols_right_of_focus = tgt_width - cols_left_of_focus - focus_cols;
  assert((cols_left_of_focus >= 0) && (focus_cols >= 0) &&
         (cols_right_of_focus >= 0));
  return provider_ref;
}

/*****************************************************************************/
/*                   kdms_data_provider::get_bytes_callback                  */
/*****************************************************************************/

size_t
  kdms_data_provider::get_bytes_callback(void *info, void *buffer,
                                         size_t offset,
                                         size_t original_byte_count)
{
  kdms_data_provider *obj = (kdms_data_provider *) info;
  assert(((offset & 3) == 0) &&
         ((original_byte_count & 3) == 0)); // While number of pels
  offset >>= 2; // Convert to a pixel offset
  int pels_left_to_transfer = (int)(original_byte_count >> 2);
  int tgt_row = (int)(offset / obj->tgt_width);
  int tgt_col = (int)(offset - tgt_row*obj->tgt_width);
  if (!obj->display_with_focus)
    { // No need for tone mapping curves
      kdu_uint32 *src = obj->buf + tgt_row*obj->buf_row_gap + tgt_col;
      kdu_uint32 *dst = (kdu_uint32 *) buffer;
#ifdef KDU_X86_INTRINSICS
      bool can_use_simd_speedups = (kdu_mmx_level >= 4); // Need SSSE3
      kdu_byte shuffle_control[16] = {3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12};
      __asm__ volatile ("movdqu %0,%%xmm0" : : "m" (shuffle_control[0]));
#endif // KDU_X86_INTRINSICS
      while (tgt_row < obj->tgt_height)
        {
          int copy_pels = obj->tgt_width-tgt_col;
          if (copy_pels > pels_left_to_transfer)
            copy_pels = pels_left_to_transfer;
          if (copy_pels == 0)
            return original_byte_count;
          pels_left_to_transfer -= copy_pels;
          int c = copy_pels;
          
#       ifdef KDU_X86_INTRINSICS
          if (can_use_simd_speedups &&
              !((_addr_to_kdu_int32(src) | _addr_to_kdu_int32(dst)) & 15))
            { // `src' and `dst' are both 16-byte aligned
              for (; c > 7; c-=8, src+=8, dst+=8)
                { // The following code uses the SSSE3 PSHUFB instruction with
                  // source operand XMM0 and destination operand XMM1.  It
                  // first loads XMM1 with *src and finishes by writing XMM1
                  // to *dst.  The slightly unrolled loop uses XMM2 as well.
                  __asm__ volatile ("movdqa %2,%%xmm1\n\t"
                                    ".byte 0x66; "
                                    ".byte 0x0F; "
                                    ".byte 0x38; "
                                    ".byte 0x00; "
                                    ".byte 0xC8 \n\t"
                                    "movdqa %%xmm1,%0\n\t"
                                    "movdqa %3,%%xmm2\n\t"
                                    ".byte 0x66; "
                                    ".byte 0x0F; "
                                    ".byte 0x38; "
                                    ".byte 0x00; "
                                    ".byte 0xD0 \n\t"
                                    "movdqa %%xmm2,%1"
                                    : "=m" (*dst), "=m" (*(dst+4))
                                    : "m" (*src), "m" (*(src+4))
                                    : "%xmm1", "%xmm2");
                }
            }
          else if (can_use_simd_speedups &&
                   !((_addr_to_kdu_int32(src) | _addr_to_kdu_int32(dst)) & 7))
            { // `src' and `dst' are both 8-byte aligned
              __asm__ volatile ("movdq2q %xmm0,%mm0");
              for (; c > 3; c-=4, src+=4, dst+=4)
                { // The following code uses the SSSE3 PSHUFB instruction with
                  // source operand MM0 and destination operand MM1.  It
                  // first loads MM1 with *src and finishes by writing MM1
                  // to *dst.
                  __asm__ volatile ("movq %2,%%mm1\n\t"
                                    ".byte 0x0F; "
                                    ".byte 0x38; "
                                    ".byte 0x00; "
                                    ".byte 0xC8 \n\t"
                                    "movq %%mm1,%0\n\t"
                                    "movq %3,%%mm2\n\t"
                                    ".byte 0x0F; "
                                    ".byte 0x38; "
                                    ".byte 0x00; "
                                    ".byte 0xD0 \n\t"
                                    "movq %%mm2,%1"
                                    : "=m" (*dst), "=m" (*(dst+2))
                                    : "m" (*src), "m" (*(src+2))
                                    : "%mm1", "%mm2");
                }
              __asm__ volatile ("emms");
            }
#       endif // KDU_X86_INTRINSICS
            
#       ifdef __BIG_ENDIAN__ // Executing on PowerPC architecture
          memcpy(dst,src,(size_t)(c<<2));
          dst += c;
          src += c;
#       else // Executing on Intel architecture
          for (; c > 0; c--, dst++, src++)
            {
              kdu_uint32 val = *src;
              *dst = ((val >> 24) + ((val >> 8) & 0xFF00) +
                      ((val << 8) & 0xFF0000) + (val << 24));
            }
#       endif // Executing on Intel architecture

          src += obj->buf_row_gap - (tgt_col + copy_pels);
          tgt_col = 0;
          tgt_row++;
        }
      return original_byte_count - (size_t)(pels_left_to_transfer<<2);
    }

  // If we get here, we need to use the tone mapping curves and adjust
  // the behaviour for background and foreground regions
  int region_rows[3] =
    {obj->rows_above_focus, obj->focus_rows, obj->rows_below_focus};
  if ((region_rows[0] -= tgt_row) < 0)
    {
      if ((region_rows[1] += region_rows[0]) < 0)
        {
          region_rows[2] += region_rows[1];
          region_rows[1] = 0;
        }
      region_rows[0] = 0;
    }      
  int region_cols[3] =
    {obj->cols_left_of_focus, obj->focus_cols, obj->cols_right_of_focus};
  if ((region_cols[0] -= tgt_col) < 0)
    {
      if ((region_cols[1] += region_cols[0]) < 0)
        {
          region_cols[2] += region_cols[1];
          region_cols[1] = 0;
        }
      region_cols[0] = 0;
    }
  kdu_byte *src = (kdu_byte *)(obj->buf + tgt_row*obj->buf_row_gap + tgt_col);
  kdu_byte *dst = (kdu_byte *) buffer;
  kdu_byte *lut = NULL;
  int inter_row_src_skip = (obj->buf_row_gap - obj->tgt_width)<<2;
  int reg_v, reg_h, r;
  for (reg_v=0; reg_v < 3; reg_v++)
    { // Scan through the vertical regions (bg, fg, bg)
      for (r=region_rows[reg_v]; r > 0; r--)
        {
          if (reg_v != 1)
            { // Everything is background
              lut = obj->background_lut;
              int copy_pels = region_cols[0]+region_cols[1]+region_cols[2];
              if (copy_pels > pels_left_to_transfer)
                copy_pels = pels_left_to_transfer;
              if (copy_pels <= 0)
                return original_byte_count;
              pels_left_to_transfer -= copy_pels;
              for (; copy_pels != 0; copy_pels--)
                {
#               ifdef __BIG_ENDIAN__ // Executing on PowerPC architecture
                  dst[0] = src[0];
                  dst[1] = lut[src[1]];
                  dst[2] = lut[src[2]];
                  dst[3] = lut[src[3]];
#               else // Executing on Intel architecture
                  dst[0] = src[3];
                  dst[1] = lut[src[2]];
                  dst[2] = lut[src[1]];
                  dst[3] = lut[src[0]];
#               endif // Executing on Intel architecture
                  dst += 4; src += 4;
                }
            }
          else
            { // Some columns may be foreground and some background
              for (reg_h=0; reg_h < 3; reg_h++)
                {
                  lut = (reg_h == 1)?obj->foreground_lut:obj->background_lut;
                  int copy_pels = region_cols[reg_h];
                  if (copy_pels == 0)
                    continue; // This region is empty
                  if (copy_pels > pels_left_to_transfer)
                    copy_pels = pels_left_to_transfer;
                  if (copy_pels <= 0)
                    return original_byte_count;
                  pels_left_to_transfer -= copy_pels;                  
                  for (; copy_pels != 0; copy_pels--)
                    {
#                   ifdef __BIG_ENDIAN__ // Executing on PowerPC architecture
                      dst[0] = src[0];
                      dst[1] = lut[src[1]];
                      dst[2] = lut[src[2]];
                      dst[3] = lut[src[3]];
#                   else // Executing on Intel architecture
                      dst[0] = src[3];
                      dst[1] = lut[src[2]];
                      dst[2] = lut[src[1]];
                      dst[3] = lut[src[0]];
#                   endif // Executing on Intel architecture
                      dst += 4; src += 4;
                    }
                }
            }
       
          // Adjust the `src' pointer
          src += inter_row_src_skip;
          
          // Reset the number of columns in each region
          region_cols[0] = obj->cols_left_of_focus;
          region_cols[1] = obj->focus_cols;
          region_cols[2] = obj->cols_right_of_focus;
        }
    }
  return original_byte_count - (size_t)(pels_left_to_transfer<<2);
}


/*===========================================================================*/
/*                            kdms_playmode_stats                            */
/*===========================================================================*/

/*****************************************************************************/
/*                        kdms_playmode_stats::update                        */
/*****************************************************************************/

void kdms_playmode_stats::update(int frame_idx, double start_time,
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
/*                               kdms_rel_dims                               */
/* ========================================================================= */

/*****************************************************************************/
/*                             kdms_rel_dims::set                            */
/*****************************************************************************/

bool kdms_rel_dims::set(kdu_dims region, kdu_dims container)
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
/*                     kdms_rel_dims::apply_to_container                     */
/*****************************************************************************/

kdu_dims kdms_rel_dims::apply_to_container(kdu_dims container)
{
  kdu_dims result;
  result.size.x = (int)(0.5 + size_x * container.size.x);
  result.size.y = (int)(0.5 + size_y * container.size.y);
  result.pos.x = (int)(0.5 + (centre_x-0.5*size_x) * container.size.x);
  result.pos.y = (int)(0.5 + (centre_y-0.5*size_y) * container.size.y);
  result.pos += container.pos;
  return result;
}


/*===========================================================================*/
/*                               kdms_renderer                               */
/*===========================================================================*/

/*****************************************************************************/
/*                        kdms_renderer::kdms_renderer                       */
/*****************************************************************************/

kdms_renderer::kdms_renderer(kdms_window *owner, 
                             kdms_document_view *doc_view,
                             NSScrollView *scroll_view,
                             kdms_window_manager *window_manager)
{
  this->window = owner;
  this->doc_view = doc_view;
  this->scroll_view = scroll_view;
  this->window_manager = window_manager;
  this->metashow = nil;
  this->window_identifier = window_manager->get_window_identifier(owner);
  this->catalog_source = nil;
  this->catalog_closed = false;
  this->editor = nil;
  
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
  else if (num_recommended_threads < 2)
    num_recommended_threads = 0;
  num_threads = num_recommended_threads;
  if (num_threads > 0)
    {
      thread_env.create();
      for (int k=1; k < num_threads; k++)
        if (!thread_env.add_thread())
          num_threads = k;
    }
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
  frame_presenter = window_manager->get_frame_presenter(owner);
  
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
  view_centre_x = view_centre_y = 0.0F;
  view_centre_known = false;
  
  enable_focus_box = false;
  highlight_focus = true;
  
  overlays_enabled = false;
  overlay_flashing_state = 1;
  overlay_highlight_state = 0;
  overlay_nominal_intensity = KDMS_OVERLAY_DEFAULT_INTENSITY;
  overlay_size_threshold = 1;
  kdu_uint32 params[KDMS_NUM_OVERLAY_PARAMS] = KDMS_OVERLAY_PARAMS;
  for (int p=0; p < KDMS_NUM_OVERLAY_PARAMS; p++)
    overlay_params[p] = params[p];
  
  buffer = NULL;
  generic_rgb_space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);

  set_key_window_status([window isKeyWindow]);
}

/*****************************************************************************/
/*                        kdms_renderer::~kdms_renderer                      */
/*****************************************************************************/

kdms_renderer::~kdms_renderer()
{
  frame_presenter = NULL; // Avoid access to potentially non-existent object
  perform_essential_cleanup_steps();
  
  if (file_server != NULL)
    { delete file_server; file_server = NULL; }
  if (compositor != NULL)
    { delete compositor; compositor = NULL; }
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
      window_manager->release_jpip_client(jpip_client,window);      
      jpip_client = NULL;
    }
  if (last_save_pathname != NULL)
    delete[] last_save_pathname;
  CGColorSpaceRelease(generic_rgb_space);
  
  assert(!view_mutex_locked);
  view_mutex.destroy();
}

/*****************************************************************************/
/*             kdms_renderer::perform_essential_cleanup_steps                */
/*****************************************************************************/

void kdms_renderer::perform_essential_cleanup_steps()
{
  if (frame_presenter != NULL)
    frame_presenter->reset(); // Make sure there is no asynchronous painting
  if (file_server != NULL)
    delete file_server;
  file_server = NULL;
  if ((window_manager != NULL) && (open_file_pathname != NULL))
    {
      if (compositor != NULL)
        { delete compositor; compositor = NULL; }
      file_in.close(); // Unlocks any files
      jp2_family_in.close(); // Unlocks any files
      window_manager->release_open_file_pathname(open_file_pathname,window);
      open_file_pathname = NULL;
      open_filename = NULL;
    }
}

/*****************************************************************************/
/*                        kdms_renderer::close_file                          */
/*****************************************************************************/

void kdms_renderer::close_file()
{
  // Prevent other windows from accessing the data source
  if (catalog_source != nil)
    [catalog_source deactivate];
  if (metashow != nil)
    [metashow deactivate];
  shape_editor = NULL; shape_istream = kdu_istream_ref();
  
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
  window_manager->schedule_wakeup(window,-1.0);
  
  // Close down all information rendering machinery
  if (compositor != NULL)
    { delete compositor; compositor = NULL; }
  if (editor != nil)
    {
      [editor close];
      assert(editor == nil);
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
      window_manager->release_jpip_client(jpip_client,window);      
      jpip_client = NULL;
    }
  client_prefs.init(); // All defaults

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
  overlay_nominal_intensity = KDMS_OVERLAY_DEFAULT_INTENSITY;
  overlay_restriction = last_selected_overlay_restriction = jpx_metanode();
  
  buffer = NULL;
  
  // Update GUI elements to reflect "closed" status
  [window setTitle:
   [NSString stringWithFormat:@"kdu_show %d: <no data>",window_identifier]];
  if ((catalog_source != nil) && (window != nil))
    [window remove_metadata_catalog];
  else if (catalog_source)
    set_metadata_catalog_source(NULL); // Just in case the window went away
  [window remove_progress_bar];
  [doc_view setNeedsDisplay:YES];
  catalog_closed = false;
  window_manager->declare_window_empty(window,true);
}

/*****************************************************************************/
/*                   kdms_renderer::set_key_window_status                    */
/*****************************************************************************/

void kdms_renderer::set_key_window_status(bool is_key_window)
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
          if ((catalog_source != nil) && (editor == nil))
            [catalog_source update_metadata];
        }
    }
  if (jpip_client_received_bytes_since_last_refresh &&
      (compositor != NULL) && compositor->is_processing_complete())
    { // May need to refresh display or schedule/reschedule a refresh
      double cur_time = get_current_time();
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
/*                     kdms_renderer::open_as_duplicate                      */
/*****************************************************************************/

void kdms_renderer::open_as_duplicate(kdms_renderer *src)
{
  this->duplication_src = src;
  if (src->open_file_pathname != NULL)
    [window open_file:[NSString stringWithUTF8String:src->open_file_pathname]];
       // We could just call `open_file' directly, but the above statement
       // interrogates the user if the file has been modified.
  else if (src->open_file_urlname != NULL)
    open_file(src->open_file_urlname);
  this->duplication_src = NULL; // Because the `src' might disappear
}

/*****************************************************************************/
/*                         kdms_renderer::open_file                          */
/*****************************************************************************/

void kdms_renderer::open_file(const char *filename_or_url)
{
  if (processing_call_to_open_file)
    return; // Prevent recursive entry to this function from event handlers
  if (filename_or_url != NULL)
    {
      close_file();
      window_manager->declare_window_empty(window,false);
    }
  processing_call_to_open_file = true;
  assert(compositor == NULL);
  enable_focus_box = false;
  focus_box.size = kdu_coords(0,0);
  client_roi.init();
  processing_suspended = false;
  enable_region_posting = false;
  bool is_jpip = false;
  try
    {
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
                window_manager->retain_jpip_client(NULL,NULL,filename_or_url,
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
              [window set_progress_bar:0.0];
              if (metashow != nil)
                [metashow activateWithSource:&jp2_family_in
                                 andFileName:open_filename];
            }
          else
            { // Open as local file
              status_id = KDS_STATUS_LAYER_RES;
              jp2_family_in.open(filename_or_url,allow_seeking);
              compositor = new kdms_region_compositor(&thread_env);
              if (jpx_in.open(&jp2_family_in,true) >= 0)
                { // We have a JP2/JPX-compatible file.
                  compositor->create(&jpx_in);
                }
              else if (mj2_in.open(&jp2_family_in,true) >= 0)
                {
                  compositor->create(&mj2_in);
                }
              else
                { // Incompatible with JP2/JPX or MJ2. Try opening raw stream
                  jp2_family_in.close();
                  file_in.open(filename_or_url,allow_seeking);
                  compositor->create(&file_in);
                }
              compositor->set_error_level(error_level);
              compositor->set_process_aggregation_threshold(0.5F);
                  // Display updates are expensive, so encourage aggregtion in
                  // calls to `process'.
              open_file_pathname =
                window_manager->retain_open_file_pathname(filename_or_url,
                                                          window);
              open_filename = strrchr(open_file_pathname,'/');
              if (open_filename == NULL)
                open_filename = open_file_pathname;
              else
                open_filename++;
              if ((metashow != nil) && jp2_family_in.exists())
                [metashow activateWithSource:&jp2_family_in
                                 andFileName:open_filename];
            }
        }
      
      if (jpip_client != NULL)
        { // See if the client is ready yet
          assert((compositor == NULL) && jp2_family_in.exists());
          bool bin0_complete = false;
          if (cache_in.get_databin_length(KDU_META_DATABIN,0,0,
                                          &bin0_complete) > 0)
            { // Open as a JP2/JPX family file
              if (metashow != nil)
                [metashow update_metadata];
              if (jpx_in.open(&jp2_family_in,false))
                {
                  compositor = new kdms_region_compositor(&thread_env);
                  compositor->create(&jpx_in);
                }
              window_manager->use_jpx_translator_with_jpip_client(jpip_client);
            }
          else if (bin0_complete)
            { // Open as a raw file
              assert(!jpx_in.exists());
              if (metashow != nil)
                [metashow deactivate];
              bool hdr_complete;
              cache_in.get_databin_length(KDU_MAIN_HEADER_DATABIN,0,0,
                                          &hdr_complete);
              if (hdr_complete)
                {
                  cache_in.set_read_scope(KDU_MAIN_HEADER_DATABIN,0,0);
                  compositor = new kdms_region_compositor(&thread_env);
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
  catch (kdu_exception)
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
      overlay_nominal_intensity = KDMS_OVERLAY_DEFAULT_INTENSITY;
      next_overlay_flash_time = -1.0;
      configure_overlays();

      invalidate_configuration();
      fit_scale_to_window = true;
      image_dims = buffer_dims = view_dims = kdu_dims();
      buffer = NULL;
      if (duplication_src != NULL)
        {
          kdms_renderer *src = duplication_src;
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
                 sizeof(kdu_uint32)*KDMS_NUM_OVERLAY_PARAMS);
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
      [window setTitle:[NSString stringWithUTF8String:title]];
      delete[] title;
    }
  
  if ((duplication_src == NULL) && jpx_in.exists() &&
      jpx_in.access_meta_manager().peek_touched_nodes(jp2_label_4cc).exists())
    [window create_metadata_catalog];
  processing_call_to_open_file = false;
}

/*****************************************************************************/
/*                   kdms_renderer::invalidate_configuration                 */
/*****************************************************************************/

void kdms_renderer::invalidate_configuration()
{
  frame_presenter->reset(); // Avoid possible access to an invalid buffer
  configuration_complete = false;
  max_components = -1; // Changed config may have different num image comps
  buffer = NULL;
  
  if (compositor != NULL)
    compositor->remove_ilayer(kdu_ilayer_ref(),false);
}

/*****************************************************************************/
/*                      kdms_renderer::initialize_regions                    */
/*****************************************************************************/

void kdms_renderer::initialize_regions(bool perform_full_window_init)
{
  frame_presenter->reset(); // Avoid possible access to invalid buffer
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
            if (!compositor->add_ilayer(single_layer_idx,kdu_dims(),kdu_dims(),
                                        false,false,false,frame_idx,0))
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
                  return; // Cannot open composition rules yet; wait for cache
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
            
            if (!frame_expander.construct(&jpx_in,frame,frame_iteration,true))
              {
                if (respect_initial_jpip_request)
                  { // See if request is compatible with opening a frame
                    if (!check_initial_request_compatibility())         
                      continue; // Mode changed to single layer or component
                  }
                else
                  update_client_window_of_interest();
                return; // Can't open all frame members yet; waiting for cache
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
        min_rendering_scale=-1.0F; // Need to discover these from
        max_rendering_scale=-1.0F; // scratch each time.
      }
      catch (kdu_exception) { // Try downgrading to more primitive viewing mode
        if ((single_component_idx >= 0) ||
            !(jpx_in.exists() || mj2_in.exists()))
          { // Already in single component mode; nothing more primitive exists
            close_file();
            return;
          }
        else if (single_layer_idx >= 0)
          { // Downgrade from single layer mode to single component mode
            single_component_idx = 0;
            single_codestream_idx = 0;
            single_layer_idx = -1;
            if (enable_region_posting && !respect_initial_jpip_request)
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
                new_rendering_scale=decrease_scale(new_rendering_scale,false);
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
                new_rendering_scale=increase_scale(new_rendering_scale,false);
                  // The above call will not actually increase the scale; it
                  // will just find the maximum possible scale that can be
                  // supported.
                assert(new_rendering_scale == max_rendering_scale);
              }
            else if (!(invalid_scale_code & KDU_COMPOSITOR_TRY_SCALE_AGAIN))
              {
                if (single_component_idx >= 0)
                  { kdu_error e;
                    e << "Cannot display the image.  Inexplicable error "
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
            while (((min_rendering_scale < 0.0F) ||
                    (new_rendering_scale >= min_rendering_scale)) &&
                   ((new_rendering_scale > max_x) ||
                    (new_rendering_scale > max_y)))
              {
                new_rendering_scale=decrease_scale(new_rendering_scale,false);
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
  catch (kdu_exception exc) { // Some error occurred while parsing code-streams
    close_file();
    return;
  }
  catch (std::bad_alloc &) {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Insufficient memory -- you could improve "
                           "upon the current implementation to invoke "
                           "`kdu_region_compositor::cull_inactive_layers' "
                           "or take other memory-saving action in response "
                           "to this condition."];
    [alert addButtonWithTitle:@"OK"];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert runModal];
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
      buffer = compositor->get_composition_buffer(buffer_dims);
      bool focus_needs_update = adjust_rel_focus();
      if (focus_needs_update &&
          rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
        view_centre_known = true;
      if (view_centre_known)
        scroll_to_view_anchors();
      if (focus_needs_update)
        update_focus_box();
    }
  else
    { // Need to size the window, possibly for the first time.
      view_dims = buffer_dims = kdu_dims();
      image_dims = new_image_dims;
      if ((duplication_src == NULL) && adjust_rel_focus() &&
          rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
        view_centre_known = true;
      size_window_for_image_dims(can_enlarge_window);
      if ((compositor != NULL) && !view_dims.is_empty())
        compositor->invalidate_rect(view_dims);
    }
  if (!respect_initial_jpip_request)
    update_client_window_of_interest();
  if (in_playmode)
    { // See if we need to turn off play mode
      if ((single_component_idx >= 0) ||
          ((single_layer_idx >= 0) && jpx_in.exists()))
        stop_playmode();      
    }
  if (overlays_enabled && !in_playmode)
    {
      jpx_metanode node;
      if (catalog_source &&
          (node=[catalog_source get_selected_metanode]).exists())
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
  rel_focus.reset();
}

/*****************************************************************************/
/*                     kdms_renderer::configure_overlays                     */
/*****************************************************************************/

void kdms_renderer::configure_overlays()
{
  if (compositor == NULL)
    return;
  float intensity = overlay_nominal_intensity;
  jpx_metanode restriction = overlay_restriction;
  double next_dwell = 0.0;
  if (overlay_highlight_state)
    { 
      intensity *= kdms_overlay_highlight_factor[overlay_highlight_state-1];
      next_dwell = kdms_overlay_highlight_dwell[overlay_highlight_state-1];
      restriction = highlight_metanode;
    }
  else if (overlay_flashing_state)
    {
      intensity *= kdms_overlay_flash_factor[overlay_flashing_state-1];
      next_dwell = kdms_overlay_flash_dwell[overlay_flashing_state-1];
    }
  compositor->configure_overlays(overlays_enabled,overlay_size_threshold,
                                 intensity,KDMS_OVERLAY_BORDER,
                                 restriction,0,overlay_params,
                                 KDMS_NUM_OVERLAY_PARAMS);
  if (overlays_enabled && compositor->is_processing_complete() &&
      (overlay_highlight_state || overlay_flashing_state))
    schedule_overlay_flash_wakeup(get_current_time() + next_dwell);
  else
    schedule_overlay_flash_wakeup(-1.0);
}

/*****************************************************************************/
/*                 kdms_renderer::get_max_scroll_view_size                   */
/*****************************************************************************/

bool kdms_renderer::get_max_scroll_view_size(NSSize &size)
{
  if ((buffer == NULL) || !image_dims)
    return false;
  NSSize doc_size; // Size of the `doc_view' frame
  doc_size.width = ((float) image_dims.size.x) * pixel_scale;
  doc_size.height = ((float) image_dims.size.y) * pixel_scale;
  size = [NSScrollView frameSizeForContentSize:doc_size
                         hasHorizontalScroller:YES
                           hasVerticalScroller:YES
                                    borderType:NSNoBorder];
  return true;
}

/*****************************************************************************/
/*                    kdms_renderer::place_duplicate_window                  */
/*****************************************************************************/

void kdms_renderer::place_duplicate_window()
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
  NSSize doc_size; // Size on the `doc_view' frame
  doc_size.width = ((float) view_size.x) * pixel_scale;
  doc_size.height = ((float) view_size.y) * pixel_scale;
  NSRect scroll_rect, content_rect, window_rect;
  scroll_rect.origin.x = scroll_rect.origin.y = 0.0F;
  scroll_rect.size = [NSScrollView frameSizeForContentSize:doc_size
                                     hasHorizontalScroller:YES
                                       hasVerticalScroller:YES
                                                borderType:NSNoBorder];
  content_rect = scroll_rect;
  content_rect.size.height += [window get_bottom_margin_height];
  content_rect.size.width += [window get_right_margin_width];
  window_rect = [window frameRectForContentRect:content_rect];
  window_manager->place_window(window,window_rect.size);
}

/*****************************************************************************/
/*                kdms_renderer::size_window_for_image_dims                  */
/*****************************************************************************/

void kdms_renderer::size_window_for_image_dims(bool first_time)
{
  NSRect max_window_rect = [[NSScreen mainScreen] visibleFrame];
  NSRect existing_window_rect = [window frame];
  if (!first_time)
    max_window_rect = existing_window_rect;
  
  // Find the size of the `doc_view' window and see if it can fit
  // completely inside the `scroll_view', given the window dimensions.
  NSSize doc_size; // Size of the `doc_view' frame
  doc_size.width = ((float) image_dims.size.x) * pixel_scale;
  doc_size.height = ((float) image_dims.size.y) * pixel_scale;
  NSRect scroll_rect, window_rect;
  scroll_rect.origin.x = scroll_rect.origin.y = 0.0F;
  scroll_rect.size = [NSScrollView frameSizeForContentSize:doc_size
                                     hasHorizontalScroller:YES
                                       hasVerticalScroller:YES
                                                borderType:NSNoBorder];
  NSRect content_rect = scroll_rect;
  content_rect.size.height += [window get_bottom_margin_height];
  content_rect.size.width += [window get_right_margin_width];
  window_rect = [window frameRectForContentRect:content_rect];
  [window setMaxSize:window_rect.size];
  
  window_rect.origin = existing_window_rect.origin;
  if (window_rect.size.width > max_window_rect.size.width)
    window_rect.size.width = max_window_rect.size.width;
  if (window_rect.size.height > max_window_rect.size.height)
    window_rect.size.height = max_window_rect.size.height;
  
  bool resize_window = false;
  if (window_rect.size.width != existing_window_rect.size.width)
    { // Need to adjust window width and position.
      resize_window = true;
      float left = existing_window_rect.origin.x;
      float right = left + window_rect.size.width;
      float max_left = max_window_rect.origin.x;
      float max_right = max_left + max_window_rect.size.width;
      float shift;
      if ((shift = (right - max_right)) > 0.0F)
        { right -= shift; left -= shift; }
      if ((shift = (max_left-left)) > 0.0F)
        { right += shift; left += shift; }
      window_rect.origin.x = left;
    }
  if (window_rect.size.height != existing_window_rect.size.height)
    { // Need to adjust window width and position.
      resize_window = true;
      float top =
        existing_window_rect.origin.y + existing_window_rect.size.height;
      float bottom = top - window_rect.size.height;
      float max_bottom = max_window_rect.origin.y;
      float max_top = max_bottom + max_window_rect.size.height;
      float shift;
      if ((shift = (max_bottom-bottom)) > 0.0F)
        { bottom += shift; top += shift; }
      if ((shift = (top - max_top)) > 0.0F)
        { bottom -= shift; top -= shift; }
      window_rect.origin.y = top - window_rect.size.height;
    }
  if (first_time || resize_window)
    {
      if (!(first_time &&
            window_manager->place_window(window,window_rect.size,true)))
        [window setFrame:window_rect display:NO];
    }
  
  // Finally set the `doc_view'
  [doc_view setFrameSize:doc_size];
  if (view_centre_known)
    scroll_to_view_anchors();
  else if (first_time)
    { // Adjust `scroll_view' position to the top of the image.
      NSRect view_rect = [scroll_view documentVisibleRect];
      NSPoint new_view_origin;
      new_view_origin.x = 0.0F;
      new_view_origin.y = doc_size.height - view_rect.size.height;
      [doc_view scrollPoint:new_view_origin];
    }
  view_dims_changed();
}

/*****************************************************************************/
/*                     kdms_renderer::view_dims_changed                      */
/*****************************************************************************/

void kdms_renderer::view_dims_changed()
{
  if (compositor == NULL)
    return;
  
  // Start by finding `view_dims' as the smallest rectangle (in the same
  // coordinate system as `image_dims') which spans the contents of
  // the `scroll_view'.
  NSRect ns_rect = [scroll_view documentVisibleRect];
  NSRect view_rect;
  view_rect.origin.x = (float) floor(ns_rect.origin.x);
  view_rect.origin.y = (float) floor(ns_rect.origin.y);
  view_rect.size.width = (float)
    ceil(ns_rect.size.width+ns_rect.origin.x) - view_rect.origin.x;
  view_rect.size.height = (float)
    ceil(ns_rect.size.height+ns_rect.origin.y) - view_rect.origin.y;
  
  kdu_dims new_view_dims = convert_region_from_doc_view(view_rect);
  new_view_dims &= image_dims;
  if ((new_view_dims == view_dims) && !buffer_dims.is_empty())
    { // No change
      update_focus_box();
      return;
    }
  
  lock_view();
  view_dims = new_view_dims;
  buffer = NULL;
  unlock_view();
  
  // Get preferred minimum dimensions for the new buffer region.
  kdu_coords size = view_dims.size;
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
  kdu_coords image_lim = image_dims.pos + image_dims.size;
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
  buffer = compositor->get_composition_buffer(buffer_dims);
  update_focus_box(true);
}

/*****************************************************************************/
/*                kdms_renderer::convert_region_from_doc_view                */
/*****************************************************************************/

kdu_dims kdms_renderer::convert_region_from_doc_view(NSRect rect)
{
  float scale_factor = 1.0F / (float) pixel_scale;
  kdu_coords region_min, region_lim;
  region_min.x = (int)
    round(rect.origin.x * scale_factor - 0.4);
  region_lim.x = (int)
    round((rect.origin.x+rect.size.width) * scale_factor + 0.4);
  region_lim.y = image_dims.size.y - (int)
    round(rect.origin.y * scale_factor - 0.4);
  region_min.y = image_dims.size.y - (int)
    round((rect.origin.y+rect.size.height) * scale_factor + 0.4);
  kdu_dims region;
  region.pos = region_min + image_dims.pos;
  region.size = region_lim - region_min;
  return region;
}

/*****************************************************************************/
/*                  kdms_renderer::convert_region_to_doc_view                */
/*****************************************************************************/

NSRect kdms_renderer::convert_region_to_doc_view(kdu_dims region)
{
  float scale_factor = (float) pixel_scale;
  kdu_coords rel_pos = region.pos - image_dims.pos;
  NSRect doc_rect;
  doc_rect.origin.x = rel_pos.x * scale_factor;
  doc_rect.size.width = region.size.x * scale_factor;
  doc_rect.origin.y = (image_dims.size.y-rel_pos.y-region.size.y)*scale_factor;
  doc_rect.size.height = region.size.y * scale_factor;
  return doc_rect;
}

/*****************************************************************************/
/*                kdms_renderer::convert_point_from_doc_view                 */
/*****************************************************************************/

kdu_coords kdms_renderer::convert_point_from_doc_view(NSPoint point)
{
  float scale_factor = 1.0F / (float) pixel_scale;
  kdu_coords pos;
  pos.x = (int) round(point.x * scale_factor);
  pos.y = image_dims.size.y - 1 - (int) round(point.y * scale_factor);
  return pos + image_dims.pos;
}

/*****************************************************************************/
/*                  kdms_renderer::convert_point_to_doc_view                 */
/*****************************************************************************/

NSPoint kdms_renderer::convert_point_to_doc_view(kdu_coords point)
{
  float scale_factor = (float) pixel_scale;
  kdu_coords rel_pos = point - image_dims.pos;
  NSPoint doc_point;
  doc_point.x = rel_pos.x * scale_factor;
  doc_point.y = (image_dims.size.y-1-rel_pos.y)*scale_factor;
  return doc_point;
}

/*****************************************************************************/
/*                        kdms_renderer::paint_region                        */
/*****************************************************************************/

bool kdms_renderer::paint_region(kdu_dims region,
                                 CGContextRef graphics_context,
                                 NSPoint *presentation_offset)
{
  if ((compositor == NULL) || (buffer == NULL) ||
      (!buffer_dims) || (!image_dims))
    return false;
  region &= buffer_dims;
  if (!region)
    return true;
  
  kdu_coords rel_pos = region.pos - buffer_dims.pos;
  if (rel_pos.x & 3)
    { // Align start of rendering region on 16-byte boundary
      int offset = rel_pos.x & 3;
      region.pos.x -= offset;
      rel_pos.x -= offset;
      region.size.x += offset;
    }
  if (region.size.x & 3)
    { // Align end of region on 16-byte boundary
      region.size.x += (4-region.size.x) & 3;
    }

  bool display_with_focus = enable_focus_box && highlight_focus;  
  kdu_dims rel_focus_box = focus_box; // In case there is a focus box, make it
  rel_focus_box.pos -= region.pos;    // relative to the region being painted
  if (shape_istream.exists())
    {
      display_with_focus = true;
      rel_focus_box = kdu_dims();
    }
  
  int buf_row_gap;
  kdu_uint32 *buf_data = buffer->get_buf(buf_row_gap,false);
  buf_data += rel_pos.y*buf_row_gap + rel_pos.x;

  NSRect target_region = convert_region_to_doc_view(region);
  if ((target_region.size.width < 0.9) || (target_region.size.height < 0.9))
    return true;
  CGRect cg_target_region;
  cg_target_region.origin.x = target_region.origin.x;
  cg_target_region.origin.y = target_region.origin.y;
  cg_target_region.size.width = target_region.size.width;
  cg_target_region.size.height = target_region.size.height;
  kdms_data_provider *provider = &app_paint_data_provider;
  if (presentation_offset != NULL)
    {
      cg_target_region.origin.x += presentation_offset->x;
      cg_target_region.origin.y += presentation_offset->y;
      provider = &presentation_data_provider;
    }

  CGImageRef image =
    CGImageCreate(region.size.x,region.size.y,8,32,
                  (region.size.x<<2),generic_rgb_space,
                  kCGImageAlphaPremultipliedFirst,
                  provider->init(region.size,buf_data,buf_row_gap,
                                 display_with_focus,rel_focus_box),
                  NULL,true,kCGRenderingIntentDefault);
  CGContextDrawImage(graphics_context,cg_target_region,image);
  CGImageRelease(image);
  return true;
}

/*****************************************************************************/
/*            kdms_renderer::paint_view_from_presentation_thread             */
/*****************************************************************************/

void kdms_renderer::paint_view_from_presentation_thread(NSGraphicsContext *gc)
{
  if (!view_dims)
    return;
  [gc saveGraphicsState];
  if ([scroll_view lockFocusIfCanDraw])
    {
      NSPoint doc_origin = {0.0F,0.0F};
      NSPoint doc_origin_on_window =
        [doc_view convertPoint:doc_origin toView:nil];
      NSRect visible_rect = [scroll_view documentVisibleRect];
      visible_rect.origin.x += doc_origin_on_window.x;
      visible_rect.origin.y += doc_origin_on_window.y;
      [NSGraphicsContext setCurrentContext:gc];
      NSRectClip(visible_rect);
      CGContextRef gc_ref = (CGContextRef)[gc graphicsPort];
      lock_view();
      paint_region(view_dims,gc_ref,&doc_origin_on_window);
      unlock_view();
      [gc flushGraphics];
      [scroll_view unlockFocus];
    }
  [gc restoreGraphicsState];
}

/*****************************************************************************/
/*                      kdms_renderer::display_status                        */
/*****************************************************************************/

void kdms_renderer::display_status()
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
      [window set_progress_bar:progress];
    }
  [window set_status_strings:((const char **)panel_text)];
}

/*****************************************************************************/
/*                           kdms_renderer::on_idle                          */
/*****************************************************************************/

bool kdms_renderer::on_idle()
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
        double cur_time = get_current_time();
        if (cur_time > defer_process_regions_deadline)
          { defer_flag=0; defer_process_regions_deadline = cur_time + 0.05; }
      }
    processed_something = compositor->process(128000,new_region,defer_flag);
    if ((!in_playmode) && (defer_process_regions_deadline <= 0.0) &&
        processed_something)
      defer_process_regions_deadline = get_current_time() + 0.05;
      
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
  catch (kdu_exception exc) { // Only safe thing is close the file.
    close_file();
    return false;
  }
  catch (std::bad_alloc &) {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Insufficient memory -- you could improve "
                           "upon the current implementation to invoke "
                           "`kdu_region_compositor::cull_inactive_layers' "
                           "or take other memory-saving action in response "
                           "to this condition."];
    [alert addButtonWithTitle:@"OK"];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert runModal];
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
      defer_process_regions_deadline = -1.0;
      cur_time = get_current_time();
      if (processing_newly_complete)
        {
          last_render_completion_time = cur_time;
          if (overlays_enabled && overlay_highlight_state)
            { // Schedule return to the regular overlay flashing state
              double dwell =
                kdms_overlay_highlight_dwell[overlay_highlight_state-1];
              schedule_overlay_flash_wakeup(cur_time+dwell);
            }
          else if (overlays_enabled && overlay_flashing_state &&
                   (!in_playmode))
            { // Schedule the next flashing overlay wakeup time
              double dwell=kdms_overlay_flash_dwell[overlay_flashing_state-1];
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
          NSRect dirty_rect = convert_region_to_doc_view(new_region);
          [doc_view setNeedsDisplayInRect:dirty_rect];
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
  if (processing_newly_complete && !in_playmode)
    [doc_view displayIfNeeded];
  return ((compositor != NULL) && !compositor->is_processing_complete());
}

/*****************************************************************************/
/*         kdms_renderer::adjust_frame_queue_presentation_and_wakeup         */
/*****************************************************************************/

void kdms_renderer::adjust_frame_queue_presentation_and_wakeup(double cur_time)
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
      buffer = compositor->get_composition_buffer(buffer_dims);
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
      buffer = compositor->get_composition_buffer(buffer_dims);
      double overdue = cur_time - pushed_frame_start_time;
      if (overdue > 0.1)
        { // We are running behind by more than 100ms
          window_manager->broadcast_playclock_adjustment(0.5*overdue);
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
/*                          kdms_renderer::wakeup                            */
/*****************************************************************************/

void kdms_renderer::wakeup(double scheduled_time,double current_time)
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
              if (overlay_highlight_state > KDMS_OVERLAY_HIGHLIGHT_STATES)
                overlay_highlight_state = 0;
            }
          else if (overlay_flashing_state)
            { 
              overlay_flashing_state++;
              if (overlay_flashing_state > KDMS_OVERLAY_FLASHING_STATES)
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
/*                   kdms_renderer::client_notification                      */
/*****************************************************************************/

void kdms_renderer::client_notification()
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
        cur_time = get_current_time();
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
            cur_time = get_current_time();
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
            cur_time = get_current_time();
          if (cur_time > (last_status_update_time+0.25))
            {
              last_status_update_time = cur_time;
              display_status();
            }
          else
            schedule_status_update_wakeup(cur_time+0.25);
        }
      
      if (editor == nil)
        {
          if ((metashow != nil) || (catalog_source != nil))
            { // Metashow should be updated
              if (metashow != nil)
                [metashow update_metadata];
              if (catalog_source != nil)
                [catalog_source update_metadata];
            }
          if ((compositor != NULL) && (catalog_source == nil) &&
              (!catalog_closed) && jpx_in.exists())
            {
              jpx_meta_manager meta_manager = jpx_in.access_meta_manager();
              meta_manager.load_matches(-1,NULL,-1,NULL);
              if (meta_manager.peek_touched_nodes(jp2_label_4cc).exists())
                [window create_metadata_catalog];
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
      // may now have enough data to make a more precise request for the
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
/*                      kdms_renderer::refresh_display                       */
/*****************************************************************************/

bool kdms_renderer::refresh_display(bool new_imagery_only)
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
      if (in_playmode)
        { // I don't think there is any way this should be able to happen
          stop_playmode();
          kdu_warning w; w << "Stopping video playback mode during a weird "
            "frame refresh activity, which has invalidated the current "
            "display dimensions.  This condition should not be possible.";
        }
      kdu_dims new_image_dims;
      bool have_valid_scale = 
        compositor->get_total_composition_dims(new_image_dims);
      if ((!have_valid_scale) || (new_image_dims != image_dims))
        initialize_regions(false);
      else
        buffer = compositor->get_composition_buffer(buffer_dims);
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
/*               kdms_renderer::schedule_overlay_flash_wakeup                */
/*****************************************************************************/

void kdms_renderer::schedule_overlay_flash_wakeup(double when)
{
  if ((next_scheduled_wakeup_time >= 0.0) &&
      (next_scheduled_wakeup_time > (when-0.1)) &&
      (next_scheduled_wakeup_time < (when+0.1)))
    when = next_scheduled_wakeup_time;
  next_overlay_flash_time = when;
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*              kdms_renderer::schedule_render_refresh_wakeup                */
/*****************************************************************************/

void kdms_renderer::schedule_render_refresh_wakeup(double when)
{
  if ((next_scheduled_wakeup_time >= 0.0) &&
      (next_scheduled_wakeup_time > (when-0.1)) &&
      (next_scheduled_wakeup_time < (when+0.1)))
    when = next_scheduled_wakeup_time;
  next_render_refresh_time = when;  
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*               kdms_renderer::schedule_status_update_wakeup                */
/*****************************************************************************/

void kdms_renderer::schedule_status_update_wakeup(double when)
{
  if ((next_scheduled_wakeup_time >= 0.0) &&
      (next_scheduled_wakeup_time > (when-0.1)) &&
      (next_scheduled_wakeup_time < (when+0.1)))
    when = next_scheduled_wakeup_time;
  next_status_update_time = when;  
  install_next_scheduled_wakeup();
}

/*****************************************************************************/
/*               kdms_renderer::install_next_scheduled_wakeup                */
/*****************************************************************************/

void kdms_renderer::install_next_scheduled_wakeup()
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
        window_manager->schedule_wakeup(window,-1.0); // Cancel existing wakeup
      next_scheduled_wakeup_time = -100.0; // Make sure it cannot pass any
        // of the tests in the above functions, which coerce a loose wakeup
        // time to equal the next scheduled wakeup time if it is already close.
    }
  else if (min_time != next_scheduled_wakeup_time)
    {
      next_scheduled_wakeup_time = min_time;
      window_manager->schedule_wakeup(window,min_time);
    }
}

/*****************************************************************************/
/*                       kdms_renderer::increase_scale                       */
/*****************************************************************************/

float kdms_renderer::increase_scale(float from_scale, bool slightly)
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
  float new_scale =
    compositor->find_optimal_scale(region_basis,scale_anchor,
                                   min_scale,max_scale);
  if (new_scale < min_scale)
    max_rendering_scale = new_scale; // This is as large as we can go
  return new_scale;
}

/*****************************************************************************/
/*                       kdms_renderer::decrease_scale                       */
/*****************************************************************************/

float kdms_renderer::decrease_scale(float from_scale, bool slightly)
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
  float new_scale =
    compositor->find_optimal_scale(region_basis,scale_anchor,
                                   min_scale,max_scale);
  if (new_scale > max_scale)
    min_rendering_scale = new_scale; // This is as small as we can go
  return new_scale;
}

/*****************************************************************************/
/*                      kdms_renderer::change_frame_idx                      */
/*****************************************************************************/

void kdms_renderer::change_frame_idx(int new_frame_idx)
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
/*                 kdms_renderer::find_compatible_frame_idx                  */
/*****************************************************************************/

int
  kdms_renderer::find_compatible_frame_idx(int num_streams, const int *streams,
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
/*                      kdms_renderer::adjust_rel_focus                      */
/*****************************************************************************/

bool kdms_renderer::adjust_rel_focus()
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
/*             kdms_renderer::calculate_rel_view_and_focus_params            */
/*****************************************************************************/

void kdms_renderer::calculate_rel_view_and_focus_params()
{
  view_centre_known = false; rel_focus.reset();
  if ((buffer == NULL) || !image_dims)
    return;
  view_centre_known = true;
  view_centre_x =
    (float)(view_dims.pos.x + view_dims.size.x/2 - image_dims.pos.x) /
    ((float) image_dims.size.x);
  view_centre_y =
    (float)(view_dims.pos.y + view_dims.size.y/2 - image_dims.pos.y) /
    ((float) image_dims.size.y);
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
/*                  kdms_renderer::scroll_to_view_anchors                    */
/*****************************************************************************/

void kdms_renderer::scroll_to_view_anchors()
{
  if (!view_centre_known)
    return;
  NSSize doc_size = [doc_view frame].size;
  NSRect view_rect = [scroll_view documentVisibleRect];
  NSPoint new_view_origin;
  new_view_origin.x = (float)
    floor(0.5 + doc_size.width * view_centre_x - 0.5F*view_rect.size.width);
  new_view_origin.y = (float)
    floor(0.5+doc_size.height*(1.0F-view_centre_y)-0.5F*view_rect.size.height);
  view_centre_known = false;
  [doc_view scrollPoint:new_view_origin];
}

/*****************************************************************************/
/*                 kdms_renderer::scroll_relative_to_view                    */
/*****************************************************************************/

void kdms_renderer::scroll_relative_to_view(float h_scroll, float v_scroll)
{
  NSSize doc_size = [doc_view frame].size;
  NSRect view_rect = [scroll_view documentVisibleRect];
  NSPoint new_view_origin = view_rect.origin;
  
  h_scroll *= view_rect.size.width;
  v_scroll *= view_rect.size.height;
  if (h_scroll > 0.0F)
    h_scroll = ((float) ceil(h_scroll / pixel_scale)) * pixel_scale;
  else if (h_scroll < 0.0F)
    h_scroll = ((float) floor(h_scroll / pixel_scale)) * pixel_scale;
  if (v_scroll > 0.0F)
    v_scroll = ((float) ceil(v_scroll / pixel_scale)) * pixel_scale;
  else if (v_scroll < 0.0F)
    v_scroll = ((float) floor(v_scroll / pixel_scale)) * pixel_scale;
  
  new_view_origin.x += h_scroll;
  new_view_origin.y -= v_scroll;
  if (new_view_origin.x < 0.0F)
    new_view_origin.x = 0.0F;
  if ((new_view_origin.x + view_rect.size.width) > doc_size.width)
    new_view_origin.x = doc_size.width - view_rect.size.width;
  if ((new_view_origin.y + view_rect.size.height) > doc_size.height)
    new_view_origin.y = doc_size.height - view_rect.size.height;
  [doc_view scrollPoint:new_view_origin];
}

/*****************************************************************************/
/*                     kdms_renderer::scroll_absolute                        */
/*****************************************************************************/

void kdms_renderer::scroll_absolute(float h_scroll, float v_scroll,
                                    bool use_doc_coords)
{
  if (!use_doc_coords)
    {
      h_scroll *= pixel_scale;
      v_scroll *= pixel_scale;
      v_scroll = -v_scroll;
    }
  
  NSSize doc_size = [doc_view frame].size;
  NSRect view_rect = [scroll_view documentVisibleRect];
  NSPoint new_view_origin = view_rect.origin;
  new_view_origin.x += h_scroll;
  new_view_origin.y += v_scroll;
  if (new_view_origin.x < 0.0F)
    new_view_origin.x = 0.0F;
  if ((new_view_origin.x + view_rect.size.width) > doc_size.width)
    new_view_origin.x = doc_size.width - view_rect.size.width;
  if ((new_view_origin.y + view_rect.size.height) > doc_size.height)
    new_view_origin.y = doc_size.height - view_rect.size.height;
  [doc_view scrollPoint:new_view_origin];
}

/*****************************************************************************/
/*                     kdms_renderer::get_new_focus_box                      */
/*****************************************************************************/

kdu_dims kdms_renderer::get_new_focus_box()
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
    new_focus_box.pos.x =
      view_dims.pos.x+view_dims.size.x-new_focus_box.size.x;
  if (new_focus_box.pos.y < view_dims.pos.y)
    new_focus_box.pos.y = view_dims.pos.y;
  if ((new_focus_box.pos.y+new_focus_box.size.y) >
      (view_dims.pos.y+view_dims.size.y))
    new_focus_box.pos.y =
      view_dims.pos.y+view_dims.size.y-new_focus_box.size.y;
  new_focus_box &= view_dims;
  
  if (!new_focus_box)
    new_focus_box = view_dims;
  return new_focus_box;
}

/*****************************************************************************/
/*                      kdms_renderer::set_focus_box                         */
/*****************************************************************************/

void kdms_renderer::set_focus_box(kdu_dims new_box)
{
  rel_focus.reset();
  if ((compositor == NULL) || (buffer == NULL))
    return;
  kdu_dims previous_box = focus_box;
  bool previous_enable = enable_focus_box;
  lock_view();
  if (new_box.area() < (kdu_long) 100)
    enable_focus_box = false;
  else
    { focus_box = new_box; enable_focus_box = true; }
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
        box.pos.x  -= KDMS_FOCUS_MARGIN;    box.pos.y  -= KDMS_FOCUS_MARGIN;
        box.size.x += 2*KDMS_FOCUS_MARGIN;  box.size.y += 2*KDMS_FOCUS_MARGIN;
        compositor->invalidate_rect(box);
      }
  if (jpip_client != NULL)
    update_client_window_of_interest();
  display_status();
}

/*****************************************************************************/
/*                    kdms_renderer::need_focus_outline                      */
/*****************************************************************************/

bool kdms_renderer::need_focus_outline(kdu_dims *focus_box)
{
  *focus_box = this->focus_box;
  return (enable_focus_box && !shape_istream);
}

/*****************************************************************************/
/*                      kdms_renderer::update_focus_box                      */
/*****************************************************************************/

void kdms_renderer::update_focus_box(bool view_may_be_scrolling)
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
      if ((previous_enable != enable_focus_box) ||
          (enable_focus_box && view_may_be_scrolling))
        compositor->invalidate_rect(view_dims);
      else if (enable_focus_box)
        for (int w=0; w < 2; w++)
          {
            kdu_dims box = (w==0)?previous_box:focus_box;
            box.pos.x -=  KDMS_FOCUS_MARGIN;   box.pos.y -=  KDMS_FOCUS_MARGIN;
            box.size.x+=2*KDMS_FOCUS_MARGIN; box.size.y+=2*KDMS_FOCUS_MARGIN;
            compositor->invalidate_rect(box);
          }
    }
  if (enable_region_posting)
    update_client_window_of_interest();
}

/*****************************************************************************/
/*            kdms_renderer::check_initial_request_compatibility             */
/*****************************************************************************/

bool kdms_renderer::check_initial_request_compatibility()
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
/*              kdms_renderer::update_client_window_of_interest              */
/*****************************************************************************/

void kdms_renderer::update_client_window_of_interest()
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
      if ([catalog_source generate_client_requests:&tmp_roi] > 0)
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
      bool catalog_is_first_responder = ![doc_view is_first_responder];
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
  if (jpip_client->post_window(&tmp_roi,client_request_queue_id,true,
                               &client_prefs))
    client_roi.copy_from(tmp_roi);
}

/*****************************************************************************/
/*                    kdms_renderer::change_client_focus                     */
/*****************************************************************************/

void kdms_renderer::change_client_focus(kdu_coords actual_resolution,
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
/*                      kdms_renderer::set_codestream                        */
/*****************************************************************************/

void kdms_renderer::set_codestream(int codestream_idx)
{
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                  kdms_renderer::set_compositing_layer                     */
/*****************************************************************************/

void kdms_renderer::set_compositing_layer(int layer_idx)
{
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                  kdms_renderer::set_compositing_layer                     */
/*****************************************************************************/

void kdms_renderer::set_compositing_layer(kdu_coords point)
{
  if (compositor == NULL)
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
  point += view_dims.pos;
  kdu_ilayer_ref ilyr = compositor->find_point(point);
  int layer_idx, cs_idx; bool is_opaque;
  if ((compositor->get_ilayer_info(ilyr,layer_idx,cs_idx,is_opaque) > 0) &&
      (layer_idx >= 0))
    set_compositing_layer(layer_idx);
}

/*****************************************************************************/
/*                        kdms_renderer::set_frame                           */
/*****************************************************************************/

bool kdms_renderer::set_frame(int new_frame_idx)
{
  if ((compositor == NULL) ||
      (single_component_idx >= 0) ||
      ((single_layer_idx >= 0) && jpx_in.exists()))
    return false;
  if (shape_istream.exists())
    { NSBeep(); return false; }
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
      buffer = compositor->get_composition_buffer(buffer_dims);
      if (enable_focus_box)
        {
          rel_focus.frame_idx = old_frame_idx;
          if (adjust_rel_focus() && enable_focus_box &&
              rel_focus.get_centre_if_valid(view_centre_x,view_centre_y))
            {
              view_centre_known = true;
              scroll_to_view_anchors();
              update_focus_box(false);
              view_centre_known = false;
            }
          rel_focus.reset();
        }
      if (jpip_client != NULL)
        update_client_window_of_interest();
      if (overlays_enabled && !in_playmode)
        {
          jpx_metanode node;
          if (catalog_source &&
              (node=[catalog_source get_selected_metanode]).exists())
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
    { // Have to initial configuration from scratch
      calculate_rel_view_and_focus_params();
      rel_focus.frame_idx = old_frame_idx;
      invalidate_configuration();
      initialize_regions(false);
    }
  return true;
}

/*****************************************************************************/
/*                      kdms_renderer::stop_playmode                         */
/*****************************************************************************/

void kdms_renderer::stop_playmode()
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
      buffer = compositor->get_composition_buffer(buffer_dims);
      compositor->refresh(); // Make sure the surface is properly initialized
    }
  display_status();
}

/*****************************************************************************/
/*                     kdms_renderer::start_playmode                         */
/*****************************************************************************/

void kdms_renderer::start_playmode(bool reverse)
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
  this->playclock_base = get_current_time();
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
  size_window_for_image_dims(false);
  view_centre_known = false;
  rel_focus.reset();
}

/*****************************************************************************/
/*                kdms_renderer::set_metadata_catalog_source                 */
/*****************************************************************************/

void kdms_renderer::set_metadata_catalog_source(kdms_catalog *source)
{
  if ((source == nil) && (this->catalog_source != nil))
    catalog_closed = true;
  if (this->catalog_source != source)
    {
      if (this->catalog_source != nil)
        {
          [this->catalog_source deactivate]; // Essential to do this first
          [this->catalog_source release];
        }
      if (source != nil)
        [source retain];
    }
  this->catalog_source = source;
  if ((source != nil) && jpx_in.exists())
    {
      [source update_metadata];
      catalog_closed = false;
    }
}

/*****************************************************************************/
/*                 kdms_renderer::reveal_metadata_at_point                   */
/*****************************************************************************/

void kdms_renderer::reveal_metadata_at_point(kdu_coords point, bool dbl_click)
{
  if ((compositor == NULL) || (!jpx_in) ||
      ((catalog_source == nil) && (metashow == nil) && !dbl_click))
    return;
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
                                                     false,kdu_dims(),
                                                     0)).exists())
                { 
                  int num_cs, num_layers; bool rres;
                  if (!scan.get_numlist_info(num_cs,num_layers,rres))
                    continue;
                  if (catalog_source)
                    nd = [catalog_source find_best_catalogued_descendant:scan
                                                            and_distance:dist
                                                       excluding_regions:true];
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
      if (metashow != nil)
        [metashow select_matching_metanode:node];
      if (catalog_source != nil)
        [catalog_source select_matching_metanode:node];
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
/*                  kdms_renderer::edit_metadata_at_point                    */
/*****************************************************************************/

void kdms_renderer::edit_metadata_at_point(kdu_coords *point_ref)
{
  if ((compositor == NULL) || !jpx_in)
    return;
  
  kdu_coords point; // Will hold mouse location in `view_dims' coordinates
  if (point_ref != NULL)
    point = *point_ref;
  else
    {
      NSRect ns_rect;
      ns_rect.origin = [window mouseLocationOutsideOfEventStream];
      ns_rect.origin = [doc_view convertPoint:ns_rect.origin fromView:nil];
      ns_rect.size.width = ns_rect.size.height = 0.0F;
      point = convert_region_from_doc_view(ns_rect).pos;
    }
  
  kdu_ilayer_ref ilyr;
  kdu_istream_ref istr;
  jpx_meta_manager manager = jpx_in.access_meta_manager();
  jpx_metanode roi_node;
  if (overlays_enabled)
    roi_node = compositor->search_overlays(point,istr,0.1F);
  kdms_matching_metalist *metalist = new kdms_matching_metalist;
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
    file_server = new kdms_file_services(open_file_pathname);
  if (editor == nil)
    {
      editor = [kdms_metadata_editor alloc];
      bool can_edit = (jpip_client == NULL) || !jpip_client->is_alive(-1);
      NSRect wnd_frame = [window frame];
      NSPoint preferred_location = wnd_frame.origin;
      preferred_location.x += wnd_frame.size.width + 10.0F;
      preferred_location.y += wnd_frame.size.height;
      [editor initWithSource:(&jpx_in)
                fileServices:file_server
                    editable:((can_edit)?YES:NO)
                    owner:this
                    location:preferred_location
               andIdentifier:window_identifier];
    }
  [editor configureWithEditList:metalist];
}

/*****************************************************************************/
/*                      kdms_renderer::edit_metanode                         */
/*****************************************************************************/

void kdms_renderer::edit_metanode(jpx_metanode node,
                                  bool include_roi_or_numlist)
{
  if ((compositor == NULL) || (!jpx_in) || (!node) || node.is_deleted())
    return;
  kdms_matching_metalist *metalist = new kdms_matching_metalist;
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
          metalist = new kdms_matching_metalist;
          metalist->append_node_expanding_numlists_and_free_boxes(edit_root);
        }
    }
  
  if (file_server == NULL)
    file_server = new kdms_file_services(open_file_pathname);
  if (editor == nil)
    {
      editor = [kdms_metadata_editor alloc];
      bool can_edit = (jpip_client == NULL) || !jpip_client->is_alive(-1);
      NSRect wnd_frame = [window frame];
      NSPoint preferred_location = wnd_frame.origin;
      preferred_location.x += wnd_frame.size.width + 10.0F;
      preferred_location.y += wnd_frame.size.height;
      [editor initWithSource:(&jpx_in)
                fileServices:file_server
                    editable:((can_edit)?YES:NO)
                    owner:this
                    location:preferred_location
               andIdentifier:window_identifier];
    }
  [editor configureWithEditList:metalist];
  [editor setActiveNode:editor_startup_node];
}

/*****************************************************************************/
/*                kdms_renderer::flash_imagery_for_metanode                  */
/*****************************************************************************/

bool
  kdms_renderer::flash_imagery_for_metanode(jpx_metanode node)
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
/*                kdms_renderer::show_imagery_for_metanode                   */
/*****************************************************************************/

bool
  kdms_renderer::show_imagery_for_metanode(jpx_metanode node,
                                           kdu_istream_ref *istream_ref)
{
  if (shape_istream.exists())
    { pending_show_metanode = jpx_metanode(); NSBeep(); return false; }
  if (istream_ref != NULL)
    *istream_ref = kdu_istream_ref();
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
          if (istream_ref != NULL)
            *istream_ref = compositor->get_next_istream(kdu_istream_ref());
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
          if (istream_ref != NULL)
            *istream_ref = compositor->get_next_istream(kdu_istream_ref());
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

  // Find compatible ilayer and istream references on the current composition
  // as appropriate.
  kdu_ilayer_ref compatible_ilayer;
  if (compatible_codestream_layer >= 0)
    { // This branch will always be followed if `frame' was non-NULL, except
      // if `have_unassociated_roi' was true.
      compatible_ilayer =
        compositor->get_next_ilayer(compatible_ilayer,
                                    compatible_codestream_layer);
    }
  else if (single_layer_idx >= 0)
    compatible_ilayer =
      compositor->get_next_ilayer(compatible_ilayer,single_layer_idx);
  kdu_istream_ref compatible_istream;
  if (compatible_ilayer.exists() && !have_unassociated_roi)
    {
      if (compatible_codestream >= 0)
        compatible_istream =
          compositor->get_ilayer_stream(compatible_ilayer,-1,
                                        compatible_codestream);
      else
        compatible_istream=compositor->get_ilayer_stream(compatible_ilayer,0);
    }
  else if ((single_codestream_idx >= 0) && !have_unassociated_roi)
    compatible_istream = compositor->get_next_istream(compatible_istream,
                                                      single_codestream_idx);
  if (istream_ref != NULL)
    *istream_ref = compatible_istream;
  
  kdu_dims roi_region;
  kdu_dims mapped_roi_region;
  if (have_compatible_layer && configuration_complete && (num_layers == 1) &&
      (frame != NULL) && (!roi) && compatible_ilayer.exists())
    { // See if we should create a fake region of interest, to identify
      // a compositing layer within its frame.
      mapped_roi_region =
        compositor->find_ilayer_region(compatible_ilayer,true);
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
      roi_istr = compatible_istream;
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
  set_focus_box(mapped_roi_region);
  
  if (can_cancel_pending_show)
    cancel_pending_show();
  return can_cancel_pending_show;
}

/*****************************************************************************/
/*                    kdms_renderer::cancel_pending_show                     */
/*****************************************************************************/

void kdms_renderer::cancel_pending_show()
{
  pending_show_metanode = jpx_metanode();
}

/*****************************************************************************/
/*                     kdms_renderer::metadata_changed                       */
/*****************************************************************************/

void kdms_renderer::metadata_changed(bool new_data_only)
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
  if (metashow != nil)
    [metashow update_metadata];
  if ((catalog_source == nil) && jpx_in.exists() &&
      jpx_in.access_meta_manager().peek_touched_nodes(jp2_label_4cc).exists())
    [window create_metadata_catalog];
  else if (catalog_source != nil)
    [catalog_source update_metadata];
}

/*****************************************************************************/
/*                    kdms_renderer::set_shape_editor                        */
/*****************************************************************************/

bool
  kdms_renderer::set_shape_editor(jpx_roi_editor *ed,
                                  kdu_istream_ref ed_istream,
                                  bool hide_path_segments,
                                  int fixed_path_thickness,
                                  bool scribble_mode)
{
  if (ed == NULL)
    { shape_editor = NULL; shape_istream = kdu_istream_ref(); }
  else
    { 
      this->shape_editor = ed;
      this->shape_istream = ed_istream;
      assert(ed_istream.exists());
      this->hide_shape_path = hide_path_segments;
    }
  this->fixed_shape_path_thickness = fixed_path_thickness;
  this->shape_scribbling_mode = scribble_mode;
  if (compositor != NULL)
    compositor->invalidate_rect(view_dims);
  return (shape_editor != NULL);
}

/*****************************************************************************/
/*                   kdms_renderer::update_shape_editor                      */
/*****************************************************************************/

void
  kdms_renderer::update_shape_editor(kdu_dims region, bool hide_path_segments,
                                     int fixed_path_thickness,
                                     bool scribble_mode)
{
  if (shape_editor == NULL)
    return;
  if (region.is_empty() || (this->hide_shape_path != hide_path_segments))
    { 
      shape_editor->get_bounding_box(region);
      this->hide_shape_path = hide_path_segments;
    }
  this->fixed_shape_path_thickness = fixed_path_thickness;
  this->shape_scribbling_mode = scribble_mode;
  [doc_view repaint_shape_region:region];
}

/*****************************************************************************/
/*               kdms_renderer::nudge_selected_shape_points                  */
/*****************************************************************************/

bool
  kdms_renderer::nudge_selected_shape_points(int delta_x, int delta_y,
                                             kdu_dims &update_region)
{
  kdu_dims regn;  regn.size.x=regn.size.y=1;
  int ninst; // not used
  if ((shape_editor == NULL) || (compositor == NULL) ||
      (shape_editor->get_selection(regn.pos,ninst) < 0) ||
      (regn=compositor->inverse_map_region(regn,shape_istream)).is_empty())
    return false;
  regn.pos.x += delta_x*regn.size.x;
  regn.pos.y += delta_y*regn.size.y;
  regn.size.x=regn.size.y=1;
  if (!compositor->map_region(regn,shape_istream))
    return false;
  update_region = shape_editor->move_selected_anchor(regn.pos);
  return true;
}

/*****************************************************************************/
/*                    kdms_renderer::get_shape_anchor                        */
/*****************************************************************************/

int kdms_renderer::get_shape_anchor(NSPoint &point, int which,
                                    bool sel_only, bool dragged)
{
  int result;
  kdu_coords pt;
  if ((shape_editor == NULL) || (compositor == NULL) ||
      !((result=shape_editor->get_anchor(pt,which,sel_only,dragged)) &&
        map_shape_point_to_doc_view(pt,point)))
    result = 0;
  return result;
}

/*****************************************************************************/
/*                     kdms_renderer::get_shape_edge                         */
/*****************************************************************************/

int kdms_renderer::get_shape_edge(NSPoint &from, NSPoint &to, int which,
                                  bool sel_only, bool dragged)
{
  int result;
  kdu_coords v1, v2;
  if ((shape_editor == NULL) || (compositor == NULL) ||
      !(result=shape_editor->get_edge(v1,v2,which,sel_only,dragged,true)))
    return 0;
  if (!(map_shape_point_to_doc_view(v1,from) &&
        map_shape_point_to_doc_view(v2,to)))
    return 0;
  int cs_idx; bool tran=false, vf=false, hf=false;
  compositor->get_istream_info(shape_istream,cs_idx,NULL,NULL,4,NULL,
                               NULL,NULL,&tran,&vf,&hf);
  if (transpose ^ tran ^ vflip ^ vf & hflip ^ hf)
    { NSPoint tmp=from; from=to; to=tmp; }
  return result;
}

/*****************************************************************************/
/*                    kdms_renderer::get_shape_curve                         */
/*****************************************************************************/

int kdms_renderer::get_shape_curve(NSPoint &centre, NSSize &extent,
                                   double &alpha, int which,
                                   bool sel_only, bool dragged)
{
  int result;
  kdu_coords ctr, ext, skew;
  if ((shape_editor == NULL) || (compositor == NULL) ||
      !(result=shape_editor->get_curve(ctr,ext,skew,which,sel_only,dragged)))
    return 0;
  kdu_coords tl=ctr-ext, br=ctr+ext;
  skew += ctr;
  NSPoint mapped_tl, mapped_br, mapped_skew;
  if (!(map_shape_point_to_doc_view(ctr,centre) &&
        map_shape_point_to_doc_view(tl,mapped_tl) &&
        map_shape_point_to_doc_view(br,mapped_br) &&
        map_shape_point_to_doc_view(skew,mapped_skew)))
    return 0;
  extent.width = 0.5F*(mapped_br.x - mapped_tl.x);
  extent.height = 0.5F*(mapped_br.y - mapped_tl.y);
  if (extent.width < 0.0)
    extent.width = -extent.width;
  bool vflip = true;
  if (extent.height < 0.0)
    {
      extent.height = -extent.height;
      vflip = false;
    }
  mapped_skew.x -= centre.x;
  mapped_skew.y -= centre.y;
  alpha = mapped_skew.x / extent.height;
  double mapped_gamma_sq = (mapped_skew.y/extent.width)*alpha;
  if (mapped_gamma_sq < 0.0)
    mapped_gamma_sq = -mapped_gamma_sq;
  if (mapped_gamma_sq >= 1.0)
    extent.width = 0.1;
  else if ((extent.width *= sqrt(1.0 - mapped_gamma_sq)) < 0.1)
    extent.width = 0.1;
  if (vflip)
    alpha = -alpha;
  return result;
}

/*****************************************************************************/
/*                     kdms_renderer::get_shape_path                         */
/*****************************************************************************/

int kdms_renderer::get_shape_path(NSPoint &from, NSPoint &to, int which)
{
  int result; kdu_coords v1, v2;
  if ((shape_editor == NULL) || hide_shape_path || (compositor == NULL) ||
      !(result=shape_editor->get_path_segment(v1,v2,which)))
    return 0;
  if (!(map_shape_point_to_doc_view(v1,from) &&
        map_shape_point_to_doc_view(v2,to)))
    return 0;
  return result;      
}

/*****************************************************************************/
/*                        kdms_renderer::save_over                           */
/*****************************************************************************/

bool kdms_renderer::save_over()
{
  const char *save_pathname =
    window_manager->get_save_file_pathname(open_file_pathname);
  const char *suffix = strrchr(open_filename,'.');
  
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
  catch (kdu_exception)
  {
    window_manager->declare_save_file_invalid(save_pathname);
    return false;
  }
  return true;
}

/*****************************************************************************/
/*                       kdms_renderer::save_as_jp2                          */
/*****************************************************************************/

void kdms_renderer::save_as_jp2(const char *filename)
{
  jp2_family_tgt tgt; 
  jp2_target jp2_out;
  jpx_input_box stream_box;
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
      
      stream_in.open_stream(&stream_box);
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
      catch (kdu_exception exc)
      {
        if (cs_in.exists())
          cs_in.destroy();
        throw exc;
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
      catch (kdu_exception exc)
      {
        delete[] buffer;
        throw exc;
      }
      delete[] buffer;
    }
  stream_box.close();
  jp2_out.close();
  tgt.close();
}

/*****************************************************************************/
/*                       kdms_renderer::save_as_jpx                          */
/*****************************************************************************/

void kdms_renderer::save_as_jpx(const char *filename,
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
        jpx_out.write_headers(NULL,NULL,n);
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
          { // Need to generate output codestreams from scratch -- transcoding
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
                  for (length=0; (xfer_bytes=box_in.read(buffer,1<<20)) > 0; )
                    length += xfer_bytes;
                const char *sep1 = strrchr(filename,'/');
                const char *sep2 = strrchr(open_file_pathname,'/');
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
        kdms_file *file;
        if ((file_server != NULL) && (addr_param == (void *)file_server) &&
            ((file = file_server->find_file(i_param)) != NULL))
          write_metadata_box_from_file(metadata_container,file);
      }
  }
  catch (kdu_exception exc)
  {
    delete[] in_streams;
    delete[] out_streams;
    delete[] buffer;
    if (cs_in.exists())
      cs_in.destroy();
    throw exc;
  }
  
  // Clean up
  delete[] in_streams;
  delete[] out_streams;
  delete[] buffer;
  jpx_out.close();
  tgt.close();
}

/*****************************************************************************/
/*                       kdms_renderer::save_as_raw                          */
/*****************************************************************************/

void kdms_renderer::save_as_raw(const char *filename)
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
/*                     kdms_renderer::copy_codestream                        */
/*****************************************************************************/

void kdms_renderer::copy_codestream(kdu_compressed_target *tgt,
                                    kdu_codestream src)
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
  catch (kdu_exception exc)
  {
    if (dst.exists())
      dst.destroy();
    throw exc;
  }
  dst.destroy();
}


/*****************************************************************************/
/*                  kdms_renderer::menu_FileDisconnectURL                    */
/*****************************************************************************/

void kdms_renderer::menu_FileDisconnectURL()
{
  if ((jpip_client != NULL) && jpip_client->is_alive(client_request_queue_id))
    jpip_client->disconnect(false,30000,client_request_queue_id,false);
             // Give it a long timeout so as to avoid accidentally breaking
             // an HTTP request channel which might be shared with other
             // request queues -- but let's not wait around here.
}

/*****************************************************************************/
/*                      kdms_renderer::menu_FileSave                         */
/*****************************************************************************/

void kdms_renderer::menu_FileSave()
{
  if ((compositor == NULL) || in_playmode || mj2_in.exists() ||
      (!have_unsaved_edits) || (open_file_pathname == NULL))
    return;
  if (!save_without_asking)
    {
      int response =
        interrogate_user("You are about to save to the file which you are "
                         "currently viewing.  In practice, the saved data "
                         "will be written to a temporary file which will "
                         "not replace the open file until after you close "
                         "it the window or load another file into it.  "
                         "Nevertheless, this operation will eventually "
                         "overwrite the existing image file.  In the process, "
                         "there is a chance that you might lose some "
                         "metadata which could not be internally "
                         "represented.  Are you sure you wish to do this?",
                         "Cancel","Proceed","Don't ask me again");
      if (response == 0)
        return;
      if (response == 2)
        save_without_asking = true;
    }
  save_over();
}

/*****************************************************************************/
/*                   kdms_renderer::menu_FileSaveReload                      */
/*****************************************************************************/

void kdms_renderer::menu_FileSaveReload()
{
  if (editor != nil)
    {
      if (interrogate_user("You have an open metadata editor, which will "
                           "be closed if you continue.  Do you still want "
                           "to save and reload the file?",
                           "Cancel","Continue") == 0)
        return;
      [editor close];
    }
  
  if (window_manager->get_open_file_retain_count(open_file_pathname) != 1)
    {
      interrogate_user("Cannot perform this operation at present.  There "
                       "are other open windows in the application which "
                       "are also using this same file -- you may not "
                       "overwrite and reload it until they are closed.",
                       "OK");
      return;
    }
  if (!save_over())
    return;
  NSString *filename = [NSString stringWithUTF8String:open_file_pathname];
  bool save_without_asking = this->save_without_asking;
  close_file();
  open_file([filename UTF8String]);
  this->save_without_asking = save_without_asking;
}

/*****************************************************************************/
/*                     kdms_renderer::menu_FileSaveAs                        */
/*****************************************************************************/

void kdms_renderer::menu_FileSaveAs(bool preserve_codestream_links,
                                    bool force_codestream_links)
{
  if ((compositor == NULL) || in_playmode)
    return;

  // Get the file name
  NSString *directory = nil;
  if (last_save_pathname != NULL)
    directory = [NSString stringWithUTF8String:last_save_pathname];
  else if (open_file_pathname != NULL)
    directory = [NSString stringWithUTF8String:open_file_pathname];
  if (directory != NULL)
    directory = [directory stringByDeletingLastPathComponent];

  NSString *suggested_file = nil;
  const char *suffix = NULL;
  if (last_save_pathname != NULL)
    {
      suggested_file = [NSString stringWithUTF8String:last_save_pathname];
      suffix = strrchr(last_save_pathname,'.');
      suggested_file = [suggested_file lastPathComponent];
    }
  else
    {
      suggested_file = [NSString stringWithUTF8String:open_filename];
      suffix = strrchr(open_filename,'.');
      suggested_file =
        [suggested_file stringByReplacingPercentEscapesUsingEncoding:
         NSASCIIStringEncoding];
    }
  if (suggested_file == nil)
    suggested_file = @"";
  
  int s;
  char suggested_file_suffix[5] = {'\0','\0','\0','\0','\0'};
  if ((suffix != NULL) && (strlen(suffix) <= 4)) 
    for (s=0; (s < 4) && (suffix[s+1] != '\0'); s++)
      suggested_file_suffix[s] = (char) tolower(suffix[s+1]);
  
  NSArray *file_types = nil;
  if (jpx_in.exists())
    {
      if (force_codestream_links)
        file_types = [NSArray arrayWithObjects:@"jpx",@"jpf",nil];
      else
        file_types = [NSArray arrayWithObjects:
                      @"jp2",@"jpx",@"jpf",@"j2c",@"j2k",nil];
      if (have_unsaved_edits &&
          (strcmp(suggested_file_suffix,"jpx") != 0) &&
          (strcmp(suggested_file_suffix,"jpf") != 0))
        suggested_file = [[suggested_file stringByDeletingPathExtension]
                          stringByAppendingPathExtension:@"jpx"];
    }
  else if (mj2_in.exists())
    {
      file_types = [NSArray arrayWithObjects:@"jp2",@"j2c",@"j2k",nil];
      if (strcmp(suggested_file_suffix,"mj2") != 0)
        suggested_file = [[suggested_file stringByDeletingPathExtension]
                          stringByAppendingPathExtension:@"jp2"];
    }
  else
    {
      file_types = [NSArray arrayWithObjects:@"j2c",@"j2k",nil]; 
      if ((strcmp(suggested_file_suffix,"j2c") != 0) &&
          (strcmp(suggested_file_suffix,"j2k") != 0))
        suggested_file = [[suggested_file stringByDeletingPathExtension]
                          stringByAppendingPathExtension:@"j2c"];
    }
  
  NSSavePanel *panel = [NSSavePanel savePanel];
  [panel setTitle:@"Select filename for saving"];
  [panel setCanCreateDirectories:YES];
  [panel setCanSelectHiddenExtension:YES];
  [panel setExtensionHidden:NO];
  [panel setAllowedFileTypes:file_types];
  [panel setAllowsOtherFileTypes:NO];
  if ([panel runModalForDirectory:directory file:suggested_file] != NSOKButton)
    return;
  
  // Remember the last saved file name
  const char *chosen_pathname = [[panel filename] UTF8String];
  suffix = strrchr(chosen_pathname,'.');
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
  try { // Protect the entire file writing process against exceptions
    bool may_overwrite_link_source = false;
    if (force_codestream_links && jpx_suffix && (open_file_pathname != NULL))
      if (kdms_compare_file_pathnames(chosen_pathname,open_file_pathname))
        may_overwrite_link_source = true;
    if (jpx_in.exists() && !jp2_family_in.uses_cache())
      {
        jp2_data_references data_refs;
        if (jpx_in.exists())
          data_refs = jpx_in.access_data_references();
        if (data_refs.exists())
          {
            int u;
            const char *url_pathname;
            for (u=0; (url_pathname = data_refs.get_file_url(u)) != NULL; u++)
              if (kdms_compare_file_pathnames(chosen_pathname,url_pathname))
                may_overwrite_link_source = true;
          }
      }
    if (may_overwrite_link_source)
      { kdu_error e; e << "You are about to overwrite an existing file "
        "which is being linked by (or potentially about to be linked by) "
        "the file you are saving.  This operation is too dangerous to "
        "attempt."; }
    save_pathname = window_manager->get_save_file_pathname(chosen_pathname);
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
  catch (kdu_exception)
  {
    if (save_pathname != NULL)
      window_manager->declare_save_file_invalid(save_pathname);
  }
}

/*****************************************************************************/
/*                   kdms_renderer::menu_FileProperties                      */
/*****************************************************************************/

void kdms_renderer::menu_FileProperties() 
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
  
  NSRect my_rect = [window frame];
  NSPoint top_left_point = my_rect.origin;
  top_left_point.x += my_rect.size.width * 0.1F;
  top_left_point.y += my_rect.size.height * 0.9F;
  
  kdms_properties *properties_dialog = [kdms_properties alloc];
  if (![properties_dialog initWithTopLeftPoint:top_left_point
                                the_codestream:codestream
                              codestream_index:c_idx])
    close_file();
  else
    [properties_dialog run_modal];
  [properties_dialog release];
}

/*****************************************************************************/
/*                      kdms_renderer::menu_ViewVflip                        */
/*****************************************************************************/

void kdms_renderer::menu_ViewVflip() 
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
/*                      kdms_renderer::menu_ViewHflip                        */
/*****************************************************************************/

void kdms_renderer::menu_ViewHflip() 
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
/*                     kdms_renderer::menu_ViewRotate                        */
/*****************************************************************************/

void kdms_renderer::menu_ViewRotate() 
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
/*                 kdms_renderer::menu_ViewCounterRotate                     */
/*****************************************************************************/

void kdms_renderer::menu_ViewCounterRotate() 
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
/*                     kdms_renderer::menu_ViewZoomIn                        */
/*****************************************************************************/

void kdms_renderer::menu_ViewZoomIn() 
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
/*                     kdms_renderer::menu_ViewZoomOut                       */
/*****************************************************************************/

void kdms_renderer::menu_ViewZoomOut()
{
  if (buffer == NULL)
    return;
  if (rendering_scale == min_rendering_scale)
    return;
  calculate_rel_view_and_focus_params();
  rendering_scale = decrease_scale(rendering_scale,false);
  initialize_regions(true);
}

/*****************************************************************************/
/*                  kdms_renderer::menu_ViewZoomInSlightly                   */
/*****************************************************************************/

void kdms_renderer::menu_ViewZoomInSlightly() 
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
/*                  kdms_renderer::menu_ViewZoomOutSlightly                  */
/*****************************************************************************/

void kdms_renderer::menu_ViewZoomOutSlightly()
{
  if (buffer == NULL)
    return;
  if (rendering_scale == min_rendering_scale)
    return;
  calculate_rel_view_and_focus_params();
  rendering_scale = decrease_scale(rendering_scale,true);
  initialize_regions(true);
}

/*****************************************************************************/
/*                  kdms_renderer::menu_ViewOptimizeScale                    */
/*****************************************************************************/

void kdms_renderer::menu_ViewOptimizeScale()
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
/*                      kdms_renderer::menu_ViewWiden                        */
/*****************************************************************************/

void kdms_renderer::menu_ViewWiden() 
{
  NSSize view_size = [[scroll_view contentView] frame].size;
  NSSize doc_size = [doc_view frame].size;
  NSSize expand; // This is how much we want to grow window size
  expand.width = doc_size.width - view_size.width;
  if (expand.width > view_size.width)
    expand.width = view_size.width;
  expand.height = doc_size.height - view_size.height;
  if (expand.height > view_size.height)
    expand.height = view_size.height;
  
  NSRect frame_rect = [window frame];
  NSRect screen_rect = [[NSScreen mainScreen] visibleFrame];
  
  calculate_rel_view_and_focus_params();
  
  float min_x = screen_rect.origin.x;
  if (min_x > frame_rect.origin.x)
    min_x = frame_rect.origin.x;
  float lim_x = screen_rect.origin.x+screen_rect.size.width;
  if (lim_x < (frame_rect.origin.x+frame_rect.size.width))
    lim_x = frame_rect.origin.x+frame_rect.size.width;
  
  float min_y = screen_rect.origin.y;
  if (min_y > frame_rect.origin.y)
    min_y = frame_rect.origin.y;
  float lim_y = screen_rect.origin.y+screen_rect.size.height;
  if (lim_y < (frame_rect.origin.y+frame_rect.size.height))
    lim_y = frame_rect.origin.y+frame_rect.size.height;
  
  float add_to_width = screen_rect.size.width - frame_rect.size.width;
  if (add_to_width < 0.0F)
    add_to_width = 0.0F;
  else if (add_to_width > expand.width)
    add_to_width = expand.width;
  float add_to_height = screen_rect.size.height - frame_rect.size.height;
  if (add_to_height < 0.0F)
    add_to_height = 0.0F;
  else if (add_to_height > expand.height)
    add_to_height = expand.height;
  if ((add_to_width == 0.0F) && (add_to_height == 0.0))
    return;
  frame_rect.size.width += add_to_width;
  frame_rect.size.height += add_to_height;
  frame_rect.origin.x -= (float) floor(add_to_width*0.5);
  frame_rect.origin.y -= (float) floor(add_to_height*0.5);
  
  if (frame_rect.origin.x < min_x)
    frame_rect.origin.x = min_x;
  if (frame_rect.origin.y < min_y)
    frame_rect.origin.y = min_y;
  if ((frame_rect.origin.x+frame_rect.size.width) > lim_x)
    frame_rect.origin.x = lim_x - frame_rect.size.width;
  if ((frame_rect.origin.y+frame_rect.size.height) > lim_y)
    frame_rect.origin.y = lim_y - frame_rect.size.height;
  
  [window setFrame:frame_rect display:YES];
  scroll_to_view_anchors();
  view_centre_known = false;
  rel_focus.reset();
}

/*****************************************************************************/
/*                     kdms_renderer::menu_ViewShrink                        */
/*****************************************************************************/

void kdms_renderer::menu_ViewShrink() 
{
  NSSize view_size = [[scroll_view contentView] frame].size;
  NSSize new_view_size = view_size;
  new_view_size.width *= 0.5F;
  new_view_size.height *= 0.5F;
  if (new_view_size.width < 50.0F)
    new_view_size.width = 50.0F;
  if (new_view_size.height < 30.0F)
    new_view_size.height = 30.0F;

  NSSize half_reduce;
  half_reduce.width = floor((view_size.width - new_view_size.width) * 0.5F);
  half_reduce.height = floor((view_size.height - new_view_size.height) * 0.5F);
  if (half_reduce.width < 0.0F)
    half_reduce.width = 0.0F;
  if (half_reduce.height < 0.0F)
    half_reduce.height = 0.0F;
  
  calculate_rel_view_and_focus_params();
  rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
  NSRect frame_rect = [window frame];
  frame_rect.size.width -= 2.0F*half_reduce.width;
  frame_rect.size.height -= 2.0F*half_reduce.height;  
  frame_rect.origin.x += half_reduce.width;
  frame_rect.origin.y += half_reduce.height;
  
  [window setFrame:frame_rect display:YES];
  scroll_to_view_anchors();
  view_centre_known = false;
  rel_focus.reset();
}

/*****************************************************************************/
/*                     kdms_renderer::menu_ViewRestore                       */
/*****************************************************************************/

void kdms_renderer::menu_ViewRestore() 
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
/*                     kdms_renderer::menu_ViewRefresh                       */
/*****************************************************************************/

void kdms_renderer::menu_ViewRefresh()
{
  refresh_display();
}

/*****************************************************************************/
/*                    kdms_renderer::menu_ViewLayersLess                     */
/*****************************************************************************/

void kdms_renderer::menu_ViewLayersLess()
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
  else if (jpip_client == NULL)
    status_id = KDS_STATUS_LAYER_RES;
  compositor->set_max_quality_layers(max_display_layers);
  refresh_display();
  display_status();  
}

/*****************************************************************************/
/*                    kdms_renderer::menu_ViewLayersMore                     */
/*****************************************************************************/

void kdms_renderer::menu_ViewLayersMore()
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
      else if (jpip_client == NULL)
        status_id = KDS_STATUS_LAYER_RES;
      compositor->set_max_quality_layers(max_display_layers);
      display_status();
    }  
}

/*****************************************************************************/
/*                   kdms_renderer::menu_ViewPixelScaleX1                    */
/*****************************************************************************/

void kdms_renderer::menu_ViewPixelScaleX1()
{
  if (pixel_scale != 1)
    {
      pixel_scale = 1;
      size_window_for_image_dims(false);
    }
}

/*****************************************************************************/
/*                   kdms_renderer::menu_ViewPixelScaleX2                    */
/*****************************************************************************/

void kdms_renderer::menu_ViewPixelScaleX2()
{
  if (pixel_scale != 2)
    {
      pixel_scale = 2;
      calculate_rel_view_and_focus_params();
      rel_focus.get_centre_if_valid(view_centre_x,view_centre_y);
      size_window_for_image_dims(true);
      view_centre_known = false;
      rel_focus.reset();
    }
}

/*****************************************************************************/
/*                       kdms_renderer::menu_FocusOff                        */
/*****************************************************************************/

void kdms_renderer::menu_FocusOff()
{
  if (enable_focus_box)
    {
      kdu_dims empty_box;
      empty_box.size = kdu_coords(0,0);
      set_focus_box(empty_box);
    }  
}

/*****************************************************************************/
/*                  kdms_renderer::menu_FocusHighlighting                    */
/*****************************************************************************/

void kdms_renderer::menu_FocusHighlighting()
{
  highlight_focus = !highlight_focus;
  if (enable_focus_box && (compositor != NULL))
    compositor->invalidate_rect(view_dims);
}

/*****************************************************************************/
/*                      kdms_renderer::menu_FocusWiden                       */
/*****************************************************************************/

void kdms_renderer::menu_FocusWiden()
{
  if (shape_istream.exists() || !enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.x -= new_box.size.x >> 2;
  new_box.pos.y -= new_box.size.y >> 2;
  new_box.size.x += new_box.size.x >> 1;
  new_box.size.y += new_box.size.y >> 1;
  new_box &= view_dims;
  set_focus_box(new_box);  
}

/*****************************************************************************/
/*                     kdms_renderer::menu_FocusShrink                       */
/*****************************************************************************/

void kdms_renderer::menu_FocusShrink()
{
  if (shape_istream.exists())
    return;
  kdu_dims new_box = focus_box;
  if (!enable_focus_box)
    new_box = view_dims;
  new_box.pos.x += new_box.size.x >> 3;
  new_box.pos.y += new_box.size.y >> 3;
  new_box.size.x -= new_box.size.x >> 2;
  new_box.size.y -= new_box.size.y >> 2;
  set_focus_box(new_box);  
}

/*****************************************************************************/
/*                      kdms_renderer::menu_FocusLeft                        */
/*****************************************************************************/

void kdms_renderer::menu_FocusLeft()
{
  if (shape_istream.exists() || !enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.x -= 20;
  if (new_box.pos.x < view_dims.pos.x)
    {
      if (new_box.pos.x < image_dims.pos.x)
        new_box.pos.x = image_dims.pos.x;
      if (new_box.pos.x < view_dims.pos.x)
        scroll_absolute(new_box.pos.x-view_dims.pos.x,0,false);
    }
  set_focus_box(new_box);  
}

/*****************************************************************************/
/*                      kdms_renderer::menu_FocusRight                       */
/*****************************************************************************/

void kdms_renderer::menu_FocusRight()
{
  if (shape_istream.exists() || !enable_focus_box)
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
        scroll_absolute(new_lim-view_lim,0,false);
    }
  set_focus_box(new_box);  
}

/*****************************************************************************/
/*                       kdms_renderer::menu_FocusUp                         */
/*****************************************************************************/

void kdms_renderer::menu_FocusUp()
{
  if (shape_istream.exists() || !enable_focus_box)
    return;
  kdu_dims new_box = focus_box;
  new_box.pos.y -= 20;
  if (new_box.pos.y < view_dims.pos.y)
    {
      if (new_box.pos.y < image_dims.pos.y)
        new_box.pos.y = image_dims.pos.y;
      if (new_box.pos.y < view_dims.pos.y)
        scroll_absolute(0,new_box.pos.y-view_dims.pos.y,false);
    }
  set_focus_box(new_box);  
}

/*****************************************************************************/
/*                      kdms_renderer::menu_FocusDown                        */
/*****************************************************************************/

void kdms_renderer::menu_FocusDown()
{
  if (shape_istream.exists() || !enable_focus_box)
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
        scroll_absolute(0,new_lim-view_lim,false);
    }
  set_focus_box(new_box);  
}

/*****************************************************************************/
/*                       kdms_renderer::menu_ModeFast                        */
/*****************************************************************************/

void kdms_renderer::menu_ModeFast()
{
  error_level = 0;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                       kdms_renderer::menu_ModeFussy                       */
/*****************************************************************************/

void kdms_renderer::menu_ModeFussy()
{
  error_level = 1;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                    kdms_renderer::menu_ModeResilient                      */
/*****************************************************************************/

void kdms_renderer::menu_ModeResilient()
{
  error_level = 2;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                   kdms_renderer::menu_ModeResilientSOP                    */
/*****************************************************************************/

void kdms_renderer::menu_ModeResilientSOP()
{
  error_level = 3;
  if (compositor != NULL)
    compositor->set_error_level(error_level);
}

/*****************************************************************************/
/*                  kdms_renderer::menu_ModeSingleThreaded                   */
/*****************************************************************************/

void kdms_renderer::menu_ModeSingleThreaded()
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
/*                  kdms_renderer::menu_ModeMultiThreaded                    */
/*****************************************************************************/

void kdms_renderer::menu_ModeMultiThreaded()
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
/*                 kdms_renderer::can_do_ModeMultiThreaded                   */
/*****************************************************************************/

bool kdms_renderer::can_do_ModeMultiThreaded(NSMenuItem *item)
{
  const char *menu_string = "Multi-Threaded";
  char menu_string_buf[80];
  if (num_threads > 0)
    {
      sprintf(menu_string_buf,"Multi-Threaded (%d threads)",num_threads);
      menu_string = menu_string_buf;
    }
  NSString *ns_string = [NSString stringWithUTF8String:menu_string];
  [item setTitle:ns_string];
  [item setState:((num_threads > 0)?NSOnState:NSOffState)];
  return true;
}

/*****************************************************************************/
/*                   kdms_renderer::menu_ModeMoreThreads                     */
/*****************************************************************************/

void kdms_renderer::menu_ModeMoreThreads()
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
/*                   kdms_renderer::menu_ModeLessThreads                     */
/*****************************************************************************/

void kdms_renderer::menu_ModeLessThreads()
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
/*                kdms_renderer::menu_ModeRecommendedThreads                 */
/*****************************************************************************/

void kdms_renderer::menu_ModeRecommendedThreads()
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
/*              kdms_renderer::can_do_ModeRecommendedThreaded                */
/*****************************************************************************/

bool kdms_renderer::can_do_ModeRecommendedThreads(NSMenuItem *item)
{
  const char *menu_string = "Recommended: single threaded";
  char menu_string_buf[80];
  if (num_recommended_threads > 0)
    {
      sprintf(menu_string_buf,"Recommended: %d threads",
              num_recommended_threads);
      menu_string = menu_string_buf;
    }
  NSString *ns_string = [NSString stringWithUTF8String:menu_string];
  [item setTitle:ns_string];
  [item setState:((num_threads > 0)?NSOnState:NSOffState)];
  return true;
}

/*****************************************************************************/
/*                    kdms_renderer::menu_NavComponent1                      */
/*****************************************************************************/

void kdms_renderer::menu_NavComponent1() 
{
  if ((compositor == NULL) || (single_component_idx == 0))
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                  kdms_renderer::menu_NavComponentNext                     */
/*****************************************************************************/

void kdms_renderer::menu_NavComponentNext() 
{
  if ((compositor == NULL) ||
      ((max_components >= 0) && (single_component_idx >= (max_components-1))))
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                  kdms_renderer::menu_NavComponentPrev                     */
/*****************************************************************************/

void kdms_renderer::menu_NavComponentPrev() 
{
  if ((compositor == NULL) || (single_component_idx == 0))
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                  kdms_renderer::menu_NavMultiComponent                    */
/*****************************************************************************/

void kdms_renderer::menu_NavMultiComponent()
{
  if ((compositor == NULL) ||
      ((single_component_idx < 0) &&
       ((single_layer_idx < 0) || (num_frames == 0) || !jpx_in)))
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                    kdms_renderer::menu_NavImageNext                       */
/*****************************************************************************/

void kdms_renderer::menu_NavImageNext() 
{
  if (compositor == NULL)
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                   kdms_renderer::can_do_NavImageNext                      */
/*****************************************************************************/

bool kdms_renderer::can_do_NavImageNext(NSMenuItem *item)
{
  if (single_component_idx >= 0)
    return (compositor != NULL) &&
           ((max_codestreams < 0) ||
            (single_codestream_idx < (max_codestreams-1)));
  else if ((single_layer_idx >= 0) && !mj2_in)
    return (compositor != NULL) &&
           ((max_compositing_layer_idx < 0) ||
            (single_layer_idx < max_compositing_layer_idx));
  else if (single_layer_idx >= 0)
    return (compositor != NULL) &&
           ((num_frames < 0) || (frame_idx < (num_frames-1)));
  else
    return (compositor != NULL) && (frame != NULL) &&
            ((num_frames < 0) || (frame_idx < (num_frames-1)));
}

/*****************************************************************************/
/*                    kdms_renderer::menu_NavImagePrev                       */
/*****************************************************************************/

void kdms_renderer::menu_NavImagePrev() 
{
  if (compositor == NULL)
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                   kdms_renderer::can_do_NavImagePrev                      */
/*****************************************************************************/

bool kdms_renderer::can_do_NavImagePrev(NSMenuItem *item)
{
  if (single_component_idx >= 0)
    return (compositor != NULL) && (single_codestream_idx > 0);
  else if ((single_layer_idx >= 0) && !mj2_in)
    return (compositor != NULL) && (single_layer_idx > 0);
  else if (single_layer_idx >= 0)
    return (compositor != NULL) && (frame_idx > 0);
  else
    return (compositor != NULL) && (frame != NULL) && (frame_idx > 0);
}

/*****************************************************************************/
/*                    kdms_renderer::menu_NavTrackNext                       */
/*****************************************************************************/

void kdms_renderer::menu_NavTrackNext()
{
  if ((compositor == NULL) || !mj2_in)
    return;
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                 kds_renderer::menu_NavCompositingLayer                    */
/*****************************************************************************/

void kdms_renderer::menu_NavCompositingLayer() 
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
/*                   kdms_renderer::menu_MetadataCatalog                     */
/*****************************************************************************/

void kdms_renderer::menu_MetadataCatalog()
{
  if ((!catalog_source) && jpx_in.exists())
    [window create_metadata_catalog];
  else if (catalog_source)
    [window remove_metadata_catalog];
  if (catalog_source != nil)
    [window makeFirstResponder:catalog_source];
}

/*****************************************************************************/
/*                  kdms_renderer::menu_MetadataSwapFocus                    */
/*****************************************************************************/

void kdms_renderer::menu_MetadataSwapFocus()
{
  if ((!catalog_source) && jpx_in.exists())
    [window create_metadata_catalog];
  else if (catalog_source)
    {
      if ([doc_view is_first_responder])
        [window makeFirstResponder:catalog_source];
      else
        [window makeFirstResponder:doc_view];
    }
}

/*****************************************************************************/
/*                     kdms_renderer::menu_MetadataShow                      */
/*****************************************************************************/

void kdms_renderer::menu_MetadataShow()
{
  if (metashow != nil)
    [metashow makeKeyAndOrderFront:window];
  else
    {
      NSRect wnd_frame = [window frame];
      NSPoint preferred_location = wnd_frame.origin;
      preferred_location.y -= 10.0F;
      metashow = [kdms_metashow alloc];
      [metashow initWithOwner:this
                     location:preferred_location
                andIdentifier:window_identifier];
      if (jp2_family_in.exists())
        [metashow activateWithSource:&jp2_family_in andFileName:open_filename];
    }
}

/*****************************************************************************/
/*                    kdms_renderer::menu_MetadataAdd                        */
/*****************************************************************************/

void kdms_renderer::menu_MetadataAdd()
{
  if ((compositor == NULL) || (!jpx_in) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)))
    return;
  if ((editor != nil) && (shape_editor != NULL))
    { 
      [editor addROIRegion:nil];
      return;
    }
  if (file_server == NULL)
    file_server = new kdms_file_services(open_file_pathname);
  if (editor == nil)
    {
      NSRect wnd_frame = [window frame];
      NSPoint preferred_location = wnd_frame.origin;
      preferred_location.x += wnd_frame.size.width + 10.0F;
      preferred_location.y += wnd_frame.size.height;
      editor = [kdms_metadata_editor alloc];
      [editor initWithSource:(&jpx_in)
                fileServices:file_server
                    editable:YES
                    owner:this
                    location:preferred_location
               andIdentifier:window_identifier];
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
      [editor configureWithInitialRegion:initial_region
                                  stream:initial_codestream_idx
                                   layer:initial_layer_idx
                 appliesToRenderedResult:false];
    }
  else if (single_component_idx >= 0)
    [editor configureWithInitialRegion:kdu_dims()
                                stream:single_codestream_idx
                                 layer:-1
               appliesToRenderedResult:false];
  else if (single_layer_idx >= 0)
    [editor configureWithInitialRegion:kdu_dims()
                                stream:-1
                                 layer:single_layer_idx
               appliesToRenderedResult:false];
  else
    [editor configureWithInitialRegion:kdu_dims()
                                stream:-1
                                 layer:-1
               appliesToRenderedResult:true];
}

/*****************************************************************************/
/*                    kdms_renderer::menu_MetadataEdit                       */
/*****************************************************************************/

void kdms_renderer::menu_MetadataEdit()
{
  if ((compositor == NULL) || !jpx_in.exists())
    return;
  if ((catalog_source != nil) && ![doc_view is_first_responder])
    {
      jpx_metanode metanode = [catalog_source get_unique_selected_metanode];
      if (metanode.exists())
        edit_metanode(metanode,true);
      else
        NSBeep();
    }
  else
    edit_metadata_at_point(NULL);
}

/*****************************************************************************/
/*                    kdms_renderer::menu_MetadataDelete                     */
/*****************************************************************************/

void kdms_renderer::menu_MetadataDelete()
{
  if ((shape_editor != NULL) && (editor != nil))
    {
      [editor deleteROIRegion:nil];
      return;
    }
  if ((catalog_source == nil) || (editor != nil) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)) ||
      [doc_view is_first_responder])
    return;
  jpx_metanode metanode = [catalog_source get_unique_selected_metanode];
  if (!metanode)
    {
      NSBeep();
      return;
    }
  if (interrogate_user("Are you sure you want to delete this label/link from "
                       "the file's metadata?  This operation cannot be "
                       "undone.","Delete","Cancel") == 1)
    return;
  jpx_metanode parent = metanode.get_parent();
  if (parent.exists() && (parent.get_num_regions() > 0))
    {
      int num_siblings=0;
      parent.count_descendants(num_siblings);
      if ((num_siblings == 1) &&
          (interrogate_user("The node you are deleting belongs to a region "
                            "of interest (ROI) with no other descendants.  "
                            "Would you like to delete the region as well?",
                            "Leave ROI","Delete ROI") == 1))
        {
          metanode = parent;
          parent = jpx_metanode();
        }
    }
      
  metanode.delete_node();
  metadata_changed(false);
  if (parent.exists())
    [catalog_source select_matching_metanode:parent];
}

/*****************************************************************************/
/*                   kdms_renderer::can_do_MetadataDelete                    */
/*****************************************************************************/

bool kdms_renderer::can_do_MetadataDelete(NSMenuItem *item)
{
  if (shape_editor != NULL)
    return true;
  else
    return ((catalog_source != nil) && (editor == nil) &&
            ((jpip_client == NULL) || !jpip_client->is_alive(-1)) &&
            [catalog_source get_unique_selected_metanode].exists() &&
            ![doc_view is_first_responder]);
}

/*****************************************************************************/
/*               kdms_renderer::menu_MetadataFindOrphanedROI                 */
/*****************************************************************************/

void kdms_renderer::menu_MetadataFindOrphanedROI()
{
  if ((compositor == NULL) || (catalog_source == nil) || !jpx_in)
    return;
  jpx_meta_manager manager = jpx_in.access_meta_manager();
  jpx_metanode node;
  kdu_dims region; region.size.x = region.size.y = INT_MAX;
  while ((node=manager.enumerate_matches(node,-1,-1,false,region,0)).exists())
    {
      jpx_metanode linker;
      while ((linker = node.enum_linkers(linker)).exists())
        if (linker.get_state_ref())
          break;
      if ((!linker) && !look_for_catalog_descendants(node))
        break; // This node is an orphan
    }
  if (node.exists())
    show_imagery_for_metanode(node);
}

/*****************************************************************************/
/*                  kdms_renderer::menu_MetadataCopyLabel                    */
/*****************************************************************************/

void kdms_renderer::menu_MetadataCopyLabel()
{
  if ((catalog_source == nil) || [doc_view is_first_responder])
    return;
  jpx_metanode node = [catalog_source get_selected_metanode];
  const char *label = NULL;
  jpx_metanode_link_type link_type;
  if (node.exists() &&
      ((label = node.get_label()) == NULL) &&
      (node = node.get_link(link_type)).exists())
    label = node.get_label();
  if (label == NULL)
    NSBeep();
  else
    [catalog_source paste_label:label];
}

/*****************************************************************************/
/*                 kdms_renderer::can_do_MetadataCopyLabel                   */
/*****************************************************************************/

bool kdms_renderer::can_do_MetadataCopyLabel(NSMenuItem *item)
{
  return ((catalog_source != nil) && ![doc_view is_first_responder]);
}

/*****************************************************************************/
/*                  kdms_renderer::menu_MetadataCopyLink                     */
/*****************************************************************************/

void kdms_renderer::menu_MetadataCopyLink()
{
  if ((compositor == NULL) || !jpx_in)
    return;
  if ((catalog_source != nil) && ![doc_view is_first_responder])
    { // Link to something selected within the catalog
      jpx_metanode node = [catalog_source get_selected_metanode];
      if (node.exists())
        [catalog_source paste_link:node];
    }
  else
    { // Link to an image feature.
      NSRect ns_rect;
      ns_rect.origin = [window mouseLocationOutsideOfEventStream];
      ns_rect.origin = [doc_view convertPoint:ns_rect.origin fromView:nil];
      ns_rect.size.width = ns_rect.size.height = 0.0F;
      kdu_coords point = convert_region_from_doc_view(ns_rect).pos;
      kdu_istream_ref istr;
      jpx_metanode roi_node =
        compositor->search_overlays(point,istr,0.1F);
      if (roi_node.exists())
        [catalog_source paste_link:roi_node];
    }
}

/*****************************************************************************/
/*                     kdms_renderer::menu_MetadataCut                       */
/*****************************************************************************/

void kdms_renderer::menu_MetadataCut()
{
  if ((catalog_source == nil) || [doc_view is_first_responder])
    return;
  jpx_metanode node = [catalog_source get_unique_selected_metanode];
  if (node.exists())
    [catalog_source paste_node_to_cut:node];
}

/*****************************************************************************/
/*                   kdms_renderer::can_do_MetadataCut                       */
/*****************************************************************************/

bool kdms_renderer::can_do_MetadataCut(NSMenuItem *item)
{
  return ((catalog_source != nil) && ![doc_view is_first_responder]);
}

/*****************************************************************************/
/*                  kdms_renderer::menu_MetadataPasteNew                     */
/*****************************************************************************/

void kdms_renderer::menu_MetadataPasteNew()
{
  if ((compositor == NULL) || (catalog_source == nil) || (editor != nil) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)))
    return;
  jpx_metanode parent = [catalog_source get_unique_selected_metanode];
  if (!parent.exists())
    {
      NSBeep();
      return;
    }
  const char *label = [catalog_source get_pasted_label];
  jpx_metanode link = [catalog_source get_pasted_link];
  jpx_metanode node_to_cut = [catalog_source get_pasted_node_to_cut];
  
  jpx_metanode_link_type link_type = JPX_METANODE_LINK_NONE;
  jpx_metanode_link_type reverse_link_type = JPX_METANODE_LINK_NONE;
  if (link.exists())
    {
      int user_response =
        interrogate_user("What type of link would you like to add?\n"
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
                         "node within which the link is found.\n",
                         "Grouping","Alternate child","Alternate parent",
                         "Cancel");
      switch (user_response) {
        case 0: link_type = JPX_GROUPING_LINK; break;
        case 1: link_type = JPX_ALTERNATE_CHILD_LINK; break;
        case 2: link_type = JPX_ALTERNATE_PARENT_LINK; break;
        default: return;
      }
      if (link_type == JPX_ALTERNATE_CHILD_LINK)
        {
          user_response =
            interrogate_user("Would you like to create an \"Alternate "
                             "parent\" link within the link target to "
                             "provide bi-directional linking for this "
                             "\"Alternate child\" link?",
                             "One-way","Two-way","Cancel");
          if (user_response == 2)
            return;
          if (user_response == 1)
            reverse_link_type = JPX_ALTERNATE_PARENT_LINK;
        }
      else if (link_type == JPX_ALTERNATE_PARENT_LINK)
        {
          user_response =
            interrogate_user("Would you like to create an \"Alternate "
                             "child\" link within the link target to "
                             "provide bi-directional linking for this "
                             "\"Alternate parent\" link?",
                             "One-way","Two-way","Cancel");
          if (user_response == 2)
            return;
          if (user_response == 1)
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
        [catalog_source select_matching_metanode:new_node];
      }
  }
  catch (kdu_exception)
  {}
}

/*****************************************************************************/
/*                 kdms_renderer::can_do_MetadataPasteNew                    */
/*****************************************************************************/

bool kdms_renderer::can_do_MetadataPasteNew(NSMenuItem *item)
{
  return ((catalog_source != nil) && (editor == nil) &&
          ((jpip_client == NULL) || !jpip_client->is_alive(-1)) &&
          (([catalog_source get_pasted_label] != NULL) ||
           [catalog_source get_pasted_link].exists() ||
           [catalog_source get_pasted_node_to_cut].exists()));
}

/*****************************************************************************/
/*                  kdms_renderer::menu_MetadataDuplicate                    */
/*****************************************************************************/

void kdms_renderer::menu_MetadataDuplicate()
{
  if ((compositor == NULL) || (catalog_source == nil) || (editor != nil) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)))
    return;
  const char *src_label = NULL;
  jpx_metanode parent, src = [catalog_source get_unique_selected_metanode];
  if ((!(src.exists() && (parent=src.get_parent()).exists())) ||
      ((src_label=src.get_label()) == NULL))
    { 
      NSBeep();
      return;
    }
  char *new_label = new char[strlen(src_label)+15];
  jpx_metanode new_node, child;
  try {
    sprintf(new_label,"(copy) %s",src_label);
    new_node = parent.add_label(new_label);
    delete[] new_label;
    new_label = NULL;
    for (int d=0; (child=src.get_descendant(d)).exists(); d++)
      new_node.add_copy(child,true);
  }
  catch (kdu_exception) { 
    if (new_label != NULL)
      delete[] new_label;
  }
  if (new_node.exists())
    { 
      metadata_changed(true);
      [catalog_source select_matching_metanode:new_node];
    }
}

/*****************************************************************************/
/*                 kdms_renderer::can_do_MetadataDuplicate                   */
/*****************************************************************************/

bool kdms_renderer::can_do_MetadataDuplicate(NSMenuItem *item)
{
  if ((catalog_source == nil) || (editor != nil) ||
      ((jpip_client != NULL) && jpip_client->is_alive(-1)))
    return false;
  jpx_metanode node = [catalog_source get_unique_selected_metanode];
  return (node.exists() && (node.get_label() != NULL));
}

/*****************************************************************************/
/*                    kdms_renderer::menu_MetadataUndo                       */
/*****************************************************************************/

void kdms_renderer::menu_MetadataUndo()
{
  if (editor != nil)
    [editor undoROIEdit:nil];
}

/*****************************************************************************/
/*                    kdms_renderer::menu_MetadataRedo                       */
/*****************************************************************************/

void kdms_renderer::menu_MetadataRedo()
{
  if (editor != nil)
    [editor redoROIEdit:nil];
}

/*****************************************************************************/
/*                   kdms_renderer::menu_OverlayEnable                       */
/*****************************************************************************/

void kdms_renderer::menu_OverlayEnable() 
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

void kdms_renderer::menu_OverlayFlashing()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled) || in_playmode)
    return;
  overlay_flashing_state = (overlay_flashing_state)?0:1;
  next_overlay_flash_time = -1.0;
  configure_overlays();
}

/*****************************************************************************/
/*                   kdms_renderer::menu_OverlayToggle                       */
/*****************************************************************************/

void kdms_renderer::menu_OverlayToggle()
{
  if ((compositor == NULL) || (!jpx_in) || in_playmode)
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
/*                  kdms_renderer::menu_OverlayRestrict                      */
/*****************************************************************************/

void kdms_renderer::menu_OverlayRestrict()
{
  jpx_metanode new_restriction;
  if ((catalog_source != nil) && ![doc_view is_first_responder])
    {
      jpx_metanode metanode = [catalog_source get_selected_metanode];
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
/*                 kdms_renderer::can_do_OverlayRestrict                     */
/*****************************************************************************/

bool kdms_renderer::can_do_OverlayRestrict(NSMenuItem *item)
{
  [item setState:(overlay_restriction.exists())?NSOnState:NSOffState];
  return (overlay_restriction.exists() ||
          ((catalog_source != nil) && ![doc_view is_first_responder]));
}

/*****************************************************************************/
/*                  kdms_renderer::menu_OverlayLighter                       */
/*****************************************************************************/

void kdms_renderer::menu_OverlayLighter()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled))
    return;
  overlay_nominal_intensity *= 1.0F/1.5F;
  if (overlay_nominal_intensity < 0.25F)
    overlay_nominal_intensity = 0.25F;  
  configure_overlays();
}

/*****************************************************************************/
/*                   kdms_renderer::menu_OverlayHeavier                      */
/*****************************************************************************/

void kdms_renderer::menu_OverlayHeavier()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled))
    return;
  overlay_nominal_intensity *= 1.5F;
  if (overlay_nominal_intensity > 4.0F)
    overlay_nominal_intensity = 4.0F;
  configure_overlays();
}

/*****************************************************************************/
/*                 kdms_renderer::menu_OverlayDoubleSize                     */
/*****************************************************************************/

void kdms_renderer::menu_OverlayDoubleSize()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled) ||
      (overlay_size_threshold & 0x40000000))
    return;
  overlay_size_threshold <<= 1;
  configure_overlays();
}

/*****************************************************************************/
/*                  kdms_renderer::menu_OverlayHalveSize                     */
/*****************************************************************************/

void kdms_renderer::menu_OverlayHalveSize()
{
  if ((compositor == NULL) || (!jpx_in) || (!overlays_enabled) ||
      (overlay_size_threshold == 1))
    return;
  overlay_size_threshold >>= 1;
  configure_overlays();
}

/*****************************************************************************/
/*                     kdms_renderer::menu_PlayNative                        */
/*****************************************************************************/

void kdms_renderer::menu_PlayNative()
{
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                    kdms_renderer::can_do_PlayNative                       */
/*****************************************************************************/

bool kdms_renderer::can_do_PlayNative(NSMenuItem *item)
{
  if (!source_supports_playmode())
    return false;
  const char *menu_string = "Native play rate";
  char menu_string_buf[80];
  if ((native_playback_multiplier > 1.01F) ||
      (native_playback_multiplier < 0.99F))
    {
      sprintf(menu_string_buf,"Native play rate x %5.2g",
              1.0F/native_playback_multiplier);
      menu_string = menu_string_buf;
    }
  NSString *ns_string = [NSString stringWithUTF8String:menu_string];
  [item setTitle:ns_string];
  [item setState:((use_native_timing)?NSOnState:NSOffState)];
  return true;
}

/*****************************************************************************/
/*                    kdms_renderer::menu_PlayCustom                         */
/*****************************************************************************/

void kdms_renderer::menu_PlayCustom()
{
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                    kdms_renderer::can_do_PlayCustom                       */
/*****************************************************************************/

bool kdms_renderer::can_do_PlayCustom(NSMenuItem *item)
{
  if (!source_supports_playmode())
    return false;
  char menu_string_buf[80];
  sprintf(menu_string_buf,"Custom frame rate = %5.2g fps",
          1.0F/custom_frame_interval);
  NSString *ns_string = [NSString stringWithUTF8String:menu_string_buf];
  [item setTitle:ns_string];
  [item setState:((use_native_timing)?NSOffState:NSOnState)];
  return true;
}
  
/*****************************************************************************/
/*                 kdms_renderer::menu_PlayFrameRateUp                       */
/*****************************************************************************/

void kdms_renderer::menu_PlayFrameRateUp()
{
  if (shape_istream.exists())
    { NSBeep(); return; }
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
/*                kdms_renderer::menu_PlayFrameRateDown                      */
/*****************************************************************************/

void kdms_renderer::menu_PlayFrameRateDown()
{
  if (shape_istream.exists())
    { NSBeep(); return; }
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

/*****************************************************************************/
/*                   kdms_renderer::menu_StatusToggle                        */
/*****************************************************************************/

void kdms_renderer::menu_StatusToggle() 
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
