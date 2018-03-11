/*****************************************************************************/
// File: simple_example_d.cpp [scope = APPS/SIMPLE_EXAMPLE]
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
   Simple example showing decompression into an intermediate buffer for the
image samples.  This is, of course, a great waste of memory and the more
sophisticated "kdu_expand" and "kdu_buffered_expand" applications do no such
thing.  This is also far from the recommended procedure for interactive
applications, for two reasons: 1) a responsive interactive application should
incrementally update the display as it goes, processing samples in order of
their relevance to the portion of the image actually being displayed; and
2) interactive applications usually maintain a viewing portal into a
potentially much larger image, so it is preferable to decompress only the image
regions which are visible, thereby avoiding the cost of buffering the entire
decompressed image.  A far superior approach for interactive applications is
demonstrated by the "kdu_show" utility and the `kdu_region_decompressor'
object used in that utility should provide a versatile and useful interface tool
for many interactive applications.
   For a more sophisticated non-interactive decompression example, which is
still quite readily comprehended, see the "kdu_buffered_expand" application.
   Although the current example is far from the recommended way of using
Kakadu in many professional applications, developers may find this type of
introductory example helpful as a first exercise in understanding how to use
Kakadu.  The vast majority of Kakadu's interesting features are not utilized by
this example, but it may be enough to satisfy the initial needs of some
developers who are working with moderately large images, already in an internal
buffer, especially when interfacing with existing image processing packages
which are buffer-oriented.
   The emphasis here is on showing a minimal number of instructions required
to open a codestream object, create a decompression engine and recover samples
from the engine.  The present application should be able to cope with any
code-stream at all, although it may decompress only the first image
component, if multiple components have differing sizes so that they are
incompatible with the simple interleaved buffer structure used here.
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
#include "kdu_stripe_decompressor.h"

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
/* STATIC                         write_image                                */
/*****************************************************************************/

static void
  write_image(char *fname, kdu_byte *buffer,
              int num_components, int height, int width)
  /* Simple PGM/PPM file writer.  The `buffer' array contains interleaved
     image samples (there are 1 or 3 components) in unsigned bytes,
     appearing in scan-line order. */
{
  std::ofstream out(fname,std::ios::out|std::ios::binary);
  if (!out)
    { kdu_error e;
      e << "Unable to open output image file, \"" << fname << "\"."; }
  if (num_components == 1)
    out << "P5\n" << width << " " << height << "\n255\n";
  else if (num_components == 3)
    out << "P6\n" << width << " " << height << "\n255\n";
  else
    assert(0); // The caller makes sure only 1 or 3 components are decompressed
  if (!out.write((char *) buffer,num_components*width*height))
    { kdu_error e;
      e << "Unable to write to output image file, \"" << fname << "\"."; }
}

/*****************************************************************************/
/* STATIC                         print_usage                                */
/*****************************************************************************/

static void
  print_usage(char *prog)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage: \"" << prog << " <J2C input file> <PNM output file>\"\n";
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

  // Construct code-stream object
  kdu_simple_file_source input(argv[1]);
    // As an alternative to raw code-stream input, you may wish to open a
    // JP2-compatible file which embeds a JPEG2000 code-stream.  Full support
    // for the optional JP2 file format is available by declaring `input'
    // to be of class `jp2_source' instead of `kdu_simple_file_source'.  You
    // will need to include "jp2.h" to enable this functionality.  You will
    // need to open the file using `jp2_family_src::open', passing the
    // `jp2_family_src' object to `jp2_source::open', and then invoking
    // `jp2_source::read_header', before you can pass the `jp2_source'
    // object into `kdu_codestream::create'.  This slightly indirect chain
    // of events allows all members of the JP2-family of file formats to be
    // supported in a uniform manner, while also allowing all of these
    // file formats to be managed interactively through the JPIP client/server
    // system.
    //   When processing JP2 files, you should also respect the palette
    // mapping, channel binding and colour transformation attributes conveyed
    // through the auxiliary boxes in the file.  All relevant information is
    // exposed through the interfaces which can be accessed from the
    // `jp2_source' object.  These objects not only provide you with a uniform
    // interface to the sometimes tangled box definitions used by JP2, but
    // they also provide tools for performing most common transformations of
    // interest.  You may wish to consult the more extensive demonstrations
    // in the "kdu_expand", "kdu_buffered_expand" and "kdu_show" applications.
    // For compliant JP2-sensitive image rendering, your best bet is to build
    // your application upon the powerful `kdu_region_decompressor' object,
    // which is demonstrated in the "kdu_show" and "kdu_render" applications
    // as well as the Java "Kdu_render" application.
    //    A common question is how you can source compressed data from a
    // memory buffer instead of a file.  To do this, simply derive a memory
    // buffered source from the abstract base class `kdu_compressed_source'
    // and pass the derived object into `kdu_codestream::create'.  If you
    // want to source an in-memory JP2 file, pass your memory buffering
    // `kdu_compressed_source' object to the second form of the
    // `jp2_family_src::open' function, and proceed as before.
  kdu_codestream codestream; codestream.create(&input);
  codestream.set_fussy(); // Set the parsing error tolerance.

      //    If you want to flip or rotate the image for some reason, change
      // the resolution, or identify a restricted region of interest, this is
      // the place to do it.  You may use `kdu_codestream::change_appearance
      // and `kdu_codestream::apply_input_restrictions' for this purpose.
      //    If you wish to truncate the code-stream prior to decompression, you
      // may use `kdu_codestream::set_max_bytes'.
      //    If you wish to retain all compressed data so that the material
      // can be decompressed multiple times, possibly with different appearance
      // parameters, you should call `kdu_codestream::set_persistent' here.
      //    There are a variety of other features which must be enabled at
      // this point if you want to take advantage of them.  See the
      // descriptions appearing with the `kdu_codestream' interface functions
      // in "kdu_compressed.h" for an itemized account of these capabilities.
  
  // Determine number of components to decompress -- simple app only writes PNM
  kdu_dims dims; codestream.get_dims(0,dims);
  int num_components = codestream.get_num_components();
  if (num_components == 2)
    num_components = 1;
  else if (num_components >= 3)
    { // Check that components have consistent dimensions (for PPM file)
      num_components = 3;
      kdu_dims dims1; codestream.get_dims(1,dims1);
      kdu_dims dims2; codestream.get_dims(2,dims2);
      if ((dims1 != dims) || (dims2 != dims))
        num_components = 1;
    }
  codestream.apply_input_restrictions(0,num_components,0,0,NULL);

  // Now decompress the image in one hit, using `kdu_stripe_decompressor'
  kdu_byte *buffer = new kdu_byte[(int) dims.area()*num_components];
  kdu_stripe_decompressor decompressor;
  decompressor.start(codestream);
  int stripe_heights[3] = {dims.size.y,dims.size.y,dims.size.y};
  decompressor.pull_stripe(buffer,stripe_heights);
  decompressor.finish();
    // As an alternative to the above, you can decompress the image samples in
    // smaller stripes, writing the stripes to disk as they are produced by
    // each call to `decompressor.pull_stripe'.  For a much richer
    // demonstration of the use of `kdu_stripe_decompressor', take a look at
    // the "kdu_buffered_expand" application.

  // Write image buffer to file and clean up
  codestream.destroy();
  input.close(); // Not really necessary here.
  write_image(argv[2],buffer,num_components,dims.size.y,dims.size.x);
  delete[] buffer;
  return 0;
}
