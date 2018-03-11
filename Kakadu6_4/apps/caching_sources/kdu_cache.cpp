/*****************************************************************************/
// File: kdu_cache.cpp [scope = APPS/CACHING_SOURCES]
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
  Implements a platform independent caching compressed data source.  A
complete implementation for the client in an interactive client-server
application can be derived from this class and requires relatively little
additional effort.  The complete client must incorporate networking elements.
******************************************************************************/

#include "cache_local.h"
#include "kdu_messaging.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(kdu_cache.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_cache.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu JPIP Cache:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu JPIP Cache:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================= */
/*                              Internal Functions                           */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                       set_cache_buf_marks                          */
/*****************************************************************************/

static void
  set_cache_buf_marks(kd_var_cache_seg *seg, bool new_state)
  /* Recursive function to set all marks to true or false, depending
     on the `new_state' value. */
{
  int i;
  if (seg->shift > 0)
    {
      for (i=0; i < 128; i++)
        if (seg->segs[i] != NULL)
          set_cache_buf_marks(seg->segs[i],new_state);
    }
  else
    {
      for (i=0; i < 128; i++)
        {
          kd_cache_buf *buf = seg->databins[i];
          if (buf != NULL)
            ((kd_cache_hd *)(buf->bytes))->marked = new_state;
        }
    }
}


/* ========================================================================= */
/*                              kd_var_cache_seg                             */
/* ========================================================================= */

/*****************************************************************************/
/*                       kd_var_cache_seg::~kd_var_cache_seg                 */
/*****************************************************************************/

kd_var_cache_seg::~kd_var_cache_seg()
{
  if (shift > 0)
    for (int n=0; n < 128; n++)
      if (segs[n] != NULL)
        delete segs[n];
}


/* ========================================================================= */
/*                              kd_class_cache                               */
/* ========================================================================= */

/*****************************************************************************/
/*                          kd_class_cache::add_mru                          */
/*****************************************************************************/

void
  kd_class_cache::add_mru(kd_var_cache_seg *seg, kdu_byte idx)
{
  kd_cache_buf *buf = seg->databins[idx];
  if (mru_seg != NULL)
    {
      kd_cache_buf *mru_buf = mru_seg->databins[mru_idx];
      ((kd_cache_hd *)(mru_buf->bytes))->mru_seg = seg;
      ((kd_cache_hd *)(mru_buf->bytes))->mru_idx = idx;
    }
  ((kd_cache_hd *)(buf->bytes))->lru_seg = mru_seg;
  ((kd_cache_hd *)(buf->bytes))->lru_idx = mru_idx;
  ((kd_cache_hd *)(buf->bytes))->mru_seg = NULL;
  ((kd_cache_hd *)(buf->bytes))->mru_idx = 0;
  mru_seg = seg;
  mru_idx = idx;
  if (lru_seg == NULL)
    { lru_seg = mru_seg; lru_idx = mru_idx; }
}

/*****************************************************************************/
/*                          kd_class_cache::add_lru                          */
/*****************************************************************************/

void
  kd_class_cache::add_lru(kd_var_cache_seg *seg, kdu_byte idx)
{
  kd_cache_buf *buf = seg->databins[idx];
  if (lru_seg != NULL)
    {
      kd_cache_buf *lru_buf = lru_seg->databins[lru_idx];
      ((kd_cache_hd *)(lru_buf->bytes))->lru_seg = seg;
      ((kd_cache_hd *)(lru_buf->bytes))->lru_idx = idx;
    }
  ((kd_cache_hd *)(buf->bytes))->mru_seg = lru_seg;
  ((kd_cache_hd *)(buf->bytes))->mru_idx = lru_idx;
  ((kd_cache_hd *)(buf->bytes))->lru_seg = NULL;
  ((kd_cache_hd *)(buf->bytes))->lru_idx = 0;
  lru_seg = seg;
  lru_idx = idx;
  if (mru_seg == NULL)
    { mru_seg = lru_seg; mru_idx = lru_idx; }
}

/*****************************************************************************/
/*                       kd_class_cache::unlink_databin                      */
/*****************************************************************************/

void
  kd_class_cache::unlink_databin(kd_var_cache_seg *seg, kdu_byte idx)
{
  kd_cache_buf *buf = seg->databins[idx];
  kd_var_cache_seg *prev_seg = ((kd_cache_hd *)(buf->bytes))->lru_seg;
  kdu_byte prev_idx = ((kd_cache_hd *)(buf->bytes))->lru_idx;
  kd_var_cache_seg *next_seg = ((kd_cache_hd *)(buf->bytes))->mru_seg;
  kdu_byte next_idx = ((kd_cache_hd *)(buf->bytes))->mru_idx;
  if (prev_seg != NULL)
    {
      kd_cache_buf *prev_buf = prev_seg->databins[prev_idx];
      assert((((kd_cache_hd *)(prev_buf->bytes))->mru_seg == seg) &&
             (((kd_cache_hd *)(prev_buf->bytes))->mru_idx == idx));
      ((kd_cache_hd *)(prev_buf->bytes))->mru_seg = next_seg;
      ((kd_cache_hd *)(prev_buf->bytes))->mru_idx = next_idx;
    }
  else
    {
      assert((lru_seg == seg) && (lru_idx == idx));
      lru_seg = next_seg;
      lru_idx = next_idx;
    }
  if (next_seg != NULL)
    {
      kd_cache_buf *next_buf = next_seg->databins[next_idx];
      assert((((kd_cache_hd *)(next_buf->bytes))->lru_seg == seg) &&
             (((kd_cache_hd *)(next_buf->bytes))->lru_idx == idx));
      ((kd_cache_hd *)(next_buf->bytes))->lru_seg = prev_seg;
      ((kd_cache_hd *)(next_buf->bytes))->lru_idx = prev_idx;
    }
  else
    {
      assert((mru_seg == seg) && (mru_idx == idx));
      mru_seg = prev_seg;
      mru_idx = prev_idx;
    }
}


/* ========================================================================= */
/*                                kdu_cache                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdu_cache::kdu_cache                            */
/*****************************************************************************/

kdu_cache::kdu_cache()
{
  attached = NULL;
  state = new kd_cache();
  mutex.create();
}

/*****************************************************************************/
/*                           kdu_cache::~kdu_cache                           */
/*****************************************************************************/

kdu_cache::~kdu_cache()
{
  close();
  delete state;
  mutex.destroy();
}

/*****************************************************************************/
/*                           kdu_cache::attach_to                            */
/*****************************************************************************/

void
  kdu_cache::attach_to(kdu_cache *existing)
{
  close();
  attached = existing;
}

/*****************************************************************************/
/*                              kdu_cache::close                             */
/*****************************************************************************/

bool
  kdu_cache::close()
{
  attached = NULL;
  assert(state != NULL);
  while ((state->current_stream=state->streams) != NULL)
    {
      state->streams = state->current_stream->next;
      for (int cls=0; cls < KDU_NUM_DATABIN_CLASSES; cls++)
        state->current_stream->classes[cls].reset();
      delete state->current_stream;
    }
  state->total_cache_segs = 0;
  state->read_buf = state->read_start = NULL;
  state->read_head = NULL;
  state->read_buf_pos = state->databin_pos = 0;

  if (state->buf_server != NULL)
    { delete state->buf_server; state->buf_server = NULL; }

  return true;
}

/*****************************************************************************/
/*                         kdu_cache::add_to_databin                         */
/*****************************************************************************/

void
  kdu_cache::add_to_databin(int cls, kdu_long codestream_id, kdu_long id,
                            const kdu_byte data[], int offset, int num_bytes,
                            bool is_complete, bool add_as_most_recent,
                            bool mark_if_augmented)
{
  assert(attached == NULL);

  if (state->buf_server == NULL)
    state->buf_server = new kd_cache_buf_server;

  if ((num_bytes == 0) && !is_complete)
    return; // Nothing to do.
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return; // No representation for the indicated class.
  if (cls == KDU_META_DATABIN)
    codestream_id = 0;

  kd_stream_cache *prev_stream, *stream;
  for (prev_stream=NULL, stream=state->streams;
       stream != NULL;
       prev_stream=stream, stream=stream->next)
    if (stream->codestream_id >= codestream_id)
      break;
  if ((stream == NULL) || (stream->codestream_id > codestream_id))
    { // Create new stream and insert into the list
      acquire_lock();
      kd_stream_cache *new_stream =
        new (std::nothrow) kd_stream_cache(codestream_id);
      if (new_stream == NULL)
        {
          release_lock();
          throw std::bad_alloc();
        }
      new_stream->next = stream;
      stream = new_stream;
      if (prev_stream == NULL)
        state->streams = stream;
      else
        prev_stream->next = stream;
      release_lock();
    }

  kd_class_cache *cache = stream->classes + cls;
  acquire_lock();

  // Locate the relevant cache entry, creating it if necessary.
  kd_var_cache_seg *seg;
  while ((cache->root == NULL) || ((id >> cache->root->shift) >= 128))
    { // Need to grow the depth of the segment tree.
      seg = new (std::nothrow) kd_var_cache_seg;
      if (seg == NULL)
        {
          release_lock();
          throw std::bad_alloc();
        }
      seg->base_id = 0;
      state->total_cache_segs++;
      if (cache->root != NULL)
        {
          seg->shift = cache->root->shift + 7;
          seg->segs[0] = cache->root;
        }
      cache->root = seg;
    }
  kdu_long base_id = 0;
  for (seg=cache->root; seg->shift > 0; )
    {
      kdu_long idx = (id >> seg->shift);
      id -= (idx << seg->shift);
      base_id += (idx << seg->shift);
      assert((idx >= 0) && (idx < 128));
      if (seg->segs[idx] == NULL)
        {
          seg->segs[idx] = new (std::nothrow) kd_var_cache_seg;
          if (seg->segs[idx] == NULL)
            {
              release_lock();
              throw std::bad_alloc();
            }
          seg->segs[idx]->base_id = base_id;
          state->total_cache_segs++;
          seg->segs[idx]->shift = seg->shift - 7;
        }
      seg = seg->segs[idx];
      assert(seg->base_id == base_id);
    }

  kd_cache_buf *buf = seg->databins[id];
  if (buf == NULL)
    { // Create new cache entry, placing it at head of the correct list
      buf = state->buf_server->get();
      memset(buf->bytes,0,sizeof(kd_cache_hd));
      seg->databins[id] = buf;
      if (add_as_most_recent)
        cache->add_mru(seg,(kdu_byte) id);
      else
        cache->add_lru(seg,(kdu_byte) id);
    }
  else if (add_as_most_recent)
    { // Move to head of MRU (tail of LRU) list
      cache->unlink_databin(seg,(kdu_byte) id);
      cache->add_mru(seg,(kdu_byte) id);
    }
  kd_cache_hd *head = (kd_cache_hd *)(buf->bytes);

  /* First write the data itself. */
  kd_cache_buf_io buf_io(state->buf_server,buf,sizeof(kd_cache_hd));
  if (is_complete)
    head->have_final = true;
  
  buf_io.advance(offset);
  buf_io.copy_from(data,num_bytes);

  cache->transferred_bytes += num_bytes; // All bytes are new

  /* Now modify prefix length and the hole list, as appropriate. */
  kd_cache_buf_io hole_src(state->buf_server,head->hole_list);
  kd_cache_buf_io hole_dst(state->buf_server); // Use to write a new hole list
  bool have_existing = false;
  int existing_start=0, existing_lim=0;

  // Merge until the new region is entirely accounted for.
  bool augmented = false;
  bool intersects_with_existing = false;
  int range_start = offset;
  int range_lim = offset+num_bytes;
  while (have_existing=hole_src.read_byte_range(existing_start,existing_lim))
    {
      if (existing_start > range_lim)
        break; // Existing byte range entirely follows new byte range
      if (existing_lim < range_start)
        { // Existing range entirely precedes new range
          hole_dst.write_byte_range(existing_start,existing_lim);
          continue;
        }
      intersects_with_existing = true;
      if (existing_start <= range_start)
        range_start = existing_start;
      else
        augmented = true;
      if (existing_lim >= range_lim)
        range_lim = existing_lim;
      else
        augmented = true;
    }

  if ((range_lim > range_start) && (range_lim > head->initial_bytes))
    { // The new byte range needs to be recorded somewhere
      if (range_start <= head->initial_bytes)
        { // Extends initial segment
          head->initial_bytes = range_lim;
          augmented = true;
        }
      else
        {
          hole_dst.write_byte_range(range_start,range_lim);
          if (!intersects_with_existing)
            augmented = true;
        }
    }

  // Copy any original ranges which have not yet been merged.
  while (have_existing)
    {
      hole_dst.write_byte_range(existing_start,existing_lim);
      have_existing = hole_src.read_byte_range(existing_start,existing_lim);
    }
  hole_dst.finish_list(); // Write terminal 0 if necessary
  
  // Replace old list with new list
  state->buf_server->release(head->hole_list);
  head->hole_list = hole_dst.get_list();
  if (augmented && mark_if_augmented)
    head->marked = true;

  release_lock();
}

/*****************************************************************************/
/*                         kdu_cache::get_databin_length                     */
/*****************************************************************************/

int
  kdu_cache::get_databin_length(int cls, kdu_long codestream_id, kdu_long id,
                                bool *is_complete)
{
  if (attached != NULL)
    return attached->get_databin_length(cls,codestream_id,id,is_complete);

  if (is_complete != NULL)
    *is_complete = false; // Until we have reason to believe otherwise.
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return 0; // No representation for the indicated class.
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return 0;

  kd_class_cache *cache = stream->classes + cls;
  kd_var_cache_seg *seg = cache->root;
  while ((seg != NULL) && (seg->shift > 0))
    {
      kdu_long idx = (id>>seg->shift);
      id -= idx<<seg->shift;
      if (idx >= 128)
        break;
      seg = seg->segs[idx];
    }
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128))
    return 0;
  kd_cache_buf *buf = seg->databins[id];
  if (buf == NULL)
    return 0;
  acquire_lock();
  kd_cache_hd *head = (kd_cache_hd *)(buf->bytes);
  int length = head->initial_bytes;
  if (is_complete != NULL)
    *is_complete = head->have_final && (head->hole_list==NULL);
  release_lock();
  return length;
}

/*****************************************************************************/
/*                         kdu_cache::promote_databin                        */
/*****************************************************************************/

void
  kdu_cache::promote_databin(int cls, kdu_long codestream_id, kdu_long id)
{
  if (attached != NULL)
    return; // Does nothing
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return;
  kd_class_cache *cache = stream->classes + cls;
  kd_var_cache_seg *seg = cache->root;
  while ((seg != NULL) && (seg->shift > 0))
    {
      kdu_long idx = (id>>seg->shift);
      id -= idx<<seg->shift;
      if (idx >= 128)
        break;
      seg = seg->segs[idx];
    }
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128) ||
      (seg->databins[id] == NULL))
    return;
  cache->unlink_databin(seg,(kdu_byte) id);
  cache->add_mru(seg,(kdu_byte) id);
}



/*****************************************************************************/
/*                          kdu_cache::demote_databin                        */
/*****************************************************************************/

void
  kdu_cache::demote_databin(int cls, kdu_long codestream_id, kdu_long id)
{
  if (attached != NULL)
    return; // Does nothing
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return;
  kd_class_cache *cache = stream->classes + cls;
  kd_var_cache_seg *seg = cache->root;
  while ((seg != NULL) && (seg->shift > 0))
    {
      kdu_long idx = (id>>seg->shift);
      id -= idx<<seg->shift;
      if (idx >= 128)
        break;
      seg = seg->segs[idx];
    }
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128) ||
      (seg->databins[id] == NULL))
    return;
  cache->unlink_databin(seg,(kdu_byte) id);
  cache->add_lru(seg,(kdu_byte) id);
}

/*****************************************************************************/
/*                      kdu_cache::get_next_codestream                       */
/*****************************************************************************/

kdu_long
  kdu_cache::get_next_codestream(kdu_long stream_id)
{
  kd_stream_cache *stream = state->streams;
  while ((stream != NULL) && (stream->codestream_id <= stream_id))
    stream = stream->next;
  if (stream == NULL)
    return -1;
  else
    return stream->codestream_id;
}

/*****************************************************************************/
/*                      kdu_cache::get_next_lru_databin                      */
/*****************************************************************************/

kdu_long
  kdu_cache::get_next_lru_databin(int cls, kdu_long codestream_id,
                                  kdu_long id, bool only_if_marked)
{
  if (attached != NULL)
    return attached->get_next_lru_databin(cls,codestream_id,id,only_if_marked);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return -1;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return -1;
  kd_class_cache *cache = stream->classes + cls;
  kd_var_cache_seg *seg = NULL;
  if (id >= 0)
    for (seg=cache->root; (seg != NULL) && (seg->shift > 0); )
      {
        kdu_long idx = (id>>seg->shift);
        id -= idx<<seg->shift;
        if (idx >= 128)
          break;
        seg = seg->segs[idx];
      }
  kd_cache_buf *buf;  
  int seg_idx;
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128) ||
      ((buf=seg->databins[id]) == NULL))
    {
      seg = cache->mru_seg;
      seg_idx = cache->mru_idx;
    }
  else
    {
      seg = ((kd_cache_hd *)(buf->bytes))->lru_seg;
      seg_idx = ((kd_cache_hd *)(buf->bytes))->lru_idx;
    }
  while (only_if_marked && (seg != NULL))
    {
      buf = seg->databins[seg_idx];
      if (((kd_cache_hd *)(buf->bytes))->marked)
        break;
      seg = ((kd_cache_hd *)(buf->bytes))->lru_seg;
      seg_idx = ((kd_cache_hd *)(buf->bytes))->lru_idx;
    }
  if (seg == NULL)
    return -1;
  return (seg->base_id + seg_idx);
}

/*****************************************************************************/
/*                      kdu_cache::get_next_mru_databin                      */
/*****************************************************************************/

kdu_long
  kdu_cache::get_next_mru_databin(int cls, kdu_long codestream_id,
                                  kdu_long id, bool only_if_marked)
{
  if (attached != NULL)
    return attached->get_next_mru_databin(cls,codestream_id,id);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return -1;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return -1;
  kd_class_cache *cache = stream->classes + cls;
  kd_var_cache_seg *seg = NULL;
  if (id >= 0)
    for (seg=cache->root; (seg != NULL) && (seg->shift > 0); )
      {
        kdu_long idx = (id>>seg->shift);
        id -= idx<<seg->shift;
        if (idx >= 128)
          break;
        seg = seg->segs[idx];
      }
  kd_cache_buf *buf;  
  int seg_idx;
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128) ||
      ((buf=seg->databins[id]) == NULL))
    {
      seg = cache->lru_seg;
      seg_idx = cache->lru_idx;
    }
  else
    {
      seg = ((kd_cache_hd *)(buf->bytes))->mru_seg;
      seg_idx = ((kd_cache_hd *)(buf->bytes))->mru_idx;
    }
  while (only_if_marked && (seg != NULL))
    {
      buf = seg->databins[seg_idx];
      if (((kd_cache_hd *)(buf->bytes))->marked)
        break;
      seg = ((kd_cache_hd *)(buf->bytes))->mru_seg;
      seg_idx = ((kd_cache_hd *)(buf->bytes))->mru_idx;
    }
  if (seg == NULL)
    return -1;
  return (seg->base_id + seg_idx);
}

/*****************************************************************************/
/*                            kdu_cache::mark_databin                        */
/*****************************************************************************/

bool
  kdu_cache::mark_databin(int cls, kdu_long codestream_id,
                          kdu_long id, bool mark_state)
{
  if (attached != NULL)
    return false;
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return false;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return false;
  kd_var_cache_seg *seg = stream->classes[cls].root;
  while ((seg != NULL) && (seg->shift > 0))
    {
      kdu_long idx = (id>>seg->shift);
      id -= idx<<seg->shift;
      if (idx >= 128)
        break;
      seg = seg->segs[idx];
    }
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128))
    return false;
  kd_cache_buf *buf = seg->databins[id];
  if (buf == NULL)
    return false;
  acquire_lock();
  bool was_marked = ((kd_cache_hd *)(buf->bytes))->marked;
  ((kd_cache_hd *)(buf->bytes))->marked = mark_state;
  release_lock();
  return was_marked;
}

/*****************************************************************************/
/*                          kdu_cache::clear_all_marks                       */
/*****************************************************************************/

void
  kdu_cache::clear_all_marks()
{
  kd_stream_cache *stream;
  int cls;

  for (stream=state->streams; stream != NULL; stream=stream->next)
    for (cls=0; cls < KDU_NUM_DATABIN_CLASSES; cls++)
      if (stream->classes[cls].root != NULL)
        set_cache_buf_marks(stream->classes[cls].root,false);
}

/*****************************************************************************/
/*                            kdu_cache::set_all_marks                       */
/*****************************************************************************/

void
  kdu_cache::set_all_marks()
{
  kd_stream_cache *stream;
  int cls;

  for (stream=state->streams; stream != NULL; stream=stream->next)
    for (cls=0; cls < KDU_NUM_DATABIN_CLASSES; cls++)
      if (stream->classes[cls].root != NULL)
        set_cache_buf_marks(stream->classes[cls].root,true);
}

/*****************************************************************************/
/*                        kdu_cache::get_databin_prefix                      */
/*****************************************************************************/

int
  kdu_cache::get_databin_prefix(int cls, kdu_long codestream_id, kdu_long id,
                                kdu_byte data[], int max_bytes)
{
  if (attached != NULL)
    return attached->get_databin_prefix(cls,codestream_id,id,data,max_bytes);
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return 0;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    if (stream->codestream_id == codestream_id)
      break;
  if (stream == NULL)
    return 0;

  kd_var_cache_seg *seg = stream->classes[cls].root;
  while ((seg != NULL) && (seg->shift > 0))
    {
      kdu_long idx = (id>>seg->shift);
      id -= idx<<seg->shift;
      if (idx >= 128)
        break;
      seg = seg->segs[idx];
    }
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128))
    return 0;
  kd_cache_buf *buf = seg->databins[id];
  if (buf == NULL)
    return 0;
  acquire_lock();
  kd_cache_hd *head = (kd_cache_hd *)(buf->bytes);
  int buf_pos = sizeof(kd_cache_hd);
  int bytes_left = head->initial_bytes;
  release_lock();
  if (bytes_left > max_bytes)
    bytes_left = max_bytes;
  max_bytes = bytes_left; // So we can return it later
  while (bytes_left > 0)
    {
      if (buf_pos == KD_CACHE_BUF_LEN)
        { buf = buf->next; buf_pos = 0; }
      int xfer_bytes = KD_CACHE_BUF_LEN - buf_pos;
      if (xfer_bytes > bytes_left)
        xfer_bytes = bytes_left;
      memcpy(data,buf->bytes+buf_pos,(size_t) xfer_bytes);
      bytes_left -= xfer_bytes;
      data += xfer_bytes;
      buf_pos += xfer_bytes;
    }
  return max_bytes;
}

/*****************************************************************************/
/*                           kdu_cache::set_read_scope                       */
/*****************************************************************************/

void
  kdu_cache::set_read_scope(int cls, kdu_long codestream_id, kdu_long id)
{
  state->read_buf = state->read_start = NULL;
  state->read_buf_pos = 0;
  state->databin_pos = 0;
  state->read_head = NULL;
  kdu_cache *ref = (attached==NULL)?this:attached;
  assert(id >= 0);
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return;

  kd_stream_cache *stream=state->current_stream;
  if ((stream == NULL) || (stream->codestream_id != codestream_id))
    for (stream=ref->state->streams; stream != NULL; stream=stream->next)
      if (stream->codestream_id == codestream_id)
        break;
  if (stream == NULL)
    return;
  state->current_stream = stream;
  kd_var_cache_seg *seg = stream->classes[cls].root;
  while ((seg != NULL) && (seg->shift > 0))
    {
      kdu_long idx = (id>>seg->shift);
      id -= idx<<seg->shift;
      if (idx >= 128)
        break;
      seg = seg->segs[idx];
    }
  if ((seg == NULL) || (seg->shift > 0) || (id >= 128))
    return;
  kd_cache_buf *buf = seg->databins[id];
  if (buf == NULL)
    return;

  state->read_start = state->read_buf = buf;
  state->read_head = (kd_cache_hd *)(buf->bytes);
  state->read_buf_pos = sizeof(kd_cache_hd);
}

/*****************************************************************************/
/*                              kdu_cache::seek                              */
/*****************************************************************************/

bool
  kdu_cache::seek(kdu_long offset)
{
  if (state->read_head != NULL)
    {
      if (offset < 0)
        offset = 0;
      int off = state->read_head->initial_bytes;
      if (offset < (kdu_long) off)
        off = (int) offset;
      if (off < state->databin_pos)
        { // Reset position
          state->read_buf = state->read_start;
          state->read_buf_pos = sizeof(kd_cache_hd);
          state->databin_pos = 0;
        }
      off -= state->databin_pos;
      while (off > 0)
        {
          if (state->read_buf_pos == KD_CACHE_BUF_LEN)
            {
              state->read_buf = state->read_buf->next;
              state->read_buf_pos = 0;
            }
          int xfer_bytes = KD_CACHE_BUF_LEN - state->read_buf_pos;
          if (xfer_bytes > off)
            xfer_bytes = off;
          off -= xfer_bytes;
          state->read_buf_pos += xfer_bytes;
          state->databin_pos += xfer_bytes;
        }
    }
  return true;
}

/*****************************************************************************/
/*                            kdu_cache::get_pos                             */
/*****************************************************************************/

kdu_long
  kdu_cache::get_pos()
{
  return state->databin_pos;
}

/*****************************************************************************/
/*                        kdu_cache::set_tileheader_scope                    */
/*****************************************************************************/

bool
  kdu_cache::set_tileheader_scope(int tnum, int num_tiles)
{
  kdu_long id = ((kdu_long) tnum);
  bool is_complete;
  kd_stream_cache *stream = state->current_stream;
  if (stream == NULL)
    { KDU_ERROR_DEV(e,0); e <<
        KDU_TXT("Attempting to invoke `kdu_cache::set_tileheader_scope' "
        "without first calling `kdu_cache::set_read_scope' to identify "
        "the code-stream which is being accessed.");
    }
  get_databin_length(KDU_TILE_HEADER_DATABIN,stream->codestream_id,
                     id,&is_complete);
  set_read_scope(KDU_TILE_HEADER_DATABIN,stream->codestream_id,id);
  return  is_complete;
}

/*****************************************************************************/
/*                          kdu_cache::set_precinct_scope                    */
/*****************************************************************************/

bool
  kdu_cache::set_precinct_scope(kdu_long id)
{
  kd_stream_cache *stream = state->current_stream;
  if (stream == NULL)
    { KDU_ERROR_DEV(e,1); e <<
        KDU_TXT("Attempting to invoke `kdu_cache::set_precinct_scope' without "
        "first calling `kdu_cache::set_read_scope' to identify the code-stream "
        "which is being accessed.");
    }
  set_read_scope(KDU_PRECINCT_DATABIN,stream->codestream_id,id);
  return true;
}

/*****************************************************************************/
/*                               kdu_cache::read                             */
/*****************************************************************************/

int
  kdu_cache::read(kdu_byte *data, int num_bytes)
{
  if (state->read_head == NULL)
    return 0;
  int read_lim = state->read_head->initial_bytes - state->databin_pos;
  if (num_bytes > read_lim)
    num_bytes = read_lim;
  int bytes_left = num_bytes;
  while (bytes_left > 0)
    {
      if (state->read_buf_pos == KD_CACHE_BUF_LEN)
        {
          state->read_buf = state->read_buf->next;
          state->read_buf_pos = 0;
        }
      int xfer_bytes = KD_CACHE_BUF_LEN - state->read_buf_pos;
      if (xfer_bytes > bytes_left)
        xfer_bytes = bytes_left;
      memcpy(data,state->read_buf->bytes+state->read_buf_pos,
             (size_t) xfer_bytes);
      bytes_left -= xfer_bytes;
      data += xfer_bytes;
      state->read_buf_pos += xfer_bytes;
      state->databin_pos += xfer_bytes;
    }
  return num_bytes;
}

/*****************************************************************************/
/*                       kdu_cache::get_peak_cache_memory                    */
/*****************************************************************************/

kdu_long
  kdu_cache::get_peak_cache_memory()
{
  if (state->buf_server == NULL)
    return 0;
  return state->buf_server->get_peak_allocated_bytes() +
         state->total_cache_segs * sizeof(kd_var_cache_seg);
}

/*****************************************************************************/
/*                      kdu_cache::get_transferred_bytes                     */
/*****************************************************************************/

kdu_long
  kdu_cache::get_transferred_bytes(int cls)
{
  if ((cls < 0) || (cls >= KDU_NUM_DATABIN_CLASSES))
    return 0;

  kdu_long result = 0;
  kd_stream_cache *stream;
  for (stream=state->streams; stream != NULL; stream=stream->next)
    result += stream->classes[cls].transferred_bytes;
  return result;
}
