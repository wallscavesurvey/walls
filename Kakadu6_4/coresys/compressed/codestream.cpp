/*****************************************************************************/
// File: codestream.cpp [scope = CORESYS/COMPRESSED]
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
   Implements a part of the compressed data management machinery, including
code-stream I/O, packet sequencing and the top level interfaces associated with
codestream objects.
******************************************************************************/

#include <math.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "kdu_elementary.h"
#include "kdu_utils.h"
#include "kdu_messaging.h"
#include "kdu_kernels.h"
#include "kdu_compressed.h"
#include "compressed_local.h"

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
   kdu_error _name("E(codestream.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
   kdu_warning _name("W(codestream.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) kdu_error _name("Kakadu Core Error:\n");
#  define KDU_WARNING(_name,_id) kdu_warning _name("Kakadu Core Warning:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


#define KDU_IDENTIFIER ("Kakadu-" KDU_CORE_VERSION)

/* ========================================================================= */
/*                              External Functions                           */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERNAL                    kdu_get_core_version                          */
/*****************************************************************************/

const char *
  kdu_get_core_version()
{
  return KDU_CORE_VERSION;
}

/*****************************************************************************/
/* EXTERN                   kd_create_dwt_description                        */
/*****************************************************************************/

void
  kd_create_dwt_description(int kernel_id, int atk_idx, kdu_params *root,
                            int tnum, bool &reversible, bool &symmetric,
                            bool &symmetric_extension, int &num_steps,
                            kdu_kernel_step_info * &step_info,
                            float * &coefficients)
{
  step_info = NULL;
  coefficients = NULL;
  num_steps = 0;

  if (kernel_id == Ckernels_ATK)
    { // Read kernel parameters from ATK data
      kdu_params *atk = root->access_cluster(ATK_params);
      if ((atk == NULL) ||
          ((atk = atk->access_relation(tnum,-1,atk_idx,true)) == NULL))
        { KDU_ERROR(e,0x22050500); e <<
            KDU_TXT("Unable to find ATK marker segment referenced from "
                    "within an COD/COC or MCC marker segment.");
        }
      int atk_ext;
      if (!(atk->get(Ksymmetric,0,0,symmetric) &&
            atk->get(Kextension,0,0,atk_ext) &&
            atk->get(Kreversible,0,0,reversible)))
        assert(0);
      symmetric_extension = (atk_ext == Kextension_SYM);

      int c, n, s, Ls, num_coeffs=0;
      for (s=0; atk->get(Ksteps,s,0,Ls); s++)
        {
          num_coeffs += Ls;
          if (num_coeffs > 16384)
            { KDU_ERROR(e,0x06110800); e <<
                KDU_TXT("Custom DWT kernel found in ATK marker segment "
                        "contains a ridiculously large number of "
                        "coefficients!");
            }
        }
      num_steps = s;
      step_info = new kdu_kernel_step_info[num_steps];
      coefficients = new float[num_coeffs];
      for (c=s=0; s < num_steps; s++)
        {
          kdu_kernel_step_info *sp = step_info + s;
          if (!(atk->get(Ksteps,s,0,sp->support_length) &&
                atk->get(Ksteps,s,1,sp->support_min) &&
                atk->get(Ksteps,s,2,sp->downshift) &&
                atk->get(Ksteps,s,3,sp->rounding_offset)))
            assert(0);
          for (n=0; n < sp->support_length; n++, c++)
            atk->get(Kcoeffs,c,0,coefficients[c]);
        }
      assert(c == num_coeffs);
    }
  else
    { // Fill in description for Part-1 standard kernels
      symmetric = true;
      symmetric_extension = true;
      if (kernel_id == Ckernels_W5X3)
        {
          reversible = true;
          num_steps = 2;
          step_info = new kdu_kernel_step_info[num_steps];
          coefficients = new float[2*num_steps];
          coefficients[0] = coefficients[1] = -0.5F;
          coefficients[2] = coefficients[3] = 0.25F;
          step_info[0].downshift = 1;
          step_info[1].downshift = 2;
          step_info[0].rounding_offset = 1;
          step_info[1].rounding_offset = 2;
        }
      else if (kernel_id == Ckernels_W9X7)
        {
          reversible = false;
          num_steps = 4;
          step_info = new kdu_kernel_step_info[num_steps];
          coefficients = new float[2*num_steps];
          coefficients[0] = coefficients[1] = (float) -1.586134342;
          coefficients[2] = coefficients[3] = (float) -0.052980118;
          coefficients[4] = coefficients[5] = (float)  0.882911075;
          coefficients[6] = coefficients[7] = (float)  0.443506852;
        }
      else
        assert(0);
      for (int n=0; n < num_steps; n++)
        {
          step_info[n].support_length = 2;
          step_info[n].support_min =
            -((step_info[n].support_length + (n & 1) - 1) >> 1);
        }
    }
}


/* ========================================================================= */
/*                              Internal Functions                           */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        is_power_2                                  */
/*****************************************************************************/

static bool
  is_power_2(int val)
{
  for (; val > 1; val >>= 1)
    if (val & 1)
      return false;
    return (val==1);
}

/* ========================================================================= */
/*                              External Functions                           */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN                      print_marker_code                             */
/*****************************************************************************/

void
  print_marker_code(kdu_uint16 code, kdu_message &out)
{
  const char *name=NULL;

  if (code == KDU_SOC)
    name = "SOC";
  else if (code == KDU_SOT)
    name = "SOT";
  else if (code == KDU_SOD)
    name = "SOD";
  else if (code == KDU_SOP)
    name = "SOP";
  else if (code == KDU_EPH)
    name = "EPH";
  else if (code == KDU_EOC)
    name = "EOC";
  else if (code == KDU_SIZ)
    name = "SIZ";
  else if (code == KDU_CBD)
    name = "CBD";
  else if (code == KDU_MCT)
    name = "MCT";
  else if (code == KDU_MCC)
    name = "MCC";
  else if (code == KDU_MCO)
    name = "MCO";
  else if (code == KDU_COD)
    name = "COD";
  else if (code == KDU_COC)
    name = "COC";
  else if (code == KDU_ADS)
    name = "ADS";
  else if (code == KDU_DFS)
    name = "DFS";
  else if (code == KDU_ATK)
    name = "ATK";
  else if (code == KDU_QCD)
    name = "QCD";
  else if (code == KDU_QCC)
    name = "QCC";
  else if (code == KDU_RGN)
    name = "RGN";
  else if (code == KDU_POC)
    name = "POC";
  else if (code == KDU_CRG)
    name = "CRG";
  else if (code == KDU_COM)
    name = "COM";
  else if (code == KDU_TLM)
    name = "TLM";
  else if (code == KDU_PLM)
    name = "PLM";
  else if (code == KDU_PLT)
    name = "PLT";
  else if (code == KDU_PPM)
    name = "PPM";
  else if (code == KDU_PPT)
    name = "PPT";

  if (name == NULL)
    {
      bool hex_state = out.set_hex_mode(true);
      out << "0x";
      out << code;
      out.set_hex_mode(hex_state);
    }
  else
    out << "<" << name << ">";
}


/* ========================================================================= */
/*                                 kd_input                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                    kd_input::process_unexpected_marker                    */
/*****************************************************************************/

void
  kd_input::process_unexpected_marker(kdu_byte last_byte)
{
  assert(throw_markers);
  kdu_uint16 code = 0xFF00; code += last_byte;
  disable_marker_throwing();
  if (!reject_all)
    {
      bool bona_fide = false;
      if ((code == KDU_SOP) || (code == KDU_SOT))
        {
          kdu_byte byte;
          kdu_uint16 length;
          if (!get(byte))
            exhausted = false;
          else
            {
              length = byte;
              if (!get(byte))
                {
                  exhausted = false;
                  putback((kdu_byte) length);
                }
              else
                {
                  length = (length<<8) + byte;
                  if (code == KDU_SOP)
                    bona_fide = (length == 4);
                  else
                    bona_fide = (length == 10);
                  putback(length);
                }
            }
        }
      if (!bona_fide)
        {
          enable_marker_throwing(reject_all);
          have_FF = (last_byte==0xFF);
          return; // Continue processing as though nothing had happened.
        }
    }
  assert(!exhausted);
  putback(code);
  throw code;
}

/*****************************************************************************/
/*                       kd_input::read (flat array)                         */
/*****************************************************************************/

int
  kd_input::read(kdu_byte *buf, int count)
{
  int xfer_bytes;
  int nbytes = 0;

  if (exhausted)
    return 0;
  while (count > 0)
    {
      if ((xfer_bytes = (int)(first_unwritten-first_unread)) == 0)
        {
          if (!load_buf())
            break;
          xfer_bytes = (int)(first_unwritten-first_unread);
          assert(xfer_bytes > 0);
        }
      xfer_bytes = (xfer_bytes < count)?xfer_bytes:count;
      nbytes += xfer_bytes;
      count -= xfer_bytes;
      if (throw_markers)
        { // Slower loop has to look for marker codes.
          kdu_byte byte;
          while (xfer_bytes--)
            {
              *(buf++) = byte = *(first_unread++);
              if (have_FF && (byte > 0x8F))
                process_unexpected_marker(byte);
              have_FF = (byte==0xFF);
            }
        }
      else
        { // Fastest loop. Probably not beneficial to use `memcpy'.
          while (xfer_bytes--)
            *(buf++) = *(first_unread++);
        }
    }
  return nbytes;
}

/*****************************************************************************/
/*                    kd_input::read (code buffer list)                      */
/*****************************************************************************/

int
  kd_input::read(kd_code_buffer *&cbuf, kdu_byte &cbuf_pos,
                 kd_buf_server *buf_server, int count)
{
  int xfer_bytes;
  int nbytes = 0;

  if (exhausted)
    return 0;

  // Create local working variables in place of `cbuf' and `cbpos' to
  // maximize access efficiency in the loop.  Copy modified results back
  // once we are done.
  kd_code_buffer *cbp = cbuf;
  kdu_byte *cbuf_dest = cbp->buf + cbuf_pos;
  int cbuf_remains = KD_CODE_BUFFER_LEN - cbuf_pos;
  while (count > 0)
    {
      if ((xfer_bytes = (int)(first_unwritten-first_unread)) == 0)
        {
          if (!load_buf())
            break;
          xfer_bytes = (int)(first_unwritten-first_unread);
          assert(xfer_bytes > 0);
        }
      xfer_bytes = (xfer_bytes < count)?xfer_bytes:count;
      nbytes += xfer_bytes;
      count -= xfer_bytes;
      if (!throw_markers)
        while (1)
          {
            if (xfer_bytes <= cbuf_remains)
              {
                memcpy(cbuf_dest,first_unread,(size_t) xfer_bytes);
                cbuf_remains -= xfer_bytes;
                cbuf_dest += xfer_bytes;
                first_unread += xfer_bytes;
                break;
              }
            else
              {
                memcpy(cbuf_dest,first_unread,(size_t) cbuf_remains);
                xfer_bytes -= cbuf_remains;
                cbuf_dest += cbuf_remains;
                first_unread += cbuf_remains;
                cbp = cbp->next = buf_server->get();
                cbuf_remains = KD_CODE_BUFFER_LEN;
                cbuf_dest = cbp->buf;
              }
          }
      else
        { // Slower loop has to look for marker codes.
          kdu_byte byte;
          while (1)
            {
              if (xfer_bytes <= cbuf_remains)
                {
                  cbuf_remains -= xfer_bytes;
                  while (xfer_bytes--)
                    {
                      *(cbuf_dest++) = byte = *(first_unread++);
                      if (have_FF && (byte > 0x8F))
                        process_unexpected_marker(byte);
                      have_FF = (byte==0xFF);
                    }
                  break;
                }
              else
                {
                  xfer_bytes -= cbuf_remains;
                  for (; cbuf_remains > 0; cbuf_remains--)
                    {
                      *(cbuf_dest++) = byte = *(first_unread++);
                      if (have_FF && (byte > 0x8F))
                        process_unexpected_marker(byte);
                      have_FF = (byte==0xFF);
                    }
                  cbp = cbp->next = buf_server->get();
                  cbuf_remains = KD_CODE_BUFFER_LEN;
                  cbuf_dest = cbp->buf;
                }
            }
        }
    }

  cbuf = cbp;
  cbuf_pos = (kdu_byte)(KD_CODE_BUFFER_LEN-cbuf_remains);
  return nbytes;
}

/*****************************************************************************/
/*                             kd_input::ignore                              */
/*****************************************************************************/

kdu_long
  kd_input::ignore(kdu_long count)
{
  int xfer_bytes;
  kdu_long nbytes = 0;

  if (exhausted)
    return 0;

  while (count > 0)
    {
      if ((xfer_bytes = (int)(first_unwritten-first_unread)) == 0)
        {
          if (!load_buf())
            break;
          xfer_bytes = (int)(first_unwritten-first_unread);
          assert(xfer_bytes > 0);
        }
      if (count < (kdu_long) xfer_bytes)
        xfer_bytes = (int) count;
      nbytes += xfer_bytes;
      count -= xfer_bytes;
      if (throw_markers)
        { // Slower loop has to look for marker codes.
          kdu_byte byte;
          while (xfer_bytes--)
            {
              byte = *(first_unread++);
              if (have_FF && (byte > 0x8F))
                process_unexpected_marker(byte);
              have_FF = (byte==0xFF);
            }
        }
      else
        first_unread += xfer_bytes;
    }
  return nbytes;
}

/* ========================================================================= */
/*                            kd_compressed_input                            */
/* ========================================================================= */

/*****************************************************************************/
/*                  kd_compressed_input::kd_compressed_input                 */
/*****************************************************************************/

kd_compressed_input::kd_compressed_input(kdu_compressed_source *source)
{
  this->source = source;
  cur_offset = 0;
  max_bytes_allowed = (KDU_LONG_MAX>>1);
  max_address_read = 0;
  suspend_ptr = alt_first_unwritten = NULL;
  suspended_bytes = last_loaded_bytes = 0;
  special_scope = false;
  if (source->get_capabilities() & KDU_SOURCE_CAP_IN_MEMORY)
    {
      kdu_long mem_pos;
      kdu_byte *mem, *mem_lim;
      if ((mem = source->access_memory(mem_pos,mem_lim)) == NULL)
        return;
      fully_buffered = true;
      first_unread = mem;
      first_unwritten = mem_lim;
      assert(mem_pos == 0);
      cur_offset = 0;
      last_loaded_bytes = first_unwritten - first_unread;
    }
}

/*****************************************************************************/
/*                     kd_compressed_input::set_max_bytes                    */
/*****************************************************************************/

void
  kd_compressed_input::set_max_bytes(kdu_long limit)
{
  if (special_scope)
    return;
  if (limit >= max_bytes_allowed)
    return; // Only allow tighter restrictions
  if (limit > (KDU_LONG_MAX>>1))
    limit = KDU_LONG_MAX>>1;
  max_bytes_allowed = limit;
  if (suspend_ptr != NULL)
    return; // Don't actually apply any limits until suspension is over.
  limit = max_bytes_allowed + suspended_bytes - cur_offset;
  if (limit < last_loaded_bytes)
    { // We have already loaded the buffer with too many bytes.
      if (alt_first_unwritten == NULL)
        alt_first_unwritten = first_unwritten;
      first_unwritten -= (last_loaded_bytes - limit);
      last_loaded_bytes = limit;
      if (first_unwritten < first_unread)
        { // We have already read past the end.
          exhausted = true;
          first_unwritten = first_unread;
          alt_first_unwritten = NULL;
        }
    }
}

/*****************************************************************************/
/*                       kd_compressed_input::load_buf                       */
/*****************************************************************************/

bool
  kd_compressed_input::load_buf()
{
  assert(first_unwritten == first_unread);
  if (fully_buffered)
    return (exhausted = true);

  first_unread = buffer + KD_IBUF_PUTBACK;
  cur_offset += (first_unwritten - first_unread);
  if (special_scope)
    {
      last_loaded_bytes = source->read(first_unread,KD_IBUF_SIZE);
      first_unwritten = first_unread + last_loaded_bytes;
      if (last_loaded_bytes == 0)
        exhausted = true;
    }
  else if (suspend_ptr == NULL)
    { // Not in suspended state; byte limits may apply.
      alt_first_unwritten = NULL;
      last_loaded_bytes = max_bytes_allowed + suspended_bytes - cur_offset;
      first_unwritten = first_unread;
      if (last_loaded_bytes <= 0)
        { exhausted = true; last_loaded_bytes=0; return false; }
      if (last_loaded_bytes > KD_IBUF_SIZE)
        last_loaded_bytes = KD_IBUF_SIZE;
      last_loaded_bytes = source->read(first_unread,(int) last_loaded_bytes);
      first_unwritten += last_loaded_bytes;
      if (last_loaded_bytes == 0)
        exhausted = true;
    }
  else
    { // In suspended state; no byte limits apply, but keep track of things
      assert(alt_first_unwritten == NULL);
      assert((suspend_ptr>=first_unread) && (suspend_ptr<=first_unwritten));
      suspended_bytes += first_unwritten - suspend_ptr;
      first_unwritten = first_unread;
      suspend_ptr = first_unread;
      last_loaded_bytes = source->read(first_unread,KD_IBUF_SIZE);
      first_unwritten += last_loaded_bytes;
      if (last_loaded_bytes == 0)
        exhausted = true;
    }
  return !exhausted;
}

/*****************************************************************************/
/*                    kd_compressed_input::get_bytes_read                    */
/*****************************************************************************/

kdu_long
  kd_compressed_input::get_bytes_read()
{
  if (special_scope)
    return 0;
  kdu_long last_read =
    cur_offset + last_loaded_bytes - (first_unwritten-first_unread) - 1;
  if (last_read > max_address_read)
    max_address_read = last_read;
  return max_address_read+1;
}

/*****************************************************************************/
/*                 kd_compressed_input::get_suspended_bytes                  */
/*****************************************************************************/

kdu_long
  kd_compressed_input::get_suspended_bytes()
{
  if (special_scope)
    return 0;
  if (suspend_ptr != NULL)
    {
      assert((suspend_ptr <= first_unread) &&
             (suspend_ptr >= (first_unwritten-last_loaded_bytes)));
      suspended_bytes += first_unread - suspend_ptr;
      suspend_ptr = first_unread;
    }
  return suspended_bytes;
}

/*****************************************************************************/
/*                  kd_compressed_input::set_tileheader_scope                */
/*****************************************************************************/

bool
  kd_compressed_input::set_tileheader_scope(int tnum, int num_tiles)
{
  special_scope = true;
  first_unread = buffer + KD_IBUF_PUTBACK;
  if (!source->set_tileheader_scope(tnum,num_tiles))
    {
      if ((source->get_capabilities() & KDU_SOURCE_CAP_CACHED) == 0)
        {
          KDU_ERROR_DEV(e,0); e <<
            KDU_TXT("Attempting to load cached tile header data from "
            "a compressed data source which does not appear to "
            "support caching.  It is possible that the source has "
            "been incorrectly implemented.");
        }
      first_unwritten = first_unread;
      exhausted = true;
      return false;
    }
  int xfer_bytes = source->read(first_unread,KD_IBUF_SIZE);
  first_unwritten = first_unread + xfer_bytes;
  assert(xfer_bytes >= 0);
  exhausted = (xfer_bytes == 0);
  return true;
}

/*****************************************************************************/
/*                         kd_compressed_input::seek                         */
/*****************************************************************************/

void
  kd_compressed_input::seek(kdu_long address)
{
  assert(!throw_markers); // Marker throwing fragile; no seeking permitted

  if (address < 0)
    { // Only for cacheable source.
      special_scope = true;
      first_unread = first_unwritten = buffer + KD_IBUF_PUTBACK;
      if (!source->set_precinct_scope(-(1+address)))
        { KDU_ERROR_DEV(e,1); e <<
            KDU_TXT("Attempting to load cached precinct packets from "
            "a compressed data source which does not appear to "
            "support caching.  It is possible that the source has "
            "been incorrectly implemented.");
        }
      int xfer_bytes = source->read(first_unread,KD_IBUF_SIZE);
      first_unwritten = first_unread + xfer_bytes;
      exhausted = (xfer_bytes == 0);
      return;
    }

  kdu_long cur_addr =
    cur_offset + last_loaded_bytes - (first_unwritten - first_unread);
  kdu_long cur_size = last_loaded_bytes;
  if (address == cur_addr)
    return; // Already at exactly the right location.

  // Before changing context, see if `max_address_read' needs updating.
  kdu_long last_read = cur_addr-1;
  if (last_read > max_address_read)
    max_address_read = last_read;

  // Now for the seeking code.
  alt_first_unwritten = NULL; // Just in case
  if (address >= max_bytes_allowed)
    {
      exhausted = true;
      if (!fully_buffered)
        {
          cur_offset = max_bytes_allowed;
          first_unwritten = buffer + KD_IBUF_PUTBACK;
        }
      first_unread = first_unwritten;
      return;
    }
  exhausted = false;
  if ((address >= cur_offset) && (address < (cur_offset+cur_size)))
    { // No need to read any new data into the internal buffer.
      first_unread += address - cur_addr;
      return;
    }
  assert(!fully_buffered);
  if (suspend_ptr != NULL)
    {
      kdu_long extra_sus = (first_unread - suspend_ptr) + (address-cur_addr);
      suspend_ptr = buffer + KD_IBUF_PUTBACK;
      if (extra_sus > 0)
        suspended_bytes += extra_sus;
    }
  cur_offset = address;
  first_unread = first_unwritten = buffer + KD_IBUF_PUTBACK;
  if (!source->seek(address))
    { KDU_ERROR_DEV(e,2); e <<
        KDU_TXT("Attempting to seek inside a compressed data source "
        "which does not appear to support seeking.  The source may "
        "have been implemented incorrectly.");
    }
  last_loaded_bytes = max_bytes_allowed - cur_offset;
  if (last_loaded_bytes > KD_IBUF_SIZE)
    last_loaded_bytes = KD_IBUF_SIZE;
  last_loaded_bytes = source->read(first_unread,(int) last_loaded_bytes);
  first_unwritten = first_unread + last_loaded_bytes;
  if (last_loaded_bytes == 0)
    exhausted = true;
}

/*****************************************************************************/
/*                        kd_compressed_input::ignore                        */
/*****************************************************************************/

kdu_long
  kd_compressed_input::ignore(kdu_long count)
{
  if (!(source->get_capabilities() & KDU_SOURCE_CAP_SEEKABLE))
    return kd_input::ignore(count);
  kdu_long old_addr, new_addr;
  old_addr = cur_offset + last_loaded_bytes - (first_unwritten-first_unread);
  new_addr = old_addr + count;
  seek(new_addr);
  new_addr = cur_offset + last_loaded_bytes - (first_unwritten-first_unread);
  return new_addr-old_addr;
}


/* ========================================================================= */
/*                               kd_pph_input                                */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_pph_input::~kd_pph_input                        */
/*****************************************************************************/

kd_pph_input::~kd_pph_input()
{
  read_buf = NULL; // Just in case.
  while ((write_buf=first_buf) != NULL)
    {
      first_buf = write_buf->next;
      buf_server->release(write_buf);
    }
}

/*****************************************************************************/
/*                         kd_pph_input::add_bytes                           */
/*****************************************************************************/

void
  kd_pph_input::add_bytes(kdu_byte *data, int num_bytes)
{
  while (num_bytes > 0)
    {
      if (write_buf == NULL)
        {
          write_buf = read_buf = first_buf = buf_server->get();
          write_pos = read_pos = 0;
        }
      else if (write_pos == KD_CODE_BUFFER_LEN)
        {
          write_buf = write_buf->next = buf_server->get();
          write_pos = 0;
        }
      int xfer_bytes = KD_CODE_BUFFER_LEN-write_pos;
      if (xfer_bytes > num_bytes)
        xfer_bytes = num_bytes;
      num_bytes -= xfer_bytes;
      while (xfer_bytes--)
        write_buf->buf[write_pos++] = *(data++);
    }
}

/*****************************************************************************/
/*                          kd_pph_input::load_buf                           */
/*****************************************************************************/

bool
  kd_pph_input::load_buf()
{
  if (read_buf == NULL)
    { exhausted = true; return false; }

  first_unread = first_unwritten = buffer + KD_IBUF_PUTBACK;
  int xfer_bytes = KD_IBUF_SIZE - KD_IBUF_PUTBACK;
  int buf_bytes;
  while (xfer_bytes > 0)
    {
      if (read_pos == KD_CODE_BUFFER_LEN)
        {
          if (read_buf != write_buf)
            {
              read_buf = read_buf->next;
              read_pos = 0;
              assert(read_buf != NULL);
            }
        }
      buf_bytes = (read_buf==write_buf)?write_pos:KD_CODE_BUFFER_LEN;
      buf_bytes -= read_pos;
      assert(buf_bytes >= 0);
      if (buf_bytes == 0)
        break;
      if (buf_bytes > xfer_bytes)
        buf_bytes = xfer_bytes;
      xfer_bytes -= buf_bytes;
      while (buf_bytes--)
        *(first_unwritten++) = read_buf->buf[read_pos++];
    }
  if (first_unread == first_unwritten)
    { exhausted = true; return false; }
  return true;
}

/* ========================================================================= */
/*                                 kd_marker                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_marker::kd_marker (copy)                        */
/*****************************************************************************/

kd_marker::kd_marker(const kd_marker &orig)
{
  source = NULL;
  codestream = orig.codestream;
  code = orig.code;
  max_length = length = orig.length;
  buf = NULL;
  if (max_length > 0)
    {
      buf = new kdu_byte[max_length];
      memcpy(buf,orig.buf,(size_t) length);
    }
  encountered_skip_code = false;
}

/*****************************************************************************/
/*                              kd_marker::read                              */
/*****************************************************************************/

bool
  kd_marker::read(bool exclude_stuff_bytes, bool skip_to_marker)
{
  assert(source != NULL);
  kdu_byte byte = 0;
  bool valid_code, force_skip;

  source->disable_marker_throwing();
  do {
      force_skip = false;
      if ((byte != 0xFF) && !source->get(byte))
        { code = 0; length = 0; return false; }
      if (skip_to_marker)
        while (byte != 0xFF)
          if (!source->get(byte))
            { code = 0; length = 0; return false; }
      if (byte != 0xFF)
        {
          source->putback(byte);
          code = 0; length = 0; return false;
        }
      if (!source->get(byte))
        { code = 0; length = 0; return false; }
      code = 0xFF00 + byte;
      valid_code = true;
      if (exclude_stuff_bytes)
        valid_code = (byte > 0x8F);
      if ((code == KDU_SOP) || (code == KDU_SOT))
        { // Want to be really sure that this is not a corrupt marker code.
          assert(valid_code);
          if (!source->get(byte))
            { code = 0; length = 0; return false; }
          length = byte; length <<= 8;
          if (!source->get(byte))
            { code = 0; length = 0; return false; }
          length += byte;
          if ((code == KDU_SOP) && (length != 4))
            {
              valid_code = false;
              source->putback((kdu_uint16) length);
              KDU_WARNING(w,0); w <<
                KDU_TXT("Skipping over corrupt SOP marker code!");
            }
          else if ((code == KDU_SOT) && (length != 10))
            {
              valid_code = false;
              source->putback((kdu_uint16) length);
              KDU_WARNING(w,1); w <<
                KDU_TXT("Skipping over corrupt SOT marker code!");
            }
          byte = (kdu_byte) code;
        }
      else if (code == KDU_EOC)
        { /* If the input source is about to end, we will simply skip over
             the EOC marker so that true EOC termination can be treated in
             the same way as premature termination of the code-stream.
             Otherwise, the EOC marker would appear to have been generated
             by some type of code-stream corruption.  In this case, we will
             treat it as an invalid marker code. */
          if (source->get(byte))
            {
              if (!codestream->resilient)
                {
                  source->putback(byte);
                  source->terminate_prematurely();
                  length = 0; code = 0;
                  return false;
                }

              valid_code = false;
              source->putback(byte);
              byte = (kdu_byte) code;
              KDU_WARNING(w,2); w <<
                KDU_TXT("Disregarding non-terminal EOC marker.");
            }
          else
            { length = 0; code = 0; return false; }
        }
      else if (valid_code && (code >= 0xFF30) && (code <= 0xFF3F))
        { // These codes are all to be skipped.
          valid_code = false;
          byte = 0;
          if (!encountered_skip_code)
            { KDU_WARNING_DEV(w,3); w <<
                KDU_TXT("Encountered one or more marker codes in "
                "the range 0xFF30 to 0xFF3F.  These are to be ignored.");
            }
          encountered_skip_code = true;
          force_skip = true;
        }
    } while ((skip_to_marker || force_skip) && (!valid_code));

  if (!valid_code)
    {
      source->putback(code);
      code = 0; length = 0;
      return false;
    }

  // Now we are committed to processing the marker, returning false only if
  // the source becomes exhausted.

  if ((code == KDU_SOC) || (code == KDU_SOD) ||
      (code == KDU_EOC) || (code == KDU_EPH))
    return true; // Delimiter found. There is no marker segment.
  if ((code != KDU_SOT) && (code != KDU_SOP) && (code != KDU_SIZ) &&
      (code != KDU_CBD) && (code != KDU_MCT) &&
      (code != KDU_MCC) && (code != KDU_MCO) &&
      (code != KDU_COD) && (code != KDU_COC) &&
      (code != KDU_ADS) && (code != KDU_DFS) && (code != KDU_ATK) &&
      (code != KDU_QCD) && (code != KDU_QCC) &&
      (code != KDU_RGN) && (code != KDU_POC) &&
      (code != KDU_CRG) && (code != KDU_COM) &&
      (code != KDU_TLM) && (code != KDU_PLM) && (code != KDU_PLT) &&
      (code != KDU_PPM) && (code != KDU_PPT))
    {
      KDU_WARNING_DEV(w,4); w <<
        KDU_TXT("Unrecognized/unimplemented marker code, ");
      print_current_code(w);
      w << KDU_TXT(", found in code-stream.");
    }
  if ((code != KDU_SOP) && (code != KDU_SOT))
    { // Otherwise, we already have the length.
      if (!source->get(byte))
        { code = 0; return false; }
      length = byte; length <<= 8;
      if (!source->get(byte))
        { code = 0; length = 0; return false; }
      length += byte;
    }
  length -= 2;
  if (length < 0)
    { code = 0; length = 0; return false; }
  if (length > max_length)
    {
      max_length = 2*length; // Don't want to have to re-allocate too often
      delete[] buf;
      buf = NULL;
      buf = new kdu_byte[max_length];
    }
  if (source->read(buf,length) < length)
    {
      code = 0; length = 0;
      return false;
    }
  return true;
}


/* ========================================================================= */
/*                                kd_pp_markers                              */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_pp_markers::~kd_pp_markers                      */
/*****************************************************************************/

kd_pp_markers::~kd_pp_markers()
{
  kd_pp_marker_list *tmp;
  while ((tmp=list) != NULL)
    {
      list = tmp->next;
      delete tmp;
    }
}

/*****************************************************************************/
/*                          kd_pp_markers::add_marker                        */
/*****************************************************************************/

void
  kd_pp_markers::add_marker(kd_marker &copy_source)
{
  if (copy_source.get_length() < 1)
    { KDU_ERROR(e,3); e <<
        KDU_TXT("PPM/PPT marker segments must be at least 3 bytes long!");
    }
  kd_pp_marker_list *elt =
    new kd_pp_marker_list(copy_source);
  kdu_byte *data = elt->get_bytes();
  elt->znum = data[0];
  elt->bytes_read = 1;
  if (elt->get_code() == KDU_PPM)
    {
      assert((list == NULL) || is_ppm);
      is_ppm = true;
    }
  else
    {
      assert(elt->get_code() == KDU_PPT);
      assert((list == NULL) || !is_ppm);
      is_ppm = false;
    }

  kd_pp_marker_list *scan, *prev;

  for (prev=NULL, scan=list; scan != NULL; prev=scan, scan=scan->next)
    if (scan->znum > elt->znum)
      break;
  elt->next = scan;
  if (prev == NULL)
    list = elt;
  else
    {
      prev->next = elt;
      if (prev->znum == elt->znum)
        { KDU_ERROR(e,4); e <<
            KDU_TXT("Found multiple PPM/PPT marker segments with "
            "identical Zppt/Zppm indices within the same header scope "
            "(main or tile-part header)!");
        }
    }
}

/*****************************************************************************/
/*                        kd_pp_markers::transfer_tpart                      */
/*****************************************************************************/

void
  kd_pp_markers::transfer_tpart(kd_pph_input *pph_input)
{
  int xfer_bytes = INT_MAX;

  if (is_ppm)
    {
      while ((list != NULL) && (list->bytes_read == list->get_length()))
        advance_list();
      if (list == NULL)
        { KDU_ERROR(e,5); e <<
            KDU_TXT("Insufficient packet header data in PPM marker "
            "segments!");
        }
      if ((list->get_length()-list->bytes_read) < 4)
        { KDU_ERROR(e,6); e <<
            KDU_TXT("Encountered malformed PPM marker: 4-byte Nppm "
            "values may not straddle multiple PPM marker segments.  "
            "Problem is most likely due to a previously incorrect "
            "Nppm value.");
        }
      kdu_byte *data = list->get_bytes();
      xfer_bytes = data[list->bytes_read++];
      xfer_bytes = (xfer_bytes << 8) + data[list->bytes_read++];
      xfer_bytes = (xfer_bytes << 8) + data[list->bytes_read++];
      xfer_bytes = (xfer_bytes << 8) + data[list->bytes_read++];
    }
  while ((list != NULL) && (xfer_bytes > 0))
    {
      int elt_bytes = list->get_length()-list->bytes_read;
      if (elt_bytes > xfer_bytes)
        elt_bytes = xfer_bytes;
      pph_input->add_bytes(list->get_bytes()+list->bytes_read,elt_bytes);
      xfer_bytes -= elt_bytes;
      list->bytes_read += elt_bytes;
      if (list->bytes_read == list->get_length())
        advance_list();
    }
  if (is_ppm && (xfer_bytes > 0))
    { KDU_ERROR(e,7); e <<
        KDU_TXT("Insufficient packet header data in PPM marker "
        "segments, or else Nppm values must be incorrect!");
    }
}

/*****************************************************************************/
/*                         kd_pp_markers::ignore_tpart                       */
/*****************************************************************************/

void
  kd_pp_markers::ignore_tpart()
{
  int xfer_bytes = INT_MAX;

  if (is_ppm)
    {
      kdu_byte byte;

      int len_bytes = 0;
      while (len_bytes < 4)
        { // Need to read 4 bytes of length information.
          if (list == NULL)
            { KDU_ERROR(e,8); e <<
                KDU_TXT("Insufficient packet header data in PPM "
                "marker segments!");
            }
          if (list->bytes_read == list->get_length())
            {
              advance_list();
              continue;
            }
          byte = (list->get_bytes())[list->bytes_read++];
          xfer_bytes = (xfer_bytes << 8) + byte;
          len_bytes++;
        }
    }
  while ((list != NULL) && (xfer_bytes > 0))
    {
      int elt_bytes = list->get_length()-list->bytes_read;
      if (elt_bytes > xfer_bytes)
        elt_bytes = xfer_bytes;
      xfer_bytes -= elt_bytes;
      list->bytes_read += elt_bytes;
      if (list->bytes_read == list->get_length())
        advance_list();
    }
  if (is_ppm && (xfer_bytes > 0))
    { KDU_ERROR(e,9); e <<
        KDU_TXT("Insufficient packet header data in PPM marker "
        "segments, or else Nppm values must be incorrect!");
    }
}

/*****************************************************************************/
/*                         kd_pp_markers::advance_list                       */
/*****************************************************************************/

void
  kd_pp_markers::advance_list()
{
  assert((list != NULL) && (list->bytes_read == list->get_length()));
  kd_pp_marker_list *tmp = list;
  list = tmp->next;
  delete tmp;
}


/* ========================================================================= */
/*                             kd_tlm_generator                              */
/* ========================================================================= */

/*****************************************************************************/
/*                          kd_tlm_generator::init                           */
/*****************************************************************************/

bool
  kd_tlm_generator::init(int num_tiles, int max_tparts,
                         int tnum_bytes, int tplen_bytes)
{
  clear();
  if ((tnum_bytes < 0) || (tnum_bytes > 2) ||
      ((tplen_bytes != 2) && (tplen_bytes != 4)))
    return false;
  if (tnum_bytes == 0)
    { 
      if (max_tparts != 1)
        return false;
    }
  else if (tnum_bytes == 1)
    {
      if (num_tiles > 256)
        return false;
    }
  this->num_tiles = num_tiles;
  if (max_tparts < 0)
    max_tparts = 1;
  else if (max_tparts > 255)
    max_tparts = 255;
  this->max_tparts = max_tparts;
  this->tnum_prec = tnum_bytes;
  this->tplen_prec = tplen_bytes;
  record_bytes = tnum_prec + tplen_prec;
  
  num_elts = max_tparts * num_tiles;
  elt_ctr = 0;
  tile_data_bytes = 0;

  int z_tlm = 0;
  int elts_left = num_elts;
  tlm_bytes = 0;
  do {
      int tlm_elts = (65535-4) / record_bytes;
      if (tlm_elts > elts_left)
        tlm_elts = elts_left;
      elts_left -= tlm_elts;
      tlm_bytes += 6 + record_bytes*tlm_elts;
      z_tlm++;
    } while ((elts_left > 0) && (z_tlm < 255));

  if (elts_left > 0)
    { // Cannot write a legal set of TLM marker segments -- too many
      // tile-parts.
      clear();
      return false;
    }

  elts = new kd_tlm_elt[num_elts];
  return true;
}

/*****************************************************************************/
/*                      kd_tlm_generator::add_tpart_length                   */
/*****************************************************************************/

void
  kd_tlm_generator::add_tpart_length(int tnum, kdu_long length)
{
  if (!exists())
    return;
  assert(elt_ctr < num_elts);
  assert((tnum >= 0) && (tnum < num_tiles));
  elts[elt_ctr].tnum = (kdu_uint16) tnum;
  elts[elt_ctr].length = (kdu_uint32) length;
  kdu_long max_tplen = (tplen_prec==2)?0x0FFFFL:0x0FFFFFFFFL;
  if (length > max_tplen)
    { KDU_ERROR(e,10); e <<
      KDU_TXT("Attempting to write TLM (tile-part length) data where "
              "at least one tile-part's length cannot be represented as an "
              "unsigned value with the precision identified via the "
              "`ORGtlm_style' parameter attribute -- or 32 bits if no such "
              "attribute was specified.");
    }
  if ((tnum_prec == 0) && (tnum != elt_ctr))
    { KDU_ERROR(e,0x25041001); e <<
      KDU_TXT("Attempting to write TLM (tile-part length) data using "
              "the \"implied\" tile-numbering style, as specified via "
              "the `ORGtlm_style' parameter attribute.  However, this "
              "requires tiles to be written in lexicographic order, which "
              "is not what's happening!");
    }
  elt_ctr++;
  tile_data_bytes += length;
}

/*****************************************************************************/
/*                      kd_tlm_generator::write_dummy_tlms                   */
/*****************************************************************************/

void
  kd_tlm_generator::write_dummy_tlms(kd_compressed_output *out)
{
  if (!exists())
    return;
  int z_tlm = 0;
  int elts_left = num_elts;
  int check_tlm_bytes = 0;
  kdu_byte *marker_body = new kdu_byte[65535];
  memset(marker_body,0,65535);
  do {
      assert(z_tlm <= 255);
      int tlm_elts = (65535-4) / record_bytes;
      if (tlm_elts > elts_left)
        tlm_elts = elts_left;
      elts_left -= tlm_elts;
      check_tlm_bytes += 6 + record_bytes*tlm_elts;
      out->put(KDU_TLM);
      out->put((kdu_uint16)(4+record_bytes*tlm_elts));
      out->put((kdu_byte) z_tlm);
      out->put((kdu_byte)((tnum_prec<<4)+((tplen_prec==4)?0x40:0)));
      z_tlm++;
      out->write(marker_body,record_bytes*tlm_elts);
    } while (elts_left > 0);
  assert(check_tlm_bytes == tlm_bytes);
  delete[] marker_body;
}

/*****************************************************************************/
/*                         kd_tlm_generator::write_tlms                      */
/*****************************************************************************/

void
  kd_tlm_generator::write_tlms(kdu_compressed_target *tgt,
                               int prev_tiles_written,
                               kdu_long prev_tile_bytes_written)
{
  if (!exists())
    return;

  kdu_long backtrack = tile_data_bytes + prev_tile_bytes_written;
  backtrack += tlm_bytes; // This should take us to the start of the TLM data
  
  // Start by walking over all TLM entries which have already been written
  int z_tlm = 0;
  int tlm_elts_left = 0;
  int elts_left = prev_tiles_written*max_tparts;
  int xfer;
  while (elts_left > 0)
    {
      if (tlm_elts_left == 0)
        { // Open a new marker segment
          assert(z_tlm <= 255);
          tlm_elts_left = (65535-4) / record_bytes;
          backtrack -= record_bytes; // For the TLM header already written
          z_tlm++;
        }
      xfer = (elts_left < tlm_elts_left)?elts_left:tlm_elts_left;
      tlm_elts_left -= xfer;
      elts_left -= xfer;
      backtrack -= record_bytes*xfer;
    }

  // Now we are ready to write the new tile-part lengths
  if (!tgt->start_rewrite(backtrack))
    { KDU_ERROR_DEV(e,11); e <<
        KDU_TXT("Attempting to invoke `kd_tlm_generator::write_final_tlms' "
        "with a compressed data target which does not support "
        "repositioning.");
    }
  elts_left = num_elts - prev_tiles_written*max_tparts;
  if (tlm_elts_left > elts_left)
    tlm_elts_left = elts_left;
  kd_tlm_elt *scan = elts;
  kdu_byte *marker_body = new kdu_byte[65535];
  kd_compressed_output out(tgt);
  while (elt_ctr > 0)
    {
      assert(elts_left > 0);
      if (tlm_elts_left == 0)
        { // Open a new marker segment
          assert(z_tlm <= 255);
          tlm_elts_left = (65535-4) / record_bytes;
          if (tlm_elts_left > elts_left)
            tlm_elts_left = elts_left;
          out.put(KDU_TLM);
          out.put((kdu_uint16)(4+record_bytes*tlm_elts_left));
          out.put((kdu_byte) z_tlm);
          out.put((kdu_byte)((tnum_prec<<4)+((tplen_prec==4)?0x40:0)));
          z_tlm++;
        }

      if (tnum_prec == 2)
        out.put(scan->tnum);
      else if (tnum_prec == 1)
        out.put((kdu_byte) scan->tnum);
      if (tplen_prec == 4)
        out.put(scan->length);
      else
        out.put((kdu_uint16)(scan->length));
      scan++;
      elt_ctr--;
      tlm_elts_left--;
      elts_left--;
    }
  delete[] marker_body;
  out.flush();
  tgt->end_rewrite();
}


/* ========================================================================= */
/*                         kd_tpart_pointer_server                           */
/* ========================================================================= */

/*****************************************************************************/
/*            kd_tpart_pointer_server::~kd_tpart_pointer_server              */
/*****************************************************************************/

kd_tpart_pointer_server::~kd_tpart_pointer_server()
{
  kd_tlm_marker_list *mkr;
  while ((mkr=tlm_markers) != NULL)
    {
      tlm_markers = mkr->next;
      delete mkr;
    }

  kd_pointer_group *grp;
  while ((grp=groups) != NULL)
    {
      groups = grp->next;
      delete grp;
    }
}

/*****************************************************************************/
/*                  kd_tpart_pointer_server::add_tlm_marker                  */
/*****************************************************************************/

void
  kd_tpart_pointer_server::add_tlm_marker(kd_marker &copy_source)
{
  translated_tlm_markers = false;
  assert(copy_source.get_code() == KDU_TLM);
  if (copy_source.get_length() < 4)
    { KDU_ERROR(e,12); e <<
        KDU_TXT("TLM marker segments must be at least 6 bytes long!");
    }
  kd_tlm_marker_list *elt = new kd_tlm_marker_list(copy_source);
  kdu_byte *data = elt->get_bytes();
  elt->znum = data[0];

  kd_tlm_marker_list *scan, *prev;

  for (prev=NULL, scan=tlm_markers; scan != NULL; prev=scan, scan=scan->next)
    if (scan->znum > elt->znum)
      break;
  elt->next = scan;
  if (prev == NULL)
    tlm_markers = elt;
  else
    {
      prev->next = elt;
      if (prev->znum == elt->znum)
        { KDU_ERROR(e,13); e <<
            KDU_TXT("Found multiple TLM marker segments with "
            "identical Ztlm indices within the main header!");
        }
    }
}

/*****************************************************************************/
/*                     kd_tpart_pointer_server::add_tpart                    */
/*****************************************************************************/

void
  kd_tpart_pointer_server::add_tpart(kd_tile_ref *tile_ref,
                                     kdu_long sot_address)
{
  kd_tpart_pointer *elt = free_list;
  if (elt == NULL)
    { // Allocate new group of pointer structures and put on free list
      kd_pointer_group *grp = new kd_pointer_group;
      grp->next = groups;
      groups = grp;
      elt = grp->elements;
      for (int g=KD_POINTER_GROUP_SIZE-1; g > 0; g--, elt++)
        elt->next = elt+1;
      elt->next = free_list;
      free_list = elt = grp->elements;
    }
  free_list = elt->next;
  elt->next = NULL;
  elt->address = sot_address;
  if (tile_ref->tpart_head == NULL)
    tile_ref->tpart_head = tile_ref->tpart_tail = elt;
  else if (tile_ref->tpart_tail != NULL)
    tile_ref->tpart_tail = tile_ref->tpart_tail->next = elt;
  else
    assert(0);
}

/*****************************************************************************/
/*                 kd_tpart_pointer_server::translate_markers                */
/*****************************************************************************/

void
  kd_tpart_pointer_server::translate_markers(kdu_long address, int num_tiles,
                                             kd_tile_ref *tile_refs)
{
  assert(groups == NULL); // Illegal to call this function more than once.
  if (tlm_markers == NULL)
    return;

  int tnum, last_tnum = -1;
  kd_tlm_marker_list *tlm;
  bool tlm_info_incomplete = false;
  while (((tlm=tlm_markers) != NULL) && !tlm_info_incomplete)
    {
      kdu_byte *data = tlm->get_bytes() + 1;
      int num_bytes = tlm->get_length() - 1;
      kdu_byte style = *(data++); num_bytes--;
      bool short_lengths = ((style & (1<<6)) == 0);
      bool short_tnums, missing_tnums;
      int num_fields, field_bytes;
      switch ((style>>4) & 3) {
        case 0: missing_tnums=true; short_tnums=false; field_bytes=0; break;
        case 1: missing_tnums=false; short_tnums=true; field_bytes=1; break;
        case 2: missing_tnums=false; short_tnums=false; field_bytes=2; break;
        default: { KDU_ERROR(e,14); e <<
                     KDU_TXT("Illegal Stlm field encountered in TLM "
                     "marker segment!");
                 }
        }
      field_bytes += (short_lengths)?2:4;
      num_fields = num_bytes / field_bytes;
      if ((num_fields < 1) || (num_bytes != (num_fields*field_bytes)))
        { KDU_ERROR(e,15); e <<
            KDU_TXT("Malformed TLM marker segment encountered in "
            "main header.  Segment length is inconsistent with the "
            "number of bytes used to represent pointer info for each "
            "tile-part.");
        }
      while (num_fields--)
        {
          if (missing_tnums)
            tnum = last_tnum + 1; // Tiles must be in order with one part
          else if (short_tnums)
            tnum = *(data++);
          else
            { tnum = *(data++); tnum<<=8; tnum += *(data++); }
          if (tnum >= num_tiles)
            { KDU_ERROR(e,16); e <<
                KDU_TXT("Illegal TLM marker segment data encountered "
                "in main header.  An illegal tile number has been "
                "identified, either explicitly or implicitly (through "
                "the rule that missing tile identifiers are legal "
                "only when tiles appear in order with only one "
                "tile-part each).");
            }
          add_tpart(tile_refs+tnum,address);
          last_tnum = tnum;
          kdu_uint32 length = *(data++); length<<=8; length += *(data++);
          if (!short_lengths)
            { length <<= 8; length += *(data++);
              length <<= 8; length += *(data++); }
          if (length < 14)
            { // Illegal length.
              tlm_info_incomplete = true;
              KDU_WARNING(w,5); w <<
                KDU_TXT("TLM marker segments contain "
                "one or more illegal lengths (< 14 bytes).  Proceeding "
                "with incomplete tile-part length information.");
              break;
            }
          address += (kdu_long) length;
        }
      tlm_markers = tlm->next;
      delete tlm;
    }

  translated_tlm_markers = true;

  // Finally, visit all the tiles, setting their address lists to have a
  // NULL tail so as to assert the fact that all tile-parts have been
  // found.
  for (tnum=0; tnum < num_tiles; tnum++)
    tile_refs[tnum].tpart_tail = NULL;
}


/* ========================================================================= */
/*                        kd_precinct_pointer_server                         */
/* ========================================================================= */

/*****************************************************************************/
/*               kd_precinct_pointer_server::add_plt_marker                  */
/*****************************************************************************/

void
  kd_precinct_pointer_server::add_plt_marker(kd_marker &marker,
                                             kdu_params *cod, kdu_params *poc)
{
  if (buf_server == NULL)
    return;
  kdu_byte *data = marker.get_bytes();
  int length = marker.get_length();
  if ((length < 1) || (data[0] != next_znum))
    { KDU_ERROR(e,17); e <<
        KDU_TXT("PLT marker segments appear out of order within one or more "
                "tile-part headers.  While this is not illegal, it is highly "
                "inadvisable since it prevents immediate condensation of the "
                "pointer information by efficient parsers.  To process this "
                "code-stream, you will have to open it again, with file "
                "seeking disabled.");
    }
  next_znum++; data++; length--;
  if (tpart_bytes_left > 0)
    { KDU_ERROR(e,18); e <<
        KDU_TXT("There appears to be a problem with the PLT marker "
        "segments included in the input code-stream.  The PLT marker "
        "segments encountered so far do not have sufficient length "
        "information to describe the lengths of all packets in the "
        "tile-parts encountered so far.  To process this code-stream, "
        "you will have to open it again, with file seeking disabled.");
    }

  int ival, layer_val, order_val;
  if (!(cod->get(Clayers,0,0,layer_val) && cod->get(Corder,0,0,order_val)))
    assert(0);
  if (num_layers == 0)
    num_layers = layer_val;
  if ((num_layers != layer_val) ||
      ((num_layers > 1) &&
       ((order_val == Corder_LRCP) || (order_val == Corder_RLCP) ||
        poc->get(Porder,0,0,ival))))
    { // PLT information is unusable.
      disable();
      if (something_served)
        { KDU_ERROR(e,19); e <<
            KDU_TXT("Unexpected change in coding parameters or "
            "packet sequencing detected while parsing packet length "
            "information in PLT marker segments.  While this is not "
            "illegal, it is highly inadvisable.  To process this "
            "code-stream, open it again with file seeking disabled!");
        }
      return;
    }
  if (head == NULL)
    initialize_recording();

  while (length > 0)
    {
      if (packets_left_in_precinct == 0)
        { packets_left_in_precinct = num_layers; precinct_length = 0; }
      kdu_long packet_length = 0;
      kdu_byte byte;
      do {
          if (length == 0)
            { KDU_ERROR(e,20); e <<
                KDU_TXT("Malformed PLT marker segment encountered "
                "in tile-part header.  Segment terminates part of the "
                "way through a multi-byte packet length "
                "specification!");
            }
          byte = *(data++); length--;
          packet_length = (packet_length<<7) + (kdu_long)(byte & 0x7F);
        } while (byte & 0x80);
      precinct_length += packet_length;
      packets_left_in_precinct--;
      if (packets_left_in_precinct == 0)
        { // Record new precinct length using the same byte extension code.
          int shift = 0;
          while ((precinct_length >> shift) >= 128)
            shift += 7;
          for (; shift >= 0; shift -= 7)
            {
              byte = (kdu_byte)(precinct_length >> shift);
              byte &= 0x7F;
              if (shift > 0)
                byte |= 0x80;
              record_byte(byte);
            }
          available_addresses++;
        }
    }
}

/*****************************************************************************/
/*              kd_precinct_pointer_server::start_tpart_body                 */
/*****************************************************************************/

void
  kd_precinct_pointer_server::start_tpart_body(kdu_long start_address,
                                kdu_uint32 length, kdu_params *cod,
                                kdu_params *poc, bool packed_headers,
                                bool final_tpart_with_unknown_length)
{
  next_znum = 0;
  if ((buf_server == NULL) ||
      ((length == 0) && !final_tpart_with_unknown_length))
    return;
  if ((head == NULL) && !something_served)
    { // There is no PLT information available and the tile-part is non-empty
      disable();
      return;
    }

  int ival, layer_val, order_val;
  if (packed_headers ||
      (!cod->get(Clayers,0,0,layer_val)) ||
      (layer_val != num_layers) ||
      ((num_layers > 1) &&
       (poc->get(Porder,0,0,ival) ||
        (!cod->get(Corder,0,0,order_val)) || (order_val == Corder_LRCP) ||
        (order_val == Corder_RLCP))))
    { // Cannot use PLT information
      disable();
      if (something_served)
        { KDU_ERROR(e,21); e <<
            KDU_TXT("Unexpected change in coding parameters or "
            "packet sequencing detected after parsing packet length "
            "information in PLT marker segments.  While this is not "
            "illegal, it is highly inadvisable.  To process this "
            "code-stream, open it again with file seeking disabled!");
        }
    }
  next_address = start_address;
  tpart_bytes_left = length;
  this->final_tpart_with_unknown_length = final_tpart_with_unknown_length;
}

/*****************************************************************************/
/*                 kd_precinct_pointer_server::pop_address                   */
/*****************************************************************************/

kdu_long
  kd_precinct_pointer_server::pop_address()
{
  if (buf_server == NULL)
    return 0;
  if ((available_addresses == 0) &&
      (final_tpart_with_unknown_length || (tpart_bytes_left != 0)))
    {
      assert(something_served);  // Otherwise, must have forgotten to call
                                 // `start_tpart_body'.
      KDU_ERROR(e,22); e <<
        KDU_TXT("Unexpectedly ran out of packet length information "
        "while processing tile-parts.  Most likely cause is that PLT "
        "marker segments are malformed, incomplete, or do not appear "
        "until after the packets whose lengths they describe.  All of "
        "these conditions are violations of the standard!");
    }
  if ((tpart_bytes_left == 0) && !final_tpart_with_unknown_length)
    return -1;

  kdu_long new_length = 0;
  kdu_byte byte;
  do {
      byte = retrieve_byte();
      new_length = (new_length << 7) + (kdu_long)(byte & 0x7F);
    } while (byte & 0x80);
  available_addresses--;
  if (final_tpart_with_unknown_length)
    {
      something_served = true;
      kdu_long result = next_address;
      next_address += new_length;
      return result;
    }

  if (new_length > (kdu_long) tpart_bytes_left)
    { KDU_ERROR(e,23); e <<
        KDU_TXT("Tile-part holds some but not all the packets of "
        "a precinct for which PLT information is being used to extract "
        "precinct addresses for random access.  In particular, the current "
        "tile has its packets sequenced so that all packets of any given "
        "precinct appear consecutively and yet a tile-part boundary has "
        "been inserted between the packets of a precinct.  While this is not "
        "illegal, it indicates very poor judgement in the placement of "
        "tile-part boundaries.  To process this code-stream, you will have "
        "to open it again with file seeking disabled.");
    }
  tpart_bytes_left -= (kdu_uint32) new_length;
  something_served = true;
  kdu_long result = next_address;
  next_address += new_length;
  return result;
}


/* ========================================================================= */
/*                               kd_buf_server                               */
/* ========================================================================= */

/*****************************************************************************/
/*                       kd_buf_server::~kd_buf_server                       */
/*****************************************************************************/

kd_buf_server::~kd_buf_server()
{
  kd_thread_buf_server *attached = attached_thread_buf_servers;
  if (attached != NULL)
    {
      kdu_thread_env *env = attached->env->owner->get_current_thread_env();
          // Retrieve our own `kdu_thread_env' via that of the first thread
          // whose `thread_buf_server' is attached to us.  It is OK if that
          // thread detaches from us while we are doing this, so that
          // `attached' may cease to reside on our list, so long as the thread
          // itself is not destroyed while we are gaining access to our
          // `env' reference.
      if (env == NULL)
        { KDU_ERROR_DEV(e,0x12070700); e <<
            KDU_TXT("You have invoked `kdu_codestream::destroy' from one "
            "thread, thread queues remain in which other threads may "
            "potentially do work.  This is OK, so long as the thread from "
            "which you invoke `kdu_codestream::destroy' is one of the "
            "threads participating in the `kdu_thread_entity' group to which "
            "the above-mentioned thread queues belong.  However, this "
            "does not appear to be the case here.  Typically, it is best to "
            "destroy codestreams from the same thread which invokes "
            "the `kdu_thread_entity::create' and "
            "`kdu_thread_entity::add_thread' functions which build "
            "cooperating multi-threaded processing systems."); }
      env->acquire_lock(KD_THREADLOCK_GENERAL);
      while (attached_thread_buf_servers != NULL)
        detach_thread_buf_server(attached_thread_buf_servers);
      env->release_lock(KD_THREADLOCK_GENERAL);
    }

  assert(num_users == 0);
  if (num_allocated_pages != 0)
    { KDU_WARNING_DEV(w,6);  w <<
        KDU_TXT("The compressed data buffer server is being "
        "destroyed before all allocated buffers have been returned.  The "
        "problem is most likely connected with a bug in the code-block "
        "destruction code."); }
  if (structure_bytes != 0)
    { KDU_WARNING_DEV(w,7);  w <<
        KDU_TXT("The compressed data buffer server is being "
        "destroyed before all size accounting associated with fixed data "
        "structures has been completed.  This is most likely due to a bug "
        "in the memory accounting logic used for dynamic cache management.");
    }
  kd_code_alloc *tmp;
  while ((tmp=alloc) != NULL)
    {
      alloc = tmp->next;
      free(tmp);
    }
}

/*****************************************************************************/
/*                         kd_buf_server::alloc_pages                        */
/*****************************************************************************/

void
  kd_buf_server::alloc_pages()
{
  int page_size = sizeof(kd_code_buffer)*KDU_CODE_BUFFERS_PER_PAGE;
  assert(page_size == (KDU_CODE_BUFFER_ALIGN*KDU_CODE_BUFFERS_PER_PAGE));
  assert((page_size & (page_size-1)) == 0); // Must be power of 2!
  kd_code_alloc *new_alloc = (kd_code_alloc *)
    malloc(sizeof(kd_code_alloc *) + (size_t)(65*page_size));
  if (new_alloc == NULL)
    throw std::bad_alloc();
  new_alloc->next = alloc;
  alloc = new_alloc;
  kdu_byte *block = new_alloc->block;
  int block_length = page_size*65;
  int page_offset = (- _addr_to_kdu_int32(block)) & (page_size-1);
  block += page_offset;
  block_length -= page_offset;
  kd_code_buffer *page = (kd_code_buffer *) block;
  for (; block_length >= page_size; block_length-=page_size,
       page+=KDU_CODE_BUFFERS_PER_PAGE)
    {
      int n;
      for (n=0; n < KDU_CODE_BUFFERS_PER_PAGE; n++)
        {
          assert(n < 64);
          page[n].next = NULL;
          page[n].state = (kdu_int16) n;
        }
      page->next = free_head;
      free_head = page;
      total_pages++;
    }
}

/*****************************************************************************/
/*                             kd_buf_server::get                            */
/*****************************************************************************/

kd_code_buffer *
  kd_buf_server::get()
{
  if (free_head == NULL)
    {
      assert(num_allocated_pages == total_pages);
      alloc_pages();
    }

  kd_code_buffer *result = free_head;
  int n, pos = result->state & 0x3F;
  kd_code_buffer *page = result - pos;
  for (n=0; n < pos; n++)
    if (page[n].state >= 0)
      { // Previous buffer in page is free; allocate this directly
        page[n].state |= 0x8000;
        return page+n;
      }
  result->state |= 0x8000;
  for (n=KDU_CODE_BUFFERS_PER_PAGE-1; n > pos; n--)
    if (page[n].state >= 0)
      { // Make this the new head of the free list
        free_head = page + n;
        page[n].next = result->next;  result->next = NULL;
        return result;
      }

  free_head = result->next;
  result->next = NULL;

  num_allocated_pages++;
  if (num_allocated_pages > peak_allocated_pages)
    peak_allocated_pages++;

  return result;
}

/*****************************************************************************/
/*                           kd_buf_server::release                          */
/*****************************************************************************/

void
  kd_buf_server::release(kd_code_buffer *buf)
{
  int n, pos = buf->state & 0x3F;
  kd_code_buffer *page = buf - pos;
  for (n=0; n < KDU_CODE_BUFFERS_PER_PAGE; n++)
    if (page[n].state >= 0)
      { // At least one buffer in the page is already on the free list
        assert(n != pos);
        buf->next = NULL;
        buf->state &= 0x7F;
        return;
      }
  assert(num_allocated_pages > 0);
  buf->next = free_head;
  buf->state &= 0x7F;
  free_head = buf;
  num_allocated_pages--;
}

/*****************************************************************************/
/*                        kd_buf_server::get_page_block                      */
/*****************************************************************************/

kd_code_buffer *
  kd_buf_server::get_page_block(int num_pages_needed,
                                kd_code_buffer * &result_tail)
{
  kd_code_buffer *scan, *prev, *next, *result_head;
  result_head = result_tail = NULL;
  for (prev=NULL, scan=free_head; num_pages_needed > 0; prev=scan, scan=next)
    {
      if (scan == NULL)
        { // Need to allocate some more pages
          alloc_pages();
          scan = free_head; // Start again from beginning of new buffers
          prev = NULL;
        }

      next = scan->next;
      int n, pos = scan->state & 0x7F;
      kd_code_buffer *page = scan - pos;
      for (n=0; n < KDU_CODE_BUFFERS_PER_PAGE; n++)
        if (page[n].state < 0)
          {
            page = NULL; // This page is not complete
            break;
          }
      if (page != NULL)
        { // Found a complete page; unlink and use it.
          if (prev == NULL)
            free_head = next;
          else
            prev->next = next;
          scan = prev; // Ensures that `prev' is the same on the next iteration
          num_allocated_pages++;
          num_pages_needed--;
          for (n=0; n < KDU_CODE_BUFFERS_PER_PAGE; n++)
            {
              page[n].state |= 0x8000;
              if (result_tail == NULL)
                result_head = result_tail = page + n;
              else
                result_tail = result_tail->next = page + n;
            }
          result_tail->next = NULL;
        }
    }
  if (num_allocated_pages > peak_allocated_pages)
    peak_allocated_pages = num_allocated_pages;
  return result_head;
}

/*****************************************************************************/
/*                  kd_buf_server::detach_thread_buf_server                  */
/*****************************************************************************/

void
  kd_buf_server::detach_thread_buf_server(kd_thread_buf_server *tbs)
{
  if (tbs->ultimate_buf_server == NULL)
    {
      assert(tbs->head == NULL);
      return;
    }
  while ((tbs->tail = tbs->head) != NULL)
    {
      tbs->head = tbs->tail->next;
      release(tbs->tail);
      tbs->num_available_buffers--;
    }
  if (tbs->prev_attached == NULL)
    {
      assert(tbs == attached_thread_buf_servers);
      attached_thread_buf_servers = tbs->next_attached;
    }
  else
    {
      assert(tbs->prev_attached->next_attached == tbs);
      tbs->prev_attached->next_attached = tbs->next_attached;
    }
  if (tbs->next_attached != NULL)
    tbs->next_attached->prev_attached = tbs->prev_attached;
  tbs->reset();
}

/*****************************************************************************/
/*                  kd_buf_server::attach_thread_buf_server                  */
/*****************************************************************************/

void
  kd_buf_server::attach_thread_buf_server(kd_thread_buf_server *tbs)
{
  assert(tbs->ultimate_buf_server == NULL);
  tbs->ultimate_buf_server = this;
  if ((tbs->next_attached = attached_thread_buf_servers) != NULL)
    tbs->next_attached->prev_attached = tbs;
  attached_thread_buf_servers = tbs;
  tbs->prev_attached = NULL;
}


/* ========================================================================= */
/*                            kd_thread_buf_server                           */
/* ========================================================================= */

/*****************************************************************************/
/*                  kd_thread_buf_server::adjust_peak_buffers                */
/*****************************************************************************/

void
  kd_thread_buf_server::adjust_peak_buffers()
{
  int num_allocated = ref_available_buffers - num_available_buffers;
  int potential_new_peak = num_allocated * KD_THREAD_PEAK_STORE_FAC;
  if (potential_new_peak > peak_buffers)
    peak_buffers = potential_new_peak;
  ref_available_buffers = num_available_buffers;
}

/*****************************************************************************/
/*                  kd_thread_buf_server::augment_local_store                */
/*****************************************************************************/

void
  kd_thread_buf_server::augment_local_store(bool general_lock_held)
{
  int new_buffers = peak_buffers - num_available_buffers;
  if (peak_buffers == 0)
    {
      new_buffers = ref_available_buffers + 1;
      new_buffers *= KD_THREAD_PEAK_STORE_FAC;
      if (new_buffers < 32)
        new_buffers = 32;
    }
  if (new_buffers <= 0)
    return;
  int new_pages = 1 + ((new_buffers-1) / KDU_CODE_BUFFERS_PER_PAGE);
  new_buffers = new_pages*KDU_CODE_BUFFERS_PER_PAGE;
  kd_code_buffer *new_head=NULL, *new_tail=NULL;
  if (!general_lock_held)
    env->owner->acquire_lock(KD_THREADLOCK_GENERAL);
  assert(ultimate_buf_server != NULL);
  new_head = ultimate_buf_server->get_page_block(new_pages,new_tail);
  if (!general_lock_held)
    env->owner->release_lock(KD_THREADLOCK_GENERAL);
  if (tail != NULL)
    tail->next = new_head;
  else
    head = new_head;
  tail = new_tail;
  num_available_buffers += new_buffers;
  ref_available_buffers += new_buffers;
}


/* ========================================================================= */
/*                               kd_thread_env                               */
/* ========================================================================= */

/*****************************************************************************/
/*                      kd_thread_env::reconcile_stats                       */
/*****************************************************************************/

void
  kd_thread_env::reconcile_stats()
{
  if ((stats_codestream != NULL) && (stats_codestream->stats != NULL) &&
      !stats.is_empty())
    {
      owner->acquire_lock(KD_THREADLOCK_STATS);
      bool need_trim = stats_codestream->stats->update_stats(stats) &&
                       !stats_codestream->header_generated;
      stats_codestream->stats->update_quant_slope_thresholds();
      if (need_trim)
        { // Need both the precinct lock and the general lock for trimming.
          // Trimming is not recommended for multi-processor platforms.
          owner->acquire_lock(KD_THREADLOCK_GENERAL);
          stats_codestream->trim_compressed_data();
          owner->release_lock(KD_THREADLOCK_GENERAL);
        }
      owner->release_lock(KD_THREADLOCK_STATS);
    }
  stats_codestream = NULL; // So we know that all is reconciled and we are
              // free to update stats for a different codestream in the future
}

/*****************************************************************************/
/*                            kd_thread_env::flush                           */
/*****************************************************************************/

void
  kd_thread_env::flush(bool force_flush)
{
  if (num_outstanding_blocks == 0)
    return;

  if (stats_codestream != NULL)
    reconcile_stats();
  
  // Now try to flush stored code-blocks
  int n;
  kd_thread_block_state *scan;
  bool have_complete_precincts=false;
  owner->acquire_lock(KD_THREADLOCK_PRECINCT);
  bool have_only_input_precincts = true;
  for (scan=outstanding_blocks,
       n=KD_THREAD_MAX_OUTSTANDING_BLOCKS; n > 0; n--, scan++)
    {
      if (scan->target_precinct == NULL)
        continue;
      kd_codestream *cs = scan->target_precinct->resolution->codestream;
      assert(scan->target_precinct->num_outstanding_blocks > 0);
      if (scan->target_precinct->num_outstanding_blocks == 1)
        {
          have_complete_precincts=true;
          continue;
        }
      if (cs->in == NULL)
        {
          scan->target_block->transfer_data(scan->block);
          have_only_input_precincts = false;
        }
      scan->target_precinct->num_outstanding_blocks--;
      scan->target_precinct = NULL;
      scan->target_block = NULL;
      num_outstanding_blocks--;
    }
  owner->release_lock(KD_THREADLOCK_PRECINCT);

  if (num_outstanding_blocks == 0)
    owner->have_outstanding_blocks = false;
  else
    assert(have_complete_precincts);
  if (buf_server.is_attached() && !have_only_input_precincts)
    buf_server.adjust_peak_buffers();

  if (have_complete_precincts)
    { // Need to acquire the general lock to continue
      if (force_flush)
        owner->acquire_lock(KD_THREADLOCK_GENERAL);
      else if (!owner->try_lock(KD_THREADLOCK_GENERAL))
        return;

      for (scan=outstanding_blocks,
           n=KD_THREAD_MAX_OUTSTANDING_BLOCKS; n > 0; n--, scan++)
        {
          if (scan->target_precinct == NULL)
            continue;
          kd_precinct *precinct = scan->target_precinct;
          kd_codestream *cs = precinct->resolution->codestream;
          if (cs->in == NULL)
            scan->target_block->transfer_data(scan->block);
          assert(precinct->num_outstanding_blocks == 1);
          precinct->num_outstanding_blocks = 0;
          scan->target_precinct = NULL;
          scan->target_block = NULL;
          num_outstanding_blocks--;
          if (cs->in != NULL)
            precinct->release();
          else if (cs->out != NULL)
            precinct->resolution->rescomp->add_ready_precinct(precinct);
        }
      owner->have_outstanding_blocks = false;
      if (buf_server.is_attached() && !have_only_input_precincts)
        buf_server.augment_local_store(true);
      owner->release_lock(KD_THREADLOCK_GENERAL);
      assert(num_outstanding_blocks == 0);
    }
}

/*****************************************************************************/
/*                         kd_thread_env::discard_all                        */
/*****************************************************************************/

void
  kd_thread_env::discard_all()
{
  if ((num_outstanding_blocks > 0) || buf_server.is_attached())
    {
      int n;
      kd_thread_block_state *scan;
      owner->acquire_lock(KD_THREADLOCK_GENERAL,false);
      for (scan=outstanding_blocks,
           n=KD_THREAD_MAX_OUTSTANDING_BLOCKS; n > 0; n--, scan++)
        {
          if (scan->target_precinct == NULL)
            continue;
          kd_codestream *cs = scan->target_precinct->resolution->codestream;
          scan->target_precinct = NULL;
          scan->target_block = NULL;
          scan->block.cleanup(cs->buf_server);
        }
      if (buf_server.is_attached())
        buf_server.detach_while_holding_general_lock();
      owner->release_lock(KD_THREADLOCK_GENERAL);
    }
  num_outstanding_blocks = 0;
  owner->have_outstanding_blocks = false;
}


/* ========================================================================= */
/*                               kdu_thread_env                              */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdu_thread_env::kdu_thread_env                      */
/*****************************************************************************/

kdu_thread_env::kdu_thread_env()
{
  state = NULL; // Just in case
  state = new kd_thread_env(this);
  have_outstanding_blocks = false;
}

/*****************************************************************************/
/*                      kdu_thread_env::~kdu_thread_env                      */
/*****************************************************************************/

kdu_thread_env::~kdu_thread_env()
{
  if (is_group_owner())
    destroy();
  if (state != NULL)
    delete state;
}

/*****************************************************************************/
/*                          kdu_thread_env::do_sync                          */
/*****************************************************************************/

void
  kdu_thread_env::do_sync(bool exception_handled)
{
  if (exception_handled)
    state->discard_all();
  else
    state->flush(true);
  assert(!have_outstanding_blocks);
}

/*****************************************************************************/
/*                        kdu_thread_env::on_finished                        */
/*****************************************************************************/

void
  kdu_thread_env::on_finished(bool exception_handled)
{
  if (state == NULL)
    return;
  if (exception_handled)
    state->discard_all();
  else if (state->buf_server.is_attached())
    state->buf_server.set_codestream_buf_server(NULL);
  state->stats_codestream = NULL;
  state->stats.init();
}


/* ========================================================================= */
/*                            kd_packet_sequencer                            */
/* ========================================================================= */

/*****************************************************************************/
/*                         kd_packet_sequencer::init                         */
/*****************************************************************************/

void
  kd_packet_sequencer::init()
{
  int c, r;
  kd_tile_comp *tc;
  kd_resolution *res;

  assert(tile->initialized);
  max_dwt_levels = 0;
  common_grids = true;
  for (c=0; c < tile->num_components; c++)
    {
      tc = tile->comps + c;
      if (tc->dwt_levels > max_dwt_levels)
        max_dwt_levels = tc->dwt_levels;
      if (!(is_power_2(tc->sub_sampling.x) && is_power_2(tc->sub_sampling.y)))
        common_grids = false;
      for (r=0; r <= tc->dwt_levels; r++)
        {
          kdu_long inc;

          res = tc->resolutions + r;
          inc = res->precinct_partition.size.x;


          inc <<= res->hor_depth;
          inc *= tc->sub_sampling.x;
          if (inc > INT_MAX)
            inc = INT_MAX;
          if ((r == 0) || (inc < tc->grid_inc.x))
            tc->grid_inc.x = (int) inc;

          inc = res->precinct_partition.size.y;
          inc <<= res->vert_depth;
          inc *= tc->sub_sampling.y;
          if (inc > INT_MAX)
            inc = INT_MAX;
          if ((r == 0) || (inc < tc->grid_inc.y))
            tc->grid_inc.y = (int) inc;
        }
      tc->grid_min = tile->dims.pos - tile->coding_origin;
      tc->grid_min.x = tc->grid_inc.x *
        floor_ratio(tc->grid_min.x,tc->grid_inc.x);
      tc->grid_min.y = tc->grid_inc.y *
        floor_ratio(tc->grid_min.y,tc->grid_inc.y);
      tc->grid_min += tile->coding_origin;
    }
  grid_lim = tile->dims.pos + tile->dims.size;
  state_saved = false;
  state.poc = NULL;
  state.next_poc_record = 0;
  next_progression();
}

/*****************************************************************************/
/*                     kd_packet_sequencer::save_state                       */
/*****************************************************************************/

void
  kd_packet_sequencer::save_state()
{
  saved_state = state;
  tile->saved_num_sequenced_packets = tile->sequenced_relevant_packets;
  for (int c=0; c < tile->num_components; c++)
    {
      kd_tile_comp *tc = tile->comps + c;
      tc->saved_grid_min = tc->grid_min;
      tc->saved_grid_inc = tc->grid_inc;
      for (int r=0; r <= tc->dwt_levels; r++)
        {
          kd_resolution *res = tc->resolutions + r;
          res->saved_current_sequencer_pos = res->current_sequencer_pos;
          int num_precincts = (int) res->precinct_indices.area();
          for (int n=0; n < num_precincts; n++)
            {
              kd_precinct *precinct = res->precinct_refs[n].deref();
              if (precinct != NULL)
                precinct->saved_next_layer_idx = precinct->next_layer_idx;
            }
        }
    }
  state_saved = true;
}

/*****************************************************************************/
/*                   kd_packet_sequencer::restore_state                      */
/*****************************************************************************/

void
  kd_packet_sequencer::restore_state()
{
  assert(state_saved);
  state = saved_state;
  tile->sequenced_relevant_packets = tile->saved_num_sequenced_packets;
  for (int c=0; c < tile->num_components; c++)
    {
      kd_tile_comp *tc = tile->comps + c;
      tc->grid_min = tc->saved_grid_min;
      tc->grid_inc = tc->saved_grid_inc;
      for (int r=0; r <= tc->dwt_levels; r++)
        {
          kd_resolution *res = tc->resolutions + r;
          res->current_sequencer_pos = res->saved_current_sequencer_pos;
          int num_precincts = (int) res->precinct_indices.area();
          for (int n=0; n < num_precincts; n++)
            {
              kd_precinct *precinct = res->precinct_refs[n].deref();
              if (precinct != NULL)
                precinct->next_layer_idx = precinct->saved_next_layer_idx;
            }
        }
    }
}

/*****************************************************************************/
/*                  kd_packet_sequencer::next_progression                    */
/*****************************************************************************/

bool
  kd_packet_sequencer::next_progression()
{
  if (state.poc == NULL) // Must be initial call.
    {
      state.poc = tile->codestream->siz->access_cluster(POC_params);
      assert(state.poc != NULL);
      state.poc = state.poc->access_relation(tile->t_num,-1,0,true);
      assert(state.poc != NULL);
      if (!state.poc->get(Porder,0,0,state.res_min))
        state.poc = NULL;
    }
  if (state.poc == NULL)
    { // Get information from COD marker segment
      kdu_params *cod =
        tile->codestream->siz->access_cluster(COD_params);
      cod = cod->access_relation(tile->t_num,-1,0,true);
      if (!cod->get(Corder,0,0,state.order))
        assert(0);
      state.comp_min = state.res_min = 0;
      state.layer_lim = tile->num_layers;
      state.comp_lim = tile->num_components;
      state.res_lim = max_dwt_levels+1;
    }
  else
    { // Get information from POC marker segment
      if (!state.poc->get(Porder,state.next_poc_record,0,state.res_min))
        { // Need to access a new POC instance.
          int inst_idx = state.poc->get_instance();
          inst_idx++;
          kdu_params *tmp_poc =
            state.poc->access_relation(tile->t_num,-1,inst_idx,true);
          if ((tmp_poc == NULL) ||
              !tmp_poc->get(Porder,0,0,state.res_min))
            {
              if (tile->codestream->in == NULL)
                { KDU_ERROR(e,24); e <<
                    KDU_TXT("Supplied progression order attributes "
                    "for tile " << tile->t_num << " are insuffient to cover "
                    "all packets for the tile!");
                }
              return false;
            }
          if (inst_idx >= tile->next_tpart)
            return false; // Need to generate a new tile-part first.
          state.poc = tmp_poc;
          state.next_poc_record = 0;
        }
      state.poc->get(Porder,state.next_poc_record,1,state.comp_min);
      state.poc->get(Porder,state.next_poc_record,2,state.layer_lim);
      state.poc->get(Porder,state.next_poc_record,3,state.res_lim);
      state.poc->get(Porder,state.next_poc_record,4,state.comp_lim);
      state.poc->get(Porder,state.next_poc_record,5,state.order);
      if (((state.comp_min != 0) || (state.res_min != 0)) &&
          (state.next_poc_record == 0) && (state.poc->get_instance() == 0) &&
          (tile->codestream->profile == 0))
        { KDU_WARNING(w,8); w <<
            KDU_TXT("Profile violation detected (code-stream is "
            "technically illegal).  In a Profile-0 code-stream, the first "
            "progression specification found in the first POC marker segment "
            "of the main or any tile header may not describe a progression "
            "which starts from resolution or component indices other than 0.");
          tile->codestream->profile = 2;
        }
      state.next_poc_record++;
    }
  if (state.layer_lim > tile->num_layers)
    state.layer_lim = tile->num_layers;
  if (state.comp_lim > tile->num_components)
    state.comp_lim = tile->num_components;
  if (state.res_lim > max_dwt_levels)
    state.res_lim = max_dwt_levels+1;
  state.layer_idx=0;
  state.comp_idx = state.comp_min;
  state.res_idx = state.res_min;
  state.pos.x = state.pos.y=0;

  bool spatial = false;
  if ((state.order == Corder_PCRL) || (state.order == Corder_RPCL))
    {
      spatial = true;
      if (!common_grids)
        { KDU_ERROR(e,25); e <<
            KDU_TXT("Attempting to use a spatially progressive "
            "packet sequence where position order dominates component order. "
            "This is illegal when the component sub-sampling factors are not "
            "exact powers of 2!");
        }
      for (int c=0; c < tile->num_components; c++)
        {
          kd_tile_comp *tc = tile->comps + c;
          if ((c == 0) || (tc->grid_inc.x < state.grid_inc.x))
            {
              state.grid_inc.x = tc->grid_inc.x;
              state.grid_min.x = tc->grid_min.x;
            }
          if ((c == 0) || (tc->grid_inc.y < state.grid_inc.y))
            {
              state.grid_inc.y = tc->grid_inc.y;
              state.grid_min.y = tc->grid_min.y;
            }
        }
      state.grid_loc = state.grid_min;
    }
  else if ((state.order == Corder_CPRL) && (state.comp_idx < state.comp_lim))
    {
      spatial = true;
      state.grid_min = tile->comps[state.comp_idx].grid_min;
      state.grid_inc = tile->comps[state.comp_idx].grid_inc;
      state.grid_loc = state.grid_min;
    }

  if (spatial)
    { // Need to reset the precinct position indices in each resolution.
      for (int c=0; c < tile->num_components; c++)
        {
          kd_tile_comp *tc = tile->comps + c;
          for (int r=0; r <= tc->dwt_levels; r++)
            {
              kd_resolution *res = tc->resolutions + r;
              res->current_sequencer_pos.x = res->current_sequencer_pos.y = 0;
            }
        }
    }
  return true;
}

/*****************************************************************************/
/*                   kd_packet_sequencer::next_in_sequence                   */
/*****************************************************************************/

kd_precinct_ref *
  kd_packet_sequencer::next_in_sequence(kd_resolution* &res,
                                        kdu_coords &idx)
{
  kd_precinct_ref *result = NULL;

  if (tile->sequenced_relevant_packets == tile->max_relevant_packets)
    return NULL;
  do {
      if (state.order == Corder_LRCP)
        result = next_in_lrcp(res,idx);
      else if (state.order == Corder_RLCP)
        result = next_in_rlcp(res,idx);
      else if (state.order == Corder_RPCL)
        result = next_in_rpcl(res,idx);
      else if (state.order == Corder_PCRL)
        result = next_in_pcrl(res,idx);
      else if (state.order == Corder_CPRL)
        result = next_in_cprl(res,idx);
      else
        assert(0);
    } while ((result == NULL) && next_progression());
  if (result == NULL)
    return NULL;
  kd_precinct *precinct = result->deref();
  if ((tile->codestream->in != NULL) &&
      ((precinct == NULL) || (precinct->next_layer_idx == 0)))
    { // See if we can recover a seek address.
      kdu_long seek_address =
        tile->precinct_pointer_server.get_precinct_address();
      if (seek_address < 0)
        return NULL; // Need a new tile-part.
      if (seek_address > 0)
        {
          if (!result->set_address(res,idx,seek_address))
            result = NULL; // Tile has been destroyed
        }
    }
  return result;
}

/*****************************************************************************/
/*                    kd_packet_sequencer::next_in_lrcp                      */
/*****************************************************************************/

kd_precinct_ref *
  kd_packet_sequencer::next_in_lrcp(kd_resolution* &p_res, kdu_coords &p_idx)
{
  for (; state.layer_idx < state.layer_lim;
       state.layer_idx++, state.res_idx=state.res_min)
    for (; state.res_idx < state.res_lim;
         state.res_idx++, state.comp_idx=state.comp_min)
      for (; state.comp_idx < state.comp_lim;
           state.comp_idx++, state.pos.y=0)
        {
          kd_tile_comp *tc = tile->comps + state.comp_idx;
          if (state.res_idx > tc->dwt_levels)
            continue; // Advance to next component.
          kd_resolution *res = tc->resolutions + state.res_idx;
          for (; state.pos.y < res->precinct_indices.size.y;
               state.pos.y++, state.pos.x=0)
            for (; state.pos.x < res->precinct_indices.size.x;
                 state.pos.x++)
              {
                kd_precinct_ref *ref = res->precinct_refs +
                  state.pos.y*res->precinct_indices.size.x + state.pos.x;
                if (!ref->is_desequenced())
                  {
                    kd_precinct *precinct = ref->deref();
                    assert(((precinct != NULL) &&
                            (precinct->next_layer_idx >= state.layer_idx)) ||
                           (state.layer_idx == 0));
                    if ((precinct == NULL) ||
                        (precinct->next_layer_idx == state.layer_idx))
                      { p_res = res; p_idx = state.pos; return ref; }
                  }
              }
        }
  return NULL;
}

/*****************************************************************************/
/*                    kd_packet_sequencer::next_in_rlcp                      */
/*****************************************************************************/

kd_precinct_ref *
  kd_packet_sequencer::next_in_rlcp(kd_resolution* &p_res, kdu_coords &p_idx)
{
  for (; state.res_idx < state.res_lim;
       state.res_idx++, state.layer_idx=0)
    for (; state.layer_idx < state.layer_lim;
         state.layer_idx++, state.comp_idx=state.comp_min)
      for (; state.comp_idx < state.comp_lim;
           state.comp_idx++, state.pos.y=0)
        {
          kd_tile_comp *tc = tile->comps + state.comp_idx;
          if (state.res_idx > tc->dwt_levels)
            continue; // Advance to next component.
          kd_resolution *res = tc->resolutions + state.res_idx;
          for (; state.pos.y < res->precinct_indices.size.y;
               state.pos.y++, state.pos.x=0)
            for (; state.pos.x < res->precinct_indices.size.x;
                 state.pos.x++)
              {
                kd_precinct_ref *ref = res->precinct_refs +
                  state.pos.y*res->precinct_indices.size.x + state.pos.x;
                if (!ref->is_desequenced())
                  {
                    kd_precinct *precinct = ref->deref();
                    assert(((precinct != NULL) &&
                            (precinct->next_layer_idx >= state.layer_idx)) ||
                           (state.layer_idx == 0));
                    if ((precinct == NULL) ||
                        (precinct->next_layer_idx == state.layer_idx))
                      { p_res = res; p_idx = state.pos; return ref; }
                  }
              }
        }
  return NULL;
}

/*****************************************************************************/
/*                    kd_packet_sequencer::next_in_rpcl                      */
/*****************************************************************************/

kd_precinct_ref *
  kd_packet_sequencer::next_in_rpcl(kd_resolution* &p_res, kdu_coords &p_idx)
{
  if (state.layer_lim <= 0)
    return NULL;
  for (; state.res_idx < state.res_lim;
       state.res_idx++, state.grid_loc.y=state.grid_min.y)
    for (; state.grid_loc.y < grid_lim.y;
         state.grid_loc.y+=state.grid_inc.y, state.grid_loc.x=state.grid_min.x)
      for (; state.grid_loc.x < grid_lim.x;
           state.grid_loc.x+=state.grid_inc.x, state.comp_idx=state.comp_min)
        for (; state.comp_idx < state.comp_lim; state.comp_idx++)
          {
            kd_tile_comp *tc = tile->comps + state.comp_idx;
            if (state.res_idx > tc->dwt_levels)
              continue; // Advance to next component.
            kd_resolution *res = tc->resolutions + state.res_idx;
            state.pos = res->current_sequencer_pos;
            if ((state.pos.x >= res->precinct_indices.size.x) ||
                (state.pos.y >= res->precinct_indices.size.y))
              continue; // No precincts left in this resolution.
            kd_precinct_ref *ref = res->precinct_refs +
              state.pos.x + state.pos.y*res->precinct_indices.size.x;
            kd_precinct *precinct;
            if (ref->is_desequenced() ||
                (((precinct=ref->deref()) != NULL) &&
                 (precinct->next_layer_idx >= state.layer_lim)))
              { // Cannnot sequence this one any further.
                state.pos.x++;
                if (state.pos.x >= res->precinct_indices.size.x)
                  { state.pos.x = 0; state.pos.y++; }
                res->current_sequencer_pos = state.pos;
                continue; // Move sequencing loops ahead.
              }

            int gpos;
            gpos = res->precinct_indices.pos.y + state.pos.y;
            gpos *= res->precinct_partition.size.y;
            gpos <<= res->vert_depth;
            gpos *= tc->sub_sampling.y;
            gpos += tile->coding_origin.y;
            if ((gpos >= state.grid_min.y) && (gpos != state.grid_loc.y))
              continue;
            gpos = res->precinct_indices.pos.x + state.pos.x;
            gpos *= res->precinct_partition.size.x;
            gpos <<= res->hor_depth;
            gpos *= tc->sub_sampling.x;
            gpos += tile->coding_origin.x;
            if ((gpos >= state.grid_min.x) && (gpos != state.grid_loc.x))
              continue;

            p_res = res; p_idx = state.pos;
            return ref;
          }
  return NULL;
}

/*****************************************************************************/
/*                    kd_packet_sequencer::next_in_pcrl                      */
/*****************************************************************************/

kd_precinct_ref *
  kd_packet_sequencer::next_in_pcrl(kd_resolution* &p_res, kdu_coords &p_idx)
{
  if (state.layer_lim <= 0)
    return NULL;

  for (; state.grid_loc.y < grid_lim.y;
       state.grid_loc.y+=state.grid_inc.y, state.grid_loc.x=state.grid_min.x)
    for (; state.grid_loc.x < grid_lim.x;
         state.grid_loc.x+=state.grid_inc.x, state.comp_idx=state.comp_min)
      for (; state.comp_idx < state.comp_lim;
           state.comp_idx++, state.res_idx=state.res_min)
        for (; state.res_idx < state.res_lim; state.res_idx++)
          {
            kd_tile_comp *tc = tile->comps + state.comp_idx;
            if (state.res_idx > tc->dwt_levels)
              break; // Advance to next component.
            kd_resolution *res = tc->resolutions + state.res_idx;
            state.pos = res->current_sequencer_pos;
            if ((state.pos.x >= res->precinct_indices.size.x) ||
                (state.pos.y >= res->precinct_indices.size.y))
              continue; // No precincts left in this resolution.
            kd_precinct_ref *ref = res->precinct_refs +
              state.pos.x + state.pos.y*res->precinct_indices.size.x;
            kd_precinct *precinct;
            if (ref->is_desequenced() ||
                (((precinct=ref->deref()) != NULL) &&
                 (precinct->next_layer_idx >= state.layer_lim)))
              { // Cannot sequence this one any further.
                state.pos.x++;
                if (state.pos.x >= res->precinct_indices.size.x)
                  { state.pos.x = 0; state.pos.y++; }
                res->current_sequencer_pos = state.pos;
                continue; // Move sequencing loops ahead.
              }

            int gpos;
            gpos = res->precinct_indices.pos.y + state.pos.y;
            gpos *= res->precinct_partition.size.y;
            gpos <<= res->vert_depth;
            gpos *= tc->sub_sampling.y;
            gpos += tile->coding_origin.y;
            if ((gpos >= state.grid_min.y) && (gpos != state.grid_loc.y))
              continue;
            gpos = res->precinct_indices.pos.x + state.pos.x;
            gpos *= res->precinct_partition.size.x;
            gpos <<= res->hor_depth;
            gpos *= tc->sub_sampling.x;
            gpos += tile->coding_origin.x;
            if ((gpos >= state.grid_min.x) && (gpos != state.grid_loc.x))
              continue;

            p_res = res; p_idx = state.pos;
            return ref;
          }
  return NULL;
}

/*****************************************************************************/
/*                    kd_packet_sequencer::next_in_cprl                      */
/*****************************************************************************/

kd_precinct_ref *
  kd_packet_sequencer::next_in_cprl(kd_resolution* &p_res, kdu_coords &p_idx)
{
  if (state.layer_lim <= 0)
    return NULL;
  while (state.comp_idx < state.comp_lim)
    {
      kd_tile_comp *tc = tile->comps + state.comp_idx;

      for (; state.grid_loc.y < grid_lim.y;
           state.grid_loc.y += state.grid_inc.y,
           state.grid_loc.x = state.grid_min.x)
        for (; state.grid_loc.x < grid_lim.x;
             state.grid_loc.x += state.grid_inc.x, state.res_idx=state.res_min)
          for (; state.res_idx < state.res_lim; state.res_idx++)
            {
              if (state.res_idx > tc->dwt_levels)
                break; // Advance to next position.
              kd_resolution *res = tc->resolutions + state.res_idx;
              state.pos = res->current_sequencer_pos;
              if ((state.pos.x >= res->precinct_indices.size.x) ||
                  (state.pos.y >= res->precinct_indices.size.y))
                continue; // No precincts left in this resolution.
              kd_precinct_ref *ref = res->precinct_refs +
                state.pos.x + state.pos.y*res->precinct_indices.size.x;
              kd_precinct *precinct;
              if (ref->is_desequenced() ||
                  (((precinct=ref->deref()) != NULL) &&
                   (precinct->next_layer_idx >= state.layer_lim)))
                { // Cannot sequence this one any further.
                  state.pos.x++;
                  if (state.pos.x >= res->precinct_indices.size.x)
                    { state.pos.x = 0; state.pos.y++; }
                  res->current_sequencer_pos = state.pos;
                  continue; // Move sequencing loops ahead.
                }

              int gpos;
              gpos = res->precinct_indices.pos.y + state.pos.y;
              gpos *= res->precinct_partition.size.y;
              gpos <<= res->vert_depth;
              gpos *= tc->sub_sampling.y;
              gpos += tile->coding_origin.y;
              if ((gpos >= state.grid_min.y) && (gpos != state.grid_loc.y))
                continue;
              gpos = res->precinct_indices.pos.x + state.pos.x;
              gpos *= res->precinct_partition.size.x;
              gpos <<= res->hor_depth;
              gpos *= tc->sub_sampling.x;
              gpos += tile->coding_origin.x;
              if ((gpos >= state.grid_min.x) && (gpos != state.grid_loc.x))
                continue;

              p_res = res; p_idx = state.pos;
              return ref;
            }

      // Advance component index.

      state.comp_idx++;
      if (state.comp_idx < state.comp_lim)
        { // Install spatial progression parameters for new tile-component.
          tc = tile->comps + state.comp_idx;
          state.grid_min = tc->grid_min;
          state.grid_inc = tc->grid_inc;
          state.grid_loc = state.grid_min;
        }
    }
  return NULL;
}


/* ========================================================================= */
/*                           kd_reslength_checker                            */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_reslength_checker::init                         */
/*****************************************************************************/

bool kd_reslength_checker::init(cod_params *cod)
{
  if (specs != NULL)
    { delete[] specs; specs = NULL; }
  int n, val, max_specs = num_specs = 0;
  current_layer_idx = -1;
  if (cod == NULL)
    return false;
  for (n=0; cod->get(Creslengths,n,0,val,false,false); n++)
    {
      if (max_specs <= n)
        {
          max_specs += max_specs + 8;
          kdu_long *new_specs = new kdu_long[max_specs];
          if (specs != NULL)
            {
              memcpy(new_specs,specs,(size_t)(num_specs*sizeof(kdu_long)));
              delete[] specs;
            }
          specs = new_specs;
        }
      specs[num_specs++] = val;
    }
  return (num_specs > 0);
}

/*****************************************************************************/
/*                      kd_reslength_checker::set_layer                      */
/*****************************************************************************/

void kd_reslength_checker::set_layer(int layer_idx)
{
  if (specs == NULL)
    return;
  if (layer_idx == current_layer_idx)
    memcpy(num_bytes,prev_layer_bytes,33*sizeof(kdu_long));
  else if (layer_idx == 0)
    {
      memset(num_bytes,0,33*sizeof(kdu_long));
      memset(prev_layer_bytes,0,33*sizeof(kdu_long));
    }
  else if (layer_idx == (current_layer_idx+1))
    memcpy(prev_layer_bytes,num_bytes,33*sizeof(kdu_long));
  else
    assert(0);
  if (layer_idx != current_layer_idx)
    { // Parse contents of the `specs' array for this layer
      memset(max_bytes,0,33*sizeof(kdu_long));
      int d, l_idx, n;
      for (l_idx=0, d=0, n=0; n < num_specs; n++, d++)
        {
          if (specs[n] <= 0)
            { l_idx++; d=-1; continue; }
          if (l_idx == layer_idx)
            max_bytes[d] = specs[n];
          else if ((l_idx > layer_idx) &&
                   ((max_bytes[d] == 0) || (max_bytes[d] > specs[n])))
            max_bytes[d] = specs[n];
        }
    }
  current_layer_idx = layer_idx;
}


/* ========================================================================= */
/*                             kd_global_rescomp                             */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_global_rescomp::close_all                       */
/*****************************************************************************/

void
  kd_global_rescomp::close_all()
{
  while ((last_ready=first_ready) != NULL)
    {
      first_ready = last_ready->next;
      last_ready->next = last_ready->prev = NULL;
      last_ready->ref->close();
    }
}

/*****************************************************************************/
/*                        kd_global_rescomp::initialize                      */
/*****************************************************************************/

void
  kd_global_rescomp::initialize(kd_codestream *codestream,
                                int depth, int comp_idx)
{
  close_all();
  this->codestream = codestream;
  this->depth = depth; this->comp_idx = comp_idx;
  kdu_coords min = codestream->region.pos;
  kdu_coords lim = min + codestream->region.size;
  kd_comp_info *ci = codestream->comp_info + comp_idx;
  min.x = ceil_ratio(min.x,ci->sub_sampling.x);
  min.y = ceil_ratio(min.y,ci->sub_sampling.y);
  lim.x = ceil_ratio(lim.x,ci->sub_sampling.x);
  lim.y = ceil_ratio(lim.y,ci->sub_sampling.y);
  min.x = 1 + ((min.x-1)>>(ci->hor_depth[depth]));
  min.y = 1 + ((min.y-1)>>(ci->vert_depth[depth]));
  lim.x = 1 + ((lim.x-1)>>(ci->hor_depth[depth]));
  lim.y = 1 + ((lim.y-1)>>(ci->vert_depth[depth]));
  total_area = ((kdu_long)(lim.x-min.x)) * ((kdu_long)(lim.y-min.y));
  area_used_by_tiles = area_covered_by_tiles = 0;
  remaining_area = total_area;
  first_ready = last_ready = NULL;
  ready_area = 0;
  ready_fraction = reciprocal_fraction = -1.0;
}

/*****************************************************************************/
/*                   kd_global_rescomp::notify_tile_status                   */
/*****************************************************************************/

void
  kd_global_rescomp::notify_tile_status(kdu_dims tile_dims,
                                        bool uses_this_resolution)
{
  kdu_coords min = tile_dims.pos;
  kdu_coords lim = min + tile_dims.size;
  kd_comp_info *ci = codestream->comp_info + comp_idx;
  min.x = ceil_ratio(min.x,ci->sub_sampling.x);
  min.y = ceil_ratio(min.y,ci->sub_sampling.y);
  lim.x = ceil_ratio(lim.x,ci->sub_sampling.x);
  lim.y = ceil_ratio(lim.y,ci->sub_sampling.y);
  min.x = 1 + ((min.x-1)>>(ci->hor_depth[depth]));
  min.y = 1 + ((min.y-1)>>(ci->vert_depth[depth]));
  lim.x = 1 + ((lim.x-1)>>(ci->hor_depth[depth]));
  lim.y = 1 + ((lim.y-1)>>(ci->vert_depth[depth]));
  kdu_long tile_area = ((kdu_long)(lim.x-min.x)) * ((kdu_long)(lim.y-min.y));
  area_covered_by_tiles += tile_area;
  if (uses_this_resolution)
    area_used_by_tiles += tile_area;
  else
    remaining_area -= tile_area;
  assert((area_covered_by_tiles <= total_area) && (remaining_area >= 0));
  ready_fraction = reciprocal_fraction = -1.0;
}

/*****************************************************************************/
/*                   kd_global_rescomp::add_ready_precinct                   */
/*****************************************************************************/

void
  kd_global_rescomp::add_ready_precinct(kd_precinct *precinct)
{
  // Add to ready list
  assert((precinct->prev == NULL) && (precinct->next == NULL) &&
         (precinct != first_ready));
  if ((precinct->prev = last_ready) == NULL)
    first_ready = last_ready = precinct;
  else
    last_ready = last_ready->next = precinct;
  
  // Calculate area
  kd_resolution *res = precinct->resolution;
  int p_idx = (int)(precinct->ref - res->precinct_refs);
  kdu_coords idx;
  idx.y = p_idx / res->precinct_indices.size.x;
  idx.x = p_idx - idx.y*res->precinct_indices.size.x;
  idx += res->precinct_indices.pos;
  kdu_dims dims = res->precinct_partition;
  dims.pos.x += dims.size.x*idx.x;
  dims.pos.y += dims.size.y*idx.y;
  dims &= res->node.dims;
  ready_area += dims.area();
  ready_fraction = reciprocal_fraction = -1.0;
}

/*****************************************************************************/
/*                  kd_global_rescomp::close_ready_precinct                  */
/*****************************************************************************/

void
  kd_global_rescomp::close_ready_precinct(kd_precinct *precinct)
{
  // Remove from ready list
  if (precinct->prev == NULL)
    {
      assert(precinct == first_ready);
      first_ready = precinct->next;
    }
  else
    precinct->prev->next = precinct->next;
  if (precinct->next == NULL)
    {
      assert(precinct == last_ready);
      last_ready = precinct->prev;
    }
  else
    precinct->next->prev = precinct->prev;
  precinct->prev = precinct->next = NULL;
  
  // Calculate area
  kd_resolution *res = precinct->resolution;
  int p_idx = (int)(precinct->ref - res->precinct_refs);
  kdu_coords idx;
  idx.y = p_idx / res->precinct_indices.size.x;
  idx.x = p_idx - idx.y*res->precinct_indices.size.x;
  idx += res->precinct_indices.pos;
  kdu_dims dims = res->precinct_partition;
  dims.pos.x += dims.size.x*idx.x;
  dims.pos.y += dims.size.y*idx.y;
  dims &= res->node.dims;
  kdu_long area = dims.area();
  ready_area -= area;
  remaining_area -= area;
  ready_fraction = reciprocal_fraction = -1.0;

  // Close the precinct itself
  precinct->ref->close();
}


/* ========================================================================= */
/*                           kd_codestream_comment                           */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_codestream_comment::init                        */
/*****************************************************************************/

void
  kd_codestream_comment::init(int length, kdu_byte *data, bool is_text)
{
  assert(!readonly);
  readonly = true;
  this->is_text = is_text;
  this->is_binary = !is_text;
  if (length <= 0)
    {
      this->num_bytes = length = 0;
      if (!is_text)
        return;
    }
  if (max_bytes <= length)
    {
      int new_max_bytes = length+1;
      kdu_byte *new_buf = new kdu_byte[new_max_bytes];
      if (buf != NULL)
        { delete[] buf; buf = NULL; }
      buf = new_buf;
      max_bytes = new_max_bytes;
    }
  memcpy(buf,data,(size_t) length);
  if (is_text && ((length == 0) || (buf[length-1] != 0)))
    buf[length++] = 0;
  num_bytes = length;
}

/*****************************************************************************/
/*                    kd_codestream_comment::write_marker                    */
/*****************************************************************************/

int
  kd_codestream_comment::write_marker(kdu_output *out, int force_length)
{
  readonly = true;
  int pad_bytes = 0;
  int write_bytes = num_bytes;
  if ((write_bytes > 0) && is_text)
    write_bytes--; // Don't write the terminating null character
  if ((force_length <= 0) && (write_bytes > 0xFFFF))
    force_length = 0xFFFF;
  if (force_length > 0)
    {
      force_length -= 6;
      if (force_length < 0)
        force_length = 0;
      if (force_length < write_bytes)
        { // Truncate the output buffer
          write_bytes = num_bytes = force_length;
          if (is_text)
            buf[num_bytes++] = 0;
        }
      else
        pad_bytes = force_length - write_bytes;
    }
  if (out != NULL)
    {
      out->put(KDU_COM);
      out->put((kdu_uint16)(4+write_bytes+pad_bytes));
      out->put((kdu_uint16)((is_text)?1:0)); // Latin values
      out->write(buf,write_bytes);
      for (int i=0; i < pad_bytes; i++)
        out->put((kdu_byte) 0);
    }
  return 6+write_bytes+pad_bytes;
}


/* ========================================================================= */
/*                           kdu_codestream_comment                          */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdu_codestream_comment::get_text                      */
/*****************************************************************************/

const char *
  kdu_codestream_comment::get_text()
{
  if (state == NULL)
    return NULL;
  if ((state->buf == NULL) || !state->is_text)
    return "";
  return (const char *)(state->buf);
}

/*****************************************************************************/
/*                     kdu_codestream_comment::get_data                      */
/*****************************************************************************/

int
  kdu_codestream_comment::get_data(kdu_byte buf[], int offset, int length)
{
  int xfer_bytes = state->num_bytes - offset;
  if (xfer_bytes <= 0)
    return 0;
  if (xfer_bytes > length)
    xfer_bytes = length;
  if (buf != NULL)
    memcpy(buf,state->buf,(size_t) xfer_bytes);
  return xfer_bytes;
}

/*****************************************************************************/
/*                  kdu_codestream_comment::check_readonly                   */
/*****************************************************************************/

bool
  kdu_codestream_comment::check_readonly()
{
  if (state == NULL)
    return true;
  else
    return state->readonly;
}

/*****************************************************************************/
/*                     kdu_codestream_comment::put_data                      */
/*****************************************************************************/

bool
  kdu_codestream_comment::put_data(const kdu_byte data[], int num_bytes)
{
  if ((state == NULL) || state->readonly || state->is_text)
    return false;
  state->is_binary = true;
  int len = state->num_bytes + num_bytes;
  if (len > 65531)
    { KDU_WARNING(w,0x30050901); w <<
      KDU_TXT("Call to `kdu_codestream_comment::put_data' leaves the total "
              "length of the codestream comment greater than 65531, which "
              "is the longest comment that can be represented in a COM "
              "marker segment in the codestream.  Comment is being "
              "truncated.");
      len = 65531;
    }
  if (len > state->max_bytes)
    {
      int new_max_bytes = len + state->max_bytes; 
      if (new_max_bytes > 65531)
        new_max_bytes = 65531;
      assert(new_max_bytes >= len);
      kdu_byte *new_buf = new kdu_byte[new_max_bytes];
      if (state->buf == NULL)
        new_buf[0] = 0;
      else
        {
          memcpy(new_buf,state->buf,(size_t)(state->num_bytes));
          delete[] state->buf;
        }
      state->max_bytes = new_max_bytes;
      state->buf = new_buf;
    }
  if (len > state->num_bytes)
    memcpy(state->buf,data,(size_t)(len-state->num_bytes));
  state->num_bytes = len;  
  return true;
}

/*****************************************************************************/
/*                     kdu_codestream_comment::put_text                      */
/*****************************************************************************/

bool
  kdu_codestream_comment::put_text(const char *string)
{
  if ((state == NULL) || state->readonly || state->is_binary)
    return false;
  state->is_text = true;
  int len = state->num_bytes + (int) strlen(string);
  if (state->num_bytes == 0)
    len++; // Leave space for the terminating null character
  if ((len-1) > 65531)
    { KDU_WARNING(w,0x06110801); w <<
      KDU_TXT("Call to `kdu_codestream_comment::put_text' leaves the total "
              "length of the codestream comment greater than 65531, which "
              "is the longest comment that can be represented in a COM "
              "marker segment in the codestream.  Comment is being "
              "truncated.");
      len = 65531+1;
    }
  if (len > state->max_bytes)
    {
      int new_max_bytes = len + state->max_bytes; 
      if (new_max_bytes > 65532)
        new_max_bytes = 65532;
      assert(new_max_bytes >= len);
      kdu_byte *new_buf = new kdu_byte[new_max_bytes];
      if (state->buf == NULL)
        new_buf[0] = 0;
      else
        {
          memcpy(new_buf,state->buf,(size_t)(state->num_bytes));
          delete[] state->buf;
        }
      state->max_bytes = new_max_bytes;
      state->buf = new_buf;
    }
  if (len > state->num_bytes)
    strncat((char *) state->buf,string,(size_t)(len-state->num_bytes));
  state->num_bytes = len;
  return true;
}


/* ========================================================================= */
/*                                kd_mct_stage                               */
/* ========================================================================= */

/*****************************************************************************/
/*                         kd_mct_stage::create_stages                       */
/*****************************************************************************/

void
  kd_mct_stage::create_stages(kd_mct_stage * &head,
                              kd_mct_stage * &tail,
                              kdu_params *root, int tnum,
                              int num_input_components,
                              kd_comp_info *input_comp_info,
                              int num_output_components,
                              kd_output_comp_info *output_comp_info)
{
  head = tail = NULL;

  // Start by accessing the MCO object which describes the transform stages
  kdu_params *cod, *mco, *mco_root = root->access_cluster(MCO_params);
  int num_stages = 0;
  if ((mco_root == NULL) ||
      ((mco = mco_root->access_relation(tnum,-1,0,true)) == NULL) ||
      (!mco->get(Mnum_stages,0,0,num_stages)) || (num_stages == 0) ||
      ((cod = root->access_cluster(COD_params)) == NULL) ||
      ((cod = cod->access_relation(tnum,-1,0,true)) == NULL))
    return;
  int mct_flags = 0;  cod->get(Cmct,0,0,mct_flags);

  // Now walk through the stages
  int stage_num, b, n, m;
  kdu_params *mcc, *mcc_root = root->access_cluster(MCC_params);
  kdu_params *mct_root = root->access_cluster(MCT_params);
  for (stage_num=0; stage_num < num_stages; stage_num++)
    {
      int mcc_idx, i_val;
      if ((!mco->get(Mstages,stage_num,0,mcc_idx)) || (mcc_root == NULL) ||
          ((mcc = mcc_root->access_relation(tnum,-1,mcc_idx,true)) == NULL) ||
          !(mcc->get(Mstage_blocks,0,0,i_val) &&
            mcc->get(Mstage_blocks,0,1,i_val)))
        { KDU_ERROR(e,0x08080502); e <<
            KDU_TXT("Unable to access the description of stage ")
            << stage_num <<
            KDU_TXT(" (starting from 0) in the multi-component transform for "
            "some tile (or in the main header).  Either the `Mstages' "
            "attribute contains insufficient entries, or no "
            "`Mstage_blocks' attribute exists with the instance "
            "identifier provided by `Mstages'.");
        }
      kd_mct_stage *stage = new kd_mct_stage;
      stage->prev_stage = tail;
      if (tail == NULL)
        head = tail = stage;
      else
        tail = tail->next_stage = stage;

      for (b=1; mcc->get(Mstage_blocks,b,0,i_val) &&
                mcc->get(Mstage_blocks,b,1,i_val); b++);
      stage->num_blocks = b;
      stage->blocks = new kd_mct_block[stage->num_blocks];
      
      int in_from=0, in_to=-1, in_range_idx=0;
      int out_from=0, out_to=-1, out_range_idx=0;
      for (b=0; b < stage->num_blocks; b++)
        {
          kd_mct_block *block = stage->blocks + b;
          block->stage = stage;
          mcc->get(Mstage_blocks,b,0,block->num_inputs);
          mcc->get(Mstage_blocks,b,1,block->num_outputs);
          if ((block->num_inputs < 1) || (block->num_outputs < 1))
            { KDU_ERROR(e,0x08080503); e <<
                KDU_TXT("Multi-component transform blocks must each have "
                "a strictly positive number of inputs and outputs, as "
                "identified by the `Mstage_blocks' parameter attribute.");
            }
          if ((block->num_inputs > 16384) || (block->num_outputs > 16384))
            { KDU_ERROR(e,0x06110802); e <<
              KDU_TXT("Multi-component transform block identifies more "
                      "input components or more output components than "
                      "the maximum number of image components in a "
                      "codestream.  Seems like an error!");
            }
          block->input_indices = new int[block->num_inputs];
          block->output_indices = new int[block->num_outputs];
          for (n=0; n < block->num_inputs; n++)
            {
              if (in_to < in_from)
                {
                  if (!(mcc->get(Mstage_inputs,in_range_idx,0,in_from) &&
                        mcc->get(Mstage_inputs,in_range_idx,1,in_to)))
                    { KDU_ERROR(e,0x16110901); e <<
                      KDU_TXT("Insufficient input ranges supplied with"
                              "`Mstage_inputs' codestream attribute to "
                              "accommodate the dimensions supplied by an "
                              "`Mstage_collections' attribute.");
                    }
                  if (!((in_to >= in_from) && (in_from >= 0)))
                    assert(0); // Should be caught by `mcc_params::finalize'
                  in_range_idx++;
                }
              block->input_indices[n] = in_from;
              if (in_from >= stage->num_inputs)
                stage->num_inputs = in_from+1;
              in_from++;
            }
          for (m=0; m < block->num_outputs; m++)
            {
              if (out_to < out_from)
                {
                  if (!(mcc->get(Mstage_outputs,out_range_idx,0,out_from) &&
                        mcc->get(Mstage_outputs,out_range_idx,1,out_to)))
                    { KDU_ERROR(e,0x16110902); e <<
                      KDU_TXT("Insufficient output ranges supplied with"
                              "`Mstage_inputs' codestream attribute to "
                              "accommodate the dimensions supplied by an "
                              "`Mstage_collections' attribute.");
                    }                  
                  if (!((out_to >= out_from) && (out_from >= 0)))
                    assert(0); // Should be caught by `mcc_params::finalize'
                  out_range_idx++;
                }
              block->output_indices[m] = out_from;
              if (out_from >= stage->num_outputs)
                stage->num_outputs = out_from+1;
              out_from++;
            }

          block->is_null_transform = true; // Until proven otherwise

          if (tnum < 0)
            continue; // No need to access the transform coefficients or even
                      // the transform type unless we are in a tile.

          int block_type, xform_idx, off_idx, aux1, aux2;
          if (!(mcc->get(Mstage_xforms,b,0,block_type) &&
                mcc->get(Mstage_xforms,b,1,xform_idx) &&
                mcc->get(Mstage_xforms,b,2,off_idx) &&
                mcc->get(Mstage_xforms,b,3,aux1) &&
                mcc->get(Mstage_xforms,b,4,aux2)))
            assert(0); // Should have been caught by `mcc_params::finalize'

          if ((block_type == Mxform_MATRIX) && (mct_flags == 0) && (aux1 != 0))
            mcc->set(Mstage_xforms,b,0,block_type=Mxform_MAT);

          if ((off_idx != 0) &&
              ((mct_root == NULL) ||
               ((block->offset_params =
                 mct_root->access_relation(tnum,-1,off_idx,true))==NULL) ||
               (!block->offset_params->get(Mvector_size,0,0,i_val)) ||
               (i_val != block->num_outputs)))
            { KDU_ERROR(e,0x08080504); e <<
                KDU_TXT("Unable to access the offset parameters "
                "identified by an `Mstage_xforms' parameter attribute.  "
                "Specifically, there should be an `Mvector_size' "
                "attribute, with instance index ")
                << off_idx <<
                KDU_TXT(" and value ")
                << block->num_outputs <<
                KDU_TXT(", corresponding to the number of output "
                "components produced by the multi-component transform "
                "block in question.");
            }

          if ((block->num_inputs != block->num_outputs) &&
              (((block_type != Mxform_MATRIX) && (block_type != Mxform_MAT)) ||
               (aux1 != 0)))
            { KDU_ERROR(e,0x08080505); e <<
                KDU_TXT("Except for irreversible matrix-based decorrelation "
                "transforms, all multi-component transform blocks described "
                "by the `Mstage_blocks' and `Mstage_xforms' parameter "
                "attributes must produce exactly one output component "
                "for each of their input components.");
            }

          if ((block_type == Mxform_MATRIX) || (block_type == Mxform_MAT))
            {
              block->is_reversible = (aux1 != 0);
              int mat_coeffs = block->num_inputs * block->num_outputs;
              if (block->is_reversible)
                mat_coeffs += block->num_inputs;
              if ((xform_idx != 0) &&
                  ((mct_root == NULL) ||
                   ((block->matrix_params =
                     mct_root->access_relation(tnum,-1,xform_idx,true))
                    == NULL) ||
                   (!block->matrix_params->get(Mmatrix_size,0,0,i_val)) ||
                   (i_val != mat_coeffs)))
                { KDU_ERROR(e,0x08080506); e <<
                    KDU_TXT("Unable to access the matrix parameters "
                    "identified by an `Mstage_xforms' parameter attribute.  "
                    "Specifically, there should be an `Mmatrix_size' "
                    "attribute, with instance index ")
                    << xform_idx <<
                    KDU_TXT(" and value ")
                    << mat_coeffs <<
                    KDU_TXT(", corresponding to the total number of "
                    "coefficients required to describe the irreversible "
                    "decorrelation transform or reversible SERM transform "
                    "in question -- note that reversible transforms are "
                    "described by M*(M+1) coefficients, where M is the "
                    "number of input (or output) components.");
                }
              if (block->matrix_params != NULL)
                block->is_null_transform = false;
              if ((block_type == Mxform_MAT) && block->is_reversible)
                {
                  block->old_mat_params = block->matrix_params;
                  block->matrix_params = NULL;
                }
            }
          else if (block_type == Mxform_DEP)
            {
              block->is_reversible = (aux1 != 0);
              int tri_coeffs = (block->num_outputs*(block->num_outputs-1)) / 2;
              if (block->is_reversible)
                tri_coeffs += block->num_outputs-1;
              if ((xform_idx != 0) &&
                  ((mct_root == NULL) ||
                   ((block->triang_params =
                     mct_root->access_relation(tnum,-1,xform_idx,true))
                    == NULL) ||
                   (!block->triang_params->get(Mtriang_size,0,0,i_val)) ||
                   (i_val != tri_coeffs)))
                { KDU_ERROR(e,0x08080507); e <<
                    KDU_TXT("Unable to access the matrix parameters "
                    "identified by an `Mstage_xforms' parameter attribute.  "
                    "Specifically, there should be an `Mtriang_size' "
                    "attribute, with instance index ")
                    << xform_idx <<
                    KDU_TXT(" and value ")
                    << tri_coeffs <<
                    KDU_TXT(", corresponding to the total number of "
                    "coefficients in the lower triangular matrix which "
                    "describes the dependency transform in question.  For "
                    "irreversible dependency transforms, the matrix should "
                    "not contain any diagonal entries.  For reversible "
                    "dependency transforms, however, all but the first "
                    "diagonal entries should also be included, as "
                    "normalization factors for the integer predictors.");
                }
              if (block->triang_params != NULL)
                block->is_null_transform = false;
            }
          else if ((block_type == Mxform_DWT) && (aux1 > 0))
            {
              block->is_null_transform = false;
              block->dwt_num_levels = aux1;
              block->dwt_canvas_origin = aux2;
              int kernel_id = (xform_idx < 2)?xform_idx:Ckernels_ATK;
              kd_create_dwt_description(kernel_id,xform_idx,root,tnum,
                        block->is_reversible,block->dwt_symmetric,
                        block->dwt_symmetric_extension,block->dwt_num_steps,
                        block->dwt_step_info,block->dwt_coefficients);
              kdu_kernels kernels;
              kernels.init(block->dwt_num_steps,block->dwt_step_info,
                           block->dwt_coefficients,block->dwt_symmetric,
                           block->dwt_symmetric_extension,
                           block->is_reversible);
              int low_hlen, high_hlen, nstps;
              kernels.get_impulse_response(KDU_SYNTHESIS_LOW,low_hlen,
                                           &(block->dwt_low_synth_min),
                                           &(block->dwt_low_synth_max));
              kernels.get_impulse_response(KDU_SYNTHESIS_HIGH,high_hlen,
                                           &(block->dwt_high_synth_min),
                                           &(block->dwt_high_synth_max));
              if (block->is_reversible)
                block->dwt_synth_gains[0] = block->dwt_synth_gains[1] = 1.0F;
              else
                {
                  kernels.get_lifting_factors(nstps,block->dwt_synth_gains[0],
                                              block->dwt_synth_gains[1]);
                  assert(nstps == block->dwt_num_steps);
                  block->dwt_synth_gains[0] = 1.0F / block->dwt_synth_gains[0];
                  block->dwt_synth_gains[1] = 0.5F / block->dwt_synth_gains[1];
                }
            }
        }

      // Once we get here, all the transform blocks have been parsed and
      // we know the value of `stage->num_inputs' and
      // `stage->num_outputs'.  Now we can allocate the input and
      // output information arrays and fill in their contents.
      if ((stage->num_inputs > 65535) || (stage->num_outputs > 65535))
        { KDU_ERROR(e,0x06110803); e <<
          KDU_TXT("Multi-component transform stage appears to have a "
                  "ridiculous number of component inputs or outputs -- "
                  "greater than 65535!  Treating this as an error.");
        }
      if ((stage->prev_stage != NULL) &&
          (stage->num_inputs > stage->prev_stage->num_outputs))
        { // It is convenient to augment the previous stage's number of
          // outputs to match the current stage's number of inputs.  Simplifies
          // processing.
          kd_output_comp_info *new_outputs =
            new kd_output_comp_info[stage->num_inputs];
          for (n=0; n < stage->prev_stage->num_outputs; n++)
            new_outputs[n] = stage->prev_stage->output_comp_info[n];
          stage->prev_stage->num_outputs = stage->num_inputs;
          delete[] stage->prev_stage->output_comp_info;
          stage->prev_stage->output_comp_info = new_outputs;
        }

      stage->input_required_indices = new int[stage->num_inputs];
      for (n=0; n < stage->num_inputs; n++)
        stage->input_required_indices[n] = -1; // Change this later
      if ((stage_num == (num_stages-1)) &&
          (stage->num_outputs < num_output_components))
        stage->num_outputs = num_output_components;
      stage->output_comp_info = new kd_output_comp_info[stage->num_outputs];
      for (b=0; b < stage->num_blocks; b++)
        {
          kd_mct_block *block = stage->blocks + b;
          block->inputs_required = new bool[block->num_inputs];
          for (n=0; n < block->num_inputs; n++)
            {
              block->inputs_required[n] = false; // Change this later
              stage->input_required_indices[block->input_indices[n]] =
                block->input_indices[n];
                // Above statement lets us check that all inputs are "touched",
                // as required by the standard.
            }
          kd_comp_info *last_ref = NULL;
             // Use this to provide a `subsampling_ref' value for output
             // components whose input component is not available.
          for (n=0; n < block->num_outputs; n++)
            {
              kd_output_comp_info *oci =
                stage->output_comp_info + block->output_indices[n];
              if (oci->block != NULL)
                { KDU_ERROR(e,0x09080500); e <<
                    KDU_TXT("Multi-component transform stage contains "
                    "multiple transform blocks which provide different "
                    "definitions for the same stage output component.");
                }
              oci->block = block;
              oci->block_comp_idx = n;
              if (n < block->num_inputs)
                {
                  kd_mct_stage *ps = stage->prev_stage;
                  m = block->input_indices[n];
                  if (ps == NULL)
                    last_ref = input_comp_info + m;
                  else if (ps->output_comp_info[m].subsampling_ref != NULL)
                    last_ref = ps->output_comp_info[m].subsampling_ref;
                }
              oci->subsampling_ref = last_ref;
              if (last_ref != NULL)
                for (m=n-1; m >= 0; m--)
                  {
                    oci = stage->output_comp_info + block->output_indices[m];
                    if (oci->subsampling_ref != NULL)
                      break;
                    oci->subsampling_ref = last_ref;
                  }
            }
        }
      for (n=0; n < stage->num_inputs; n++)
        if (stage->input_required_indices[n] < 0)
          break;
      if ((n < stage->num_inputs) ||
          ((stage->prev_stage == NULL) &&
           (stage->num_inputs != num_input_components)) ||
          ((stage->prev_stage != NULL) &&
           (stage->num_inputs < stage->prev_stage->num_outputs)))
        { KDU_ERROR(e,0x09080501); e <<
            KDU_TXT("Multi-component transform does not satisfy the "
            "constraints imposed by Part 2 of the JPEG2000 standard.  The "
            "first transform stage must touch every codestream image "
            "component (no more and no less), while subsequent stages must "
            "touch every component produced by the previous stage.");
        }
    }
  if (tail->num_outputs > num_output_components)
    { KDU_ERROR(e,0x09080502); e <<
        KDU_TXT("The last stage of the multi-component transform may not "
        "produce more output components than the number specified in the "
        "CBD marker segment (i.e. the `Mcomponents' value).");
    }
  for (n=0; n < tail->num_outputs; n++)
    {
      tail->output_comp_info[n].is_signed = output_comp_info[n].is_signed;
      tail->output_comp_info[n].precision = output_comp_info[n].precision;
      if ((tnum < 0) &&
          (tail->output_comp_info[n].subsampling_ref == NULL))
        { KDU_ERROR(e,0x09080503); e <<
            KDU_TXT("Although not strictly illegal, the present "
            "Part-2 codestream contains insufficient information to "
            "determine the dimensions of all image components output by "
            "the multi-component transform, based on main header marker "
            "segments.  The fact that this is not illegal is almost "
            "certainly an oversight in the development of the Part-2 "
            "syntax, but Kakadu cannot work with such codestreams.");
        }
    }

  tail->apply_output_restrictions(output_comp_info);
}

/*****************************************************************************/
/*                 kd_mct_stage::apply_output_restrictions                   */
/*****************************************************************************/

void
  kd_mct_stage::apply_output_restrictions(
                                  kd_output_comp_info *global_comp_info,
                                  int num_comps_of_interest,
                                  const int *comps_of_interest)
{
  int n, b;

  // Start by identifying the number and order of apparent output components
  num_apparent_outputs = 0;
  if (next_stage == NULL)
    {
      assert(global_comp_info != NULL);
      for (n=0; n < num_outputs; n++)
        {
          kd_output_comp_info *oci = output_comp_info + n;
          oci->apparent_idx = global_comp_info[n].apparent_idx;
          oci->from_apparent = global_comp_info[n].from_apparent;
          oci->is_of_interest = false;
          if (oci->apparent_idx >= 0)
            {
              num_apparent_outputs++;
              if (num_comps_of_interest == 0)
                oci->is_of_interest = true;
              else if (comps_of_interest == NULL)
                oci->is_of_interest =
                  (oci->apparent_idx < num_comps_of_interest);
            }
        }
      if (comps_of_interest != NULL)
        for (n=0; n < num_comps_of_interest; n++)
          {
            int idx = comps_of_interest[n];
            if ((idx < 0) || (idx >= num_apparent_outputs))
              continue;
            idx = output_comp_info[idx].from_apparent;
            kd_output_comp_info *oci = output_comp_info + idx;
            assert(oci->apparent_idx == comps_of_interest[n]);
            oci->is_of_interest = true;
          }
    }
  else
    {
      assert((global_comp_info == NULL) &&
             (num_outputs == next_stage->num_inputs));
      for (n=0; n < num_outputs; n++)
        {
          kd_output_comp_info *oci = output_comp_info + n;
          oci->from_apparent = 0; // So everything at least gets initialized
          if (next_stage->input_required_indices[n] >= 0)
            {
              output_comp_info[num_apparent_outputs].from_apparent = n;
              oci->apparent_idx = num_apparent_outputs;
              num_apparent_outputs++;
              oci->is_of_interest = true;
              assert(next_stage->input_required_indices[n]==oci->apparent_idx);
            }
          else
            {
              oci->apparent_idx = -1; // This one is not apparent
              oci->is_of_interest = false;
            }
        }
      assert(num_apparent_outputs == next_stage->num_required_inputs);
    }

  // Next, scan through the blocks, identifying the required input components
  // and setting the appearance parameters for each block's outputs
  num_required_inputs = 0;
  for (n=0; n < num_inputs; n++)
    input_required_indices[n] = -1; // Initially mark everything not required
  for (b=0; b < num_blocks; b++)
    {
      kd_mct_block *block = blocks + b;

      block->num_required_inputs = 0;
      for (n=0; n < block->num_inputs; n++)
        block->inputs_required[n] = false;
      block->num_apparent_outputs = 0;
      for (n=0; n < block->num_outputs; n++)
        {
          kd_output_comp_info *oci = output_comp_info+block->output_indices[n];
          if (!oci->is_of_interest)
            continue; // This output is not required
          assert((oci->apparent_idx >= 0) && (oci->block == block));
          oci->apparent_block_comp_idx = block->num_apparent_outputs;
          block->num_apparent_outputs++;
        }
      if (block->num_apparent_outputs == 0)
        continue; // This entire transform block is not required.
      if ((block->num_apparent_outputs == block->num_inputs) ||
          ((block->matrix_params != NULL) || (block->old_mat_params != NULL)))
        { // Require all inputs
          for (n=0; n < block->num_inputs; n++)
            {
              block->inputs_required[n] = true;
              block->num_required_inputs++;
            }
        }
      else if (block->triang_params != NULL)
        { // Require a leading set of the inputs
          for (n=0; n < block->num_outputs; n++)
            if (output_comp_info[block->output_indices[n]].is_of_interest)
              block->num_required_inputs = n+1;
          for (n=0; n < block->num_required_inputs; n++)
            block->inputs_required[n] = true;
        }
      else if (block->is_null_transform)
        { // Require only those inputs which correspond to required outputs
          for (n=0; n < block->num_outputs; n++)
            if ((n < block->num_inputs) &&
                output_comp_info[block->output_indices[n]].is_of_interest)
              {
                block->inputs_required[n] = true;                
                block->num_required_inputs++;
              }
        }
      else
        { // We have a DWT transform block, at least some of whose outputs
          // are not required.  Deducing the required set of inputs
          // in this case is a bit more difficult.
          int length = block->num_inputs;
          if (block->scratch == NULL)
            block->scratch = new float[length];
          bool *used = (bool *)(block->scratch);
          bool *expanded_scratch = used + length;
          for (n=0; n < length; n++)
            used[n]=output_comp_info[block->output_indices[n]].is_of_interest;
          int c_min=block->dwt_canvas_origin;
          int lev_min=c_min, lev_lim=c_min+length;
          int unset_inputs=length; // Decreases as we process levels
          for (int lev=0; lev < block->dwt_num_levels; lev++)
            {
              used -= lev_min; // So we can use absolute indices
              bool *expanded = expanded_scratch - lev_min;

              int band_min, band_max, k, c;
              // Map the components used at this level from `used' into the
              // set of components which are required in each of the subbands,
              // storing the result in `expanded'.
              for (n=lev_min; n < lev_lim; n++)
                expanded[n] = false;
              for (n=lev_min; n < lev_lim; n++)
                if (used[n])
                  { // Expand this one into its required subbands
                    // Start with low-pass subband
                    band_min = n - block->dwt_low_synth_max;
                    band_max = n - block->dwt_low_synth_min;
                    band_min += (band_min & 1);  band_max -= (band_max & 1);
                    for (k=band_min; k <= band_max; k+=2)
                      {
                        for (c=k; (c < lev_min) || (c >= lev_lim); )
                          if (c < lev_min)
                            c = 2*lev_min - c;
                          else
                            c = 2*(lev_lim-1) - c;
                        expanded[c] = true;
                      }

                    // Now for the high-pass subband
                    band_min = n - block->dwt_high_synth_max;
                    band_max = n - block->dwt_high_synth_min;
                    band_min += 1-(band_min & 1);  band_max -= 1-(band_max&1);
                    for (k=band_min; k <= band_max; k+=2)
                      {
                        for (c=k; (c < lev_min) || (c >= lev_lim); )
                          if (c < lev_min)
                            c = 2*lev_min - c;
                          else
                            c = 2*(lev_lim-1) - c;
                        expanded[c] = true;
                      }
                  }

              // Transfer high-pass subband entries to the end of the
              // `inputs_required' array.
              band_min = lev_min + 1-(lev_min & 1);
              band_max = (lev_lim-1) - (lev_lim & 1);
              for (n=band_max; n >= band_min; n-=2)
                {
                  assert(unset_inputs > 0);
                  block->inputs_required[--unset_inputs] = expanded[n];
                  block->num_required_inputs += (expanded[n])?1:0;
                }

              // Transfer low-pass subband entries from `expanded' to `used'
              used += lev_min;     // Restore to true array base
              lev_min = (lev_min+1)>>1;
              lev_lim = (lev_lim+1)>>1;
              for (n=lev_min; n < lev_lim; n++)
                used[n-lev_min] = expanded[2*n];
            }

          // At this point we have only to write the low-pass subband flags out
          // to the remaining inputs
          used -= lev_min;
          for (n=lev_lim-1; n >= lev_min; n--)
            {
              assert(unset_inputs > 0);
              block->inputs_required[--unset_inputs] = used[n];
              block->num_required_inputs += (used[n])?1:0;
            }
          assert(unset_inputs == 0);
        }

      // Now transfer the required input flags from `block' to the stage
      for (n=0; n < block->num_inputs; n++)
        if (block->inputs_required[n])
          input_required_indices[block->input_indices[n]] = 0;
                     // We use 0 to temporarily mark the component as required
    }

  // Now enter the correct ordinal values in all non-negative entries of the
  // `input_required_indices' array.
  assert(num_required_inputs == 0);
  for (n=0; n < num_inputs; n++)
    if (input_required_indices[n] >= 0)
      input_required_indices[n] = num_required_inputs++;

  // Recursively invoke the function on the previous stage, if any
  if (prev_stage != NULL)
    prev_stage->apply_output_restrictions(NULL);
}


/* ========================================================================= */
/*                                kd_mct_block                               */
/* ========================================================================= */

/*****************************************************************************/
/*                      kd_mct_block::analyze_sensitivity                    */
/*****************************************************************************/

void
  kd_mct_block::analyze_sensitivity(int which_input, float input_weight,
                                    int &min_output_idx, int &max_output_idx,
                                    bool restrict_to_interest)
{
  if (is_null_transform)
    {
      int out_idx = output_indices[which_input];
      kd_output_comp_info *oci = stage->output_comp_info+out_idx;
      if (oci->is_of_interest || !restrict_to_interest)
        {
          if (min_output_idx > max_output_idx)
            {
              min_output_idx = max_output_idx = out_idx;
              oci->ss_tmp = 0.0F;
            }
          else
            {
              while (min_output_idx > out_idx)
                stage->output_comp_info[--min_output_idx].ss_tmp = 0.0F;
              while (max_output_idx < out_idx)
                stage->output_comp_info[++max_output_idx].ss_tmp = 0.0F;
            }
          oci->ss_tmp += input_weight;
        }
      return;
    }

  if (ss_models == NULL)
    {
      ss_models = new kd_mct_ss_model[num_inputs];
      if ((matrix_params != NULL) && !is_reversible)
        create_matrix_ss_model();
      else if (matrix_params != NULL)
        create_rxform_ss_model();
      else if (old_mat_params != NULL)
        create_old_rxform_ss_model();
      else if (triang_params != NULL)
        create_dependency_ss_model();
      else if (dwt_num_levels > 0)
        create_dwt_ss_model();
      else
        assert(0);
    }

  kd_mct_ss_model *model = ss_models+which_input;
  for (kdu_int16 n=0; n < model->range_len; n++)
    {
      int out_idx = output_indices[model->range_min + n];
      kd_output_comp_info *oci = stage->output_comp_info+out_idx;
      if (oci->is_of_interest || !restrict_to_interest)
        {
          if (min_output_idx > max_output_idx)
            {
              min_output_idx = max_output_idx = out_idx;
              oci->ss_tmp = 0.0F;
            }
          else
            {
              while (min_output_idx > out_idx)
                stage->output_comp_info[--min_output_idx].ss_tmp = 0.0F;
              while (max_output_idx < out_idx)
                stage->output_comp_info[++max_output_idx].ss_tmp = 0.0F;
            }
          oci->ss_tmp += input_weight * model->ss_vals[n];
        }
    }
}

/*****************************************************************************/
/*                    kd_mct_block::create_matrix_ss_model                   */
/*****************************************************************************/

void
  kd_mct_block::create_matrix_ss_model()
{
  float *coeffs;
  int n, m, num_coeffs = num_inputs * num_outputs;
  assert(!this->is_reversible);
  kd_mct_ss_model *model = ss_models;
  model->ss_handle = coeffs = new float[num_coeffs];
  for (n=0; n < num_inputs; n++, model++, coeffs+=num_outputs)
    {
      model->ss_vals = coeffs;
      model->range_min = 0;
      model->range_len = (kdu_int16) num_outputs;
      for (m=0; m < num_outputs; m++)
        {
          model->ss_vals[m] = 0.0F; // Avoids any possible failure condition
          matrix_params->get(Mmatrix_coeffs,m*num_inputs+n,0,
                             model->ss_vals[m]);
        }
    }
}

/*****************************************************************************/
/*                    kd_mct_block::create_rxform_ss_model                   */
/*****************************************************************************/

void
  kd_mct_block::create_rxform_ss_model()
{
  float *coeffs;
  int n, m, k, N=num_inputs;  assert(N==num_outputs);
  assert(this->is_reversible);

  // Allocate the storage
  int num_coeffs = (N+1)*N;
  kd_mct_ss_model *model = ss_models;
  model->ss_handle = coeffs = new float[num_coeffs];
  for (n=0; n < num_inputs; n++, model++, coeffs+=N)
    {
      model->ss_vals = coeffs;
      model->range_min = 0;
      model->range_len = (kdu_int16) N;
      for (m=0; m < N; m++)
        model->ss_vals[m] =
          (m==n)?1.0F:0.0F; // Start with inputs mapped to outputs
    }

  // Now generate the model coefficients
  int s; // Stands for the "step"
  for (s=0; s <= N; s++)
    {
      m = (N-1) - ((s==N)?0:s); // Index of the channel affected by this step
      float scale = 1.0F;
      matrix_params->get(Mmatrix_coeffs,s*N+m,0,scale);
      scale = 1.0F / scale;
      float sign = 1.0F;
      if (scale < 0.0F)
        {
          assert(s == N);
          sign = -1.0F;
          scale = -scale;
        }

      for (k=0; k < N; k++)
        { // k is the index of the source channel for this step
          if (k == m)
            continue;
          float val = 0.0F;
          matrix_params->get(Mmatrix_coeffs,s*N+k,0,val);
          val *= scale; // This is the update multiplier
          for (model=ss_models, n=0; n < N; n++, model++)
            { // Find the effect of this step update on each input's model
              model->ss_vals[m] -= val*model->ss_vals[k];
              model->ss_vals[m] *= sign;
            }
        }
    }
}

/*****************************************************************************/
/*                   kd_mct_block::create_old_rxform_ss_model                */
/*****************************************************************************/

void
  kd_mct_block::create_old_rxform_ss_model()
{
  float *coeffs;
  int n, m, k, N=num_inputs;  assert(N==num_outputs);
  assert(this->is_reversible);

  // Allocate the storage
  int num_coeffs = (N+1)*N;
  kd_mct_ss_model *model = ss_models;
  model->ss_handle = coeffs = new float[num_coeffs];
  for (n=0; n < num_inputs; n++, model++, coeffs+=N)
    {
      model->ss_vals = coeffs;
      model->range_min = 0;
      model->range_len = (kdu_int16) N;
      for (m=0; m < N; m++)
        model->ss_vals[m] =
          (m==n)?1.0F:0.0F; // Start with inputs mapped to outputs
    }

  // Now generate the model coefficients
  int s; // Stands for the "step"
  for (s=0; s <= N; s++)
    {
      m = (N-1) - ((s==N)?0:s); // Index of the channel affected by this step
      float scale = 1.0F;
      old_mat_params->get(Mmatrix_coeffs,m*(N+1)+s,0,scale);
      scale = 1.0F / scale;
      float sign = 1.0F;
      if (scale < 0.0F)
        {
          assert(s == N);
          sign = -1.0F;
          scale = -scale;
        }

      for (k=0; k < N; k++)
        { // k is the index of the source channel for this step
          if (k == m)
            continue;
          float val = 0.0F;
          old_mat_params->get(Mmatrix_coeffs,k*(N+1)+s,0,val);
          val *= scale; // This is the update multiplier
          for (model=ss_models, n=0; n < N; n++, model++)
            { // Find the effect of this step update on each input's model
              model->ss_vals[m] -= val*model->ss_vals[k];
              model->ss_vals[m] *= sign;
            }
        }
    }
}

/*****************************************************************************/
/*                 kd_mct_block::create_dependency_ss_model                  */
/*****************************************************************************/

void
  kd_mct_block::create_dependency_ss_model()
{
  float *coeffs;
  int n, m, k, N=num_inputs;  assert(N==num_outputs);

  // Allocate the storage
  int num_coeffs = ((N+1)*N)/2;
  kd_mct_ss_model *model = ss_models;
  model->ss_handle = coeffs = new float[num_coeffs];
  for (n=0; n < num_inputs; n++, model++)
    {
      model->range_min = (kdu_int16) n;
      model->range_len = (kdu_int16)(num_outputs-n);
      model->ss_vals = coeffs;
      coeffs += model->range_len;
    }

  // Now generate the model coefficients
  int row_param_start = 0; // Idx of first entry for the row in `triang_params'
  for (m=0; m < num_outputs; m++)
    {
      for (model=ss_models, n=0; n < m; n++, model++)
        model->ss_vals[m-model->range_min] = 0.0F;
      assert(m == (int) model->range_min);
      model->ss_vals[0] = 1.0F;
      if (m == 0)
        continue;

      float scale = 1.0F;
      if (is_reversible)
        { // Find normalization factor
          triang_params->get(Mtriang_coeffs,row_param_start+m,0,scale);
          scale = 1.0F / scale;
        }
      for (k=0; k < m; k++)
        { // Include contribution from output k to output m
          float val = 0.0F;
          triang_params->get(Mtriang_coeffs,row_param_start+k,0,val);
          val *= scale;
          for (model=ss_models, n=0; n <= k; n++, model++)
            model->ss_vals[m-model->range_min] +=
              val * model->ss_vals[k-model->range_min];
        }
      row_param_start += (is_reversible)?(m+1):m;
    }
}

/*****************************************************************************/
/*                      kd_mct_block::create_dwt_ss_model                    */
/*****************************************************************************/

void
  kd_mct_block::create_dwt_ss_model()
{
  int N=num_inputs;  assert(N==num_outputs);
  if (scratch == NULL)
    scratch = new float[N];
  float *buf = scratch - dwt_canvas_origin; // So we can use absolute addresses

  int band_start_idx = 0;
  for (int lev_idx=dwt_num_levels; lev_idx > 0; lev_idx--)
    {
      int b = (lev_idx==dwt_num_levels)?0:1;
      for (; b < 2; b++)
        {
          int band_min = 1+((dwt_canvas_origin-1-(b<<(lev_idx-1)))>>lev_idx);
          int band_lim = 1+((dwt_canvas_origin+N-1-(b<<(lev_idx-1)))>>lev_idx);
          for (int band_pos=band_min; band_pos < band_lim; band_pos++)
            {
              int n, synth_rdx, range_min, range_max;
              range_min = range_max = (band_pos<<lev_idx) + (b<<(lev_idx-1));
              buf[range_min] = 1.0F;
              for (synth_rdx=lev_idx-1; synth_rdx >= 0; synth_rdx--)
                { // Walk up through the synthesis levels
                  int synth_gap=1<<synth_rdx;// Between synthesized samples
                  int synth_min = 1+((dwt_canvas_origin-1) >> synth_rdx);
                  int synth_max = ((dwt_canvas_origin+N-1) >> synth_rdx);
                  synth_min <<= synth_rdx;  synth_max <<= synth_rdx;
                  assert((range_min >= synth_min) && (range_max <= synth_max));

                  if (synth_min == synth_max)
                    { // Unit length sequence synthesized differently
                      assert(range_min == range_max);
                      if ((b == 1) && (synth_rdx == (lev_idx-1)))
                        buf[range_min] *= 0.5F; // Hi-pass subband divided by 2
                      continue;
                    }

                  // Find even and odd coset bounds for current synth level
                  int bound_min[2], bound_max[2];
                  bound_min[0] = synth_min + (synth_min&synth_gap);
                  bound_min[1] = synth_min + synth_gap - (synth_min&synth_gap);
                  bound_max[0] = synth_max - (synth_max&synth_gap);
                  bound_max[1] = synth_max - synth_gap + (synth_max&synth_gap);

                  // Apply pre-lifting scaling of subband samples, initialize
                  // new subband samples in the range and find initial ranges
                  // for the even and odd cosets
                  int rg_min[2], rg_max[2];
                  if (synth_rdx == (lev_idx-1))
                    {
                      assert(range_min == range_max);
                      buf[range_min] *= dwt_synth_gains[b];
                      rg_min[b] = rg_max[b] = range_min;
                      rg_min[1-b] = 0;  rg_max[1-b] = -1; // Empty range
                    }
                  else
                    {
                      assert(!((range_min|range_max) & synth_gap));
                      for (n=range_min; n <= range_max; n+=2*synth_gap)
                        buf[n] *= dwt_synth_gains[0];
                      for (n=range_min+synth_gap; n<range_max; n+=2*synth_gap)
                        buf[n] = 0.0F;
                      rg_min[0] = range_min;  rg_max[0] = range_max;
                      rg_min[1] = 0;  rg_max[1] = -1; // Range starts empty
                    }

                  // Now work through the lifting steps
                  int step_idx;
                  float *step_coeffs = dwt_coefficients;
                  for (step_idx=0; step_idx < dwt_num_steps; step_idx++)
                    step_coeffs += dwt_step_info[step_idx].support_length;
                  for (step_idx=dwt_num_steps-1; step_idx >= 0; step_idx--)
                    {
                      kdu_kernel_step_info *step = dwt_step_info+step_idx;
                      step_coeffs -= step->support_length;
                      int Ns=step->support_min;
                      int Ps=(Ns+step->support_length)-1;
                      Ns <<= (synth_rdx+1);  Ps <<= (synth_rdx+1);
                      if (step_idx & 1)
                        { Ns += synth_gap;  Ps += synth_gap; }
                      else
                        { Ns -= synth_gap;  Ps -= synth_gap; }
                      bool sym_ext = dwt_symmetric_extension;
                      int update_parity = 1-(step_idx&1);
                      int src_min = rg_min[step_idx&1];
                      int src_max = rg_max[step_idx&1];
                      if (src_min > src_max)
                        continue; // Nothing to do here; source coset is empty

                      int update_min = src_min - Ps;
                      int update_max = src_max - Ns;
                      if ((update_min < bound_min[update_parity]) ||
                          ((2*synth_min -
                            (bound_min[update_parity] + Ns)) >= src_min))
                        update_min = bound_min[update_parity];
                      if ((update_max > bound_max[update_parity]) ||
                          ((2*synth_max -
                            (bound_max[update_parity]+Ps)) <= src_max))
                        update_max = bound_max[update_parity];

                      if (rg_min[update_parity] > rg_max[update_parity])
                        { // Range of update samples is currently empty
                          rg_min[update_parity] = update_min;
                          rg_max[update_parity] = update_max;
                        }
                      else if (update_min < rg_min[update_parity])
                        rg_min[update_parity] = update_min;
                      else if (update_max > rg_max[update_parity])
                        rg_max[update_parity] = update_max;
                      while (range_min > update_min)
                        buf[range_min-=synth_gap] = 0.0F;
                      while (range_max < update_max)
                        buf[range_max+=synth_gap] = 0.0F;

                      int edge_min = (sym_ext)?synth_min:bound_min[step_idx&1];
                      int edge_max = (sym_ext)?synth_max:bound_max[step_idx&1];
                      for (n=update_min; n <= update_max; n+=2*synth_gap)
                        {
                          float *cf = step_coeffs;
                          for (int k=Ns; k <= Ps; k+=2*synth_gap, cf++)
                            {
                              int src_idx = n + k;
                              while ((src_idx<edge_min) || (src_idx>edge_max))
                                if (!sym_ext)
                                  src_idx=(src_idx<edge_min)?edge_min:edge_max;
                                else if (src_idx < edge_min)
                                  src_idx = 2*edge_min - src_idx;
                                else
                                  src_idx = 2*edge_max - src_idx;
                              if ((src_idx >= src_min) && (src_idx <= src_max))
                                buf[n] -= buf[src_idx] * *cf;
                            }
                        }
                    }
                  assert(step_coeffs == dwt_coefficients);
                }

              // Finally transfer the data
              kd_mct_ss_model *model =
                ss_models + band_start_idx + (band_pos-band_min);
              model->range_min = (kdu_int16)(range_min-dwt_canvas_origin);
              model->range_len = (kdu_int16)(range_max+1-range_min);
              model->ss_handle = model->ss_vals = new float[model->range_len];
              for (n=range_min; n <= range_max; n++)
                model->ss_vals[n-range_min] = buf[n];

              // See if we can just replicate this model into subsequent
              // locations within the same subband -- this should be possible
              // so long as the model and its replica do not intersect the
              // boundaries.
              int left_gap = model->range_min;
              int right_gap = N - model->range_min - model->range_len;
              while ((left_gap > 0) && (right_gap > (1<<lev_idx)) &&
                     (band_pos < band_lim))
                {
                  left_gap += 1<<lev_idx;
                  right_gap -= 1<<lev_idx;
                  model[1].ss_vals = model[0].ss_vals;
                  model[1].range_min = (kdu_int16) left_gap;
                  model[1].range_len = model[0].range_len;
                  band_pos++;
                  model++;
                }
            }
          band_start_idx += band_lim-band_min;
        }
    }
  assert(band_start_idx == num_inputs);
}


/* ========================================================================= */
/*                               kd_codestream                               */
/* ========================================================================= */

/*****************************************************************************/
/*                        kd_codestream::construct_common                    */
/*****************************************************************************/

void
 kd_codestream::construct_common()
{
  initial_fragment = final_fragment = true;
  construction_finalized = false;
  fragment_area_fraction = 1.0;

  // Get summary parameters from SIZ object.
  ((kdu_params *) siz)->finalize(out==NULL);
  if (!(siz->get(Sprofile,0,0,profile) &&
        siz->get(Scomponents,0,0,num_components) &&
        siz->get(Ssize,0,0,canvas.size.y) && // Subtract y_pos later
        siz->get(Ssize,0,1,canvas.size.x) && // Subtract x_pos later
        siz->get(Sorigin,0,0,canvas.pos.y) &&
        siz->get(Sorigin,0,1,canvas.pos.x) &&
        siz->get(Stiles,0,0,tile_partition.size.y) &&
        siz->get(Stiles,0,1,tile_partition.size.x) &&
        siz->get(Stile_origin,0,0,tile_partition.pos.y) &&
        siz->get(Stile_origin,0,1,tile_partition.pos.x)))
    assert(0);
  if  (profile == 0)
    next_tnum = 0;
  else
    next_tnum = -1;
  canvas.size -= canvas.pos;
  if ((canvas.size.y <= 0) || (canvas.size.x <= 0) ||
      (tile_partition.pos.x > canvas.pos.x) ||
      (tile_partition.pos.y > canvas.pos.y) ||
      ((tile_partition.pos.x+tile_partition.size.x) <= canvas.pos.x) ||
      ((tile_partition.pos.y+tile_partition.size.y) <= canvas.pos.y))
    { KDU_ERROR(e,26); e <<
        KDU_TXT("Illegal canvas coordinates: the first tile is "
        "required to have a non-empty intersection with the image on the "
        "high resolution grid.");
    }

  // Configure the codestream component structure
  int n;

  if (num_components > 16384)
    { KDU_ERROR(e,0x06110804); e <<
      KDU_TXT("Trying to create a `kdu_codestream' object with more than "
              "16384 image components -- this is the maximum number allowed "
              "by the standard.");
    }
  
  comp_info = new kd_comp_info[num_components];
  for (n=0; n < num_components; n++)
    {
      kd_comp_info *ci = comp_info + n;
      if (!siz->get(Sprecision,n,0,ci->precision))
        { KDU_ERROR(e,27); e <<
            KDU_TXT("No information available concerning component "
            "sample bit-depths (i.e., sample precision).");
        }
      if (!siz->get(Ssigned,n,0,ci->is_signed))
        { KDU_ERROR(e,28); e <<
            KDU_TXT("No information available regarding whether "
            "components are signed or unsigned.");
        }
      if (!(siz->get(Ssampling,n,0,ci->sub_sampling.y) &&
            siz->get(Ssampling,n,1,ci->sub_sampling.x)))
        { KDU_ERROR(e,29); e <<
            KDU_TXT("No information available concerning component "
            "sub-sampling factors.");
        }
      ci->apparent_idx = n;
      ci->from_apparent = ci;
      ci->crg_x = ci->crg_y = 0.0F;
      for (int d=0; d <= 32; d++)
        ci->hor_depth[d] = ci->vert_depth[d] = (kdu_byte) d;
           // This default initialization of the depth values may be
           // changed in `finalize_construction'.
    }

  // Configure tiles
  tiles_in_progress_head = tiles_in_progress_tail = NULL;
  tile_span.y = ceil_ratio(canvas.size.y+canvas.pos.y-tile_partition.pos.y,
                           tile_partition.size.y);
  tile_span.x = ceil_ratio(canvas.size.x+canvas.pos.x-tile_partition.pos.x,
                           tile_partition.size.x);
  int total_tiles = tile_span.x*tile_span.y;
  if ((total_tiles > 65535) || (total_tiles < 0))
    { KDU_ERROR(e,30); e <<
        KDU_TXT("Maximum number of allowable tiles is 65535 for any "
        "JPEG2000 code-stream.  You have managed to exceed this number!!");
    }
  tile_indices.pos = kdu_coords(0,0);
  tile_indices.size = tile_span;
  tile_refs = new kd_tile_ref[total_tiles];
  memset(tile_refs,0,((size_t) total_tiles) * sizeof(kd_tile_ref));

  // Check for profile violations
  if (profile == 0)
    {
      if (((tile_partition.size.x != 128) ||
           (tile_partition.size.y != 128)) && (total_tiles > 1))
        { KDU_WARNING(w,9); w <<
            KDU_TXT("Profile violation detected (code-stream is technically "
            "illegal).  Profile-0 code-streams must either be untiled "
            "or else the tile dimensions must be exactly 128x128.  Try "
            "setting \"Sprofile\" to 1 or 2 or avoid using tiles.");
          profile = 2;
        }
      else if (tile_partition.pos.x || tile_partition.pos.y ||
               canvas.pos.x || canvas.pos.y)
         { KDU_WARNING(w,10); w <<
             KDU_TXT("Profile violation detected (code-stream is technically "
             "illegal).  Profile-0 code-streams must have image and tiling "
             "origins (anchor points) set to zero.  Try setting "
             "\"Sprofile\" to 1 or 2.");
           profile = 2;
         }
       else
         {
           for (n=0; n < num_components; n++)
             {
               kd_comp_info *ci = comp_info + n;
               if (((ci->sub_sampling.x != 1) && (ci->sub_sampling.x != 2) &&
                    (ci->sub_sampling.x != 4)) ||
                   ((ci->sub_sampling.y != 1) && (ci->sub_sampling.y != 2) &&
                    (ci->sub_sampling.y != 4)))
                 break;
             }
           if (n < num_components)
             { KDU_WARNING(w,11); w <<
                 KDU_TXT("Profile violation detected (code-stream is "
                 "technically illegal).  Component sub-sampling factors for "
                 "Profile-0 code-streams are restricted to the values 1, 2 "
                 "and 4.  Try setting \"Sprofile\" to 1 or 2.");
               profile = 2;
             }
         }
    }
  else if (profile == 1)
    { // Check for violations
      if (total_tiles > 1)
        { // The image is tiled
          if (tile_partition.size.x != tile_partition.size.y)
            { KDU_WARNING(w,12); w <<
                KDU_TXT("Profile violation detected (code-stream is "
                "technically illegal).  Profile-1 code-streams must "
                "either be untiled or else the horizontal and vertical "
                "tile dimensions must be identical (square tiles on the "
                "hi-res canvas).  You might like to set \"Sprofile\" "
                "to 2 or avoid using tiles.");
              profile = 2;
            }
          else
            {
              for (n=0; n < num_components; n++)
                {
                  kd_comp_info *ci = comp_info + n;
                  if ((tile_partition.size.x > (ci->sub_sampling.x<<10)) ||
                      (tile_partition.size.y > (ci->sub_sampling.y<<10)))
                    break;
                }
              if (n < num_components)
                { KDU_WARNING(w,13); w <<
                    KDU_TXT("Profile violation detected (code-stream is "
                    "technically illegal).  If a Profile-1 code-stream is "
                    "tiled (has multiple tiles), the width and height of "
                    "its tiles, projected onto any given image component, "
                    "may not exceed 1024.  You might like to set "
                    "\"Sprofile\" to 2 or avoid using tiles.");
                  profile = 2;
                }
            }
        }
    }

  // Build the parameter structure.
  kdu_params *element;

  element = new mct_params;
  element->link(siz,-1,-1,total_tiles,0);

  element = new mcc_params;
  element->link(siz,-1,-1,total_tiles,0);

  element = new mco_params;
  element->link(siz,-1,-1,total_tiles,0);

  element = new atk_params; // Must link this before `cod_params', if at all
  element->link(siz,-1,-1,total_tiles,0);

  element = new cod_params;
  element->link(siz,-1,-1,total_tiles,num_components);

  element = new dfs_params; // Must link this one after `cod_params'
  element->link(siz,-1,-1,0,0);

  element = new ads_params; // Must link this one after `cod_params'
  element->link(siz,-1,-1,total_tiles,0);

  element = new qcd_params; // Must link this one after `cod_params'
  element->link(siz,-1,-1,total_tiles,num_components);

  element = new rgn_params;
  element->link(siz,-1,-1,total_tiles,num_components);

  element = new poc_params;
  element->link(siz,-1,-1,total_tiles,0);

  element = new org_params;
  element->link(siz,-1,-1,total_tiles,0);

  element = new crg_params;
  element->link(siz,-1,-1,0,0);

  // Now set up some common services, which we are sure to need.
  buf_server = new kd_buf_server;
  precinct_server = new kd_precinct_server(buf_server);
  buf_server->attach();
  block = new kdu_block();

  // Now some final initialization steps.
  if (in != NULL)
    {
      if (in->get_capabilities() & KDU_SOURCE_CAP_SEEKABLE)
        tpart_ptr_server = new kd_tpart_pointer_server;
      read_main_header();
    }
  if (output_comp_info == NULL)
    construct_output_comp_info();

  region = canvas;
  discard_levels = 0;
  min_dwt_levels = 100; // Ridiculous value
  max_apparent_layers = 0xFFFF;
  num_apparent_components = num_components;
  max_tile_layers = 1;
  tiles_accessed = false;
  stats = NULL;
  start_time = clock();

  cannot_flip = false; // Until proven otherwise
}

/*****************************************************************************/
/*                  kd_codestream::construct_output_comp_info                */
/*****************************************************************************/

void
  kd_codestream::construct_output_comp_info()
{
  assert(output_comp_info == NULL);

  int n, extension_flags = 0;
  siz->get(Sextensions,0,0,extension_flags);
  uses_mct = ((extension_flags & Sextensions_MCT) != 0);
  bool have_cbd =
    (siz->get(Mcomponents,0,0,num_output_components) &&
     (num_output_components > 0));
  if (have_cbd != uses_mct)
    { KDU_ERROR(e,0x04080500); e <<
        KDU_TXT("The `Mcomponents' parameter attribute must be assigned a "
                "non-zero value if and only if the `MCT' flag is present "
                "in the `Sextensions' attribute.");
    }
  if (!have_cbd)
    num_output_components = num_components;
  else if (num_output_components > 16384)
    { KDU_ERROR(e,0x07110800); e <<
      KDU_TXT("Number of multi-component transform output components "
              "defined by CBD marker segment exceeds the maximum "
              "allowed value of 16384.");
    }
  num_apparent_output_components = num_output_components;
  output_comp_info = new kd_output_comp_info[num_output_components];
  for (n=0; n < num_output_components; n++)
    {
      kd_output_comp_info *oci = output_comp_info + n;
      if (!have_cbd)
        {
          oci->precision = comp_info[n].precision;
          oci->is_signed = comp_info[n].is_signed;
        }
      else if (!(siz->get(Mprecision,n,0,oci->precision) &&
                 siz->get(Msigned,n,0,oci->is_signed)))
        assert(0); // Should have been configured during finalization.
      oci->subsampling_ref = comp_info + n; // May change based on MCT info
      oci->apparent_idx = n;
      oci->from_apparent = n;
      oci->block = NULL;
      oci->block_comp_idx = 0;
      oci->apparent_block_comp_idx = 0;
    }

  component_access_mode = KDU_WANT_OUTPUT_COMPONENTS; // Default access mode
}

/*****************************************************************************/
/*                     kd_codestream::finalize_construction                  */
/*****************************************************************************/

void
  kd_codestream::finalize_construction()
{
  if (output_comp_info == NULL)
    construct_output_comp_info();
  if (construction_finalized)
    return;
  construction_finalized = true;

  int n, c, d;

  // Obtain codestream registration information, if any
  kdu_params *crg = siz->access_cluster(CRG_params);
  for (n=0; n < num_components; n++)
    {
      kd_comp_info *ci = comp_info + n;
      if ((crg == NULL) ||
          (!crg->get(CRGoffset,n,0,ci->crg_y)) ||
          (!crg->get(CRGoffset,n,1,ci->crg_x)))
        ci->crg_x = ci->crg_y = 0.0F;
    }

  // Obtain the downsampling factor structure
  kdu_params *cod = siz->access_cluster(COD_params);
  for (n=0; n < num_components; n++)
    {
      kd_comp_info *ci = comp_info + n;
      kdu_params *coc = cod->access_relation(-1,n,0,true);
      for (d=1; d < 33; d++)
        {
          int decomp=3;
          coc->get(Cdecomp,d-1,0,decomp);
          ci->hor_depth[d] = ci->hor_depth[d-1] + (decomp & 1);
          ci->vert_depth[d] = ci->vert_depth[d-1] + ((decomp >> 1) & 1);
        }
    }
  
  // Configure the resolution-component progress status management system
  if (out != NULL)
    {
      kd_global_rescomp *rc = global_rescomps;
      if (rc == NULL)
        rc = global_rescomps = new kd_global_rescomp[33*num_components];
      for (d=0; d < 33; d++)
        for (c=0; c < num_components; c++, rc++)
          rc->initialize(this,d,c);
    }
  
  // Configure the codestream-level reslength information
  if (out != NULL)
    {
      reslength_constraints_used = reslength_constraints_violated = false;
      if (reslength_checkers == NULL)
        reslength_checkers = new kd_reslength_checker[1+num_components];
      for (c=-1; c < num_components; c++)
        {
          cod_params *coc = (cod_params *) cod->access_unique(-1,c);
          if (reslength_checkers[c+1].init(coc))
            reslength_constraints_used = true;
        }
    }

  if (uses_mct)
    { // Now we have only to build an association between each MCT output
      // component and a corresponding codestream component, so that
      // applications can determine the dimensions of each output component,
      // right from the beginning.  We will base this on an analysis of the
      // main header MCC and MCO marker segments which are available -- if
      // there are none, our assumptions might be wrong, but the error can
      // be caught later on when we come to actually process each tile.
      kd_mct_stage::create_stages(global_mct_head,global_mct_tail,siz,-1,
                                  num_components,comp_info,
                                  num_output_components,output_comp_info);
      if (global_mct_tail != NULL)
        for (n=0; n < num_output_components; n++)
          output_comp_info[n].subsampling_ref =
            global_mct_tail->output_comp_info[n].subsampling_ref;
    }
}

/*****************************************************************************/
/*                     kd_codestream::restrict_to_fragment                   */
/*****************************************************************************/

void
 kd_codestream::restrict_to_fragment(kdu_dims fragment_region,
                                     int fragment_tiles_generated,
                                     kdu_long fragment_tile_bytes_generated)
{
  assert(out != NULL);
  fragment_region &= canvas;
  fragment_area_fraction =
    ((double) fragment_region.area()) / ((double) canvas.area());
  this->prev_tiles_written = fragment_tiles_generated;
  this->prev_tile_bytes_written = fragment_tile_bytes_generated;

  kdu_coords tsize = tile_partition.size;
  kdu_coords min = fragment_region.pos - tile_partition.pos;
  kdu_coords lim = min + fragment_region.size;
  kdu_coords idx_min, idx_lim;
  idx_min.x = min.x / tsize.x;
  idx_min.y = min.y / tsize.y;
  idx_lim.x = 1 + ((lim.x-1) / tsize.x);
  idx_lim.y = 1 + ((lim.y-1) / tsize.y);
  bool last_x = (lim.x == (canvas.pos.x+canvas.size.x));
  bool last_y = (lim.y == (canvas.pos.y+canvas.size.y));
  bool first_x = (min.x == canvas.pos.x);
  bool first_y = (min.y == canvas.pos.y);
  if (!((first_x || (min.x == idx_min.x*tsize.x)) &&
        (first_y || (min.y == idx_min.y*tsize.y))))
    { KDU_ERROR_DEV(e,0x06110901); e <<
      KDU_TXT("The fragment region supplied to `kdu_codestream::create' "
              "is not correctly aligned with its left and upper edges on a "
              "tile boundary (or the image boundary).");
    }
  if (!((last_x || (lim.x == idx_lim.x*tsize.x)) &&
        (last_y || (lim.y == idx_lim.y*tsize.y))))
    { KDU_ERROR_DEV(e,0x06110902); e <<
      KDU_TXT("The fragment region supplied to `kdu_codestream::create' "
              "is not correctly aligned with its right and lower edges on a "
              "tile boundary (or the image boundary).");
    }
  if ((idx_lim.x <= idx_min.x) || (idx_lim.y <= idx_min.y))
    { KDU_ERROR_DEV(e,32); e <<
        KDU_TXT("The fragment region supplied to "
        "`kdu_codestream::create' is empty.");
    }

  kdu_coords num_indices = idx_lim - idx_min;
  int current_fragment_tiles = num_indices.x*num_indices.y;
  assert(current_fragment_tiles > 0);
  int future_fragment_tiles = tile_span.x*tile_span.y -
    (current_fragment_tiles + fragment_tiles_generated);
  if (future_fragment_tiles < 0)
    { KDU_ERROR_DEV(e,33); e <<
        KDU_TXT("The fragment region supplied to "
        "`kdu_codestream::create' represents too many tiles, allowing for "
        "the number of tiles indicated for previously generated fragments.");
    }
  
  initial_fragment = (fragment_tiles_generated == 0);
  final_fragment = (future_fragment_tiles == 0);

  if (num_indices == tile_indices.size)
    assert((idx_min == tile_indices.pos) &&
           (canvas == fragment_region) && initial_fragment && final_fragment);
  else
    {
      delete[] tile_refs;
      tile_refs = NULL;
      tile_indices.pos = idx_min;
      tile_indices.size = num_indices;
      canvas = fragment_region;
      region = canvas;
      tile_refs = new kd_tile_ref[current_fragment_tiles];
      memset(tile_refs,0,(size_t) current_fragment_tiles*sizeof(kd_tile_ref));
    }
}

/*****************************************************************************/
/*                            kd_codestream::restart                         */
/*****************************************************************************/

void
  kd_codestream::restart()
{
  // Remove some uncommon services; this may cost us nothing in efficiency
  if (ppm_markers != NULL)
    delete ppm_markers;
  ppm_markers = NULL;
  if (tpart_ptr_server != NULL)
    delete tpart_ptr_server;
  tpart_ptr_server = NULL;
  if (stats != NULL)
    delete stats;
  stats = NULL;
  tlm_generator.clear();
  construction_finalized = false;

  // Reset rate-control related members
  min_fixed_header_size = min_layer_header_size = 0;
  if (layer_sizes != NULL)
    delete[] layer_sizes;
  if (layer_targets != NULL)
    delete[] layer_targets;
  if (layer_thresholds != NULL)
    delete[] layer_thresholds;
  layer_sizes = layer_targets = NULL;
  layer_thresholds = NULL;
  num_sized_layers = 0;

  // Delete the output component info array
  if (output_comp_info != NULL)
    delete[] output_comp_info;
  output_comp_info = NULL;
  num_output_components = num_apparent_output_components = 0;

  // Restart all existing tiles.
  assert(tile_refs != NULL);
  kdu_coords idx;
  kd_tile_ref *tref = tile_refs;
  for (idx.y=0; idx.y < tile_indices.size.y; idx.y++)
    for (idx.x=0; idx.x < tile_indices.size.x; idx.x++, tref++)
      {
        tref->tpart_head = tref->tpart_tail = NULL;
        if (tref->tile == NULL)
          continue;
        assert(tref->tile != KD_EXPIRED_TILE);
          // In restart mode, no tiles should be allowed to expire
        if (tref->tile->is_open)
          { KDU_ERROR_DEV(e,34); e <<
              KDU_TXT("You must close all open tile interfaces "
              "before calling `kdu_codestream::restart'.");
          }
        tref->tile->restart();
      }

  // Delete all previously released typical tiles
  kd_tile *typ;
  while ((typ=typical_tile_cache) != NULL)
    {
      typical_tile_cache = typ->typical_next;
      assert(typ->tile_ref == NULL);
      delete typ;
    }

  // Now some final initialization steps.
  while ((comtail=comhead) != NULL)
    {
      comhead = comtail->next;
      delete comtail;
    }
  comments_frozen = false;
  header_generated = false;
  header_length = 0;
  siz->clear_marks();
  if (in != NULL)
    read_main_header();
  if (output_comp_info == NULL)
    construct_output_comp_info();
  tiles_accessed = false;
  active_tile = NULL;
  next_sot_address = 0;
  next_tnum = 0;
  num_completed_tparts = 0;
  num_open_tiles = 0;
  textualize_out = NULL;
}

/*****************************************************************************/
/*                 kd_codestream::set_reserved_layer_info_bytes              */
/*****************************************************************************/

void
  kd_codestream::set_reserved_layer_info_bytes(int num_layers)
{
  // First remove any existing layer info comment
  kd_codestream_comment *prev, *scan;
  for (prev=NULL, scan=comhead; scan != NULL; prev=scan, scan=scan->next)
    {
      kdu_codestream_comment com(scan);
      if (strncmp(com.get_text(),
                  "Kdu-Layer-Info: ",strlen("Kdu-Layer-Info: ")) == 0)
        break;
    }
  if (scan != NULL)
    {
      if (prev == NULL)
        comhead = scan->next;
      else
        prev->next = scan->next;
      delete scan;
      if (scan == comtail)
        comtail = prev;
    }

  // Now set aside a sufficient number of bytes
  reserved_layer_info_bytes = 6 + num_layers*17 + (int) strlen(
    "Kdu-Layer-Info: log_2{Delta-D(squared-error)/Delta-L(bytes)}, L(bytes)\n"
    );
}

/*****************************************************************************/
/*                     kd_codestream::gen_layer_info_comment                 */
/*****************************************************************************/

void
  kd_codestream::gen_layer_info_comment(int num_layers, kdu_long *layer_bytes,
                                        kdu_uint16 *thresholds)
{
  if (reserved_layer_info_bytes == 0)
    return;

  kd_codestream_comment *elt = new kd_codestream_comment;
  if (comhead == NULL)
    comhead = comtail = elt;
  else
    comtail = comtail->next = elt;
  kdu_codestream_comment com(elt);
  com <<
    "Kdu-Layer-Info: log_2{Delta-D(squared-error)/Delta-L(bytes)}, L(bytes)\n";
  double size_factor = 1.0 / fragment_area_fraction;
  char line[18];
  for (int n=0; n < num_layers; n++)
    {
      double log_lambda = thresholds[n];
      log_lambda *= 1.0/256.0;
      log_lambda -= 256-64;
      sprintf(line,"%6.1f, %8.1e\n",log_lambda,
              size_factor * ((double) layer_bytes[n]));
      com << line;
    }
  elt->write_marker(NULL,reserved_layer_info_bytes);
      // Truncates or pads the comment marker if length is wrong and sets it
      // to read-only mode.
}

/*****************************************************************************/
/*                        kd_codestream::read_main_header                    */
/*****************************************************************************/

void
  kd_codestream::read_main_header()
{
  if (in == NULL)
    return;
  bool have_tlm = false;
  do {
      if (!marker->read())
        {
          if (in->failed())
            break;
          KDU_ERROR(e,35); e <<
            KDU_TXT("Main code-stream header appears corrupt!");
        }
      if (marker->get_code() == KDU_PPM)
        {
          if (cached)
            { KDU_ERROR_DEV(e,36); e <<
                KDU_TXT("You cannot use PPM or PPT marker "
                "segments (packed packet headers) with cached compressed "
                "data sources.");
            }
          if (profile == 0)
            { KDU_WARNING(w,14);  w <<
                KDU_TXT("Profile violation detected (code-stream is "
                "technically illegal).  PPM marker segments may not "
                "appear within a Profile-0 code-stream.  You should "
                "set \"Sprofile\" to 1 or 2.");
              profile = 2;
            }
          if (ppm_markers == NULL)
            ppm_markers = new kd_pp_markers;
          ppm_markers->add_marker(*marker);
        }
      else if (marker->get_code() == KDU_TLM)
        {
          if (tpart_ptr_server != NULL)
            tpart_ptr_server->add_tlm_marker(*marker);
          have_tlm = true;
        }
      else if (marker->get_code() == KDU_COM)
        {
          int len = marker->get_length();
          kdu_byte *dat = marker->get_bytes();
          if ((len > 2) && (dat[0] == 0) && (dat[1] < 2))
            {
              kd_codestream_comment *new_elt = new kd_codestream_comment;
              if (comtail == NULL)
                comhead = comtail = new_elt;
              else
                comtail = comtail->next = new_elt;
              bool is_text = (dat[1] == 1);
              comtail->init(len-2,dat+2,is_text);
            }
        }
      else
        siz->translate_marker_segment(marker->get_code(),
                                      marker->get_length(),
                                      marker->get_bytes(),-1,0);
    } while (marker->get_code() != KDU_SOT);

  siz->finalize_all(-1,true); // Finalize all main header parameter objects

  if (tpart_ptr_server != NULL)
    {
      if (ppm_markers != NULL)
        {
          delete tpart_ptr_server;
          tpart_ptr_server = NULL;
          if (have_tlm)
            { KDU_WARNING_DEV(w,15); w <<
                KDU_TXT("Dynamic indexing of tile-parts, whether "
                "by TLM (tile-part length) marker segments or otherwise, "
                "cannot be used by the current implementation when PPM "
                "(packed packet header) marker segments are also used.");
            }
        }
      else
        {
          assert(tile_span == tile_indices.size);
          tpart_ptr_server->translate_markers(in->get_bytes_read()-12,
                                              tile_span.x*tile_span.y,
                                              tile_refs);
        }
    }
  
  finalize_construction();
}

/*****************************************************************************/
/*                        kd_codestream::freeze_comments                     */
/*****************************************************************************/

void
  kd_codestream::freeze_comments()
{
  if (comments_frozen)
    return;
#ifdef KDU_IDENTIFIER
  kd_codestream_comment *comscan;
  for (comscan=comhead; comscan != NULL; comscan=comscan->next)
    {
      kdu_codestream_comment com(comscan);
      if (strcmp(com.get_text(),KDU_IDENTIFIER) == 0)
        break;
    }
  if (comscan == NULL)
    {
      comscan = new kd_codestream_comment;
      comscan->init((int) strlen(KDU_IDENTIFIER),(kdu_byte *) KDU_IDENTIFIER,
                    true);
      if (comtail == NULL)
        comhead = comtail = comscan;
      else
        comtail = comtail->next = comscan;
    }
#endif // KDU_IDENTIFIER
  comments_frozen = true;
}

/*****************************************************************************/
/*                        kd_codestream::~kd_codestream                      */
/*****************************************************************************/

kd_codestream::~kd_codestream()
{
  // Delete arrays
  if (comp_info != NULL)
    { delete[] comp_info; comp_info = NULL; }
  if (output_comp_info != NULL)
    { delete[] output_comp_info; output_comp_info = NULL; }
  if (global_rescomps != NULL)
    { delete[] global_rescomps; global_rescomps = NULL; }
  if (reslength_checkers != NULL)
    { delete[] reslength_checkers; reslength_checkers = NULL; }

  active_tile = NULL;
  if (tile_refs != NULL)
    {
      kdu_coords idx, abs_idx;
      kd_tile_ref *tref = tile_refs;
      for (idx.y=0; idx.y < tile_indices.size.y; idx.y++)
        for (idx.x=0; idx.x < tile_indices.size.x; idx.x++, tref++)
          {
            if ((tref->tile != NULL) && (tref->tile != KD_EXPIRED_TILE))
              {
                abs_idx = idx + tile_indices.pos;
                assert(abs_idx == tref->tile->t_idx);
                delete tref->tile; // Invokes tile destructor.
                assert((tref->tile == NULL) ||
                       (tref->tile == KD_EXPIRED_TILE));
              }
        }
      delete[] tile_refs;
    }

  kd_tile *typ;
  while ((typ=typical_tile_cache) != NULL)
    {
      typical_tile_cache = typ->typical_next;
      assert(typ->tile_ref == NULL);
      delete typ;
    }

  // Delete other owned resources
  if (in != NULL)
    { delete in; in = NULL; }
  if (out != NULL)
    { delete out; out = NULL; }
  if (siz != NULL)
    delete siz;
  if (marker != NULL)
    delete marker;
  if (ppm_markers != NULL)
    delete ppm_markers;
  if (precinct_server != NULL)
    delete precinct_server;
  if (block != NULL)
    delete block;
  if (stats != NULL)
    delete stats;
  if (tpart_ptr_server != NULL)
    delete tpart_ptr_server;
  while ((comtail=comhead) != NULL)
    {
      comhead = comtail->next;
      delete comtail;
    }
  if (layer_sizes != NULL)
    delete[] layer_sizes;
  if (layer_targets != NULL)
    delete[] layer_targets;
  if (layer_thresholds != NULL)
    delete[] layer_thresholds;
  if (buf_server != NULL)
    {
      buf_server->detach();
      if (buf_server->can_destroy())
        delete buf_server;
    }
  while ((global_mct_tail=global_mct_head) != NULL)
    {
      global_mct_head = global_mct_tail->next_stage;
      delete global_mct_tail;
    }
}

/*****************************************************************************/
/*                         kd_codestream::create_tile                        */
/*****************************************************************************/

kd_tile *
  kd_codestream::create_tile(kdu_coords idx)
{
  kdu_coords rel_idx = idx - tile_indices.pos;
  assert((rel_idx.x >= 0) && (rel_idx.x < tile_indices.size.x) &&
         (rel_idx.y >= 0) && (rel_idx.y < tile_indices.size.y));
  kd_tile_ref *tref = tile_refs + rel_idx.x + rel_idx.y*tile_indices.size.x;
  kd_tile *tp = tref->tile;
  assert(tp == NULL);

  kdu_dims tile_dims = tile_partition; // `tile_partition' is not affected by
                                       // fragment regions
  tile_dims.pos.x += idx.x*tile_dims.size.x;
  tile_dims.pos.y += idx.y*tile_dims.size.y;

  tile_dims &= canvas;
  assert(!tile_dims.is_empty());
  if ((in != NULL) && (!persistent) && (!allow_restart) &&
      !tile_dims.intersects(region))
    { // Don't even bother creating the tile; it will never be accessed.  Just
      // mark it as expired and return.
      return (tref->tile = KD_EXPIRED_TILE);
    }

  if (typical_tile_cache != NULL)
    { // Try re-using an element from the typical tile cache
      tp = tref->tile = typical_tile_cache;
      typical_tile_cache = tp->typical_next;
      tp->recycle(tref,idx,tile_dims);
                    // Similar to `kd_tile::initialize', but tries to
                    // reuse the tile's data structures
    }
  else
    {
      tp = tref->tile = new kd_tile(this,tref,idx,tile_dims);
      tp->initialize();
    }
  return tref->tile;
}

/*****************************************************************************/
/*                   kd_codestream::trim_compressed_data                     */
/*****************************************************************************/

void
  kd_codestream::trim_compressed_data()
{
  if (stats == NULL)
    return;
  kdu_uint16 threshold = stats->get_conservative_slope_threshold(true);
  if (threshold <= 1)
    return;

  // Work through all the ready precincts
  kd_global_rescomp *rc = global_rescomps + 32*num_components;
  kd_precinct *precinct;
  for (int d=32; d >= 0; d--, rc-=(num_components<<1))
    for (int c=0; c < num_components; c++, rc++)
      for (precinct=rc->first_ready; precinct!=NULL; precinct=precinct->next)
        for (int b=0; b < precinct->resolution->num_subbands; b++)
          {
            kd_precinct_band *pb = precinct->subbands + b;
            for (int n=0; n < (int) pb->block_indices.area(); n++)
              {
                kd_block *block = pb->blocks + n;
                block->trim_data(threshold,buf_server);
              }
          }
}

/*****************************************************************************/
/*                       kd_codestream::simulate_output                      */
/*****************************************************************************/

kdu_long
  kd_codestream::simulate_output(kdu_long &header_bytes, int layer_idx,
                                 kdu_uint16 slope_threshold,
                                 bool finalize_layer, bool last_layer,
                                 kdu_long max_bytes,
                                 kdu_long *sloppy_bytes)
{
  header_bytes = 0;
  if (reslength_constraints_used && !reslength_warning_issued)
    {
      int c;
      if (reslength_checkers != NULL)
        for (c=0; c <= num_components; c++)
          reslength_checkers[c].set_layer(layer_idx);
      for (kd_tile *tile=tiles_in_progress_head;
           tile != NULL; tile=tile->in_progress_next)
        if (tile->reslength_checkers != NULL)
          for (c=0; c <= num_components; c++)
            tile->reslength_checkers[c].set_layer(layer_idx);
    }
  
  if ((layer_idx == 0) && initial_fragment && !header_generated)
    {
      header_bytes = 2 + siz->generate_marker_segments(NULL,-1,0);
      if (!comments_frozen)
        freeze_comments();
      kd_codestream_comment *comscan;
      for (comscan=comhead; comscan != NULL; comscan=comscan->next)
        header_bytes += comscan->write_marker(NULL);
      header_bytes += reserved_layer_info_bytes;
    }

  kdu_long total_bytes = header_bytes;

  // Calculate the cost of including tile-part headers.  This calculation
  // may be off by a bit if the `ORGtparts' attribute has been used to request
  // extra tile-parts, or if `ORGgen_plt' has been used to request the
  // generation of PLT marker segments.  Also calculate the cost of adding in
  // empty packets for tiles which have too many layers.
  if ((layer_idx == 0) || last_layer)
    {
      for (kd_tile *tile=tiles_in_progress_head;
           tile != NULL; tile=tile->in_progress_next)
        {
          kdu_long tile_header_bytes = 0;
          if (layer_idx == 0)
            tile_header_bytes += 12 + 2 +
              siz->generate_marker_segments(NULL,tile->t_num,
                                            tile->next_tpart);
            if (last_layer && (tile->num_layers > (layer_idx+1)))
              tile_header_bytes += (tile->num_layers-1-layer_idx) *
              tile->total_precincts * ((tile->use_eph)?3:1);
            header_bytes += tile_header_bytes;
            total_bytes += tile_header_bytes;
          }
    }
  if (total_bytes > max_bytes)
    {
      assert(!finalize_layer);
      return total_bytes;
    }

  // Now work through all the ready precincts
  kd_global_rescomp *rc = global_rescomps + 32*num_components;
  for (int d=32; d >= 0; d--, rc-=(num_components<<1))
    for (int c=0; c < num_components; c++, rc++)
      {
        kd_precinct *precinct = rc->first_ready;
        if (precinct == NULL)
          continue;

        // Scale sizes to properly handle incremental flushing.
        bool all_ready = (rc->remaining_area==rc->ready_area);
        if (rc->ready_fraction < 0.0)
          { // The `ready_fraction' value needs to be estimated
            assert((rc->remaining_area > 0) &&
                   (rc->remaining_area >= rc->ready_area));
            if (all_ready)
              rc->ready_fraction = rc->reciprocal_fraction = 1.0;
            else
              {
                double A;
                kdu_long T = rc->total_area - rc->area_covered_by_tiles;
                if (T > 0)
                  {
                    assert(T <= (rc->remaining_area + rc->ready_area));
                    A = ((double)(rc->remaining_area - T))
                      + ((double) T) * ((double) rc->area_used_by_tiles) /
                                   ((double) rc->area_covered_by_tiles);
                  }
                else
                  A = (double)(rc->remaining_area);
                rc->ready_fraction = ((double)(rc->ready_area)) / A;
                if (rc->ready_fraction > 1.0)
                  rc->ready_fraction = 1.0; // In case of rounding problems.
                rc->reciprocal_fraction = 1.0 / rc->ready_fraction;
              }
          }

        kdu_long local_max_bytes = max_bytes-total_bytes;
        kdu_long local_sloppy = (sloppy_bytes!=NULL)?(*sloppy_bytes):0;
        if (!all_ready)
          {
            local_max_bytes = (kdu_long)(local_max_bytes*rc->ready_fraction);
            local_sloppy = ((kdu_long)(local_sloppy*rc->ready_fraction)) - 1;
            if (local_sloppy < 0)
              local_sloppy = 0;
          }
        kdu_long initial_local_sloppy = local_sloppy;
        kdu_long local_header_bytes=0, local_total_bytes=0;

        // Now size the packets using scaled sizes
        kdu_long packet_header_bytes, packet_bytes;
        for (; precinct != NULL; precinct=precinct->next)
          {
            if (precinct->next_layer_idx != 0)
              { KDU_ERROR_DEV(e,37); e <<
                  KDU_TXT("Attempting to run rate-control simulation "
                  "on a precinct for which one or more packets have already "
                  "been written to the code-stream.  Problem is most likely "
                  "caused by trying to use the incremental code-stream "
                  "flushing feature with one of the progression orders, LRCP "
                  "or RLCP.");
              }
            if (layer_idx >= precinct->resolution->tile_comp->tile->num_layers)
              continue;
            packet_bytes = packet_header_bytes = 0;
            if (sloppy_bytes != NULL)
              {
                assert(finalize_layer && last_layer && (local_sloppy >= 0));
                assert(slope_threshold < 0xFFFF);
                packet_bytes =
                  precinct->simulate_packet(packet_header_bytes,layer_idx,
                                            slope_threshold+1,false,true);
                kdu_long packet_max_bytes = packet_bytes + local_sloppy;
                if (packet_max_bytes > (local_max_bytes-local_total_bytes))
                  packet_max_bytes = local_max_bytes-local_total_bytes;
                assert(packet_max_bytes >= packet_bytes);
                packet_bytes =
                  precinct->simulate_packet(packet_header_bytes,layer_idx,
                                            slope_threshold,true,true,
                                            packet_max_bytes,true);
                assert(packet_bytes <= packet_max_bytes);
                local_sloppy = packet_max_bytes - packet_bytes;
              }
            else
              packet_bytes =
                precinct->simulate_packet(packet_header_bytes,layer_idx,
                             slope_threshold,finalize_layer,last_layer,
                             local_max_bytes-local_total_bytes);
            local_total_bytes += packet_bytes;
            local_header_bytes += packet_header_bytes;
            if (reslength_constraints_used && !reslength_warning_issued)
              for (int glob=0; glob < 2; glob++)
                {
                  kd_tile *tile = precinct->resolution->tile_comp->tile;
                  kd_reslength_checker *checkers =
                    (glob)?this->reslength_checkers:tile->reslength_checkers;
                  if (checkers != NULL)
                    {
                      if (!checkers[0].check_packet(packet_bytes,d))
                        reslength_constraints_violated = true;
                      if (!checkers[c+1].check_packet(packet_bytes,d))
                        reslength_constraints_violated = true;
                    }
                }
            if (local_total_bytes > local_max_bytes)
              {
                assert(!finalize_layer);
                  // Otherwise, you are not driving this function correctly
                break;
              }
          }

        // Now scale the packet sizes back up again
        if (all_ready)
          {
            header_bytes += local_header_bytes;
            total_bytes += local_total_bytes;
            if (sloppy_bytes != NULL)
              *sloppy_bytes = local_sloppy;
          }
        else
          {
            header_bytes +=
              1 + (kdu_long)(local_header_bytes*rc->reciprocal_fraction);
            total_bytes +=
              1 + (kdu_long)(local_total_bytes*rc->reciprocal_fraction);
            if (sloppy_bytes != NULL)
              {
                kdu_long delta_sloppy = initial_local_sloppy - local_sloppy;
                delta_sloppy =
                  1 + (kdu_long)(delta_sloppy * rc->reciprocal_fraction);
                *sloppy_bytes -= delta_sloppy;
                if (*sloppy_bytes < 0)
                  *sloppy_bytes = 0;
              }
          }
        if (total_bytes > max_bytes)
          return total_bytes;
      }
  return total_bytes;
}

/*****************************************************************************/
/*                  kd_codestream::calculate_min_header_cost                 */
/*****************************************************************************/

void
  kd_codestream::calculate_min_header_cost()
{
  assert(min_fixed_header_size == 0);
  min_fixed_header_size = min_layer_header_size = 0;
  if (initial_fragment)
    min_fixed_header_size += 2 + siz->generate_marker_segments(NULL,-1,0);
  if (!comments_frozen)
    freeze_comments();
  if (initial_fragment)
    {
      kd_codestream_comment *comscan;
      for (comscan=comhead; comscan != NULL; comscan=comscan->next)
        min_fixed_header_size += comscan->write_marker(NULL);
      min_fixed_header_size += reserved_layer_info_bytes;
    }
  kdu_long tile_area=0, tile_layer_cost=0;
  kd_tile_ref *tref = tile_refs;
  kdu_coords idx, abs_idx;
  for (idx.y=0; idx.y < tile_indices.size.y; idx.y++)
    for (idx.x=0; idx.x < tile_indices.size.x; idx.x++, tref++)
      {
        abs_idx = idx + tile_indices.pos;
        assert((abs_idx.x < tile_span.x) && (abs_idx.y < tile_span.y));
        int t_num = abs_idx.x + abs_idx.y*tile_span.x;
        min_fixed_header_size += 12 + 2 +
          siz->generate_marker_segments(NULL,t_num,0);
        kd_tile *tile = tref->tile;
        if ((tile == NULL) || (tile == KD_EXPIRED_TILE))
          continue;
        tile_area += tile->dims.area();
        tile_layer_cost += tile->total_precincts;
        if (tile->use_eph)
          tile_layer_cost += tile->total_precincts*2;
        if (tile->use_sop)
          tile_layer_cost += tile->total_precincts*6;
      }

  kdu_long total_area=canvas.area();
  assert(total_area >= tile_area);
  if (tile_area > 0)
    {
      double scale = ((double) total_area) / ((double) tile_area);
      min_layer_header_size = (kdu_long)(tile_layer_cost * scale);
    }
}

/*****************************************************************************/
/*                          kd_codestream::pcrd_opt                          */
/*****************************************************************************/

void
  kd_codestream::pcrd_opt(bool trim_to_rate, double tolerance)
{
  if (tolerance > 0.5)
    tolerance = 0.5;
  if (min_fixed_header_size == 0)
    calculate_min_header_cost();
  kdu_long header_bytes = 0;
  kdu_long rough_upper_bound = 0;
  if (layer_targets[num_sized_layers-1] == 0)
    { // Find rough upper bound for the purpose of spacing intermediate layers
      rough_upper_bound = simulate_output(header_bytes,0,0,false,false);
    }

  // Fill in unknown target rates
  int layer_idx;
  for (layer_idx=0; layer_idx < num_sized_layers; layer_idx++)
    if (layer_targets[layer_idx] == 0)
      break;
  if (layer_idx < (num_sized_layers-1))
    { // At least one layer needs to have a target size assigned to it
      kdu_long next_target, last_target = 0;
      for (layer_idx=0; layer_targets[layer_idx] != 0; layer_idx++)
        { // Add the minimum header cost to each initial non-zero layer
          layer_targets[layer_idx] += min_fixed_header_size +
            (layer_idx+1)*min_layer_header_size;
          last_target = layer_targets[layer_idx];
        }
      while (layer_idx < (num_sized_layers-1))
        {
          if (layer_targets[layer_idx] != 0)
            {
              last_target = layer_targets[layer_idx];
              layer_idx++;
              continue;
            }
          int last_idx=layer_idx-1; // Could be negative
          int next_idx=layer_idx+1;
          while ((layer_targets[next_idx] == 0) &&
                 (next_idx < (num_sized_layers-1)))
            next_idx++;
          next_target = layer_targets[next_idx];
          if (next_target == 0)
            {
              assert(next_idx == (num_sized_layers-1));
              next_target = rough_upper_bound;
            }
          double adjusted_next_target = (double)
            (next_target - min_fixed_header_size -
             (next_idx+1)*min_layer_header_size);
          if (adjusted_next_target < 1.0)
            adjusted_next_target = 1.0;
          double adjusted_last_target;
          if (last_target == 0)
            {
              assert(last_idx == -1);
              adjusted_last_target =
                (adjusted_next_target*pow(0.5,0.5*(next_idx-last_idx)));
            }
          else
            adjusted_last_target = (double)
              (last_target - min_fixed_header_size -
               (last_idx+1)*min_layer_header_size);
          assert(adjusted_last_target > 0.0);
          adjusted_last_target = log(adjusted_last_target);
          adjusted_next_target = log(adjusted_next_target);
          for (; layer_idx < next_idx; layer_idx++)
            {
              double w1 = ((double)(layer_idx-last_idx)) /
                ((double)(next_idx-last_idx));
              double val = (1.0-w1)*adjusted_last_target +
                w1*adjusted_next_target;
              val = exp(val);
              layer_targets[layer_idx] = ((kdu_long) val) +
                min_fixed_header_size + (layer_idx+1)*min_layer_header_size;
            }
        }
    }

  // Now we are ready to run the rate control loop
  kdu_long last_layer_bytes=0, generated_bytes=0, new_layer_bytes;
  int threshold;
  int min_threshold = 0;
  int lim_threshold = (1<<16);
  for (layer_idx=0; layer_idx < num_sized_layers; layer_idx++)
    {
      bool last_layer = (layer_idx == (num_sized_layers-1));

      // Establish target bounds on the size of this layer
      generated_bytes += layer_sizes[layer_idx];
      kdu_long target_max = layer_targets[layer_idx];
      kdu_long target_min = target_max;
      if (target_max == 0)
        {
          assert(last_layer);
          target_min = target_max = KDU_LONG_HUGE;
          if ((!reslength_constraints_used) || reslength_warning_issued)
            lim_threshold = 1;
        }
      else
        {
          // Adjust to compensate for simulation not including the EOC cost
          if (final_fragment)
            target_max -= 2;

          // Find normalized minimum header cost
          double min_cost = (double) min_layer_header_size;
          if (layer_idx == 0)
            min_cost += (double) min_fixed_header_size;
          if ((generated_bytes > 0) && (generated_bytes < target_max) &&
              (tolerance > 0.0))
            min_cost -= (kdu_long)(min_cost *
                                   (((double) generated_bytes)/target_max));

          // Adjust maximum size to find limit on remaining bytes
          target_max -= generated_bytes;

          // Adjust maximum size to reflect bytes for this layer alone
          target_max -= last_layer_bytes;

          // Find preferred minimum size bound using tolerances
          target_min = target_max;
          if (tolerance > 0.0)
            {
              if (target_max > (kdu_long) min_cost)
                target_min -= (kdu_long)((target_max-min_cost) * tolerance);
            }
        }

      if (lim_threshold < 1)
        lim_threshold = 1;
      int best_threshold = lim_threshold-1;
      min_threshold = 0;

      // Use a simple bisection search for the slope threshold
      do {
          threshold = (min_threshold+lim_threshold)>>1;
          assert(threshold < (1<<16));
          reslength_constraints_violated = false;
          new_layer_bytes =
            simulate_output(header_bytes,layer_idx,(kdu_uint16) threshold,
                            false,last_layer,target_max);
          if ((new_layer_bytes > target_max) || reslength_constraints_violated)
            { // Must not exceed the threshold
              min_threshold = threshold+1;
            }
          else if (new_layer_bytes >= target_min)
            {
              best_threshold = threshold;
              break; // Found a suitable slope threshold satisfying bounds
            }
          else
            {
              best_threshold = threshold; // Best so far
              lim_threshold = threshold;
            }
        } while (lim_threshold > min_threshold);

      // Finalize the allocation for this layer
      threshold = best_threshold;
      new_layer_bytes =  // Don't finalize yet
        simulate_output(header_bytes,layer_idx,(kdu_uint16) threshold,
                        false,last_layer);
      kdu_long remaining_bytes = target_max - new_layer_bytes;
      if (last_layer && (remaining_bytes>0) && (threshold>0) && trim_to_rate)
        {
          threshold--;
          new_layer_bytes = // Finalize with sloppy fill in.
            simulate_output(header_bytes,layer_idx,(kdu_uint16) threshold,
                            true,true,target_max,&remaining_bytes);
        }
      else
        new_layer_bytes = // Finalize without sloppy fill in.
          simulate_output(header_bytes,layer_idx,(kdu_uint16) threshold,
                          true,last_layer);
      layer_thresholds[layer_idx] = (kdu_uint16) threshold;
      lim_threshold = threshold+1; // We are allowed to repeat the threshold.
      last_layer_bytes += new_layer_bytes;
      if (layer_targets[layer_idx] == 0)
        {
          assert(last_layer);
          layer_targets[layer_idx] = generated_bytes + last_layer_bytes
            + ((final_fragment)?2:0); // Put EOC back.
        }
    }
}

/*****************************************************************************/
/*                  kd_codestream::generate_codestream                       */
/*****************************************************************************/

bool
  kd_codestream::generate_codestream(int max_layers)
{
  assert(out != NULL);

  // Create the `layer_sizes' array if necessary
  assert((layer_sizes != NULL) && (layer_thresholds != NULL));
  if (max_layers > num_sized_layers)
    { KDU_ERROR_DEV(e,38); e <<
        KDU_TXT("Using the `kdu_codestream::generate_codestream' "
        "function in an illegal manner.  The `max_layers' argument may not "
        "exceed the maximum number of layers which are being sized.  The "
        "problem may have arisen from an incorrect use of the incremental "
        "code-stream flushing capability.");
    }

  // Write the main header, if necessary
  if (!header_generated)
    {
      header_generated = true;
      if (initial_fragment)
        {
          layer_sizes[0] += out->put(KDU_SOC);
          layer_sizes[0] += siz->generate_marker_segments(out,-1,0);

          // Write out comment marker segments
          if (!comments_frozen)
            freeze_comments();
          kd_codestream_comment *comscan;
          for (comscan=comhead; comscan != NULL; comscan=comscan->next)
            layer_sizes[0] += comscan->write_marker(out);

          // See if we need to write TLM marker segments
        }
      assert(!tlm_generator);
      kdu_params *org = siz->access_cluster(ORG_params); assert(org != NULL);
      int max_tlm_tparts;
      if (org->get(ORGgen_tlm,0,0,max_tlm_tparts) && (max_tlm_tparts > 0))
        { 
          if (max_tlm_tparts > 255)
            max_tlm_tparts = 255;
          int tnum_bytes, tplen_bytes;
          if (org->get(ORGtlm_style,0,0,tnum_bytes) &&
              org->get(ORGtlm_style,0,1,tplen_bytes))
            { 
              if ((tnum_bytes == 0) && (max_tlm_tparts > 1))
                { KDU_ERROR(e,0x25041002); e <<
                  KDU_TXT("The \"implied\" style for signalling tile numbers "
                          "in TLM marker segments, as requested via the "
                          "\"ORGtlm_style\" parameter attribute, cannot be "
                          "used unless there is only one tile-part per tile, "
                          "so the \"ORGgen_tlm\" parameter attribute should "
                          "specify a value of 1 for the maximum number of "
                          "tile-parts per tile in this case.");
                }
              if ((tnum_bytes == 1) && ((tile_span.x*tile_span.y) > 256))
                { KDU_ERROR(e,0x25041003); e <<
                  KDU_TXT("You have used the \"ORGtlm_style\" parameter "
                          "attribute to specify a TLM marker segment style "
                          "in which tile numbers are represented using "
                          "only one byte.  However, the number of tiles "
                          "in the image is greater than 256, so this is "
                          "clearly going to be a problem.");
                }
            }
          else
            { tnum_bytes = 2; tplen_bytes = 4; }
          kdu_compressed_target *tgt = out->access_tgt();
          if (tgt->start_rewrite(0))
            {
              tgt->end_rewrite();
              if (tlm_generator.init(tile_span.x*tile_span.y,max_tlm_tparts,
                                     tnum_bytes,tplen_bytes))
                {
                  if (initial_fragment)
                    tlm_generator.write_dummy_tlms(out);
                }
              else
                { KDU_WARNING(w,16); w <<
                    KDU_TXT("Unable to generate the TLM marker "
                    "segments requested via the `ORGgen_tlm' parameter "
                    "attribute.  The reason for this is that the total "
                    "number of tile-parts whose lengths would need to be "
                    "represented exceeds the amount of data which can "
                    "legally be stored in the maximum allowable 256 TLM "
                    "marker segments, allowing for 6 bytes per tile-part "
                    "length value.");
                }
            }
          else
            { KDU_WARNING(w,17); w <<
                KDU_TXT("Unable to generate the TLM marker segments "
                "requested via the `ORGgen_tlm' parameter attribute.  "
                "The reason for this is that the logical compressed data "
                "target supplied by the application does not appear to "
                "support rewriting (i.e., seeking).  We need this to reserve "
                "space for the TLM marker segments up front and later "
                "overwrite the reserved space with valid tile-part lengths.");
            }
        }
      assert(header_length == 0);
      header_length = out->get_bytes_written();
      if (!initial_fragment)
        assert(header_length == 0);
    }

  // Now cycle through the tiles, interleaving their tile-parts.
  bool done;
  do {
      done = true;
      kd_tile *tnext = NULL;
      for (kd_tile *tile=tiles_in_progress_head; tile != NULL; tile=tnext)
        {
          tnext = tile->in_progress_next;
          kd_tile_ref *tref = tile->tile_ref;
          kdu_long tpart_bytes =
            tile->generate_tile_part(max_layers,layer_thresholds);
          if (((tile = tref->tile) != KD_EXPIRED_TILE) && (tpart_bytes > 0))
            done = false;
        }
    } while (!done);

  // Finish up with the EOC marker, once all data has been generated
  if (num_incomplete_tiles == 0)
    {
      if (tlm_generator.exists())
        {
          kdu_compressed_target *tgt = out->access_tgt();
          tlm_generator.write_tlms(tgt,prev_tiles_written,
                                   prev_tile_bytes_written);
        }
      if (final_fragment)
        layer_sizes[0] += out->put(KDU_EOC);
      out->flush();
    }
  return (num_incomplete_tiles == 0);
}

/*****************************************************************************/
/*            kd_codestream::unload_tiles_to_cache_threshold                 */
/*****************************************************************************/

void
  kd_codestream::unload_tiles_to_cache_threshold()
{
  while ((num_unloadable_tiles > 0) &&
         ((num_unloadable_tiles > max_unloadable_tiles) ||
          buf_server->cache_threshold_exceeded()))
    {
      while ((unloadable_tile_scan != NULL) &&
             unloadable_tile_scan->dims.intersects(region))
        unloadable_tile_scan = unloadable_tile_scan->unloadable_next;
      if (unloadable_tile_scan != NULL)
        {
          assert(unloadable_tile_scan->is_unloadable);
          unloadable_tile_scan->release(); // Automatically updates pointers
        }
      else
        unloadable_tiles_head->release(); // Automatically updates pointers
    }
}


/* ========================================================================= */
/*                              kdu_codestream                               */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdu_codestream::create (output)                  */
/*****************************************************************************/

void
  kdu_codestream::create(siz_params *siz_in,
                         kdu_compressed_target *target,
                         kdu_dims *fragment_region,
                         int fragment_tiles_generated,
                         kdu_long fragment_tile_bytes_generated)
{
  assert((state == NULL) && (target != NULL));
  state = new kd_codestream;
  state->out = new kd_compressed_output(target);
  state->siz = new siz_params;
  state->siz->copy_from(siz_in,-1,-1);
  state->construct_common();
  if (fragment_region != NULL)
    state->restrict_to_fragment(*fragment_region,fragment_tiles_generated,
                                fragment_tile_bytes_generated);
  state->num_incomplete_tiles = (int) state->tile_indices.area();
}

/*****************************************************************************/
/*                       kdu_codestream::create (input)                      */
/*****************************************************************************/

void
  kdu_codestream::create(kdu_compressed_source *source, kdu_thread_env *env)
{
  assert(state == NULL);
  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);
  state = new kd_codestream;
  state->in = new kd_compressed_input(source);
  state->cached = (source->get_capabilities() & KDU_SOURCE_CAP_CACHED) != 0;
  state->in_memory_source = state->in->is_fully_buffered();
  state->marker = new kd_marker(state->in,state);
  if ((!state->marker->read()) ||
      (state->marker->get_code() != KDU_SOC))
    { KDU_ERROR(e,39); e <<
        KDU_TXT("Code-stream must start with an SOC marker!");
    }
  state->siz = new siz_params;
  if (!(state->marker->read() &&
        state->siz->translate_marker_segment(state->marker->get_code(),
                                             state->marker->get_length(),
                                             state->marker->get_bytes(),-1,0)))
    { KDU_ERROR(e,40); e <<
        KDU_TXT("Code-stream must contain a valid SIZ marker segment, "
        "immediately after the SOC marker!");
    }
  state->construct_common();
  state->comments_frozen = true;
  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
}

/*****************************************************************************/
/*                     kdu_codestream::create (interchange)                  */
/*****************************************************************************/

void
  kdu_codestream::create(siz_params *siz_in)
{
  assert(state == NULL);
  state = new kd_codestream;
  state->siz = new siz_params;
  state->siz->copy_from(siz_in,-1,-1);
  state->construct_common();
  state->interchange = true;
  state->persistent = true; // Structures persist after tile is closed, but
                            // compressed data does not.
}

/*****************************************************************************/
/*                        kdu_codestream::restart (output)                   */
/*****************************************************************************/

void
  kdu_codestream::restart(kdu_compressed_target *target)
{
  if (!state->allow_restart)
    { KDU_ERROR_DEV(e,41); e <<
        KDU_TXT("You may not use the `kdu_codestream::restart' "
        "function unless `kdu_codestream::enable_restart' was called after "
        "the code-stream management machinery was first created.");
    }
  if (state->out == NULL)
    { KDU_ERROR_DEV(e,42); e <<
        KDU_TXT("You may not use the output form of "
        "`kdu_codestream::restart' if the code-stream management machinery "
        "was originally created using anything other than the output form of "
        "`kdu_codestream::create'.");
    }
  delete state->out;
  state->out = new kd_compressed_output(target);
  state->restart();
  state->num_incomplete_tiles = (int) state->tile_indices.area();
}

/*****************************************************************************/
/*                         kdu_codestream::restart (input)                   */
/*****************************************************************************/

void
  kdu_codestream::restart(kdu_compressed_source *source, kdu_thread_env *env)
{
  if (!state->allow_restart)
    { KDU_ERROR_DEV(e,43); e <<
        KDU_TXT("You may not use the `kdu_codestream::restart' "
        "function unless `kdu_codestream::enable_restart' was called after "
        "the code-stream management machinery was first created.");
    }
  if (state->in == NULL)
    { KDU_ERROR_DEV(e,44); e <<
        KDU_TXT("You may not use the input form of "
        "`kdu_codestream::restart' if the code-stream management machinery "
        "was originally created using anything other than the input form of "
        "`kdu_codestream::create'.");
    }

  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);
  delete state->in;
  state->in = new kd_compressed_input(source);
  delete state->marker;
  state->marker = new kd_marker(state->in,state);
  state->cached = (source->get_capabilities() & KDU_SOURCE_CAP_CACHED) != 0;
  state->in_memory_source = state->in->is_fully_buffered();
  state->block_truncation_factor = 0;
  if ((!state->marker->read()) ||
      (state->marker->get_code() != KDU_SOC))
    { KDU_ERROR(e,45); e <<
        KDU_TXT("Code-stream must start with an SOC marker!");
    }

  // Read the SIZ marker segment and determine whether or not the high
  // level structure has changed; if so, we might as well create the internal
  // machinery again from scratch.
  siz_params siz;
  siz.copy_from(state->siz,-1,-1);
  siz.clear_marks(); // So we will be able to tell if anything changed
  if (!(state->marker->read() &&
        siz.translate_marker_segment(state->marker->get_code(),
                                     state->marker->get_length(),
                                     state->marker->get_bytes(),-1,0)))
    { KDU_ERROR(e,46); e <<
        KDU_TXT("Code-stream must contain a valid SIZ marker segment, "
        "immediately after the SOC marker!");
    }
  if (siz.any_changes())
    { // Rebuild the code-stream management machinery from scratch
      kd_codestream *old_state = state;
      state = new kd_codestream;
      state->in = old_state->in; old_state->in = NULL;
      state->marker = old_state->marker->move(state->in,state);
      old_state->marker = NULL;
      state->siz = new siz_params;
      state->siz->copy_from(&siz,-1,-1);
      state->construct_common();

      state->discard_levels = old_state->discard_levels;
      state->max_apparent_layers = old_state->max_apparent_layers;
      if (state->num_components == old_state->num_components)
        {
          state->num_apparent_components = old_state->num_apparent_components;
          for (int c=0; c < state->num_components; c++)
            {
              state->comp_info[c].apparent_idx =
                old_state->comp_info[c].apparent_idx;
              state->comp_info[c].from_apparent = state->comp_info +
                (old_state->comp_info[c].from_apparent - old_state->comp_info);
            }
        }
      if (state->num_output_components == old_state->num_output_components)
        {
          state->num_apparent_output_components =
            old_state->num_apparent_output_components;
          for (int c=0; c < state->num_output_components; c++)
            {
              state->output_comp_info[c].apparent_idx =
                old_state->output_comp_info[c].apparent_idx;
              state->output_comp_info[c].from_apparent =
                old_state->output_comp_info[c].from_apparent;
            }
        }
      state->component_access_mode = old_state->component_access_mode;

      state->allow_restart = old_state->allow_restart;
      state->transpose = old_state->transpose;
      state->vflip = old_state->vflip;
      state->hflip = old_state->hflip;
      state->resilient = old_state->resilient;
      state->expect_ubiquitous_sops = old_state->expect_ubiquitous_sops;
      state->fussy = old_state->fussy;
      state->persistent = old_state->persistent;
      state->cached = old_state->cached;
      state->in_memory_source = old_state->in_memory_source;
      state->min_slope_threshold = old_state->min_slope_threshold;

      delete old_state;
    }
  else
    state->restart();
  state->comments_frozen = true;
  state->unloadable_tile_scan = state->unloadable_tiles_head;
  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
}

/*****************************************************************************/
/*                           kdu_codestream::destroy                         */
/*****************************************************************************/

void
  kdu_codestream::destroy()
{
  assert(state != NULL);
  delete state;
  state = NULL;
}

/*****************************************************************************/
/*                      kdu_codestream::share_buffering                      */
/*****************************************************************************/

void
  kdu_codestream::share_buffering(kdu_codestream existing)
{
  assert(!state->tiles_accessed);
  if ((state->buf_server->get_peak_structure_bytes() > 0) ||
      (state->buf_server->get_peak_buf_bytes() > 0))
    { KDU_ERROR_DEV(e,47); e <<
        KDU_TXT("You cannot use the `kdu_codestream::share_buffering' "
        "function if the codestream object which will be sharing another "
        "codestream's buffering has already allocated some internal "
        "resources.");
    }
  state->buf_server->detach();
  delete state->buf_server;
  if (state->precinct_server != NULL)
    {
      delete state->precinct_server;
      state->precinct_server = NULL;
    }
  state->buf_server = existing.state->buf_server;
  state->buf_server->attach();
  state->precinct_server = new kd_precinct_server(state->buf_server);
}

/*****************************************************************************/
/*                   kdu_codestream::augment_cache_threshold                 */
/*****************************************************************************/

kdu_long
  kdu_codestream::augment_cache_threshold(int extra_bytes)
{
  return state->buf_server->augment_cache_threshold(extra_bytes);
}

/*****************************************************************************/
/*                kdu_codestream::set_tile_unloading_threshold               */
/*****************************************************************************/

int
  kdu_codestream::set_tile_unloading_threshold(int max_tiles_on_list)
{
  if (max_tiles_on_list < 0)
    max_tiles_on_list = 0;
  int prev_value = state->max_unloadable_tiles;
  state->max_unloadable_tiles = max_tiles_on_list;
  state->unload_tiles_to_cache_threshold();
  return prev_value;
}


/*****************************************************************************/
/*                      kdu_codestream::enable_restart                       */
/*****************************************************************************/

void
  kdu_codestream::enable_restart()
{
  if (state->allow_restart)
    return;
  if (state->tiles_accessed)
    { KDU_ERROR_DEV(e,48); e <<
        KDU_TXT("You may not call `kdu_codestream::enable_restart' "
        "after opening the first tile.");
    }
  state->allow_restart = true;
}

/*****************************************************************************/
/*                       kdu_codestream::set_persistent                      */
/*****************************************************************************/

void
  kdu_codestream::set_persistent()
{
  if (state->in == NULL)
    return;
  if (state->tiles_accessed)
    { KDU_ERROR_DEV(e,49); e <<
        KDU_TXT("You may only set the codestream object into its "
        "\"persistent\" mode prior to opening the first tile.");
    }
  state->persistent = true;
}

/*****************************************************************************/
/*                      kdu_codestream::is_last_fragment                     */
/*****************************************************************************/

bool
  kdu_codestream::is_last_fragment()
{
  return (state!=NULL) && (state->out!=NULL) && state->final_fragment;
}

/*****************************************************************************/
/*                          kdu_codestream::access_siz                       */
/*****************************************************************************/

siz_params *
  kdu_codestream::access_siz()
{
  return state->siz;
}

/*****************************************************************************/
/*                      kdu_codestream::set_textualization                   */
/*****************************************************************************/

void
  kdu_codestream::set_textualization(kdu_message *output)
{
  assert(!state->tiles_accessed);
  if (output != NULL)
    {
      state->siz->textualize_attributes(*output,-1,-1);
      output->flush();
    }
  state->textualize_out = output;
}

/*****************************************************************************/
/*                        kdu_codestream::set_max_bytes                      */
/*****************************************************************************/

void
  kdu_codestream::set_max_bytes(kdu_long max_bytes, bool simulate_parsing,
                                bool enable_periodic_trimming)
{
  assert(!state->tiles_accessed);

  if (state->in != NULL)
    {
      state->simulate_parsing_while_counting_bytes = simulate_parsing;
      state->in->set_max_bytes(max_bytes);
      if (state->in->failed())
        { KDU_ERROR(e,50); e <<
            KDU_TXT("Attempting to impose too small a limit on the "
            "number of code-stream bytes. " << (int) max_bytes << " bytes is "
            "insufficient to accomodate even the main header!");
        }
    }
  else if (state->out != NULL)
    {
      if (state->stats != NULL)
        { KDU_ERROR_DEV(e,51); e <<
            KDU_TXT("\"kdu_codestream::set_max_bytes\" may not be "
            "called multiple times.");
        }
      kdu_long total_samples = 0;
      for (int c=0; c < state->num_components; c++)
        {
          kdu_dims comp_dims; get_dims(c,comp_dims);
          total_samples += comp_dims.area();
        }
      state->stats = new kd_compressed_stats(total_samples,max_bytes,
                                             enable_periodic_trimming);
    }
}

/*****************************************************************************/
/*                   kdu_codestream::set_min_slope_threshold                 */
/*****************************************************************************/

void
  kdu_codestream::set_min_slope_threshold(kdu_uint16 threshold)
{
  state->min_slope_threshold = threshold;
}

/*****************************************************************************/
/*                        kdu_codestream::set_resilient                      */
/*****************************************************************************/

void
  kdu_codestream::set_resilient(bool expect_ubiquitous_sops)
{
  state->resilient = true;
  state->expect_ubiquitous_sops = expect_ubiquitous_sops;
  state->fussy = false;
}

/*****************************************************************************/
/*                          kdu_codestream::set_fussy                        */
/*****************************************************************************/

void
  kdu_codestream::set_fussy()
{
  state->resilient = false;
  state->fussy = true;
}

/*****************************************************************************/
/*                          kdu_codestream::set_fast                         */
/*****************************************************************************/

void
  kdu_codestream::set_fast()
{
  state->resilient = false;
  state->fussy = false;
}

/*****************************************************************************/
/*                kdu_codestream::apply_input_restrictions                   */
/*****************************************************************************/

void
  kdu_codestream::apply_input_restrictions(int first_component,
                        int max_components, int discard_levels,
                        int max_layers, kdu_dims *region_of_interest,
                        kdu_component_access_mode access_mode)
{
  if (state->out != NULL)
    { KDU_ERROR_DEV(e,52); e <<
        KDU_TXT("The `kdu_codestream::apply_input_restrictions' "
        "function may not be invoked on codestream objects opened for output "
        "(i.e. for compression).");
    }
  if (state->tiles_accessed)
    {
      if (state->num_open_tiles != 0)
        { KDU_ERROR_DEV(e,53); e <<
            KDU_TXT("You may apply restrictions to the resolution "
            "or number of image components only after closing all open "
            "tiles.");
        }
      if (state->tiles_accessed && !state->persistent)
        { KDU_ERROR_DEV(e,54); e <<
            KDU_TXT("You may not apply restrictions to the resolution "
            "or number of image components after the first tile access, "
            "unless the codestream object is set up to be persistent.");
        }
    }

  state->discard_levels = discard_levels;
  if (max_layers <= 0)
    max_layers = 0xFFFF;
  state->max_apparent_layers = max_layers;
  state->region = state->canvas;
  if (region_of_interest != NULL)
    state->region &= *region_of_interest;
  state->unloadable_tile_scan = state->unloadable_tiles_head;

  // Now process component restrictions.
  int c, from_idx;
  state->component_access_mode = access_mode;
  if (access_mode == KDU_WANT_CODESTREAM_COMPONENTS)
    {
      if ((first_component < 0) || (first_component >= state->num_components))
        { KDU_ERROR_DEV(e,55); e <<
            KDU_TXT("The range of apparent image components supplied to "
            "`kdu_codestream::apply_input_restrictions' is empty or illegal!");
        }
      state->num_apparent_output_components = 0;
           // Output components are inaccessible in codestream access mode
      state->num_apparent_components =
        state->num_components - first_component;
      if ((max_components > 0) &&
          (max_components < state->num_apparent_components))
        state->num_apparent_components = max_components;
      for (c=from_idx=0; c < state->num_components; c++)
        {
          kd_comp_info *ci = state->comp_info + c;
          ci->apparent_idx = c - first_component;
          if (ci->apparent_idx >= state->num_apparent_components)
            ci->apparent_idx = -1;
          ci->from_apparent = NULL;
          if (ci->apparent_idx >= 0)
            state->comp_info[from_idx++].from_apparent = ci;
        }
    }
  else if (access_mode == KDU_WANT_OUTPUT_COMPONENTS)
    {
      // Start by removing any restrictions on the codestream components
      state->num_apparent_components = state->num_components;
      for (c=0; c < state->num_components; c++)
        {
          kd_comp_info *ci = state->comp_info + c;
          ci->apparent_idx = c;
          ci->from_apparent = ci;
        }

      // Now is to apply/remove restrictions on the output image components.
      if ((first_component < 0) ||
          (first_component >= state->num_output_components))
        { KDU_ERROR_DEV(e,0x05090500); e <<
            KDU_TXT("The range of apparent output image components supplied "
            "to `kdu_codestream::apply_input_restrictions' is empty or "
            "illegal!");
        }

      state->num_apparent_output_components =
        state->num_output_components - first_component;
      if ((max_components > 0) &&
          (max_components < state->num_apparent_output_components))
        state->num_apparent_output_components = max_components;
      for (c=from_idx=0; c < state->num_output_components; c++)
        {
          kd_output_comp_info *oci = state->output_comp_info + c;
          oci->apparent_idx = c - first_component;
          if (oci->apparent_idx >= state->num_apparent_output_components)
            oci->apparent_idx = -1;
          oci->from_apparent = 0;
          if (oci->apparent_idx >= 0)
            state->output_comp_info[from_idx++].from_apparent = c;
        }
    }
  else
    assert(0);
}

/*****************************************************************************/
/*                kdu_codestream::apply_input_restrictions                   */
/*****************************************************************************/

void
  kdu_codestream::apply_input_restrictions(int num_components,
                        int *component_indices, int discard_levels,
                        int max_layers, kdu_dims *region_of_interest,
                        kdu_component_access_mode access_mode)
{
  int c, apparent_idx;

  // Start by setting up everything except components, by calling the
  // first form of the function
  apply_input_restrictions(0,0,discard_levels,max_layers,region_of_interest,
                           access_mode);
  if (access_mode == KDU_WANT_CODESTREAM_COMPONENTS)
    { // In this case, we have only to configure any special restrictions on
      // the codestream image components.  Everything else has already been
      // done for us by the first form of the function.
      for (c=0; c < state->num_components; c++)
        {
          state->comp_info[c].from_apparent = NULL;
          state->comp_info[c].apparent_idx = -1;
        }
      state->num_apparent_components = num_components;
      for (c=apparent_idx=0; c < num_components; c++)
        {
          int idx = component_indices[c];
          if ((idx >= state->num_components) || (c >= state->num_components) ||
              (state->comp_info[idx].apparent_idx >= 0))
            continue;
          state->comp_info[idx].apparent_idx = apparent_idx;
          state->comp_info[apparent_idx].from_apparent = state->comp_info+idx;
          apparent_idx++;
        }
    }
  else if (access_mode == KDU_WANT_OUTPUT_COMPONENTS)
    { // Restrictions have already been removed on codestream image
      // components, so all we have to do here is apply/remove restrictions
      // on the output image components.
      for (c=0; c < state->num_output_components; c++)
        {
          state->output_comp_info[c].from_apparent = 0;
          state->output_comp_info[c].apparent_idx = -1;
        }
      state->num_apparent_output_components = num_components;
      for (c=apparent_idx=0; c < num_components; c++)
        {
          int idx = component_indices[c];
          if ((idx >= state->num_output_components) ||
              (c >= state->num_output_components) ||
              (state->output_comp_info[idx].apparent_idx >= 0))
            continue;
          state->output_comp_info[idx].apparent_idx = apparent_idx;
          state->output_comp_info[apparent_idx].from_apparent = idx;
          apparent_idx++;
        }
    }
  else
    assert(0);
}

/*****************************************************************************/
/*                    kdu_codestream::change_appearance                      */
/*****************************************************************************/

void
  kdu_codestream::change_appearance(bool transpose, bool vflip, bool hflip)
{
  if (state->tiles_accessed)
    {
      if (state->num_open_tiles != 0)
        { KDU_ERROR_DEV(e,56); e <<
            KDU_TXT("You may change the apparent geometry of the "
            "code-stream only after closing all open tiles.");
        }
      if (state->tiles_accessed && !state->persistent)
        { KDU_ERROR_DEV(e,57); e <<
            KDU_TXT("You may not change the apparent geometry of the "
            "code-stream after the first tile access, unless "
            "the codestream object is set up to be persistent.");
        }
    }
  state->transpose = transpose;
  state->vflip = vflip;
  state->hflip = hflip;
}

/*****************************************************************************/
/*                   kdu_codestream::set_block_truncation                    */
/*****************************************************************************/

void
  kdu_codestream::set_block_truncation(kdu_int32 factor)
{
  if (factor < 0)
    factor = 0;
  if (state != NULL)
    state->block_truncation_factor = factor;
}

/*****************************************************************************/
/*                       kdu_codestream::get_tile_dims                       */
/*****************************************************************************/

void
  kdu_codestream::get_tile_dims(kdu_coords tile_idx,
                                int comp_idx, kdu_dims &dims,
                                bool want_output_comps)
{
  tile_idx.from_apparent(state->transpose,state->vflip,state->hflip);
  assert((tile_idx.x >= 0) && (tile_idx.x < state->tile_span.x) &&
         (tile_idx.y >= 0) && (tile_idx.y < state->tile_span.y));
  dims = state->tile_partition;
  dims.pos.x += tile_idx.x * dims.size.x;
  dims.pos.y += tile_idx.y * dims.size.y;
  dims &= state->canvas;
  if (state->out == NULL)
    dims &= state->region;
  kdu_coords min = dims.pos;
  kdu_coords lim = min + dims.size;
  kdu_coords subs = kdu_coords(1,1);
  if (comp_idx >= 0)
    {
      if (!state->construction_finalized)
        state->finalize_construction();
      kd_comp_info *ci = NULL;
      if (want_output_comps &&
          (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
        {
          if (comp_idx < state->num_apparent_output_components)
            {
              kd_output_comp_info *oci = state->output_comp_info +
                state->output_comp_info[comp_idx].from_apparent;
              ci = oci->subsampling_ref;
            }
        }
      else
        {
          if (comp_idx < state->num_apparent_components)
            ci = state->comp_info[comp_idx].from_apparent;
        }

      subs = ci->sub_sampling;
      subs.x <<= ci->hor_depth[state->discard_levels];
      subs.y <<= ci->vert_depth[state->discard_levels];
    }
  min.x = ceil_ratio(min.x,subs.x);
  min.y = ceil_ratio(min.y,subs.y);
  lim.x = ceil_ratio(lim.x,subs.x);
  lim.y = ceil_ratio(lim.y,subs.y);
  dims.pos = min;
  dims.size = lim-min;
  dims.to_apparent(state->transpose,state->vflip,state->hflip);
}

/*****************************************************************************/
/*                    kdu_codestream::get_num_components                     */
/*****************************************************************************/

int
  kdu_codestream::get_num_components(bool want_output_comps)
{
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    return state->num_apparent_output_components;
  else
    return state->num_apparent_components;
}

/*****************************************************************************/
/*                      kdu_codestream::get_bit_depth                        */
/*****************************************************************************/

int
  kdu_codestream::get_bit_depth(int comp_idx, bool want_output_comps)
{
  if (comp_idx < 0)
    return 0; // Invalid component
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    {
      if (comp_idx >= state->num_apparent_output_components)
        return 0;
      kd_output_comp_info *ci = state->output_comp_info +
        state->output_comp_info[comp_idx].from_apparent;
      return ci->precision;
    }
  else
    {
      if (comp_idx >= state->num_apparent_components)
        return 0;
      kd_comp_info *ci = state->comp_info[comp_idx].from_apparent;
      return ci->precision;
    }
}

/*****************************************************************************/
/*                       kdu_codestream::get_signed                          */
/*****************************************************************************/

bool
  kdu_codestream::get_signed(int comp_idx, bool want_output_comps)
{
  if (comp_idx < 0)
    return false; // Invalid component
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    {
      if (comp_idx >= state->num_apparent_output_components)
        return false;
      kd_output_comp_info *ci = state->output_comp_info +
        state->output_comp_info[comp_idx].from_apparent;
      return ci->is_signed;
    }
  else
    {
      if (comp_idx >= state->num_apparent_components)
        return false;
      kd_comp_info *ci = state->comp_info[comp_idx].from_apparent;
      return ci->is_signed;
    }
}

/*****************************************************************************/
/*                      kdu_codestream::get_subsampling                      */
/*****************************************************************************/

void
  kdu_codestream::get_subsampling(int comp_idx, kdu_coords &subs,
                                  bool want_output_comps)
{
  if (!state->construction_finalized)
    state->finalize_construction();
  if (comp_idx < 0)
    { subs = kdu_coords(0,0); return; }
  kd_comp_info *ci;
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    {
      if (comp_idx >= state->num_apparent_output_components)
        { subs = kdu_coords(0,0); return; }
      kd_output_comp_info *oci = state->output_comp_info +
        state->output_comp_info[comp_idx].from_apparent;
      ci = oci->subsampling_ref;
    }
  else
    {
      if (comp_idx >= state->num_apparent_components)
        { subs = kdu_coords(0,0); return; }
      ci = state->comp_info[comp_idx].from_apparent;
    }
  subs = ci->sub_sampling;
  subs.x <<= ci->hor_depth[state->discard_levels];
  subs.y <<= ci->vert_depth[state->discard_levels];
  if (state->transpose)
    subs.transpose();
}

/*****************************************************************************/
/*                      kdu_codestream::get_registration                     */
/*****************************************************************************/

void
  kdu_codestream::get_registration(int comp_idx, kdu_coords scale,
                                   kdu_coords &reg, bool want_output_comps)
{
  if (!state->construction_finalized)
    state->finalize_construction();
  if (comp_idx < 0)
    { reg = kdu_coords(0,0); return; }
  kd_comp_info *ci;
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    {
      if (comp_idx >= state->num_apparent_output_components)
        { reg = kdu_coords(0,0); return; }
      kd_output_comp_info *oci = state->output_comp_info +
        state->output_comp_info[comp_idx].from_apparent;
      ci = oci->subsampling_ref;
    }
  else
    {
      if (comp_idx >= state->num_apparent_components)
        { reg = kdu_coords(0,0); return; }
      ci = state->comp_info[comp_idx].from_apparent;
    }
  if (state->transpose)
    scale.transpose();
  reg.x = (int) floor(ci->crg_x*scale.x+0.5);
  reg.y = (int) floor(ci->crg_y*scale.y+0.5);
  reg.to_apparent(state->transpose,state->vflip,state->hflip);
}

/*****************************************************************************/
/*                kdu_codestream::get_relative_registration                  */
/*****************************************************************************/

void
  kdu_codestream::get_relative_registration(int comp_idx, int ref_comp_idx,
                                            kdu_coords scale, kdu_coords &reg,
                                            bool want_output_comps)
{
  if (!state->construction_finalized)
    state->finalize_construction();
  if ((comp_idx < 0) || (ref_comp_idx < 0))
    { reg = kdu_coords(0,0); return; }
  kd_comp_info *ci;
  kd_comp_info *ref_ci;
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    {
      if ((comp_idx >= state->num_apparent_output_components) ||
          (ref_comp_idx >= state->num_apparent_output_components))
        { reg = kdu_coords(0,0); return; }
      kd_output_comp_info *oci = state->output_comp_info +
        state->output_comp_info[comp_idx].from_apparent;
      ci = oci->subsampling_ref;
      oci = state->output_comp_info +
        state->output_comp_info[ref_comp_idx].from_apparent;
      ref_ci = oci->subsampling_ref;
    }
  else
    {
      if ((comp_idx >= state->num_apparent_components) ||
          (ref_comp_idx >= state->num_apparent_components))
        { reg = kdu_coords(0,0); return; }
      ci = state->comp_info[comp_idx].from_apparent;
      ref_ci = state->comp_info[ref_comp_idx].from_apparent;
    }
  if (state->transpose)
    scale.transpose();
  float ref_crg_x = ref_ci->crg_x *
    ((float) ref_ci->sub_sampling.x) / ((float) ci->sub_sampling.x);
  float ref_crg_y = ref_ci->crg_y *
    ((float) ref_ci->sub_sampling.y) / ((float) ci->sub_sampling.y);
  reg.x = (int) floor((ci->crg_x-ref_crg_x)*scale.x+0.5);
  reg.y = (int) floor((ci->crg_y-ref_crg_y)*scale.y+0.5);
  reg.to_apparent(state->transpose,state->vflip,state->hflip);
}

/*****************************************************************************/
/*                           kdu_codestream::get_dims                        */
/*****************************************************************************/
    
void
  kdu_codestream::get_dims(int comp_idx, kdu_dims &dims,
                           bool want_output_comps)
{
  if (comp_idx < 0)
    dims = state->region;
  else
    {
      if (!state->construction_finalized)
        state->finalize_construction();
      kd_comp_info *ci = NULL;
      if (want_output_comps &&
          (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
        {
          if (comp_idx < state->num_apparent_output_components)
            {
              kd_output_comp_info *oci = state->output_comp_info +
                state->output_comp_info[comp_idx].from_apparent;
              ci = oci->subsampling_ref;
            }
        }
      else
        {
          if (comp_idx < state->num_apparent_components)
            ci = state->comp_info[comp_idx].from_apparent;
        }

      kdu_coords min = state->region.pos;
      kdu_coords lim = min+state->region.size;
      int y_fact =
        ci->sub_sampling.y << ci->vert_depth[state->discard_levels];
      int x_fact =
        ci->sub_sampling.x << ci->hor_depth[state->discard_levels];

      min.x = ceil_ratio(min.x,x_fact);
      lim.x = ceil_ratio(lim.x,x_fact);
      min.y = ceil_ratio(min.y,y_fact);
      lim.y = ceil_ratio(lim.y,y_fact);
      dims.pos = min;
      dims.size = lim-min;
    }
  dims.to_apparent(state->transpose,state->vflip,state->hflip);
}

/*****************************************************************************/
/*                     kdu_codestream::get_tile_partition                    */
/*****************************************************************************/
    
void
  kdu_codestream::get_tile_partition(kdu_dims &partition)
{
  partition = state->tile_partition;

  // Convert dimensions to those of the entire partition so geometric
  // transformations produce correct `pos' member
  partition.size.x *= state->tile_span.x;
  partition.size.y *= state->tile_span.y;
  partition.to_apparent(state->transpose,state->vflip,state->hflip);
  partition.size = state->tile_partition.size; // Put back the element size
  if (state->transpose)
    partition.size.transpose();
}

/*****************************************************************************/
/*                      kdu_codestream::get_max_tile_layers                  */
/*****************************************************************************/

int
  kdu_codestream::get_max_tile_layers()
{
  return state->max_tile_layers;
}

/*****************************************************************************/
/*                      kdu_codestream::get_min_dwt_levels                   */
/*****************************************************************************/

int
  kdu_codestream::get_min_dwt_levels()
{
  if (state->min_dwt_levels > 32)
    { // Check main header COD marker segment
      int levels;
      kdu_params *cod = state->siz->access_cluster(COD_params);
      if (cod->get(Clevels,0,0,levels) && (levels < state->min_dwt_levels))
        state->min_dwt_levels = levels;
      if (state->min_dwt_levels > 32)
        state->min_dwt_levels = 32;
    }
  return state->min_dwt_levels;
}

/*****************************************************************************/
/*                           kdu_codestream::can_flip                        */
/*****************************************************************************/

bool
  kdu_codestream::can_flip(bool check_current_appearance_only)
{
  return !(state->cannot_flip && (state->hflip || state->vflip));
}

/*****************************************************************************/
/*                          kdu_codestream::map_region                       */
/*****************************************************************************/

void
  kdu_codestream::map_region(int comp_idx, kdu_dims comp_region,
                             kdu_dims &canvas_region, bool want_output_comps)
{
  comp_region.from_apparent(state->transpose,state->vflip,state->hflip);
  kdu_coords min = comp_region.pos;
  kdu_coords lim = min + comp_region.size;
  if (comp_idx >= 0)
    {
      if (!state->construction_finalized)
        state->finalize_construction();
      kd_comp_info *ci = NULL;
      if (want_output_comps &&
          (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
        {
          if (comp_idx < state->num_apparent_output_components)
            {
              kd_output_comp_info *oci = state->output_comp_info +
                state->output_comp_info[comp_idx].from_apparent;
              ci = oci->subsampling_ref;
            }
        }
      else
        {
          if (comp_idx < state->num_apparent_components)
            ci = state->comp_info[comp_idx].from_apparent;
        }

      min.x *= ci->sub_sampling.x << ci->hor_depth[state->discard_levels];
      min.y *= ci->sub_sampling.y << ci->vert_depth[state->discard_levels];
      lim.x *= ci->sub_sampling.x << ci->hor_depth[state->discard_levels];
      lim.y *= ci->sub_sampling.y << ci->vert_depth[state->discard_levels];
    }
  canvas_region.pos = min;
  canvas_region.size = lim-min;
  canvas_region &= state->canvas;
}

/*****************************************************************************/
/*                       kdu_codestream::get_valid_tiles                     */
/*****************************************************************************/

void
  kdu_codestream::get_valid_tiles(kdu_dims &indices)
{
  kdu_coords min = state->region.pos - state->tile_partition.pos;
  kdu_coords lim = min + state->region.size;

  indices.pos.x = floor_ratio(min.x,state->tile_partition.size.x);
  indices.size.x =
    ceil_ratio(lim.x,state->tile_partition.size.x)-indices.pos.x;
  if (lim.x <= min.x)
    indices.size.x = 0;
  indices.pos.y = floor_ratio(min.y,state->tile_partition.size.y);
  indices.size.y =
    ceil_ratio(lim.y,state->tile_partition.size.y) - indices.pos.y;
  if (lim.y <= min.y)
    indices.size.y = 0;
  indices.to_apparent(state->transpose,state->vflip,state->hflip);
}

/*****************************************************************************/
/*                          kdu_codestream::find_tile                        */
/*****************************************************************************/

bool
  kdu_codestream::find_tile(int comp_idx, kdu_coords loc, kdu_coords &tile_idx,
                            bool want_output_comps)
{
  if (!state->construction_finalized)
    state->finalize_construction();
  if (comp_idx < 0)
    return false;
  kd_comp_info *ci = NULL;
  if (want_output_comps &&
      (state->component_access_mode == KDU_WANT_OUTPUT_COMPONENTS))
    {
      if (comp_idx >= state->num_apparent_output_components)
        return false;
      kd_output_comp_info *oci = state->output_comp_info +
        state->output_comp_info[comp_idx].from_apparent;
      ci = oci->subsampling_ref;
    }
  else
    {
      if (comp_idx >= state->num_apparent_components)
        return false;
      ci = state->comp_info[comp_idx].from_apparent;
    }

  loc.from_apparent(state->transpose,state->vflip,state->hflip);
  loc.x *= ci->sub_sampling.x << ci->hor_depth[state->discard_levels];
  loc.y *= ci->sub_sampling.y << ci->vert_depth[state->discard_levels];
  loc -= state->region.pos;
  if ((loc.x < 0) || (loc.y < 0) ||
      (loc.x >= state->region.size.x) || (loc.y >= state->region.size.y))
    return false;
  loc += state->region.pos;
  loc -= state->tile_partition.pos;
  tile_idx.x = floor_ratio(loc.x,state->tile_partition.size.x);
  tile_idx.y = floor_ratio(loc.y,state->tile_partition.size.y);
  tile_idx.to_apparent(state->transpose,state->vflip,state->hflip);
  return true;
}

/*****************************************************************************/
/*                         kdu_codestream::open_tile                         */
/*****************************************************************************/

kdu_tile
  kdu_codestream::open_tile(kdu_coords tile_idx, kdu_thread_env *env)
{
  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);
  state->tiles_accessed = true;
  if (!state->construction_finalized)
    state->finalize_construction();

  tile_idx.from_apparent(state->transpose,state->vflip,state->hflip);
  assert((tile_idx.x >= 0) && (tile_idx.x < state->tile_span.x) &&
         (tile_idx.y >= 0) && (tile_idx.y < state->tile_span.y));

  kdu_coords rel_idx = tile_idx - state->tile_indices.pos;
  assert((rel_idx.x >= 0) && (rel_idx.x < state->tile_indices.size.x) &&
         (rel_idx.y >= 0) && (rel_idx.y < state->tile_indices.size.y));
  kd_tile_ref *tref = state->tile_refs +
    rel_idx.x + rel_idx.y*state->tile_indices.size.x;
  kd_tile *tp = tref->tile;

  if (tp == NULL)
    tp = state->create_tile(tile_idx);
  else if ((tp != KD_EXPIRED_TILE) && tp->needs_reinit)
    { // We must have restarted the code-stream, since the tile was created
      assert(state->allow_restart && (tref->tpart_head == NULL));
      tp->reinitialize();
    }
  if ((tp == KD_EXPIRED_TILE) || tp->closed)
    { KDU_ERROR_DEV(e,58); e << 
        KDU_TXT("Attempting to access a tile which has already been "
        "discarded or closed!");
    }
  assert(tp->tile_ref == tref);
  tp->open();
  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
  return kdu_tile(tp);
}

/*****************************************************************************/
/*                       kdu_codestream::create_tile                         */
/*****************************************************************************/

void kdu_codestream::create_tile(kdu_coords tile_idx, kdu_thread_env *env)
{
  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);
  state->tiles_accessed = true;
  if (!state->construction_finalized)
    state->finalize_construction();
  
  tile_idx.from_apparent(state->transpose,state->vflip,state->hflip);
  assert((tile_idx.x >= 0) && (tile_idx.x < state->tile_span.x) &&
         (tile_idx.y >= 0) && (tile_idx.y < state->tile_span.y));
  
  kdu_coords rel_idx = tile_idx - state->tile_indices.pos;
  assert((rel_idx.x >= 0) && (rel_idx.x < state->tile_indices.size.x) &&
         (rel_idx.y >= 0) && (rel_idx.y < state->tile_indices.size.y));
  kd_tile_ref *tref = state->tile_refs +
  rel_idx.x + rel_idx.y*state->tile_indices.size.x;
  kd_tile *tp = tref->tile;
  
  if (tp == NULL)
    tp = state->create_tile(tile_idx);
  else if ((tp != KD_EXPIRED_TILE) && tp->needs_reinit)
    { // We must have restarted the code-stream, since the tile was created
      assert(state->allow_restart && (tref->tpart_head == NULL));
      tp->reinitialize();
    }
  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
}  

/*****************************************************************************/
/*                        kdu_codestream::get_comment                        */
/*****************************************************************************/

kdu_codestream_comment
  kdu_codestream::get_comment(kdu_codestream_comment prev)
{
  kdu_codestream_comment result;
  if (state != NULL)
    {
      result.state = state->comhead;
      if (prev.state != NULL)
        result.state = prev.state->next;
    }
  return result;
}

/*****************************************************************************/
/*                        kdu_codestream::add_comment                        */
/*****************************************************************************/

kdu_codestream_comment
  kdu_codestream::add_comment()
{
  kdu_codestream_comment result;
  if (state != NULL)
    {
      result.state = new kd_codestream_comment;
      if (state->comtail == NULL)
        state->comhead = state->comtail = result.state;
      else
        state->comtail = state->comtail->next = result.state;
      result.state = state->comtail;
    }
  return result;
}

/*****************************************************************************/
/*                          kdu_codestream::flush                            */
/*****************************************************************************/

void
  kdu_codestream::flush(kdu_long *layer_bytes, int num_layer_specs,
                        kdu_uint16 *thresholds, bool trim_to_rate,
                        bool record_in_comseg, double tolerance,
                        kdu_thread_env *env)
{
  int n;

  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);

  assert((state->out != NULL) && (num_layer_specs > 0));
  if (!state->construction_finalized)
    state->finalize_construction();

  // Start by maintaining the internal record of slope or size thresholds to
  // ensure consistency between multiple calls to this function.
  if (state->layer_targets == NULL)
    { // First call to `flush'
      assert((state->layer_thresholds==NULL) && (state->layer_sizes==NULL));
      state->num_sized_layers = num_layer_specs;
      state->layer_sizes = new kdu_long[num_layer_specs];
      state->layer_targets = new kdu_long[num_layer_specs];
      state->layer_thresholds = new kdu_uint16[num_layer_specs];
      for (n=0; n < num_layer_specs; n++)
        {
          state->layer_sizes[n] = 0;
          state->layer_targets[n] = (layer_bytes==NULL)?0:(layer_bytes[n]);
          state->layer_thresholds[n] = (thresholds==NULL)?0:(thresholds[n]);
        }
    }
  else
    {
      assert((state->layer_thresholds!=NULL) && (state->layer_sizes!=NULL));
      if (num_layer_specs != state->num_sized_layers)
        { KDU_ERROR_DEV(e,59); e <<
            KDU_TXT("When generating code-stream output "
            "incrementally, each call to `kdu_codestream::flush' must provide "
            "the same number of quality layer specifications.");
        }
      if (state->reslength_constraints_used &&
          !state->reslength_warning_issued)
        { KDU_WARNING_DEV(w,0x10110800); w <<
            KDU_TXT("You cannot currently use the `Creslength' parameter "
                    "attribute in conjunction with incremental flushing of "
                    "the codestream.  Ignoring the `Creslength' "
                    "constraints.");
          state->reslength_warning_issued = true;
        }
    }
  bool using_slopes = (state->layer_thresholds[0] != 0);
  bool last_layer_takes_all = // If last layer is to hold all generated data
    (state->layer_targets[num_layer_specs-1] == 0) && !using_slopes;
  if (using_slopes && (thresholds != NULL) && (thresholds[0] != 0))
    { // Update slopes with the new ones supplied by the caller
      // It is safe to update slope thresholds between incremental flushing
      // operations, but not to update layer size targets.
      for (n=0; n < num_layer_specs; n++)
        state->layer_thresholds[n] = thresholds[n];
    }

  // Now for the simulation phase.
  state->reserved_layer_info_bytes = 0;
  if (record_in_comseg &&
      !(state->header_generated || !state->initial_fragment))
    state->set_reserved_layer_info_bytes(num_layer_specs);
  if (using_slopes)
    { // Use slope thresholds to generate the layers directly.
      bool have_constraints = (state->reslength_constraints_used &&
                               !state->reslength_warning_issued);
      kdu_long header_bytes, total_bytes = 0;
      for (n=0; n < num_layer_specs; n++)
        {
          bool last_layer = (n == (num_layer_specs-1));
          if (have_constraints)
            { // Use a simple bisection search for the slope threshold
              int best_threshold = -1;
              int min_threshold = state->layer_thresholds[n];
              int lim_threshold = (1<<16);
              if (n > 0)
                lim_threshold = state->layer_thresholds[n-1]+1;
              do {
                  int threshold = (min_threshold+lim_threshold)>>1;
                  assert(threshold < (1<<16));
                  state->reslength_constraints_violated = false;
                  state->simulate_output(header_bytes,n,(kdu_uint16)threshold,
                                         false,last_layer);
                  if (state->reslength_constraints_violated)
                    min_threshold = threshold+1;
                  else
                    {
                      best_threshold = threshold; // Best so far
                      lim_threshold = threshold;
                    }
                } while (lim_threshold > min_threshold);
              if (best_threshold < 0)
                { KDU_WARNING_DEV(w,0x11110800); w <<
                    KDU_TXT("Unable to find modified distortion-length "
                            "slope thresholds which can satisfy the "
                            "compressed length constraints imposed by the "
                            "supplied `Creslength' parameter attribute(s).");
                  state->reslength_warning_issued = true;
                }
              else
                state->layer_thresholds[n] = (kdu_uint16) best_threshold;
            }
          kdu_uint16 thresh = state->layer_thresholds[n];
          total_bytes +=
            state->simulate_output(header_bytes,n,thresh,true,last_layer);
          state->layer_targets[n] = total_bytes;
        }
    }
  else
    { // Use layer targets.
      if (trim_to_rate)
        { // First check to see if we are allowed to trim to the exact rate yet
          if (state->reslength_constraints_used)
            trim_to_rate = false;
          kd_global_rescomp *rc = state->global_rescomps;
          for (n=state->num_components*33; (n > 0) && trim_to_rate; n--, rc++)
            if (rc->ready_area < rc->remaining_area)
              trim_to_rate = false;
        }
      state->pcrd_opt(trim_to_rate,tolerance);
    }

  // Generate the code-stream
  if (state->reserved_layer_info_bytes && state->initial_fragment &&
      !state->header_generated)
    state->gen_layer_info_comment(num_layer_specs,state->layer_targets,
                                  state->layer_thresholds);
  state->generate_codestream(num_layer_specs);

  // Write results back to user-supplied arrays if available
  if (thresholds != NULL)
    for (n=0; n < num_layer_specs; n++)
      thresholds[n] = state->layer_thresholds[n];
  if (layer_bytes != NULL)
    {
      kdu_long generated_bytes=0;
      for (n=0; n < num_layer_specs; n++)
        layer_bytes[n] = (generated_bytes += state->layer_sizes[n]);
    }

  // Update arrays
  if (!using_slopes)
    for (n=0; n < num_layer_specs; n++)
      state->layer_thresholds[n] = 0;
  if (last_layer_takes_all)
    { // Make sure next incremental call to flush not limited by current size
      state->layer_targets[num_layer_specs-1] = 0;
    }

  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
}

/*****************************************************************************/
/*                         kdu_codestream::trans_out                         */
/*****************************************************************************/

int
  kdu_codestream::trans_out(kdu_long max_bytes,
                            kdu_long *layer_bytes, int layer_bytes_entries,
                            bool record_in_comseg, kdu_thread_env *env)
{
  int n;

  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);

  assert(state->out != NULL);
  if (!state->construction_finalized)
    state->finalize_construction();
  if (state->reslength_constraints_used && !state->reslength_warning_issued)
    { KDU_WARNING_DEV(w,0x10110801); w <<
        KDU_TXT("You cannot currently use the `Creslength' parameter "
                "attribute in conjunction with `kdu_codestream::trans_out' "
                "(i.e., you cannot use this parameter to control the "
                "compressed lengths of individual resolutions during "
                "transcoding).  Ignoring the `Creslength' constraints.");
      state->reslength_warning_issued = true;
    }
  if (max_bytes == 0)
    max_bytes = KDU_LONG_HUGE;
  if (state->layer_targets == NULL)
    { // Set up `num_sized_layers' and internal arrays on this first call
      state->num_sized_layers=1;
      for (kd_tile *tile=state->tiles_in_progress_head;
           tile != NULL; tile=tile->in_progress_next)
        if (tile->num_layers > state->num_sized_layers)
          state->num_sized_layers = tile->num_layers;
      state->layer_sizes = new kdu_long[state->num_sized_layers];
      state->layer_targets = new kdu_long[state->num_sized_layers];
      state->layer_thresholds = new kdu_uint16[state->num_sized_layers];
      for (n=0; n < state->num_sized_layers; n++)
        {
          state->layer_sizes[n] = 0;
          state->layer_targets[n] = 0;
          state->layer_thresholds[n] = 0xFFFF - n - 1;
        }
      state->layer_targets[n-1] = max_bytes;
    }
  else
    {
      assert(state->num_sized_layers > 0);
      max_bytes = state->layer_targets[state->num_sized_layers-1];
    }

  kdu_long save_max_bytes = max_bytes;

  state->reserved_layer_info_bytes = 0;
  if (record_in_comseg && state->initial_fragment &&
      !state->header_generated)
    state->set_reserved_layer_info_bytes(state->num_sized_layers);

  // Run simulations until we find a suitable number of non-empty layers
  if (state->final_fragment)
    max_bytes -= 2; // Allow room for the EOC marker.
  for (n=0; n < state->num_sized_layers; n++)
    max_bytes -= state->layer_sizes[n];
  kdu_long total_bytes, header_bytes;
  int num_layers = state->num_sized_layers;
  bool last_layer;
  do {
      total_bytes = 0;
      for (n=0; n < num_layers; n++)
        {
          last_layer = (n == (num_layers-1));
          kdu_uint16 threshold = state->layer_thresholds[n];
          if (last_layer)
            threshold = 0xFFFF; // This way we will just count the header cost
          total_bytes +=
            state->simulate_output(header_bytes,n,threshold,true,last_layer);
          if (total_bytes >= max_bytes)
            {
              if (last_layer)
                {
                  num_layers = n; // Could not even afford the header cost.
                  last_layer = false; // Make sure we iterate.
                }
              else
                num_layers = n+1; // Try running current layer as the last one,
                                  // where we count only the cost of headers
            }
        }
      if (num_layers == 0)
        { KDU_ERROR(e,60); e <<
            KDU_TXT("You have set the byte limit too low.  "
            "All compressed data would have to be discarded in the call to "
            "`kdu_codestream::trans_out'!");
        }
    } while (!last_layer);

  // Now go back through all the layers, finalizing the allocation.
  total_bytes = 0;
  for (n=0; n < num_layers; n++)
    {
      last_layer = (n == (num_layers-1));
      kdu_uint16 threshold = state->layer_thresholds[n];
      if (!last_layer)
        {
          total_bytes +=
            state->simulate_output(header_bytes,n,threshold,true,last_layer);
          assert(total_bytes < max_bytes);
        }
      else
        { // Last layer is a sloppy one
          kdu_long trial_bytes = total_bytes +
            state->simulate_output(header_bytes,n,threshold+1,false,true);
          assert(trial_bytes <= max_bytes); // Header cost only.
          kdu_long sloppy_bytes = max_bytes - trial_bytes;
          total_bytes +=
            state->simulate_output(header_bytes,n,threshold,true,true,
                                   max_bytes-total_bytes,&sloppy_bytes);
          assert(total_bytes <= max_bytes);
        }
      state->layer_targets[n] = total_bytes;
    }

  if (state->reserved_layer_info_bytes && state->initial_fragment &&
      !state->header_generated)
    state->gen_layer_info_comment(num_layers,state->layer_targets,
                                  state->layer_thresholds);

  // Generate the code-stream.
  state->generate_codestream(num_layers);
  state->layer_targets[state->num_sized_layers-1] = save_max_bytes;

  if (layer_bytes != NULL)
    {
      kdu_long generated_bytes = 0;
      for (n=0; n < layer_bytes_entries; n++)
        if (n < state->num_sized_layers)
          layer_bytes[n] = (generated_bytes += state->layer_sizes[n]);
        else
          layer_bytes[n] = generated_bytes;
    }

  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
  return num_layers;
}

/*****************************************************************************/
/*                      kdu_codestream::ready_for_flush                      */
/*****************************************************************************/

bool
  kdu_codestream::ready_for_flush(kdu_thread_env *env)
{
  if ((state == NULL) || (state->out == NULL))
    return false;

  if (env != NULL)
    env->acquire_lock(KD_THREADLOCK_GENERAL);
  for (kd_tile *tile = state->tiles_in_progress_head;
       tile != NULL; tile=tile->in_progress_next)
    {
      kd_precinct *precinct;
      kd_precinct_ref *p_ref;
      kd_resolution *p_res;
      kdu_coords p_idx;
      p_ref = tile->sequencer->next_in_sequence(p_res,p_idx);
      if (p_ref != NULL)
        {
          precinct = p_ref->open(p_res,p_idx,true);
          assert(precinct != NULL);
          if (precinct->num_outstanding_blocks == 0)
            {
              if (env != NULL)
                env->release_lock(KD_THREADLOCK_GENERAL);
              return true; // We have something which can be flushed.
            }
        }
    }
  if (env != NULL)
    env->release_lock(KD_THREADLOCK_GENERAL);
  return false;
}

/*****************************************************************************/
/*                      kdu_codestream::get_total_bytes                      */
/*****************************************************************************/

kdu_long
  kdu_codestream::get_total_bytes(bool exclude_main_header)
{
  kdu_long excluded = 0;
  if (exclude_main_header)
    excluded = state->header_length;
  if (state->in != NULL)
    return (state->in->get_bytes_read() -
            state->in->get_suspended_bytes() - excluded);
  else if (state->out != NULL)
    return (state->out->get_bytes_written() - excluded);
  else
    return 0;
}

/*****************************************************************************/
/*                      kdu_codestream::get_num_tparts                       */
/*****************************************************************************/

int
  kdu_codestream::get_num_tparts()
{
  return state->num_completed_tparts;
}

/*****************************************************************************/
/*                   kdu_codestream::collect_timing_stats                    */
/*****************************************************************************/

void
  kdu_codestream::collect_timing_stats(int num_coder_iterations)
{
  if (num_coder_iterations < 0)
    num_coder_iterations = 0;
  state->block->initialize_timing(num_coder_iterations);
  state->start_time = clock();
}

/*****************************************************************************/
/*                     kdu_codestream::get_timing_stats                      */
/*****************************************************************************/

double
  kdu_codestream::get_timing_stats(kdu_long *num_samples, bool coder_only)
{
  double system_time = clock() - state->start_time;
  system_time /= CLOCKS_PER_SEC;
  kdu_long system_samples = 0;
  for (int c=0; c < state->num_apparent_components; c++)
    {
      kdu_dims region; get_dims(c,region);
      system_samples += region.area();
    }

  kdu_long coder_samples;
  double wasted_time;
  double coder_time =
    state->block->get_timing_stats(coder_samples,wasted_time);
  system_time -= wasted_time;

  if (coder_only)
    {
      if (num_samples != NULL)
        *num_samples = coder_samples;
      return coder_time;
    }
  if (num_samples != NULL)
    *num_samples = system_samples;
  return system_time;
}

/*****************************************************************************/
/*                kdu_codestream::get_compressed_data_memory                 */
/*****************************************************************************/

kdu_long
  kdu_codestream::get_compressed_data_memory(bool get_peak_allocation)
{
  if (get_peak_allocation)
    return state->buf_server->get_peak_buf_bytes();
  else
    return state->buf_server->get_current_buf_bytes();
}

/*****************************************************************************/
/*                kdu_codestream::get_compressed_state_memory                */
/*****************************************************************************/

kdu_long
  kdu_codestream::get_compressed_state_memory(bool get_peak_allocation)
{
  if (get_peak_allocation)
    return state->buf_server->get_peak_structure_bytes();
  else
    return state->buf_server->get_current_structure_bytes();
}
