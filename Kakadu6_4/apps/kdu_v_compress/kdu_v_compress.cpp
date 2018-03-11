/*****************************************************************************/
// File: kdu_v_compress.cpp [scope = APPS/V_COMPRESSOR]
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
   File-based Motion JPEG2000 compressor application.
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
#include "v_compress_local.h"

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
  out << "This is Kakadu's \"kdu_v_compress\" application.\n";
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
  out << "-i <vix or yuv file>\n";
  if (comprehensive)
    out << "\tTo avoid over complicating this simple demonstration "
           "application, input video must be supplied as a VIX file or a "
           "raw YUV file.  If a raw YUV file is used, the dimensions, "
           "frame rate and format must be found in the filename itself, "
           "as a string of the form: \"<width>x<height>x<fps>x<format>\", "
           "where <width> and <height> are integers, <fps> is real-valued "
           "and <format> is one of \"422\", \"420\" or \"444\".  Any file "
           "with a \".yuv\" suffix, which does not contain a string of this "
           "form in its name, will be rejected.  VIX is a trivial "
           "non-standard video file format, consisting of a plain ASCI text "
           "header, followed by raw binary data.\n"
           "\t  The header must commence with the 3 character magic string, "
           "\"vix\", followed by a new-line character.  The rest of the "
           "header consists of a sequence of unframed tags.  Each tag "
           "commences with a tag identifier, inside angle quotes.  In "
           "particular, the final quoted tag must be \">IMAGE<\".  Besides "
           "the \"IMAGE\" tag, \"VIDEO\" and \"COLOUR\" tags are recognized.  "
           "Text inside tag bodies is free format, with regard to white "
           "space, but the \">\" character which introduces a new tag must "
           "be the first character on its line.\n"
           "\t  The \"VIDEO\" tag is followed by text strings containing "
           "the numeric value of the nominal frame rate (real-valued) and "
           "the number of frames (integer) -- the latter may be 0 if the "
           "number of frames is unknown.\n"
           "\t  The \"COLOUR\" tag must be followed by one of the two "
           "strings, \"RGB\" or \"YCbCr\".  If no \"Colour\" tag is present, "
           "an RGB colour space will be assumed, unless there are fewer "
           "than 3 image components.\n"
           "\t  The \"IMAGE\" tag must be followed by a 4 token description "
           "of the numerical sample representation: 1) \"signed\" or "
           "\"unsigned\"; 2) \"char\", \"word\" or \"dword\"; 3) the number "
           "of most significant bits from each sample's byte, word or dword "
           "which are actually used (the unused LSB's must be 0); and 4) "
           "\"little-endian\" or \"big-endian\".  These are followed by a 3 "
           "token description of the dimensions of each video frame: "
           "1) canvas width; 2) canvas height; and 3) the number of image "
           "components.  Finally, horizontal and vertical sub-sampling "
           "factors (relative to the canvas dimensions) are provided for "
           "each successive component; these must be integers in the range "
           "1 to 255.\n"
           "\t  The actual image data follows the new-line character "
           "which concludes the \"Image\" tag's last sub-sampling factor.  "
           "Each video frame appears component by component, without any "
           "framing or padding or any type.\n";
  out << "-frames <max frames to process>\n";
  if (comprehensive)
    out << "\tBy default, all available input frames are processed.  This "
           "argument may be used to limit the number of frames which are "
           "actually processed.\n";
  out << "-subsample <temporal subsampling factor>\n";
  if (comprehensive)
    out << "\tYou may use this argument to subsample the input frame "
           "sequence, keeping only frames with index n*K, where K is "
           "the subsampling factor and frame indices start from 0.\n";
  out << "-o <MJ2 or MJC (almost raw) compressed output file>\n";
  if (comprehensive)
    out << "\tTwo types of compressed video files may be generated.  If the "
           "file has the suffix, \".mj2\" (or \".mjp2\"), the compressed "
           "video will be wrapped in the Motion JPEG2000 file format, which "
           "embeds all relevant timing and indexing information, as well as "
           "rendering hints to allow faithful reproduction and navigation by "
           "suitably equipped readers.\n"
           "\t  Otherwise, the suffix must be \".mjc\" and the "
           "compressed video will be packed into a simple output file, "
           "having a 16 byte header, followed by the video "
           "image code-streams, each preceeded by a four byte big-endian "
           "word, which holds the length of the ensuing code-stream, "
           "not including the length word itself.  The 16 byte header "
           "consists of a 4-character magic string, \"MJC2\", followed "
           "by three big-endian integers: a time scale (number of ticks "
           "per second); the frame period (number of time scale ticks); "
           "and a flags word, which is currently used to distinguish "
           "between YCC and RGB type colour spaces.\n";
  out << "-rate -|<bits/pel>,<bits/pel>,...\n";
  if (comprehensive)
    out << "\tOne or more bit-rates, expressed in terms of the ratio between "
           "the total number of compressed bits (including headers) per video "
           "frame, and the product of the largest horizontal and vertical "
           "image component dimensions.  A dash, \"-\", may be used in place "
           "of the first bit-rate in the list to indicate that the final "
           "quality layer should include all compressed bits.  Specifying a "
           "very large rate target is fundamentally different to using the "
           "dash, \"-\", because the former approach may cause the "
           "incremental rate allocator to discard terminal coding passes "
           "which do not lie on the rate-distortion convex hull.  This means "
           "that reversible compression might not yield a truly lossless "
           "representation if you specify `-rate' without a dash for the "
           "first rate target, no matter how large the largest rate target "
           "is.\n"
           "\t   If \"Clayers\" is not used, the number of layers is "
           "set to the number of rates specified here. If \"Clayers\" is used "
           "to specify an actual number of quality layers, one of the "
           "following must be true: 1) the number of rates specified here is "
           "identical to the specified number of layers; or 2) one, two or no "
           "rates are specified using this argument.  When two rates are "
           "specified, the number of layers must be 2 or more and intervening "
           "layers will be assigned roughly logarithmically spaced bit-rates. "
           "When only one rate is specified, an internal heuristic determines "
           "a lower bound and logarithmically spaces the layer rates over the "
           "range.\n"
           "\t   Note carefully that all bit-rates refer only to the "
           "code-stream data itself, including all code-stream headers, but "
           "not including any auxiliary information from the wrapping file "
           "format.\n";
  out << "-slope <layer slope>,<layer slope>,...\n";
  if (comprehensive)
    out << "\tIf present, this argument provides rate control information "
           "directly in terms of distortion-length slope values, overriding "
           "any information which may or may not have been supplied via a "
           "`-rate' argument.  If the number of quality layers is  not "
           "specified via a `Qlayers' argument, it will be deduced from the "
           "number of slope values.  Slopes are inversely related to "
           "bit-rate, so the slopes should decrease from layer to layer.  The "
           "program automatically sorts slopes into decreasing order so you "
           "need not worry about getting the order right.  For reference "
           "we note that a slope value of 0 means that all compressed bits "
           "will be included by the end of the relevant layer, while a "
           "slope value of 65535 means that no compressed bits will be "
           "included in the  layer.\n";
  out << "-accurate -- slower, slightly more reliable rate control\n";
  if (comprehensive)
    out << "\tThis argument is relevant only when `-rate' is used for rate "
           "control, in place of `-slope'.  By default, distortion-length "
           "slopes derived during rate control for the previous frame, are "
           "used to inform the block encoder of a lower bound on the "
           "distortion-length slopes associated with coding passes it "
           "produces.  This allows the block coder to stop before processing "
           "all coding passes, saving time.  The present argument may be "
           "used to disable this feature, which will slow the compressor "
           "down (except during lossless compression), but may improve the "
           "reliability of the rate control slightly.  The present "
           "argument also enables more careful rate control for each "
           "frame so that the rate budget for each individual frame "
           "is used up as fully as possible.\n";
  out << "-add_info -- causes the inclusion of layer info in COM segments.\n";
  if (comprehensive)
    out << "\tIf you specify this flag, a code-stream COM (comment) marker "
           "segment will be included in the main header of every "
           "codestream, to record the distortion-length slope and the "
           "size of each quality layer which is generated.  Since this "
           "is done for each codestream and there is one codestream "
           "for each frame, you may find that the size overhead of this "
           "feature is unwarranted.  The information can be of use for "
           "R-D optimized delivery of compressed content using Kakadu's "
           "JPIP server, but this feature is mostly of interest when "
           "accessing small regions of large images (frames).  Most "
           "video applications, however, involve smaller frame sizes.  For "
           "this reason, this feature is disabled by default in this "
           "application, while it is enabled by default in the "
           "\"kdu_compress\" still image compression application.\n";
  out << "-no_weights -- target MSE minimization for colour images.\n";
  if (comprehensive)
    out << "\tBy default, visual weights will be automatically used for "
           "colour imagery (anything with 3 compatible components).  Turn "
           "this off if you want direct minimization of the MSE over all "
           "reconstructed colour components.\n";
  siz_params siz; siz.describe_attributes(out,comprehensive);
  cod_params cod; cod.describe_attributes(out,comprehensive);
  qcd_params qcd; qcd.describe_attributes(out,comprehensive);
  rgn_params rgn; rgn.describe_attributes(out,comprehensive);
  poc_params poc; poc.describe_attributes(out,comprehensive);
  crg_params crg; crg.describe_attributes(out,comprehensive);
  org_params org; org.describe_attributes(out,comprehensive);
  atk_params atk; atk.describe_attributes(out,comprehensive);
  dfs_params dfs; dfs.describe_attributes(out,comprehensive);
  ads_params ads; ads.describe_attributes(out,comprehensive);
  out << "-s <switch file>\n";
  if (comprehensive)
    out << "\tSwitch to reading arguments from a file.  In the file, argument "
           "strings are separated by whitespace characters, including spaces, "
           "tabs and new-line characters.  Comments may be included by "
           "introducing a `#' or a `%' character, either of which causes "
           "the remainder of the line to be discarded.  Any number of "
           "\"-s\" argument switch commands may be included on the command "
           "line.\n";
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
           "one actual thread of execution.\n";
  out << "-cpu -- report CPU time required for processing and I/O.\n";
  out << "-quiet -- suppress informative messages.\n";
  out << "-version -- print core system version I was compiled against.\n";
  out << "-v -- abbreviation of `-version'\n";
  out << "-usage -- print a comprehensive usage statement.\n";
  out << "-u -- print a brief usage statement.\"\n\n";

  if (!comprehensive)
    {
      out.flush();
      exit(0);
    }

  out.set_master_indent(0);
  out << "Notes:\n";
  out.set_master_indent(3);
  out << "    Arguments which commence with an upper case letter (rather than "
         "a dash) are used to set up code-stream parameter attributes. "
         "These arguments have the general form:"
         "  <arg name>={fld1,fld2,...},{fld1,fld2,...},..., "
         "where curly braces enclose records and each record is composed of "
         "fields.  The type and acceptable values for the fields are "
         "identified in the usage statements, along with whether or not "
         "multiple records are allowed.  In the special case where only one "
         "field is defined per record, the curly braces may be omitted. "
         "In no event may any spaces appear inside an attribute argument.\n";
  out << "    Most of the code-stream parameter attributes take an optional "
         "tile-component modifier, consisting of a colon, followed by a "
         "tile specifier, a component specifier, or both.  The tile specifier "
         "consists of the letter `T', followed immediately be the tile index "
         "(tiles are numbered in raster order, starting from 0).  Similarly, "
         "the component specifier consists of the letter `C', followed "
         "immediately by the component index (starting from 0). These "
         "modifiers may be used to specify parameter changes in specific "
         "tiles, components, or tile-components.\n";
  out << "    If you do not remember the exact form or description of one of "
         "the code-stream attribute arguments, simply give the attribute name "
         "on the command-line and the program will exit with a detailed "
         "description of the attribute.\n";
  out << "    If SIZ parameters are to be supplied explicitly on the "
         "command line, be aware that these may be affected by simultaneous "
         "specification of geometric transformations.  If uncertain of the "
         "behaviour, use `-record' to determine the final compressed "
         "code-stream parameters which were used.\n";
  out << "    If you are compressing a 3 component image using the "
         "reversible or irreversible colour transform (this is the default), "
         "the program will automatically introduce a reasonable set of visual "
         "weighting factors, unless you use the \"Clev_weights\" or "
         "\"Cband_weights\" options yourself.  This does not happen "
         "automatically in the case of single component images, which are "
         "optimized purely for MSE by default.  To see whether weighting "
         "factors were used, you may like to use the `-record' option.\n";
  out.flush();
  exit(0);
}

/*****************************************************************************/
/* STATIC                     parse_simple_args                              */
/*****************************************************************************/

static void
  parse_simple_arguments(kdu_args &args, char * &ifname, char * &ofname,
                         int &max_frames, int &subsampling_factor,
                         bool &no_slope_predict, bool &no_info,
                         bool &no_weights, int &num_threads,
                         bool &cpu, bool &quiet)
  /* Parses most simple arguments (those involving a dash). */
{
  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  max_frames = INT_MAX;
  subsampling_factor = 1;
  no_slope_predict = false;
  no_info = true;
  no_weights = false;
  num_threads = 0; // Not actually the default value -- see belwo
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
  else
    { kdu_error e; e << "You must supply an output file name."; }

  if (args.find("-frames") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_frames) != 1) ||
          (max_frames <= 0))
        { kdu_error e; e << "The `-frames' argument requires a positive "
          "integer parameter."; }
      args.advance();
    }

  if (args.find("-subsample") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&subsampling_factor) != 1) ||
          (subsampling_factor <= 0))
        { kdu_error e; e << "The `-subsample' argument requires a positive "
          "integer parameter."; }
      args.advance();
    }

  if (args.find("-accurate") != NULL)
    {
      no_slope_predict = true;
      args.advance();
    }

  if (args.find("-add_info") != NULL)
    {
      no_info = false;
      args.advance();
    }

  if (args.find("-no_weights") != NULL)
    {
      no_weights = true;
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
/* STATIC                      check_yuv_suffix                              */
/*****************************************************************************/

static bool
  check_yuv_suffix(const char *fname)
/* Returns true if the file-name has the suffix ".yuv", where the
   check is case insensitive. */
{
  const char *cp = strrchr(fname,'.');
  if (cp == NULL)
    return false;
  cp++;
  if ((*cp != 'y') || (*cp == 'Y'))
    return false;
  cp++;
  if ((*cp != 'u') && (*cp != 'U'))
    return false;
  cp++;
  if ((*cp != 'v') && (*cp != 'V'))
    return false;
  cp++;
  return (*cp == '\0');
}

/*****************************************************************************/
/* STATIC                      check_mj2_suffix                              */
/*****************************************************************************/

static bool
  check_mj2_suffix(const char *fname)
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
  cp++;
  return (*cp == '\0');
}

/*****************************************************************************/
/* STATIC                      check_mjc_suffix                              */
/*****************************************************************************/

static bool
  check_mjc_suffix(const char *fname)
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
  return (*cp == '\0');
}

/*****************************************************************************/
/* STATIC                       parse_yuv_format                             */
/*****************************************************************************/

static const char *
  parse_yuv_format(const char *fname, int &height, int &width,
                   double &frame_rate)
  /* Returns NULL, or one of the strings "444", "420" or "422", setting the
     various arguments to their values. */
{
  const char *format = "x444";
  const char *end = strstr(fname,format);
  if (end == NULL)
    { format = "x420"; end = strstr(fname,format); }
  if (end == NULL)
    { format = "x422"; end = strstr(fname,format); }
  if (end == NULL)
    return NULL;
  const char *start = end-1;
  for (; (start > fname) && (*start != 'x'); start--);
  if ((start == fname) || (sscanf(start+1,"%lf",&frame_rate) != 1) ||
      (frame_rate <= 0.0))
    return NULL;
  for (start--; (start > fname) && isdigit(*start); start--);
  if ((start == fname) || (*start != 'x') ||
      (sscanf(start+1,"%d",&height) != 1) || (height < 1))
    return NULL;
  for (start--; (start > fname) && isdigit(*start); start--);  
  if ((sscanf(start+1,"%d",&width) != 1) || (width < 1))
    return NULL;
  return format+1;
}

/*****************************************************************************/
/* INLINE                    eat_white_and_comments                          */
/*****************************************************************************/

static inline void
  eat_white_and_comments(FILE *fp)
{
  int ch;
  bool in_comment;

  in_comment = false;
  while ((ch = fgetc(fp)) != EOF)
    if ((ch == '#') || (ch == '%'))
      in_comment = true;
    else if (ch == '\n')
      in_comment = false;
    else if ((!in_comment) && (ch != ' ') && (ch != '\t') && (ch != '\r'))
      {
        ungetc(ch,fp);
        return;
      }
}

/*****************************************************************************/
/* STATIC                         read_token                                 */
/*****************************************************************************/

static bool
  read_token(FILE *fp, char *buffer, int buffer_len)
{
  int ch;
  char *bp = buffer;

  while (bp == buffer)
    {
      eat_white_and_comments(fp);
      while ((ch = fgetc(fp)) != EOF)
        {
          if ((ch == '\n') || (ch == ' ') || (ch == '\t') || (ch == '\r') ||
              (ch == '#') || (ch == '%'))
            {
              ungetc(ch,fp);
              break;
            }
          *(bp++) = (char) ch;
          if ((bp-buffer) == buffer_len)
            { kdu_error e; e << "Input VIX file contains an unexpectedly long "
              "token in its text header.  Header is almost certainly "
              "corrupt or malformed."; }
        }
      if (ch == EOF)
        break;
    }
  if (bp > buffer)
    {
      *bp = '\0';
      return true;
    }
  else
    return false;
}

/*****************************************************************************/
/* INLINE                        read_to_tag                                 */
/*****************************************************************************/

static inline bool
  read_to_tag(FILE *fp, char *buffer, int buffer_len)
{
  while (read_token(fp,buffer,buffer_len))
    if ((buffer[0] == '>') && (buffer[strlen(buffer)-1] == '<'))
      return true;
  return false;
}

/*****************************************************************************/
/* STATIC                       open_vix_file                                */
/*****************************************************************************/

static FILE *
  open_vix_file(char *ifname, kdu_params *siz, int &num_frames,
                kdu_uint32 &timescale, kdu_uint32 &frame_period,
                int &sample_bytes, int &msbs_used,
                bool &is_signed, bool &native_order, bool &is_ycc, bool quiet)
  /* Opens the VIX (or YUV) input file, reading its header and returning a
     file pointer from which the sample values can be read. */
{
  double frame_rate = 1.0; // Default frame rate
  num_frames = 0; // Default number of frames
  sample_bytes = 0; // In case we forget to set it
  msbs_used = 0; // In case we forget to set it
  is_ycc = false;
  is_signed = false;
  native_order = true;
  FILE *fp = fopen(ifname,"rb");
  if (fp == NULL)
    { kdu_error e; e << "Unable to open input file, \"" << ifname << "\"."; }
  if (check_yuv_suffix(ifname))
    { 
      int height=0, width=0;
      const char *format = parse_yuv_format(ifname,height,width,frame_rate);
      if (format == NULL)
        { kdu_error e; e << "YUV input filename must contain format and "
          "dimensions -- see `-i' in the usage statement.";
          fclose(fp);
        }
      sample_bytes = 1;
      msbs_used = 8;
      is_ycc = true;
      native_order = true;
      is_signed = false;
      siz->set(Ssize,0,0,height); siz->set(Ssize,0,1,width);
      siz->set(Scomponents,0,0,3);
      for (int c=0; c < 3; c++)
        { 
          siz->set(Ssigned,c,0,false);
          siz->set(Sprecision,c,0,8);
          int sub_y=1, sub_x=1;
          if (c > 0)
            { 
              if (strcmp(format,"420") == 0)
                sub_x = sub_y = 2;
              else if (strcmp(format,"422") == 0)
                sub_x = 2;
            }
          siz->set(Ssampling,c,0,sub_y); siz->set(Ssampling,c,1,sub_x);
        }
      siz->finalize();
    }
  else
    { 
      int height=0, width=0, components=0;
      int native_order_is_big = 1; ((kdu_byte *)(&native_order_is_big))[0] = 0;
      char buffer[64];
      if ((fread(buffer,1,3,fp) != 3) ||
          (buffer[0] != 'v') || (buffer[1] != 'i') || (buffer[2] != 'x'))
        { kdu_error e; e << "The input file, \"" << ifname << "\", does not "
          "commence with the magic string, \"vix\"."; }
      while (read_to_tag(fp,buffer,64))
        if (strcmp(buffer,">VIDEO<") == 0)
          { 
            if ((!read_token(fp,buffer,64)) ||
                (sscanf(buffer,"%lf",&frame_rate) != 1) ||
                (!read_token(fp,buffer,64)) ||
                (sscanf(buffer,"%d",&num_frames) != 1) ||
                (frame_rate <= 0.0) || (num_frames < 0))
              { kdu_error e; e << "Malformed \">VIDEO<\" tag found in "
                "VIX input file.  Tag requires two numeric fields: a real-"
                "valued positive frame rate; and a non-negative number of "
                "frames."; }
          }
        else if (strcmp(buffer,">COLOUR<") == 0)
          { 
            if ((!read_token(fp,buffer,64)) ||
                (strcmp(buffer,"RGB") && strcmp(buffer,"YCbCr")))
              { kdu_error e; e << "Malformed \">COLOUR<\" tag found in "
                "VIX input file.  Tag requires a single token, with one of "
                "the strings, \"RGB\" or \"YCbCr\"."; }
            if (strcmp(buffer,"YCbCr") == 0)
              is_ycc = true;
          }
        else if (strcmp(buffer,">IMAGE<") == 0)
          { 
            if ((!read_token(fp,buffer,64)) ||
                (strcmp(buffer,"unsigned") && strcmp(buffer,"signed")))
              { kdu_error e; e << "Malformed \">IMAGE<\" tag found in VIX "
                "input file.  First token in tag must be one of the strings, "
                "\"signed\" or \"unsigned\"."; }
            is_signed = (strcmp(buffer,"signed") == 0);

            if ((!read_token(fp,buffer,64)) ||
                (strcmp(buffer,"char") && strcmp(buffer,"word") &&
                 strcmp(buffer,"dword")))
              { kdu_error e; e << "Malformed \">IMAGE<\" tag found in VIX "
                "input file.  Second token in tag must be one of the strings, "
                "\"char\", \"word\" or \"dword\"."; }
            if (strcmp(buffer,"char") == 0)
              sample_bytes = 1;
            else if (strcmp(buffer,"word") == 0)
              sample_bytes = 2;
            else
              sample_bytes = 4;

            if ((!read_token(fp,buffer,64)) ||
                (sscanf(buffer,"%d",&msbs_used) != 1) ||
                (msbs_used < 1) || (msbs_used > (8*sample_bytes)))
              { kdu_error e; e << "Malformed  \">IMAGE<\" tag found in VIX "
                "input file.  Third token in tag must hold the number of "
                "MSB's used in each sample word, a quantity in the range 1 "
                "through to the number of bits in the sample word."; }

            if ((!read_token(fp,buffer,64)) ||
                (strcmp(buffer,"little-endian") &&
                 strcmp(buffer,"big-endian")))
              { kdu_error e; e << "Malformed \">IMAGE<\" tag found in VIX "
                "input file.  Fourth token in tag must hold one of the "
                "strings \"little-endian\" or \"big-endian\"."; }
            if (strcmp(buffer,"little-endian") == 0)
              native_order = (native_order_is_big)?false:true;
            else
              native_order = (native_order_is_big)?true:false;

            if ((!read_token(fp,buffer,64)) ||
                (sscanf(buffer,"%d",&width) != 1) ||
                (!read_token(fp,buffer,64)) ||
                (sscanf(buffer,"%d",&height) != 1) ||
                (!read_token(fp,buffer,64)) ||
                (sscanf(buffer,"%d",&components) != 1) ||
                (width <= 0) || (height <= 0) || (components <= 0))
              { kdu_error e; e << "Malformed \">IMAGE<\" tag found in VIX "
                "input file.  Fifth through seventh tags must hold positive "
                "values for the width, height and number of components in "
                "each frame, respectively."; }

            siz->set(Ssize,0,0,height); siz->set(Ssize,0,1,width);
            siz->set(Scomponents,0,0,components);
            for (int c=0; c < components; c++)
              { 
                siz->set(Ssigned,c,0,is_signed);
                siz->set(Sprecision,c,0,msbs_used);
                int sub_y, sub_x;
                if ((!read_token(fp,buffer,64)) ||
                    (sscanf(buffer,"%d",&sub_x) != 1) ||
                    (!read_token(fp,buffer,64)) ||
                    (sscanf(buffer,"%d",&sub_y) != 1) ||
                    (sub_x < 1) || (sub_x > 255) ||
                    (sub_y < 1) || (sub_y > 255))
                  { kdu_error e; e << "Malformed \">IMAGE<\" tag found in VIX "
                    "input file.  Horizontal and vertical sub-sampling "
                    "factors in the range 1 to 255 must appear for each "
                    "image component."; }
                siz->set(Ssampling,c,0,sub_y); siz->set(Ssampling,c,1,sub_x);
              }
            siz->finalize();
            break;
          }
        else if (!quiet)
          { kdu_warning w; w << "Unrecognized tag, \"" << buffer << "\", "
            "found in VIX input file."; }
      if (sample_bytes == 0)
        { kdu_error e; e << "Input VIX file does not contain the mandatory "
          "\">IMAGE<\" tag."; }
      if (components < 3)
        is_ycc = false;

      // Read past new-line character which separates header from data
      int ch;
      while (((ch = fgetc(fp)) != EOF) && (ch != '\n'));
    }
  
  // Convert frame rate to a suitable timescale/frame period combination
  kdu_uint32 ticks_per_second, best_ticks;
  kdu_uint32 period, best_period;
  double exact_period = 1.0 / frame_rate;
  double error, best_error=1000.0; // Ridiculous value
  for (ticks_per_second=1000; ticks_per_second < 2000; ticks_per_second++)
    {
      period = (kdu_uint32)(exact_period*ticks_per_second + 0.5);
      error = fabs(exact_period-((double) period)/((double) ticks_per_second));
      if (error < best_error)
        {
          best_error = error;
          best_period = period;
          best_ticks = ticks_per_second;
        }
    }
  timescale = best_ticks;
  frame_period = best_period;

  return fp;
}

/*****************************************************************************/
/* STATIC                        get_bpp_dims                                */
/*****************************************************************************/

static kdu_long
  get_bpp_dims(siz_params *siz)
{
  int comps, max_width, max_height, n;

  siz->get(Scomponents,0,0,comps);
  max_width = max_height = 0;
  for (n=0; n < comps; n++)
    {
      int width, height;
      siz->get(Sdims,n,0,height);
      siz->get(Sdims,n,1,width);
      if (width > max_width)
        max_width = width;
      if (height > max_height)
        max_height = height;
    }
  return ((kdu_long) max_height) * ((kdu_long) max_width);
}

/*****************************************************************************/
/* STATIC                      assign_layer_bytes                            */
/*****************************************************************************/

static kdu_long *
  assign_layer_bytes(kdu_args &args, siz_params *siz, int &num_specs)
  /* Returns a pointer to an array of `num_specs' quality layer byte
     targets.  The value of `num_specs' is determined in this function, based
     on the number of rates (or slopes) specified on the command line,
     together with any knowledge about the number of desired quality layers.
     Before calling this function, you must parse all parameter attribute
     strings into the code-stream parameter lists rooted at `siz'.  Note that
     the returned array will contain 0's whenever a quality layer's
     bit-rate is unspecified.  This allows the compressor's rate allocator to
     assign the target size for those quality layers on the fly. */
{
  char *cp;
  char *string = NULL;
  int arg_specs = 0;
  int slope_specs = 0;
  int cod_specs = 0;

  if (args.find("-slope") != NULL)
    {
      string = args.advance(false); // Need to process this arg again later.
      if (string != NULL)
        {
          while (string != NULL)
            {
              slope_specs++;
              string = strchr(string+1,',');
            }
        }
    }

  // Determine how many rates are specified on the command-line
  if (args.find("-rate") != NULL)
    {
      string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-rate\" argument must be followed by a "
          "string identifying one or more bit-rates, separated by commas."; }
      cp = string;
      while (cp != NULL)
        {
          arg_specs++;
          cp = strchr(cp,',');
          if (cp != NULL)
            cp++;
        }
    }

  // Find the number of layers specified by the main COD marker

  kdu_params *cod = siz->access_cluster(COD_params);
  assert(cod != NULL);
  cod->get(Clayers,0,0,cod_specs);
  if (!cod_specs)
    cod_specs = (arg_specs>slope_specs)?arg_specs:slope_specs;
  num_specs = cod_specs;
  if (num_specs == 0)
    num_specs = 1;
  if ((arg_specs != num_specs) &&
      ((arg_specs > 2) || ((arg_specs == 2) && (num_specs == 1))))
    { kdu_error e; e << "The relationship between the number of bit-rates "
      "specified by the \"-rate\" argument and the number of quality layers "
      "explicitly specified via \"Clayers\" does not conform to the rules "
      "supplied in the description of the \"-rate\" argument.  Use \"-u\" "
      "to print the usage statement."; }
  cod->set(Clayers,0,0,num_specs);
  int n;
  kdu_long *result = new kdu_long[num_specs];
  for (n=0; n < num_specs; n++)
    result[n] = 0;

  kdu_long total_pels = get_bpp_dims(siz);
  bool have_dash = false;
  for (n=0; n < arg_specs; n++)
    {
      cp = strchr(string,',');
      if (cp != NULL)
        *cp = '\0'; // Temporarily terminate string.
      if (strcmp(string,"-") == 0)
        { have_dash = true; result[n] = KDU_LONG_MAX; }
      else
        {
          double bpp;
          if ((!sscanf(string,"%lf",&bpp)) || (bpp <= 0.0))
            { kdu_error e; e << "Illegal sub-string encoutered in parameter "
              "string supplied to the \"-rate\" argument.  Rate parameters "
              "must be strictly positive real numbers, with multiple "
              "parameters separated by commas only.  Problem encountered at "
              "sub-string: \"" << string << "\"."; }
          result[n] = (kdu_long) floor(bpp * 0.125 * total_pels);
        }
      if (cp != NULL)
        { *cp = ','; string = cp+1; }
    }

  if (arg_specs)
    { // Bubble sort the supplied specs.
      bool done = false;
      while (!done)
        { // Use trivial bubble sort.
          done = true;
          for (int n=1; n < arg_specs; n++)
            if (result[n-1] > result[n])
              { // Swap misordered pair.
                kdu_long tmp=result[n];
                result[n]=result[n-1];
                result[n-1]=tmp;
                done = false;
              }
        }
    }

  if (arg_specs && (arg_specs != num_specs))
    { // Arrange for specified rates to identify max and/or min layer rates
      assert((arg_specs < num_specs) && (arg_specs <= 2));
      result[num_specs-1] = result[arg_specs-1];
      result[arg_specs-1] = 0;
    }

  if (have_dash)
    { // Convert final rate target of KDU_LONG_MAX into 0 (forces rate
      // allocator to assign all remaining compressed bits to that layer.)
      assert(result[num_specs-1] == KDU_LONG_MAX);
      result[num_specs-1] = 0;
    }

  if (string != NULL)
    args.advance();
  return result;
}

/*****************************************************************************/
/* STATIC                      assign_layer_thresholds                            */
/*****************************************************************************/

static kdu_uint16 *
  assign_layer_thresholds(kdu_args &args, int num_specs)
  /* Returns a pointer to an array of `num_specs' slope threshold values,
     all of which are set to 0 unless the command-line arguments contain
     an explicit request for particular distortion-length slope thresholds. */
{
  int n;
  kdu_uint16 *result = new kdu_uint16[num_specs];

  for (n=0; n < num_specs; n++)
    result[n] = 0;
  if (args.find("-slope") == NULL)
    return result;
  char *string = args.advance();
  if (string == NULL)
    { kdu_error  e; e << "The `-slope' argument must be followed by a "
      "comma-separated list of slope values."; }
  for (n=0; (n < num_specs) && (string != NULL); n++)
    {
      char *delim = strchr(string,',');
      if (delim != NULL)
        { *delim = '\0'; delim++; }
      int val;
      if ((sscanf(string,"%d",&val) != 1) || (val < 0) || (val > 65535))
        { kdu_error e; e << "The `-slope' argument must be followed by a "
          "comma-separated  list of integer distortion-length slope values, "
          "each of which must be in the range 0 to 65535, inclusive."; }
      result[n] = (kdu_uint16) val;
      string = delim;
    }

  // Now sort the entries into decreasing order.
  int k;
  if (n > 1)
    {
      bool done = false;
      while (!done)
        { // Use trivial bubble sort.
          done = true;
          for (k=1; k < n; k++)
            if (result[k-1] < result[k])
              { // Swap misordered pair.
                kdu_uint16 tmp=result[k];
                result[k]=result[k-1];
                result[k-1]=tmp;
                done = false;
              }
        }
    }
  
  // Fill in any remaining missing values.
  for (k=n; k < num_specs; k++)
    result[k] = result[n-1];
  args.advance();
  return result;
}

/*****************************************************************************/
/* STATIC                  set_default_colour_weights                        */
/*****************************************************************************/

static void
  set_default_colour_weights(kdu_params *siz, bool is_ycc, bool quiet)
  /* If the data to be compressed already has a YCbCr representation,
     (`is_ycc' is true) or the code-stream colour transform is to be used,
     this function sets appropriate weights for the luminance and
     chrominance components.  The weights are taken from the Motion JPEG2000
     standard (ISO/IEC 15444-3).  Otherwise, no weights are used. */
{
  kdu_params *cod = siz->access_cluster(COD_params);
  assert(cod != NULL);

  float weight;
  if (cod->get(Clev_weights,0,0,weight) ||
      cod->get(Cband_weights,0,0,weight))
    return; // Weights already specified explicitly.
  bool can_use_ycc = !is_ycc;
  bool rev0=false;
  int c, depth0=0, sub_x0=1, sub_y0=1;
  for (c=0; c < 3; c++)
    {
      int depth=0; siz->get(Sprecision,c,0,depth);
      int sub_y=1; siz->get(Ssampling,c,0,sub_y);
      int sub_x=1; siz->get(Ssampling,c,1,sub_x);
      kdu_params *coc = cod->access_relation(-1,c,0,true);
      if (coc->get(Clev_weights,0,0,weight) ||
          coc->get(Cband_weights,0,0,weight))
        return;
      bool rev=false; coc->get(Creversible,0,0,rev);
      if (c == 0)
        { rev0=rev; depth0=depth; sub_x0=sub_x; sub_y0=sub_y; }
      else if ((rev != rev0) || (depth != depth0) ||
               (sub_x != sub_x0) || (sub_y != sub_y0))
        can_use_ycc = false;
    }
  bool use_ycc = can_use_ycc;
  if (can_use_ycc && !cod->get(Cycc,0,0,use_ycc))
    cod->set(Cycc,0,0,use_ycc=true);
  if (!(use_ycc || is_ycc))
    return;

  for (c=0; c < 3; c++)
    {
      kdu_params *coc = cod->access_relation(-1,c,0,false);
      int sub_y=1; siz->get(Ssampling,c,0,sub_y);
      int sub_x=1; siz->get(Ssampling,c,1,sub_x);
      
      double weight;
      int b=0;
      while ((sub_y > 1) && (sub_x > 1))
        { sub_y >>= 1; sub_x >>= 1; b += 3; }
      if (c == 0)
        for (; b < 9; b++)
          {
            switch (b) {
               case 0:         weight = 0.090078; break;
               case 1: case 2: weight = 0.275783; break;
               case 3:         weight = 0.701837; break;
               case 4: case 5: weight = 0.837755; break;
               case 6:         weight = 0.999988; break;
               case 7: case 8: weight = 0.999994; break;
              }
            coc->set(Cband_weights,b,0,weight);
          }
      else if (c == 1)
        for (; b < 15; b++)
          {
            switch (b) {
                case 0:           weight = 0.027441; break;
                case 1:  case 2:  weight = 0.089950; break;
                case 3:           weight = 0.141965; break;
                case 4:  case 5:  weight = 0.267216; break;
                case 6:           weight = 0.348719; break;
                case 7:  case 8:  weight = 0.488887; break;
                case 9:           weight = 0.567414; break;
                case 10: case 11: weight = 0.679829; break;
                case 12:          weight = 0.737656; break;
                case 13: case 14: weight = 0.812612; break;
              }
            coc->set(Cband_weights,b,0,weight);
          }
      else
        for (; b < 15; b++)
          {
            switch (b) {
                case 0:           weight = 0.070185; break;
                case 1:  case 2:  weight = 0.166647; break;
                case 3:           weight = 0.236030; break;
                case 4:  case 5:  weight = 0.375136; break;
                case 6:           weight = 0.457826; break;
                case 7:  case 8:  weight = 0.587213; break;
                case 9:           weight = 0.655884; break;
                case 10: case 11: weight = 0.749805; break;
                case 12:          weight = 0.796593; break;
                case 13: case 14: weight = 0.856065; break;
              }
            coc->set(Cband_weights,b,0,weight);
          }
    }

  if (!quiet)
    pretty_cout << "Note:\n\tThe default rate control policy for colour "
                   "video employs visual (CSF) weighting factors.  To "
                   "minimize MSE, instead of visually weighted MSE, "
                   "specify `-no_weights'.\n";
}

/*****************************************************************************/
/* STATIC                  set_mj2_video_attributes                          */
/*****************************************************************************/

static void
  set_mj2_video_attributes(mj2_video_target *video, kdu_params *siz,
                           bool is_ycc)
{
  // Set colour attributes
  jp2_colour colour = video->access_colour();
  int num_components; siz->get(Scomponents,0,0,num_components);
  if (num_components >= 3)
    colour.init((is_ycc)?JP2_sYCC_SPACE:JP2_sRGB_SPACE);
  else
    colour.init(JP2_sLUM_SPACE);
}

/*****************************************************************************/
/* STATIC                   transfer_bytes_to_line                           */
/*****************************************************************************/

static void
  transfer_bytes_to_line(void *src, kdu_line_buf line, int width,
                         int msbs_used, bool is_signed)
{
  kdu_byte *sp = (kdu_byte *) src;
  if (line.get_buf16() != NULL)
    {
      kdu_sample16 *dp = line.get_buf16();
      if (line.is_absolute())
        {
          int downshift = 8-msbs_used;
          if (is_signed)
            for (; width--; dp++, sp++)
              dp->ival = ((kdu_int16) *sp) >> downshift;
          else
            for (; width--; dp++, sp++)
              dp->ival = (((kdu_int16) *sp) - 128) >> downshift;
        }
      else
        {
          if (is_signed)
            for (; width--; dp++, sp++)
              dp->ival = ((kdu_int16) *sp) << (KDU_FIX_POINT-8);
          else
            for (; width--; dp++, sp++)
              dp->ival = (((kdu_int16) *sp) - 128) << (KDU_FIX_POINT-8);
        }
    }
  else
    {
      kdu_sample32 *dp = line.get_buf32();
      if (line.is_absolute())
        {
          int downshift = 8-msbs_used;
          if (is_signed)
            for (; width--; dp++, sp++)
              dp->ival = ((kdu_int32) *sp) >> downshift;
          else
            for (; width--; dp++, sp++)
              dp->ival = (((kdu_int32) *sp) - 128) >> downshift;
        }
      else
        {
          if (is_signed)
            for (; width--; dp++, sp++)
              dp->fval = ((float) *sp) * (1.0F/256.0F);
          else
            for (; width--; dp++, sp++)
              dp->fval = ((float) *sp) * (1.0F/256.0F) - 0.5F;
        }
    }
}

/*****************************************************************************/
/* STATIC                   transfer_words_to_line                           */
/*****************************************************************************/

static void
  transfer_words_to_line(void *src, kdu_line_buf line, int width,
                         int msbs_used, bool is_signed)
{
  kdu_int16 *sp = (kdu_int16 *) src;
  if (line.get_buf16() != NULL)
    {
      kdu_sample16 *dp = line.get_buf16();
      int downshift = 16-msbs_used;
      if (!line.is_absolute())
        downshift = 16-KDU_FIX_POINT;
      if (is_signed)
        for (; width--; dp++, sp++)
          dp->ival = (*sp) >> downshift;
      else
        for (; width--; dp++, sp++)
          dp->ival = ((kdu_int16)(*sp - 0x8000)) >> downshift;
    }
  else
    {
      kdu_sample32 *dp = line.get_buf32();
      if (line.is_absolute())
        {
          int downshift = 16-msbs_used;
          if (is_signed)
            for (; width--; dp++, sp++)
              dp->ival = ((kdu_int32) *sp) >> downshift;
          else
            for (; width--; dp++, sp++)
              dp->ival = (kdu_int32)(((kdu_int16)(*sp - 0x8000)) >> downshift);
        }
      else
        {
          if (is_signed)
            for (; width--; dp++, sp++)
              dp->fval = ((float) *sp) * (1.0F / (float)(1<<16));
          else
            for (; width--; dp++, sp++)
              dp->fval = (*((kdu_uint16 *) sp)) * (1.0F/(1<<16)) - 0.5F;
        }
    }
}


/* ========================================================================= */
/*                               kdv_compressor                              */
/* ========================================================================= */

/*****************************************************************************/
/*                        kdv_compressor::kdv_compressor                     */
/*****************************************************************************/

kdv_compressor::kdv_compressor(kdu_codestream codestream, int sample_bytes,
                               bool native_order, bool is_signed,
                               int num_layer_specs, kdu_thread_env *env)
{
  int c;

  this->env = env;
  this->sample_bytes = sample_bytes;
  this->native_order = native_order;
  this->is_signed = is_signed;
  frame_bytes = 0; // We will grow this in a little while
  buffer = NULL; // We will allocate this after determining `frame_bytes'
  num_components = codestream.get_num_components();
  components = new kdv_component[num_components];
  for (c=0; c < num_components; c++)
    {
      kdv_component *comp = components + c;
      codestream.get_dims(c,comp->comp_dims);
      kdu_coords subs; codestream.get_subsampling(c,subs);
      comp->count_val = subs.y;
      frame_bytes += sample_bytes * (int)  comp->comp_dims.area();
      comp->env_queue = NULL;
      if (env != NULL)
        comp->env_queue = env->add_queue(NULL,NULL,"tile-component root");
    }

  // Allocate buffer and fill in component pointers
  buffer = new kdu_byte[frame_bytes];
  kdu_byte *comp_buf = buffer;
  for (c=0; c < num_components; c++)
    {
      kdv_component *comp = components + c;
      comp->comp_buf = comp_buf;
      comp_buf += sample_bytes * (int) comp->comp_dims.area();
    }

  // Allocate storage for the per-frame layer bytes and slope thresholds
  this->num_layer_specs = num_layer_specs;
  layer_bytes = new kdu_long[num_layer_specs];
  layer_thresholds = new kdu_uint16[num_layer_specs];
}

/*****************************************************************************/
/*                          kdv_compressor::load_frame                       */
/*****************************************************************************/

bool
  kdv_compressor::load_frame(FILE *fp)
{
  int num_bytes = (int) fread(buffer,1,(size_t) frame_bytes,fp);
  if (num_bytes == frame_bytes)
    {
      if ((!native_order) && (sample_bytes > 1))
        { // Reorder bytes in each sample word
          int n = frame_bytes / sample_bytes;
          kdu_byte tmp, *bp = buffer;
          if (sample_bytes == 2)
            for (; n--; bp += 2)
              {
                tmp=bp[0]; bp[0]=bp[1]; bp[1]=tmp;
              }
          else if (sample_bytes == 4)
            for (; n--; bp += 4)
              {
                tmp=bp[0]; bp[0]=bp[3]; bp[3]=tmp;
                tmp=bp[1]; bp[1]=bp[2]; bp[2]=tmp;
              }
          else
            assert(0);
        }
      return true;
    }
  else if (num_bytes > 0)
    { kdu_error w; w << "The input VIX file does not contain a whole number "
      "of frames, suggesting that the file has been incorrectly formatted."; }
  return false;
}

/*****************************************************************************/
/*                        kdv_compressor::process_frame                      */
/*****************************************************************************/

kdu_uint16
  kdv_compressor::process_frame(kdu_compressed_video_target *video,
                                kdu_codestream codestream,
                                kdu_long layer_bytes[],
                                kdu_uint16 layer_thresholds[],
                                bool trim_to_rate, bool no_info)
{
  int n, c;
  kdu_dims tile_indices;
  kdu_coords idx;
  kdv_component *comp;

  // Copy quality layer specifications
  for (n=0; n < num_layer_specs; n++)
    {
      this->layer_bytes[n] = layer_bytes[n];
      this->layer_thresholds[n] = layer_thresholds[n];
    }

  // Process tiles one by one
  codestream.get_valid_tiles(tile_indices);
  for (idx.y=0; idx.y < tile_indices.size.y; idx.y++)
    for (idx.x=0; idx.x < tile_indices.size.x; idx.x++)
      {
        kdu_tile tile = codestream.open_tile(tile_indices.pos + idx);
        bool use_ycc = tile.get_ycc();
        kdu_tile_comp tc;
        kdu_resolution res;
        bool reversible, use_shorts;

        // Initialize tile-components
        for (comp=components, c=0; c < num_components; c++, comp++)
          {
            tc = tile.access_component(c);
            res = tc.access_resolution();
            comp->counter = 0;
            comp->precision = tc.get_bit_depth();
            reversible = tc.get_reversible();
            res.get_dims(comp->tile_dims);
            assert((comp->tile_dims & comp->comp_dims) == comp->tile_dims);
            comp->bp = comp->comp_buf + sample_bytes *
              ((comp->tile_dims.pos.x - comp->comp_dims.pos.x) +
               (comp->tile_dims.pos.y - comp->comp_dims.pos.y) *
               comp->comp_dims.size.x);
            use_shorts = (tc.get_bit_depth(true) <= 16);
            if (res.which() == 0)
              comp->engine =
                kdu_encoder(res.access_subband(LL_BAND),&allocator,
                            use_shorts,1.0F,NULL,env,comp->env_queue);
            else
              comp->engine =
                kdu_analysis(res,&allocator,use_shorts,1.0F,NULL,
                             env,comp->env_queue);
            comp->line.pre_create(&allocator,comp->tile_dims.size.x,
                                  reversible,use_shorts);
          }

        // Complete sample buffer allocation
        allocator.finalize();
        for (comp=components, c=0; c < num_components; c++, comp++)
          comp->line.create();

        // Now for the processing
        bool empty, done = false;
        while (!done)
          {
            done = empty = true;

            // Load line buffers for each component whose counter is 0
            for (comp=components, c=0; c < num_components; c++, comp++)
              if (comp->tile_dims.size.y > 0)
                {
                  done = false;
                  comp->counter--;
                  if (comp->counter >= 0)
                    continue;
                  empty = false;
                  int width = comp->tile_dims.size.x;
                  if (sample_bytes == 1)
                    {
                      transfer_bytes_to_line(comp->bp,comp->line,width,
                                             comp->precision,is_signed);
                      comp->bp += comp->comp_dims.size.x;
                    }
                  else if (sample_bytes == 2)
                    {
                      transfer_words_to_line(comp->bp,comp->line,width,
                                             comp->precision,is_signed);
                      comp->bp += (comp->comp_dims.size.x << 1);
                    }
                  else
                    { kdu_error e; e << "The VIX reader provided here "
                      "currently supports only \"byte\" and \"word\" data "
                      "types as sample data containers.   The \"dword\" "
                      "data type could easily be added if necessary."; }
                }
            if (empty)
              continue;

            // Process newly loaded line buffers
            if (use_ycc && (components[0].counter < 0))
              {
                assert((components[1].counter < 0) &&
                       (components[2].counter < 0));
                kdu_convert_rgb_to_ycc(components[0].line,
                                       components[1].line,
                                       components[2].line);
              }
            for (comp=components, c=0; c < num_components; c++, comp++)
              if (comp->counter < 0)
                {
                  comp->engine.push(comp->line,env);
                  comp->tile_dims.size.y--;
                  comp->counter += comp->count_val;
                }
          }

        // Cleanup and advance to next tile position
        for (comp=components, c=0; c < num_components; c++, comp++)
          {
            if (env != NULL)
              env->terminate(comp->env_queue,true);
            comp->engine.destroy();
            comp->line.destroy();
          }
        tile.close();
        allocator.restart();
      }
  
  codestream.flush(this->layer_bytes,num_layer_specs,
                   this->layer_thresholds,trim_to_rate,!no_info);

  kdu_uint16 min_slope = this->layer_thresholds[num_layer_specs-1];
  if (layer_bytes[num_layer_specs-1] > 0)
    { // See how close to the target size we came.  If we are a long way
      // off, adjust `min_slope' to help ensure that we do not excessively
      // truncate the code-block generation process in subsequent frames.
      // This is only a heuristic.
      kdu_long target = layer_bytes[num_layer_specs-1];
      kdu_long actual = this->layer_bytes[num_layer_specs-1];
      kdu_long gap = target - actual;
      if (gap > actual)
        min_slope = 0;
      else
        while (gap > (target>>4))
          {
            gap >>= 1;
            if (min_slope < 64)
              { min_slope = 0; break; }
            min_slope -= 64;
          }
    }
  return min_slope;
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

  // Collect simple arguments.
  char *ifname = NULL;
  char *ofname = NULL;
  int max_frames = INT_MAX;
  int subsampling_factor = 1;
  int num_threads;
  bool no_slope_predict, no_info, no_weights, cpu, quiet;
  parse_simple_arguments(args,ifname,ofname,max_frames,subsampling_factor,
                         no_slope_predict,no_info,no_weights,
                         num_threads,cpu,quiet);
  
  // Collect any parameters relevant to the SIZ marker segment
  siz_params siz;
  const char *string;
  for (string=args.get_first(); string != NULL; )
    string = args.advance(siz.parse_string(string));

  // Open input file and collect dimensional information
  int sample_bytes, msbs_used, num_frames;
  bool is_signed, is_ycc, native_order;
  kdu_uint32 timescale, frame_period;
  FILE *vix_file =
    open_vix_file(ifname,&siz,num_frames,timescale,frame_period,
                  sample_bytes,msbs_used,is_signed,native_order,is_ycc,quiet);
  frame_period *= (kdu_uint32) subsampling_factor;
  num_frames /= subsampling_factor;

  // Create the compressed data target.
  kdu_compressed_video_target *video;

  jp2_family_tgt mj2_ultimate_tgt;
  mj2_target movie;
  mj2_video_target *mj2_video = NULL;
  kdu_simple_video_target mjc_video;

  if (check_mj2_suffix(ofname))
    {
      mj2_ultimate_tgt.open(ofname);
      movie.open(&mj2_ultimate_tgt);
      mj2_video = movie.add_video_track();
      mj2_video->set_timescale(timescale);
      mj2_video->set_frame_period(frame_period);
      video = mj2_video;
    }
  else if (check_mjc_suffix(ofname))
    {
      mjc_video.open(ofname,timescale,frame_period,
                     (is_ycc)?KDU_SIMPLE_VIDEO_YCC:KDU_SIMPLE_VIDEO_RGB);
      video = &mjc_video;
    }
  else
    { kdu_error e; e << "Output file must have one of the suffices "
      "\".mj2\" or \".mjc\".  See usage statement for more information."; }

  // Create a code-stream management object.
  kdu_codestream codestream;
  codestream.create(&siz,video);
  for (string=args.get_first(); string != NULL; )
    string = args.advance(codestream.access_siz()->parse_string(string));
  int num_layers = 0;
  kdu_long *layer_bytes =
    assign_layer_bytes(args,codestream.access_siz(),num_layers);
  kdu_uint16 *layer_thresholds =
    assign_layer_thresholds(args,num_layers);
  if ((num_layers < 2) && !quiet)
    pretty_cout << "Note:\n\tIf you want quality scalability, you should "
                   "generate multiple layers with `-rate' or by using "
                   "the \"Clayers\" option.\n";
  if ((codestream.get_num_components() >= 3) && (!no_weights))
    set_default_colour_weights(codestream.access_siz(),is_ycc,quiet);
  if (mj2_video != NULL)
    set_mj2_video_attributes(mj2_video,codestream.access_siz(),is_ycc);
  codestream.access_siz()->finalize_all();
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

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

  // Construct sample processing machinery and process frames one by one.
  codestream.enable_restart();
  kdv_compressor compressor(codestream,sample_bytes,native_order,
                            is_signed,num_layers,env_ref);
  int s, frame_count = 0;
  double processing_time = 0.0;
  double file_time = 0.0;
  kdu_uint16 previous_min_slope=0;
  do {
      clock_t start_time = clock();
      for (s=subsampling_factor; s > 0; s--)
        if (!compressor.load_frame(vix_file))
          break;
      if (s > 0)
        break; // No more frames available.

      clock_t load_time = clock();

      if (frame_count)
        codestream.restart(video);
      video->open_image();
      if ((num_layers > 0) && (layer_thresholds[num_layers-1] > 0))
        codestream.set_min_slope_threshold(layer_thresholds[num_layers-1]);
      else if ((num_layers > 0) && (layer_bytes[num_layers-1] > 0))
        {
          if ((previous_min_slope != 0) && !no_slope_predict)
            codestream.set_min_slope_threshold(previous_min_slope);
          else
            codestream.set_max_bytes(layer_bytes[num_layers-1]);
        }
      previous_min_slope =
        compressor.process_frame(video,codestream,
                                 layer_bytes,layer_thresholds,
                                 no_slope_predict,no_info);
      video->close_image(codestream);
      
      file_time += ((double)(load_time-start_time)) / CLOCKS_PER_SEC;
      processing_time += ((double)(clock()-load_time)) / CLOCKS_PER_SEC;

      frame_count++;
      if (!quiet)
        pretty_cout << "Number of frames processed = " << frame_count << "\n";
    } while ((frame_count != num_frames) && (frame_count < max_frames));

  if ((num_frames > 0) && (frame_count < num_frames) &&
      (frame_count < max_frames))
    { kdu_warning w; w << "Managed to read only " << frame_count << " of the "
      << num_frames << " frames advertised by the input VIX file's text "
      "header.  It is likely that a formatting error exists."; }

  // Print summary statistics
  if (cpu && frame_count)
    {
      pretty_cout << "Processing time = "
                  << processing_time/frame_count
                  << " seconds per frame\n";
      pretty_cout << "File reading time = "
                  << file_time/frame_count
                  << " seconds per frame\n";
    }
  if (!quiet)
    {
      if (num_threads == 0)
        pretty_cout << "Processed using the single-threaded environment "
          "(see `-num_threads').\n";
      else
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          << "    " << num_threads << " parallel threads of execution "
          "(see `-num_threads')\n";
    }

  // If multi-threaded processing was used, we must now ensure that the
  // local stash of code buffers managed by each thread is returned to
  // the codestream, before it is destroyed.  This is not necessary during
  // decompression.  One way to accomplish the release of these buffers is
  // to invoke `kdu_thread_entity::terminate' at the root level (i.e.,
  // terminating all queues) -- this ensures that each thread's
  // `kdu_thread_env::on_finished', and it is the approach we use here.
  // Another way to achieve the same goal is to just destroy the environment.
  // However, this is a little dangerous, since then you need to make very
  // sure that the environment gets destroyed before the codestream is
  // destroyed.  For this reason, we propose to use the more explicit
  // `terminate' method here, destroying the multi-threading environment
  // later on, so as to demonstrate the approach.
  if (env.exists())
    env.terminate(NULL,true);

  // Cleanup
  delete[] ifname;
  delete[] ofname;
  delete[] layer_bytes;
  delete[] layer_thresholds;
  fclose(vix_file);
  codestream.destroy();
  video->close();
  movie.close(); // Does nothing if not an MJ2 file.
  if (env.exists())
    env.destroy(); // Destroy multi-threading environment, if one was used.
  return 0;
}
