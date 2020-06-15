/*****************************************************************************/
// File: kdu_roi_processing.h [scope = CORESYS/COMMON]
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
   Uniform interfaces to ROI information services.
******************************************************************************/

#ifndef KDU_ROI_PROCESSING_H
#define KDU_ROI_PROCESSING_H

#include <assert.h>
#include "kdu_messaging.h"
#include "kdu_compressed.h"

// Defined here:
class kdu_roi_node;
class kdu_roi_level;
class kdu_roi_image;

// Defined elsewhere:
class kd_roi_level;

/*****************************************************************************/
/*                                kdu_roi_node                               */
/*****************************************************************************/

class kdu_roi_node {
	/* [BIND: reference]
	   [SYNOPSIS]
	   This abstract base class defines an interface to a derived object
	   which is able to supply ROI mask information.  It has no state (no
	   data members) of its own.  The `pull' function retrieves a single
	   line of ROI information from some tile-component, at some resolution
	   or in some subband.  The scope depends upon the object (`kdu_roi_level'
	   or `kdu_roi_image') which supplied the relevant interface.
	*/
public: // Member functions
	virtual ~kdu_roi_node() { return; } // Prevent compiler warnings
	virtual void release() { return; }
	/* [SYNOPSIS]
		 When a `kdu_roi_node' object is acquired from another
		 object (`kdu_roi_level' or `kdu_roi_image'), resources may
		 or may not be allocated by the granting object.  For safety,
		 always call this function after you are done with the node,
		 so that the derived object has an opportunity to release
		 any such resources.  The default implementation does
		 nothing.
	*/
	virtual void
		pull(kdu_byte buf[], int width) = 0;
	/* [SYNOPSIS]
		 It is illegal to request more lines than are available.
	*/
};

/*****************************************************************************/
/*                               kdu_roi_level                               */
/*****************************************************************************/

class kdu_roi_level {
	/* [SYNOPSIS]
	   This object propagates ROI information from one node in the DWT
	   decomposition tree to its immediate children.  In JPEG2000 Part-1,
	   these nodes are inevitably connected with resolution levels, but
	   for the more general case of Part-2 decomposition structures, we
	   identify the node via a `kdu_node' interface rather than `kdu_resolution'.
	   [//]
	   The `source' object supplied to the `create' function provides ROI
	   information for the intermediate image at the input to the relevant
	   DWT stage, while `acquire_band' provides `kdu_roi_node' interfaces to
	   ROI information for the subbands derived from that DWT stage.
	   [//]
	   The object is actually only an interface to an internal object
	   whose implementation is not in public view.  Interfaces may be
	   copied at will, without impacting this internal object.  For this
	   reason, there is no meaningful destructor, since we do not want the
	   internal object to be destroyed when any of its interfaces goes out
	   of scope.  You must use the explicit `destroy' function to destroy
	   the internal object.
	*/
public: // Member functions
	kdu_roi_level() { state = NULL; }
	/* [SYNOPSIS]
		 Creates an empty interface; one whose `exists' function returns
		 false.  You must call the `create' function to construct the
		 internal machinery to which the interface refers.  Only then
		 can you use the `acquire_node' function.
	*/
	bool exists() { return (state != NULL); }
	/* [SYNOPSIS]
		 Returns false until the call to `create' and after any call to
		 `destroy'.
	*/
	bool operator!() { return (state == NULL); }
	/* [SYNOPSIS]
		 Opposite of `exists', returning true until the call to `create'
		 and after any call to `destroy'.
	*/
	void create(kdu_node node, kdu_roi_node *source);
	/* [SYNOPSIS]
		 Creates the internal object to which this class provides an
		 interface.  The `source' object may not be NULL.  Its `release'
		 function will automatically be called upon destruction of the
		 current object, or prior to that time (if the object is able to
		 figure out that no further ROI information will be required).
	*/
	void create(kdu_resolution resolution, kdu_roi_node *source)
	{
		create(resolution.access_node(), source);
	}
	/* [SYNOPSIS]
		 This function is provided for backward compatibility with Kakadu
		 versions 4.5 and earlier, where each stage in the DWT decomposition
		 tree was necessarily an image resolution.  This version of the
		 overloaded `create' function just invokes the first version with
		 the resolution's primary node, obtained via its
		 `kdu_resolution::access_node' function.
	*/
	void destroy();
	/* [SYNOPSIS]
		 Destroys the internal object to which the interface refers -- this
		 internal object was created by the `create' call.
	*/
	kdu_roi_node *acquire_node(int child_idx);
	/* [SYNOPSIS]
		 Provides an interface through which a particular subband's ROI
		 information may be accessed.  Note carefully that the returned
		 node must be released PRIOR TO DESTROYING the current object.
	   [ARG: child_idx]
		 Must be one of `LL_BAND', `LH_BAND', `HL_BAND' or `HH_BAND'.
		 This argument plays the same role as the `child_idx' argument in
		 `kdu_node::access_child'.
	*/
private: // Data
	kd_roi_level *state;
};

/*****************************************************************************/
/*                               kdu_roi_image                               */
/*****************************************************************************/

class kdu_roi_image {
	/* [BIND: reference]
	   [SYNOPSIS]
	   This abstract base class is not derived by any class defined by
	   the Kakadu core system.  To use ROI features, the application must
	   supply a suitable derivation.  Some useful examples are provided with
	   the "kdu_compress" application.  The derived object might obtain ROI
	   information interactively from a user (e.g., in a clinical application).
	   [//]
	   However the ROI information is obtained, the derived object must be
	   able to supply a suitably derived `kdu_roi_node' object, which is capable
	   of delivering the ROI information for any given image component and
	   any given tile region.  The present base class only defines the formal
	   mechanisms to be used to accessing these derived `kdu_roi_node' objects.
	*/
public: // Member functions
	virtual ~kdu_roi_image() { return; }
	/* [SYNOPSIS] Allows destruction from the abstract base. */
	virtual kdu_roi_node *
		acquire_node(int component, kdu_dims tile_region) = 0;
	/* [SYNOPSIS]
		 Acquires a suitably derived `kdu_roi_node' object, which is
		 capable of delivering the ROI information for the indicated
		 image component, confined to the indicated tile region.
	   [ARG: component]
		 Image components start from 0.
	   [ARG: tile_region]
		 Regions supplied here are expected to form a partition of the image
		 region, mapped to the relevant image component.  Specifically,
		 the `tile_region' will normally be that returned by the
		 `kdu_resolution::get_dims' function, applied to the highest
		 resolution of the relevant tile-component (that recovered
		 using `kdu_tile_comp::access_resolution').  The reason for
		 supplying explicit tile region coordinates rather than tile
		 indices is to relieve the implementation of the derived
		 `kdu_image' object from having to understand tiles.  The
		 partition requirement ensures that every requested region will
		 belong to the region occupied by the relevant image component
		 and that no two requested regions will overlap.
	   [RETURNS]
		 May return NULL if the entire region or component is to be
		 interpreted as part of the foreground.  Otherwise, the
		 returned `kdu_roi_node' object should be released
		 (through `kdu_roi_node::release') once the application is done
		 with it.  Usually, the node will be supplied in the constructor
		 for a sample data processing engine (see
		 `kdu_analysis::kdu_analysis' or `kdu_encoder::kdu_encoder'), in
		 which case the relevant object will take care of these release
		 responsibilities itself.
	*/
};

#endif // KDU_ROI_PROCESSING_H
