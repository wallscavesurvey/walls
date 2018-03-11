/*****************************************************************************/
// File: kdu_compress.cpp [scope = APPS/COMPRESSOR]
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
   File-based JPEG2000 compressor application, demonstrating many of the
compression-oriented capabilities of the Kakadu framework.
******************************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdio.h> // so we can use `sscanf' for arg parsing.
#include <math.h>
#include <assert.h>
#include <iostream>
// Kakadu core includes
#include "kdu_arch.h"
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_roi_processing.h"
#include "kdu_sample_processing.h"
// Application includes
#include "kdu_args.h"
#include "kdu_image.h"
#include "kdu_file_io.h"
#include "jp2.h"
#include "jpx.h"
#include "compress_local.h"
#include "roi_sources.h"

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
  out << "This is Kakadu's \"kdu_compress\" application.\n";
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
  out << "-i <file 1>[*<copies>@<size>],...  {see also `-fprec' & `-icrop'}\n";
  if (comprehensive)
    out << "\tOne or more input files. If multiple files are provided, "
           "they must be separated by commas. Any spaces will be treated as "
           "part of the file name.  If any filename contains the optional "
           "\"*<copies>@<size>\" suffix, that file actually contributes "
           "<copies> inputs, where the k'th copy starts (k-1)*<size> "
           "bytes into the file; this is most useful for raw files, "
           "allowing a single raw file to contribute multiple image "
           "components.\n"
           "\t   Currently accepted image file formats are: TIFF (including "
           "BigTIFF), RAW (big-endian), RAWL (little-endian), BMP, PBM, PGM "
           "and PPM, as determined by the file suffix.  For raw files, "
           "the sample bits must be in the least significant bit positions "
           "of an 8, 16, 24 or 32 bit word, depending on the bit-depth.  "
           "Unused MSB's in each word are entirely disregarded.  The word "
           "organization is big-endian for files with the suffix \".raw\", "
           "but little-endian for files with the suffix \".rawl\" -- be "
           "careful to get this right.  Also, with raw files, the dimensions, "
           "precision and signed/unsigned characteristics must be provided "
           "separately using `Sdims' (or any other appropriate combination "
           "of SIZ-type parameters), `Sprecision' and `Ssigned'.\n"
           "\t   For non-raw files, the `S...' parameters are obtained from "
           "the file header automatically.  If there is a Part-2 "
           "multi-component transform, however, the `Mcomponents', "
           "`Mprecision' and `Msigned' attributes are set based on the "
           "file header, and you must generally supply suitable values "
           "for `Scomponents', `Sprecision' and `Ssigned' along with the "
           "multi-component transform specification.  See the discussion "
           "and examples which appear at the end of this usage statement "
           "for more on the interaction between file header precision and "
           "dimensional information with the configuration of coding "
           "parameter attributes.\n"
           "\t   There can be cases where you wish to compress the "
           "sample values found in a file, using different precision "
           "properties to those which are specified in the file header.  "
           "This can be arranged via the `-fprec' argument.\n"
           "\t   There are also cases where you wish to compress only "
           "a cropped portion of an input file; this is particularly "
           "useful with the `-frag' option.  You can arrange for "
           "such cropping to take place via the `-icrop' argument.\n";
  out << "-o <compressed file -- raw code-stream, JP2 or JPX file>\n";
  if (comprehensive)
    out << "\tName of file to receive the compressed code-stream.  If the "
           "file name has a \".jp2\" suffix (not case sensitive), the "
           "code-stream will be wrapped up inside the JP2 file format.  "
           "If the file name has a \".jpx\" or \".jpf\" suffix (not case "
           "sensitive), the code-stream will be wrapped up inside the more "
           "sophisticated JPX file format, with a single compositing layer.  "
           "In either case, the first 3 source image components will be "
           "treated as sRGB colour channels (red, green then blue) and the "
           "remainder will be identified as auxiliary undefined components "
           "in the JP2/JPX file, unless there are less than 3 colours, or "
           "a separate colour space is identified via the `-jp2_space' and/or "
           "`-jpx_space' arguments.\n"
           "\t   If an input file defines a colour palette (quite common "
           "with BMP files), this will be preserved through the JP2/JPX file "
           "format, and samples will be correctly de-palettized by "
           "conformant JP2/JPX readers.  If there are fewer than 3 components "
           "available (taking any palette into account), the first component "
           "will be identified as a luminance component with the sRGB "
           "gamma and any remaining component will be identified as an "
           "auxiliary undefined channel.  Again, these default decisions "
           "may be overridden by the `-jp2_space' and/or `-jpx_space' "
           "arguments.\n";
  out << "-fprec <comp-0 precision>[L|M][,<comp-1 precision>[L|M][...]]\n";
  if (comprehensive)
    out << "\tYou can use this argument to adjust the way in which sample "
           "data precision is interpreted for the image components "
           "found in the input files.  For files which "
           "have header information, the data is compressed as though the "
           "precision (bit-depth) were equal to the value(s) supplied here.  "
           "This is most useful when the actual data occupies only a subset "
           "of the bits used to record information in the input image "
           "file(s); in this case, you would typically supply a "
           "smaller precision value for this argument, in order to provide "
           "a more suitable interpretation for the data being compressed -- "
           "and hence subsequent decompression and rendering.  For raw "
           "files, the precision of the original file data is still obtained "
           "from the `Sprecision' attribute (or `Mprecision' for Part 2 multi-"
           "component transforms), but the values of this attribute are "
           "changed to reflect the precision being forced.  In either case, "
           "the generated codestream contains `Sprecision' (or `Mprecision') "
           "attributes which reflect the forced precision values rather "
           "than those of the original sample data.\n"
           "\t   The argument takes a comma-separated list of non-negative "
           "precision values.  The list can contain more values than the "
           "number of image components in the collection of input files.  "
           "If it contains fewer values, the last value is replicated as "
           "required to provide forced precision values for all image "
           "components.  You can supply the special value of 0 for any "
           "component (including the last one in the comma-separated list, "
           "which gets replicated).  This special value means that the "
           "original precision should be left untouched.\n"
           "\t   Each value in the comma-separated list may optionally be "
           "followed by the single-character suffix of `M' or `L'.  The "
           "`M' suffix means that the precision forcing algorithm should "
           "scale the data by a power of 2, so that the most significant "
           "bit in the original sample values aligns with the most "
           "significant bit of the representation produced by precision "
           "forcing.  You would typically use this mode if you know that "
           "the least significant bits in a source file are guaranteed "
           "to be 0 and so there is no need to compress them -- particularly "
           "relevant for reversible (e.g., lossless) compression.  The `L' "
           "suffix means that the precision forcing algorithm aligns least "
           "significant bits -- this is also the default policy, in the event "
           "that no suffix is provided.  In this case, there is no scaling, "
           "but values which exceed the range which can be represented "
           "using the forced precision must be clipped.  You would typically "
           "use this mode if you know that some number of most significant "
           "bits in the original source samples are likely to be 0.\n"
           "\t   Note that it is up to the individual file format reading "
           "modules to respect (or not) the information supplied via the "
           "`-fprec' argument.  If you have add your own file "
           "format support to this application, prior to Kakadu version 6.1, "
           "this feature will probably not be supported.\n";
  out << "-icrop {<off_y>,<off_x>,<height>,<width>},...\n";
  if (comprehensive)
    out << "\tThis argument provides a means to effectively crop the input "
           "files supplied by `-i' -- essentially, the input file reading "
           "code limits its access to just the cropped region and adjusts "
           "the dimensions reported for the input file(s) accordingly.  This "
           "option is especially useful when combined with the fragmented "
           "compression option offered by `-frag'.  For example, you could "
           "invoke \"kdu_compress\" 40 times to compress a 200GB input "
           "file in 5GB fragments, each corresponding to large tile (for "
           "example), simply by supplying the relevant tile indices to "
           "`-frag' and the corresponding tile regions to `-icrop'.  "
           "The argument takes one or more sets of 4 cropping parameters, "
           "each of which is enclosed in curly braces (remember to escape "
           "the braces in unix shells).  The first set of cropping parameters "
           "applies to the first file supplied via `-i'.  The second set of "
           "cropping parameters applies to the second input file, and so "
           "forth, with the final set of cropping parameters applying to any "
           "remaining input files.  You should note that individual readers "
           "for each file format may or may not support cropping -- if not "
           "supported you will receive an appropriate error message.  "
           "Certainly, cropping is supported for at least some TIFF files.\n";
  out << "-frag <tidx_y>,<tidx_x>,<thigh>,<twide>\n";
  if (comprehensive)
    out << "\tUse this argument to compress only a fragment of the "
           "final codestream.  The four parameters identify the tiles "
           "which belong to the current fragment.  Specifically, <tidx_y> "
           "is the vertical tile index of the first tile in the new fragment, "
           "measured relative to the first tile in the image.  That is, "
           "<tidx_y> is the number of tile rows above the current fragment.  "
           "Similarly, <tidx_x> is the number of tile columns to the left "
           "of the fragment, while <thigh> and <twide> are the number of "
           "tile rows and number of tile columns in the fragment.  The "
           "first fragment must have <tidx_x>=0 and <tidx_y>=0.  The "
           "main codestream header, plus any JP2/JPX header information "
           "will be written only in the first fragment.  The program "
           "leaves behind some additional information in the output file, "
           "after a temporary EOC marker, which can be recovered when the "
           "next fragment is compressed.  This information identifies the "
           "total number of tiles which have been compressed in all "
           "previous fragments, and the total number of bytes associated "
           "with these previous fragments.  Together, this information "
           "is used to determine whether or not this is the last fragment, "
           "and where any TLM information must be written.  If this argument "
           "is present, the dimensions of the entire image must be explicitly "
           "identified via the `Sdims' attribute and/or the "
           "`Ssize' and `Sorigin' attributes.  Also, you must supply the "
           "tiling attributes via `Stiles' and (optionally) `Stile_origin'.  "
           "The input image(s) supplied via `-i' describe only the "
           "fragment being compressed, not the entire image.  It is "
           "worth noting that TLM (tile-part-length) marker segments can "
           "be requested by defining `ORGgen_tlm', and that this "
           "funcionality works even when the image is being compressed "
           "in fragments.  Note finally, that the `-icrop' argument "
           "allows you to effectively crop input files to just the "
           "fragment you are interested in compressing -- saves you "
           "having to split large input files up into fragments first.\n";
  out << "-roi {<top>,<left>},{<height>,<width>} | <PGM image>,<threshold>\n";
  if (comprehensive)
    out << "\tEstablish a region of interest (foreground) to be coded more "
           "precisely and/or earlier in the progression than the rest of "
           "the image (background).  This argument has no effect unless "
           "the \"Rshift\" attribute is also set.  The \"Rlevels\" attribute "
           "may also be used to control the number of DWT levels which "
           "will be affected by the ROI information.\n"
           "\t   The single parameter supplied with this "
           "argument may take one of two forms.  The first form provides "
           "a simple rectangular region of interest, specified in terms of "
           "its upper left hand corner coordinates (comma-separated and "
           "enclosed in curly braces) and its dimensions (also comma-"
           "separated and enclosed in braces).  All coordinates and "
           "dimensions are expressed relative to the origin and dimensions "
           "of the high resolution grid (or canvas), using real numbers in "
           "the range 0 to 1. If the original image is to be rotated during "
           "compression (see `-rotate'), the coordinates supplied here are "
           "to be interpreted with respect to the orientation of the image "
           "being compressed.\n"
           "\t   The second form for the single parameter string supplied "
           "with the `-roi' argument involves a named (PGM) image file, "
           "separated by a comma from an ensuing real-valued threshold in "
           "the range 0 to 1.  In this case, the image is scaled "
           "(interpolated) to fill the region occupied by each image "
           "component.  Pixel values whose relative amplitude exceeds the "
           "threshold identify the foreground region.\n";
  out << "-rate -|<bits/pel>,<bits/pel>,...\n";
  if (comprehensive)
    out << "\tOne or more bit-rates, expressed in terms of the ratio between "
           "the total number of compressed bits (including headers) and the "
           "product of the largest horizontal and  vertical image component "
           "dimensions.  A dash, \"-\", may be used in place of the first "
           "bit-rate in the list to indicate that the final quality layer "
           "should include all compressed bits.  Specifying a very large "
           "rate target is fundamentally different to using the dash, \"-\", "
           "because the former approach may cause the incremental rate "
           "allocator to discard terminal coding passes which do not lie "
           "on the rate-distortion convex hull.  This means that reversible "
           "compression might not yield a truly lossless representation if "
           "you specify `-rate' without a dash for the first rate target, no "
           "matter how large the largest rate target is.\n"
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
           "range.\n";
  out << "-slope <layer slope>,<layer slope>,...\n";
  if (comprehensive)
    out << "\tIf present, this argument provides rate control information "
           "directly in terms of distortion-length slope values, overriding "
           "any information which may or may not have been supplied via a "
           "`-rate' argument.  If the number of quality layers is  not "
           "specified via a `Clayers' argument, it will be deduced from the "
           "number of slope values.  Slopes are inversely related to "
           "bit-rate, so the slopes should decrease from layer to layer.  The "
           "program automatically sorts slopes into decreasing order so you "
           "need not worry about getting the order right.  For reference "
           "we note that a slope value of 0 means that all compressed bits "
           "will be included by the end of the relevant layer, while a "
           "slope value of 65535 means that no compressed bits will be "
           "included in the  layer.  The list of layer slope values must "
           "include at least one non-zero value.  If fewer slopes are "
           "provided than the number of quality layers, there is an "
           "internal algorithm which either extrapolates or interpolates "
           "the values you have provided using a reasonable heuristic.  "
           "Basically, the heuristic starts with the assumption that 256 "
           "is an excellent amount to separate successive layer slopes, "
           "since it represents roughly a sqrt(2) change in the bit-rate "
           "ignoring header overhead for most cases.  The heuristic will "
           "not insert smaller slopes than the smallest one you supply, "
           "since that represents the maximum desired quality.  If you "
           "supply two slopes, which are reasonably close together, the "
           "heuristic will reproduce the spacing you supply with these, "
           "but if interpolating the largest two supplied slopes leaves "
           "a gap closer to 256, that approach will be adopted.\n";
  out << "-full -- forces encoding and storing of all bit-planes.\n";
  if (comprehensive)
    out << "\tBy default, the system incrementally constructs conservative "
           "estimates of the final rate allocation parameters and uses these "
           "to skip coding passes which are very likely to be discarded "
           "during rate allocation.  It also discards any compressed "
           "code-bytes which we know we will not be needing on a regular "
           "basis, so as to conserve memory.  For large images, the memory "
           "consumption can approach the final compressed file size.  You "
           "might like to use the `-full' argument if you are compressing "
           "an image with highly non-uniform statistics.\n";
  out << "-precise -- forces the use of 32-bit representations.\n";
  if (comprehensive)
    out << "\tBy default, 16-bit data representations will be employed for "
           "sample data processing operations (colour transform and DWT) "
           "whenever the image component bit-depth is sufficiently small.\n";
  out << "-tolerance <percent tolerance on layer sizes given using `-rate'>\n";
  if (comprehensive)
    out << "\tThis argument modifies the behaviour of the `-rate' argument "
           "slightly, providing a tolerance specification on the achievement "
           "of the cumulative layer bit-rates given by that argument.  It "
           "has no effect if layer construction is controlled using the "
           "`-slope' argument.  The rate allocation algorithm "
           "will attempt to find a distortion-length slope such that the "
           "bit-rate, R_L, associated with layer L is in the range "
           "T_L*(1-tolerance/100) <= R_L <= T_L, where T_L is the target "
           "bit-rate, which is the difference between the cumulative bit-rate "
           "at layer L and the cumulative bit-rate at layer L-1, as specified "
           "in the `-rate' list.  Note that the tolerance is given as a "
           "percentage, that it affects only the lower bound, not the upper "
           "bound on the bit-rate, and that the default tolerance is 0.  If "
           "it is not possible to satisfy both bounds on the layer bit-rate "
           "exactly, the layer will be assigned the largest size which is "
           "consistent with the upper bound, if possible.\n";
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
           "\t   Incremental flushing may be used with either `-slope' "
           "controlled quality layers, or `-rate' driven quality layers; "
           "however, with rate-driven quality layers you should be "
           "particularly careful to keep the flushing period large enough to "
           "give the rate control algorithm a decent amount of compressed "
           "data to perform effective rate control.  Generally a period of "
           "at least 1000 or 2000 image lines should be used for rate "
           "driven flushing.\n"
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
  out << "-no_info -- prevents the inclusion of layer info in COM segments.\n";
  if (comprehensive)
    out << "\tA code-stream COM (comment) marker segment is "
           "included in the main header to record the distortion-length "
           "slope and the size of each quality layer which is generated.  "
           "If you wish to make the file as small as possible and are "
           "working with small images, you may wish to disable this feature "
           "by specifying the `-no_info' flag.\n";
  out << "-com <comment string>\n";
  if (comprehensive)
    out << "\tYou can use this argument any number of times to include your "
           "own comments directly in the codestream, as COM marker segments.  "
           "Of course, the supplied comment string must appear as a single "
           "command-line argument, so you will need to quote any strings "
           "which contain spaces when you supply this argument on the "
           "command line.\n";
  out << "-no_weights -- target MSE minimization for colour images.\n";
  if (comprehensive)
    out << "\tBy default, visual weights will be automatically used for "
           "colour imagery (anything with 3 compatible components).  Turn "
           "this off if you want direct minimization of the MSE over all "
           "reconstructed colour components.\n";
  out << "-no_palette\n";
  if (comprehensive)
    out << "\tThis argument is meaningful only when reading palettized "
           "imagery and compressing to a JP2/JPX file.  By default, the "
           "palette will be preserved in the JP2/JPX file and only the "
           "palette indices will be compressed.  In many cases, it may "
           "be more efficient to compress the RGB data as a 24-bit "
           "continuous tone image. To make sure that this happens, select "
           "the `-no_palette' option.\n";
  out << "-jp2_space <sLUM|sRGB|sYCC|iccLUM|iccRGB>[,<parameters>]\n";
  if (comprehensive)
    out << "\tYou may use this to explicitly specify a JP2 compatible "
           "colour space description to be included in a JP2/JPX file.  "
           "If the colour space is `sLUM' or `iccLUM', only one colour "
           "channel will be defined, even if the codestream contains 3 or "
           "more components.  The argument is illegal except when the output "
           "file has the \".jp2\", \".jpx\" or \".jpf\" suffix, as explained "
           "above.  Note that for JPX files (those having a \".jpx\" or "
           "\".jpf\" suffix), the `-jpx_space' argument may be supplied "
           "as an alternative or in addition to this argument to provide "
           "richer colour descriptions or even multiple colour descriptions.  "
           "The pesent argument must be followed by a single string "
           "consisting of one of 6 colour space names, possibly followed "
           "by a comma-separated list of parameters.\n"
           "\t   If the space is \"iccLUM\", two parameters must "
           "follow, `gamma' and `beta', which identify the tone reproduction "
           "curve.  As examples, the sRGB space has gamma=2.4 and beta=0.055, "
           "while NTSC RGB has gammma=2.2 and beta=0.099.  A pure power law "
           "has beta=0, but is not recommended due to the ill-conditioned "
           "nature of the resulting function around 0.\n"
           "\t   If the space is \"iccRGB\", 9 parameters must follow in "
           "the comma separated list.  The first two of these are the gamma "
           "and beta values, as above.  The next 2 parameters hold the "
           "X and Y chromaticity coordinates of the first (typically red) "
           "primary colour.  Similarly, the next 4 parameters hold the X,Y "
           "coordinates of the second (typically green) and third (typically "
           "blue) primary colour.  The final parameter must be one of the "
           "two strings \"D50\" or \"D65\", identifying the illuminant.  "
           "The present function assumes that equal amounts of all 3 "
           "primary colours produce the neutral (white) associated with this "
           "illuminant.\n";
  out << "-jpx_space <enumerated colour space>,[<prec>,<approx>]\n";
  if (comprehensive)
    out << "\tThis argument may be used only when writing JPX files (those "
           "with a \".jpx\" or \".jpf\" suffix).  Although JPX files may "
           "contain arbitrary ICC profiles, we do not provide the capability "
           "to include these from the command line.  Instead, we list here "
           "only the enumerated colour space options defined by JPX.  If "
           "`-jp2_space' is also supplied, multiple colour descriptions "
           "will be written, with the JP2 compatible description appearing "
           "first.  If the `prec' and `approx' parameters are omitted from "
           "the parameter list, they default to 0.  Otherwise, the "
           "supplied precedence must lie in the range -128 to +127 and the "
           "supplied approximation level must lie in the range 0 to 4.  The "
           "following enumerated colour space names are recognized:\n\t\t"
           "`bilevel1', `bilevel2', `YCbCr1', `YCbCr2', `YCbCr3', "
           "`PhotoYCC', `CMY', `CMYK', `YCCK', `CIELab', `CIEJab', "
           "`sLUM', `sRGB', `sYCC', `esRGB', `esYCC', `ROMMRGB', "
           "`YPbPr60',  `YPbPr50'.\n";
  out << "-jp2_aspect <aspect ratio of high-res canvas grid>\n";
  if (comprehensive)
    out << "\tIdentifies the aspect ratio to be used by a conformant JP2/JPX "
           "reader when rendering the decompressed image to a display, "
           "printer or other output device.  The aspect ratio identifies "
           "ratio formed by dividing the vertical grid spacing by the "
           "horizontal grid spacing, where the relevant grid is that of the "
           "high resolution canvas.  Sub-sampling factors determine the "
           "number of high resolution canvas grid points occupied by any "
           "given image component sample in each direction.  By "
           "default conformant JP2/JPX readers are expected to assume a 1:1 "
           "aspect ratio on the high resolution canvas, so that the use of "
           "non-identical sub-sampling factors for an image component "
           "implies a required aspect ratio conversion after decompression.\n";
  out << "-jp2_alpha -- treat 2'nd or 4'th image component as alpha\n";
  if (comprehensive)
    out << "\tUse this argument if you want one of the image components to "
           "be treated as an alpha channel for the pixels whose colour is "
           "represented by the preceding components.  If the colour space "
           "is grey-scale (see `-jp2_space'), component 0 represents the "
           "intensity and component 1 represents alpha.  More generally, if "
           "the colour space involves C colour channels, the first C "
           "components represent these colour channels and the next one "
           "represents alpha.\n";
  out << "-jpx_layers [*|<num layers>]\n";
  if (comprehensive)
    out << "\tThis argument provides a simple mechanism for generating "
           "JPX files which contain multiple compositing layers, each "
           "drawing their information from a single codestream.  A common "
           "application for this argument would be to assign each image "
           "component (each slice) in a compressed medical volume to a "
           "separate compositing layer.  This allows efficient interactive "
           "delivery of the compressed volume over JPIP, even where a "
           "multi-component transform has been used to exploit redundancy "
           "between components.  To create richer JPX files, involving "
           "any number of codestreams and the possibility of mixing "
           "components from different codestreams in a single compositing "
           "layer, use the \"kdu_merge\" utility to combine sources "
           "and redefine the layering, colour space and other metadata.  The "
           "present argument takes a single parameter, which either "
           "specifies the number of layers L >= 1 to be generated, or "
           "specifies the wildcard `*', which means that as many layers "
           "should be generated as possible.  The number of image components, "
           "C, used by each compositing layer is determined by the colour "
           "space supplied to `-jp2_space' or `-jpx_space', possibly "
           "supplemented by an alpha component if `-jp2_alpha' is specified.  "
           "In the absence of a supplied colour space, the colour space is "
           "set to sLUM (if the number of components is less than 3) or sRGB, "
           "for which C=1 and C=3, respectively.  The created JPX compositing "
           "layers consume components in order, C at a time, so that there "
           "must be at least C*L image components available -- these are "
           "the output image components produced at the output of any "
           "multi-component transform during decompression (given by the "
           "`Mcomponents' attribute).  If the wildcard is given, the "
           "value of L is set as large as possible so that C*L does not "
           "exceed the number of available components.\n";
  out << "-jp2_box <file 1>[,<file 2>[,...]]\n";
  if (comprehensive)
    out << "\tThis argument provides a crude method for allowing extra "
           "boxes to be inserted into a JP2 or JPX file.  The extra boxes are "
           "written after the main file header boxes, but before the "
           "contiguous code-stream box.  The argument takes a comma-separated "
           "list of file names, without any intervening space.  Each file "
           "represents a single top-level box, whose box-type is found in "
           "the first 4 characters of the file, and whose contents start "
           "immediately after the first new-line character and continue "
           "until the end of the file.  The first line of the file (the "
           "one containing the box-type characters and preceding the box "
           "contents) should not be more than 128 characters long.  "
           "Each file may contain arbitrary binary or ASCII data, but is "
           "always opened as binary.\n";
  out << "-rotate <degrees>\n";
  if (comprehensive)
    out << "\tRotate source image prior to compression. "
           "Must be multiple of 90 degrees.\n";
  siz_params siz; siz.describe_attributes(out,comprehensive);
  cod_params cod; cod.describe_attributes(out,comprehensive);
  qcd_params qcd; qcd.describe_attributes(out,comprehensive);
  rgn_params rgn; rgn.describe_attributes(out,comprehensive);
  poc_params poc; poc.describe_attributes(out,comprehensive);
  crg_params crg; crg.describe_attributes(out,comprehensive);
  org_params org; org.describe_attributes(out,comprehensive);
  mct_params mct; mct.describe_attributes(out,comprehensive);
  mcc_params mcc; mcc.describe_attributes(out,comprehensive);
  mco_params mco; mco.describe_attributes(out,comprehensive);
  atk_params atk; atk.describe_attributes(out,comprehensive);
  dfs_params dfs; dfs.describe_attributes(out,comprehensive);
  ads_params ads; ads.describe_attributes(out,comprehensive);
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
           "block encoding (which is always spread across multiple threads, "
           "if available).  However, even for a small number of threads, the "
           "amount of thread idle time can be reduced by specifying the "
           "`-double_buffering' option.  In this case, only a small number "
           "of image rows in each image component are actually double "
           "buffered, so that one set can be processed by colour "
           "transformation and sample reading operations, while the other "
           "set is processed by the DWT analysis engines, which themselves "
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
           "cases, DWT input lines are only single buffered, but "
           "increasing the size of this buffer (stripe height) allows the "
           "thread to do more work within a single tile-component, before "
           "moving to another tile-component.\n";
  out << "-progress <interval>\n";
  if (comprehensive)
    out << "\tThis option is useful when processing massive input images; it "
           "allows you to receive feedback each time a vertical row of tiles "
           "has been processed, but potentially more frequently, depending "
           "upon the <interval> parameter.  The application also provides "
           "feedback each time codestream flushing is initiated (paricularly "
           "useful in conjunction with `-flush_period').  The <interval> "
           "parameter indicates the maximum number of lines that can be "
           "pushed into the compression machinery before some progress is "
           "provided -- if this value is smaller than the tile height, you "
           "will receive periodic information about the percentage of the "
           "vertical row of tiles which has been processed.\n";
  out << "-cpu <coder-iterations>\n";
  if (comprehensive)
    out << "\tTimes end-to-end execution and, optionally, the block encoding "
           "operation, reporting throughput statistics.  If "
           "`coder-iterations' is 0, the block coder will not be timed, "
           "leading to the most accurate end-to-end system execution "
           "times.  Otherwise, `coder-iterations' must be a positive "
           "integer -- larger values will result in more accurate "
           "estimates of the block encoder processing time, but "
           "degrade the accuracy of end-to-end system execution time "
           "estimates.  Note that end-to-end times include the impact "
           "of image file reading, which can be considerable.  Note also "
           "that timing information may not be at all reliable unless "
           "`-num_threads' is 1.  Since the default value for the "
           "`-num_threads' argument may be greater than 1, you should "
           "explicitly set the number of threads to 1 before collecting "
           "timing information.\n";
  out << "-mem -- Report memory usage.\n";
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
           "which is accepted when specifying the parameters on the command "
           "line.\n";
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
         "factors were used, you may like to use the `-record' option.\n\n";

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
    "source components (image planes) found in the set of supplied input "
    "files.  The value of `Mcomponents' may also be larger than the number "
    "of source components found in the supplied input files.  In this case, "
    "the source files provide the initial set of image components which will "
    "be recovered during decompression.  This subset must be larger enough to "
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
    "number of output and codestream components; it will be set to the "
    "number of source components found in the set of supplied input files.  "
    "`Sprecision' and `Ssigned' hold the bit-depth and signed/unsigned "
    "attributes of the image components.  For raw files, these must be "
    "supplied explicitly on the command line; otherwise, they are derived "
    "from the input file headers.\n";
  out << "   It is worth noting that the dimensions of the N=`Scomponents' "
    "codestream image components are assumed to be identical to those of the "
    "N source image components contained in the set of supplied input files.  "
    "This assumption is imposed for simplicity in this demonstration "
    "application; it is not required by the Kakadu core system.\n\n";

  out.flush();
  exit(0);
}

/*****************************************************************************/
/* STATIC                     parse_simple_args                              */
/*****************************************************************************/

static kdc_file_binding *
  parse_simple_args(kdu_args &args, char * &ofname,
                    std::ostream * &record_stream,
                    bool &transpose, bool &vflip, bool &hflip,
                    int &flush_period, double &rate_tolerance,
                    bool &allow_rate_prediction, bool &allow_shorts,
                    bool &no_info, bool &no_weights, bool &no_palette,
                    int &num_jpx_layers, int &num_threads,
                    int &double_buffering_height, int &progress_interval,
                    int &cpu_iterations, bool &mem, bool &quiet)
  /* Parses most simple arguments (those involving a dash). Most parameters are
     returned via the reference arguments, with the exception of the input
     file names, which are returned via a linked list of `kdc_file_binding'
     objects.  Only the `fname' field of each `kdc_file_binding' record is
     filled out here.  The value returned via `cpu_iterations' is negative
     unless CPU times are required.
        Note that `num_threads' is set to 0 if no multi-threaded processing
     group is to be created, as distinct from a value of 1, which means
     that a multi-threaded processing group is to be used, but this group
     will involve only one thread. */
{
  int rotate;
  kdc_file_binding *files, *last_file, *new_file;

  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();      

  files = last_file = NULL;
  ofname = NULL;
  record_stream = NULL;
  rotate = 0;
  flush_period = INT_MAX;
  rate_tolerance = 0.0;
  allow_rate_prediction = true;
  allow_shorts = true;
  no_info = false;
  no_weights = false;
  no_palette = false;
  num_jpx_layers = 1;
  num_threads = 0; // This is not actually the default -- see below.
  double_buffering_height = 0; // i.e., no double buffering
  progress_interval = 0; // Don't provide any progress indication at all
  cpu_iterations = -1;
  mem = false;
  quiet = false;

  if (args.find("-i") != NULL)
    {
      char *string, *cp, *name_end;
      int len;

      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "\"-i\" argument requires a file name!"; }
      while ((len = (int) strlen(string)) > 0)
        {
          cp = strchr(string,',');
          if (cp == NULL)
            cp = string+len;
          for (name_end=string; name_end < cp; name_end++)
            if (*name_end == '*')
              break;
          int num_copies=1, copy_size=0;
          if (name_end < cp)
            {
              if ((sscanf(name_end,"*%d@%d",&num_copies,&copy_size) != 2) ||
                  (num_copies < 1) || (copy_size < 1))
                { kdu_error e; e << "Malformed copy replicator suffix found "
                  "within file name in the comma-separated list supplied "
                  "with the \"-i\" argument.  Copy replicator suffices "
                  "must have the form \"*<copies>@<copy size>\".";
                }
            }
          kdu_long copy_offset = 0;
          for (; num_copies > 0; num_copies--, copy_offset+=copy_size)
            {
              new_file = new kdc_file_binding(string,(int)(name_end-string),
                                              copy_offset);
              if (last_file == NULL)
                files = last_file = new_file;
              else
                last_file = last_file->next = new_file;
            }
          if (*cp == ',') cp++;
          string = cp;
        }
      args.advance();
    }
  
  if (args.find("-icrop") != NULL)
    {
      kdc_file_binding *file = NULL;
      const char *field_sep, *string = args.advance();
      for (field_sep=NULL; string != NULL; string=field_sep)
        {
          if (field_sep != NULL)
            {
              if (*string != ',')
                { kdu_error e; e << "\"-icrop\" argument requires a comma-"
                  "separated list of cropping specifications."; }
              string++; // Walk past the separator
            }
          if (*string == '\0')
            break;
          if (((field_sep = strchr(string,'}')) != NULL) &&
              (*(++field_sep) == '\0'))
            field_sep = NULL;
          if (file == NULL)
            file = files;
          else
            file = file->next;
          if ((sscanf(string,"{%d,%d,%d,%d}", &(file->cropping.pos.y),
                      &(file->cropping.pos.x),&(file->cropping.size.y),
                      &(file->cropping.size.x)) != 4) ||
              (file->cropping.pos.x < 0) || (file->cropping.pos.y < 0) ||
              (file->cropping.size.x <= 0) || (file->cropping.size.y <= 0))
            { kdu_error e; e << "\"-icrop\" argument contains malformed "
              "cropping specification.  Expected to find four comma-separated "
              "integers, enclosed by curly braces.  The first two (y and x "
              "offsets must be non-negative) and the last two (height and "
              "width) must be strictly positive."; }
        }
      if (file == NULL)
        { kdu_error e; e << "\"-icrop\" argument requires at least one "
          "cropping specification!"; }
      while (file->next != NULL)
        { file->next->cropping = file->cropping; file=file->next; }
      args.advance();
    }

  if (args.find("-o") != NULL)
    {
      char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "\"-o\" argument requires a file name!"; }
      ofname = new char[strlen(string)+1];
      strcpy(ofname,string);
      args.advance();
    }

  if (args.find("-full") != NULL)
    {
      args.advance();
      allow_rate_prediction = false;
    }

  if (args.find("-precise") != NULL)
    {
      args.advance();
      allow_shorts = false;
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

  if (args.find("-tolerance") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%lf",&rate_tolerance) != 1) ||
          (rate_tolerance < 0.0) || (rate_tolerance > 50.0))
        { kdu_error e; e << "\"-tolerance\" argument requires a real-valued "
          "parameter (percentage) in the range 0 to 50."; }
      rate_tolerance *= 0.01; // Convert from percentage to a fraction
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

  if (args.find("-no_info") != NULL)
    {
      no_info = true;
      args.advance();
    }

  if (args.find("-no_weights") != NULL)
    {
      no_weights = true;
      args.advance();
    }

  if (args.find("-no_palette") != NULL)
    {
      no_palette = true;
      args.advance();
    }

  if (args.find("-jpx_layers") != NULL)
    {
      const char *string = args.advance();
      if ((string[0] == '*') && (string[1] == '\0'))
        num_jpx_layers = 0; // Wildcard value
      else if ((sscanf(string,"%d",&num_jpx_layers) != 1) ||
               (num_jpx_layers < 1))
        { kdu_error e; e << "\"-jpx_layers\" argument requires a positive "
          "integer parameter, or else the wildcard character, `*'."; }
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
  
  if (args.find("-mem") != NULL)
    {
      mem = true;
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

  if (files == NULL)
    { kdu_error e; e << "Must provide one or more input files!"; }
  if (ofname == NULL)
    { kdu_error e; e << "Must provide an output file name!"; }
  while (rotate >= 4)
    rotate -= 4;
  while (rotate < 0)
    rotate += 4;
  switch (rotate) {
    case 0: transpose = false; vflip = false; hflip = false; break;
    case 1: transpose = true; vflip = true; hflip = false; break;
    case 2: transpose = false; vflip = true; hflip = true; break;
    case 3: transpose = true; vflip = false; hflip = true; break;
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
/* STATIC                     parse_fragment_args                            */
/*****************************************************************************/

static bool
  parse_fragment_args(kdu_args &args, kdu_dims &frag_indices)
{
  if (args.find("-frag") == NULL)
    return false;
  const char *string = args.advance();
  if (string == NULL)
    return false;
  
  if ((sscanf(string,"%d,%d,%d,%d",&frag_indices.pos.y,&frag_indices.pos.x,
              &frag_indices.size.y,&frag_indices.size.x) != 4) ||
      (frag_indices.pos.x < 0) || (frag_indices.pos.y < 0) ||
      (frag_indices.size.x <= 0) || (frag_indices.size.y <= 0))
    { kdu_error e; e << "Malformed `-frag' argument.  Insufficient or "
      "insufficient comma-separated parameters found in the SINGLE parameter "
      "string."; }

  args.advance();
  return true;
}

/*****************************************************************************/
/* STATIC                    find_fragment_region                            */
/*****************************************************************************/

static kdu_dims
  find_fragment_region(kdu_dims tile_indices, kdu_params *full_siz,
                       kdu_params *frag_siz)
  /* This function determines the location and dimensions of a new codestream
     fragment to be compressed, based on the tile indices recovered from
     the command line, and the size information in `full_siz'.  The
     function initializes `frag_siz' to contain all the same information
     as `full_siz', but with dimensions adjusted to reflect just the fragment
     in question.  The adjusted `frag_siz' information can be passed to the
     image file opening routines to ensure consistency.  The function
     returns the fragment region, which can be passed directly into
     `kdu_codestream::create'. */
{
  kdu_dims canvas, tile_partition;
  full_siz->finalize(); // Just to be sure
  if (!(full_siz->get(Ssize,0,0,canvas.size.y) &&
        full_siz->get(Ssize,0,1,canvas.size.x) &&
        full_siz->get(Sorigin,0,0,canvas.pos.y) &&
        full_siz->get(Sorigin,0,1,canvas.pos.x) &&
        full_siz->get(Stiles,0,0,tile_partition.size.y) &&
        full_siz->get(Stiles,0,1,tile_partition.size.x) &&
        full_siz->get(Stile_origin,0,0,tile_partition.pos.y) &&
        full_siz->get(Stile_origin,0,1,tile_partition.pos.x)))
    assert(0);
  canvas.size -= canvas.pos;

  // Compute fragment region
  kdu_dims region;
  region.pos.x =
    tile_partition.pos.x + tile_indices.pos.x*tile_partition.size.x;
  region.pos.y =
    tile_partition.pos.y + tile_indices.pos.y*tile_partition.size.y;
  region.size.x = tile_indices.size.x*tile_partition.size.x;
  region.size.y = tile_indices.size.y*tile_partition.size.y;
  region &= canvas;
  if (!region)
    { kdu_error e; e << "Illegal fragment supplied via `-frag'.  Indicated "
      "region of tile indices has no intersection with the codestream "
      "canvas."; }

  // Create fragment-restricted canvas inside `frag_siz'
  frag_siz->set(Ssize,0,0,region.size.y+region.pos.y);
  frag_siz->set(Ssize,0,1,region.size.x+region.pos.x);
  frag_siz->set(Sorigin,0,0,region.pos.y);
  frag_siz->set(Sorigin,0,1,region.pos.x);

  int c;
  kdu_coords subs;
  for (c=0; full_siz->get(Ssampling,c,0,subs.y,false,false,false) &&
            full_siz->get(Ssampling,c,1,subs.x,false,false,false); c++)
    {
      frag_siz->set(Ssampling,c,0,subs.y);
      frag_siz->set(Ssampling,c,1,subs.x);
    }

  bool b_val;
  int i_val;
  for (c=0; full_siz->get(Ssigned,c,0,b_val,false,false,false); c++)
    frag_siz->set(Ssigned,c,0,b_val);
  for (c=0; full_siz->get(Sprecision,c,0,i_val,false,false,false); c++)
    frag_siz->set(Sprecision,c,0,i_val);
  for (c=0; full_siz->get(Msigned,c,0,b_val,false,false,false); c++)
    frag_siz->set(Msigned,c,0,b_val);
  for (c=0; full_siz->get(Mprecision,c,0,i_val,false,false,false); c++)
    frag_siz->set(Mprecision,c,0,i_val);

  if (full_siz->get(Mcomponents,0,0,i_val))
    frag_siz->set(Mcomponents,0,0,i_val);
  if (full_siz->get(Scomponents,0,0,i_val))
    frag_siz->set(Scomponents,0,0,i_val);

  frag_siz->finalize();

  // Return fragment region
  return region;
}

/*****************************************************************************/
/* STATIC                   retrieve_fragment_state                          */
/*****************************************************************************/

static void
  retrieve_fragment_state(kdu_simple_file_target *tgt,
                          int &fragment_tiles_generated,
                          kdu_long &fragment_bytes_generated,
                          int &fragment_tlm_tparts)
  /* This function retrieves the fragment state from the end of the
     file created during compression of the last codestream fragment.  This
     is a convenience feature, allowing fragments to be simply created
     by separate invocations of the program. */
{
  int n;
  kdu_byte tail[15];
  if (!(tgt->strip_tail(tail,15) &&
        (tail[0] == 0xFF) && (tail[1] == (kdu_byte) KDU_EOC)))
    { kdu_error e; e << "Attempting to append a non-initial codestream "
      "fragment to an existing file which does not appear to contain "
      "previous fragments produced by `kdu_compress'.  Remember that the "
      "first fragment of a codestream must be the one which contains the "
      "upper left tile index."; }

  fragment_tiles_generated = 0;
  fragment_bytes_generated = 0;
  for (n=2; n < 6; n++)
    fragment_tiles_generated = (fragment_tiles_generated<<8) + (int) tail[n];
  for (n=6; n < 14; n++)
    fragment_bytes_generated = (fragment_bytes_generated<<8) + (int) tail[n];

  fragment_tlm_tparts = (int) tail[14];
}

/*****************************************************************************/
/* STATIC                     save_fragment_state                            */
/*****************************************************************************/

static void
  save_fragment_state(kdu_compressed_target *tgt,
                      int fragment_tiles_generated,
                      kdu_long fragment_bytes_generated,
                      int fragment_tlm_tparts)
  /* Saves the information required by the next fragment's call to
     `retrieve_fragment_state'. */
{
  int n, shift;
  kdu_byte tail[15];
  tail[0] = 0xFF;
  tail[1] = (kdu_byte) KDU_EOC; // Write temporary EOC marker
  for (shift=24, n=2; n < 6; n++, shift-=8)
    tail[n] = (kdu_byte)(fragment_tiles_generated>>shift);
  for (shift=56, n=6; n < 14; n++, shift-=8)
    tail[n] = (kdu_byte)(fragment_bytes_generated>>shift);
  tail[14] = (kdu_byte) fragment_tlm_tparts;
  tgt->write(tail,15);
}

/*****************************************************************************/
/* STATIC                   set_jp2_coding_defaults                          */
/*****************************************************************************/

static void
  set_jp2_coding_defaults(jp2_palette plt, jp2_colour colr, kdu_params *siz)
{
  int m_components = 0;
  siz->get(Mcomponents,0,0,m_components);
  kdu_params *cod = siz->access_cluster(COD_params);
  assert(cod != NULL);
  int num_colours = colr.get_num_colours();
  bool using_palette = (plt.get_num_luts() > 0);
  bool use_ycc, reversible;
  int dwt_levels;
  if (((num_colours < 3) || (colr.is_opponent_space()) || using_palette) &&
      (m_components == 0) && !cod->get(Cycc,0,0,use_ycc))
    cod->set(Cycc,0,0,use_ycc=false);
  if (using_palette && !cod->get(Creversible,0,0,reversible))
    cod->set(Creversible,0,0,reversible=true);
  if (using_palette && !cod->get(Clevels,0,0,dwt_levels))
    cod->set(Clevels,0,0,dwt_levels=0);
}

/*****************************************************************************/
/* STATIC                  set_default_colour_weights                        */
/*****************************************************************************/

static void
  set_default_colour_weights(kdu_params *siz, bool quiet)
{
  kdu_params *cod = siz->access_cluster(COD_params);
  assert(cod != NULL);

  float weight;
  if (cod->get(Clev_weights,0,0,weight) ||
      cod->get(Cband_weights,0,0,weight))
    return; // Weights already specified explicitly.
  bool can_use_ycc = true;
  bool rev0=false;
  int depth0=0, sub_x0=1, sub_y0=1;
  for (int c=0; c < 3; c++)
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
  if (!can_use_ycc)
    return;

  bool use_ycc;
  if (!cod->get(Cycc,0,0,use_ycc))
    cod->set(Cycc,0,0,use_ycc=true);
  if (!use_ycc)
    return;

  /* These example weights are adapted from numbers generated by Marcus Nadenau
     at EPFL, for a viewing distance of 15 cm and a display resolution of
     300 DPI. */
  
  cod->parse_string("Cband_weights:C0="
                    "{0.0901},{0.2758},{0.2758},"
                    "{0.7018},{0.8378},{0.8378},{1}");
  cod->parse_string("Cband_weights:C1="
                    "{0.0263},{0.0863},{0.0863},"
                    "{0.1362},{0.2564},{0.2564},"
                    "{0.3346},{0.4691},{0.4691},"
                    "{0.5444},{0.6523},{0.6523},"
                    "{0.7078},{0.7797},{0.7797},{1}");
  cod->parse_string("Cband_weights:C2="
                    "{0.0773},{0.1835},{0.1835},"
                    "{0.2598},{0.4130},{0.4130},"
                    "{0.5040},{0.6464},{0.6464},"
                    "{0.7220},{0.8254},{0.8254},"
                    "{0.8769},{0.9424},{0.9424},{1}");
  if (!quiet)
    pretty_cout << "Note:\n\tThe default rate control policy for colour "
                   "images employs visual (CSF) weighting factors.  To "
                   "minimize MSE instead, specify `-no_weights'.\n";
}

/*****************************************************************************/
/* STATIC                        get_bpp_dims                                */
/*****************************************************************************/

static kdu_long
  get_bpp_dims(kdu_codestream &codestream)
{
  int comps = codestream.get_num_components();
  int n, max_width=0, max_height=0;
  for (n=0; n < comps; n++)
    {
      kdu_dims dims;  codestream.get_dims(n,dims);
      if (dims.size.x > max_width)
        max_width = dims.size.x;
      if (dims.size.y > max_height)
        max_height = dims.size.y;
    }
  return ((kdu_long) max_height) * ((kdu_long) max_width);
}

/*****************************************************************************/
/* STATIC                      check_jp2_suffix                              */
/*****************************************************************************/

static bool
  check_jp2_suffix(char *fname)
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

/*****************************************************************************/
/* STATIC                      check_jpx_suffix                              */
/*****************************************************************************/

static bool
  check_jpx_suffix(char *fname)
  /* Returns true if the file-name has the suffix, ".jpx" or ".jpf", where the
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
  if ((*cp != 'x') && (*cp != 'X') && (*cp != 'f') && (*cp != 'F'))
    return false;
  return true;
}

/*****************************************************************************/
/* STATIC                      set_jp2_attributes                            */
/*****************************************************************************/

static char *
  set_jp2_attributes(jp2_dimensions dims, jp2_palette pclr,
                     jp2_resolution res, jp2_channels channels,
                     jp2_colour colr, siz_params *siz,
                     kdu_rgb8_palette &palette,
                     int num_components, kdu_args &args,
                     jpx_layer_target &jpx_layer,
                     kdu_image_dims &idims, bool transpose)
  /* The return value, if non-NULL, represents the comma-separated list of
     extra JP2 box files obtained from a `-jp2_box' argument.
        If `jpx_layer.exists()' returns true, the JP2 attributes are being
     prepared for a JPX file.  In this case, additional colour space
     information may be supplied via a "-jpx_space" command-line argument.
       The `transpose' is true, we must transpose any resolution informtion
     found in `idims' -- this is the only reason for supplying the argument. */
{
  char *extra_box_files = NULL;
  // Set dimensional information (all redundant with the SIZ marker segment)
  dims.init(siz);

  // Set resolution information (optional)
  if (args.find("-jp2_aspect") != NULL)
    {
      float aspect_ratio;
      char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%f",&aspect_ratio) != 1) ||
          (aspect_ratio <= 0.0F))
        { kdu_error e; e << "Missing or illegal aspect ratio "
          "parameter supplied with the `-jp2_aspect' argument!"; }
      args.advance();
      res.init(aspect_ratio);
    }
  else
    { // See if `idims' contains any resolution information
      bool units_known;
      double xpels_per_metre, ypels_per_metre;
      if (idims.get_resolution(0,units_known,xpels_per_metre,ypels_per_metre))
        {
          if (transpose)
            { double tmp = xpels_per_metre;
              xpels_per_metre = ypels_per_metre;  ypels_per_metre = tmp; }
          int xfac=0, yfac=0;
          siz->get(Ssampling,0,0,yfac);  siz->get(Ssampling,0,1,xfac);
          assert((xfac > 0) && (yfac > 0));
          xpels_per_metre *= xfac;  ypels_per_metre *= yfac;
          res.init((float)(xpels_per_metre/ypels_per_metre));
          if (units_known)
            res.set_resolution((float) ypels_per_metre);
        }
    }

  // Set colour space information (mandatory)
  bool have_opponent_space=false, have_non_opponent_space=false;
  int min_colours=1, max_colours=num_components;
  if (palette.exists())
    {
      if (palette.source_component == 0)
        min_colours = max_colours = (palette.is_monochrome())?1:3;
      else
        max_colours = palette.source_component;
    }

  bool have_premultiplied_alpha, have_unassociated_alpha;
  jp2_colour_space in_space;
  int in_space_confidence;
  int in_colours = // Will be zero if the file reader does not know # colours
    idims.get_colour_info(have_premultiplied_alpha,have_unassociated_alpha,
                          in_space_confidence,in_space);

  if (args.find("-jp2_space") != NULL)
    {
      char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "The `-jp2_space' argument requires a parameter "
          "string!"; }
      char *delim = strchr(string,',');
      if (delim != NULL)
        *delim = '\0';
      if (strcmp(string,"sRGB") == 0)
        colr.init(JP2_sRGB_SPACE);
      else if (strcmp(string,"sYCC") == 0)
        colr.init(JP2_sYCC_SPACE);
      else if (strcmp(string,"sLUM") == 0)
        colr.init(JP2_sLUM_SPACE);
      else if (strcmp(string,"iccLUM") == 0)
        {
          double gamma, beta;
          string = delim+1;
          if ((delim == NULL) || ((delim = strchr(string,',')) == NULL) ||
              ((*delim = '\0') > 0) || (sscanf(string,"%lf",&gamma) != 1) ||
              (sscanf(delim+1,"%lf",&beta) != 1) || (gamma <= 1.0) ||
              (beta < 0.0) || (beta >= 1.0))
            { kdu_error e; e << "Missing or illegal gamma/beta parameters "
              "supplied in comma-separated parameter list which must follow "
              "the \"sLUM\" JP2 colour space specification supplied via the "
              "`-jp2_space' argument.  `gamma' must be greater than 1 and "
              "`beta' must be in the range 0 to 1."; }
          colr.init(gamma,beta);
        }
      else if (strcmp(string,"iccRGB") == 0)
        {
          double val, gamma, beta, xy_red[2], xy_green[2], xy_blue[2];
          bool illuminant_is_D50;
          int p;
          for (p=0; p < 8; p++)
            {
              string = delim+1;
              if ((delim == NULL) ||
                  ((delim = strchr(string,',')) == NULL) ||
                  ((*delim = '\0') > 0) ||
                  (sscanf(string,"%lf",&val) != 1))
                { kdu_error e; e << "The \"iccRGB\" specification must be "
                  "followed immediately by a comma-separated list of 9 "
                  "parameters, all within the single parameter string "
                  "supplied with the `-jp2_space' argument.  For more details "
                  "review the usage statement."; }
              if (p == 0)
                gamma = val;
              else if (p == 1)
                beta = val;
              else if (p < 4)
                xy_red[p-2] = val;
              else if (p < 6)
                xy_green[p-4] = val;
              else if (p < 8)
                xy_blue[p-6] = val;
            }
          string = delim + 1;
          if (delim == NULL)
            { kdu_error e; e << "The \"iccRGB\" specification must be "
              "followed by a list of 9 parameters, the last of which is "
              "one of the strings \"D50\" or \"D65\"."; }
          else if (strcmp(string,"D50") == 0)
            illuminant_is_D50 = true;
          else if (strcmp(string,"D65") == 0)
            illuminant_is_D50 = false;
          else
            { kdu_error e; e << "The \"iccRGB\" specification must be "
              "followed by a list of 9 parameters, the last of which is "
              "one of the strings \"D50\" or \"D65\"."; }
          for (p=0; p < 2; p++)
            if ((beta < 0.0) || (beta >= 1.0) || (gamma <= 1.0) ||
                (xy_red[p]   < 0.0) || (xy_red[p]   > 1.0) ||
                (xy_green[p] < 0.0) || (xy_green[p] > 1.0) ||
                (xy_blue[p]  < 0.0) || (xy_blue[p]  > 1.0))
              { kdu_error e; e << "One or more parameters supplied with the "
                "\"iccRGB\" `-jp2_space' argument lie outside the legal "
                "range."; }
          colr.init(xy_red,xy_green,xy_blue,gamma,beta,100,illuminant_is_D50);
        }
      else
        { kdu_error e; e << "Invalid parameter string following `-jp2_space' "
          "argument.  The string must identify the colour space as one of "
          "\"sLUM\", \"sRGB\", \"sYCC\", \"iccLUM\" or \"iccRGB\"."; }
      args.advance();

      if ((colr.get_num_colours() > max_colours) ||
          (colr.get_num_colours() < min_colours))
        { kdu_error e; e << "The number of colours associated with the "
          "colour space specified using `-jp2_space' are not compatible "
          "with the number of supplied image components and/or colour "
          "palette."; }
      min_colours = max_colours = colr.get_num_colours();

      if (colr.is_opponent_space())
        have_opponent_space = true;
      else
        have_non_opponent_space = true;
      colr = jp2_colour(NULL); // So we know that colour space is initialized
    }

  if (args.find("-jpx_space") != NULL)
    {
      char *delim, *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "The `-jpx_space' argument requires a parameter "
          "string!"; }
      if (!jpx_layer)
        { kdu_error e; e << "The `-jpx_space' argument may only be used "
          "with JPX files -- i.e., your output file must have either a "
          "`.jpx' or `.jpf' suffix."; }
      int prec=0, approx=0;
      jp2_colour_space space;
      delim = strchr(string,',');
      if (delim != NULL)
        *(delim++) = '\0';
      if (strcmp(string,"bilevel1") == 0)
        space = JP2_bilevel1_SPACE;
      else if (strcmp(string,"bilevel2") == 0)
        space = JP2_bilevel2_SPACE;
      else if (strcmp(string,"YCbCr1") == 0)
        space = JP2_YCbCr1_SPACE;
      else if (strcmp(string,"YCbCr2") == 0)
        space = JP2_YCbCr2_SPACE;
      else if (strcmp(string,"YCbCr3") == 0)
        space = JP2_YCbCr3_SPACE;
      else if (strcmp(string,"PhotoYCC") == 0)
        space = JP2_PhotoYCC_SPACE;
      else if (strcmp(string,"CMY") == 0)
        space = JP2_CMY_SPACE;
      else if (strcmp(string,"CMYK") == 0)
        space = JP2_CMYK_SPACE;
      else if (strcmp(string,"YCCK") == 0)
        space = JP2_YCCK_SPACE;
      else if (strcmp(string,"CIELab") == 0)
        space = JP2_CIELab_SPACE;
      else if (strcmp(string,"CIEJab") == 0)
        space = JP2_CIEJab_SPACE;
      else if (strcmp(string,"sLUM") == 0)
        space = JP2_sLUM_SPACE;
      else if (strcmp(string,"sRGB") == 0)
        space = JP2_sRGB_SPACE;
      else if (strcmp(string,"sYCC") == 0)
        space = JP2_sYCC_SPACE;
      else if (strcmp(string,"esRGB") == 0)
        space = JP2_esRGB_SPACE;
      else if (strcmp(string,"esYCC") == 0)
        space = JP2_esYCC_SPACE;
      else if (strcmp(string,"ROMMRGB") == 0)
        space = JP2_ROMMRGB_SPACE;
      else if (strcmp(string,"YPbPr60_SPACE") == 0)
        space = JP2_YPbPr60_SPACE;
      else if (strcmp(string,"YPbPr50_SPACE") == 0)
        space = JP2_YPbPr50_SPACE;
      else
        { kdu_error e; e << "Unrecognized colour space type, \""
          << string << "\", provided with `-jpx_space' argument."; }
      if ((delim != NULL) &&
          ((sscanf(delim,"%d,%d",&prec,&approx) != 2) ||
           (prec < -128) || (prec > 127) || (approx < 0) || (approx > 4)))
        { kdu_error e; e << "Illegal or incomplete precedence/approximation "
          "information provided with `-jpx_space' argument."; }
      if (!colr.exists())
        colr = jpx_layer.add_colour(prec,(kdu_byte) approx);
      colr.init(space);
      args.advance();

      if ((colr.get_num_colours() > max_colours) ||
          (colr.get_num_colours() < min_colours))
        { kdu_error e; e << "The number of colours associated with the "
          "colour space specified using `-jpx_space' are not compatible "
          "with the number of supplied image components and/or colour "
          "palette."; }
      min_colours = max_colours = colr.get_num_colours();

      if (colr.is_opponent_space())
        have_opponent_space = true;
      else
        have_non_opponent_space = true;
      colr = jp2_colour(NULL); // So we know that colour space is initialized
    }

  if (colr.exists() && (in_space_confidence > 0))
    { // Colour space specification derived from the source file
      colr.init(in_space);
      min_colours = max_colours = colr.get_num_colours();
      if (colr.is_opponent_space())
        have_opponent_space = true;
      else
        have_non_opponent_space = true;
      colr = jp2_colour(NULL); // So we know that colour space is initialized
    }

  if (have_opponent_space && have_non_opponent_space)
    { kdu_error e; e << "You have specified multiple colour specifications, "
      "where one specification represents an opponent colour space, while "
      "the other does not.  This contradictory information leaves us "
      "uncertain as to whether the code-stream colour transform should be "
      "used or not, but is almost certainly a mistake anyway."; }

  // Set the actual number of colour planes and the index of any alpha
  // component
  int opacity_idx=-1;
  if (palette.exists())
    opacity_idx = palette.source_component;
  if (in_colours > 0)
    { // Source image file identifies the number of colours
      if (have_premultiplied_alpha && (opacity_idx < 0))
        opacity_idx = in_colours;
      if ((min_colours > in_colours) || (max_colours < in_colours))
        {
          kdu_warning w; w << "The number of colour planes identified by the "
            "image file format reading logic is not consistent with the "
            "indicated colour space, with the number of available image "
            "components, or with the use of a colour palette.";
          if (have_premultiplied_alpha && (args.find("-jp2_alpha") == NULL))
            {
              have_premultiplied_alpha = false;
              kdu_warning w; w << "Since you have specified a different "
                "number of colours to that indicated by the file, the "
                "premultiplied alpha channel embedded in the file will not "
                "be regarded as an alpha channel unless you explicitly "
                "supply the `-jp2_alpha' switch to confirm that this is "
                "really what you want.  The alpha channel will be taken from "
                "component " << opacity_idx << " (starting from 0), which "
                "may or may not conflict with the use of components for "
                "your colour space.";
            }
        }
      else
        min_colours = max_colours = in_colours;
    }
  int num_colours = max_colours;
  if (max_colours > min_colours)
    { // Actual number of colours is not known; we can make up our own mind
      assert(min_colours == 1);
      num_colours = (max_colours < 3)?1:3;
    }
  if (opacity_idx < 0)
    opacity_idx = num_colours;

  if (colr.exists())
    { // Still have not initialized the colour space yet
      colr.init((num_colours==3)?JP2_sRGB_SPACE:JP2_sLUM_SPACE);
    }

  // Check for alpha support
  if (args.find("-jp2_alpha") != NULL)
    {
      args.advance();
      if (!have_premultiplied_alpha)
        have_unassociated_alpha = true;
    }

  // Set the colour palette and channel mapping as needed.
  if (palette.exists() && (palette.source_component == 0))
    {
      if ((have_unassociated_alpha || have_premultiplied_alpha) &&
          (opacity_idx >= num_components))
        { kdu_error e; e << "The `-jp2_alpha' argument or the image "
          "file header itself suggest that there should be an alpha "
          "component.  Yet there are not sufficient image components "
          "available to accommodate an alpha channel."; }
      if (palette.is_monochrome())
        {
          pclr.init(1,1<<palette.input_bits);
          pclr.set_lut(0,palette.red,palette.output_bits);
          assert(num_colours == 1);
          channels.init(1);
          channels.set_colour_mapping(0,palette.source_component,0);
          if (have_unassociated_alpha)
            channels.set_opacity_mapping(0,opacity_idx);
          else if (have_premultiplied_alpha)
            channels.set_premult_mapping(0,opacity_idx);
        }
      else
        {
          pclr.init(3,1<<palette.input_bits);
          pclr.set_lut(0,palette.red,palette.output_bits);
          pclr.set_lut(1,palette.green,palette.output_bits);
          pclr.set_lut(2,palette.blue,palette.output_bits);
          assert(num_colours == 3);
          channels.init(3);
          channels.set_colour_mapping(0,palette.source_component,0);
          channels.set_colour_mapping(1,palette.source_component,1);
          channels.set_colour_mapping(2,palette.source_component,2);
          if (have_unassociated_alpha)
            {
              channels.set_opacity_mapping(0,opacity_idx);
              channels.set_opacity_mapping(1,opacity_idx);
              channels.set_opacity_mapping(2,opacity_idx);
            }
          else if (have_premultiplied_alpha)
            {
              channels.set_premult_mapping(0,opacity_idx);
              channels.set_premult_mapping(1,opacity_idx);
              channels.set_premult_mapping(2,opacity_idx);
            }
        }
    }
  else if (have_unassociated_alpha || have_premultiplied_alpha)
    {
      if (opacity_idx >= num_components)
        { kdu_error e; e << "The `-jp2_alpha' argument or the image "
          "file header itself suggest that there should be an alpha "
          "component.  Yet there are not sufficient image components "
          "available to accommodate an alpha channel."; }
      channels.init(num_colours);
      int lut_idx = -1;
      if (palette.exists() && (palette.source_component == opacity_idx))
        {
          pclr.init(1,1<<palette.input_bits);
          pclr.set_lut(0,palette.red,palette.output_bits);
          lut_idx = 0;
        }
      for (int c=0; c < num_colours; c++)
        {
          channels.set_colour_mapping(c,c);
          if (have_unassociated_alpha)
            channels.set_opacity_mapping(c,opacity_idx,lut_idx);
          else
            channels.set_premult_mapping(c,opacity_idx,lut_idx);
        }
    }
  else
    {
      channels.init(num_colours);
      for (int c=0; c < num_colours; c++)
        channels.set_colour_mapping(c,c);
    }

  // Find extra JP2 boxes.
  if (args.find("-jp2_box") != NULL)
    {
      char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "The `-jp2_box' argument requires a parameter "
          "string!"; }
      extra_box_files = new char[strlen(string)+1];
      strcpy(extra_box_files,string);
      args.advance();
    }

  return extra_box_files;
}

/*****************************************************************************/
/* STATIC                  create_extra_jpx_layers                           */
/*****************************************************************************/

static void
  create_extra_jpx_layers(jpx_target &tgt, jpx_layer_target &first_layer,
                          int num_jpx_layers, int num_available_components)
  /* This function implements the functionality described in conjunction
     with the `-jpx_layers' command-line argument (see `print_usage').  It
     replicates the features of the `first_layer' into additional quality
     layers, associating them with consecutive image components.  If
     `num_jpx_layers'=0, the function first determines the maximum number of
     JPX quality layers which can be supported by the available set of
     image components. */
{
  jp2_channels first_channels = first_layer.access_channels();
  int c, num_colours = first_channels.get_num_colours();
  int comp_idx, lut_idx, cs_idx, num_layer_components = 0;
  for (c=0; c < num_colours; c++)
    {
      if (first_channels.get_colour_mapping(c,comp_idx,lut_idx,cs_idx) &&
          (comp_idx >= num_layer_components))
        num_layer_components = comp_idx+1;
      if (first_channels.get_opacity_mapping(c,comp_idx,lut_idx,cs_idx) &&
          (comp_idx >= num_layer_components))
        num_layer_components = comp_idx+1;
      if (first_channels.get_premult_mapping(c,comp_idx,lut_idx,cs_idx) &&
          (comp_idx >= num_layer_components))
        num_layer_components = comp_idx+1;
    }
  if (num_layer_components == 0)
    return;
  if ((num_jpx_layers == 0) ||
      ((num_jpx_layers*num_layer_components) > num_available_components))
    num_jpx_layers = num_available_components / num_layer_components;

  int layer_idx;
  for (layer_idx=1; layer_idx < num_jpx_layers; layer_idx++)
    {
      int comp_offset = layer_idx*num_layer_components;
      jpx_layer_target new_layer = tgt.add_layer();

      new_layer.access_resolution().copy(first_layer.access_resolution());

      jp2_colour first_colour;
      int which_colr = 0;
      while ((first_colour = first_layer.access_colour(which_colr)).exists())
        {
          jp2_colour new_colour =
            new_layer.add_colour(first_colour.get_precedence(),
                                 first_colour.get_approximation_level());
          new_colour.copy(first_colour);
          which_colr++;
        }

      jp2_channels new_channels = new_layer.access_channels();
      new_channels.init(num_colours);
      for (c=0; c < num_colours; c++)
        {
          if (first_channels.get_colour_mapping(c,comp_idx,lut_idx,cs_idx))
            new_channels.set_colour_mapping(c,comp_idx+comp_offset,lut_idx,0);
          if (first_channels.get_opacity_mapping(c,comp_idx,lut_idx,cs_idx))
            new_channels.set_opacity_mapping(c,comp_idx+comp_offset,lut_idx,0);
          if (first_channels.get_premult_mapping(c,comp_idx,lut_idx,cs_idx))
            new_channels.set_premult_mapping(c,comp_idx+comp_offset,lut_idx,0);
        }

    }
}

/*****************************************************************************/
/* STATIC                    write_extra_jp2_boxes                           */
/*****************************************************************************/

static void
  write_extra_jp2_boxes(jp2_family_tgt *tgt, char *box_files,
                        kdu_image_dims &idims)
{
  jp2_output_box out;

  // Start by writing any extra meta-data boxes recorded in `idims'
  int idx;
  jp2_output_box *box_ref=NULL;
  for (idx=0; (box_ref=idims.get_source_metadata(idx)) != NULL; idx++)
    {
      kdu_long length;
      kdu_uint32 box_type = box_ref->get_box_type();
      const kdu_byte *contents =
        box_ref->get_contents(length);
      out.open(tgt,box_type);
      out.set_target_size(length);
      out.write(contents,(int) length);
      out.close();
    }

  // Finish by writing extra JP2 box files
  char *next;
  for (; box_files != NULL; box_files = next)
    {
      next = strchr(box_files,',');
      if (next != NULL)
        { *next = '\0'; next++; }
      if (*box_files == '\0')
        continue;
      FILE *fp = fopen(box_files,"rb");
      if (fp == NULL)
        { kdu_error e; e << "Unable to open the extra JP2 box file, \""
          << box_files << "\"\n"; }

      char header[130];
      strcpy(header,"mdata\n"); // Just in case
      fgets(header,130,fp);
      kdu_uint32 box_type = (int) header[0];
      box_type = (box_type << 8) + (int) header[1];
      box_type = (box_type << 8) + (int) header[2];
      box_type = (box_type << 8) + (int) header[3];
      
      out.open(tgt,box_type);
      out.write_header_last(); // Saves buffering or computing length first
      kdu_byte *buf = (kdu_byte *) header;
      int xfer_bytes;
      while ((xfer_bytes = (int) fread(buf,1,128,fp)) > 0)
        out.write(buf,xfer_bytes);
      out.close();
      fclose(fp);
    }
}

/*****************************************************************************/
/* STATIC                      assign_layer_bytes                            */
/*****************************************************************************/

static kdu_long *
  assign_layer_bytes(kdu_args &args, kdu_codestream &codestream,
                     int &num_specs)
  /* Returns a pointer to an array of `num_specs' quality layer byte
     targets.  The value of `num_specs' is determined in this function, based
     on the number of rates (or slopes) specified on the command line,
     together with any knowledge about the number of desired quality layers.
     Before calling this function, you must parse all parameter attribute
     strings into the code-stream parameter lists rooted at
     `codestream.access_siz'.  Note that the returned array will contain 0's
     whenever a quality layer's bit-rate is unspecified.  This allows the
     compressor's rate allocator to assign the target size for those quality
     layers on the fly. */
{
  char *cp;
  char *string = NULL;
  int arg_specs = 0;
  int slope_specs = 0;
  int cod_specs = 0;

  kdu_params *params = codestream.access_siz();
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

  kdu_params *cod = params->access_cluster(COD_params);
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
  
  kdu_long total_pels = get_bpp_dims(codestream);
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
     an explicit request for particular distortion-length slope thresholds.
     If too few slope thresholds are provided on the command line, the
     missing entries are interpolated or extrapolated following a
     heuristic. */
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
  args.advance();
  
  // Fill in any remaining missing values.
  if ((num_specs > n) && (n > 0) && (result[0] > 1))
    {
      int slope_delta = 256; // Default separation between missing slopes
      bool extrapolate = true;
      int num_missing_specs = num_specs - n;
      if ((n > 1) && (result[1] > 1))
        {
          int existing_delta = result[0]-result[1];
          int interpolated_delta = existing_delta / (num_missing_specs+1);
          if (interpolated_delta >= slope_delta)
            { slope_delta = interpolated_delta; extrapolate = false; }
          else if (existing_delta < slope_delta)
            { slope_delta = existing_delta; extrapolate = true; }
          else if ((((kdu_long) slope_delta)*slope_delta) <
                   (((kdu_long) interpolated_delta)*existing_delta))
            { slope_delta = interpolated_delta; extrapolate = false; }
          else
            { slope_delta = existing_delta; extrapolate = true; }
          if (extrapolate && (slope_delta > 512))
            slope_delta = 256;
        }
      if (extrapolate)
        {
          if (((slope_delta*(num_missing_specs+1))+result[0]) > 0x0FFFF)
            slope_delta = (0x0FFFF - (int) result[0]) / (num_missing_specs+1);
          for (k=num_specs-1; k >= num_missing_specs; k--)
            result[k] = result[k-num_missing_specs];
          for (; k >= 0; k--)
            result[k] = result[k+1]+slope_delta;
        }
      else
        { // Interpolate the final 2 values
          for (k=num_specs-1; k > num_missing_specs; k--)
            result[k] = result[k-num_missing_specs];
          for (; k > 0; k--)
            result[k] = result[k+1]+slope_delta;
        }
    }
  return result;
}

/*****************************************************************************/
/* STATIC                      create_roi_source                             */
/*****************************************************************************/

static kdu_roi_image *
  create_roi_source(kdu_codestream codestream, kdu_args &args)
{
  if (args.find("-roi") == NULL)
    return NULL;
  char *string = args.advance();
  kdu_roi_image *result = NULL;
  if ((string != NULL) && (*string == '{'))
    {
      double top, left, height, width;

      if ((sscanf(string,"{%lf,%lf},{%lf,%lf}",
                  &top,&left,&height,&width) != 4) ||
          ((top < 0.0) || (left < 0.0) || (height < 0.0) || (width < 0.0)))
        { kdu_error e; e << "The `-roi' argument requires a set of "
          "coordinates of the form, \"{<top>,<left>},{<height>,<width>}\", "
          "where all quantities must be real numbers in the range 0 to 1."; }
      kdu_dims region; codestream.get_dims(-1,region);
      region.pos.y += (int) floor(region.size.y * top);
      region.pos.x += (int) floor(region.size.x * left);
      region.size.y = (int) ceil(region.size.y * height);
      region.size.x = (int) ceil(region.size.x * width);
      result = new kdu_roi_rect(codestream,region);
    }
  else
    { // Must be file-name/threshold form.
      char *fname = string;
      float threshold;

      if ((fname == NULL) || ((string = strchr(fname,',')) == NULL) ||
          (sscanf(string+1,"%f",&threshold) == 0) ||
          (threshold < 0.0F) || (threshold >= 1.0F))
        { kdu_error e; e << "The `-roi' argument requires a single parameter "
          "string, which should either identify the four coordinates of a "
          "rectangular foreground region or else an image file and threshold "
          "value, separated by a comma.  The threshold may be no less than 0 "
          "and must be strictly less than 1."; }
      *string = '\0';
      result = new kdu_roi_graphics(codestream,fname,threshold);
    }
  args.advance();
  return result;
}

/*****************************************************************************/
/* STATIC                  compress_single_threaded                          */
/*****************************************************************************/

static kdu_long
  compress_single_threaded(kdu_codestream codestream, kdu_dims tile_indices,
                           kdc_file_binding *inputs,
                           kdu_roi_image *roi_source,
                           kdu_long *layer_bytes, int num_layer_specs,
                           kdu_uint16 *layer_thresholds,
                           bool record_info_in_comseg, double rate_tolerance,
                           bool allow_shorts, int flush_period,
                           int dwt_stripe_height, int progress_interval)
  /* This function wraps up the operations required to actually compress
     the source samples and flush the generated output codestream.  It is
     called directly from `main' after setting up the input files (passed in
     via the `inputs' list), configuring the `codestream' object and
     parsing relevant command-line arguments.
        This particular function implements all compression processing
     using a single thread of execution.  This is the simplest approach.
     From version 5.1 of Kakadu, the processing may also be efficiently
     distributed across multiple threads, which allows for the
     exploitation of multiple physical processors.  The implementation in
     that case is only slightly different from the multi-threaded case, but
     we encapsulate it in a separate version of this function,
     `compress_multi_threaded', mainly for illustrative purposes.
        The function returns the amount of memory allocated for sample
     processing, including all intermediate line buffers managed by
     the DWT engines associated with each active tile-component and the block
     encoding machinery associated with each tile-component-subband.
        The implementation here processes image lines one-by-one,
     maintaining W complete tile processing engines, where W is the number
     of tiles which span the width of the image.  There are a variety of
     alternate processing paradigms which can be used.  The
     "kdu_buffered_compress" application demonstrates a different strategy,
     managed by the higher level `kdu_stripe_compressor' object, in which
     whole image stripes are buffered in memory and then passed into
     tile processing engines.  If the stripe height is equal to the tile
     height, only one tile processing engine need be active at any given
     time in that model. */
{
  int x_tnum;
  kdc_flow_control **tile_flows = new kdc_flow_control *[tile_indices.size.x];
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    tile_flows[x_tnum] =
      new kdc_flow_control(inputs,codestream,x_tnum,allow_shorts,roi_source,
                           dwt_stripe_height);
  bool done = false;
  int flush_counter = flush_period;
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
          if (!done)
            { 
              flush_counter--;
              if ((++progress_counter) == progress_interval)
                { 
                  pretty_cout << "\t\tProgress with current tile row = "
                              << tile_flows[0]->percent_pushed()
                              << "%\n";
                  progress_counter = 0;
                }
            }
          if (flush_counter <= 0)
            {
              int rem = tile_flows[0]->get_max_remaining_lines();
              if ((rem > 0) && (rem < (flush_period>>2)))
                flush_counter = rem; // Worth waiting til row of tiles is done
              else if (!codestream.ready_for_flush())
                flush_counter = 1 + (flush_period >> 2); // Try again later on
              else
                { 
                  if (progress_interval)
                    pretty_cout << "\tInitiating codestream flush ...\n";
                  codestream.flush(layer_bytes,num_layer_specs,
                                   layer_thresholds,true,
                                   record_info_in_comseg,rate_tolerance);
                  flush_counter += flush_period;
                }
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
  kdu_long sample_processing_bytes = 0;
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      sample_processing_bytes += tile_flows[x_tnum]->get_buffer_memory();
      delete tile_flows[x_tnum];
    }
  delete[] tile_flows;

  if (progress_interval)
    pretty_cout << "\tInitiating final codestream flush ...\n";
  if (codestream.ready_for_flush())
    codestream.flush(layer_bytes,num_layer_specs,layer_thresholds,
                     true,record_info_in_comseg,rate_tolerance);
          // Otherwise, incremental flush did it all
  return sample_processing_bytes;
}

/*****************************************************************************/
/* STATIC                   compress_multi_threaded                          */
/*****************************************************************************/

static kdu_long
  compress_multi_threaded(kdu_codestream codestream, kdu_dims tile_indices,
                          kdc_file_binding *inputs,
                          kdu_roi_image *roi_source,
                          kdu_long *layer_bytes, int num_layer_specs,
                          kdu_uint16 *layer_thresholds,
                          bool record_info_in_comseg, double rate_tolerance,
                          bool allow_shorts, int flush_period,
                          int &num_threads, bool dwt_double_buffering,
                          int dwt_stripe_height, int progress_interval)
  /* This function provides exactly the same functionality as
     `compress_single_threaded', except that it uses Kakadu's multi-threaded
     processing features.  By and large, multi-threading does not substantially
     complicate the implementation, since Kakadu's threading framework
     conceal almost all of the details.  However, the application does have
     to create a multi-threaded environment, assigning it a suitable number
     of threads.  It must also be careful to close down the multi-threaded
     environment, which incorporates all required synchronization.  Finally,
     where incremental flushing of the codestream is required, this is best
     achieved by registering synchronized jobs with the multi-threading
     environment, rather than explicitly synchronizing all threads and then
     running the flush operation directly.
        Upon return, `num_threads' is set to the actual number of threads
     which were created -- this value could be smaller than the value supplied
     on input, if insufficient internal resources exist.
        For other aspects of the present function, refer to the comments
     found with `compress_single_threaded'. */
{
  // Construct multi-threaded processing environment if required
  kdu_thread_env env;
  env.create();
  for (int nt=1; nt < num_threads; nt++)
    if (!env.add_thread())
      { num_threads = nt; break; }

  // Now set up the tile processing objects
  int x_tnum;
  kdc_flow_control **tile_flows = new kdc_flow_control *[tile_indices.size.x];
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      kdu_thread_queue *tile_queue =
        env.add_queue(NULL,NULL,"tile compressor");      
      tile_flows[x_tnum] =
        new kdc_flow_control(inputs,codestream,x_tnum,allow_shorts,roi_source,
                             dwt_stripe_height,dwt_double_buffering,
                             &env,tile_queue);
    }

  // Create a `kdc_flush_worker' object to do incremental flushing of the
  // codestream in the background, while threads continue processing.  This
  // is not necessary, but it helps minimize thread idle time.
  kdc_flush_worker flush_worker;

  // Now run the tile processing engines
  bool done = false;
  int flush_counter = flush_period;
  int post_flush_counter = -1; // If this is > 0, we are in the period
                   // immediately following the registration of an intent to
                   // flush the codestream, but we still do not know if the
                   // flush was successful (i.e., we don't know if
                   // `codestream.ready_for_flush' returned true or false
                   // inside the flush worker).  Once we know this, we can
                   // determine whether or not an offset needs to be introduced
                   // into the `flush_counter' to account for lag in the DWT.
                   // This was much easier to do in the single-threaded case,
                   // since we always new the outcome of `ready_for_flush'
                   // immediately.

  int tile_row = 0; // Just for progress counter
  int progress_counter = 0;
  while (!done)
    {
      bool flush_on_tile_boundary=false;
      while (!done)
        { // Process a row of tiles line by line.
          done = true;
          for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
            {
              if (tile_flows[x_tnum]->advance_components(&env))
                {
                  done = false;
                  tile_flows[x_tnum]->process_components(&env);
                }
            }
          if (!done)
            { 
              flush_counter--;
              post_flush_counter--;
              if ((++progress_counter) == progress_interval)
                { 
                  pretty_cout << "\t\tProgress with current tile row = "
                              << tile_flows[0]->percent_pushed()
                              << "%\n";
                  progress_counter = 0;
                }
            }

          if (((flush_counter <= 0) || (post_flush_counter==0)) &&
              !flush_on_tile_boundary)
            {
              int rem = tile_flows[0]->get_max_remaining_lines();
              if (rem == 0)
                {
                  flush_on_tile_boundary = true;
                  flush_counter += flush_period;
                  post_flush_counter = -1;
                }
              else if (rem < (flush_period>>2))
                { // Wait until row of tiles is done.
                  flush_counter = rem;
                  post_flush_counter = -1;
                }
              else if (flush_worker.is_free())
                {
                  if ((flush_counter <= 0) ||
                      flush_worker.ready_for_flush_failed())
                    { 
                      if (progress_interval)
                        pretty_cout <<
                          "\tInitiating potential codestream flush ...\n";
                      flush_worker.init(codestream,layer_bytes,num_layer_specs,
                                   layer_thresholds,true,record_info_in_comseg,
                                   rate_tolerance);
                      env.register_synchronized_job(&flush_worker,NULL,true);
                      if (flush_counter > 0)
                        flush_counter = flush_period;
                      else
                        flush_counter += flush_period;
                      post_flush_counter = 1 + (flush_period>>2);
                    }
                }
              else
                { // We will have to check the outcome of a previously
                  // installed, but not yet run, flush job later.
                  flush_counter += 1 + (flush_period>>2);
                  post_flush_counter += 1 + (flush_period>>2);
                }
            }
        }
      
      for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
        if (tile_flows[x_tnum]->advance_tile(&env))
          done = false;
      if (flush_on_tile_boundary && flush_worker.is_free())
        { // Do incremental flush asynchronously.  The reason we install the
          // condition here when it corresponds to a tile boundary is that
          // the flushing operation may lock up access to the codestream
          // management machinery for some time.  So long as we have been able
          // to open the new row of tiles, however, we can continue processing
          // while the flush is going on, until we need to flush
          // thread-local code-block storage to the central repository.
          if (progress_interval)
            pretty_cout << "\tInitiating potential codestream flush ...\n";
          flush_worker.init(codestream,layer_bytes,num_layer_specs,
                            layer_thresholds,true,record_info_in_comseg,
                            rate_tolerance);
          env.register_synchronized_job(&flush_worker,NULL,true);
        }
      flush_on_tile_boundary = false;
    
      tile_row++;
      progress_counter = 0;
      if (progress_interval > 0)
        pretty_cout << "\tFinished processing " << tile_row
                    << " of " << tile_indices.size.y << " tile rows\n";
    }

  // Cleanup processing environment
  env.terminate(NULL,true); // Makes sure any outstanding flush job finishes
  env.destroy();
  kdu_long sample_processing_bytes = 0;
  for (x_tnum=0; x_tnum < tile_indices.size.x; x_tnum++)
    {
      sample_processing_bytes += tile_flows[x_tnum]->get_buffer_memory();
      delete tile_flows[x_tnum];
    }
  delete[] tile_flows;

  // Final flush, if required
  if (progress_interval)
    pretty_cout << "\tInitiating final codestream flush ...\n";
  if (codestream.ready_for_flush())
    codestream.flush(layer_bytes,num_layer_specs,layer_thresholds,
                     true,record_info_in_comseg,rate_tolerance);
  return sample_processing_bytes;
}


/* ========================================================================= */
/*                              kdc_flow_control                             */
/* ========================================================================= */

/*****************************************************************************/
/*                     kdc_flow_control::kdc_flow_control                    */
/*****************************************************************************/

kdc_flow_control::kdc_flow_control(kdc_file_binding *files,
                                   kdu_codestream codestream, int x_tnum,
                                   bool allow_shorts,
                                   kdu_roi_image *roi_image,
                                   int dwt_stripe_height,
                                   bool dwt_double_buffering,
                                   kdu_thread_env *env,
                                   kdu_thread_queue *env_queue)
{
  int n;

  this->codestream = codestream;
  this->x_tnum = x_tnum;
  codestream.get_valid_tiles(valid_tile_indices);
  assert((x_tnum >= 0) && (x_tnum < valid_tile_indices.size.x));
  tile_idx = valid_tile_indices.pos;
  tile_idx.x += x_tnum;
  tile = codestream.open_tile(tile_idx,env);
  this->roi_image = roi_image;
  this->allow_shorts = allow_shorts;
  this->dwt_double_buffering = dwt_double_buffering;
  this->dwt_stripe_height = dwt_stripe_height;
  this->env_queue = (env==NULL)?NULL:env_queue;
  assert((env == NULL) || (env_queue != NULL));

  // Set up the individual components.
  num_components = codestream.get_num_components(true);
  components = new kdc_component_flow_control[num_components];
  count_delta = 0;
  for (n=0; n < num_components; n++)
    {
      kdc_component_flow_control *comp = components + n;
      comp->line = NULL;
      assert(n >= files->first_comp_idx);
      if ((n-files->first_comp_idx) >= files->num_components)
        files = files->next;
      if (files == NULL)
        {
          num_components = n;
          break;
        }
      comp->reader = files->reader;
      kdu_coords subsampling;  codestream.get_subsampling(n,subsampling,true);
      kdu_dims dims;  codestream.get_tile_dims(tile_idx,n,dims,true);
      comp->vert_subsampling = subsampling.y;
      if ((n == 0) || (comp->vert_subsampling < count_delta))
        count_delta = comp->vert_subsampling;
      comp->ratio_counter = 0;
      comp->remaining_lines = comp->initial_lines = dims.size.y;
    }
  assert(num_components > 0);

  tile.set_components_of_interest(num_components);
  max_buffer_memory =
    engine.create(codestream,tile,!allow_shorts,roi_image,false,
                  dwt_stripe_height,env,env_queue,dwt_double_buffering);
}

/*****************************************************************************/
/*                    kdc_flow_control::~kdc_flow_control                    */
/*****************************************************************************/

kdc_flow_control::~kdc_flow_control()
{
  if (components != NULL)
    delete[] components;
  if (engine.exists())
    engine.destroy();
}

/*****************************************************************************/
/*                  kdc_flow_control::advance_components                     */
/*****************************************************************************/

bool
  kdc_flow_control::advance_components(kdu_thread_env *env)
{
  bool found_line=false;

  while (!found_line)
    {
      bool all_done = true;
      for (int n=0; n < num_components; n++)
        {
          kdc_component_flow_control *comp;
          comp = components + n;
          assert(comp->ratio_counter >= 0);
          if (comp->remaining_lines > 0)
            {
              all_done = false;
              comp->ratio_counter -= count_delta;
              if (comp->ratio_counter < 0)
                {
                  found_line = true;
                  comp->line = engine.exchange_line(n,NULL,env);
                  assert(comp->line != NULL);
                  if (comp->reader.exists())
                    comp->reader.get(n,*(comp->line),x_tnum);
                }
            }
        }
      if (all_done)
        return false;
    }
  return true;
}

/*****************************************************************************/
/*                  kdc_flow_control::access_compressor_line                 */
/*****************************************************************************/

kdu_line_buf *
  kdc_flow_control::access_compressor_line(int comp_idx)
{
  assert((comp_idx >= 0) && (comp_idx < num_components));
  kdc_component_flow_control *comp = components + comp_idx;
  return (comp->ratio_counter < 0)?(comp->line):NULL;
}

/*****************************************************************************/
/*                    kdc_flow_control::process_components                   */
/*****************************************************************************/

void
  kdc_flow_control::process_components(kdu_thread_env *env)
{
  for (int n=0; n < num_components; n++)
    {
      kdc_component_flow_control *comp;
      comp = components + n;
      if (comp->ratio_counter < 0)
        {
          comp->ratio_counter += comp->vert_subsampling;
          assert(comp->ratio_counter >= 0);
          assert(comp->remaining_lines > 0);
          comp->remaining_lines--;
          assert(comp->line != NULL);
          engine.exchange_line(n,comp->line,env);
          comp->line = NULL;
        }
    }
}

/*****************************************************************************/
/*                        kdc_flow_control::advance_tile                     */
/*****************************************************************************/

bool
  kdc_flow_control::advance_tile(kdu_thread_env *env)
{
  int n;

  if (!tile)
    return false;

  assert(engine.exists());
  if (env != NULL)
    {
      assert(env_queue != NULL);
      env->terminate(env_queue,true);
    }
  engine.destroy();

  for (n=0; n < num_components; n++)
    {
      kdc_component_flow_control *comp = components + n;
      assert(comp->remaining_lines == 0);
      comp->line = NULL;
    }
  tile.close(env);
  tile = kdu_tile(NULL);

  tile_idx.y++;
  if ((tile_idx.y-valid_tile_indices.pos.y) == valid_tile_indices.size.y)
    return false;

  // Prepare for processing the next vertical tile.

  tile = codestream.open_tile(tile_idx,env);
  for (n=0; n < num_components; n++)
    {
      kdc_component_flow_control *comp = components + n;
      kdu_dims dims;  codestream.get_tile_dims(tile_idx,n,dims,true);
      comp->ratio_counter = 0;
      comp->remaining_lines = comp->initial_lines = dims.size.y;
    }

  tile.set_components_of_interest(num_components);
  kdu_long buffer_memory =
    engine.create(codestream,tile,!allow_shorts,roi_image,false,
                  dwt_stripe_height,env,env_queue,dwt_double_buffering);
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

  bool transpose, vflip, hflip;
  bool allow_rate_prediction, allow_shorts, mem, quiet;
  bool no_info, no_weights, no_palette;
  double rate_tolerance;
  int flush_period, num_jpx_layers;
  int num_threads, double_buffering_height, progress_interval, cpu_iterations;
  char *ofname;
  std::ostream *record_stream;
  kdc_file_binding *inputs =
    parse_simple_args(args,ofname,record_stream,transpose,vflip,hflip,
                      flush_period,rate_tolerance,allow_rate_prediction,
                      allow_shorts,no_info,no_weights,no_palette,
                      num_jpx_layers,num_threads,double_buffering_height,
                      progress_interval,cpu_iterations,mem,quiet);

  kdu_dims fragment_tile_indices;
  bool using_fragments =
    parse_fragment_args(args,fragment_tile_indices);
  bool first_fragment = (fragment_tile_indices.pos.x == 0) &&
                        (fragment_tile_indices.pos.y == 0);

  // Create appropriate output file format
  kdu_compressed_target *output = NULL;
  kdu_simple_file_target file_out;
  jp2_family_tgt jp2_ultimate_tgt;
  jp2_target jp2_out;
  jpx_target jpx_out;
  jpx_codestream_target jpx_stream;
  jpx_layer_target jpx_layer;
  jp2_dimensions jp2_family_dimensions;
  jp2_palette jp2_family_palette;
  jp2_resolution jp2_family_resolution;
  jp2_channels jp2_family_channels;
  jp2_colour jp2_family_colour;
  bool is_jp2=false, is_jpx=false;
  if ((num_jpx_layers != 1) || (args.find("-jpx_space") != NULL))
    is_jpx = true;
  else if (check_jp2_suffix(ofname))
    is_jp2 = true;
  else if (check_jpx_suffix(ofname))
    is_jpx = true;

  if (is_jp2 && first_fragment)
    {
      jp2_ultimate_tgt.open(ofname);
      jp2_out.open(&jp2_ultimate_tgt);
      jp2_family_dimensions = jp2_out.access_dimensions();
      jp2_family_palette = jp2_out.access_palette();
      jp2_family_resolution = jp2_out.access_resolution();
      jp2_family_channels = jp2_out.access_channels();
      jp2_family_colour = jp2_out.access_colour();
      output = &jp2_out;
    }
  else if (is_jpx && first_fragment)
    {
      jp2_ultimate_tgt.open(ofname);
      jpx_out.open(&jp2_ultimate_tgt);
      jpx_stream = jpx_out.add_codestream();
      jpx_layer = jpx_out.add_layer();
      jp2_family_dimensions = jpx_stream.access_dimensions();
      jp2_family_palette = jpx_stream.access_palette();
      jp2_family_resolution = jpx_layer.access_resolution();
      jp2_family_channels = jpx_layer.access_channels();
      jp2_family_colour = jpx_layer.add_colour();
      output = jpx_stream.access_stream();
    }
  else
    {
      output = &file_out;
      file_out.open(ofname,!first_fragment);
    }
  delete[] ofname;

  // Collect any command-line information concerning the input files.
  siz_params siz;
  const char *string;
  for (string=args.get_first(); string != NULL; )
    if (*string == '-')
      { args.advance(false); string = args.advance(false); }
    else
      string = args.advance(siz.parse_string(string));
  siz_params siz_scratch;
  siz_params *input_siz_ref=&siz, *codestream_siz_ref=&siz;
  kdu_dims fragment_region;
  int fragment_tiles_generated=0;
  kdu_long fragment_tile_bytes_generated=0;
  int fragment_tlm_tparts = 0;
  if (using_fragments)
    { // Convert fragment tile indices to a fragment region and move
      // the image file dimensions to a new `siz_params' object
      ((kdu_params *) codestream_siz_ref)->finalize();
                 // Must have sufficient information in
                 // explicit command-line arguments, from which to build a
                 // complete SIZ marker for the entire image, since we
                 // cannot reliably derive this information from the images
                 // used to generate a single fragment.
      input_siz_ref = &siz_scratch;
      fragment_region =
        find_fragment_region(fragment_tile_indices,
                             codestream_siz_ref,input_siz_ref);
      if (!first_fragment)
        retrieve_fragment_state(&file_out,fragment_tiles_generated,
                                fragment_tile_bytes_generated,
                                fragment_tlm_tparts);
    }

  // Set up input image files, recovering dimensions and precision information
  // from them where we can.  The code below looks a little complex, only
  // because we want to allow for multi-component transforms, as defined in
  // JPEG2000 Part 2.  A multi-component transform is being used if the
  // `Mcomponents' attribute is defined and greater than 0.  In this case,
  // `Mcomponents' identifies the set of image components that will be
  // decoded after applying the multi-component tranform to the `Scomponents'
  // codestream components.
  //    During compression, we supply `num_source_components' source components
  // to the `kdu_stripe_compressor' object, where `num_source_components' is
  // allowed to be less than `Mcomponents' if we believe that the
  // multi-component transform network can be inverted (this is done
  // automatically by `kdu_multi_analysis' on top of which
  // `kdc_flow_control' is built) to produce the `Scomponents' codestream
  // components from the `num_source_components' supplied source components.
  // These source components correspond to the initial `num_source_components'
  // components reconstructed by the decompressor, out of the total
  // `Mcomponents'.  This is why the code below involves three different
  // component counts (`m_components', `c_components' and `num_components').
  //    For Part-1 codestreams, `Mcomponents' is 0 and `num_source_components'
  // and `c_components' are identical.  In this case, `Scomponents' can be
  // derived simply by counting the components supplied by the source files,
  // and `Ssigned' and `Sprecision' can be set based on the source file
  // headers (except for raw files).
  //    For Part-2 codestreams, `Mcomponents' is greater than 0 and
  // `Scomponents' must be explicitly set by the application (or by parsing the
  // command line).  In this case, `Msigned' and `Mprecision' can be set, based
  // on the source file headers, but `Ssigned' and `Sprecision' must be set by
  // the application (or by parsing the command line).  If you have
  // `Mcomponents' > 0 and no defined value for `Scomponents', the default
  // `Scomponents' value is set to `num_source_components' (i.e., to the
  // number of components found in the input files).
  int c;
  int c_components=0; // Will become number of codestream components
  int m_components=0;  // From an existing `Mcomponents' attribute, if any
  input_siz_ref->get(Mcomponents,0,0,m_components);

  // Initialize component dimensions/precision from `input_siz_ref'
  kdu_image_dims idims; // To exchange component dimensions with `kdu_image_in'
  const char *signed_key = (m_components>0)?Msigned:Ssigned;
  const char *precision_key = (m_components>0)?Mprecision:Sprecision;
  int siz_rows=-1, siz_cols=-1, siz_precision=-1, siz_signed=-1;
  for (c=0; input_siz_ref->get(Sdims,c,0,siz_rows,false,false) ||
            input_siz_ref->get(precision_key,c,0,siz_precision,false,false) ||
            input_siz_ref->get(signed_key,c,0,siz_signed,false,false); c++)
    { // Scan components so long as something is explicitly available
      input_siz_ref->get(Sdims,c,0,siz_rows);
      input_siz_ref->get(Sdims,c,1,siz_cols);
      input_siz_ref->get(precision_key,c,0,siz_precision);
      input_siz_ref->get(signed_key,c,0,siz_signed);
      if ((siz_rows < 0) || (siz_cols < 0) ||
          (siz_precision < 0) || (siz_signed < 0))
        break; // Insufficient information to create a complete record for
               // even one component
      idims.add_component(siz_rows,siz_cols,siz_precision,(siz_signed!=0));
    }
  parse_forced_precisions(args,idims);

  // Open images
  int num_source_components = 0; // Derived from source files
  kdu_rgb8_palette palette; // To support palettized imagery.
  kdc_file_binding *iscan;
  bool extra_flip=false;
  for (iscan=inputs; iscan != NULL; iscan=iscan->next)
    {
      int i;
      bool flip;

      i = iscan->first_comp_idx = num_source_components;
      if ((iscan->next != NULL) && ((i+1) >= idims.get_num_components()))
        idims.append_component(); // This is relevant only for raw files where
          // Sprecision/Mprecision values supplied on the command line are
          // extrapolated and used to initialize the raw file reader; if we
          // do not explicitly invoke `append_component' here, the precision
          // information will not necessarily be extrapolated before the
          // file reader overwrites it in processing a `-fprec' forcing
          // precision.
        
      if (!iscan->cropping.is_empty())
        do {
          idims.set_cropping(iscan->cropping.pos.y,iscan->cropping.pos.x,
                             iscan->cropping.size.y,iscan->cropping.size.x,i);
          i++;
        } while (i < idims.get_num_components());
      iscan->reader =
        kdu_image_in(iscan->fname,idims,num_source_components,flip,
                     ((no_palette || !(is_jp2||is_jpx))?NULL:(&palette)),
                     iscan->offset,quiet);
      iscan->num_components = num_source_components - iscan->first_comp_idx;
      if (iscan == inputs)
        extra_flip = flip;
      if (extra_flip != flip)
        { kdu_error e; e << "Cannot mix input file types which have different "
          "vertical ordering conventions (i.e., top-to-bottom and "
          "bottom-to-top)."; }
      int crop_y, crop_x, crop_height, crop_width;
      for (i=iscan->first_comp_idx; i < num_source_components; i++)
        if (idims.get_cropping(crop_y,crop_x,crop_height,crop_width,i) &&
            ((idims.get_width(i) != crop_width) ||
             (idims.get_height(i) != crop_height)))
          { kdu_error e; e << "Cropping requested for image component " << i <<
            " is not supported by the relevant image file reader at this "
            "time.  Try using a different image format (uncompressed "
            "TIFF files are likely to be best supported)."; }
    }
  if (extra_flip)
    vflip = !vflip;

  // Transfer dimension information back to `codestream_siz' object
  assert(num_source_components <= idims.get_num_components());
  if (!codestream_siz_ref->get(Scomponents,0,0,c_components))
    codestream_siz_ref->set(Scomponents,0,0,
                            c_components=num_source_components);
  for (c=0; c < num_source_components; c++)
    {
      if (!using_fragments)
        {
          codestream_siz_ref->set(Sdims,c,0,idims.get_height(c));
          codestream_siz_ref->set(Sdims,c,1,idims.get_width(c));
        }
      codestream_siz_ref->set(precision_key,c,0,idims.get_bit_depth(c));
      codestream_siz_ref->set(signed_key,c,0,idims.get_signed(c));
    }

  // Complete SIZ information and initialize JP2/JPX boxes
  ((kdu_params *) codestream_siz_ref)->finalize(); // Access virtual function
  if (transpose)
    {
      if (using_fragments)
        { kdu_error e; e << "You cannot compress transposed imagery "
          "(rotated by an odd multiple of 90 degrees) in fragments.  You "
          "could proceed by dropping the `-frag' argument."; }
      siz_scratch.copy_from(codestream_siz_ref,-1,-1,-1,0,0,true,false,false);
      codestream_siz_ref = &siz_scratch;
    }

  char *extra_jp2_box_files = NULL;
  if (jp2_ultimate_tgt.exists())
    {
      int num_available_comps = ((m_components>0)?m_components:c_components);
      extra_jp2_box_files =
        set_jp2_attributes(jp2_family_dimensions,jp2_family_palette,
                           jp2_family_resolution,jp2_family_channels,
                           jp2_family_colour,codestream_siz_ref,palette,
                           num_available_comps,args,jpx_layer,idims,transpose);
      if (num_jpx_layers != 1)
        create_extra_jpx_layers(jpx_out,jpx_layer,num_jpx_layers,
                                num_available_comps);
    }

  // Construct the `kdu_codestream' object and parse all remaining arguments.
  kdu_codestream codestream;
  if (using_fragments)
    codestream.create(codestream_siz_ref,output,&fragment_region,
                      fragment_tiles_generated,fragment_tile_bytes_generated);
  else
    codestream.create(codestream_siz_ref,output);
  for (string=args.get_first(); string != NULL; )
    if (*string == '-')
      { args.advance(false); string = args.advance(false); }
    else
      string = args.advance(codestream.access_siz()->parse_string(string));
  while (args.find("-com") != NULL)
    {
      if ((string = args.advance()) == NULL)
        { kdu_error e; e << "The \"-com\" argument must be followed by a "
          "string parameter."; }
      codestream.add_comment() << string;
      args.advance();
    }
  if (jp2_ultimate_tgt.exists())
    set_jp2_coding_defaults(jp2_family_palette,jp2_family_colour,
                            codestream.access_siz());

  int num_layer_specs = 0;
  kdu_long *layer_bytes =
    assign_layer_bytes(args,codestream,num_layer_specs);
  kdu_uint16 *layer_thresholds =
    assign_layer_thresholds(args,num_layer_specs);
  if ((num_layer_specs > 0) && allow_rate_prediction &&
      (layer_bytes[num_layer_specs-1] > 0))
    codestream.set_max_bytes(layer_bytes[num_layer_specs-1]);
  if ((num_layer_specs > 0) && allow_rate_prediction &&
      (layer_thresholds[num_layer_specs-1] > 0))
    codestream.set_min_slope_threshold(layer_thresholds[num_layer_specs-1]);
  if ((num_layer_specs < 2) && !quiet)
    pretty_cout << "Note:\n\tIf you want quality scalability, you should "
                   "generate multiple layers with `-rate' or by using "
                   "the \"Clayers\" option.\n";
  if ((c_components >= 3) && (m_components == 0) && (!no_weights))
    set_default_colour_weights(codestream.access_siz(),quiet);

  codestream.access_siz()->finalize_all();
  if (jp2_family_dimensions.exists())
    jp2_family_dimensions.finalize_compatibility(codestream.access_siz());

  kdu_message_formatter *formatted_recorder = NULL;;
  kdu_stream_message recorder(record_stream);
  if (record_stream != NULL)
    {
      formatted_recorder = new kdu_message_formatter(&recorder);
      codestream.set_textualization(formatted_recorder);
    }
  if (cpu_iterations >= 0)
    codestream.collect_timing_stats(cpu_iterations);
  codestream.change_appearance(transpose,vflip,hflip);
  kdu_roi_image *roi_source = create_roi_source(codestream,args);
  if (args.show_unrecognized(pretty_cout) != 0)
    { kdu_error e; e << "There were unrecognized command line arguments!"; }

  // Check that fragments are consistently using TLM marker segments, to
  // help users avoid accidental misuse of fragments.
  if (using_fragments)
    {
      kdu_params *org = codestream.access_siz()->access_cluster(ORG_params);
      int org_tlm_tparts = 0;
      if (org != NULL)
        org->get(ORGgen_tlm,0,0,org_tlm_tparts);
      if (first_fragment)
        { assert(fragment_tlm_tparts==0); fragment_tlm_tparts=org_tlm_tparts; }
      else if (fragment_tlm_tparts != org_tlm_tparts)
        { kdu_error e; e << "You are using the `ORGgen_tlm' parameter "
          "attribute inconsistently between generating different "
          "codestream fragments.  This will generally result in the "
          "generation of something unpredictable and non-compliant."; }
    }

  // Write JP2/JPX headers, if required
  if (jpx_out.exists())
    jpx_out.write_headers();
  else if (jp2_ultimate_tgt.exists())
    jp2_out.write_header();
  if (jp2_ultimate_tgt.exists())
    {
      write_extra_jp2_boxes(&jp2_ultimate_tgt,
                            extra_jp2_box_files,idims);
      if (extra_jp2_box_files != NULL)
        delete[] extra_jp2_box_files;
    }
  if (jpx_out.exists())
    {
      jp2_output_box *out_box = jpx_stream.open_stream();
      if (using_fragments)
        out_box->set_rubber_length();
      else
        out_box->write_header_last();
    }
  else if (jp2_ultimate_tgt.exists())
    jp2_out.open_codestream(true);

  // Now we are ready for sample data processing.
  kdu_dims tile_indices; codestream.get_valid_tiles(tile_indices);
  kdu_long sample_processing_bytes=0;
  if (num_threads == 0)
    {
      int dwt_stripe_height = 1;
      if (double_buffering_height > 0)
        dwt_stripe_height = double_buffering_height;
      sample_processing_bytes =
        compress_single_threaded(codestream,tile_indices,inputs,roi_source,
                                 layer_bytes,num_layer_specs,layer_thresholds,
                                 !no_info,rate_tolerance,
                                 allow_shorts,flush_period,dwt_stripe_height,
                                 progress_interval);
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
        compress_multi_threaded(codestream,tile_indices,inputs,roi_source,
                                layer_bytes,num_layer_specs,layer_thresholds,
                                !no_info,rate_tolerance,
                                allow_shorts,flush_period,num_threads,
                                dwt_double_buffering,dwt_stripe_height,
                                progress_interval);
    }

  // Finalize the compressed output.
  bool last_fragment = true;
  if (using_fragments)
    {
      fragment_tiles_generated += (int) tile_indices.area();
      fragment_tile_bytes_generated += codestream.get_total_bytes(true);
      last_fragment = codestream.is_last_fragment();
      if (!last_fragment)
        save_fragment_state(output,fragment_tiles_generated,
                            fragment_tile_bytes_generated,
                            fragment_tlm_tparts);
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
          pretty_cout << "Block encoding CPU time (estimated) ";
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
      double bpp_dims = (double) get_bpp_dims(codestream);
      pretty_cout << "\nGenerated " << codestream.get_num_tparts()
                  << " tile-part(s) for a total of "
                  << tile_indices.area() << " tile(s).\n";
      pretty_cout << "Code-stream bytes (excluding any file "
                     "format) = " << codestream.get_total_bytes()
                  << " = "
                  << 8.0*codestream.get_total_bytes() / bpp_dims
                  << " bits/pel.\n";

      int layer_idx;
      pretty_cout << "Layer bit-rates (possibly inexact if tiles are "
        "divided across tile-parts):\n\t\t";
      for (layer_idx=0; layer_idx < num_layer_specs; layer_idx++)
        {
          if (layer_idx > 0)
            pretty_cout << ", ";
          pretty_cout << 8.0 * layer_bytes[layer_idx] / bpp_dims;
        }
      pretty_cout << "\n";
      pretty_cout << "Layer thresholds:\n\t\t";
      for (layer_idx=0; layer_idx < num_layer_specs; layer_idx++)
        {
          if (layer_idx > 0)
            pretty_cout << ", ";
          pretty_cout << (int)(layer_thresholds[layer_idx]);
        }
      pretty_cout << "\n";

      if (using_fragments && !last_fragment)
        pretty_cout << "To finish the codestream, you still have more "
                       "fragments to compress, but you can decompress "
                       "or show the existing codestream as it is if you "
                       "like.\n";
      if (using_fragments && last_fragment)
        pretty_cout << "All fragments successfully compressed.\n";
      if (num_threads == 0)
        pretty_cout << "Processed using the single-threaded environment "
          "(see `-num_threads')\n";
      else
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          << "    " << num_threads << " parallel threads of execution "
          "(see `-num_threads')\n";
    }

  delete[] layer_bytes;
  delete[] layer_thresholds;
  codestream.destroy();
  output->close();
  jpx_out.close();
  if (jp2_ultimate_tgt.exists())
    jp2_ultimate_tgt.close();
  if (roi_source != NULL)
    delete roi_source;
  if (record_stream != NULL)
    {
      delete formatted_recorder;
      delete record_stream;
    }
  delete inputs;
  return 0;
}
