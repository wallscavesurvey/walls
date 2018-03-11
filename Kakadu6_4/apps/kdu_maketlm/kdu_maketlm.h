/*****************************************************************************/
// File: kdu_maketlm.h [scope = APPS/MAKETLM]
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
   Local definitions for small application which reads an existing
JPEG2000 code-stream, or JP2 file, and writes a new one (raw code-stream or
JP2 file, as appropriate), with TLM (Tile-Part Length) marker segments
included in the main header.
******************************************************************************/

#ifndef KDU_MAKETLM_H
#define KDU_MAKETLM_H

#include <stdio.h>
#include <assert.h>
#include "kdu_compressed.h"

/*****************************************************************************/
/*                              kdu_input_box                                */
/*****************************************************************************/

  /* Note: This is a somewhat simplified version of the class of the same
     name defined in "jp2_local.h" -- we are not using the JP2 services here,
     since we have no interest in interpreting the contents of boxes. */
class kdu_input_box {
  public: // Member functions
    kdu_input_box()
      { box_type = 0; file = NULL; }
    ~kdu_input_box()
      { // Closes the box if necessary. */
        close();
      }
    bool operator!()
      { return (box_type == 0); }
    bool exists()
      { return (box_type != 0); }
    kdu_input_box &open(FILE *file)
      {
        assert(box_type == 0); // Must have been previously closed.
        this->file = file; read_header(); return *this;
      }
    bool close();
      /* Skips over any outstanding bytes which have not been read from the
         body of the box.  The function returns false if there were some
         unread bytes, which may be an error or warning condition.  Otherwise,
         it returns true.  Also returns true if the box has already been
         closed.  Subsequent calls to `exists' will return false. */
    kdu_uint32 get_box_type()
      { return box_type; }
      /* Returns the box_type, which is usually a 4 character code.  Returns
         0 if the box could not be opened.  This is also the condition which
         causes `exists' to return false. */
    int read(kdu_byte *buf, int num_bytes);
      /* Attempts to read the indicated number of bytes from the body of the
         box, filling the supplied buffer with these bytes.  Returns the
         actual number of bytes which were read, which may be smaller than
         `num_bytes' if the end of the box (or surrounding file) is
         encountered. */
    bool read(kdu_uint32 &dword);
      /* Reads a big-endian 4 byte word from the box, returning false if
         the source is exhausted. */
    bool read(kdu_uint16 &word);
      /* Reads a big-endian 2 byte word from the box, returning false if
         the source is exhausted. */
    bool read(kdu_byte &byte)
      { return (read(&byte,1) == 1); }
      /* Reads a single byte from the box, returning false if the source
         is exhausted. */
  private: // Helper functions
    void read_header();
  private: // Data
    kdu_uint32 box_type;
    kdu_uint32 box_bytes; // 0 if box has rubber length or length too large.
    kdu_uint32 remaining_bytes; // Unused if `box_bytes' = 0.
    FILE *file;
  };

/*****************************************************************************/
/*                             kdu_output_box                                */
/*****************************************************************************/

  /* Note: This is a somewhat simplified version of the class of the same
     name defined in "jp2_local.h" -- we are not using the JP2 services here,
     since we are interested only in copying the contents of boxes. */
class kdu_output_box {
  public: // Member functions
    kdu_output_box()
      { box_type = 0; rubber_length = output_failed = false;
        file = NULL; buffer_size = buffered_bytes = 0; buffer = NULL; }
    ~kdu_output_box()
      { close(); }
    kdu_output_box &
      open(FILE *file, kdu_uint32 box_type, bool rubber_length=false)
      {
        assert((this->box_type == 0) && (buffer == NULL));
        this->box_type = box_type; this->rubber_length = rubber_length;
        this->file = file; buffer_size = buffered_bytes = 0;
        output_failed = false;
        if (rubber_length) write_header();
        return *this;
      }
    bool close();
      /* Flushes the contents of the box to the output.  If the box had
         rubber length, there will be nothing to flush and this function
         will return false, meaning that no further boxes may be opened.  The
         function may also return false if the box contents cannot be
         completely flushed to the output device for some reason (e.g., disk
         full).  If the box has already been closed (or never opened), this
         function returns true. */
    bool write(kdu_byte *buf, int num_bytes);
      /* Writes the indicated number of bytes to the body of the box, from
         the supplied buffer.  Returns false if the output device is unable
         to receive all of the supplied data (e.g., disk full). */
    bool write(kdu_uint32 dword);
      /* Writes a big-endian 32-bit unsigned integer to the body of the box,
         returning false if the output device is unable to receive all of
         the supplied data for some reason. */
    bool write(kdu_uint16 word);
      /* Writes a big-endian 16-bit unsigned integer to the body of the box,
         returning false if the output device is unable to receive all of
         the supplied data for some reason. */
    bool write(kdu_byte byte)
      { return write(&byte,1); }
      /* Writes a single byte to the body of the box, returning false if the
         output device is unable to receive all of the supplied data for some
         reason. */
  private: // Helper functions
    void write_header();
  private: // Data
    kdu_uint32 box_type; // 0 if box does not exist or has been closed.
    bool rubber_length;
    FILE *file;
    int buffer_size, buffered_bytes;
    kdu_byte *buffer;
    bool output_failed; // True if the output device failed to absorb all data
  };

/*****************************************************************************/
/*                               kdu_marker_seg                              */
/*****************************************************************************/

class kdu_marker_seg {
  public: // Member functions
    kdu_marker_seg()
      { writing=false; marker_code=0; body_length = 0; }
    kdu_byte *start_seg(kdu_uint16 marker_code)
      /* Starts a new marker segment, having the indicated marker code.  The
         marker segment must be finished by a call to `end_seg' before calls
         to `get_code', `get_length' or `get_body' will be considered legal.
         Returns a pointer to the body of the marker segment (immediately
         after the length field), into which the data (if any) should be
         written. */
      {
        this->marker_code = marker_code; writing=true;
        return buffer;
      }
    void end_seg(kdu_uint16 length)
      /* Set the length of the marker segment which has just been written.
         The length does not include the marker code or the length field
         itself (only the actual data in the body of the marker segment).  The
         actual length field will hold this value plus 2. */
      {
        assert(writing && (length <= (kdu_uint16)((1<<16)-3)));
        writing = false; body_length = length;
      }
    kdu_uint16 get_code()
      /* Get the marker code of an existing marker segment.  Calls to this
         function are illegal if the marker segment has been started, but
         not ended. */
      { assert(!writing); return marker_code; }
    kdu_uint16 get_length()
      /* Get the total number of bytes in the body of the marker segment
         (not including the marker code or the length field itself).  Calls
         to this function are illegal if the marker segment has been
         started, but not ended. */
      { assert(!writing); return body_length; }
    kdu_byte *get_body()
      /* Get a pointer to the body of an existing marker segment, not
         including the length field.  Calls to this function are illegal if
         the marker segment has been started, but not ended. */
      { assert(!writing); return buffer; }
  private: // Packet
    bool writing; // True when started but not yet ended.
    kdu_byte buffer[(1<<16)];
    kdu_uint16 marker_code;
    kdu_uint16 body_length; // Does not include the length field.
  };

/*****************************************************************************/
/*                             kdu_codestream_xfer                           */
/*****************************************************************************/

class kdu_codestream_xfer {
  public: // Member functions
    kdu_codestream_xfer()
      { in_file = NULL; out_file = NULL;
        in_box = NULL; out_box = NULL;
        in_tile_body = false; }
    ~kdu_codestream_xfer()
      { close(); }
    void open(char *in_fname, char *out_fname);
      /* Opens the input file and, if `out_fname' is non-NULL, the output
         file.  If the input file name ends wih the suffix, `jp2' (case
         insensitive), the file is treated as a JP2 or JP2 compatible file
         and boxes are read and copied to the output file, if any, until the
         contiguous codestream box is encountered. */
    bool read_marker(kdu_marker_seg &seg);
      /* Reads the next code-stream marker and any associated marker segment,
         storing the result in the supplied `seg' object.  If one or more
         code-stream bytes must be skipped over prior to the segment, these
         bytes are automatically copied to the output file (unless `out_fname'
         was NULL in the call to `open').  Marker segments, however, are not
         automatically copied to the output file.  To do this, `write_marker'
         must be called. This allows the caller to modify, remove, or insert
         marker segments into the output file, without modifying the rest of
         the structure.  The function returns false if there are no more
         marker segments, although reading will normally cease once the EOC
         marker is detected. */
    void write_marker(kdu_marker_seg &seg);
      /* Writes a marker segment to the output file.  This call is illegal if
         a NULL `out_fname' pointer was supplied to the `close' function. */
    void close();
      /* If the input file was a JP2 file, this function closes the contiguous
         code-stream box at the input and output file.  It then checks that
         the input file contains no further boxes, generating a warning if
         it does.  This is because, the output file's contiguous code-stream
         box will have a rubber length, meaning that it must be terminated
         by the end of the file. */
  private: // Helper functions
    bool read_byte(kdu_byte &byte)
      {
        if (in_box != NULL)
          return in_box->read(byte);
        else
          {
            int val = getc(in_file);
            if (val == EOF)
              return false;
            byte = (kdu_byte) val;
            return true;
          }
      }
    void write_byte(kdu_byte byte)
      {
        if (out_box != NULL)
          out_box->write(byte);
        else if (out_file != NULL)
          putc(byte,out_file);
      }
  private: // Data
    FILE *in_file, *out_file;
    kdu_input_box *in_box; // NULL if not a JP2 file.
    kdu_output_box *out_box; // NULL if not a JP2 file, or if no output.
    bool in_tile_body;
  };

/*****************************************************************************/
/*                                kdu_tlm_info                               */
/*****************************************************************************/

class kdu_tlm_info {
  public: // Member functions
    kdu_tlm_info()
      { head = tail = NULL; next_tnum = 0; need_tnums = false; ztlm = 0; }
    ~kdu_tlm_info()
      {
        while ((tail=head) != NULL)
          { head = tail->next;
            delete tail; }
      }
    void add_record(kdu_uint16 tnum, kdu_uint32 tpart_length)
      {
        tlm_record *rec = new tlm_record;
        rec->length = tpart_length; rec->tnum = tnum;
        rec->next = NULL;
        if (tail == NULL)
          head = tail = rec;
        else
          tail = tail->next = rec;
        if (tnum != next_tnum)
          need_tnums = true;
        next_tnum++;
      }
    bool generate_segment(kdu_marker_seg &seg);
      /* Writes a single marker segment, unless there are no records left in
         the object, in which case it returns false, writing no information
         into the supplied marker segment. */
  private: // Definitions
      struct tlm_record {
          kdu_uint32 length; // Tile-part length
          kdu_uint16 tnum; // Tile index
          tlm_record *next;
        };

  private: // Data
    tlm_record *head, *tail;
    bool need_tnums; // True if we need to write tile indices in TLM segments
    kdu_uint16 next_tnum; // Value of next tnum if `need_tnums' stays false
    kdu_byte ztlm;
  };

#endif // KDU_MAKETLM_H
