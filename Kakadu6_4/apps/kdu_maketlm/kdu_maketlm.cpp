/*****************************************************************************/
// File: kdu_maketlm.cpp [scope = APPS/MAKETLM]
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
   Small application which reads an existing JPEG2000 code-stream, or JP2
file, and writes a new one (raw code-stream or JP2 file, as appropriate), with
TLM (Tile-Part Length) marker segments included in the main header.  Note
that this application does not actually depend upon the Kakadu core system
(usually in a static or shared library, e.g., kdu_v30.dll).
Tile-part length information facilitates efficient seeking by decompression
applications when working with compressed data sources which support random
access.  The reason for adding the information in a separate pass, after
compression, is that computation of tile-part length information is
inherently a multi-pass operation.
******************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "kdu_messaging.h"
#include "kdu_utils.h"
#include "kdu_maketlm.h"

static inline kdu_uint32
  string_to_int(const char *string)
{
  assert((string[0] != '\0') && (string[1] != '\0') &&
         (string[2] != '\0') && (string[3] != '\0') && (string[4] == '\0'));
  kdu_uint32 result = ((kdu_byte) string[0]);
  result = (result<<8) + ((kdu_byte) string[1]);
  result = (result<<8) + ((kdu_byte) string[2]);
  result = (result<<8) + ((kdu_byte) string[3]);
  return result;
}

static const kdu_uint32 j2_codestream_box = string_to_int("jp2c");

/* ========================================================================= */
/*                         Set up messaging services                         */
/* ========================================================================= */

class kdu_stream_message : public kdu_message {
  public: // Member classes
    kdu_stream_message(std::ostream *stream)
      { this->stream = stream; }
    void put_text(const char *string)
      { (*stream) << string; }
    void flush(bool end_of_message=false)
      { stream->flush(); }
  private: // Data
    std::ostream *stream;
  };

static kdu_stream_message cout_message(&std::cout);
static kdu_stream_message cerr_message(&std::cerr);
static kdu_message_formatter pretty_cout(&cout_message);
static kdu_message_formatter pretty_cerr(&cerr_message);


/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        print_usage                                 */
/*****************************************************************************/

static void
  print_usage(char *prog)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage:\n  \"" << prog << " ...\n";
  out.set_master_indent(3);
  out << "<input file> <output file>\n";
  out << "\tThe input file is treated as a JP2 file if it has a \".jp2\" "
         "suffix.  Otherwise, it is treated as a raw code-stream.  The "
         "output file will have the same type (JP2 or raw code-stream) as "
         "the input file and so must have the same suffix.\n";
  out.flush();
  exit(0);
}

/*****************************************************************************/
/* STATIC                      check_jp2_suffix                              */
/*****************************************************************************/

static bool
  check_jp2_suffix(char *fname)
  /* Returns true if the file-name has the suffix, ".jp2", where the
     check is case insensitive. */
{
  const char *cp = strrchr(fname,'.');
  if (cp == NULL)
    return false;
  cp++;
  if ((*cp != 'j') && (*cp != 'J'))
    return false;
  cp++;
  if ((*cp != 'p') && (*cp != 'P'))
    return false;
  cp++;
  if (*cp != '2')
    return false;
  return true;
}

/* ========================================================================= */
/*                               kdu_input_box                               */
/* ========================================================================= */

/*****************************************************************************/
/*                         kdu_input_box::read_header                        */
/*****************************************************************************/

void
  kdu_input_box::read_header()
{
  remaining_bytes = 0xFFFFFFFF; // So initial reads do not fail.
  box_type = 1; // Dummy value until we can read the actual length
  if (!(read(box_bytes) && read(box_type) && (box_type != 0)))
    { box_bytes = remaining_bytes = box_type = 0; return; }
  if (box_bytes == 1)
    { // 64-bit box length
      kdu_uint32 box_bytes_high;
      if (!(read(box_bytes_high) && read(box_bytes)))
        { box_bytes = remaining_bytes = box_type = 0; return; }
      if (box_bytes_high)
        box_bytes = 0; // Can't hold such big lengths. Treat as rubber length.
      else if (box_bytes < 16)
        { kdu_error e;
          e << "Illegal extended box length encountered in JP2 file."; }
      remaining_bytes = box_bytes - 16;
    }
  else if (box_bytes != 0)
    {
      if (box_bytes < 8)
        { kdu_error e;
          e << "Illegal box length field encountered in JP2 file."; }
      remaining_bytes = box_bytes - 8;
    }
}

/*****************************************************************************/
/*                            kdu_input_box::close                           */
/*****************************************************************************/

bool
  kdu_input_box::close()
{
  if (box_type == 0)
    return true;
  if (box_bytes == 0)
    { // Rubber length box.
      box_bytes = remaining_bytes = box_type = 0;
      char dummy;
      if (fread(&dummy,1,1,file) > 0)
        return false;
      return true;
    }
  else if (remaining_bytes > 0)
    {
      fseek(file,remaining_bytes,SEEK_CUR);
      remaining_bytes = box_bytes = box_type = 0;
      return false;
    }
  remaining_bytes = box_bytes = box_type = 0;
  return true;
}

/*****************************************************************************/
/*                         kdu_input_box::read (array)                       */
/*****************************************************************************/

int
  kdu_input_box::read(kdu_byte buf[], int num_bytes)
{
  if (box_type == 0)
    { kdu_error e; e << "Attempting to read from a closed JP2 file box."; }
  if ((box_bytes != 0) && (remaining_bytes < (kdu_uint32) num_bytes))
    num_bytes = remaining_bytes;
  num_bytes = (int) fread(buf,1,(size_t) num_bytes,file);
  remaining_bytes -= (kdu_uint32) num_bytes;
  return num_bytes;
}

/*****************************************************************************/
/*                         kdu_input_box::read (dword)                       */
/*****************************************************************************/

bool
  kdu_input_box::read(kdu_uint32 &dword)
{
  kdu_byte buf[4];
  if (read(buf,4) < 4)
    return false;
  dword = buf[0];
  dword = (dword<<8) + buf[1];
  dword = (dword<<8) + buf[2];
  dword = (dword<<8) + buf[3];
  return true;
}

/*****************************************************************************/
/*                         kdu_input_box::read (word)                        */
/*****************************************************************************/

bool
  kdu_input_box::read(kdu_uint16 &word)
{
  kdu_byte buf[2];
  if (read(buf,2) < 2)
    return false;
  word = buf[0];
  word = (word<<8) + buf[1];
  return true;
}


/* ========================================================================= */
/*                              kdu_output_box                               */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdu_output_box::write_header                        */
/*****************************************************************************/

void
  kdu_output_box::write_header()
{
  if (rubber_length)
    { write((kdu_uint32) 0); write(box_type); }
  else
    {
      kdu_uint32 box_bytes = 8 + (kdu_uint32) buffered_bytes;
      rubber_length = true; // So that the header is output directly.
      write(box_bytes); write(box_type);
      rubber_length = false;
    }
}

/*****************************************************************************/
/*                           kdu_output_box::close                           */
/*****************************************************************************/

bool
  kdu_output_box::close()
{
  if (buffer != NULL)
    {
      assert(!rubber_length);
      write_header();
      output_failed = (fwrite(buffer,1,(size_t) buffered_bytes,file) !=
                       (size_t) buffered_bytes);
      delete[] buffer;
      buffer_size = buffered_bytes = 0; buffer = NULL;
    }
  box_type = 0;
  return !output_failed;
}

/*****************************************************************************/
/*                       kdu_output_box::write (buffer)                      */
/*****************************************************************************/

bool
  kdu_output_box::write(kdu_byte buf[], int num_bytes)
{
  if ((box_type == 0) || output_failed)
    return false;
  if (rubber_length)
    { // Flush data directly to the output.
      output_failed =
        (fwrite(buf,1,(size_t) num_bytes,file) != (size_t) num_bytes);
      return !output_failed;
    }

  buffered_bytes += num_bytes;
  if (buffered_bytes > buffer_size)
    {
      buffer_size += buffered_bytes + 1024;
      kdu_byte *tmp_buf = new kdu_byte[buffer_size];
      if (buffer != NULL)
        {
          memcpy(tmp_buf,buffer,(size_t)(buffered_bytes-num_bytes));
          delete[] buffer;
        }
      buffer = tmp_buf;
    }
  memcpy(buffer+buffered_bytes-num_bytes,buf,(size_t) num_bytes);
  return true;
}

/*****************************************************************************/
/*                        kdu_output_box::write (dword)                      */
/*****************************************************************************/

bool
  kdu_output_box::write(kdu_uint32 dword)
{
  kdu_byte buf[4];
  buf[3] = (kdu_byte) dword; dword >>= 8;
  buf[2] = (kdu_byte) dword; dword >>= 8;
  buf[1] = (kdu_byte) dword; dword >>= 8;
  buf[0] = (kdu_byte) dword;
  return write(buf,4);
}

/*****************************************************************************/
/*                        kdu_output_box::write (word)                       */
/*****************************************************************************/

bool
  kdu_output_box::write(kdu_uint16 word)
{
  kdu_byte buf[2];
  buf[1] = (kdu_byte) word; word >>= 8;
  buf[0] = (kdu_byte) word;
  return write(buf,2);
}


/* ========================================================================= */
/*                              kdu_codestream_xfer                          */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdu_codestream_xfer::open                       */
/*****************************************************************************/

void
  kdu_codestream_xfer::open(char *in_fname, char *out_fname)
{
  in_file = fopen(in_fname,"rb");
  if (in_file == NULL)
    { kdu_error e; e << "Unable to open input file, \"" << in_fname << "\"."; }
  if (check_jp2_suffix(in_fname))
    {
      in_box = new kdu_input_box;
      if (out_fname != NULL)
        {
          if (!check_jp2_suffix(out_fname))
            { kdu_error e; e << "Output file should have the suffix \".jp2\" "
              "if and only if the input file has this suffix!"; }
          out_box = new kdu_output_box;
        }
    }
  if (out_fname != NULL)
    {
      out_file = fopen(out_fname,"wb");
      if (out_file == NULL)
        { kdu_error  e;
          e << "Unable to open output file, \"" << out_fname << "\"."; }
    }
  if (in_box != NULL)
    { // Read to the contiguous codestream box.
      bool codestream_box = false;
      while (in_box->open(in_file).exists())
        {
          codestream_box = (in_box->get_box_type() == j2_codestream_box);

          if (out_box != NULL)
            out_box->open(out_file,in_box->get_box_type(),codestream_box);
          if (codestream_box)
            break;
          if (out_box != NULL)
            {
              kdu_byte byte;
              while (in_box->read(byte))
                out_box->write(byte);
              out_box->close();
            }
          in_box->close();
        }
      if (!codestream_box)
        { kdu_error e; e << "The supplied JP2 (or JP2 compatible) input file "
          "does not contain a contiguous codestream box."; }
    }
  in_tile_body = false;
}

/*****************************************************************************/
/*                           kdu_codestream_xfer::close                      */
/*****************************************************************************/

void
  kdu_codestream_xfer::close()
{
  if (in_box != NULL)
    {
      in_box->close();
      if (in_box->open(in_file).exists())
        { kdu_warning w;
          w << "The supplied JP2 file contains boxes beyond the "
          "contiguous code-stream box.  These will be discarded!"; }
      delete in_box;
    }
  if (out_box != NULL)
    delete out_box;
  in_box = NULL; out_box = NULL;
  if (in_file != NULL)
    fclose(in_file);
  if (out_file != NULL)
    fclose(out_file);
  in_file = out_file = NULL;
}

/*****************************************************************************/
/*                        kdu_codestream_xfer::read_marker                   */
/*****************************************************************************/

bool
  kdu_codestream_xfer::read_marker(kdu_marker_seg &seg)
{
  kdu_uint16 code = 0;
  kdu_byte byte;

  assert(in_file != NULL);
  while (read_byte(byte))
    {
      if (code == 0xFF)
        {
          code = (code << 8) + byte;
          if (in_tile_body && (code <= 0xFF8F))
            write_byte(0xFF);
          else
            {
              kdu_byte *buf = seg.start_seg(code);
              kdu_uint16 length = 0;

              if ((code == KDU_SOT) || (code == KDU_SOP) ||
                  (code == KDU_SIZ) || (code == KDU_COD) ||
                  (code == KDU_COC) || (code == KDU_QCD) ||
                  (code == KDU_QCC) || (code == KDU_RGN) ||
                  (code == KDU_POC) || (code == KDU_CRG) ||
                  (code == KDU_COM) || (code == KDU_TLM) ||
                  (code == KDU_PLM) || (code == KDU_PLT) ||
                  (code == KDU_PPM) || (code == KDU_PPT))
                { // Marker code has associated marker segment.
                  read_byte(byte); length = byte; length <<= 8;
                  read_byte(byte); length += byte;
                  length -= 2;
                  if (in_box != NULL)
                    in_box->read(buf,length);
                  else
                    fread(buf,1,length,in_file);
                }
              seg.end_seg(length);
              if (code == KDU_SOD)
                in_tile_body = true;
              else if (code == KDU_SOT)
                in_tile_body = false;
              code = 0;
              return true;
            }
        }
      code = byte;
      if (code != 0xFF)
        {
          if (in_tile_body)
            write_byte(byte);
          else
            { kdu_error e; e << "Input code-stream appears to be corrupt.  "
              "Expected to encounter a marker code."; }
        }
    }

  return false;
}

/*****************************************************************************/
/*                       kdu_codestream_xfer::write_marker                   */
/*****************************************************************************/

void
  kdu_codestream_xfer::write_marker(kdu_marker_seg &seg)
{
  kdu_uint16 code = seg.get_code();
  kdu_uint16 length = seg.get_length();
  kdu_byte *buf = seg.get_body();

  if (out_box != NULL)
    {
      out_box->write(code);
      if (length)
        {
          out_box->write((kdu_uint16)(length+2));
          out_box->write(buf,length);
        }
    }
  else if (out_file != NULL)
    {
      putc((code >> 8),out_file);
      putc((code & 0xFF),out_file);
      if (length)
        {
          putc((length+2) >> 8,out_file);
          putc((length+2) & 0xFF,out_file);
          fwrite(buf,1,length,out_file);
        }
    }
  else
    { kdu_error e; e << "Attempting to write marker segment to non-existent "
      "output file!"; }
}


/* ========================================================================= */
/*                                kdu_tlm_info                               */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdu_tlm_info::generate_segment                      */
/*****************************************************************************/

bool
  kdu_tlm_info::generate_segment(kdu_marker_seg &seg)
{
  tlm_record *scan;
  int cur_seg_records, max_seg_records;
  bool long_tnums = false;
  bool long_lengths = false;

  if (head == NULL)
    return false;

  // First determine the most efficient representation parameters
  if (need_tnums)
    max_seg_records = ((1<<16)-5)/6; // Need to send lengths and tile indices
  else
    max_seg_records = ((1<<16)-5)/4; // Need only send lengths
  for (cur_seg_records=0, scan=head;
       (scan != NULL) && (cur_seg_records < max_seg_records);
       scan=scan->next, cur_seg_records++)
    {
      if (scan->tnum > 254) // This weird limit is described in the standard
        long_tnums = true;          // It was an error/typo, which has stuck
      if (scan->length & 0xFFFF0000)
        long_lengths = true;
    }
  max_seg_records = cur_seg_records;

  // Now write the marker segment.
  kdu_byte *buf = seg.start_seg(KDU_TLM);
  kdu_byte *bp = buf;
  *(bp++) = ztlm++;
  if (ztlm == 0)
    { kdu_error e; e << "This image has a huge number of tile-parts.  Cannot "
      "fit all the tile-part length information into the maximum of 256 "
      "TLM marker segments permitted by the standard!"; }
  kdu_byte stlm = (kdu_byte)((long_lengths)?(1<<6):0);
  if (need_tnums)
    stlm += (kdu_byte)((long_tnums)?(1<<5):(1<<4));
  *(bp++) = stlm;
  for (cur_seg_records=0; cur_seg_records < max_seg_records; cur_seg_records++)
    {
      scan = head;
      head = scan->next;
      if (need_tnums)
        {
          if (long_tnums)
            *(bp++) = (kdu_byte)(scan->tnum >> 8);
          *(bp++) = (kdu_byte)(scan->tnum);
        }
      if (long_lengths)
        {
          *(bp++) = (kdu_byte)(scan->length >> 24);
          *(bp++) = (kdu_byte)(scan->length >> 16);
        }
      *(bp++) = (kdu_byte)(scan->length >> 8);
      *(bp++) = (kdu_byte)(scan->length >> 0);
      delete scan;
    }
  seg.end_seg((kdu_uint16)(bp-buf));
  return true;
}


/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN                           main                                     */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);
  if (argc != 3)
    print_usage(argv[0]);

  kdu_codestream_xfer stream;
  kdu_marker_seg seg;
  int num_tparts = 0;

  // First open only the input file and collect tile-part lengths.
  kdu_tlm_info tlm_info;
  stream.open(argv[1],NULL);
  if ((!stream.read_marker(seg)) || (seg.get_code() != KDU_SOC))
    { kdu_error e; e << "Expected, but did not find, the SOC (Start Of "
      "Codestream) Marker."; }
  while (stream.read_marker(seg) && (seg.get_code() != KDU_EOC))
    {
      if (seg.get_code() == KDU_SOT)
        {
          if (seg.get_length() != 8)
            { kdu_error e; e << "Illegal or misdetected SOT marker segment "
              "encountered in input file.  SOT marker segments must have "
              "exactly 8 bytes after the initial 2 byte length field."; }
          num_tparts++;
          kdu_byte *buf = seg.get_body();
          kdu_byte *buf_end = buf+8;
          kdu_uint16 tnum = (kdu_uint16) kdu_read(buf,buf_end,2);
          kdu_uint32 tplen = (kdu_uint32) kdu_read(buf,buf_end,4);
          tlm_info.add_record(tnum,tplen);
        }
    }
  stream.close();

  // Now open both files and copy the contents, inserting TLM markers.
  stream.open(argv[1],argv[2]);
  while (stream.read_marker(seg))
    {
      if (seg.get_code() != KDU_TLM)
        stream.write_marker(seg);
      if (seg.get_code() == KDU_EOC)
        break;
      if (seg.get_code() == KDU_SIZ)
        { // Generate the TLM marker segments now.
          while (tlm_info.generate_segment(seg))
            stream.write_marker(seg);
        }
    }
  stream.close();

  // Report statistics
  pretty_cout << "\tProcessed " << num_tparts << " tile parts.\n";

  return 0;
}
