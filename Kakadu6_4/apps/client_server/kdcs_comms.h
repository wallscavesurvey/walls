/*****************************************************************************/
// File: kdcs_comms.h [scope = APPS/CLIENT-SERVER]
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
  Common definitions used in the implementation of basic communication
services for both the client and the server.  This file contains
definitions which are platform specific.
******************************************************************************/

#ifndef KDCS_COMMS_H
#define KDCS_COMMS_H

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "kdu_elementary.h"

// Defined here:
class kdcs_message_block;
class kdcs_timer;
class kdcs_sockaddr;
class kdcs_channel_servicer;
class kdcs_channel_monitor;
class kdcs_channel;
class kdcs_tcp_channel;

// Defined elsewhere
struct kdcs_socket;
struct kdcs_fd_sets; // Single object containing read/write/err fd_sets
struct sockaddr; // So we don't have to include any system headers here
struct kdcs_channel_ref;


/* ========================================================================= */
/*                               Exceptions                                  */
/* ========================================================================= */

#define KDCS_LIFESPAN_EXCEPTION ((kdu_exception) 1)
#define KDCS_CLOSING_EXCEPTION  ((kdu_exception) -1)
#define KDCS_CLOSED_EXCEPTION   ((kdu_exception) -2)

/* ========================================================================= */
/*                         Base JPIP syntax names                            */
/* ========================================================================= */

#define JPIP_FIELD_TARGET          "target"
#define JPIP_FIELD_SUB_TARGET      "subtarget"
#define JPIP_FIELD_TARGET_ID       "tid"

#define JPIP_FIELD_CHANNEL_ID      "cid"
#define JPIP_FIELD_CHANNEL_NEW     "cnew"
#define JPIP_FIELD_CHANNEL_CLOSE   "cclose"
#define JPIP_FIELD_REQUEST_ID      "qid"


#define JPIP_FIELD_FULL_SIZE       "fsiz"
#define JPIP_FIELD_REGION_SIZE     "rsiz"
#define JPIP_FIELD_REGION_OFFSET   "roff"
#define JPIP_FIELD_COMPONENTS      "comps"
#define JPIP_FIELD_CODESTREAMS     "stream"
#define JPIP_FIELD_CONTEXTS        "context"
#define JPIP_FIELD_ROI             "roi"
#define JPIP_FIELD_LAYERS          "layers"
#define JPIP_FIELD_SOURCE_RATE     "srate"

#define JPIP_FIELD_META_REQUEST    "metareq"

#define JPIP_FIELD_MAX_LENGTH      "len"
#define JPIP_FIELD_MAX_QUALITY     "quality"

#define JPIP_FIELD_ALIGN           "align"
#define JPIP_FIELD_WAIT            "wait"
#define JPIP_FIELD_TYPE            "type"
#define JPIP_FIELD_DELIVERY_RATE   "drate"

#define JPIP_FIELD_MODEL           "model"
#define JPIP_FIELD_TP_MODEL        "tpmodel"
#define JPIP_FIELD_NEED            "need"

#define JPIP_FIELD_CAPABILITIES    "cap"
#define JPIP_FIELD_PREFERENCES     "pref"

#define JPIP_FIELD_UPLOAD          "upload"
#define JPIP_FIELD_XPATH_BOX       "xpbox"
#define JPIP_FIELD_XPATH_BIN       "xpbin"
#define JPIP_FIELD_XPATH_QUERY     "xpquery"


/* ========================================================================= */
/*                     Return Data Termination Codes                         */
/* ========================================================================= */

#define JPIP_EOR_IMAGE_DONE             ((kdu_byte) 1)
#define JPIP_EOR_WINDOW_DONE            ((kdu_byte) 2)
#define JPIP_EOR_WINDOW_CHANGE          ((kdu_byte) 3)
#define JPIP_EOR_BYTE_LIMIT_REACHED     ((kdu_byte) 4)
#define JPIP_EOR_QUALITY_LIMIT_REACHED  ((kdu_byte) 5)
#define JPIP_EOR_SESSION_LIMIT_REACHED  ((kdu_byte) 6)
#define JPIP_EOR_RESPONSE_LIMIT_REACHED ((kdu_byte) 7)
#define JPIP_EOR_UNSPECIFIED_REASON     ((kdu_byte) 0xFF)

/* ========================================================================= */
/*                Timing Objectives for JPIP Client/Server                   */
/* ========================================================================= */

#define KDCS_TARGET_SERVICE_EPOCH_SECONDS 0.5
   // Whether implemented at the client or the server, flow control algorithms
   // aim to maintain responsiveness by avoiding the preparation of large
   // chunks of data by the server which could clog up the delivery pipe and
   // limit the client's ability to issue pre-emptive requests.  The service
   // epoch is the estimated amount of transmission time required to deliver
   // a chunk of data prepared by the server.  This constant controls the
   // duration of each service epoch -- it represents a trade-off between
   // responsiveness (small epochs) and server efficiency (large epochs).

/* ========================================================================= */
/*                               Functions                                   */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                  kdcs_caseless_search                              */
/*****************************************************************************/

inline const char *
  kdcs_caseless_search(const char *string, const char *pattern)
  /* [SYNOPSIS]
       Searches for `pattern' within `string', returning a pointer to the
       character immediately after the pattern if successful, or NULL if
       unsuccessful.  All comparisons are case insensitive -- of course, this
       is meaningful only for ASCII patterns.
  */
{
  const char *mp;
  for (mp=pattern; *string != '\0'; string++)
    if (tolower(*string) == tolower(*mp))
      mp++;
    else if (*mp == '\0')
      break;
    else
      mp = pattern;
  return (*mp == '\0')?string:NULL;
}

/*****************************************************************************/
/* INLINE                 kdcs_has_caseless_prefix                           */
/*****************************************************************************/

inline bool
  kdcs_has_caseless_prefix(const char *string, const char *prefix)
  /* [SYNOPSIS]
       Returns true if `prefix' is a prefix of the supplied string; the
       comparison is case insensitive -- of course, this is meaningful only
       for ASCII prefixes.
  */
{
  for (; (*string != '\0') && (*prefix != '\0'); string++, prefix++)
    if (tolower(*string) != tolower(*prefix))
      return false;
  return (*prefix == '\0');
}

/*****************************************************************************/
/* INLINE                   kdcs_caseless_compare                            */
/*****************************************************************************/

inline bool
  kdcs_caseless_compare(const char *string, const char *ref)
  /* [SYNOPSIS]
       Returns true if `string' is identical to `ref', apart from
       lower-case/upper-case differences -- of course, this is meaningful only
       for ASCII strings.
  */
{
  for (; (*string != '\0') && (*ref != '\0'); string++, ref++)
    if (tolower(*string) != tolower(*ref))
      return false;
  return (*string == '\0') && (*ref == '\0');
}

/*****************************************************************************/
/* INLINE                 kdcs_parse_request_field                           */
/*****************************************************************************/

inline bool
  kdcs_parse_request_field(const char * &string, const char *field_name)
  /* [SYNOPSIS]
       If `string' commences with the `field_name' string, followed by '=',
       this function returns true, setting `string' to point immediately after
       the `=' sign.  Otherwise, the function returns false and leaves `string'
       unchanged.
  */
{
  const char *sp=field_name, *dp=string;
  for (; *sp != '\0'; sp++, dp++)
    if (*dp != *sp) return false;
  if (*dp != '=')
    return false;
  string = dp+1;
  return true;
}

/*****************************************************************************/
/* INLINE                  kdcs_parse_jpip_header                            */
/*****************************************************************************/

inline const char *
  kdcs_parse_jpip_header(const char *string, const char *field_name)
  /* [SYNOPSIS]
       Scans through the supplied `string' looking for a header which commences
       with the prefix "JPIP-", immediately preceded by a new-line character,
       and immediately followed by `field_name' and a ":".  If successful,
       the function returns a pointer to the first non-white-space character
       following the ":".  Otherwise, it returns NULL.
  */
{
  const char *fp, *sp=string;
  for (; *sp != '\0'; sp++)
    {
      if ((sp[0] != '\n') || (sp[1] != 'J') || (sp[2] != 'P') ||
          (sp[3] != 'I') || (sp[4] != 'P') || (sp[5] != '-'))
        continue;
      for (sp+=5, fp=field_name; *fp != '\0'; fp++, sp++)
        if (sp[1] != *fp)
          break;
      if (*fp != '\0')
        continue;
      if (sp[1] != ':')
        continue;
      for (sp+=2; (*sp == ' ') || (*sp == '\t'); sp++);
      return sp;
    }
  return NULL;
}

/*****************************************************************************/
/* EXTERN                     kdcs_microsleep                                */
/*****************************************************************************/

KDU_AUX_EXPORT extern void kdcs_microsleep(int usecs);
  /* [SYNOPSIS]
       Sleeps for the indicated number of microseconds.  On some OS's, the
       sleep period may need to be rounded up to the nearest number of
       milliseconds.
  */

/*****************************************************************************/
/* EXTERN                        kdcs_chdir                                  */
/*****************************************************************************/

KDU_AUX_EXPORT extern bool kdcs_chdir(const char *pathname);
  /* [SYNOPSIS]
       Abstracts the operating-system-dependent operation of changing to
       a new directory.  This is useful for implementing a server and
       possibly for other applications.  The function returns false if it
       fails to change the directory to that supplied via `pathname', which
       may happen because the operation is not supported on the current
       operating system.  Absolute and relative path names may be supported.
  */

/*****************************************************************************/
/* EXTERN                   kdcs_start_network                               */
/*****************************************************************************/

KDU_AUX_EXPORT extern bool kdcs_start_network();
  /* [SYNOPSIS]
       It might be useful to call this at the start of an application which
       is going to be using networking resources and to finish up with a
       matching call to `kdcs_cleanup_network'.  In practice, these calls do
       nothing except in a Windows-based OS.  Even then, there is no need
       to explicitly call the function yourself.  Whenever you use any
       of the API functions defined in this header file which calls for
       network services, the `kdcs_start_network' function will be called
       for you automatically -- there is no harm in calling it multiple
       times -- to start up networking services if they are not already
       available.
       [//]
       You may find it useful to invoke `kdcs_cleanup_network' yourself if
       there is a point in an application when networking services are not
       required for some time and you wish to reclaim the resources.
       However, in practice, it is unlikely that you will feel the need to
       call these functions explicitly yourself.
       [//]
       If something goes wrong with an application which depends upon
       networking, it may be useful to explicitly call this function and
       check the return value. If false, you know that the networking
       services failed to start.  This might happen, for example, if too
       many tasks are currently interacting with networking resources on
       the current platform.
  */

/*****************************************************************************/
/* EXTERN                  kdcs_cleanup_network                              */
/*****************************************************************************/

KDU_AUX_EXPORT extern void kdcs_cleanup_network();
  /* [SYNOPSIS]
       See `kdcs_start_network'.
  */

/* ========================================================================= */
/*                                Classes                                    */
/* ========================================================================= */

/*****************************************************************************/
/*                           kdcs_message_block                              */
/*****************************************************************************/

class kdcs_message_block {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides services which are useful for parsing and
       assembling blocks (e.g., paragraphs) of text or raw binary data
       for HTTP communications.  The object itself has no dependence on
       a communication channel.
  */  
  public: // Member functions
    kdcs_message_block()
      {
        block_bytes = 0; block = next_unread = next_unwritten = NULL;
        mode_hex = leave_white = false; text = NULL; text_max = 0;
      }
    ~kdcs_message_block()
      {
        if (block != NULL) delete[] block;
        if (text != NULL) delete[] text;
      }
    KDU_AUX_EXPORT void restart()
      /* [SYNOPSIS]
           Initializes both read and write pointers to the beginning of the
           block.  It is safe to call this function any time.
       */
      { next_unread = next_unwritten = block; mode_hex = false; }
    void backup()
      /* [SYNOPSIS]
           Initializes the read pointer to the beginning of the block, without
           altering the write pointer.  This allows a block of data to be read
           again from scratch, but the behaviour may be unpredictable if
           reading and writing have been interleaved (write some data, read
           some data, write some more data, etc.), since in that case some
           memory may have been recycled.  The function is primarily of use
           if some data may have been written on an outgoing connection which
           is subsequently found to have been dropped.
       */
      {
        next_unread = block;
      }
    int get_remaining_bytes() { return (int)(next_unwritten-next_unread); }
      /* [SYNOPSIS]
           Returns the difference between the write pointer and the read
           pointer, representing the number of bytes which have been written
           to the block, but not yet read, since the last `restart' call.
       */
    const kdu_byte *peek_block() { return next_unread; }
      /* [SYNOPSIS]
           Returns a pointer to the unread portion of the internal message
           block.  The number of valid bytes is indicated by the
           `get_remaining_bytes' member function.
       */
    bool keep_whitespace(bool yes=true)
      { /* [SYNOPSIS]
             By default, all superfluous white space is removed in calls to
             `read_line' or `read_paragraph'.  Use this function to alter the
             whitespace keeping mode; the function returns the previous mode.
        */
        bool result = leave_white; leave_white = yes; return result;
      }
    KDU_AUX_EXPORT const char *read_line(char delim='\n');
      /* [SYNOPSIS]
           Returns the next line of text stored in the block, where lines
           are delimited by the indicated character.
           [//]
           The returned character string has all superfluous white space
           characters (' ', '\t', '\r', '\n') removed (unless
           `keep_whitespace' has been used to change the whitespace keeping
           mode).  This includes all leading white space, all trailing white
           space and all but one space character between words.  '\t' or '\r'
           characters between words are converted to ' ' characters.
           [//]
           The returned pointer refers to an internal resource, which may not
           be valid after subsequent use of the object's member functions.
           [//]
           If there are no more lines left, the function returns NULL.
       */
    KDU_AUX_EXPORT const char *read_paragraph(char delim='\n');
      /* [SYNOPSIS]
           Same as `read_line', but returns an entire paragraph, delimited by
           two consecutive instances of the `delim' character, separated at
           most by white space, one instance of the `delim' character which
           is encountered before any non-whitespace character, or the end
           of the message block.  Unlike `read_line', this function returns
           a non-NULL pointer even if there is no text to read -- in this
           case, the returned string will be empty.
       */
    KDU_AUX_EXPORT kdu_byte *read_raw(int num_bytes);
      /* [SYNOPSIS]
           Returns a pointer to an internal buffer, holding the next
           `num_bytes' bytes from the input.  The function returns NULL if
           the requested number of bytes is not available.  You can always
           check availability by calling `get_remaining_bytes'.
       */
    KDU_AUX_EXPORT void write_raw(const kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Writes the supplied bytes to the internal memory block.
       */
    void append(kdcs_message_block &src)
      /* [SYNOPSIS]
           Copies the contents of `src' to be appended to the
           current block, as raw data.  Does not actually consume any
           data from `src'.
      */
      { write_raw(src.peek_block(),src.get_remaining_bytes()); }
    int backspace(int num_chars)
      { /* [SYNOPSIS]
             Causes the most recent `num_chars' characters in the block to be
             erased, returning the number of characters left.
         */
        if (((int)(next_unwritten-next_unread)) < num_chars)
          { assert(0); num_chars = ((int)(next_unwritten-next_unread)); }
        next_unwritten -= num_chars;
        return (int)(next_unwritten-next_unread);
      }
    KDU_AUX_EXPORT int
      hex_hex_encode_tail(int num_chars, const char *special_chars=NULL);
      /* [SYNOPSIS]
           This is function facilitates the construction of URI-legal JPIP
           query fields in an efficient manner.  It essentially invokes
           `kdu_hex_hex_encode' on the most recently written `num_chars'
           characters (the read pointer must be at least `num_chars'
           behind the write pointer), passing `special_chars' through to
           `kdu_hex_hex_encode' to allow for stronger encoding.  Of course,
           you should be careful to apply this function in a way which does
           not encode strings which have already been subjected to hex-hex
           encoding.
           [//]
           The function returns the length of the string which replaces
           the original `num_chars'-long suffix.  This is necessarily no
           smaller than `num_chars'; it will be larger if and only if
           some characters were encoded.
           [//]
           This function utilizes unused internal buffer space so as to
           avoid unnecessary allocation and reallocation of buffers.
           As such, it is generally more efficient than performing
           hex-hex encoding externally using a seprately allocated buffer.
      */
    bool set_hex_mode(bool new_mode)
      { bool old_mode = mode_hex; mode_hex = new_mode; return old_mode; }
      /* [SYNOPSIS]
           Hex mode affects the way integers are written as text by the
           overloaded "<<" operator.
       */
    void write_text(const char *string)
      { write_raw((kdu_byte *) string,(int) strlen(string)); }
      /* [SYNOPSIS]
           Writes the supplied text string to the internal memory block.  The
           supplied null-terminated string is written as-is, without the
           null-terminator.
       */
    kdcs_message_block &operator<<(const char *string)
      { write_text(string); return *this; }
    kdcs_message_block &operator<<(char ch)
      { char text[2]; text[0]=ch; text[1]='\0';
        write_text(text); return *this; }
    kdcs_message_block &operator<<(int val)
      { char text[80];
        sprintf(text,(mode_hex)?"%x":"%d",val);
        write_text(text); return *this; }
    kdcs_message_block &operator<<(unsigned int val)
      { char text[80];
        sprintf(text,(mode_hex)?"%x":"%u",val);
        write_text(text); return *this; }
    kdcs_message_block &operator<<(short int val)
      { return (*this)<<(int) val; }
    kdcs_message_block &operator<<(unsigned short int val)
      { return (*this)<<(unsigned int) val; }
#ifdef KDU_LONG64
    kdcs_message_block &operator<<(kdu_long val)
      { // Conveniently prints in decimal with thousands separated by commas
        if (val < 0)
          { (*this)<<'-'; val = -val; }
        kdu_long base = 1;
        while ((1000*base) <= val) base *= 1000;
        int digits = (int)(val/base);   (*this) << digits;
        while (base > 1)
          { val -= base*digits;  base /= 1000;  digits = (int)(val/base);
            char text[4]; sprintf(text,"%03d",digits); (*this)<<','<<text; }
        return (*this);
      }
#endif // KDU_LONG64
    kdcs_message_block &operator<<(float val)
      { char text[80]; sprintf(text,"%f",val);
        write_text(text); return *this; }
    kdcs_message_block &operator<<(double val)
      { char text[80]; sprintf(text,"%f",val);
        write_text(text); return *this; }
  private: // Data
    int block_bytes;
    kdu_byte *block;
    kdu_byte *next_unread; // Points to next location to read from`block'.
    kdu_byte *next_unwritten; // Points to next location to write into `block'.
    char *text; // Buffer used for accumulating parsed ASCII text
    int text_max; // Max chars allowed in `text', before reallocation needed
    bool mode_hex;
    bool leave_white;
  };

/*****************************************************************************/
/*                               kdcs_timer                                  */
/*****************************************************************************/

class kdcs_timer {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides high resolution (if possible) timing
       facilities in a platform independent way.  It plays an
       important role in the implementation of `kdcs_channel_monitor'.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdcs_timer();
      /* [SYNOPSIS]
           Initializes the object so that subsequent calls to
           `get_ellapsed_microseconds' will provide meaningful results.
      */
    KDU_AUX_EXPORT kdu_long get_ellapsed_microseconds();
      /* [SYNOPSIS]
           Returns the number of microseconds which have ellapsed since
           the object was constructed.
           [//]
           On some platforms, the resolution of the clock used here might
           be on the order of several milliseconds.  On other platforms,
           the clock resolution might be on the order of microseconds or
           might even run at the same rate as the CPU clock.  In any case,
           this object provides the means for reliably accumulating times
           at very high precision, when available, without having to worry
           about the internal mechanics.
           [//]
           On some platforms, it is possible that the function will not work
           correctly if it is invoked too infrequently (e.g., only invoked
           once a month or something like that).  In practice, though, this
           should not be an issue for reasonable client/server applications.
           [//]
           Depending on the precision of the internal clock, it is possible
           that successive calls to this function return the same value;
           however, no call to this function will ever return a value smaller
           than a previous call, even if the internal clock undergoes
           resynchronization by the system.  In such cases, the object will
           adjust its internal state so as to maintain faithful accumulated
           times as well as it possibly can.
           [//]
           This function is not necessarily thread safe.  You should call it
           within a protected environment or ensure access from only one
           thread.
      */
    int get_precision() { return clock_resolution; }
      /* [SYNOPSIS]
           Returns the precision of the internal clock, measured in
           microseconds.  If you want to compare the timing produced by this
           object with those used by other system functions (e.g., calls to
           the BSD "select" function with a timeout), you may need to take
           the precision into account.
      */
    void synchronize(kdcs_timer &ref)
      {
        *this = ref; // Works at least for the current implementations.
      }
      /* [SYNOPSIS]
           Synchronizes the time reported by this object's
           `get_ellapsed_microseconds' function with that reported by
           `ref.get_ellapsed_microseconds'.  You should be careful to
           avoid calling this function while other calls are in progress
           on this object or the `ref' object, from another thread.
      */
  private: // Data
#ifdef KDU_WINDOWS_OS
    __int64 high_resolution_freq; // Use with `QueryPerformanceFrequency'
    __int64 high_resolution_time; // Use with `QueryPerformanceCounter'
    double high_resolution_factor; // microseconds per high-res clock tick
    kdu_long high_resolution_base; // Allows infrequent reset of hires time
    kdu_uint32 tick_count_milliseconds; // Use with `GetTickCount'
#else // !Windows
    kdu_timespec tspec; // Defined in "kdu_elementary.h"
#endif // !Windows
    int clock_resolution; // Measured in microseconds
    kdu_long last_ellapsed_microseconds;
};

/*****************************************************************************/
/*                           kdcs_channel_servicer                           */
/*****************************************************************************/

#define KDCS_CONDITION_READ            ((int) 1)
#define KDCS_CONDITION_WRITE           ((int) 2)
#define KDCS_CONDITION_CONNECT         ((int) 4)
#define KDCS_CONDITION_ACCEPT          ((int) 8)
#define KDCS_CONDITION_WAKEUP          ((int) 16)
#define KDCS_CONDITION_MONITOR_CLOSING ((int) 32)
#define KDCS_CONDITION_ERROR           ((int) 64)

#define KDCS_READ_CONDITIONS (KDCS_CONDITION_READ | KDCS_CONDITION_ACCEPT)
#define KDCS_WRITE_CONDITIONS (KDCS_CONDITION_WRITE | KDCS_CONDITION_CONNECT)
#define KDCS_ERROR_CONDITIONS (KDCS_CONDITION_CONNECT | KDCS_CONDITION_ERROR |\
                               KDCS_CONDITION_READ | KDCS_CONDITION_WRITE)
      // Not clear right now whether READ and WRITE waits need to register
      // for errors as well when calling `select', but CONNECT waits do.

class kdcs_channel_servicer {
  protected: // Privileged operations
    friend class kdcs_channel_monitor;
    KDU_AUX_EXPORT virtual ~kdcs_channel_servicer();
      /* [SYNOPSIS]
           You should not explicitly call the destructor yourself; instead,
           use the reference counting method provided by `add_ref' and
           `release_ref' to safely manage the destruction of this object.
      */
    KDU_AUX_EXPORT void add_ref();
      /* [SYNOPSIS]
           The object is assigned a reference count of 1 by the constructor.
           This function is called by each additional context which retains a
           reference to the object.  In particular, the `kdcs_channel_monitor'
           object adds a reference to this object when it is passed to the
           `kdcs_channel_monitor::add_channel' function.  Access to this
           function is controlled internally by a mutex, to ensure thread
           safety.
      */
    KDU_AUX_EXPORT void release_ref();
      /* [SYNOPSIS]
           Use this function instead of explicitly calling the object's
           destructor.  This ensures that the object can be cleaned up safely.
           Otherwise, there is a chance that one thread might destroy the
           object while another thread tries to invoke one of its member
           functions from within `kdcs_channel_monitor::run_once'.  Access
           to this function is controlled internally by a mutex, to ensure
           thread safety.
      */
  public: // Reference counting
    KDU_AUX_EXPORT kdcs_channel_servicer();
      /* [SYNOPSIS]
           Sets up reference counting and installs one reference to the object
           so that an immediate call to `release_ref' will destroy it.
           [//]
           You must be sure to always create objects of this type on the heap,
           using `new', rather than declaring them on the stack.
      */
  public: // Abstract interface
    virtual void service_channel(kdcs_channel_monitor *monitor,
                                 kdcs_channel *channel, int cond_flags) = 0;
      /* [SYNOPSIS]
           Called from within `kdcs_channel_monitor::run_once' if a condition
           registered by `kdcs_channel_monitor::queue_conditions',
           or a time supplied to `kdcs_channel_monitor::schedule_wakeup' has
           occured, or if the `kdcs_channel_monitor::request_closure'
           function has been called.
           [//]
           In any case, the condition (or conditions) which
           have occurred are communicated via the `cond_flags' argument
           and the corresponding `channel' is also identified.
           [//]
           Some comments on design are in order here.  Although an
           implementation of this function can test each of the individual
           condition flags passed via `cond_flags', it could just attempt
           to perform any outstanding I/O whenever the function is
           called.  Even if the application has requested the
           `kdcs_channel_monitor' object to shutdown (as indicated by
           the `KDCS_CONDITION_MONITOR_CLOSING' flag), it is still
           appropriate to attempt I/O on the `kdcs_channel' object and
           detect such unusual conditions via exceptions which are thrown.
           For more on this, consult the exceptions documented for
           the `kdcs_tcp_channel' object's connect, accept, read and write
           functions.
         [ARG: cond_flags]
           Logical OR of one or more of the flags `KDCS_CONDITION_READ',
           `KDCS_CONDITION_WRITE', `KDCS_CONDITION_CONNECT',
           `KDCS_CONDITION_ACCEPT', `KDCS_CONDITION_ERROR',
           `KDCS_CONDITION_WAKEUP' and `KDCS_CONDITION_MONITOR_CLOSING',
           indicating which conditions have occurred.
           [//]
           The first four conditions (read, write, connect and accept) have
           been cleared from the set of conditions that will be monitored for
           the channel on the next call to `kdcs_channel_monitor::run_once'.
           The implementation can re-instate any of these conditions by
           re-invoking `kdcs_channel_monitor::queue_conditions' (typically
           done by I/O member functions of the `kdcs_channel' object).
           [//]
           If a scheduled wakeup time has occurred, the `cond_flags' argument
           will include the `KDCS_CONDITION_WAKEUP' flag, possibly mixed with
           some of the above four flags if additional monitored conditions
           have occurred.
           [//]
           The `KDCS_CONDITION_MONITOR_CLOSING' flag means that the `monitor'
           object's `kdcs_channel_monitor::request_closure' function has been
           called.
           [//]
           If a channel error condition is detected, the `KDCS_CONDITION_ERROR'
           flag will be set, in addition to any of the `KDCS_CONDITION_READ',
           `KDCS_CONDITION_WRITE' or `KDCS_CONDITION_CONNECT' conditions which
           were being monitored when the error was detected.  This way, an
           application may either explicitly test for the
           `KDCS_CONDITION_ERROR' condition or else it may just test for the
           READ/WRITE/CONNECT condition and proceed as if errors had not
           occurred, detecting the error later when issuing the relevant
           I/O call.
      */
  private: // Data
    int ref_count;
    kdu_mutex ref_counting_mutex;
};

/*****************************************************************************/
/*                           kdcs_channel_monitor                            */
/*****************************************************************************/

class kdcs_channel_monitor {
  public: // Member functions
    KDU_AUX_EXPORT kdcs_channel_monitor();
    KDU_AUX_EXPORT ~kdcs_channel_monitor();
      /* [SYNOPSIS]
           The destructor should not be called while a call to `run_once'
           (or any other member function for that matter) is in progress.
           This function does not invoke `request_closure' or wait for
           threads to finish.  It is expected that all channels which have
           added themselves to the monitor remove themselves before this
           destructor is called; the condition is verified with an
           `assert'.
      */
    KDU_AUX_EXPORT void request_closure();
      /* [SYNOPSIS]
           Calling this function causes the next (or currently in-progress)
           call to `run_once' to return false.  Before that happens,
           all channels will have the `KDCS_CONDITION_MONITOR_CLOSING' flag
           passed to their `kdcs_channel_servicer::service_channel'
           function, from within the `run_once' function.
           [//]
           Once this function has been called, any calls to `add_channel'
           will return NULL.
      */
    KDU_AUX_EXPORT void synchronize_timing(kdcs_timer &timer);
      /* [SYNOPSIS]
           Synchronizes the supplied `timer' with the internal timer used
           by this object.  This ensures that absolute times supplied to
           other member functions can be meaningfully generated by the
           caller.
           [//]
           Frequent calls to this function could be expensive (and
           completely unnecessary) since they require a mutex to be
           locked in order to protect the internal `kdcs_timer' object
           from simultaneous access by multiple threads.
      */
    KDU_AUX_EXPORT kdu_long get_current_time();
      /* [SYNOPSIS]
           Obtains the current time, as measured by the internal clock, in
           microseconds.  This is the same clock used to time scheduled
           wakeups.  The returned time is measured in microseconds, from
           the point at which this object was created.  It will not be
           negative (unless wrap-around occurs, which should be practically
           impossible in the usual case where `kdu_long' is defined as a
           64-bit integer).
           [//]
           Frequent calls to this function could be expensive since they
           require a mutex to be locked in order to protect access to the
           internal `kdcs_timer' object.  If you need to access time values
           frequently, you are much better off creating a separate
           `kdcs_timer' object and synchronizing it with the current
           object's timer by calling the `synchronize_timing' function.
      */
    KDU_AUX_EXPORT kdcs_channel_ref *
      add_channel(kdcs_channel *channel, kdcs_channel_servicer *servicer);
      /* [SYNOPSIS]
           Adds a channel to the set of channels for which conditions can
           be monitored by this object.  The returned opaque `kdcs_channel_ref'
           pointer should be used in all other channel-specific calls.
           [//]
           Returns NULL if the `request_closure' function has been called,
           or if too many channels have already been added (depends on the
           value of the `FD_SETSIZE' macro, as defined in "comms_local.h").
           [//]
           You can use this function to change the `servicer' object used
           by a channel which is already being monitored.  In this case,
           you should note that any scheduled wakeups or queued I/O
           conditions for the channel will be cancelled before changing the
           channel servicer.
      */
    KDU_AUX_EXPORT void remove_channel(kdcs_channel_ref *ref);
      /* [SYNOPSIS]
           Removes a channel previously added by `add_channel'.
      */
    KDU_AUX_EXPORT bool
      queue_conditions(kdcs_channel_ref *ref, int cond_flags);
      /* [SYNOPSIS]
           This function adds the supplied condition flags to a queue, whose
           contents will be added to the "select set" on the next call to
           `run_once'.
         [RETURNS]
           False if `request_closure' has been called or `ref' is invalid.
         [ARG: cond_flags]
           Logical OR of one or more of the flags `KDCS_CONDITION_READ',
           `KDCS_CONDITION_WRITE', `KDCS_CONDITION_CONNECT' and
           `KDCS_CONDITION_ACCEPT', indicating which conditions need to
           be monitored.  These conditions will be added to any conditions
           already being monitored (or queued for monitoring).
      */
    KDU_AUX_EXPORT bool
      schedule_wakeup(kdcs_channel_ref *ref, kdu_long earliest,
                      kdu_long latest);
      /* [SYNOPSIS]
           Schedules a wakeup call for the indicated channel.  If a wakeup
           was already scheduled for this channel, it is simply replaced
           with the one supplied here.  To cancel a wakeup, set `earliest'
           and `latest' to -1.  To guarantee that `run_once' will call
           `kdcs_channel_servicer::service_channel' with the
           `KDCS_CONDITION_WAKEUP' flag as soon as possible, you can set
           `earliest' and `latest' both equal to 0.
           [//]
           The `earliest' and `latest' arguments are expressed in microseconds,
           relative to the creation of the current object.  For precise timing,
           you should base these on the time returned by a `kdcs_timer'
           object's `kdcs_timer::get_ellapsed_microseconds'
           function, where the `kdcs_timer' object has been synchronized
           with the present object, via a call to `synchronize_timing'.
           [//]
           You can be sure that `kdcs_channel_servicer::service_channel' will
           not be invoked with the `KDCS_CONDITION_WAKEUP' prior to the
           `earliest' time.  It may be invoked a little after the
           `latest' time, but the purpose of providing a `latest' wakeup time
           which is later than `earliest' is to indicate how tolerant you are
           of imprecise timing.  This allows the internal machinery to avoid
           lots of short sleep cycles.
           [//]
           Note that the wakeup call is always invoked from within `run_once'
           and that function may currently be blocked on a socket `select'
           operation.  As a result, the wakeup might not be serviced until
           the `wait_microseconds' period supplied to `run_once' ellapses
           and `run_once' is called again.  In applications where `run_once'
           is invoked from a separate I/O service thread, the
           `wait_microseconds' parameter might typically be on the order of
           1 to 10 milliseconds.  If you are using scheduled wakeups for
           rate control purposes when delivering data over a socket, you
           should be aware that data delivery may be bursty at this timescale.
           In practice this is unlikely to present a problem, especially
           since bursty delivery of traffic into a socket provides more
           opportunity to estimate channel delay and transmission rate
           characteristics.
         [RETURNS]
           False if `request_closure' has been called or `ref' is invalid.
      */
    KDU_AUX_EXPORT void wake_from_run();
      /* [SYNOPSIS]
           Provides a simple mechanism to signal the event on which the
           `run_once' function may be waiting if it has no conditions queued
           to wait for with `select'.
      */
    KDU_AUX_EXPORT bool
      run_once(int wait_microseconds, int new_condition_wait_microseconds);
      /* [SYNOPSIS]
           Execute one iteration of the monitor run-loop.  This involves
           the following steps:
           [>>] Collect any outstanding channel monitoring conditions
                (those registered using `queue_conditions', since the last
                iteration of the run loop) and put them into the current
                "select set".
           [>>] Wait on the conditions in the "select set", until at least
                one of them occurs, or a scheduled wakeup occurs.  The function
                waits for at most `wait_microseconds' on channel conditions
                which it is monitoring.  If no condition occurs in this
                period, the function returns.  Waiting for only a limited time
                is important, since calls to `queue_conditions',
                `schedule_wakeup' and `request_closure' which arrive while
                the object is waiting on existing channel conditions will not
                be considered until the wait is complete.
           [>>] Invoke all relevant `kdcs_channel_servicer::service_channel'
                functions; if `request_closure' is found to have been called,
                the relevant `kdcs_channel_servicer::service_channel' functions
                are called with the `KDCS_CONDITION_MONITOR_CLOSING' flag.
           [//]
           The function returns false if the `request_closure' function is
           found to have been called.  Otherwise, it returns true.
           [//]
           In many cases, the `run_once' function will be invoked repeatedly
           from a special channel monitoring thread until it returns false.
           However, it is possible to invoke the function from within a
           single threaded application to wait upon all pending channel
           conditions together.
         [RETURNS]
           True unless the `request_closure' function has been called, from
           this or any other thread.
         [ARG: wait_microseconds]
           Maximum time spent in the function waiting for channel conditions
           to occur.  If 0, the function effectively polls the conditions and
           then returns once any required channel servicing calls have
           completed.  A -ve value places no restriction on the time spent
           waiting; in practice, though, the function will never wait more
           than 1 second before returning, just to be on the safe side.
           In practice, it is strongly advisable to supply a value of at
           most a few thousand microseconds here, since new conditions
           identified via calls to `queue_conditions', `schedule_wakeup' or
           `request_closure' cannot be responded to until a current call to
           the system `select' function returns.  On the other hand, if the
           only way in which such conditions can be registered is through
           calls to the `kdcs_channel_servicer::service_channel' function,
           it should be OK to wait for much longer, since that function is
           invoked only from within the present one.
         [ARG: new_condition_wait_microseconds]
           This argument affects the behaviour only if there are no channel
           conditions which need to be monitored when the function enters.
           In this case, the function waits instead on an internal `kdu_event',
           which becomes signalled whenever `queue_conditions',
           `schedule_wakeup', `wake_from_run' or `request_closure' is called.
           [//]
           If you set this argument to 0, the function returns immediately if
           there are no channel conditions to wait upon.  This is exactly what
           you should do if your application involves only one thread.
           [//]
           A negative value for this argument causes the function to wait
           indefinitely on the internal `kdu_event'.
           [//]
           Notwithstanding any of the above, the function will become unblocked
           and return if a scheduled wakeup occurs (see `schedule_wakeup').
      */
  private: // Data
    bool close_requested;
    int num_channels;
    int max_channels; // Set upon construction
    kdcs_channel_ref *channel_refs; // List of all channels
    kdcs_fd_sets *active_fd_sets; // Dynamically allocated
    kdcs_timer timer; // Access to this object is protected by `mutex'
    kdu_mutex mutex;
    kdu_event event; // Event used to wait when there are no monitor conditions
};
  /* Notes:
       `channel_refs' points to a doubly-linked list of all channels.  This
     list has no particular order.  It is connected via the `next' and `prev'
     fields in the `kdcs_channel_ref' structure.
       `active_refs' points to a singly-linked list of only those channels
     which are being processed by a current `select' call within `run_once'.
     These are the channels referenced by at least one of `active_read_set'
     or `active_write_set.  The list is connected via the `active_next' field
     in the `kdcs_channel_ref' structure.  It is constructed from scratch in
     each call to `run_once' and is manipulated exclusively from that function.
       `active_read_set', `active_write_set' and `active_error_set' hold the
     sets of channels which are passed to the current `select' call.  The
     `select' call modifies these sets to reflect the conditions which have
     actually occurred. */

/*****************************************************************************/
/*                               kdcs_sockaddr                               */
/*****************************************************************************/

#define KDCS_ADDR_FLAG_IPV4_ONLY          ((int) 0x01)
#define KDCS_ADDR_FLAG_IPV6_ONLY          ((int) 0x02)
#define KDCS_ADDR_FLAG_LITERAL_ONLY       ((int) 0x04)
#define KDCS_ADDR_FLAG_NEED_PORT          ((int) 0x10)
#define KDCS_ADDR_FLAG_BRACKETED_LITERALS ((int) 0x20)
#define KDCS_ADDR_FLAG_ESCAPED_NAMES      ((int) 0x40)

class kdcs_sockaddr {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides services which abstract network addresses, in
       a manner which is protocol and platform independent.  It provides
       features to capture the information provided by the high level
       system function `getaddrinfo', managing multiple candidate IPv4 and/or
       IPv6 addresses.  It can translate addresses from host names and/or
       IP address strings; it can also be initialized by addresses obtained
       by other means.  The function provides everything you need to
       correctly create and connect sockets.
  */
  public: // Member functions
    kdcs_sockaddr()
      { 
        kdcs_start_network();
        addresses=NULL; addr_handle=NULL;
        address_lengths=NULL; address_families=NULL;
        string_buf=NULL; string_buf_len=0;
        reset();
      }
    kdcs_sockaddr(const kdcs_sockaddr &rhs)
      { // Copy constructor
        addresses=NULL; addr_handle=NULL;
        address_lengths=NULL; address_families=NULL;
        string_buf=NULL; string_buf_len=0;
        reset();
        copy(rhs);
      }
    ~kdcs_sockaddr()
      {
        reset();
        if (string_buf != NULL) delete[] string_buf;
      }
    KDU_AUX_EXPORT void reset();
      /* [SYNOPSIS]
           Returns the object to the empty state, so that `is_valid' returns
           false.
      */
    bool is_valid() const
      { return ((addresses!=NULL) && (num_addresses > 0) && port_valid); }
      /* [SYNOPSIS]
           Returns true if the object holds at least one valid address.
           This is only possible if the object has been successfully
           initialized by one of the `init' functions or assigned equal to
           such an object.
      */
    KDU_AUX_EXPORT void copy(const kdcs_sockaddr &rhs);
      /* [SYNOPSIS]
           Resets the current object, then copies its contents from `rhs'.
           The active address within the copy is also identical to that found
           in `rhs'.
      */
    const kdcs_sockaddr &operator=(const kdcs_sockaddr &rhs)
      { copy(rhs); return *this; }
    KDU_AUX_EXPORT bool equals(const kdcs_sockaddr &rhs) const;
      /* [SYNOPSIS]
           Returns true if the current object is valid (see `is_valid') and
           represents the same set of addresses as `rhs'.  The active address
           within this set need not be the same.
      */
    bool operator==(const kdcs_sockaddr &rhs) const { return equals(rhs); }
    bool operator!=(const kdcs_sockaddr &rhs) const { return !equals(rhs); }
    KDU_AUX_EXPORT static bool
      test_ip_literal(const char *name, int flags);
      /* [SYNOPSIS]
           This static function provides a convenient means of determining
           whether or not `name' is a literal IP address in IPv4 or IPv6
           notation.  The only meaningful flags that can be used are:
           [>>] `KDCS_ADDR_FLAG_IPV4_ONLY' -- in this case, the function
                returns true only if `name' is a dotted IPv4 literal address.
           [>>] `KDCS_ADDR_FLAG_IPV6_ONLY' -- in this case, the function
                returns true only if `name' is recognized as an IPv6
                literal address.
           [//]
           Note that the behaviour of this function can also be emulated by
           constructing a `kdcs_sockaddr' object and checking the result
           from a call to `init' with `KDCS_ADDR_FLAG_LITERAL_ONLY' added
           to the `flags' argument.  However, this latter approach allocates
           memory inside the `kdcs_sockaddr' object to store the address
           itself, whereas the present static function only performs the
           test.
      */
    KDU_AUX_EXPORT bool
      init(const char *name, int flags);
      /* [SYNOPSIS]
           Use this function to lookup a host name or translate a dotted
           IPv4 (decimal) or IPv6 (hexadecimal) address string, as found
           in `name'.  The name may resolve to multiple addresses, in which
           case the object's `first' and `next' functions may be used to
           navigate between them -- the `first' call is implied if the
           present function returns true.
           [//]
           This function invokes the powerful system function `getaddrinfo'
           and copies its results, retaining at most the `AF_INET'
           and `AF_INET6' addresses.  However, the function also provides
           additional features for pre-processing the `name' string based
           on the supplied `flags', as documented below.
           [//]
           As an added convenience, `name' may actually be NULL, in which
           case the function looks up the address of the current host.  If
           this fails, the function falls back to using the local host's
           loopback address.
         [RETURNS]
           False if the function is unable to recover any addresses which
           match the supplied data.
         [ARG: name]
           If NULL, the function resolves the current host's address.
           Otherwise, `name' holds either a host name to be resolved or a
           literal IP address.  The `flags' argument determines whether or
           not additional pre-processing is performed on the `name' string,
           primarily to support the translation of host details which are
           found within URI's.
         [ARG: flags]
           Logical OR of zero or more of the following flags:
           [>>] `KDCS_ADDR_FLAG_IPV4_ONLY' -- means that only IPv4 addresses
                will be discovered and stored in the object; this flag may
                be useful for managing interaction with systems which do not
                correctly handle IPv6.
           [>>] `KDCS_ADDR_FLAG_IPV6_ONLY' -- means that only IPv6 addresses
                will be discovered and stored in the object; this flag is
                useful primarily for testing IPv6 support under various
                scenarios.
           [>>] `KDCS_ADDR_FLAG_LITERAL_ONLY' -- means that the `name' string
                will be interpreted as a literal IPv4 or IPv6 address only;
                no attempt will be made at name resolution.  Note that this
                flag refers to the interpretation of the `name' string, whereas
                the above two flags refer to the type of results that can
                be produced by translation or name resolution.
           [>>] `KDCS_ADDR_FLAG_NEED_PORT' -- means that the port information
                is assumed to be invalid, so that `is_valid' will return false
                until such time as `set_port' has been called.  Nevertheless,
                if the present function returns true, you can still recover
                the addresses managed by the object via calls to `get_addr',
                `get_addr_len', `get_addr_family', `first' and `next'.
           [>>] `KDCS_ADDR_FLAG_BRACKETED_LITERALS' -- means that IPv6
                literal addresses are expected to be encapsulated within
                square brackets, while IPv4 literal addresses may optionally
                be encapsulated within square brackets.  Encapsulation within
                square brackets means that the first and last characters in
                `name' are `[' and `]', respectively.  If this flag is present,
                strings found within such square brackets will only be
                interpreted as IP literals and strings which do not have
                square brackets will never be interpreted as IPv6 literals.
           [>>] `KDCS_ADDR_FLAG_ESCAPED_NAMES' -- means that host names are
                hex-hex decoded prior to name resolution.  Hex-hex decoding
                does not apply to IP literals.  Together, this flag and the
                `KDCS_ADDR_FLAG_BRACKETED_LITERALS' flag allow for the correct
                parsing and resolution of the host component of a URI, in
                accordance with RFC 3986.
       */
    KDU_AUX_EXPORT bool
      init(const sockaddr *addr, size_t addr_len, int address_family);
      /* [SYNOPSIS]
           Sets the object to manage a single address, described by the
           supplied arguments.  The `address_family' argument informs users
           of the address how `addr' should be treated.  In practice, this
           information is redundant, because the family can be found
           embedded inside the opaque `addr' object, whose length is given
           by `addr_len', but the function may not check for consistency.
           [//]
           The principle use of this function is for managing the address
           information returned by such functions as `getpeername',
           `getsocketname' and `recvfrom'.
         [RETURNS]
           False if `addr_len' is too long (or zero), or some other
           inconsistency is detected -- unlikely.  Otherwise, sets the
           object to contain the single address and returns true.
      */
    KDU_AUX_EXPORT bool set_port(kdu_uint16 port);
      /* [SYNOPSIS]
           You can use this function to change the port number associated
           with all the addresses currently managed by this object.  If
           `init' was called with the `require_set_port' argument set to
           true, a call to this function is required before the `is_valid'
           function can return true.  In any event, you can always change
           the port number associated with the object's addresses, as often
           as you wish.  The information passed via this function, however,
           will not survive a subsequent call to either of the `init'
           functions.
      */
    KDU_AUX_EXPORT kdu_uint16 get_port();
      /* [SYNOPSIS]
           Retrieves the port number associated with the current active
           address -- 0 if there is none.  After a call to `next' or `first',
           this value might possibly change, although that is highly unlikely,
           since `set_port' is normally used to establish the same port number
           for all addresses managed by the object.
           [//]
           Note that although 0 is returned if there are no current valid
           addresses, it is also possible that a valid address has 0 for its
           port number.
      */
    KDU_AUX_EXPORT const char *textualize(int flags);
      /* [SYNOPSIS]
           This function returns a string which represents the current
           active address, or NULL if there is none.  The `first' and
           `next' functions may be used to adjust the current active address.
           [//]
           Note that the returned address does not include port numbering;
           the port number can be obtained by invoking `get_port' separately.
         [RETURNS]
           NULL if the object is not currently managing any addresses.  This
           function returns NULL under exactly the same conditions that
           `get_addr' returns NULL.  Otherwise, the function returns a
           pointer to a string resource which is managed internally by the
           object; the returned string should not be accessed after the
           object is destroyed, `init' is called, or the present function
           is invoked again (native languages only).
         [ARG: flags]
           Logical OR of zero or more of the following flags:
           [>>] `KDCS_ADDR_FLAG_LITERAL_ONLY' -- means that the returned
                string should provide a numeric representation of the IP
                address, so that no address to hostname resolution is required.
           [>>] `KDCS_ADDR_FLAG_BRACKETED_LITERALS' -- means that square
                brackets should be placed around IP literals whenever they
                are produced by the underlying `getnameinfo' system call.
           [>>] `KDCS_ADDR_FLAG_ESCAPED_NAMES' -- means that hostnames
                produced by a lookup operation should be subjected to hex-hex
                encoding to a URI-legal character set.  Note that the use of
                `KDCS_ADDR_FLAG_BRACKETED_LITERALS' together with this flag
                ensures that the generated string can be used as the host
                component of a URI, in conformance with RFC 3986.
      */
    bool first()
      { 
        if (num_addresses == 0) return false;
        active_address = 0;
        return true;
      }
      /* [SYNOPSIS]
           Sets the currently active address to the first address within
           the set established by `init'.  Returns false only if there are
           no addresses available.  Note that this function does not depend
           upon the provision of a valid port via a call to `set_port', even
           if the `KDCS_ADDR_FLAG_NEED_PORT' flag was passed to `init'.
      */
    bool next()
      { 
        if ((active_address < 0) || (active_address >= (num_addresses-1)))
          return false;
        active_address++;
        return true;
      }
      /* [SYNOPSIS]
           Advances to the next active address, if any, within the set
           established by `init'.  If the current active address is the
           last one, the function returns false, leaving the state of the
           object unchanged.  If there are no addresses, the function also
           returns false.  As with `first', this function does not depend
           upon the provision of a valid port via a call to `set_port',
           even if `KDCS_ADDR_FLAG_NEED_PORT' flag was passed to `init'.
      */
    const sockaddr *get_addr() const
      { 
        if ((active_address < 0) || (active_address >= num_addresses))
          return NULL;
        return addresses[active_address];
      }
      /* [SYNOPSIS]
           Returns a pointer to the current address descriptor, the length of
           which is returned by `get_addr_len'.  Returns NULL if there is no
           valid active address.
      */
    size_t get_addr_len() const
      { 
        if ((active_address < 0) || (active_address >= num_addresses))
          return 0;
        return address_lengths[active_address];
      }
      /* [SYNOPSIS]
           Returns the length of the current address descriptor, a pointer to
           which is returned by `get_addr'.  Returns 0 if there is no valid
           active address.
      */
    int get_addr_family() const
      { 
        if ((active_address < 0) || (active_address >= num_addresses))
          return 0;
        return address_families[active_address];
      }
      /* [SYNOPSIS]
           Returns the address family associated with the currently
           active address.  This is either `AF_INET' (IPv4), `AF_INET6' (IPv6),
           or 0 (meaning that there is no valid active address).
      */
  private: // Helper functions
    void set_num_addresses(int num);
      /* Allocates all the arrays and sets `addresses' to point to relevant
         blocks of storage.  This function does nothing if `num_addresses' is
         already equal to the `num' value. */
    void size_string_buf(size_t min_len);
      /* Ensures that the internal `string_buf' array is at least `min_len'
         characters long, deleting any existing string buffer and allocating
         a new one, if necessary. */
  private: // Data
    int num_addresses;
    sockaddr **addresses; // Array of `num_addresses' sockaddr references
    sockaddr *addr_handle; // Memory block containing the referenced addresses
    size_t *address_lengths; // Array of sockaddr lengths;
    int *address_families; // Array of address family identifiers
    size_t max_address_length; // Set by `set_num_addresses'
    int active_address; // Currently active entry in above arrays
    bool port_valid;
    size_t string_buf_len; // Size of `string_buf' array
    char *string_buf; // For textualized addresses
  };

/*****************************************************************************/
/*                                kdcs_channel                               */
/*****************************************************************************/

class kdcs_channel {
  /* [SYNOPSIS]
       Note that access to this object is not inherently thread safe, even
     though the `kdcs_channel_monitor' object is thread safe.  In
     multi-threaded applications, you are expected to provide some kind of
     protection against simultaneous access to the object from multiple
     threads.
  */
  public:
    kdcs_channel(kdcs_channel_monitor *monitor)
      {
        kdcs_start_network();
        this->monitor = monitor;
        socket = NULL; socket_connected = false;
        channel_ref = NULL; servicer = NULL;
        suppress_errors = false; error_exception = KDCS_CLOSED_EXCEPTION;
      }
    virtual ~kdcs_channel() { close(); }
    KDU_AUX_EXPORT virtual void
      set_channel_servicer(kdcs_channel_servicer *servicer);
      /* [SYNOPSIS]
           Use this to change the channel servicing object.  It may be
           changed to or from NULL if you like.  The function often
           needs to be overridden in derived objects to take care of
           additional implications of a change in the servicing object.
           [//]
           Note that changing the channel servicing object generally
           removes any scheduled wakeups and installed I/O conditions.
      */
    virtual bool is_active() { return (socket != NULL); }
      /* [SYNOPSIS]
           Returns true if the internal socket has been created and not
           yet destroyed.  This means that a call to `connect' has been issued,
           even if the success of the call is still pending.  A call to
           `close' or a failed call to `connect' (one which discovers the
           address to be unreachable) will result in this function returning
           false.
      */
    virtual bool is_connected() { return socket_connected; }
      /* [SYNOPSIS]
           Returns true if the internal socket exists and has been connected
           to the network (i.e., if a call to `connect' has succeeded and there
           has been no subsequent call to `close').  This function does not
           test the channel to see if the other party has closed the
           connection; however, if a previous read or write operation failed
           due to premature termination of the connection, the present
           function will return false.
      */
    KDU_AUX_EXPORT virtual void close();
      /* [SYNOPSIS]
           Closes any open connection and releases the socket, but does
           not destroy other internal resources which might be used if the
           object is re-opened.  It is safe to call this function at any time,
           even if no connection is open.
      */
    KDU_AUX_EXPORT bool get_peer_address(kdcs_sockaddr &address);
      /* [SYNOPSIS]
           Returns with `address' filled out to reflect all relevant details
           of the peer socket to which the current channel is connected.
           Returns false if the channel is not associated with a connected
           socket, after calling `address.reset'.
      */
    KDU_AUX_EXPORT bool get_local_address(kdcs_sockaddr &address);
      /* [SYNOPSIS]
           Returns with `address' filled out to reflect all relevant details
           of the local socket to which the current channel is connected.
           Returns false if the channel is not associated with a
           connected socket, after calling `address.reset'.
      */
    void set_error_mode(bool suppress, kdu_exception exception_val)
      { this->suppress_errors=suppress;  this->error_exception=exception_val; }
      /* [SYNOPSIS]
           This function has a persistent effect on the internal error
           behaviour.
           [//]
           The default error behaviour set by `close' and upon construction
           is to deliver connection-related errors to `kdu_error'; when
           error messages are suppressed, however, an exception of type
           `kdu_exception' is thrown instead, with the value supplied by
           `exception_val'.
       */
  protected: // Data
    friend class kdcs_channel_monitor; // Needs to access `sock' member
    kdcs_socket *socket; // Dynamically allocated so we don't need size here
    bool socket_connected;
    bool suppress_errors;
    kdu_exception error_exception;
    kdcs_channel_ref *channel_ref;
    kdcs_channel_monitor *monitor;
    kdcs_channel_servicer *servicer;
};

/*****************************************************************************/
/*                             kdcs_tcp_channel                              */
/*****************************************************************************/

class kdcs_tcp_channel : public kdcs_channel {
  /* [SYNOPSIS]
       This powerful object provides all the mechanisms you need to implement
       socket communications in a wide variety of different contexts.
       [>>] In the most general (yet most complex case), all I/O calls
            (connect, accept, read and write) are inherently non-blocking,
            but failed calls register callbacks for the events of interest
            via a `kdcs_channel_servicer' object.  In this case, your
            application supplies a `kdcs_channel_monitor' object which manages
            scheduled wakeups and event monitoring for a potentially very
            large number of channels, typically running a separate event
            monitoring thread which calls `kdcs_channel_monitor::run_once' in
            a loop.  If you choose to use the object in this non-blocking
            fashion, the most efficient thing to do is to arrange for your
            individual `kdcs_channel_servicer::service_channel' functions
            to perform outstanding I/O immediately when they are invoked,
            so that they can immediately re-register still pending I/O
            conditions with the `kdcs_channel_monitor' object.  The
            registration of I/O conditions itself occurs automatically
            whenever an I/O call fails to complete immediately.
       [>>] A much simpler way to interact with the object is to make all
            I/O calls block the caller until they complete (or fail
            permanently).  This model is realized if you do not supply a
            `kdcs_channel_servicer' object in the initial call to `connect',
            `listen' or `accept'.  In this case, the object implements its
            own internal `kdcs_channel_servicer', which wakes up any blocked
            I/O call to retry the operation.  With this model, you still
            provide your own external `kdcs_channel_monitor' object, but you
            have the choice of executing its `kdcs_channel_monitor::run_once'
            function in a separate event monitoring thread, or directly from
            within the blocked I/O call.  The latter approach allows for
            completely single-threaded implementations.  In either case, your
            application can have other channels which execute non-blocking
            I/O calls and share the same `kdcs_channel_monitor' object; their
            their `kdcs_channel_servicer' objects will be invoked from within
            the `kdcs_channel_monitor::run_once' function is appropriate.
       [//]
       With the wide array of options outlined above, it is a relatively
       simple matter to robustly implement client/server applications using
       a variety of programming models.  For server applications, there are
       likely to be a large number of sockets and threads and the non-blocking
       approach with immediate execution of I/O calls from within
       `kdcs_channel_servicer::service_channel' is likely to be the most
       efficient strategy.
       [//]
       For client applications with only one channel, the blocking approach
       with in-thread execution of the `kdcs_channel_monitor::run_once'
       function is simplest and entirely appropriate.  In a more complex
       client application, with multiple channels, the blocking approach
       with a single thread of execution, may still be appropriate if there
       is a single primary channel that you can readily identify to run
       the `kdcs_channel_monitor::run_once' function (on behalf of all
       channels) when its I/O calls are blocked.
       [//]
       Note that access to this object is not inherently thread safe, even
       though the `kdcs_channel_monitor' object is thread safe.  In
       multi-threaded applications, you may need to provide some kind of
       protection against simultaneous access to the object's member
       functions from multiple threads.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdcs_tcp_channel(kdcs_channel_monitor *monitor,
                                    bool have_separate_monitor_thread);
      /* [SYNOPSIS]
           Does not actually create a socket.  All the real work of
           establishing a channel that can be used for socket communications
           of one form or another is done by `connect', `listen' and `accept'.
         [ARG: monitor]
           This argument must not be NULL.  All channels need to have a
           `kdcs_channel_monitor' which can be used to unblock blocking I/O
           calls or to serve notification that non-blocking I/O calls should
           be re-invoked.
           [//]
           In most applications, there will be only one `kdcs_channel_monitor'
           object, shared by all communication channels.  However, it is
           possible to create separate monitors if you have a good reason
           for doing this.
         [ARG: have_separate_monitor_thread]
           If true, blocking I/O calls to this object will assume that the
           `kdcs_channel_monitor::run_once' function is being executed from
           a separate channel monitoring thread and not attempt to execute
           the function themselves.  In this case, blocking calls wait upon
           an internal synchronization event that is set by the channel
           monitoring thread through relevant calls to an internal
           channel servicer object's `kdcs_channel_servicer::service_channel'
           function.
           [//]
           If false, blocking I/O calls will invoke the
           `kdcs_channel_monitor::run_once' function themselves, until the
           call can succeed (or an exception is thrown, as documented for
           each individual I/O call below).  In the process of invoking the
           `kdcs_channel_monitor::run_once' function, a blocking call may
           also wind up servicing other non-blocking channels which have
           registered I/O conditions with the same `monitor'.
      */
    KDU_AUX_EXPORT virtual ~kdcs_tcp_channel();
    KDU_AUX_EXPORT virtual void close();
      /* [SYNOPSIS]
           Augments the functionality of `kdcs_channel::close', returning
           the object to a pristine state, as if first constructed.
      */
    KDU_AUX_EXPORT void set_blocking_lifespan(float seconds);
      /* [SYNOPSIS]
           This function may be used to configure the total lifespan of a
           blocking channel, from the time the channel is first connected
           by `connect' or `accept' or put into a listening state by `listen',
           to the time blocking I/O on the channel fails.  Once the lifespan
           expires, I/O functions return immediately, leaving the channel
           closed if it was not already closed.
           [//]
           The lifespan only affects channels which are being used in the
           blocking mode (i.e., without an external `kdcs_channel_servicer').
           It plays no role for non-blocking channels.
           [//]
           The lifespan may be altered at any point.  If the lifespan is
           altered prior to a call to `connect' or `listen', the lifespan
           commences from the time one of those functions is subsequently
           invoked.  If the lifespan is altered after the channel is already
           connected or listening, the lifespan is still interpreted as
           commencing from the point at which communication began (with
           the first call to `connect' or `listen' or when the channel was
           first created by a call to another channel's `accept' function).
      */
    KDU_AUX_EXPORT bool schedule_wakeup(kdu_long earliest, kdu_long latest);
      /* [SYNOPSIS]
           This function may be used only with non-blocking channels which are
           already connected.  It schedules a wakeup call with the indicated
           parameters, by invoking the `kdcs_channel_monitor::schedule_wakeup'
           function.  The call will arrive at the relevant
           `kdcs_channel_servicer::service_channel' function.  To understand
           how `eariest' and `latest' are to be interpreted, consult the
           description of the `kdcs_channel_monitor::schedule_wakeup'
           function.
      */
    KDU_AUX_EXPORT void set_channel_servicer(kdcs_channel_servicer *servicer);
      /* [SYNOPSIS]
           This function may be used at any point after the first call to
           `connect' or `listen', or after the channel is first created by
           another channel's `accept' function.  It may be used to change
           a non-blocking channel to a blocking one (by supplying a NULL
           `servicer' argument) or vice-versa.  It may also be used to
           change the `kdcs_channel_servicer' object used by a current
           non-blocking channel.  In this case, you should note that any
           previously scheduled wakeups or pending I/O conditions will be
           cancelled, so you may need to invoke `schedule_wakeup' or an
           I/O function of interest again.  In practice, this is unlikely to
           prove problematic, since you would normally only change the
           channel servicer to move it from one managing object to another.
      */
    KDU_AUX_EXPORT bool connect(const kdcs_sockaddr &address,
                                kdcs_channel_servicer *servicer);
      /* [SYNOPSIS]
           Creates a socket (if required) and establishes a connection to
           the indicated address.  If the socket cannot be created, the
           function generates an error through `kdu_error' (in most
           applications, this results in the throwing of an exception of type
           `kdu_exception', but you can arrange for an exception to be thrown
           without the error message by using `kdcs_channel::set_error_mode').
           Failure to create the socket may result from an operating system
           imposed limit on the number of open sockets, or a limit imposed
           by the `kdcs_channel_monitor::add_channel' function.
           [//]
           If `servicer' is NULL, the function sets up the channel for
           blocking communications and this function itself will block the
           caller until the connection is established, the connection is
           found to be unreachable, or an exception is thrown (see below).
           [//]
           If `servicer' is non-NULL, the function sets up the channel for
           non-blocking communications and this function itself will not
           block the caller.  In this case, the function returns false if
           the connection cannot be established immediately, but leaves the
           channel active (`kdcs_channel::is_active' reports true) and
           registers the `KDCS_CONDITION_CONNECT' condition with the
           channel monitor, to be passed to the `servicer->service_channel'
           function once the `connect' call should be able to proceed.
         [RETURNS]
           False if the connection attempt failed (in this case, the
           internal socket is also destroyed) or if a non-blocking
           connection attempt cannot be completed at this point (in this
           case, the `kdcs_channel::is_active' function returns true and
           the connection request has been added as a condition to be
           monitored by the `kdcs_channel_monitor' object originally supplied
           to the constructor).
           [//]
           As noted above, you can expect an exception to be thrown (rather
           than a return value) in the event that the underlying socket
           cannot be created.
           [//]
           Otherwise, one of the following exceptions may be thrown (all are
           of type `kdu_exception'):
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is left in the inactive state so that subsequent
                calls to this function start from scratch.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  After this happens, the channel is left in the
                inactive state so that subsequent calls to this function
                start from scratch.
         [ARG: address]
           Note that the `address' object may actually hold more than one
           address, if a call to `kdcs_sockaddr::init' resolved multiple
           possible addresses for a given name.  This is particularly likely
           in the world of IPv6.  The function actually walks through all the
           available addresses, trying each one in turn, using the
           `kdcs_sockaddr::first' and `kdcs_sockaddr::next' functions.  The
           `address' argument itself is left unchanged, but an internal copy
           (`connect_address') is made to keep track of this address walking
           process.
      */
    bool get_connected_address(kdcs_sockaddr &address)
      { 
        if (!socket_connected) return false;
        address = connect_address; return true;
      }
      /* [SYNOPSIS]
           If a call to `connect' has succeeded, this function returns true,
           setting `address' to the connected address.  Otherwise, the
           function returns false, leaving `address' unchanged.  If you
           need to do something with the address used in a successful call
           to `connect', it is a good idea to recover the address using
           this function, rather than blindly re-using the address passed
           to `connect'.  The reason is that each `kdcs_sockaddr' object
           may actually represent a set of possible addresses that were
           obtained via name resolution.  The `address' argument's currently
           active address is set here to reflect the address which was
           actually found to yield a successful connection.
      */
    KDU_AUX_EXPORT bool listen(const kdcs_sockaddr &address, int backlog_limit,
                               kdcs_channel_servicer *servicer);
      /* [SYNOPSIS]
           This function is similar to `connect', except that it configures
           the internal socket to listen for incoming calls from clients,
           binding to the supplied listening `address'.  As for `connect',
           an error will be generated through `kdu_error' (or else an
           exception will be thrown directly if errors have been suppressed
           using `kdcs_channel::set_error_mode') if the socket itself cannot
           be created.  Failure to create the socket may result from an
           operating system imposed limit on the number of open sockets, or a
           limit imposed by the `monitor->add_channel' function.
           [//]
           Unlike `connect', this function does not have to be re-issued
           if it does not succeed.  No conditions need to be queued
           with the `kdcs_channel_monitor' object.  The presence (or absence)
           of a `servicer' object determines whether subsequent calls to
           `accept' will be blocking or non-blocking, but has no impact on
           whether or not the present function blocks the caller (socket
           binding is essentially immediate).
         [RETURNS]
           False if the supplied listening `address' cannot be bound.  In
           this case, the internal socket is destroyed and `is_active'
           will return false -- there is no need in this case to call
           `close', but you can if you like.
           [//]
           As noted above, you can expect an exception to be thrown (rather
           than a return value) in the event that the underlying socket
           cannot be created.
         [ARG: backlog_limit]
           Sizes the listening queue; a value greater than 1 allows multiple
           clients to be queued for `accept' calls.  If the value supplied here
           is less than 1, the value of 1 will be used.
      */
    bool get_listening_address(kdcs_sockaddr &address)
      { 
        if (socket_connected ||
            (listen_address.get_addr()==NULL)) return false;
        address = listen_address; return true;
      }
      /* [SYNOPSIS]
           If a call to `listen' has succeeded, this function returns true,
           setting `address' to the listening address.  Otherwise, the
           function returns false, leaving `address' unchanged.  If you
           need to do something with the address used in a successful call
           to `listen', it is a good idea to recover the address using
           this function, rather than blindly re-using the address passed
           to `listen'.  The reason is that each `kdcs_sockaddr' object
           may actually represent a set of possible addresses that were
           obtained via name resolution.  The `address' argument's currently
           active address is set here to reflect the address which was
           actually found to succeed in setting up the listening state.
      */
    KDU_AUX_EXPORT kdcs_tcp_channel *
      accept(kdcs_channel_monitor *target_monitor,
             kdcs_channel_servicer *target_servicer,
             bool have_separate_target_monitor_thread);
      /* [SYNOPSIS]
           This function is used to accept an incoming call on the present
           channel, which should have been configured using `listen'.  If
           successful, the function returns a newly created `kdcs_tcp_channel'
           object, which is connected for ongoing communications; the
           returned object's `kdcs_channel::is_connected' member will return
           true. It is up to you to delete this channel once you are done
           with it.
           [//]
           If the `servicer' argument supplied to `listen' was NULL, this
           function blocks the caller until a new connection can be accepted
           or an exception occurs (see below).
           [//]
           If the `servicer' argument supplied to `listen;' was non-NULL, this
           function is non-blocking.  If a connection is not already available
           to accept, the function returns NULL, but leaves the
           accept condition queued on the `kdcs_channel_monitor' object which
           was supplied during construction.  Once an incoming connection
           arrives, that object's `kdcs_channel_monitor::run_once' function
           will invoke the `kdcs_channel_servicer::service_channel' member
           of the `kdcs_channel_servicer' object supplied to `listen', which
           should normally re-issue the `accept' call.
           [//]
           If the function succeeds, the returned channel is constructed
           with the supplied `target_monitor' and
           `have_separate_target_monitor_thread' parameters (see the
           `kdcs_tcp_channel' constructor's description for more on these).
           Also, the returned channel is initially assigned the supplied
           `target_servicer' object for servicing non-blocking I/O calls. If
           NULL, the returned object is initially configured for blocking I/O
           but you can change this later by invoking `set_channel_servicer'.
         [RETURNS]
           NULL if the channel is non-blocking and no incoming call
           can be accepted at this time.  You can generally re-invoke the
           function at a later time.
           [//]
           This function may generate an error via `kdu_error' if it is
           not possible to create a new socket or to add it to the
           `target_monitor' object for monitoring (e.g., a socket/channel
           limit is exceeded).  As with the other I/O functions, an
           exception might be thrown immediately, rather than calling
           `kdu_error', if the present object's `kdcs_channel::set_error_mode'
           function has been used to suppress errors.
           [//]
           Otherwise, one of the following exceptions (all of type
           `kdu_exception') may be thrown:
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is not automatically closed, however, so it is
                possible to change the lifespan and re-initiate I/O if you
                so-desire.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  Before throwing this exception, the `close' function
                is called for you.  As a result, further calls to the function
                will throw the `KDCS_CLOSED_EXCEPTION' instead.
           [>>] `KDCS_CLOSED_EXCEPTION' means that the channel has been
                closed, either from this end or from the other end.  Before
                throwing this exception, the `close' function is called for
                you if required.
      */
    KDU_AUX_EXPORT const char *
      read_line(bool accumulate=false, char delim='\n');
      /* [SYNOPSIS]
           Accumulates text characters from the TCP channel until either the
           `delim' character or the null character, `\0', is encountered.
           [//]
           If no channel servicer was supplied to the relevant `connect' or
           `accept' call (or if `set_channel_servicer' has since been called
           with a NULL argument), this function blocks the caller until the
           request can be completed or an exception occurs (see below).
           [//]
           If the channel has an externally supplied `kdcs_channel_servicer'
           object, the call is non-blocking.  If insufficient data is
           available to complete the request, the function returns NULL,
           registering the read condition with the `kdcs_channel_monitor'
           object supplied during construction, using the
           `kdcs_channel_monitor::queue_conditions' function.
           [//]
           If successful, the function returns a null-terminated character
           string which terminates with the `delim' character if it was found.
           The returned character string has all superfluous whitespace
           characters (' ', '\t', '\r', '\n') removed.  This includes all
           leading whitespace, all trailing white space and all but one
           space character between words (whitespace-delimited tokens).
           '\t' or '\r' characters between words are converted to ' '
           characters.
           [//]
           If `accumulate' is false, this function automatically discards
           any text which was returned by a previous successful call to
           `read_line' or `read_paragraph', before commencing the new call.
           Otherwise, the function appends the new line of text to previously
           accumulated text, returning the multi-line response once a new
           complete line has become available, or NULL if the request cannot
           be completed without blocking.
           [//]
           If the call cannot proceed, either now or in the future, an
           exception (of type `kdu_exception') is thrown.  The exceptions
           are as follows:
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is not automatically closed, however, so it is
                possible to change the lifespan and re-initiate I/O if you
                so-desire.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  Before throwing this exception, the `close' function
                is called for you.  As a result, further calls to the function
                will throw the `KDCS_CLOSED_EXCEPTION' instead.
           [>>] `KDCS_CLOSED_EXCEPTION' means that the channel has been
                closed, either from this end or from the other end.  Before
                throwing this exception, the `close' function is called for
                you if required.
      */
    KDU_AUX_EXPORT const char *read_paragraph(char delim='\n');
      /* [SYNOPSIS]
           Essentially calls `read_line' until two consecutive occurrences of
           the `delim' character are encountered, or else one occurrence of
           the `delim' character occurs immediately, before any
           non-whitespace character is encountered, or '\0' is encountered.
           In both cases, the returned string is null-terminated.  The
           returned string will conclude with all recovered delimiters.
           [//]
           If no channel servicer was supplied to the relevant `connect' or
           `accept' call (or if `set_channel_servicer' has since been called
           with a NULL argument), this function blocks the caller until the
           request can be completed or an exception occurs (see below).
           [//]
           If the channel has an externally supplied `kdcs_channel_servicer'
           object, the call is non-blocking.  If insufficient data is
           available to complete the request, the function returns NULL,
           registering the read condition with the `kdcs_channel_monitor'
           object supplied during construction, using the
           `kdcs_channel_monitor::queue_conditions' function.
           [//]
           If the call cannot proceed, either now or in the future, an
           exception (of type `kdu_exception') is thrown.  The exceptions
           are as follows:
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is not automatically closed, however, so it is
                possible to change the lifespan and re-initiate I/O if you
                so-desire.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  Before throwing this exception, the `close' function
                is called for you.  As a result, further calls to the function
                will throw the `KDCS_CLOSED_EXCEPTION' instead.
           [>>] `KDCS_CLOSED_EXCEPTION' means that the channel has been
                closed, either from this end or from the other end.  Before
                throwing this exception, the `close' function is called for
                you if required.
      */
    KDU_AUX_EXPORT kdu_byte *read_raw(int num_bytes);
      /* [SYNOPSIS]
           Reads the indicated number of bytes from the TCP channel, returning
           a pointer to an internal buffer which holds the requested data.
           [//]
           If no channel servicer was supplied to the relevant `connect' or
           `accept' call (or if `set_channel_servicer' has since been called
           with a NULL argument), this function blocks the caller until the
           request can be completed or an exception occurs (see below).
           [//]
           If the channel has an externally supplied `kdcs_channel_servicer'
           object, the call is non-blocking.  If insufficient data is
           available to complete the request, the function returns NULL,
           registering the read condition with the `kdcs_channel_monitor'
           object supplied during construction, using the
           `kdcs_channel_monitor::queue_conditions' function.
           [//]
           If the call cannot proceed, either now or in the future, an
           exception (of type `kdu_exception') is thrown.  The exceptions
           are as follows:
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is not automatically closed, however, so it is
                possible to change the lifespan and re-initiate I/O if you
                so-desire.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  Before throwing this exception, the `close' function
                is called for you.  As a result, further calls to the function
                will throw the `KDCS_CLOSED_EXCEPTION' instead.
           [>>] `KDCS_CLOSED_EXCEPTION' means that the channel has been
                closed, either from this end or from the other end.  Before
                throwing this exception, the `close' function is called for
                you if required.
      */
    KDU_AUX_EXPORT bool read_block(int num_bytes, kdcs_message_block &block);
      /* [SYNOPSIS]
           Equivalent to concatenating `read_raw' with `block.write_raw', but
           generally more efficient.
           [//]
           If no channel servicer was supplied to the relevant `connect' or
           `accept' call (or if `set_channel_servicer' has since been called
           with a NULL argument), this function blocks the caller until the
           request can be completed or an exception occurs (see below).
           [//]
           If the channel has an externally supplied `kdcs_channel_servicer'
           object, the call is non-blocking.  If insufficient data is
           available to complete the request, the function returns false,
           registering the read condition with the `kdcs_channel_monitor'
           object supplied during construction, using the
           `kdcs_channel_monitor::queue_conditions' function.
           [//]
           If the call cannot proceed, either now or in the future, an
           exception (of type `kdu_exception') is thrown.  The exceptions
           are as follows:
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is not automatically closed, however, so it is
                possible to change the lifespan and re-initiate I/O if you
                so-desire.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  Before throwing this exception, the `close' function
                is called for you.  As a result, further calls to the function
                will throw the `KDCS_CLOSED_EXCEPTION' instead.
           [>>] `KDCS_CLOSED_EXCEPTION' means that the channel has been
                closed, either from this end or from the other end.  Before
                throwing this exception, the `close' function is called for
                you if required.
      */
    KDU_AUX_EXPORT bool write_raw(const kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Writes `num_bytes' from the supplied buffer to the TCP channel.
           [//]
           If no channel servicer was supplied to the relevant `connect' or
           `accept' call (or if `set_channel_servicer' has since been called
           with a NULL argument), this function blocks the caller until the
           request can be completed or an exception occurs (see below).
           [//]
           If the channel has an externally supplied `kdcs_channel_servicer'
           object, the call is non-blocking.  If insufficient space is
           available on the outgoing TCP queue to complete the request, the
           function return false, registering the write condition with the
           `kdcs_channel_monitor' object supplied during construction, using
           the `kdcs_channel_monitor::queue_conditions' function.  Subsequent
           calls to this function should provide identical values for the
           `buf' and `num_bytes' arguments until the call succeeds.
           [//]
           If the call cannot proceed, either now or in the future, an
           exception (of type `kdu_exception') is thrown.  The exceptions
           are as follows:
           [>>] `KDCS_LIFESPAN_EXCEPTION' means that the lifespan has expired
                during a blocking call (see `set_blocking_lifespan').  The
                channel is not automatically closed, however, so it is
                possible to change the lifespan and re-initiate I/O if you
                so-desire.
           [>>] `KDCS_CLOSING_EXCEPTION' means that the
                `kdcs_channel_monitor::request_closure' function has been
                called.  Before throwing this exception, the `close' function
                is called for you.  As a result, further calls to the function
                will throw the `KDCS_CLOSED_EXCEPTION' instead.
           [>>] `KDCS_CLOSED_EXCEPTION' means that the channel has been
                closed, either from this end or from the other end.  Before
                throwing this exception, the `close' function is called for
                you if required.
      */
    bool write_block(kdcs_message_block &block)
      {
        int num_bytes = block.get_remaining_bytes();
        return write_raw(block.peek_block(),num_bytes);
      }
      /* [SYNOPSIS]
           Writes the entire contents of `block', starting from the position
           of its current read pointer, but otherwise identical to
           `write_raw'.  Does not alter the `block's read pointer though.
      */
  protected: // Internal definitions
    class kdcs_private_tcp_servicer : public kdcs_channel_servicer {
      protected:
        virtual ~kdcs_private_tcp_servicer()
          { mutex.destroy();  event.destroy(); }
      public: // Member functions
        kdcs_private_tcp_servicer(kdcs_channel_monitor *monitor)
          { // Supply NULL if the channel monitor is run in a separate thread
            timer_expired = false; 
            if ((this->monitor = monitor) == NULL)
              { mutex.create();  event.create(true); }
          }
        void service_channel(kdcs_channel_monitor *monitor,
                             kdcs_channel *channel, int cond_flags)
          {
            if (this->monitor == NULL) mutex.lock();
            if (cond_flags & KDCS_CONDITION_WAKEUP)
              timer_expired = true;
            if (this->monitor == NULL)
              { event.set(); mutex.unlock(); }
          }
        bool wait_for_service()
          { // Returns false only if the lifespan expires
            if (this->monitor == NULL)
              { mutex.lock();  event.wait(mutex); }
            else
              this->monitor->run_once(1000000,0);
                // waits at most 1 second (but the value should not matter)
            bool result = !timer_expired;
            timer_expired = false;
            if (this->monitor == NULL) mutex.unlock();
            return result;
          }
        void release() // Call this when done, instead of `release_ref'
          { release_ref(); }
      private: // Data
        bool timer_expired;
        kdu_mutex mutex;
        kdu_event event;
        kdcs_channel_monitor *monitor; // NULL if separate monitor thread
    };
  protected: // Data
    kdcs_sockaddr connect_address; // Valid if connected or connecting
    kdcs_sockaddr listen_address; // Valid if listening
    kdcs_private_tcp_servicer *internal_servicer; // Used for blocking I/O mode
    bool have_separate_monitor_thread; // Relevant only for blocking I/O
    kdu_long start_time; // Set inside `connect', `listen' and `accept'
    kdu_long blocking_lifespan; // Measured in microseconds
    bool lifespan_expired;
    bool connect_call_has_valid_args; // True if a previous call to `::connect'
      // with the current address and socket gave `kdcs_check_wouldblock' true.
  
    kdu_byte tbuf[256]; // Temporary holding buffer for reading
    int tbuf_bytes; // Total read bytes currently stored in `tbuf'
    int tbuf_used; // Number of initial bytes from `tbuf' already used
    char *text; // Buffer used for accumulating parsed ASCII text
    int text_len; // Number of characters accumulated in `text'
    int text_max; // Max chars allowed in `text', before reallocation needed
    bool text_complete; // True if `text' was returned exactly as is in a
              // previous successful call to `read_text' or `read_paragraph'.
    bool skip_white; // True if white space is to be skipped
    bool line_start; // True if we have not yet found a non-white character
    kdu_byte *raw; // Buffer used for accumulating raw binary data
    int raw_len; // Number of bytes accumulated in `raw'
    int raw_max; // Max bytes allowed in `raw', before reallocation needed
    bool raw_complete; // True if `raw' was returned exactly as is in a
              // previous successful call to `read_raw'.
    int block_len;  // Used by `read_block'.
    int partial_bytes_sent; // Total bytes sent after incomplete non-blocking
                            // call to `write_raw'.
  };

#endif // KDCS_COMMS_H
