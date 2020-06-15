/*****************************************************************************/
// File: kdu_params.h [scope = CORESYS/COMMON]
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
   Defines a code-stream parameter management system.  The system manages
information which might be stored in code-stream marker segments, as well
as some information which cannot be preserved in any well-defined way
within a JPEG2000 code-stream.  The derived object names, "siz_params",
"cod_params", "qcd_params", etc., are intended to identify the association
with particular types of code-stream marker segments (SIZ, COD, QCD, etc.).
Services are provided to parse and generate such code-stream marker segments.
However, the design here is intended to provide a more uniform view of
code-stream attributes than that offered by the JPEG2000 standard's marker
segments.
   Parameters which belong together (e.g., all COD/COC type parameters) are
grouped into what we call parameter clusters.  Each cluster has a collection
of defined attributes which may be queried, textualized, translated and so
forth, in a variety of different ways.  Each cluster generally has multiple
incarnations.  The primary incarnation is known as the "tile-head", which
represents the "fall-back" attribute values to be used for all tiles, in the
event that tile- or tile-component specific information is not available.
Additional incarnations exist for every tile-component, and also as
"component-heads" (fall-back attributes for a component, when no specific
information is provided for a tile) and "tile-component" heads (fall-back
attributes for components of a specific tile, when no specific information
is provided for that tile-component).  An elaborate system is provided to
implement fall-back policies and to provide the various "lock-outs" required
to ensure that information cannot be specialized to tiles or tile-components
when that is forbidden by the standard.
   The services defined here are sensitive to the fact that code-stream
parameters may become available incrementally (say while decompressing an
existing JPEG2000 code-stream incrementally).
   Services are provided to copy a network of code-stream parameters or
transform them into suitable subsets or geometrically adjusted views, for
the benefit of transcoding applications.  These services support incremental
availability of the information.
   Services are also provided to generate human readable descriptions of
any or all of the code-stream parameter attributes and to create attributes
from descriptions in this same form (used, for example, with command-line
argument or switch-file input).  The parameters are also capable of generating
rich or comprehensive descriptions of themselves and their interpretation.
******************************************************************************/

#ifndef KDU_PARAMS_H
#define KDU_PARAMS_H

#include <assert.h>
#include "kdu_elementary.h"
#include "kdu_messaging.h"

// Defined here:

class kdu_params;
class siz_params;
class mct_params;
class mcc_params;
class mco_params;
class atk_params;
class cod_params;
class ads_params;
class dfs_params;
class qcd_params;
class rgn_params;
class poc_params;
class crg_params;

// Referenced here, defined elsewhere:

struct kd_attribute;

/*****************************************************************************/
/*                                kdu_output                                 */
/*****************************************************************************/

#define KDU_OBUF_SIZE 512

class kdu_output {
	/* [BIND: reference]
	   [SYNOPSIS]
	   This abstract base class must be derived to construct meaningful
	   output objects.  In particular, derived objects must implement the
	   protected pure virtual function, `flush_buf'.  Since there may be
	   many low level byte-oriented transactions, the base implementation
	   emphasizes efficiency, providing a small buffer of its own, with all
	   I/O functions in-lined.
	*/
public: // Member functions
	kdu_output()
	{
		next_buf = buffer; end_buf = buffer + KDU_OBUF_SIZE;
	}
	virtual ~kdu_output() { return; }
	/* [SYNOPSIS]
		 Derived objects should usually override this function to provide a
		 destructor which will at least call the protected `flush_buf'
		 function. */
	int put(kdu_byte byte)
	{
		/* [SYNOPSIS] Writes a single byte.
		   [RETURNS] Always returns 1. */
		if (next_buf == end_buf)
		{
			flush_buf(); assert(next_buf < end_buf);
		}
		*(next_buf++) = byte;
		return 1;
	}
	int put(kdu_uint16 word)
	{
		/* [SYNOPSIS] Writes a 2-byte word in big-endian order.
		   [RETURNS] Always returns 2. */
		put((kdu_byte)(word >> 8));
		put((kdu_byte)(word >> 0));
		return 2;
	}
	int put(kdu_uint32 word)
	{
		/* [SYNOPSIS] Writes a 4-byte word in big-endian order.
		   [RETURNS] Always returns 4. */
		put((kdu_byte)(word >> 24));
		put((kdu_byte)(word >> 16));
		put((kdu_byte)(word >> 8));
		put((kdu_byte)(word >> 0));
		return 4;
	}
	int put(float val)
	{
		union {
			kdu_uint32 ival;
			float fval;
		} both;
		both.fval = val;
		put(both.ival);
		return 4;
	}
	void write(kdu_byte *buf, int count)
	{
		/* [SYNOPSIS] Writes zero or more bytes from the supplied buffer.
		   [ARG: buf] Points to the source byte buffer.
		   [ARG: count] The number of bytes to write from the buffer. */
		while (count > 0)
		{
			int xfer_bytes = (int)(end_buf - next_buf);
			if (xfer_bytes == 0)
			{
				flush_buf(); xfer_bytes = (int)(end_buf - next_buf);
			}
			xfer_bytes = (count < xfer_bytes) ? count : xfer_bytes;
			count -= xfer_bytes;
			while (xfer_bytes--)
				*(next_buf++) = *(buf++);
		}
	}
protected: // Data and functions used for buffer management.
	virtual void flush_buf() = 0;
	/* Flushes the buffer and returns with `next_buf' = `buffer'. */
	kdu_byte buffer[KDU_OBUF_SIZE];
	kdu_byte *next_buf; // Points to the next location to be written
	kdu_byte *end_buf; // Points immediately beyond the end of the buffer.
};

/*****************************************************************************/
/*                                kdu_params                                 */
/*****************************************************************************/

#define KD_MAX_PARAM_DEPENDENCIES 4 // see `kdu_params::dependencies'

class kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
		 This abstract base class must be derived to form a complete parameter
		 class, such as `siz_params', `cod_params', etc.  Each complete parameter
		 class is intended primarily to represent the parameters embodied by a
		 single type of code-stream marker segment; however, parameter classes
		 may represent additional parameters which are not recorded in
		 code-stream marker segments.  In fact, it is possible to create
		 parameter classes which are not associated with marker segments at all,
		 but it is not possible to have code-stream marker segments whose
		 parameters are managed by more than one parameter object.
		 [//]
		 We use the term `attribute' here to refer to a group of related
		 parameter values within any given `kdu_params'-derived object.  An
		 attribute may have one value, a fixed number of values, or
		 a variable length array of values, which describe some aspect of
		 the coding system.  For example, the width and height of an image
		 together form a single attribute, its dimensions.  The collection
		 of mode switches (or flags) which control the behaviour of the
		 JPEG2000 embedded block coder constitute another attribute, and so
		 on.  Attributes have identifying names which are useful when
		 processing or generating text descriptors and also help to annotate
		 the use of the generic access functions provided by the `kdu_params'
		 base class.
		 [//]
		 The `kdu_params' base class provides several different mechanisms for
		 importing and exporting parameter attribute values.  Attributes
		 may be recovered by parsing text strings (e.g., from a command-line or
		 interactive prompt), by translating code-stream marker segments, or
		 by explicit calls to a collection of overloaded `set' functions
		 provided by the `kdu_params' base class.  As an example, the following
		 two mechanisms each set the number of DWT levels 7 in the main COD
		 marker segment of an image whose coding parameters are headed by an
		 object, root.
		 [>>] root->parse_string("Clevels={7}");
		 [>>] root->access_cluster(COD_params)->set(Clevels,0,0,7);
		 [//]
		 As for internalization, parameter attributes may also be externalized
		 in various ways: as human readable text (capable of being parsed back
		 in); as valid JPEG2000 code-stream marker segments; or by direct
		 query through a collection of overloaded `get' functions provided by
		 the `kdu_params' base class.
		 [//]
		 Objects from each derived parameter class are collected into clusters,
		 which manage related code-stream marker segments from different
		 tiles and different image components.  Specifically, the term `cluster'
		 refers to all instances of any particular class derived from
		 `kdu_params' which are involved in the description of a single
		 compressed image, or its coding configuration.  Within each cluster,
		 the various instances of the relevant parameter class are identified
		 by 3 coordinates: a tile index; a component index; and an instance
		 index.  Tile indices start from the special value of -1 (see below);
		 component indices also start from a special value of -1; and instance
		 indices start from 0.  For some parameter classes (i.e., some types of
		 clusters), multiple components, multiple tiles or multiple instances
		 may be disallowed.
		 [//]
		 To understand the role played by elements with negative `tile_idx'
		 or `comp_idx' arguments, consider the `kdu_params' object associated
		 with tile t, and image component c, within some cluster.  This object
		 represents coding parameters which are specific to that tile-component.
		 If the object does not provide specific values for some parameter
		 attribute, the values default to those specified in the corresponding
		 tile head (i.e., the object with tile index t and a component index
		 of -1).  If this object also fails to provide specific values for the
		 relevant attribute, they are inherited from the object with component
		 index c and a tile index of -1.  If this also fails to produce results,
		 the attribute is inherited from the cluster head itself, having tile
		 and component indices of -1.
		 [//]
		 It is worth noting that specific parameter classes might not support
		 tile-specific forms at all, so all elements in the cluster have
		 `tile_idx'=-1.  Similarly, some parameter classes might
		 not support component-specific forms, so all elements in the cluster
		 have `comp_idx'=-1.
		 [//]
		 As suggested by the above discussion, inheritance applies to entire
		 attributes rather than specific values.  If any values are available
		 for some attribute in a particular parameter object, the entire
		 attribute is considered to be specified for the purpose of the
		 default inheritance procedure described above.
		 [//]
		 In most practical instances, most or all of the coding parameters
		 will be identical in all tiles of all components (these are called
		 `tile-components' in JPEG2000), meaning that most or all of the
		 attributes will be specified only by the cluster head.  Nevertheless,
		 JPEG2000 allows tile-specific, component-specific and
		 tile-component-specific overrides for many coding parameters and
		 the inheritance rules described above are designed to mimic the
		 philosophy underlying the JPEG2000 standard.  It is worth noting
		 that some parameter classes configure themselves (through calls to
		 the protected `define_attribute' function) to explicitly disallow
		 component-specific overrides of certain attributes, to reflect the
		 capabilities offered by the JPEG2000 code-stream syntax.
		 [//]
		 Although it is not necessary to know the details of the internal
		 organization of parameter objects into clusters, it is helpful to
		 understand some of its aspects.  For the moment, we will consider
		 only one instance (`inst_idx'=0) for each value of the `tile_idx'
		 and `comp_idx' identifies within each cluster.  Equivalently, we
		 consider only instance heads.  Some types of parameter classes
		 can have multiple instances, in which case a linked list of instances
		 is built from the instance head, but instance heads play a distinctive
		 role.  Moreover, many parameter classes cannot have any more than
		 one instance.
		 [//]
		 Associated with each cluster, is an array of references to all
		 instance heads in the cluster.  The array has `num_tiles'+1 rows
		 and `num_comps'+1 columns, where `num_tiles' and `num_cols' identify
		 the number of tiles and components in the cluster, not including the
		 special default elements which have `tile_idx' or `comp_idx' equal
		 to -1.  Each `kdu_params' object has a pointer to this shared array
		 of references for its cluster, and this is how efficient navigation
		 is achieved.  The array is notionally owned (for the purpose of
		 deletion) by the `cluster head', which is the element whose
		 `tile_idx' and `comp_idx' indices are both equal to -1.  The array
		 is not created until the cluster head is linked, using the
		 `link' function.  That is also the point at which non-negative
		 `tile_idx' and `comp_idx' identifiers can be recorded with the object.
		 [//]
		 To save space, where an image has a large number of tiles or components,
		 entries in the references array for any given cluster need not be
		 unique.  In particular, the entry on row t and column c of the
		 references array either points to a unique `kdu_params' object
		 which corresponds to that tile-component, or else it points to one
		 of the `kdu_params' objects whose tile and/or component index is equal
		 to -1.  In many cases, all entries will point to the cluster head,
		 since it contains default coding parameters for all tile-components
		 in the cluster.  If the entry at (t,-1) and (-1,c) are unique, the
		 entry at (t,c) must also be unique, rather than pointing to the
		 tile head or component head.  This allows the entry at (t,c) to borrow
		 some attributes from its tile-head and some from its component-head.
		 [//]
		 It is possible for the non-uniqueness of `kdu_params' objects to cause
		 some confusion, since the `access_relation' member may return a pointer
		 to a different `kdu_params' object to the one you expect.  If you then
		 modify any of the parameter values, you may end up modifying the wrong
		 object.  To avoid this difficulty, the `access_relation' function takes
		 a special argument to identify whether you wish to modify any parameters
		 or not.  If you do, a new `kdu_params' object will be created, if
		 necessary, so that the one you are accessing is unique to the
		 indicated tile-component.
	 */
	 // --------------------------------------------------------------------------
public: // Lifecycle functions
	KDU_EXPORT
		kdu_params(const char *cluster_name, bool allow_tile_diversity,
			bool allow_component_diversity,
			bool allow_instance_diversity,
			bool force_component_specific_forms = false,
			bool treat_instances_like_components = false);
	/* [SYNOPSIS]
		 Since `kdu_params' is an abstract base class, this constructor will
		 always be invoked from that of some derived class, which sets the
		 various arguments to reflect the meaningfulness of tile diversity,
		 component diversity, instance diversity, and so forth.
		 [//]
		 Note that all newly constructed objects have tile and component
		 indices of -1 and instance indices of 0.  This can be changed
		 by a subsequent call to the `link' function.
	   [ARG: cluster_name]
		 This string will NOT be copied; it normally points to a constant
		 string resource.
	   [ARG: force_component_specific_forms]
		 This argument should be true only for marker segments which may
		 need to be written separately for each component, even where all
		 components are the same, since there are no tile-wide and image-wide
		 forms which can legally be written.  This is the case for the RGN
		 marker segment, represented by the `rgn_params' derived class.
	   [ARG: treat_instances_like_components]
		 This argument may be true only if `allow_instance_diversity' is
		 true and `allow_component_diversity' is false.  It treats
		 instance indices as though they were component indices when
		 implementing inheritance rules.  This allows for the efficient
		 handling of marker segments which are not component-specific
		 but have a separate index (often with a variable range) which
		 behaves as though it were a component index for the purpose of
		 inheritance.  This is true of a number of marker segments defined
		 by Part 2 of the JPEG2000 standard.
	  */
	virtual kdu_params *new_object() = 0;
	/* [SYNOPSIS]
		 Creates a new object of the same class as the present one.  This
		 function is implemented within each derived parameter class to
		 simply invoke that class's constructor.  It is used by
		 `new_instance' and `access_relation' whenever a new object must
		 be created within an existing cluster.
	*/
protected:
	void add_dependency(const char *cluster);
	/*  Call this function to add the supplied cluster name of a list of
		dependencies.  If a unique object is created to represent
		parameters for any of these dependencies, a unique instance
		of the present class must also be created with the same tile
		and component indices.  This ensures that the present object's
		`finalize', copy_with_xforms' and `write_marker_segment' functions
		will be invoked for each combination of tile and component indices
		for which parameter variations in any of the dependencies might
		exist.  The need for this function arises only because the
		`link' function does not actually create unique instances of
		the object for each declared component and tile -- this is
		done for memory and access efficiency reasons.
		[//]
		The function is generally invoked from within the constructor
		of a derived parameter class.  If the current object has
		component diversity but one of its dependencies does not,
		a unique instance of the current object must be created for
		every tile-component of a tile, whenever a unique instance of
		the dependency is created with that tile index.
		This does not apply to the main header (negative tile indices).
	*/
public:
	KDU_EXPORT virtual ~kdu_params();
	/* [SYNOPSIS]
		 For objects whose tile and component indices are both non-negative,
		 this function destroys the `kdu_params' object and writes a NULL
		 reference into the cluster's references array, so that future
		 attempts to access the object via `access_relation' will fail.
		 [//]
		 For objects whose component index is -ve, all objects in the
		 cluster with the same tile index will be destroyed.
		 [//]
		 For objects whose tile index is -ve, all objects in the cluster
		 with the same component index will be destroyed.
		 [//]
		 For cluster heads (objects whose tile and component indices are
		 both negative), the entire cluster will be destroyed.
		 [//]
		 If the present object is the head of the cluster list, all clusters
		 will be destroyed.
	*/
	KDU_EXPORT kdu_params *
		link(kdu_params *existing, int tile_idx, int comp_idx,
			int num_tiles, int num_comps);
	/* [BIND: donate]
	   [SYNOPSIS]
		 Links the object into a list of existing object clusters, of which
		 `existing' must be a member.  In the process, the tile index and
		 component index of the new object are replaced by the supplied
		 values.
		 [//]
		 Upon initial construction, `kdu_params' objects are configured
		 as cluster heads, with tile and component indices of -1 and
		 an instance index of 0.  If the existing list of clusters already
		 contains an instance with the same cluster name and identical
		 tile and component indices to those supplied here, the instance
		 index will automatically be incremented to the next available
		 one for that tile-component.  If multiple instances are disallowed
		 by the derived parameter class, an appropriate error will be
		 generated.
		 [//]
		 If an object with the same cluster name does not currently exist,
		 the new object must be the cluster head, having `tile_idx' and
		 `comp_idx' both equal to -1.  In this case, the cluster's array
		 of references will be created for the first time and initialized
		 such that all entries point back to the cluster head.  Thereafter,
		 each new object linked into the same cluster must specify the
		 same values for `num_tiles' and `num_comps'.
	   [RETURNS]
		 Returns a pointer to itself (the `this') pointer, as a convenience
		 for some statement constructions.
	   [ARG: existing]
		 Pointer to any member (usually, but not necessarily a cluster head)
		 of an existing list of clusters, into which the current object is
		 to be linked.
	   [ARG: tile_idx]
		 New value to use for the tile index -- upon construction, objects
		 will normally have a tile index of -1.  The value supplied here
		 should be in the range 0 to `num_tiles'-1 for tile-specific
		 members of the cluster, or -1 for cluster heads and
		 component- but not tile-specific default objects.
	   [ARG: comp_idx]
		 New value to use for the component index -- upon construction,
		 objects will normally have a component index of -1.  The value
		 supplied here should be in the range 0 to `num_comps'-1 for
		 component-specific members of the cluster, or -1 for cluster and
		 tile heads.
	   [ARG: num_tiles]
		 Number of tiles (excluding the one with a negative tile
		 index) for objects in this cluster.  Must agree with the values
		 supplied when linking new objects into the same cluster.
		 Note that some parameter classes do not support tile
		 diversity, in which case `num_tiles' must be 0.
	   [ARG: num_comps]
		 Number of image components (excluding the one with a negative
		 component index) for objects in this cluster.  Must agree with the
		 values supplied when linking new objects into the same cluster.
		 Note that some parameter classes do not support component
		 diversity, in which case `num_comps' must be 0.
	*/
	KDU_EXPORT kdu_params *new_instance();
	/* [SYNOPSIS]
		 Adds a new instance to the end of the current object's
		 instance list.  The current object need not be the head or the
		 tail of its instance list.
		 [//]
		 If the parameter class supports tile diversity, new instances
		 must not be added to objects with a negative tile index, except
		 in the event that the object was constructed with
		 `treat_instances_like_components' equal to true.  Similarly,
		 if the parameter class supports component diversity, new
		 instances must not be added to objects whith a negative component
		 index.  Finally, new instances may not be added at all if the
		 parameter class does not support instance diversity.  See
		 the `kdu_params' constructor for more on diversity flags.
		 [//]
		 The present function is primarily intended for adding instances
		 of a parameter class which correspond to successive tile-parts
		 of a particular tile.  Instance indices are simply incremented
		 as new instances are appended to the list.  If the object
		 was constructed with `treat_instances_like_components' equal
		 to true, however, you may well wish to add instances in non-linear
		 fashion.  For this purpose, you should use the `access_relation'
		 function.
	   [RETURNS]
		 A pointer to the newly created object, or NULL if a new instance
		 can or should not be created for any reason.
	*/
	KDU_EXPORT void
		copy_from(kdu_params *source, int source_tile, int target_tile,
			int instance = -1, int skip_components = 0,
			int discard_levels = 0, bool transpose = false,
			bool vflip = false, bool hflip = false);
	/* [SYNOPSIS]
		 This is a very powerful function, which is particularly useful in
		 implementing code-stream transcoders.  The function generates an
		 error unless the `source' object and the current object are
		 both cluster heads (i.e., they both must have tile and
		 component indices of -1).  The function locates all entries
		 within the source cluster which have the indicated `source_tile'
		 index and all entries within the current object's cluster which
		 have the `target_tile' index and copies them, skipping the
		 first `skip_components' codestream image components of the
		 source cluster, selecting instances on the basis of the
		 `instance' argument, and applying whatever transformations are
		 required (see below).
		 [//]
		 If the `source' and current objects are both the head of their
		 respective cluster lists, the function scans through all clusters
		 repeating the copying process on each one in turn.  Even if
		 one cluster does not have a tile which matches the `source_tile'
		 or `target_tile' arguments, the function continues to scan
		 through all the clusters.  If either or both of the `source'
		 and current objects is not the head of its list of clusters,
		 the function restricts its copying to just the one cluster.
		 Moreover, an error is generated if the clusters do not belong
		 to the same class.
		 [//]
		 If you wish to copy information from all tiles simultaneously,
		 you may use the `copy_all' function; in many applications, however,
		 it is useful to be able to copy information on a tile-by-tile
		 basis.  In particular, if the source parameters are being
		 discovered incrementally as a code-stream is being parsed (usually
		 the case for transcoding operations), the `copy_all' function will
		 not be appropriate, since information from some tile headers may
		 not yet be available.
		 [//]
		 If `instance' is -1, all instances of each tile-component are
		 copied.  Otherwise, only the instance whoses index is equal to
		 the value of the `instance' argument is copied.  If necessary,
		 new target instances will be created in this process.
		 [//]
		 Note carefully that no `kdu_params'-derived object will be copied
		 if the object has already had any record of any of its attributes
		 set by any means.  In this event, the object is skipped, but an
		 attempt is still made to copy any other objects which lie within
		 the scope of the call, following the rules outlined above.  The
		 intent is to copy source objects into newly constructed (and hence
		 empty) objects; however, the fact that individual objects will not
		 be overwritten can come in handy when copying large parameter
		 groupings, only some of which have already been set.
		 [//]
		 The actual copying operation is performed by the protected pure
		 virtual function, `copy_with_xforms', which must be implemented by
		 each derived class.  It is supplied the 5 arguments starting from
		 `skip_components', which may be used to modify the data as it is
		 being copied.
		 [//]
		 To ensure robust behaviour, you should be sure to ensure that
		 the source parameter objects have all been finalized before
		 invoking this function.  In most cases, you will not need to
		 worry about doing this, because any necessary finalization
		 steps are performed inside the `kdu_codestream' management system
		 when the parameters are recovered from a source codestream.
		 However, if you are filling in the source parameters yourself by
		 means of `parse_string' and/or `set' calls, you should be sure
		 to invoke `finalize_all' before calling this function.
		 [//]
		 It is important to realize that the source object corresponding
		 to a non-negative `source_tile' may be identical to the one
		 which is associated with `tile_idx'=-1 (main code-stream header
		 defaults).  Similarly, on entry, the target object corresponding
		 to a non-negative `target_tile' may also be identical to the one
		 which is associated with `tiel_idx'=-1.  When both these conditions
		 are true, the function does not copy anything for that object,
		 assuming that a consistent copying operation has already been
		 performed for the main codestream defaults (`tile_idx'=-1).  If
		 this is not true in your application, you can ensure that the
		 target objects are always copied explicitly (possibly wasting some
		 space).  To do this, simply invoke `access_relation' on each
		 target object of interest before calling the present function,
		 being sure to use a false value for the `read_only' argument
		 in the call to `access_relation'.  This will cause each
		 relevant target to be assigned its own unique object, rather
		 than re-using a main code-stream header default.  There should
		 be few if any interesting applications where you need to do this.
	   [ARG: source]
		 Cluster head from which to copy attributes in accordance with
		 the processing rules described above.
	   [ARG: source_tile]
		 Tile index which must be matched by any source objects which are
		 actually copied.  As usual, a value of -1 refers to
		 non-tile-specific source objects (i.e., those associated with
		 the main code-stream header, as opposed to tile headers).  See
		 the discussion of tile indices and defaults in the comments
		 appearing with the definition of the `kdu_params' object.
	   [ARG: target_tile]
		 Tile index which must be matched by target objects into which
		 data is copied -- same interpretation as `source_tile'.
	   [ARG: instance]
		 Instance index which must be matched by source and target objects
		 which are copied, except that a value of -1 matches all instances.
	   [ARG: skip_components]
		 Number of source image components to be skipped during copying.
		 Source objects with a component index of c+`skip_components' are
		 copied to target objects with a component index of c.  The
		 special component indices of -1 (associated with tile heads, as
		 described in the comments appearing with the declaration of
		 `kdu_params') are unaffected by this argument.
		 [//]
		 Note carefully, that the components referred to here are the
		 codestream image components, the number of which is identified by
		 the `Scomponents' attribute in `siz_params'.  If a Part-2
		 multi-component transform has been used, these components are
		 subject to further transformation before producing a final set
		 of MCT output components.  MCT output components are not affected
		 by component skipping, but the existing multi-component transform
		 will be automatically configured to treat discarded original
		 codestream components as 0.
	   [ARG: discard_levels]
		 Indicates the number of DWT resolution levels which are to be
		 discarded in a transcoding operation to produce a reduced
		 resolution version of a compressed image.
	   [ARG: transpose]
		 Indicates whether or not horizontal and vertical coordinates of
		 all dimensional and other related parameters are to be swapped
		 so as to describe a transposed image (rows become columns and
		 vice-versa).
	   [ARG: vflip]
		 If true, the target object will be adjusted to yield a
		 vertically flipped version of the source representation.
		 When `transpose' is used together with `vflip', the interpretation
		 is that the top-most row in the target representation should be
		 equivalent to the right-most column in the source representation.
		 That is, we think of first transposing the source and then
		 flipping it afterwards.
	   [ARG: hflip]
		 If true, the target object will be adjusted to yield a
		 horizontally flipped version of the source representation.
		 When `transpose' is used together with `hflip', the interpretation
		 is that the left-most target column should be equivalent
		 to the bottom-most source row.  That is, we think of first
		 transposing the source and then flipping it afterwards.
	 */
	KDU_EXPORT void
		copy_all(kdu_params *source, int skip_components = 0,
			int discard_levels = 0, bool transpose = false,
			bool vflip = false, bool hflip = false);
	/* [SYNOPSIS]
		 Same as invoking `copy_from' with tile indices from -1 through to
		 the number of tiles - 1, so as to copy all tile-specific and
		 non-specific coding parameters.  The various arguments have the
		 same interpretation as their namesakes in `copy_from'.
	*/
protected: // Specific transcoding members implemented by each derived class
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose,
		bool vflip, bool hflip) = 0;
	/* Each derived class must implement this pure virtual copy function.
	   It may fail with an error message if any of the requested
	   transformations cannot be implemented.  The last 5 arguments have
	   identical interpretations to those given in the description of
	   the `copy_from' function above. */
	   // --------------------------------------------------------------------------
public: // Navigation and identification member functions
	const char *identify_cluster() { return cluster_name; }
	/* [SYNOPSIS]
		 Returns the name of the cluster to which this object
		 belongs.  All instances of any particular derived parameter class
		 have the same cluster name.
	   [RETURNS]
		 Pointer to a constant string resource, which was used to construct
		 objects of this class.  The string is not copied and you may not
		 overwrite it.
	*/
	KDU_EXPORT kdu_params *
		access_cluster(const char *cluster_name);
	/* [SYNOPSIS]
		 Returns the cluster head of any cluster whose name
		 matches the `cluster_name' argument.  The function may be
		 invoked on any member of the cluster.
	   [RETURNS]
		 NULL if the `cluster_name' cannot be matched.
	   [ARG: cluster_name]
		 Relevant cluster names are given by macros which are defined
		 with the corresponding derived class definitions.  Check the
		 documentation for derived classes such as `siz_params',
		 `cod_params', `qcd_params', etc., for appropriate macro names.
		 A full string match is performed on cluster names so it is not
		 strictly necessary to use these macros.
	*/
	KDU_EXPORT kdu_params *
		access_cluster(int sequence_idx);
	/* [SYNOPSIS]
		 Returns a particular cluster head by its position in the
		 underlying cluster list. The function may be invoked on any
		 member of any cluster in the list.  The first
		 cluster has `sequence_idx'=0 and may also be retrieved by
		 supplying a NULL `cluster_name' argument.  This first cluster
		 will generally correspond to the `siz_params' class.
	   [RETURNS]
		 NULL if the requested cluster does not exist.
	   [ARG: sequence_idx]
		 0 for the first cluster in sequence.
	*/
	int get_instance() { return inst_idx; }
	/* [SYNOPSIS]
		 Returns the instance index of the current object within its
		 cluster.  As described in the comments appearing with the
		 declaration of `kdu_params', only a few parameter classes support
		 multiple instances, so this function will usually return 0.
	*/
	int get_num_comps() { return num_comps; }
	/* [SYNOPSIS]
		 Returns the number of non-negative component indices (0 through
		 `num_comps'-1) which are valid for objects in the present cluster.
		 This will generally be the number of components in the image,
		 unless the parameter class does not support component diversity,
		 in which case the returned value will be 0.  Remember that there
		 is always a special component with index -1, which is not counted
		 in the number of component returned here.
	*/
	int get_num_tiles() { return num_tiles; }
	/* [SYNOPSIS]
		 Returns the number of non-negative tile indices (0 through
		 `num_tiles'-1) which are valid for objects in the present cluster.
		 This will generally be the number of tiles in the image,
		 unless the parameter class does not support tile diversity,
		 in which case the returned value will be 0.  Remember that there
		 is always a special tile with index -1, which is not counted
		 in the number of tiles returned here.
	*/
	KDU_EXPORT kdu_params *
		access_relation(int tile_idx, int comp_idx, int inst_idx = 0,
			bool read_only = false);
	/* [SYNOPSIS]
		 Locates a particular object within the same cluster.  You
		 should be aware that there might not be a unique `kdu_params'
		 object for each valid tile and component index.
		 [//]
		 If the parameters for that object are identical to those for a
		 default object (one with a component and/or tile index of -1),
		 one of the following occurs:
		 [>>] If `read_only' is true, the function returns the appropriate
			  default object.  This is fine if you are only interested in
			  reading parameter values.  However, if you are accessing
			  the object with a view to changing its parameter values in
			  any way, you should set `read_only' to false.
		 [>>] If `read_only' is false, the function creates a unique
			  object for the indicated tile and component indices, if one
			  does not already exist.  If the newly created object has a
			  negative tile or component index, the function also updates
			  all relevant links, possibly creating unique objects for
			  other tile-components.  New instances will not be created
			  on demand unless the oject was constructed with the special
			  `treat_instances_like_components' argument set to true.
	   [RETURNS]
		 NULL if no object with the indicated tile, component and
		 instance indices exists in the same cluster as the current
		 object.  Note that new instances will not be created on demand,
		 even if `read_only' is set to false, unless the object was
		 constructed with the `treat_instances_like_components' argument
		 equal to true.
	   [ARG: tile_idx]
		 Tile index, starting from -1.
	   [ARG: comp_idx]
		 Component index, starting from -1.
	   [ARG: inst_idx]
		 Instance index, starting from 0.
	   [ARG: read_only]
		 If true, the function may return a pointer to an appropriate
		 default object (tile or component indices equal to -1) rather
		 than the one requested, so long as the default object has
		 exactly the same coding parameters.  You should not
		 modify the parameters of the returned object, though.
	*/
	KDU_EXPORT kdu_params *
		access_unique(int tile_idx, int comp_idx, int inst_idx = 0);
	/* [SYNOPSIS]
		 Same as `access_relation' except that if the requested
		 tile-component does not have its own unique object (i.e., if it
		 simply references a default object with tile or component index
		 = -1), the function returns NULL.  This is particularly useful
		 if you need to delete any objects which belong to a tile-component
		 which is no longer in use.
	*/
	kdu_params *
		access_next_inst() { return next_inst; }
	/* [SYNOPSIS]
		 Use this function to navigate to the next instance, if any, in
		 the current object's instance list.  If there are no more
		 instances, the function returns NULL.  This is useful primarily
		 for derived parameter classes which treat instances like
		 components.  In this case, random access to instances is possible
		 and the instance list might be sparse.  This function provides
		 an efficient way to navigate instance lists for this type of
		 derived parameter class (an example is the `ads_params' class).
	 */
	 // --------------------------------------------------------------------------
protected: // Assistance in constructing derived parameter classes
	KDU_EXPORT void
		define_attribute(const char *name, const char *comment,
			const char *pattern, int flags = 0);
	/* This function is called by the derived class's initializer to
	   define all recognized attributes for the relevant class.
	   `name' points to a string which may be used for textualizing
	   the attribute. Relevant attribute names are defined with the derived
	   classes toward the end of this file. `comment' provides a textual
	   description of the attribute which may be used in generating usage
	   statements. Valid `flags' are as follows: MULTI_RECORD means that
	   multiple records are allowed; CAN_EXTRAPOLATE means that records
	   can be accessed beyond those which have been actually written, with
	   the missing elements extrapolated from those which are available;
	   ALL_COMPONENTS means that this attribute does not have a
	   component-specific form and may only be set in an object whose
	   component index is -1. `pattern' identifies the structure of each
	   field. The string contains a concatenation of any or all of the
	   following:
		  * "F" -- a floating point field
		  * "I" -- an integer field
		  * "B" -- a boolean field (textualized as "yes/no")
		  * "(<string1>=<val>,<string2>=<val>,...)" -- an integer field,
			translated from one of the strings in the comma separated list
			of string/value pairs. Translation is case-sensitive.
		  * "[<string1>=<val>|<string2>=<val>|...]" -- as above, but multiple
			strings, sparated by "|" symbols, may be translated and their
			values OR'd together to form the integer value. This is useful
			for building flag words.
		  * "C" -- an integer field with custom textualized representation.
			Any such field could potentially be represented by the
			enumerated type, but custom parsing/textualization may be
			required if the number of possible values becomes too large.
			Custom parsing and textualization are performed by class-specific
			overrides of the `custom_parse_field' and
			`custom_textualize_field' member functions.
	*/
	static const int MULTI_RECORD;
	static const int CAN_EXTRAPOLATE;
	static const int ALL_COMPONENTS;
	// --------------------------------------------------------------------------
public: // Marker oriented attribute access functions
	KDU_EXPORT void
		clear_marks();
	/* [SYNOPSIS]
		 This function allows an existing parameter sub-system to be
		 re-used for efficiently compressing or decompressing multiple
		 code-streams (see `kdu_codestream::restart' for the principle
		 intended application).  It clears the marks which prevent
		 attributes from being changed once markers have been parsed or
		 generated.  It also clears the flags used to detect whether or
		 not any parameter attributes have been changed.
	*/
	KDU_EXPORT bool
		any_changes();
	/* [SYNOPSIS]
		 The return value from this function is the same for any object
		 which ultimately belongs to the same list of clusters.  The
		 function returns true if any attribute of any parameter object
		 has changed since the last call to `clear_marks', either by an
		 explicit call to `set', or by marker translation
		 (`translate_marker_segment') or string parsing (`parse_string').
		 [//]
		 This functionality is exploited primarily for efficient re-use of
		 constructed code-stream management machinery with video (see
		 `kdu_codestream::restart' for more on this).
		 [//]
		 The return value may be excessively conservative, returning true
		 even though it is possible that nothing might have changed.  In
		 particular, calls to `parse_string' implicitly invoke
		 `delete_unparsed_attribute', which will cause the present function
		 to return true if the attribute for which values are being parsed
		 was previously set by any other method.
	*/
	KDU_EXPORT bool
		check_typical_tile(int tile_idx, const char *excluded_clusters = NULL);
	/* [SYNOPSIS]
		 The return value from this function is the same for any object
		 which ultimately belongs to the same list of clusters.
		 [//]
		 This function provides an efficient way to check whether a tile
		 inherits all (or a relevant subset of) its coding parameters
		 from the main code-stream header.  If `excluded_clusters' is
		 NULL, the function returns true only if all parameters for the
		 identified tile are identical to those indicated in the main
		 header.  Otherwise, the `excluded_clusters' string must hold
		 a colon-separated list of cluster names, corresponding to those
		 clusters for which no checking will be performed.  All other
		 clusters must contain parameters which agree with the main
		 header.
		 [//]
		 In the interest of efficiency, the function might not perform
		 complex comparisons.  As a result, it might return false even
		 when the actual parameter values for a tile are actually
		 in agreement with those found in the main header.  The
		 current implementation explicitly tests attribute values
		 only where the number of values is at most 1.  Otherwise, it
		 considers that parameter values which are defined in a
		 tile-specific `kdu_params' object are likely to differ from
		 those of the main header.
		 [//]
		 The function is currently used by the codestream management
		 machinery to cache and re-use the structures constructed for
		 typical tiles when reading an existing code-stream.  This can
		 avoid some computational and memory manipulation overhead when
		 the tiles are very small, or when they are being decompressed
		 at a reduced resolution.
	*/
	KDU_EXPORT bool
		translate_marker_segment(kdu_uint16 code, int num_bytes,
			kdu_byte bytes[], int which_tile,
			int tpart_idx);
	/* [SYNOPSIS]
		 This function invokes the protected (and generally overridden)
		 `read_marker_segment' member function of every object whose tile
		 index agrees with `which_tile', in the network of code-stream
		 parameter objects to which the current object belongs, until one
		 is found which returns true.  Note that it does not make any
		 difference whether the current object is a cluster head or not.
		 In any event, all objects of all clusters are scanned, starting
		 from the first cluster (usually that associated with the derived
		 `siz_params' class).
		 [//]
		 The marker code and the number of segment bytes are
		 explicitly provided. The `bytes' array starts right after the
		 length indicator of the original marker segment and
		 `num_bytes' indicates the length of this array (this means that
		 `num_bytes' is always 2 bytes less than the length indicator in
		 the original code-stream marker segment, since the length indicator
		 includes its own length in JPEG2000).
	   [ARG: code]
		 The 16-bit marker code.  All valid JPEG2000 marker codes have
		 a most significant byte of 0xFF.  A set of macros defining useful
		 JPEG2000 marker codes may be found in the "kdu_compressed.h"
		 header file.
	   [ARG: num_bytes] Length of the `bytes' array.
	   [ARG: bytes] Array holding the body of the marker segment.
	   [ARG: which_tile]
		 Index of the tile to which the marker segment belongs, or -1 for
		 marker segments belonging to the main header.  The first tile in
		 the image has an index of 0.
	   [ARG: tpart_idx]
		 Identifies the tile-part in the code-stream from which the marker
		 segment originates.  This index has a value of 0 in the first
		 tile-part of each tile and for all marker segments in the main
		 header.
	   [RETURNS]
		 False unless the marker segment was translated by some object.
	*/
	KDU_EXPORT int
		generate_marker_segments(kdu_output *out, int which_tile, int tpart_idx);
	/* [SYNOPSIS]
		 This function invokes the protected (and generally overridden)
		 `write_marker_segment' member function of every object whose tile
		 index agrees with `which_tile', in the network of code-stream
		 parameter objects to which the current object belongs.  Note that
		 it does not make any difference whether the current object is a
		 cluster head or not.  In any event, all objects of all clusters
		 are scanned, starting from the first cluster (usually that
		 associated with the derived `siz_params' class).
		 [//]
		 Marker segments are written cluster by cluster, starting from the
		 tile head and working through each tile-component in turn.
		 [//]
		 You can simulate the marker generation process by supplying a
		 NULL value for the `out' argument.  This is useful, since the
		 function returns the total number of bytes which would have been
		 generated, regardless of whether `out' is NULL.
	   [RETURNS]
		 The total number of bytes written (or simulated), including the
		 marker codes and segment bytes for all marker segments.
	   [ARG: out] If NULL, the marker generation process is only simulated
		 -- nothing is written, but the return value represents the number of
		 bytes which would be written out if `out' had not been NULL.
	   [ARG: which_tile]
		 Index of the tile for which marker segments are to be generated
		 (or simulated).  A value of -1 causes marker segments to be
		 generated for the main header, while the first tile in
		 the image has an index of 0.
	   [ARG: tpart_idx]
		 Indicates the tile-part whose header is being written.
		 A value of 0 refers to the first tile-part of the tile.  A
		 value of 0 is also required for the main header (i.e., when
		 `which_tile' = -1).
	*/
protected: // Individual marker translators
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx)
	{
		return 0;
	}
	/* This function should be overridden in any derived class which
	   is capable of writing its contents into a code-stream marker segment.
	   The function is called from within `generate_marker_segments', which
	   also provides the `last_marked' pointer. This pointer may be safely
	   cast to a pointer to the derived object. If not NULL, the function
	   can determine whether or not there is a need to write a marker segment
	   for this object by comparing the contents of the last marked object
	   with those of the current object. If there is no difference, no marker
	   segment need be written. In practice, `last_marked' is the most recent
	   object in the current object's inheritance chain for which a marker
	   segment was generated (return value not equal to zero). The function
	   returns the total number of bytes occupied by the marker segment,
	   including the marker code and the segment length field. If `stream'
	   is NULL, the process is only simulated -- the length should be
	   returned correctly, but nothing is actually written.  The `tpart_idx'
	   field identifies the tile-part (starting from 0) into which the
	   marker segment is being written. */
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx) = 0;
	/* This function should be implemented in each derived class.  It is
	   called from within the public `translate_marker_segment' method
	   to determine whether or not the `code' corresponds to a valid
	   marker for the present parameter class, and also to determine
	   what component index the marker segment belongs to.  If the
	   object was constructed with the `treat_instances_like_components'
	   argument equal to true, the component index returned via `c_idx'
	   should be interpreted as an instance index by the caller.
	   [//]
	   The function should return false if the `code' does not apply to the
	   current derived parameter class.  Otherwise, it should return true,
	   unless some other parsing error occurred.  Parsing proceeds only so
	   far as is required to determine the component (or instance) index.
	   For some types of marker segments, component diversity may not
	   be supported.  If this is true, and instance diversity (treated
	   like components) is also not supported, the function has no need
	   to parse into the `bytes' array.
	   [//]
	   Note that the `bytes' array holds the data which follows the marker
	   length field in the complete codestream marker segment. */
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx) = 0;
	/* This function should be implemented in each derived class. The
	   function is called from within the public `translate_marker_segment'
	   method, once it has determined that a marker segment is relevant (by
	   calling `check_marker_segment') and created the `kdu_params'
	   object, if necessary.  The function should generally return true
	   unless an incompatibility problem is encountered.
		  The marker code is supplied, and the `bytes' array holds the data
	   which follows the marker length field in the complete code-stream
	   marker segment.  `num_bytes' indicates the number of bytes in the
	   `bytes' array.  Thus, `num_bytes' is always 2 bytes less than the
	   marker length field in the complete code-stream marker segment.
		  Note that when a marker segment is translated by an object which
	   supports multiple instances, a new instance will automatically be
	   created to translate the contents of any future marker segments for
	   the same tile-component.  The `tpart_idx' field indicates the
	   tile-part from whose header the marker segment is being read.  The
	   first tile-part has an index of 0. */
	   // --------------------------------------------------------------------------
public: // Binary attribute access functions
	KDU_EXPORT bool
		get(const char *name, int record_idx, int field_idx, int &value,
			bool allow_inherit = true, bool allow_extend = true,
			bool allow_derived = true);
	/* [SYNOPSIS]
		 Retrieves a single integer (or integer compatible) parameter
		 (field) from the named attribute.
		 [//]
		 The parameter values managed by any given attribute are organized
		 as an array of records, each of which has an identical structure,
		 defined by fields.  Fields are typed and each field in the
		 record may have a different type; however, the type of each field
		 is immutable after configuration by the relevant parameter class's
		 constructor.  Simple field types are as follows: I (integer);
		 B (boolean); and F (float).  The attribute definition language
		 used by parameter class constructors also supports enumerated
		 and flag field types, both of which have integer values for the
		 purpose of direct function queries.  The enumerated type can
		 take on only one of a number of pre-defined values, each of which
		 has a textual identifier for use in parsing and textualization.
		 The flag type represents a logical OR of any or all of a set of
		 bit flags, each of which has a pre-defined value and associated
		 textual identifier.
		 [//]
		 The present function may be used to query field values which may
		 be represented as integers.  This includes boolean fields, for
		 which a value of 0 means false and a value of 1 means true;
		 however, it is recommended that you use the explicit boolean
		 form of this function for querying boolean valued fields.
		 [//]
		 The function generates an error (through `kdu_error') if the request
		 is incompatible with the record structure defined for this
		 attribute.  For example, the attribute might not support multiple
		 records, it might not define sufficient fields, or the identified
		 field might not have an integer type.
		 [//]
		 If the request is legal, but the relevant field has simply not
		 been set, the function may return false.  Before doing so, however,
		 it applies a set of synthesis rules which may be used to synthesize
		 the required value from available information.  These synthesis
		 rules form an essential role in effectively managing the vast
		 number of coding parameters which could potentially be specified
		 as part of a legal JPEG2000 code-stream.  It is important to note
		 that the `allow_inherit' and `allow_extend' arguments may be used
		 to override certain aspects of the synthesis rules.  We now
		 describe the synthesis rules themselves.
		 [//]
		 If one or more values have already been supplied for the attribute,
		 it may be possible to extrapolate these to satisfy a request for
		 which data is not available.  Extrapolation is permitted only
		 if the `allow_extend' argument is true (the default) and the
		 parameter class's constructor supplied the CAN_EXTRAPOLATE flag
		 when defining this attribute (calling the protected
		 `define_attribute' function).  Extrapolation applies only when
		 the requested record lies beyond the last record for which any
		 field values have been supplied, in which case, the function
		 returns the value of the indicated field from the last available
		 record.  If the field has not been defined in that record, the
		 function returns false; no further synthesis is attempted.
		 [//]
		 If the attribute is empty (no field values have been supplied
		 for any records), the request may be passed on to another object
		 in the same cluster, following the inheritance rules outlined in
		 the discussion appearing with the declaration of `kdu_params'.
		 In particular, the request is first passed on to the tile head
		 (object with the same tile index, but a component index of -1),
		 if one exists -- this object manages tile defaults for the
		 cluster.  If that object is also unable to satisfy the
		 request, it is passed on to the object which has the same
		 component index, but a tile index of -1 -- this object manages
		 component defaults for the cluster.  If both of these fail, the
		 request is forwarded to the cluster head (has both tile and
		 component indices of -1).  These inheritance rules apply only
		 if the `allow_inherit' argument is true (the default) and only
		 if the current object is the head of its instance list (instance
		 index of 0).
		 [//]
		 For non-primary instances, inheritance is not normally available.
		 However, some types of marker segments defined by Part 2 of the
		 JPEG2000 standard define an inheritance relationship which is
		 similar to that defined for components, even though there are
		 not component-specific forms.  This is most easily handled by
		 treating the instance index as though it were a component index
		 for the purpose of inheritance.  Objects which are to be treated
		 in this way will have been constructed with the special
		 `treat_instances_like_components' argument set to true.
	   [RETURNS]
		 False if the requested value has not yet been set and it cannot
		 be synthesized from information available within the same cluster.
		 Synthesis rules are described above.
	   [ARG: name]
		 Name of the attribute, from which a single parameter value is
		 being queried.  Attribute names are unique across all
		 clusters.  If necessary, a full string match is performed on the
		 attribute name, but you would do well to use one of the pre-defined
		 macros for the relevant derived parameter class.  These are
		 documented with the derived classes themselves.
	   [ARG: record_idx] 0 for the first record.
	   [ARG: field_idx] 0 for the first field in the record.
	   [ARG: value]
		 Used to return the requested value.  Affected only if the
		 function returns true.
	   [ARG: allow_inherit]
		 If true, the request may be deferred to any other object
		 in the cluster, following the standard inheritance rules.
		 Otherwise, the request may be satisfied only from information in
		 the current object.
	   [ARG: allow_extend]
		 If true, the request may be satisfied by extrapolating from the
		 last available record, if that record has an index less than
		 `record_idx' and it contains a value for the requested field.
	   [ARG: allow_derived]
		  If false, the function will ignore any attributes which have been
		  marked as holding derived quantities through calls to the
		  `set_derived' member function.  This is useful when you want to
		  inherit attributes from objects in the same cluster, bypassing
		  any intermediate objects for which values might have been
		  automatically derived.  This capability is most useful when
		  implementing `finalize' functions for specific parameter classes;
		  these functions may have to do quite a bit of work to come up
		  with appropriate default values where required parameter
		  attributes have not been explicitly supplied.
	*/
	KDU_EXPORT bool
		get(const char *name, int record_idx, int field_idx, bool &value,
			bool allow_inherit = true, bool allow_extend = true,
			bool allow_derived = true);
	/* [SYNOPSIS]
		 Retrieves a single boolean-valued parameter (field) from the
		 named attribute.  Boolean values may also be retrieved as
		 integers, using the first form of the `get' function.
		 [//]
		 For a thorough description of records, fields, inheritance
		 and extension rules and field typing, consult the description
		 of the first form of the overloaded `get' function.
	*/
	KDU_EXPORT bool
		get(const char *name, int record_idx, int field_idx, float &value,
			bool allow_inherit = true, bool allow_extend = true,
			bool allow_derived = true);
	/* [SYNOPSIS]
		 Retrieves a single real-valued parameter (field) from the
		 named attribute.
		 [//]
		 For a thorough description of records, fields, inheritance
		 and extension rules and field typing, consult the description
		 of the first form of the overloaded `get' function.
	*/
	bool compare(const char *name, int record_idx, int field_idx, int value)
	{
		int val;
		return (get(name, record_idx, field_idx, val) && (val == value));
	}
	/* [SYNOPSIS]
		 Convenience inlined function which collapses a call to `get'
		 with a comparison of the result with the supplied integer
		 `value'.  The function substantially simplifies the process
		 of writing code-stream marker segments, which can often be
		 avoided if all attributes are identical to those of another
		 object in the inheritance chain of the same cluster.
	   [RETURNS]
		 False if the call to `get' fails (returns false), or the call
		 to `get' succeeds, but the retrieved value is not identical to
		 the `value' argument.
	   [ARG: name] Passed through to `get'.
	   [ARG: record_idx] Passed through to `get'.
	   [ARG: field_idx] Passed through to `get'.
	   [ARG: value] The value to be compared.
	*/
	bool compare(const char *name, int record_idx, int field_idx, bool value)
	{
		bool val;
		return (get(name, record_idx, field_idx, val) && (val == value));
	}
	/* [SYNOPSIS]
		 Same as the integer version of the `compare' function, except that
		 it compares a boolean value with a boolean attribute field. */
	bool compare(const char *name, int record_idx, int field_idx, float value)
	{
		float val;
		return (get(name, record_idx, field_idx, val) && (val == value));
	}
	/* [SYNOPSIS]
		 Same as the integer version of the `compare' function, except that
		 it compares a floating point value with a floating point
		 attribute field. */
	KDU_EXPORT void
		set(const char *name, int record_idx, int field_idx, int value);
	/* [SYNOPSIS]
		 Sets the value of a single parameter (field) in the
		 named attribute, having the I (integer) type (or an
		 integer-compatible) type.  It is legal to overwrite
		 existing values.
		 [//]
		 The parameter values managed by any given attribute are organized
		 as an array of records, each of which has an identical structure,
		 defined by fields.  Fields are typed and each field in the
		 record may have a different type; however, the type of each field
		 is immutable after configuration by the relevant parameter class's
		 constructor.  Simple field types are as follows: I (integer);
		 B (boolean); and F (float).  The attribute definition language
		 used by parameter class constructors also supports enumerated
		 and flag field types, both of which have integer values for the
		 purpose of direct function queries.  The enumerated type can
		 take on only one of a number of pre-defined values, each of which
		 has a textual identifier for use in parsing and textualization.
		 The flag type represents a logical OR of any or all of a set of
		 bit flags, each of which has a pre-defined value and associated
		 textual identifier.
		 [//]
		 The present function may be used to set any field value which may
		 be represented as an integer.  This includes the I (integer) type,
		 as well as the enumerated and flag types.  In the latter cases,
		 however, a check is made to verify that the supplied integer is
		 compatible with legal enumerated values or flag bits.  The
		 function may also be used to set B (boolean) field types,
		 although this is not recommended (you should really use the
		 boolean form of the overloaded `set' function).  If the function
		 is used to set booleans, a check will be made to verify that the
		 integer is either 0 (false) or 1 (true).
		 [//]
		 The function generates an error (through `kdu_error') if the request
		 is incompatible with the record structure defined for this
		 attribute.  For example, the attribute might not support multiple
		 records, it might not define sufficient fields, or the identified
		 field might not be compatible with the supplied integer value,
		 as described above.
		 [//]
		 It is not strictly necessary to set all fields in a record before
		 moving on to another record; nor is it strictly necessary to set
		 records in sequence.  However, most reasonable applications will
		 expect to see a consecutive set of completed records eventually
		 and other methods for internalizing attribute values such as
		 string parsing and marker segment translation will also fill out
		 completed records in sequence.
	   [ARG: name]
		 Name of the attribute, in which the identified field is to be
		 set.  Attribute names are unique across all
		 clusters.  If necessary, a full string match is performed on the
		 attribute name, but you would do well to use one of the pre-defined
		 macros for the relevant derived parameter class.  These are
		 documented with the derived classes themselves.
	   [ARG: record_idx] 0 for the first record.
	   [ARG: field_idx] 0 for the first field in the record.
	   [ARG: value] The integer value to which the field should be set.
		 Note that some types of fields have limited legal numeric ranges,
		 in which case the function will check them, generating any
		 appropriate error messages through `kdu_error'.
	*/
	KDU_EXPORT void
		set(const char *name, int record_idx, int field_idx, bool value);
	/* [SYNOPSIS]
		 Sets the value of a single parameter (field) in the
		 named attribute, having the B (boolean) type.
		 [//]
		 For a thorough description of records, fields and field typing
		 conventions, consult the description of the first form of this
		 overloaded `set' function. */
	KDU_EXPORT void
		set(const char *name, int record_idx, int field_idx, double value);
	/* [SYNOPSIS]
		 Sets the value of a single parameter (field) in the
		 named attribute, having the F (floating point) type.
		 [//]
		 For a thorough description of records, fields and field typing
		 conventions, consult the description of the first form of this
		 overloaded `set' function. */
	KDU_EXPORT void
		set_derived(const char *name);
	/* [SYNOPSIS]
		 Marks the attribute identified by `name' as holding automatically
		 derived (generated, or synthesized) data which might not be
		 treated in the same way as original data, explicitly specified
		 by an application or user.  In particular, derived attributes
		 might be skipped in the inheritance chain used by the `get'
		 member function, if its `allow_derived' argument is false.  Also,
		 derived attributes can be skipped during textualization if
		 the `textualize_attributes' function's `skip_derived' argument is
		 true.
		 [ARG: name]
		 Attribute names are unique across all clusters.  If necessary, a
		 full string match is performed on the attribute name, but you
		 would do well to use one of the pre-defined macros for the
		 relevant derived parameter class.  These are documented with the
		 derived classes themselves.
	*/
	// --------------------------------------------------------------------------
public: // String oriented attribute access functions
	KDU_EXPORT bool
		parse_string(const char *string);
	/* [SYNOPSIS]
		 Parses parameter values for a single attribute from the supplied
		 string.  The string may not contain any white space of any form
		 at all.  In fact, an error message will generally be produced
		 (through `kdu_error') if this requirement is violated.
		 [//]
		 The syntax of the supplied string may be expressed as
		 "<name>[:<spec>]=<vals>" where "<name>" stands for the name of
		 a paricular attribute, "[...]" denotes an optional sub-string,
		 "<spec>" stands for a tile-, component- or tile-component
		 specifier which may be used to specify the particular object in
		 a cluster in which the attribute is to be set, and "<vals>"
		 stands for a construction which specifies values for the
		 attribute's fields.
		 [//]
		 The function examines the available attributes in the current
		 object, for one whose name matches the "<name>" portion of the
		 supplied string -- i.e., the portion of the string which is
		 delimited by an "=" or a ":" character.  If the match fails, and
		 the current object is also the cluster head for the very first
		 cluster in the network of parameter objects describing a
		 particular image (this will generally be a `siz_params' object
		 in practice), the function proceeds to scan through each cluster
		 in turn, looking for attributes whose name matches the "<name>"
		 prefix of `string'.  Note that this examination of multiple
		 clusters occurs only if the current object is the head of the
		 first cluster.  If this process does not turn up a matching
		 attribute in some appropriate cluster, the function returns false.
		 [//]
		 Once a match is found, the function checks the optional "<spec>"
		 field in the supplied `string'.  This field may contain a
		 tile-specifier of the form "T<num>", a component-specifier of the
		 form "C<num>", or both (in either order).  For certain special
		 parameter classes, a general index number, "I<num>" may be
		 supplied in place of a "C<num>" specifier.  these are the
		 parameter classes which were constructed with the
		 `treat_instances_as_components' argument equal to true.
		 Tile, component and index numbers start from 0.  If a tile- or
		 component-specifier is disallowed by the relevant parameter class,
		 or does not correspond to a valid object, the function generates
		 an appropriate error message through `kdu_error'.  A missing
		 component-specifier means that the string is associated with the
		 tile head object (the one having a component index of -1).
		 Similarly, a missing tile-specifier means that the string is
		 associated with an object with tile index of -1.  If no tile- or
		 component-specifier is supplied, the string refers to the first
		 object in which the attribute name was matched, which is either
		 the current object or a cluster head.  Remember that cluster heads
		 contain default coding parameters for use in all tile-components
		 of the image except those in which the parameters are explicitly
		 overridden.  For more on tiles, components and clusters, consult
		 the comments appearing with the declaration of `kdu_params'.
		 [//]
		 As noted above, the string concludes with parameter values for
		 the relevant attribute; the relevant text immediately follows the
		 "=" character.  This "<vals>" portion is structured as a
		 comma-separated list of records, where each record contains one
		 or more fields.  Note that multiple records are disallowed unless
		 the MULTI_RECORD flag was supplied when the relevant attribute
		 was defined -- i.e., when the relevant parameter class's
		 constructor called the protected `define_attribute' function.
		 [//]
		 Within the "<vals>" sub-string, each record consists of one or more
		 fields, surrounded by curly braces.  In the special case where
		 there is only one field per record, the braces may optionally
		 be omitted as a convenience.  The fields must match the type
		 specifications provided in the `pattern' string which was
		 supplied to the protected `define_attribute' function when
		 the attribute was defined.  Any failure to obey these rules will
		 cause an informative error message to be generated through
		 `kdu_error'.  Specifically, the following patterns are available
		 (the specific patterns to which any particular attributes fields
		 must conform may be queried by a call to `describe_attribute' or
		 `describe_attributes' if desired.)
		 [//]
		 In the field description pattern: an "I" refers to an integer
		 field; an "F" refers to a floating point value; a "B" refers to
		 a boolean field, taking values of "yes" and "no"; a pattern of
		 the form "(<string1>=<val1>,<string2>=<val2>,...)" refers to an
		 enumerated field, taking one of the values, "<string1>",
		 "<string2>", ..., which are translated into the corresponding
		 integers, "<val1>", "<val2>", ...., respectively; and a pattern
		 of the form "[<string1>=<val1>|<string2>=<val2>|...]" refers to a
		 flags field, taking zero or more of the values, "<string1>",
		 "<string2>", ..., separated by vertical bars, "|", which are
		 translated into corresponding integers, "<val1>", "<val2>", ...
		 and OR'd together.
		 [//]
		 As an example, the pattern "IIB(go=1,stop=0)" describes a
		 record structure consisting of 4 fields: 2 integer fields; one
		 boolean field; and one enumerated field which must hold one of
		 the strings "go" or "stop".  An example of a valid "<vals>"
		 suffix suffix for the `string' argument setting such an attribute
		 would be "{56,-4,yes,stop},{0,1,no,go}".  You will find the
		 pattern strings for each attribute described in the comments
		 appearing with each of the derived parameter classes (e.g.,
		 `siz_params', `cod_params', `qcd_params', etc.).  Alternatively,
		 you may query the patterns at run-time by calling
		 `describe_attribute' or `describe_attributes', as already
		 mentioned.
		 [//]
		 In several respects, attribute parsing is unlike the other two
		 methods provided for setting parameter attributes (i.e., the `set'
		 function and marker segment translation via the
		 `translate_marker_segment' function).  Firstly, it is illegal to
		 parse the same attribute twice -- an appropriate error message
		 will be generated through `kdu_error' if information has already
		 been parsed into the attribute (but not if it has been set using
		 `set' or `translate_marker_segment').  The only exception to this
		 rule applies to objects which support multiple instances, in which
		 case a new instance of the object will be created automatically to
		 accommodate multiple occurrences of the same attribute string.
		 [//]
		 New instances will not be created if previous values
		 were set into an attribute by any means other than string parsing.
		 Quite to the contrary, any information set by any other means is
		 automatically deleted before parsing new information into an
		 attribute.  More than that, if the attribute string does not
		 specify a particular component, all values for the relevant
		 attribute will be cleared in all component-specific objects which
		 do not already contain parsed information for that attribute.
		 Similarly, if the attribute string does not specify a particular
		 tile, that attribute's values will be cleared in all tile-component
		 specific objects which do not already contain parsed information
		 for the attribute.  This functionality is intended to reflect the
		 most reasonable behaviour for transcoding applications, in which
		 a user may modify some parameter attributes of an existing
		 code-stream; it would be disconcerting to find that tile-
		 or component-specific versions of those same attributes were
		 left intact by a non-specific modification command.
	   [RETURNS]
		 False if no attribute whose name matches that indicated in the
		 "<name>" part of `string' can be found in the current object or
		 (if the current object is the head of the first cluster) in
		 any of the clusters in the cluster list.
	   [ARG: string]
		 A string with the form, "<name>[:<spec>]=<vals>" where "<name>"
		 stands for the name a parameter attribute, "[...]" refers to
		 an optional sub-string, "<spec>" refers to an optional
		 element which specifies a specific tile, component or
		 both, and "<vals>" contains a comma-separated list of records,
		 each containing a comma-separated list of fields, enclosed within
		 braces.  For a thorough description, see the main text of the
		 function description, above.  Examples of some common code-stream
		 parameter attribute strings are:
		 [>>] "Clevels={5}"  [may be abbreviated as "Clevels=5"]
		 [>>] "Sdims={201,311},{22,56}" [sets dimensions for two image
			  components]
		 [>>] "Porder:T1={0,0,2,10,10,LRCP},{0,0,4,10,10,PCRL}"
	*/
	KDU_EXPORT bool
		parse_string(const char *string, int tile_idx);
	/* [SYNOPSIS]
		 This function is identical to the first form of the `parse_string'
		 function, except that it parses only those strings which refer
		 to the tile indicated by `tile_idx', returning false if an
		 object with that tile index would not have a parameter attribute
		 set in a call to the first, more generic `parse_string' function.
		 As usual, a tile index of -1 refers to the summary object for all
		 tiles.
		 [//]
		 This version of the function is useful in transcoding applications,
		 where original code-stream parameters are discovered incrementally
		 as we recover each successive tile from an original code-stream.
		 In this case, it is helpful to delay parsing any strings which
		 may modify those tile-specific attributes until after the
		 relevant marker segments have been encountered in the original
		 code-stream.
	*/
	KDU_EXPORT void
		textualize_attributes(kdu_message &output, bool skip_derived = true);
	/* [SYNOPSIS]
		 Textualizes all attributes for which information has been written,
		 using the same format as described for `parse_string' and
		 writing the result out to the `output' object.
	   [ARG: output] Destination for the generated text.
	   [ARG: skip_derived] If true, attributes marked as automatically
		 generated (by previous calls to `set_derived') will not
		 be textualized.
	*/
	KDU_EXPORT void
		textualize_attributes(kdu_message &output, int min_tile, int max_tile,
			bool skip_derived = true);
	/* [SYNOPSIS]
		 Same as the first form of this overloaded `textualize_attributes'
		 function, with two important differences: 1) only objects whose
		 tile indices lie between `min_tile' and `max_tile' (inclusive) have
		 their attributes textualized; and 2) the function textualizes all
		 objects which lie within any list or scope which is headed by the
		 current object.  If the object on which this function is invoked is
		 an instance head, all instances for that the relevant
		 tile-component have their attributes textualized.  Similarly, if
		 the object is a tile head (component index of -1), all
		 tile-components for that tile and all of their respective instances
		 have their attributes textualized.  If the object is a cluster head,
		 (tile and component indices both negative), all objects in the
		 cluster which conform to the supplied tile bounds will have their
		 attributes textualized.  Finally, if the object is the head
		 of the first cluster in the network of code-stream parameters,
		 all objects with conforming tile indices, in all clusters have
		 their attributes textualized.
	   [ARG: output] Destination for the generated text.
	   [ARG: min_tile] First tile (inclusive) for which textualization is
		 to be performed.  May be -1.
	   [ARG: max_tile] Last tile (inclusive) for which textualization is
		 to be performed.
	   [ARG: skip_derived] If true, attributes marked as automatically
		 generated (by previous calls to `set_derived') will not
		 be textualized.
	*/
	KDU_EXPORT void
		describe_attributes(kdu_message &output, bool include_comments = true);
	/* [SYNOPSIS]
		 Generates a text description of the record structure defined for
		 all attributes associated with the current object.  This is
		 principally intended for printing usage statements or generating
		 descriptive text for interactive windows.  However, the function
		 could be used by an interactive dialog generator to configure
		 parameter entry dialogs.
		 [//]
		 For a discussion of records, fields, typing and the pattern
		 strings used to describe record structures, consult the comments
		 appearing with the declaration of `parse_string'.
	   [ARG: output] Destination for the generated text.
	   [ARG: include_comments] If true, descriptive comments
		 will also be generated for each attribute.  Otherwise, only the
		 record structure will be generated. */
	KDU_EXPORT void
		describe_attribute(const char *name, kdu_message &output,
			bool include_comments = true);
	/* [SYNOPSIS]
		 Same as `describe_attributes' except that only one attribute's
		 record structure will be described.  An appropriate error message
		 will be generated (through `kdu_error') if the supplied `name'
		 does not match any of the object's attributes.
	   [ARG: name]
		 If necessary, a full string match is performed on the attribute
		 name, but you would do well to use one of the pre-defined macros
		 for the relevant derived parameter class.  These are documented
		 with the derived classes themselves.
	   [ARG: output] As for `describe_attributes'.
	   [ARG: include_comments] As for `describe_attributes'.
	*/
	KDU_EXPORT kdu_params *
		find_string(char *string, const char * &name);
	/* [SYNOPSIS]
		 Finds the object which would be used by `parse_string' if parsing
		 the supplied `string', returning a pointer to that object, if it
		 exists.  Follows the same searching rules as `parse_string'.
	   [RETURNS]
		 NULL if a match is not found.
	   [ARG: name]
		 Used to return the attribute name string (not decorated with
		 tile- or component- specifiers) associated with the match, if any.
		 If the function returns NULL, this argument will be unaffected;
		 otherwise, it points to a constant string resource, which should
		 not be overwritten.
	  */
	KDU_EXPORT void
		delete_unparsed_attribute(const char *name);
	/* [SYNOPSIS]
		 Clears all entries for the attribute with the indicated `name'
		 in the current object.  Does the same for all objects in any
		 list (or scope) which is headed by the current object.  If it is
		 an instance head, the function clears the parameter attribute in
		 all objects of the corresponding instance list.  If the current
		 object is a tile head (component index of -1), the function does
		 the same for all objects of the cluster with the same tile index.
		 If the current object is the cluster head, all entries for the
		 indicated attribute are cleared in all objects in the cluster.
		 Does not examine different clusters to the one within which the
		 function is called.
		 [//]
		 Importantly, this object will not touch attributes whose values
		 have been parsed in using the `parse_string' member function --
		 such attributes are considered special for a variety of practical
		 reasons.
		 [//]
		 An appropriate error message is generated (through `kdu_error')
		 if there is no attribute with the indicated name in the current
		 object.
	*/
	virtual int
		custom_parse_field(const char *string, const char *name,
			int field_idx, int &val)
	{
		return 0;
	}
	/* [SYNOPSIS]
		 This function is called when `parse_string' encounters an
		 attribute which was assigned the `C' (custom) type identifier
		 by the `define_attribute' function.  In this case, the attribute
		 has a complex textual representation, which cannot conveniently
		 be described using the generic enumerated type.  Any meaningful
		 implementation of this function must be provided by the specific
		 `kdu_params'-derived object in question.
	   [RETURNS]
		 If the string cannot be translated into an integer value, the
		 function should return 0.  Otherwise, it returns the number
		 of characters actually parsed, placing the translated value in
		 `val'.
	   [ARG: string]
		 Custom text string to be parsed.
	   [ARG: name]
		 Identifies the attribute name, e.g., `Cdecomp'.
	   [ARG: field_idx]
		 Identifies which field in a record is actually being translated.
		 A value of 0 refers to the first field.
	   [ARG: val]
		 Receives the translated value.
	*/
	virtual void
		custom_textualize_field(kdu_message &output, const char *name,
			int field_idx, int val)
	{
		return;
	}
	/* [SYNOPSIS]
		 This function is used to textualize parameter attributes which
		 were assigned the `C' (custom) type identifier by the
		 `define_attribute' function.  In this case, the attribute
		 has a complex textual representation, which cannot conveniently
		 be described using the generic enumerated type.  Any meaningful
		 implementation of this function must be provided by the
		 specific `kdu_params'-derived object in question.
	   [ARG: output]
		 Location to which the translated text is written.
	   [ARG: name]
		 Identifies the attribute name, e.g., `Cdecomp'.
	   [ARG: field_idx]
		 Identifies which field in a record is actually being translated.
		 A value of 0 refers to the first field.
	   [ARG: val]
		 The value to be translated into a text string.
	*/
	// --------------------------------------------------------------------------
public: // Finalization functions
	virtual void finalize(bool after_reading = false) { return; }
	/* [SYNOPSIS]
		 This function is generally invoked through the `finalize_all'
		 function.  Although it can usually be invoked quite safely when
		 code-stream marker segments are used to recover attributes, it is
		 intended primarily for use when attributes are derived by parsing
		 text strings supplied as command-line arguments.  Since the
		 information provided in this manner is quite sparse, it is
		 frequently necessary to fill in default values or convert summary
		 quantities into a more complete set of attribute values, as
		 required to generate a valid set of marker segments or to
		 perform compression.
		 [//]
		 The implementation in the base object does nothing at all;
		 it should be overridden in any derived class where finalization
		 processing is required.  Most of the derived parameter classes
		 do this.
	*/
	KDU_EXPORT void
		finalize_all(bool after_reading = false);
	/* [SYNOPSIS]
		 This function invokes the `finalize' member function of the current
		 object.  If the object is an instance head, all objects in the
		 instance list are finalized.  If the object has a component index
		 of -1, all objects with the same tile index are also finalized.  If
		 it has a tile index of -1, all objects with the same component
		 index are finalized.  If it is a cluster head, all objects in the
		 same cluster are finalized, and if it is the head of the list of
		 clusters, all objects in all clusters are finalized.
	   [ARG: after_reading]
		 If true, the finalization process is being performed on parameters
		 which were obtained by reading codestream marker segments.  This
		 can affect the behaviour of individual derived `finalize'
		 functions, and also allows them to minimize their effort, since
		 complex finalization is often not required in this case.  The
		 function is invoked with this argument equal to true from within
		 the codestream management system -- you would not normally
		 invoke it in this way directly from within an application.
	*/
	KDU_EXPORT void
		finalize_all(int tile_idx, bool after_reading = false);
	/* [SYNOPSIS]
		 Same as the first form of the overloaded `finalize_all' function,
		 except that only those objects whose tile index agrees with
		 the supplied argument are actually finalized.  This is useful
		 when information becomes available on a tile-by-tile basis, as
		 can happen when transcoding an existing code-stream.
	   [ARG: tile_idx]
		 A value of -1 refers to the objects in the main header, while
		 tile-specific objects to be finalized have tile indices starting
		 from 0.
	   [ARG: after_reading]
		 If true, the finalization process is being performed on parameters
		 which were obtained by reading codestream marker segments.  This
		 can affect the behaviour of individual derived `finalize'
		 functions, and also allows them to minimize their effort, since
		 complex finalization is often not required in this case.  The
		 function is invoked with this argument equal to true from within
		 the codestream management system -- you would not normally
		 invoke it in this way directly from within an application.
	*/
	// --------------------------------------------------------------------------
protected: // Data members shared with derived classes
	const char *cluster_name; // Identitifies the derived class type.
	int tile_idx, comp_idx, inst_idx; // Object coordinates within cluster
	int num_tiles; // Num non-initial tiles in current cluster.
	int num_comps; // Num non-initial tile-components in current tile.
	bool empty; // True until the first attempt to set any attribute records.
	bool marked; /* Becomes true when the contents of this object
					are written to or read from a code-stream marker. */
					// --------------------------------------------------------------------------
private: // Structural data members
	bool allow_tiles, allow_comps, allow_insts, force_comps;
	bool treat_instances_like_components; // See constructor for explanation
	kdu_params *first_cluster, *next_cluster; /* Links clusters to one another.
						  Links are valid only for the first object in each
						  cluster (cluster heads). Otherwise, they are NULL. */
	kdu_params **refs; /* Array of references to all instance list heads in
		the current cluster.  For the purpose of deallocation, the array
		is understood to be owned by the cluster head (the object with
		`tile_idx'=-1 and `comp_idx'=-1).  All other elements in the
		cluster have the same value for the `refs' pointer.  The array
		has `num_tiles+1' rows, each with `num_comps'+1 columns.  Entries
		might not be unique, since if a tile-component is identical to its
		tile-head (`comp_idx'=-1) or its cluster-head (`tile_idx'=-1), the
		reference may point back to the relevant head. */
	kdu_params *dummy_ref; // `refs' points to this until object is linked
	kdu_params *first_inst, *next_inst; /* Links instances of the same
										   tile-component. */
										   // --------------------------------------------------------------------------
private: // Parameter attribute data members
	kd_attribute *attributes; /* Points to a list of attributes. */
	friend struct kd_attribute;
	bool changed; /* True if any attribute value is changed or added since the
					 last call to `clear_marks'.  May be valid only in the
					 cluster head object, referring to all objects in all
					 clusters. */
					 // --------------------------------------------------------------------------
private: // Dependency data members
	const char *dependencies[KD_MAX_PARAM_DEPENDENCIES + 1]; // NULL terminated
};

/*****************************************************************************/
/*                                   siz_params                              */
/*****************************************************************************/

  // Cluster name
#define SIZ_params "SIZ"

  // Attributes recorded in the SIZ marker segment
#define Sprofile "Sprofile" // One record x
							// "(PROFILE0=0,PROFILE1=1,PROFILE2=2,PART2=3,
							//   CINEMA2K=4,CINEMA4K=5,BROADCAST=6)"
#define Scap "Scap" // One record x B -- if extra capabilities in CAP segment
#define Sextensions "Sextensions" // One record x
								  // "[DC=1|VARQ=2|TCQ=4|PRECQ=2048|VIS=8
								  //  |SSO=16|DECOMP=32|ANY_KNL=64|SYM_KNL=128
								  //  |MCT=256|CURVE=512|ROI=1024]"
#define Sbroadcast "Sbroadcast" // One record x 
								// "I(single=0,multi=1)(irrev=0,rev=1)"
#define Ssize "Ssize" // One record x "II"
#define Sorigin "Sorigin" // One record x "II"
#define Stiles "Stiles" // One record x "II"
#define Stile_origin "Stile_origin" // One record x "II"
#define Scomponents "Scomponents" // One record x "I"
#define Ssigned "Ssigned" // Multiple records x "B"
#define Sprecision "Sprecision" // Multiple records x "I"
#define Ssampling "Ssampling" // Multiple records x "II"

  // Attributes available only during content creation.
#define Sdims "Sdims" // Multiple records x "II"

  // Attributes recorded in the CBD marker segment
#define Mcomponents "Mcomponents" // One record x "I"
#define Msigned "Msigned" // Multiple records x "B"
#define Mprecision "Mprecision" // Multiple records x "I"

  // Values for the Sprofile attribute
#define Sprofile_PROFILE0  ((int) 0)
#define Sprofile_PROFILE1  ((int) 1)
#define Sprofile_PROFILE2  ((int) 2)
#define Sprofile_PART2     ((int) 3)
#define Sprofile_CINEMA2K  ((int) 4)
#define Sprofile_CINEMA4K  ((int) 5)
#define Sprofile_BROADCAST ((int) 6)

  // Values for the Sextensions attribute
#define Sextensions_DC      ((int) 1)
#define Sextensions_VARQ    ((int) 2)
#define Sextensions_TCQ     ((int) 4)
#define Sextensions_PRECQ   ((int) 2048)
#define Sextensions_VIS     ((int) 8)
#define Sextensions_SSO     ((int) 16)
#define Sextensions_DECOMP  ((int) 32)
#define Sextensions_ANY_KNL ((int) 64)
#define Sextensions_SYM_KNL ((int) 128)
#define Sextensions_MCT     ((int) 256)
#define Sextensions_CURVE   ((int) 512)
#define Sextensions_ROI     ((int) 1024)

  // Values for the Sbroadcast attribute
#define Sbroadcast_single   ((int) 0)
#define Sbroadcast_multi    ((int) 1)
#define Sbroadcast_irrev    ((int) 0)
#define Sbroadcast_rev      ((int) 1)

class siz_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with a JPEG2000 `SIZ' marker segment.
	   There is only one of these per image -- i.e., tile-specific and
	   component-specific forms of this parameter class (cluster type) are
	   both disallowed.
	   [//]
	   The cluster name is "SIZ", but you are recommended to use the macro
	   `SIZ_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   This object also manages the information associated with a JPEG2000 `CBD'
	   (Component Bit-Depth) marker segment, if any.  There is at most one of
	   these per image, which must appear in the main header.  The CBD marker
	   segment is written if and only if the `Sextensions_MCT' flag is
	   present in the `Sextensions' attribute managed by `siz_params'.
	   The `CBD' marker segment is central to JPEG2000 Part-2 multi-component
	   transforms.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Sprofile';
			Pattern =
			"(PROFILE0=0,PROFILE1=1,PROFILE2=2,PART2=3,
			  CINEMA2K=4,CINEMA4K=5,BROADCAST=6)" --
			Restricted profile to which the code-stream conforms.  The
			value must currently be an integer in the range 0 to 6.  Profile0
			is the most restrictive profile for Part 1 conforming codestreams.
			Profile2 places no restrictions on the code-stream other than
			those restrictions imposed by ISO/IEC 15444-1 (JPEG2000, Part 1).
			A value of 3 means that the codestream requires support for one or
			more features defined in ISO/IEC 15444-2 (JPEG2000, Part 2) --
			additional information about these extensions is identified via
			the `Sextensions' parameter attribute.
			   The values 4 and 5 correspond to new profile restrictions for
			Digital Cinema, while 6 identifies profile restrictions for
			broadcast applications.  Even though these have higher numerical
			values than Part 2 codestreams, this is only for the purpose of
			backward compatibility with earlier versions of Kakadu.
			The 2K and 4K digital cinema profiles and the broadcast profiles
			are closely relatred and very restrictive subsets ofJPEG2000 Part 1,
			defined by various ammendments to the original standard.
			   If the system determines that support for Part 2 features is
			required, the profile will be set automatically to 3.  Also, if the
			`Sbroadcast' attribute is present, the profile is set automatically
			to 6.  Otherwise, the profile is not adjusted by Kakadu's codestream
			creation machinery.  However, if the profile is found to be
			insufficient, the system will generate a warning at the point where
			it first encounters an inconsistency; this might not occur until
			near the end of the processing in certain rare circumstances.
			   The system does perform some extensive checks for compliance
			with the Digital Cinema profiles when they are used, but only during
			codestream generation.  It makes every effort to set default
			values in such a way as to ensure comliance with Digital Cinema
			profiles, where they are used, but it is ultimately up to the
			user to set the `Creslengths' attribute to ensure that compressed
			data sizes match the application-dependent constraints specified
			by the Digital Cinema ammendment.
			   Similar considerations apply to the broadcast profile, except
			that it depends on additional information provided via the
			`Sbroadcast' parameter attribute (similar to the connection between
			the Part 2 profile and `Sextensions').
			   The default value for the `Sprofile' attribute is Profile2.
			When setting the profile, you may find it convenient to use the
			macro's `Sprofile_PROFILE0', `Sprofile_PROFILE1',
			`Sprofile_PROFILE2', `Sprofile_CINEMA2K', `Sprofile_CINEMA4K',
			`Sprofile_BROADCAST' and `Sprofile_PART2'.
	   [>>] Macro = `Scap'; Pattern = "B" --
			Flag indicating whether or not capabilities from additional
			parts (beyond parts 1 and 2) in the JPEG2000 family of standards
			are defined in a separate capabilities marker segment.  The
			capabilities marker segment itself is not yet recognized by
			Kakadu, but the option to do so is left open to implement
			future standardization outputs.
	   [>>] Macro = `Sextensions'; Pattern =
			"[DC=1 | VARQ=2 | TCQ=4 | PRECQ=2048 | VIS=8 | SSO=16 | DECOMP=32 |
			ANY_KNL=64 | SYM_KNL=128 | MCT=256 | CURVE=512 | ROI=1024]" --
			Logical OR of any combination of a number of flags, indicating
			extended features from Part 2 of the JPEG2000 standard which may
			be found in this codestream.  When working with the `kdu_params::set'
			and `kdu_params::get' functions, use the macros, `Sextensions_DC',
			`Sextensions_VARQ', `Sextensions_TCQ', `Sextensions_PRECQ',
			`Sextensions_VIS', `Sextensions_SSO', `Sextensions_DECOMP',
			`Sextensions_ANY_KNL', `Sextensions_SYM_KNL', `Sextensions_MCT',
			`Sextensions_CURVE' and `Sextensions_ROI'.
	   [>>] Macro = `Sbroadcast';
			Pattern = "I(single=0,multi=1)(irrev=0,rev=1)" --
			This parameter attribute provides more specific details for the
			BROADCAST profile (`Sprofile'=6).  Its 3 fields have the following
			interpretation.
			   The first field identifies the broadcast profile
			level, which is currently required to lie in the range 0 through 7.
			The profile level is concerned with the bit-rate and sampling rate
			of a compressed video stream, as described in Ammendment 4 to
			IS15444-1.  The system does not explicitly impose these
			constraints, since they depend upon the intended frame rate -- this
			can readily be done at the application level.
			   The second field indicates whether the single-tile (0) or
			multi-tile (1) variation of the profile is specified; the multi-tile
			variation allows for either 1 or 4 tiles per image, where multiple
			tiles must be identical in size and involve either vertical tiles
			or 2 tiles in each direction.  When working with the
			`kdu_params::set' and `kdu_params::get' functions, use the macros
			`Sbroadcast_single' and `Sbroadcast_multi' for this field's values.
			   The third field indicates whether the irreversible or reversible
			transform variation of the profile is specified.  When working with
			the `kdu_parms::set' and `kdu_params::get' functions, use the macros
			`Sbroadcast_irrev' and `Sbroadcast_rev' for this field's values.
			   During codestream generation, this parameter attribute will
			default to the single-tile, irreversible level 1 variant if
			`Sprofile' identifies the BROADCAST profile.  Also, if `Sbroadcast'
			is specified and `Sprofile' is left unspecified, the system will
			automatically set `Sprofile' to 6 -- BROADCAST.
	   [>>] Macro = `Ssize'; Pattern = "II" --
			Canvas dimensions: vertical dimension first.  For compressors, this
			will normally be derived from the dimensions of the individual
			image components. Explicitly supplying the canvas dimensions may
			desirable if the source image files do not indicate their
			dimensions, or if custom sub-sampling factors are desired.
	   [>>] Macro = `Sorigin'; Pattern = "II" --
			Image origin on canvas: vertical coordinate first.  Defaults to
			{0,0}, or the tile origin if one is given.
	   [>>] Macro = `Stiles'; Pattern = "II" --
			Tile partition size: vertical dimension first.  Defaults to {0,0}.
	   [>>] Macro = `Stile_origin'; Pattern = "II" --
			Tile origin on the canvas: vertical coordinate first.  Defaults
			to {0,0}].
	   [>>] Macro = `Scomponents'; Pattern = "I" --
			Number of codestream image components.  For compressors, this will
			normally be deduced from the number and type of image files
			supplied to the compressor.  Note carefully, however, that if
			a multi-component transform is used, the number of codestream
			image components might not be equal to the number of "output image
			components" given by `Mcomponents'.  In this case, the value of
			`Mcomponents' and the corresponding `Mprecision' and `Msigned'
			attributes should generally be associated with the image files
			being read (for compression) or written (for decompression).
	   [>>] Macro = `Ssigned'; Pattern = "B",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Indicates whether each codestream image component contains signed
			or unsigned sample values.  For compressors, this will normally be
			deduced from the image files supplied to the compressor, but may
			be explicitly set if raw input files are to be used.  Also, if you
			happen to be using the Part-2 multi-component transform capabilities,
			the signed/unsigned attributes of the original image components
			should be expressed by `Msigned'; in this case, you might want
			to explicitly set `Ssigned' in a manner which reflects the
			signed/unsigned characteristics of the codestream image components
			produced after subjecting the original components to the forward
			multi-component transform.  Note that the last supplied identifier
			is repeated indefinitely for all remaining components.
	   [>>] Macro = `Sprecision'; Pattern = "I",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Indicates the bit-depth of each codestream image component.  For
			compressors, this will normally be deduced from the image files
			supplied to the compressor, but may need to be explicitly set if
			raw input files are to be used.  Also, if you happen to be using the
			Part-2 multi-component transform capabilities, the precision of the
			original image components should be expressed by `Mprecision'; in
			this case, you might want to explicitly set `Sprecision' to
			reflect the bit-depth of the codestream image components produced
			after subjecting the original components to the forward
			multi-component transform.  Note that the last supplied value is
			repeated indefinitely for all remaining components.
	   [>>] Macro = `Ssampling'; Pattern = "II",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Indicates the sub-sampling factors for each codestream image
			component. In each record, the vertical factor appears
			first, followed by the horizontal sub-sampling factor.
			The last supplied record is repeated indefinitely for
			all remaining components.  For compressors, a suitable set
			of sub-sampling factors will normally be deduced from the
			individual image component dimensions, expressed via `Sdims'.
	   [>>] Macro = `Sdims'; Pattern = "II",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Indicates the dimensions (vertical, then horizontal) of
			each individual codestream image component. The last supplied
			record is repeated indefinitely for all remaining
			components.  For compressors, the image component dimensions
			will normally be deduced from the image files supplied to the
			compressor, but may be explicitly set if raw input files are to
			be used.
	   [>>] Macro = `Mcomponents'; Pattern = "I" --
			Number of image components produced at the output of the inverse
			multi-component transform -- during compression, you may think
			of these as original image components.  In any event, we refer to
			them as "MCT output components", taking the perspective of the
			decompressor.  The value of `Mcomponents' may be smaller than or
			larger than the `Scomponents' value, which refers to the number
			of "codestream image components".  During decompression, the
			codestream image components are supplied to the input of the
			inverse multi-component transform.  Note carefully, however, that
			for Kakadu to perform a forward multi-component transform on image
			data supplied to a compressor, the value of `Mcomponents' must be
			at least as large as `Scomponents' and the inverse multi-component
			transform must provide sufficient invertible transform blocks
			to derive the codestream components from the output image components.
			In the special case where `Mcomponents' is 0, or not specified,
			there is no multi-component transform.  In this case, `Scomponents',
			`Ssigned' and `Sprecision' define the output image components.
	   [>>] Macro = `Msigned'; Pattern = "B",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Indicates whether each MCT output component (see `Mcomponents' for
			a definition of "MCT output components") contains signed
			or unsigned sample values.  If fewer than `Mcomponents' values are
			provided, the last supplied value is repeated as required for
			all remaining components.  Compressors should be able to deduce
			this information from the image files supplied, in which case
			the `Ssigned' attribute might have to be set explicitly to reflect
			the impact of forward multi-component transformation on the
			source data (the source data are the "MCT output components" during
			compression).
	   [>>] Macro = `Mprecision'; Pattern = "I",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Indicates the bit-depth of each MCT output component (see
			`Mcomponents' for a definition of "MCT output components").
			If fewer than `Mcomponents' values are provided, the last supplied
			value is repeated as required for all remaining components.
			Compressors might be able to deduce this information from the
			image files supplied, in which case the `Sprecision' attribute might
			have to be set explicitly to reflect the impact of forward
			multi-component transformation on the source data (the source data
			are the "MCT output components" during compression).
	   [//]
	   Although the JPEG2000 `SIZ' marker cannot represent negative coordinates,
	   it is particularly convenient to allow this object to store and report
	   negative coordinates.  These should not trouble applications which
	   interface with it, since all of the algebraic properties of the coordinate
	   system still hold. Negative coordinates will be converted into appropriate
	   non-negative coordinates only when a valid marker segment must be written
	   out -- to do this the `write_marker_segment' function must examine various
	   `cod_params' attributes from every tile-component in the image to
	   determine appropriate offsets to the negative coordinates which will not
	   alter the interpretation of the canvas coordinate system.
	   [//]
	   For the above reason, you should avoid writing a `SIZ' marker segment
	   (calling `kdu_params::generate_marker_segments') until all code-stream
	   parameters have been finalized for all tiles and tile-components.
	*/
public: // Member functions
	KDU_EXPORT siz_params();
	/* [SYNOPSIS]
		 Constructing a `siz_params' object causes the `kdu_params::finalize'
		 member function to be overridden to implement a number of useful
		 conversions between partially completed representations of the
		 image dimensions.  The behaviour is as follows:
		 [//]
		 The `kdu_params::finalize' function verifies the consistency
		 of all available dimension information and derives and sets any
		 dimensions which have not otherwise been set.  For example, we
		 may start with a collection of individual component dimensions
		 (set using the `Sdims' attribute), from which the function must
		 deduce an appropriate set of canvas dimensions and sub-sampling
		 parameters.  The function does its best to handle all combinations
		 of inputs.  It generates an error if insufficient or conflicting
		 information prevent it from determining a full set of consistent
		 dimensions.
	*/
protected:
	virtual kdu_params *new_object() { return NULL; }
	/* There can be only one `siz_params' object. */
	virtual void finalize(bool after_reading = false);
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                 mct_params                                */
/*****************************************************************************/

  // Cluster name
#define MCT_params "MCT"
  // Attributes recorded in an MCT marker segment
#define Mmatrix_size   "Mmatrix_size"   // One record x "I"
#define Mmatrix_coeffs "Mmatrix_coeffs" // Multiple records x "F"
#define Mvector_size   "Mvector_size"   // One record x "I"
#define Mvector_coeffs "Mvector_coeffs"   // Multiple records x "F"
#define Mtriang_size   "Mtriang_size"   // One record x "I"
#define Mtriang_coeffs "Mtriang_coeffs" // Multiple records x "F"

class mct_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with a JPEG2000 `MCT'
	   (Multi-Component Transform) marker segment.  Tile-specific forms are
	   allowed and multiple instances are treated like components for the
	   purpose of inheritance (see the `treat_instances_like_components'
	   argument to the `kdu_params' constructor).
	   [//]
	   The purpose of this parameter class is to provide the coefficients
	   associated with decorrelation matrices, offset vectors and
	   dependency transformations, any or all of which may be involved
	   in the description of the complete multi-component transformation.
	   [//]
	   The instance number of the `mct_params' object is referenced
	   by the `Mstage_xforms' attribute, which is managed by the
	   `mcc_params' object.  These instance numbers must be in
	   the range 1 through 255.
	   [//]
	   The cluster name is "MCT", but you are recommended to use the macro
	   `MCT_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Mmatrix_size'; Pattern = "I" --
			Identifies the number of matrix elements, if any, represented
			by this object.  The actual matrix coefficients are represented
			by the `Mmatrix_coeffs' attribute.  Matrices are used to describe
			reversible and irreversible inverse component decorrelation
			transforms.
	   [>>] Macro = `Mmatrix_coeffs'; Pattern = "F", [MULTI_RECORD] --
			Coefficients of the matrix, if there is one, whose number of
			elements is given by `Mmatrix_size'.  The coefficients appear
			in row-major order (first row, then second row, etc.).  The
			height and width of the matrix are not recorded here, but
			matrices are not required to be square.  For reversible transforms
			the coefficients must all be integers.
	   [>>] Macro = `Mvector_size'; Pattern = "I" --
			Identifies the number of vector elements, if any, represented by
			this object.  The actual vector coefficients are represented by
			the `Mvector_coeffs' attribute.  Vectors are used to describe
			offsets to be applied to the component sample values after
			inverse transformation.
	   [>>] Macro = `Mvector_coeffs'; Pattern = "F",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Coefficients of the vector, if there is one, whose number of
			elements is given by `Mvector_size'.  Unlike `Mmatrix_coeffs'
			and `Mtriang_coeffs', this attribute contains the
			`CAN_EXTRAPOLATE' property, meaning that the last provided
			entry will be replicated as required in order to provide the
			`Mvector_size' vector elements.
	   [>>] Macro = `Mtriang_size'; Pattern = "I" --
			Identifies the total number of sub-triangular matrix elements,
			if any, represented by this object.  A sub-triangular matrix is
			square, with no coefficients above the diagonal and at least one
			coefficient missing from the diagonal.  A strictly sub-triangular
			M x M matrix will have M*(M-1)/2 coefficients, all below the
			diagonal.  Matrices of this form are used to describe irreversible
			multi-component dependency transforms.  Reversible dependency
			transforms, however, include all but the upper left diagonal entry,
			for a total of M*(M+1)/2-1 coefficients.
	   [>>] Macro = `Mtriang_coeffs'; Pattern = "F", [MULTI_RECORD] --
			Coefficients of the sub-triangular matrix, if any, whose number
			of elements is represented by the `Mtriang_size' attribute.  The
			coefficients are arranged in row-major order.  Thus, for a
			dependency transform with M inputs and outputs, the first coefficient
			(first two for reversible transforms) comes from the second
			row of the matrix, the next two (three for reversible transforms)
			comes from the third row of the matrix, and so forth.  For
			reversible transforms, the coefficients must all have integer values.
	*/
public: // Member functions
	KDU_EXPORT mct_params();
protected:
	virtual kdu_params *new_object() { return new mct_params(); }
	virtual void finalize(bool after_reading = false);
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
private: // Counters used to keep track of multiple MCT marker segments
		 // which need to be concatenated to form a complete description
	int Zmct_matrix_next;
	int Zmct_matrix_final;  // Holds Ymct for matrix MCT marker segs
	int Zmct_vector_next;
	int Zmct_vector_final;  // Holds Ymct for vector MCT marker segs
	int Zmct_triang_next;
	int Zmct_triang_final;  // Holds Ymct for sub-triangular MCT marker segs
};

/*****************************************************************************/
/*                                 mcc_params                                */
/*****************************************************************************/

  // Cluster name
#define MCC_params "MCC"
  // Attributes recorded in an MCC marker segment
#define Mstage_inputs      "Mstage_inputs"      // Multiple records x "II"
#define Mstage_outputs     "Mstage_outputs"     // Multiple records x "II"
#define Mstage_blocks      "Mstage_collections" // Multiple records x "II"
#define Mstage_xforms      "Mstage_xforms"      // Multiple records x
								   // "(DEP=0,MATRIX=9,DWT=3,MAT=1000)IIII"
  // Values for the Mstage_xforms attribute
#define Mxform_DEP ((int) 0)
#define Mxform_MATRIX ((int) 9)
#define Mxform_DWT ((int) 3)
#define Mxform_MAT ((int) 1000) // This option is defined only so as to catch
								// and properly warn of compatibility problems
								// between Kakadu versions prior to v6.0 and
								// those after.

class mcc_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with a JPEG2000 `MCC'
	   (Multi-component transform Component Collection) marker segment.
	   MCC marker segments describe a single multi-component transform
	   stage.  The complete transform consists of a sequence of stages which
	   are referenced by a `MCO' (Multi-Component Order) marker segment
	   (see `mco_params').  Each stage processes a collection of input
	   image components, producing a collection of output image components.
	   The processing proceeds via one or more independent transform
	   blocks, each of which produces a so-called "component collection".
	   The actual coefficients of the transforms in each block are
	   managed by `mct_params', unless the transform is implemented using
	   a discrete wavelet transform, in which case the coefficients are
	   described by an `atk_params' (Arbitrary Transform Kernels) object.
	   [//]
	   As with `mct_params' and `atk_params', tile-specific forms of this
	   object are allowed and multiple instances are treated like components
	   for the purpose of inheritance (see the `treat_instances_like_components'
	   argument to the `kdu_params' constructor).
	   [//]
	   The instance number of the `mcc_params' object is referenced
	   by the `Mstages' attribute managed by `mco_params'.  These instance
	   numbers must be in the range 0 through 255.
	   [//]
	   The cluster name is "MCC", but you are recommended to use the macro
	   `MCC_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Mstage_inputs'; Pattern = "II", [MULTI_RECORD] --
			This attribute is used to describe a list of input component
			indices which are used by all transforms in the stage.  This
			list of component indices is a concatenation of the index
			ranges <A1>-<B1>, <A2>-<B2>, ..., where An <= Bn are the
			first and second fields in the n'th record of the `Mstage_inputs'
			attribute.   The list of input component indices may contain
			repeated values, but must cover all components produced by the
			previous stage (or all codestream component indices, if this is
			the first stage).  In particular, it must always include 0.
			The first transform block operates on the first N1 components
			identified by this list; the second transform block operates on the
			next N2 components in this list; and so forth.
	   [>>] Macro = `Mstage_outputs'; Pattern = "II", [MULTI_RECORD] --
			This attribute is used to describe a list of output component
			indices which are produced by this stage.  This list  of component
			indices is a concatenation of the index ranges <A1>-<B1>, <A2>-<B2>,
			..., where An <= Bn are the first and second fields in the n'th
			record of the `Mstage_outputs' attribute.  The list of output
			component indices may not contain any repeated component indices,
			but it may contain "holes".  The transform stage is considered
			to generate components with indices from 0 to the largest index
			in the output list; any components in this range which are not
			listed (these are the holes) are taken to be identically equal to
			0.  The first transform block in the stage processes the first N1
			components in the list to produce the first M1 components in the
			output list; the second transform block in the stage processes the
			next N1 components in the input list, producing the next M2
			components in the output list; and so forth.
	   [>>] Macro = `Mstage_blocks'; Pattern = "II", [MULTI_RECORD] --
			This attribute provides the values Nc and Mc which appear in the
			descriptions of `Mstage_inputs' and `Mstage_outputs', for each
			transform block (equivalently, each component collection), c.
			The `Mstage_blocks' parameter attribute should contain one
			record for each transform.  Each record contains two strictly
			positive integers, identifying the number of input components Nk,
			and the number of output components, Mk, produced by the k'th
			transform.  No transform may consume or produce 0 components.
			Between them, the various transform blocks must consume all
			components in the input list described by `Mstage_inputs' and
			produce all components in the output list described by
			`Mstage_outputs'.
	   [>>] Macro = `Mstage_xforms';
			Pattern = "(DEP=0,MATRIX=9,DWT=3,MAT=1000)IIII", [MULTI_RECORD] --
			This attribute provides one record for each transform block,
			which describes the type of transform to be implemented in that
			block and the parameters of the transform.  The first field
			identifies the transform as one of "dependency transform"
			(use `Mxform_DEP' in source code, or `DEP' when supplying
			text to be parsed), "decorrelation matrix transform" (use
			`Mxform_MATRIX' in source code, or `MATRIX' when supplying text
			to be parsed), or "discrete wavelet transform" (use `Mxform_DWT'
			in source code, or `DWT' when suppling text to be parsed).
			Do not use the `MAT' option (`Mxform_MAT' in source code).  This
			option is defined so as to catch compatibility problems with
			versions prior to v6.0.  The `MAT' and `MATRIX' options notionally
			mean the same thing, but prior to v6.0 Kakadu accidentally used
			a non-compliant order for the coefficients in reversible
			matrix transforms.  To prevent such problems propagating into
			the future, Kakadu will explicitly refuse to generate codestreams
			which use the `MAT' option, issuing an appropriate error message.
			It will, however, digest and decompress codestreams generated using
			the old non-compliant syntax.

			The 2'nd field of each record holds the instance index of the
			`Mtriang_coeffs' (for dependency transforms) or `Mmatrix_coeffs'
			(for decorrelation matrix transforms) attributes, which provide
			the actual transform coefficients, unless the transform is a DWT;
			in this last case the 2'nd field holds 0 (`Ckernels_W9X7') for
			the 9/7 DWT, 1 (`Ckernels_W5X3') for the 5/3 DWT, or the
			instance index (in the range 2 to 255) of an `ATK' marker
			segment whose `Kreversible', `Ksymmetric', `Kextension', `Ksteps'
			and `Kcoeffs' attributes describe the DWT kernel.  Apart from DWT
			transforms, a 0 for this field means that the transform block
			just passes its inputs through to its outputs (setting any extra
			output components equal to 0) and adds any offsets specified via the
			3'rd field -- we refer to this as a "null" transform block.

			The 3'rd field of each record holds the instance index of the
			`Mvector_coeffs' attribute which describes any offsets to be
			applied after inverse transformation of the input components to
			the block.  A value of 0 for this field means that there is no
			offset; otherwise, the value must be in the range 1 to 255.

			For DWT transforms, the 4'th field in the record identifies
			the number of DWT levels to be used, in the range 0 to 32, while
			the final field holds the transform origin, which plays the
			same role as `Sorigin', but along the component axis.  For
			dependency and decorrelation transforms, the 4'th field
			must hold 0 if the transform is irreversible, or 1 if it is
			reversible, while the 5'th field must hold 0.
	*/
public: // Member functions
	KDU_EXPORT mcc_params();
protected:
	virtual kdu_params *new_object() { return new mcc_params(); }
	virtual void finalize(bool after_reading = false);
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                 mco_params                                */
/*****************************************************************************/

  // Cluster name
#define MCO_params "MCO"
  // Attributes recorded in an MCO marker segment
#define Mnum_stages  "Mnum_stages"  // One record x "I"
#define Mstages      "Mstages"      // Multiple records x "I"

class mco_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with a JPEG2000 `MCO'
	   (Multi-Component transform stage Order) marker segment.
	   MCO marker segments are ultimately responsible for controlling
	   the multi-component transform, by defining the transform stages
	   which are to be applied within any given tile.  This is done
	   via the `Mstages' attribute, which provides the instance indices of
	   each stage in the inverse (synthesis) transform in sequence.  These
	   instance indices are applied to the `Mstage_inputs', `Mstage_outputs',
	   `Mstage_blocks' and `Mstage_xforms' attributes to recover all relevant
	   details of the transformation process.
	   described by an `atk_params' (Arbitrary Transform Kernels) object.
	   [//]
	   As with `mct_params' and `mcc_params', tile-specific forms of this
	   object are allowed.  For obvious reasons, component-specific and
	   instance-specific forms of this object cannot exist.
	   [//]
	   The cluster name is "MCO", but you are recommended to use the macro
	   `MCO_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Mnum_stages'; Pattern = "I" --
			Identifies the number of stages in the multi-component transform
			to be applied to this tile, or (for main header attributes) as
			a default for tiles which do not specify the `Mnum_stages' attribute.
			If this value is 0, the spatially transformed codestream components
			associated with the relevant tile are mapped directly to the
			output components specified via the global `Mcomponents',
			`Msigned' and `Mprecision' attributes.  If `Mcomponents' is larger
			than `Scomponents', some final components are automatically
			set to 0.  Where the number of stages is 0, codestream components
			which are identified as unsigned by the `Ssigned' attribute are
			first offset (at least nominally) by half their dynamic range,
			in the usual fashion.  If, on the other hand, `Mnum_stages' specifies
			a non-zero number of transform stages, component offsets must
			be provided by the multi-component transform stages themselves.
			This parameter defaults to 0 if no value is set in the main
			header, and a non-zero `Mcomponents' value exists.
	   [>>] Macro = `Mstages'; Pattern = "I", [MULTI_RECORD] --
			Holds one entry for each stage in the multi-component transform
			for the tile in question.  Each entry must lie in the range 0 to
			255, being the instance index of a corresponding `mcc_params'
			object whose `Mstage_inputs', `Mstage_outputs', `Mstage_blocks'
			and `Mstage_xforms' attributes fully describe the transform stage.
	*/
public: // Member functions
	KDU_EXPORT mco_params();
protected:
	virtual kdu_params *new_object() { return new mco_params(); }
	virtual void finalize(bool after_reading = false);
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                  atk_params                               */
/*****************************************************************************/

  // Cluster name
#define ATK_params "ATK"
  // Attributes recorded in an ATK marker segment
#define Kreversible "Kreversible" // One record x "B"
#define Ksymmetric "Ksymmetric" // One record x "B"
#define Kextension "Kextension" // One record x "(CON=0,SYM=1)"
#define Ksteps "Ksteps" // Multiple records x "IIII" -- Len/Off/Eps/Rnd
#define Kcoeffs "Kcoeffs" // Multiple records x "F"

  // Values for the Kextension attribute
#define Kextension_CON ((int) 0)
#define Kextension_SYM ((int) 1)

class atk_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 Part-2 `ATK'
	   marker segments.  Tile-specific forms are allowed and multiple
	   instances are treated like components for the purpose of inheritance
	   (see the `treat_instances_like_components' argument to the `kdu_params'
	   constructor).
	   [//]
	   The purpose of this parameter class is to describe a wavelet transform
	   kernel.  The instance number of the `atk_params' object is referenced
	   by the `Catk' attribute, if `Ckernels' holds the value `Ckernels_ATK'.
	   These instance numbers must be in the range 2 through 255.
	   [//]
	   The cluster name is "ATK", but you are recommended to use the macro
	   `ATK_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Kreversible'; Pattern = "B" --
			This attribute determines how the `Ksteps' and `Kcoeffs' attributes
			should be treated.  A default value cannot be created
			automatically, so you must explicitly specify a value if you
			want ATK information to become available for any particular
			instance index in the main header or a tile header.  In the end,
			this parameter attribute must agree with the value of the
			`Creversible' attribute, for any tile-component which uses this
			transformation kernel.  In practice, this consistency can only
			be ensured once `cod_params::finalize' has been called, which
			means that the `kdu_params::finalize_all' function must be called
			both during the writing and during the reading of codestream
			parameters.
	   [>>] Macro = `Ksymmetric'; Pattern = "B" --
			If true, the transform kernel belongs to the whole-sample
			symmetric class, which is treated specially by JPEG2000 Part-2.
			The translated impulse responses of these kernels are all
			symmetric about 0 -- see the Taubman & Marcellin book, Chapter 6,
			for a definition of translated impulse responses.  Equivalently,
			all lifting steps involve even-length symmetric lifting coefficients,
			Cs[n], where the coefficients associated with steps s=0, 2, 4, ...
			are symmetric about n = 1/2 and the coefficients associated with
			steps s=1, 3, 5, ... are symmetric about n = -1/2.
	   [>>] Macro = `Kextension'; Pattern = "(CON=0,SYM=1)" --
			Identifies the boundary extension method to be applied in each
			lifting step.  If `CON', boundary samples are simply replicated.
			The other possible value, `SYM', means that boundary samples
			are symmetrically extended.  The centre of symmetry in this case
			is the boundary sample location within an interleaved representation
			in which low-pass samples occupy the even indexed locations and
			high-pass samples occupy the odd indexed locations.  The `SYM'
			method must be used if `Ksymmetric' is true.  Conversely,
			for non-symmetric filters you are strongly recommded to use the
			`CON' method, since this method can use less memory and other
			methods can make region-of-interest calculations very difficult
			for small regions which are close to the image boundaries.
			When setting the profile, you may find it convenient to use the
			macro's `Kextension_CON' and `Kextension_SYM'.
	   [>>] Macro = `Ksteps'; Pattern = "IIII", [MULTI_RECORD] --
			Array with one entry for each lifting step.  The first entry
			corrsponds to lifting step s=0, which updates odd indexed samples,
			based on even indexed samples; the second entry corresponds to
			lifting step s=1, which updates even indexed samples, based on
			odd indexed samples; and so forth.  The first field in each record
			holds the length, Ls, of the coefficient array Cs[n], for the
			relevant step s.  The second field is the location of the first
			entry, Ns, where Cs[n] is defined for n=Ns to Ns+Ls-1.  The value
			of Ns is typically negative, but need not be.  For symmetric
			kernels, Ls must be even and Ns must satisfy Ns=-floor((Ls+p-1)/2),
			where p is the lifting step parity (0 if s is even, 1 if s is odd).
			The third and fourth fields must both be 0 if `Kreversible' is
			false.  For reversible transform kernels, however, the third
			field holds the downshift value, Ds, while the fourth field holds
			the rounding offset, Rs, to be added immediately prior to
			downshifting.
	   [>>] Macro = `Kcoeffs'; Pattern = "F", [MULTI_RECORD] --
			Holds the lifting coefficients, Cs[n].  The first L0 records
			describe the coefficients of the first lifting step.  These are
			followed by the L1 coefficients of the second lifting step, and
			so forth.  The Ls values are identified by the first field in
			each `Ksteps' record.  Lifting step s may be described by
			X_s[2k+1-p] += TRUNC(sum_{Ns<=n<Ns+Ls} Cs[n]*X_{s-1}[2k+p+2n]).
			In the case of an irreversible transform, the TRUNC operator
			does nothing and all arithmetic is performed (at least
			notionally) in floating point.  For reversible transforms,
			TRUNC(a) = floor(a + Rs*2^{-Ds}) and Cs[n] is guaranteed to be
			an integer multiple of 2^{-Ds}.
	*/
public: // Member Functions
	KDU_EXPORT
		atk_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new atk_params(); }
	virtual void finalize(bool after_reading = false);
	/* Call this function through the public "kdu_params::finalize'
	   function. */
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                   cod_params                              */
/*****************************************************************************/

  // Cluster name
#define COD_params "COD"
  // Attributes recorded in COD marker segments
#define Cycc "Cycc" // One record x "B"
#define Cmct "Cmct" // One record x "[ARRAY=2|DWT=4]"
#define Clayers "Clayers" // One record x "I"
#define Cuse_sop "Cuse_sop" // One record x "B"
#define Cuse_eph "Cuse_eph" // One record x "B"
#define Corder "Corder" // One record x "(LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4)"
#define Calign_blk_last "Calign_blk_last" // One record x "BB"
  // Attributes recorded in COD or COD marker segments
#define Clevels "Clevels" // One record x "I"
#define Cads "Cads" // One record x "I"
#define Cdfs "Cdfs" // One record x "I"
#define Cdecomp "Cdecomp" // Multiple records x "C" -- see detailed description
#define Creversible "Creversible" // One record x "B"
#define Ckernels "Ckernels" // One record x "(W9X7=0,W5X3=1,ATK=-1)"
#define Catk "Catk" // One record x "I"
#define Cuse_precincts "Cuse_precincts" // One record x "B"
#define Cprecincts "Cprecincts" // Multiple records x "II"
#define Cblk "Cblk" // One record x "II"
#define Cmodes "Cmodes" // One record x "[BYPASS=1|RESET=2|RESTART=4|CAUSAL=8|ERTERM=16|SEGMARK=32]"
  // Attributes available only during content creation.
#define Cweight       "Cweight" // One record x "F"
#define Clev_weights  "Clev_weights" // Multiple records x "F"
#define Cband_weights "Cband_weights" // Multiple records x "F"
#define Creslengths   "Creslengths" // Multiple records x "I"
  // Values for the "Cmct" attribute
#define Cmct_ARRAY     ((int) 2)
#define Cmct_DWT       ((int) 4)
  // Values for the "Corder" attribute
#define Corder_LRCP    ((int) 0)
#define Corder_RLCP    ((int) 1)
#define Corder_RPCL    ((int) 2)
#define Corder_PCRL    ((int) 3)
#define Corder_CPRL    ((int) 4)
  // Values for the "Ckernels" attribute
#define Ckernels_W9X7  ((int) 0)
#define Ckernels_W5X3  ((int) 1)
#define Ckernels_ATK   ((int) -1)
  // Values for the "Cmodes" attribute
#define Cmodes_BYPASS  ((int) 1)
#define Cmodes_RESET   ((int) 2)
#define Cmodes_RESTART ((int) 4)
#define Cmodes_CAUSAL  ((int) 8)
#define Cmodes_ERTERM  ((int) 16)
#define Cmodes_SEGMARK ((int) 32)

class cod_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 `COD' and `COC'
	   marker segments.  Tile- and component-specific forms are allowed (in
	   fact, required for a complete code-stream description); however, not
	   all attributes are allowed to have component-specific values; these
	   are marked in the list below as having the `ALL_COMPONENTS' flag set.
	   [//]
	   The cluster name is "COD", but you are recommended to use the macro
	   `COD_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   NOTE: If you are going to include an `atk_params' object in the
	   parameter list, it is important that you link it in ahead of
	   the `cod_params' object.  This ensures that `atk_params::finalize'
	   will be called prior to `cod_params::finalize' so that reversibility
	   can be reliably determined from the `Kreversible' attribute if
	   necessary.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Cycc'; Pattern = "B",
			[ALL_COMPONENTS] --
			RGB to Luminance-Chrominance conversion?  Default is to convert
			images with 3 or more components to a luminance-chrominance
			representation, unless a Part 2 multi-component transform is
			being used (i.e., `Mcomponents' > 0).
	   [>>] Macro = `Cmct'; Pattern = "[ARRAY=2|DWT=4]",
			[ALL_COMPONENTS] --
			This parameter should be 0 unless a Part 2 multi-component transform
			is being used, in which case it contains the logical OR of up to
			two flags, `Cmct_ARRAY' and `Cmct_DWT'.  The first flag should be
			present if and only if an array-based multi-component transform
			block is associated with the image, or the relevant tile (for
			tile-specific instances of the COD marker segment).  The second
			flag should be present if and only if a DWT-based multi-component
			transform block is associated with the image, or the relevant tile
			(for tile-specific instances of the COD marker segment).  Both
			flags should be present if both types of multi-component transform
			block are employed for the image or tile, as appropriate.  Normally,
			there is no need to explicitly set this parameter yourself.  In fact,
			anything you do set for this parameter will be overridden in the
			object's `finalize' member function, which inspects the relevant
			`Mstages' and `Mstage_xforms' attributes to determine which, if any,
			of the multi-component transform options are being used.  When
			reading a codestream created by Kakadu versions prior to v6.0, you
			will find that this parameter is not set, even if a multi-component
			transform has been used.  This is useful, since it allows the
			internal machinery to transparently compensate for an error in the
			multi-component transform implementation in previous versions of
			Kakadu, whereby the matrix coefficients for reversible array-based
			multi-component transforms were accidentally transposed.
	   [>>] Macro = `Clayers'; Pattern = "I",
			[ALL_COMPONENTS] --
			Number of quality layers.  Default is 1.
	   [>>] Macro = `Cuse_sop'; Pattern = "B",
			[ALL_COMPONENTS] --
			Include SOP markers (i.e., resync markers)?  Default is no
			SOP markers.
	   [>>] Macro = `Cuse_eph'; Pattern = "B",
			[ALL_COMPONENTS] --
			Include EPH markers (marker end of each packet header)?
			Default is no EPH markers.
	   [>>] Macro = `Corder'; Pattern = "(LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4)",
			[ALL_COMPONENTS]
			Default progression order (may be overridden by
			`POCorder'.  The four character identifiers have the
			following interpretation: L=layer; R=resolution; C=component;
			P=position. The first character in the identifier refers to
			the index which progresses most slowly, while the last refers
			to the index which progresses most quickly.  Default is "LRCP".
			When working with the `kdu_params::set' and `kdu_params::get'
			functions, use one of the macros, `Corder_LRCP', `Corder_RLCP',
			`Corder_RPCL', `Corder_PCRL' or `Corder_CPRL' to set or test
			for specific progression orders.
	   [>>] Macro = `Calign_blk_last'; Pattern = "BB",
			[ALL_COMPONENTS] --
			If true, the code-block partition is aligned so that the last
			sample in each nominal block (ignoring the effect of boundaries)
			is aligned at a multiple of the block dimension (a power of 2).
			Equivalently, the first sample in each nominal block lies at a
			location which is a multiple of the block dimension, plus 1. By
			default (i.e., false), the first sample of each block is
			aligned at a multiple of the block dimension. The alignment is
			specified separately for both dimensions, with the vertical
			dimension specified first.  Note that this is a Part-2 feature.
	   [>>] Macro = `Clevels'; Pattern = "I" --
			Number of wavelet decomposition levels, or stages.  Default is 5.
	   [>>] Macro = `Cdfs'; Pattern = "I" --
			Index of the DFS marker segment used to hold Downsampling Factor
			Style information.  If DFS information is involved, the value of
			the `Cdfs' index must be in the range 1 to 127.  A value of 0 means
			that no DFS marker segment is referenced.  This attribute is ignored
			outside of the main header (i.e., for non-negative tile indices).
			You will not normally set this parameter yourself.  Rather, it is
			preferable to allow `cod_params::finalize' to find a suitable index
			for you.  In any event, the DFS instructions will be generated
			automatically by `cod_params::finalize' in order to record
			downsampling style information derived from `Cdecomp'.  During
			marker segment reading, the DFS instructions will be read, along
			with any ADS information (see `Cads') in order to reconstruct the
			`Cdecomp' attribute.  For a discussion of these mechanisms, see the
			introductory comments appearing with the definition of the
			`ads_params' class.
	   [>>] Macro = `Cads'; Pattern = "I" --
			Index of the ADS marker segment used to hold Arbitrary Downsampling
			Style information.  If ADS information is involved, the value of
			the `Cads' index must lie in the range 1 to 127.  A value of 0 means
			that no ADS marker segment is referenced.  You will not normally
			set this parameter yourself.  Rather, it is preferable to allow
			`cod_params::finalize' to find a suitable index for you.  In
			any event, the ADS information will be generated automatically
			from information contained in `Cdecomp'.  During marker segment
			reading, the ADS information is used together with any DFS
			information (see `Cdfs') in order to reconstruct the `Cdecomp'
			attribute.  For a discussion of these mechanisms, see the
			introductory comments appearing with the definition of the
			`ads_params' class.
	   [>>] Macro = `Cdecomp'; Pattern = "C" [MULTI_RECORD] --
			This attribute manages decomposition structure information for each
			DWT level.  For Part-1 codestreams, there will always be exactly
			one value, equal to 3, which means that the Mallat decomposition
			structure is being used -- this is also the default value, if you
			do not explicitly specify a decomposition structure.  For Part-2
			codestreams, there may be one 32-bit integer for each DWT level,
			starting from the highest resolution level and working downwards.
			These values contain information recovered from (or to be written
			in) Part-2 ADS and DFS marker segments.  A full discussion of the
			interpretation assigned to each `Cdecomp' value, along with the
			extension of available values to additional decomposition levels,
			may be found further below.
	   [>>] Macro = `Creversible'; Pattern = "B" --
			Reversible compression?  In the absence of any other information,
			the default is irreversible.  However, if `Ckernels' is `W5X3'
			the transform will be reversible.  If `Catk' is non-zero,
			the reversibility can be derived from the `Catk' index and the
			`Kreversible' attribute.
	   [>>] Macro = `Ckernels'; Pattern = "(W9X7=0,W5X3=1,ATK=-1)" --
			Wavelet kedfiernels to use.  Default is W5X3 for reversible
			compression, W9X7 for irreversible compression, and ATK if
			`Catk' is non-zero.  When working with the `kdu_params::set'
			and `kdu_params::get' functions, use one of the
			macros, `Ckernels_W9X7', `Ckernels_W5X3' or `Ckernels_ATK' to
			set or test for specific sets of wavelet kernels.
	   [>>] Macro = `Catk'; Pattern = "I" --
			If non-zero, this attribute holds the Index of the
			Arbitrary Transformation Kernel (ATK) marker
			segment from which the DWT kernel should be derived.  In this
			case, the value must be in the range 2 to 255 and a corresponding "
			`Kreversible' attribute must exist, having the same index
			(instance) value.  Thus, for example, if `Catk=3', you must also
			supply a value for `Kreversible:I3'.  This allows
			`cod_params::finalize' to deduce the required value for
			`Creversible'.  The ATK information in these parameter attributes
			can also be tile-specific.
	   [>>] Macro = `Cuse_precincts'; Pattern = "B" --
			Explicitly specify whether or not precinct dimensions
			are supplied.  Default is false, unless `Cprecincts' is used.
	   [>>] Macro = `Cprecincts'; Pattern = "II",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Precinct dimensions (must be powers of 2). Multiple records may be
			supplied, in which case the first record refers to the highest
			resolution level and subsequent records to lower resolution
			levels. The last specified record is used for any remaining lower
			resolution levels.  Inside each record, vertical coordinates
			appear first.
	   [>>] Macro = `Cblk'; Pattern = "II" --
			Nominal code-block dimensions (must be powers of 2 and
			no less than 4). Actual dimensions are subject to precinct, tile
			and image dimensions. Vertical coordinates appear first.
			Default block dimensions are {64,64}.
	   [>>] Macro = `Cmodes'; Pattern =
			"[BYPASS=1|RESET=2|RESTART=4|CAUSAL=8|ERTERM=16|SEGMARK=32]" --
			Block coder mode switches. Any combination is legal.  By default,
			all mode switches are turned off.
			When working with the `kdu_params::set' and `kdu_params::get'
			functions, use the macros, `Cmodes_BYPASS', `Cmodes_RESET',
			`Cmodes_RESTART', `Cmodes_CAUSAL', `Cmodes_ERTERM' and
			`Cmodes_SEGMARK' to set and test for specific modes.
	   [>>] Macro = `Cweight'; Pattern = "F",
			Multiplier for subband weighting factors (see `Clev_weights' and
			`Cband_weights' below).  Scaling all the weights by a single
			quantity has no impact on their relative significance.  However,
			you may supply a separate weight for each component, or even each
			tile-component, allowing you to control the relative signicance
			of image components or tile-components in a simple manner.
	   [>>] Macro = `Clev_weights'; Pattern = "F",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Weighting factors for each successive resolution level, starting
			from the highest resolution and working down to the lowest (but
			not including the LL band!!). The last supplied weight is repeated
			as necessary.  Weight values are squared to obtain energy weights
			for weighted MSE calculations.  The LL subband always has a weight
			of 1.0, regardless of the number of resolution levels.  However,
			the weights associated with all subbands, including the LL band,
			are multiplied by the value supplied by `Cweight', which may be
			specialized to individual components or tile-components.
	   [>>] Macro = `Cband_weights'; Pattern = "F",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Weighting factors for each successive subband, starting from the
			highest frequency subbands and working down (i.e., HH1, LH1, HL1,
			HH2, ...). The last supplied weight is repeated as necessary for
			all remaining subbands (except the LL band). If `Clev_weights' is
			also supplied, both sets of weighting factors are combined
			(multiplied).  Weight values are squared to obtain energy
			weights for weighted MSE calculations.  The LL subband always has
			a weight of 1.0, which avoids problems which may occur when image
			components or tiles have different numbers of resolution levels.
			To modify the relative weighting of components or tile-components,
			including their LL subbands, use the `Cweight' option; its
			weighting factors are multiplied by those specified using
			`Cband_weights' and `Clev_weights'.  If the `Cdecomp' attribute is
			used to describe more general packet wavelet transforms, all
			subbands obtained by splitting an HL, LH or HH subband will be
			assigned the same weight.  No mechanism is provided for specifying
			their weights separately.  Moreover, all three weights (HL, LH
			and HH) are present for each resolution level, even if that
			level only involves horizontal or vertical splitting, and even in
			the degenerate case of no splitting at all.  For horizontal
			splitting only, subbands derived from HX use the corresponding
			HL weight; HH and LH weights are then ignored.  Similarly for
			vertical splitting only, subbands derived from XH use the
			corresponding LH weight; HH and HL weights are then ignored.
	   [>>] Maro = `Creslengths'; Pattern = "I",
			[MULTI_RECORD] --
			Maximum number of compressed bytes (packet headers plus packet
			bodies) that can be produced for each successive image resolution,
			starting from the highest resolution and working down to the lowest.
			The limit applies to the cumulative number of bytes generated for
			the resolution in question and all lower resolutions.  If the
			parameter object in which this attribute is set is global to the
			entire codestream, the limit for each resolution applies to the
			cumulative number of bytes up to that resolution in all tiles and
			all image components.  If the parameter object is tile-specific
			but not component-specific, the limit for each resolution applies
			to the cumulative number of bytes up to that resolution for all
			image components within the tile.  If the parameter object is
			component-specific, the limit applies to the cumulative number of
			bytes up to the resolution in question across all tiles, but only
			in that image component.  Finally, if the parameter object is
			component-specific and tile-specific, the limit applies to the
			cumulative number of bytes up to the resolution in question, within
			just that tile-component.  You can provide limits of all four types.
			Moreover, you need not provide limits for all resolutions.
			The initial set of byte limits applies only to the first quality
			layer to be generated during compression.  Limits for additional
			quality layers may be supplied by inserting zero or negative values
			into the list; these are treated as layer delimiters.  So, for
			example, 1000,700,0,3000,2000,0,10000 provides limits of 1000 and
			700 bytes for the highest and second highest resolutions in the
			first quality layer, 3000 and 2000 bytes for the same resolutions
			in the second quality layer, and a limit of 10000 bytes only to the
			highest resolution in the third quality layer.  Any subsequent
			quality layers are not restricted by this parameter attribute.
	   [//]
	   We turn our attention now to a thorough discussion of the
	   `Cdecomp' parameter values, as promised above.  Each `Cdecomp'
	   record describes the subband splitting processes for one entire
	   DWT level, starting from the first (highest frequency) level.
	   All 32 bits of any given `Dstyle' parameter may be required to
	   describe the splitting structure for the relevant DWT level.  The
	   32 bits are divided into a sequence of 16 2-bit fields, labeled F0
	   through F15, where F0 represents the least significant 2 bits and
	   F15 represents the most significant 2 bits.
	   [//]
	   F0 identifies the primary splitting process associated with the DWT
	   level in question.  A value of F0=3 means that the image supplied to
	   that DWT level is split horizontally and vertically, as in the
	   standard Mallat decomposition structure.  This produces 3 primary
	   detail subbands, denoted LH, HL and HH.  A value of F0=2 means that the
	   image is split only in the vertical direction, producing primary detail
	   subband XH.  A value of F0=1 means that the image is split only in the
	   horizontal direction, producing primary detail subband HX.  Finally,
	   the degenerate value of F0=0 means that the DWT level does nothing at
	   all, passing its input image along to the next DWT level -- in this case,
	   no detail subbands are produced at all.
	   [//]
	   The values F1 through F15 describe additional subband splitting processes,
	   if any, which are applied to each primary detail subband produced at
	   the DWT level in question.  If F0=3, F1-F5 describe HL band splitting,
	   F6-F10 describe LH band splitting and F11-F15 describe HH band splitting.
	   If F0=1 or F0=2, only F1-F5 are used to describe additional splitting
	   for the single detail subband (HX or XH).  In the degenerate case of
	   F0=0, none of F1 through F15 are used.  Unused fields must all be set
	   to 0.  In the remainder of this discussion, we describe the splitting
	   process for a single primary detail subband via F1-F5 (the others are
	   the same).
	   [>>] If F1=0, the initial detail subband is not split further
	   [>>] If F1=1, the initial detail subband is split horizontally; F2 and
			F3 then describe any further splitting of the newly created
			horizontally low- and high-pass subbands, respectively.
	   [>>] If F1=2, the initial detail subband is split vertically; F2 and
			F3 then describe any further splitting of the newly created
			vertically low- and high-pass subbands, respectively.
	   [>>] If F1=3, the initial detail subband is split both horizontally
			and vertically; F2 through F5 then describe any further splitting
			of the newly created LL, HL, LH and HH subbands, in that order.
	   [>>] Unused fields must be 0.
	   [//]
	   Where required, F2 through F5 each take values in the range 0 to 3,
	   corresponding to no splitting, horizontal splitting, vertical splitting
	   and bi-directional splitting, respectively, of subbands produced
	   by the primary split.
	   [//]
	   When the splitting structures described above are represented by a
	   custom text string, the following conventions are used.
	   [>>] F0 is represented by one of the characters `-' (F0=0), `H'
			(F0=1), `V' (F0=2), or `B' (F0=3).  This character must be
			followed by a colon-separated list of additional splitting
			instructions, contained within parentheses.  In the degenerate
			case of F0=0, there can be no further splitting instructions, but
			the parentheses must still be present; this means that the
			custom string must be "-()".  In the case of a single primary
			detail subband (F0=1 or F0=2), the parentheses contain a single
			sub-string.  In the case of three primary detail subbands (F0=3),
			the parentheses contain three sub-strings, separated by colons.
			These describe the additional splitting instructions for the HL,
			LH and HH primary subbands, in that order.
	   [>>] Each sub-string consists of 1, 3 or 5 characters, drawn from the set
			`-' (meaning no-split), `H' (meaning horizontal split),
			`V' (meaning vertical split) and `B' (meaning bi-directional split).
	   [>>] If the sub-string commences with a `-', it may contain no further
			characters.  Evidently, this corresponds to the case F1=0 in the
			above numeric description.
	   [>>] If the sub-string commences with `H' or `V', it may two further
			characters, describing the splitting processes, if any, for the
			low- and high-pass subbands produced by the primary split, in that
			order.  Evidently, these correspond to F2 and F3 in the
			above numeric description.  Alternatively, the sub-string may
			contain only the initial `H' or `V', in which  case no
			further splitting occurs (F2=F3=0).
	   [>>] If the sub-string commences with `B', it may contain four further
			characters, describing the splitting processes, if any, for
			the LL, LH, HL and HH subbands produced by the primary split, in
			that order.  Evidently, these correspond to F2 through F5 in the
			above numeric description.  Alternatively, the sub-string may
			contain only the first character, in which case no further
			splitting occurs (F2=F3=F4=F5=0).
	   [//]
	   It is worth giving a few examples for clarification:
	   [>>] "B(-:-:-)" means that the input image is split horizontally and
			vertically, in standard Mallat style, with no further splitting of
			the primary detail subbands.
	   [>>] "V(HH-)" means that an initial vertical split produces only one
			primary detail subband at this DWT level; this is split
			horizontally and the horizontal low-pass subband produced in this
			way is split horizontally again.
	   [>>] "B(BBBBB:BBBBB:-)" means that the image supplied to this DWT
			level is split horizontally and vertically.  The HH primary subband
			produced by this primary split is left as-is, while the HL and LH
			primary subbands are each split into 16 uniform subbands.
	   [//]
	   If insufficient records are supplied during codestream creation,
	   to describe the subband splitting processes for all DWT levels,
	   the last descriptor is simply replicated for all lower resolution
	   DWT levels.  Note, however, that due to the way in which the JPEG2000
	   Part-2 standard defines extrapolation rules, the final splitting
	   descriptor must conform to the following rules:
	   [>>] The final splitting descriptor must be identical for all
			of the relevant primary detail subbands.  That is, where F0=3,
			F(k) = F(k+5) = F(k+10) for k=1,2,3,4,5.
	   [>>] All active 2-bit splitting codes must be identical, except where
			whole sub-levels are omitted -- that is, so long as there is
			no secondary splitting of the subbands produced by splitting
			each primary subband.  Thus, "B(-:-:-)", "B(BBBBB:BBBBB:BBBBB)",
			"B(HHH:HHH:HHH)", "V(V--)", "V(H--)", "B(V--:V--:V--)" and
			"V(VVV)" are all legal terminators, while "B(B:B:-)" and
			"B(V--:H--:B----)" are examples of illegal terminators.
	*/
public: // Member Functions
	KDU_EXPORT
		cod_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new cod_params; }
	virtual void finalize(bool after_reading = false);
	/* Call this function through the public `kdu_params::finalize'
	   function. */
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
	virtual int custom_parse_field(const char *string, const char *name,
		int field_idx, int &val);
	/* [SYNOPSIS]
		 Parses custom text strings describing the decomposition of any
		 given DWT level, following the description provided for the
		 `Cdecomp' attribute.
	*/
	virtual void custom_textualize_field(kdu_message &output,
		const char *name, int field_idx,
		int val);
	/* [SYNOPSIS]
		 Creates custom text strings describing the decomposition of any
		 given DWT level, following the description provided for the
		 `Cdecomp' attribute.
	*/
public: // Public helper functions, accessible to other classes
	KDU_EXPORT static bool
		is_valid_decomp_terminator(int val);
	/* [SYNOPSIS]
		 Applies the complex set of rules described above to check if
		 `val' is a legitimate terminating parameter for the `Cdecomp'
		 attribute.
	*/
	KDU_EXPORT static void
		textualize_decomp(char buf[], int val);
	/* [SYNOPSIS]
		 Writes a null-terminated textualized form of the `Cdecomp'
		 parameter in `val' into `buf'.  The supplied buffer must be
		 able to hold at least 20 characters, in addition to the null
		 terminator.
	*/
public:
	KDU_EXPORT static int
		transpose_decomp(int val);
	/* [SYNOPSIS]
		 This static member function takes a single `Cdecomp' parameter,
		 describing the splitting structure for one DWT level, and
		 returns a new parameter representing the equivalent splitting
		 structure for a transposed version of the image.  This is
		 used to implement the `copy_with_xforms' function, but also
		 has use in the implementation of other objects.  A definition
		 of the bit positions in the `val' word may be found with the
		 introduction to the `cod_params' object.
	*/
	KDU_EXPORT static int
		expand_decomp_bands(int decomp_val, kdu_int16 band_descriptors[]);
	/* [SYNOPSIS]
		 This function plays an important role in managing extended
		 decomposition styles, as identified by JPEG2000 Part-2 `ADS'
		 and/or `DFS' marker segments.
		 [//]
		 The `decomp_val' argument is the `Cdecomp' parameter associated
		 with a single DWT level.  The descriptors for each subband
		 produced at that DWT level are returned via the `band_descriptors'
		 array, which should have at least 49 elements.  The function also
		 returns the number of initial entries in this array which are
		 occupied by valid subband descriptors.
		 [//]
		 Upon return, the first entry in the `band_descriptors' array
		 always describes the low-pass subband, which is passed
		 on to the next DWT level, if any.  The maximum number of
		 additional subbands which can be produced at any level of the DWT
		 is 48 (16 for each initial detail subband, of which there can be
		 at most 3 -- HL, LH and HH).  Each subband identifies the exact
		 sequence of horizontal and vertical filtering operations which
		 is required to produce the subband from the low-pass subband
		 produced at the next higher DWT level (or from the original
		 image, if this is the first DWT level) -- call this the source
		 band.
		 [//]
		 The degenerate case, in which the 2 LSB's in `decomp_val' are
		 both 0, produces only one subband descriptor, the "low-pass"
		 subband, which is just a copy of the image supplied to this
		 DWT level.
		 [//]
		 Each subband descriptor contains 10 meaningful bits.  Bits 0-4
		 (the 5 LSB's) describe the horizontal filtering and decimation
		 processes which produce the subband from the source band,
		 while bits 8-12 (the 5 LSB's of the most significant byte in
		 the 16-bit descriptor) describe the vertical filtering and
		 decimation processes which produce the subband from the source
		 band.  There are anywhere from 0 to 3 successive horizontal
		 filtering steps and 0 to 3 successive vertical filtering steps.
		 We describe only the first 5 bits here, corresponding to
		 horizontal operations, since the remaining 5 bits play the
		 same role for vertical operations.
		 [>>] The two-bit word in bits 0-1 holds the number of horizontal
			  filtering and decimation operations, in the range 0 to 3.
		 [>>] Bit 2 holds 1 if the first of the stages identified above
			  is high-pass; if there are no horizontal filtering &
			  decimation stages), the value of this bit is 0.
		 [>>] Bit 3 holds 1 if the second of the stages identified above
			  is high-pass; it holds 0 if there is no such stage.
		 [>>] Bit 4 holds 1 if the third of the stages identified above
			  is high-pass; it holds 0 if there is no such stage.
	   [RETURNS]
		 Number of subbands produced at this DWT level, including the
		 low-pass subband which is typically passed on to the next
		 decomposition level.  This value is guaranteed to lie in the
		 range 1 to 49, where 1 is the degenerate case of a DWT level
		 which involves no subband decomposition whatsoever.
	   [ARG: band_descriptors]
		 Array with at least 49 elements, into which a descriptor for
		 each subband will be written.  The format of the descriptors is
		 documented above.  The order in which subband descriptors are
		 written into this array is important.  It is identical to the
		 order in which subband information is recorded within
		 codestream packets and identical to the order in which
		 quantization information is recorded within the QCD/QCC
		 marker segments, bearing in mind that the first entry in
		 the array is not a final subband, unless this is the last
		 DWT level.
	*/
	KDU_EXPORT static void
		get_max_decomp_levels(int decomp_val,
			int &max_horizontal_levels,
			int &max_vertical_levels);
	/* [SYNOPSIS]
		 This function returns the maximum number of horizontal
		 and vertical decomposition levels associated with any of the
		 subbands created by a single DWT level, having the supplied
		 decomposition descriptor.  The behaviour of the function is
		 equivalent to that which could be obtained by obtaining all
		 of the individual subband descriptors using `expand_decomp_bands'
		 and then taking the maximum of the horizontal filtering
		 depth fields (2 LSB's of least significant byte) and vertical
		 filtering depth fields (2 LSB's of the most significant byte)
		 within each such descriptor.  The present function, however,
		 obtains the same result much more efficiently.
	*/
private: // Internal helper functions
	int find_suitable_ads_idx();
	/* This function is called if unique `Cdecomp' information is
	   supplied, but there is no unique `Cads' index for some
	   tile-component.  It tries to find the most suitable index.  If
	   necessary, it creates a new `ads_params' object with this
	   instance index before returning the index which is found. */
	int find_suitable_dfs_idx();
	/* This function is called if unique `Cdecomp' information is
	   supplied, but there is no unique `Cdfs' index for some
	   tile-component.  It tries to find the most sutiable index.  If
	   necessary, it creates a new `dfs_params' object with this
	   instance index before returning the index which is found. */
	void derive_decomposition_structure(kdu_params *dfs, kdu_params *ads);
	/* This function uses the supplied DFS and ADS information to
	   set the `Cdecomp' attribute values.  Either, but not both of
	   `dfs' and `ads' may be NULL, in which case default values are
	   used. */
	void validate_ads_data(int ads_idx);
	/* This function accesses the `ads_params' object with the given
	   instance index.  If its `Ddecomp' attribute is empty, the function
	   copies all members of the `Cdecomp' attribute into `Ddecomp'.
	   Otherwise, it checks that the `Cdecomp' and `Ddecomp' attributes
	   hold identical values, generating an error through `kdu_error'
	   if they do not.  This function also verifies that the `Cdecomp'
	   attribute contains a valid terminator. */
	void validate_dfs_data(int dfs_idx);
	/* This function accesses the `dfs_params' object with the given
	   instance index.  If its `DSdfs' attribute is empty, the function
	   creates it from the `Cdecomp' contents.  Otherwise, it checks
	   that the `Cdecomp' information agrees with the existing information
	   in `DSdfs', generating an error through `kdu_error' if it does
	   not. */
};

/*****************************************************************************/
/*                                  ads_params                               */
/*****************************************************************************/

  // Cluster name
#define ADS_params "ADS"
  // Attributes recorded in an ADS marker segment
#define Ddecomp "Ddecomp" // Multiple records x "C"
#define DOads "DOads" // Multiple records x "(X=0,H=1,V=2,B=3)"
#define DSads "DSads" // Multiple records x "(X=0,H=1,V=2,B=3)"

class ads_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   This object works together with `dfs_params' and `cod_params' to
	   manage the information associated with JPEG2000 Part-2 `ADS' and `DFS'
	   marker segments.  Unfortunately, information which is split amongst
	   these various marker segments needs to be processed jointly in order
	   to extract any meaning from the `ADS' marker segment.  The reason
	   for this is that ADS describes the further splitting of primary high-pass
	   DWT subbands, to form packet wavelet transforms, but the number of
	   primary high-pass subbands and their properties depend upon information
	   contained in the `DFS' marker segment.  These are connected only
	   indirectly through references found in `COD' and `COC' marker
	   segments.  To make matters even more complicated, the connection between
	   DFS and ADS marker segments found in the main header cannot be
	   discovered until a tile header is encountered, in which they are
	   both used.  In some cases, no connection might be found at all, so
	   that the information in the main header can never be properly
	   interpreted.
	   [//]
	   To deal with the complications and uncertainties caused by this
	   division of information, Kakadu uses `cod_params' to store the fully
	   resolved and interpreted decomposition style information.  Specifically,
	   this information is stored in the single `Cdecomp' parameter attribute.
	   However, the `cod_params' object also manages indices (references) of
	   the DFS and/or ADS tables from which this information was originally
	   recovered, or to which it must be written.  These indices are found
	   in the `Cdfs' and `Cads' parameter attributes.  The `finalize' members of
	   these three parameter classes play a very significant role in
	   moving information between them and assigning table indices, where
	   appropriate.  To ensure that the logic works correctly, it is important
	   that the `ads_params' and `dfs_params' parameter clusters be linked
	   into the cluster list after `cod_params'.
	   [//]
	   This present object maintains two types of attributes.  The
	   `Ddecomp' attribute is absolutely identical to `Cdecomp'.  In fact, it
	   is always created by `cod_params::finalize'.  The `DOads' and `DSads'
	   attributes record the raw information contained in an ADS marker
	   segment.  This information can be extracted from `Ddecomp', but not
	   vice-versa.  The `ads_params::finalize' member performs this extraction
	   if it finds that `Ddecomp' has been filled in by `cod_params::finalize',
	   but the raw information is not yet available.  The `cod_params::finalize'
	   function, on the other hand, creates `Cdecomp' in the first place when
	   it encounters a non-trivial `Cads' index without accompanying
	   `Cdecomp' information; it does this using the raw `DOads' and
	   `DSads' information -- this happens when reading an existing codestream.
	   [//]
	   The cluster name is "ADS", but you are recommended to use the macro
	   `ADS_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The various parameter attributes are as follows:
	   [>>] Macro = `DOads'; Pattern = "I" [MULTI_RECORD] --
			Number of sub-levels in each successive DWT level, starting from
			the highest level.  Accesses to non-existent values are
			supported by repeating the last available value.  All entries
			must lie in the range 1 to 3.  For the meaning of sub-levels in
			JPEG2000 Part-2, the reader is referred to Annex F of IS 15444-2.
			You would not normally set values for this parameter attribute
			yourself.
	   [>>] Macro = `DSads'; Pattern = "(X=0,H=1,V=2,B=3)" [MULTI_RECORD] --
			Array of splitting instructions, whose interpretation depends
			upon other information available via the `cod_params' object whose
			`Cads' index agrees with our instance index.  All splitting
			instructions must lie in the range 0 to 3.  The last value is
			repeated as necessary, if accesses are made beyond the end of the
			array.  For the meaning of these splitting instructions, the
			reader is referred to Annex F of IS 15444-2.  You would not normally
			set values for this parameter attribute yourself.
	   [>>] Macro = `Ddecomp'; Pattern = "I" [MULTI_RECORD] --
			This attribute is created automatically from information in
			`Cdecomp'.  You should not set the attribute yourself!  There
			is one parameter for each DWT level, starting from the highest
			resolution level.  Each parameter describes the primary splitting
			structure at that level (DFS information), along with all sub-level
			splitting (ADS information).  The 32-bit integers used to build
			these descriptions are identical to those used by `Cdecomp'.
	*/
public: // Member Functions
	KDU_EXPORT
		ads_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new ads_params(); }
	virtual void finalize(bool after_reading = false);
	/* Call this function through the public "kdu_params::finalize'
	   function.  In this case, the function translates `Ddecomp' attributes
	   into `DSads' and `DOads' attributes, as required. */
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip)
	{
		return;
	}
	/* This function does not actually copy anything.  This ensures a
	   blank slate into which the correct values will be written when
	   `kdu_params::finalize_all' is called. */
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
	virtual void custom_textualize_field(kdu_message &output,
		const char *name, int field_idx,
		int val);
	/* [SYNOPSIS]
		 Creates custom text strings describing the decomposition of any
		 given DWT level, following the description provided for the
		 `Ddecomp' attribute.
	*/
};

/*****************************************************************************/
/*                                  dfs_params                               */
/*****************************************************************************/

  // Cluster name
#define DFS_params "DFS"
  // Attributes recorded in an DFS marker segment
#define DSdfs "DSdfs" // Multiple records x "(X=0,H=1,V=2,B=3)"

class dfs_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 Part-2 `DFS'
	   marker segments.  Component-specific forms are allowed, but not
	   tile-specific forms.
	   [//]
	   The cluster name is "DFS", but you are recommended to use the macro
	   `DFS_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   NOTE: It is important that you link `cod_params' into the cluster
	   list ahead of `dfs_params'.  For an explanation of this, see the
	   introductory comments appearing with the definition of `ads_params'.
	   [//]
	   The following attribute is defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `DSdfs'; Pattern = "(X=0,H=1,V=2,B=3)" --
			Describes the primary subband decomposition type associated with
			each DWT level, starting from the highest resolution (1'st level).
			The value may be one of `B' (split in both directions), `H'
			(split horizontally), `V' (split vertically) or `X' (don't split
			at all).  The last case is degenerate, since it means that the
			DWT level in question produces no detail subbands whatsoever,
			simply passing its input image through to the next DWT level.
			However, this can be useful in some circumstances.  The primary
			subband decomposition determines the downsampling factors between
			each successive resolution level.  If there are more DWT levels
			than `DSdfs' values, the last available value is replicated, as
			required.
	*/
public: // Member Functions
	KDU_EXPORT
		dfs_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new dfs_params(); }
	virtual void finalize(bool after_reading = false);
	/* Call this function through the public "kdu_params::finalize'
	   function. */
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip)
	{
		return;
	}
	/* This function does not actually copy anything.  This ensures a
	   blank slate into which the correct values will be written when
	   `kdu_params::finalize_all' is called. */
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                 qcd_params                                */
/*****************************************************************************/

  // Cluster name
#define QCD_params "QCD"
  // Attributes recorded in QCD/QCC marker segments
#define Qguard "Qguard" // One record x "I"
#define Qderived "Qderived" // One record x "B"
#define Qabs_steps "Qabs_steps" // Multiple records x "F"; No resizing
#define Qabs_ranges "Qabs_ranges" // Multiple records x "I"; No resizing
  // Attributes available only during content creation.
#define Qstep "Qstep" // One record x "F"

class qcd_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 `QCD' and `QCC'
	   marker segments.  Tile- and component-specific forms are allowed (in
	   fact, required for a complete code-stream description).
	   [//]
	   The cluster name is "QCD", but you are recommended to use the macro
	   `QCD_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   NOTE: It is important that you link `cod_params' into the cluster
	   list ahead of `qcd_params'.  This ensures that `cod_params::finalize'
	   will be called prior to `qcd_params::finalize' so that reversibility
	   can be reliably determined from the `Creversible' attribute.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Qguard'; Pattern = "I" --
			Number of guard bits to prevent overflow in the magnitude
			bit-plane representation. Typical values are 1 or 2.  Default is 1.
	   [>>] Macro = `Qderived'; Pattern = "B" --
			Quantization steps derived from LL band parameters?
			If true, all quantization step sizes will be related
			to the LL subband's step sizes through appropriate powers
			of 2 and only the LL band step size will be written in
			code-stream markers. Otherwise, a separate step size will
			be recorded for every subband. You cannot use this option
			with reversible compression.  Default is not derived.
	   [>>] Macro = `Qstep'; Pattern = "F" --
			Base step size to be used in deriving irreversible
			quantization step sizes for every subband. The base
			step parameter should be in the range 0 to 2.  Default is 1/256.
	   [>>] Macro = `Qabs_steps'; Pattern = "F",
			[MULTI_RECORD] --
			Absolute quantization step sizes for each subband,
			expressed as a fraction of the nominal dynamic range
			for that subband. The nominal range is equal to 2^B
			(B is the image sample bit-depth) multiplied by the DC gain of
			each low-pass subband analysis filter and the AC gain of each
			high-pass subband analysis filter, involved in the construction
			of the relevant subband.  The bands are described one by one,
			in the following sequence: LL_D, HL_D, LH_D, ..., HL_1, LH_1,
			HH_1.  Here, D denotes the number of DWT levels.  A single
			step size must be supplied for every subband (there is
			no extrapolation), except in the event that `Qderived' is set
			to true -- then, only one parameter is allowed, corresponding to the
			LL_D subband.  For compressors, the absolute step sizes are
			ignored if `Qstep' has been used.
	   [>>] Macro = `Qabs_ranges'; Pattern = "I",
			[MULTI_RECORD] --
			Number of range bits used to code each subband during
			reversible compression.  Subbands appear in the sequence,
			LL_D, HL_D, LH_D, ..., HL_1, LH_1, HH_1, where D denotes
			the number of DWT levels.  The number of range bits for a
			reversibly compressed subband, plus the number of guard
			bits (see `Qguard'), is equal to 1 plus the number of
			magnitude bit-planes which are used for coding its
			samples.  For compressors, most users will accept the
			default policy, which sets the number of range bits to
			the smallest value which is guaranteed to avoid overflow
			or underflow in the bit-plane representation, assuming that
			the RCT (colour transform) is used.  If explicit values are
			supplied, they must be given for each and every subband.
	*/
public: // Member Functions
	KDU_EXPORT
		qcd_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new qcd_params(); }
	virtual void finalize(bool after_reading = false);
	/* Call this function through the public "kdu_params::finalize'
	   function. */
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                   rgn_params                              */
/*****************************************************************************/

  // Cluster name
#define RGN_params "RGN"
  // Attributes recorded in RGN marker segments
#define Rshift "Rshift" // One record x "I"
  // Attributes available only during content creation.
#define Rlevels "Rlevels" // One record x "I"
#define Rweight "Rweight" // One record x "F"

class rgn_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 `RGN'
	   (encoder-side region of interest) marker segment.
	   Tile- and component-specific forms are allowed (in
	   fact, required for a complete code-stream description).
	   [//]
	   The cluster name is "RGN", but you are recommended to use the macro
	   `RGN_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Rshift'; Pattern = "I" --
			Region of interest up-shift value.  All subband samples
			which are involved in the synthesis of any image sample
			which belongs to the foreground region of an ROI mask
			will be effectively shifted up (scaled by two the power
			of this shift value) prior to quantization.  The
			region geometry is specified independently and is not
			explicitly signalled through the code-stream; instead,
			this shift must be sufficiently large to enable the
			decoder to separate the foreground and background
			on the basis of the shifted sample amplitudes alone.
			You will receive an appropriate error message if the
			shift value is too small.  Default is 0.
	   [>>] Macro = `Rlevels'; Pattern = "I" --
			Number of initial (highest frequency) DWT levels through
			which to propagate geometric information concerning the
			foreground region for ROI processing.  Additional
			levels (i.e., lower frequency subbands) will be treated
			as belonging entirely to the foreground region.  Default is 4.
	   [>>] Macro = `Rweight'; Pattern = "F" --
			Region of interest significance weight.  Although this
			attribute may be used together with `Rshift', it is
			common to use only one or the other.  All code-blocks
			whose samples contribute in any way to the reconstruction
			of the foreground region of an ROI mask will have their
			distortion metrics scaled by the square of the supplied
			weighting factor, for the purpose of rate allocation.
			This renders such blocks more important and assigns to
			them relatively more bits, in a manner which
			is closely related to the effect of the `Clevel_weights'
			and `Cband_weights' attributes on the importance of
			whole subbands.  Note that this region weighting
			strategy is most effective when working with large
			images and relatively small code-blocks (or precincts).
			Default is 1, i.e., no extra weighting.
	*/
public: // Member Functions.
	KDU_EXPORT
		rgn_params();
protected: // Member functions
	virtual void finalize(bool after_reading = false);
	/* Invoke this function through the public `kdu_params::finalize'
	   member. */
	virtual kdu_params *new_object() { return new rgn_params(); }
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                   poc_params                              */
/*****************************************************************************/

  // Cluster name
#define POC_params "POC"
  // Attributes recorded in POC marker segments
#define Porder "Porder" // Multiple records x "IIIII(LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4)"

class poc_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 `POC' marker segment.
	   Tile-specific are allowed, but not component-specific forms.
	   [//]
	   The cluster name is "POC", but you are recommended to use the macro
	   `POC_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `Porder'; Pattern =
			"IIIII(LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4)",
			[MULTI_RECORD] --
			Progression order change information. There may be
			multiple instances of this attribute, in which case
			each instance corresponds to a tile-part boundary.
			Each instance may contain one or more progression
			records, each of which defines the order for
			a collection of packets. Each record contains 6 fields.
			The first two fields identify inclusive lower bounds for
			the resolution level and image component indices,
			respectively. The next three fields identify exclusive
			upper bounds for the quality layer, resolution level and
			image component indices, respectively. All indices are
			zero-based, with resolution level 0 corresponding to the
			LL_D subband. The final field in each record identifies
			the progression order to be applied within the indicated
			bounds. This order is applied only to those packets which
			have not already been sequenced by previous records or
			instances.
	*/
public: // Member Functions
	KDU_EXPORT
		poc_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new poc_params(); }
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                   crg_params                              */
/*****************************************************************************/

  // Cluster name
#define CRG_params "CRG"
  // Attributes recorded in the CRG marker segment
#define CRGoffset "CRGoffset" // Multiple records x "FF"

class crg_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   Manages the information associated with JPEG2000 `CRG' (component
	   registration) marker segment.  Only one instance appears in a
	   single code-stream; tile-specific and component-specific forms
	   are not allowed.
	   [//]
	   The cluster name is "CRG", but you are recommended to use the macro
	   `CRG_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `CRGoffset'; Pattern = "FF",
			[MULTI_RECORD, CAN_EXTRAPOLATE] --
			Provides additional component registration offsets.
			The offsets add to those implied by the canvas
			coordinate system and should only be used when
			canvas coordinates (notably `Ssize', `Soffset' and
			`Ssampling') cannot be found, which adequately reflect
			the relative displacement of the components. Each record
			specifies offsets for one component, with the vertical
			"offset appearing first. Offsets must be in the range
			0 (inclusive) to 1 (exclusive) and represent a fraction
			of the relevant component sub-sampling factor (see
			`Ssampling'). The last supplied record is repeated
			as needed to recover offsets for all components.
	*/
public: // Member Functions
	KDU_EXPORT
		crg_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new crg_params(); }
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual int write_marker_segment(kdu_output *out, kdu_params *last_marked,
		int tpart_idx);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx);
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx);
};

/*****************************************************************************/
/*                                   org_params                              */
/*****************************************************************************/

  // Cluster name
#define ORG_params "ORG"
  // ORG parameter attributes
#define ORGtparts  "ORGtparts"  // One record x "[R=1|L=2|C=4]"
#define ORGgen_tlm "ORGgen_tlm" // One record x "I"
#define ORGgen_plt "ORGgen_plt" // One record x "B"
#define ORGtlm_style "ORGtlm_style" // One record x "(implied=0,byte=1,short=2)
									//               (short=2,long=4)"
  // Values for the "ORDtparts" attribute
#define ORGtparts_R    ((int) 1)
#define ORGtparts_L    ((int) 2)
#define ORGtparts_C    ((int) 4)
#define ORGtlm_style_implied  ((int) 0)

class org_params : public kdu_params {
	/* [BIND: reference]
	   [SYNOPSIS]
	   The `org_params' parameter class records attributes which control
	   aspects of the code-stream organization (hence the name, "ORG").  None
	   of these attributes are actually preserved in code-stream marker
	   segments, but they may be established during compression in the same
	   manner as any other code-stream parameter attributes, most of which do
	   wind up in marker segments.  Tile-specific forms of the ORG parameters
	   exist, but component-specific forms do not.
	   [//]
	   The cluster name is "ORG", but you are recommended to use the macro
	   `ORG_params', in functions which take a cluster name like
	   `kdu_params::access_cluster'.
	   [//]
	   The following attributes are defined.  For an explanation of pattern
	   strings, consult the comments appearing with `kdu_params::parse_string'.
	   [>>] Macro = `ORGtparts'; Pattern = "[R=1|L=2|C=4]" --
			Controls the division of each tile's packets into
			tile-parts.  The attribute consists of one or
			more of the flags, `R', `L' and `C', separated by
			the vertical bar character, `|'.  If the `R' flag is
			supplied, tile-parts will be introduced as necessary
			to ensure that each tile-part consists of packets from
			only one resolution level.  If `L' is supplied,
			tile-parts are introduced as necessary to ensure that
			each tile-part consists of packets from only one
			quality layer.  Similarly, if the `C' flag is supplied,
			each tile-part will consist of packets from only one
			component.  Note that the cost of extra tile-part
			headers will not be taken into account during rate
			control, so that the code-stream may end up being a
			little larger than you expect.  By default, tile-part
			boundaries are introduced only as required by the presence
			of multiple `Porder' attribute specifications (see `poc_params').
	   [>>] Macro = `ORGgen_plt'; Pattern = "B" --
			Requests the insertion of packet length information in
			the header of all tile-parts associated with tiles for which this
			attribute is true.  The PLT marker segments written into the
			relevant tile-part headers will hold the lengths of
			those packets which belong to the same tile-part.  Note
			that the cost of any PLT marker segments generated
			as a result of this attribute being enabled will not be
			taken into account during rate allocation.  This means
			that the resulting code-streams will generally be a
			little larger than one might expect; however, this is
			probably a reasonable policy, since the PLT marker
			segments may be removed without losing any information.
	   [>>] Macro = `ORGgen_tlm'; Pattern = "I" --
			Requests the insertion of TLM (tile-part-length) marker
			segments in the main header, to facilitate random access
			to the code-stream.  This attribute takes a single integer-valued
			parameter, which identifies the maximum number of tile-parts
			which will be written to the code-stream for each tile.  The
			reason for including this parameter is that space for the
			TLM information must be reserved ahead of time; once the
			entire code-stream has been written the generation machinery
			goes back and overwrites this reserved space with actual TLM
			data.  If the actual number of tile-parts which are generated
			is less than the value supplied here, empty tile-parts will be
			inserted into the code-stream so as to use up all of the
			reserved TLM space.  For this reason, you should try to
			estimate the maximum number of tile-parts you will need as
			accurately as possible, noting that the actual value may be
			hard to determine ahead of time if incremental flushing
			features are to be employed.  An error will be generated at
			run-time if the number of declared maximum number of tile-parts
			turns out to be insufficient.  You should note that this
			attribute may be ignored if the `kdu_compressed_target' object,
			to which generated data is written, does not support repositioning
			via `kdu_compressed_target::start_rewrite'.
	   [>>] Macro = `ORGtlm_style';
			Pattern="(implied=0,byte=1,short=2)(short=2,long=4)" --
			Can be used to control the format used to record TLM
			(tile-part-length) marker segments; it is relevant only in
			conjunction with "ORGgen_tlm".  The standard defines 6
			different formats for the TLM marker segment, some of which
			are more compact than others.  The main reason for providing
			this level of control is that some applications/profiles may
			expect a specific format to be used.  By default, each record
			in a TLM marker segment is written with 6 bytes, 2 of which
			identify the tile number, while the remaining 4 give the length
			of the relevant tile-part.  This attribute takes two fields: the
			first field specifies the number of bytes to be used to record
			tile numbers (0, 1 or 2); the second field specifies the number
			of bytes to be used to record tile-part lengths (2 or 4).  The
			values provided here might not be checked ahead of time,
			which means that some combinations may be found to be illegal
			at some point during the compression process.  Also, the first field
			may be 0 (`ORGtlm_style_implied') only if tiles are written in
			order and have exactly one tile-part each.  This is usually the
			case if "ORGtparts" is not used, but incremental flushing of tiles
			which are generated in an unusual order may violate this
			assumption -- this sort of thing can happen if
			`kdu_codestream::change_appearance' is used to
			compress imagery which is presented in a transposed or flipped
			order, for example.
	*/
public: // Member Functions
	KDU_EXPORT
		org_params();
protected: // Member functions
	virtual kdu_params *new_object() { return new org_params(); }
	virtual void copy_with_xforms(kdu_params *source, int skip_components,
		int discard_levels, bool transpose, bool vflip, bool hflip);
	virtual bool check_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int &c_idx)
	{
		return false;
	} // No marker segments for ORG parameters
	virtual bool read_marker_segment(kdu_uint16 code, int num_bytes,
		kdu_byte bytes[], int tpart_idx)
	{
		return false;
	} // No marker segments for ORG parameters
};


#endif // KDU_PARAMS_H
