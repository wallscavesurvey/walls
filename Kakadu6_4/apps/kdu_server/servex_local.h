/*****************************************************************************/
// File: servex_local.h [scope = APPS/SERVER]
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
   Private header file used by "kdu_servex.cpp".
******************************************************************************/

#ifndef SERVEX_LOCAL_H
#define SERVEX_LOCAL_H

#include "kdu_compressed.h"
#include "kdu_servex.h"
#include "kdu_file_io.h"

// Defined here:
struct kdsx_event;
class kdsx_image_entities;
struct kdsx_metagroup;
class kdsx_stream;
struct kdsx_context_mappings;
struct kdsx_stream_mapping;
struct kdsx_layer_member;
struct kdsx_layer_mapping;
struct kdsx_comp_instruction;

/*****************************************************************************/
/*                                kdsx_event                                 */
/*****************************************************************************/

struct kdsx_event {
  kdsx_event() { event.create(false); next = NULL; }
  ~kdsx_event() { event.destroy(); }
  kdu_event event; // This is an auto-reset event
  kdsx_event *next;
};

/*****************************************************************************/
/*                           kdsx_image_entities                             */
/*****************************************************************************/

// Extends `kds_image_entities' to include private book-keeping information.
class kdsx_image_entities : public kds_image_entities {
  public: // Member functions
    kdsx_image_entities()
      { num_entities=max_entities=0; universal_flags=0;
        entities=NULL; next=prev=NULL; ref_id=-1; }
    ~kdsx_image_entities() { if (entities != NULL) delete[] entities; }
    void add_entity(kdu_int32 idx);
      /* Adds a new entity, keeping the sorting order.  It does
         no harm to add existing entities over again. */
    void add_universal(kdu_int32 flags);
      /* `flags' is either or both of 0x01000000 (add all codestreams) or
         0x02000000 (add all compositing layers). */
    void copy_from(kdsx_image_entities *src)
      {
        add_universal(src->universal_flags);
        for (int n=0; n < src->num_entities; n++)
          add_entity(src->entities[n]);
      }
      /* Adds elements from `src' to the current list of image entities,
         avoiding duplicates. */
    kdsx_image_entities *find_match(kdsx_image_entities *head,
                                    kdsx_image_entities * &goes_after_this);
      /* Looks on the list headed by `head' for an object identical to
         this one, returning a pointer to the object it finds.  If none
         is found, the function returns NULL, but `goes_after_this' points
         to the object immediately after which a copy of the present
         object should be added (NULL, if it should be added at the head). */
    void reset()
      { num_entities = 0; universal_flags = 0; }
    void serialize(FILE *fp);
    void deserialize(FILE *fp);
  public: // Data
    int max_entities; // Size of the base object's `entities' array.
    int ref_id; // Used for serialization and deserialization
    kdsx_image_entities *next; // Used to build doubly linked lists
    kdsx_image_entities *prev;
  };

/*****************************************************************************/
/*                              kdsx_metagroup                               */
/*****************************************************************************/

// Extends `kds_metagroup' to include private book-keeping information.
struct kdsx_metagroup : public kds_metagroup {
  public: // Member functions
    kdsx_metagroup(kdu_servex *owner);
      /* Sets all member variables to 0 or NULL. */
    virtual ~kdsx_metagroup();
    void create(kdsx_metagroup *parent,
                kdsx_image_entities *parent_entities, jp2_input_box *box,
                int max_size, kdu_long &last_used_bin_id,
                int &num_codestreams, int &num_jpch, int &num_jplh,
                kdu_long fpos_lim, bool is_first_subbox_of_asoc);
      /* Initializes a newly constructed object to represent the supplied
         open box.  Each group created by this function holds a single
         top-level box or else a placeholder to a list of box groups
         representing its sub-boxes.  The `max_size' argument is used to
         decide whether or not a box should be replaced by a placeholder.
         If so, the `last_used_bin_id' argument is incremented and used for
         the new meta data-bin identifier.
            The `num_codestreams' argument is used to keep track of the
         number of contiguous codestream boxes or fragment table boxes
         which have been seen so far.
            Similarly, `num_jpch' and `num_jplh' keep track of the number
         of codestream headers and compositing layer header boxes which have
         been seen so far.
            Whenever a contiguous codestream or fragment table is encountered,
         a new `kdsx_stream' object is created using `kdu_server::add_stream'.
            The `fpos_lim' argument provides an exclusive upper bound to
         file positions which can be considered part of the target.  The
         limit is applied when filling in group lengths and stream lengths. */
    void inherit_child_scope(kdsx_image_entities *entities,
                             kds_metagroup *child);
      /* This function is called after parsing descendants of a given
         metagroup to augment the scope of the current group so as to
         represent a union of all its children.  Normally, this is done
         after all sub-groups have been created so that the sub-groups
         inherit from their parents only new scope which is induced by
         information in the parent box itself.  However, for association
         boxes, the first sub-box's scope is added to the association
         box immediately before passing that scope into all the other
         sub-boxes.   Augmenting the current scope means growing regions
         of interest so as to include that of the child (if any), augmenting
         the list of associated image entities, and including all flags
         from the child except KDS_METAGROUP_LEAF and
         KDS_METAGROUP_INCLUDE_NEXT_SIBLING.  The image entities which will
         eventually form part of the current object's scope are recorded
         in the temporary `entities' object, rather than in the
         `scope->entities' member which will eventually be formed by
         invoking `kdu_servex::commit_image_entities' on the temporary
         `entities' object. */
    void serialize(FILE *fp);
      /* For saving the meta-data structure to a file.  Recursively serializes
         placeholders. */
    bool deserialize(kdsx_metagroup *parent, FILE *fp);
      /* Recursively deserializes placeholders.  Returns true if there are
         still more meta-data groups to be deserialized. Otherwise returns
         false.  Generates an error if parsing fails. */
  private: // Helper functions
    int synthesize_placeholder(kdu_long original_header_length,
                               kdu_long original_box_length,
                               int stream_id);
      /* Allocates the `data' array and writes a placeholder box into it,
         returning the total length of the placeholder box.  The placeholder
         is to represent a box whose type code is in the `last_box_type'
         member and whose original header length and total length are as
         indicated.  If the original box type is a code-stream, an
         incremental code-stream placeholder box is written, using
         `stream_id' for the code-stream identifier.  For non-codestream
         placeholders, the data-bin ID is found in the `phld_bin_id'
         member. */
  public: // Data
    kdu_servex *owner;
    kdsx_metagroup *parent;
    kds_metascope scope_data; // Give each group its own scope record
    kdu_byte *data; // Non-NULL only if boxes do not exist in file.
  };

/*****************************************************************************/
/*                            kdsx_stream_suminfo                            */
/*****************************************************************************/

struct kdsx_stream_suminfo {
  public: // Functions
    kdsx_stream_suminfo()
      { 
        max_discard_levels=max_quality_layers=0;
        num_components=num_output_components=0;
        component_subs = output_component_subs = NULL;
      }
    ~kdsx_stream_suminfo()
      { 
        if (component_subs != NULL)
          delete[] component_subs;
        if (output_component_subs != NULL)
          delete[] output_component_subs;
      }
    void create(kdu_codestream cs);
    bool equals(const kdsx_stream_suminfo *src);
    void serialize(FILE *fp, kdu_servex *owner);
    void deserialize(FILE *fp, kdu_servex *owner);
  public: // Data
    kdu_dims image_dims;
    kdu_dims tile_partition;
    kdu_dims tile_indices;
    int max_discard_levels;
    int max_quality_layers;
    int num_components;
    int num_output_components;
    kdu_coords *component_subs;
    kdu_coords *output_component_subs;
  };
  /* Notes: this structure stores summary information concerning a
     single codestream that is likely to be replicated across all
     codestreams in a video stream. */

/*****************************************************************************/
/*                                kdsx_stream                                */
/*****************************************************************************/

class kdsx_stream : public kdu_compressed_source {
  public: // Member functions
    kdsx_stream()
      {
        stream_id = -1;
        suminfo = local_suminfo=NULL; layer_stats_handle = NULL;
        num_layer_stats = 0; layer_log_slopes = NULL; layer_lengths = NULL;
        url_idx = 0; target_filename = NULL; expanded_filename = NULL;
        start_pos = length = 0; num_attachments = num_waiting_for_lock = 0;
        fp = NULL; rel_pos = 0; next = NULL; locking_thread_handle = NULL;
        per_client_cache = 0;
        unlock_event = NULL;
      }
    ~kdsx_stream()
      { 
        close();
        if (expanded_filename != NULL)
          delete[] expanded_filename;
        if (local_suminfo != NULL)
          delete local_suminfo;
        suminfo = local_suminfo = NULL;
        if (layer_stats_handle != NULL)
          { delete layer_stats_handle; layer_stats_handle = NULL; }
        layer_log_slopes = NULL; layer_lengths = NULL;
      }
    virtual bool close()
      { // Called when `num_attachments' goes to 0
        if (codestream.exists())
          codestream.destroy();
        if (fp != NULL)
          { fclose(fp); fp = NULL; }
        assert(unlock_event == NULL);          
        return true;
      }
    void open(const char *parent_filename);
      /* Call this on first attachment. */
    virtual int get_capabilities()
      { return KDU_SOURCE_CAP_SEQUENTIAL | KDU_SOURCE_CAP_SEEKABLE; }
    virtual bool seek(kdu_long offset)
      { assert(fp != NULL);
        if (offset > length) offset = length;
        if (offset < 0) offset = 0;
        rel_pos = offset;
        return true;
      }
    virtual kdu_long get_pos()
      {
        return (fp==NULL)?-1:rel_pos;
      }
    virtual int read(kdu_byte *buf, int num_bytes)
      {
        assert(fp != NULL);
        if ((rel_pos+num_bytes) > length)
          num_bytes = (int)(length-rel_pos);
        if (num_bytes <= 0) return 0;
        kdu_fseek(fp,start_pos+rel_pos);
        num_bytes = (int) fread(buf,1,(size_t) num_bytes,fp);
        rel_pos += num_bytes;
        return num_bytes;
      }
    void serialize(FILE *fp, kdu_servex *owner);
      /* For saving the code-stream structure to a file. */
    void deserialize(FILE *fp, kdu_servex *owner);
      /* For recovering the saved code-stream structure from a file. */
  public: // Data members used to summarize the codestream's properties
    int stream_id;
    const kdsx_stream_suminfo *suminfo; // local_suminfo or global default
    kdsx_stream_suminfo *local_suminfo; // May be NULL
    int num_layer_stats; // Length of two arrays below
    int *layer_log_slopes;   // Pointers into `layer_stats_handle'
    kdu_long *layer_lengths;
    int *layer_stats_handle;
  public: // Data members used to access the codestream itself
    int url_idx; // 0 if main file, else index into data references box
    const char *target_filename; // Pointer to name of target file
    char *expanded_filename; // Non-NULL if `target_filename' is relative
    kdu_long start_pos; // Absolute location of the stream within the file
    kdu_long length; // Length of stream
    int num_attachments; // May close file and `codestream' when this goes to 0
    int num_waiting_for_lock; // Number of threads waiting to lock this stream
    kd_serve *locking_thread_handle; // NULL unless a thread has the lock
    kdsx_event *unlock_event; // See below
    FILE *fp; // Open file pointer (or NULL)
    kdu_long rel_pos; // Position of open file pointer relative to `start_pos'
    int per_client_cache;
    kdu_codestream codestream; // Exists only if `fp' non-NULL
    kdsx_stream *next; // To build a linked list of streams in the file
  };
  /* The `num_waiting_for_lock', `locking_thread_handle' and `unlock_event'
     members work together to implement the functionality of
     `kdu_serve_target::lock_codestreams' and
     `kdu_serve_target::release_codestreams'.  The `unlock_event' member
     becomes non-NULL if there is at least one thread waiting to lock the
     stream.  The `kdu_event::set' function should be invoked on the
     internal event once a thread relinquishes its lock on the stream.
     Once a thread wakes up from a lock, the `num_waiting_for_lock' member
     may be used to determine whether there are other threads waiting for
     the same stream; if not, the event is recycled to the `kdu_servex'
     object's `free_events' list.  The underlying `kdu_event' object is
     created for auto-reset, which means that the operating system unblocks
     only one waiting thread at a time. */

/*****************************************************************************/
/*                           kdsx_stream_mapping                             */
/*****************************************************************************/

struct kdsx_stream_mapping {
  /* This object represents information which might be recovered from a
     codestream header box, which is required for mapping codestream
     contexts to codestreams and components. */
  public: // Member functions
    kdsx_stream_mapping()
      { num_channels = num_components = 0; component_indices = NULL; }
    ~kdsx_stream_mapping()
      { if (component_indices != NULL) delete[] component_indices; }
    void parse_ihdr_box(jp2_input_box *box);
      /* Parses to the end of the image header box leaving it open, using
         the information to set the `size' and `num_components' members. */
    void parse_cmap_box(jp2_input_box *box);
      /* Parses to the end of the component mapping box leaving it open,
         using the information to set the `num_channels' and
         `component_indices' members. */
    void finish_parsing(kdsx_stream_mapping *defaults);
      /* Fills in any information not already parsed, by using defaults
         (parsed from within the JP2 header box) as appropriate. */
    void serialize(FILE *fp);
    void deserialize(FILE *fp);
      /* The above functions save and reload only the `size' and
         `num_components' values.  The channel indices are required only
         during parsing, to compute the components which belong to each
         compositing layer. */
  public: // Data
    kdu_coords size;
    int num_components;
    int num_channels;
    int *component_indices; // Lists component indices of each channel
  };

/*****************************************************************************/
/*                           kdsx_layer_member                               */
/*****************************************************************************/

struct kdsx_layer_member {
  public: // Member functions
    kdsx_layer_member()
      { codestream_idx = num_components = 0; component_indices = NULL; }
    ~kdsx_layer_member()
      { if (component_indices != NULL) delete[] component_indices; }
  public: // Data
    int codestream_idx;
    kdu_coords reg_subsampling;
    kdu_coords reg_offset;
    int num_components;
    int *component_indices;
  };

/*****************************************************************************/
/*                           kdsx_layer_mapping                              */
/*****************************************************************************/

struct kdsx_layer_mapping {
  public: // Member functions
    kdsx_layer_mapping()
      { num_members = num_channels = num_colours = 0;
        have_opct_box = have_opacity_channel = false;
        layer_idx = -1; members=NULL; channel_indices=NULL; }
    kdsx_layer_mapping(int layer_idx)
      { num_members = num_channels = num_colours = 0;
        have_opct_box = have_opacity_channel = false;
        this->layer_idx = layer_idx; members=NULL; channel_indices=NULL; }
    ~kdsx_layer_mapping()
      {
        if (members != NULL) delete[] members;
        if (channel_indices != NULL) delete[] channel_indices;
      }
    void parse_cdef_box(jp2_input_box *box);
      /* Parses to the end of the channel definition box leaving it open, using
         the information to set the `num_channels' and `channel_indices'
         members. */
    void parse_creg_box(jp2_input_box *box);
      /* Parses to the end of the codestream registration box, leaving it open,
         using the information to set the `num_members' and `members'
         members. */
    void parse_opct_box(jp2_input_box *box);
      /* Parses the opacity box, leaving it open.  Sets `have_opct_box' to
         true.  Also sets `have_opacity_channel' to true if the box has
         an OTyp field equal to 0 or 1.  In this case, there will be no
         channel definition box, but `num_colours' must be augmented by 1
         to get the number of channels used. */
    void parse_colr_box(jp2_input_box *box);
      /* Parses to the end of the colour description box leaving it open,
         using the information to set the `num_colours' member if possible.
         Once any colour description box is encountered, the value of
         `num_colours' is set to a non-zero value -- either the actual
         number of colours (if the box can be parsed) or -1 (if the box
         cannot be parsed). */
    void finish_parsing(kdsx_layer_mapping *defaults,
                        kdsx_context_mappings *owner);
      /* Fills in uninitialized members with appropriate defaults and
         determines the `kdsx_layer_member::num_components' values and
         `kdsx_layer_member::component_indices' arrays, based on the
         available information. */
    void serialize(FILE *fp);
    void deserialize(FILE *fp, kdsx_context_mappings *owner);
      /* The above functions save and reload everything except for the
         channel indices, since these are used only while parsing, to
         determine the set of image components which are involved from
         each codestream which is used by each compositing layer. */
  public: // Data
    int layer_idx;
    kdu_coords layer_size;
    kdu_coords reg_precision;
    int num_members;
    kdsx_layer_member *members;
    bool have_opct_box; // If opacity box was found in the layer header
    bool have_opacity_channel; // If true, `num_channels'=1+`num_colours'
    int num_colours; // Used to set `num_channels' if no `cdef' box exists
    int num_channels;
    int *channel_indices; // Sorted into increasing order
  };

/*****************************************************************************/
/*                          kdsx_comp_instruction                            */
/*****************************************************************************/

struct kdsx_comp_instruction {
  public: // Data
    kdu_dims source_dims; // After cropping
    kdu_dims target_dims; // On final rendering grid
  };
  /* Notes:
       If `source_dims' has zero area, the source JPX compositing layer is to
     be used without any cropping.  Otherwise, it is cropped by
     `source_dims.pos' from the upper left hand corner, and to a size of
     `source_dims.size' on the compositing layer registration grid.
       If `target_dims' has zero area, the compositing layer is to be
     placed on the rendering canvas without scaling.  The size is derived
     either from `source_dims.size' or, if `source_dims' has zero area, from
     the original compositing layer dimensions. */

/*****************************************************************************/
/*                          kdsx_context_mappings                            */
/*****************************************************************************/

struct kdsx_context_mappings {
  public: // Member functions
    kdsx_context_mappings()
      {
        num_codestreams = max_codestreams = 0;
        num_compositing_layers = max_compositing_layers = 0;
        stream_refs = NULL; layer_refs = NULL;
        num_comp_sets = max_comp_sets = 0;
        comp_set_starts = NULL;
        num_comp_instructions = max_comp_instructions = 0;
        comp_instructions = NULL;
      }
    ~kdsx_context_mappings()
      {
        int n;
        for (n=0; n < num_codestreams; n++)
          delete stream_refs[n];
        if (stream_refs != NULL) delete[] stream_refs;
        for (n=0; n < num_compositing_layers; n++)
          delete layer_refs[n];
        if (layer_refs != NULL) delete[] layer_refs;
        if (comp_set_starts != NULL) delete[] comp_set_starts;
        if (comp_instructions != NULL) delete[] comp_instructions;
      }
    kdsx_stream_mapping *add_stream(int idx);
      /* If the indicated codestream does not already have an allocated
         `kdsx_stream_mapping' object, one is created here.  In any event,
         the relevant object is returned. */
    kdsx_layer_mapping *add_layer(int idx);
      /* If the indicated compositing layer does not already have an allocated
         `kdsx_layer_mapping' object, one is created here.  In any event,
         the relevant object is returned. */
    void parse_copt_box(jp2_input_box *box);
      /* Parses to the end of the composition options box, leaving it open,
         and using the contents to set the `composited_size' member. */
    void parse_iset_box(jp2_input_box *box);
      /* Parses to the end of the composition instruction box, leaving it open,
         and using the contents to add a new composition instruction set to
         the `comp_set_starts' array and to parse its instructions into the
         `comp_instructions' array, adjusting the relevant counters. */
    void finish_parsing(int num_codestreams, int num_layer_headers);
      /* The arguments indicate the total number of codestreams encountered,
         and the total number of compositing layer headers encountered while
         parsing boxes. */
    void serialize(FILE *fp);
    void deserialize(FILE *fp);
  public: // Data
    kdsx_stream_mapping stream_defaults;
    kdsx_layer_mapping layer_defaults;
    int num_codestreams;
    int max_codestreams; // Size of following array
    kdsx_stream_mapping **stream_refs;
    int num_compositing_layers;
    int max_compositing_layers; // Size of following array
    kdsx_layer_mapping **layer_refs;
    kdu_coords composited_size; // Valid only if `num_comp_sets' > 0
    int num_comp_sets; // Num valid elts in `comp_set_starts' array
    int max_comp_sets; // Max elts in `comp_set_starts' array
    int *comp_set_starts; // Index of first instruction in each comp set
    int num_comp_instructions; //  Num valid elts in `comp_instructions' array
    int max_comp_instructions; // Max elts in `comp_instructions' array
    kdsx_comp_instruction *comp_instructions; // Array of all instructions
  };

#endif // SERVEX_LOCAL_H
