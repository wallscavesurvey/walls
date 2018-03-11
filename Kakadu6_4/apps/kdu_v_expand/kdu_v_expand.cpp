/*****************************************************************************/
// File: kdu_v_expand.cpp [scope = APPS/V_DECOMPRESSOR]
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
   File-based Motion JPEG2000 decompressor application.
******************************************************************************/

#include <string.h>
#include <stdio.h> // so we can use `sscanf' for arg parsing.
#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
// Kakadu core includes
#include "kdu_arch.h"
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
// Application includes
#include "kdu_args.h"
#include "kdu_video_io.h"
#include "mj2.h"
#include "v_expand_local.h"

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
/* STATIC                        print_version                               */
/*****************************************************************************/

static void
  print_version()
{
  kdu_message_formatter out(&cout_message);
  out.start_message();
  out << "This is Kakadu's \"kdu_v_expand\" application.\n";
  out << "\tCompiled against the Kakadu core system, version "
      << KDU_CORE_VERSION << "\n";
  out << "\tCurrent core system version is "
      << kdu_get_core_version() << "\n";
  out.flush(true);
  exit(0);
}

/*****************************************************************************/
/* STATIC                        print_usage                                 */
/*****************************************************************************/

static void
  print_usage(char *prog, bool comprehensive=false)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage:\n  \"" << prog << " ...\n";
  out.set_master_indent(3);
  out << "-i <MJ2 or raw compressed input file>\n";
  if (comprehensive)
    out << "\tTwo types of compressed video files may be supplied.  If the "
           "file has the suffix, \".mj2\", it is taken to be a Motion "
           "JPEG2000 file (as defined by ISO/IEC 15444-3).  Otherwise "
           "the suffix must be \".mjc\" and the input file is taken to "
           "consist of a 16-byte header, followed by the video image "
           "code-streams, each preceded by a four byte big-endian length "
           "field (length of the code-stream, not including the length "
           "field itself).  For more information on the 16-byte header, "
           "consult the usage statements produced by the \"kdu_v_compress\" "
           "application.\n";
  out << "-o <vix file>\n";
  if (comprehensive)
    out << "\tTo avoid over complicating this simple demonstration "
           "application, decompressed video is written as a VIX file.  VIX "
           "is a trivial non-standard video file format, consisting of a "
           "plain ASCI text header, followed by raw binary data.  A "
           "description of the VIX format is embedded in the usage "
           "statements printed by the \"kdu_v_compress\" application.  It "
           "is legal to leave this argument out, the principle purpose of "
           "which would be to time the decompression processing (using "
           "`-cpu'), without cluttering the statistics with file I/O "
           "issues.\n";
  out << "-frames <max frames to process>\n";
  if (comprehensive)
    out << "\tBy default, all available frames are decompressed.  This "
           "argument may be used to limit the number of frames which are "
           "actually written to the output VIX file.\n";
  out << "-layers <max layers to decode>\n";
  if (comprehensive)
    out << "\tSet an upper bound on the number of quality layers to actually "
           "decode.\n";
  out << "-components <max image components to decode>\n";
  if (comprehensive)
    out << "\tSet the maximum number of image components to decode from each "
           "frame.  This would typically be used to decode luminance only "
           "from a colour video.\n";
  out << "-reduce <discard levels>\n";
  if (comprehensive)
    out << "\tSet the number of highest resolution levels to be discarded.  "
           "The frame resolution is effectively divided by 2 to the power of "
           "the number of discarded levels.\n";
  out << "-int_region {<top>,<left>},{<height>,<width>}\n";
  if (comprehensive)
    out << "\tEstablish a region of interest within the original compressed "
           "frames.  Only the region of interest will be decompressed and the "
           "output frame dimensions will be modified accordingly.  The "
           "coordinates of the top-left corner of the region are given first, "
           "separated by a comma and enclosed in curly braces, after which "
           "the dimensions of the region are given in similar fashion.  The "
           "two coordinate pairs must be separated by a comma, with no "
           "intervening spaces.  All coordinates and dimensions are expressed "
           "as integer numbers of pixels for the first image component to "
           "be decompressed, taking into account any resolution adjustments "
           "associated with the `-reduce' argument.  The location of the "
           "region is expressed relative to the upper left hand corner of the "
           "relevant image component, at the relevant resolution.  If any "
           "part of the specified region does not intersect with the image, "
           "the decompressed region will be reduced accordingly.  Note that "
           "the `-region' argument offered by the \"kdu_expand\" application "
           "is similar, except that it accepts normalized region coordinates, "
           "in the range 0 to 1.\n";
  out << "-num_threads <0, or number of parallel threads to use>\n";
  if (comprehensive)
    out << "\tUse this argument to gain explicit control over "
           "multi-threaded or single-threaded processing configurations.  "
           "The special value of 0 may be used to specify that you want "
           "to use the conventional single-threaded processing "
           "machinery -- i.e., you don't want to create or use a "
           "threading environment.  Otherwise, you must supply a "
           "positive integer here, and the object will attempt to create "
           "a threading environment with that number of concurrent "
           "processing threads.  The actual number of created threads "
           "may be smaller than the number requested, if your "
           "request exceeds internal resource limits.  It is worth "
           "noting that \"-num_threads 1\" and \"-num_threads 0\" "
           "both result in single-threaded processing, although the "
           "former creates an explicit threading environment and uses "
           "it to schedule the processing steps, even if there is only "
           "one actual thread of execution.  This incurs some unnecessary "
           "overhead.  Note also that the multi-threading behaviour can "
           "be further customized using the `-double_buffering' and "
           "`-overlapped_frames' arguments.\n";
  out << "-double_buffering <stripe height>\n";
  if (comprehensive)
    out << "\tThis option is intended to be used in conjunction with "
           "`-num_threads'.  If `-double_buffering' is not specified, the "
           "DWT operations will all be performed by the "
           "single thread which \"owns\" the multi-threaded processing "
           "group.  For a small number of processors, this may be acceptable, "
           "since the DWT is generally quite a bit less CPU intensive than "
           "block decoding (which is always spread across multiple threads, "
           "if available).  However, even for a small number of threads, the "
           "amount of thread idle time can be reduced by specifying the "
           "`-double_buffering' option.  In this case, only a small number "
           "of image rows in each image component are actually double "
           "buffered, so that one set can be processed by colour "
           "transformation and sample writing operations, while the other "
           "set is generated by the DWT synthesis engines, which themselves "
           "feed off the block decoding engines.  The number of rows in "
           "each component which are to be double buffered is known as the "
           "\"stripe height\", supplied as a parameter to this argument.  The "
           "stripe height can be as small as 1, but this may add quite a bit "
           "of thread context switching overhead.  For this reason, a stripe "
           "height in the range 4 to 16 is recommended.  If you are working "
           "with small horizontal tiles, you may find that an even larger "
           "stripe height is required for maximum throughput.  In the "
           "extreme case of very small tile widths, you may find that the "
           "`-double_buffering' option hurts throughput.  In any case, the "
           "message is that for maximum throughput on a multi-processor "
           "platform, you should be prepared to play with both the "
           "`-num_threads' and `-double_buffering' options.\n";
  out << "-overlapped_frames -- allow overlapped processing of frames\n";
  if (comprehensive)
    out << "\tThis option is intended to be used in conjunction with "
           "`-num_threads'.  In multi-processor environments, with at least "
           "one thread per processor, you would hope to get a speedup in "
           "proportion to the number of available processors.  One reason "
           "this is not quite realized is that as each frame nears "
           "completion, the number of available jobs falls below the "
           "number of threads.  Kakadu's multi-threading environment "
           "provides an asynchronous mechanism for detecting the point "
           "beyond which the thread pool is likely to remain under-utilized.  "
           "The present argument allows you to exploit this feature, by "
           "arranging for processing of the next frame to commence once "
           "under-utilization is detected.  This can be done in many ways, "
           "but the current implementation simply opens the next frame's "
           "codestream ahead of time and starts the `kdu_multi_synthesis' "
           "processing engine in a dormant queue.  Once under-utilization "
           "is detected, dormant queues are automatically activated in "
           "the background by the internal system, so high utilization "
           "should be achievable.  One potential drawback of this "
           "potential strategy is that opening the next codestream "
           "requires some disk I/O, which may temporarily block disk "
           "accesses associated with the currently active frame's "
           "codestream.  You can play around with this command-line "
           "argument to determine whether the cost of any such blocking "
           "is outweighed by the benefits of higher thread utilization.  "
           "By default, codestreams are processed sequentially, without "
           "any overlapping.  You should also note that overlapped "
           "processing can be used only with MJ2 files, since the "
           "underlying `mj2_source' object provides full support for "
           "concurrent access to multiple frames.  If you attempt to use "
           "this option with the simplistic MJC format, an appropriate "
           "error will be generated.\n";
  out << "-in_memory <number of times to repeat each frame>\n";
  if (comprehensive)
    out << "\tThis option is provided mainly for examining the maximum "
           "speed of the underlying core decompression system.  It causes "
           "each frame's decompressed representation to be first loaded "
           "into memory and then decompressed directly from memory.  If "
           "the supplied repetition factor is greater than 1, each frame's "
           "compressed representation is compressed the indicated "
           "number of times, although only the last copy is actually "
           "written to the output file, if one is supplied.  This enables "
           "you to explore the cost of disk reading and factor it out of "
           "overall timing figures.  To improve the readability of the "
           "implementation, the \"-in_memory\" option is supported only in "
           "conjunction with \"-overlapped_frames\".\n";
  out << "-cpu -- collect CPU time statistics.\n";
  if (comprehensive)
    out << "\tThis option cannot be used with \"-overlapped_frames\".\n";
  out << "-s <switch file>\n";
  if (comprehensive)
    out << "\tSwitch to reading arguments from a file.  In the file, argument "
           "strings are separated by whitespace characters, including spaces, "
           "tabs and new-line characters.  Comments may be included by "
           "introducing a `#' or a `%' character, either of which causes "
           "the remainder of the line to be discarded.  Any number of "
           "\"-s\" argument switch commands may be included on the command "
           "line.\n";
  out << "-quiet -- suppress informative messages.\n";
  out << "-version -- print core system version I was compiled against.\n";
  out << "-v -- abbreviation of `-version'\n";
  out << "-usage -- print a comprehensive usage statement.\n";
  out << "-u -- print a brief usage statement.\"\n\n";

  out.flush();
  exit(0);
}

/*****************************************************************************/
/* STATIC                     parse_simple_args                              */
/*****************************************************************************/

static void
  parse_simple_arguments(kdu_args &args, char * &ifname, char * &ofname,
                         int &max_frames, int &max_layers,
                         int &max_components, int &discard_levels,
                         kdu_dims &region, int &num_threads,
                         int &double_buffering_height, bool &overlapped_frames,
                         int &in_memory_repetitions, bool &cpu, bool &quiet)
  /* Parses most simple arguments (those involving a dash). */
{
  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  max_frames = INT_MAX;
  num_threads = 0; // Not actually the default value -- see below
  ofname = NULL;
  max_layers = 0;
  max_components = 0;
  discard_levels = 0;
  region.size = region.pos = kdu_coords(0,0);
  double_buffering_height = 0; // i.e., no double buffering
  overlapped_frames = false;
  in_memory_repetitions = 0;
  cpu = false;
  quiet = false;

  if (args.find("-i") != NULL)
    {
      char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-i\" argument requires a file name!"; }
      ifname = new char[strlen(string)+1];
      strcpy(ifname,string);
      args.advance();
    }
  else
    { kdu_error e; e << "You must supply an input file name."; }

  if (args.find("-o") != NULL)
    {
      char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-o\" argument requires a file name!"; }
      ofname = new char[strlen(string)+1];
      strcpy(ofname,string);
      args.advance();
    }

  if (args.find("-frames") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_frames) != 1) ||
          (max_frames <= 0))
        { kdu_error e; e << "The `-frames' argument requires a positive "
          "integer parameter."; }
      args.advance();
    }

  if (args.find("-layers") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_layers) != 1) ||
          (max_layers < 1))
        { kdu_error e; e << "\"-layers\" argument requires a positive "
          "integer parameter!"; }
      args.advance();
    }

  if (args.find("-components") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_components) != 1) ||
          (max_components < 1))
        { kdu_error e; e << "\"-components\" argument requires a positive "
          "integer parameter!"; }
      args.advance();
    }

  if (args.find("-reduce") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&discard_levels) != 1) ||
          (discard_levels < 0))
        { kdu_error e; e << "\"-reduce\" argument requires a non-negative "
          "integer parameter!"; }
      args.advance();
    }

  if (args.find("-int_region") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"{%d,%d},{%d,%d}",
                  &region.pos.y,&region.pos.x,&region.size.y,&region.size.x)
           != 4) ||
          (region.pos.x < 0) || (region.pos.y < 0) ||
          (region.size.x <= 0) || (region.size.y <= 0))
        { kdu_error e; e << "\"-int_region\" argument requires a set of four "
          "coordinates of the form, \"{<top>,<left>},{<height>,<width>}\", "
          "where `top' and `left' must be non-negative integers, and "
          "`height' and `width' must be positive integers."; }
      args.advance();
    }

  if (args.find("-num_threads") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&num_threads) != 1) ||
          (num_threads < 0))
        { kdu_error e; e << "\"-num_threads\" argument requires a "
          "non-negative integer."; }
      args.advance();
    }
  else if ((num_threads = kdu_get_num_processors()) < 2)
    num_threads = 0;

  if (args.find("-double_buffering") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&double_buffering_height) != 1) ||
          (double_buffering_height < 1))
        { kdu_error e; e << "\"-double_buffering\" argument requires a "
          "positive integer, specifying the number of rows from each "
          "component which are to be double buffered."; }
      args.advance();
    }

  if (args.find("-overlapped_frames") != NULL)
    {
      overlapped_frames = true;
      args.advance();
    }

  if (args.find("-in_memory") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&in_memory_repetitions) != 1) ||
          (in_memory_repetitions < 1))
        { kdu_error e; e << "\"-in_memory\" argument requires a "
          "strictly positive integer."; }
      if (!overlapped_frames)
        { kdu_error e; e << "\"-in_memory\" argument may be used only in "
          "conjunction with \"-overlapped_frames\"."; }
      args.advance();
    }

  if (args.find("-cpu") != NULL)
    {
      cpu = true;
      args.advance();
    }

  if (args.find("-quiet") != NULL)
    {
      quiet = true;
      args.advance();
    }
}

/*****************************************************************************/
/* STATIC                      check_mj2_suffix                              */
/*****************************************************************************/

static bool
  check_mj2_suffix(char *fname)
  /* Returns true if the file-name has the suffix, ".mj2" or ".mjp2", where the
     check is case insensitive. */
{
  const char *cp = strrchr(fname,'.');
  if (cp == NULL)
    return false;
  cp++;
  if ((*cp != 'm') && (*cp != 'M'))
    return false;
  cp++;
  if ((*cp != 'j') && (*cp != 'J'))
    return false;
  cp++;
  if (*cp == '2')
    return true;
  if ((*cp != 'p') && (*cp != 'P'))
    return false;
  cp++;
  if (*cp != '2')
    return false;
  return true;
}

/*****************************************************************************/
/* STATIC                      check_mjc_suffix                              */
/*****************************************************************************/

static bool
  check_mjc_suffix(char *fname)
  /* Returns true if the file-name has the suffix, ".mjc", where the
     check is case insensitive. */
{
  const char *cp = strrchr(fname,'.');
  if (cp == NULL)
    return false;
  cp++;
  if ((*cp != 'm') && (*cp != 'M'))
    return false;
  cp++;
  if ((*cp != 'j') && (*cp != 'J'))
    return false;
  cp++;
  if ((*cp != 'c') && (*cp != 'C'))
    return false;
  return true;
}

/*****************************************************************************/
/* STATIC                        open_vix_file                               */
/*****************************************************************************/

static FILE *
  open_vix_file(char *fname, kdu_codestream codestream, int discard_levels,
                int num_frames, kdu_uint32 timescale, int frame_period,
                bool is_ycc, int &precision, bool &is_signed)
{
  FILE *file;
  if ((file = fopen(fname,"wb")) == NULL)
    { kdu_error e; e << "Unable to open VIX file, \"" << fname << "\", for "
      "writing.  File may be write-protected."; }
  fprintf(file,"vix\n");
  if (timescale == 0)
    timescale = frame_period = 1;
  else if (frame_period == 0)
    frame_period = timescale;
  fprintf(file,">VIDEO<\n%f %d\n",
          ((double) timescale) / ((double) frame_period),
          num_frames);
  fprintf(file,">COLOUR<\n%s\n",(is_ycc)?"YCbCr":"RGB");

  is_signed = codestream.get_signed(0);
  precision = codestream.get_bit_depth(0);
  const char *container_string = "char";
  if (precision > 8)
    container_string = "word";
  if (precision > 16)
    container_string = "dword";
  int is_big = 1; ((char *)(&is_big))[0] = 0;
  const char *endian_string = (is_big)?"big-endian":"little-endian";
  int components = codestream.get_num_components();
  
  kdu_dims dims; codestream.get_dims(-1,dims);
  kdu_coords min=dims.pos, lim=dims.pos+dims.size;
  min.x = (min.x+1)>>discard_levels;  min.y = (min.y+1)>>discard_levels;
  lim.x = (lim.x+1)>>discard_levels;  lim.y = (lim.y+1)>>discard_levels;
  dims.pos = min; dims.size = lim - min;

  fprintf(file,">IMAGE<\n%s %s %d %s\n",
          ((is_signed)?"signed":"unsigned"),
          container_string,precision,endian_string);
  fprintf(file,"%d %d %d\n",dims.size.x,dims.size.y,components);
  for (int c=0; c < components; c++)
    {
      kdu_coords subs; codestream.get_subsampling(c,subs);
      subs.x >>= discard_levels;  subs.y >>= discard_levels;
      fprintf(file,"%d %d\n",subs.x,subs.y);
    }
  return file;
}

/*****************************************************************************/
/* STATIC                    transfer_line_to_bytes                          */
/*****************************************************************************/

static void
  transfer_line_to_bytes(kdu_line_buf &line, void *dst, int width,
                         int precision, bool is_signed)
{
  kdu_byte *dp = (kdu_byte *) dst;
  kdu_int32 val;
  if (line.get_buf16() != NULL)
    {
      kdu_sample16 *sp = line.get_buf16();
      if (line.is_absolute() && (precision <= 8))
        {
          int upshift = 8-precision;
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
      else if (line.is_absolute())
        {
          int downshift = precision-8;
          kdu_int32 offset = (1<<downshift) >> 1;
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> downshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> downshift;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
      else
        {
          kdu_int32 offset = 1 << (KDU_FIX_POINT-1);
          offset += (1 << (KDU_FIX_POINT-8)) >> 1;
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> (KDU_FIX_POINT-8);
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = (((kdu_int32) sp->ival) + offset) >> (KDU_FIX_POINT-8);
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
    }
  else
    {
      kdu_sample32 *sp = line.get_buf32();
      if (line.is_absolute())
        {
          assert(precision >= 8);
          int downshift = precision-8;
          kdu_int32 offset = (1<<downshift)-1;
          offset += 128 << downshift;
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
      else
        {
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<15)) >> 16;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte)(val-128);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<15)) >> 16;
                val += 128;
                if (val & 0xFFFFFF00)
                  val = (val < 0)?0:255;
                *dp = (kdu_byte) val;
              }
        }
    }
}

/*****************************************************************************/
/* STATIC                    transfer_line_to_words                          */
/*****************************************************************************/

static void
  transfer_line_to_words(kdu_line_buf &line, void *dst, int width,
                         int precision, bool is_signed)
{
  kdu_int16 *dp = (kdu_int16 *) dst;
  kdu_int32 val;
  if (line.get_buf16() != NULL)
    {
      kdu_sample16 *sp = line.get_buf16();
      if (line.is_absolute())
        {
          int upshift = 16-precision;
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
      else
        {
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << (16-KDU_FIX_POINT);
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = ((kdu_int32) sp->ival) << (16-KDU_FIX_POINT);
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
    }
  else
    {
      kdu_sample32 *sp = line.get_buf32();
      if (line.is_absolute() && (precision > 16))
        {
          int downshift = precision - 16;
          kdu_int32 offset = (1<<downshift) - 1;
          offset += 0x8000 << downshift;
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = (sp->ival + offset) >> downshift;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
        }
      else if (line.is_absolute())
        {
          int upshift = 16-precision;
          if (is_signed)
            for (; width--; sp++, dp++)
              {
                val = sp->ival << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; sp++, dp++)
              {
                val = sp->ival << upshift;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
      else
        {
          if (is_signed)
            for (; width--; dp++, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<7)) >> 8;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16)(val - 0x8000);
              }
          else
            for (; width--; dp++, sp++)
              {
                val = (kdu_int32)(sp->fval * (float)(1<<24));
                val = (val + (1<<7)) >> 8;
                val += 0x8000;
                if (val & 0xFFFF0000)
                  val = (val < 0)?0:0x0000FFFF;
                *dp = (kdu_int16) val;
              }
        }
    }
}


/* ========================================================================= */
/*                              kdv_decompressor                             */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdv_decompressor::kdv_decompressor                    */
/*****************************************************************************/

kdv_decompressor::kdv_decompressor(kdu_codestream codestream,
                                   int precision, bool is_signed,
                                   kdu_thread_env *env,
                                   int double_buffering_height)
{
  int c;

  this->env = env;
  double_buffering = false;
  processing_stripe_height = 1;
  if ((env != NULL) && (double_buffering_height > 0))
    {
      double_buffering = true;
      processing_stripe_height = double_buffering_height;
    }
  this->is_signed = is_signed;
  sample_bytes = 1;
  if (precision > 8)
    sample_bytes = 2;
  if (precision > 16)
    sample_bytes = 4;
  frame_bytes = 0; // We will increment this as we go.
  buffer = NULL;
  num_components = codestream.get_num_components();
  comp_info = new kdv_comp_info[num_components];
  for (c=0; c < num_components; c++)
    {
      kdv_comp_info *comp = comp_info + c;
      codestream.get_dims(c,comp->comp_dims);
      kdu_coords  subs; codestream.get_subsampling(c,subs);
      comp->count_val = subs.y;
      frame_bytes += sample_bytes * (int) comp->comp_dims.area();
    }
  buffer = new kdu_byte[frame_bytes];
  kdu_byte *comp_buf = buffer;
  for (c=0; c < num_components; c++)
    {
      kdv_comp_info *comp = comp_info + c;
      comp->comp_buf = comp_buf;
      comp_buf += sample_bytes * (int) comp->comp_dims.area();
    }
  env_batch_idx = 0;
  current_tile = tiles;
  if (env == NULL)
    next_tile = NULL;
  else
    {
      next_tile = tiles+1;
      current_tile->next = next_tile;
      next_tile->next = current_tile;
      init_tile(next_tile,codestream);
    }
}

/*****************************************************************************/
/*                 kdv_decompressor::init_tile (codestream)                  */
/*****************************************************************************/

bool
  kdv_decompressor::init_tile(kdv_tile *tile, kdu_codestream codestream)
{
  int c;
  assert(!(tile->engine.exists() || tile->ifc.exists()));
  tile->codestream = codestream;
  if (!codestream)
    return false;
  codestream.get_valid_tiles(tile->valid_indices);
  tile->idx = kdu_coords();
  tile->ifc = codestream.open_tile(tile->valid_indices.pos,env);
  if (tile->components == NULL)
    tile->components = new kdv_tcomp[num_components];
  if (env != NULL)
    tile->env_queue = env->add_queue(NULL,NULL,"tile root",env_batch_idx++);
  tile->engine.create(codestream,tile->ifc,false,false,false,
                      processing_stripe_height,env,tile->env_queue,
                      double_buffering);
  for (c=0; c < num_components; c++)
    {
      kdv_tcomp *comp = tile->components + c;
      comp->comp_size = comp_info[c].comp_dims.size;
      comp->tile_size = tile->engine.get_size(c);
      comp->counter = 0;
      comp->precision = codestream.get_bit_depth(c,true);
      comp->tile_rows_left = comp->tile_size.y;
      comp->bp = comp->next_tile_bp = comp_info[c].comp_buf;
      if (tile->idx.x < (tile->valid_indices.size.x-1))
        comp->next_tile_bp += sample_bytes * comp->tile_size.x;
      else
        comp->next_tile_bp += sample_bytes *
          (comp->tile_size.x + (comp->tile_size.y-1)*comp->comp_size.x);
      if ((comp->next_tile_bp-comp_info[c].comp_buf) >
          (sample_bytes*comp->comp_size.x*comp->comp_size.y))
        { kdu_error e; e << "All frames in the compressed video sequence "
          "should have the same dimensions for each image component.  "
          "Encountered a frame for which one image component "
          "is larger than the corresponding component in the first frame."; }
    }
  return true;
}

/*****************************************************************************/
/*                  kdv_decompressor::init_tile (tile)                       */
/*****************************************************************************/

bool
  kdv_decompressor::init_tile(kdv_tile *tile, kdv_tile *ref_tile)
{
  int c;
  assert(!(tile->engine.exists() || tile->ifc.exists()));
  tile->codestream = ref_tile->codestream;
  if (!tile->codestream)
    return false;
  tile->valid_indices = ref_tile->valid_indices;
  tile->idx = ref_tile->idx;
  tile->idx.x++;
  if (tile->idx.x >= tile->valid_indices.size.x)
    {
      tile->idx.x = 0;
      tile->idx.y++;
      if (tile->idx.y >= tile->valid_indices.size.y)
        return false;
    }
  tile->ifc =
    tile->codestream.open_tile(tile->valid_indices.pos+tile->idx,env);
  if (tile->components == NULL)
    tile->components = new kdv_tcomp[num_components];
  if (env != NULL)
    tile->env_queue = env->add_queue(NULL,NULL,"tile root",env_batch_idx++);
  tile->engine.create(tile->codestream,tile->ifc,false,false,false,
                      processing_stripe_height,env,tile->env_queue,
                      double_buffering);
  for (c=0; c < num_components; c++)
    {
      kdv_tcomp *comp = tile->components + c;
      kdv_tcomp *ref_comp = ref_tile->components + c;
      comp->comp_size = ref_comp->comp_size;
      comp->tile_size = tile->engine.get_size(c);
      comp->counter = 0;
      comp->precision = ref_comp->precision;
      comp->tile_rows_left = comp->tile_size.y;
      comp->bp = comp->next_tile_bp = ref_comp->next_tile_bp;
      if (tile->idx.x < (tile->valid_indices.size.x-1))
        comp->next_tile_bp += sample_bytes * comp->tile_size.x;
      else
        comp->next_tile_bp += sample_bytes *
          (comp->tile_size.x + (comp->tile_size.y-1)*comp->comp_size.x);
      if ((comp->next_tile_bp-comp_info[c].comp_buf) >
          (sample_bytes*comp->comp_size.x*comp->comp_size.y))
        { kdu_error e; e << "All frames in the compressed video sequence "
          "should have the same dimensions for each image component.  "
          "Encountered a frame for which one image component "
          "is larger than the corresponding component in the first frame."; }
    }
  return true;
}

/*****************************************************************************/
/*                       kdv_decompressor::close_tile                        */
/*****************************************************************************/

void
  kdv_decompressor::close_tile(kdv_tile *tile)
{
  if (env != NULL)
    env->terminate(tile->env_queue,false);
  tile->engine.destroy();
  tile->ifc.close(env);
}

/*****************************************************************************/
/*                       kdv_decompressor::store_frame                       */
/*****************************************************************************/

void
  kdv_decompressor::store_frame(FILE *file)
{
  if (fwrite(buffer,1,(size_t) frame_bytes,file) != (size_t) frame_bytes)
    { kdu_error e; e << "Unable to write to output VIX file.  Device may "
      "be full."; }
}

/*****************************************************************************/
/*                       kdv_decompressor::process_frame                     */
/*****************************************************************************/

void
  kdv_decompressor::process_frame(kdu_codestream codestream,
                                  kdu_codestream next_codestream)
{
  int c;

  // Process tiles one by one
  bool frame_complete = false;
  while (!frame_complete)
    {
      // Set up current tile processing engine
      if (env != NULL)
        { // Set up the current and next tile engine
          next_tile = (current_tile = next_tile)->next;
          if (!current_tile->is_active())
            init_tile(current_tile,codestream);
          frame_complete = !init_tile(next_tile,current_tile);
          if (frame_complete && next_codestream.exists())
            init_tile(next_tile,next_codestream);
        }
      else
        { // Set up the current tile engine only
          if (!current_tile->is_active())
            init_tile(current_tile,codestream); // Must be first tile
        }

      // Now process the tile
      bool done = false;
      while (!done)
        {
          done = true;

          // Decompress a line for each component whose counter is 0
          for (c=0; c < num_components; c++)
            {
              kdv_tcomp *comp = current_tile->components + c;
              if (comp->tile_rows_left == 0)
                continue;
              done = false;
              comp->counter--;
              if (comp->counter >= 0)
                continue;
              comp->counter += comp_info[c].count_val;
              comp->tile_rows_left--;
              kdu_line_buf *line = current_tile->engine.get_line(c,env);
              if (sample_bytes == 1)
                {
                  transfer_line_to_bytes(*line,comp->bp,comp->tile_size.x,
                                         comp->precision,is_signed);
                  comp->bp += comp->comp_size.x;
                }
              else if (sample_bytes == 2)
                {
                  transfer_line_to_words(*line,comp->bp,comp->tile_size.x,
                                         comp->precision,is_signed);
                  comp->bp += (comp->comp_size.x << 1);
                }
              else
                { kdu_error e; e << "The VIX writer provided here "
                    "currently supports only \"byte\" and \"word\" data "
                    "types as sample data containers.   The \"dword\" "
                    "data type could easily be added if necessary."; }
            }
        }

      // All processing done for current tile
      close_tile(current_tile);
      if (env == NULL)
        frame_complete = !init_tile(current_tile,current_tile);
    }
}

/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/*                                   main                                    */
/*****************************************************************************/

int main(int argc, char *argv[])
{
  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);
  kdu_args args(argc,argv,"-s");

  // Collect arguments.
  char *ifname = NULL;
  char *ofname = NULL;
  int max_frames = INT_MAX;
  int max_layers, max_components, discard_levels;
  int num_threads, double_buffering_height, in_memory_repetitions;
  kdu_dims region;
  bool overlapped_frames, cpu, quiet;
  parse_simple_arguments(args,ifname,ofname,max_frames,max_layers,
                         max_components,discard_levels,region,
                         num_threads,double_buffering_height,
                         overlapped_frames,in_memory_repetitions,cpu,quiet);
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

  // Open input file and first codestream
  jp2_family_src mj2_ultimate_src;
  mj2_source movie;
  mj2_video_source *mj2_video = NULL;
  kdu_simple_video_source mjc_video;
  kdu_compressed_video_source *video;
  bool is_ycc;
  if (check_mj2_suffix(ifname))
    {
      mj2_ultimate_src.open(ifname);
      movie.open(&mj2_ultimate_src);
      mj2_video = movie.access_video_track(1);
      if (mj2_video == NULL)
        { kdu_error e; e << "Motion JPEG2000 data source contains no video "
          "tracks."; }
      is_ycc = (mj2_video->access_colour().get_space() == JP2_sYCC_SPACE);
      video = mj2_video;
    }
  else if (check_mjc_suffix(ifname))
    {
      kdu_uint32 flags;
      mjc_video.open(ifname,flags);
      is_ycc = ((flags & KDU_SIMPLE_VIDEO_YCC) != 0);
      video = &mjc_video;
    }
  else
    { kdu_error e; e << "Input file must have one of the suffices "
      "\".mj2\" or \".mjc\".  See usage statement for more information."; }

  if ((max_components > 0) && (max_components < 3))
    is_ycc = false;

  if (video->open_image() < 0)
    { kdu_error e; e << "Compressed video source contains no images!"; }

  kdu_codestream codestream;
  codestream.create(video);

  // Restrict quality, resolution and/or spatial region as required
  codestream.apply_input_restrictions(0,max_components,discard_levels,
                                      max_layers,NULL);
  kdu_dims *reg_ptr = NULL;
  if (region.area() > 0)
    {
      kdu_dims dims; codestream.get_dims(0,dims);
      dims &= region;
      if (!dims)
        { kdu_error e; e << "Region supplied via `-int_region' argument "
          "has no intersection with the first image component to be "
          "decompressed, at the resolution selected."; }
      codestream.map_region(0,dims,region);
      reg_ptr = &region;
      codestream.apply_input_restrictions(0,max_components,discard_levels,
                                          max_layers,reg_ptr);
    }

        // If you wish to have geometric transformations folded into the
        // decompression process automatically, this is the place to call
        // `kdu_codestream::change_appearance'.

  // Create VIX file
  int num_frames = video->get_num_frames();
  if (num_frames > max_frames)
    num_frames = max_frames;
  kdu_uint32 timescale = video->get_timescale();
  int frame_period = (int) video->get_frame_period();
  int precision=0; // Value set by `open_vix_file' call below
  bool is_signed=false; // Value set by `open_vix_file' call below
  FILE *vix_file = NULL;
  if (ofname != NULL)
    vix_file = open_vix_file(ofname,codestream,discard_levels,num_frames,
                             timescale,frame_period,is_ycc,precision,
                             is_signed);
  
  // Create the multi-threading environment if required
  kdu_thread_env env, *env_ref=NULL;
  if (num_threads > 0)
    {
      env.create();
      for (int nt=1; nt < num_threads; nt++)
        if (!env.add_thread())
          num_threads = nt; // Unable to create all the threads requested
      env_ref = &env;
    }

  // Process frames one by one.  To make the code as readable as possible,
  // we provide two separate execution paths below.  The first is the
  // straightforward one, in which there is no overlapped frame processing.
  // The second path is slightly more complicated, because it has to
  // maintain two open codestreams at once.

  // Prepare to process frames
  int frame_count = 0;
  double processing_time = 0.0;
  double file_time = 0.0;
  if (!overlapped_frames)
    {
      codestream.enable_restart();
      kdv_decompressor decompressor(codestream,precision,is_signed,env_ref,
                                    double_buffering_height);
      do {
          clock_t start_time = clock();
          if (frame_count)
            {
              codestream.restart(video);
              codestream.apply_input_restrictions(0,max_components,
                                       discard_levels,max_layers,reg_ptr);
            }
          decompressor.process_frame(codestream);
          video->close_image();

          clock_t store_time = clock();
          if (vix_file != NULL)
            decompressor.store_frame(vix_file);

          processing_time += ((double)(store_time-start_time))/CLOCKS_PER_SEC;
          file_time += ((double)(clock()-store_time)) / CLOCKS_PER_SEC;

          frame_count++;
          if (!quiet)
            pretty_cout << "Number of frames processed = "
                        << frame_count << "\n";
          if (frame_count == num_frames)
            break;
        } while (video->open_image() >= 0);
      codestream.destroy();
    }
  else
    {
      if (!mj2_video)
        { kdu_error e; e << "The \"-overlapped_frames\" argument can be "
          "used only with MJ2 files.  Your input file appears to be of "
          "the MJC variety -- a simple, non-standard concatenation of "
          "codestreams."; }
      if (cpu)
        { kdu_error e; e << "Reliable CPU timing information cannot be "
          "collected in the overlapped frame processing mode (see "
          "the usage statement for \"-cpu\").  I suggest "
          "you time the entire application externally -- if you don't want "
          "to include output file writing time, just don't specify an "
          "output file."; }
      int frame_repetitions = in_memory_repetitions;
      if (frame_repetitions < 1)
        frame_repetitions = 1;
      num_frames *= frame_repetitions;
      codestream.destroy(); // Close existing handles, since we will directly
      video->close_image(); // open `jp2_input_box' objects for each frame.
      kdu_codestream codestreams[2];
      jp2_input_box boxes[2];
      int box_repetitions[2] = {0,0}; // Used with `frame_repetitions'
      int cur_idx=0; // Identifies current element in `codestreams' and `boxes'
      mj2_video->seek_to_frame(0);
      mj2_video->open_stream(0,&(boxes[cur_idx]),env_ref);
      if (frame_repetitions > 1)
        boxes[cur_idx].load_in_memory(100000000);
      codestreams[cur_idx].create(&(boxes[cur_idx]),env_ref);
      codestreams[cur_idx].enable_restart();
      codestreams[cur_idx].apply_input_restrictions(0,max_components,
                                       discard_levels,max_layers,reg_ptr);
      kdv_decompressor decompressor(codestreams[cur_idx],precision,is_signed,
                                    env_ref,double_buffering_height);
      int next_frame_to_open = 1;
      do {
          frame_count++;
          if ((!boxes[1-cur_idx].exists()) &&
              (frame_count < num_frames) &&
              mj2_video->seek_to_frame(next_frame_to_open) &&
              (mj2_video->open_stream(0,&(boxes[1-cur_idx]),
                                      env_ref) == next_frame_to_open))
            next_frame_to_open++;
          if (boxes[1-cur_idx].exists())
            { // Open the next frame's codestream
              if (in_memory_repetitions && (box_repetitions[1-cur_idx] == 0))
                boxes[1-cur_idx].load_in_memory(100000000);
              if (codestreams[1-cur_idx].exists())
                codestreams[1-cur_idx].restart(&(boxes[1-cur_idx]),env_ref);
              else
                {
                  codestreams[1-cur_idx].create(&(boxes[1-cur_idx]),env_ref);
                  codestreams[1-cur_idx].enable_restart();
                }
              codestreams[1-cur_idx].apply_input_restrictions(0,max_components,
                                            discard_levels,max_layers,reg_ptr);
            }
          else if (codestreams[1-cur_idx].exists())
            codestreams[1-cur_idx].destroy();
          decompressor.process_frame(codestreams[cur_idx],
                                     codestreams[1-cur_idx]);
          box_repetitions[cur_idx]++;
          if (box_repetitions[cur_idx] == frame_repetitions)
            {
              boxes[cur_idx].close();
              box_repetitions[cur_idx] = 0;
            }
          else
            boxes[cur_idx].seek(0);
          if ((vix_file != NULL) && (box_repetitions[cur_idx] == 0))
            decompressor.store_frame(vix_file);
          if (!quiet)
            pretty_cout << "Number of frames processed = "
                        << frame_count << "\n";
          cur_idx = 1-cur_idx;
        } while (boxes[cur_idx].exists());
      for (int c=0; c < 2; c++)
        if (codestreams[c].exists())
          codestreams[c].destroy();
    }

  // Print summary statistics
  if (cpu && frame_count)
    {
      pretty_cout << "Processing time = "
                  << processing_time/frame_count
                  << " seconds per frame\n";
      if (vix_file != NULL)
        pretty_cout << "File saving time = "
                    << file_time/frame_count
                    << " seconds per frame\n";
    }
  if (!quiet)
    {
      if (num_threads == 0)
        pretty_cout << "Processed using the single-threaded environment "
          "(see `-num_threads')\n";
      else
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          << "    " << num_threads << " parallel threads of execution "
          "(see `-num_threads')\n";
    }
  
  // Cleanup
  delete[] ifname;
  if (ofname != NULL)
    delete[] ofname;
  if (vix_file != NULL)
    fclose(vix_file);
  video->close();
  movie.close(); // Does nothing if not an MJ2 file.

  if (env.exists())
    env.destroy(); // Destroy multi-threading environment, if one was used
  return 0;
}
