/*****************************************************************************/
// File: kdu_expand.cpp [scope = APPS/DECOMPRESSOR]
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
   File-based JPEG2000 decompressor application, demonstrating quite a few of
the decompression-oriented capabilities of the Kakadu framework.  For further
demonstration of these capabilities, refer to the "kdu_show" application.
******************************************************************************/

// System includes
#include <string.h>
#include <stdio.h> // so we can use `sscanf' for arg parsing.
#include <math.h>
#include <assert.h>
#include <iostream>
#include <fstream>
// Kakadu core includes
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
#include "kdu_arch.h"
// Application includes
#include "kdu_args.h"
#include "kdu_image.h"
#include "kdu_file_io.h"
#include "jpx.h"
#include "expand_local.h"

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
/*                           Internal Functions                              */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        print_version                               */
/*****************************************************************************/

static void
  print_version()
{
  kdu_message_formatter out(&cout_message);
  out.start_message();
  out << "This is Kakadu's \"kdu_expand\" application.\n";
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
           "wrapped in the JP2 or JPX file formats.  We no longer identify "
           "the file type based on its suffix.  Instead, we first try "
           "to open the file as a JP2-family file.  If this fails, we open "
           "it as a raw code-stream.  The only drawback of this approach "
           "is that raw code-streams are opened and closed twice.\n";
  out << "-o <file 1>,...\n";
  if (comprehensive)
    out << "\tOne or more output files. If multiple files are provided, "
           "they must be separated by commas. Any spaces will be treated as "
           "part of the file name.  This argument is not mandatory; if "
           "no output files are given, the decompressor will run completely "
           "but produce no image output.  This can be useful for timing "
           "purposes.  Currently accepted image file formats are: "
           "TIFF, RAW (big-endian), RAWL (little-endian), BMP, PGM and "
           "PPM, as determined by the suffix.  There need not be sufficient "
           "image files to represent all image components in the "
           "code-stream.  Raw files are written with the sample bits in the "
           "least significant bit positions of an 8, 16, 24 or 32 bit word, "
           "depending on the bit-depth.  For signed data, the word is sign "
           "extended. The word organization is big-endian if the file "
           "suffix is \".raw\" and little-endian if the file suffix is "
           "\".rawl\".\n"
           "\t   There can be cases where you wish to explicitly specify "
           "a different precision for the saved sample values to the "
           "precision which is recorded internally in the compressed "
           "image (Sprecision parameter attribute).  You can make this "
           "happen by providing the `-fprec' argument, described below.\n";
  out << "-fprec <comp-0 precision>[M|L][,<comp-1 precision>[M|L][...]]\n";
  if (comprehensive)
    out << "\tYou can use this argument to adjust the way in which sample "
           "values are recorded in the output file(s).  The argument takes a "
           "comma-separated list of non-negative precision values.  The list "
           "can contain more values than the number of image components in "
           "the collection of input files.  If it contains fewer values, the "
           "last value is replicated as required to provide forced precision "
           "values for all image components.  You can supply the special "
           "value of 0 for any component (including the last one in the "
           "comma-separated list, which gets replicated).  This special "
           "value means that the original precision should be left "
           "untouched.\n"
           "\t   Each supplied value can optionally be followed by a single "
           "character suffix of `M' or `L'.  If `M' is used sample values (at "
           "original precision) are effectively scaled by a power of 2 so "
           "as to match the forced precision; this means that the most "
           "significant bits align before and after precision forcing.  This "
           "also means that forcing to a reduced precision will cause "
           "rounding and discard of least significant bits.  If the `L' "
           "suffix is used, the precision forcing algorithm aligns least "
           "significant bits (this is also the default, in case there is no "
           "suffix).  What this means is that there is no scaling, "
           "but forcing to a reduced precision may cause truncation of "
           "values which would not otherwise be representable in the forced "
           "precision.  On the other hand, forcing to a higher precision "
           "may allow some values which would ordinarily exceed the original "
           "dynamic range (e.g., due to quantization errors) to be "
           "preserved.\n"
           "\t   It is worth noting the difference between saving "
           "in a file format whose words must use one of a number of "
           "fixed precisions and explicitly forcing the precision to be "
           "used for each written word.  For example, if you save samples "
           "with Sprecision=4 to a PGM file, the samples are clipped to "
           "their nominal range and written into the most significant "
           "bit positions of each 8-bit byte.  If you also specify "
           "\"-fprec 8L\" or just \"-fprec 8\", the sample values are "
           "written into the least significant 4 bits and only clipped to "
           "the range which can be accommodated by each 8-bit byte.  If "
           "you were to save with \"-fprec 6\", the least significant 2 "
           "bits in each written byte would be 0, and sample values up to "
           "4 times as large as the nominal range can be preserved.\n"
           "\t   An important application for the `-fprec' option is saving "
           "samples with unusual precisions to TIFF files.  For example, if "
           "you save data with Sprecision=11 to a TIFF file, the 11 bits of "
           "each written sample are packed tightly into each row of output "
           "bytes.  The complex packing rule is often not correctly "
           "implemented by third party image readers, but you can use "
           "\"-fprec 16L\" or \"-fprec 16M\", for example, to force each "
           "sample to be saved into its own 16-bit word (occupying the 11 "
           "least significant bits or the 11 most significant bits, "
           "respectively); of course, the TIFF file will then "
           "indicate that the data has 16-bit precision, which is not "
           "quite correct, but this is the way in which many third party "
           "applications expect to find high bit-depth data in TIFF files.\n"
           "\t   Another interesting way to use this function is to force "
           "raw files containing samples with 17 to 24 bits of precision "
           "to be saved into 32-bit words, rather than 24 bit words, "
           "simplifying the interaction with other applications.\n";
  out << "-jpx_layer <compositing layer index>\n";
  if (comprehensive)
    out << "\tBy default, the first compositing layer of a JPX file is "
           "used to determine the colour channels to decompress.  This "
           "argument allows you to identify a different compositing layer "
           "(note that layer indices start from 0).  Plain JP2 files are "
           "treated as having only one compositing layer.  This argument "
           "is ignored if the input is a raw code-stream.  It is also "
           "ignored if `-raw_components' is used.\n";
  out << "-raw_components [<codestream index>]\n";
  if (comprehensive)
    out << "\tBy default, when a JP2/JPX file is decompressed, only "
           "those components which contribute to colour channels in the "
           "first compositing layer (or the layer identified by "
           "`-jpx_layer') are decompressed (one for luminance, three "
           "for RGB, etc.), applying any required palette mapping along "
           "the way.  In some cases, however, it may be desirable to "
           "decompress all of the raw image components available from a "
           "code-stream.  Use this switch to accomplish this.  There will "
           "then be no palette mapping in this case.  The optional "
           "parameter may be used to specify the index of the code-stream "
           "to be decompressed.  This is relevant only for JPX files which "
           "may contain multiple code-streams.  By default, the first "
           "code-stream (index 0) is decompressed.\n";
  out << "-codestream_components -- suppress multi-component/colour xforms\n";
  if (comprehensive)
    out << "\tThis flag allows you to gain access to the decompressed "
           "codestream image components, as they appear after inverse "
           "spatial wavelet transformation, prior to the inversion of any "
           "Part 1 colour transform (RCT/ICT decorrelation transforms) or "
           "Part 2 multi-component transform.  To understand this flag, it "
           "is helpful to know that there are two types of image components: "
           "\"codestream image components\" and \"output image components\".  "
           "The output image components are the ones which the content "
           "creator intended you to reconstruct, but it is sometimes "
           "interesting to reconstruct the codestream image components.  "
           "Thus, in the normal output-oriented mode, if you ask for the "
           "second image component (e.g., using `-skip_components') of a "
           "colour image, you will receive the green channel, even if this "
           "has to be recovered by first decompressing luminance and "
           "chrominance channels.  If you ask for codestream components, "
           "however, you will receive the Cb (blue colour difference) "
           "component, assuming that the ICT (YCbCr) transform was used "
           "during compression.  It should be noted that the "
           "`-codestream_components' flag cannot be used in conjunction with "
           "JP2/JPX files unless `-raw_components' is also specified.  "
           "`-raw_components' by itself, however, recovers only the "
           "\"output image components\" defined by the codestream, as opposed "
           "to its \"codestream image components\".\n";
  out << "-no_seek\n";
  if (comprehensive)
    out << "\tBy default, the system will try to exploit pointer information "
           "in the code-stream to seek over irrelevant elements.  This flag "
           "turns this behaviour off, which may result in higher memory "
           "consumption or even failure to read non-linear file formats, "
           "but avoids the potential for wasted disk accesses.\n";
  out << "-rotate <degrees>\n";
  if (comprehensive)
    out << "\tRotate source image prior to compression. "
           "Must be multiple of 90 degrees.\n";
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
           "`-rate' and when reporting overall bit-rates.  Also, if a "
           "reduced spatial region of the image is required (see `-region'), "
           "only those bytes which are relevant to that region are counted "
           "when applying bit-rate limits and reporting overall bit-rates.  "
           "The effect is intended to be the same as if the code-stream were "
           "first parsed to remove the resolutions, components, quality "
           "layers or precincts which are not being used.  Note, however, "
           "that this facility might not currently work as expected if the "
           "image happens to be tiled.\n";
  out << "-skip_components <num initial image components to skip>\n";
  if (comprehensive)
    out << "\tSkips over one or more initial image components, reconstructing "
           "as many remaining image components as can be stored in the "
           "output image file(s) specified with \"-o\" (or all remaining "
           "components, if no \"-o\" argument is supplied).  This argument "
           "is not meaningful if the input is a JP2/JPX file, unless "
           "the `-raw_components' switch is also selected.\n";
  out << "-no_alpha -- do not decompress alpha channel\n";
  if (comprehensive)
    out << "\tBy default, an alpha channel which is described by the "
           "source file will be decompressed and written to the output "
           "file(s) if possible.  This is relevant only for output file "
           "formats which can store alpha.  The present argument may be "
           "used to suppress the decompression of alpha channels.\n";
  out << "-layers <max layers to decode>\n";
  if (comprehensive)
    out << "\tSet an upper bound on the number of quality layers to actually "
           "decode.\n";
  out << "-reduce <discard levels>\n";
  if (comprehensive)
    out << "\tSet the number of highest resolution levels to be discarded.  "
           "The image resolution is effectively divided by 2 to the power of "
           "the number of discarded levels.\n";
  out << "-region {<top>,<left>},{<height>,<width>}\n";
  if (comprehensive)
    out << "\tEstablish a region of interest within the original compressed "
           "image.  Only the region of interest will be decompressed and the "
           "output image dimensions will be modified accordingly.  The "
           "coordinates of the top-left corner of the region are given first, "
           "separated by a comma and enclosed in curly braces, after which "
           "the dimensions of the region are given in similar fashion.  The "
           "two coordinate pairs must be separated by a comma, with no "
           "intervening spaces.  All coordinates and dimensions are expressed "
           "relative to the origin and dimensions of the high resolution "
           "grid, using real numbers in the range 0 to 1.\n";
  out << "-precise -- forces the use of 32-bit representations.\n";
  if (comprehensive)
    out << "\tBy default, 16-bit data representations will be employed for "
           "sample data processing operations (colour transform and DWT) "
           "whenever the image component bit-depth is sufficiently small.\n";
  out << "-fussy\n";
  if (comprehensive)
    out << "\tEncourage fussy code-stream parsing, in which most code-stream "
           "compliance failures will terminate execution, with an appropriate "
           "error message.\n";
  out << "-resilient\n";
  if (comprehensive)
    out << "\tEncourage error resilient processing, in which an attempt is "
           "made to recover from errors in the code-stream with minimal "
           "degradation in reconstructed image quality.  The current "
           "implementation should avoid execution failure so long as only "
           "a single tile-part was used and no errors are found in the main "
           "or tile header.  The implementation recognizes tile-part headers "
           "only if the first 4 bytes of the marker segment are correct, "
           "which makes it extremely unlikely that a code-stream with only "
           "one tile-part will be mistaken for anything else.  Multiple "
           "tiles or tile-parts can create numerous problems for an error "
           "resilient decompressor; complete failure may occur if a "
           "multi-tile-part code-stream is corrupted.\n";
  out << "-resilient_sop\n";
  if (comprehensive)
    out << "\tSame as \"-resilient\" except that the error resilient code-"
           "stream parsing algorithm is informed that it can expect SOP "
           "markers to appear in front of every single packet, whenever "
           "the relevant flag in the Scod style byte of the COD marker is "
           "set.  The JPEG2000 standard interprets this flag as meaning "
           "that SOP markers may appear; however, this does not give the "
           "decompressor any idea where it can expect SOP markers "
           "to appear.  In most cases, SOP markers, if used, will be placed "
           "in front of every packet and knowing this a priori can "
           "improve the performance of the error resilient parser.\n";
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
           "\t   For effective use of parallel processing resources, you "
           "should consider creating at least one thread for each CPU; you "
           "should also consider using the `-double_buffering' option to "
           "minimize the amount of time threads might potentially sit idle.\n"
           "\t   If the `-num_threads' argument is not supplied explicitly, "
           "the default behaviour is to create a threading environment only "
           "if the system offers multiple CPU's (or virtual CPU's), with "
           "one thread per CPU.  However, this default behaviour depends "
           "upon knowledge of the number of CPU's which are available -- "
           "something which cannot always be accurately determined through "
           "system calls.  The default value might also not yield the "
           "best possible throughput.\n";
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
           "`-num_threads' and `-double_buffering' options.\n"
           "\t   You may find the `-double_buffering' option to be useful "
           "even when working with only one thread, or when no threading "
           "environment is created at all (see `-num_threads').  In these "
           "cases, synthesized DWT lines are only single buffered, but "
           "increasing the size of this buffer (stripe height) allows the "
           "thread to do more work within a single tile-component, before "
           "moving to another tile-component.\n";
  out << "-progress <interval>\n";
  if (comprehensive)
    out << "\tThis option is useful when decompressing massive images; it "
          "allows you to receive feedback each time a vertical row of tiles "
          "has been processed, but potentially more frequently, depending "
          "upon the <interval> parameter.  The <interval> parameter "
          "indicates the maximum number of lines that can be pulled from the "
          "compression machinery before some progress is provided -- if this "
          "value is smaller than the tile height, you will receive periodic "
          "information about the percentage of the vertical row of tiles "
          "which has been processed.\n";
  out << "-cpu <coder-iterations>\n";
  if (comprehensive)
    out << "\tTimes end-to-end execution and, optionally, the block decoding "
           "operation, reporting throughput statistics.  If "
           "`coder-iterations' is 0, the block decoder will not be timed, "
           "leading to the most accurate end-to-end system execution "
           "times.  Otherwise, `coder-iterations' must be a positive "
           "integer -- larger values will result in more accurate "
           "estimates of the block decoder processing time, but "
           "degrade the accuracy of end-to-end execution times.  "
           "Note that end-to-end times include image file writing, which "
           "can have a dominant impact.  To avoid this, you may specify "
           "no output files at all.  Note also that timing information may "
           "not be at all reliable unless `-num_threads' is 1.  Since the "
           "default value for the `-num_threads' argument may be greater "
           "than 1, you should explicitly set the number of threads to 1 "
           "before collecting timing information.\n";
  out << "-mem -- Report memory usage\n";
  out << "-stats -- Report parsed data stats by resolution & quality layer\n";
  if (comprehensive)
    out << "\tThis argument exercises the `kdu_tile::get_parsed_packet_stats' "
           "function, new to Kakadu v6.1, to report the number of compressed "
           "bytes (packet headers and packet bodies) parsed up to each "
           "quality layer boundary, within each resolution.  If you "
           "decompress the enire image, with no restrictions, the reported "
           "numbers should be independent of random access or organizational "
           "properties of the codestream.  However, if you decompress only "
           "a limited region of interest, limited number of quality layers "
           "or limited resolution, the amount of data which actually gets "
           "parsed depends on whether or not the codestream's precincts can "
           "be randomly accessed and, if not, on the packet progression "
           "order used by the codestream.\n";
  out << "-com -- requests the printing of textual codestream comments.\n";
  out << "-s <switch file>\n";
  if (comprehensive)
    out << "\tSwitch to reading arguments from a file.  In the file, argument "
           "strings are separated by whitespace characters, including spaces, "
           "tabs and new-line characters.  Comments may be included by "
           "introducing a `#' or a `%' character, either of which causes "
           "the remainder of the line to be discarded.  Any number of "
           "\"-s\" argument switch commands may be included on the command "
           "line.\n";
  out << "-record <file>\n";
  if (comprehensive)
    out << "\tRecord code-stream parameters in a file, using the same format "
           "which is accepted when specifying the parameters to the "
           "compressor. Parameters specific to tiles which do not intersect "
           "with the region of interest will not generally be recorded.\n";
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

static kde_file_binding *
  parse_simple_args(kdu_args &args, char * &ifname,
                    std::ostream * &record_stream,
                    float &max_bpp, bool &transpose, bool &vflip, bool &hflip,
                    bool &allow_shorts, int &skip_components,
                    bool &want_alpha, int &jpx_layer, int &raw_codestream,
                    kdu_component_access_mode &component_access_mode,
                    bool &no_seek, int &max_layers, int &discard_levels,
                    int &num_threads, int &double_buffering_height,
                    int &progress_interval, int &cpu_iterations,
                    bool &simulate_parsing, bool &mem, bool &stats,
                    bool &want_comments, bool &quiet)
  /* Parses most simple arguments (those involving a dash). Most parameters are
     returned via the reference arguments, with the exception of the input
     file names, which are returned via a linked list of `kde_file_binding'
     objects.  Only the `fname' field of each `kde_file_binding' record is
     filled out here.  Note that `max_bpp' is returned as negative if the
     bit-rate is not explicitly set.  Note also that the function may return
     NULL if no output files are specified; in this case, the decompressor
     is expected to run completely, but not output anything. The value returned
     via `cpu_iterations' is negative unless CPU times are required.  The
     `raw_codestream' variable will be set to the index of the code-stream
     to be decompressed if the `-raw_components' argument was used.  Otherwise
     it will be set to a negative integer.
        Note that `num_threads' is set to 0 if no multi-threaded processing
     group is to be created, as distinct from a value of 1, which means
     that a multi-threaded processing group is to be used, but this group
     will involve only one thread. */
{
  int rotate;
  kde_file_binding *files, *last_file, *new_file;

  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  files = last_file = NULL;
  ifname = NULL;
  record_stream = NULL;
  rotate = 0;
  max_bpp = -1.0F;
  allow_shorts = true;
  skip_components = 0;
  want_alpha = true;
  jpx_layer = 0;
  raw_codestream = -1;
  component_access_mode = KDU_WANT_OUTPUT_COMPONENTS;
  no_seek = false;
  max_layers = 0;
  discard_levels = 0;
  num_threads = 0; // This is not actually the default -- see below.
  double_buffering_height = 0; // i.e., no double buffering
  progress_interval = 0; // Don't provide any progress indication at all
  cpu_iterations = -1;
  simulate_parsing = false;
  mem = false;
  stats = false;
  want_comments = false;
  quiet = false;

  if (args.find("-o") != NULL)
    {
      char *string, *cp;
      int len;

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "\"-o\" argument requires a file name!"; }
      while ((len = (int) strlen(string)) > 0)
        {
          cp = strchr(string,',');
          if (cp == NULL)
            cp = string+len;
          new_file = new kde_file_binding(string,(int)(cp-string));
          if (last_file == NULL)
            files = last_file = new_file;
          else
            last_file = last_file->next = new_file;
          if (*cp == ',') cp++;
          string = cp;
        }
      args.advance();
    }

  if (args.find("-i") != NULL)
    {
      if ((ifname = args.advance()) == NULL)
        { kdu_error e; e << "\"-i\" argument requires a file name!"; }
      args.advance();
    }

  if (args.find("-jpx_layer") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&jpx_layer) != 1) ||
          (jpx_layer < 0))
        { kdu_error e; e << "\"-jpx_layer\" argument requires a "
          "non-negative integer parameter!"; }
      args.advance();
    }

  if (args.find("-raw_components") != NULL)
    {
      raw_codestream = 0;
      char *string = args.advance();
      if ((string != NULL) && (*string != '-') &&
          (sscanf(string,"%d",&raw_codestream) == 1))
        args.advance();
    }

  if (args.find("-codestream_components") != NULL)
    {
      component_access_mode = KDU_WANT_CODESTREAM_COMPONENTS;
      args.advance();
    }

  if (args.find("-rate") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%f",&max_bpp) != 1) ||
          (max_bpp <= 0.0F))
        { kdu_error e; e << "\"-rate\" argument requires a positive "
          "numeric parameter!"; }
      args.advance();
    }

  if (args.find("-simulate_parsing") != NULL)
    {
      args.advance();
      simulate_parsing = true;
    }

  if (args.find("-skip_components") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&skip_components) != 1) ||
          (skip_components < 0))
        { kdu_error e; e << "\"-skip_components\" argument requires a "
          "non-negative integer parameter!"; }
      args.advance();
    }

  if (args.find("-no_alpha") != NULL)
    {
      want_alpha = false;
      args.advance();
    }

  if (args.find("-no_seek") != NULL)
    {
      no_seek = true;
      args.advance();
    }

  if (args.find("-layers") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_layers) != 1) ||
          (max_layers < 1))
        { kdu_error e; e << "\"-layers\" argument requires a positive "
          "integer parameter!"; }
      args.advance();
    }

  if (args.find("-reduce") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&discard_levels) != 1) ||
          (discard_levels < 0))
        { kdu_error e; e << "\"-reduce\" argument requires a non-negative "
          "integer parameter!"; }
      args.advance();
    }

  if (args.find("-rotate") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&rotate) != 1) ||
          ((rotate % 90) != 0))
        { kdu_error e; e << "\"-rotate\" argument requires an integer "
          "multiple of 90 degrees!"; }
      args.advance();
      rotate /= 90;
    }

  if (args.find("-precise") != NULL)
    {
      args.advance();
      allow_shorts = false;
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

  if (args.find("-progress") != NULL)
    { 
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&progress_interval) == 0) ||
          (progress_interval < 1))
        { kdu_error e; e << "\"-progress\" argument requires a positive "
          "integer parameter, identifying the maximum reporting interval."; }
      args.advance();
    }
  
  if (args.find("-cpu") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&cpu_iterations) != 1) ||
          (cpu_iterations < 0))
        { kdu_error e; e << "\"-cpu\" argument requires a non-negative "
          "integer, specifying the number of times to execute the block "
          "coder within a timing loop."; }
      args.advance();
    }

  if (args.find("-mem") != NULL)
    {
      mem = true;
      args.advance();
    }

  if (args.find("-stats") != NULL)
    {
      stats = true;
      args.advance();
    }

  if (args.find("-com") != NULL)
    {
      want_comments = true;
      args.advance();
    }
  
  if (args.find("-quiet") != NULL)
    {
      quiet = true;
      args.advance();
    }

  if (args.find("-record") != NULL)
    {
      char *fname = args.advance();
      if (fname == NULL)
        { kdu_error e; e << "\"-record\" argument requires a file name!"; }
      record_stream = new std::ofstream(fname);
      if (record_stream->fail())
        { kdu_error e; e << "Unable to open record file, \"" << fname << "\"."; }
      args.advance();
    }

  if (ifname == NULL)
    { kdu_error e; e << "Must provide an input file name!"; }
  while (rotate >= 4)
    rotate -= 4;
  while (rotate < 0)
    rotate += 4;
  switch (rotate) {
    case 0: transpose = false; vflip = false; hflip = false; break;
    case 1: transpose = true; vflip = false; hflip = true; break;
    case 2: transpose = false; vflip = true; hflip = true; break;
    case 3: transpose = true; vflip = true; hflip = false; break;
    }

  return(files);
}

/*****************************************************************************/
/* STATIC                   parse_forced_precisions                          */
/*****************************************************************************/

static void
  parse_forced_precisions(kdu_args &args, kdu_image_dims &idims)
{
  if (args.find("-fprec") == NULL)
    return;
  char *string = args.advance();
  if (string == NULL)
    { kdu_error e; e << "Malformed `-fprec' argument.  Expected a comma "
      "separated list of non-negative forced precision values, each of "
      "which may optionally be followed by at most an `M' suffix."; }    
  int comp_idx = 0;
  do {
    int val = 0;
    size_t len;
    const char *sep = strchr(string,',');
    if (sep != NULL)
      len = sep-string;
    else
      len = strlen(string);
    bool align_lsbs = true; // This is the default
    if ((len > 0) && (string[len-1] == 'M'))
      { string[len-1] = '\0'; align_lsbs = false; }
    else if ((len > 0) && (string[len-1] == 'L'))
      { string[len-1] = '\0'; align_lsbs = true; }
    if ((sscanf(string,"%d",&val) != 1) || (val < 0))
      { kdu_error e; e << "Malformed `-fprec' argument.  Expected a comma "
        "separated list of non-negative forced precision values, each of "
        "which may optionally be followed by at most an `M' suffix."; }    
    idims.set_forced_precision(comp_idx++,val,align_lsbs);
    string += len+1;
  } while (string[-1] == ',');
  args.advance();
}

/*****************************************************************************/
/* STATIC                    set_error_behaviour                             */
/*****************************************************************************/

static void
  set_error_behaviour(kdu_args &args, kdu_codestream codestream)
{
  bool fussy = false;
  bool resilient = false;
  bool ubiquitous_sops = false;
  if (args.find("-fussy") != NULL)
    { args.advance(); fussy = true; }
  if (args.find("-resilient") != NULL)
    { args.advance(); resilient = true; }
  if (args.find("-resilient_sop") != NULL)
    { args.advance(); resilient = true; ubiquitous_sops = true; }
  if (resilient)
    codestream.set_resilient(ubiquitous_sops);
  else if (fussy)
    codestream.set_fussy();
  else
    codestream.set_fast();
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
/* STATIC                     set_region_of_interest                         */
/*****************************************************************************/

static void
  set_region_of_interest(kdu_args &args, kdu_dims &region, siz_params *siz,
                         double &width_fraction, double &height_fraction)
  /* Parses the `-region' argument to see if a reduced region of interest
     is required.  Returns the region of interest, expressed on the
     original codestream canvas (no geometric transformations) along with
     the fraction of the full image width and height which are represented
     by this region. */
{
  width_fraction = height_fraction = 1.0;
  if (!(siz->get(Sorigin,0,0,region.pos.y) &&
        siz->get(Sorigin,0,1,region.pos.x) &&
        siz->get(Ssize,0,0,region.size.y) &&
        siz->get(Ssize,0,1,region.size.x)))
    assert(0);
  region.size.y -= region.pos.y;
  region.size.x -= region.pos.x;
  if (args.find("-region") == NULL)
    return;
  char *string = args.advance();
  if (string != NULL)
    {
      double top, left, height, width;

      if (sscanf(string,"{%lf,%lf},{%lf,%lf}",&top,&left,&height,&width) != 4)
        string = NULL;
      else if ((top < 0.0) || (left < 0.0) || (height < 0.0) || (width < 0.0))
        string = NULL;
      else
        {
          region.pos.y += (int) floor(region.size.y * top);
          region.pos.x += (int) floor(region.size.x * left);
          region.size.y = (int) ceil(region.size.y * height);
          region.size.x = (int) ceil(region.size.x * width);
          width_fraction = width;
          height_fraction = height;
        }
    }
  if (string == NULL)
    { kdu_error e; e << "The `-region' argument requires a set of coordinates "
      "of the form, \"{<top>,<left>},{<height>,<width>}\". All quantities "
      "must be real numbers in the range 0 to 1."; }
  args.advance();
}

/*****************************************************************************/
/* STATIC                 extract_jp2_resolution_info                        */
/*****************************************************************************/

static void
  extract_jp2_resolution_info(kdu_image_dims &idims, jp2_resolution resolution,
                              kdu_coords ref_size, double roi_width_fraction,
                              double roi_height_fraction, bool transpose)
  /* Extracts pixel resolution (and aspect ratio) information from the
     `resolution' interface and uses this to invoke `idims.set_resolution'
     so that resolution and associated information can be saved with the
     image.  Since the image might be rendered at a reduced resolution, or
     within a reduced region of interest, we send reference image dimensions
     to `idims.set_resolution', against which the display/capture resolution
     information should be assessed.  These are compared with the actual
     rendered image dimensions to determine how the pixel resolution
     information should be scaled -- done automatically by
     `idims.get_resolution'.  The reference image dimensions are taken from
     `ref_size', which is the full size of the relevant compositing layer
     (in its most general form, `jp2_resolution' is supposed to be interpreted
     relative to the compositing layer) and scaled by the `roi_..._fraction'
     parameters, which represent the fraction of the image width/height
     which remains after the region of interest has been extracted for
     saving to the output file.  These parameters are all expressed in the
     original coordinate image geometry (prior to any transposition), but
     the `idims' information needs to be configured for the final rendering
     geometry, which is why we need the `transpose' argument. */
{
  if ((ref_size.x <= 0) || (ref_size.y <= 0))
    return;

  // Start by seeing whether we have display resolution info, or only capture
  // resolution info.
  bool for_display=true, have_absolute_res=true;
  float ypels_per_metre = resolution.get_resolution(for_display);
  if (ypels_per_metre <= 0.0F)
    {
      for_display = false;
      ypels_per_metre = resolution.get_resolution(for_display);
      if (ypels_per_metre <= 0.0F)
        { have_absolute_res = false; ypels_per_metre = 1.0F; }
    }
  float xpels_per_metre =
    ypels_per_metre * resolution.get_aspect_ratio(for_display);
  assert(xpels_per_metre > 0.0F);
  double ref_width = ((double) ref_size.x) * roi_width_fraction;
  double ref_height = ((double) ref_size.y) * roi_height_fraction;
  if (transpose)
    idims.set_resolution(ref_height,ref_width,have_absolute_res,
                         ypels_per_metre,xpels_per_metre);
  else
    idims.set_resolution(ref_width,ref_height,have_absolute_res,
                         xpels_per_metre,ypels_per_metre);
}

/*****************************************************************************/
/* STATIC                   extract_jp2_colour_info                          */
/*****************************************************************************/

static void
  extract_jp2_colour_info(kdu_image_dims &idims, jp2_channels channels,
                          jp2_colour colour, bool have_alpha,
                          bool alpha_is_premultiplied)
{
  int num_colours = channels.get_num_colours();
  bool have_premultiplied_alpha = have_alpha && alpha_is_premultiplied;
  bool have_unassociated_alpha = have_alpha && !alpha_is_premultiplied;
  int colour_space_confidence = 1;
  jp2_colour_space space = colour.get_space();
  if ((space == JP2_iccLUM_SPACE) ||
      (space == JP2_iccRGB_SPACE) ||
      (space == JP2_iccANY_SPACE) ||
      (space == JP2_vendor_SPACE))
    colour_space_confidence = 0;
  idims.set_colour_info(num_colours,have_premultiplied_alpha,
                        have_unassociated_alpha,colour_space_confidence,
                        space);
}

/*****************************************************************************/
/* STATIC             convert_samples_to_palette_indices                     */
/*****************************************************************************/

static void
  convert_samples_to_palette_indices(kdu_line_buf &src, kdu_line_buf &dst,
                                     int bit_depth, bool is_signed,
                                     int palette_bits)
{
  int i=src.get_width();
  kdu_sample16 *dp = dst.get_buf16();
  assert(dp != NULL);
  if (src.get_buf32() != NULL)
    {
      kdu_sample32 *sp = src.get_buf32();
      if (src.is_absolute())
        {
          kdu_int32 offset = (is_signed)?0:((1<<bit_depth)>>1);
          kdu_int32 mask = ((kdu_int32)(-1))<<palette_bits;
          kdu_int32 val;
          for (; i > 0; i--, sp++, dp++)
            {
              val = sp->ival + offset;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp->ival = (kdu_int16) val;
            }
        }
      else
        {
          float scale = (float)(1<<(palette_bits+10)); // 10 temp fraction bits
          kdu_int32 val;
          kdu_int32 offset = (is_signed)?0:(1<<(palette_bits+9));
          offset += (1<<9); // Rounding offset.
          kdu_int32 mask = ((kdu_int32)(-1))<<palette_bits;
          for (; i > 0; i--, sp++, dp++)
            {
              val = (kdu_int32)(sp->fval * scale);
              val = (val+offset)>>10;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp->ival = (kdu_int16) val;
            }
        }
    }
  else
    {
      kdu_sample16 *sp = src.get_buf16();
      if (src.is_absolute())
        {
          kdu_int16 offset=(kdu_int16)((is_signed)?0:((1<<bit_depth)>>1));
          kdu_int16 mask = ((kdu_int16)(-1))<<palette_bits;
          kdu_int16 val;
          for (; i > 0; i--, sp++, dp++)
            {
              val = sp->ival + offset;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp->ival = val;
            }
        }
      else
        {
          kdu_int16 offset=(kdu_int16)((is_signed)?0:((1<<KDU_FIX_POINT)>>1));
          int downshift = KDU_FIX_POINT-palette_bits; assert(downshift > 0);
          offset += (kdu_int16)((1<<downshift)>>1);
          kdu_int32 mask = ((kdu_int16)(-1))<<palette_bits;
          kdu_int16 val;
          for (; i > 0; i--, sp++, dp++)
            {
              val = (sp->ival + offset) >> downshift;
              if (val & mask)
                val = (val<0)?0:(~mask);
              dp->ival = val;
            }
        }
    }
}

/*****************************************************************************/
/* STATIC                 print_parsed_packet_stats                          */
/*****************************************************************************/

static void
  print_parsed_packet_stats(int num_layers, int num_resolutions,
                            kdu_long *reslayer_bytes,
                            kdu_long *reslayer_packets,
                            kdu_long *resmax_packets)
{
  int r, n, used_layers=1;
  for (n=0; n < num_layers; n++)
    {
      for (r=0; r < num_resolutions; r++)
        if (reslayer_packets[n+r*num_layers] != 0)
          break;
      if (r < num_resolutions)
        used_layers = n+1;
    }
  pretty_cout << "\n";
  pretty_cout << "Parsed packet statistics by quality layer\n";
  pretty_cout << "-----------------------------------------\n";
  for (r=0; r < num_resolutions; r++)
    {
      if (r == 0)
        pretty_cout << "Resolution being expanded:\n";
      else
        pretty_cout << "Additional packets parsed from higher resolutions by "
                    << r << ":\n";
      pretty_cout << "\t";
      kdu_long num_bytes = 0;
      kdu_long max_packets = resmax_packets[r];
      for (n=0; n < used_layers; n++)
        {
          num_bytes += reslayer_bytes[n+r*num_layers];
          kdu_long num_packets = reslayer_packets[n+r*num_layers];
          if (r > 0)
            {
              num_bytes -= reslayer_bytes[n+(r-1)*num_layers];
              num_packets -= reslayer_packets[n+(r-1)*num_layers];
              max_packets -= resmax_packets[r-1];
            }
          double percent = 100.0;
          if (max_packets > 0)
            percent = 100.0 * ((double) num_packets) / ((double) max_packets);
          if (n > 0)
            pretty_cout << ", ";
          pretty_cout << num_bytes << "(" << ((int) percent) << "%)"; 
        }
      pretty_cout << "\n";
    }
}

/*****************************************************************************/
/* STATIC                   expand_single_threaded                           */
/*****************************************************************************/

static kdu_long
  expand_single_threaded(kdu_codestream codestream, kdu_dims tile_indices,
                         kde_file_binding *outputs, int num_output_channels,
                         bool last_output_channel_is_alpha,
                         bool alpha_is_premultiplied,
                         int num_used_components, int *used_component_indices,
                         jp2_channels channels, jp2_palette palette,
                         bool allow_shorts, bool skip_ycc,
                         int dwt_stripe_height,
                         int num_stats_layers, int num_stats_resolutions,
                         kdu_long *stats_reslayer_bytes,
                         kdu_long *stats_reslayer_packets,
                         kdu_long *stats_resmax_packets,
                         int progress_interval)
  /* This function wraps up the operations required to actually decompress
     the image samples.  It is called directly from `main' after setting up
     the output files (passed in via the `outputs' list), configuring the
     `codestream' object and parsing relevant command-line arguments.
        This particular function implements all decompression processing
     using a single thread of execution.  This is the simplest approach.
     From version 5.1 of Kakadu, the processing may also be efficiently
     distributed across multiple threads, which allows for the
     exploitation of multiple physical processors.  The implementation in
     that case is only slightly different from the multi-threaded case, but
     we encapsulate it in a separate version of this function,
     `expand_multi_threaded', mainly for illustrative purposes.
        The function returns the amount of memory allocated for sample
     processing, including all intermediate line buffers managed by
     the DWT engines associated with each active tile-component and the block
     decoding machinery associated with each tile-component-subband.
        The implementation here processes image lines one-by-one,
     maintaining W complete tile processing engines, where W is the number
     of tiles which span the width of the image (or the image region which
     is being reconstructed).  There are a variety of alternate processing
     paradigms which can be used.  The "kdu_buffered_expand" application
     demonstrates a different strategy, managed by the higher level
     `kdu_stripe_decompressor' object, in which whole image stripes are
     decompressed into a memory buffer.  If the stripe height is equal to
     the tile height, only one tile processing engine need be active at
     any given time in that model.  Yet another model is used by the
     `kdu_region_decompress' object, which decompresses a specified image
     region into a memory buffer.  In that case, processing is done
     tile-by-tile. */
{
  int x_tnum;
  kde_flow_control **tile_flows = new kde_flow_control *[tile_indices.size.x];
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      tile_flows[x_tnum] = new
        kde_flow_control(outputs,num_output_channels,
                         last_output_channel_is_alpha,alpha_is_premultiplied,
                         num_used_components,used_component_indices,
                         codestream,x_tnum,allow_shorts,channels,palette,
                         skip_ycc,dwt_stripe_height);
      if (num_stats_layers > 0)
        tile_flows[x_tnum]->collect_layer_stats(num_stats_layers,
                                                num_stats_resolutions,
                                                num_stats_resolutions-1,
                                                stats_reslayer_bytes,
                                                stats_reslayer_packets,
                                                stats_resmax_packets,NULL);
    }
  bool done = false;
  int tile_row = 0; // Just for progress counter
  int progress_counter = 0;
  while (!done)
    { 
      while (!done)
        { // Process a row of tiles line by line.
          done = true;
          for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
            {
              if (tile_flows[x_tnum]->advance_components())
                {
                  done = false;
                  tile_flows[x_tnum]->process_components();
                }
            }
          if ((!done) && ((++progress_counter) == progress_interval))
            { 
              pretty_cout << "\t\tProgress with current tile row = "
                          << tile_flows[0]->percent_pulled() << "%\n";
              progress_counter = 0;
            }
        }
      for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
        if (tile_flows[x_tnum]->advance_tile())
          done = false;
      tile_row++;
      progress_counter = 0;
      if (progress_interval > 0)
        pretty_cout << "\tFinished processing " << tile_row
                    << " of " << tile_indices.size.y << " tile rows\n";
    }
  kdu_long processing_sample_bytes = 0;
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      processing_sample_bytes += tile_flows[x_tnum]->get_buffer_memory();
      delete tile_flows[x_tnum];
    }
  delete[] tile_flows;

  return processing_sample_bytes;
}

/*****************************************************************************/
/* STATIC                    expand_multi_threaded                           */
/*****************************************************************************/


static kdu_long
  expand_multi_threaded(kdu_codestream codestream, kdu_dims tile_indices,
                        kde_file_binding *outputs, int num_output_channels,
                        bool last_output_channel_is_alpha,
                        bool alpha_is_premultiplied,
                        int num_used_components, int *used_component_indices,
                        jp2_channels channels, jp2_palette palette,
                        bool allow_shorts, bool skip_ycc, int &num_threads,
                        bool dwt_double_buffering, int dwt_stripe_height,
                        int num_stats_layers, int num_stats_resolutions,
                        kdu_long *stats_reslayer_bytes,
                        kdu_long *stats_reslayer_packets,
                        kdu_long *stats_resmax_packets,
                        int progress_interval)
  /* This function provides exactly the same functionality as
     `expand_single_threaded', except that it uses Kakadu's multi-threaded
     processing features.  By and large, multi-threading does not substantially
     complicate the implementation, since Kakadu's threading framework
     conceal almost all of the details.  However, the application does have
     to create a multi-threaded environment, assigning it a suitable number
     of threads.  It must also be careful to close down the multi-threaded
     environment, which incorporates all required synchronization.
        Upon return, `num_threads' is set to the actual number of threads
     which were created -- this value could be smaller than the value supplied
     on input, if insufficient internal resources exist.
        For other aspects of the present function, refer to the comments
     found with `expand_single_threaded'. */
{
  // Construct multi-threaded processing environment if required
  kdu_thread_env env;
  env.create();
  for (int nt=1; nt < num_threads; nt++)
    if (!env.add_thread())
      { num_threads = nt; break; }

  // Now set up the tile processing objects
  int x_tnum;
  kde_flow_control **tile_flows = new kde_flow_control *[tile_indices.size.x];
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      kdu_thread_queue *tile_queue =
        env.add_queue(NULL,NULL,"tile expander");
      tile_flows[x_tnum] = new
        kde_flow_control(outputs,num_output_channels,
                         last_output_channel_is_alpha,alpha_is_premultiplied,
                         num_used_components,used_component_indices,
                         codestream,x_tnum,allow_shorts,channels,palette,
                         skip_ycc,dwt_stripe_height,dwt_double_buffering,
                         &env,tile_queue);
      if (num_stats_layers > 0)
        tile_flows[x_tnum]->collect_layer_stats(num_stats_layers,
                                                num_stats_resolutions,
                                                num_stats_resolutions-1,
                                                stats_reslayer_bytes,
                                                stats_reslayer_packets,
                                                stats_resmax_packets,&env);
    }

  // Now run the tile processing engines
  bool done = false;
  int tile_row = 0; // Just for progress counter
  int progress_counter = 0;
  while (!done)
    { 
      while (!done)
        { // Process a row of tiles line by line.
          done = true;
          for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
            {
              if (tile_flows[x_tnum]->advance_components(&env))
                {
                  done = false;
                  tile_flows[x_tnum]->process_components();
                }
            }
          if ((!done) && ((++progress_counter) == progress_interval))
            { 
              pretty_cout << "\t\tProgress with current tile row = "
                          << tile_flows[0]->percent_pulled() << "%\n";
              progress_counter = 0;
            }
        }
      for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
        if (tile_flows[x_tnum]->advance_tile(&env))
          done = false;
      tile_row++;
      progress_counter = 0;
      if (progress_interval > 0)
        pretty_cout << "\tFinished processing " << tile_row
                    << " of " << tile_indices.size.y << " tile rows\n";
    }

  // Cleanup processing environment
  env.terminate(NULL,true);
  env.destroy();
  kdu_long processing_sample_bytes = 0;
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      processing_sample_bytes += tile_flows[x_tnum]->get_buffer_memory();
      delete tile_flows[x_tnum];
    }
  delete[] tile_flows;

  return processing_sample_bytes;
}

/* ========================================================================= */
/*                              kde_flow_control                             */
/* ========================================================================= */

/*****************************************************************************/
/*                     kde_flow_control::kde_flow_control                    */
/*****************************************************************************/

kde_flow_control::kde_flow_control(kde_file_binding *files, int num_channels,
                      bool last_channel_is_alpha, bool alpha_is_premultiplied,
                      int num_components, const int *component_indices,
                      kdu_codestream codestream, int x_tnum, bool allow_shorts,
                      jp2_channels channel_mapping, jp2_palette palette,
                      bool skip_ycc, int dwt_stripe_height,
                      bool dwt_double_buffering, kdu_thread_env *env,
                      kdu_thread_queue *env_queue)
{
  int c;

  this->codestream = codestream;
  codestream.get_valid_tiles(this->valid_tile_indices);
  assert((x_tnum >= 0) && (x_tnum < valid_tile_indices.size.x));
  this->tile_idx = valid_tile_indices.pos;
  this->tile_idx.x += x_tnum;
  this->x_tnum = x_tnum;
  this->tile = codestream.open_tile(tile_idx,env);
  this->num_channels = num_channels;
  channels = new kde_channel[num_channels];
  this->component_indices = component_indices;
  this->allow_shorts = allow_shorts;
  this->skip_ycc = skip_ycc;
  this->dwt_double_buffering = dwt_double_buffering;
  this->dwt_stripe_height = dwt_stripe_height;
  this->env_queue = (env==NULL)?NULL:env_queue;
  assert((env == NULL) || (env_queue != NULL));
  
  stats_num_layers = 0;
  stats_num_resolutions = 0;
  stats_max_discard_levels = 0;
  stats_resolution_layer_bytes = NULL;
  stats_resolution_layer_packets = NULL;
  stats_resolution_max_packets = NULL;

  // Create processing engine
  this->max_buffer_memory =
    engine.create(codestream,tile,!allow_shorts,skip_ycc,false,
                  dwt_stripe_height,env,env_queue,dwt_double_buffering);

  // Set up the individual components
  this->num_components = num_components;
  components = new kde_component_flow_control[num_components];
  count_delta = 0;
  for (c=0; c < num_components; c++)
    {
      kde_component_flow_control *comp = components + c;
      comp->line = NULL;
      comp->is_signed = codestream.get_signed(c,true);
      comp->bit_depth = codestream.get_bit_depth(c,true);
      comp->mapped_by_channel = false;
      comp->palette_bits = 0;
      kdu_coords subsampling;  codestream.get_subsampling(c,subsampling,true);
      kdu_dims dims;  codestream.get_tile_dims(tile_idx,c,dims,true);
      comp->width = dims.size.x;
      comp->vert_subsampling = subsampling.y;
      if ((c == 0) || (comp->vert_subsampling < count_delta))
        count_delta = comp->vert_subsampling; // Delta is min sampling factor
      comp->ratio_counter = 0;
      comp->remaining_lines = comp->initial_lines = dims.size.y;
    }

  // Initialize channels
  for (c=0; c < num_channels; c++)
    {
      kde_channel *chnl = channels + c;
      if (files != NULL)
        {
          assert(c >= files->first_channel_idx);
          if ((c-files->first_channel_idx) >= files->num_channels)
            {
              files = files->next;
              assert((files != NULL) && (files->first_channel_idx == c));
            }
          chnl->writer = files->writer;
        }
      chnl->lut = NULL;
      if (channel_mapping.exists())
        {
          int cmp, plt_cmp, stream, i;
          bool success;

          if ((c==(num_channels-1)) && last_channel_is_alpha)
            success = (alpha_is_premultiplied)?
              channel_mapping.get_premult_mapping(0,cmp,plt_cmp,stream):
              channel_mapping.get_opacity_mapping(0,cmp,plt_cmp,stream);
          else 
            success = channel_mapping.get_colour_mapping(c,cmp,plt_cmp,stream);
          assert(success);
          for (i=0; i < num_components; i++)
            if (component_indices[i] == cmp)
              break;
          assert(i < num_components);
          chnl->source_component = components + i;
          if (plt_cmp >= 0)
            { // Set up palette lookup table.
              int i, num_entries = palette.get_num_entries();
              assert(num_entries <= 1024);
              int palette_bits = 1;
              while ((1<<palette_bits) < num_entries)
                palette_bits++;
              chnl->source_component->palette_bits = palette_bits;
              chnl->lut = new kdu_sample16[(int)(1<<palette_bits)];
              palette.get_lut(plt_cmp,chnl->lut);
              for (i=num_entries; i < (1<<palette_bits); i++)
                chnl->lut[i] = chnl->lut[num_entries-1];
            }
        }
      else
        chnl->source_component = components + c;
      chnl->source_component->mapped_by_channel = true;
      chnl->width = chnl->source_component->width;
    }

  // Complete components and channels
  for (c=0; c < num_components; c++)
    {
      kde_component_flow_control *comp = components + c;
      assert(comp->mapped_by_channel); // Otherwise the `num_components' and
            // `component_indices' arguments were not reflective of the set of
            // components which are actually being used.
      if (comp->palette_bits > 0)
        comp->palette_indices.pre_create(&aux_allocator,comp->width,
                                         true,true);
    }
  for (c=0; c < num_channels; c++)
    if (channels[c].lut != NULL)
      channels[c].palette_output.pre_create(&aux_allocator,channels[c].width,
                                            false,true);

  // Finalize any palette resources
  aux_allocator.finalize();
  for (c=0; c < num_components; c++)
    components[c].palette_indices.create(); // Harmless if not pre-created.
  for (c=0; c < num_channels; c++)
    channels[c].palette_output.create(); // Does no harm if not pre-created.

  this->max_buffer_memory += aux_allocator.get_size();
}

/*****************************************************************************/
/*                    kde_flow_control::~kde_flow_control                    */
/*****************************************************************************/

kde_flow_control::~kde_flow_control()
{
  if (components != NULL)
    delete[] components;
  for (int c=0; c < num_channels; c++)
    {
      kde_channel *chnl = channels + c;
      if (chnl->lut != NULL)
        delete[] (chnl->lut);
    }
  delete[] channels;
  if (engine.exists())
    engine.destroy();
}

/*****************************************************************************/
/*                    kde_flow_control::collect_layer_stats                  */
/*****************************************************************************/

void
  kde_flow_control::collect_layer_stats(int num_layers, int num_resolutions,
                                        int max_discard_levels,
                                        kdu_long *resolution_layer_bytes,
                                        kdu_long *resolution_layer_packets,
                                        kdu_long *resolution_max_packets,
                                        kdu_thread_env *env)
{
  this->stats_num_layers = num_layers;
  this->stats_num_resolutions = num_resolutions;
  this->stats_max_discard_levels = max_discard_levels;
  this->stats_resolution_layer_bytes = resolution_layer_bytes;
  this->stats_resolution_layer_packets = resolution_layer_packets;
  this->stats_resolution_max_packets = resolution_max_packets;
}

/*****************************************************************************/
/*                    kde_flow_control::advance_components                   */
/*****************************************************************************/

bool
  kde_flow_control::advance_components(kdu_thread_env *env)
{
  bool found_line=false;

  while (!found_line)
    {
      bool all_done = true;
      for (int n=0; n < num_components; n++)
        {
          kde_component_flow_control *comp = components + n;
          assert(comp->ratio_counter >= 0);
          if (comp->remaining_lines > 0)
            {
              all_done = false;
              comp->ratio_counter -= count_delta;
              if (comp->ratio_counter < 0)
                {
                  found_line = true;
                  comp->line = engine.get_line(n,env);
                  assert(comp->line != NULL);
                  if (comp->palette_bits > 0)
                    convert_samples_to_palette_indices(*(comp->line),
                                   comp->palette_indices,comp->bit_depth,
                                   comp->is_signed,comp->palette_bits);
                }
            }
        }
      if (all_done)
        return false;
    }

  for (int c=0; c < num_channels; c++)
    {
      kde_channel *chnl = channels + c;
      kde_component_flow_control *comp = chnl->source_component;
      if ((comp->ratio_counter < 0) && (chnl->lut != NULL))
        { // Perform LUT mapping.
          kdu_sample16 *lut = chnl->lut;
          kdu_sample16 *dp = chnl->palette_output.get_buf16();
          kdu_sample16 *sp = comp->palette_indices.get_buf16();
          assert(sp != NULL);
          for (int i=chnl->width; i > 0; i--, sp++, dp++)
            *dp = lut[sp->ival];
        }
    }

  return true;
}

/*****************************************************************************/
/*                 kde_flow_control::access_decompressed_line                */
/*****************************************************************************/

kdu_line_buf *
  kde_flow_control::access_decompressed_line(int channel_idx)
{
  assert((channel_idx >= 0) && (channel_idx < num_channels));
  kde_channel *chnl = channels + channel_idx;
  kde_component_flow_control *comp = chnl->source_component;
  if (comp->ratio_counter >= 0)
    return NULL;
  if (chnl->lut == NULL)
    return comp->line;
  else
    return &(chnl->palette_output);
}

/*****************************************************************************/
/*                    kde_flow_control::process_components                   */
/*****************************************************************************/

void
  kde_flow_control::process_components()
{
  for (int c=0; c < num_channels; c++)
    {
      kde_channel *chnl = channels + c;
      kde_component_flow_control *comp = chnl->source_component;
      if ((comp->ratio_counter < 0) && chnl->writer.exists())
        {
          if (chnl->lut == NULL)
            chnl->writer.put(c,*(comp->line),x_tnum);
          else
            chnl->writer.put(c,chnl->palette_output,x_tnum);
        }
    }

  for (int n=0; n < num_components; n++)
    {
      kde_component_flow_control *comp = components + n;
      if (comp->ratio_counter < 0)
        {
          comp->ratio_counter += comp->vert_subsampling;
          assert(comp->ratio_counter >= 0);
          assert(comp->remaining_lines > 0);
          comp->remaining_lines--;
        }
    }
}

/*****************************************************************************/
/*                        kde_flow_control::advance_tile                     */
/*****************************************************************************/

bool
  kde_flow_control::advance_tile(kdu_thread_env *env)
{
  int c;

  if (!tile)
    return false;

  assert(engine.exists());
  if (env != NULL)
    {
      assert(env_queue != NULL);
      env->terminate(env_queue,true);
    }
  engine.destroy();
  aux_allocator.restart();

  // Clean up existing resources
  for (c=0; c < num_components; c++)
    {
      components[c].palette_indices.destroy();
      components[c].line = NULL;
    }
  for (c=0; c < num_channels; c++)
    channels[c].palette_output.destroy();
  
  // Collect statistics, if desired
  if (stats_num_layers > 0)
    {
      int r, dlevs;
      for (r=0, dlevs=stats_max_discard_levels; r < stats_num_resolutions;
           r++, dlevs--)
        {
          int off = r*stats_num_layers;
          stats_resolution_max_packets[r] +=
            tile.get_parsed_packet_stats(-1,dlevs,stats_num_layers,
                                         stats_resolution_layer_bytes+off,
                                         stats_resolution_layer_packets+off);
        }
    }

  // Advance to next vertical tile.
  tile.close(env);
  tile = kdu_tile(NULL);
  tile_idx.y++;
  if ((tile_idx.y-valid_tile_indices.pos.y) == valid_tile_indices.size.y)
    return false;
  tile = codestream.open_tile(tile_idx,env);

  // Create new processing engine
  kdu_long buffer_memory =
    engine.create(codestream,tile,!allow_shorts,skip_ycc,false,
                  dwt_stripe_height,env,env_queue,dwt_double_buffering);

  // Prepare for processing the new tile.
  for (c=0; c < num_components; c++)
    {
      kde_component_flow_control *comp = components + c;
      kdu_dims dims;  codestream.get_tile_dims(tile_idx,c,dims,true);
      comp->ratio_counter = 0;
      comp->remaining_lines = comp->initial_lines = dims.size.y;
      if (comp->palette_bits > 0)
        comp->palette_indices.pre_create(&aux_allocator,comp->width,
                                         true,true);
    }
  for (c=0; c < num_channels; c++)
    if (channels[c].lut != NULL)
      channels[c].palette_output.pre_create(&aux_allocator,channels[c].width,
                                            false,true);

  // Finalize any palette resources
  aux_allocator.finalize();
  for (c=0; c < num_components; c++)
    components[c].palette_indices.create(); // Harmless if not pre-created.
  for (c=0; c < num_channels; c++)
    channels[c].palette_output.create(); // Does no harm if not pre-created.

  buffer_memory += aux_allocator.get_size();
  if (buffer_memory > max_buffer_memory)
    max_buffer_memory = buffer_memory;

  return true;
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

  bool transpose, vflip, hflip, allow_shorts, mem, stats, quiet;
  char *ifname;
  std::ostream *record_stream;
  float max_bpp;
  bool want_comments, no_seek, simulate_parsing, want_alpha;
  int jpx_layer_idx, raw_codestream, skip_components;
  int max_layers, discard_levels;
  int num_threads, double_buffering_height, progress_interval, cpu_iterations;
  kdu_component_access_mode component_access_mode;
  kde_file_binding *outputs =
    parse_simple_args(args,ifname,record_stream,max_bpp,
                      transpose,vflip,hflip,allow_shorts,
                      skip_components,want_alpha,jpx_layer_idx,
                      raw_codestream,component_access_mode,no_seek,
                      max_layers,discard_levels,
                      num_threads,double_buffering_height,progress_interval,
                      cpu_iterations,simulate_parsing,mem,stats,
                      want_comments,quiet);
  assert((component_access_mode == KDU_WANT_CODESTREAM_COMPONENTS) ||
         (component_access_mode == KDU_WANT_OUTPUT_COMPONENTS));

  // Open the input file in an appropriate way
  kdu_compressed_source *input = NULL;
  kdu_simple_file_source file_in;
  jp2_family_src jp2_ultimate_src;
  jpx_source jpx_in;
  jpx_codestream_source jpx_stream;
  jpx_layer_source jpx_layer;
  jp2_channels channels;
  jp2_palette palette;
  jp2_resolution resolution;
  jp2_colour colour;
  jp2_ultimate_src.open(ifname,!no_seek);
  kdu_coords layer_size; // Zero unless `jpx_layer' exists.  Records the size
        // of the layer on its registration.  According to the JPX standard,
        // this is the size against which any `jp2_resolution' information is
        // to be interpreted -- interpretation is backward compatible with JP2.
  if (jpx_in.open(&jp2_ultimate_src,true) < 0)
    { // Not compatible with JP2 or JPX.  Try opening as a raw code-stream.
      jp2_ultimate_src.close();
      file_in.open(ifname,!no_seek);
      input = &file_in;
      raw_codestream = 0;
    }
  else
    { // We have an open JP2/JPX-compatible file.
      if (skip_components && (raw_codestream < 0))
        { kdu_error e; e << "The `-skip_components' argument may be "
          "used only with raw code-stream sources or with the "
          "`-raw_components' switch."; }
      if (raw_codestream >= 0)
        {
          jpx_stream = jpx_in.access_codestream(raw_codestream);
          if (!jpx_stream)
            { kdu_error e; e << "Unable to find the code-stream identified "
              "by your `-raw_components' argument.  Note that the first "
              "code-stream in the file has an index of 0."; }
        }
      else
        {
          if (component_access_mode == KDU_WANT_CODESTREAM_COMPONENTS)
            { kdu_error e; e << "The `-codestream_components' flag may "
              "not be used with JP2/JPX sources unless you explicitly "
              "identify raw codestream access via the `-raw_components' "
              "argument."; }
          jpx_layer = jpx_in.access_layer(jpx_layer_idx);
          if (!jpx_layer)
            { kdu_error e; e << "Unable to find the compositing layer "
              "identified by your `-jpx_layer' argument.  Note that the "
              "first layer in the file has an index of 0."; }
          channels = jpx_layer.access_channels();
          resolution = jpx_layer.access_resolution();
          colour = jpx_layer.access_colour(0);
          layer_size = jpx_layer.get_layer_size();
          int cmp, plt, stream_id;
          channels.get_colour_mapping(0,cmp,plt,stream_id);
          jpx_stream = jpx_in.access_codestream(stream_id);
          palette = jpx_stream.access_palette();
        }
      input = jpx_stream.open_stream();
    }

  // Create the codestream object.
  kdu_codestream codestream;
  codestream.create(input);
  set_error_behaviour(args,codestream);
  if (cpu_iterations >= 0)
    codestream.collect_timing_stats(cpu_iterations);
  if ((max_bpp > 0.0F) || simulate_parsing)
    {
      kdu_long max_bytes = KDU_LONG_MAX;
      if (max_bpp > 0.0F)
        max_bytes = (kdu_long)
          (0.125 * max_bpp * get_bpp_dims(codestream.access_siz()));
      codestream.set_max_bytes(max_bytes,simulate_parsing);
    }
  kdu_message_formatter *formatted_recorder = NULL;
  kdu_stream_message recorder(record_stream);
  if (record_stream != NULL)
    {
      formatted_recorder = new kdu_message_formatter(&recorder);
      codestream.set_textualization(formatted_recorder);
    }
  kdu_dims region;
  double roi_width_fraction=1.0, roi_height_fraction=1.0;
  set_region_of_interest(args,region,codestream.access_siz(),
                         roi_width_fraction,roi_height_fraction);
  codestream.apply_input_restrictions(skip_components,0,discard_levels,
                                      max_layers,&region,
                                      component_access_mode);
  codestream.change_appearance(transpose,vflip,hflip);
  
  kdu_image_dims idims;
  parse_forced_precisions(args,idims);
  
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

  // Print codestream comments, if required
  if (want_comments)
    { 
      kdu_codestream_comment com;
      while ((com = codestream.get_comment(com)).exists())
        { 
          const char *com_text = com.get_text();
          if (com_text != NULL)
            printf("<<Codestream comment>>\n\"%s\"\n",com_text);
        }
    }
  
  // Get the component (or mapped colour channel) dimensional properties
  int n, num_channels, num_components;
  num_components = codestream.get_num_components(true);
  if (channels.exists())
    num_channels = channels.get_num_colours();
  else
    num_channels = num_components;

  for (n=0; n < num_channels; n++)
    {
      kdu_dims dims;
      int precision;
      bool is_signed;

      int cmp=n, plt=-1, stream_id;
      if (channels.exists())
        {
          channels.get_colour_mapping(n,cmp,plt,stream_id);
          if (stream_id != jpx_stream.get_codestream_id())
            {
              num_channels = n; // Process only initial channels which come
                                // from the same code-stream.
              break;
            }
        }
      codestream.get_dims(cmp,dims,true);
      if (plt < 0)
        {
          precision = codestream.get_bit_depth(cmp,true);
          is_signed = codestream.get_signed(cmp,true);
        }
      else
        {
          precision = palette.get_bit_depth(plt);
          is_signed = palette.get_signed(plt);
        }
      idims.add_component(dims.size.y,dims.size.x,precision,is_signed);
    }

  // See if we can add an extra alpha channel to the collection; this is a
  // little involved, since there are two types of alpha components which can
  // be represented in JP2/JPX files.  I have included this stuff despite
  // my inclination to leave it out of a demo application, only because
  // so many people seem to judge what Kakadu can do based on its demonstration
  // applications.
  bool last_channel_is_alpha=false, alpha_is_premultiplied=false;
  int alpha_component_idx=-1; // Valid only if `last_channel_is_alpha'
  if (want_alpha && channels.exists())
    {
      int cmp=-1, plt=-1, stream_id;
      if (channels.get_opacity_mapping(0,cmp,plt,stream_id) &&
          (stream_id == jpx_stream.get_codestream_id()))
        {
          last_channel_is_alpha = true;
          alpha_is_premultiplied = false;
          alpha_component_idx = cmp;
        }
      else if (channels.get_premult_mapping(0,cmp,plt,stream_id) &&
               (stream_id == jpx_stream.get_codestream_id()))
        {
          last_channel_is_alpha = true;
          alpha_is_premultiplied = true;
          alpha_component_idx = cmp;
        }
      if (last_channel_is_alpha)
        {
          kdu_dims dims;
          int precision;
          bool is_signed;

          num_channels++;
          codestream.get_dims(cmp,dims,true);
          if (plt < 0)
            {
              precision = codestream.get_bit_depth(cmp,true);
              is_signed = codestream.get_signed(cmp,true);
            }
          else
            {
              precision = palette.get_bit_depth(plt);
              is_signed = palette.get_signed(plt);
            }
          idims.add_component(dims.size.y,dims.size.x,precision,is_signed);
        }
    }

  if (resolution.exists())
    extract_jp2_resolution_info(idims,resolution,layer_size,
                                roi_width_fraction,roi_height_fraction,
                                transpose);
  if (channels.exists())
    extract_jp2_colour_info(idims,channels,colour,
                            last_channel_is_alpha,
                            alpha_is_premultiplied);
  if (jpx_in.exists())
    idims.set_meta_manager(jpx_in.access_meta_manager());

  // Now we are ready to open the output files.
  kde_file_binding *oscan;
  bool extra_vertical_flip = false;
  int num_output_channels=0;
  for (oscan=outputs; oscan != NULL; oscan=oscan->next)
    {
      bool flip;

      oscan->first_channel_idx = num_output_channels;
      oscan->writer =
        kdu_image_out(oscan->fname,idims,num_output_channels,flip,quiet);
      oscan->num_channels = num_output_channels - oscan->first_channel_idx;
      if (oscan == outputs)
        extra_vertical_flip = flip;
      if (extra_vertical_flip != flip)
        { kdu_error e; e << "Cannot mix output file types which have "
          "different vertical ordering conventions (i.e., top-to-bottom and "
          "bottom-to-top)."; }
    }
  if (extra_vertical_flip)
    {
      vflip = !vflip;
      codestream.change_appearance(transpose,vflip,hflip);
    }
  if (num_output_channels == 0)
    num_output_channels=num_channels; // No output files specified
  if (num_output_channels < num_channels)
    last_channel_is_alpha = false;


  // Find out which codestream components are actually used
  int num_used_components = 0;
  int *used_component_indices = new int[num_output_channels];
  for (n=0; n < num_output_channels; n++)
    {
      int c, cmp;

      if ((n==(num_channels-1)) && last_channel_is_alpha)
        cmp = alpha_component_idx;
      else if (channels.exists())
        {
          int plt, stream_id;
          channels.get_colour_mapping(n,cmp,plt,stream_id);
        }
      else
        cmp = skip_components+n;

      for (c=0; c < num_used_components; c++)
        if (used_component_indices[c] == cmp)
          break;
      if (c == num_used_components)
        used_component_indices[num_used_components++] = cmp;
    }

  // The following call saves us the cost of buffering up unused image
  // components.  All image components with indices other than those
  // recorded in `used_image_components' will be discarded.  However, we
  // have to remember from now on to use the indices of the relevant
  // component's entry in `used_component_indices' rather than the actual
  // component index, in calls into the `kdu_codestream' machinery.  This is
  // because the `apply_input_restrictions' function creates a new view of
  // the image in which the identified components are the only ones which
  // appear to be available.
  codestream.apply_input_restrictions(num_used_components,
                                      used_component_indices,discard_levels,
                                      max_layers,&region,
                                      component_access_mode);
  
  int num_stats_layers = 0;
  int num_stats_resolutions = 0;
  kdu_long *stats_reslayer_bytes = NULL;
  kdu_long *stats_reslayer_packets = NULL;
  kdu_long *stats_resmax_packets = NULL;
  if (stats)
    { // Set things up to collect parsed packet statistics
      num_stats_layers = 128;
      num_stats_resolutions = discard_levels+1;
      int num_stats_entries = num_stats_layers * num_stats_resolutions;
      stats_reslayer_bytes = new kdu_long[num_stats_entries];
      stats_reslayer_packets = new kdu_long[num_stats_entries];
      stats_resmax_packets = new kdu_long[num_stats_resolutions];
      memset(stats_reslayer_bytes,0,sizeof(kdu_long)*num_stats_entries);
      memset(stats_reslayer_packets,0,sizeof(kdu_long)*num_stats_entries);
      memset(stats_resmax_packets,0,sizeof(kdu_long)*num_stats_resolutions);
    }

  // Now we are ready for sample data processing.
  kdu_dims tile_indices; codestream.get_valid_tiles(tile_indices);
  kdu_long sample_processing_bytes=0;
  bool skip_ycc = (component_access_mode == KDU_WANT_CODESTREAM_COMPONENTS);
  if (num_threads == 0)
    {
      int dwt_stripe_height = 1;
      if (double_buffering_height > 0)
        dwt_stripe_height = double_buffering_height;
      sample_processing_bytes =
        expand_single_threaded(codestream,tile_indices,outputs,
                               num_output_channels,
                               last_channel_is_alpha,alpha_is_premultiplied,
                               num_used_components,used_component_indices,
                               channels,palette,allow_shorts,skip_ycc,
                               dwt_stripe_height,
                               num_stats_layers,num_stats_resolutions,
                               stats_reslayer_bytes,stats_reslayer_packets,
                               stats_resmax_packets,progress_interval);
    }
  else
    {
      if (cpu_iterations > 0)
        { kdu_warning w; w << "CPU time statistics are likely to be "
          "incorrect unless you explicitly specify \"-num_threads 0\"."; }
      bool dwt_double_buffering = false;
      int dwt_stripe_height = 1;
      if (double_buffering_height > 0)
        {
          dwt_stripe_height = double_buffering_height;
          dwt_double_buffering = (num_threads > 1);
        }
    sample_processing_bytes =
      expand_multi_threaded(codestream,tile_indices,outputs,
                            num_output_channels,
                            last_channel_is_alpha,alpha_is_premultiplied,
                            num_used_components,used_component_indices,
                            channels,palette,allow_shorts,skip_ycc,
                            num_threads,dwt_double_buffering,
                            dwt_stripe_height,
                            num_stats_layers,num_stats_resolutions,
                            stats_reslayer_bytes,stats_reslayer_packets,
                            stats_resmax_packets,progress_interval);
    }
  // Cleanup
  if (cpu_iterations >= 0)
    {
      kdu_long num_samples;
      double seconds = codestream.get_timing_stats(&num_samples);
      pretty_cout << "End-to-end CPU time ";
      if (cpu_iterations > 0)
        pretty_cout << "(estimated) ";
      pretty_cout << "= " << seconds << " seconds ("
                  << 1.0E6*seconds/num_samples << " us/sample)\n";
    }
  if (cpu_iterations > 0)
    {
      kdu_long num_samples;
      double seconds = codestream.get_timing_stats(&num_samples,true);
      if (seconds > 0.0)
        {
          pretty_cout << "Block decoding CPU time (estimated) ";
          pretty_cout << "= " << seconds << " seconds ("
                      << 1.0E6*seconds/num_samples << " us/sample)\n";
        }
    }
  if (mem)
    {
      pretty_cout << "\nSample processing/buffering memory = "
                  << sample_processing_bytes << " bytes.\n";
      pretty_cout << "Compressed data memory = "
                  << codestream.get_compressed_data_memory()
                  << " bytes.\n";
      pretty_cout << "State memory associated with compressed data = "
                  << codestream.get_compressed_state_memory()
                  << " bytes.\n";
    }
  if (!quiet)
    {
      pretty_cout << "\nConsumed " << codestream.get_num_tparts()
                  << " tile-part(s) from a total of "
                  << tile_indices.area() << " tile(s).\n";
      pretty_cout << "Code-stream bytes (excluding any file format) = "
                  << codestream.get_total_bytes() << " = "
                  << 8.0*codestream.get_total_bytes() /
                     get_bpp_dims(codestream.access_siz())
                  << " bits/pel.\n";
      if (jpx_in.exists() && !jpx_in.access_compatibility().is_jp2())
        {
          if (jpx_layer.exists())
            pretty_cout << "Decompressed channels are based on JPX "
                           "compositing layer "
                        << jpx_layer.get_layer_id() << ".\n";
          if (jpx_stream.exists())
            pretty_cout << "Decompressed codestream "
                        << jpx_stream.get_codestream_id()
                        << " of JPX file.\n";
        }
      if (num_threads == 0)
        pretty_cout << "Processed using the single-threaded environment "
        "(see `-num_threads')\n";
      else
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          "    " << num_threads << " parallel threads of execution "
          "(see `-num_threads')\n";
    }
  delete[] used_component_indices;
  if (stats)
    {
      print_parsed_packet_stats(num_stats_layers,num_stats_resolutions,
                                stats_reslayer_bytes,stats_reslayer_packets,
                                stats_resmax_packets);
      delete[] stats_reslayer_bytes;
      delete[] stats_reslayer_packets;
      delete[] stats_resmax_packets;
    }
  codestream.destroy();
  input->close();
  jpx_in.close();
  if (record_stream != NULL)
    {
      delete formatted_recorder;
      delete record_stream;
    }
  delete outputs;

  return 0;
}
