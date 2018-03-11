/*****************************************************************************/
// File: mj2.cpp [scope = APPS/JP2]
// Version: Kakadu, V6.4
// Author: David Taubman
// Last Revised: 8 July, 2010
/******************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/******************************************************************************/
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
   Implements the internal machinery whose external interfaces are defined
in the compressed-io header file, "mj2.h".
******************************************************************************/

#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "kdu_file_io.h"
#include "mj2.h"
#include "mj2_local.h"

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
     kdu_error _name("E(mj2.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(mj2.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu File Format Support:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu File Format Support:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        pre_multiply                                */
/*****************************************************************************/

static void
  pre_multiply(double tgt[], double src[], int rows, int cols,
               double scratch[])
  /* Pre-multiply the `tgt' matrix by the `src' matrix, producing an
     output matrix with the same dimensions.  The `tgt' matrix has `rows'
     rows and `cols' columns, while `src' is necessary `rows' x `rows'.
     The `scratch' matrix must contain at least `rows'*`cols' elements.
     Its contents on entry and exit are undefined. */
{
  int m, n, k;
  double sum;
  for (m=0; m < rows; m++)
    for (n=0; n < cols; n++)
      {
        for (sum=0.0, k=0; k < rows; k++)
          sum += src[rows*m+k] * tgt[cols*k+n];
        scratch[cols*m+n] = sum;
      }
  for (m=0; m < rows; m++)
    for (n=0; n < cols; n++)
      tgt[cols*m+n] = scratch[cols*m+n];
}


/* ========================================================================= */
/*                              mj_sample_sizes                              */
/* ========================================================================= */

/*****************************************************************************/
/*                      mj_sample_sizes::load_from_box                       */
/*****************************************************************************/

void
  mj_sample_sizes::load_from_box(jp2_input_box *stsz)
{
  if (head != NULL)
    { KDU_ERROR(e,0); e <<
        KDU_TXT("MJ2 track contains multiple STSZ (Sample Size) boxes.");
    }
  assert((total_samples == 0) &&
         (stsz->get_box_type() == mj2_sample_size_4cc));
  kdu_uint32 ver_flags;
  if (!(stsz->read(ver_flags) && stsz->read(default_size) &&
        stsz->read(total_samples) && (ver_flags == 0)))
    { KDU_ERROR(e,1); e <<
        KDU_TXT("Malformed STSZ (Sample Size) box  found in Motion "
        "JPEG2000 data source.  Insufficient or illegal fields encountered.  "
        "Version and flags must both be zero.");
    }
  if (default_size == 0)
    { // Need to read individual sample sizes.
      kdu_uint32 i, sample_count=total_samples;
      while (sample_count > 0)
        {
          mj_sample_size_store *elt = new mj_sample_size_store;
          if (tail == NULL)
            head = tail = elt;
          else
            tail = tail->next = elt;
          tail->num_elts = tail->remaining_elts;
          if (sample_count < tail->num_elts)
            tail->num_elts = sample_count;
          tail->remaining_elts -= tail->num_elts;
          sample_count -= tail->num_elts;
          for (i=0; i < tail->num_elts; i++)
            if (!stsz->read(tail->elts[i]))
              { KDU_ERROR(e,2); e <<
                  KDU_TXT("Malformed STSZ (Sample Size) box found in Motion "
                  "JPEG2000 data source.  Box contains insufficient "
                  "sample size data to accommodate all samples.");
              }
        }
      tail = NULL;
    }
  if (stsz->get_remaining_bytes() != 0)
    { KDU_ERROR(e,3); e <<
        KDU_TXT("Malformed STSZ (Sample Size) box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  stsz->close();
}

/*****************************************************************************/
/*                       mj_sample_sizes::save_to_box                        */
/*****************************************************************************/

void
  mj_sample_sizes::save_to_box(jp2_output_box *super_box)
{
  finalize();
  jp2_output_box stsz;
  stsz.open(super_box,mj2_sample_size_4cc);
  stsz.write((kdu_uint32) 0); // Version=0, Flags=0
  stsz.write((kdu_uint32) default_size);
  stsz.write((kdu_uint32) total_samples);
  if (default_size == 0)
    {
      mj_sample_size_store *scan;
      kdu_uint32 i, test_count = 0;
      for (scan=head; scan != NULL; scan=scan->next)
        {
          test_count += scan->num_elts;
          for (i=0; i < scan->num_elts; i++)
            stsz.write(scan->elts[i]);
        }
      assert(test_count == total_samples);
    }
  stsz.close();
}

/*****************************************************************************/
/*                          mj_sample_sizes::append                        */
/*****************************************************************************/

void
  mj_sample_sizes::append(kdu_uint32 size)
{
  assert(current == NULL);
  if (total_samples == 0)
    {
      head = tail = new mj_sample_size_store;
      default_size = size;      
    }
  else if (size != default_size)
    default_size = 0;
  if (tail->remaining_elts == 0)
    {
      mj_sample_size_store *elt = new mj_sample_size_store;
      tail = tail->next = elt;
    }
  tail->elts[tail->num_elts++] = size;
  tail->remaining_elts--;
  total_samples++;
}

/*****************************************************************************/
/*                          mj_sample_sizes::finalize                        */
/*****************************************************************************/

void
  mj_sample_sizes::finalize()
{
  if (tail == NULL)
    return; // Finalization already complete, or unnecessary
  assert(current == NULL);
  if (default_size != 0)
    { // We can delete the storage for individual sizes; they are all the same
      while ((tail=head) != NULL)
        {
          head = tail->next;
          delete tail;
        }
    }
  tail = NULL;
}

/*****************************************************************************/
/*                      mj_sample_sizes::get_sample_size                     */
/*****************************************************************************/

kdu_uint32
  mj_sample_sizes::get_sample_size(kdu_uint32 sample_idx)
{
  assert(sample_idx < total_samples);
  if (default_size != 0)
    return default_size;
  if ((current == NULL) || (sample_idx < current_sample_idx))
    { current=head; current_sample_idx=0; }
  
  kdu_uint32 offset = sample_idx - current_sample_idx;
  while (offset >= current->num_elts)
    {
      offset -= current->num_elts;
      current_sample_idx += current->num_elts;
      current = current->next;
      assert(current != NULL);
    }
  return current->elts[offset];
}


/* ========================================================================= */
/*                              mj_chunk_offsets                             */
/* ========================================================================= */

/*****************************************************************************/
/*                     mj_chunk_offsets::load_from_box                       */
/*****************************************************************************/

void
  mj_chunk_offsets::load_from_box(jp2_input_box *stco)
{
  if (head != NULL)
    { KDU_ERROR(e,4); e <<
        KDU_TXT("MJ2 track contains multiple STCO (Chunk Offset) boxes.");
    }
  assert((total_chunks == 0) &&
         ((stco->get_box_type() == mj2_chunk_offset_4cc) ||
          (stco->get_box_type() == mj2_chunk_offset64_4cc)));
  kdu_uint32 ver_flags;
  if (!(stco->read(ver_flags) && stco->read(total_chunks) && (ver_flags == 0)))
    { KDU_ERROR(e,5); e <<
        KDU_TXT("Malformed STCO (Chunk Offset) box found in Motion JPEG2000 "
        "data source.  Insufficient or illegal fields encountered.  Version "
        "and flags must both be zero.");
    }
  kdu_uint32 i, val1, val2, chunk_count=total_chunks;
  while (chunk_count > 0)
    {
      mj_chunk_offset_store *elt = new mj_chunk_offset_store;
      if (tail == NULL)
        head = tail = elt;
      else
        tail = tail->next = elt;
      tail->num_elts = tail->remaining_elts;
      if (chunk_count < tail->num_elts)
        tail->num_elts = chunk_count;
      tail->remaining_elts -= tail->num_elts;
      chunk_count -= tail->num_elts;
      if (stco->get_box_type() == mj2_chunk_offset_4cc)
        { // 32-bit chunk offsets
          for (i=0; i < tail->num_elts; i++)
            {
              if (!stco->read(val1))
                { KDU_ERROR(e,6);  e <<
                    KDU_TXT("Malformed STCO (Chunk Offset) box found in "
                    "Motion JPEG2000 data source.  Box contains insufficient "
                    "sample size data to accommodate all samples.");
                }
              tail->elts[i] = (kdu_long) val1;
              if (tail->elts[i] < 0)
                { KDU_ERROR_DEV(e,7); e <<
                    KDU_TXT("Chunk offset found in Motion JPEG2000 "
                    "data STCO (Chunk Offset) box is too large to be "
                    "represented as a signed 32-bit integer.  To avoid this "
                    "problem, recompile the Kakadu software to work with "
                    "64-bit variants of the \"kdu_long\" primitive data "
                    "type.");
                }
            }
        }
      else
        { // 64-bit chunk offsets
          for (i=0; i < tail->num_elts; i++)
            {
              if (!(stco->read(val1) && stco->read(val2)))
                { KDU_ERROR(e,8); e <<
                    KDU_TXT("Malformed STCO (Chunk Offset) box found in "
                    "Motion JPEG2000 data source.  Box contains insufficient "
                    "sample size data to accommodate all samples.");
                }
#ifdef KDU_LONG64
              tail->elts[i] = (((kdu_long) val1) << 32) + val2;
#else
              tail->elts[i] = (kdu_long) val2;
              if ((val1 != 0) || (tail->elts[i] < 0))
                { KDU_ERROR_DEV(e,9); e <<
                    KDU_TXT("Chunk offset found in Motion JPEG2000 "
                    "data STCO (Chunk Offset) box is too large to be "
                    "represented as a signed 32-bit integer.  To avoid this "
                    "problem, recompile the Kakadu software to work with "
                    "64-bit variants of the \"kdu_long\" primitive data "
                    "type.");
                }
#endif // KDU_LONG64
            }
        }
    }
  tail = NULL;
  if (stco->get_remaining_bytes() != 0)
    { KDU_ERROR(e,10); e <<
        KDU_TXT("Malformed STCO (Chunk Offset) box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  stco->close();
}

/*****************************************************************************/
/*                      mj_chunk_offsets::save_to_box                        */
/*****************************************************************************/

void
  mj_chunk_offsets::save_to_box(jp2_output_box *super_box)
{
  jp2_output_box stco;
  kdu_uint32 max_offset32 = (kdu_uint32) max_offset;
  if (max_offset == (kdu_long) max_offset32)
    { // Use the version which has 32-bit chunk offsets
      stco.open(super_box,mj2_chunk_offset_4cc);
      stco.write((kdu_uint32) 0); // Version=0, Flags=0
      stco.write(total_chunks);
      mj_chunk_offset_store *scan;
      kdu_uint32 i, test_count = 0;
      for (scan=head; scan != NULL; scan=scan->next)
        {
          test_count += scan->num_elts;
          for (i=0; i < scan->num_elts; i++)
            stco.write((kdu_uint32)(scan->elts[i]));
        }
      assert(test_count == total_chunks);
    }
  else
    { // Use the version which has 64-bit chunk offsets
#ifdef KDU_LONG64
      stco.open(super_box,mj2_chunk_offset64_4cc);
      stco.write((kdu_uint32) 0); // Version=0, Flags=0
      stco.write(total_chunks);
      mj_chunk_offset_store *scan;
      kdu_uint32 i, test_count = 0;
      for (scan=head; scan != NULL; scan=scan->next)
        {
          test_count += scan->num_elts;
          for (i=0; i < scan->num_elts; i++)
            {
              stco.write((kdu_uint32)(scan->elts[i]>>32));
              stco.write((kdu_uint32)(scan->elts[i]));
            }
        }
      assert(test_count == total_chunks);
#else
      assert(0);
#endif // KDU_LONG64
    }
  stco.close();
}

/*****************************************************************************/
/*                          mj_chunk_offsets::append                         */
/*****************************************************************************/

void
  mj_chunk_offsets::append(kdu_long offset)
{
  assert(current == NULL);
  if (tail == NULL)
    head = tail = new mj_chunk_offset_store;
  if (tail->remaining_elts == 0)
    {
      mj_chunk_offset_store *elt = new mj_chunk_offset_store;
      tail = tail->next = elt;
    }
  tail->elts[tail->num_elts++] = offset;
  tail->remaining_elts--;
  total_chunks++;
  max_offset = (offset>max_offset)?offset:max_offset;
}

/*****************************************************************************/
/*                     mj_chunk_offsets::get_chunk_offset                    */
/*****************************************************************************/

kdu_long
  mj_chunk_offsets::get_chunk_offset(kdu_uint32 chunk_idx)
{
  assert(chunk_idx < total_chunks);
  if ((current == NULL) || (chunk_idx < current_chunk_idx))
    { current=head; current_chunk_idx=0; }

  kdu_uint32 offset = chunk_idx - current_chunk_idx;
  while (offset >= current->num_elts)
    {
      offset -= current->num_elts;
      current_chunk_idx += current->num_elts;
      current = current->next;
      assert(current != NULL);
    }
  return current->elts[offset];
}


/* ========================================================================= */
/*                              mj_sample_chunks                             */
/* ========================================================================= */

/*****************************************************************************/
/*                      mj_sample_chunks::load_from_box                      */
/*****************************************************************************/

void
  mj_sample_chunks::load_from_box(jp2_input_box *stsc)
{
  if (head != NULL)
    { KDU_ERROR(e,11); e <<
        KDU_TXT("MJ2 track contains multiple STSC (Sample to Chunk) boxes.");
    }
  assert(stsc->get_box_type() == mj2_sample_to_chunk_4cc);
  kdu_uint32 ver_flags, entry_count;
  if (!(stsc->read(ver_flags) && stsc->read(entry_count) && (ver_flags == 0)))
    { KDU_ERROR(e,12); e <<
        KDU_TXT("Malformed STSC (Sample to Chunk) box found in Motion "
        "JPEG2000 data source.  Insufficient or illegal fields encountered.  "
        "Version and flags must both be zero.");
    }
  kdu_uint32 first_chunk, prev_first_chunk, samples_per_chunk, description_idx;
  for (; entry_count > 0; entry_count--, prev_first_chunk=first_chunk)
    {
      if (!(stsc->read(first_chunk) && stsc->read(samples_per_chunk) &&
            stsc->read(description_idx)))
        { KDU_ERROR(e,13); e <<
            KDU_TXT("Malformed STSC (Sample to Chunk) box found in Motion "
            "JPEG2000 data source.  Insufficient fields found inside box.");
        }
      if (description_idx != 1)
        { KDU_ERROR(e,14); e <<
            KDU_TXT("Found reference to non-initial sample description in a "
            "STSC (Sample to Chunk) box within the Motion JPEG2000 data "
            "source.  While this is legal, the current implementation "
            "supports only single description tracks, which is consistent "
            "with simpler Motion JPEG2000 profiles.");
        }
      if (tail == NULL)
        head = tail = new mj_samples_per_chunk;
      else
        {
          if (first_chunk <= prev_first_chunk)
            { KDU_ERROR(e,15); e <<
                KDU_TXT("Malformed STSC (Sample to Chunk) box found in Motion "
                "JPEG2000 data source.  The `first_chunk' indices "
                "associated with successive list elements must be "
                "strictly increasing.");
            }
          mj_samples_per_chunk *elt = new mj_samples_per_chunk;
          tail->run = first_chunk - prev_first_chunk;
          tail = tail->next = elt;
        }
      tail->num_samples = samples_per_chunk;
    }
  tail = NULL;
  if (stsc->get_remaining_bytes() != 0)
    { KDU_ERROR(e,16);  e <<
        KDU_TXT("Malformed STSC (Sample to Chunk) box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  stsc->close();
}

/*****************************************************************************/
/*                       mj_sample_chunks::save_to_box                       */
/*****************************************************************************/

void
  mj_sample_chunks::save_to_box(jp2_output_box *super_box)
{
  kdu_uint32 counter;
  mj_samples_per_chunk *scan;

  finalize();
  jp2_output_box stsc;
  stsc.open(super_box,mj2_sample_to_chunk_4cc);
  stsc.write((kdu_uint32) 0); // Version=0, Flags=0
  for (counter=0, scan=head; scan != NULL; scan=scan->next)
    {
      counter++;
      if (scan->run == 0)
        break;
    }
  stsc.write(counter);
  for (counter=1, scan=head; scan != NULL; scan=scan->next)
    {
      stsc.write(counter);
      stsc.write(scan->num_samples);
      stsc.write((kdu_uint32) 1); // In simple profiles, each track may have
                                  // only one sample description.
      counter += scan->run;
      if (scan->run == 0)
        break;
    }
  stsc.close();
}

/*****************************************************************************/
/*                       mj_sample_chunks::append_sample                     */
/*****************************************************************************/

void
  mj_sample_chunks::append_sample(kdu_uint32 chunk_idx)
{
  assert(current == NULL);
  if (head == NULL)
    {
      tail = head = new mj_samples_per_chunk;
      current_chunk_idx = 0;
      sample_counter = 0;
    }
  while (chunk_idx > current_chunk_idx)
    {
      current_chunk_idx++;
      if ((tail->run > 0) && (tail->num_samples != sample_counter))
        {
          mj_samples_per_chunk *elt = new mj_samples_per_chunk;
          tail = tail->next = elt;
        }
      tail->num_samples = sample_counter;
      tail->run++;
      sample_counter = 0;
    }
  sample_counter++;
}

/*****************************************************************************/
/*                         mj_sample_chunks::finalize                        */
/*****************************************************************************/

void
  mj_sample_chunks::finalize()
{
  if (tail == NULL)
    return; // Finalization already complete, or else unnecessary.
  assert(current == NULL);
  if ((tail->run > 0) && (tail->num_samples != sample_counter))
    {
      mj_samples_per_chunk *elt = new mj_samples_per_chunk;
      tail = tail->next = elt;
    }
  tail->num_samples = sample_counter;
  tail->run++;
  tail = NULL; // So we will not attempt to finalize again.
  sample_counter = 0;
}

/*****************************************************************************/
/*                 mj_sample_chunks::get_sample_chunk                        */
/*****************************************************************************/

kdu_uint32
  mj_sample_chunks::get_sample_chunk(kdu_uint32 sample_idx,
                                     kdu_uint32 &sample_in_chunk)
{
  if ((current == NULL) || (sample_idx < current_sample_idx))
    { // Start from the beginning
      current = head; current_sample_idx = 0; current_chunk_idx = 0;
    }

  kdu_uint32 incr;
  kdu_uint32 sample_gap = sample_idx - current_sample_idx;
  while ((current->run > 0) &&
         ((incr = current->num_samples*current->run) <= sample_gap))
    { // Advance to next record
      sample_gap -= incr;
      current_sample_idx += incr;
      current_chunk_idx += current->run;
      current = current->next;
      assert(current != NULL);
    }
  incr = sample_gap / current->num_samples; // Chunks into record
  sample_gap -= incr * current->num_samples;
  sample_in_chunk = sample_gap;
  return current_chunk_idx+incr;
}


/* ========================================================================= */
/*                              mj_sample_times                              */
/* ========================================================================= */

/*****************************************************************************/
/*                       mj_sample_times::load_from_box                      */
/*****************************************************************************/

void
  mj_sample_times::load_from_box(jp2_input_box *stts)
{
  if (head != NULL)
    { KDU_ERROR(e,17); e <<
        KDU_TXT("MJ2 track contains multiple STTS (Time to Sample) boxes.");
    }
  assert((total_samples == 0) && (total_ticks == 0));
  assert(stts->get_box_type() == mj2_time_to_sample_4cc);
  kdu_uint32 ver_flags, entry_count;
  if (!(stts->read(ver_flags) && stts->read(entry_count) && (ver_flags == 0)))
    { KDU_ERROR(e,18); e <<
        KDU_TXT("Malformed STTS (Time to Sample) box found in Motion JPEG2000 "
        "data source.  Insufficient or illegal fields encountered.  "
        "Version and flags must both be zero.");
    }
  for (; entry_count > 0; entry_count--)
    {
      mj_ticks_per_sample *elt = new mj_ticks_per_sample;
      if (tail == NULL)
        head = tail = elt;
      else
        tail = tail->next = elt;

      if (!(stts->read(tail->run) && stts->read(tail->num_ticks)))
        { KDU_ERROR(e,19); e <<
            KDU_TXT("Malformed STTS (Time to Sample) box found in Motion "
            "JPEG2000 data source.  Box terminated unexpectedly.");
        }
      total_samples += tail->run;
      total_ticks += tail->num_ticks*tail->run;
    }
  tail = NULL;
  if (stts->get_remaining_bytes() != 0)
    { KDU_ERROR(e,20); e <<
        KDU_TXT("Malformed STTS (Time to Sample) box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  stts->close();
}

/*****************************************************************************/
/*                        mj_sample_times::save_to_box                       */
/*****************************************************************************/

void
  mj_sample_times::save_to_box(jp2_output_box *super_box)
{
  kdu_uint32 entry_count;
  mj_ticks_per_sample *scan;

  jp2_output_box stts;
  stts.open(super_box,mj2_time_to_sample_4cc);
  stts.write((kdu_uint32) 0); // Version=0, Flags=0
  for (entry_count=0, scan=head; scan != NULL; scan=scan->next)
    entry_count++;
  stts.write(entry_count);
  for (scan=head; scan != NULL; scan=scan->next)
    {
      stts.write(scan->run);
      stts.write(scan->num_ticks);
    }
  stts.close();
}

/*****************************************************************************/
/*                          mj_sample_times::append                          */
/*****************************************************************************/

void
  mj_sample_times::append(kdu_uint32 sample_ticks)
{
  assert(current == NULL);
  if (tail == NULL)
    {
      head = tail = new mj_ticks_per_sample;
      tail->num_ticks = sample_ticks;
    }
  if (tail->num_ticks != sample_ticks)
    {
      mj_ticks_per_sample *elt = new mj_ticks_per_sample;
      tail = tail->next = elt;
      tail->num_ticks = sample_ticks;
    }
  tail->run++;
  total_samples++;
  total_ticks += sample_ticks;
}

/*****************************************************************************/
/*                         mj_sample_times::get_period                       */
/*****************************************************************************/

kdu_uint32
  mj_sample_times::get_period()
{
  if (total_samples == 0)
    return 0; // Just in case there are no frames.
  assert(current_sample_idx < total_samples);
  if (current == NULL)
    {
      current=head; current_sample_idx=current_tick=0;
      while (current->run == 0)
        current = current->next;
    }
  return current->num_ticks;
}

/*****************************************************************************/
/*                       mj_sample_times::seek_to_sample                     */
/*****************************************************************************/

kdu_uint32
  mj_sample_times::seek_to_sample(kdu_uint32 sample_idx)
{
  assert(sample_idx < total_samples);
  if ((current == NULL) || (sample_idx < current_sample_idx))
    { // Start from the beginning
      current = head; current_sample_idx = current_tick = 0;
    }
  kdu_uint32 sample_gap = sample_idx - current_sample_idx;
  while (current->run <= sample_gap)
    { // Advance to next record
      sample_gap -= current->run;
      current_sample_idx += current->run;
      current_tick += current->num_ticks * current->run;
      current = current->next;
      assert(current != NULL);
    }

  return current_tick + sample_gap*current->num_ticks;
}

/*****************************************************************************/
/*                        mj_sample_times::seek_to_time                      */
/*****************************************************************************/

kdu_uint32
  mj_sample_times::seek_to_time(kdu_uint32 time)
{
  assert(total_samples > 0);
  if (time >= total_ticks)
    {
      seek_to_sample(total_samples-1);
      return total_samples-1;
    }

  if ((current == NULL) || (time < current_tick))
    { // Start from the beginning
      current = head; current_sample_idx = current_tick = 0;
    }
  while ((time-current_tick) >= (current->num_ticks*current->run))
    {
      current_tick += current->num_ticks*current->run;
      current_sample_idx += current->run;
      current = current->next;
      assert(current != NULL);
    }
  kdu_uint32 offset = (time-current_tick) / current->num_ticks;
  assert(offset < current->run);
  return current_sample_idx + offset;
}


/* ========================================================================= */
/*                                 mj_time                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                           mj_time::finalize                             */
/*****************************************************************************/

void
  mj_time::set_defaults()
{
  if (creation_time == 0)
    {
      if (modification_time != 0)
        creation_time = modification_time;
      else
        { // Set to current time.
          time_t sys_time;
          time(&sys_time); // Get num seconds, since midnight, 1/1/1970
          creation_time = 24107*24*60*60; // Seconds from 1/1/1904 to 1/1/1970
#ifdef KDU_LONG64
          creation_time += (kdu_long) sys_time;
#else
          creation_time += (kdu_uint32) sys_time;
#endif
        }
    }
  if (modification_time == 0)
    modification_time = creation_time;
}


/* ========================================================================= */
/*                                 mj_track                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                            mj_track::~mj_track                            */
/*****************************************************************************/

mj_track::~mj_track()
{
  if (video_track != NULL)
    delete video_track;
}

/*****************************************************************************/
/*                              mj_track::flush                              */
/*****************************************************************************/

void
  mj_track::flush()
{
  if (video_track != NULL)
    video_track->flush();
  media_times.duration = sample_times.get_duration();
  media_times.set_defaults();
  track_times = media_times; // May change the time scale for the track when
                             // determining that for the movie as a whole.
}

/*****************************************************************************/
/*                      mj_track::read_track_header_box                      */
/*****************************************************************************/

void
  mj_track::read_track_header_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_track_header_4cc);
  int i, m, n;
  kdu_uint32 data[8];
  kdu_uint32 uint32;
  kdu_int32 int32;
  kdu_int16 int16;
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) > 1))
    { KDU_ERROR(e,21); e <<
        KDU_TXT("Malformed track header box found in Motion "
        "JPEG2000 data source.  Version number must be 1 or 0.");
    }
  if (!(ver_flags & 1))
    disabled = true;
  bool long_version = ((ver_flags >> 24) == 1);
  for (i=0; i < ((long_version)?8:5); i++)
    box->read(data[i]);
  track_times.creation_time = data[(long_version)?1:0];
  track_times.modification_time = data[(long_version)?3:1];
  track_idx = data[(long_version)?4:2];
  track_times.duration = data[(long_version)?7:4];
#ifdef KDU_LONG64
  if (long_version)
    {
      track_times.creation_time += ((kdu_long) data[0])<<32;
      track_times.modification_time += ((kdu_long) data[2])<<32;
      track_times.duration += ((kdu_long) data[6])<<32;
    }
#endif // KDU_LONG64
  for (i=0; i < 2; i++)
    box->read(uint32); // Reserved
  box->read(compositing_order);
  box->read(int16); // Pre-defined
  box->read(int16);
  presentation_volume = ((double) int16) / 256.0;
  box->read(int16);
  for (n=0; n < 3; n++)
    for (m=0; m < 3; m++)
      if (m < 2)
        {
          box->read(int32);
          transformation_matrix[3*m+n] = ((double) int32) / ((double)(1<<16));
        }
      else
        {
          box->read(int32);
          transformation_matrix[3*m+n] = ((double) int32) / ((double)(1<<30));
        }

  box->read(int32);
  presentation_width = ((double) int32) / ((double)(1<<16));
  if (!box->read(int32))
    { KDU_ERROR(e,22); e <<
        KDU_TXT("Malformed track header box found in Motion "
        "JPEG2000 data source.  Box terminated prematurely.");
    }
  presentation_height = ((double) int32) / ((double)(1<<16));

  if (box->get_remaining_bytes() != 0)
    { KDU_ERROR(e,23); e <<
        KDU_TXT("Malformed track header box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  box->close();
}

/*****************************************************************************/
/*                      mj_track::write_track_header_box                     */
/*****************************************************************************/

void
  mj_track::write_track_header_box(jp2_output_box *super_box)
{
  jp2_output_box box;
  box.open(super_box,mj2_track_header_4cc);
  bool long_version = track_times.need_long_version();
  box.write((kdu_uint32)((long_version)?0x01000003:0x00000003));
#ifdef KDU_LONG64
  if (long_version)
    {
      box.write((kdu_uint32)(track_times.creation_time>>32));
      box.write((kdu_uint32) track_times.creation_time);
      box.write((kdu_uint32)(track_times.modification_time>>32));
      box.write((kdu_uint32) track_times.modification_time);
      box.write(track_idx);
      box.write((kdu_uint32) 0); // Reserved
      box.write((kdu_uint32)(track_times.duration>>32));
      box.write((kdu_uint32) track_times.duration);
    }
  else
#endif // KDU_LONG64
    {
      assert(!long_version);
      box.write((kdu_uint32) track_times.creation_time);
      box.write((kdu_uint32) track_times.modification_time);
      box.write(track_idx);
      box.write((kdu_uint32) 0); // Reserved
      box.write((kdu_uint32) track_times.duration);
    }
  box.write((kdu_uint32) 0); // Reserved
  box.write((kdu_uint32) 0); // Reserved
  box.write(compositing_order);
  box.write((kdu_int16) 0); // Pre-defined
  if (fabs(presentation_volume) > 127.0)
    { KDU_ERROR(e,24); e <<
        KDU_TXT("Presentation volume too large to be correctly "
        "represented in MJ2 track header box.");
    }
  box.write((kdu_int16)(presentation_volume*256.0+0.5));
  box.write((kdu_int16) 0); // Reserved
  int m, n;
  for (n=0; n < 3; n++)
    for (m=0; m < 3; m++)
      if (m < 2)
        {
          if (fabs(transformation_matrix[3*m+n]) > (double)((1<<15)-1))
            { KDU_ERROR(e,25); e <<
                KDU_TXT("Non-trivial elements of the video "
                "transformation matrix must be representable as 16.16 signed "
                "fixed point numbers.");
            }
          box.write((kdu_int32)(transformation_matrix[3*m+n]*(1<<16)));
        }
      else
        {
          if (fabs(transformation_matrix[3*m+n]) >= 2.0)
            { KDU_ERROR(e,26); e <<
                KDU_TXT("The last row of the video "
                "transformation matrix must be representable as 2.30 signed "
                "fixed point numbers.");
            }
          box.write((kdu_int32)(transformation_matrix[3*m+n]*(1<<30)));
        }
  box.write((kdu_int32)(presentation_width*(1<<16)));
  box.write((kdu_int32)(presentation_height*(1<<16)));
  box.close();
}

/*****************************************************************************/
/*                     mj_track::read_media_header_box                       */
/*****************************************************************************/

void
  mj_track::read_media_header_box(jp2_input_box *box)
{
  assert((box->get_box_type() == mj2_media_header_4cc) ||
         (box->get_box_type() == mj2_media_header_typo_4cc));
  int i;
  kdu_uint32 data[7];
  kdu_uint16 uint16;
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) > 1))
    { KDU_ERROR(e,27); e <<
        KDU_TXT("Malformed media header box found in Motion "
        "JPEG2000 data source.  Version number must be 1 or 0.");
    }
  bool long_version = ((ver_flags >> 24) == 1);
  for (i=0; i < ((long_version)?7:4); i++)
    box->read(data[i]);
  media_times.creation_time = data[(long_version)?1:0];
  media_times.modification_time = data[(long_version)?3:1];
  media_times.timescale = data[(long_version)?4:2];
  media_times.duration = data[(long_version)?6:3];
#ifdef KDU_LONG64
  if (long_version)
    {
      media_times.creation_time += ((kdu_long) data[0])<<32;
      media_times.modification_time += ((kdu_long) data[2])<<32;
      media_times.duration += ((kdu_long) data[5])<<32;
    }
#endif // KDU_LONG64
  box->read(uint16); // Language word
  if (!box->read(uint16)) // Pre-defined
    { KDU_ERROR(e,28); e <<
        KDU_TXT("Malformed media header box found in Motion "
        "JPEG2000 data source.  Box terminated prematurely.");
    }
  if (box->get_remaining_bytes() != 0)
    { KDU_ERROR(e,29); e <<
        KDU_TXT("Malformed media header box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  box->close();
}

/*****************************************************************************/
/*                     mj_track::write_media_header_box                      */
/*****************************************************************************/

void
  mj_track::write_media_header_box(jp2_output_box *super_box)
{
  jp2_output_box box;
  box.open(super_box,mj2_media_header_4cc);
  bool long_version = media_times.need_long_version();
  box.write((kdu_uint32)((long_version)?0x01000000:0x00000000));
#ifdef KDU_LONG64
  if (long_version)
    {
      box.write((kdu_uint32)(media_times.creation_time>>32));
      box.write((kdu_uint32) media_times.creation_time);
      box.write((kdu_uint32)(media_times.modification_time>>32));
      box.write((kdu_uint32) media_times.modification_time);
      box.write(media_times.timescale);
      box.write((kdu_uint32)(media_times.duration>>32));
      box.write((kdu_uint32) media_times.duration);
    }
  else
#endif // KDU_LONG64
    {
      assert(!long_version);
      box.write((kdu_uint32) media_times.creation_time);
      box.write((kdu_uint32) media_times.modification_time);
      box.write(media_times.timescale);
      box.write((kdu_uint32) media_times.duration);
    }

  kdu_uint16 language;
  language = ((kdu_uint16)('e'-0x60)<<5) +
    ((kdu_uint16)('n'-0x60)<<5) + ((kdu_uint16)('g'-0x60)<<5);
  box.write(language);
  box.write((kdu_uint16) 0); // Pre-defined
  box.close();
}

/*****************************************************************************/
/*                    mj_track::read_media_handler_box                       */
/*****************************************************************************/

void
  mj_track::read_media_handler_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_media_handler_4cc);
  kdu_uint32 uint32;
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) != 0))
    { KDU_ERROR(e,30); e <<
        KDU_TXT("Malformed media handler box found in Motion "
        "JPEG2000 data source.  Version number must be 0.");
    }
  box->read(uint32); // Reserved
  if (!box->read(handler_id))
    { KDU_ERROR(e,31); e <<
        KDU_TXT("Malformed meda handler box found in Motion "
        "JPEG2000 data source.  Box terminated prior to the appearance of a "
        "media handler identifier (a 4-character-code).");
    }
  box->close(); // We are not interested in the rest of the box's contents.
}

/*****************************************************************************/
/*                    mj_track::write_media_handler_box                      */
/*****************************************************************************/

void
  mj_track::write_media_handler_box(jp2_output_box *super_box)
{
  jp2_output_box box;
  box.open(super_box,mj2_media_handler_4cc);
  box.write((kdu_uint32) 0); // Version=0; Flags=0
  box.write((kdu_uint32) 0); // Pre-defined
  box.write(handler_id);
  for (int i=0; i < 3; i++)
    box.write((kdu_uint32) 0); // Reserved
  assert(handler_id == mj2_video_handler_4cc);
  box.write((kdu_byte *) "Video",(int) strlen("Video")+1);
  box.close();
}

/*****************************************************************************/
/*                     mj_track::read_data_reference_box                     */
/*****************************************************************************/

void
  mj_track::read_data_reference_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_data_reference_4cc);
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) != 0))
    { KDU_ERROR(e,32); e <<
        KDU_TXT("Malformed data reference box found in Motion "
        "JPEG2000 data source.  Version number must be 0.");
    }

  kdu_uint32 num_entries;
  if ((!box->read(num_entries)) || (num_entries != 1))
    {
      disabled = true;
      box->close();
      KDU_WARNING(w,0); w <<
        KDU_TXT("The current implementation can only handle "
        "Motion JPEG2000 tracks which have single-entry data reference "
        "boxes; multiple-entry data reference boxes are only required if "
        "the MJ2 file contains external data references -- not currently "
        "supported.  Non-conforming tracks will be treated as disabled.");
      return;
    }
    
  jp2_input_box url;
  kdu_uint32 entry_flags;
  if (!(url.open(box) && url.read(entry_flags)))
    { KDU_ERROR(e,33); e <<
        KDU_TXT("Malformed data reference box found in Motion "
        "JPEG2000 data source.  Box appears to terminate prematurely.");
    }
  if ((url.get_box_type() != mj2_url_4cc) || !(entry_flags & 1))
    {
      disabled = true;
      url.close();
      box->close();
      KDU_WARNING(w,1); w <<
        KDU_TXT("The current implementation cannot handle "
        "Motion JPEG2000 tracks which contain external data references.  "
        "Non-conforming tracks will be treated as disabled.");
      return;
    }
  url.close();
  box->close();
}

/*****************************************************************************/
/*                    mj_track::write_data_reference_box                     */
/*****************************************************************************/

void
  mj_track::write_data_reference_box(jp2_output_box *super_box)
{
  jp2_output_box box;
  box.open(super_box,mj2_data_reference_4cc);
  box.write((kdu_uint32) 0); // Version=0; Flags=0
  box.write((kdu_uint32) 1); // One entry

  jp2_output_box url;
  url.open(&box,mj2_url_4cc);
  url.write((kdu_uint32) 0x00000001); // Version=0; Flags=1
  url.close();

  box.close();
}

/*****************************************************************************/
/*                  mj_track::read_sample_description_box                    */
/*****************************************************************************/

void
  mj_track::read_sample_description_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_sample_description_4cc);
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) != 0))
    { KDU_ERROR(e,34); e <<
        KDU_TXT("Malformed sample description box found in Motion "
        "JPEG2000 data source.  Version number must be 0.");
    }

  kdu_uint32 num_entries;
  box->read(num_entries);
  if (num_entries != 1)
    {
      disabled = true;
      box->close();
      KDU_WARNING(w,2); w <<
        KDU_TXT("Current implementation can only handle "
        "Motion JPEG2000 tracks with a single-entry sample description box.  "
        "Other tracks will be treated as disabled.");
      return;
    }

  jp2_input_box entry;
  if (!entry.open(box))
    { KDU_ERROR(e,35); e <<
        KDU_TXT("Malformed sample description box found in Motion "
        "JPEG2000 data source.  Box terminates prematurely.");
    }
  if (entry.get_box_type() == mj2_visual_sample_entry_4cc)
    {
      if (video_track == NULL)
        video_track = new mj_video_track(this);
      video_track->read_sample_entry_box(&entry);
    }
  else
    entry.close();
  box->close();
}

/*****************************************************************************/
/*                  mj_track::write_sample_description_box                   */
/*****************************************************************************/

void
  mj_track::write_sample_description_box(jp2_output_box *super_box)
{
  jp2_output_box box;
  box.open(super_box,mj2_sample_description_4cc);
  box.write((kdu_uint32) 0); // Version=0; Flags=0
  box.write((kdu_uint32) 1); // Only one description per track
  if (video_track != NULL)
    video_track->write_sample_entry_box(&box);
  else
    assert(0);
  box.close();
}

/*****************************************************************************/
/*                          mj_track::load_from_box                          */
/*****************************************************************************/

void
  mj_track::load_from_box(jp2_input_box *trak)
{
  bool found_track_header = false;
  bool found_media_header = false;
  bool found_sample_description = false;
  bool found_time_to_sample = false;
  bool found_sample_to_chunk = false;
  bool found_sample_size = false;
  bool found_chunk_offset = false;

  jp2_input_box sub_1;
  while (sub_1.open(trak))
    if (sub_1.get_box_type() == mj2_track_header_4cc)
      {
        found_track_header = true;
        read_track_header_box(&sub_1);
        if (disabled)
          { // No point in processing this track further
            trak->close(); return;
          }
      }
    else if (sub_1.get_box_type() == mj2_media_4cc)
      {
        jp2_input_box sub_2;
        while (sub_2.open(&sub_1))
          if ((sub_2.get_box_type() == mj2_media_header_4cc) ||
              (sub_2.get_box_type() == mj2_media_header_typo_4cc))
            {
              read_media_header_box(&sub_2);
              found_media_header = true;
            }
          else if (sub_2.get_box_type() == mj2_media_handler_4cc)
            {
              read_media_handler_box(&sub_2);
              if (handler_id != mj2_video_handler_4cc)
                { // We ignore non-video tracks for the time being.
                  sub_1.close(); trak->close(); return;
                }
            }
          else if (sub_2.get_box_type() == mj2_media_information_4cc)
            {
              jp2_input_box sub_3;
              while (sub_3.open(&sub_2))
                if (sub_3.get_box_type() == mj2_video_media_header_4cc)
                  {
                    if (video_track == NULL)
                      video_track = new mj_video_track(this);
                    video_track->read_media_header_box(&sub_3);
                  }
                else if (sub_3.get_box_type() == mj2_data_information_4cc)
                  {
                    jp2_input_box sub_4;
                    while (sub_4.open(&sub_3))
                      if (sub_4.get_box_type() == mj2_data_reference_4cc)
                        {
                          read_data_reference_box(&sub_4);
                          if (disabled)
                            { // No point in processing further
                              sub_3.close(); sub_2.close(); sub_1.close();
                              trak->close(); return;
                            }
                        }
                      else
                        sub_4.close();
                    sub_3.close();
                  }
                else if (sub_3.get_box_type() == mj2_sample_table_4cc)
                  {
                    jp2_input_box sub_4;
                    while (sub_4.open(&sub_3))
                      if (sub_4.get_box_type() == mj2_sample_description_4cc)
                        {
                          found_sample_description = true;
                          read_sample_description_box(&sub_4);
                          if (disabled || (video_track == NULL))
                            { // We ignore non-video tracks for the time being
                              sub_3.close(); sub_2.close(); sub_1.close();
                              trak->close(); return;
                            }
                        }
                      else if (sub_4.get_box_type() == mj2_time_to_sample_4cc)
                        {
                          sample_times.load_from_box(&sub_4);
                          found_time_to_sample = true;
                        }
                      else if (sub_4.get_box_type() == mj2_sample_to_chunk_4cc)
                        {
                          sample_chunks.load_from_box(&sub_4);
                          found_sample_to_chunk = true;
                        }
                      else if (sub_4.get_box_type() == mj2_sample_size_4cc)
                        {
                          sample_sizes.load_from_box(&sub_4);
                          found_sample_size = true;
                        }
                      else if (sub_4.get_box_type() == mj2_chunk_offset_4cc)
                        {
                          chunk_offsets.load_from_box(&sub_4);
                          found_chunk_offset = true;
                        }
                      else if (sub_4.get_box_type() == mj2_chunk_offset64_4cc)
                        {
                          chunk_offsets.load_from_box(&sub_4);
                          found_chunk_offset = true;
                        }
                      else
                        sub_4.close();
                    sub_3.close();
                  }
                else
                  sub_3.close();
              sub_2.close();
            }
          else
            sub_2.close();
        sub_1.close();
      }
    else
      sub_1.close();
  trak->close(); // Discards any extra sub-boxes (e.g., user data boxes).
  if (disabled)
    return;

  if (!found_track_header)
    { KDU_ERROR(e,36); e <<
        KDU_TXT("No track header box found in Motion JPEG2000 track.");
    }
  if (!found_media_header)
    { KDU_ERROR(e,37); e <<
        KDU_TXT("No media header box found in Motion JPEG2000 track.");
    }
  if (!handler_id)
    { KDU_ERROR(e,38); e <<
        KDU_TXT("No media handler box found in Motion JPEG2000 track.");
    }
  if (!found_sample_description)
    { KDU_ERROR(e,39); e <<
        KDU_TXT("No sample description box found in Motion JPEG2000 track.");
    }
  if (!found_time_to_sample)
    { KDU_ERROR(e,40); e <<
        KDU_TXT("No time to sample box found in Motion JPEG2000 track.");
    }
  if (!found_sample_to_chunk)
    { KDU_ERROR(e,41); e <<
        KDU_TXT("No sample to chunk box found in Motion JPEG2000 track.");
    }
  if (!found_sample_size)
    { KDU_ERROR(e,42); e <<
        KDU_TXT("No sample size box found in Motion JPEG2000 track.");
    }
  if (!found_chunk_offset)
    { KDU_ERROR(e,43); e <<
        KDU_TXT("No chunk offset box found in Motion JPEG2000 track.");
    }

  if (video_track != NULL)
    video_track->initialize_read_state();
}

/*****************************************************************************/
/*                           mj_track::save_to_box                           */
/*****************************************************************************/

void
  mj_track::save_to_box(jp2_output_box *super_box)
{
  jp2_output_box trak;
  trak.open(super_box,mj2_track_4cc);
  write_track_header_box(&trak);

  // Write Media box
    {
      jp2_output_box mdia;
      mdia.open(&trak,mj2_media_4cc);
      write_media_header_box(&mdia);
      write_media_handler_box(&mdia);

      // Write Media Information box
        {
          jp2_output_box minf;
          minf.open(&mdia,mj2_media_information_4cc);
          if (video_track != NULL)
            video_track->write_media_header_box(&minf);
          else
            assert(0); // Support non-video track types later.

          // Write Data Information box
            {
              jp2_output_box dinf;
              dinf.open(&minf,mj2_data_information_4cc);
              write_data_reference_box(&dinf);
              dinf.close();
            }

          // Write Sample Table box
            {
              jp2_output_box stbl;
              stbl.open(&minf,mj2_sample_table_4cc);
              write_sample_description_box(&stbl);
              sample_times.save_to_box(&stbl);
              sample_chunks.save_to_box(&stbl);
              sample_sizes.save_to_box(&stbl);
              chunk_offsets.save_to_box(&stbl);
              stbl.close();
            }

          minf.close();
        }

      mdia.close();
    }
  
  trak.close();
}

/* ========================================================================= */
/*                                mj_chunk_buf                               */
/* ========================================================================= */

/*****************************************************************************/
/*                            mj_chunk_buf::store                            */
/*****************************************************************************/

void
  mj_chunk_buf::store(const kdu_byte *buf, int num_bytes)
{
  if (head == NULL)
    head = new mj_chunk_store;
  while (num_bytes > 0)
    {
      if (tail == NULL)
        { tail=head; tail->remaining_elts+=tail->num_elts; tail->num_elts=0; }
      else if (tail->remaining_elts == 0)
        {
          if (tail->next == NULL)
            tail->next = new mj_chunk_store;
          tail = tail->next;
          tail->remaining_elts += tail->num_elts;
          tail->num_elts = 0;
        }
      int xfer = tail->remaining_elts;
      if (xfer > num_bytes)
        xfer = num_bytes;
      memcpy(tail->elts+tail->num_elts,buf,(size_t) xfer);
      num_bytes -= xfer; buf += xfer;
      tail->remaining_elts -= xfer; tail->num_elts += xfer;
    }
}

/*****************************************************************************/
/*                           mj_chunk_buf::write                             */
/*****************************************************************************/

void
  mj_chunk_buf::write(jp2_output_box *box, int num_bytes)
{
  if (current == NULL)
    { current = head; current_bytes_retrieved = 0; }
  while (num_bytes > 0)
    {
      if (current_bytes_retrieved == current->num_elts)
        {
          current = current->next; current_bytes_retrieved = 0;
          assert(current != NULL);
        }
      int xfer = current->num_elts-current_bytes_retrieved;
      if (xfer > num_bytes)
        xfer = num_bytes;
      if (xfer == 0)
        continue;
      assert(xfer > 0);
      if (!box->write(current->elts+current_bytes_retrieved,xfer))
        { KDU_ERROR(e,44); e <<
            KDU_TXT("Unable to write to output device; disk may be full.");
        }
      num_bytes -= xfer;
      current_bytes_retrieved += xfer;
    }
}


/* ========================================================================= */
/*                              mj_video_track                               */
/* ========================================================================= */

/*****************************************************************************/
/*                  mj_video_track::read_media_header_box                    */
/*****************************************************************************/

void
  mj_video_track::read_media_header_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_video_media_header_4cc);
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) != 0) || !(ver_flags & 1))
    {
      box->close();
      KDU_WARNING(w,3); w <<
        KDU_TXT("Malformed video media header box (VMHD) found in "
        "Motion JPEG2000 data source.  Incorrect version number or least "
        "significant flag bit not set.  Ignoring box and using default "
        "graphics mode for the track.");
      return;
    }
  box->read(graphics_mode);
  for (int i=0; i < 3; i++)
    if (!box->read(opcolour[i]))
      { KDU_ERROR(e,45); e <<
          KDU_TXT("Malformed video media header box (VMHD) found "
          "in Motion JPEG2000 data source.  Box terminated prematurely.");
      }
  box->close();
  if ((graphics_mode != MJ2_GRAPHICS_COPY) &&
      (graphics_mode != MJ2_GRAPHICS_BLUE_SCREEN) &&
      (graphics_mode != MJ2_GRAPHICS_ALPHA) &&
      (graphics_mode != MJ2_GRAPHICS_PREMULT_ALPHA) &&
      (graphics_mode != MJ2_GRAPHICS_COMPONENT_ALPHA))
    {
      graphics_mode = 0;
      KDU_WARNING(w,4); w <<
        KDU_TXT("Unrecognized graphics mode encountered in "
        "video media header box (VMHD) within Motion JPEG2000 data source.  "
        "Proceeding with the default \"copy\" mode.");
    }
}

/*****************************************************************************/
/*                  mj_video_track::write_media_header_box                   */
/*****************************************************************************/

void
  mj_video_track::write_media_header_box(jp2_output_box *super_box)
{
  jp2_output_box vmhd;
  vmhd.open(super_box,mj2_video_media_header_4cc);
  vmhd.write((kdu_uint32) 0x00000001); // Version=0; Flags=1
  if ((graphics_mode != MJ2_GRAPHICS_COPY) &&
      (graphics_mode != MJ2_GRAPHICS_BLUE_SCREEN) &&
      (graphics_mode != MJ2_GRAPHICS_ALPHA) &&
      (graphics_mode != MJ2_GRAPHICS_PREMULT_ALPHA) &&
      (graphics_mode != MJ2_GRAPHICS_COMPONENT_ALPHA))
    { KDU_ERROR_DEV(e,46); e <<
        KDU_TXT("Illegal graphics mode cannot be represented in "
        "the video media header box (VMHD) within a Motion JPEG2000 data "
        "source.");
    }
  vmhd.write(graphics_mode);
  for (int i=0; i < 3; i++)
    vmhd.write(opcolour[i]);
  vmhd.close();
}

/*****************************************************************************/
/*                   mj_video_track::read_sample_entry_box                   */
/*****************************************************************************/

void
  mj_video_track::read_sample_entry_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_visual_sample_entry_4cc);
  kdu_uint16 uint16;
  kdu_uint32 uint32;
  kdu_int16 int16;
  kdu_int32 int32;
  kdu_byte reserved[6];
  char compressor_name[33];
  compressor_name[32] = '\0'; // Ensure null termination

  box->read(reserved,6);
  box->read(uint16); // Data reference index
  if (uint16 != 1)
    {
      track->disabled = true;
      box->close();
      KDU_WARNING(w,5); w <<
        KDU_TXT("The current implementation can only handle "
        "Motion JPEG2000 tracks which have single-entry data reference "
        "boxes; multiple-entry data reference boxes are only required if "
        "the MJ2 file contains external data references -- not currently "
        "supported.  Non-conforming tracks will be treated as disabled.");
      return;
    }
  box->read(uint16); // Pre-defined
  box->read(uint16); // Reserved

  box->read(uint32); // Pre-defined
  box->read(uint32); // Pre-defined
  box->read(uint32); // Pre-defined

  box->read(frame_width);
  box->read(frame_height);
  box->read(int32); horizontal_resolution = ((double) int32)/((double)(1<<16));
  box->read(int32); vertical_resolution = ((double) int32) / ((double)(1<<16));

  box->read(uint32); // Reserved
  box->read(uint16); // Pre-defined; should be 1
  box->read((kdu_byte *) compressor_name,32);
  box->read(int16); // Depth
  box->read(int16); // Pre-defined; should be -1

  jp2_input_box sub;
  if ((!sub.open(box)) || (sub.get_box_type() != jp2_header_4cc))
    { KDU_ERROR(e,47); e <<
        KDU_TXT("Malformed video sample entry box in Motion JPEG2000 "
        "data source.  Failed to locate the embedded JP2 header box.");
    }
  if (!header.read(&sub))
    { KDU_ERROR(e,48); e <<
        KDU_TXT("Failed to completely read the JP2 header embedded "
        "in a Motion JPEG2000 data source's visual sample entry box.  It is "
        "likely that you are attempting to open a motion JPEG2000 stream from "
        "a dynamic cache (any object derived from `kdu_cache2').");
    }

  if (sub.open(box) && (sub.get_box_type() == mj2_field_coding_4cc))
    {
      kdu_byte data[2];
      if ((sub.read(data,2) != 2) || ((data[0] != 1) && (data[0] != 2)) ||
          ((data[1] != 0) && (data[1] != 1) && (data[1] != 6)))
        { KDU_ERROR(e,49); e <<
            KDU_TXT("Malformed field coding box found inside a video "
            "sample entry box in the Motion JPEG2000 data source.  The body "
            "of the field coding box should consist of 2 single byte "
            "quantities representing the number of fields (1 or 2) and the "
            "field order, (values 0, 1 or 6).");
        }
      if (data[0] == 1)
        field_order = KDU_FIELDS_NONE;
      else if (data[1] <= 1)
        field_order = KDU_FIELDS_TOP_FIRST;
      else
        field_order = KDU_FIELDS_TOP_SECOND;
      if (sub.get_remaining_bytes() != 0)
        { KDU_ERROR(e,50); e <<
            KDU_TXT("Malformed field coding box found inside a video "
            "sample entry box in the Motion JPEG2000 data source.  The box "
            "appears to be too long.");
        }
    }
  sub.close();
  box->close();
}

/*****************************************************************************/
/*                  mj_video_track::write_sample_entry_box                   */
/*****************************************************************************/

void
  mj_video_track::write_sample_entry_box(jp2_output_box *super_box)
{
  if (header.access_dimensions().get_num_components() == 0)
    { KDU_ERROR(e,51); e <<
        KDU_TXT("Attempting to save a video track to which "
        "a whole frame has not yet been written.  For interlaced frames, "
        "at least two fields must be written to constitute a whole frame.");
    }

  jp2_output_box box;
  kdu_byte reserved[6] = {0,0,0,0,0,0};
  char compressor_name[32];

  box.open(super_box,mj2_visual_sample_entry_4cc);
  box.write(reserved,6);
  box.write((kdu_uint16) 1); // Data reference index (refers to cur file)
  box.write((kdu_uint16) 0); // Pre-defined
  box.write((kdu_uint16) 0); // Reserved

  box.write((kdu_uint32) 0); // Pre-defined
  box.write((kdu_uint32) 0); // Pre-defined
  box.write((kdu_uint32) 0); // Pre-defined

  box.write(frame_width);
  box.write(frame_height);
  jp2_resolution res = header.access_resolution();
  if (res.get_resolution() > 0.0F)
    { // Configure resolution information from JP2 Resolution box.
      vertical_resolution = res.get_resolution()*0.0254; // Convert to dpi
      horizontal_resolution = vertical_resolution * res.get_aspect_ratio();
    }
  if ((fabs(horizontal_resolution) > (float)((1<<16)-1)) ||
      (fabs(vertical_resolution) > (float)((1<<16)-1)))
    { KDU_ERROR(e,52); e <<
        KDU_TXT("Recommended display resolutions must be small "
        "enough to fit inside a 16.16 signed fixed point representation "
        "for recording in the MJ2 Sample Description box.   Typical values "
        "are 72 dpi.");
    }
  box.write((kdu_int32)(0.5F+horizontal_resolution*((float)(1<<16))));
  box.write((kdu_int32)(0.5F+vertical_resolution*((float)(1<<16))));
  box.write((kdu_uint32) 0); // Reserved
  box.write((kdu_uint16) 1); // Pre-defined
  memset(compressor_name,0,32);
  strcpy(compressor_name+1,"Motion JPEG2000");
  compressor_name[0] = (char) strlen(compressor_name+1);
  assert(compressor_name[0] < 32);
  box.write((kdu_byte *) compressor_name,32);
  kdu_int16 depth = (header.access_channels().get_num_colours()==3)?0x18:0x28;
  box.write(depth);
  box.write((kdu_int16)(-1)); // Pre-defined

  {
    jp2_output_box header_box;
    header_box.open(&box,jp2_header_4cc);
    header.write(&header_box); // Write JP2 Header
    header_box.close();
  }
  if (field_order != KDU_FIELDS_NONE)
    { // Write Field Coding box
      jp2_output_box fields;
      fields.open(&box,mj2_field_coding_4cc);
      fields.write((kdu_byte) 2);
      switch (field_order) {
        case KDU_FIELDS_TOP_FIRST:
          fields.write((kdu_byte) 1); break;
        case KDU_FIELDS_TOP_SECOND:
          fields.write((kdu_byte) 6); break;
        default:
          assert(0);
        }
      fields.close();
    }
  box.close();
}

/*****************************************************************************/
/*                   mj_video_track::initialize_read_state                   */
/*****************************************************************************/

void
  mj_video_track::initialize_read_state()
{
  read_state.num_frames = track->sample_times.get_num_samples();
  if (read_state.num_frames > track->sample_sizes.get_num_samples())
    read_state.num_frames = track->sample_sizes.get_num_samples();
  read_state.fields_per_frame = (field_order==KDU_FIELDS_NONE)?1:2;
  read_state.next_frame_idx = read_state.next_field_idx = 0;
  read_state.reset_next_frame_info();
  if (read_state.source == NULL)
    {
      read_state.source = new mj2_video_source;
      read_state.source->state = this;
    }
  num_codestreams = read_state.num_frames * read_state.fields_per_frame;
}


/* ========================================================================= */
/*                           mj_video_read_state                             */
/* ========================================================================= */

/*****************************************************************************/
/*                    mj_video_read_state::find_frame_pos                    */
/*****************************************************************************/

kdu_long
  mj_video_read_state::find_frame_pos(mj_track *track, kdu_uint32 frame_idx)
{
  kdu_uint32 chunk_idx, frame_in_chunk;
  chunk_idx = track->sample_chunks.get_sample_chunk(frame_idx,frame_in_chunk);
  if ((chunk_idx != recent_chunk_idx) ||
      (frame_in_chunk < recent_frame_in_chunk) ||
      (recent_frame_pos < 0))
    { // Have to start from beginning of chunk
      recent_chunk_idx = chunk_idx;
      recent_frame_in_chunk = 0;
      recent_frame_pos = track->chunk_offsets.get_chunk_offset(chunk_idx);
    }

  kdu_uint32 scan_idx =
    frame_idx - frame_in_chunk; // Index of first frame in chunk
  scan_idx += recent_frame_in_chunk; // Index of frame with `recent_frame_pos'
  while (recent_frame_in_chunk < frame_in_chunk)
    {
      recent_frame_pos += track->sample_sizes.get_sample_size(scan_idx);
      recent_frame_in_chunk++;
      scan_idx++;
    }
  return recent_frame_pos;
}


/* ========================================================================= */
/*                           mj_video_write_state                            */
/* ========================================================================= */

/*****************************************************************************/
/*                     mj_video_write_state::flush_chunk                     */
/*****************************************************************************/

void
  mj_video_write_state::flush_chunk(mj_track *track)
{
  if (chunk_frames == 0)
    return;
  assert(field_idx > 0);
  jp2_output_box *mdat = track->movie->get_mdat();
  assert(mdat->exists());
  jp2_family_tgt *tgt = track->movie->get_tgt();
  track->chunk_offsets.append(tgt->get_bytes_written());

  kdu_uint32 i, j, k, field_bytes;
  jp2_output_box jp2c;
  for (i=k=0; i < chunk_frames; i++)
    {
      kdu_long frame_start = tgt->get_bytes_written();
      for (j=0; j < fields_per_frame; j++, k++)
        {
          if (k == field_idx)
            { // Repeat last field
              k--; chunk_buf.goto_mark();
            }
          field_bytes = field_sizes[k];
          chunk_buf.set_mark();
          jp2c.open(mdat,jp2_codestream_4cc);
          jp2c.set_target_size(field_bytes);
          chunk_buf.write(&jp2c,field_bytes);
          jp2c.close();
        }
      kdu_long frame_bytes = tgt->get_bytes_written() - frame_start;
      track->sample_sizes.append((kdu_uint32) frame_bytes);
    }
  field_idx = 0;
  chunk_frames = 0;
  chunk_time = 0;
  chunk_idx++;
  chunk_buf.clear();
}

/* ========================================================================= */
/*                             mj2_video_source                              */
/* ========================================================================= */

/*****************************************************************************/
/*                       mj2_video_source::get_track_idx                     */
/*****************************************************************************/

kdu_uint32
  mj2_video_source::get_track_idx()
{
  return state->track->track_idx;
}

/*****************************************************************************/
/*                  mj2_video_source::get_compositing_order                  */
/*****************************************************************************/

kdu_int16
  mj2_video_source::get_compositing_order()
{
  return state->track->compositing_order;
}

/*****************************************************************************/
/*                    mj2_video_source::get_graphics_mode                    */
/*****************************************************************************/

kdu_int16
  mj2_video_source::get_graphics_mode(kdu_int16 &op_red, kdu_int16 &op_green,
                                      kdu_int16 &op_blue)
{
  op_red = state->track->video_track->opcolour[0];
  op_green = state->track->video_track->opcolour[1];
  op_blue = state->track->video_track->opcolour[2];
  return state->track->video_track->graphics_mode;
}

/*****************************************************************************/
/*              mj2_video_source::get_graphics_mode (no opcode)              */
/*****************************************************************************/

kdu_int16
  mj2_video_source::get_graphics_mode()
{
  return state->track->video_track->graphics_mode;
}

/*****************************************************************************/
/*                       mj2_video_source::get_geometry                      */
/*****************************************************************************/

void
  mj2_video_source::get_geometry(double &presentation_width,
                                 double &presentation_height,
                                 double matrix[], bool for_movie)
{
  presentation_width = state->track->presentation_width;
  presentation_height = state->track->presentation_height;
  double *track_matrix = state->track->transformation_matrix;
  if (for_movie)
    {
      double sum, *movie_matrix=state->track->movie->transformation_matrix;
      int m, n, k;
      for (m=0; m < 3; m++)
        for (n=0; n < 3; n++)
          {
            for (sum=0.0, k=0; k < 3; k++)
              sum += movie_matrix[3*m+k] * track_matrix[3*k+n];
            matrix[3*m+n] = sum;
          }
    }
  else
    {
      int k;
      for (k=0; k < 9; k++)
        matrix[k] = track_matrix[k];
    }
}

/*****************************************************************************/
/*                  mj2_video_source::get_cardinal_geometry                  */
/*****************************************************************************/

void
  mj2_video_source::get_cardinal_geometry(kdu_dims &pre_dims,
                                          bool &transpose, bool &vflip,
                                          bool &hflip, bool for_movie)
{
  double pres_width, pres_height;
  double matrix[9], scratch[9];
  get_geometry(pres_width,pres_height,matrix,for_movie);

  // Find centre of image
  double centre[3] = {0.5*pres_width, 0.5*pres_height, 1.0};
  pre_multiply(centre,matrix,3,1,scratch);

  // Start by adjusting the affine transform so that the horizontal axis of
  // the compressed frame is aligned with one of the cardinal axes, whichever
  // is closest.  The adjustment we make here is rotation by an angle as
  // close to 0 as possible.
  int m, n;
  double pre_factor[9];
  for (m=0; m < 3; m++)
    for (n=0; n < 3; n++)
      pre_factor[3*m+n] = (m==n)?1.0:0.0; // Initialize to the identity
  double tan_theta, cos_theta, sin_theta; // Elements of rotation matrix
  if (fabs(matrix[0]) > fabs(matrix[3]))
    tan_theta = matrix[3]/matrix[0];  // Choose rotation to zero out matrix[3]
  else
    tan_theta = -matrix[0]/matrix[3]; // Choose rotation to zero out matrix[0]
  cos_theta = sqrt(1.0 / (1.0+tan_theta*tan_theta));
  sin_theta = tan_theta * cos_theta;
  pre_factor[0] =  cos_theta;
  pre_factor[1] =  sin_theta;
  pre_factor[3] = -sin_theta;
  pre_factor[4] =  cos_theta;
  pre_multiply(matrix,pre_factor,3,3,scratch);

  // Next, remove any skew from the affine transform so that affine matrix
  // becomes diagonal
  if (fabs(matrix[0]) > fabs(matrix[3]))
    matrix[1] = 0.0;
  else
    matrix[4] = 0.0;

  // Adjust offset so that centre of frame remains unchanged
  double tmp_centre[3] = {0.5*pres_width, 0.5*pres_height, 1.0};
  pre_multiply(tmp_centre,matrix,3,1,scratch);
  matrix[2] += centre[0] - tmp_centre[0];
  matrix[5] += centre[1] - tmp_centre[1];

  // Now we have a cardinal approximation to the original matrix, we can
  // find the `transpose', `vflip' and `hflip' flags and work back to the
  // transformation matrix which would produce the frame immediately prior
  // to application of these flags.
  transpose = hflip = vflip = false;
  for (m=0; m < 3; m++)
    for (n=0; n < 3; n++)
      pre_factor[3*m+n] = (m==n)?1.0:0.0; // Initialize to the identity
  if ((matrix[0] + matrix[1]) < 0.0)
    { // Need horizontal flip
      hflip = true;
      pre_factor[0] = -1.0;
    }
  if ((matrix[3] + matrix[4]) < 0.0)
    { // Need vertical flip
      vflip = true;
      pre_factor[4] = -1.0;
    }
  pre_multiply(matrix,pre_factor,3,3,scratch);

  if (fabs(matrix[3]) > fabs(matrix[0]))
    {
      transpose = true;
      pre_factor[0] = pre_factor[4] = 0.0;
      pre_factor[1] = pre_factor[3] = 1.0;
      pre_multiply(matrix,pre_factor,3,3,scratch);
    }

  // Finally, apply the modified `matrix' to the presentation dimensions
  double pres_min[3] = {0.0,0.0,1.0};
  double pres_lim[3] = {pres_width,pres_height,1.0};
  pre_multiply(pres_min,matrix,3,1,scratch);
  pre_multiply(pres_lim,matrix,3,1,scratch);
  assert((pres_lim[0] > pres_min[0]) && (pres_lim[1] > pres_min[1]));
  pre_dims.pos.x = (int) floor(0.5 + pres_min[0]);
  pre_dims.pos.y = (int) floor(0.5 + pres_min[1]);
  pre_dims.size.x = (int) floor(0.5 + pres_lim[0]-pres_min[0]);
  pre_dims.size.y = (int) floor(0.5 + pres_lim[1]-pres_min[1]);
  if (pre_dims.size.x < 1)
    pre_dims.size.x = 1;
  if (pre_dims.size.y < 1)
    pre_dims.size.y = 1;
}

/*****************************************************************************/
/*                    mj2_video_source::access_dimensions                    */
/*****************************************************************************/

jp2_dimensions
  mj2_video_source::access_dimensions()
{
  return state->header.access_dimensions();
}

/*****************************************************************************/
/*                    mj2_video_source::access_resolution                    */
/*****************************************************************************/

jp2_resolution
  mj2_video_source::access_resolution()
{
  return state->header.access_resolution();
}

/*****************************************************************************/
/*                      mj2_video_source::access_colour                      */
/*****************************************************************************/

jp2_colour
  mj2_video_source::access_colour()
{
  return state->header.access_colour();
}

/*****************************************************************************/
/*                     mj2_video_source::access_palette                      */
/*****************************************************************************/

jp2_palette
  mj2_video_source::access_palette()
{
  return state->header.access_palette();
}

/*****************************************************************************/
/*                     mj2_video_source::access_channels                     */
/*****************************************************************************/

jp2_channels
  mj2_video_source::access_channels()
{
  return state->header.access_channels();
}

/*****************************************************************************/
/*                      mj2_video_source::get_timescale                      */
/*****************************************************************************/

kdu_uint32
  mj2_video_source::get_timescale()
{
  return state->track->media_times.timescale;
}

/*****************************************************************************/
/*                     mj2_video_source::get_field_order                     */
/*****************************************************************************/

kdu_field_order
  mj2_video_source::get_field_order()
{
  return state->field_order;
}

/*****************************************************************************/
/*                     mj2_video_source::set_field_mode                      */
/*****************************************************************************/

void
  mj2_video_source::set_field_mode(int which)
{
  if (state->read_state.fields_per_frame != 2)
    return;
  if (which == 2)
    state->read_state.field_step = 1;
  else
    {
      state->read_state.field_step = 2;
      state->read_state.next_field_idx = (kdu_uint32) which;
    }
}

/*****************************************************************************/
/*                      mj2_video_source::get_num_frames                     */
/*****************************************************************************/

int
  mj2_video_source::get_num_frames()
{
  return (int) state->read_state.num_frames;
}

/*****************************************************************************/
/*                       mj2_video_source::get_duration                      */
/*****************************************************************************/

kdu_long
  mj2_video_source::get_duration()
{
  return (kdu_long) state->track->sample_times.get_duration();
}

/*****************************************************************************/
/*                      mj2_video_source::seek_to_frame                      */
/*****************************************************************************/

bool
  mj2_video_source::seek_to_frame(int frame_idx)
{
  if ((frame_idx < 0) || (frame_idx >= (int) state->read_state.num_frames))
    return false;
  if (state->read_state.field_step == 1)
    state->read_state.next_field_idx = 0;
  if (state->read_state.next_frame_idx != (kdu_uint32) frame_idx)
    {
      state->read_state.next_frame_idx = (kdu_uint32) frame_idx;
      state->read_state.reset_next_frame_info();
    }
  return true;
}

/*****************************************************************************/
/*                      mj2_video_source::time_to_frame                      */
/*****************************************************************************/

int
  mj2_video_source::time_to_frame(kdu_long time_instant)
{
  if (time_instant < 0)
    time_instant = 0;
  int result = (int)
    state->track->sample_times.seek_to_time((kdu_uint32) time_instant);
  return result;
}

/*****************************************************************************/
/*                     mj2_video_source::get_frame_instant                   */
/*****************************************************************************/

kdu_long
  mj2_video_source::get_frame_instant()
{
  if (state->field_open)
    return (kdu_long) state->read_state.open_frame_instant;
  if (state->read_state.next_frame_period == 0)
    { // Regenerate the next frame period and next frame instant values
      state->read_state.next_frame_instant =
        state->track->sample_times.seek_to_sample(
                                           state->read_state.next_frame_idx);
      state->read_state.next_frame_period =
        state->track->sample_times.get_period();
    }
  return (kdu_long) state->read_state.next_frame_instant;
}

/*****************************************************************************/
/*                     mj2_video_source::get_frame_period                    */
/*****************************************************************************/

kdu_long
  mj2_video_source::get_frame_period()
{
  if (state->field_open)
    return (kdu_long) state->read_state.open_frame_period;
  if (state->read_state.next_frame_period == 0)
    { // Regenerate the next frame period and next frame instant values
      if (state->read_state.next_frame_idx >= state->read_state.num_frames)
        return (kdu_long) 0;
      state->read_state.next_frame_instant =
        state->track->sample_times.seek_to_sample(
                                           state->read_state.next_frame_idx);
      state->read_state.next_frame_period =
        state->track->sample_times.get_period();
    }
  return (kdu_long) state->read_state.next_frame_period;
}

/*****************************************************************************/
/*                     mj2_video_source::get_stream_idx                      */
/*****************************************************************************/

int
  mj2_video_source::get_stream_idx(int field_idx)
{
  mj_track *track = state->track;
  mj_movie *movie = track->movie;
  mj_video_read_state *rs = &(state->read_state);
  if (state->first_codestream_id < 0)
    { // Evaluate codestream indices using `mj2_source::count_codestreams'
      mj2_source ifc(movie);
      int count;
      ifc.count_codestreams(count);
    }
  if (state->first_codestream_id < 0)
    return -1; // In case the codestream labeling is still unknown
  if ((rs->next_frame_idx >= rs->num_frames) ||
      (field_idx < 0) || (field_idx >= (int) rs->fields_per_frame))
    return -1;
  int stream_off = rs->next_frame_idx * rs->fields_per_frame + field_idx;
  assert(stream_off < state->num_codestreams);
  return (state->first_codestream_id + stream_off);
}

/*****************************************************************************/
/*                     mj2_video_source::can_open_stream                     */
/*****************************************************************************/

bool
  mj2_video_source::can_open_stream(int field_idx, bool need_main_header)
{
  mj_video_read_state *rs = &(state->read_state);
  if ((rs->next_frame_idx >= rs->num_frames) ||
      (field_idx < 0) || (field_idx >= (int) rs->fields_per_frame))
    return false;

  return true;
}

/*****************************************************************************/
/*                       mj2_video_source::open_stream                       */
/*****************************************************************************/

int
  mj2_video_source::open_stream(int field_idx, jp2_input_box *input_box,
                                kdu_thread_env *env)
{
  mj_track *track = state->track;
  mj_movie *movie = track->movie;
  mj_video_read_state *rs = &(state->read_state);
  if ((rs->next_frame_idx >= rs->num_frames) ||
      (field_idx < 0) || (field_idx >= (int) rs->fields_per_frame))
    return -1;
  if (state->field_open || rs->field_box.exists())
    { KDU_ERROR_DEV(e,53); e <<
        KDU_TXT("You may not call `mj2_video_source::open_stream' "
        "without first closing any image from the same track, opened by "
        "`mj2_video_source::open_image'.");
    }

  if (rs->next_frame_period == 0)
    { // Regenerate the next frame period and next frame instant values
      rs->next_frame_instant =
        track->sample_times.seek_to_sample(rs->next_frame_idx);
      rs->next_frame_period = track->sample_times.get_period();
    }
  if (rs->next_frame_pos < 0)
    rs->next_frame_pos = rs->find_frame_pos(track,rs->next_frame_idx);

  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);
  jp2_locator locator;
  locator.set_file_pos(rs->next_frame_pos);
  if (field_idx == 1)
    { // Skip over field 0
      if (rs->next_field0_len <= 0)
        { // Find first field length by opening its codestream box
          if (!rs->field_box.open(movie->get_src(),locator))
            { KDU_ERROR(e,54); e <<
                KDU_TXT("Motion JPEG2000 data source terminated "
                "prematurely!  Not all of the indicated sample data appears "
                "to be available.");
            }
          if (rs->field_box.get_box_type() != jp2_codestream_4cc)
            { KDU_ERROR(e,55); e <<
                KDU_TXT("Index tables in Motion JPEG2000 data source "
                "appear to be corrupt.  Failed to find a valid JP2 "
                "code-stream box at the location identified in the file.");
            }
          rs->next_field0_len = rs->field_box.get_box_bytes();
          rs->field_box.close();
        }
      locator.set_file_pos(rs->next_frame_pos + rs->next_field0_len);
    }

  if (!input_box->open(movie->get_src(),locator))
    { KDU_ERROR(e,56); e <<
        KDU_TXT("Motion JPEG2000 data source terminated "
        "prematurely!  Not all of the indicated sample data appears "
        "to be available.");
    }
  if (input_box->get_box_type() != jp2_codestream_4cc)
    { KDU_ERROR(e,57); e <<
        KDU_TXT("Index tables in Motion JPEG2000 data source "
        "appear to be corrupt.  Failed to find a valid JP2 code-stream "
        "box at the location identified in the file.");
    }
  if (field_idx == 0)
    rs->next_field0_len = input_box->get_box_bytes();
  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);

  return (int) rs->next_frame_idx;
}

/*****************************************************************************/
/*                       mj2_video_source::open_image                        */
/*****************************************************************************/

int
  mj2_video_source::open_image()
{
  mj_track *track = state->track;
  mj_movie *movie = track->movie;
  mj_video_read_state *rs = &(state->read_state);
  if (rs->next_frame_idx >= rs->num_frames)
    return -1;
  if (state->field_open || rs->field_box.exists())
    { KDU_ERROR_DEV(e,58); e <<
        KDU_TXT("You may not call `mj2_video_source::open_image' "
        "without first closing any open image from the same track.");
    }

  if (rs->next_frame_period == 0)
    { // Regenerate the next frame period and next frame instant values
      rs->next_frame_instant =
        track->sample_times.seek_to_sample(rs->next_frame_idx);
      rs->next_frame_period = track->sample_times.get_period();
    }
  if (rs->next_frame_pos < 0)
    rs->next_frame_pos = rs->find_frame_pos(track,rs->next_frame_idx);

  jp2_locator locator;
  locator.set_file_pos(rs->next_frame_pos);
  if (rs->next_field_idx == 1)
    { // Skip over field 0
      if (rs->next_field0_len <= 0)
        { // Find first field length by opening its codestream box
          if (!rs->field_box.open(movie->get_src(),locator))
            { KDU_ERROR(e,59); e <<
                KDU_TXT("Motion JPEG2000 data source terminated "
                "prematurely!  Not all of the indicated sample data appears "
                "to be available.");
            }
          if (rs->field_box.get_box_type() != jp2_codestream_4cc)
            { KDU_ERROR(e,60); e <<
                KDU_TXT("Index tables in Motion JPEG2000 data source "
                "appear to be corrupt.  Failed to find a valid JP2 "
                "code-stream box at the location identified in the file.");
            }
          rs->next_field0_len = rs->field_box.get_box_bytes();
          rs->field_box.close();
        }
      locator.set_file_pos(rs->next_frame_pos + rs->next_field0_len);
    }

  if (!rs->field_box.open(movie->get_src(),locator))
    { KDU_ERROR(e,61); e <<
        KDU_TXT("Motion JPEG2000 data source terminated "
        "prematurely!  Not all of the indicated sample data appears "
        "to be available.");
    }
  if (rs->field_box.get_box_type() != jp2_codestream_4cc)
    { KDU_ERROR(e,62); e <<
        KDU_TXT("Index tables in Motion JPEG2000 data source "
        "appear to be corrupt.  Failed to find a valid JP2 code-stream "
        "box at the location identified in the file.");
    }
  if (rs->next_field_idx == 0)
    rs->next_field0_len = rs->field_box.get_box_bytes();

  state->field_open = true;
  rs->open_frame_idx = rs->next_frame_idx;
  rs->open_field_idx = rs->next_field_idx;
  rs->open_frame_instant = rs->next_frame_instant;
  rs->open_frame_period = rs->next_frame_period;

  rs->next_field_idx += rs->field_step;
  if (rs->next_field_idx >= rs->fields_per_frame)
    {
      rs->next_frame_idx++;
      rs->next_field_idx -= rs->fields_per_frame;
      rs->reset_next_frame_info();
    }

  return (int) rs->open_frame_idx;
}

/*****************************************************************************/
/*                      mj2_video_source::close_image                        */
/*****************************************************************************/

void
  mj2_video_source::close_image()
{
  if (!state->field_open)
    return;
  state->field_open = false;
  state->read_state.field_box.close();
  state->read_state.open_frame_idx = 0;
  state->read_state.open_field_idx = 0;
  state->read_state.open_frame_instant = 0;
  state->read_state.open_frame_period = 0;
}

/*****************************************************************************/
/*                   mj2_video_source::access_image_box                      */
/*****************************************************************************/

jp2_input_box *
  mj2_video_source::access_image_box()
{
  if (!state->field_open)
    return NULL;
  return &(state->read_state.field_box);
}

/*****************************************************************************/
/*                     mj2_video_source::get_capabilities                    */
/*****************************************************************************/

int
  mj2_video_source::get_capabilities()
{
  return state->read_state.field_box.get_capabilities();
}

/*****************************************************************************/
/*                         mj2_video_source::seek                            */
/*****************************************************************************/

bool
  mj2_video_source::seek(kdu_long offset)
{
  if (!state->field_open)
    { KDU_ERROR_DEV(e,63); e <<
        KDU_TXT("Attempting to invoke `mj2_video_source::seek' "
        "on an `mj2_video_source' object which currently has no active "
        "image.");
    }
  state->read_state.field_box.seek(offset);
  return true;
}

/*****************************************************************************/
/*                         mj2_video_source::get_pos                         */
/*****************************************************************************/

kdu_long
  mj2_video_source::get_pos()
{
  if (!state->field_open)
    { KDU_ERROR_DEV(e,64); e <<
        KDU_TXT("Attempting to invoke `mj2_video_source::get_pos' "
        "on an `mj2_video_source' object which currently has no active "
        "image.");
    }
  return state->read_state.field_box.get_pos();
}

/*****************************************************************************/
/*                           mj2_video_source::read                          */
/*****************************************************************************/

int
  mj2_video_source::read(kdu_byte *buf, int num_bytes)
{
  if (!state->field_open)
    return 0;
  return state->read_state.field_box.read(buf,num_bytes);
}


/* ========================================================================= */
/*                             mj2_video_target                              */
/* ========================================================================= */

/*****************************************************************************/
/*                      mj2_video_target::get_track_idx                      */
/*****************************************************************************/

kdu_uint32
  mj2_video_target::get_track_idx()
{
  return state->track->track_idx;
}

/*****************************************************************************/
/*                  mj2_video_target::set_compositing_order                  */
/*****************************************************************************/

void
  mj2_video_target::set_compositing_order(kdu_int16 layer_idx)
{
  state->track->compositing_order = layer_idx;
}

/*****************************************************************************/
/*                    mj2_video_target::set_graphics_mode                    */
/*****************************************************************************/

void
  mj2_video_target::set_graphics_mode(kdu_int16 graphics_mode,
                      kdu_int16 op_red, kdu_int16 op_green, kdu_int16 op_blue)
{
  if ((graphics_mode != MJ2_GRAPHICS_COPY) &&
      (graphics_mode != MJ2_GRAPHICS_BLUE_SCREEN) &&
      (graphics_mode != MJ2_GRAPHICS_ALPHA) &&
      (graphics_mode != MJ2_GRAPHICS_PREMULT_ALPHA) &&
      (graphics_mode != MJ2_GRAPHICS_COMPONENT_ALPHA))
    { KDU_ERROR_DEV(e,65); e <<
        KDU_TXT("Illegal graphics mode supplied for Motion JPEG2000 "
        "video track.  Legal values are given by the macros "
        "`MJ2_GRAPHICS_COPY', `MJ2_GRAPHICS_BLUE_SCREEN', "
        "`MJ2_GRAPHICS_ALPHA', `MJ2_GRAPHICS_PREMULT_ALPHA' "
        "and `MJ2_GRAPHICS_COMPONENT_ALPHA'.");
    }
  state->graphics_mode = graphics_mode;
  state->opcolour[0] = op_red;
  state->opcolour[1] = op_green;
  state->opcolour[2] = op_blue;
}

/*****************************************************************************/
/*                      mj2_video_target::access_colour                      */
/*****************************************************************************/

jp2_colour
  mj2_video_target::access_colour()
{
  return state->header.access_colour();
}

/*****************************************************************************/
/*                     mj2_video_target::access_palette                      */
/*****************************************************************************/

jp2_palette
  mj2_video_target::access_palette()
{
  return state->header.access_palette();
}

/*****************************************************************************/
/*                    mj2_video_target::access_resolution                    */
/*****************************************************************************/

jp2_resolution
  mj2_video_target::access_resolution()
{
  return state->header.access_resolution();
}

/*****************************************************************************/
/*                     mj2_video_target::access_channels                     */
/*****************************************************************************/

jp2_channels
  mj2_video_target::access_channels()
{
  return state->header.access_channels();
}

/*****************************************************************************/
/*                      mj2_video_target::set_timescale                      */
/*****************************************************************************/

void
  mj2_video_target::set_timescale(kdu_uint32 ticks_per_second)
{
  if (state->write_state.started())
    { KDU_ERROR_DEV(e,66); e <<
        KDU_TXT("Attempting to set the time scale used for a video "
        "track after opening one or more fields/frames for that track.");
    }
  if (ticks_per_second == 0)
    { KDU_ERROR_DEV(e,67); e <<
        KDU_TXT("Zero is not a legitimate value for the time scale "
        "(number of ticks per second) of a video track.");
    }
  state->track->media_times.timescale = ticks_per_second;
}

/*****************************************************************************/
/*                     mj2_video_target::set_field_order                     */
/*****************************************************************************/

void
  mj2_video_target::set_field_order(kdu_field_order order)
{
  if (state->write_state.started())
    { KDU_ERROR_DEV(e,68); e <<
        KDU_TXT("Attempting to modify the field coding for a video "
        "track after opening one or more fields/frames for that track.");
    }
  state->field_order = order;
  state->write_state.fields_per_frame = (order==KDU_FIELDS_NONE)?1:2;
}

/*****************************************************************************/
/*                 mj2_video_target::set_max_frames_per_chunk                */
/*****************************************************************************/

void
  mj2_video_target::set_max_frames_per_chunk(kdu_uint32 max_frames)
{
  state->write_state.set_max_chunk_frames(max_frames);
}

/*****************************************************************************/
/*                     mj2_video_target::set_frame_period                    */
/*****************************************************************************/

void
  mj2_video_target::set_frame_period(kdu_long num_ticks)
{
  assert(num_ticks > 0);
  state->write_state.frame_time = (kdu_uint32) num_ticks;
  assert(num_ticks == (kdu_long)(state->write_state.frame_time));
}

/*****************************************************************************/
/*                        mj2_video_target::open_image                       */
/*****************************************************************************/

void
  mj2_video_target::open_image()
{
  if (state->field_open)
    { KDU_ERROR_DEV(e,69); e <<
        KDU_TXT("You must close the currently open image (field or "
        "frame), before opening a new one.");
    }
  state->field_open = true;
  if (state->write_state.frame_time == 0)
    { KDU_ERROR_DEV(e,70); e <<
        KDU_TXT("You must set the frame period to a non-zero number "
        "of reference clock ticks before attempting to open any video "
        "images.");
    }
  if (!(state->write_state.field_idx % state->write_state.fields_per_frame))
    { // Start of a new frame
      if (((state->write_state.chunk_time + state->write_state.frame_time) >
           state->track->media_times.timescale) ||
          (state->write_state.chunk_frames >=
           state->write_state.max_chunk_frames))
        state->write_state.flush_chunk(state->track);
      state->track->sample_times.append(state->write_state.frame_time);
      state->track->sample_chunks.append_sample(state->write_state.chunk_idx);
      state->write_state.chunk_frames++;
      state->write_state.chunk_time += state->write_state.frame_time;
    }
  state->write_state.field_sizes[state->write_state.field_idx] = 0;
}

/*****************************************************************************/
/*                       mj2_video_target::close_image                       */
/*****************************************************************************/

void
  mj2_video_target::close_image(kdu_codestream codestream)
{
  if (!state->field_open)
    { KDU_ERROR_DEV(e,71); e <<
        KDU_TXT("Attempting to close a video image which has not "
        "yet been opened");
    }
  state->field_open = false;
  state->write_state.field_idx++;
  state->num_codestreams++;

  kdu_coords size1, size2;
  jp2_dimensions dimensions = state->header.access_dimensions();
  siz_params *siz = codestream.access_siz();
  if (!(siz->get(Ssize,0,0,size1.y) && siz->get(Ssize,0,1,size1.x) &&
        siz->get(Sorigin,0,0,size2.y) && siz->get(Sorigin,0,1,size2.x)))
    assert(0);
  size1 -= size2;

  if (dimensions.get_num_components() == 0)
    {
      if (state->write_state.field_idx == 1)
        {
          state->frame_width = (kdu_int16) size1.x;
          state->frame_height = (kdu_int16) size1.y;
        }
      else
        { // Have the second field of an interlaced video
          assert(state->write_state.field_idx == 2);
          state->frame_height += (kdu_int16) size1.y;
        }
      if ((state->field_order == KDU_FIELDS_NONE) ||
          (state->write_state.field_idx == 2))
        { // Can initialize the frame dimensions
          siz_params siz;
          kdu_params *siz_ref = &siz;
          siz_ref->copy_from(codestream.access_siz(),-1,-1);
          siz_ref->set(Sorigin,0,0,0);  siz_ref->set(Sorigin,0,1,0);
          siz_ref->set(Ssize,0,0,(int) state->frame_height);
          siz_ref->set(Ssize,0,1,(int) state->frame_width);
          dimensions.init(&siz);
          state->track->presentation_height =
            ((double) state->frame_height) * ((double)(1<<16));
          state->track->presentation_width =
            ((double) state->frame_width) * ((double)(1<<16));
        }
      else
        return;
    }
  size2.x = state->frame_width;
  size2.y = state->frame_height;
  if (state->field_order == KDU_FIELDS_TOP_FIRST)
    {
      if (state->write_state.field_idx == 1)
        size2.y = (size2.y+1)>>1; // First (top) field can have 1 extra line
      else
        size2.y >>= 1;
    }
  else if (state->field_order == KDU_FIELDS_TOP_SECOND)
    {
      if (state->write_state.field_idx == 2)
        size2.y = (size2.y+1)>>1; // Second (top) field can have 1 extra line
      else
        size2.y >>= 1;
    }
  if (size1 != size2)
    { KDU_ERROR_DEV(e,72); e <<
        KDU_TXT("Codestreams being written as successive fields "
        "or frames of an MJ2 video track must have compatible dimensions.  "
        "For interlaced frames, the field dimensions must be compatible "
        "with the declared field placement order and implied frame "
        "dimensions.  This means that the two fields of a frame can have "
        "different heights only if their heights differ by 1 and then the "
        "higher field must be the one whose first line appears at the top "
        "of the frame.");
    }
}

/*****************************************************************************/
/*                          mj2_video_target::write                          */
/*****************************************************************************/

bool
  mj2_video_target::write(const kdu_byte *buf, int num_bytes)
{
  if (!state->field_open)
    { KDU_ERROR_DEV(e,73); e <<
        KDU_TXT("Attempting to write compressed data to a video track "
        "which has no open video field.");
    }
  state->write_state.chunk_buf.store(buf,num_bytes);
  state->write_state.field_sizes[state->write_state.field_idx] +=
    (kdu_uint32) num_bytes;
  return true;
}


/* ========================================================================= */
/*                                 mj_movie                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                            mj_movie::~mj_movie                            */
/*****************************************************************************/

mj_movie::~mj_movie()
{
  mj_track *tmp;
  while ((tmp=tracks) != NULL)
    {
      tracks=tmp->next;
      delete tmp;
    }
}

/*****************************************************************************/
/*                     mj_movie::read_movie_header_box                      */
/*****************************************************************************/
  
void
  mj_movie::read_movie_header_box(jp2_input_box *box)
{
  assert(box->get_box_type() == mj2_movie_header_4cc);
  int i, m, n;
  kdu_uint32 data[7];
  kdu_int16 int16;
  kdu_int32 int32;
  kdu_uint16 uint16;
  kdu_uint32 uint32;
  kdu_uint32 ver_flags;
  if ((!box->read(ver_flags)) || ((ver_flags >> 24) > 1))
    { KDU_ERROR(e,74); e <<
        KDU_TXT("Malformed movie header box found in Motion "
        "JPEG2000 data source.  Version number must be 1 or 0.");
    }
  bool long_version = ((ver_flags >> 24) == 1);
  for (i=0; i < ((long_version)?7:4); i++)
    box->read(data[i]);
  movie_times.creation_time = data[(long_version)?1:0];
  movie_times.modification_time = data[(long_version)?3:1];
  movie_times.timescale = data[(long_version)?4:2];
  movie_times.duration = data[(long_version)?6:3];
#ifdef KDU_LONG64
  if (long_version)
    {
      movie_times.creation_time += ((kdu_long) data[0])<<32;
      movie_times.modification_time += ((kdu_long) data[2])<<32;
      movie_times.duration += ((kdu_long) data[5])<<32;
    }
#endif // KDU_LONG64
  box->read(int32); playback_rate = ((double) int32) / ((double)(1<<16));
  box->read(int16); playback_volume = ((double) int16) / ((double)(1<<8));
  box->read(uint16); // Reserved
  box->read(uint32); // Reserved
  box->read(uint32); // Reserved
  for (n=0; n < 3; n++)
    for (m=0; m < 3; m++)
      if (m < 2)
        {
          box->read(int32);
          transformation_matrix[3*m+n] = ((double) int32) / ((double)(1<<16));
        }
      else
        {
          box->read(int32);
          transformation_matrix[3*m+n] = ((double) int32) / ((double)(1<<30));
        }
  for (i=0; i < 6; i++)
    box->read(uint32); // Pre-defined
  if (!box->read(uint32)) // Next track index
    { KDU_ERROR(e,75); e <<
        KDU_TXT("Malformed movie header box found in Motion "
        "JPEG2000 data source.  Box terminated prematurely.");
    }
  if (box->get_remaining_bytes() != 0)
    { KDU_ERROR(e,76); e <<
        KDU_TXT("Malformed movie header box found in Motion "
        "JPEG2000 data source.  Box appears to be too long.");
    }
  box->close();
}

/*****************************************************************************/
/*                     mj_movie::write_movie_header_box                      */
/*****************************************************************************/
  
void
  mj_movie::write_movie_header_box(jp2_output_box *super_box)
{
  int i, m, n;
  kdu_uint32 next_track_idx=1;
  mj_track *scan;
  for (scan=tracks; scan != NULL; scan=scan->next)
    if (scan->track_idx >= next_track_idx)
      {
        next_track_idx = scan->track_idx+1;
        if (next_track_idx == 0)
          break;
      }

  jp2_output_box box;
  box.open(super_box,mj2_movie_header_4cc);
  bool long_version = movie_times.need_long_version();
  box.write((kdu_uint32)((long_version)?0x01000000:0x00000000));
#ifdef KDU_LONG64
  if (long_version)
    {
      box.write((kdu_uint32)(movie_times.creation_time>>32));
      box.write((kdu_uint32) movie_times.creation_time);
      box.write((kdu_uint32)(movie_times.modification_time>>32));
      box.write((kdu_uint32) movie_times.modification_time);
      box.write(movie_times.timescale);
      box.write((kdu_uint32)(movie_times.duration>>32));
      box.write((kdu_uint32) movie_times.duration);
    }
  else
#endif // KDU_LONG64
    {
      assert(!long_version);
      box.write((kdu_uint32) movie_times.creation_time);
      box.write((kdu_uint32) movie_times.modification_time);
      box.write(movie_times.timescale);
      box.write((kdu_uint32) movie_times.duration);
    }
  if (fabs(playback_rate) > (double)((1<<16)-1))
    { KDU_ERROR(e,77); e <<
        KDU_TXT("Playback rate too large to be correctly "
        "represented in MJ2 movie header box.");
    }
  box.write((kdu_int32)(playback_rate*((double)(1<<16))+0.5));
  if (fabs(playback_volume) > 127.0)
    { KDU_ERROR(e,78); e <<
        KDU_TXT("Playback volume too large to be correctly "
        "represented in MJ2 movie header box.");
    }
  box.write((kdu_int16)(playback_volume*256.0+0.5));
  box.write((kdu_uint16) 0); // Reserved
  box.write((kdu_uint32) 0); // Reserved
  box.write((kdu_uint32) 0); // Reserved
  for (n=0; n < 3; n++)
    for (m=0; m < 3; m++)
      if (m < 2)
        {
          if (fabs(transformation_matrix[3*m+n]) > (double)((1<<15)-1))
            { KDU_ERROR(e,79); e <<
                KDU_TXT("Non-trivial elements of the video "
                "transformation matrix must be representable as 16.16 signed "
                "fixed point numbers.");
            }
          box.write((kdu_int32)(transformation_matrix[3*m+n]*(1<<16)));
        }
      else
        {
          if (fabs(transformation_matrix[3*m+n]) >= 2.0)
            { KDU_ERROR(e,80); e <<
                KDU_TXT("The last row of the video "
                "transformation matrix must be representable as 2.30 signed "
                "fixed point numbers.");
            }
          box.write((kdu_int32)(transformation_matrix[3*m+n]*(1<<30)));
        }
  for (i=0; i < 6; i++)
    box.write((kdu_uint32) 0); // Pre-defined
  box.write(next_track_idx);
  box.close();
}


/* ========================================================================= */
/*                                mj2_source                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                             mj2_source::open                              */
/*****************************************************************************/

int
  mj2_source::open(jp2_family_src *src, bool return_if_incompatible)
{
  assert(state == NULL);
  state = new mj_movie;
  state->writing = false;
  state->src = src;

  kdu_uint32 signature;
  jp2_input_box box;
  if ((!box.open(src)) || (box.get_box_type() != jp2_signature_4cc) ||
      (!box.read(signature)) || (signature != jp2_signature) ||
      (box.get_remaining_bytes() != 0))
    {
      close();
      if (return_if_incompatible)
        return -1;
      else
        { KDU_ERROR(e,81); e <<
            KDU_TXT("MJP2 data source does not commence with the "
            "JP2 family signature box.");
        }
    }
  box.close();

  if ((!box.open_next()) || (box.get_box_type() != jp2_file_type_4cc))
    {
      close();
      if (return_if_incompatible)
        return -1;
      else
        { KDU_ERROR(e,82); e <<
            KDU_TXT("MJP2 data source does not contain a JP2 file type "
            "box in the correct position -- should be second box in file.");
        }
    }
  else
    {
      kdu_uint32 brand, minor_version, compat=0;
      box.read(brand); box.read(minor_version);
      while (box.read(compat))
        if (compat == mj2_brand)
          break;
      box.close();
      if (compat != mj2_brand)
        {
          close();
          if (return_if_incompatible)
            return -1;
          else
            { KDU_ERROR(e,83);  e <<
                KDU_TXT("The mandatory file type box within the MJP2 "
                "data source, does not include MJP2 in its compatibility "
                "list.");
            }
        }
    }

  bool found_movie_header = false;
  while ((!found_movie_header) && box.open_next())
    {
      if (box.get_box_type() == mj2_movie_4cc)
        {
          jp2_input_box sub;
          while (sub.open(&box))
            if (sub.get_box_type() == mj2_movie_header_4cc)
              {
                state->read_movie_header_box(&sub);
                found_movie_header = true;
              }
            else if (sub.get_box_type() == mj2_track_4cc)
              {
                mj_track *new_track = new mj_track(state);
                new_track->next = state->tracks;
                state->tracks = new_track;
                new_track->load_from_box(&sub);
              }
            else
              sub.close();
        }
      box.close();
    }

  if (!found_movie_header)
    {
      close();
      if (return_if_incompatible)
        return -1;
      else
        { KDU_ERROR(e,84); e <<
            KDU_TXT("Motion JPEG2000 data source does not appear to "
            "contain a movie header (MVHD) box.");
        }
    }

  mj_track *scan;
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    scan->track_times.timescale = state->movie_times.timescale;
  return 1;
}

/*****************************************************************************/
/*                            mj2_source::close                              */
/*****************************************************************************/

void
  mj2_source::close()
{
  if (state == NULL)
    return;
  delete state;
  state = NULL;
}

/*****************************************************************************/
/*                       mj2_source::get_ultimate_src                        */
/*****************************************************************************/

jp2_family_src *
  mj2_source::get_ultimate_src()
{
  if (state == NULL)
    return NULL;
  return state->src;
}

/*****************************************************************************/
/*                        mj2_source::get_movie_dims                         */
/*****************************************************************************/

kdu_dims
  mj2_source::get_movie_dims()
{
  if (state == NULL)
    return kdu_dims();
  if (!(state->movie_dims))
    {
      kdu_coords movie_min, movie_lim;
      mj_track *scan;
      for (scan=state->tracks; scan != NULL; scan=scan->next)
        if (scan->video_track != NULL)
          {
            mj2_video_source *vsrc = scan->video_track->get_source();
            kdu_dims track_dims;
            bool transpose, vflip, hflip;
            vsrc->get_cardinal_geometry(track_dims,transpose,vflip,hflip,true);
            track_dims.to_apparent(transpose,vflip,hflip);
            kdu_coords track_min = track_dims.pos;
            kdu_coords track_lim = track_min + track_dims.size;
            if (movie_min == movie_lim)
              {
                movie_min = track_min;
                movie_lim = track_lim;
              }
            if (track_min.x < movie_min.x)
              movie_min.x = track_min.x;
            if (track_min.y < movie_min.y)
              movie_min.y = track_min.y;
            if (track_lim.x > movie_lim.x)
              movie_lim.x = track_lim.x;
            if (track_lim.y > movie_lim.y)
              movie_lim.y = track_lim.y;
          }
      state->movie_dims.pos = movie_min;
      state->movie_dims.size = movie_lim - movie_min;
    }
  return state->movie_dims;
}

/*****************************************************************************/
/*                        mj2_source::get_next_track                         */
/*****************************************************************************/

kdu_uint32
  mj2_source::get_next_track(kdu_uint32 prev_track_idx)
{
  assert(state != NULL);
  kdu_uint32 result = 0;
  mj_track *scan;
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    if ((scan->track_idx > prev_track_idx) &&
        ((result == 0) || (scan->track_idx < result)))
      result = scan->track_idx;
  return result;
}

/*****************************************************************************/
/*                         mj2_source::get_track_type                        */
/*****************************************************************************/

int
  mj2_source::get_track_type(kdu_uint32 track_idx)
{
  assert(state != NULL);
  mj_track *scan;
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    if (scan->track_idx == track_idx)
      break;
  if (scan == NULL)
    return MJ2_TRACK_NON_EXISTENT;
  else if (scan->video_track != NULL)
    return MJ2_TRACK_IS_VIDEO;
  else
    return MJ2_TRACK_IS_OTHER;
}

/*****************************************************************************/
/*                       mj2_source::access_video_track                      */
/*****************************************************************************/

mj2_video_source *
  mj2_source::access_video_track(kdu_uint32 track_idx)
{
  assert(state != NULL);
  mj_track *scan;
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    if (scan->track_idx == track_idx)
      break;
  if ((scan == NULL) || (scan->video_track == NULL))
    return NULL;
  else
    return scan->video_track->get_source();
}

/*****************************************************************************/
/*                           mj2_source::find_stream                         */
/*****************************************************************************/

bool
  mj2_source::find_stream(int stream_idx, kdu_uint32 &track_idx,
                          int &frame_idx, int &field_idx)
{
  if (state == NULL)
    return true;
  int count;
  bool all_found =
    count_codestreams(count); // Make sure we are as up-to-date as possible

  mj_track *scan;
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    {
      mj_video_track *vt = scan->video_track;
      if (vt == NULL)
        continue;
      if (vt->first_codestream_id < 0)
        return false;
      int stream_off = stream_idx - vt->first_codestream_id;
      if ((stream_off >= 0) && (stream_off < vt->num_codestreams))
        { // Found it
          track_idx = scan->track_idx;
          if (vt->field_order == KDU_FIELDS_NONE)
            { // Progressive video
              frame_idx = stream_off;
              field_idx = 0;
            }
          else
            { // Interlaced video
              frame_idx = stream_off >> 1;
              field_idx = stream_off & 1;
            }
          return true;
        }
    }

  if (!all_found)
    return false;

  track_idx = 0;
  frame_idx = field_idx = 0;
  return true;
}

/*****************************************************************************/
/*                       mj2_source::count_codestreams                       */
/*****************************************************************************/

bool
  mj2_source::count_codestreams(int &count)
{
  count = 0;
  if (state == NULL)
    return true;

  mj_track *scan;
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    {
      mj_video_track *vt = scan->video_track;
      if (vt == NULL)
        continue;
      if (vt->first_codestream_id < 0)
        { // Need to fill in details
          vt->first_codestream_id = count;
          count += vt->num_codestreams;
        }
    }
  return true;
}


/* ========================================================================= */
/*                                mj2_target                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                             mj2_target::open                              */
/*****************************************************************************/

void
  mj2_target::open(jp2_family_tgt *tgt)
{
  assert(state == NULL);
  state = new mj_movie;
  state->writing = true;
  state->tgt = tgt;

  // Write the JP2 signature box
    {
      jp2_output_box signature;
      signature.open(tgt,jp2_signature_4cc);
      signature.write(jp2_signature);
      signature.close();
    }

  // Write the JP2 file-type compatibility box
    {
      jp2_output_box file_type;
      file_type.open(tgt,jp2_file_type_4cc);
      file_type.write(mj2_brand);
      file_type.write((kdu_uint32) 0);
      file_type.write((kdu_uint32) mj2_brand);
      file_type.close();
    }

  // Start an `mdat' box to contain the sample data
  state->mdat.open(tgt,jp2_mdat_4cc);
  state->mdat.write_header_last(); // Works only for seekable files
}

/*****************************************************************************/
/*                            mj2_target::close                              */
/*****************************************************************************/

void
  mj2_target::close()
{
  if (state == NULL)
    return;
  double movie_duration = 0.0;
  mj_track *scan;

  // Determine the whole movie duration
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    {
      scan->flush();
      assert(scan->track_times.timescale > 0);
      double track_duration = (double) scan->track_times.duration;
      track_duration /= (double) scan->track_times.timescale;
      if (track_duration >= movie_duration)
        {
          movie_duration = track_duration;
          state->movie_times = scan->track_times;
        }
    }

  // Now modify the time scale associated with each track to conform to
  // that for the movie -- the track's media time scale can be different,
  // but not the overall track time scale.
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    {
      double track_duration = (double) scan->track_times.duration;
      track_duration /= (double) scan->track_times.timescale;
      track_duration *= (double) state->movie_times.timescale;
      scan->track_times.timescale = state->movie_times.timescale;
      scan->track_times.duration = (kdu_long) track_duration;
    }

  // Close the `mdat' box.
  assert(state->mdat.exists());
  state->mdat.close();

  // Write `moov' box, containing all meta-data
  jp2_output_box moov;
  moov.open(state->tgt,mj2_movie_4cc);
  state->write_movie_header_box(&moov);
  for (scan=state->tracks; scan != NULL; scan=scan->next)
    scan->save_to_box(&moov);
  moov.close();

  // Clean up
  delete state;
  state = NULL;
}

/*****************************************************************************/
/*                        mj2_target::add_video_track                        */
/*****************************************************************************/

mj2_video_target *
  mj2_target::add_video_track()
{
  mj_track *tail = NULL;
  kdu_uint32 track_idx = 1;
  if (state->tracks == NULL)
    state->tracks = tail = new mj_track(state);
  else
    {
      do {
          tail = (tail==NULL)?(state->tracks):(tail->next);
          if (tail->track_idx >= track_idx)
            track_idx = tail->track_idx+1;
        } while (tail->next != NULL);
      tail = tail->next = new mj_track(state);
    }
  tail->track_idx = track_idx;
  tail->handler_id = mj2_video_handler_4cc;
  tail->video_track = new mj_video_track(tail);
  return tail->video_track->get_target();
}

