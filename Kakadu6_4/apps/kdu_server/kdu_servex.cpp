/*****************************************************************************/
// File: kdu_servex.cpp [scope = APPS/SERVER]
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
   Implements the `kdu_servex' object, which is used by the "kdu_server"
application to manage target files which are raw code-streams, JP2 files or
JPX files.
******************************************************************************/

#include "servex_local.h"
#include "kdu_utils.h"
#include <math.h>

#define KDSX_OUTBUF_LEN 64


/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                           write_big                                */
/*****************************************************************************/

static inline void
  write_big(kdu_uint32 val, kdu_byte * &bp)
{
  *(bp++) = (kdu_byte)(val>>24);
  *(bp++) = (kdu_byte)(val>>16);
  *(bp++) = (kdu_byte)(val>>8);
  *(bp++) = (kdu_byte) val;
}

static inline void
  write_big(kdu_int32 val, kdu_byte * &bp)
{
  write_big((kdu_uint32) val,bp);
}

/*****************************************************************************/
/* INLINE                           write_big                                */
/*****************************************************************************/

static inline void
  read_big(kdu_uint32 &val, kdu_byte * &bp)
{
  val = *(bp++);
  val = (val<<8) + *(bp++);
  val = (val<<8) + *(bp++);
  val = (val<<8) + *(bp++);
}

static inline void
  read_big(kdu_int32 &val, kdu_byte * &bp)
{
  val = *(bp++);
  val = (val<<8) + *(bp++);
  val = (val<<8) + *(bp++);
  val = (val<<8) + *(bp++);
}

/*****************************************************************************/
/* STATIC                     find_subbox_types                              */
/*****************************************************************************/

static void
  find_subbox_types(jp2_input_box *super, kdu_uint32 * &types,
                    int &num_types, int &max_types)
  /* Recursively scans through all sub-boxes of the supplied box, augmenting
     the `types' array, whose length is `num_types', so as to include all
     scanned box types exactly once (no particular order).  May need to
     allocate the `types' array if it is NULL on entry. */
{
  jp2_input_box sub;
  assert(super->exists() && jp2_is_superbox(super->get_box_type()));
  while (sub.open(super))
    {
      int n;
      kdu_uint32 new_type = sub.get_box_type();
      for (n=0; n < num_types; n++)
        if (types[n] == new_type)
          break;
      if (n == num_types)
        {
          if (num_types == max_types)
            {
              max_types += 16;
              kdu_uint32 *existing = types;
              types = new kdu_uint32[max_types];
              for (n=0; n < num_types; n++)
                types[n] = existing[n];
              if (existing != NULL)
                delete[] existing;
            }
          types[num_types++] = new_type;
          assert(num_types <= max_types);
        }
      if (jp2_is_superbox(new_type))
        find_subbox_types(&sub,types,num_types,max_types);
      sub.close();
    }
}


/* ========================================================================= */
/*                             kdsx_image_entities                           */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdsx_image_entities::add_entity                     */
/*****************************************************************************/

void
  kdsx_image_entities::add_entity(kdu_int32 idx)
{
  if (idx & universal_flags)
    return;
  int n, k;
  bool found_match = false;
  for (n=0; n < num_entities; n++)
    if (entities[n] >= idx)
      {
        found_match = (entities[n] == idx);
        break;
      }
  if (found_match)
    return;
  if (max_entities == num_entities)
    {
      int new_max_entities = max_entities + num_entities + 1;
      kdu_int32 *tmp = new kdu_int32[new_max_entities];
      for (k=0; k < num_entities; k++)
        tmp[k] = entities[k];
      if (entities != NULL)
        delete[] entities;
      entities = tmp;
      max_entities = new_max_entities;
    }
  for (k=num_entities; k > n; k--)
    entities[k] = entities[k-1];
  entities[n] = idx;
  num_entities++;
}

/*****************************************************************************/
/*                     kdsx_image_entities::add_universal                    */
/*****************************************************************************/

void
  kdsx_image_entities::add_universal(kdu_int32 flags)
{
  flags &= 0xFF000000;
  if ((universal_flags | flags) == universal_flags)
    return;
  universal_flags |= flags;
  int n;
  kdu_int32 *sp, *dp;
  for (sp=dp=entities, n=num_entities; n > 0; n--, sp++)
    if (!(*sp & universal_flags))
      *(dp++) = *sp;
  num_entities = (int)(dp-entities);
}

/*****************************************************************************/
/*                      kdsx_image_entities::find_match                      */
/*****************************************************************************/

kdsx_image_entities *
  kdsx_image_entities::find_match(kdsx_image_entities *head,
                                  kdsx_image_entities * &goes_after_this)
{
  int n;
  kdu_int32 idx, ref_idx = (num_entities==0)?0:entities[0];
  kdsx_image_entities *scan, *prev;
  goes_after_this = NULL;
  for (prev=NULL, scan=head; scan != NULL; prev=scan, scan=scan->next)
    {
      idx = (scan->num_entities==0)?0:scan->entities[0];
      if (idx > ref_idx)
        break; // Won't find any more matches in sorted list
      if ((idx == ref_idx) && (num_entities == scan->num_entities) &&
          (universal_flags == scan->universal_flags))
        { // Otherwise no match
          for (n=0; n < num_entities; n++)
            if (entities[n] != scan->entities[n])
              break;
          if (n == num_entities)
            return scan;
        }
    }
  goes_after_this = prev;
  return NULL;
}

/*****************************************************************************/
/*                      kdsx_image_entities::serialize                       */
/*****************************************************************************/

void
  kdsx_image_entities::serialize(FILE *fp)
{
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  write_big(universal_flags,bp);
  write_big(num_entities,bp);
  for (int n=0; n < num_entities; n++)
    {
      write_big(entities[n],bp);
      if ((bp-outbuf) > (KDSX_OUTBUF_LEN-4))
        { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
    }
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp);
}

/*****************************************************************************/
/*                     kdsx_image_entities::deserialize                      */
/*****************************************************************************/

void
  kdsx_image_entities::deserialize(FILE *fp)
{
  kdu_byte *bp, in_buf[KDSX_OUTBUF_LEN];
  if (fread(bp=in_buf,1,8,fp) != 8)
    { kdu_error e;
      e << "Unable to deserialize metadata structure from the cache."; }
  read_big(universal_flags,bp);
  read_big(num_entities,bp);
  max_entities = num_entities;
  entities = new int[num_entities];
  for (int n=0; n < num_entities; n++)
    {
      if (fread(bp=in_buf,1,4,fp) != 4)
        { kdu_error e;
          e << "Unable to deserialize metadata structure from the cache."; }
      read_big(entities[n],bp);
    }
}


/* ========================================================================= */
/*                               kdsx_metagroup                              */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdsx_metagroup::kdsx_metagroup                      */
/*****************************************************************************/

kdsx_metagroup::kdsx_metagroup(kdu_servex *owner)
{
  this->owner = owner;
  is_placeholder = false;
  is_last_in_bin = false;
  is_rubber_length = false;
  num_box_types = 0;
  box_types = NULL;
  length = 0;
  last_box_header_prefix = 0;
  last_box_type = 0;
  link_fpos = -1;
  next = NULL;
  phld = NULL;
  phld_bin_id = 0;
  scope = &scope_data;
  parent = NULL;
  fpos = 0;
  data = NULL;
}

/*****************************************************************************/
/*                      kdsx_metagroup::~kdsx_metagroup                      */
/*****************************************************************************/

kdsx_metagroup::~kdsx_metagroup()
{
  if (data != NULL)
    delete[] data;
  if (box_types != &(last_box_type))
    delete[] box_types;

  kds_metagroup *sub;
  while ((sub=phld) != NULL)
    {
      phld = sub->next;
      delete sub;
    }
}

/*****************************************************************************/
/*                    kdsx_metagroup::inherit_child_scope                    */
/*****************************************************************************/

void
  kdsx_metagroup::inherit_child_scope(kdsx_image_entities *entities,
                                      kds_metagroup *src)
{
  if (src->scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA)
    {
      if (!(scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA))
        {
          scope->region = src->scope->region;
          scope->max_discard_levels = src->scope->max_discard_levels;
        }
      else
        {
          kdu_coords min = scope->region.pos;
          kdu_coords lim = min + scope->region.size;
          kdu_coords src_min = src->scope->region.pos;
          kdu_coords src_lim = src_min + src->scope->region.size;
          min.x = (min.x < src_min.x)?min.x:src_min.x;
          min.y = (min.y < src_min.y)?min.y:src_min.y;
          lim.x = (lim.x > src_lim.x)?lim.x:src_lim.x;
          lim.y = (lim.y > src_lim.y)?lim.y:src_lim.y;
          scope->region.pos = min;
          scope->region.size = lim - min;
          if (scope->max_discard_levels > src->scope->max_discard_levels)
            scope->max_discard_levels = src->scope->max_discard_levels;
        }
    }
  entities->copy_from((kdsx_image_entities *) src->scope->entities);
  scope->flags |= src->scope->flags &
    ~(KDS_METASCOPE_LEAF | KDS_METASCOPE_INCLUDE_NEXT_SIBLING |
      KDS_METASCOPE_IS_REGION_SPECIFIC_DATA |
      KDS_METASCOPE_IS_CONTEXT_SPECIFIC_DATA);
  if (scope->sequence > src->scope->sequence)
    scope->sequence = src->scope->sequence;
}

/*****************************************************************************/
/*                          kdsx_metagroup::create                           */
/*****************************************************************************/

void
  kdsx_metagroup::create(kdsx_metagroup *parent,
                         kdsx_image_entities *parent_entities,
                         jp2_input_box *box, int max_size,
                         kdu_long &last_used_bin_id,
                         int &num_codestreams, int &num_jpch, int &num_jplh,
                         kdu_long fpos_lim, bool is_first_subbox_of_asoc)
{
  this->parent = parent;
  assert(box->exists());
  kdu_long contents_length = box->get_remaining_bytes();
  kdu_long total_length = box->get_box_bytes();
  kdu_long header_length = total_length - contents_length;
  is_rubber_length = false;
  if (contents_length < 0)
    { // Rubber length
      is_rubber_length = true;
      header_length = total_length; // Num bytes read so far
    }
  last_box_type = box->get_box_type();
  bool create_stream_equivalent = (parent == NULL) &&
    ((last_box_type == jp2_codestream_4cc) ||
     (last_box_type == jp2_fragment_table_4cc));
  is_placeholder = create_stream_equivalent;
  if (!(is_placeholder || is_first_subbox_of_asoc))
    is_placeholder =
      ((total_length > (kdu_long) max_size) ||
       ((contents_length > 0) &&
        ((last_box_type == jp2_association_4cc) ||
         (last_box_type == jp2_cross_reference_4cc) ||
         (last_box_type == jp2_header_4cc) ||
         (last_box_type == jp2_codestream_header_4cc) ||
         (last_box_type == jp2_compositing_layer_hdr_4cc) ||
         (last_box_type == jp2_colour_group_4cc) ||
         (last_box_type == jp2_dtbl_4cc))));
  fpos = box->get_locator().get_file_pos();

  // Create the scope information, starting with the scope of the parent
  kdsx_image_entities *entities = owner->get_temp_entities();
  if (parent != NULL)
    {
      scope->flags = (parent->scope->flags &
                      ~KDS_METASCOPE_INCLUDE_NEXT_SIBLING);
      // For the moment, we will inherit all reasonable scope flags from our
      // parent.  However, the KDCS_METASCOPE_HAS_IMAGE_WIDE_DATA and
      // KDCS_METASCOPE_HAS_GLOBAL_DATA flags may well prove to be
      // inappropriate.  We will specifically look into cancelling these if
      // the current box or its first sub-box (if a super-box) contains
      // region- or image-specific data.

      scope->max_discard_levels = parent->scope->max_discard_levels;
      scope->region = parent->scope->region;
      scope->sequence = parent->scope->sequence;
      entities->copy_from(parent_entities);
      if ((parent->last_box_type == jp2_compositing_layer_hdr_4cc) ||
          (parent->last_box_type == jp2_codestream_header_4cc) ||
          (parent->last_box_type == jp2_header_4cc) ||
          (parent->last_box_type == jp2_fragment_table_4cc) ||
          (parent->last_box_type == jp2_colour_group_4cc))
        { // All box headers inside the above super-boxes are required.
          scope->flags |= KDS_METASCOPE_INCLUDE_NEXT_SIBLING;
          if (!((last_box_type == jp2_image_header_4cc) ||
                (last_box_type == jp2_bits_per_component_4cc) ||
                (last_box_type == jp2_palette_4cc) ||
                (last_box_type == jp2_colour_4cc) ||
                (last_box_type == jp2_component_mapping_4cc) ||
                (last_box_type == jp2_channel_definition_4cc) ||
                (last_box_type == jp2_resolution_4cc) ||
                (last_box_type == jp2_registration_4cc) ||
                (last_box_type == jp2_opacity_4cc) ||
                (last_box_type == jp2_colour_group_4cc)))
            { // Other than the above box types, the contents of sub-boxes of
              // the above super-boxes may be left unexpanded for the purpose
              // of image rendering.
              scope->flags &=
                ~(KDS_METASCOPE_IMAGE_MANDATORY | KDS_METASCOPE_MANDATORY);
              scope->sequence = KDS_METASCOPE_MAX_SEQUENCE;
            }
        }
    }
  else
    {
      scope->max_discard_levels = 0;
      scope->sequence = KDS_METASCOPE_MAX_SEQUENCE; // Until proven more useful
      scope->region.pos = scope->region.size = kdu_coords(0,0);
      scope->flags = KDS_METASCOPE_INCLUDE_NEXT_SIBLING;
             // Includes all of data-bin 0
      scope->flags |= KDS_METASCOPE_HAS_GLOBAL_DATA; // May cancel this later
    }

  if ((last_box_type == jp2_signature_4cc) ||
      (last_box_type == jp2_file_type_4cc) ||
      (last_box_type == jp2_reader_requirements_4cc) ||
      (last_box_type == jp2_composition_4cc))
    {
      scope->flags |= KDS_METASCOPE_MANDATORY
                   |  KDS_METASCOPE_IMAGE_MANDATORY
                   |  KDS_METASCOPE_HAS_GLOBAL_DATA;
      entities->add_universal(0x01000000 | 0x02000000);
      scope->sequence = 0;
    }
  else if (last_box_type == jp2_compositing_layer_hdr_4cc)
    {
      scope->flags |= KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA
                   |  KDS_METASCOPE_HAS_IMAGE_WIDE_DATA
                   |  KDS_METASCOPE_IMAGE_MANDATORY;
      entities->add_entity(0x02000000 | (kdu_int32) num_jplh);
      scope->sequence = 0;
    }
  else if (last_box_type == jp2_codestream_header_4cc)
    {
      scope->flags |= KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA
                   |  KDS_METASCOPE_HAS_IMAGE_WIDE_DATA
                   |  KDS_METASCOPE_IMAGE_MANDATORY;
      entities->add_entity(0x01000000 | (kdu_int32) num_jpch);
      scope->sequence = 0;
    }
  else if ((last_box_type == jp2_codestream_4cc) ||
           (last_box_type == jp2_fragment_table_4cc))
    {
      scope->flags |= KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA
                   |  KDS_METASCOPE_HAS_IMAGE_WIDE_DATA
                   |  KDS_METASCOPE_IMAGE_MANDATORY;
      entities->add_entity(0x01000000 | (kdu_int32) num_codestreams);
      scope->sequence = 0;
    }
  else if (last_box_type == jp2_header_4cc)
    { // Set image-specific flags, without giving any specific image entities,
      // which makes the object apply to all image entities.
      scope->flags |= KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA
                   |  KDS_METASCOPE_HAS_IMAGE_WIDE_DATA
                   |  KDS_METASCOPE_IMAGE_MANDATORY;
      entities->add_universal(0x01000000 | 0x02000000);
      scope->sequence = 0;
    }
  else if (last_box_type == jp2_cross_reference_4cc)
    { // Need to figure out the `link_fpos' value
      kdu_uint32 cref_box_type;
      jp2_input_box flst;
      kdu_uint16 nf, url_idx;
      kdu_uint32 off1, off2, len;
      if (box->read(cref_box_type) && flst.open(box) &&
          (flst.get_box_type()==jp2_fragment_list_4cc) &&
          flst.read(nf) && (nf >= 1) && flst.read(off1) && flst.read(off2) &&
          flst.read(len) && flst.read(url_idx) && (url_idx == 0))
        {
          link_fpos = (kdu_long) off2;
#ifdef KDU_LONG64
          link_fpos += ((kdu_long) off1) << 32;
#endif // KDU_LONG64
        }
      flst.close();
    }
  else if (last_box_type == jp2_number_list_4cc)
    {
      // assert(!is_placeholder);
      kdu_uint32 idx;
      bool is_context_specific = false;
      while (box->read(idx))
        {
          if (idx == 0)
            entities->add_universal(0x01000000 | 0x02000000);
          else
            {
              entities->add_entity((kdu_int32) idx);
              if (idx & 0x02000000)
                is_context_specific = true;
            }
          scope->flags |= KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA;
          if (!(scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA))
            scope->flags |= KDS_METASCOPE_HAS_IMAGE_WIDE_DATA;
        }
      if (is_context_specific)
        {
          scope->flags |= KDS_METASCOPE_IS_CONTEXT_SPECIFIC_DATA;
          if (this == parent->phld)
            parent->scope->flags |= KDS_METASCOPE_IS_CONTEXT_SPECIFIC_DATA;
        }
    }
  else if (last_box_type == jp2_roi_description_4cc)
    { 
      kdu_dims rect;
      kdu_byte num=0; box->read(num);
      kdu_long roi_area = 0;
      for (int n=0; n < (int) num; n++)
        {
          kdu_byte Rstatic, Rtyp, Rcp;
          if (!(box->read(Rstatic) && box->read(Rtyp) &&
                box->read(Rcp) && box->read(rect.pos.x) &&
                box->read(rect.pos.y) && box->read(rect.size.x) &&
                box->read(rect.size.y) && (Rstatic < 2) && (Rtyp < 2)))
            break; // Syntax error, but don't bother holding up server for it
          if (Rtyp == 1)
            { // Elliptical region; `pos' indicates centre, rather than corner
              rect.pos.x -= rect.size.x;
              rect.pos.y -= rect.size.y;
              rect.size.x <<= 1;
              rect.size.y <<= 1;
            }
          else if (Rtyp != 0)
            continue; // Skip over elliptical and quadrilateral refinements
          roi_area += rect.area();
          if (!(scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA))
            {
              scope->flags |= KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA
                           |  KDS_METASCOPE_IS_REGION_SPECIFIC_DATA
                           |  KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA;
              scope->region = rect;
              if (this == parent->phld)
                parent->scope->flags |= KDS_METASCOPE_IS_REGION_SPECIFIC_DATA;
            }
          else
            {
              kdu_coords min = scope->region.pos;
              kdu_coords lim = min + scope->region.size;
              kdu_coords new_min = rect.pos;
              kdu_coords new_lim = new_min + rect.size;
              min.x = (min.x < new_min.x)?min.x:new_min.x;
              min.y = (min.y < new_min.y)?min.y:new_min.y;
              lim.x = (lim.x > new_lim.x)?lim.x:new_lim.x;
              lim.y = (lim.y > new_lim.y)?lim.y:new_lim.y;
              scope->region.pos = min;
              scope->region.size = lim - min;
            }
        }
      int max_size = scope->region.size.x;
      if (max_size < scope->region.size.y)
        max_size = scope->region.size.y;
      int discard_levels = 0;
      while (max_size > 4)
        { max_size >>= 1; discard_levels++; }
      if (discard_levels > scope->max_discard_levels)
        scope->max_discard_levels = discard_levels;
     
      // Finally set `scope->sequence' to 64 + log_2(roi_cost / roi_area)
      scope->sequence = 64;
      for (; (roi_area > 1) && (scope->sequence > 0); roi_area >>= 1)
        scope->sequence--;
      for (roi_area=total_length; roi_area > 1; roi_area>>=1)
        scope->sequence++;
      if (scope->sequence > KDS_METASCOPE_MAX_SEQUENCE)
        scope->sequence = KDS_METASCOPE_MAX_SEQUENCE;
    }
  else if ((last_box_type == jp2_image_header_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_header_4cc)
        owner->context_mappings->stream_defaults.parse_ihdr_box(box);
      else if (parent->last_box_type == jp2_codestream_header_4cc)
        owner->context_mappings->add_stream(num_jpch)->parse_ihdr_box(box);
    }
  else if ((last_box_type == jp2_component_mapping_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_header_4cc)
        owner->context_mappings->stream_defaults.parse_cmap_box(box);
      else if (parent->last_box_type == jp2_codestream_header_4cc)
        owner->context_mappings->add_stream(num_jpch)->parse_cmap_box(box);
    }
  else if ((last_box_type == jp2_channel_definition_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_header_4cc)
        owner->context_mappings->layer_defaults.parse_cdef_box(box);
      else if (parent->last_box_type == jp2_compositing_layer_hdr_4cc)
        owner->context_mappings->add_layer(num_jplh)->parse_cdef_box(box);
    }
  else if ((last_box_type == jp2_opacity_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_compositing_layer_hdr_4cc)
        owner->context_mappings->add_layer(num_jplh)->parse_opct_box(box);
    }
  else if ((last_box_type == jp2_colour_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_header_4cc)
        owner->context_mappings->layer_defaults.parse_colr_box(box);
      else if ((parent->last_box_type == jp2_colour_group_4cc) &&
               (parent->parent != NULL) &&
               (parent->parent->last_box_type==jp2_compositing_layer_hdr_4cc))
        owner->context_mappings->add_layer(num_jplh)->parse_colr_box(box);
    }
  else if ((last_box_type == jp2_registration_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_compositing_layer_hdr_4cc)
        owner->context_mappings->add_layer(num_jplh)->parse_creg_box(box);
    }
  else if ((last_box_type == jp2_comp_options_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_composition_4cc)
        owner->context_mappings->parse_copt_box(box);
    }
  else if ((last_box_type == jp2_comp_instruction_set_4cc) && (parent != NULL))
    {
      if (parent->last_box_type == jp2_composition_4cc)
        owner->context_mappings->parse_iset_box(box);
    }
  else if ((last_box_type == jp2_dtbl_4cc) && (parent == NULL))
    {
      owner->data_references.init(box);
      scope->flags = 0;
    }

  if ((contents_length == 0) ||
      !(jp2_is_superbox(last_box_type) && is_placeholder))
    scope->flags |= KDS_METASCOPE_LEAF;
  
  // Now make a preliminary decision as to whether the HAS_GLOBAL_DATA or
  // HAS_IMAGE_WIDE_DATA flags should be removed.
  if (scope->flags & KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA)
    {
      scope->flags &= ~KDS_METASCOPE_HAS_GLOBAL_DATA;
      if (scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA)
        scope->flags &= ~KDS_METASCOPE_HAS_IMAGE_WIDE_DATA;
    }

  // Create the group fields, including placeholder lists (recursively)
  if (is_placeholder)
    {
      num_box_types = 1;
      box_types = &last_box_type;
      if (!create_stream_equivalent)
        {
          phld_bin_id = ++last_used_bin_id;
          if (!jp2_is_superbox(last_box_type))
            { // Placeholder is for the contents only
              kdsx_metagroup *contents = new kdsx_metagroup(owner);
              phld = contents;
              contents->length = (int)(total_length-header_length);
              contents->fpos = fpos + header_length;
              contents->link_fpos = this->link_fpos; // Link position should
              this->link_fpos = 0; // be referenced from the metagroup
                  // which holds the cross-reference box's contents, not its
                  // header, since we are only obliged to serve up the header
                  // of the box which contains the linked content if the
                  // cross reference information is to be delivered.
              contents->is_last_in_bin = true;
              if (is_rubber_length)
                {
                  contents->is_rubber_length = true;
                  contents->length = KDU_INT32_MAX;
                }
              if ((contents->fpos+contents->length) > fpos_lim)
                contents->length = (int)(fpos_lim - contents->fpos);
            }
          else
            {
              assert(link_fpos < 0);
              kdsx_metagroup *tail=NULL, *elt;
              kdu_int32 super_flag_mask = 0xFFFFFFFF & // Mask will be used to
                ~(KDS_METASCOPE_HAS_GLOBAL_DATA |      // remove flags missing
                  KDS_METASCOPE_HAS_IMAGE_WIDE_DATA);  // from all children
                                         // after seeing non-initial sub-boxes.
              jp2_input_box sub;
              sub.open(box);
              bool first_subbox_of_asoc = (last_box_type==jp2_association_4cc);
              do {
                  elt = new kdsx_metagroup(owner);
                  if (tail == NULL)
                    phld = tail = elt;
                  else
                    { tail->next = elt; tail = elt; }
                  if (sub.exists())
                    {
                      elt->create(this,entities,&sub,max_size,
                                  last_used_bin_id,num_codestreams,
                                  num_jpch,num_jplh,fpos_lim,
                                  first_subbox_of_asoc);
                      first_subbox_of_asoc = false;
                      super_flag_mask |= elt->scope->flags;
                      if ((last_box_type == jp2_association_4cc) &&
                          (elt == phld))
                        { // First sub-box of an asoc box affects the scope
                          // of the asoc box and hence all of its sub-boxes
                          inherit_child_scope(entities,elt); // May add
                            // HAS_IMAGE_WIDE_DATA or HAS_REGION_SPECIFIC_DATA
                            // and may extend or create a region of interest
                            // and image entities for us.
                          scope->flags &= super_flag_mask; // May eliminate
                            // HAS_GLOBAL_DATA or HAS_IMAGE_WIDE_DATA from us
                            // and hence indirectly from all of our sub-boxes.
                        }
                    }
                  sub.close();
                } while (sub.open(box));
              tail->is_last_in_bin = true;
              if (phld != NULL)
                scope->flags &= super_flag_mask;
                   // Cancels HAS_GLOBAL_DATA if no child turns out to have
                   // global data.  Cancels HAS_IMAGE_WIDE_DATA if no child
                   // turns out to have image-wide data

              // Scan through descendants again, updating our own scope
              kds_metagroup *escan;
              for (escan=phld; escan != NULL; escan=escan->next)
                inherit_child_scope(entities,escan);
            }
        }
      if (is_rubber_length)
        total_length = 0; // so that placeholder is written correctly below
      is_rubber_length = false; // Placeholder itself is not rubber.
      length = synthesize_placeholder(header_length,total_length,
                                      num_codestreams);
      last_box_header_prefix = length; // Need the whole thing
    }
  else
    {
      if (is_rubber_length)
        length = KDU_INT32_MAX;
      else
        length = (int) total_length;
      if ((fpos+length) > fpos_lim)
        length = (int)(fpos_lim - fpos);
      last_box_header_prefix = (int) header_length;
      if (last_box_header_prefix > length)
        last_box_header_prefix = length;
      if (!jp2_is_superbox(last_box_type))
        {
          num_box_types = 1;
          box_types = &last_box_type;
        }
      else
        {
          int max_box_types = 1;
          num_box_types = 1;
          box_types = new kdu_uint32[1];
          box_types[0] = last_box_type;
          find_subbox_types(box,box_types,num_box_types,max_box_types);
        }
    }

  // Finalize image entities in the scope at this point
  scope->entities = owner->commit_image_entities(entities);

  // Check to see if we need to create a code-stream.
  if (create_stream_equivalent)
    {
      assert(phld == NULL);
      kdsx_stream *str = owner->add_stream(num_codestreams);
      assert(str->stream_id == num_codestreams);
      if (last_box_type == jp2_codestream_4cc)
        {
          str->start_pos = fpos + header_length;
          if (((str->length = contents_length) < 0) || // Had rubber length
              (str->length > (fpos_lim-str->start_pos)))
            str->length = fpos_lim - str->start_pos;
          str->url_idx = 0;
        }
      else
        {
          assert(last_box_type == jp2_fragment_table_4cc);
          jp2_input_box sub;
          while (sub.open(box) && (sub.get_box_type()!=jp2_fragment_list_4cc))
            sub.close();
          str->start_pos = str->length = 0;
          kdu_uint16 nf, url_idx;
          kdu_uint32 off1, off2, len;
          if (sub.exists() && sub.read(nf) && (nf > 0))
            while (((nf--) > 0) && sub.read(off1) && sub.read(off2) &&
                   sub.read(len) && sub.read(url_idx))
              {
                kdu_long offset = (kdu_long) off2;
#ifdef KDU_LONG64
                offset += ((kdu_long) off1) << 32;
#endif // KDU_LONG64
                if (str->length == 0)
                  {
                    str->start_pos = offset; 
                    str->length = (kdu_long) len;
                    str->url_idx = (int) url_idx;
                  }
                else if ((offset == (str->start_pos+str->length)) &&
                         (str->url_idx == (int) url_idx))
                  str->length += len;
                else
                  break;
              }
          sub.close();
        }
    }

  // Update counters last of all
  if (parent == NULL)
    {
      if ((last_box_type == jp2_codestream_4cc) ||
          (last_box_type == jp2_fragment_table_4cc))
        num_codestreams++;
      else if (last_box_type == jp2_compositing_layer_hdr_4cc)
        num_jplh++;
      else if (last_box_type == jp2_codestream_header_4cc)
        num_jpch++;
    }
}

/*****************************************************************************/
/*                           kdsx_metagroup::serialize                       */
/*****************************************************************************/


void
  kdsx_metagroup::serialize(FILE *fp)
{
  bool have_placeholder_list = (phld != NULL);
  bool have_data = (data != NULL);
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  *(bp++) = (is_placeholder)?1:0;
  *(bp++) = (is_last_in_bin)?1:0;
  *(bp++) = (have_placeholder_list)?1:0;
  *(bp++) = (have_data)?1:0;
  *(bp++) = (is_rubber_length)?1:0;
  *(bp++) = 0; // Reserved
  *(bp++) = 0; // Reserved
  *(bp++) = 0; // Reserved
  write_big(num_box_types,bp);
  write_big(length,bp);
  write_big(last_box_header_prefix,bp);
  write_big(last_box_type,bp);
  write_big((kdu_int32) phld_bin_id,bp);
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf;
  
  if (num_box_types > 1)
    {
      bp = outbuf;
      for (int n=0; n < num_box_types; n++)
        {
          write_big(box_types[n],bp);
          if ((bp-outbuf) > (KDSX_OUTBUF_LEN-4))
            { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
        }
      if (bp > outbuf)
        { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
    }
  
  for (int long_vals=0; long_vals < 2; long_vals++)
    {
      kdu_long val = (long_vals==0)?fpos:link_fpos;
      for (int i=7; i >= 0; i--, val>>=8)
        outbuf[i] = (kdu_byte) val;
      fwrite(outbuf,1,8,fp);
    }
  if (have_data)
    fwrite(data,1,length,fp);

  bp = outbuf;
  write_big((kdu_int32) scope->flags,bp);
  write_big(scope->max_discard_levels,bp);
  write_big(scope->sequence,bp);
  if (scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA)
    {
      write_big(scope->region.pos.x,bp);
      write_big(scope->region.pos.y,bp);
      write_big(scope->region.size.x,bp);
      write_big(scope->region.size.y,bp);
    }
  if (scope->flags & KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA)
    {
      kdsx_image_entities *entities = (kdsx_image_entities *) scope->entities;
      assert(entities->ref_id >= 0);
      write_big(entities->ref_id,bp);
    }
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp);

  if (have_placeholder_list)
    for (kds_metagroup *grp=phld; grp != NULL; grp=grp->next)
      ((kdsx_metagroup *) grp)->serialize(fp);
}

/*****************************************************************************/
/*                          kdsx_metagroup::deserialize                      */
/*****************************************************************************/

bool
  kdsx_metagroup::deserialize(kdsx_metagroup *parent, FILE *fp)
{
  this->parent = parent;
  kdu_byte *bp, inbuf[KDSX_OUTBUF_LEN];
  kdu_uint32 big_val;
  if (fread(bp=inbuf,1,28,fp) != 28)
    { kdu_error e;
      e << "Unable to deserialize metadata structure from the cache."; }
  is_placeholder =             (*(bp++) != 0)?true:false;
  is_last_in_bin =             (*(bp++) != 0)?true:false;
  bool have_placeholder_list = (*(bp++) != 0)?true:false;
  bool have_data =             (*(bp++) != 0)?true:false;
  is_rubber_length =           (*(bp++) != 0)?true:false;
  bp += 3; // 3 Reserved bytes
  read_big(num_box_types,bp);
  read_big(length,bp);
  read_big(last_box_header_prefix,bp);
  read_big(last_box_type,bp);
  read_big(big_val,bp); phld_bin_id = big_val;
  assert((bp-inbuf) == 28);

  if ((num_box_types < 0) || (num_box_types > 1024))
    { kdu_error e; e << "Cache representation of meta-data structure appears "
      "to be corrupt.  Unlikely to have " << num_box_types << " different "
      "box types within a single meta-data group!"; }
  if (num_box_types == 0)
    box_types = NULL;
  else if (num_box_types == 1)
    box_types = &last_box_type;
  else
    {
      box_types = new kdu_uint32[num_box_types];
      for (int n=0; n < num_box_types; n++)
        {
          fread(bp=inbuf,1,sizeof(kdu_uint32),fp);
          read_big(box_types[n],bp);
        }
    }

  for (int long_vals=0; long_vals < 2; long_vals++)
    {
      fread(bp=inbuf,1,8,fp);
      kdu_long val = 0;
      for (int i=7; i >= 0; i--)
        val = (val << 8) + *(bp++);
      if (long_vals == 0)
        fpos = val;
      else
        link_fpos = val;
    }

  if (have_data)
    {
      if ((length < 0) || (length > (1<<16)))
        { kdu_error e; e << "Cache representation of meta-data structure "
          "appears to be corrupt.  Unlikely to have "<<length<<" bytes "
          "of meta-data specially synthesized as a streaming equivalent "
        "for an original box."; }
      data = new kdu_byte[length];
      fread(data,1,length,fp);
    }
    
  fread(bp=inbuf,1,12,fp);
  read_big(scope->flags,bp);
  read_big(scope->max_discard_levels,bp);
  read_big(scope->sequence,bp);
  if (scope->flags & KDS_METASCOPE_HAS_REGION_SPECIFIC_DATA)
    {
      fread(bp=inbuf,1,16,fp);
      read_big(scope->region.pos.x,bp);
      read_big(scope->region.pos.y,bp);
      read_big(scope->region.size.x,bp);
      read_big(scope->region.size.y,bp);
    }
  if (scope->flags & KDS_METASCOPE_HAS_IMAGE_SPECIFIC_DATA)
    {
      kdu_int32 ref_id=0;
      fread(bp=inbuf,1,4,fp);
      read_big(ref_id,bp);
      scope->entities = owner->get_parsed_image_entities(ref_id);
    }

  if (have_placeholder_list)
    {
      kdsx_metagroup *grp, *tail=NULL;
      do {
          grp = new kdsx_metagroup(owner);
          if (tail == NULL)
            phld = tail = grp;
          else
            {
              tail->next = grp;
              tail = grp;
            }
        } while (grp->deserialize(this,fp));
    }

  return !is_last_in_bin;
}

/*****************************************************************************/
/*                   kdsx_metagroup::synthesize_placeholder                  */
/*****************************************************************************/

int
  kdsx_metagroup::synthesize_placeholder(kdu_long original_header_length,
                                         kdu_long original_box_length,
                                         int stream_id)
{
  bool is_codestream =
    (last_box_type == jp2_codestream_4cc) ||
    (last_box_type == jp2_fragment_table_4cc);
  assert((original_header_length == 8) || (original_header_length == 16));
  bool is_extended = (original_header_length > 8);
  int data_len = 20 + ((is_extended)?16:8);
  kdu_uint32 phld_flags = 1;
  if (is_codestream)
    {
      data_len += 24;
      phld_flags = 4;
    }

  data = new kdu_byte[data_len];
  kdu_byte *bp = data;
  write_big(data_len,bp);
  write_big(jp2_placeholder_4cc,bp);
  write_big(phld_flags,bp);
  write_big(0,bp);
  write_big((kdu_uint32) phld_bin_id,bp);
  if (is_extended)
    {
      write_big(1,bp);
      write_big(last_box_type,bp);
      for (int i=32; i >= 0; i-= 32)
        write_big((kdu_uint32)(original_box_length>>i),bp);
    }
  else
    {
      write_big((kdu_uint32) original_box_length,bp);
      write_big(last_box_type,bp);
    }
  if (is_codestream)
    {
      write_big(0,bp);
      write_big(0,bp);
      write_big(0,bp);
      write_big(0,bp);
      write_big(0,bp);
      write_big(stream_id,bp);
    }
  assert((bp-data) == data_len);
  return data_len;
}


/* ========================================================================= */
/*                             kdsx_stream_suminfo                           */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdsx_stream_suminfo::equals                        */
/*****************************************************************************/

bool kdsx_stream_suminfo::equals(const kdsx_stream_suminfo *src)
{ 
  return ((image_dims == src->image_dims) &&
          (tile_partition == src->tile_partition) &&
          (tile_indices == src->tile_indices) &&
          (max_discard_levels == src->max_discard_levels) &&
          (max_quality_layers == src->max_quality_layers) &&
          (num_components == src->num_components) &&
          (num_output_components == src->num_output_components) &&
          (memcmp(component_subs,src->component_subs,
                  sizeof(kdu_coords)*(size_t)num_components) == 0) &&
          (memcmp(output_component_subs,src->output_component_subs,
                  sizeof(kdu_coords)*(size_t)num_output_components) == 0));
}

/*****************************************************************************/
/*                        kdsx_stream_suminfo::create                        */
/*****************************************************************************/

void kdsx_stream_suminfo::create(kdu_codestream cs)
{
  int c;
  cs.get_dims(-1,image_dims);
  cs.get_tile_partition(tile_partition);
  cs.get_valid_tiles(tile_indices);
  num_components = cs.get_num_components(false);
  num_output_components = cs.get_num_components(true);
  component_subs = new kdu_coords[num_components];
  output_component_subs = new kdu_coords[num_output_components];
  for (c=0; c < num_components; c++)
    cs.get_subsampling(c,component_subs[c]);
  for (c=0; c < num_output_components; c++)
    cs.get_subsampling(c,output_component_subs[c],true);
  kdu_coords idx;
  for (idx.y=0; idx.y < tile_indices.size.y; idx.y++)
    for (idx.x=0; idx.x < tile_indices.size.x; idx.x++)
      cs.create_tile(idx+tile_indices.pos);
  max_discard_levels = cs.get_min_dwt_levels();
  max_quality_layers = cs.get_max_tile_layers();
}

/*****************************************************************************/
/*                      kdsx_stream_suminfo::serialize                       */
/*****************************************************************************/

void kdsx_stream_suminfo::serialize(FILE *fp, kdu_servex *owner)
{ 
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  write_big(image_dims.pos.x,bp);
  write_big(image_dims.pos.y,bp);
  write_big(image_dims.size.x,bp);  
  write_big(image_dims.size.y,bp);
  
  write_big(tile_partition.pos.x,bp);
  write_big(tile_partition.pos.y,bp);
  write_big(tile_partition.size.x,bp);  
  write_big(tile_partition.size.y,bp);
  
  write_big(tile_indices.pos.x,bp);
  write_big(tile_indices.pos.y,bp);
  write_big(tile_indices.size.x,bp);  
  write_big(tile_indices.size.y,bp);
  
  write_big(max_discard_levels,bp);
  write_big(max_quality_layers,bp);
  write_big(num_components,bp);
  write_big(num_output_components,bp);
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp);

  int i;
  size_t subs_len = (size_t)(8*(num_components+num_output_components));
  kdu_byte *subs_buf = bp = owner->get_scratch_buf(subs_len);
  for (i=0; i < num_components; i++)
    { 
      write_big(component_subs[i].x,bp);
      write_big(component_subs[i].y,bp);
    }
  for (i=0; i < num_output_components; i++)
    { 
      write_big(output_component_subs[i].x,bp);
      write_big(output_component_subs[i].y,bp);
    }
  assert((bp-subs_buf) == subs_len);
  fwrite(subs_buf,1,subs_len,fp);
}

/*****************************************************************************/
/*                     kdsx_stream_suminfo::deserialize                      */
/*****************************************************************************/

void kdsx_stream_suminfo::deserialize(FILE *fp, kdu_servex *owner)
{ 
  kdu_byte *bp, inbuf[KDSX_OUTBUF_LEN];
  if (fread(bp=inbuf,1,64,fp) != 64)
    { kdu_error e; e << "Unable to deserialize code-stream summary info "
      "structure from cache."; }
  read_big(image_dims.pos.x,bp);
  read_big(image_dims.pos.y,bp);
  read_big(image_dims.size.x,bp);  
  read_big(image_dims.size.y,bp);
  
  read_big(tile_partition.pos.x,bp);
  read_big(tile_partition.pos.y,bp);
  read_big(tile_partition.size.x,bp);  
  read_big(tile_partition.size.y,bp);
  
  read_big(tile_indices.pos.x,bp);
  read_big(tile_indices.pos.y,bp);
  read_big(tile_indices.size.x,bp);  
  read_big(tile_indices.size.y,bp);
  
  read_big(max_discard_levels,bp);
  read_big(max_quality_layers,bp);
  read_big(num_components,bp);
  read_big(num_output_components,bp);
  assert((bp-inbuf) <= KDSX_OUTBUF_LEN);
  
  int i;
  size_t subs_len = (size_t)(8*(num_components+num_output_components));
  kdu_byte *subs_buf = bp = owner->get_scratch_buf(subs_len);
  if (fread(bp=subs_buf,1,subs_len,fp) != subs_len)
    { kdu_error e; e << "Unable to deserialize code-stream summary info "
      "structure from cache."; }
  assert((component_subs == NULL) && (output_component_subs == NULL));
  component_subs = new kdu_coords[num_components];
  output_component_subs = new kdu_coords[num_output_components];
  for (i=0; i < num_components; i++)
    { 
      read_big(component_subs[i].x,bp);
      read_big(component_subs[i].y,bp);
    }
  for (i=0; i < num_output_components; i++)
    { 
      read_big(output_component_subs[i].x,bp);
      read_big(output_component_subs[i].y,bp);
    }
  assert((bp-subs_buf) == subs_len);  
}
  

/* ========================================================================= */
/*                               kdsx_stream                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdsx_stream::serialize                           */
/*****************************************************************************/

void
  kdsx_stream::serialize(FILE *fp, kdu_servex *owner)
{
  int i;
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  write_big(stream_id,bp);
  for (i=32; i >= 0; i -= 32)
    write_big((kdu_uint32)(start_pos>>i),bp);
  for (i=32; i >= 0; i -= 32)
    write_big((kdu_uint32)(length>>i),bp);
  write_big(url_idx,bp);
  write_big(num_layer_stats,bp);
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp);
  size_t stats_len = (size_t)(num_layer_stats*12);
  kdu_byte *stats_buf = bp = owner->get_scratch_buf(stats_len);
  for (i=0; i < num_layer_stats; i++)
    { 
      write_big(layer_log_slopes[i],bp);
      write_big((kdu_uint32)(layer_lengths[i]>>32),bp);
      write_big((kdu_uint32) layer_lengths[i],bp);
    }
  assert((bp-stats_buf) == stats_len);
  fwrite(stats_buf,1,stats_len,fp);
}

/*****************************************************************************/
/*                           kdsx_stream::deserialize                        */
/*****************************************************************************/

void
  kdsx_stream::deserialize(FILE *fp, kdu_servex *owner)
{
  int i;
  kdu_byte *bp, inbuf[KDSX_OUTBUF_LEN];
  if (fread(bp=inbuf,1,28,fp) != 28)
    { kdu_error e;
      e << "Unable to deserialize code-stream structure from cache."; }
  read_big(stream_id,bp);
  for (start_pos=0, i=8; i > 0; i--)
    start_pos = (start_pos << 8) + *(bp++);
  for (length=0, i=8; i > 0; i--)
    length = (length << 8) + *(bp++);
  read_big(url_idx,bp);
  read_big(num_layer_stats,bp);
  size_t stats_len = (size_t)(num_layer_stats*12);
  kdu_byte *stats_buf = owner->get_scratch_buf(stats_len);
  if (fread(bp=stats_buf,1,stats_len,fp) != stats_len)
    { kdu_error e;
      e << "Unable to deserialize code-stream structure from cache."; }
  size_t num_ints = (size_t)(num_layer_stats*3);
  layer_stats_handle = new int[num_ints];
  memset(layer_stats_handle,0,sizeof(int)*num_ints);
  layer_lengths = (kdu_long *)(layer_stats_handle);
  layer_log_slopes = (int *)(layer_lengths+num_layer_stats);
  for (i=0; i < num_layer_stats; i++)
    { 
      kdu_uint32 len_high, len_low;
      read_big(layer_log_slopes[i],bp);
      read_big(len_high,bp);
      read_big(len_low,bp);
      layer_lengths[i] = (((kdu_long) len_high) << 32) + ((kdu_long) len_low);
    }
}

/*****************************************************************************/
/*                               kdsx_stream::open                           */
/*****************************************************************************/

void
  kdsx_stream::open(const char *parent_filename)
{
  assert((fp == NULL) && !codestream);
  const char *fname = expanded_filename;
  if (fname == NULL)
    {
      fname = target_filename;
      bool is_absolute =
        (*fname == '/') || (*fname == '\\') ||
        ((fname[0] != '\0') && (fname[1] == ':') &&
         ((fname[2] == '/') || (fname[2] == '\\')));
      if ((fname != parent_filename) && (!is_absolute))
        { // Need an expanded filename
          expanded_filename = new char[strlen(fname)+strlen(parent_filename)+2];
          strcpy(expanded_filename,parent_filename);
          char *cp = expanded_filename+strlen(expanded_filename);
          while ((cp>expanded_filename) && (cp[-1] != '/') && (cp[-1] != '\\'))
            cp--;
          strcpy(cp,fname);
          fname = expanded_filename;
        }
    }

  fp = fopen(fname,"rb");
  rel_pos = 0;
  if (fp == NULL)
    { kdu_error e; e << "Unable to open codestream, \"" << fname << "\"!"; }
  codestream.create(this);
  codestream.set_persistent();
}


/* ========================================================================= */
/*                             kdsx_stream_mapping                           */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdsx_stream_mapping::parse_ihdr_box                  */
/*****************************************************************************/

void
  kdsx_stream_mapping::parse_ihdr_box(jp2_input_box *ihdr)
{
  if (num_components > 0)
    return; // Already parsed one
  kdu_uint32 height, width;
  kdu_uint16 nc;
  if (ihdr->read(height) && ihdr->read(width) && ihdr->read(nc))
    {
      num_components = (int) nc;
      size.x = (int) width;
      size.y = (int) height;
    }
}

/*****************************************************************************/
/*                      kdsx_stream_mapping::parse_cmap_box                  */
/*****************************************************************************/

void
  kdsx_stream_mapping::parse_cmap_box(jp2_input_box *cmap)
{
  if (component_indices != NULL)
    return; // Already parsed one
  num_channels = ((int) cmap->get_remaining_bytes()) >> 2;
  component_indices = new int[num_channels];
  for (int n=0; n < num_channels; n++)
    {
      kdu_uint16 cmp;
      kdu_byte mtyp, pcol;
      if (!(cmap->read(cmp) && cmap->read(mtyp) && cmap->read(pcol)))
        component_indices[n] = 0; // Easy way to defer error condition
      else
        component_indices[n] = (int) cmp;
    }
}

/*****************************************************************************/
/*                      kdsx_stream_mapping::finish_parsing                  */
/*****************************************************************************/

void
  kdsx_stream_mapping::finish_parsing(kdsx_stream_mapping *defaults)
{
  if (num_components == 0)
    {
      num_components = defaults->num_components;
      size = defaults->size;
    }
  if (component_indices == NULL)
    {
      if (defaults->component_indices != NULL)
        { // Copy defaults
          num_channels = defaults->num_channels;
          component_indices = new int[num_channels];
          for (int n=0; n < num_channels; n++)
            component_indices[n] = defaults->component_indices[n];
        }
      else
        { // Components in 1-1 correspondence with channels
          num_channels = num_components;
          component_indices = new int[num_channels];
          for (int n=0; n < num_channels; n++)
            component_indices[n] = n;
        }
    }
}

/*****************************************************************************/
/*                        kdsx_stream_mapping::serialize                     */
/*****************************************************************************/

void
  kdsx_stream_mapping::serialize(FILE *fp)
{
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  write_big(size.x,bp);
  write_big(size.y,bp);
  write_big(num_components,bp);
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp);
}

/*****************************************************************************/
/*                       kdsx_stream_mapping::deserialize                    */
/*****************************************************************************/

void
  kdsx_stream_mapping::deserialize(FILE *fp)
{
  assert(component_indices == NULL);
  kdu_byte *bp, inbuf[KDSX_OUTBUF_LEN];
  if (fread(bp=inbuf,1,12,fp) != 12)
    { kdu_error e;
      e << "Unable to deserialize context mapping rules from the cache."; }
  read_big(size.x,bp);
  read_big(size.y,bp);
  read_big(num_components,bp);
}



/* ========================================================================= */
/*                             kdsx_layer_mapping                            */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdsx_layer_mapping::parse_cdef_box                   */
/*****************************************************************************/

void
  kdsx_layer_mapping::parse_cdef_box(jp2_input_box *cdef)
{
  if (channel_indices != NULL)
    return; // Already parsed one
  kdu_uint16 num_descriptions;
  if (!cdef->read(num_descriptions))
    return;
  num_channels = (int) num_descriptions;
  channel_indices = new int[num_channels];

  int n, k;
  for (n=0; n < num_channels; n++)
    {
      kdu_uint16 channel_idx, typ, assoc;
      if (!(cdef->read(channel_idx) && cdef->read(typ) && cdef->read(assoc)))
        channel_indices[n] = 0; // Easiest way to defer error condition
      else
        channel_indices[n] = (int) channel_idx;
    }

  // Sort the channel indices into increasing order
  bool done = false;
  while (!done)
    { // Use simple bubble sort
      done = true;
      for (n=1; n < num_channels; n++)
        if (channel_indices[n] < channel_indices[n-1])
          { // Swap
            done = false;
            k = channel_indices[n];
            channel_indices[n] = channel_indices[n-1];
            channel_indices[n-1] = k;
          }
    }

  // Remove redundant elements
  for (n=1; n < num_channels; n++)
    if (channel_indices[n] == channel_indices[n-1])
      { // Found redundant channel
        num_channels--;
        for (k=n; k < num_channels; k++)
          channel_indices[k] = channel_indices[k+1];
      }
}

/*****************************************************************************/
/*                      kdsx_layer_mapping::parse_creg_box                   */
/*****************************************************************************/

void
  kdsx_layer_mapping::parse_creg_box(jp2_input_box *creg)
{
  if (members != NULL)
    return; // Already parsed one
  kdu_uint16 xs, ys;
  if (!(creg->read(xs) && creg->read(ys) && (xs > 0) && (ys > 0)))
    { kdu_error e; e << "Malformed codestream registration box encountered "
      "in JPX compositing layer header box."; }
  reg_precision.x = (int) xs;
  reg_precision.y = (int) ys;
  num_members = ((int) creg->get_remaining_bytes()) / 6;
  members = new kdsx_layer_member[num_members];
  for (int n=0; n < num_members; n++)
    {
      kdsx_layer_member *mem = members + n;
      kdu_uint16 cdn;
      kdu_byte xr, yr, xo, yo;
      if (!(creg->read(cdn) && creg->read(xr) && creg->read(yr) &&
            creg->read(xo) && creg->read(yo) && (xr > 0) && (yr > 0)))
        { kdu_error e; e << "Malformed codestream registration box "
          "encountered in JPX compositing layer header box."; }
      mem->codestream_idx = (int) cdn;
      mem->reg_offset.x = (int) xo;
      mem->reg_offset.y = (int) yo;
      mem->reg_subsampling.x = (int) xr;
      mem->reg_subsampling.y = (int) yr;
    }
}

/*****************************************************************************/
/*                      kdsx_layer_mapping::parse_opct_box                   */
/*****************************************************************************/

void
  kdsx_layer_mapping::parse_opct_box(jp2_input_box *opct)
{
  have_opct_box = true;
  kdu_byte otyp;
  if (!opct->read(otyp))
    return;
  if (otyp < 2)
    have_opacity_channel = true;
}

/*****************************************************************************/
/*                      kdsx_layer_mapping::parse_colr_box                   */
/*****************************************************************************/

void
  kdsx_layer_mapping::parse_colr_box(jp2_input_box *colr)
{
  if (num_colours > 0)
    return; // Already have a value.
  assert(colr->get_box_type() == jp2_colour_4cc);
  kdu_byte meth, prec_val, approx;
  if (!(colr->read(meth) && colr->read(prec_val) && colr->read(approx) &&
        (approx <= 4) && (meth >= 1) && (meth <= 4)))
    return; // Malformed colour description box.
  kdu_uint32 enum_cs;
  if ((meth == 1) && colr->read(enum_cs))
    switch (enum_cs) {
      case 0:  num_colours=1; break; // JP2_bilevel1_SPACE
      case 1:  num_colours=3; break; // space=JP2_YCbCr1_SPACE
      case 3:  num_colours=3; break; // space=JP2_YCbCr2_SPACE
      case 4:  num_colours=3; break; // space=JP2_YCbCr3_SPACE
      case 9:  num_colours=3; break; // space=JP2_PhotoYCC_SPACE
      case 11: num_colours=3; break; // space=JP2_CMY_SPACE
      case 12: num_colours=4; break; // space=JP2_CMYK_SPACE
      case 13: num_colours=4; break; // space=JP2_YCCK_SPACE
      case 14: num_colours=3; break; // space=JP2_CIELab_SPACE
      case 15: num_colours=1; break; // space=JP2_bilevel2_SPACE
      case 16: num_colours=3; break; // space=JP2_sRGB_SPACE
      case 17: num_colours=1; break; // space=JP2_sLUM_SPACE
      case 18: num_colours=3; break; // space=JP2_sYCC_SPACE
      case 19: num_colours=3; break; // space=JP2_CIEJab_SPACE
      case 20: num_colours=3; break; // space=JP2_esRGB_SPACE
      case 21: num_colours=3; break; // space=JP2_ROMMRGB_SPACE
      case 22: num_colours=3; break; // space=JP2_YPbPr60_SPACE
      case 23: num_colours=3; break; // space=JP2_YPbPr50_SPACE
      case 24: num_colours=3; break; // space=JP2_esYCC_SPACE
      default: // Unrecognized colour space.
        return;
      }
}

/*****************************************************************************/
/*                    kdsx_layer_mapping::finish_parsing                     */
/*****************************************************************************/

void
  kdsx_layer_mapping::finish_parsing(kdsx_layer_mapping *defaults,
                                     kdsx_context_mappings *owner)
{
  if (members == NULL)
    { // One codestream whose index is equal to that of the compositing layer
      assert(layer_idx >= 0);
      num_members = 1;
      members = new kdsx_layer_member[1];
      members->codestream_idx = layer_idx;
      reg_precision = kdu_coords(1,1);
      members->reg_subsampling = reg_precision;
    }

  layer_size = kdu_coords(0,0); // Find layer size by intersecting its members
  int n, m, total_channels=0;
  for (m=0; m < num_members; m++)
    {
      kdsx_layer_member *mem = members + m;
      if (mem->codestream_idx >= owner->num_codestreams)
        { kdu_error e; e << "Invalid codestream registration box found in "
          "compositing layer header box: attempting to use a non "
          "existent codestream."; }
      kdsx_stream_mapping *stream = owner->stream_refs[mem->codestream_idx];
      kdu_coords stream_size = stream->size;
      stream_size.x *= mem->reg_subsampling.x;
      stream_size.y *= mem->reg_subsampling.y;
      stream_size += mem->reg_offset;
      if ((m == 0) || (stream_size.x < layer_size.x))
        layer_size.x = stream_size.x;
      if ((m == 0) || (stream_size.y < layer_size.y))
        layer_size.y = stream_size.y;
      total_channels += stream->num_channels;
    }
  layer_size.x = ceil_ratio(layer_size.x,reg_precision.x);
  layer_size.y = ceil_ratio(layer_size.y,reg_precision.y);

  if (num_colours == 0)
    num_colours = defaults->num_colours; // Inherit any colour box info
  if (num_colours <= 0)
    { // No information about number of colours could be deduced; let's have
      // a reasonably conservative guess
      num_colours = total_channels;
      if (num_colours > 3)
        num_colours = 3;
      if (num_colours < 2)
        num_colours = 1;
    }

  if (channel_indices == NULL)
    {
      if ((defaults->channel_indices != NULL) && !have_opct_box)
        { // Copy default cdef info
          num_channels = defaults->num_channels;
          channel_indices = new int[num_channels];
          for (n=0; n < num_channels; n++)
            channel_indices[n] = defaults->channel_indices[n];
        }
      else
        { // Must be using initial set of `num_colours' channels
          num_channels = num_colours + ((have_opacity_channel)?1:0);
          if (num_channels > total_channels)
            num_channels = total_channels;
          channel_indices = new int[num_channels];
          for (n=0; n < num_channels; n++)
            channel_indices[n] = n;
        }
    }

  // Determine the image components in each codestream member
  total_channels = 0;
  int chan = 0; // Indexes first unused entry in `channel_indices' array
  for (m=0; m < num_members; m++)
    {
      kdsx_layer_member *mem = members + m;
      kdsx_stream_mapping *stream = owner->stream_refs[mem->codestream_idx];
      assert((mem->num_components == 0) && (mem->component_indices == NULL));
      if (chan == num_channels)
        break; // No more channels, and hence no more member components
      mem->component_indices = new int[stream->num_channels]; // Max value
      for (n=0; n < stream->num_channels; n++, total_channels++)
        {
          if (chan == num_channels)
            break;
          if (channel_indices[chan] == total_channels)
            { // Add corresponding image component
              mem->component_indices[mem->num_components++] =
                stream->component_indices[n];
              chan++;
            }
          else
            assert(channel_indices[chan] > total_channels);
        }
    }
}

/*****************************************************************************/
/*                        kdsx_layer_mapping::serialize                      */
/*****************************************************************************/

void
  kdsx_layer_mapping::serialize(FILE *fp)
{
  int n, k;
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  write_big(layer_size.x,bp);
  write_big(layer_size.y,bp);
  write_big(reg_precision.x,bp);
  write_big(reg_precision.y,bp);
  write_big(num_members,bp);
  for (n=0; n < num_members; n++)
    {
      if ((bp-outbuf) > (KDSX_OUTBUF_LEN-24))
        { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
      kdsx_layer_member *mem = members + n;
      write_big(mem->codestream_idx,bp);
      write_big(mem->reg_subsampling.x,bp);
      write_big(mem->reg_subsampling.y,bp);
      write_big(mem->reg_offset.x,bp);
      write_big(mem->reg_offset.y,bp);
      write_big(mem->num_components,bp);
    }

  for (n=0; n < num_members; n++)
    {
      kdsx_layer_member *mem = members + n;
      for (k=0; k < mem->num_components; k++)
        {
          if ((bp-outbuf) > (KDSX_OUTBUF_LEN-4))
            { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
          write_big(mem->component_indices[k],bp);
        }
    }
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf;
}

/*****************************************************************************/
/*                       kdsx_layer_mapping::deserialize                     */
/*****************************************************************************/

void
  kdsx_layer_mapping::deserialize(FILE *fp, kdsx_context_mappings *owner)
{
  assert((channel_indices == NULL) && (members == NULL));
  int n, k;
  kdu_byte *bp, inbuf[KDSX_OUTBUF_LEN];
  if (fread(bp=inbuf,1,20,fp) != 20)
    { kdu_error e;
      e << "Unable to deserialize context mapping rules from the cache."; }
  read_big(layer_size.x,bp);
  read_big(layer_size.y,bp);
  read_big(reg_precision.x,bp);
  read_big(reg_precision.y,bp);
  read_big(num_members,bp);
  members = new kdsx_layer_member[num_members];
  for (n=0; n < num_members; n++)
    {
      if (fread(bp=inbuf,1,24,fp) != 24)
        { kdu_error e;
          e << "Unable to deserialize context mapping rules from the cache."; }
      kdsx_layer_member *mem = members + n;
      read_big(mem->codestream_idx,bp);
      read_big(mem->reg_subsampling.x,bp);
      read_big(mem->reg_subsampling.y,bp);
      read_big(mem->reg_offset.x,bp);
      read_big(mem->reg_offset.y,bp);
      read_big(mem->num_components,bp);
      assert((bp-inbuf) == 24);
      if ((mem->codestream_idx < 0) ||
          (mem->codestream_idx >= owner->num_codestreams))
        { kdu_error e;
          e << "Unable to deserialize context mapping rules from the cache."; }
    }

  for (n=0; n < num_members; n++)
    {
      kdsx_layer_member *mem = members + n;
      mem->component_indices = new int[mem->num_components];
      for (k=0; k < mem->num_components; k++)
        {
          if (fread(bp=inbuf,1,4,fp) != 4)
            { kdu_error e;
              e << "Unable to deserialize context mapping rules "
                   "from the cache."; }
          read_big(mem->component_indices[k],bp);
        }
    }
}


/* ========================================================================= */
/*                            kdsx_context_mappings                          */
/* ========================================================================= */

/*****************************************************************************/
/*                      kdsx_context_mappings::add_stream                    */
/*****************************************************************************/

kdsx_stream_mapping *
  kdsx_context_mappings::add_stream(int idx)
{
  if (idx >= num_codestreams)
    { // Need to allocate new stream
      if (idx >= max_codestreams)
        { // Need to reallocate references array
          max_codestreams += idx+8;
          kdsx_stream_mapping **refs =
            new kdsx_stream_mapping *[max_codestreams];
          for (int n=0; n < num_codestreams; n++)
            refs[n] = stream_refs[n];
          if (stream_refs != NULL)
            delete[] stream_refs;
          stream_refs = refs;
        }
      while (num_codestreams <= idx)
        stream_refs[num_codestreams++] = new kdsx_stream_mapping;
    }
  return stream_refs[idx];
}

/*****************************************************************************/
/*                      kdsx_context_mappings::add_layer                     */
/*****************************************************************************/

kdsx_layer_mapping *
  kdsx_context_mappings::add_layer(int idx)
{
  if (idx >= num_compositing_layers)
    { // Need to allocate new layer
      if (idx >= max_compositing_layers)
        { // Need to reallocate references array
          max_compositing_layers += idx+8;
          kdsx_layer_mapping **refs =
            new kdsx_layer_mapping *[max_compositing_layers];
          for (int n=0; n < num_compositing_layers; n++)
            refs[n] = layer_refs[n];
          if (layer_refs != NULL)
            delete[] layer_refs;
          layer_refs = refs;
        }
      while (num_compositing_layers <= idx)
        {
          layer_refs[num_compositing_layers] =
            new kdsx_layer_mapping(num_compositing_layers);
          num_compositing_layers++;
        }
    }
  return layer_refs[idx];
}

/*****************************************************************************/
/*                    kdsx_context_mappings::parse_copt_box                  */
/*****************************************************************************/

void
  kdsx_context_mappings::parse_copt_box(jp2_input_box *box)
{
  composited_size = kdu_coords(0,0);
  kdu_uint32 height, width;
  if (!(box->read(height) && box->read(width)))
    return; // Leaves `composited_size' 0, so rest of comp box is ignored
  composited_size.x = (int) width;
  composited_size.y = (int) height;
}

/*****************************************************************************/
/*                    kdsx_context_mappings::parse_iset_box                  */
/*****************************************************************************/

void
  kdsx_context_mappings::parse_iset_box(jp2_input_box *box)
{
  int n;
  if ((composited_size.x <= 0) || (composited_size.y <= 0))
    return; // Failed to parse a valid `copt' box previously

  kdu_uint16 flags, rept;
  kdu_uint32 tick;
  if (!(box->read(flags) && box->read(rept) && box->read(tick)))
    { kdu_error e;
      e << "Malformed Instruction Set box found in JPX data source."; }

  if (num_comp_sets == max_comp_sets)
    { // Augment the array
      max_comp_sets += num_comp_sets + 8;
      int *tmp_starts = new int[max_comp_sets];
      for (n=0; n < num_comp_sets; n++)
        tmp_starts[n] = comp_set_starts[n];
      if (comp_set_starts != NULL)
        delete[] comp_set_starts;
      comp_set_starts = tmp_starts;
    }
  comp_set_starts[num_comp_sets++] = num_comp_instructions;
  
  kdu_dims source_dims, target_dims;
  bool have_target_pos = ((flags & 1) != 0);
  bool have_target_size = ((flags & 2) != 0);
  bool have_life_persist = ((flags & 4) != 0);
  bool have_source_region = ((flags & 32) != 0);
  if (!(have_target_pos || have_target_size ||
        have_life_persist || have_source_region))
    return; // No new instructions in this set

  while (1)
    {
      if (have_target_pos)
        {
          kdu_uint32 X0, Y0;
          if (!(box->read(X0) && box->read(Y0)))
            return; // No more instructions
          target_dims.pos.x = (int) X0;
          target_dims.pos.y = (int) Y0;
        }
      else
        target_dims.pos = kdu_coords(0,0);

      if (have_target_size)
        {
          kdu_uint32 XS, YS;
          if (!(box->read(XS) && box->read(YS)))
            {
              if (!have_target_pos)
                return; // No more instructions
              kdu_error e; e <<
                "Malformed Instruction Set box found in JPX data source.";
            }
          target_dims.size.x = (int) XS;
          target_dims.size.y = (int) YS;
        }
      else
        target_dims.size = kdu_coords(0,0);

      if (have_life_persist)
        {
          kdu_uint32 life, next_reuse;
          if (!(box->read(life) && box->read(next_reuse)))
            {
              if (!(have_target_pos || have_target_size))
                return; // No more instructions
              kdu_error e; e <<
                "Malformed Instruction Set box found in JPX data source.";
            }
        }
      
      if (have_source_region)
        {
          kdu_uint32 XC, YC, WC, HC;
          if (!(box->read(XC) && box->read(YC) &&
                box->read(WC) && box->read(HC)))
            {
              if (!(have_life_persist || have_target_pos || have_target_size))
                return; // No more instructions
              kdu_error e; e <<
                "Malformed Instruction Set box found in JPX data source.";
            }
          source_dims.pos.x = (int) XC;
          source_dims.pos.y = (int) YC;
          source_dims.size.x = (int) WC;
          source_dims.size.y = (int) HC;
        }
      else
        source_dims.size = source_dims.pos = kdu_coords(0,0);

      if (num_comp_instructions == max_comp_instructions)
        { // Augment array
          max_comp_instructions += num_comp_instructions + 8;
          kdsx_comp_instruction *tmp_inst =
            new kdsx_comp_instruction[max_comp_instructions];
          for (n=0; n < num_comp_instructions; n++)
            tmp_inst[n] = comp_instructions[n];
          if (comp_instructions != NULL)
            delete[] comp_instructions;
          comp_instructions = tmp_inst;
        }
      comp_instructions[num_comp_instructions].source_dims = source_dims;
      comp_instructions[num_comp_instructions].target_dims = target_dims;
      num_comp_instructions++;
    }
}

/*****************************************************************************/
/*                    kdsx_context_mappings::finish_parsing                  */
/*****************************************************************************/

void
  kdsx_context_mappings::finish_parsing(int total_codestreams,
                                        int total_layer_headers)
{
  int n;
  while (num_codestreams < total_codestreams)
    add_stream(num_codestreams);
  for (n=0; n < num_codestreams; n++)
    stream_refs[n]->finish_parsing(&stream_defaults);
  if (total_layer_headers == 0)
    total_layer_headers = total_codestreams;
  while (num_compositing_layers < total_layer_headers)
    add_layer(num_compositing_layers);
  for (n=0; n < num_compositing_layers; n++)
    layer_refs[n]->finish_parsing(&layer_defaults,this);
}

/*****************************************************************************/
/*                      kdsx_context_mappings::serialize                     */
/*****************************************************************************/

void
  kdsx_context_mappings::serialize(FILE *fp)
{
  int n;
  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;
  write_big(num_codestreams,bp);
  write_big(num_compositing_layers,bp);
  write_big(composited_size.x,bp);
  write_big(composited_size.y,bp);
  write_big(num_comp_sets,bp);
  write_big(num_comp_instructions,bp);
  fwrite(outbuf,1,bp-outbuf,fp); bp = outbuf;

  for (n=0; n < num_codestreams; n++)
    stream_refs[n]->serialize(fp);
  for (n=0; n < num_compositing_layers; n++)
    layer_refs[n]->serialize(fp);

  for (n=0; n < num_comp_sets; n++)
    {
      if ((bp-outbuf) > (KDSX_OUTBUF_LEN-4))
        { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
      write_big(comp_set_starts[n],bp);
    }
  for (n=0; n < num_comp_instructions; n++)
    {
      if ((bp-outbuf) > (KDSX_OUTBUF_LEN-32))
        { fwrite(outbuf,1,bp-outbuf,fp); bp=outbuf; }
      write_big(comp_instructions[n].source_dims.pos.x,bp);
      write_big(comp_instructions[n].source_dims.pos.y,bp);
      write_big(comp_instructions[n].source_dims.size.x,bp);
      write_big(comp_instructions[n].source_dims.size.y,bp);
      write_big(comp_instructions[n].target_dims.pos.x,bp);
      write_big(comp_instructions[n].target_dims.pos.y,bp);
      write_big(comp_instructions[n].target_dims.size.x,bp);
      write_big(comp_instructions[n].target_dims.size.y,bp);
    }
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp);
}

/*****************************************************************************/
/*                     kdsx_context_mappings::deserialize                    */
/*****************************************************************************/

void
  kdsx_context_mappings::deserialize(FILE *fp)
{
  assert((stream_refs == NULL) && (layer_refs == NULL) &&
         (comp_set_starts == NULL) && (comp_instructions == NULL));
  kdu_int32 n, num;
  kdu_byte *bp, in_buf[KDSX_OUTBUF_LEN];
  if (fread(bp=in_buf,1,24,fp) != 24)
    { kdu_error e;
      e << "Unable to deserialize context mappings from the cache."; }
  read_big(num,bp);
  for (n=0; n < num; n++)
    add_stream(n);
  read_big(num,bp);
  for (n=0; n < num; n++)
    add_layer(n);
  read_big(composited_size.x,bp);
  read_big(composited_size.y,bp);
  read_big(num_comp_sets,bp);
  read_big(num_comp_instructions,bp);
  max_comp_sets = num_comp_sets;
  max_comp_instructions = num_comp_instructions;
  if (num_comp_sets > 0)
    comp_set_starts = new int[max_comp_sets];
  if (num_comp_instructions > 0)
    comp_instructions = new kdsx_comp_instruction[max_comp_instructions];

  for (n=0; n < num_codestreams; n++)
    stream_refs[n]->deserialize(fp);
  for (n=0; n < num_compositing_layers; n++)
    layer_refs[n]->deserialize(fp,this);

  for (n=0; n < num_comp_sets; n++)
    {
      if (fread(bp=in_buf,1,4,fp) != 4)
        { kdu_error e;
          e << "Unable to deserialize context mappings from the cache."; }
      read_big(comp_set_starts[n],bp);
    }
  for (n=0; n < num_comp_instructions; n++)
    {
      if (fread(bp=in_buf,1,32,fp) != 32)
        { kdu_error e;
          e << "Unable to deserialize context mappings from the cache."; }
      read_big(comp_instructions[n].source_dims.pos.x,bp);
      read_big(comp_instructions[n].source_dims.pos.y,bp);
      read_big(comp_instructions[n].source_dims.size.x,bp);
      read_big(comp_instructions[n].source_dims.size.y,bp);
      read_big(comp_instructions[n].target_dims.pos.x,bp);
      read_big(comp_instructions[n].target_dims.pos.y,bp);
      read_big(comp_instructions[n].target_dims.size.x,bp);
      read_big(comp_instructions[n].target_dims.size.y,bp);
    }
}


/* ========================================================================= */
/*                                 kdu_servex                                */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdu_servex::kdu_servex                          */
/*****************************************************************************/

kdu_servex::kdu_servex()
{
  mutex.create();
  free_events = NULL;
  target_filename = NULL;
  codestream_range[0] = 0; codestream_range[1] = -1;
  metatree = NULL;
  meta_fp = NULL;
  stream_head = stream_tail = NULL;
  stream_refs = NULL;
  max_stream_refs = 0;
  default_stream_suminfo = NULL;
  tmp_entities = NULL;
  free_tmp_entities = NULL;
  committed_entities_list = NULL;
  num_committed_entities = 0;
  committed_entity_refs = NULL;
  context_mappings = NULL;
  scratch_buf_len = 0;
  scratch_buf = NULL;
}

/*****************************************************************************/
/*                             kdu_servex::open                              */
/*****************************************************************************/

void
  kdu_servex::open(const char *filename, int phld_threshold,
                   int per_client_cache, FILE *cache_fp,
                   bool cache_exists, kdu_long sub_start, kdu_long sub_lim)
{
  close(); // Just in case.

  context_mappings = new kdsx_context_mappings;

  target_filename = new char[strlen(filename)+1];
  strcpy(target_filename,filename);
  if ((cache_fp == NULL) || !cache_exists)
    {
      create_structure(sub_start,sub_lim,phld_threshold);
      enable_codestream_access(per_client_cache);
      read_codestream_summary_info();
      if (cache_fp != NULL)
        save_structure(cache_fp);
    }
  else
    { 
      load_structure(cache_fp);
      enable_codestream_access(per_client_cache);
    }

  // Open the metadata file pointer if necessary
  if ((metatree != NULL) && (metatree->length > 0))
    {
      meta_fp = fopen(target_filename,"rb");
      if (meta_fp == NULL)
        { kdu_error e; e << "Unable to open target file."; }
    }
}

/*****************************************************************************/
/*                             kdu_servex::close                             */
/*****************************************************************************/

void
  kdu_servex::close()
{
  if (context_mappings != NULL)
    delete context_mappings;
  context_mappings = NULL;

  if (target_filename != NULL)
    delete[] target_filename;
  target_filename = NULL;

  codestream_range[0] = 0; codestream_range[1] = -1;

  kdsx_metagroup *gscan;
  while ((gscan=metatree) != NULL)
    {
      metatree = (kdsx_metagroup*) gscan->next;
      delete gscan; // Recursive deletion function
    }

  if (meta_fp != NULL)
    fclose(meta_fp);
  meta_fp = NULL;

  while ((stream_tail=stream_head) != NULL)
    {
      stream_head = stream_tail->next;
      assert(stream_tail->num_attachments <= 0);
      delete stream_tail;
    }
  if (stream_refs != NULL)
    {
      delete[] stream_refs;
      stream_refs = NULL;
    }
  max_stream_refs = 0;

  if (default_stream_suminfo != NULL)
    { 
      delete default_stream_suminfo;
      default_stream_suminfo = NULL;
    }
  
  kdsx_image_entities *escan;
  while ((escan=tmp_entities) != NULL)
    {
      tmp_entities = escan->next;
      delete escan;
    }
  while ((escan=free_tmp_entities) != NULL)
    {
      free_tmp_entities = escan->next;
      delete escan;
    }
  while ((escan=committed_entities_list) != NULL)
    {
      committed_entities_list = escan->next;
      delete escan;
    }
  if (committed_entity_refs != NULL)
    {
      delete[] committed_entity_refs;
      committed_entity_refs = NULL;
    }
  num_committed_entities = 0;
}

/*****************************************************************************/
/*                           kdu_servex::get_event                           */
/*****************************************************************************/

inline kdsx_event *kdu_servex::get_event()
{
  if (free_events == NULL)
    free_events = new kdsx_event;
  kdsx_event *ev = free_events;  free_events = ev->next;
  return ev;
}

/*****************************************************************************/
/*                         kdu_servex::release_event                         */
/*****************************************************************************/

inline void kdu_servex::release_event(kdsx_event *ev)
{
  ev->next = free_events;
  free_events = ev;
}

/*****************************************************************************/
/*                        kdu_servex::trim_resources                         */
/*****************************************************************************/

void kdu_servex::trim_resources()
{
  mutex.lock();
  kdsx_event *ev;
  while ((ev=free_events) != NULL)
    {
      free_events = ev->next;
      delete ev;
    }
  mutex.unlock();
}

/*****************************************************************************/
/*                   kdu_servex::get_codestream_siz_info                     */
/*****************************************************************************/

bool
  kdu_servex::get_codestream_siz_info(int stream_id, kdu_dims &image_dims,
                                      kdu_dims &tile_partition,
                                      kdu_dims &tile_indices,
                                      int &num_components,
                                      int &num_output_components,
                                      int &max_discard_levels,
                                      int &max_quality_layers,
                                      kdu_coords *component_subs,
                                      kdu_coords *output_component_subs)
{
  kdsx_stream *str;
  if ((stream_id < 0) || (stream_id > codestream_range[1]) ||
      ((str = stream_refs[stream_id]) == NULL))
    return false;
  int num_comps = num_components;
  int num_out_comps = num_output_components;
  image_dims = str->suminfo->image_dims;
  tile_partition = str->suminfo->tile_partition;
  tile_indices = str->suminfo->tile_indices;
  num_components = str->suminfo->num_components;
  num_output_components = str->suminfo->num_output_components;
  max_discard_levels = str->suminfo->max_discard_levels;
  max_quality_layers = str->suminfo->max_quality_layers;
  if (component_subs != NULL)
    { 
      if (num_comps > num_components)
        num_comps = num_components;
      for (int c=0; c < num_comps; c++)
        component_subs[c] = str->suminfo->component_subs[c];
    }
  if (output_component_subs != NULL)
    { 
      if (num_out_comps > num_output_components)
        num_out_comps = num_output_components;
      for (int c=0; c < num_out_comps; c++)
        output_component_subs[c] = str->suminfo->output_component_subs[c];    
    }
  return true;
}

/*****************************************************************************/
/*                   kdu_servex::get_codestream_rd_info                      */
/*****************************************************************************/

bool
  kdu_servex::get_codestream_rd_info(int stream_id, int &num_layer_slopes,
                                     int &num_layer_lengths,
                                     int *layer_log_slopes,
                                     kdu_long *layer_lengths)
{
  kdsx_stream *str;
  if ((stream_id < 0) || (stream_id > codestream_range[1]) ||
      ((str = stream_refs[stream_id]) == NULL))
    return false;
  if ((str->layer_log_slopes == NULL) || (str->layer_lengths == NULL))
    return false;
  int num_slopes = num_layer_slopes;
  int num_lengths = num_layer_lengths;
  num_layer_slopes = num_layer_lengths = str->num_layer_stats;
  if (layer_log_slopes != NULL)
    { 
      if (num_slopes > num_layer_slopes)
        num_slopes = num_layer_slopes;
      for (int c=0; c < num_slopes; c++)
        layer_log_slopes[c] = str->layer_log_slopes[c];
    }
  if (layer_lengths != NULL)
    { 
      if (num_lengths > num_layer_lengths)
        num_lengths = num_layer_lengths;
      for (int c=0; c < num_lengths; c++)
        layer_lengths[c] = str->layer_lengths[c];
    }
  return true;
}

/*****************************************************************************/
/*                     kdu_servex::attach_to_codestream                      */
/*****************************************************************************/

kdu_codestream
  kdu_servex::attach_to_codestream(int stream_id, kd_serve *thread_handle)
{
  kdsx_stream *str;
  if ((stream_id < 0) || (stream_id > codestream_range[1]) ||
      ((str = stream_refs[stream_id]) == NULL))
    return kdu_codestream();
  mutex.lock();
  if (str->fp == NULL)
    {
      try {
          str->open(target_filename);
        }
      catch (...) {
          mutex.unlock();
          throw;
        }
    }
  str->codestream.augment_cache_threshold(str->per_client_cache);
  str->num_attachments++;
  mutex.unlock();
  return str->codestream;
}

/*****************************************************************************/
/*                     kdu_servex::detach_from_codestream                    */
/*****************************************************************************/

void
  kdu_servex::detach_from_codestream(int stream_id, kd_serve *thread_handle)
{
  kdsx_stream *str;
  if ((stream_id < 0) || (stream_id > codestream_range[1]) ||
      ((str = stream_refs[stream_id]) == NULL))
    return;

  // Check to see if the caller is detaching a codestream which it had locked
  // and forgotten to unlock (could happen if detach operation is occurring
  // inside an exception handler).
  if ((thread_handle != NULL) && (thread_handle == str->locking_thread_handle))
    release_codestreams(1,&stream_id,thread_handle);

  mutex.lock();
  str->num_attachments--;
  str->codestream.augment_cache_threshold(-str->per_client_cache);
  if (str->num_attachments <= 0)
    {
      if (str->unlock_event != NULL)
        { // This really should not happen, but best to better sure than sorry
          release_event(str->unlock_event);
          str->unlock_event = NULL;
        }
      str->close();
    }
  mutex.unlock();
  assert(str->num_attachments >= 0);
}

/*****************************************************************************/
/*                       kdu_servex::lock_codestreams                        */
/*****************************************************************************/

void
  kdu_servex::lock_codestreams(int num_streams, int *stream_indices,
                               kd_serve *thread_handle)
{
  mutex.lock();

  // The following code works through the streams one by one in numerically
  // ascending order, locking those that it can and waiting on any stream
  // it cannot lock.  Proceeding in numerical order is very important to
  // preventing deadlocks.  If another thread is trying to lock a collection
  // of codestreams which intersects with those in `stream_indices', it may
  // end up waiting upon a codestream that we have already locked, while we
  // are waiting on a codestream that it has already locked.  This, however,
  // cannot happen if we lock codestreams in a well-defined order.
  int n, index, last_index=-1;
  kdsx_stream *str=NULL;
  do {
      int min_index = -1;
      kdsx_stream *min_str = NULL;
      for (n=0; n < num_streams; n++)
        {
          index = stream_indices[n];
          if ((index < 0) || (index <= last_index) ||
              (index > codestream_range[1]) ||
              ((str = stream_refs[index]) == NULL))
            continue;
          if ((min_str == NULL) || (index < min_index))
            { min_index = index; min_str = str; }
        }
      if ((str = min_str) != NULL)
        { // Need to see if we can lock `str'
          if ((str->locking_thread_handle != NULL) &&
              (str->locking_thread_handle != thread_handle))
            {
              if (str->unlock_event == NULL)
                {
                  assert(str->num_waiting_for_lock == 0);
                  str->unlock_event = get_event();
                  str->unlock_event->event.reset();
                }
              str->num_waiting_for_lock++;
              while (str->locking_thread_handle != NULL)
                str->unlock_event->event.wait(mutex); // Event is auto-reset
              str->num_waiting_for_lock--;
              if (str->num_waiting_for_lock == 0)
                {
                  release_event(str->unlock_event);
                  str->unlock_event = NULL;
                }
            }
          str->locking_thread_handle = thread_handle;
          last_index = min_index;
        }
    } while (str != NULL);
  mutex.unlock();
}

/*****************************************************************************/
/*                      kdu_servex::release_codestreams                      */
/*****************************************************************************/

void
  kdu_servex::release_codestreams(int num_streams, int *stream_indices,
                                  kd_serve *thread_handle)
{
  int n, index;
  kdsx_stream *str;
  mutex.lock();
  for (n=0; n < num_streams; n++)
    {
      index = stream_indices[n];
      if ((index < 0) || (index > codestream_range[1]) ||
          ((str = stream_refs[index]) == NULL))
        continue;
      assert(str->locking_thread_handle == thread_handle);
      str->locking_thread_handle = NULL;
      if (str->num_waiting_for_lock > 0)
        str->unlock_event->event.set();
    }
  mutex.unlock();
}

/*****************************************************************************/
/*                         kdu_servex::read_metagroup                        */
/*****************************************************************************/

int
  kdu_servex::read_metagroup(const kds_metagroup *group, kdu_byte *buf,
                             int offset, int num_bytes)
{
  if (meta_fp == NULL)
    return 0;
  kdsx_metagroup *grp = (kdsx_metagroup *) group;
  assert(offset >= 0);
  if (num_bytes > (grp->length-offset))
    num_bytes = grp->length-offset;
  if (num_bytes < 0)
    return 0;
  if (grp->data != NULL)
    {
      assert(!grp->is_rubber_length);
      memcpy(buf,grp->data+offset,num_bytes);
    }
  else
    {
      mutex.lock();
      kdu_fseek(meta_fp,grp->fpos+offset);
      num_bytes = (int) fread(buf,1,(size_t) num_bytes,meta_fp);
      mutex.unlock();
    }
  return num_bytes;
}

/*****************************************************************************/
/*                    kdu_servex::get_num_context_members                    */
/*****************************************************************************/

int
  kdu_servex::get_num_context_members(int context_type, int context_idx,
                                      int remapping_ids[])
{
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= context_mappings->num_compositing_layers))
    return 0;
  kdsx_layer_mapping *layer = context_mappings->layer_refs[context_idx];
  if ((remapping_ids[0] >= 0) || (remapping_ids[1] >= 0))
    { // See if the instruction reference is valid
      int iset = remapping_ids[0];
      int inum = remapping_ids[1];
      if ((iset < 0) || (iset >= context_mappings->num_comp_sets) ||
          (inum < 0) || ((inum + context_mappings->comp_set_starts[iset]) >=
                         context_mappings->num_comp_instructions) ||
          (context_mappings->composited_size.x < 1) ||
          (context_mappings->composited_size.y < 1))
        remapping_ids[0] = remapping_ids[1] = -1; // Invalid instruction set
    }
  return layer->num_members;
}

/*****************************************************************************/
/*                    kdu_servex::get_context_codestream                     */
/*****************************************************************************/

int
  kdu_servex::get_context_codestream(int context_type, int context_idx,
                                     int remapping_ids[], int member_idx)
{
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= context_mappings->num_compositing_layers))
    return -1;
  kdsx_layer_mapping *layer = context_mappings->layer_refs[context_idx];
  if ((member_idx < 0) || (member_idx >= layer->num_members))
    return -1;
  return layer->members[member_idx].codestream_idx;
}

/*****************************************************************************/
/*                    kdu_servex::get_context_components                     */
/*****************************************************************************/

const int *
  kdu_servex::get_context_components(int context_type, int context_idx,
                                     int remapping_ids[], int member_idx,
                                     int &num_components)
{
  num_components = 0;
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= context_mappings->num_compositing_layers))
    return NULL;
  kdsx_layer_mapping *layer = context_mappings->layer_refs[context_idx];
  if ((member_idx < 0) || (member_idx >= layer->num_members))
    return NULL;
  num_components = layer->members[member_idx].num_components;
  return layer->members[member_idx].component_indices;
}

/*****************************************************************************/
/*                   kdu_servex::perform_context_remapping                   */
/*****************************************************************************/

bool
  kdu_servex::perform_context_remapping(int context_type, int context_idx,
                                        int remapping_ids[], int member_idx,
                                        kdu_coords &resolution,
                                        kdu_dims &region)
{
  if ((context_type != KDU_JPIP_CONTEXT_JPXL) || (context_idx < 0) ||
      (context_idx >= context_mappings->num_compositing_layers))
    return false;
  kdsx_layer_mapping *layer =
    context_mappings->layer_refs[context_idx];
  if ((member_idx < 0) || (member_idx >= layer->num_members))
    return false;
  kdsx_layer_member *member = layer->members + member_idx;
  kdsx_stream_mapping *stream =
    context_mappings->stream_refs[member->codestream_idx];

  if ((layer->layer_size.x <= 0) || (layer->layer_size.y <= 0))
    { region.size = region.pos = kdu_coords(0,0); }
  else if ((remapping_ids[0] < 0) || (remapping_ids[1] < 0) ||
           (context_mappings->num_comp_instructions < 1) ||
           (context_mappings->composited_size.x <= 0) ||
           (context_mappings->composited_size.y <= 0))
    { // View window expressed relative to compositing layer registration grid
      region.pos.x -= (int)
        ceil((double) member->reg_offset.x * (double) resolution.x /
             ((double) layer->reg_precision.x * (double) layer->layer_size.x));
      resolution.x = (int) 
        ceil(((double) resolution.x) *
             ((double) stream->size.x * (double) member->reg_subsampling.x) /
             ((double) layer->reg_precision.x * (double) layer->layer_size.x));
      region.pos.y -= (int)
        ceil((double) member->reg_offset.y * (double) resolution.y /
             ((double) layer->reg_precision.y * (double) layer->layer_size.y));
      resolution.y = (int) 
        ceil(((double) resolution.y) *
             ((double) stream->size.y * (double) member->reg_subsampling.y) /
             ((double) layer->reg_precision.y * (double) layer->layer_size.y));
    }
  else
    { // View window expressed relative to composition grid
      int iset = remapping_ids[0];
      int inum = remapping_ids[1];
      if ((iset >= 0) && (iset < context_mappings->num_comp_sets))
        inum += context_mappings->comp_set_starts[iset];
      if (inum < 0)
        inum = 0; // Just in case
      if (inum >= context_mappings->num_comp_instructions)
        inum = context_mappings->num_comp_instructions-1;
      kdsx_comp_instruction *inst =
        context_mappings->comp_instructions + inum;
      kdu_coords comp_size = context_mappings->composited_size;

      kdu_dims source_dims = inst->source_dims;
      kdu_dims target_dims = inst->target_dims;
      if (!source_dims)
        source_dims.size = layer->layer_size;
      if (!target_dims)
        target_dims.size = source_dims.size;

      double alpha, beta;
      kdu_coords region_lim = region.pos+region.size;
      int min_val, lim_val;

      alpha = ((double) resolution.x) / ((double) comp_size.x);
      beta =  ((double) target_dims.size.x) / ((double) source_dims.size.x);
      min_val = (int) floor(alpha * target_dims.pos.x);
      lim_val = (int) ceil(alpha * (target_dims.pos.x + target_dims.size.x));
      if (min_val > region.pos.x)
        region.pos.x = min_val;
      if (lim_val < region_lim.x)
        region_lim.x = lim_val;
      region.size.x = region_lim.x - region.pos.x;
      region.pos.x -= (int)
        ceil(alpha * (target_dims.pos.x -
                      beta * (source_dims.pos.x -
                              ((double) member->reg_offset.x) /
                              ((double) layer->reg_precision.x))));
      resolution.x = (int)
        ceil(alpha * beta * ((double) stream->size.x) *
             ((double) member->reg_subsampling.x) /
             ((double) layer->reg_precision.x));

      alpha = ((double) resolution.y) / ((double) comp_size.y);
      beta =  ((double) target_dims.size.y) / ((double) source_dims.size.y);
      min_val = (int) floor(alpha * target_dims.pos.y);
      lim_val = (int) ceil(alpha * (target_dims.pos.y + target_dims.size.y));
      if (min_val > region.pos.y)
        region.pos.y = min_val;
      if (lim_val < region_lim.y)
        region_lim.y = lim_val;
      region.size.y = region_lim.y - region.pos.y;
      region.pos.y -= (int)
        ceil(alpha * (target_dims.pos.y -
                      beta * (source_dims.pos.y -
                              ((double) member->reg_offset.y) /
                              ((double) layer->reg_precision.y))));
      resolution.y = (int)
        ceil(alpha * beta * ((double) stream->size.y) *
             ((double) member->reg_subsampling.y) /
             ((double) layer->reg_precision.y));
    }

  // Adjust region to ensure that it fits inside the codestream resolution
  kdu_coords lim = region.pos + region.size;
  if (region.pos.x < 0)
    region.pos.x = 0;
  if (region.pos.y < 0)
    region.pos.y = 0;
  if (lim.x > resolution.x)
    lim.x = resolution.x;
  if (lim.y > resolution.y)
    lim.y = resolution.y;
  region.size = lim - region.pos;
  if (region.size.x < 0)
    region.size.x = 0;
  if (region.size.y < 0)
    region.size.y = 0;

  return true;
}

/*****************************************************************************/
/*                          kdu_servex::add_stream                           */
/*****************************************************************************/

kdsx_stream *
  kdu_servex::add_stream(int stream_id)
{
  assert(stream_id > codestream_range[1]);

  kdsx_stream *result = new kdsx_stream;
  result->stream_id = stream_id;
  if (stream_tail == NULL)
    stream_head = stream_tail = result;
  else
    stream_tail = stream_tail->next = result;

  if (stream_id >= max_stream_refs)
    {
      int n;
      max_stream_refs += stream_id+1;
      kdsx_stream **new_refs = new kdsx_stream *[max_stream_refs];
      for (n=0; n <= codestream_range[1]; n++)
        new_refs[n] = stream_refs[n];
      if (stream_refs != NULL)
        delete[] stream_refs;
      stream_refs = new_refs;
    }
  while (codestream_range[1] < stream_id)
    stream_refs[++codestream_range[1]] = NULL;
  stream_refs[stream_id] = result;
  return result;
}

/*****************************************************************************/
/*                       kdu_servex::get_temp_entities                       */
/*****************************************************************************/

kdsx_image_entities *
  kdu_servex::get_temp_entities()
{
  kdsx_image_entities *result = free_tmp_entities;
  if (result == NULL)
    result = new kdsx_image_entities;
  else
    free_tmp_entities = result->next;
  result->prev = NULL;
  if ((result->next = tmp_entities) != NULL)
    tmp_entities->prev = result;
  tmp_entities = result;
  tmp_entities->reset();
  return result;
}

/*****************************************************************************/
/*                     kdu_servex::commit_image_entities                     */
/*****************************************************************************/

kdsx_image_entities *
  kdu_servex::commit_image_entities(kdsx_image_entities *tmp)
{
  // Start by unlinking `tmp' and returning it to the free list
  if (tmp->prev == NULL)
    {
      assert(tmp == tmp_entities);
      tmp_entities = tmp->next;
    }
  else
    tmp->prev->next = tmp->next;
  if (tmp->next != NULL)
    tmp->next->prev = tmp->prev;
  tmp->next = free_tmp_entities;
  tmp->prev = NULL;
  free_tmp_entities = tmp;
  
  // Now find or create a committed image entities object for returning
  kdsx_image_entities *prev;
  kdsx_image_entities *result = tmp->find_match(committed_entities_list,prev);
  if (result != NULL)
    return result;

  // If we get here, we have to create the new committed object
  num_committed_entities++;
  result = new kdsx_image_entities;
  result->copy_from(tmp);
  result->prev = prev;
  if (prev == NULL)
    {
      result->next = committed_entities_list;
      committed_entities_list = result;
    }
  else
    {
      result->next = prev->next;
      prev->next = result;
    }
  if (result->next != NULL)
    result->next->prev = result;
  return result;
}

/*****************************************************************************/
/*                   kdu_servex::get_parsed_image_entities                   */
/*****************************************************************************/

kdsx_image_entities *
  kdu_servex::get_parsed_image_entities(kdu_int32 ref_id)
{
  if ((ref_id < 0) || (ref_id >= num_committed_entities))
    { kdu_error e; e << "Cache representation of meta-data structure "
      "appears to be corrupt.  Referencing non-existent "
      "image entities list."; }
  assert(committed_entity_refs != NULL);
  kdsx_image_entities *result = committed_entity_refs[ref_id];
  assert(result->ref_id == ref_id);
  return result;
}

/*****************************************************************************/
/*                       kdu_servex::create_structure                        */
/*****************************************************************************/

void
  kdu_servex::create_structure(kdu_long sub_start, kdu_long sub_lim,
                               int phld_threshold)
{
  jp2_family_src src;
  src.open(target_filename); // Throws exception if non-existent
  jp2_locator loc;
  loc.set_file_pos(sub_start);

  int n;
  jp2_input_box box;
  box.open(&src,loc);
  bool is_raw = (box.get_box_type() != jp2_signature_4cc);
  if (is_raw)
    { // Just a raw code-stream
      box.close(); src.close();
      FILE *fp = fopen(target_filename,"rb");
      kdu_byte marker_buf[2];
      if (fp != NULL)
        kdu_fseek(fp,sub_start);
      bool have_soc = true;
      if ((fp == NULL) || (fread(marker_buf,1,2,fp) != 2) ||
          (marker_buf[0] != (kdu_byte)(KDU_SOC>>8)) ||
          (marker_buf[1] != (kdu_byte) KDU_SOC))
        have_soc = false;
      if (fp != NULL)
        fclose(fp);
      if (!have_soc)
        { kdu_error e; e << "File \"" << target_filename << "\"";
          if (sub_start > 0)
            e << ", with byte offset " << (int) sub_start << ",";
          e << " does not correspond to a valid JP2-family file "
               "or a raw code-stream!";
        }
      metatree = new kdsx_metagroup(this);
      metatree->is_last_in_bin = true;
      metatree->scope->flags = KDS_METASCOPE_MANDATORY
                             | KDS_METASCOPE_IMAGE_MANDATORY
                             | KDS_METASCOPE_LEAF;
      kdsx_image_entities *tmp_entities = get_temp_entities();
      tmp_entities->add_universal(0x01000000 | 0x02000000);
      metatree->scope->entities = commit_image_entities(tmp_entities);
      metatree->scope->sequence = 0;
      kdsx_stream *str = add_stream(0);
      str->start_pos = sub_start;
      str->length = sub_lim - sub_start;
    }
  else
    { // Full JP2/JPX file
      kdsx_metagroup *tail=NULL, *elt;
      kdu_long last_used_bin_id = 0;
      int num_codestreams=0, num_jplh=0, num_jpch=0;
      while (box.exists())
        {
          elt = new kdsx_metagroup(this);
          if (tail == NULL)
            metatree = tail = elt;
          else
            { tail->next = elt; tail = elt; }
          elt->create(NULL,NULL,&box,phld_threshold,last_used_bin_id,
                      num_codestreams,num_jpch,num_jplh,sub_lim,false);
          kdu_long current_pos =
            box.get_locator().get_file_pos() + box.get_box_bytes();
          box.close();
          if (current_pos >= sub_lim)
            break;
          box.open_next();
        }
      if (tail != NULL)
        tail->is_last_in_bin = true;
      src.close();
      
      if (num_codestreams > (codestream_range[1]+1))
        { // Must have found some final codestreams we could not expand;
          // augment the `stream_refs' array so that it provides an entry
          // for each codestream, whether we were able to expand it or not.
          if (num_codestreams > max_stream_refs)
            {
              max_stream_refs = num_codestreams;
              kdsx_stream **new_refs = new kdsx_stream *[max_stream_refs];
              for (n=0; n <= codestream_range[1]; n++)
                new_refs[n] = stream_refs[n];
              if (stream_refs != NULL)
                delete[] stream_refs;
              stream_refs = new_refs;
            }
          while (codestream_range[1] < (num_codestreams-1))
            stream_refs[++codestream_range[1]] = NULL;
        }
      else
        assert(num_codestreams == (codestream_range[1]+1));
      context_mappings->finish_parsing(num_codestreams,num_jplh);
    }

  // Now create the `committed_entity_refs' array
  committed_entity_refs = new kdsx_image_entities *[num_committed_entities];
  kdsx_image_entities *entities=committed_entities_list;
  for (n=0; n < num_committed_entities; n++, entities=entities->next)
    {
      assert(entities != NULL);
      entities->ref_id = n;
      committed_entity_refs[n] = entities;
    }
}

/*****************************************************************************/
/*                  kdu_servex::enable_codestream_access                     */
/*****************************************************************************/

void kdu_servex::enable_codestream_access(int per_client_cache)
{ 
  kdsx_stream *str;
  for (str=stream_head; str != NULL; str=str->next)
    { 
      str->per_client_cache = per_client_cache;
      if (str->url_idx == 0)
        str->target_filename = target_filename;
      else
        { 
          jp2_data_references drefs(&data_references);
          str->target_filename = drefs.get_file_url(str->url_idx);
          if (str->target_filename == NULL)
            { kdu_error e; e << "One or more codestreams in the data source "
              "are defined by reference to an external file which is not "
              "listed in a data references box.  The data source "
              "is illegal."; }
          if (str->target_filename[0] == '\0')
            str->target_filename = target_filename;
        }
    }
}

/*****************************************************************************/
/*                kdu_servex::read_codestream_summary_info                   */
/*****************************************************************************/

void kdu_servex::read_codestream_summary_info()
{
  
  // Finish by generating summary information for all the codestreams
  const char *old_info_header =
    "Kdu-Layer-Info: log_2{Delta-D(MSE)/[2^16*Delta-L(bytes)]}, L(bytes)";
  const char *new_info_header =
    "Kdu-Layer-Info: log_2{Delta-D(squared-error)/Delta-L(bytes)}, L(bytes)";
  size_t old_info_header_len = strlen(old_info_header);
  size_t new_info_header_len = strlen(new_info_header);
  int max_layers=0; // Allocated size of temporary arrays below
  int *slopes=NULL; // Used for temporary storage
  kdu_long *lengths=NULL; // Used for temporary storage
  kdsx_stream *str;
  for (str=stream_head; str != NULL; str=str->next)
    { 
      try {
        int c;
        assert(str->local_suminfo == NULL);
        str->suminfo = str->local_suminfo = new kdsx_stream_suminfo;
        str->open(target_filename);
        kdu_codestream cs = str->codestream;
        str->local_suminfo->create(cs);
        str->num_layer_stats = 0;
        kdu_codestream_comment com;
        int num_layers = 0;
        while ((num_layers == 0) && (com = cs.get_comment(com)).exists())
          { 
            const char *com_text = com.get_text();
            bool old_style_info =
              (strncmp(com_text,old_info_header,old_info_header_len) == 0);
            if ((!old_style_info) &&
                (strncmp(com_text,new_info_header,new_info_header_len) != 0))
              continue;
            com_text = strchr(com_text,'\n');
            double val1, val2;
            while ((com_text != NULL) &&
                   (sscanf(com_text+1,"%lf, %lf",&val1,&val2) == 2))
              { 
                if (num_layers == max_layers)
                  { 
                    max_layers += 10;
                    int *new_slopes = new int[max_layers];
                    for (c=0; c < num_layers; c++)
                      new_slopes[c] = slopes[c];
                    if (slopes != NULL) delete[] slopes;
                    slopes = new_slopes;
                    kdu_long *new_lengths = new kdu_long[max_layers];
                    for (c=0; c < num_layers; c++)
                      new_lengths[c] = lengths[c];
                    if (lengths != NULL) delete[] lengths;
                    lengths = new_lengths;
                  }
                slopes[num_layers] = (int)(256.0*(256-64+val1)+0.5);
                lengths[num_layers] = (kdu_long)(val2+0.5);
                if (old_style_info)
                  slopes[num_layers] += 256*32;
                if ((num_layers > 0) &&
                    (slopes[num_layers] >= slopes[num_layers-1]))
                  slopes[num_layers] = slopes[num_layers-1]-1;
                if (slopes[num_layers] < 0)
                  slopes[num_layers] = 0;
                else if (slopes[num_layers] > 0xFFFF)
                  slopes[num_layers] = 0xFFFF;
                num_layers++;
                com_text = strchr(com_text+1,'\n');
              }
          }
        str->num_layer_stats = num_layers;
        size_t num_ints = (size_t)(num_layers*3);
        str->layer_stats_handle = new int[num_ints];
        memset(str->layer_stats_handle,0,sizeof(int)*num_ints);
        str->layer_lengths = (kdu_long *)(str->layer_stats_handle);
        str->layer_log_slopes = (int *)(str->layer_lengths+num_layers);
        for (c=0; c < num_layers; c++)
          { str->layer_log_slopes[c] = slopes[c];
            str->layer_lengths[c] = lengths[c]; }
        
        // Now take advantage of defaults to save storage
        if (default_stream_suminfo == NULL)
          { 
            default_stream_suminfo = str->local_suminfo;
            str->local_suminfo = NULL;
          }
        else if (default_stream_suminfo->equals(str->local_suminfo))
          { 
            str->suminfo = default_stream_suminfo;
            delete str->local_suminfo;
            str->local_suminfo = NULL;
          }
      } catch (...) {
        if (slopes != NULL)
          delete[] slopes;
        if (lengths != NULL)
          delete[] lengths;
        str->close();
        throw;
      }
      str->close();
    }
  if (slopes != NULL)
    delete[] slopes;
  if (lengths != NULL)
    delete[] lengths;  
}

/*****************************************************************************/
/*                        kdu_servex::save_structure                         */
/*****************************************************************************/

void
  kdu_servex::save_structure(FILE *fp)
{
  fprintf(fp,"kdu_servex2/" KDU_CORE_VERSION "\n");

  kdu_byte outbuf[KDSX_OUTBUF_LEN];
  kdu_byte *bp=outbuf;

  write_big(codestream_range[1]+1,bp);
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp); bp = outbuf;
  default_stream_suminfo->serialize(fp,this);

  kdsx_stream *str;
  for (str=stream_head; str != NULL; str=str->next)
    { 
      int str_type_id = (str->local_suminfo != NULL)?2:1;
      fputc(str_type_id,fp);
      str->serialize(fp,this);
      if (str_type_id == 2)
        str->local_suminfo->serialize(fp,this);
    }
  fputc(0,fp); // No more streams

  int n;
  write_big(num_committed_entities,bp);
  assert((bp-outbuf) <= KDSX_OUTBUF_LEN);
  fwrite(outbuf,1,bp-outbuf,fp); bp = outbuf;
  for (n=0; n < num_committed_entities; n++)
    {
      kdsx_image_entities *entities = committed_entity_refs[n];
      assert(entities->ref_id == n);
      entities->serialize(fp);
    }

  kdsx_metagroup *grp;
  for (grp=metatree; grp != NULL; grp=(kdsx_metagroup *) grp->next)
    grp->serialize(fp);

  context_mappings->serialize(fp);

  jp2_data_references drefs(&data_references);
  int num_drefs = drefs.get_num_urls();
  write_big(num_drefs,bp);
  fwrite(outbuf,1,bp-outbuf,fp); bp = outbuf;
  for (n=1; n <= drefs.get_num_urls(); n++)
    {
      const char *url = drefs.get_url(n);
      int url_len = (int) strlen(url);
      write_big(url_len,bp);
      fwrite(outbuf,1,bp-outbuf,fp); bp = outbuf;
      fwrite(url,1,(size_t) url_len,fp);
    }

  fprintf(fp,"kdu_servex/" KDU_CORE_VERSION "\n");
    // Trailing string used for consistency checking
}

/*****************************************************************************/
/*                         kdu_servex::load_structure                        */
/*****************************************************************************/

void
  kdu_servex::load_structure(FILE *fp)
{
  char header[80]; memset(header,0,80);
  if ((fgets(header,79,fp) == NULL) ||
      (strcmp(header,"kdu_servex2/" KDU_CORE_VERSION "\n") != 0))
    { kdu_error e; e << "Cached metadata and stream identifier structure "
      "file appears to be incompatible with the current server "
      "implementation."; }

  kdu_byte *bp, inbuf[KDSX_OUTBUF_LEN];
  if (fread(bp=inbuf,1,4,fp) != 4)
    { kdu_error e;
      e << "Unable to deserialize metadata structure from the cache."; }
  read_big(max_stream_refs,bp);
  codestream_range[0] = 0;
  codestream_range[1] = max_stream_refs-1;
  assert(default_stream_suminfo == NULL);
  default_stream_suminfo = new kdsx_stream_suminfo;
  default_stream_suminfo->deserialize(fp,this);
  
  stream_refs = new kdsx_stream *[max_stream_refs];
  memset(stream_refs,0,(size_t) max_stream_refs*sizeof(kdsx_stream *));
  int str_type_id;
  while (((str_type_id=fgetc(fp)) == 1) || (str_type_id == 2))
    {
      kdsx_stream *str = new kdsx_stream;
      if (stream_tail == NULL)
        stream_head = stream_tail = str;
      else
        stream_tail = stream_tail->next = str;
      str->deserialize(fp,this);
      if (str_type_id == 2)
        { 
          str->suminfo = str->local_suminfo = new kdsx_stream_suminfo;
          str->local_suminfo->deserialize(fp,this);
        }
      else
        str->suminfo = default_stream_suminfo;
      if ((str->stream_id < 0) || (str->stream_id >= max_stream_refs) ||
          (stream_refs[str->stream_id] != NULL))
        { kdu_error e; e << "Error encountered while deserializing "
          "codestream structure from cache."; }
      stream_refs[str->stream_id] = str;
    }

  if (fread(bp=inbuf,1,4,fp) != 4)
    { kdu_error e;
      e << "Unable to deserialize metadata structure from the cache."; }
  read_big(num_committed_entities,bp);
  if ((num_committed_entities < 0) || (num_committed_entities > 10000))
    { kdu_error e; e << "Unable to deserialize codestream structure from "
      "cache.  Ridiculous number of image entities suggests an error has "
      "occurred."; }
  committed_entity_refs = new kdsx_image_entities *[num_committed_entities];

  int n;
  kdsx_image_entities *entities, *entities_tail=NULL;
  for (n=0; n < num_committed_entities; n++)
    {
      committed_entity_refs[n] = entities = new kdsx_image_entities;
      entities->ref_id = n;
      entities->next = NULL;
      entities->prev = entities_tail;
      if (entities_tail == NULL)
        entities_tail = committed_entities_list = entities;
      else
        entities_tail = entities_tail->next = entities;
      entities->deserialize(fp);
    }

  kdsx_metagroup *mgrp = metatree = new kdsx_metagroup(this);
  while (mgrp->deserialize(NULL,fp))
    mgrp = (kdsx_metagroup *)(mgrp->next = new kdsx_metagroup(this));

  context_mappings->deserialize(fp);

  jp2_data_references drefs(&data_references);
  int num_drefs;
  if (fread(bp=inbuf,1,4,fp) != 4)
    { kdu_error e;
      e << "Unable to deserialize metadata structure from the cache."; }
  read_big(num_drefs,bp);
  char url_buf[512];
  for (n=1; n <= num_drefs; n++)
    {
      int url_len;
      if (fread(bp=inbuf,1,4,fp) != 4)
        { kdu_error e;
          e << "Unable to deserialize metadata structure from the cache."; }
      read_big(url_len,bp);
      if ((url_len < 0) || (url_len > 511) ||
          (fread(url_buf,1,(size_t) url_len,fp) != (size_t) url_len))
        { kdu_error e;
          e << "Unable to deserialize metadata structure from the cache."; }
      url_buf[url_len] = '\0';
      drefs.add_url(url_buf,n);
    }

  memset(header,0,80);
  if ((fgets(header,79,fp) == NULL) ||
      (strcmp(header,"kdu_servex/" KDU_CORE_VERSION "\n") != 0))
    { kdu_error e; e << "Cached metadata and stream identifier structure "
      "file appears to be corrupt."; }
}
