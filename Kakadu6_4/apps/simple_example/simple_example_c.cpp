/*****************************************************************************/
// File: simple_example_c.cpp [scope = APPS/SIMPLE_EXAMPLE]
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
   Simple example showing compression with an intermediate buffer used to
store the image samples.  This is, of course, a great waste of memory and
the more sophisticated "kdu_compress" and "kdu_buffered_compress" applications
do no such thing.  However, developers may find this type of introductory
example helpful as a first exercise in understanding how to use Kakadu.  The
vast majority of Kakadu's interesting features are not utilized by this
example, but it may be enough to satisfy the initial needs of some developers
who are working with moderately large images, already in an internal buffer.
For a more sophisticated compression application, which is still quite
readily comprehended, see the "kdu_buffered_compress" application.
   The emphasis here is on showing a minimal number of instructions required
to open a codestream object, create a processing engine and push an image into
the engine, producing a given number of automatically selected rate-distortion
optimized quality layers for a quality-progressive representation.
   You may find it interesting to open the compressed codestream using
the "kdu_show" utility, examine the properties (file:properties menu item),
and investigate the quality progression associated with the default set of
quality layers created here (use the "<" and ">" keys to navigate through
the quality layers interactively).
******************************************************************************/

// System includes
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
// Kakadu core includes
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"
// Application level includes
#include "kdu_file_io.h"
#include "kdu_stripe_compressor.h"

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
/* STATIC                    eat_white_and_comments                          */
/*****************************************************************************/

static void
  eat_white_and_comments(std::istream &stream)
{
  char ch;
  bool in_comment;

  in_comment = false;
  while (!(stream.get(ch)).fail())
    if (ch == '#')
      in_comment = true;
    else if (ch == '\n')
      in_comment = false;
    else if ((!in_comment) && (ch != ' ') && (ch != '\t') && (ch != '\r'))
      {
        stream.putback(ch);
        return;
      }
}

/*****************************************************************************/
/* STATIC                          read_image                                */
/*****************************************************************************/

static kdu_byte *
  read_image(char *fname, int &num_components, int &height, int &width)
  /* Simple PGM/PPM file reader.  Returns an array of interleaved image
     samples (there are 1 or 3 components) in unsigned bytes, appearing
     in scan-line order.  If the file cannot be open, the program simply
     exits for simplicity. */
{
  char magic[3];
  int max_val; // We don't actually use this.
  char ch;
  std::ifstream in(fname,std::ios::in|std::ios::binary);
  if (in.fail())
    { kdu_error e; e << "Unable to open input file, \"" << fname << "\"!"; }

  // Read PGM/PPM header
  in.get(magic,3);
  if (strcmp(magic,"P5") == 0)
    num_components = 1;
  else if (strcmp(magic,"P6") == 0)
    num_components = 3;
  else
    { kdu_error e; e << "PGM/PPM image file must start with the magic string, "
      "\"P5\" or \"P6\"!"; }
  eat_white_and_comments(in); in >> width;
  eat_white_and_comments(in); in >> height;
  eat_white_and_comments(in); in >> max_val;
  if (in.fail())
    {kdu_error e; e << "Image file \"" << fname << "\" does not appear to "
     "have a valid PGM/PPM header."; }
  while (!(in.get(ch)).fail())
    if ((ch == '\n') || (ch == ' '))
      break;

  // Read sample values
  int num_samples = height*width*num_components;
  kdu_byte *buffer = new kdu_byte[num_samples];
  if ((int)(in.read((char *) buffer,num_samples)).gcount() != num_samples)
    { kdu_error e; e << "PGM/PPM image file \"" << fname
      << "\" terminated prematurely!"; }
  return buffer;
}

/*****************************************************************************/
/* STATIC                         print_usage                                */
/*****************************************************************************/

static void
  print_usage(char *prog)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage: \"" << prog << " <PNM input file> <J2C output file>\"\n";
  exit(0);
}

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  if (argc != 3)
    print_usage(argv[0]);

  // Custom messaging services
  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);

  // Load image into a buffer
  int num_components, height, width;
  kdu_byte *buffer = read_image(argv[1],num_components,height,width);

  // Construct code-stream object
  siz_params siz;
  siz.set(Scomponents,0,0,num_components);
  siz.set(Sdims,0,0,height);  // Height of first image component
  siz.set(Sdims,0,1,width);   // Width of first image component
  siz.set(Sprecision,0,0,8);  // Image samples have original bit-depth of 8
  siz.set(Ssigned,0,0,false); // Image samples are originally unsigned
  kdu_params *siz_ref = &siz; siz_ref->finalize();
     // Finalizing the siz parameter object will fill in the myriad SIZ
     // parameters we have not explicitly specified in this simple example.
     // The capabilities of the finalization process are documented in
     // "kdu_params.h".  Note that we execute the virtual member function
     // through a pointer, since the address of the function is not explicitly
     // exported by the core DLL (minimizes the export table).
  kdu_simple_file_target output; output.open(argv[2]);
     // As an alternative to raw code-stream output, you may wish to wrap
     // the code-stream in a JP2 file, which then allows you to add a
     // additional information concerning the colour and geometric properties
     // of the image, all of which should be respected by conforming readers.
     // To do this, include "jp2.h" and declare "output" to be of class
     // `jp2_target' instead of `kdu_simple_file_target'.  Rather than opening
     // the `jp2_target' object directly, you first creat a `jp2_family_tgt'
     // object, using it to open the file to which you want to write, and then
     // pass it to `jp2_target::open'.  At a minimum, you must execute the
     // following additional configuration steps to create a JP2 file:
     //     jp2_dimensions dims = output.access_dimensions(); dims.init(&siz);
     //     jp2_colour colr = output.access_colour();
     //     colr.init((num_components==3)?:JP2_sRGB_SPACE:JP2_sLUM_SPACE);
     // Of course, there is a lot more you can do with JP2.  Read the
     // interface descriptions in "jp2.h" and/or take a look at the more
     // sophisticated demonstration in "kdu_compress.cpp".  A simple
     // demonstration appears also in "kdu_buffered_compress.cpp".
     //    A common question is how you can get the compressed data written
     // out to a memory buffer, instead of a file.  To do this, simply
     // derive a memory buffering object from the abstract base class,
     // `kdu_compressed_target' and pass this into `kdu_codestream::create'.
     // If you want a memory resident JP2 file, pass your memory-buffered
     // `kdu_compressed_target' object to the second form of the overloaded
     // `jp2_family_tgt::open' function and proceed as before.
  kdu_codestream codestream; codestream.create(&siz,&output);

  // Set up any specific coding parameters and finalize them.
  codestream.access_siz()->parse_string("Clayers=12");
  codestream.access_siz()->parse_string("Creversible=yes");
  codestream.access_siz()->finalize_all(); // Set up coding defaults

  // Now compress the image in one hit, using `kdu_stripe_compressor'
  kdu_stripe_compressor compressor;
  compressor.start(codestream);
  int stripe_heights[3]={height,height,height};
  compressor.push_stripe(buffer,stripe_heights);
  compressor.finish();
    // As an alternative to the above, you can read the image samples in
    // smaller stripes, calling `compressor.push_stripe' after each stripe.
    // Note that the `kdu_stripe_compressor' object can be used to realize
    // a wide range of Kakadu compression features, while supporting a wide
    // range of different application-selected stripe buffer configurations
    // precisions and bit-depths.  When developing your Kakadu-based
    // compression application, be sure to read through the extensive
    // documentation provided for this object.  For a much richer demonstration
    // of the use of `kdu_stripe_compressor', take a look at the
    // "kdu_buffered_compress" application.

  // Finally, cleanup
  codestream.destroy(); // All done: simple as that.
  output.close(); // Not really necessary here.
  return 0;
}
