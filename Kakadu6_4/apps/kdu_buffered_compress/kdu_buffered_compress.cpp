/******************************************************************************/
// File: kdu_stripe_compressor.cpp [scope = APPS/BUFFERED_COMPRESS]
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
`kdu_stripe_compressor' object.
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
#include "kdu_stripe_compressor.h"
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

/*****************************************************************************/
/* INLINE                    eat_white_and_comments                          */
/*****************************************************************************/

static inline void
  eat_white_and_comments(FILE *in)
{
  int ch;
  bool in_comment;

  in_comment = false;
  while ((ch = getc(in)) != EOF)
    if (ch == '#')
      in_comment = true;
    else if (ch == '\n')
      in_comment = false;
    else if ((!in_comment) && (ch != ' ') && (ch != '\t') && (ch != '\r'))
      {
        ungetc(ch,in);
        return;
      }
}


/* ========================================================================= */
/*                               kd_source_file                             */
/* ========================================================================= */

struct kd_source_file {
  public: // Lifecycle functions
    kd_source_file()
      { fname=NULL; fp=NULL; bytes_per_sample=1; precision=8;
        is_signed=is_raw=swap_bytes=false; size=kdu_coords(0,0); next=NULL; }
    ~kd_source_file()
      { if (fname != NULL) delete[] fname;
        if (fp != NULL) fclose(fp); }
    void read_pgm_header();
    void read_stripe(int height, kdu_int16 *buf);
  public: // Data
    char *fname;
    FILE *fp;
    int bytes_per_sample;
    int precision; // Num bits
    bool is_signed;
    bool is_raw;
    bool swap_bytes; // If raw file word order differs from machine word order
    kdu_coords size; // Width, and remaining rows
    kd_source_file *next;
  };

/*****************************************************************************/
/*                       kd_source_file::read_pgm_header                     */
/*****************************************************************************/

void
  kd_source_file::read_pgm_header()
{
  int ch;
  char magic[3] = {'\0','\0','\0'};
  fread(magic,1,2,fp);
  if (strcmp(magic,"P5") != 0)
    { kdu_error e; e << "PGM image file must start with the magic string, "
      "\"P5\"!"; }
  bool failed = false;
  eat_white_and_comments(fp);
  if (fscanf(fp,"%d",&size.x) != 1)
    failed = true;
  eat_white_and_comments(fp);
  if (fscanf(fp,"%d",&size.y) != 1)
    failed = true;
  eat_white_and_comments(fp);
  if (fscanf(fp,"%d",&ch) != 1)
    failed = true;
  if (failed || (size.x < 1) || (size.y < 1))
    {kdu_error e; e << "Image file \"" << fname << "\" does not appear to "
     "have a valid PGM header."; }
  while ((ch = fgetc(fp)) != EOF)
    if ((ch == '\n') || (ch == ' '))
      break;
}

/*****************************************************************************/
/*                         kd_source_file::read_stripe                       */
/*****************************************************************************/

void
  kd_source_file::read_stripe(int height, kdu_int16 *buf)
{
  int num_samples = height*size.x;
  int num_bytes = num_samples * bytes_per_sample;
  if (num_samples <= 0) return;
  if (fread(buf,1,num_bytes,fp) != (size_t) num_bytes)
    { kdu_error e; e << "File, \"" << fname << "\" terminated unexpectedly."; }
  kdu_int16 val, off = (is_signed)?0:((1<<precision)>>1);
  if (bytes_per_sample == 1)
    { // Expand to a 16-bit signed representation
      kdu_byte *sp=(kdu_byte *) buf;
      sp += num_samples-1; buf += num_samples-1;
      while (num_samples--)
        *(buf--) = ((kdu_int16) *(sp--)) - off;
    }
  else if (swap_bytes)
    { // Swap byte order and make signed representation
      while (num_samples--)
        { val = *buf; val=(val<<8) + ((val>>8)&0x00FF); *(buf++) = val - off; }
    }
  else if (off != 0)
    { // Make signed representation
      while (num_samples--)
        { val = *buf; *(buf++) = val-off; }
    }
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
  out << "This is Kakadu's \"kdu_buffered_compress\" demo application.\n";
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
  out << "-i <PGM/raw file 1>[,<PGM/raw file 2>[,...]]\n";
  if (comprehensive)
    out << "\tOne or more input image files.  To understand how the number "
           "and dimensions of the input files interact with the dimensions "
           "and bit-depths recorded in the codestream header, along with any "
           "defined multi-component transform, see the discussion which "
           "appears at the end of this usage statement.  To simplify this demo "
           "application, each file represents only one image component, "
           "and must be either a PGM file, or a raw file.  PGM files have "
           "their own header, but raw files rely upon the dimensions, "
           "precision and signed/unsigned characteristics being configured "
           "using `Sdims', `Sprecision' (or `Mprecision') and `Ssigned' "
           "(or `Msigned') command-line arguments -- see multi-component "
           "transforms discussion below.  Similar to the \"kdu_compress\" "
           "application, the sample bits in a raw file must be in the least "
           "significant bit positions of an 8 or 16 bit word, depending on the "
           "`Sprecision' attribute.  Unused MSB's in each word are entirely "
           "disregarded.  The default word organization is big-endian, "
           "regardless of your machine architecture, but this application "
           "allows you to explicitly nominate a different byte order, "
           "via the `-little_endian' argument.\n";
  out << "-o <compressed file -- raw code-stream unless suffix is \".jp2\">\n";
  if (comprehensive)
    out << "\tName of file to receive the compressed code-stream.  If the "
           "file name has a \".jp2\" suffix (not case sensitive), the "
           "code-stream will be wrapped up inside the JP2 file format.  In "
           "this case, the first 3 source image components will be treated "
           "as sRGB colour channels (red, green then blue) and the remainder "
           "will be identified as auxiliary undefined components in the JP2 "
           "file.  For other options in writing JP2 files, refer to the "
           "more sophisticated \"kdu_compress\" application.\n";
  out << "-rate -|<max bits/pel>[,<min bits/pel>]\n";
  if (comprehensive)
    out << "\tUse this argument to control the maximum bit-rate and/or the "
           "minimum bit-rate associated with the layered code-stream.  The "
           "number of layers is given by the `Clayers' attribute, which you "
           "must set separately, if you want more than one quality layer.  "
           "If the `-' character is substituted for a maximum bit-rate, or "
           "if no `-rate' argument is supplied, the highest quality layer "
           "includes all generated bits.  If the minimum bit-rate is not "
           "supplied, it will be determined by an internal heuristic.  Layer "
           "bit-rates are spaced logarithmically between the minimum and "
           "maximum bit-rates.\n";
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
    out << "\tRegardless of the desire to process in stripes whose height is "
           "equal to the tile height, wherever the image is horizontally "
           "tiled, this argument provides an upper bound on the maximum "
           "stripe height.  If the tile height exceeds this value, "
           "an entire row of tiles will be kept open for processing.  This "
           "avoids excessive memory consumption.  This argument allows you "
           "to control the trade-off between stripe buffering and "
           "tile compression engine memory.  The default limit is 1024.\n";
  out << "-flush_period <incremental flush period, measured in image lines>\n";
  if (comprehensive)
    out << "\tBy default, the system waits until all compressed data has "
           "been generated, by applying colour transforms, wavelet transforms "
           "and block encoding processes to the entire image, before any of "
           "this compressed data is actually written to the output file.  "
           "The present argument may be used to request incremental flushing, "
           "where the compressed data is periodically flushed to the output "
           "file, thereby avoiding the need for internal buffering of the "
           "entire compressed image.  The agument takes a single parameter, "
           "identifying the minimum number of image lines which should be "
           "processed before each attempt to flush new code-stream data.  The "
           "actual period may be larger, if insufficient data has "
           "been generated to progress the code-stream.\n"
           "\t   You should be careful to keep the flushing period large "
           "enough to give the rate control algorithm a decent amount of "
           "compressed data to perform effective rate control.  Generally "
           "a period of at least 1000 or 2000 image lines should be used "
           "for rate driven flushing.\n"
           "\t   You should be aware of the fact that incremental flushing "
           "is possible only on tile boundaries or when the packet "
           "progression sequence is spatially progressive (PCRL), with "
           "sufficiently small precincts.  The vertical dimension of "
           "precincts in the lowest resolution levels must be especially "
           "tightly controlled, particularly if you have a large number of "
           "DWT levels.  As an example, with `Clevels=6', the following "
           "precinct dimensions would be a good choice for use with 32x32 "
           "code-blocks: `Cprecincts={256,256},{128,128},{64,64},{32,64},"
           "{16,64},{8,64},{4,64}'.\n";
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
  mct_params mct; mct.describe_attributes(out,comprehensive);
  mcc_params mcc; mcc.describe_attributes(out,comprehensive);
  mco_params mco; mco.describe_attributes(out,comprehensive);
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
           "is generally quite a bit less CPU intensive than block encoding "
           "(which is always spread across multiple threads, if available).  "
           "When `-double_buffering' is specified, a (typically) small number "
           "of image rows (the parameter supplied with this argument) in each "
           "image component are double buffered, so that one set can be "
           "loaded from the image source and colour transformed, while the "
           "other set is being subjected to DWT analysis.  Importantly, this "
           "allows the DWT operations associated with each tile-component to "
           "be performed concurrently, in different threads.  The number of "
           "double-buffered rows can be set as small as 1, but this may add "
           "quite a bit of thread context switching overhead.  A value in the "
           "range 4 to 16 is recommended for moderate to large image/tile "
           "widths.\n";
  out << "-cpu -- report processing CPU time\n";
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
  out << "Understanding Multi-Component Transforms:\n";
  out.set_master_indent(3);
  out << "   From version 4.6, Kakadu supports JPEG2000 Part 2 multi-component "
    "transforms.  These features are used if you define the `Mcomponents' "
    "attribute to be anything other than 0.  In this case, `Mcomponents' "
    "denotes the number of multi-component transformed output components "
    "produced during decompression, with `Mprecision' and `Msigned' "
    "identifying the precision and signed/unsigned attributes of these "
    "components.  These parameters will be derived from the source files "
    "(non-raw files), or else they will be used to figure out the source "
    "file format (raw files).  When working with multi-component transforms, "
    "the term \"codestream components\" refers to the set of components "
    "which are subjected to spatial wavelet transformation, quantization "
    "and coding.  These are the components which are supplied to the input "
    "of the multi-component transform during decompression.  The number of "
    "codestream components is given by the `Scomponents' attribute, while "
    "their precision and signed/unsigned properties are given by `Sprecision' "
    "and `Ssigned'.  You should set these parameter attributes "
    "to suitable values yourself.  If you do not explicitly supply a value "
    "for the `Scomponents' attribute, it will default to the number of "
    "supplied source files.  The value of `Mcomponents' may also be larger "
    "than the number of supplied source files.  In this case, the source "
    "files represent the initial set of image components which will be "
    "recovered during decompression.  This subset must be larger enough to "
    "allow the internal machinery to invert the multi-component transform "
    "network, so as to recover a full set of codestream image components.  If "
    "not, you will receive a descriptive error message explaining what is "
    "lacking.\n";
  out << "   As an example, suppose the codestream image components "
    "correspond to the first N <= M principle components of an original "
    "set of M image components -- obtained by applying the KLT to, say, "
    "a hyperspectral data set.  To compress the image, you would "
    "probably want to supply all M original image planes.  However, you "
    "could supply as few as the first N original image planes.  Here, "
    "M is the value of `Mcomponents' and N is the value of `Scomponents'.\n";
  out << "   If there is no multi-component transform, `Scomponents' is the "
    "number of output and codestream components; it is derived from the "
    "supplied input files.  `Sprecision' and `Ssigned' hold the bit-depth "
    "and signed/unsigned attributes of the image components.  For raw files, "
    "these must be supplied explicitly on the command line; otherwise, they "
    "are derived from the input file headers.\n";
  out << "   It is worth noting that the dimensions of the N=`Scomponents' "
    "codestream image components are assumed to be identical to those of the "
    "first N source files.  This assumption is imposed for simplicity in this "
    "demonstration application; it is not required by the Kakadu core "
    "system.\n\n";

  out.flush();
  exit(0);
}

/*****************************************************************************/
/* STATIC                     parse_simple_args                              */
/*****************************************************************************/

static kd_source_file *
  parse_simple_args(kdu_args &args, char * &ofname, float &max_rate,
                    float &min_rate, int &preferred_min_stripe_height,
                    int &absolute_max_stripe_height, int &flush_period,
                    int &num_threads, int &double_buffering_height,
                    bool &cpu)
  /* Parses all command line arguments whose names include a dash.  Returns
     a list of open input files. */
{
  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  ofname = NULL;
  max_rate = min_rate = -1.0F;
  preferred_min_stripe_height = 8;
  absolute_max_stripe_height = 1024;
  flush_period = 0;
  num_threads = 0; // This is not actually the default -- see below.
  double_buffering_height = 0; // i.e., no double buffering
  cpu = false;
  bool little_endian = false;
  kd_source_file *fhead=NULL, *ftail=NULL;

  if (args.find("-o") != NULL)
    {
      const char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-o\" argument requires a file name!"; }
      ofname = new char[strlen(string)+1];
      strcpy(ofname,string);
      args.advance();
    }
  else
    { kdu_error e; e << "You must supply an output file name."; }

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

  if (args.find("-rate") != NULL)
    {
      const char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-rate\" argument requires a parameter string!"; }
      bool valid = false;
      if ((*string == '-') && (string[1] == ',') &&
          (sscanf(string+2,"%f",&min_rate) == 1) && (min_rate > 0.0F))
        valid = true;
      else if ((*string == '-') && (string[1] == '\0'))
        valid = true;
      else if ((sscanf(string,"%f,%f",&max_rate,&min_rate) == 2) &&
               (min_rate > 0.0F) && (max_rate > min_rate))
        valid = true;
      else if (sscanf(string,"%f",&max_rate) == 1)
        valid = true;
      if (!valid)
        { kdu_error e; e << "\"-rate\" argument has an invalid parameter "
          "string; you must specify either one or two rate tokens, "
          "corresponding to maximum and minimum bit-rates (in order), over "
          "which to allocate the quality layers.  The maximum rate spec may "
          "be replaced by a '-' character, meaning use all available bits.  "
          "The minimum rate spec, if missing, will be automatically created.  "
          "Both parameters must be strictly positive if supplied."; }
      args.advance();
    }

  if (args.find("-flush_period") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&flush_period) != 1) ||
          (flush_period < 128))
        { kdu_error e; e << "\"-flush_period\" argument requires a positive "
          "integer, no smaller than 128.  Typical values will generally be "
          "in the thousands; incremental flushing has no real benefits, "
          "except when the image is large."; }
      args.advance();
    }

  if (args.find("-i") != NULL)
    {
      const char *delim, *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-i\" argument requires a parameter string."; }
      for (; *string != '\0'; string=delim)
        {
          while (*string == ',') string++;
          for (delim=string; (*delim != '\0') && (*delim != ','); delim++);
          kd_source_file *file = new kd_source_file;
          if (ftail == NULL)
            fhead = ftail = file;
          else
            ftail = ftail->next = file;
          file->fname = new char[(delim-string)+1];
          strncpy(file->fname,string,delim-string);
          file->fname[delim-string] = '\0';
          if ((file->fp = fopen(file->fname,"rb")) == NULL)
            { kdu_error e;
              e << "Unable to open input file, \"" << file->fname << "\"."; }
        }
      args.advance();
    }

  if (fhead == NULL)
    { kdu_error e; e << "You must supply at least one input file"; }

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
/* STATIC                      check_jp2_suffix                              */
/*****************************************************************************/

static bool
  check_jp2_suffix(const char *fname)
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
  char *ofname;
  float min_rate, max_rate;
  int preferred_min_stripe_height, absolute_max_stripe_height;
  int num_threads, env_dbuf_height, flush_period;
  bool cpu;
  kd_source_file *in_files =
    parse_simple_args(args,ofname,max_rate,min_rate,preferred_min_stripe_height,
                      absolute_max_stripe_height,flush_period,
                      num_threads,env_dbuf_height,cpu);

  // Create appropriate output file
  kdu_compressed_target *output = NULL;
  kdu_simple_file_target file_out;
  jp2_family_tgt jp2_ultimate_tgt;
  jp2_target jp2_out;
  if (check_jp2_suffix(ofname))
    {
      output = &jp2_out;
      jp2_ultimate_tgt.open(ofname);
      jp2_out.open(&jp2_ultimate_tgt);
    }
  else
    {
      output = &file_out;
      file_out.open(ofname);
    }
  delete[] ofname;

  // Collect any dimensioning/tiling parameters supplied on the command line;
  // need dimensions for raw files, if any.
  siz_params siz;
  const char *string;
  for (string=args.get_first(); string != NULL; )
    string = args.advance(siz.parse_string(string));

  // Set input file dimensions (either by reading header, or using `siz')
  // This looks a little complex, only because we want to allow for
  // multi-component transforms, as defined in JPEG2000 Part 2.  A
  // multi-component transform is being used if the `Mcomponents' attribute
  // is defined and greater than 0.  In this case, `Mcomponents' identifies
  // the set of image components that will be decoded after applying the
  // multi-component transform to the `Scomponents' codestream components.
  // During compression, we supply `num_components' source components to
  // the `kdu_stripe_compressor' object, where `num_components' is allowed to
  // be less than `Mcomponents' if we believe that the multi-component
  // transform network can be inverted (this is done automatically by
  // `kdu_multi_analysis' on top of which `kdu_stripe_compressor' is built)
  // to produce the `Scomponents' codestream components from the
  // `num_components' supplied source components.  These source components
  // correspond to the initial `num_components' components reconstructed by
  // the decompressor, out of the total `Mcomponents'.  This is why the code
  // below involves three different component counts (`m_components',
  // `c_components' and `num_components').
  //    For Part-1 codestreams, `Mcomponents' is 0 and `num_components' and
  // `c_components' are identical.  In this case, `Scomponents' can be derived
  // simply by counting files, and `Ssigned' and `Sprecision' can be set,
  // based on the source file headers.
  //    For Part-2 codestreams, `Mcomponents' is greater than 0 and
  // `Scomponents' must be explicitly set by the application (or by parsing the
  // command line).  In this case, `Msigned' and `Mprecision' can be set, based
  // on the source file headers, but `Ssigned' and `Sprecision' must be set by
  // the application (or by parsing the command line).  If you have
  // `Mcomponents' > 0 and no defined value for `Scomponents', the default
  // `Scomponents' value is set to `num_components' (i.e., to the number of
  // source files).
  int m_components=0;  siz.get(Mcomponents,0,0,m_components);
  kdu_long total_samples=0, total_pixels=0;
  int n, num_components=0;
  kd_source_file *in;
  for (in=in_files; in != NULL; in=in->next, num_components++)
    {
      if (in->is_raw)
        {
          if ((!(siz.get(Sdims,num_components,0,in->size.y) &&
                 siz.get(Sdims,num_components,1,in->size.x))) ||
              ((m_components <= 0) && 
               !(siz.get(Ssigned,num_components,0,in->is_signed) &&
                 siz.get(Sprecision,num_components,0,in->precision))) ||
              ((m_components > 0) &&
               !(siz.get(Msigned,num_components,0,in->is_signed) &&
                 siz.get(Mprecision,num_components,0,in->precision))) ||
              (in->precision > 16))
            { kdu_error e; e << "Raw file, \"" << in->fname << "\" supplied "
              "on command line has no matching dimension parameters; these "
              "must be provided via the `Sdims', `Sprecision' and `Ssigned' "
              "arguments -- you must specify all three attributes."; }
          in->bytes_per_sample = (in->precision > 8)?2:1;
        }
      else
        { // In this case, the image dimensions are known from the header
          in->read_pgm_header();
          siz.set(Sdims,num_components,0,in->size.y);
          siz.set(Sdims,num_components,1,in->size.x);
          if (m_components > 0)
            {
              siz.set(Msigned,num_components,0,in->is_signed=false);
              siz.set(Mprecision,num_components,0,in->precision=8);
            }
          else
            {
              siz.set(Ssigned,num_components,0,in->is_signed=false);
              siz.set(Sprecision,num_components,0,in->precision=8);
            }
          in->bytes_per_sample = 1;
        }
      kdu_long samples = in->size.x; samples *= in->size.y;
      total_samples += samples;
      total_pixels = (samples > total_pixels)?samples:total_pixels;
    }
  int c_components=0;
  if (!siz.get(Scomponents,0,0,c_components))
    siz.set(Scomponents,0,0,c_components=num_components);
  siz.finalize_all();

  // Start the timer
  time_t start_time, end_time, reading_time;
  if (cpu)
    start_time = clock();

  // Construct the `kdu_codestream' object and parse all remaining args
  kdu_codestream codestream;
  codestream.create(&siz,output);
  for (string=args.get_first(); string != NULL; )
    string = args.advance(codestream.access_siz()->parse_string(string));
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }
  codestream.access_siz()->finalize_all();

  // Write the JP2 header, if necessary
  if (jp2_ultimate_tgt.exists())
    { // Do minimal JP2 file initialization, for demonstration purposes
      jp2_dimensions dimensions = jp2_out.access_dimensions();
      dimensions.init(codestream.access_siz());
      dimensions.finalize_compatibility(codestream.access_siz());
           // There is no need to actually call the above function here,
           // since the `init' function was invoked using the already
           // finalized parameter sub-system.  However, if your application
           // needs to initialize the `jp2_dimensions' object using only the
           // siz information (as in "kdu_compress") you really should later
           // call `jp2_dimensions::finalize_compatibility' once you have
           // created the codestream and finalized the parameter sub-system.
      jp2_colour colour = jp2_out.access_colour();
      colour.init((num_components>=3)?JP2_sRGB_SPACE:JP2_sLUM_SPACE);
      jp2_out.write_header();
           // If you want to write additional JP2 boxes, this is the place to
           // do it.  For an example, refer to the `write_extra_jp2_boxes'
           // function in the "kdu_compress" demo application.
      jp2_out.open_codestream(true);
    }

  // Determine the desired cumulative layer sizes
  int num_layer_sizes;
  kdu_params *cod = codestream.access_siz()->access_cluster(COD_params);
  if (!(cod->get(Clayers,0,0,num_layer_sizes) && (num_layer_sizes > 0)))
    cod->set(Clayers,0,0,num_layer_sizes=1);
  kdu_long *layer_sizes = new kdu_long[num_layer_sizes];
  memset(layer_sizes,0,sizeof(kdu_long)*num_layer_sizes);
  if ((min_rate > 0.0F) && (num_layer_sizes < 2))
    { kdu_error e; e << "You have specified two bit-rates using the `-rate' "
      "argument, but only one quality layer.  Use `Clayers' to specify more "
      "layers -- they will be spaced logarithmically between the min and max "
      "bit-rates."; }
  if (min_rate > 0.0F)
    layer_sizes[0] = (kdu_long)(total_pixels*min_rate*0.125F);
  if (max_rate > 0.0F)
    layer_sizes[num_layer_sizes-1] =
      (kdu_long)(total_pixels*max_rate*0.125);

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
  // `env.handle_exception(exc)', where `exc' is the exception code which you
  // catch, of type `kdu_exception'.  Even this is not necessary if you are
  // happy for the `kdu_thread_env' object to be destroyed when an
  // error/exception occurs.
  kdu_thread_env env, *env_ref=NULL;
  if (num_threads > 0)
    {
      env.create();
      for (int nt=1; nt < num_threads; nt++)
        if (!env.add_thread())
          num_threads = nt; // Unable to create all the threads requested
      env_ref = &env;
    }

  // Construct the stripe-compressor object (this does all the work) and
  // assign stripe buffers for incremental processing.  Note that nothing stops
  // you from passing in stripes of an image you have in memory, produced by
  // your application in any desired manner, but the present demonstration uses
  // files to recover stripes.  The present application uses
  // `kdu_stripe_compressor::get_recommended_stripe_heights' to find suitable
  // stripe heights for processing.  If your application has its own idea of
  // what constitutes a good set of stripe heights, you may generally use those
  // values instead (could be up to the entire image in one stripe), but note
  // that whenever the image is tiled, the stripe heights can have an impact
  // on the efficiency with which the image is compressed (a fundamental
  // issue, not a Kakadu implementation issue).  For more on this, see the
  // extensive documentation provided for `kdu_stripe_compressor::push_stripe'.
  int *precisions = new int[num_components];
  int *stripe_heights = new int[num_components];
  int *max_stripe_heights = new int[num_components];
  kdu_int16 **stripe_bufs = new kdu_int16 *[num_components];
                // Note: we will be using 16-bit stripe buffers throughout for
                // this demonstration, but you can supply 8-bit stripe buffers
                // to `kdu_stripe_compressor::push_stripe' if you prefer.
  kdu_stripe_compressor compressor;
  compressor.start(codestream,num_layer_sizes,layer_sizes,NULL,0,false,
                   false,true,0.0,num_components,false,env_ref,NULL,
                   env_dbuf_height);
  compressor.get_recommended_stripe_heights(preferred_min_stripe_height,
                                            absolute_max_stripe_height,
                                            stripe_heights,max_stripe_heights);
  for (in=in_files, n=0; n < num_components; n++, in=in->next)
    if ((stripe_bufs[n]=new kdu_int16[in->size.x*max_stripe_heights[n]])==NULL)
      { kdu_error e; e << "Insufficient memory to allocate stripe buffers."; }
    else
      precisions[n] = in->precision;

  // Now for the incremental processing
  do {
      compressor.get_recommended_stripe_heights(preferred_min_stripe_height,
                                                absolute_max_stripe_height,
                                                stripe_heights,NULL);
      if (cpu)
        reading_time = clock();
      for (in=in_files, n=0; n < num_components; n++, in=in->next)
        {
          assert(stripe_heights[n] <= max_stripe_heights[n]);
          in->read_stripe(stripe_heights[n],stripe_bufs[n]);
        }
      if (cpu)
        start_time += (clock() - reading_time); // Don't count I/O time
    } while (compressor.push_stripe(stripe_bufs,stripe_heights,NULL,NULL,
                                    precisions,NULL,flush_period));

  if (cpu)
    { // Report processing time
      end_time = clock()+1; // Avoid risk of divide by 0
      float seconds = (end_time-start_time) / (float) CLOCKS_PER_SEC;
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
  compressor.finish();
  codestream.destroy();
  if (env.exists())
    env.destroy(); // If you do this before destroying the codestream, you
       // must call `env.terminate' prior to destroying the codestream, so as
       // to return any codestream-specific buffer resources they may be
       // holding onto.  This is not required during decompression.

  output->close();
  if (jp2_ultimate_tgt.exists())
    jp2_ultimate_tgt.close();
  for (n=0; n < num_components; n++)
    delete[] stripe_bufs[n];
  delete[] stripe_bufs;
  delete[] precisions;
  delete[] stripe_heights;
  delete[] max_stripe_heights;
  delete[] layer_sizes;
  while ((in=in_files) != NULL)
    { in_files=in->next; delete in; }
  return 0;
}
