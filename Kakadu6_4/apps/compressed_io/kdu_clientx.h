/*****************************************************************************/
// File: kdu_clientx.h [scope = APPS/COMPRESSED_IO]
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
  Defines a JPX-specific version of the `kdu_client_translator' interface,
for translating JPX codestream context requests into individual codestream
window requests for the purpose of determining what image elements already
exist in the client's cache.
******************************************************************************/

#ifndef KDU_CLIENTX_H
#define KDU_CLIENTX_H

#include "kdu_client.h"
#include "jp2.h"

// Defined here:
class kdu_clientx;

// Defined elsewhere:
struct kdcx_stream_mapping;
struct kdcx_layer_mapping;
struct kdcx_comp_mapping;

/*****************************************************************************/
/*                                kdu_clientx                                */
/*****************************************************************************/

class kdu_clientx : public kdu_client_translator {
  /* [BIND: reference]
     [SYNOPSIS]
       Provides an implementation of the `kdu_client_translator' interface
       for the specific case of JP2/JPX files.  It is not strictly necessary
       to provide any translator for JP2 files, but since JP2 is a subset
       of JPX, there is no problem providing the implementation here.  All
       the translation work is done inside the `update' member.  If it finds
       the data source not to be JPX-compatible, all translation calls
       will fail, producing null/false results, as appropriate.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_clientx();
    KDU_AUX_EXPORT virtual ~kdu_clientx();
      /* [SYNOPSIS]
           Do not destroy the object while it is still in use by a
           `kdu_client' object.  If the translator has already been
           passed to `kdu_client::install_context_translator', you must
           either destroy the `kdu_client' object, or pass a NULL
           argument to `kdu_client::install_context_translator' before
           destroying the translator itself.
      */
    KDU_AUX_EXPORT virtual void close();
      /* [SYNOPSIS]
           Overrides `kdu_client_translator::close' to clean up internal
           resources.
      */
    KDU_AUX_EXPORT virtual bool update();
      /* [SYNOPSIS]
           Overrides `kdu_client_translator::update' to actually update
           the internal representation, based on any new information which
           might have arrived in the cache.
      */
    KDU_AUX_EXPORT virtual int
      get_num_context_members(int context_type, int context_idx,
                              int remapping_ids[]);
      /* [SYNOPSIS]
           Provides the implementation of
           `kdu_client_translator::get_num_context_members'.
      */
    KDU_AUX_EXPORT virtual int
      get_context_codestream(int context_type,
                             int context_idx, int remapping_ids[],
                             int member_idx);
      /* [SYNOPSIS]
           Provides the implementation of
           `kdu_client_translator::get_context_codestream'.
      */
    KDU_AUX_EXPORT virtual const int *
      get_context_components(int context_type,
                             int context_idx, int remapping_ids[],
                             int member_idx, int &num_components);
      /* [SYNOPSIS]
           Provides the implementation of
           `kdu_client_translator::get_context_components'.
      */
    KDU_AUX_EXPORT virtual bool
      perform_context_remapping(int context_type,
                                int context_idx, int remapping_ids[],
                                int member_idx, kdu_coords &resolution,
                                kdu_dims &region);
      /* [SYNOPSIS]
           Provides the implementation of
           `kdu_client_translator::perform_context_remapping'.
      */
  private: // Helper functions
    kdcx_stream_mapping *add_stream(int idx);
      /* If the indicated codestream does not already have an allocated
         `kdcx_stream_mapping' object, one is created here.  In any event,
         the relevant object is returned. */
    kdcx_layer_mapping *add_layer(int idx);
      /* If the indicated compositing layer does not already have an allocated
         `kdcx_layer_mapping' object, one is created here.  In any event,
         the relevant object is returned. */
  private: // Data members used to store file-wide status
    friend struct kdcx_layer_mapping;
    jp2_input_box top_box; // Current incomplete top-level box
    jp2_input_box jp2h_sub; // First incomplete sub-box of jp2 header box
    jp2_family_src src;
    bool started; // False if `update' has not yet been called
    bool not_compatible; // True if file format is found to be incompatible
    bool is_jp2; // If file is plain JP2, rather than JPX
    bool top_level_complete; // If no need to parse any further top level boxes
    bool defaults_complete; // If `stream_defaults' & `layer_defaults' complete
    kdcx_stream_mapping *stream_defaults;
    kdcx_layer_mapping *layer_defaults;
    kdcx_comp_mapping *composition;
  private: // Data members used to unpack codestream headers
    int num_jp2c_or_frag; // Num JP2C or fragment boxes found so far
    int num_jpch; // Num JPCH boxes found so far
    int num_codestreams;
    int max_codestreams; // Size of following array
    kdcx_stream_mapping **stream_refs;
  private: // Data members used to unpack compositing layers
    int num_jplh; // Number of JPLH boxes found so far
    int num_compositing_layers;
    int max_compositing_layers; // Size of following array
    kdcx_layer_mapping **layer_refs;
  };

#endif // KDU_CLIENTX_H
