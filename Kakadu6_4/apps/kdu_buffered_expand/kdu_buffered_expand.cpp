/******************************************************************************/
// File: kdu_stripe_expand.cpp [scope = APPS/BUFFERED_EXPAND]
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
/*******************************************************************************
Description:
   A Kakadu demo application, demonstrating use of the powerful
`kdu_stripe_decompressor' object.
*******************************************************************************/

#include <stdio.h>
#include <iostream>
#include <assert.h>
// Kakadu core includes
#include "kdu_arch.h"
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_stripe_decompressor.h"
// Application includes
#include "kdu_args.h"
#include "kdu_file_io.h"
#include "jp2.h"

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
/*                               kd_output _file                             */
/* ========================================================================= */

struct kd_output_file {
  public: // Lifecycle functions
    kd_output_file()
      { fname=NULL; fp=NULL; bytes_per_sample=1; precision=8;
        is_signed=is_raw=swap_bytes=false; size=kdu_coords(0,0); next=NULL; }
    ~kd_output_file()
      { if (fname != NULL) delete[] fname;
        if (fp != NULL) fclose(fp); }
    void write_pgm_header();
    void write_stripe(int height, kdu_int16 *buf);
  public: // Data
    char *fname;
    FILE *fp;
    int bytes_per_sample;
    int precision; // Num bits
    bool is_signed;
    bool is_raw;
    bool swap_bytes; // If raw file word order differs from machine word order
    kdu_coords size; // Width, and remaining rows
    kd_output_file *next;
  };

/*****************************************************************************/
/*                       kd_output_file::write_pgm_header                    */
/*****************************************************************************/

void
  kd_output_file::write_pgm_header()
{
  fprintf(fp,"P5\n%d %d\n255\n",size.x,size.y);
}

/*****************************************************************************/
/*                         kd_output_file::write_stripe                      */
/*****************************************************************************/

void
  kd_output_file::write_stripe(int height, kdu_int16 *buf)
{
  int n, num_samples = height*size.x;
  int num_bytes = num_samples * bytes_per_sample;
  if (num_samples <= 0) return;
  kdu_int16 val, off = (is_signed)?0:((1<<precision)>>1);
  kdu_int16 *sp = buf;
  if (bytes_per_sample == 1)
    { // Reduce to an 8-bit unsigned representation
      kdu_byte *dp=(kdu_byte *) buf;
      for (n=num_samples; n > 0; n--)
        *(dp++) = (kdu_byte)(*(sp++) + off);
    }
  else if (swap_bytes)
    { // Swap byte order and make representation unsigned if necessary
      for (n=num_samples; n > 0; n--)
        { val = *sp+off; val = (val<<8) + ((val>>8)&0x00FF); *(sp++) = val; }
    }
  else if (off != 0)
    { // Make representation unsigned
      for (n=num_samples; n > 0; n--)
        { val = *sp+off; *(sp++) = val; }
    }
  if (fwrite(buf,1,num_bytes,fp) != (size_t) num_bytes)
    { kdu_error e; e << "Unable to write to file \"" << fname << "\"."; }
}


/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        print_version                               */
/*****************************************************************************/

static void
  print_version()
{
  kdu_message_formatter out(&cout_message);
  out.start_message();
  out << "This is Kakadu's \"kdu_buffered_expand\" demo application.\n";
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
  out << "-i <compressed file>\n";
  if (comprehensive)
    out << "\tCurrently accepts raw code-stream files and code-streams "
           "wrapped in any JP2 compatible file format.  The file signature "
           "is used to check whether or not the file is a raw codestream, "
           "rather than relying upon a file name suffix.\n";
  out << "-o <PGM/raw file 1>[,<PGM/raw file 2>[,...]]\n";
  if (comprehensive)
    out << "\tIf you omit this argument, all image components will be fully "
           "decompressed into memory stripe buffers internally, but only "
           "the final file writing phase will be omitted.  This is very useful "
           "if you want to evaluate the decompression speed (use the `-cpu') "
           "without having the results confounded by file writing -- for large "
           "images, disk I/O accounts for most of the processing time on "
           "modern CPU's.\n"
           "\t   The argument takes one or more output image files.  To "
           "simplify this demo application, each file represents only one "
           "image component, and must be either a PGM file, or a raw file.  As "
           "in the \"kdu_expand\" application, the sample bits in a raw file "
           "are written to the least significant bit positions of an 8 or 16 "
           "bit word, depending on the bit-depth.  For signed data, the word "
           "is sign extended.  The default word organization is big-endian, "
           "regardless of your machine architecture, but this application "
           "allows you to explicitly nominate a different byte order, "
           "via the `-little_endian' argument.\n";
  out << "-rate <bits per pixel>\n";
  if (comprehensive)
    out << "\tMaximum bit-rate, expressed in terms of the ratio between the "
           "total number of compressed bits (including headers) and the "
           "product of the largest horizontal and  vertical image component "
           "dimensions. Note that we use the original dimensions of the "
           "compressed image, regardless or resolution scaling and regions "
           "of interest.  Note CAREFULLY that the file is simply truncated "
           "to the indicated limit, so that the effect of the limit will "
           "depend strongly upon the packet sequencing order used by the "
           "code-stream.  The effect of the byte limit may be modified by "
           "supplying the `-simulate_parsing' flag, described below.\n";
  out << "-simulate_parsing\n";
  if (comprehensive)
    out << "\tIf this flag is supplied, discarded resolutions, image "
           "components or quality layers (see `-reduce' and `-layers') will "
           "not be counted when applying any rate limit supplied via "
           "`-rate' and when reporting overall bit-rates.  The effect is "
           "intended to be the same as if the code-stream were first "
           "parsed to remove the resolutions, components or quality layers "
           "which are not being used.\n";
  out << "-skip_components <num initial image components to skip>\n";
  if (comprehensive)
    out << "\tSkips over one or more initial image components, reconstructing "
           "as many remaining image components as can be stored in the "
           "output image file(s) specified with \"-o\" (or all remaining "
           "components, if no \"-o\" argument is supplied).\n";
  out << "-layers <max layers to decode>\n";
  if (comprehensive)
    out << "\tSet an upper bound on the number of quality layers to actually "
           "decode.\n";
  out << "-reduce <discard levels>\n";
  if (comprehensive)
    out << "\tSet the number of highest resolution levels to be discarded.  "
           "The image resolution is effectively divided by 2 to the power of "
           "the number of discarded levels.\n";
  out << "-int_region {<top>,<left>},{<height>,<width>}\n";
  if (comprehensive)
    out << "\tEstablish a region of interest within the original compressed "
           "image.  Only the region of interest will be decompressed and the "
           "output image dimensions will be modified accordingly.  The "
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
  out << "-min_height <preferred minimum stripe height>\n";
  if (comprehensive)
    out << "\tAllows you to control the processing stripe height which is "
           "preferred in the event that the image is not tiled.  If the image "
           "is tiled, the preferred stripe height is the height of a tile, so "
           "that partially processed tiles need not be buffered.  Otherwise, "
           "the stripes used for incremental processing of the image data "
           "may be as small as 1 line, but it is usually preferable to use "
           "a larger value, as specified here, so as to avoid switching back "
           "and forth between file reading and compression too frequently.  "
           "The default value, for this parameter is 8.  Play around with it "
           "if you want to get the best processing performance.\n";
  out << "-max_height <maximum stripe height>\n";
  if (comprehensive)
    out << "Regardless of the desire to process in stripes whose height is "
           "equal to the tile height, wherever the image is horizontally "
           "tiled, this argument provides an upper bound on the maximum "
           "stripe height.  If the tile height exceeds this value, "
           "an entire row of tiles will be kept open for processing.  This "
           "avoids excessive memory consumption.  This argument allows you "
           "to control the trade-off between stripe buffering and "
           "tile decompression engine memory.  The default limit is 1024.\n";
  out << "-s <switch file>\n";
  if (comprehensive)
    out << "\tSwitch to reading arguments from a file.  In the file, argument "
           "strings are separated by whitespace characters, including spaces, "
           "tabs and new-line characters.  Comments may be included by "
           "introducing a `#' or a `%' character, either of which causes "
           "the remainder of the line to be discarded.  Any number of "
           "\"-s\" argument switch commands may be included on the command "
           "line.\n";
  out << "-little_endian -- use little-endian byte order with raw files\n";
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
           "one actual thread of execution.\n"
           "\t   If the `-num_threads' argument is not supplied explicitly, "
           "the default behaviour is to create a threading environment only "
           "if the system offers multiple CPU's (or virtual CPU's), with "
           "one thread per CPU.  However, this default behaviour depends "
           "upon knowledge of the number of CPU's which are available -- "
           "something which cannot always be accurately determined through "
           "system calls.  The default value might also not yield the "
           "best possible throughput.\n";
  out << "-double_buffering <num double buffered rows>\n";
  if (comprehensive)
    out << "\tThis option may be used only in conjunction with a non-zero "
           "`-num_threads' value.  If `-double_buffering' is not specified, "
           "the DWT operations will all be performed by the single thread "
           "which \"owns\" the multi-threaded processing group.  For a small "
           "number of processors, this may be close to optimal, since the DWT "
           "is generally quite a bit less CPU intensive than block decoding "
           "(which is always spread across multiple threads, if available).  "
           "When `-double_buffering' is specified, a (typically) small number "
           "of image rows (the parameter supplied with this argument) in each "
           "image component are double buffered, so that one set can be "
           "in the process of being generated by the DWT synthesis engines, "
           "while the other set is subjected to colour transformation and "
           "accessed by the application.  Importantly, this allows the DWT "
           "operations associated with each tile-component to be performed "
           "concurrently, in different threads.  The number of double-buffered "
           "rows can be set as small as 1, but this may add quite a bit of "
           "thread context switching overhead.  A value in the range 4 to 16 "
           "is recommended for moderate to large image/tile "
           "widths.\n";
  out << "-cpu -- report processing CPU time\n";
  if (comprehensive)
    out << "\tFor results which more closely reflect the actual decompression "
           "processing time, do not specify any output files via the `-o' "
           "option.  The image is still fully decompressed into memory "
           "buffers, but the final phase of writing the contents of these "
           "buffers to disk files is skipped.  This can have a huge impact "
           "on timing, depending on your platform, and many applications "
           "do not need to write the results to disk.\n";
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

static kd_output_file *
  parse_simple_args(kdu_args &args, char * &ifname, float &max_bpp,
                    bool &simulate_parsing, int &skip_components,
                    int &max_layers, int &discard_levels,
                    kdu_dims &region, int &preferred_min_stripe_height,
                    int &absolute_max_stripe_height,
                    int &num_threads, int &double_buffering_height,
                    bool &cpu)
  /* Parses all command line arguments whose names include a dash.  Returns
     a list of open output files.
        Note that `num_threads' is set to 0 if no multi-threaded processing
     group is to be created, as distinct from a value of 1, which means
     that a multi-threaded processing group is to be used, but this group
     will involve only one thread. */
{
  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  ifname = NULL;
  max_bpp = -1.0F;
  simulate_parsing = false;
  skip_components = 0;
  max_layers = 0;
  discard_levels = 0;
  region.size = region.pos = kdu_coords(0,0);
  preferred_min_stripe_height = 8;
  absolute_max_stripe_height = 1024;
  num_threads = 0; // This is not actually the default -- see below.
  double_buffering_height = 0; // i.e., no double buffering
  cpu = false;
  bool little_endian = false;
  kd_output_file *fhead=NULL, *ftail=NULL;

  if (args.find("-i") != NULL)
    {
      const char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-i\" argument requires a file name!"; }
      ifname = new char[strlen(string)+1];
      strcpy(ifname,string);
      args.advance();
    }
  else
    { kdu_error e; e << "You must supply an input file name."; }

  if (args.find("-rate") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%f",&max_bpp) != 1) ||
          (max_bpp <= 0.0F))
        { kdu_error e; e << "\"-rate\" argument requires a positive "
          "real-valued parameter."; }
      args.advance();
    }
  if (args.find("-simulate_parsing") != NULL)
    {
      simulate_parsing = true;
      args.advance();
    }
  if (args.find("-skip_components") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&skip_components) != 1) ||
          (skip_components < 0))
        { kdu_error e; e << "\"-skip_components\" argument requires a "
          "non-negative integer parameter!"; }
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
  if (args.find("-little_endian") != NULL)
    {
      little_endian = true;
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
      if (num_threads == 0)
        { kdu_error e; e << "\"-double_buffering\" may only be used with "
          "a non-zero `-num_threads' value."; }
      char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&double_buffering_height) != 1) ||
          (double_buffering_height < 1))
        { kdu_error e; e << "\"-double_buffering\" argument requires a "
          "positive integer, specifying the number of rows from each "
          "component which are to be double buffered."; }
      args.advance();
    }

  if (args.find("-cpu") != NULL)
    {
      cpu = true;
      args.advance();
    }
  if (args.find("-min_height") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&preferred_min_stripe_height) != 1) ||
          (preferred_min_stripe_height < 1))
        { kdu_error e; e << "\"-min_height\" argument requires a positive "
          "integer parameter."; }
      args.advance();
    }
  if (args.find("-max_height") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&absolute_max_stripe_height) != 1) ||
          (absolute_max_stripe_height < preferred_min_stripe_height))
        { kdu_error e; e << "\"-max_height\" argument requires a positive "
          "integer parameter, no smaller than the value associated with the "
          "`-min_height' argument."; }
      args.advance();
    }

  if (args.find("-o") != NULL)
    {
      const char *delim, *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-o\" argument requires a parameter string."; }
      for (; *string != '\0'; string=delim)
        {
          while (*string == ',') string++;
          for (delim=string; (*delim != '\0') && (*delim != ','); delim++);
          kd_output_file *file = new kd_output_file;
          if (ftail == NULL)
            fhead = ftail = file;
          else
            ftail = ftail->next = file;
          file->fname = new char[(delim-string)+1];
          strncpy(file->fname,string,delim-string);
          file->fname[delim-string] = '\0';
          if ((file->fp = fopen(file->fname,"wb")) == NULL)
            { kdu_error e;
              e << "Unable to open output file, \"" << file->fname << "\"."; }
        }
      args.advance();
    }

  // Go through file list, setting `is_raw' and `swap_bytes'
  for (ftail=fhead; ftail != NULL; ftail=ftail->next)
    {
      ftail->is_raw = ftail->swap_bytes = false;
      const char *delim = strrchr(ftail->fname,'.');
      if ((delim == NULL) || (toupper(delim[1]) != (int) 'P') ||
          (toupper(delim[2]) != (int) 'G') || (toupper(delim[3]) != (int) 'M'))
        {
          ftail->is_raw = true;
          int is_bigendian=1;
          ((kdu_byte *) &is_bigendian)[0] = 0;
          if (is_bigendian)
            ftail->swap_bytes = little_endian;
          else
            ftail->swap_bytes = !little_endian;
        }
    }
  return fhead;
}

/*****************************************************************************/
/* STATIC                     check_jp2_family_file                          */
/*****************************************************************************/

static bool
  check_jp2_family_file(const char *fname)
  /* This function opens the file and checks its first box, to see if it
     contains the JP2-family signature.  It then closes the file and returns
     the result.  This should avoid some confusion associated with files
     whose suffix has been unreliably names. */
{
  jp2_family_src src;
  jp2_input_box box;
  src.open(fname);
  bool result = box.open(&src) && (box.get_box_type() == jp2_signature_4cc);
  src.close();
  return result;
}

/*****************************************************************************/
/* STATIC                        get_bpp_dims                                */
/*****************************************************************************/

static kdu_long
  get_bpp_dims(siz_params *siz)
  /* Figures out the number of "pixels" in the image, for the purpose of
     converting byte counts into bits per pixel, or vice-versa. */
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


/* ========================================================================= */
/*                            External Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/*                                   main                                    */
/*****************************************************************************/

int main(int argc, char *argv[])
{
  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);
  kdu_args args(argc,argv,"-s");

  // Parse simple arguments from command line

  
  char *ifname;
  float max_bpp;
  int skip_components, max_layers, discard_levels;
  int preferred_min_stripe_height, absolute_max_stripe_height;
  kdu_dims region;
  int num_threads, env_dbuf_height;
  bool simulate_parsing, cpu;
  kd_output_file *out_files =
    parse_simple_args(args,ifname,max_bpp,simulate_parsing,skip_components,
                      max_layers,discard_levels,
                      region,preferred_min_stripe_height,
                      absolute_max_stripe_height,
                      num_threads,env_dbuf_height,cpu);
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

  // Create appropriate output file
  kdu_compressed_source *input = NULL;
  kdu_simple_file_source file_in;
  jp2_family_src jp2_ultimate_src;
  jp2_source jp2_in;
  if (check_jp2_family_file(ifname))
    {
      input = &jp2_in;
      jp2_ultimate_src.open(ifname);
      if (!jp2_in.open(&jp2_ultimate_src))
        { kdu_error e; e << "Supplied input file, \"" << ifname << "\", does "
          "not appear to contain any boxes."; }
      jp2_in.read_header();
    }
  else
    {
      input = &file_in;
      file_in.open(ifname);
    }
  delete[] ifname;

  // Create the code-stream, and apply any restrictions/transformations
  kdu_codestream codestream;
  codestream.create(input);
  if ((max_bpp > 0.0F) || simulate_parsing)
    {
      kdu_long max_bytes = KDU_LONG_MAX;
      if (max_bpp > 0.0F)
        max_bytes = (kdu_long)
          (0.125 * max_bpp * get_bpp_dims(codestream.access_siz()));
      codestream.set_max_bytes(max_bytes,simulate_parsing);
    }
  codestream.apply_input_restrictions(skip_components,0,
                                      discard_levels,max_layers,NULL,
                                      KDU_WANT_OUTPUT_COMPONENTS);

  kdu_dims *reg_ptr = NULL;
  if (region.area() > 0)
    {
      kdu_dims dims; codestream.get_dims(0,dims,true);
      dims &= region;
      if (!dims)
        { kdu_error e; e << "Region supplied via `-int_region' argument "
          "has no intersection with the first image component to be "
          "decompressed, at the resolution selected."; }
      codestream.map_region(0,dims,region,true);
      reg_ptr = &region;
      codestream.apply_input_restrictions(skip_components,0,
                                          discard_levels,max_layers,reg_ptr,
                                          KDU_WANT_OUTPUT_COMPONENTS);
    }

        // If you wish to have geometric transformations folded into the
        // decompression process automatically, this is the place to call
        // `kdu_codestream::change_appearance'.

  // Find the dimensions of each image component we will be decompressing
  int n, num_components = codestream.get_num_components(true);
  kdu_dims *comp_dims = new kdu_dims[num_components];
  for (n=0; n < num_components; n++)
    codestream.get_dims(n,comp_dims[n],true);

  // Next, prepare the output files
  kd_output_file *out;
  if (out_files != NULL)
    {
      for (n=0, out=out_files; out != NULL; out=out->next, n++)
        {
          if (n >= num_components)
            { kdu_error e; e << "You have supplied more output files than "
              "there are image components to decompress!"; }
          out->size = comp_dims[n].size;
          if (out->is_raw)
            { // Try to preserve all the original precision and signed/unsigned
              // properties for raw files.
              out->precision = codestream.get_bit_depth(n,true);
              out->is_signed = codestream.get_signed(n,true);
              assert(out->precision > 0);
              if (out->precision > 16)
                out->precision = 16; // Can't store more than 16 bits/sample
              out->bytes_per_sample = (out->precision > 8)?2:1;
            }
          else
            { // PGM files always have an 8-bit unsigned representation
              out->precision = 8; out->is_signed = false;
              out->bytes_per_sample = 1;
              out->write_pgm_header();
            }
        }
      if (n < num_components)
        codestream.apply_input_restrictions(skip_components,num_components=n,
                                            discard_levels,max_layers,reg_ptr,
                                            KDU_WANT_OUTPUT_COMPONENTS);
    }

  // Start the timer
  time_t start_time, end_time, writing_time;
  if (cpu)
    start_time = clock();

  // Construct multi-threaded processing environment, if requested.  Note that
  // all we have to do to leverage the presence of multiple physical processors
  // is to create the multi-threaded environment with at least one thread for
  // each processor, pass a reference (`env_ref') to this environment into
  // `kdu_stripe_decompressor::start', and destroy the environment once we are
  // all done.
  //    If you are going to run the processing within a try/catch
  // environment, with an error handler which throws exceptions rather than
  // exiting the process, the only extra thing you need to do to realize
  // robust multi-threaded processing, is to arrange for your `catch' clause
  // to invoke `kdu_thread_entity::handle_exception' -- i.e., call
  // `env.handle_exception(exc)', where `exc' is the exception code you catch,
  // of type `kdu_exception'.  Even this is not necessary if you are happy for
  // the `kdu_thread_env' object to be destroyed when an error/exception
  // occurs.
  kdu_thread_env env, *env_ref=NULL;
  if (num_threads > 0)
    {
      env.create();
      for (int nt=1; nt < num_threads; nt++)
        if (!env.add_thread())
          num_threads = nt; // Unable to create all the threads requested
      env_ref = &env;
    }

  // Construct the stripe-decompressor object (this does all the work) and
  // assigns stripe buffers for incremental processing.  The present application
  // uses `kdu_stripe_decompressor::get_recommended_stripe_heights' to find
  // suitable stripe heights for processing.  If your application has its own
  // idea of what constitutes a good set of stripe heights, you may generally
  // use those values instead (could be up to the entire image in one stripe),
  // but note that whenever the image is tiled, the stripe heights can have an
  // impact on the efficiency with which the image is decompressed (a
  // fundamental issue, not a Kakadu implementation issue).  For more on this,
  // see the extensive documentation provided for
  // `kdu_stripe_decompressor::pull_stripe'.
  int *stripe_heights = new int[num_components];
  int *max_stripe_heights = new int[num_components];
  kdu_int16 **stripe_bufs = new kdu_int16 *[num_components];
                // Note: we will be using 16-bit stripe buffers throughout for
                // this demonstration, but you can supply 8-bit stripe buffers
                // to `kdu_stripe_compressor::push_stripe' if you prefer.
  kdu_stripe_decompressor decompressor;
  decompressor.start(codestream,false,false,env_ref,NULL,env_dbuf_height);
  decompressor.get_recommended_stripe_heights(preferred_min_stripe_height,
                                              absolute_max_stripe_height,
                                              stripe_heights,
                                              max_stripe_heights);
  for (n=0; n < num_components; n++)
    if ((stripe_bufs[n] =
         new kdu_int16[comp_dims[n].size.x*max_stripe_heights[n]]) == NULL)
      { kdu_error e; e << "Insufficient memory to allocate stripe buffers."; }
  int *precisions = NULL;
  if (out_files != NULL)
    { // Otherwise, represent all samples with 16-bit precision
      precisions = new int[num_components];
      for (out=out_files, n=0; n < num_components; n++, out=out->next)
        precisions[n] = out->precision;
    }

  // Now for the incremental processing
  bool continues=true;
  while (continues)
    {
      decompressor.get_recommended_stripe_heights(preferred_min_stripe_height,
                                                  absolute_max_stripe_height,
                                                  stripe_heights,NULL);
      continues = decompressor.pull_stripe(stripe_bufs,stripe_heights,NULL,NULL,
                                           precisions);
      if (out_files != NULL)
        { // Attempt to discount file writing time; note, however, that this
          // does not account for the fact that writing large stripes can tie
          // up a disk in the background, dramatically increasing the time
          // taken to read new compressed data while the next stripe is being
          // decompressed.  To avoid excessive skewing of timing results due
          // to disk I/O time, it is recommended that you run the application
          // without any output files for timing purposes.  All the stripes
          // still get fully decompressed into memory buffers, but the only
          // disk I/O is that due to reading of the compressed source.
          if (cpu)
            writing_time = clock();
          for (out=out_files, n=0; n < num_components; n++, out=out->next)
            out->write_stripe(stripe_heights[n],stripe_bufs[n]);
          if (cpu)
            start_time += (clock() - writing_time); // Don't count I/O time
        }
    }

  if (cpu)
    { // Report processing time
      end_time = clock()+1; // Avoid risk of divide by 0
      float seconds = (end_time-start_time) / (float) CLOCKS_PER_SEC;
      kdu_long total_samples = 0;
      for (n=0; n < num_components; n++)
        total_samples += comp_dims[n].area();
      float samples_per_second = total_samples / seconds;
      pretty_cout << "Processing time = " << seconds << " s; i.e., ";
      pretty_cout << samples_per_second << " samples/s\n";
      if (num_threads == 0)
        pretty_cout << "Processed using the single-threaded environment "
          "(see `-num_threads')\n";
      else
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          << "    " << num_threads << " parallel threads of execution "
          "(see `-num_threads')\n";
    }

  // Clean up
  decompressor.finish();
  if (env.exists())
    env.destroy(); // You can do this either before or after you destroy the
          // codestream, but not until after `kdu_stripe_decompressor::finish'.
  codestream.destroy();
  input->close();
  if (jp2_ultimate_src.exists())
    jp2_ultimate_src.close();
  for (n=0; n < num_components; n++)
    delete[] stripe_bufs[n];
  delete[] stripe_bufs;
  if (precisions != NULL)
    delete[] precisions;
  delete[] stripe_heights;
  delete[] max_stripe_heights;
  delete[] comp_dims;
  while ((out=out_files) != NULL)
    { out_files=out->next; delete out; }
  return 0;
}
