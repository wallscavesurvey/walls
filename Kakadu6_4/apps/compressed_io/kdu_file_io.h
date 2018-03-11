/*****************************************************************************/
// File: kdu_file_io.h [scope = APPS/COMPRESSED-IO]
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
   Defines classes derived from "kdu_compressed_source" and
"kdu_compressed_target" which may be used by applications to realize
simple file-oriented compressed data I/O.  Also defines inline functions,
"kdu_ftell" and "kdu_fseek" which behave similarly to "ftell" and "fseek",
except that they work with file position arguments of type "kdu_long", which
may be a 64-bit data type even on 32-bit machines.  Implementation of these
inlines is architecture specific and may need to be modified as new
architectures are considered.
******************************************************************************/

#ifndef KDU_FILE_IO_H
#define KDU_FILE_IO_H

#include <stdio.h> // Use C I/O functions for speed; can make a big difference
#include "kdu_elementary.h"
#include "kdu_compressed.h"
#include "kdu_messaging.h"

#if defined WIN32_64 || defined _WIN64
#include <io.h>
#endif // WIN32_64 || _WIN64

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
      When defining these macros in header files, be sure to undefine
   them at the end of the header.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(kdu_file_io.h)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_file_io.h)",_id);
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


// Classes defined here:
class kdu_simple_file_source;
class kdu_simple_file_target;

/* ========================================================================= */
/*                        File Positioning Functions                         */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                        kdu_ftell                                   */
/*****************************************************************************/

inline kdu_long
  kdu_ftell(FILE *fp)
{
#if (defined WIN32_64 || defined _WIN64)
  { // On Windows platform, `fpos_t' and `kdu_long' are the same data type.
    fpos_t result;
    fgetpos(fp,&result);
    return result;
  }
#elif (defined KDU_LONG64 && defined __APPLE__)
  {
    return ftello(fp);
  }
#elif (defined KDU_LONG64 && defined __USE_FILE_OFFSET64)
  { // For GCC builds with 64-bit file addressing
    return ftello64(fp);
  }
#else
  return ftell(fp);
#endif
}

/*****************************************************************************/
/* INLINE                        kdu_fseek                                   */
/*****************************************************************************/

inline int
  kdu_fseek(FILE *fp, kdu_long offset)
  /* `offset' is always relative to the start of the file. */
{
#if (defined WIN32_64 || defined _WIN64)
  { // On Windows platform, `fpos_t' and `kdu_long' are the same data type.
    return fsetpos(fp,&offset);
  }
#elif (defined KDU_LONG64 && defined __APPLE__)
  { // For GCC builds wtih 64-bit file addressing
    return fseeko(fp,offset,SEEK_SET);
  }
#elif (defined KDU_LONG64 && defined __USE_FILE_OFFSET64)
  { // For GCC builds wtih 64-bit file addressing
    return fseeko64(fp,offset,SEEK_SET);
  }
#else
  return fseek(fp,offset,SEEK_SET);
#endif
}

/*****************************************************************************/
/* INLINE                     kdu_fseek (origin)                             */
/*****************************************************************************/

inline int
  kdu_fseek(FILE *fp, kdu_long offset, int origin)
{
#if (defined WIN32_64 || defined _WIN64)
  { // On Windows platform, `fpos_t' and `kdu_long' are the same data type.
    if (origin == SEEK_CUR)
      offset += kdu_ftell(fp);
    else if (origin == SEEK_END)
      {
        int seek_success = fseek(fp,0,SEEK_END);
        if (seek_success != 0)
          return seek_success;
        offset += kdu_ftell(fp);
      }
    return fsetpos(fp,&offset);
  }
#elif (defined KDU_LONG64 && defined __APPLE__)
  { // For GCC builds wtih 64-bit file addressing
    return fseeko(fp,offset,origin);
  }
#elif (defined KDU_LONG64 && defined __USE_FILE_OFFSET64)
  { // For GCC builds wtih 64-bit file addressing
    return fseeko64(fp,offset,origin);
  }
#else
  return fseek(fp,offset,origin);
#endif
}


/* ========================================================================= */
/*                            Class Definitions                              */
/* ========================================================================= */

/*****************************************************************************/
/*                          kdu_simple_file_source                           */
/*****************************************************************************/

class kdu_simple_file_source : public kdu_compressed_source {
  /* [BIND: reference] */
  public: // Member functions
    kdu_simple_file_source() { file = NULL; }
    kdu_simple_file_source(const char *fname, bool allow_seeks=true)
      { file = NULL; open(fname,allow_seeks); }
      /* [SYNOPSIS] Convenience constructor, which also calls `open'. */
    ~kdu_simple_file_source() { close(); }
      /* [SYNOPSIS] Automatically calls `close'. */
    bool exists() { return (file != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() { return (file == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    bool open(const char *fname, bool allow_seeks=true,
              bool return_on_failure=false)
      {
      /* [SYNOPSIS]
           Closes any currently open file and attempts to open a new one.
           If the file cannot be opened, the function either returns false
           (`return_on_failure'=true) or generates an appropriate error
           through `kdu_error'.
         [ARG: fname]
           Relative path name of file to be opened.
         [ARG: allow_seeks]
           If false, seeking within the code-stream will not be permitted.
           Disabling seeking has no effect unless the code-stream contains
           TLM and/or PLT marker segments, in which case the ability
           to seek within the file can save a lot of memory when working
           with large images, but this may come at the expense of some loss
           in speed if we know ahead of time that we want to decompress
           the entire image.
         [ARG: return_on_failure]
           Determines whether an error is generated or the function returns
           false, in the event that the file cannot be opened.
      */
        close();
        file = fopen(fname,"rb");
        if (file == NULL)
          {
            if (return_on_failure) return false;
            KDU_ERROR(e,0); e <<
              KDU_TXT("Unable to open compressed data file")
              << ", \"" << fname << "\"!";
          }
        capabilities = KDU_SOURCE_CAP_SEQUENTIAL;
        if (allow_seeks)
          capabilities |= KDU_SOURCE_CAP_SEEKABLE;
        return true;
      }
    virtual int get_capabilities() { return capabilities; }
      /* [SYNOPSIS]
           The returned capabilities word always includes the flag,
           `KDU_SOURCE_CAP_SEQUENTIAL', but may also include
           `KDU_SOURCE_CAP_SEEKABLE', depending on the `allow_seeks' argument
           passed to `open'.  See `kdu_compressed_source::get_capabilities'
           for an explanation of capabilities.
      */
    virtual bool seek(kdu_long offset)
      { /* [SYNOPSIS] See `kdu_compressed_source::seek' for an explanation. */
        assert(file != NULL);
        if (!(capabilities & KDU_SOURCE_CAP_SEEKABLE))
          return false;
        kdu_fseek(file,offset);
        return true;
      }
    virtual kdu_long get_pos()
      { /* [SYNOPSIS]
           See `kdu_compressed_source::get_pos' for an explanation. */
        return (file==NULL)?-1:kdu_ftell(file);
      }
    virtual int read(kdu_byte *buf, int num_bytes)
      { /* [SYNOPSIS] See `kdu_compressed_source::read' for an explanation. */
        assert(file != NULL);
        num_bytes = (int) fread(buf,1,(size_t) num_bytes,file);
        return num_bytes;
      }
    virtual bool close()
      { /* [SYNOPSIS]
             It is safe to call this function, even if no file has been opened.
             The return value has no meaning here.
        */
        if (file != NULL)
          fclose(file);
        file = NULL;
        return true;
      }
  private: // Data
    int capabilities;
    FILE *file;
  };

/*****************************************************************************/
/*                          kdu_simple_file_target                           */
/*****************************************************************************/

class kdu_simple_file_target : public kdu_compressed_target {
  /* [BIND: reference] */
  public: // Member functions
    kdu_simple_file_target() { file=NULL; can_strip_tail=false; }
    kdu_simple_file_target(const char *fname, bool append_to_existing=false)
      { file = NULL; open(fname,append_to_existing); }
      /* [SYNOPSIS] Convenience constructor, which also calls `open'. */
    ~kdu_simple_file_target() { close(); }
      /* [SYNOPSIS] Automatically calls `close'. */
    bool exists() { return (file != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() { return (file == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    bool open(const char *fname, bool append_to_existing=false,
              bool return_on_failure=false)
      {
      /* [SYNOPSIS]
           Opens the indicated file for writing.  If `return_on_failure'
           is true, the function returns false if it is unable to open the
           file.  Otherwise, the function generates an error message
           through `kdu_error' if the file cannot be opened.
           [//]
           If the file already exists and `append_to_existing' is true, the
           file is opened in a state which looks as if the application has
           already written all of the pre-existing contents.  This allows the
           `start_rewrite' and `end_rewrite' functions to be used to overwrite
           some of the pre-existing content -- a useful functionality, for
           writing TLM marker segments incrementally during fragmented
           compression of massive images.
      */
        close(); restore_pos = -1;  cur_pos = 0;
        if (append_to_existing && ((file = fopen(fname,"r+b")) != NULL))
          {
            kdu_fseek(file,0,SEEK_END);
            cur_pos = kdu_ftell(file);
            can_strip_tail=true;
          }
        else
          { file = fopen(fname,"wb"); can_strip_tail = false; }
        if (file == NULL)
          {
            if (return_on_failure) return false;
            KDU_ERROR(e,1); e <<
              KDU_TXT("Unable to open compressed data file")
              << ", \"" << fname << "\"!";
          }
        return true;
      }
    bool strip_tail(kdu_byte *buf, int num_bytes)
      {
      /* [SYNOPSIS]
           This function may be used only if the file was opened with
           `append_to_existing' equal to true, and the file was found to
           exist already, and the `write' function has not yet been used.
           In that case, the function reads the final `num_bytes' of the
           original file into `buf' and adjusts the existing file length
           to reflect the fact that these bytes have been removed.  If
           fewer than `num_bytes' were in the original file, or if any
           of the above conditions were not met, the function returns
           false immediately.
      */
        if ((((kdu_long) num_bytes) > cur_pos) || !can_strip_tail)
          return false;
        cur_pos -= num_bytes;  kdu_fseek(file,cur_pos);
        if (fread(buf,1,(size_t) num_bytes,file) != (size_t) num_bytes)
          return false;
        can_strip_tail = false;  kdu_fseek(file,cur_pos);
        return true;
      }
    virtual bool write(const kdu_byte *buf, int num_bytes)
      { /* [SYNOPSIS] See `kdu_compressed_target::write' for an explanation. */
        int write_bytes = num_bytes;
        if ((restore_pos >= 0) && ((cur_pos+write_bytes) > restore_pos))
          write_bytes = (int)(restore_pos-cur_pos);
        if (write_bytes > 0)
          {
            write_bytes = (int) fwrite(buf,1,(size_t) write_bytes,file);
            cur_pos += write_bytes;
          }
        can_strip_tail = false;
        return (write_bytes == num_bytes);
      }
    virtual bool close()
      { /* [SYNOPSIS]
             It is safe to call this function, even if no file has yet been
             opened.
        */
        if (file != NULL)
          fclose(file);
        file = NULL;
        can_strip_tail = false;
        return true;
      }
    virtual bool start_rewrite(kdu_long backtrack)
      {
        if ((file == NULL) || (restore_pos >= 0) ||
            (backtrack < 0) || (backtrack > cur_pos))
          return false;
        fflush(file);
        restore_pos = cur_pos;
        if (backtrack > 0)
          { cur_pos -= backtrack;  kdu_fseek(file,cur_pos); }
        return true;
      }
    virtual bool end_rewrite()
      {
        if (restore_pos < 0)
          return false;
        kdu_long advance = restore_pos - cur_pos;
        restore_pos = -1;
        if (advance != 0)
          { cur_pos += advance;  fflush(file);  kdu_fseek(file,cur_pos); }
        return true;
      }
  private: // Data
    FILE *file;
    kdu_long restore_pos; // -ve if not in a rewrite
    kdu_long cur_pos; // Current position in file
    bool can_strip_tail; // If file is open for update
  };

#undef KDU_ERROR
#undef KDU_ERROR_DEV
#undef KDU_WARNING
#undef KDU_WARNING_DEV
#undef KDU_TXT

#endif // KDU_FILE_IO_H
