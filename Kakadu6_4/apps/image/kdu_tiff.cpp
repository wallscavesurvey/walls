/*****************************************************************************/
// File: kdu_tiff.cpp [scope = APPS/JP2]
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
/*****************************************************************************
Description:
   Implements the `kdu_tiffdir' class defined in `kdu_tiff.h"
******************************************************************************/

#include <assert.h>
#include <string.h>
#include <math.h>
#include "kdu_tiff.h"


/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                    tiff_parse (kdu_uint16)                         */
/*****************************************************************************/

inline kdu_uint16
  tiff_parse(kdu_uint16 &val, kdu_byte buf[], bool littlendian)
{
  if (littlendian)
    { val = buf[1];  val = (val<<8) + buf[0]; }
  else
    { val = buf[0];  val = (val<<8) + buf[1]; }
  return val;
}

/*****************************************************************************/
/* INLINE                     tiff_parse (kdu_uint32)                        */
/*****************************************************************************/

inline kdu_uint32
  tiff_parse(kdu_uint32 &val, kdu_byte buf[], bool littlendian)
{
  if (littlendian)
    { val = buf[3];  val = (val<<8) + buf[2];
      val = (val<<8) + buf[1];  val = (val<<8) + buf[0]; }
  else
    { val = buf[0];  val = (val<<8) + buf[1];
      val = (val<<8) + buf[2];  val = (val<<8) + buf[3]; }
  return val;
}

/*****************************************************************************/
/* INLINE                      tiff_parse (kdu_long)                         */
/*****************************************************************************/

inline kdu_long
  tiff_parse(kdu_long &val, kdu_byte buf[], bool littlendian)
{
  if (littlendian)
    { val = buf[7];  val = (val<<8) + buf[6];
      val = (val<<8) + buf[5];  val = (val<<8) + buf[4];
      val = (val<<8) + buf[3];  val = (val<<8) + buf[2];
      val = (val<<8) + buf[1];  val = (val<<8) + buf[0]; }
  else
    { val = buf[0];  val = (val<<8) + buf[1];
      val = (val<<8) + buf[2];  val = (val<<8) + buf[3];
      val = (val<<8) + buf[4];  val = (val<<8) + buf[5];
      val = (val<<8) + buf[6];  val = (val<<8) + buf[7]; }
  return val;
}

/*****************************************************************************/
/* INLINE                     tiff_put (kdu_uint16)                          */
/*****************************************************************************/

inline void
  tiff_put(kdu_uint16 val, kdu_byte buf[], bool littlendian)
{
  if (littlendian)
    { buf[0] = (kdu_byte) val;      buf[1] = (kdu_byte)(val>>8); }
  else
    { buf[1] = (kdu_byte) val;      buf[0] = (kdu_byte)(val>>8); }
}

/*****************************************************************************/
/* INLINE                     tiff_put (kdu_uint32)                          */
/*****************************************************************************/

inline void
  tiff_put(kdu_uint32 val, kdu_byte buf[], bool littlendian)
{
  if (littlendian)
    { buf[0] = (kdu_byte) val;      buf[1] = (kdu_byte)(val>>8);
      buf[2] = (kdu_byte)(val>>16); buf[3] = (kdu_byte)(val>>24); }
  else
    { buf[3] = (kdu_byte) val;      buf[2] = (kdu_byte)(val>>8);
      buf[1] = (kdu_byte)(val>>16); buf[0] = (kdu_byte)(val>>24); }
}

/*****************************************************************************/
/* INLINE                      tiff_put (kdu_long)                           */
/*****************************************************************************/

inline void
  tiff_put(kdu_long val, kdu_byte buf[], bool littlendian)
{
  if (littlendian)
    { buf[0] = (kdu_byte) val;      buf[1] = (kdu_byte)(val>>=8);
      buf[2] = (kdu_byte)(val>>=8); buf[3] = (kdu_byte)(val>>=8);
      buf[4] = (kdu_byte)(val>>=8); buf[5] = (kdu_byte)(val>>=8);
      buf[6] = (kdu_byte)(val>>=8); buf[7] = (kdu_byte)(val>>=8); }
  else
    { buf[7] = (kdu_byte) val;      buf[6] = (kdu_byte)(val>>=8);
      buf[5] = (kdu_byte)(val>>=8); buf[4] = (kdu_byte)(val>>=8);
      buf[3] = (kdu_byte)(val>>=8); buf[2] = (kdu_byte)(val>>=8);
      buf[1] = (kdu_byte)(val>>=8); buf[0] = (kdu_byte)(val>>=8); }
}


/* ========================================================================= */
/*                                kdu_tiffdif                                */
/* ========================================================================= */

/*****************************************************************************/
/*                             kdu_tiffdir::init                             */
/*****************************************************************************/

void
  kdu_tiffdir::init(bool littlendian, bool bigtiff)
{
  close();
  this->littlendian = littlendian;
  this->is_open = true;
  this->bigtiff = bigtiff;
}

/*****************************************************************************/
/*                             kdu_tiffdir::close                            */
/*****************************************************************************/

void
  kdu_tiffdir::close()
{
  is_open = false;
  littlendian = native_littlendian;
  bigtiff = false;
  kd_tifftag *tmp;
  while ((tmp=tags) != NULL)
    {
      tags = tmp->next;
      delete tmp;
    }
  src = NULL;
  src_read_ptr = 0;
}

/*****************************************************************************/
/*                           kdu_tiffdir::opendir                            */
/*****************************************************************************/

bool
  kdu_tiffdir::opendir(kdu_compressed_source *in)
{
  close();

  kdu_long global_offset = in->get_pos();

  kdu_byte head[8];
  if (in->read(head,8) < 8)
    return false;
  if ((head[0]==0x49) && (head[1]==0x49) && (head[2]==42) && (head[3]==0))
    { littlendian = true; bigtiff = false; }
  else if ((head[0]==0x4D) && (head[1]==0x4D) && (head[2]==0) && (head[3]==42))
    { littlendian = false; bigtiff = false; }
  else if ((head[0]==0x49) && (head[1]==0x49) && (head[2]==43) && (head[3]==0))
    { littlendian = true; bigtiff = true; }
  else if ((head[0]==0x4D) && (head[1]==0x4D) && (head[2]==0) && (head[3]==43))
    { littlendian = false; bigtiff = true; }
  else
    return false;
  
  kdu_uint32 offset32;
  tiff_parse(offset32,head+4,littlendian);
  kdu_long offset = offset32;
  if (bigtiff)
    {
      if (offset32 != 8)
        { kdu_error e; e << "Invalid BigTIFF file header encountered -- bytes "
          "4-7 of the header should represent 8 (the size of file offsets in "
          "BigTIFF."; }
      if (in->read(head,8) < 8)
        return false;
      tiff_parse(offset,head,littlendian);
    }
  
  if (!in->seek(global_offset + offset))
    return false;

  kdu_byte buf[20];

  kdu_long entry_count;
  if (bigtiff)
    {
      if ((in->read(buf,8)<8) || (tiff_parse(entry_count,buf,littlendian)==0))
        { kdu_error e; e << "Error reading BigTIFF directory structure."; }
    }
  else
    {
      kdu_uint16 val;
      if ((in->read(buf,2)<2) || (tiff_parse(val,buf,littlendian)==0))
        { kdu_error e; e << "Error reading TIFF directory structure."; }
      entry_count = val;
    }

  for (; entry_count > 0; entry_count--)
    {
      kdu_uint16 tag, data_type;
      kdu_long tag_length, tag_offset;
      if (bigtiff)
        {
          if ((in->read(buf,20) < 20) || (tiff_parse(tag,buf,littlendian)==0))
            { kdu_error e; e << "Illegal or incomplete entry encountered "
              "in BigTIFF directory structure."; }
          tiff_parse(data_type,buf+2,littlendian);
          tiff_parse(tag_length,buf+4,littlendian);
          tiff_parse(tag_offset,buf+12,littlendian);          
        }
      else
        {
          if ((in->read(buf,12) < 12) || (tiff_parse(tag,buf,littlendian)==0))
            { kdu_error e; e << "Illegal or incomplete entry encountered "
              "in TIFF directory structure."; }
          tiff_parse(data_type,buf+2,littlendian);
          kdu_uint32 tag_length32, tag_offset32;
          tiff_parse(tag_length32,buf+4,littlendian);
          tiff_parse(tag_offset32,buf+8,littlendian);
          tag_length = tag_length32;
          tag_offset = tag_offset32;
        }          
          
      kdu_uint32 tag_type = tag;  tag_type = (tag_type<<16) + data_type;
      int field_bytes = get_fieldlength(tag_type);
      if (field_bytes == 0)
        continue; // Discard tags with unknown type
      kd_tifftag *elt = new kd_tifftag;
      elt->tag_type = tag_type;
      elt->num_fields = tag_length;
      elt->bytes_per_field = (kdu_uint32) field_bytes;
      elt->num_bytes = elt->num_fields*elt->bytes_per_field;
      if (bigtiff)
        {
          if (elt->num_bytes <= 8)
            {
              elt->data[0]=buf[12]; elt->data[1]=buf[13];
              elt->data[2]=buf[14]; elt->data[3]=buf[15];
              elt->data[4]=buf[16]; elt->data[5]=buf[17];
              elt->data[6]=buf[18]; elt->data[7]=buf[19];
            }
          else
            elt->location = global_offset + tag_offset;          
        }
      else
        {
          if (elt->num_bytes <= 4)
            {
              elt->data[0]=buf[8];  elt->data[1]=buf[9];
              elt->data[2]=buf[10]; elt->data[3]=buf[11];
              elt->data[4]=0;       elt->data[5]=0;
              elt->data[6]=0;       elt->data[7]=0;
            }
          else
            elt->location = global_offset + tag_offset;
        }
      elt->next = tags;
      tags = elt;
    }
  
  // Don't bother reading the terminator.  Just prepare the JP2 family source.
  if (!in->seek(0))
    { close(); return false; }
  src = in;
  src_read_ptr = 0;
  is_open = true;
  return true;
}

/*****************************************************************************/
/*                         kdu_tiffdir::get_dirlength                        */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::get_dirlength()
{
  if (!is_open)
    return 0;
  kdu_long result=(bigtiff)?16:6;
  kd_tifftag *scan;
  for (scan=tags; scan != NULL; scan=scan->next)
    {
      kdu_long num_bytes = scan->num_bytes;
      if (num_bytes == 0)
        continue; // Nothing to write yet
      if (bigtiff)
        {
          result += 20;
          if (num_bytes > 8)
            result += num_bytes;
        }
      else
        {
          result += 12;
          if (num_bytes > 4)
            result += num_bytes;
        }
    }
  return result;
}

/*****************************************************************************/
/*                       kdu_tiffdir::get_fieldlength                        */
/*****************************************************************************/

int
  kdu_tiffdir::get_fieldlength(kdu_uint32 tag_type)
{
  kdu_uint32 data_type = tag_type & 0x0000FFFF;
  switch (data_type) {
    case 1: return 1; // Byte
    case 2: return 1; // Ascii
    case 3: return 2; // Short
    case 4: return 4; // Long
    case 5: return 8; // Rational
    case 6: return 1; // SChar
    case 7: return 1; // Undefined
    case 8: return 2; // SShort
    case 9: return 4; // SLong
    case 10: return 8; // SRational
    case 11: return 4; // Float
    case 12: return 8; // Double
    case 16: return 8; // 64-bit unsigned
    case 17: return 8; // 64-bit signed
    default: break;
  }
  return 0; // Unrecognized tag data type.
}

/*****************************************************************************/
/*                         kdu_tiffdir::write_header                         */
/*****************************************************************************/

int
  kdu_tiffdir::write_header(kdu_compressed_target *tgt, kdu_long dir_offset)
{
  if (!is_open)
    return 0;
  kdu_byte head[16];

  if (bigtiff)
    {
      if (littlendian)
        { head[0]=head[1]=0x49; head[2]=43; head[3]=0; }
      else
        { head[0]=head[1]=0x4D; head[2]=0; head[3]=43; }
      kdu_uint32 pointer_size = 8;
      tiff_put(pointer_size,head+4,littlendian);
      tiff_put(dir_offset,head+8,littlendian);
      if (!tgt->write(head,16))
        return 0;
      return 16;
    }
  else
    {
      if (littlendian)
        { head[0]=head[1]=0x49; head[2]=42; head[3]=0; }
      else
        { head[0]=head[1]=0x4D; head[2]=0; head[3]=42; }
      kdu_uint32 offset = (kdu_uint32) dir_offset;
      tiff_put(offset,head+4,littlendian);
      if (!tgt->write(head,8))
        return 0;
      return 8;
    }
}

/*****************************************************************************/
/*                           kdu_tiffdir::writedir                           */
/*****************************************************************************/

bool
  kdu_tiffdir::writedir(kdu_compressed_target *tgt, kdu_long dir_offset)
{
  if (!is_open)
    return false;
  kd_tifftag *scan;
  kdu_byte buf[20];

  // Sort the tags -- bubble sort will do
  bool done=false;
  while ((tags != NULL) && !done)
    {
      done = true;
      kd_tifftag *next, *prev=NULL;
      for (scan=tags; scan->next != NULL; prev=scan, scan=next)
        {
          next = scan->next;
          if (next->tag_type >= scan->tag_type)
            continue; // No need to swap
          done = false;
          if (prev == NULL)
            tags = next;
          else
            prev->next = next;
          scan->next = next->next;
          next->next = scan;
          break;
        }
    }

  // Find number of entries and the expected length of the directory itself
  kdu_long entry_count=0;
  for (scan=tags; scan != NULL; scan=scan->next)
    if (scan->num_fields > 0)
      entry_count++;
  
  // Write number of entries
  kdu_long expected_dirlength=0;
  kdu_long out_length = 0;
  if (bigtiff)
    {
      expected_dirlength = 16 + entry_count*20;
      tiff_put(entry_count,buf,littlendian);
      tgt->write(buf,8);
      out_length = 8;
    }
  else
    {
      expected_dirlength = 6 + entry_count*12;
      tiff_put((kdu_uint16) entry_count,buf,littlendian);
      tgt->write(buf,2);
      out_length = 2;
    }

  // Write the tag entries
  kdu_long extra_offset=dir_offset+expected_dirlength;
  for (scan=tags; scan != NULL; scan=scan->next)
    {
      if (scan->num_fields == 0)
        continue;
      if (scan->num_bytes != (scan->num_fields*scan->bytes_per_field))
        { kdu_error e; e << "Attempting to write a TIFF directory containing "
          "tags whose contents are not aligned a field boundary.  It looks "
          "like you have used the unstructured `kdu_tiffdir::write_tag' "
          "function to write data whose length is not a multiple of the "
          "field size -- " << scan->bytes_per_field << " in this case.  "
          "Problem encountered at tag 0x"; e.set_hex_mode(true);
          e << (scan->tag_type>>16) << "."; }
      tiff_put((kdu_uint16)(scan->tag_type>>16),buf,littlendian);
      tiff_put((kdu_uint16) scan->tag_type,buf+2,littlendian);
      if (bigtiff)
        {
          tiff_put(scan->num_fields,buf+4,littlendian);      
          if (scan->num_bytes > 8)
            { // Generate pointer to the actual data
              tiff_put(extra_offset,buf+12,littlendian);
              extra_offset += scan->num_bytes;
            }
          else if (scan->out_buf != NULL)
            { // Copy bytes from `out_buf'
              buf[12]  = scan->out_buf[0]; buf[13]  = scan->out_buf[1];
              buf[14] = scan->out_buf[2];  buf[15] = scan->out_buf[3];
              buf[16] = scan->out_buf[4];  buf[17] = scan->out_buf[5];
              buf[18] = scan->out_buf[6];  buf[19] = scan->out_buf[7];
            }
          else
            { // Copy bytes from `data'
              buf[12]  = scan->data[0]; buf[13]  = scan->data[1];
              buf[14] = scan->data[2];  buf[15] = scan->data[3];
              buf[16] = scan->data[4];  buf[17] = scan->data[5];
              buf[18] = scan->data[6];  buf[19] = scan->data[7];
            }
          tgt->write(buf,20);
          out_length += 20;
        }
      else
        {
          if ((scan->num_fields != (kdu_long)((kdu_uint32)scan->num_fields)) ||
              (extra_offset != ((kdu_long)((kdu_uint32)extra_offset))))
            { kdu_error e; e << "Cannot store image in a regular TIFF file.  "
              "Need BigTIFF to accommodate large file offsets or tag "
              "contents.  Kakadu supports BigTIFF also."; }
          tiff_put((kdu_uint32)(scan->num_fields),buf+4,littlendian);      
          if (scan->num_bytes > 4)
            { // Generate pointer to the actual data
              tiff_put((kdu_uint32) extra_offset,buf+8,littlendian);
              extra_offset += scan->num_bytes;
            }
          else if (scan->out_buf != NULL)
            { // Copy bytes from `out_buf'
              buf[8]  = scan->out_buf[0];  buf[9]  = scan->out_buf[1];
              buf[10] = scan->out_buf[2];  buf[11] = scan->out_buf[3];
            }
          else
            { // Copy bytes from `data'
              buf[8]  = scan->data[0];  buf[9]  = scan->data[1];
              buf[10] = scan->data[2];  buf[11] = scan->data[3];
            }
          tgt->write(buf,12);
          out_length += 12;
        }
    }

  // Write the directory terminator
  if (bigtiff)
    {
      kdu_long terminator=0;
      tiff_put(terminator,buf,littlendian);
      if (!tgt->write(buf,8))
        return false;
      out_length += 8;
    }
  else
    {
      kdu_uint32 terminator=0;
      tiff_put(terminator,buf,littlendian);
      if (!tgt->write(buf,4))
        return false;
      out_length += 4;
    }

  // Write any tag bodies which could not fit inside the directory itself
  assert(out_length == expected_dirlength);
  kdu_long length_threshold = (bigtiff)?8:4;
  for (scan=tags; scan != NULL; scan=scan->next)
    if (scan->num_bytes > length_threshold)
      { // Need to write extra data
        bool success=true;
        if (scan->out_buf != NULL)
          success = tgt->write(scan->out_buf,(int)scan->num_bytes);
        else
          { // We must read contents first
            assert(src != NULL);
            kdu_byte *buf = new kdu_byte[1<<16];
            kdu_long xfer_bytes, bytes_left=scan->num_bytes;
            kdu_long pos = scan->location;
            for (; bytes_left > 0; bytes_left-=xfer_bytes, pos+=xfer_bytes)
              {
                xfer_bytes = (bytes_left>(1<<16))?(1<<16):bytes_left;
                read_bytes(buf,xfer_bytes,pos,scan->tag_type);
                success = tgt->write(buf,(int) xfer_bytes);
              }
            delete[] buf;
          }
        out_length += scan->num_bytes;
        if (!success)
          return false;
      }
  assert((dir_offset+out_length) == extra_offset);
  return true;
}

/*****************************************************************************/
/*                            kdu_tiffdir::find_tag                          */
/*****************************************************************************/

kd_tifftag *
  kdu_tiffdir::find_tag(kdu_uint32 tag_type)
{
  kd_tifftag *scan;
  for (scan=tags; scan != NULL; scan=scan->next)
    if (scan->tag_type == tag_type)
      return scan;
  return NULL;
}

/*****************************************************************************/
/*                         kdu_tiffdir::get_taglength                        */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::get_taglength(kdu_uint32 tag_type)
{
  kd_tifftag *elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  return elt->num_fields;
}

/*****************************************************************************/
/*                          kdu_tiffdir::delete_tag                          */
/*****************************************************************************/

bool
  kdu_tiffdir::delete_tag(kdu_uint32 tag_type)
{
  kd_tifftag *scan, *prev=NULL;
  for (scan=tags; scan != NULL; prev=scan, scan=scan->next)
    if (scan->tag_type == tag_type)
      {
        if (prev == NULL)
          tags = scan->next;
        else
          prev->next = scan->next;
        delete scan;
        return true;
      }
  return false;
}

/*****************************************************************************/
/*                         kdu_tiffdir::read_bytes                           */
/*****************************************************************************/

void
  kdu_tiffdir::read_bytes(kdu_byte *buf, kdu_long num_bytes,
                          kdu_long pos, kdu_uint32 tag_type)
{
  assert((src != NULL) && (pos >= 0) && (num_bytes >= 0));
  if (num_bytes == 0)
    return;
  if (src_read_ptr != pos)
    {
      src->seek(pos);
      src_read_ptr = pos;
    }
  if (((int)src->read(buf,(int)num_bytes)) < num_bytes)
    { kdu_error e; e << "Unable to fully read the contents of TIFF tag 0x";
      e.set_hex_mode(true);  e << (tag_type>>16) << "."; }
  src_read_ptr += num_bytes;
}

/*****************************************************************************/
/*                           kdu_tiffdir::open_tag                           */
/*****************************************************************************/

kdu_uint32
  kdu_tiffdir::open_tag(kdu_uint32 tag_type)
{
  kd_tifftag *elt=NULL;
  if ((tag_type & 0x0000FFFF) == 0)
    { // Scan for any tag whose 16-bit tag code matches
      kd_tifftag *scan;
      for (scan=tags; scan != NULL; scan=scan->next)
        if ((scan->tag_type & 0xFFFF0000) == tag_type)
          { elt=scan; break; }
    }
  else
    elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  elt->read_ptr = 0;
  return elt->tag_type;
}

/*****************************************************************************/
/*                     kdu_tiffdir::read_tag (kdu_byte)                      */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::read_tag(kdu_uint32 tag_type, kdu_long length, kdu_byte data[])
{
  assert(length >= 0);
  kd_tifftag *elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  kdu_long remaining_bytes = elt->num_bytes-elt->read_ptr;
  if (remaining_bytes < length)
    length = remaining_bytes;
  if (length == 0)
    return 0;
  if (elt->out_buf != NULL)
    memcpy(data,elt->out_buf+elt->read_ptr,(size_t) length);
  else if (elt->num_bytes <= ((bigtiff)?8:4))
    memcpy(data,elt->data+elt->read_ptr,(size_t) length);
  else
    read_bytes(data,length,elt->location+elt->read_ptr,elt->tag_type);
  elt->read_ptr += length;
  return length;
}

/*****************************************************************************/
/*                    kdu_tiffdir::read_tag (kdu_uint16)                     */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::read_tag(kdu_uint32 tag_type, kdu_long length,
                        kdu_uint16 data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type != 3) && (data_type != 8))
    { kdu_error e; e << "Attempting to use 16-bit `kdu_tiffdir::read_tag' "
      "function to read data which does not represent 16-bit signed or "
      "unsigned words.  Problem occurred while trying to read from tag 0x";
      e.set_hex_mode(true); e << (tag_type>>16) << "."; }

  kd_tifftag *elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  if ((elt->read_ptr & 1) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::read_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `read_tag' functions with one "
      "of the structured value-wise `read_tag' functions."; }
  kdu_long remaining_fields = (elt->num_bytes-elt->read_ptr)>>1;
  if (remaining_fields < length)
    length = remaining_fields;
  if (length == 0)
    return 0;
  kdu_long len_bytes = (length<<1);
  if (elt->out_buf != NULL)
    memcpy(data,elt->out_buf+elt->read_ptr,(size_t) len_bytes);
  else if (elt->num_bytes <= ((bigtiff)?8:4))
    memcpy(data,elt->data+(int)(elt->read_ptr),(size_t) len_bytes);
  else
    read_bytes((kdu_byte *) data,len_bytes,
               elt->location+elt->read_ptr,elt->tag_type);
  elt->read_ptr += len_bytes;
  if (native_littlendian != littlendian)
    {
      kdu_uint16 val, *dp=data;
      for (kdu_long n=length; n > 0; n--, dp++)
        {
          val = *dp;
          val = (val>>8) + (val<<8);
          *dp = val;
        }
    }
  return length;
}

/*****************************************************************************/
/*                    kdu_tiffdir::read_tag (kdu_uint32)                     */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::read_tag(kdu_uint32 tag_type, kdu_long length,
                        kdu_uint32 data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type == 3) || (data_type == 8))
    { // Read as 16-bit data and then upconvert
      kdu_uint16 *dest = (kdu_uint16 *)data;
#ifdef KDU_POINTERS64
      dest += length;
#else
      dest += (kdu_uint32) length;
#endif
      length = read_tag(tag_type,length,dest);
      if (data_type == 3)
        { // Unsigned data
          kdu_uint16 *sp=dest;
          kdu_uint32 *dp=data;
          for (kdu_long n=length; n > 0; n--)
            *(dp++) = *(sp++);
        }
      else
        { // Signed data
          kdu_int16 *sp=(kdu_int16 *) dest;
          kdu_int32 *dp=(kdu_int32 *) data;
          for (kdu_long n=length; n > 0; n--)
            *(dp++) = *(sp++);
        }
      return length;
    }

  if ((data_type != 4) && (data_type != 9))
    { kdu_error e; e << "Attempting to use 32-bit `kdu_tiffdir::read_tag' "
      "function to read data which does not represent 16 or 32-bit signed or "
      "unsigned words.  Problem occurred while trying to read from tag 0x";
      e.set_hex_mode(true); e << (tag_type>>16) << "."; }
  kd_tifftag *elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  if ((elt->read_ptr & 3) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::read_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `read_tag' functions with one "
      "of the structured value-wise `read_tag' functions."; }
  kdu_long remaining_fields = (elt->num_bytes-elt->read_ptr)>>2;
  if (remaining_fields < length)
    length = remaining_fields;
  if (length == 0)
    return 0;
  kdu_long len_bytes = (length<<2);
  if (elt->out_buf != NULL)
    memcpy(data,elt->out_buf+elt->read_ptr,(size_t) len_bytes);
  else if (elt->num_bytes <= ((bigtiff)?8:4))
    memcpy(data,elt->data+(int)(elt->read_ptr),(size_t) len_bytes);
  else
    read_bytes((kdu_byte *) data,len_bytes,
               elt->location+elt->read_ptr,elt->tag_type);
  elt->read_ptr += len_bytes;
  if (native_littlendian != littlendian)
    {
      kdu_long n;
      kdu_uint32 val, *dp=data;
      for (n=length; n > 0; n--, dp++)
        {
          val = *dp;
          val = (val>>24) + ((val>>8) & 0x0000FF00)
              + ((val<< 8) & 0x00FF0000) + (val<<24);
          *dp = val;
        }
    }
  return length;
}

/*****************************************************************************/
/*                      kdu_tiffdir::read_tag (double)                       */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::read_tag(kdu_uint32 tag_type, kdu_long length, double data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type != 5) && (data_type != 10) &&
      (data_type != 11) && (data_type != 12))
    { kdu_error e; e << "Attempting to use floating point "
      "`kdu_tiffdir::read_tag' function to read data which does not "
      "represent a signed/unsigned fraction, or a single or double precision "
      "floating point quantity.  Problem occurred while trying to read from "
      "tag 0x"; e.set_hex_mode(true); e << (tag_type>>16) << "."; }

  kd_tifftag *elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  if ((elt->read_ptr % elt->bytes_per_field) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::read_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `read_tag' functions with one "
      "of the structured value-wise `read_tag' functions."; }
  kdu_long remaining_fields =
    (elt->num_bytes-elt->read_ptr) / elt->bytes_per_field;
  if (remaining_fields < length)
    length = remaining_fields;
  if (length == 0)
    return 0;
  kdu_long len_bytes = (length * elt->bytes_per_field);
  kdu_uint32 *dest = (kdu_uint32 *)data;
  if (elt->bytes_per_field == 4)
    { // Move `dest' to upper half of the buffer for in-place resizing later
#ifdef KDU_POINTERS64
      dest += length;
#else
      dest += (kdu_uint32) length;
#endif
    }
  else
    assert(elt->bytes_per_field == 8);

  if (elt->out_buf != NULL)
    memcpy(dest,elt->out_buf+elt->read_ptr,(size_t) len_bytes);
  else if (elt->num_bytes <= ((bigtiff)?8:4))
    memcpy(dest,elt->data+(int)(elt->read_ptr),(size_t) len_bytes);
  else
    read_bytes((kdu_byte *) dest,len_bytes,
               elt->location+elt->read_ptr,elt->tag_type);
  elt->read_ptr += len_bytes;

  if (native_littlendian != littlendian)
    {
      // Start by flipping bytes inside each 32-bit word
      assert(elt->bytes_per_field >= 4);
      kdu_long n, num_dwords=(len_bytes>>2);
      kdu_uint32 val, *wp;
      for (wp=dest, n=num_dwords; n > 0; n--, wp++)
        {
          val = *wp;
          val = (val>>24) + ((val>>8) & 0x0000FF00)
              + ((val<< 8) & 0x00FF0000) + (val<<24);
          *wp = val;
        }

      // Finally flip pairs of 32-bit words if the data type is doubles
      if (data_type == 12)
        for (wp=dest, n=num_dwords; n > 0; n-=2, wp+=2)
          { val = wp[0];  wp[0] = wp[1];  wp[1] = val; }
    }

  // Finally, we come to the conversions
  if (data_type == 12)
    return length; // Double precision floats already
  else if (data_type == 11)
    { // Upconvert from single precision floats
      float *sp=((float *) dest);
      double *dp=data;
      for (kdu_long n=length; n > 0; n--)
        *(dp++) = *(sp++);
    }
  else if (data_type == 5)
    { // Convert from unsigned rational
      kdu_uint32 *sp=dest;
      double *dp=data;
      for (kdu_long n=length; n > 0; n--, sp+=2, dp++)
        *dp = (sp[1]==0)?0.0:(((double) sp[0]) / ((double) sp[1]));
    }
  else if (data_type == 10)
    { // Convert from signed rational
      kdu_int32 *sp=(kdu_int32 *) dest;
      double *dp=data;
      for (kdu_long n=length; n > 0; n--, sp+=2, dp++)
        *dp = (sp[1]==0)?0.0:(((double) sp[0]) / ((double) sp[1]));
    }
  else
    assert(0);

  return length;
}

/*****************************************************************************/
/*                     kdu_tiffdir::read_tag (kdu_long)                      */
/*****************************************************************************/

kdu_long
  kdu_tiffdir::read_tag(kdu_uint32 tag_type, kdu_long length,
                        kdu_long data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type == 3) || (data_type == 8))
    { // Read as 16-bit data and then upconvert
      kdu_uint16 *dest = (kdu_uint16 *)data;
#ifdef KDU_POINTERS64
      dest += length*3;
#else
      dest += (kdu_uint32)(length*3);
#endif
      length = read_tag(tag_type,length,dest);
      kdu_long *dp=data;
      if (data_type == 3)
        { // Unsigned data
          kdu_uint16 *sp=dest;
          for (kdu_long n=length; n > 0; n--)
            *(dp++) = *(sp++);
        }
      else
        { // Signed data
          kdu_int16 *sp=(kdu_int16 *) dest;
          for (kdu_long n=length; n > 0; n--)
            *(dp++) = *(sp++);
        }
      return length;
    }
  
  if ((data_type == 4) || (data_type == 9))
    { // Read as 32-bit data and then upconvert
      kdu_uint32 *dest = (kdu_uint32 *)data;
#ifdef KDU_POINTERS64
      dest += length;
#else
      dest += (kdu_uint32) length;
#endif
      length = read_tag(tag_type,length,dest);
      kdu_long *dp=data;
      if (data_type == 4)
        { // Unsigned data
          kdu_uint32 *sp=dest;
          for (kdu_long n=length; n > 0; n--)
            *(dp++) = *(sp++);
        }
      else
        { // Signed data
          kdu_int32 *sp=(kdu_int32 *) dest;
          for (kdu_long n=length; n > 0; n--)
            *(dp++) = *(sp++);
        }
      return length;
    } 
  
  if ((data_type != 16) && (data_type != 17))
    { kdu_error e; e << "Attempting to use 64-bit `kdu_tiffdir::read_tag' "
      "function to read data which does not represent 16-, 32- or 64-bit "
      "signed or unsigned words.  Problem occurred while trying to read "
      "from tag 0x";
      e.set_hex_mode(true); e << (tag_type>>16) << ".";
    }
  kd_tifftag *elt = find_tag(tag_type);
  if (elt == NULL)
    return 0;
  if ((elt->read_ptr & 7) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::read_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `read_tag' functions with one "
      "of the structured value-wise `read_tag' functions.";
    }
  kdu_long remaining_fields = (elt->num_bytes-elt->read_ptr)>>3;
  if (remaining_fields < length)
    length = remaining_fields;
  if (length == 0)
    return 0;
  kdu_long len_bytes = (length<<3);
  if (elt->out_buf != NULL)
    memcpy(data,elt->out_buf+elt->read_ptr,(size_t) len_bytes);
  else if (elt->num_bytes <= ((bigtiff)?8:4))
    memcpy(data,elt->data+(int)(elt->read_ptr),(size_t) len_bytes);
  else
    read_bytes((kdu_byte *) data,len_bytes,
               elt->location+elt->read_ptr,elt->tag_type);
  elt->read_ptr += len_bytes;
  if (native_littlendian != littlendian)
    {
      // Start by flipping bytes inside each 32-bit word
      kdu_long n, num_dwords=(len_bytes>>2);
      kdu_uint32 val, *wp;
      for (wp=(kdu_uint32 *)data, n=num_dwords; n > 0; n--, wp++)
        {
          val = *wp;
          val = (val>>24) + ((val>>8) & 0x0000FF00)
          + ((val<< 8) & 0x00FF0000) + (val<<24);
          *wp = val;
        }
      
      // Finally flip pairs of 32-bit words
      for (wp=(kdu_uint32 *)data, n=num_dwords; n > 0; n-=2, wp+=2)
        { val = wp[0];  wp[0] = wp[1];  wp[1] = val; }
    }
  return length;
}

/*****************************************************************************/
/*                          kdu_tiffdir::create_tag                          */
/*****************************************************************************/

void
  kdu_tiffdir::create_tag(kdu_uint32 tag_type)
{
  kd_tifftag *elt;

  // Start by checking for illegal codes
  if ((get_fieldlength(tag_type) == 0) || ((tag_type>>16) == 0))
    { kdu_error e; e << "Illegal TIFF tag-type supplied to "
      "`kdu_tiffdir::create_tag'."; }
  if (!bigtiff)
    {
      kdu_uint16 data_type = (kdu_uint16)(tag_type);
      if ((data_type == 16) || (data_type == 17))
        { kdu_error e; e << "Tag data-type supplied to "
          "`kdu_tiffdir::create_tag' can only be used with the BigTIFF "
          "file format, but you are writing a regular TIFF file."; }
    }
  for (elt=tags; elt != NULL; elt=elt->next)
    if ((((elt->tag_type ^ tag_type) & 0xFFFF0000) == 0) &&
        (((elt->tag_type ^ tag_type) & 0x0000FFFF) != 0))
      { kdu_error e; e << "Tag-type supplied to `kdu_tiffdir::create_tag' "
        "already exists, with a different data type."; }
  if ((elt = find_tag(tag_type)) == NULL)
    {
      elt = new kd_tifftag;
      elt->next = tags;
      tags = elt;
      elt->tag_type = tag_type;
      elt->bytes_per_field = get_fieldlength(tag_type);
    }
  elt->read_ptr = 0;
  elt->num_bytes = elt->num_fields = 0;
  elt->location = 0;
}

/*****************************************************************************/
/*                     kdu_tiffdir::write_tag (kdu_byte)                     */
/*****************************************************************************/

void
  kdu_tiffdir::write_tag(kdu_uint32 tag_type, int length, kdu_byte data[])
{
  assert(length >= 0);
  kd_tifftag *elt = find_tag(tag_type);
  if ((elt == NULL) || ((elt->out_buf == NULL) && (elt->num_bytes > 0)))
    { // Either it does not exist, or it is an input tag
      create_tag(tag_type);
      elt = find_tag(tag_type);
    }
  assert(elt != NULL);
  kdu_long new_total_bytes = elt->num_bytes + length;
  if (new_total_bytes > elt->max_bytes)
    { // Allocate or reallocate buffer
#ifdef KDU_POINTERS64
      elt->max_bytes += 4 + new_total_bytes; // Always at least 4 bytes long      
      kdu_byte *new_buf = new kdu_byte[elt->max_bytes];
#else
      int maxb = (int)(4+new_total_bytes);
      elt->max_bytes = maxb;
      if (elt->max_bytes != (4+new_total_bytes))
        { kdu_error e; e << "Allocating way too much memory in "
          "`kdu_tiffdir::write_tag'!!"; }
      kdu_byte *new_buf = new kdu_byte[maxb];
#endif
      if (elt->out_buf != NULL)
        {
          memcpy(new_buf,elt->out_buf,(size_t)(elt->num_bytes));
          delete[] elt->out_buf; elt->out_buf = NULL;
        }
      elt->out_buf = new_buf;
    }
#ifdef KDU_POINTERS64
  kdu_byte *dest = elt->out_buf + elt->num_bytes;
#else
  kdu_byte *dest = elt->out_buf + ((kdu_uint32) elt->num_bytes);
#endif
  memcpy(dest,data,(size_t) length);
  elt->num_bytes = new_total_bytes;
  elt->num_fields = new_total_bytes / elt->bytes_per_field;
}

/*****************************************************************************/
/*                    kdu_tiffdir::write_tag (kdu_uint16)                    */
/*****************************************************************************/

void
  kdu_tiffdir::write_tag(kdu_uint32 tag_type, int length, kdu_uint16 data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type != 3) && (data_type != 8))
    { kdu_error e; e << "Attempting to use 16-bit `kdu_tiffdir::write_tag' "
      "function to write a TIFF tag which does not represent 16-bit signed or "
      "unsigned words.  Problem occurred while trying to write to tag 0x";
      e.set_hex_mode(true); e << (tag_type>>16) << "."; }
  kd_tifftag *elt = find_tag(tag_type);
  if ((elt == NULL) || ((elt->out_buf == NULL) && (elt->num_bytes > 0)))
    { // Either it does not exist, or it is an input tag
      create_tag(tag_type);
      elt = find_tag(tag_type);
    }
  assert(elt != NULL);
  if ((elt->num_bytes & 1) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::write_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `write_tag' functions with one "
      "of the structured value-wise `write_tag' functions."; }
  kdu_long new_total_bytes = elt->num_bytes + (length<<1);
  if (new_total_bytes > elt->max_bytes)
    { // Allocate or reallocate buffer
#ifdef KDU_POINTERS64
      elt->max_bytes += 4 + new_total_bytes; // Always at least 4 bytes long      
      kdu_byte *new_buf = new kdu_byte[elt->max_bytes];
#else
      int maxb = (int)(4+new_total_bytes);
      elt->max_bytes = maxb;
      if (elt->max_bytes != (4+new_total_bytes))
        { kdu_error e; e << "Allocating way too much memory in "
        "`kdu_tiffdir::write_tag'!!"; }
      kdu_byte *new_buf = new kdu_byte[maxb];
#endif
      if (elt->out_buf != NULL)
        {
          memcpy(new_buf,elt->out_buf,(size_t)(elt->num_bytes));
          delete[] elt->out_buf; elt->out_buf = NULL;
        }
      elt->out_buf = new_buf;
    }
#ifdef KDU_POINTERS64
  kdu_byte *dest = elt->out_buf + elt->num_bytes;
#else
  kdu_byte *dest = elt->out_buf + ((kdu_uint32) elt->num_bytes);
#endif
  memcpy(dest,data,(size_t)(new_total_bytes-elt->num_bytes));
  if (native_littlendian != littlendian)
    { // Flip bytes in each word
      kdu_uint16 val, *dp = (kdu_uint16 *) dest;
      for (int n=0; n < length; n++)
        {
          val = dp[n];
          val = (val>>8) + (val<<8);
          dp[n] = val;
        }
    }
  elt->num_bytes = new_total_bytes;
  elt->num_fields = new_total_bytes / elt->bytes_per_field;
}

/*****************************************************************************/
/*                    kdu_tiffdir::write_tag (kdu_uint32)                    */
/*****************************************************************************/

void
  kdu_tiffdir::write_tag(kdu_uint32 tag_type, int length, kdu_uint32 data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type != 4) && (data_type != 9))
    { kdu_error e; e << "Attempting to use 32-bit `kdu_tiffdir::write_tag' "
      "function to write a tag which does not represent 32-bit signed or "
      "unsigned words.  Problem occurred while trying to write to tag 0x";
      e.set_hex_mode(true); e << (tag_type>>16) << "."; }
  kd_tifftag *elt = find_tag(tag_type);
  if ((elt == NULL) || ((elt->out_buf == NULL) && (elt->num_bytes > 0)))
    { // Either it does not exist, or it is an input tag
      create_tag(tag_type);
      elt = find_tag(tag_type);
    }
  assert(elt != NULL);
  if ((elt->num_bytes & 3) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::write_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `write_tag' functions with one "
      "of the structured value-wise `write_tag' functions."; }
  kdu_long new_total_bytes = elt->num_bytes + (length<<2);
  if (new_total_bytes > elt->max_bytes)
    { // Allocate or reallocate buffer
#ifdef KDU_POINTERS64
      elt->max_bytes += 4 + new_total_bytes; // Always at least 4 bytes long      
      kdu_byte *new_buf = new kdu_byte[elt->max_bytes];
#else
      int maxb = (int)(4+new_total_bytes);
      elt->max_bytes = maxb;
      if (elt->max_bytes != (4+new_total_bytes))
        { kdu_error e; e << "Allocating way too much memory in "
        "`kdu_tiffdir::write_tag'!!"; }
      kdu_byte *new_buf = new kdu_byte[maxb];
#endif
      if (elt->out_buf != NULL)
        {
          memcpy(new_buf,elt->out_buf,(size_t)(elt->num_bytes));
          delete[] elt->out_buf;
        }
      elt->out_buf = new_buf;
    }
#ifdef KDU_POINTERS64
  kdu_byte *dest = elt->out_buf + elt->num_bytes;
#else
  kdu_byte *dest = elt->out_buf + ((kdu_uint32) elt->num_bytes);
#endif
  memcpy(dest,data,(size_t)(new_total_bytes-elt->num_bytes));
  if (native_littlendian != littlendian)
    { // Flip bytes in each word
      kdu_uint32 val, *dp = (kdu_uint32 *) dest;
      for (int n=0; n < length; n++)
        {
          val = dp[n];
          val = (val>>24) + ((val>>8) & 0x0000FF00)
              + ((val<< 8) & 0x00FF0000) + (val<<24);
          dp[n] = val;
        }
    }
  elt->num_bytes = new_total_bytes;
  elt->num_fields = new_total_bytes / elt->bytes_per_field;
}

/*****************************************************************************/
/*                      kdu_tiffdir::write_tag (double)                      */
/*****************************************************************************/

void
  kdu_tiffdir::write_tag(kdu_uint32 tag_type, int length, double data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type != 5) && (data_type != 10) &&
      (data_type != 11) && (data_type != 12))
    { kdu_error e; e << "Attempting to use double precision "
      "`kdu_tiffdir::write_tag' function to write a tag which does not "
      "represent signed/unsigned fractions or single/double precision "
      "floating point values.  Problem occurred while trying to write to "
      "tag 0x"; e.set_hex_mode(true); e << (tag_type>>16) << "."; }
  kd_tifftag *elt = find_tag(tag_type);
  if ((elt == NULL) || ((elt->out_buf == NULL) && (elt->num_bytes > 0)))
    { // Either it does not exist, or it is an input tag
      create_tag(tag_type);
      elt = find_tag(tag_type);
    }
  assert(elt != NULL);
  if (elt->num_bytes != (elt->num_fields*elt->bytes_per_field))
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::write_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `write_tag' functions with one "
      "of the structured value-wise `write_tag' functions."; }
  kdu_long new_total_bytes = elt->num_bytes + length*elt->bytes_per_field;
  if (new_total_bytes > elt->max_bytes)
    { // Allocate or reallocate buffer
#ifdef KDU_POINTERS64
      elt->max_bytes += 4 + new_total_bytes; // Always at least 4 bytes long      
      kdu_byte *new_buf = new kdu_byte[elt->max_bytes];
#else
      int maxb = (int)(4+new_total_bytes);
      elt->max_bytes = maxb;
      if (elt->max_bytes != (4+new_total_bytes))
        { kdu_error e; e << "Allocating way too much memory in "
        "`kdu_tiffdir::write_tag'!!"; }
      kdu_byte *new_buf = new kdu_byte[maxb];
#endif
      if (elt->out_buf != NULL)
        {
          memcpy(new_buf,elt->out_buf,(size_t)(elt->num_bytes));
          delete[] elt->out_buf;
        }
      elt->out_buf = new_buf;
    }

  // Copy data with any required conversions
#ifdef KDU_POINTERS64
  kdu_byte *dest = elt->out_buf + elt->num_bytes;
#else
  kdu_byte *dest = elt->out_buf + ((kdu_uint32) elt->num_bytes);
#endif
  int n;
  if (data_type == 12)
    { // Double precision
      double *dp = (double *) dest;
      for (n=0; n < length; n++)
        dp[n] = data[n];
    }
  else if (data_type == 11)
    { // Upconvert to single precision floats
      float *dp=(float *) dest;
      for (n=0; n < length; n++)
        dp[n] = (float) data[n];
    }
  else if (data_type == 5)
    { // Convert to unsigned rational
      kdu_uint32 *dp=(kdu_uint32 *) dest;
      for (n=0; n < length; n++, dp+=2)
        {
          double val = data[n];
          if (val <= 0.0)
            { dp[0]=0; dp[1]=1; }
          else
            {
              kdu_uint32 num, den=1;
              while (((num=(kdu_uint32)(0.5+val*den)) < 0x3FFFFFFF) &&
                     (val*den != (double) num) && (den < 0x80000000))
                den <<= 1;
              dp[0] = num;  dp[1] = den;
            }
        }
    }
  else if (data_type == 10)
    { // Convert to signed rational
      kdu_int32 *dp=(kdu_int32 *) dest;
      for (n=0; n < length; n++, dp+=2)
        {
          double val = data[n];
          if (val < 0.0)
            {
              val = -val;
              kdu_int32 num, den=1;
              while (((num=(kdu_int32)(0.5+val*den)) < 0x3FFFFFFF) &&
                     (val*den != (double) num) && (den < 0x40000000))
                den <<= 1;
              dp[0] = -num;  dp[1] = den;
            }
          else
            {
              kdu_int32 num, den=1;
              while (((num=(kdu_int32)(0.5+val*den)) < 0x3FFFFFFF) &&
                     (val*den != (double) num) && (den < 0x40000000))
                den <<= 1;
              dp[0] = num;  dp[1] = den;
            }
        }
    }
  else
    assert(0);

  if (littlendian != native_littlendian)
    {
      // Start by flipping bytes inside each 32-bit word
      assert(elt->bytes_per_field >= 4);
      int num_dwords = (int)((new_total_bytes-elt->num_bytes)>>2);
      kdu_uint32 val, *wp = (kdu_uint32 *) dest;
      for (n=0; n < num_dwords; n++)
        {
          val = wp[n];
          val = (val>>24) + ((val>>8) & 0x0000FF00)
              + ((val<< 8) & 0x00FF0000) + (val<<24);
          wp[n] = val;
        }

      // Finally flip pairs of 32-bit words if the data type is doubles
      if (data_type == 12)
        for (n=0; n < num_dwords; n+=2)
          { val = wp[n];  wp[n] = wp[n+1];  wp[n+1] = val; }
    }

  elt->num_bytes = new_total_bytes;
  elt->num_fields = new_total_bytes / elt->bytes_per_field;
}

/*****************************************************************************/
/*                     kdu_tiffdir::write_tag (kdu_long)                     */
/*****************************************************************************/

void
  kdu_tiffdir::write_tag(kdu_uint32 tag_type, int length, kdu_long data[])
{
  assert(length >= 0);
  kdu_uint16 data_type = (kdu_uint16)(tag_type);
  if ((data_type != 3) && (data_type != 8) &&
      (data_type != 4) && (data_type != 9) &&
      (data_type != 16) && (data_type != 17))
    { kdu_error e; e << "Attempting to use 64-bit `kdu_tiffdir::write_tag' "
      "function to write a tag which does not represent 16-bit, 32-bit or "
      "64-bit signed or unsigned words.  Problem occurred while trying "
      "to write to tag 0x";
      e.set_hex_mode(true); e << (tag_type>>16) << ".";
    }
  kd_tifftag *elt = find_tag(tag_type);
  if ((elt == NULL) || ((elt->out_buf == NULL) && (elt->num_bytes > 0)))
    { // Either it does not exist, or it is an input tag
      create_tag(tag_type);
      elt = find_tag(tag_type);
    }
  assert(elt != NULL);
  if ((elt->num_bytes & 7) != 0)
    { kdu_error e; e << "Misaligned access to structured data fields in a "
      "TIFF tag using `kdu_tiffdir::write_tag'.  You appear to be mixing "
      "calls to the unstructured byte-wise `write_tag' functions with one "
      "of the structured value-wise `write_tag' functions."; }
  
  
  kdu_long new_total_bytes = elt->num_bytes + length * elt->bytes_per_field;
  if (new_total_bytes > elt->max_bytes)
    { // Allocate or reallocate buffer
#ifdef KDU_POINTERS64
      elt->max_bytes += 4 + new_total_bytes; // Always at least 4 bytes long      
      kdu_byte *new_buf = new kdu_byte[elt->max_bytes];
#else
      int maxb = (int)(4+new_total_bytes);
      elt->max_bytes = maxb;
      if (elt->max_bytes != (4+new_total_bytes))
        { kdu_error e; e << "Allocating way too much memory in "
        "`kdu_tiffdir::write_tag'!!"; }
      kdu_byte *new_buf = new kdu_byte[maxb];
#endif
      if (elt->out_buf != NULL)
        {
          memcpy(new_buf,elt->out_buf,(size_t)(elt->num_bytes));
          delete[] elt->out_buf;
        }
      elt->out_buf = new_buf;
    }
#ifdef KDU_POINTERS64
  kdu_byte *dest = elt->out_buf + elt->num_bytes;
#else
  kdu_byte *dest = elt->out_buf + ((kdu_uint32) elt->num_bytes);
#endif
  if ((data_type == 3) || (data_type == 8))
    { // Copy least significant 16 bits of each data value
      kdu_uint16 *dp = (kdu_uint16 *) dest;
      int n=(int)length;
      if (native_littlendian == littlendian)
        for (; n > 0; n--, dp++, data++)
          *dp = (kdu_uint16) *data;
      else
        for (; n > 0; n--, dp++, data++)
          {
            kdu_uint16 val = (kdu_uint16) *data;
            val = (val>>8) + (val<<8);
            *dp = val;
          }
    }
  else if ((data_type == 4) || (data_type == 9))
    { // Copy least significant 32 bits of each data value
      kdu_uint32 *dp = (kdu_uint32 *) dest;
      int n=(int)length;
      if (native_littlendian == littlendian)
        for (; n > 0; n--, dp++, data++)
          *dp = (kdu_uint32) *data;
      else
        for (; n > 0; n--, dp++, data++)
          {
            kdu_uint32 val = (kdu_uint32) *data;
            val = (val>>24) + ((val>>8) & 0x0000FF00)
            + ((val<< 8) & 0x00FF0000) + (val<<24);
            *dp = val;
          }      
    }
  else
    { // Directly transfer the 64-bit `data' values
      memcpy(dest,data,(size_t)(new_total_bytes-elt->num_bytes));
      if (native_littlendian != littlendian)
        { // Flip bytes in each word
          int n, num_dwords = (int)((new_total_bytes-elt->num_bytes)>>2);
          kdu_uint32 val, *wp = (kdu_uint32 *) dest;
          for (n=0; n < num_dwords; n++)
            {
              val = wp[n];
              val = (val>>24) + ((val>>8) & 0x0000FF00)
                  + ((val<< 8) & 0x00FF0000) + (val<<24);
              wp[n] = val;
            }
          // Finally flip pairs of 32-bit words
          for (n=0; n < num_dwords; n+=2)
            { val = wp[n];  wp[n] = wp[n+1];  wp[n+1] = val; }
        }
    }
  elt->num_bytes = new_total_bytes;
  elt->num_fields = new_total_bytes / elt->bytes_per_field;
}

/*****************************************************************************/
/*                           kdu_tiffdir::copy_tag                           */
/*****************************************************************************/

void
  kdu_tiffdir::copy_tag(kdu_tiffdir &src, kdu_uint32 tag_type)
{
  kdu_byte buf[256];
  int xfer_bytes;
  while ((xfer_bytes = (int) src.read_tag(tag_type,256,buf)) > 0)
    {
      if (src.littlendian != this->littlendian)
        { // Need to flip bytes
          kdu_byte tbyte;
          int n, word_size = get_fieldlength(tag_type);
          if (((tag_type & 0x0000FFFF)==5) || ((tag_type & 0x0000FFFF)==10))
            { // Rationals consist of two words each
              assert(word_size == 8);
              word_size = 4;
            }
          if (word_size == 2)
            {
              assert((xfer_bytes & 1) == 0);
              for (n=0; n < xfer_bytes; n += 2)
                { tbyte=buf[n+0]; buf[n+0]=buf[n+1]; buf[n+1]=tbyte; }
            }
          else if (word_size == 4)
            {
              assert((xfer_bytes & 3) == 0);
              for (n=0; n < xfer_bytes; n += 4)
                { tbyte=buf[n+0]; buf[n+0]=buf[n+3]; buf[n+3]=tbyte;
                  tbyte=buf[n+1]; buf[n+1]=buf[n+2]; buf[n+2]=tbyte; }
            }
          else if (word_size == 8)
            {
              assert((xfer_bytes & 7) == 0);
              for (n=0; n < xfer_bytes; n += 8)
                { tbyte=buf[n+0]; buf[n+0]=buf[n+7]; buf[n+7]=tbyte;
                  tbyte=buf[n+1]; buf[n+1]=buf[n+6]; buf[n+6]=tbyte;
                  tbyte=buf[n+2]; buf[n+2]=buf[n+5]; buf[n+5]=tbyte;
                  tbyte=buf[n+3]; buf[n+3]=buf[n+4]; buf[n+4]=tbyte; }
            }
          else
            assert(word_size == 1);
        }
      write_tag(tag_type,xfer_bytes,buf);
      if (xfer_bytes < 256)
        break;
    }
}
