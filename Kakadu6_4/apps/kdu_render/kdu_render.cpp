/*****************************************************************************/
// File: kdu_render.cpp [scope = APPS/RENDER]
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
   The purpose of this demo application is to provide a collection of examples
showing how the power `kdu_region_decompressor' and `kdu_region_compositor'
objects can be used to decompress and render a JPEG2000 compressed data
source to a memory buffer -- in this case, the memory buffer is written
directly to a BMP file.  The examples can easily be modified to decompress
an arbitrary region, scale the results by an arbitrary amount (not just
integers), add overlay imagery and much much more.  From version 6.3, a new
example has been added here to show how to render an arbitrarily complex
image composition at floating-point precision (as opposed to 8-bit/sample
precision).
******************************************************************************/

// System includes
#include <string.h>
#include <math.h>
#include <assert.h>
#include <iostream>
// Kakadu core includes
#include "kdu_arch.h"
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
// Application includes
#include "kdu_args.h"
#include "kdu_file_io.h"
#include "jp2.h"
#include "kdu_region_decompressor.h"
#include "kdu_region_compositor.h" // Required for demos 3 and 4

//+++++++++++++++++++++++++++++++++++++++++++++++++++
// The actual demo functions appear at the end of this
// file.  Here are the stubs.  For simplicity here, each
// function takes file names for its input and output,
// but Kakadu is vastly more general than this.

static void render_demo_1(const char *in, const char *out, float scale);
static void render_demo_2(const char *in, const char *out, float scale);
static void render_demo_3(const char *in, const char *out, float scale,
                          float overlay_intensity);
static void render_demo_4(const char *in, const char *out, float scale,
                          float overlay_intensity);
static void render_demo_5(const char *in, const char *out, float scale,
                          float overlay_intensity);
static void render_demo_6(const char *in, const char *out, float scale,
                          float overlay_intensity);
static void render_demo_cpu(const char *in, int num_threads, float scale);

//+++++++++++++++++++++++++++++++++++++++++++++++++++
// While not strictly necessary, just about any serious Kakadu application
// will configure error and warning message services.  Here is an example
// of how to do it, which includes support for throwing exceptions when
// errors occur -- if you don't do this, the program will exit when an 
// error occurs.

/* ========================================================================= */
/*                         Set up messaging services                         */
/* ========================================================================= */

class kdu_stream_message : public kdu_message {
  public: // Member classes
    kdu_stream_message(std::ostream *stream, bool throw_exc)
      { this->stream = stream; this->throw_exc = throw_exc; }
    void put_text(const char *string)
      { (*stream) << string; }
    void flush(bool end_of_message=false)
      {
        stream->flush();
        if (end_of_message && throw_exc)
          throw KDU_ERROR_EXCEPTION;
      }
  private: // Data
    std::ostream *stream;
    bool throw_exc;
  };

static kdu_stream_message cout_message(&std::cout,false);
static kdu_stream_message cerr_message(&std::cerr,true);
static kdu_message_formatter pretty_cout(&cout_message);
static kdu_message_formatter pretty_cerr(&cerr_message);

//+++++++++++++++++++++++++++++++++++++++++++++++++++
// This stuff is just to convert floating point scale
// factors into rational numbers with integer numerator
// and denominator, for use with the `kdu_region_decompressor'
// based demos.  In real applications, starting with a
// rational scale factor often makes more sense than
// starting with a floating point value, because we might
// want to fit the image to a known scaled size.

/* ========================================================================= */
/*                   Scale Factor to Rational Conversion                     */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                   find_rational_scale                              */
/*****************************************************************************/

static void
  find_rational_scale(float scale, kdu_coords &numerator,
                      kdu_coords &denominator, double min_safe_scale_prod,
                      double max_safe_scale_x, double max_safe_scale_y)
  /* The min/max safe scaling factors supplied here are derived from the
     `kdu_region_decompressor' object.  In practice, the maximum safe scaling
     factor will be vastly larger than the point at which you will run out
     of memory in these simple demo apps, because they try to render the
     entire image to a buffer.  A real application should ideally use these
     to limit scaling for the case in which a region of an image is
     being rendered at a massively expanded or reduced scale, to avoid the
     possibility of numerical problems inside `kdu_region_decompressor' -- the
     bounds are, in practice, far beyond anything you are likely to
     encounter in the real world. */
{
  if (scale > (float) max_safe_scale_x)
    scale = (float) max_safe_scale_x;
  if (scale > (float) max_safe_scale_y)
    scale = (float) max_safe_scale_y;
  if (scale*scale < (float) min_safe_scale_prod)
    scale = (float) sqrt(min_safe_scale_prod);

  int num, den=1;
  if (scale > (float)(1<<30))
    scale = (float)(1<<30); // Avoid ridiculous factors
  bool close_enough = true;
  do {
      if (!close_enough)
        den <<= 1;
      num = (int)(scale * den + 0.5F);
      float ratio = num / (scale * den);
      close_enough = (ratio > 0.9999F) && (ratio < 1.0001F);
    } while ((!close_enough) && (den < (1<<28)) && (num < (1<<28)));
  numerator.x = numerator.y = num;
  denominator.x = denominator.y = den;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++
// This stuff is only to make the demo write output
// files (BMP files) for ease of testing via the command
// line.  It has nothing to do with Kakadu proper.

/* ========================================================================= */
/*                            BMP Writing Stuff                              */
/* ========================================================================= */

/*****************************************************************************/
/*                               bmp_header                                  */
/*****************************************************************************/

struct bmp_header {
    kdu_uint32 size; // Size of this structure: must be 40
    kdu_int32 width; // Image width
    kdu_int32 height; // Image height; -ve means top to bottom.
    kdu_uint32 planes_bits; // Planes in 16 LSB's (must be 1); bits in 16 MSB's
    kdu_uint32 compression; // Only accept 0 here (uncompressed RGB data)
    kdu_uint32 image_size; // Can be 0
    kdu_int32 xpels_per_metre; // We ignore these
    kdu_int32 ypels_per_metre; // We ignore these
    kdu_uint32 num_colours_used; // Entries in colour table; 0 = use default
    kdu_uint32 num_colours_important; // 0 means all colours are important.
  };
  /* Notes:
        This header structure must be preceded by a 14 byte field, whose
     first 2 bytes contain the string, "BM", and whose next 4 bytes contain
     the length of the entire file.  The next 4 bytes must be 0. The final
     4 bytes provides an offset from the start of the file to the first byte
     of image sample data.
        If the bit_count is 1, 4 or 8, the structure must be followed by
     a colour lookup table, with 4 bytes per entry, the first 3 of which
     identify the blue, green and red intensities, respectively. */

/*****************************************************************************/
/* STATIC                       to_little_endian                             */
/*****************************************************************************/

static void
  to_little_endian(kdu_int32 * words, int num_words)
  /* Used to convert the BMP header structure to a little-endian word
     organization for platforms which use the big-endian convention. */
{
  kdu_int32 test = 1;
  kdu_byte *first_byte = (kdu_byte *) &test;
  if (*first_byte)
    return; // Machine uses little-endian architecture already.
  kdu_int32 tmp;
  for (; num_words--; words++)
    {
      tmp = *words;
      *words = ((tmp >> 24) & 0x000000FF) +
               ((tmp >> 8)  & 0x0000FF00) +
               ((tmp << 8)  & 0x00FF0000) +
               ((tmp << 24) & 0xFF000000);
    }
}

/*****************************************************************************/
/* STATIC                       write_bmp_file                               */
/*****************************************************************************/

static void
  write_bmp_file(const char *fname, kdu_coords size, kdu_uint32 buffer[],
                 int buffer_row_gap)
{
  FILE *file = fopen(fname,"wb");
  if (file == NULL)
    { kdu_error e; e << "Unable to open output file, \"" << fname << "\"."; }
  kdu_byte magic[14];
  bmp_header header;
  int header_bytes = 14+sizeof(header);
  assert(header_bytes == 54);
  int line_bytes = size.x*4; line_bytes += (4-line_bytes)&3;
  int file_bytes = line_bytes * size.y + header_bytes;
  magic[0] = 'B'; magic[1] = 'M';
  magic[2] = (kdu_byte) file_bytes;
  magic[3] = (kdu_byte)(file_bytes>>8);
  magic[4] = (kdu_byte)(file_bytes>>16);
  magic[5] = (kdu_byte)(file_bytes>>24);
  magic[6] = magic[7] = magic[8] = magic[9] = 0;
  magic[10] = (kdu_byte) header_bytes;
  magic[11] = (kdu_byte)(header_bytes>>8);
  magic[12] = (kdu_byte)(header_bytes>>16);
  magic[13] = (kdu_byte)(header_bytes>>24);
  header.size = 40;
  header.width = size.x;
  header.height = size.y;
  header.planes_bits = 1; // Set `planes'=1 (mandatory)
  header.planes_bits |= (32 << 16); // Set bits-per-pel to upper word
  header.compression = 0;
  header.image_size = 0;
  header.xpels_per_metre = header.ypels_per_metre = 0;
  header.num_colours_used = header.num_colours_important = 0;
  to_little_endian((kdu_int32 *) &header,10);
  fwrite(magic,1,14,file);
  fwrite(&header,1,40,file);
  for (int r=size.y; r > 0; r--, buffer += buffer_row_gap)
    if (fwrite(buffer,1,(size_t) line_bytes,file) != (size_t) line_bytes)
      { kdu_error e;
        e << "Unable to write to output file.  Device may be full."; }
  fclose(file);
}

/*****************************************************************************/
/* STATIC                convert_four_floats_to_argb32                       */
/*****************************************************************************/

static kdu_uint32 *
  convert_four_floats_to_argb32(float *in_buf, int row_gap, kdu_coords size)
 /* Converts `in_buf' in-place from pixels consisting of 4 floats each to
    pixels consisting of a single 32-bit integer each.  The row gap for
    both structures is measured in multiples of 32-bit words (floating-point
    samples for the input buffer and pixels for the output buffer).  The
    in-place conversion is performed line-by-line, so that each line's
    active size is reduced by a quarter, while the line-gap remains
    constant. */
{
  kdu_uint32 *dpp, *dp, *out_buf = (kdu_uint32 *) in_buf;
  float *spp, *sp;
  int m, n;
  for (dpp=out_buf, spp=in_buf,
       m=size.y; m > 0; m--, spp+=row_gap, dpp+=row_gap)
    for (dp=dpp, sp=spp, n=size.x; n > 0; n--, sp+=4, dp++)
      {
        kdu_uint32 val = (kdu_uint32)(sp[0] * 255.0F + 0.5F);
        val = (val << 8) + (kdu_uint32)(sp[1] * 255.0F + 0.5F);
        val = (val << 8) + (kdu_uint32)(sp[2] * 255.0F + 0.5F);
        val = (val << 8) + (kdu_uint32)(sp[3] * 255.0F + 0.5F);
        *dp = val;
      }
  return out_buf;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++
// This stuff is just to make the command-line
// interface look nice.  It has nothing to do with
// Kakadu proper.

/* ========================================================================= */
/*                           Usage Statement Stuff                           */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        print_version                               */
/*****************************************************************************/

static void
  print_version()
{
  kdu_message_formatter out(&cout_message);
  out.start_message();
  out << "This is Kakadu's \"kdu_render\" application.\n";
  out << "\tCompiled against the Kakadu core system, version "
      << KDU_CORE_VERSION << "\n";
  out << "\tCurrent core system version is "
      << kdu_get_core_version() << "\n";
  out.flush(true);
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
    out << "\tThe range of JPEG2000 family files which are accepted depends "
           "upon the particular code fragment which is executed to do the "
           "decompression and rendering.  This is selected by the `-demo' "
           "argument, but the default should be to use the most sophisticated "
           "of the demonstrations.  Currently, demo 4 accepts any raw "
           "codestream, JP2 file, JPX file or MJ2 file, performing "
           "compositions (possibly from multiple codestreams), "
           "colour space conversions, and inverting complex multi-component "
           "transforms as required.\n";
  out << "-o <bmp output file>\n";
  if (comprehensive)
    out << "\tThe only output image file format produced by this simple "
           "demonstration program is BMP.  In fact, for added simplicity, "
           "the output file will always have a 32 bpp representation, even "
           "if the original image was monochrome.\n";
  out << "-scale <rendering scale factor>\n";
  if (comprehensive)
    out << "\tScales the dimensions of the rendered result.  By default, "
           "rendering will be performed at a scale of 1.0, but this "
           "may produce an image which is too large to fit in a single "
           "memory buffer -- in that event you should get an appropriate "
           "error message and may wish to try rendering at a smaller scale.  "
           "The high level objects demonstrated by this application are "
           "intended primarily to be used in GUI applications, rendering "
           "regions of interest to a screen buffer, rather than entire "
           "images.  If you want to decompress a massive image in one hit "
           "you should use one of the \"kdu_expand\" or "
           "\"kdu_buffered_expand\" demo applications, which do not "
           "allocate image sized buffers.  Alternatively, you may modify "
           "one of the rendering demonstrations in this application to "
           "render the image progressively in segments.  Our goal here is "
           "simplicity of demonstration -- each of the demos is implemented "
           "using only 30 to 60 lines of code in total.\n";
  out << "-demo <which demo to run>\n";
  if (comprehensive)
    out << "\tCurrently takes an integer in the range 1 through 6, "
           "corresponding to code fragments which demonstrate different "
           "ways to very simply get up and running with decompression and "
           "rendering of JPEG2000 files.  The current default is to use "
           "demo 4.  All of the demos essentially do the same thing, but "
           "they take a progressively wider range of input files, up to "
           "demo 4 (see \"-i\" for what it can accept).  Demo 5 is the "
           "same as demo 4, but it runs a multi-threaded processing "
           "environment with two threads of execution.  Finally, demo 6 "
           "is the same as demo 4, except that it runs Kakadu's powerful "
           "region compositor object with floating-point buffers, rather "
           "than 8-bit output samples, demonstraing how to preserve "
           "all available precision, even with complex multi-layer "
           "compositions.  It is expected that you will use this argument "
           "in conjunction with a debugger to step through different "
           "implementation strategies and see how they work.\n";
  out << "-overlays <blending intensity>\n";
  if (comprehensive)
    out << "\tThis option causes region-of-interest overlays to be painted "
           "onto the rendered image, using the same colour scheme as that "
           "employed by the \"kdu_macshow\" and \"kdu_winshow\" apps.  The "
           "option has no effect at all unless the underlying data source "
           "is a JPX file which contains some region-of-interest metadata "
           "and the demonstration code fragment being used (as controlled "
           "via `-demo') is at least \"demo 3\", which is the first one "
           "that uses the `kdu_region_compositor' object.  The argument "
           "takes a single floating-point parameter which modulates the "
           "intensity with which the overlay information is blended onto "
           "the image.  This <blending intensity> may be greater than 1 "
           "if desired; it may also be -ve, in which case the overlay "
           "colours are inverted.  In an interactive application, the "
           "blending intensity may be modulated at will and the underlying "
           "machinery very efficiently generates the effect, allowing for "
           "flashing overlays and related effects.\n";
  out << "-cpu [<num_threads>]\n";
  if (comprehensive)
    out << "\tIf this option is supplied, neither the `-demo' nor the `-o' "
           "arguments should appear.  In this case, the rendering approach "
           "is almost identical to that associated with `-demo 2', which "
           "is based on direct invocation of the `kdu_region_decompressor' "
           "object, except that a multi-threaded processing environment may "
           "be used to obtain the maximum processing speed.  The only other "
           "difference in this case is that the image is decompressed "
           "incrementally into a small temporary memory buffer, rather than "
           "a single whole-image memory buffer.  This is exactly what you "
           "would probably do in a real application involving huge images.  "
           "It allows you to measure the CPU time required to render a huge "
           "image, without including the time taken to actually write the "
           "image to an output file.  In fact, you cannot supply an output "
           "file with this argument.  If the `num_threads' parameter is 0, "
           "Kakadu's single-threaded processing environment is used; if "
           "`num_threads' is 1, the multi-threaded processing environment is "
           "used with exactly one processing thread -- this is still single "
           "threaded, but involves a different implicit processing order and "
           "some overhead.  Normally, you would set `num_threads' to 0 or "
           "the number of processors in your system.  If you omit the "
           "parameter altogether, the number of threads will be set equal "
           "to the application's best guess as to the number of CPU's in "
           "your system (usually correct).\n";
  out << "-version -- print core system version I was compiled against.\n";
  out << "-v -- abbreviation of `-version'\n";
  out << "-usage -- print a comprehensive usage statement.\n";
  out << "-u -- print a brief usage statement.\"\n\n";
  out.flush();
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++
// A simple main() function, which just reads the
// command-line arguments, installs the error/warning
// handlers and runs the selected demo function.

/*****************************************************************************/
/*                                   main                                    */
/*****************************************************************************/

int main(int argc, char *argv[])
{
  kdu_customize_warnings(&pretty_cout); // Deliver warnings to stdout.
  kdu_customize_errors(&pretty_cerr); // Deliver errors to stderr + throw exc
  kdu_args args(argc,argv); // Ingest args


  // Collect arguments.

  if ((args.get_first() == NULL) || (args.find("-u") != NULL))
    { print_usage(args.get_prog_name());  return 0; }
  if (args.find("-usage") != NULL)
    { print_usage(args.get_prog_name(),true);  return 0; }
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    { print_version();  return 0; }

  float scale=1.0F;
  float ovl_intensity=0.0F; // In the case of 0, no overlays are used
  int failure_code = 0; // No failure
  char *in_fname=NULL, *out_fname=NULL;
  try { // Any errors generated inside here will produce exceptions
      int num_threads = -1;
      int demo = 4;
      if (args.find("-cpu") != NULL)
        {
          demo = 0;
          const char *string = args.advance();
          if ((string != NULL) && (*string != '-') &&
              (sscanf(string,"%d",&num_threads) == 1))
            args.advance();
          else
            num_threads = kdu_get_num_processors();
        }

      if (args.find("-demo") != NULL)
        {
          if (demo == 0)
            { kdu_error e; e << "The `-demo' argument may not be used in "
              "conjunction with `-cpu'."; }
          const char *string = args.advance();
          if ((string == NULL) || (sscanf(string,"%d",&demo) != 1) ||
              (demo < 1) || (demo > 6))
            { kdu_error e; e<<"`-demo' switch needs an integer from 1 to 6."; }
          args.advance();
        }

      if (args.find("-i") != NULL)
        {
          const char *string = args.advance();
          if (string == NULL)
            { kdu_error e; e << "Must supply a name with the `-i' switch."; }
          in_fname = new char[strlen(string)+1];
          strcpy(in_fname,string);
          args.advance();
        }
      else
        { kdu_error e; e << "Must supply an input file via the `-i' switch."; }

      if (args.find("-o") != NULL)
        {
          if (demo == 0)
            { kdu_error e; e << "The `-o' argument may not be used in "
              "conjunction with `-cpu'."; }
          const char *string = args.advance();
          if (string == NULL)
            { kdu_error e; e << "Must supply a name with the `-o' switch."; }
          out_fname = new char[strlen(string)+1];
          strcpy(out_fname,string);
          args.advance();
        }
      else if (demo > 0)
        { kdu_error e; e<<"Must supply an output file via the `-o' switch."; }
    
      if (args.find("-scale") != NULL)
        {
          const char *string = args.advance();
          if ((string == NULL) || (sscanf(string,"%f",&scale) == 0) ||
              (scale <= 0.0F))
            { kdu_error e; e << "Must supply a positive scale factor via the "
              "`-scale' switch."; }
          args.advance();
        }
    
      if (args.find("-overlays") != NULL)
        { 
          const char *string = args.advance();
          if ((string==NULL) || (sscanf(string,"%f",&ovl_intensity)==0))
            { kdu_error e; e << "Must supply a real-valued overlay blending "
              "intensity value via the `-overlays' argument."; }
          args.advance();
        }

      if (args.show_unrecognized(pretty_cout) != 0)
        { kdu_error e; e<<"There were unrecognized command line arguments!"; }

      switch (demo) {
        case 0: render_demo_cpu(in_fname,num_threads,scale); break;
        case 1: render_demo_1(in_fname,out_fname,scale);  break;
        case 2: render_demo_2(in_fname,out_fname,scale);  break;
        case 3: render_demo_3(in_fname,out_fname,scale,ovl_intensity);  break;
        case 4: render_demo_4(in_fname,out_fname,scale,ovl_intensity);  break;
        case 5: render_demo_5(in_fname,out_fname,scale,ovl_intensity);  break;
        case 6: render_demo_6(in_fname,out_fname,scale,ovl_intensity);  break;
        default: assert(0); // Should not be possible
        }
    }
  catch (kdu_exception)
    {
      failure_code = 1;
    }
  catch (std::bad_alloc &)
    {
      failure_code = 2;
      pretty_cerr << "Insufficient memory to buffer whole decompressed "
      "image.  Consider supplying a smaller value for the \"-scale\" "
      "argument on the command-line.  Alternatively, use \"kdu_expand\" "
      "or \"kdu_buffered_expand\" to decompress the image incrementally, "
      "or build your implementation to use \"kdu_region_decompressor\" "
      "or \"kdu_region_compositor\" in an incremental fashion -- "
      "region-by-region.\n";
    }
  catch (...)
    {
      failure_code = 3;
    }

  if (in_fname != NULL)
    delete[] in_fname;
  if (out_fname != NULL)
    delete[] out_fname;
  return failure_code;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++
// Finally, the demo functions themselves.  Note, each
// of these demo functions uses a memory buffer into
// which it decompresses the whole image.  While this
// sort of thing might seem convenient, JPEG2000 really
// comes into its own when dealing with large images.
// In this setting, interactive decompression of
// selected pieces of the image is the interesting
// thing to do.  The code fragments here do not do
// this, but they all use objects which are designed to
// decompress/render arbitrary regions efficiently.

/* ========================================================================= */
/*                        DEMONSTRATION FUNCTIONS                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                  render_demo_1 (raw only)                          */
/*****************************************************************************/

static void
  render_demo_1(const char *in_fname, const char *out_fname, float scale)
  /* This function demonstrates decompression of a raw codestream
     via the `kdu_region_decompressor' object.
       Using the `kdu_region_decompressor' object is a little more complex
     and provides less features than the higher level `kdu_region_compositor'
     object -- the latter uses `kdu_region_decompressor'.  However,
     `kdu_region_decompressor' provides you with more choices in regard
     to the bit-depth at which samples are rendered and the organization
     of memory buffers.  To understand this, consult the descriptions of
     the various overloaded `kdu_region_decompressor::process' functions. */
{
  // Open a suitable compressed data source object
  kdu_simple_file_source input;
  input.open(in_fname);

  // Create the code-stream management machinery for the compressed data
  kdu_codestream codestream;
  codestream.create(&input);
  codestream.change_appearance(false,true,false); // Flip vertically to deal
              // with the fact that BMP files have a bottom-up organization;
              // this way, we can write the buffer without having to flip it.

  // Configure a `kdu_channel_mapping' object for `kdu_region_decompressor'
  kdu_channel_mapping channels;
  channels.configure(codestream);

  // Determine dimensions and create output buffer
  kdu_region_decompressor decompressor;
  kdu_dims image_dims, new_region, incomplete_region;
  kdu_uint32 *buffer = NULL;

  // Start region decompressor and render the image to the buffer
  try { // Catch errors to properly deallocate resources before returning
    do { // Loop around in the odd event that the number of discard
         // levels is found to be too large after processing starts.
      if (!codestream.can_flip(true))
        {
          kdu_warning w; w << "This Part-2 JPEG2000 image can't be vertically "
          "flipped so as to write a BMP file (these are upside down) in "
          "native order.  Of course we can write it backwards, but this is "
          "a simple demo.  To make sure you get at least some output the "
          "image will be rendered without flipping -- and hence written "
          "upside down.";
          codestream.change_appearance(false,false,false);
        }
      if (buffer != NULL)
        {
          kdu_warning w; w << "Discovered a tile with fewer DWT levels part "
          "way through processing; repeating the rendering process with "
          "fewer DWT levels, together with resolution reduction signal "
          "processing.";
          delete[] buffer; buffer = NULL;
        }
      int discard_levs=0, max_discard_levs = codestream.get_min_dwt_levels();
      while ((scale < 0.6F) && (discard_levs < max_discard_levs))
        { scale *= 2.0F; discard_levs++; }
      double min_scale_prod, max_scale_x, max_scale_y;
      decompressor.get_safe_expansion_factors(codestream,&channels,-1,
                                              discard_levs,min_scale_prod,
                                              max_scale_x,max_scale_y);
      kdu_coords scale_num, scale_den;
      find_rational_scale(scale,scale_num,scale_den,
                          min_scale_prod,max_scale_x,max_scale_y);
      image_dims =
        decompressor.get_rendered_image_dims(codestream,&channels,-1,
                                             discard_levs,scale_num,scale_den);
      buffer = new kdu_uint32[(size_t) image_dims.area()];
      decompressor.start(codestream,&channels,-1,discard_levs,INT_MAX,
                         image_dims,scale_num,scale_den);
      incomplete_region=image_dims;
      while (decompressor.process((kdu_int32 *) buffer,image_dims.pos,
                                  image_dims.size.x,256000,0,incomplete_region,
                                  new_region)); // Loop until buffer is filled
      kdu_exception exc=KDU_NULL_EXCEPTION;
      if (!decompressor.finish(&exc))
        kdu_rethrow(exc);
    } while (!incomplete_region.is_empty());
    write_bmp_file(out_fname,image_dims.size,buffer,image_dims.size.x);
  }
  catch (...)
    {
      if (buffer != NULL) delete[] buffer;
      codestream.destroy();
      throw; // Rethrow the exception
    }

  // Cleanup
  delete[] buffer;
  codestream.destroy();
}


/*****************************************************************************/
/* STATIC                 render_demo_2 (raw or JP2)                         */
/*****************************************************************************/

static void
  render_demo_2(const char *in_fname, const char *out_fname, float scale)
  /* This function demonstrates decompression of either a raw codestream
     or a JP2 file, using the the `kdu_region_decompressor' object.
       Using the `kdu_region_decompressor' object is a little more complex
     and provides less features than the higher level `kdu_region_compositor'
     object -- the latter uses `kdu_region_decompressor'.  However,
     `kdu_region_decompressor' provides you with more choices in regard
     to the bit-depth at which samples are rendered and the organization
     of memory buffers.  To understand this, consult the descriptions of
     the various overloaded `kdu_region_decompressor::process' functions. */
{
  // Start by checking whether the input file belongs to the JP2 family of
  // file formats by testing the signature.
  jp2_family_src src;
  jp2_input_box box;
  src.open(in_fname);
  bool is_jp2 = box.open(&src) && (box.get_box_type() == jp2_signature_4cc);
  src.close();

  // Open a suitable compressed data source object
  kdu_compressed_source *input = NULL;
  kdu_simple_file_source file_in;
  jp2_family_src jp2_ultimate_src;
  jp2_source jp2_in;
  if (!is_jp2)
    {
      file_in.open(in_fname);
      input = &file_in;
    }
  else
    {
      jp2_ultimate_src.open(in_fname);
      if (!jp2_in.open(&jp2_ultimate_src))
        { kdu_error e; e << "Supplied input file, \"" << in_fname
                         << "\", does not appear to contain any boxes."; }
      if (!jp2_in.read_header())
        assert(0); // Should not be possible for file-based sources
      input = &jp2_in;
    }

  // Create the code-stream management machinery for the compressed data
  kdu_codestream codestream;
  codestream.create(input);
  codestream.change_appearance(false,true,false); // Flip vertically to deal
              // with the fact that BMP files have a bottom-up organization;
              // this way, we can write the buffer without having to flip it.

  // Configure a `kdu_channel_mapping' object for `kdu_region_decompressor'
  kdu_channel_mapping channels;
  if (jp2_in.exists())
    channels.configure(&jp2_in,false);
  else
    channels.configure(codestream);

  // Determine dimensions and create output buffer
  kdu_region_decompressor decompressor;
  kdu_dims image_dims, new_region, incomplete_region;
  kdu_uint32 *buffer = NULL;
  
  // Start region decompressor and render the image to the buffer
  try { // Catch errors to properly deallocate resources before returning
    do { // Loop around in the odd event that the number of discard
      // levels is found to be too large after processing starts.
      if (!codestream.can_flip(true))
        {
          kdu_warning w; w << "This Part-2 JPEG2000 image can't be vertically "
          "flipped so as to write a BMP file (these are upside down) in "
          "native order.  Of course we can write it backwards, but this is "
          "a simple demo.  To make sure you get at least some output the "
          "image will be rendered without flipping -- and hence written "
          "upside down.";
          codestream.change_appearance(false,false,false);
        }
      if (buffer != NULL)
        {
          kdu_warning w; w << "Discovered a tile with fewer DWT levels part "
          "way through processing; repeating the rendering process with "
          "fewer DWT levels, together with resolution reduction signal "
          "processing.";
          delete[] buffer; buffer = NULL;
        }
      int discard_levs=0, max_discard_levs = codestream.get_min_dwt_levels();
      while ((scale < 0.6F) && (discard_levs < max_discard_levs))
        { scale *= 2.0F; discard_levs++; }
      double min_scale_prod, max_scale_x, max_scale_y;
      decompressor.get_safe_expansion_factors(codestream,&channels,-1,
                                              discard_levs,min_scale_prod,
                                              max_scale_x,max_scale_y);
      kdu_coords scale_num, scale_den;
      find_rational_scale(scale,scale_num,scale_den,
                          min_scale_prod,max_scale_x,max_scale_y);
      image_dims =
        decompressor.get_rendered_image_dims(codestream,&channels,-1,
                                             discard_levs,scale_num,scale_den);
      buffer = new kdu_uint32[(size_t) image_dims.area()];
      decompressor.start(codestream,&channels,-1,discard_levs,INT_MAX,
                         image_dims,scale_num,scale_den);
      incomplete_region=image_dims;
      while (decompressor.process((kdu_int32 *) buffer,image_dims.pos,
                                  image_dims.size.x,256000,0,incomplete_region,
                                  new_region)); // Loop until buffer is filled
      kdu_exception exc=KDU_NULL_EXCEPTION;
      if (!decompressor.finish(&exc))
        kdu_rethrow(exc);
    } while (!incomplete_region.is_empty());
    write_bmp_file(out_fname,image_dims.size,buffer,image_dims.size.x);
  }
  catch (...)
  {
    if (buffer != NULL) delete[] buffer;
    codestream.destroy();
    throw; // Rethrow the exception
  }
  
  // Cleanup
  delete[] buffer;
  codestream.destroy();
}

/*****************************************************************************/
/* STATIC                      configure_overlays                            */
/*****************************************************************************/

static void
  configure_overlays(kdu_region_compositor &compositor, float intensity)
  /* This function shows how to set up region-of-interest metadata overlays
     with the `kdu_region_compositor' object.  It is invoked from within
     demos 3 through 6 if a non-zero overlay intensity is supplied to those
     functions -- see the `-overlays' command-line argument. */
{
  int border_size = 4;
  int num_aux_params = 18;
  kdu_uint32 aux_params[18] =
    {4, 0x20FFFF00, 0x600000FF, 0xA00000FF, 0xA0FFFF00, 0x60FFFF00, 10, \
     2, 0x200000FF, 0x600000FF, 0x80FFFF00, 6, \
     1, 0x300000FF, 0x60FFFF00, 3, \
     0, 0x400000FF };
  // For an explanation of the meaning of the above parameters, either read
  // the interface definition for `kdu_region_compositor::configure_overlays'
  // or refer to the description of the same set of parameters which appears
  // at the start of "kdms_renderer.h" (in the "kdu_macshow" app) or
  // "kdws_renderer.h" (in the "kdu_winshow" app).
  compositor.configure_overlays(true,1,intensity,border_size,
                                jpx_metanode(),0,aux_params,num_aux_params);
}

/*****************************************************************************/
/* STATIC             render_demo_3 (raw, JP2, JPX or MJ2)                   */
/*****************************************************************************/

static void
  render_demo_3(const char *in_fname, const char *out_fname, float scale,
                float overlay_intensity)
  /* This function demonstrates decompression of either a raw codestream
     or a JP2, JPX or MJ2 file, using the the `kdu_region_compositor' object.
     In the case of JPX files, only the first compositing layer is
     decompressed.  In the case of MJ2 files, only one frame is
     decompressed. */
{
  // Open a suitable compressed data source object
  kdu_region_compositor compositor;
  kdu_simple_file_source file_in;
  jp2_family_src src;
  jpx_source jpx_in;
  mj2_source mj2_in;
  src.open(in_fname);
  if (jpx_in.open(&src,true) >= 0)
    { // We have a JP2/JPX-compatible file
      compositor.create(&jpx_in); 
      compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  else if (mj2_in.open(&src,true) >= 0)
    { // We have an MJ2-compatible file
      kdu_uint32 track_idx = mj2_in.get_next_track(0);
      mj2_video_source *track;
      while ((mj2_in.get_track_type(track_idx) != MJ2_TRACK_IS_VIDEO) ||
             ((track=mj2_in.access_video_track(track_idx)) == NULL) ||
             (track->get_num_frames() == 0))
        if ((track_idx = mj2_in.get_next_track(0)) == 0)
          { kdu_error e; e << "MJ2 source contains no video frames at all."; }
      compositor.create(&mj2_in); 
      compositor.add_ilayer((int)(track_idx-1),
                            kdu_dims(),kdu_dims(),false,false,false,0,0);
    }
  else
    { // Incompatible with JP2/JPX or MJ2.  Try opening as raw codestream
      src.close();
      file_in.open(in_fname);
      compositor.create(&file_in);
      compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  
  if (overlay_intensity != 0.0F)
    configure_overlays(compositor,overlay_intensity);

  // Render the image onto the frame buffer created by `kdu_region_compositor'.
  // You can override the buffer allocation method to create frames with the
  // configuration of your choosing.
  kdu_dims new_region, image_dims;
  kdu_compositor_buf *buf=NULL;
  bool vflip=true; // Flip to generate BMP buffer in its native top-down order
  do { // Loop around in the odd event that the number of discard
       // levels is found to be too large after processing starts.
    compositor.set_scale(false,vflip,false,scale);
    if (!compositor.get_total_composition_dims(image_dims))
      { // The following code adjuss geometry and/or scale in response to some
        // extremely unlikely corner cases.
        int code = compositor.check_invalid_scale_code();
        if (code & (KDU_COMPOSITOR_SCALE_TOO_SMALL |
                    KDU_COMPOSITOR_SCALE_TOO_LARGE))
          scale = compositor.find_optimal_scale(kdu_dims(),scale,scale,scale);
        if ((code & KDU_COMPOSITOR_CANNOT_FLIP) && vflip)
          {
            kdu_warning w; w << "This Part-2 JPEG2000 image can't be "
            "vertically flipped so as to write a BMP file (these are upside "
            "down) in native order.  Of course we can write it backwards, "
            "but this is a simple demo.  To make sure you get at least some "
            "output the image will be rendered without flipping -- and "
            "hence written upside down.";
            vflip = false;
          }
        else if (code & KDU_COMPOSITOR_CANNOT_FLIP)
          {
            kdu_error e; e << "The current multi-codestream composition "
            "cannot be rendered in any orientation because it requires "
            "codestreams to be flipped in incompatible ways, while the "
            "codestreams use advanced Part-2 decomposition structures which "
            "happen to make in-place geometric flipping impossible.";
          }
        compositor.set_scale(false,vflip,false,scale);
        compositor.get_total_composition_dims(image_dims);
      }
    compositor.set_buffer_surface(image_dims); // Render the whole thing
    while (compositor.process(256000,new_region));
  } while ((buf = compositor.get_composition_buffer(image_dims)) == NULL);
  int buffer_row_gap;
  kdu_uint32 *buffer = buf->get_buf(buffer_row_gap,false);
  write_bmp_file(out_fname,image_dims.size,buffer,buffer_row_gap);
}

/*****************************************************************************/
/* STATIC     render_demo_4 (raw, JP2, JPX, MJ2 + JPX compositions)          */
/*****************************************************************************/

static void
  render_demo_4(const char *in_fname, const char *out_fname, float scale,
                float overlay_intensity)
  /* Same as `render_demo_3', except that for JPX files which contain
     a composition box (these are used for animation amongst other things),
     the first composited frame in the animation is rendered. */
{
  // Open a suitable compressed data source object
  kdu_region_compositor compositor;
  kdu_simple_file_source file_in;
  jp2_family_src src;
  jpx_source jpx_in;
  mj2_source mj2_in;
  src.open(in_fname);
  if (jpx_in.open(&src,true) >= 0)
    { // We have a JP2/JPX-compatible file
      compositor.create(&jpx_in); 
      jpx_composition compositing_rules = jpx_in.access_composition();
      jpx_frame_expander frame_expander;
      jx_frame *frame;
      if (compositing_rules.exists() &&
          ((frame = compositing_rules.get_next_frame(NULL)) != NULL) &&
          frame_expander.construct(&jpx_in,frame,0,true) &&
          (frame_expander.get_num_members() > 0))
        compositor.set_frame(&frame_expander);
      else
        compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  else if (mj2_in.open(&src,true) >= 0)
    { // We have an MJ2-compatible file
      kdu_uint32 track_idx = mj2_in.get_next_track(0);
      mj2_video_source *track;
      while ((mj2_in.get_track_type(track_idx) != MJ2_TRACK_IS_VIDEO) ||
             ((track=mj2_in.access_video_track(track_idx)) == NULL) ||
             (track->get_num_frames() == 0))
        if ((track_idx = mj2_in.get_next_track(0)) == 0)
          { kdu_error e; e << "MJ2 source contains no video frames at all."; }
      compositor.create(&mj2_in); 
      compositor.add_ilayer((int)(track_idx-1),
                            kdu_dims(),kdu_dims(),false,false,false,0,0);
    }
  else
    { // Incompatible with JP2/JPX or MJ2.  Try opening as raw codestream
      src.close();
      file_in.open(in_fname);
      compositor.create(&file_in);
      compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  
  if (overlay_intensity != 0.0F)
    configure_overlays(compositor,overlay_intensity);

  // Render the image onto the frame buffer created by `kdu_region_compositor'.
  // You can override the buffer allocation method to create frames with the
  // configuration of your choosing.
  kdu_dims new_region, image_dims;
  kdu_compositor_buf *buf=NULL;
  bool vflip=true; // Flip to generate BMP buffer in its native top-down order
  do { // Loop around in the odd event that the number of discard
    // levels is found to be too large after processing starts.
    compositor.set_scale(false,vflip,false,scale);
    if (!compositor.get_total_composition_dims(image_dims))
      { // The following code adjuss geometry and/or scale in response to some
        // extremely unlikely corner cases.
        int code = compositor.check_invalid_scale_code();
        if (code & (KDU_COMPOSITOR_SCALE_TOO_SMALL |
                    KDU_COMPOSITOR_SCALE_TOO_LARGE))
          scale = compositor.find_optimal_scale(kdu_dims(),scale,scale,scale);
        if ((code & KDU_COMPOSITOR_CANNOT_FLIP) && vflip)
          {
            kdu_warning w; w << "This Part-2 JPEG2000 image can't be "
            "vertically flipped so as to write a BMP file (these are upside "
            "down) in native order.  Of course we can write it backwards, "
            "but this is a simple demo.  To make sure you get at least some "
            "output the image will be rendered without flipping -- and "
            "hence written upside down.";
            vflip = false;
          }
        else if (code & KDU_COMPOSITOR_CANNOT_FLIP)
          {
            kdu_error e; e << "The current multi-codestream composition "
            "cannot be rendered in any orientation because it requires "
            "codestreams to be flipped in incompatible ways, while the "
            "codestreams use advanced Part-2 decomposition structures which "
            "happen to make in-place geometric flipping impossible.";
          }
        compositor.set_scale(false,vflip,false,scale);
        compositor.get_total_composition_dims(image_dims);
      }
    compositor.set_buffer_surface(image_dims); // Render the whole thing
    while (compositor.process(256000,new_region));
  } while ((buf = compositor.get_composition_buffer(image_dims)) == NULL);
  int buffer_row_gap;
  kdu_uint32 *buffer = buf->get_buf(buffer_row_gap,false);
  write_bmp_file(out_fname,image_dims.size,buffer,buffer_row_gap);
}

/*****************************************************************************/
/* STATIC     render_demo_5 (raw, JP2, JPX, MJ2 + JPX compositions)          */
/*****************************************************************************/

static void
  render_demo_5(const char *in_fname, const char *out_fname, float scale,
                float overlay_intensity)
  /* Same as `render_demo_4', except that the implementation uses a
     multi-threaded processing environment with two threads, just to
     show you how to do it. */
{
  // Create a multi-threaded processing environment
  kdu_thread_env env;
  env.create(); // Creates the single "owner" thread
  env.add_thread(); // Adds an extra "worker" thread

  // Open a suitable compressed data source object
  kdu_region_compositor compositor;
  kdu_simple_file_source file_in;
  jp2_family_src src;
  jpx_source jpx_in;
  mj2_source mj2_in;
  src.open(in_fname);
  if (jpx_in.open(&src,true) >= 0)
    { // We have a JP2/JPX-compatible file
      compositor.create(&jpx_in); 
      jpx_composition compositing_rules = jpx_in.access_composition();
      jpx_frame_expander frame_expander;
      jx_frame *frame;
      if (compositing_rules.exists() &&
          ((frame = compositing_rules.get_next_frame(NULL)) != NULL) &&
          frame_expander.construct(&jpx_in,frame,0,true) &&
          (frame_expander.get_num_members() > 0))
        compositor.set_frame(&frame_expander);
      else
        compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  else if (mj2_in.open(&src,true) >= 0)
    { // We have an MJ2-compatible file
      kdu_uint32 track_idx = mj2_in.get_next_track(0);
      mj2_video_source *track;
      while ((mj2_in.get_track_type(track_idx) != MJ2_TRACK_IS_VIDEO) ||
             ((track=mj2_in.access_video_track(track_idx)) == NULL) ||
             (track->get_num_frames() == 0))
        if ((track_idx = mj2_in.get_next_track(0)) == 0)
          { kdu_error e; e << "MJ2 source contains no video frames at all."; }
      compositor.create(&mj2_in); 
      compositor.add_ilayer((int)(track_idx-1),
                            kdu_dims(),kdu_dims(),false,false,false,0,0);
    }
  else
    { // Incompatible with JP2/JPX or MJ2.  Try opening as raw codestream
      src.close();
      file_in.open(in_fname);
      compositor.create(&file_in);
      compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }

  compositor.set_thread_env(&env,NULL); // You can install the multi-threaded
         // processing environment at any time, except when the `compositor'
         // is in the middle of processing -- if you need to change the
         // multi-threaded processing environment at an unknown point, the
         // safest thing to do is call `compositor.halt_processing' first.
         // You can even supply the same multi-threaded processing environment
         // to multiple `kdu_region_compositor' or `kdu_region_decompressor'
         // objects, simultaneously, allowing the work associated with each
         // such object to be distributed over the available working threads.
         // See the description of `kdu_region_compositor::set_thread_env' for
         // more ideas on what you can do with multi-threading in Kakadu.
  
  if (overlay_intensity != 0.0F)
    configure_overlays(compositor,overlay_intensity);
  
  // Render the image onto the frame buffer created by `kdu_region_compositor'.
  // You can override the buffer allocation method to create frames with the
  // configuration of your choosing.
  kdu_dims new_region, image_dims;
  kdu_compositor_buf *buf=NULL;
  bool vflip=true; // Flip to generate BMP buffer in its native top-down order
  do { // Loop around in the odd event that the number of discard
    // levels is found to be too large after processing starts.
    compositor.set_scale(false,vflip,false,scale);
    if (!compositor.get_total_composition_dims(image_dims))
      { // The following code adjuss geometry and/or scale in response to some
        // extremely unlikely corner cases.
        int code = compositor.check_invalid_scale_code();
        if (code & (KDU_COMPOSITOR_SCALE_TOO_SMALL |
                    KDU_COMPOSITOR_SCALE_TOO_LARGE))
          scale = compositor.find_optimal_scale(kdu_dims(),scale,scale,scale);
        if ((code & KDU_COMPOSITOR_CANNOT_FLIP) && vflip)
          {
            kdu_warning w; w << "This Part-2 JPEG2000 image can't be "
            "vertically flipped so as to write a BMP file (these are upside "
            "down) in native order.  Of course we can write it backwards, "
            "but this is a simple demo.  To make sure you get at least some "
            "output the image will be rendered without flipping -- and "
            "hence written upside down.";
            vflip = false;
          }
        else if (code & KDU_COMPOSITOR_CANNOT_FLIP)
          {
            kdu_error e; e << "The current multi-codestream composition "
            "cannot be rendered in any orientation because it requires "
            "codestreams to be flipped in incompatible ways, while the "
            "codestreams use advanced Part-2 decomposition structures which "
            "happen to make in-place geometric flipping impossible.";
          }
        compositor.set_scale(false,vflip,false,scale);
        compositor.get_total_composition_dims(image_dims);
      }
    compositor.set_buffer_surface(image_dims); // Render the whole thing
    while (compositor.process(256000,new_region));
  } while ((buf = compositor.get_composition_buffer(image_dims)) == NULL);
  env.terminate(NULL,true); // Best to always call this prior to `env.destroy'
             // just to be sure that all deferred synchronized jobs finish
             // running before we destroy the environment.  In the present
             // case it doesn't matter, since we have not registered any
             // synchronized jobs, deferred or otherwise -- see
             // `kdu_thread_entity::register_synchronized_job' for more info.
  env.destroy(); // Not strictly necessary, since the destructor will call it.
  int buffer_row_gap;
  kdu_uint32 *buffer = buf->get_buf(buffer_row_gap,false);
  write_bmp_file(out_fname,image_dims.size,buffer,buffer_row_gap);
}

/*****************************************************************************/
/* STATIC     render_demo_6 (raw, JP2, JPX, MJ2 + JPX compositions)          */
/*****************************************************************************/

    // Before we get started with this rendering demo, we need to derive
    // from `kdu_region_compositor', so we can perform all the rendering
    // operations at floating-point precision.
    class my_precision_compositor : public kdu_region_compositor {
      kdu_compositor_buf *
        allocate_buffer(kdu_coords min_size, kdu_coords &actual_size,
                        bool read_access_required)
          {
            min_size.x = (min_size.x < 1)?1:min_size.x;
            min_size.y = (min_size.y < 1)?1:min_size.y;
            actual_size = min_size;
            int rgap = min_size.x * 4; // 4-sample per pixel
            kdu_long array_len = ((kdu_long) rgap) * ((kdu_long) min_size.y);
            size_t array_len_val = (size_t) array_len;
            if (((kdu_long) array_len_val) != array_len)
              throw std::bad_alloc();
            float *mem = new(std::nothrow) float[array_len_val];
            if (mem == NULL)
              { // See if we can reclaim some unused memory
                cull_inactive_ilayers(0);
                mem = new float[array_len_val];
              }
            kdu_compositor_buf *result = new(std::nothrow) kdu_compositor_buf;
            if (result == NULL) // Just in case
              { delete[] mem; throw std::bad_alloc(); }
            result->init_float(mem,rgap);
            result->set_read_accessibility(true);
            return result;
          }
      void delete_buffer(kdu_compositor_buf *buf) { delete buf; }
    };

static void
  render_demo_6(const char *in_fname, const char *out_fname, float scale,
                float overlay_intensity)
  /* Same as `render_demo_4', except that all the rendering is done to
     floating point buffer surfaces, at high precision, and then the
     buffers are downconverted to 8-bits/sample only so as to write the
     output file; this shows you how to build high precision rendering
     applications, for medical, scientific or other purposes, which are
     simultaneously able to perform frame composition and animation if
     required. */
{
  // Open a suitable compressed data source object
  my_precision_compositor compositor;
  kdu_simple_file_source file_in;
  jp2_family_src src;
  jpx_source jpx_in;
  mj2_source mj2_in;
  src.open(in_fname);
  if (jpx_in.open(&src,true) >= 0)
    { // We have a JP2/JPX-compatible file
      compositor.create(&jpx_in); 
      jpx_composition compositing_rules = jpx_in.access_composition();
      jpx_frame_expander frame_expander;
      jx_frame *frame;
      if (compositing_rules.exists() &&
          ((frame = compositing_rules.get_next_frame(NULL)) != NULL) &&
          frame_expander.construct(&jpx_in,frame,0,true) &&
          (frame_expander.get_num_members() > 0))
        compositor.set_frame(&frame_expander);
      else
        compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  else if (mj2_in.open(&src,true) >= 0)
    { // We have an MJ2-compatible file
      kdu_uint32 track_idx = mj2_in.get_next_track(0);
      mj2_video_source *track;
      while ((mj2_in.get_track_type(track_idx) != MJ2_TRACK_IS_VIDEO) ||
             ((track=mj2_in.access_video_track(track_idx)) == NULL) ||
             (track->get_num_frames() == 0))
        if ((track_idx = mj2_in.get_next_track(0)) == 0)
          { kdu_error e; e << "MJ2 source contains no video frames at all."; }
      compositor.create(&mj2_in); 
      compositor.add_ilayer((int)(track_idx-1),
                            kdu_dims(),kdu_dims(),false,false,false,0,0);
    }
  else
    { // Incompatible with JP2/JPX or MJ2.  Try opening as raw codestream
      src.close();
      file_in.open(in_fname);
      compositor.create(&file_in);
      compositor.add_ilayer(0,kdu_dims(),kdu_dims());
    }
  
  if (overlay_intensity != 0.0F)
    configure_overlays(compositor,overlay_intensity);

  // Render the image onto the floating-point frame buffers allocated by
  // `kdu_precision_compositor'.
  kdu_dims new_region, image_dims;
  kdu_compositor_buf *buf=NULL;
  bool vflip=true; // Flip to generate BMP buffer in its native top-down order
  do { // Loop around in the odd event that the number of discard
    // levels is found to be too large after processing starts.
    compositor.set_scale(false,vflip,false,scale);
    if (!compositor.get_total_composition_dims(image_dims))
      { // The following code adjuss geometry and/or scale in response to some
        // extremely unlikely corner cases.
        int code = compositor.check_invalid_scale_code();
        if (code & (KDU_COMPOSITOR_SCALE_TOO_SMALL |
                    KDU_COMPOSITOR_SCALE_TOO_LARGE))
          scale = compositor.find_optimal_scale(kdu_dims(),scale,scale,scale);
        if ((code & KDU_COMPOSITOR_CANNOT_FLIP) && vflip)
          {
            kdu_warning w; w << "This Part-2 JPEG2000 image can't be "
            "vertically flipped so as to write a BMP file (these are upside "
            "down) in native order.  Of course we can write it backwards, "
            "but this is a simple demo.  To make sure you get at least some "
            "output the image will be rendered without flipping -- and "
            "hence written upside down.";
            vflip = false;
          }
        else if (code & KDU_COMPOSITOR_CANNOT_FLIP)
          {
            kdu_error e; e << "The current multi-codestream composition "
            "cannot be rendered in any orientation because it requires "
            "codestreams to be flipped in incompatible ways, while the "
            "codestreams use advanced Part-2 decomposition structures which "
            "happen to make in-place geometric flipping impossible.";
          }
        compositor.set_scale(false,vflip,false,scale);
        compositor.get_total_composition_dims(image_dims);
      }
    compositor.set_buffer_surface(image_dims); // Render the whole thing
    while (compositor.process(256000,new_region));
  } while ((buf = compositor.get_composition_buffer(image_dims)) == NULL);
  int buffer_row_gap;
  float *buffer = buf->get_float_buf(buffer_row_gap,false);
  kdu_uint32 *buffer32 = convert_four_floats_to_argb32(buffer,buffer_row_gap,
                                                       image_dims.size);
  write_bmp_file(out_fname,image_dims.size,buffer32,buffer_row_gap);
}

/*****************************************************************************/
/* STATIC                      render_demo_cpu                               */
/*****************************************************************************/

static void
  render_demo_cpu(const char *in_fname, int num_threads, float scale)
  /* This function demonstrates decompression of either a raw codestream
     or a JP2 file, using the the `kdu_region_decompressor' object, writing
     results to a temporary memory buffer, which can be much smaller than
     the full image, in the case of huge images.  This is done to provide
     realistic CPU processing times for the `kdu_region_decompressor' object
     when used with huge images.  No output file is actually written. */
{
  // Start by checking whether the input file belongs to the JP2 family of
  // file formats by testing the signature.
  jp2_family_src src;
  jp2_input_box box;
  src.open(in_fname);
  bool is_jp2 = box.open(&src) && (box.get_box_type() == jp2_signature_4cc);
  src.close();

  // Open a suitable compressed data source object
  kdu_compressed_source *input = NULL;
  kdu_simple_file_source file_in;
  jp2_family_src jp2_ultimate_src;
  jp2_source jp2_in;
  if (!is_jp2)
    {
      file_in.open(in_fname);
      input = &file_in;
    }
  else
    {
      jp2_ultimate_src.open(in_fname);
      if (!jp2_in.open(&jp2_ultimate_src))
        { kdu_error e; e << "Supplied input file, \"" << in_fname
                         << "\", does not appear to contain any boxes."; }
      if (!jp2_in.read_header())
        assert(0); // Should not be possible for file-based sources
      input = &jp2_in;
    }

  // Create a multi-threaded processing environment
  kdu_thread_env env;
  kdu_thread_env *env_ref = NULL;
  if (num_threads > 0)
    {
      env.create(); // Creates the single "owner" thread
      for (int nt=1; nt < num_threads; nt++)
        if (!env.add_thread())
          num_threads = nt; // Unable to create all the threads requested
      env_ref = &env;
    }

  // Create the code-stream management machinery for the compressed data
  kdu_codestream codestream;
  codestream.create(input);

  // Configure a `kdu_channel_mapping' object for `kdu_region_decompressor'
  kdu_channel_mapping channels;
  if (jp2_in.exists())
    channels.configure(&jp2_in,false);
  else
    channels.configure(codestream);

  // Determine dimensions and create temporary buffer
  kdu_region_decompressor decompressor;
  kdu_dims image_dims, new_region, incomplete_region;
  kdu_int32 *buffer = NULL;
  clock_t start_time, end_time;
  
  // Start region decompressor and render the image to the buffer
  try { // Catch errors to properly deallocate resources before returning
    do { // Loop around in the odd event that the number of discard
         // levels is found to be too large after processing starts.
      if (buffer != NULL)
        {
          kdu_warning w; w << "Discovered a tile with fewer DWT levels part "
          "way through processing; repeating the rendering process with "
          "fewer DWT levels, together with resolution reduction signal "
          "processing.";
          delete[] buffer; buffer = NULL;
        }
      int discard_levs=0, max_discard_levs = codestream.get_min_dwt_levels();
      while ((scale < 0.6F) && (discard_levs < max_discard_levs))
        { scale *= 2.0F; discard_levs++; }
      double min_scale_prod, max_scale_x, max_scale_y;
      decompressor.get_safe_expansion_factors(codestream,&channels,-1,
                                              discard_levs,min_scale_prod,
                                              max_scale_x,max_scale_y);
      kdu_coords scale_num, scale_den;
      find_rational_scale(scale,scale_num,scale_den,
                          min_scale_prod,max_scale_x,max_scale_y);
      image_dims =
        decompressor.get_rendered_image_dims(codestream,&channels,-1,
                                             discard_levs,scale_num,scale_den);
      int max_region_pixels = (1<<16); // Provide at most a 256 kB buffer
      if (image_dims.area() < (kdu_long) max_region_pixels)
        max_region_pixels = (int) image_dims.area();
      if (image_dims.size.x > max_region_pixels)
        max_region_pixels = image_dims.size.x;
      buffer = new kdu_int32[max_region_pixels];
      start_time = clock();
      decompressor.start(codestream,&channels,-1,discard_levs,INT_MAX,
                         image_dims,scale_num,scale_den,false,
                         KDU_WANT_OUTPUT_COMPONENTS,true,env_ref,NULL);
      incomplete_region=image_dims;
      while (decompressor.process(buffer,kdu_coords(0,0),0,(1<<18),
                                  max_region_pixels,incomplete_region,
                                  new_region)); // Loop until buffer is filled
      if (!decompressor.finish())
        throw KDU_ERROR_EXCEPTION; // An error was caught during `process'
    } while (!incomplete_region.is_empty());
    end_time = clock();
    
    double processing_time = ((double)(end_time-start_time))/CLOCKS_PER_SEC;
    double processing_rate = ((double) image_dims.area()) / processing_time;
    pretty_cout << "Total processing time = "<<processing_time<<" seconds\n";
      pretty_cout << "Processing rate = " << (processing_rate/1000000.0)
                  << " pixels/micro-second\n";
      if (num_threads == 0)
        pretty_cout << "Processed using the single-threaded environment\n";
      else
        pretty_cout << "Processed using the multi-threaded environment, with\n"
          << "    " << num_threads << " parallel threads of execution\n";
    }
  catch (...)
    {
      env.terminate(NULL,true);
      codestream.destroy();
      if (buffer != NULL)
        delete[] buffer;
      throw; // Rethrow the exception
    }

  // Cleanup
  env.terminate(NULL,true);
  codestream.destroy();
  delete[] buffer;
}
