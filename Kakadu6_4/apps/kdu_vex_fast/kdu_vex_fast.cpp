/*****************************************************************************/
// File: kdu_vex_fast.cpp [scope = APPS/VEX_FAST]
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
   High Performance Motion JPEG2000 decompressor -- this demo can form the
foundation for a real-time software-based digital cinema solution.
******************************************************************************/

#include <string.h>
#include <stdio.h> // so we can use `sscanf' for arg parsing.
#include <math.h>
#include <assert.h>
#include <time.h>
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
#include "kdu_vex.h"
#include "vex_display.h"

/* ========================================================================= */
/*                         Set up messaging services                         */
/* ========================================================================= */

class kdu_stream_message : public kdu_thread_safe_message {
  public: // Member classes
    kdu_stream_message(FILE *stream, kdu_exception exception_code)
      { // Service throws exception on end of message if
        // `exception_code' != KDU_NULL_EXCEPTION.
        this->stream = stream;  this->exception_code = exception_code;
      }
    void put_text(const char *string)
      { if (stream != NULL) fputs(string,stream); }
    void flush(bool end_of_message=false)
      {
        fflush(stream);
        kdu_thread_safe_message::flush(end_of_message);
        if (end_of_message && (exception_code != KDU_NULL_EXCEPTION))
          throw exception_code;
      }
  private: // Data
    FILE *stream;
    int exception_code;
  };

static kdu_stream_message cout_message(stdout, KDU_NULL_EXCEPTION);
static kdu_stream_message cerr_message(stderr, KDU_ERROR_EXCEPTION);
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
  out << "This is Kakadu's \"kdu_vex_fast\" application.\n";
  out << "\tCompiled against the Kakadu core system, version "
      << KDU_CORE_VERSION << "\n";
  out << "\tCurrent core system version is "
      << kdu_get_core_version() << "\n";
  out << "This demo application could form the basis for a real-time "
         "software-only digital cinema playback solution, provided a "
         "sufficiently powerful computational platform.\n";
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
  out << "-i <MJ2 input file>\n";
  if (comprehensive)
    out << "\tThe input file is opened as a Motion JPEG2000 file (as defined "
           "by ISO/IEC 15444-3), regardless of its suffix.  The "
           "implementation can readily be adapted to other file formats, "
           "though.\n";
  out << "-o <vix file>\n";
  if (comprehensive)
    out << "\tTo avoid over complicating this simple demonstration "
           "application, decompressed video is written as a VIX file.  VIX "
           "is a trivial non-standard video file format, consisting of a "
           "plain ASCI text header, followed by raw binary data.  A "
           "description of the VIX format is embedded in the usage "
           "statements printed by the \"kdu_v_compress\" application.  It "
           "is legal to leave this argument out, the principle purpose of "
           "which would be to time the decompression processing alone, "
           "without cluttering the statistics with file I/O issues.\n";
  out << "-display [F<fps>|W<fps>]\n";
  if (comprehensive)
    out << "\tThis argument provides an alternative way to consume the "
           "decompressed results.  You may supply either \"-o\" or "
           "\"-display\", but not both.  If this option is selected, the "
           "decompressed imagery will be written to an interleaved ARGB "
           "buffer, with 8 bits per sample, regardless of the original "
           "image precision, or the original number of image components.  "
           "This option is allowed only for the case in which there is "
           "only 1 image component (greyscale), 3 identically "
           "sized image components (colour) or 4 identically sized "
           "image components (colour + alpha).  In the greyscale case, the "
           "RGB channels are all set to the same value.  This is done for "
           "simplicity, so as to keep the implementation as compact (and "
           "hence readable) as possible.  For more generic rendering of "
           "arbitrary imagery, the \"kdu_show\" application is more "
           "appropriate.\n"
           "\t   The purpose of the \"-display\" argument here is to show "
           "how the decompressed content can be most efficiently prepared for "
           "display and then blasted to a graphics card, in high performance "
           "applications.  To enable this latter phase, a parameter must "
           "be supplied with the argument, indicating the frame rate <fps> "
           "and whether full-screen (\"F<fps>\") or a windowed (\"W<fps>\") "
           "presentation should be attempted.  Both of these options will "
           "fail if suitable DirectX support is not available.  Note that "
           "frames transferred to the graphics card can only be flipped "
           "into the foreground on the next available frame blanking period, "
           "which limits the maximum frame rate to the display's refresh rate; "
           "moreover, the maximum portion of the decompressed frame which "
           "is transferred to the graphics card is limited by the available "
           "display dimensions, even though the entire frame is "
           "decompressed into an off-screen memory buffer.  When this "
           "happens, you can move the display around the video frame region "
           "with the arrow keys.\n";
  out << "-engine_threads <#threads>[:<cpus>],<#threads>[:<cpus>],...\n";
  if (comprehensive)
    out << "\tThis application provides two mechanisms to exploit "
           "multiple CPU's: 1) by processing frames in parallel; and 2) by "
           "using Kakadu's multi-threaded environment to speed up the "
           "processing of each frame.  These can be blended in whatever "
           "way you like by separately selecting the number of frame "
           "processing engines and the number of threads to use within each "
           "engine.  The present argument takes a comma-separated list of "
           "descriptors, with one for each frame processing engine you would "
           "like to create.  Each descriptor commences with a positive "
           "integer, identifying the number of threads you would like to "
           "associate with the engine.  This may, optionally, be followed "
           "a CPU affinity mask, separated from the first integer by a "
           "colon.  The CPU affinity mask identifies the set of CPUs on "
           "which the threads for the frame processing engine should be "
           "scheduled.  If you have a system with a large number of CPUs, "
           "it is advisable to bind the frame processing engine's threads "
           "to specific CPUs so as to improve cache utilization.  Each "
           "non-zero bit in the binary representation of the CPU affinity "
           "mask corresponds to a CPU on which the thread can be scheduled.\n"
           "\t   If you do not provide a \"-engine_threads\" argument, "
           "the default policy is to create one frame processing engine for "
           "each physical CPU, assigning it a single-threaded processing "
           "environment.  Overall, the default policy provides a reasonable "
           "configuration, but it is not necessarily optimal.  The following "
           "things are worth considering when constructing different "
           "processing environments via this argument:\n"
           "\t  1) A separate management thread always consumes some "
           "resources to pre-load compressed data for the frame processing "
           "engines and to save the decompressed frame data.  If the "
           "`-display' option is used with an auxiliary parameter, at "
           "least one extra thread is created to manage the display update "
           "process -- in practice, however, DirectX creates some threads of "
           "its own.  On a system with a large number of CPUs, it might "
           "possibly be best to create less frame processing threads than "
           "the number of CPU's so as to ensure timely operation of these "
           "other management and display oriented threads.\n"
           "\t  2) As more threads are added to each processing engine, "
           "increasing inefficiencies are incurred due to occasional blocking "
           "on access to critical sections in the codestream management "
           "sub-system.  These are likely to decrease as Kakadu evolves, "
           "which might change the optimal balance between parallel engines "
           "and parallel processing within each engine.\n"
           "\t  3) Although the single threaded processing environment (i.e., "
           "one thread per engine) has minimal overhead, multi-threaded "
           "engines have the potential to better exploit the sharing of L2 "
           "cache memory between close CPUs.\n"
           "\t  4) The CPU affinity mask functionality might be implemented "
           "differently on different operating systems and might not be "
           "implemented at all on some platforms.  To see what support is "
           "offered, you may wish to refer to the implementation of "
           "`kdu_thread::set_cpu_affinity' in \"kdu_elementary.h\", "
           "assuming you have access to the Kakadu core system's source "
           "code.\n";
  out << "-read_ahead <num frames read ahead by the management thread>\n";
  if (comprehensive)
    out << "\tBy default, the number of frames which can be active at any "
           "given time is set to twice the number of processing engines.  "
           "By \"active\", we mean frames whose compressed contents have "
           "been read, but whose decompressed output has not yet been "
           "consumed by the management thread.  This argument allows you "
           "to specify the number of active frames as E + A, where E is "
           "the number of frame processing engines and A is the read-ahead "
           "value supplied as the argument's parameter.\n";
  out << "-trunc <block truncation factor>\n";
  if (comprehensive)
    out << "\tYou may use this option to experiment with the framework's "
           "dynamic block truncation feature.  The real-valued parameter is "
           "multiplied by 256 before ultimately passing it to the "
           "`kdu_codestream::set_block_truncation' function, so that "
           "the supplied real-valued parameter can be roughly interpreted "
           "as the number of coding passes to discard.  Fractional values "
           "may cause coding passes to be discarded only from some "
           "code-blocks.  Ultimately, this features allows you to trade "
           "computation time for quality, even when the compressed "
           "source contains only one quality layer.  The internal objects "
           "allow the truncation factor to be changed dynamically, so you "
           "could implement a feedback loop to maintain a target frame "
           "rate for computation-limited applications.  The present "
           "demonstration application does not implement such a feedback "
           "loop, since it would obscure true processing performance.\n";
  out << "-repeat <number of times to cycle through the entire video>\n";
  if (comprehensive)
    out << "\tUse this option to simulate larger video sequences for more "
           "accurate timing information, by looping over the supplied "
           "video source the indicated number of times.\n";
  out << "-reduce <discard levels>\n";
  if (comprehensive)
    out << "\tSet the number of highest resolution levels to be discarded.  "
           "The frame resolution is effectively divided by 2 to the power of "
           "the number of discarded levels.\n";
  out << "-components <max image components to decompress>\n";
  if (comprehensive)
    out << "\tYou can use this argument to limit the number of (leading) "
           "image components which are decompressed.\n";
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
/* STATIC                       parse_arguments                              */
/*****************************************************************************/

static int *
  parse_arguments(kdu_args &args, char * &ifname, char * &ofname,
                  int &discard_levels, int &max_components,
                  int &truncation_factor, int &num_engines,
                  int &repeat_factor, int &read_ahead_frames,
                  bool &want_display, bool &want_full_screen, float &want_fps,
                  bool &quiet)
  /* Returns an array with `num_engines' pairs of integers.  The first
     integer in each pair is the number of threads associated with the
     relevant engine.  The second integer in each pair is the CPU affinity
     mask for those threads.  If the cpu affinity mask is 0, the threads
     can be scheduled on any processor. */
{
  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  num_engines = 0;
  ofname = NULL;
  want_display = want_full_screen = false;
  want_fps = -1.0F; // Anything <= 0.0 means no physical display
  discard_levels = 0;
  max_components = 0; // Means no limit
  truncation_factor = 0; // No truncation
  repeat_factor = 1;
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

  if (args.find("-display") != NULL)
    {
      want_display = true;
      const char *string = args.advance();
      if ((string != NULL) && (*string == 'F'))
        {
          want_full_screen = true;
          if ((sscanf(string+1,"%f",&want_fps) != 1) || (want_fps <= 0.0F))
            { kdu_error e; e << "The optional parameter to \"-display\" "
              "must contain a positive real-valued frame rate (frames/second) "
              "after the `F' or `W' prefix."; }
          args.advance();
        }
      else if ((string != NULL) && (*string == 'W'))
        {
          if ((sscanf(string+1,"%f",&want_fps) != 1) || (want_fps <= 0.0F))
            { kdu_error e; e << "The optional parameter to \"-display\" "
              "must contain a positive real-valued frame rate (frames/second) "
              "after the `F' or `W' prefix."; }
          args.advance();
        }
    }

  if (want_display && (ofname != NULL))
    { kdu_error e; e << "The \"-o\" and \"-display\" arguments are "
      "mutually exclusive."; }

  if (args.find("-reduce") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&discard_levels) != 1) ||
          (discard_levels < 0))
        { kdu_error e; e << "\"-reduce\" argument requires a non-negative "
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

  if (args.find("-trunc") != NULL)
    {
      const char *string = args.advance();
      float factor;
      if ((string == NULL) || (sscanf(string,"%f",&factor) != 1) ||
          (factor < 0.0F) || (factor > 255.0F))
        { kdu_error e; e << "\"-trunc\" argument requires a non-negative "
          "real-valued parameter, no larger than 255.0!"; }
      args.advance();
      truncation_factor = (int) floor(factor*256.0+0.5);
    }

  if (args.find("-repeat") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&repeat_factor) != 1) ||
          (repeat_factor < 1))
        { kdu_error e; e << "\"-repeat\" argument requires a positive "
          "integer parameter!"; }
      args.advance();
    }

  if (args.find("-quiet") != NULL)
    {
      quiet = true;
      args.advance();
    }

  int *engine_threads = NULL;
  if (args.find("-engine_threads") != NULL)
    {
      const char *cp, *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-engine_threads\" requires a parameter "
          "string consisting of a comma-separated list of descriptors, "
          "where each descriptor is either a single integer, or a pair "
          "of colon-separated integers."; }
      int nthrds = 0;
      for (cp=string; (sscanf(cp,"%d",&nthrds) == 1); )
        {
          if (nthrds < 1)
            { kdu_error e; e << "\"-engine_threads\" requires a parameter "
              "string consisting of a comma-separated list of descriptors, "
              "where each descriptor is either a single integer, or a pair "
              "of colon-separated integers."; }
          num_engines++;
          cp = strchr(cp,',');
          if (cp == NULL)
            break;
          cp++;
        }
      if (cp != NULL)
        { kdu_error e; e << "\"-engine_threads\" parameter string contains "
          "an unrecognized suffix, \"" << cp << "\"."; }
      int n=0;
      engine_threads = new int[2*num_engines];
      for (cp=string; (sscanf(cp,"%d",&nthrds) == 1); n++)
        {
          assert(n < num_engines);
          if (nthrds < 1)
            { kdu_error e; e << "The descriptors supplied with the "
              "\"-engine_threads\" argument must commence with a strictly "
              "positive integer, representing the number of threads for "
              "the relevant engine."; }
          engine_threads[2*n] = nthrds;
          engine_threads[2*n+1] = 0; // Can be scheduled on all CPUs.
          for (cp++; (*cp != ',') && (*cp != ':') && (*cp != '\0'); cp++);
          if (*cp == ':')
            {
              int affinity_mask;
              if ((sscanf(cp+1,"%d",&affinity_mask) == 0) ||
                  (affinity_mask < 1))
                { kdu_error e; e << "Any descriptor which contains a colon, "
                  "in the parameter string supplied with the "
                  "\"-engine_threads\" argument, must contain a valid "
                  "CPU affinity mask following the colon.  Valid affinity "
                  "masks must be positive integers."; }
              engine_threads[2*n+1] = affinity_mask;
            }
          while ((*cp != ',') && (*cp != '\0'))
            cp++;
          if (*cp != ',')
            break;
          cp++;
        }
      args.advance();
    }
  else
    { // Create a default set of engines.
      int n, num_cpus = kdu_get_num_processors();
      num_engines = num_cpus;
      engine_threads = new int[2*num_engines];
      for (n=0; n < num_engines; n++)
        {
          engine_threads[2*n] = 1;
          engine_threads[2*n+1] = 0;
        }
    }

  read_ahead_frames = num_engines;
  if (args.find("-read_ahead") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&read_ahead_frames) != 1) ||
          (read_ahead_frames < 0))
        { kdu_error e; e << "\"-read_ahead\" argument requires a "
          "non-negative integer parameter!"; }
      args.advance();
    }

  return engine_threads;
}

/*****************************************************************************/
/* STATIC                        open_vix_file                               */
/*****************************************************************************/

static FILE *
  open_vix_file(char *fname, vex_frame_queue *queue,
                kdu_uint32 timescale, int frame_period, bool is_ycc)
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
  fprintf(file,">VIDEO<\n%f 0\n",
          ((double) timescale) / ((double) frame_period));
  fprintf(file,">COLOUR<\n%s\n",(is_ycc)?"YCbCr":"RGB");
  const char *container_string = "char";
  int precision = queue->get_precision();
  if (precision > 8)
    container_string = "word";
  if (precision > 16)
    container_string = "dword";
  int is_big = 1; ((char *)(&is_big))[0] = 0;
  const char *endian_string = (is_big)?"big-endian":"little-endian";
  int components = queue->get_num_components();
  kdu_dims dims = queue->get_frame_dims();
  bool is_signed = queue->get_signed();
  fprintf(file,">IMAGE<\n%s %s %d %s\n",
          ((is_signed)?"signed":"unsigned"),
          container_string,precision,endian_string);
  fprintf(file,"%d %d %d\n",dims.size.x,dims.size.y,components);
  for (int c=0; c < components; c++)
    {
      kdu_coords subs = queue->get_component_subsampling(c);
      fprintf(file,"%d %d\n",subs.x,subs.y);
    }
  return file;
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

  char *ifname=NULL, *ofname=NULL;
  int discard_levels, max_components, truncation_factor;
  int num_engines, repeat_factor, read_ahead_frames;
  bool want_display, want_full_screen, quiet;
  float want_fps=-1.0F; // <= 0.0 means no physical display
  int *engine_threads = NULL;
  jp2_family_src mj2_ultimate_src;
  mj2_source movie;
  mj2_video_source *video = NULL;
  vex_frame_queue *queue = NULL;
  vex_engine *engines=NULL;
  FILE *vix_file = NULL;

  vex_display display; // Impotent if `KDU_DX9' is not defined

  int return_code = 0;
  try {
      kdu_args args(argc,argv,"-s");
      engine_threads =
        parse_arguments(args,ifname,ofname,discard_levels,max_components,
                        truncation_factor,num_engines,repeat_factor,
                        read_ahead_frames,want_display,want_full_screen,
                        want_fps,quiet);
      if (args.show_unrecognized(pretty_cout) != 0)
        { kdu_error e;
          e << "There were unrecognized command line arguments!"; }
      int t, total_engine_threads = 0;
      for (t=0; t < num_engines; t++)
        total_engine_threads += engine_threads[2*t];

      
      mj2_ultimate_src.open(ifname);
      movie.open(&mj2_ultimate_src);
      if ((video = movie.access_video_track(1)) == NULL)
        { kdu_error e; e << "Motion JPEG2000 data source contains no video "
          "tracks."; }

      queue = new vex_frame_queue;
      int max_active_frames = num_engines + read_ahead_frames;
      if (!queue->init(video,discard_levels,max_components,
                       repeat_factor,max_active_frames,want_display))
        { kdu_error e; e << "Video track contains no frames!"; }

      queue->set_truncation_factor(truncation_factor); // You can call this
                  // at any time in applications requiring dynamic tradeoff
                  // between computation speed and quality.

      bool is_ycc = (video->access_colour().get_space()==JP2_sYCC_SPACE);
      kdu_uint32 timescale = video->get_timescale();
      int frame_period = (int) video->get_frame_period();

      vex_frame_memory_allocator *frame_memory_allocator = queue;
      if (ofname != NULL)
        vix_file = open_vix_file(ofname,queue,timescale,frame_period,is_ycc);
      else if (want_display && (want_fps > 0.0F))
        {
          int total_frame_buffers = max_active_frames + 4;
          const char *result = display.init(queue->get_component_dims(0).size,
                                            want_full_screen,want_fps,
                                            total_frame_buffers);
          if (result == NULL)
            {
              pretty_cout << "   Use arrows to pan; hit any other key to "
                             "terminate ...\n";
              pretty_cout.flush(false);
              frame_memory_allocator = &display;
            }
          else
            { kdu_error e; e << result; }
        }

      int thread_concurrency = kdu_get_num_processors();
      if (thread_concurrency < total_engine_threads)
        thread_concurrency = total_engine_threads;
      engines = new vex_engine[num_engines];
      for (t=0; t < num_engines; t++)
        engines[t].startup(queue,t,engine_threads[2*t],engine_threads[2*t+1],
                           thread_concurrency);

      clock_t start_time = clock();
      vex_frame *frame;
      int num_processed_frames = 0;
      if (num_engines > 1)
        { // Set the management thread to have a larger priority than the
          // engine threads so as to make extra sure that we have data
          // available for the engines whenever the processing resources
          // are available to use it.
          int cur_priority, min_priority, max_priority;
          kdu_thread thread;
          thread.set_to_self();
          cur_priority = thread.get_priority(min_priority,max_priority);
          thread.set_priority(max_priority);
        }
      while ((frame =
              queue->get_processed_frame(frame_memory_allocator)) != NULL)
        {
          num_processed_frames++;
          if (vix_file != NULL)
            {
              if (fwrite(frame->buffer,1,(size_t)(frame->frame_bytes),
                         vix_file) != (size_t)(frame->frame_bytes))
                { kdu_error e; e << "Unable to write to output VIX file.  "
                  "device may be full."; }
            }
          if (display.exists() && !display.push_frame(frame))
            break;
          queue->recycle_processed_frame(frame);
          if (!quiet)
            pretty_cout << "Number of frames processed = "
                        << num_processed_frames << "\n";
        }

      clock_t end_time = clock();
      double cpu_seconds = ((double)(end_time-start_time)) / CLOCKS_PER_SEC;
      pretty_cout << "Processed a total of\n\t"
             << num_processed_frames << " frames, using\n\t"
             << num_engines << " frame processing engines, with\n\t"
             << total_engine_threads << " frame processing threads, in\n\t"
             << cpu_seconds << " seconds = "
             << num_processed_frames/cpu_seconds << " frames/second.\n";
    }
  catch (kdu_exception) {
      return_code = 1;
    }
  catch (std::bad_alloc &) {
      pretty_cerr << "Memory allocation failure detected!\n";
      return_code = 2;
    }

  // Cleanup
  if (display.exists() && want_full_screen && (engines != NULL))
    { // Be extra careful to avoid the risk of deadlocks during
      // premature (user-kill) termination, by gracefully shutting
      // down the engines.
      for (int e=0; e < num_engines; e++)
        engines[e].shutdown(true);
    }
  if (ifname != NULL)
    delete[] ifname;
  if (ofname != NULL)
    delete[] ofname;
  if (engine_threads != NULL)
    delete[] engine_threads;
  if (vix_file != NULL)
    fclose(vix_file);
  if (engines != NULL)
    delete[] engines;  // Must delete engines before the queue
  if (queue != NULL)
    delete queue;
  if (video != NULL)
    video->close();
  movie.close();

  return 0;
}
