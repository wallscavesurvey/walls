/*****************************************************************************/
// File: kdu_client.h [scope = APPS/COMPRESSED_IO]
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
  Describes a complete caching compressed data source for use in the client
of an interactive client-server application.
******************************************************************************/

#ifndef KDU_CLIENT_H
#define KDU_CLIENT_H

#include "kdu_cache.h"
#include "kdu_client_window.h"

// Defined here:
class kdu_client_translator;
class kdu_client_notifier;
class kdu_client;

// Defined elsewhere
class kdcs_timer;
class kdcs_channel_monitor;
class kdcs_message_block;
struct kdc_request;
class kdc_model_manager;
class kdc_primary;
class kdc_cid;
struct kdc_request_queue;

// Status flags returned by `kdu_client::get_window_in_progress'
#define KDU_CLIENT_WINDOW_IS_MOST_RECENT      ((int) 1)
#define KDU_CLIENT_WINDOW_RESPONSE_TERMINATED ((int) 2)
#define KDU_CLIENT_WINDOW_IS_COMPLETE         ((int) 4)

/*****************************************************************************/
/*                              kdu_client_mode                              */
/*****************************************************************************/

enum kdu_client_mode {
  KDU_CLIENT_MODE_AUTO=1,
  KDU_CLIENT_MODE_INTERACTIVE=2,
  KDU_CLIENT_MODE_NON_INTERACTIVE=3
};

/*****************************************************************************/
/*                           kdu_client_translator                           */
/*****************************************************************************/

class kdu_client_translator {
  /* [BIND: reference]
     [SYNOPSIS]
       Base class for objects which can be supplied to
       `kdu_client::install_context_translator' for use in translating
       codestream contexts into their constituent codestreams, and
       translating the view window into one which is appropriate for each
       of the codestreams derived from a codestream context.  The interface
       functions provided by this object are designed to match the
       translating functions provided by the `kdu_serve_target' object, from
       which file-format-specific server components are derived.
       [//]
       This class was first defined in Kakadu v4.2 to address the need for
       translating JPX compositing layers and compositing instructions so
       that cache model statements could be delivered correctly to a server
       (for stateless services and for efficient re-use of data cached from
       a previous session).  The translator for JPX files is embodied by the
       `kdu_clientx' object, which is, in some sense, the client version of
       `kdu_servex'.  In the future, however, translators for other file
       formats such as MJ2 and JPM could be implemented on top of this
       interface and then used as-is, without modifying the `kdu_client'
       implementation itself.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_client_translator();
    virtual ~kdu_client_translator() { return; }
    virtual void init(kdu_cache *main_cache)
      { close(); aux_cache.attach_to(main_cache); }
      /* [SYNOPSIS]
           This function first calls `close' and then attaches the
           internal auxiliary `kdu_cache' object to the `main_cache' so
           that meta data-bins can be asynchronously read by the
           implementation.  You may need to override this function to
           perform additional initialization steps.
      */
    virtual void close() { aux_cache.close(); }
      /* [SYNOPSIS]
           Destroys all internal resources, detaching from the main
           cache installed by `init'.  This function will be called from
           `kdu_client' when the client-server connection becomes
           disconnected.
      */
    virtual bool update() { return false; }
      /* [SYNOPSIS]
           This function is called from inside `kdu_client' when the
           member functions below are about to be used to translate
           codestream contexts.  It parses as much of the file format
           as possible, returning true if any new information was
           parsed.  If sufficient information has already been parsed
           to answer all relevant requests via the member functions below,
           the function returns false immediately, without doing any work.
      */
    virtual int get_num_context_members(int context_type, int context_idx,
                                        int remapping_ids[])
      { return 0; }
      /* [SYNOPSIS]
           Designed to mirror `kdu_serve_target::get_num_context_members'.
           [//]
           This is one of four functions which work together to translate
           codestream contexts into codestreams, and view windows expressed
           relative to a codestream context into view windows expressed
           relative to the individual codestreams associated with those
           contexts.  A codestream context might be a JPX compositing layer
           or an MJ2 video track, for instance.  The context is generally
           a higher level imagery object which may be represented by one
           or more codestreams.  The context is identified by the
           `context_type' and `context_idx' arguments, but may also be
           modified by the `remapping_ids' array, as explained below.
         [RETURNS]
           The number of codestreams which belong to the context.  If the
           object is unable to translate this context, it should return 0.
           This situation may change in the future if more data becomes
           available in the cache, allowing the context to be translated.
           On the other hand, the context might simply not exist.  The
           return value does not distinguish between these two types of
           failure.
         [ARG: context_type]
           Identifies the type of context.  Currently defined context type
           identifiers (in "kdu_client_window.h") are
           `KDU_JPIP_CONTEXT_JPXL' (for JPX compositing layer contexts) and
           `KDU_JPIP_CONTEXT_MJ2T' (for MJ2 video tracks).  For more
           information on these, see `kdu_sampled_range::context_type'.
         [ARG: context_idx]
           Identifies the specific index of the JPX compositing layer,
           MJ2 video track or other context.  Note that JPX compositing
           layers are numbered from 0, while MJ2 video track numbers start
           from 1.
         [ARG: remapping_ids]
           Array containing two integers, whose values may alter the
           context membership and/or the way in which view windows are to
           be mapped onto each member codestream.  For more information on
           the interpretation of remapping id values, see the comments
           appearing with `kdu_sampled_range::remapping_ids' and
           `kdu_sampled_range::context_type'.
           [//]
           If the supplied `remapping_ids' values cannot be processed as-is,
           they may be modified by this function, but all of the other
           functions which accept a `remapping_ids' argument (i.e.,
           `get_context_codestream', `get_context_components' and
           `perform_context_remapping') must be prepared to operate
           correctly with the modified remapping values.
      */
    virtual int get_context_codestream(int context_type,
                                       int context_idx, int remapping_ids[],
                                       int member_idx)
      { return -1; }
      /* [SYNOPSIS]
           Designed to mirror `kdu_serve_target::get_context_codestream'.
           [//]
           See `get_num_context_members' for a general introduction.  The
           `context_type', `context_idx' and `remapping_ids' arguments have
           the same interpretation as in that function.  The `member_idx'
           argument is used to enumerate the individual codestreams which
           belong to the context.  It runs from 0 to the value returned by
           `get_num_context_members' less 1.
         [RETURNS]
           The index of the codestream associated with the indicated member
           of the codestream context.  If the object is unable to translate
           this context, it should return -1.  As for
           `get_num_context_members', this situation may change in the
           future, if more data is added to the cache.
      */
    virtual const int *get_context_components(int context_type,
                                              int context_idx,
                                              int remapping_ids[],
                                              int member_idx,
                                              int &num_components)
      { return NULL; }
      /* [SYNOPSIS]
           Designed to mirror `kdu_serve_target::get_context_components'.
           [//]
           See `get_num_context_members' for a general introduction.  The
           `context_type', `context_idx', `remapping_ids' and `member_idx'
           arguments have the same interpretation as in
           `get_context_codestream', except that this function returns a
           list of all image components from the codestream which are used
           by this context.
         [RETURNS]
           An array with one entry for each image component used by the
           indicated codestream context, from the specific codestream
           associated with the indicated context member.  This is the
           same codestream whose index is returned `get_context_codestream'.
           Each element in the array holds the index (starting from 0) of
           the image component.  If the object is unable to translate
           the indicated context, it should return NULL.  As for
           the other functions, however, a NULL return value may change in
           the future, if more data is added to the cache.
         [ARG: num_components]
           Used to return the number of elements in the returned array.
      */
    virtual bool perform_context_remapping(int context_type,
                                           int context_idx,
                                           int remapping_ids[],
                                           int member_idx,
                                           kdu_coords &resolution,
                                           kdu_dims &region)
      { return false; }
      /* [SYNOPSIS]
           Designed to mirror `kdu_serve_target::perform_context_remapping'.
           [//]
           See `get_num_context_members' for a general introduction.  The
           `context_type', `context_idx', `remapping_ids' and `member_idx'
           arguments have the same interpretation as in
           `get_context_codestream', except that this function serves to
           translate view window coordinates.
         [RETURNS]
           False if translation of the view window coordinates is not
           possible for any reason.  As for the other functions, a
           false return value may become true in the future, if more
           data is added to the cache.
         [ARG: resolution]
           On entry, this argument represents the full size of the image, as
           it is to be rendered within the indicated codestream context.  Upon
           return, the values of this argument will be converted to represent
           the full size of the codestream, required to render the context at
           the full size indicated on entry.
         [ARG: region]
           On entry, this argument represents the location and dimensions of
           the view window, expressed relative to the full size image
           at the resolution given by `resolution'.  Upon return, the
           values of this argument will be converted to represent the
           location and dimensions of the view window within the codestream.
      */
    protected: // Data members, shared by all file-format implementations
      kdu_cache aux_cache; // Attached to the main client cache; allows
                           // asynchronous reading of meta data-bins.
  };

/*****************************************************************************/
/*                            kdu_client_notifier                            */
/*****************************************************************************/

class kdu_client_notifier {
  /* [BIND: reference]
     [SYNOPSIS]
       Base class, derived by an application and delivered to
       `kdu_client' for the purpose of receiving notification messages when
       information is transferred from the server.
  */
  public:
    kdu_client_notifier() { return; }
    virtual ~kdu_client_notifier() { return; }
    virtual void notify() { return; }
    /* [BIND: callback]
       [SYNOPSIS]
         Called whenever an event occurs, such as a change in the status
         message (see `kdu_client::get_status') or a change in the contents
         of the cache associated with the `kdu_client' object.
    */
  };

/*****************************************************************************/
/*                                kdu_client                                 */
/*****************************************************************************/

class kdu_client : public kdu_cache {
  /* [BIND: reference]
     [SYNOPSIS]
     Implements a full JPIP network client, building on the services offered
     by `kdu_cache'.
     [//]
     BEFORE USING THIS OBJECT IN YOUR APPLICATION, READ THE FOLLOWING ADVICE:
     [>>] Because errors may be generated through `kdu_error' (and perhaps
          warnings through `kdu_warning') both in API calls from your
          application and inside the network management thread created by
          this object, you must at least use a thread-safe message handler
          (e.g., derived from `kdu_thread_safe_message') when you configure
          the error and warning handlers (see `kdu_customize_errors' and
          `kdu_customize_warnings').   Your message handler is expected to
          throw an exception of type `kdu_exception' inside
          `kdu_message::flush' if the `end_of_message' argument is true --
          much better than letting the entire process exit, which is the
          default behaviour if you don't throw an exception.  Message
          handlers may be implemented in a non-native language such as
          Java or C#, in which case exceptions which they throw in their
          override of `kdu_message::flush' will ultimately be converted
          into `kdu_exception'-valued C++ exceptions.
     [>>] Perhaps less obvious is the fact that your error/warning message
          handler must not wait for activity on the main application thread.
          Otherwise, your application can become deadlocked inside one of
          the API calls offered by this function if the network management
          thread is simultaneously generating an error.  Blocking on the
          main application thread can happen in subtle ways in GUI
          applications, since many GUI environments rely upon the main
          thread to dispatch all windowing messages; indeed some are not
          even thread safe.
     [>>] To avoid any potential problems with multi-threaded errors and
          warnings in interactive environments, you are strongly advised to
          implement your error/warning message handlers on top of
          `kdu_message_queue', which itself derives from
          `kdu_thread_safe_message'.  This object builds a queue of messages,
          each of which is bracketed by the usual `kdu_message::start_message'
          and `kdu_message::flush(true)' calls, which are automatically
          inserted by the `kdu_error' and `kdu_warning' services.  Your
          derived object can override `kdu_message::flush' to send a
          message to the window management thread requesting it to iteratively
          invoke `kdu_message_queue::pop_message' and display the relevant
          message text in appropriate interactive windows.  Examples of this
          for Windows and MAC-OSX platforms are provided in the "kdu_show"
          and "kdu_macshos" demo applications.  If you don't do something like
          this, blocking calls to `kdu_client::disconnect', for example, run
          a significant risk of deadlocking an interactive application if
          the underlying platform does not support true multi-threaded
          windowing.
     [//]
     This single network client can be used for everything from a single
     shot request (populate the cache based on a single window of interest)
     to multiple persistent request queues.  The notion of a request queue
     deserves some explanation here so that you can make sense of the
     interface documentation:
     [>>] JPIP offers both stateless servicing of requests and stateful
          request sessions.  You can choose which flavour you prefer, but
          stateful serving of requests (in which the server keeps track of
          the client's cache state) is the most efficient for ongoing
          interactive communications.
     [>>] A stateful JPIP session is characterized by one or more JPIP
          channels, each of which has a unique identifier; any of these
          channel identifiers effectively identifies the session.  You do
          not need to worry about the mechanics of JPIP channels or channel
          identifiers when using the `kdu_client' API, but it is worth
          knowing that JPIP channels are associated with stateful sessions and
          there can be multiple JPIP channels.
     [>>] Each JPIP channel effectively manages its own request queue.  Within
          the channel, new requests may generally pre-empt earlier requests,
          so that the earlier request's response may be truncated or even
          empty.  This is generally a good idea, since interactive users
          may change their window of interest frequently and desire a high
          level of responsiveness to their new interests.  Pre-emptive
          behaviour like this can only be managed by the JPIP server itself
          in the context of a stateful session.  For stateless communications,
          maintaining responsiveness is the client's responsibility.
     [>>] The present object provides the application with an interface
          which offers any number of real or virtual JPIP channels,
          regardless of whether the server supports stateful sessions and
          regardless of the number of JPIP channels the server is capable
          of supporting.  The abstraction used to deliver this behaviour
          is that of a "request queue".
     [>>] Each request queue is associated with some underlying communication
          channel.  Ideally, each request queue corresponds to a separate
          JPIP channel, but servers may not offer sessions or multiple JPIP
          channels.  If the server offers only one stateful session channel,
          for example, the client interleaves requests from the different
          request queues onto that channel -- of course, this will cause
          some degree of pre-emption and context swapping by the server,
          with efficiency implications, but it is better than creating
          separate clients with distinct caches.  If the connection is
          stateless, the client explicitly limits the length of each response
          based upon estimates of the underlying channel conditions, so that
          responsiveness will not be lost.  For stateless communications,
          the client always interleaves requests from multiple request
          queues onto the same underlying communication channel, so as to
          avoid consuming excessive amounts of server processing resources
          (stateless requests are more expensive for the server to handle).
          Regardless of whether communication is stateful or stateless, the
          client manages its own version of request pre-emption.  In
          particular, requests which arrive on a request queue can pre-empt
          earlier requests which have not yet been delivered to the server.
     [//]
     The following is a brief summary of the way in which
     this object is expected to be used:
     [>>] The client application uses the `connect' function to
          initiate communication with the server.  This starts a new thread
          of execution internally to manage the communication process.
          The cache may be used immediately, if desired, although its
          contents remain empty until the server connection has been
          established.  The `connect' function always creates a single
          initial request queue, which may be used to post requests even
          before communication has actually been established.
     [>>] The client application determines whether or not the remote
          object is a JP2-family file, by invoking
          `kdu_cache::get_databin_length' on meta data-bin 0, using its
          `is_complete' member to determine whether a returned length
          of 0 means that meta data-bin 0 is empty, or simply that nothing
          is yet known about the data-bin, based on communication with the
          server so far (the server might not even have been connected yet).
     [>>] If the `kdu_cache::get_databin_length' function returns a length of
          0 for meta data-bin 0, and the data-bin is complete, the image
          corresponds to a single raw JPEG2000 code-stream and the client
          application must wait until the main header data-bin is complete,
          calling `kdu_cache::get_databin_length' again (this time querying
          the state of the code-stream main header data-bin) to determine
          when this has occurred.  At that time, the present object may be
          passed directly to `kdu_codestream::create'.
     [>>] If the `kdu_cache::get_databin_length' function identifies meta
          data-bin 0 as non-empty, the image source is a JP2-family file.
          The client application uses the present object to open a
          `jp2_family_src' object, which is then used to open a `jp2_source'
          object (or any other suitable file format parser derived from
          `jp2_input_box').  The application then calls
          `jp2_source::read_header' until it returns true, meaning that the
          cache has sufficient information to read both the JP2 file header
          and the main header of the embedded JPEG2000 code-stream.  At
          that point, the `jp2_source' object is passed to
          `kdu_codestream::create'.
     [>>] At any point, the client application may use the `disconnect'
          function to disconnect from the server, while continuing to use
          the cache, or the `close' function to both disconnect from the
          server (if still connected) and discard the contents of the cache.
          The `disconnect' function can also be used to delete a single
          request queue (and any communication resources exclusively
          associated with that request queue).
     [>>] Additional request queues may be added with the `add_queue'
          function.  In fact, this function may be invoked any time after
          the call to `connect', even if communication has not yet been
          established with the server.  In a typical application, multiple
          request queues might be associated with multiple open viewing
          windows, so that an interactive user can select distinct windows
          of interest (region of interest, zoom factor of interest,
          components of interest, codestreams of interest, etc.) within
          each viewing window.
     [//]
     The `kdu_client' object provides a number of status functions for
     monitoring the state of the network connection.  These are
     [>>] `is_active' -- returns true from the point at which `connect'
          is called, remaining true until `close' is called, even if all
          connections with the server have been dropped or none were ever
          completed.
     [>>] `is_alive' -- allows you to check whether the communication
          channel used by a single request queue (or by any of the request
          queues) is alive.  Request queues whose underlying
          communication channel is still in the process
          of being established are considered to be alive.
     [>>] `is_idle' -- allows you to determine whether or not a request
          queue (or all request queues) are idle, in the sense that the
          server has fully responded to existing requests on the queue,
          but the queue is alive.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_client();
    KDU_AUX_EXPORT virtual ~kdu_client();
      /* [SYNOPSIS]
           Invokes `close' before destroying the object.
      */
    KDU_AUX_EXPORT static const char *
      check_compatible_url(const char *url,
                           bool resource_component_must_exist,
                           const char **port_start=NULL,
                           const char **resource_start=NULL,
                           const char **query_start=NULL);
      /* [SYNOPSIS]
           This static function provides a useful service to applications
           prior to calling `connect'.  It identifies whether or not the
           supplied `url' string might represent a JPIP URL and returns
           information concerning its major components.  To be a compatible
           URL, the `url' string must be of the general form:
           [>>] "<prot>://<HOST>[:<port>]/<resource>[?<query>]", where
           [>>] <HOST> is a <hostname>, <IPv4 literal> or '['<IP literal>']'
           [>>] Notice that the <HOST> and <port> components follow the
                conventions outlined in RFC 3986.  Specifically, the optional
                <port> suffix is separated by a colon and IPv6 literal
                addresses should be enclosed in square brackets.  IPv4 literal
                addresses may appear in bare form or enclosed within square
                brackets.
           [>>] The <prot> prefix must be one of "jpip" or "http" -- case
                insensitive.  The port number, if present, should be a decimal
                integer in the range 0 to (2^16)-1, although the port suffix
                is not specifically examined by this function.
           [>>] It is expected that <hostname>, <resource> and <query>
                components have been hex-hex encoded, if necessary, so that
                they do not contain non-URI-legal characters or any
                characters which could cause ambiguity in the interpretation
                of the `url'.  In particular, while "&" and "?" are URI-legal,
                they should be hex-hex encoded where found within the hostname
                or resource components of the `url'.
           [//]
           If the string appears to have the above form, the function returns
           a pointer to the start of the <HOST> component.  Additionally:
           if `port_start' is non-NULL, it is used to return the start of
           any <port> component (NULL if there is none); if `resource_start'
           is non-NULL, it is used to return the start of the <resource>
           component; and if `query_start' is non-NULL it is used to return
           the start of any <query> component (NULL if there is none).
         [RETURNS]
           NULL if the `url' string does not appear to be a compatible JPIP
           URL, else the start of the <hostname> component of the URL within
           the `url' sltring.
         [ARG: resource_component_must_exist]
           If false, the function does not actually require the "<resource>"
           component to be present within the `url' string.  In this case,
           it returns non-NULL so long as a compatible "<prot>://" prefix is
           detected; the `resource_start' argument may be used to return
           information about the presence or absence of the otherwise
           mandatory <resource> component.
         [ARG: port_start]
           If non-NULL, this argument is used to return a pointer to the start
           of any <port> sub-string within the `url' string; if there is none,
           *`port_start' is set to NULL.
         [ARG: resource_start]
           If non-NULL, this argument is used to return a pointer to the start
           of the <resource> sub-string within the `url' string.  If
           `resource_component_must_exist' is true, a <resource> sub-string
           must be present for `url' to be a compatible URL.  However, if
           `resource_component_must_exist' is false and the "/" separator
           is not found within the text which follows the "<prot>://" prefix,
           the function will set *`resource_start' to NULL.
         [ARG: query_start]
           If non-NULL, this argument is used to return a pointer to the start
           of any <query> sub-string within the `url' string; if there is none,
           *`query_start' is set to NULL.
      */
    KDU_AUX_EXPORT void
      install_context_translator(kdu_client_translator *translator);
      /* [SYNOPSIS]
           You may call this function at any time, to install a translator
           for context requests.  Translators are required only if you
           wish to issue requests which involve a codestream context
           (see `kdu_window::contexts'), and the client-server communications
           are stateless, or information from a previous cached session is
           to be reused.  In these cases, the context translator serves to
           help the client determine what codestreams are being (implicitly)
           requested, and what each of their effective request windows are.
           This information, in turn, allows the client to inform the server
           of relevant data-bins which it already has in its cache, either
           fully or in part.  If you do not install a translator,
           communications involving codestream contexts may become
           unnecessarily inefficient under the circumstances described above.
           [//]
           Conceivably, a different translator might be created to handle
           each different type of image file format which can be accessed
           using a JPIP client.  For the moment, a single translator serves
           the needs of all JP2/JPX files, while a separate translator may
           be required for MJ2 (motion JPEG2000) files, and another one might
           be required for JPM (compound document) files.
           [//]
           You can install the translator prior to or after the call to
           `connect'.  If you have several possible translators available,
           you might want to wait until after `connect' has been called
           and sufficient information has been received to determine the
           file format, before installing a translator.
           [//]
           You may replace an existing translator, which has already been,
           installed, but you may only do this when the client is not
           active, as identified by the `is_active' function.  To
           remove an existing translator, without replacing it, supply
           NULL for the `translator' argument.
      */
   void install_notifier(kdu_client_notifier *notifier)
      { acquire_lock(); this->notifier = notifier; release_lock(); }
      /* [SYNOPSIS]
           Provides a generic mechanism for the network management thread
           to notify the application when the connection state changes or
           when new data is placed in the cache.  Specifically, the
           application provides an appropriate object, derived from the
           abstract base class, `kdu_client_notifier'.  The object's virtual
           `notify' function is called from within the network management
           thread whenever any of the following conditions occur:
           [>>] Connection with the server is completed;
           [>>] The server has acknowledged a request for a new window into
                the image, possibly modifying the requested window to suit
                its needs (call `get_window_in_progress' to learn about any
                changes to the window which is actually being served);
           [>>] One or more new messages from the server have been received
                and used to augment the cache; or
           [>>] The server has disconnected -- in this case,
                `notifier->notify' is called immediately before the network
                management thread is terminated, but `is_alive' is guaranteed
                to be false from immediately before the point at which the
                notification was raised, so as to avoid possible race
                conditions in the client application.
           [//]
           A typical notifier would post a message on an interactive client's
           message queue so as to wake the application up if necessary.  A
           more intelligent notifier may choose to wake the application up in
           this way only if a substantial change has occurred in the cache --
           something which may be determined with the aid of the
           `kdu_cache::get_transferred_bytes' function.
           [//]
           The following functions should never be called from within the
           `notifier->notify' call, since that call is generated from within
           the network management thread: `connect', `add_queue', `close',
           `disconnect' and `post_window'.
           [//]
           We note that the notifier remains installed after its `notify'
           function has been called, so there is no need to re-install the
           notifier.  If you wish to remove the notifier, the
           `install_notifier' function may be called with a NULL argument.
           [//]
           You may install the notifier before or after the first call to
           `connect'; however, the call to `connect' starts a network
           management thread, and the notifier will be removed as the final
           step in the termination of this thread - thus, you cannot
           expect the notifier to remain installed after an unsuccessful call
           to `connect'.  The notifier will also be removed by a call to
           `close'.
           [//]
           In the future, you can expect a more powerful notification
           mechanism to be offered by Kakadu, providing more detailed
           information about the relevant events -- this will exist side
           by side with the current method, for backward compatibility.
      */
    KDU_AUX_EXPORT virtual int
      connect(const char *server, const char *proxy,
              const char *request, const char *channel_transport,
              const char *cache_dir, kdu_client_mode mode=KDU_CLIENT_MODE_AUTO,
              const char *compatible_url=NULL);
      /* [SYNOPSIS]
           Creates a new thread of execution to manage network communications
           and initiate a connection with the appropriate server.  The
           function returns immediately, without blocking on the success of
           the connection.  The application may then monitor the value
           returned by `is_active' and `is_alive' to determine when the
           connection is actually established, if at all.  The application
           may also monitor transfer statistics using
           `kdu_cache::get_transferred_bytes' and it may receive notification
           of network activity by supplying a `kdu_client_notifier'-derived
           object to the `install_notifier' function.
           [//]
           The function creates an initial request queue to which requests can
           be posted by `post_window' (in interactive mode -- see `mode'),
           returning the queue identifier.  In interactive mode, additional
           request queues can be created with `add_queue', even before the
           request completes.
           [//]
           The network management thread is terminated once there are no
           more alive request queues -- request queues are considered
           to be alive if the associated communication channel is alive or
           in the process of being established.  One way to kill the
           network management thread is to invoke `close', since this
           performs a hard close on all the open communication channels.
           A more graceful approach is to invoke the `disconnect' function
           on all request queues (one by one, or all at once).  Request
           queues may also disconnect asynchronously if their underlying
           communication channels are dropped by the server or through
           some communication error.
           [//]
           You may call this function again, to establish a new connection.
           However, you should note that `close' will be called automatically
           for you if you have not already invoked that function.  If
           a previous connection was gracefully closed down through one or
           more successful calls to `disconnect', the object may be able to
           re-use an established TCP channel in a subsequent `connect'
           attempt.  For more on this, consult the description of the
           `disconnect' function.
           [//]
           The present function may itself issue a terminal error message
           through `kdu_error' if there is something wrong with any of the
           supplied arguments.  For this reason, you should generally provide
           try/catch protection when calling this function if you don't want
           your application to die in the event of a bad call.
         [RETURNS]
           A request queue identifier, which can be used with calls to
           `post_window', `get_window_in_progress', `is_idle' and quite
           a few other functions.  In practice, when you first connect
           to the server, the initial request queue is assigned an identifier
           of 0, which is always the value returned by this function.
           [//]
           In summary, this function always returns 0 (or else generates
           an error), but you should use the return value in calls which
           require a request queue identifier without assuming that the
           identifier is 0.
         [ARG: server]
           Holds the host name or IP literal address of the server to be
           contacted, together with optional port information.  This string
           must follow the same conventions as those outlined for the <HOST>
           and <port> components of the string supplied to
           `check_compatible_url'.  Specifically, an optional decimal numeric
           <port> suffix may appear at the end of the string, separated by a
           colon, while the remainder of the string is either a host name
           string (address to be resolved), a dotted IPv4 literal address, or
           an IPv4 or IPv6 literal address enclosed in square brackets.
           Host name strings are hex-hex decoded prior to resolution.
           [//]
           The default HTTP port number of 80 is assumed if none is provided.
           Communication with the `server' machine proceeds over HTTP, and
           may involve intermediate proxies, as described in connection with
           the `proxy' argument.
           [//]
           This argument may be NULL only if the `compatible_url' argument
           is non-NULL, in which case `compatible_url' must pass the test
           associated with the `check_compatible_url' function, and the
           `server' string is obtained from the <HOST> and <port> components
           of the `compatible_url'.
         [ARG: proxy]
           Same syntax as `server', but gives the host (and optionally the
           port number) of the machine with which the initial TCP connection
           should be established.  This may either be the server itself, or
           an HTTP proxy server.  May be NULL, or point to an empty string,
           if no proxy is to be used.
           [//]
           As for `server', the function anticipates potential hex-hex
           encoding of any host name component of the `proxy' string and
           performs hex-hex decoding prior to any host resolution attempts.
         [ARG: request]
           This argument may be NULL only if `compatible_url' is non-NULL, in
           which case the `compatible_url' string must pass the test
           associated with `check_compatible_url' and contain a valid
           <resource> sub-string; the entire contents of `compatible_url',
           commencing from the <resource> sub-string, are interpreted as the
           `request' in that case.  In either case, the `request' string may
           not be empty.
           [//]
           As explained in connection with `check_compatible_url', the
           `request' string may consist of both a <resource> component and a
           <query> component (the latter is optional) and hex-hex encoding
           of each component is expected.  Hex-hex decoding of any query
           component is performed separately on each request field, so that
           hex-hex encoded '&' characters within the contents of any query
           field will not be mistaken for query field separators.
         [ARG: channel_transport]
           If NULL or a pointer to an empty string or the string "none", no
           attempt will be made to establish a JPIP channel.  In this case,
           no attempt will be made to establish a stateful communication
           session; each request is delivered to the server (possibly through
           the proxy) in a self-contained manner, with all relevant cache
           contents identified using appropriate JPIP-defined header lines.
           When used in the interactive mode (see `mode'), this may
           involve somewhat lengthy requests, and may cause the server to go
           to quite a bit of additional work, re-creating the context for each
           and every request.
           [//]
           If `channel_transport' holds the string "http" and the mode is
           interactive (see `mode'), the client's first request asks
           for a channel with HTTP as the transport.  If the server refuses
           to grant a channel, communication will continue as if the
           `channel_transport' argument had been NULL.
           [//]
           If `channel_transport' holds the string "http-tcp", the behaviour
           is the same as if it were "http", except that a second TCP
           channel is established for the return data.  This transport
           variant is somewhat more efficient for both client and server,
           but requires an additional TCP channel and cannot be used from
           within organizations which mandate that all external communication
           proceed through HTTP proxies.  If the server does not support
           the "http-tcp" transport, it may fall back to an HTTP transported
           channel.  This is because the client's request to the server
           includes a request field of the form "cnew=http-tcp,http", giving
           the server both options.
           [//]
           It is worth noting that wherever a channel is requested, the
           server may respond with the address of a different host to be
           used in all future requests; redirection of this form is
           handled automatically by the internal machinery.
         [ARG: cache_dir]
           If non-NULL, this argument provides the path name for a directory
           in which cached data may be saved at the end of an interactive
           communication session.  The directory is also searched at the
           beginning of an interactive session, to see if information is
           already available for the image in question.  If the argument
           is NULL, or points to an empty string, the cache contents will
           not be saved and no previously cached information may be re-used
           here.  Files written to or read from the cache directory have the
           ".kjc" suffix, which stands for (Kakadu JPIP Cache).  These
           files commence with some details which may be used to re-establish
           connection with the server, if necessary (not currently
           implemented) followed by the cached data itself, stored as a
           concatenated list of data-bins.  The format of these files is
           private to the current implementation and subject to change in
           subsequent releases of the Kakadu software, although the files are
           at least guaranteed to have an initial header which can be used
           for version-validation purposes.
         [ARG: mode]
           Determines whether the client is to operate in interactive or
           non-interactive modes.  This argument may take on one of three
           values, as follows:
           [>>] `KDU_CLIENT_MODE_INTERACTIVE' -- in this mode, the
                application may post new requests to `post_window'.  New
                request queues may also be created in interactive mode using
                `add_queue'.
           [>>] `KDU_CLIENT_MODE_NON_INTERACTIVE' -- in this mode, all calls
                to `post_window' are ignored and the `request' string
                (or `compatible_url' string) is expected to express the
                application's interests completely.  In non-interactive mode,
                the client issues a single request to the server and collects
                the response, closing the channel upon completion.  If the
                `cache_dir' argument is non-NULL and there is at least one
                cache file which appears to be compatible with the request,
                the client executes exactly two requests: one to obtain the
                target-id from the server, so as to determine the compatibility
                of any cached contents; and one to request the information of
                interest.
           [>>] `KDU_CLIENT_MODE_AUTO' -- in this case, the `connect' function
                automatically decides whether to use the interactive or
                non-interactive mode, based on the form of the request, as
                found in the `request' or `compatible_url' arguments.
                If the request string contains a query component (i.e., if
                it is of the form <resource>?<query>, where "?" is the
                query separator), the <query> string may contain multiple
                fields, each of the form <name>=<value>, separated by the
                usual "&" character.  If a <query> string contains anything
                other than the "target" or "subtarget" fields, the
                non-interactive mode will be selected; otherwise, the
                interacive mode is selected.
         [ARG: compatible_url]
           This optional argument allows you to avoid explicitly extracting
           the `server' and `request' sub-strings from a compatible JPIP
           URL string, letting the present function do that for you.  In this
           case, you may set either or both of the `server' and `request'
           arguments to NULL.  However, you do need to first check that
           the `check_compatible_url' function returns true when supplied with
           the `compatible_url' string and the `resource_component_must_exist'
           argument to that function is true.  If the `request' argument
           is non-NULL, the <resource> component of `compatible_url' will not
           be used.  Similarly, if the `server' argument is non-NULL, the
           <HOST> and <port> components of `compatible_url' will not be used.
           If both `request' and `server' are non-NULL, the `compatible_url'
           string will not be used at all, but may still be supplied if you
           like.
      */
    bool is_interactive() { return (!non_interactive) && is_alive(-1); }
      /* [SYNOPSIS]
           Returns true if the client is operating in the interactive mode
           and at least one request queue is still alive.  For more on
           interactive vs. non-interactive mode, see the `mode' argument
           to `connect'.
      */
    bool is_one_time_request() { return non_interactive; }
      /* [SYNOPSIS]
           Returns true if the client is operating in the non-interactive
           mode, as determined by the `mode' argument passed to `connect'.
           The return value from this function remains the same from the
           call to `connect' until `close' is called (potentially from
           inside a subsequent `connect' call).
      */
    bool connect_request_has_non_empty_window()
      { return initial_connection_window_non_empty; }
      /* [SYNOPSIS]
           Returns true if the request passed in the call to `connect'
           supplied a non-empty window of interest.  This means that a query
           string was supplied, which contained request fields that affected
           the initialization of a `kdu_window' object for the first request.
           This could mean that a specific region of interest was requested,
           specific codestreams or codestream contexts were requested,
           specific image components were requested, or specific metadata
           was requested, for example.  The application might use this
           information to decide whether to wait for the server to reply
           with enough data to open the relevant imagery, or issue its own
           overriding requests for the imagery it thinks should be most
           relevant.
           [//]
           The value returned by this function remains constant from the
           call to `connect' until a subsequent call to `close' (potentially
           issued implicitly from another call to `connect').
      */
    KDU_AUX_EXPORT virtual const char *get_target_name();
      /* [SYNOPSIS]
           This function returns a pointer to an internally managed string,
           which will remain valid at least until `close' is called.  If
           the `connect' function has not been called since the last call to
           `close' (i.e., if `is_active' returns false), this function returns
           the constant string "<no target>".  Otherwise the returned string
           was formed from the parameters passed to `connect'.  If the
           `connect' request string contained a `target' query parameter,
           that string is used; otherwise, the <resource> component of the
           request string is used.
           [//]
           If the requested resource has a sub-target (i.e., a byte range
           is identified within the primary target), the string returned by
           this function includes the sub-target information immediately
           before any file extension.
           [//]
           The function also decodes any hex-hex encoding which may have
           been used to ensure URI-legal names in the original call to
           `connect'.
      */
    KDU_AUX_EXPORT virtual bool
      check_compatible_connection(const char *server,
                                  const char *request,
                                  kdu_client_mode mode=KDU_CLIENT_MODE_AUTO,
                                  const char *compatible_url=NULL);
      /* [SYNOPSIS]
           This function is commonly used before calling `add_queue' to
           determine whether the object is currently connected in a manner
           which is compatible with a new connection that might otherwise
           need to be established.  If so, the caller can add a queue to
           the current object, rather than creating a new `kdu_client' object
           and constructing a connection from scratch.
           [//]
           If the client has no alive connections, or is executing in
           non-interactive mode (i.e., if `is_interactive' returns false), the
           function can still return true so long as the request and `mode'
           are compatible; although in this case the caller will not be able
           to add request queues or post window requests via `post_window'.
           [//]
           Compatibility for requests which contain query fields other than
           `target' or `subtarget' deserves some additional explanation.
           Firstly, a request for interactive communication (as determined
           by `mode') can only be compatible with a client object which is
           already in the interactive mode and vice-versa, noting that the
           interactivity of the request may need to be determined from the
           presence or absence of query fields other than `target' or
           `subtarget' if `mode' is `KDU_CLIENT_MODE_AUTO'.  Secondly, if
           there are query fields other than `target' or `subtarget', the
           request is considered compatible only if the intended mode is
           non-interactive and the current object is also in the
           non-interactive mode, with exactly the same query.
         [RETURNS]
           True if `is_active' returns true (note that the connection does
           not need to still be alive though) and the most recent call to
           `connect' was supplied with `server', `request' and
           `compatible_url' arguments which are compatible with those
           supplied here.
         [ARG: server]
           Provides the host name/address and (optionally) port components of
           the connection, if non-NULL.  Otherwise, this information must be
           recovered from `compatible_url'.  In either case, the host
           information must be compatible with that recovered by the most
           recent call to `connect', either recovered from its `server'
           argument or its `compatible_url' argument, or else the function
           returns false.
         [ARG: request]
           Provides the request component of the connection (resource name +
           an optional query string component).  Otherwise, the request
           component is recovered from `compatible_url' (the entire suffix
           of the `compatible_url' string, commencing with the <resource>
           sub-string).  In either case, the request component must be
           compatible with that recovered by the most recent call to
           `connect', either from its `request' argument or its
           `compatible_url' argument, or else the function returns false.
         [ARG: mode]
           This is the `mode' argument that the caller would supply to
           another `kdu_client' object's `connect' function.  If `mode'
           is `KDU_CLIENT_MODE_AUTO', the function determines whether the
           caller is interested in interactive behaviour based on the
           presence of any query fields other than `target' and `subtarget',
           using the same procedure as `connect'.  Whether the intended
           client mode is interactive or non-interactive affects the
           compatibility.
      */
    KDU_AUX_EXPORT virtual int add_queue();
      /* [SYNOPSIS]
           In non-interactive mode or if there are no more alive connections
           to the server (i.e. if `is_interactive' returns false), this
           function fails to create a new queue, returning -1.  Otherwise, the
           function creates a new request queue, returning the identifier you
           can use to post requests, disconnect and otherwise utilize the new
           queue.  If the server supports multiple JPIP channels and
           communication is not stateless, request queues may wind up being
           associated with distinct JPIP channels; this is generally the most
           efficient approach.  Otherwise, the client machinery takes care of
           interleaving requests onto an existing stateful or stateless
           communication channel.
         [RETURNS]
           A non-negative request queue identifier, or else -1, indicating
           that a new queue cannot be created.  This happens only
           under the following conditions:
           [>>] The `connect' function has not yet been called.
           [>>] All connections have been dropped, by a call to `close',
                `disconnect', or for some other reason (e.g., the server
                may have dropped the connection); this condition can be
                checked by calling `is_alive' with a -ve `queue_id' argument.
           [>>] The original call to `connect' configured the client for
                non-interactive communications -- i.e., `is_one_time_request'
                should be returning true and `is_interactive' should be
                returning false.
      */
    bool is_active() { return active_state; }
      /* [SYNOPSIS]
           Returns true from the point at which `connect' is called until the
           `close' function is called.  This means that the function may
           return true before a network connection with the server has
           actually been established, and it may return true after the
           network connection has been closed.
      */
    KDU_AUX_EXPORT virtual bool is_alive(int queue_id=-1);
      /* [SYNOPSIS]
           Returns true so long as a request queue (or all request queues)
           is alive.  A request queue is considered alive from the point
           at which it is created by `connect' or `add_queue' (even though
           it may take a while for actual communication to be established)
           until the point at which all associated communication is
           permanently stopped.  This may happen due to loss of the
           communication channel (e.g., the server may shut it down) or as
           a result of a call to `disconnect' or `close'.
           [//]
           Note that the `disconnect' function may supply a time limit for
           closure to complete, in which case the request queue may remain
           alive for a period of time after the call to `disconnect' returns.
           See that function for more information.
         [RETURNS]
           True if the the supplied `queue_id' identifies any request
           queue which is associated with a connected communication channel
           or one which is still being established.
         [ARG: queue_id]
           One of the identifiers returned by `connect' or `add_queue', or
           else a negative integer.  If negative, the function checks
           whether there are any communication channels which are alive.  If
           the argument is non-negative, but does not refer to any request
           queue identifier ever returned by `connect' or `add_queue', the
           function behaves as if supplied with the identifier of a
           request queue which has disconnected.
      */
    KDU_AUX_EXPORT virtual bool is_idle(int queue_id=-1);
      /* [SYNOPSIS]
           Returns true if the server has finished responding to all
           pending window requests on the indicated request queue and
           the `is_alive' function would return true if passed the same
           `queue_id'.
           [//]
           The state of this function reverts to false immediately after
           any call to `post_window' which returns true (i.e., any call which
           would cause a request to be sent to the server).  See that function
           for more information.
         [RETURNS]
           True if `is_alive' would return true with the same
           `queue_id' argument and if the identified request queue
           (or all request queues, if `queue_id' < 0) is idle.
         [ARG: queue_id]
           One of the identifiers returned by `connect' or `add_queue', or
           else a negative integer.  If a negative value is supplied, the
           function returns true if there is at least one communication
           channel still alive and ALL of the communication channels are
           currently idle (i.e., connected, with no outstanding data to be
           delivered for any request queue).
      */
    KDU_AUX_EXPORT virtual void disconnect(bool keep_transport_open=false,
                                           int timeout_milliseconds=2000,
                                           int queue_id=-1,
                                           bool wait_for_completion=true);
      /* [SYNOPSIS]
           Use this function to gracefully close request queues and the
           associated communication channels (once all request queues using
           the communication channel have disconnected).  Unlike `close',
           this function also leaves the object open for reading data
           from the cache.
           [//]
           This function may safely be called at any time.  If the `close'
           function has not been called since the last call to `connect',
           this function leaves `is_active' returning true, but will
           eventually cause `is_alive' to return false when invoked with the
           same `queue_id'.
           [//]
           After this function has been called, you will not be able to post
           any more window changes to the request queue via `post_window',
           even though the request queue may remain alive for some time
           (if `wait_for_completion' is false), in the sense that
           `is_alive' does not immediately return false.
           [//]
           The function actually causes a pre-emptive request to be posted as
           the last request in the queue, which involves an empty window of
           interest, to encourage the queue to become idle as soon as
           possible.  The function then notifies the thread management
           function that the request queue should be closed once it becomes
           idle, unless the `timeout_milliseconds' period expires first.
         [ARG: keep_transport_open]
           This argument does not necessarily have an immediate effect.  Its
           purpose is to try to keep a TCP channel open beyond the point at
           which all request channels have disconnected, so that the channel
           can be re-used in a later call to `connect'.  This can be useful
           in automated applications, which need to establish multiple
           connections in sequence with minimal overhead.
           [//]
           If this argument is true, the function puts the underlying primary
           communication channel for the identified request queue (or any
           request queue if `queue_id' is -1) in the "keep-alive" state,
           unless there is another primary channel already in the "keep-alive"
           state.  In practice, the primary channel will not be kept alive if
           it is closed by the server, or if an error occurs on some request
           channel which is using it.
           [//]
           If `keep_transport_open' is false, the function cancels the
           "keep-alive" status of any primary TCP channel, not just the one
           associated with an identified request queue, closing that channel
           if it is no longer in use.  The function may be used in this way
           to kill a channel that was being kept alive, even after all request
           queues have disconnected and, indeed, even after a call to `close'.
         [ARG: timeout_milliseconds]
           Specifies the maximum amount of time the network management thread
           should wait for the request queue to become idle before closing
           it down.  If this time limit expires, the forced closure of the
           request queue will also cause forced shutdown of the relevant
           underlying communication channels and any other request queues
           which may be using them.
           [//]
           If you have multiple request queues open and they happen to be
           sharing a common primary HTTP request channel (e.g., because
           the server was unwilling to assign multiple JPIP channels),
           you should be aware that forced termination of the request queue
           due to a timeout will generally cause the primary channel to
           be shut down.  This means that your other request queues will also
           be disconnected.  To avoid this, you are recommended to specify
           a timeout which is quite long, unless you are in the process of
           closing all request queues associated with the client.
           [//]
           You can always reduce the timeout by calling this function again.
         [ARG: wait_for_completion]
           If true, the function blocks the caller until the request queue
           ceases to be alive.  As explained above, this means that the
           request queue must become idle, or the specified timeout must
           expire.  If false, the function returns immediately, so that
           subsequent calls to `is_alive' may return true for some time.
           [//]
           If you need to specify a long timeout, for the reasons outlined
           above under `timeout_milliseconds', it is usually best not to
           wait.  Waiting usually makes more sense when closing all request
           queues associated with the client, in which case a short timeout
           should do no harm.
         [ARG: queue_id]
           One of the request queue identifiers returned by a previous call
           to `connect' or `add_queue', or else a negative integer, in which
           case all request queues will be disconnected with the same
           parameters.  If the indicated queue identifier was never issued
           by a call to `connect' or `add_queue' or was previously
           disconnected, the function does nothing except potentially
           remove the "keep-alive" state of a primary TCP channel, as
           discussed in the description of the `keep_transport_open'
           argument -- this may cause a previously saved TCP transport
           channel to be closed.
      */
    KDU_AUX_EXPORT virtual bool close();
      /* [SYNOPSIS]
           Effectively, this function disconnects all open communication
           channels immediately and discards the entire contents of the
           cache, after which all calls to `is_alive' and `is_active' will
           to return false.  It is safe to call this function at any time,
           regardless of whether or not there is any connection.
           [//]
           If `disconnect' has already been used to disconnect all request
           queues and their communication channels, and if that function has
           saved an underlying transport channel, the present function will
           not destroy it -- that will happen only if a subsequent call to
           `connect' cannot use it, if `disconnect' is called explicitly with
           `keep_transport_open'=false, or if the object is destroyed.
      */
    KDU_AUX_EXPORT virtual bool
      post_window(const kdu_window *window, int queue_id=0,
                  bool preemptive=true, const kdu_window_prefs *prefs=NULL);
      /* [SYNOPSIS]
           Requests that a message be delivered to the server identifying
           a change in the user's window of interest into the compressed
           image and/or a change in the user's service preferences.  The
           message may not be delivered immediately; it may not
           even be delivered at all, if the function is called again
           specifying a different access window for the same request queue,
           with `preemptive'=true, before the function has a chance to deliver
           a request for the current window to the server.
           [//]
           You should note that each request queue (as identified by the
           `queue_id' argument) behaves independently, in the sense that
           newly posted window requests can only preempt exising ones within
           the same queue.
           [//]
           It is important to note that the server may not be able to
           serve some windows in the exact form they are requested.  When
           this happens, the server's response will indicate modified
           attributes of the window which it is able to service.  The
           client may learn about the actual window for which data is being
           served by calling `get_window_in_progress'. The service preferences
           signalled by `prefs' may affect whether or not the server actually
           makes any modifications.  In particular, the `KDU_WINDOW_PREF_FULL'
           and `KDU_WINDOW_PREF_PROGRESSIVE' options may be used to manipulate
           the way in which the server treats large request windows.
           [//]
           This function may be called at any time after a request queue
           is instantiated by `connect' or `add_queue', without
           necessarily waiting for the connection to be established or for
           sufficient headers to arrive for a call to `open' to succeed.
           This can be useful, since it allows an initial window to be
           requested, while the initial transfer of mandatory headers is
           in progress, or even before it starts, thereby avoiding the
           latency associated with extra round-trip-times.
         [RETURNS]
           False if `queue_id' does not refer to a request queue
           which is currently alive, or if the call otherwise has no effect.
           The call may have no effect because the request queue is in
           the process of being disconnected (see `disconnect').  The
           call may also have no effect if the supplied `window' is a
           subset of a recent window which is known to have been
           completed delivered by the server and the request queue is idle
           or the posting is non-preemptive, or if the window is identical
           in every respect to the last one posted (unless that one was
           not preemptive and the current one is).
           [//]
           If the function returns true, the window is held in the queue
           until a suitable request message can be sent to the server.  This
           message will eventually be sent, unless a new (pre-emptive) call
           to `post_window' arrives in the mean time.
         [ARG: preemptive]
           If this argument is false, a request will be queued for the
           relevant window of interest.  It will not pre-empt the ongoing
           response to any previously posted window (even in the same queue).
           This means that previously posted windows of interest within this
           queue must be served in full before the current request is
           considered (although they can still be pre-empted by later
           calls to this function with `preemptive'=true).
           [//]
           If you call this function with `preemptive'=true (this is best for
           interactive applications), the new window will pre-empt any
           undelivered requests and potentially pre-empt requests which
           have been sent to the server but for which the server has not
           yet generated the complete response.  That is, requests may be
           pre-empted within the client's queue of undelivered requests, or
           within the server's queue of unprocessed requests pre-empted
           within the server's own processing queue).  This provides a useful
           way of discarding a queue of waiting non-preemptive window
           requests.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the queue identifier is
           invalid or the relevant queue is no longer alive, the
           function simply returns false.
         [ARG: prefs]
           If non-NULL, the function updates its internal understanding of
           the application's service preferences to reflect any changes
           supplied via the `prefs' object.  These changes will be passed to
           the server with the next actual request which is sent.  Even if
           requests are not delivered immediately, preference changes are
           accumulated internally between successive calls to this function,
           until a request is actually sent to the server.
           [//]
           It is important to note that the function does not replace existing
           preferences wholesale, based upon a `prefs' object supplied here.
           Instead, replacement proceeds within "related-pref-sets", based on
           whether or not any preference information is supplied within the
           "related-pref-set".  This is accomplished using the
           `kdu_window_prefs::update' function, whose documentation you might
           like to peruse to understand "related-pref-sets" better.
           [//]
           Kakadu's server currently supports quite a few of the preference
           options which are offered by JPIP.  Amongst these, probably the
           most useful are those which affect whether or not the requested
           window can be limited by the server (`KDU_WINDOW_PREF_PROGRESSIVE')
           for the most efficient quality progressive service, or whether
           the server should try to serve the whole thing even if spatially
           progressive delivery is required (`KDU_WINDOW_PREF_FULLWINDOW'),
           along with those which affect the order in which the relevant
           codestreams are delivered (`KDU_CODESEQ_PREF_FWD',
           `KDU_CODESEQ_PREF_BWD' or the default `KDU_CODESEQ_PREF_ILVD').
           [//]
           Kakadu's demonstration viewers ("kdu_macshow" and "kdu_winshow")
           use the preferences mentioned above to facilitate the most
           effective interactive browsing experience based on the way the
           user manipulates a focus window.
           [//]
           You are strongly advised NOT to supply any "required" preferences
           at the moment.  Although required preferences are supported in
           the implementation of Kakadu's client and server, if a server
           does not support some preference option that you identify as
           "required" (as opposed to just "preferred"), the server is
           obliged to respond with an error message.  Currently, the
           `kdu_client' implementation does not provide specific handling
           for preference-related error messages, so that they will be
           treated like any other server error, terminating ongoing
           communication.  In the future, special processing should be
           introduced to catch these errors and re-issue requests without
           the required preference while remembering which types of
           preferences are not supported by the server.
           [//]
           You should be aware that preferences are managed separately for
           each `queue_id'.
      */
    KDU_AUX_EXPORT virtual bool
      get_window_in_progress(kdu_window *window,int queue_id=0,
                             int *status_flags=NULL);
      /* [SYNOPSIS]
           This function may be used to learn about the window
           which the server is currently servicing within a given request
           queue, as identified by the `queue_id' argument.  To be specific,
           the current service window is interpreted as the window
           associated with the most recent JPIP request to which the
           server has sent a reply paragraph -- the actual response data may
           take considerably longer to arrive, but the reply paragraph informs
           the present object of the server's intent to respond to the
           window, along with any dimensional changes the server has made
           within its discretion.
           [//]
           If the request queue is currently idle, meaning that the server
           has finished serving all outstanding requests for the queue,
           the present function will continue to identify the most
           recently serviced window as the one which is in progress, since
           it is still the most recent window associated with a server reply.
           [//]
           If the indicated request queue is not alive (i.e., if `is_alive'
           would return false when invoked with the same `queue_id' value), or
           if the request queue has been disconnected using `disconnect', the
           function returns false after invoking `window->init' on any
           supplied `window'.
           [//]
           Finally, if no reply has yet been received by the server, there
           is considered to be no current service window and so this
           function also returns false after invoking `window->init' on any
           supplied `window'.
         [RETURNS]
           True if the current service window on the indicated request queue
           corresponds to the window which was most recently requested via a
           call to `post_window' which returned  true.  Also returns true
           if no successful call to `post_window' has yet been generated, but
           the client has received a reply paragraph to a request which it
           synthesized internally for this request queue (based on the
           parameters passed to `connect').  Otherwise, the
           function returns false, meaning that the server has not yet
           finished serving a previous window request on this queue, or a
           new request message has yet to be sent to the server.
         [ARG: window]
           This argument may be NULL, in which case the caller is interested
           only in the function's return value and/or `status_flags'.
           If non-NULL, the function modifies the various members of this
           object to indicate the current service window.
           [//]
           Note that the `window->resolution' member will be set to reflect
           the dimensions of the image resolution which is currently being
           served.  These can, and probably should, be used in posting new
           window requests at the same image resolution.
           [//]
           If there is no current service window, because no server reply
           has ever been received for requests delivered from this request
           queue, or because the queue is no longer alive, the `window'
           object will be set to its initialized state (i.e., the function
           automatically invokes `window->init'); amongst other things, this
           means that `window->num_components' will be zero.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the request queue identifier is
           invalid or the relevant queue is no longer alive, the
           function returns false after invoking `window->init' on any
           supplied `window'.
         [ARG: status_flags]
           You can use this argument to receive more detailed information
           about the status of the request associated with the window for
           which this function is returning information.  The defined flags
           are as follows:
           [>>] `KDU_CLIENT_WINDOW_IS_MOST_RECENT' -- this flag is set if
                and only if the function is returning true.
           [>>] `KDU_CLIENT_WINDOW_RESPONSE_TERMINATED' -- this flag is set
                if the server has finished responding to the request
                associated with this window and there are no internal
                duplicates of the request which have been delivered or are
                waiting to be delivered (internal duplicates are created
                to implement the client's flow control algorithm or to
                synthesize virtual JPIP channels by interleaving requests
                over a real JPIP channel).
           [>>] `KDU_CLIENT_WINDOW_IS_COMPLETE' -- this flag is set if the
                server has finished responding to the request associated
                with this window and the response data renders the client's
                knowledge of the source complete, with respect to the
                requested elements.  There is no need to post any further
                requests with the same parameters, although doing so will
                cause no harm.
      */
    KDU_AUX_EXPORT virtual const char *get_status(int queue_id=0);
      /* [SYNOPSIS]
           Returns a pointer to a null-terminated character string which
           best describes the current state of the identified request queue.
           [//]
           Even if the `queue_id' does not correspond to a valid
           channel, or `connect' has not even been called yet, the
           function always returns a pointer to some valid string, which is
           a constant resource (i.e., a resource which will not be deleted,
           even if the value returned by this function changes).
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the queue identifier is
           invalid or the relevant request queue is no longer alive, the
           function returns an appropriate string to indicate that this is
           the case.
      */
    KDU_AUX_EXPORT virtual kdu_long
      get_received_bytes(int queue_id=-1, double *non_idle_seconds=NULL,
                         double *seconds_since_first_active=NULL);
      /* [SYNOPSIS]
           Returns the total number of bytes which have been received from
           the server, including all headers and other overhead information,
           in response to requests posted to the indicated request queue.
           [//]
           This function differs from `kdu_cache::get_transferred_bytes'
           in three respects: firstly, it can be used to query information
           about transfer over a specific request queue; second, it returns
           only the amount of data transferred by the server, not including
           additional bytes which may have been retrieved from a local cache;
           finally, it includes all the transfered overhead.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions, or else a negative integer.  In the
           latter case, the function returns the total number of received
           bytes belonging to all request queues, since the `connect' function
           was last called.  If this argument is not negative and does not
           refer to an alive request queue (see `is_alive'), the function
           returns 0.  Thus, once all queues have been removed via the
           `disconnect' function, this function will return 0 for all
           non-negative `queue_id' values.
         [ARG: non_idle_seconds]
           If this argument is non-NULL, it is used to return the total number
           of seconds for which the indicated request queue has been
           non-idle, or (if `queue_id' < 0) the total number of seconds for
           which the aggregate of all request queues managed by the client
           over its lifetime has been non-idle.  A request queue is non-idle
           between the point at which it issues a request and the point at
           which it becomes idle, as identified by the `is_idle' member
           function.  The client is non-idle between the point at which any
           request queue issues a request and the point at which all request
           queues become idle.
         [ARG: seconds_since_first_active]
           If non-NULL, this argument is used to return the total number of
           seconds since the indicated request queue first became active, or
           (if `queue_id' < 0) the total number of seconds since any request
           queue in the client first became active.  A request queue becomes
           active when it first issues a request.  During the time spent
           resolving network addresses or attempting to connect TCP channels
           prior to issuing a request, the queue is not considered active.
      */
  private: // Startup function
    friend void _kd_start_client_thread_(void *);
    void thread_start(); // Called from the network thread startup procedure
  private: // Internal functions which are called only once per `connect' call
    void run(); // Called from `thread_start'
    void thread_cleanup(); // Called once `run' returns or if it never ran
  private: // Mutex locking/unlocking functions for use by management thread
    void acquire_management_lock()
      {
        if (management_lock_acquired) return;
        mutex.lock(); management_lock_acquired = true;
      }
    void release_management_lock()
      {
        if (!management_lock_acquired) return;
        management_lock_acquired = false; mutex.unlock();
      }
  private: // Helper functions used to manage primary channel life cycle
    kdc_primary *add_primary_channel(const char *host,
                                     kdu_uint16 default_port,
                                     bool host_is_proxy);
      /* Creates a new `kdc_primary' object, adding it to the list.  The new
         object's immediate server name (and optionally port number)
         are derived by parsing the `host' string.  The actual
         `kdc_tcp_channel' is not created at this point, nor is the IP
         address of the host resolved.  These happen when an attempt is made
         to use the channel. */
    void release_primary_channel(kdc_primary *primary);
      /* If there are any CID's using the primary channel when this function
         is called, they are automatically released as a first step. */
  private: // Helper functions used to manage CID (JPIP channel) life cycle
    kdc_cid *add_cid(kdc_primary *primary, const char *server_name,
                     const char *resource_name);
      /* Creates a new `kdc_cid' object, adding it to the list.  The new
         object will use the identified `primary' channel and its requests
         with use the identified `server_name' and `resource_name' strings. */
    void release_cid(kdc_cid *cid);
      /* Performs all relevant cleanup, including closure of channels.  If
         there are any request queues still using this CID, they are
         automatically released first. */
  private: // Helper functions used to manage request queue life cycle
    kdc_request_queue *add_request_queue(kdc_cid *cid);
      /* Creates a new request queue, adding it to the list.  The new queue
         will use the identified `cid' for its initial (and perhaps ongoing)
         communications.  The queue is set up with `just_started'=true, which
         will cause the first request to include `cnew' and any
         one-time-request components, as required.  The response to a `cnew'
         request may cause a new `kdc_cid' object to be created and the
         request queue to be bound to this new object behind the scenes.
         The new queue's identifier is obtained by using the current object's
         `next_request_queue_id' state variable. */
    void release_request_queue(kdc_request_queue *queue);
      /* If this is the last request queue associated with a CID, the CID is
         also released; this, in turn, may cause primary channels to be
         released as well.  Before returning, this function always signals
         the `disconnect_event' to wake an application thread which might be
         waiting on the release of a request queue. */
  private: // Cache file I/O -- future versions will move this into `kdu_cache'
    bool look_for_compatible_cache_file();
      /* Called when the `target_id' first becomes available, if a cache
         directory was supplied to `connect'.  The function explores various
         possible cache files, based on the one named in `cache_path', until
         it encounters one for which `read_cache_contents' returns 1, or
         decides that there is no compatible cache file.  It is not sufficient
         to invoke `read_cache_contents' on the initial `cache_path' alone,
         since there may be multiple cache files, corresponding to resources
         with the same name but different target-id's.  In any event, by the
         time this function exits, the `cache_path' member holds the pathname
         of the file into which the cache contents should be saved, when the
         client connection dies.
            Returns true if any cache file's contents were read into the
         cache.  If so, the caller may need to ensure that an extra request
         is added to the relevant request queue so that the server can be
         informed of cache contents.
            Note that this function is called from a context in which the
         management mutex is locked.  Internally, the mutex is unlocked and
         then relocked again to avoid suspending the application during
         a potentially lengthy search. */
    int read_cache_contents(const char *path, const char *target_id);
      /* Attempts to open the cache file whose name is found in `path' and
         read its contents into the cache.  Returns 1 if a file was found
         and read into the cache.  Returns -1 if a file was found, but the
         file has an incompatible `target_id'.  Returns 0 if the file was
         not found.  If the function encounters a file whose version number
         is incompatible with the current version, the function returns -2,
         without making any attempt to check the `target_id'.  The caller
         may wish to delete any such files.  If any error is encountered
         in the format of a file, the function automatically deletes it and
         returns 0. */
    bool save_cache_contents(const char *path, const char *target_id,
                             const char *host_name, const char *resource_name,
                             const char *target_name,
                             const char *sub_target_name);
      /* Saves the contents of the cache to the file whose name is found in
         `path', recording the `target_id' (for cache consistency validation),
         along with sufficient details to connect to the server and open the
         resource again -- these are summarized by the `server_name',
         `resource_name', `target_name' and `sub_target_name' strings.  Either
         or both of the last two may be NULL (for example, the `resource_name'
         might be sufficient to identify the target resource.
         Returns false if the cache file cannot be opened. */
  private: // Other helper functions
    void signal_status()
      { // Called when some status string changes
        if (notifier != NULL) notifier->notify();
      }
    int *get_scratch_ints(int len);
      /* Return a temporary array with space for at least `len' integers.
         The function generates an error if `len' is ridiculously large. */
    char *make_temp_string(const char *src, int max_copy_chars);
      /* Makes a temporary copy of the first `max_copy_chars' characters from
         `src', appending a null-terminator.  The copied string may be shorter
         than `max_copy_chars' if a null terminator is encountered first
         within `src'.  The array is allocated and managed internally and may
         be overwritten by a subsequent call to this function.  The function
         generates an error if the string to be copied is ridiculously
         long. */
    kdc_model_manager *add_model_manager(kdu_long codestream_id);
      /* Called whenever a code-stream main header data-bin becomes complete
         in the cache, this function adds a new manager for the server's
         cache model for that code-stream.  In stateful sessions, it is not
         necessary to create a new client model manager for a code-stream
         whose main header was completed during the current session, since
         the server has a properly synchronized cache model for such
         code-streams.  Returns a pointer to the relevant model manager. */
    bool signal_model_corrections(kdu_window &ref_window,
                                  kdcs_message_block &block);
      /* Called when generating a request message to be sent to the server.
         The purpose of this function is to identify elements from the
         client's cache which the server is not expected to know about and
         signal them using a `model' request field.  For stateless requests,
         the server cannot be expected to know anything about the client's
         cache.  For stateful sessions, the server cannot be expected to
         know about any information which the client received in a
         previous session, stored in a `.kjc' file.
         [//]
         The function is best called from within a try/catch construct to
         catch any exceptions generated from ill-constructed code-stream
         headers.
         [//]
         Returns true if any cache model information was written.  Returns
         false otherwise.  This information may be used to assist in
         determining whether or not the request/response pair is cacheable
         by intermediate HTTP proxies.
      */
    bool parse_query_string(char *query, kdc_request *req,
                            bool create_target_strings,
                            bool &contains_non_target_fields);
      /* Extracts all known JPIP request fields from `query', modifying it so
         that only only the unparsed request fields remain.
         If `req' is non-NULL, the function uses any parsed request fields
         which relate to the window of interest to set members of
         `req->window'.  The function also parses any "len" request fields
         to set the `req->byte_limit' field, if possible; indeed, it may or
         may not be able to parse other fields.  You are responsible for
         ensuring that `req->init()' has already been called.
         If `create_target_strings' is true, the function uses
         request fields which relate to the requested target (if found) to
         allocate and fill `target_name' and/or `sub_target_name' strings for
         the current object, as appropriate.  Otherwise, the function checks
         any encountered target or sub-target strings for compatibility with
         those found in the current object's `target_name' or
         `sub_target_name' members, as appropriate.
         [//]
         The `contains_non_target_fields' argument is set to indicate whether
         or not the `query' string was found to contain any request fields
         other than those which identify the target or sub-target.  If so,
         the query string should be considered a one-time request, but the
         function does not itself set the `non_interactive' member of the
         current object.  Unrecognized query fields which are left behind in
         the `query' string are of course also considered to be non-target
         request fields.
         [//]
         The function returns true if all request fields could be parsed and
         (if `create_target_strings' is false) there was no mismatch detected
         between target fields encountered in the `query' string and the
         current object's `target_name' and `sub_target_name' strings.
       */
  private: // Synchronization and other shared resources
    friend struct kdc_request_queue;
    friend class kdc_cid;
    friend class kdc_primary;
    kdu_thread thread; // Separate network management thread
    kdu_mutex mutex; // For guarding access by app & network management thread
    bool management_lock_acquired; // Used by `acquire_management_lock'
    kdu_event disconnect_event; // Signalled when a request queue is released
    kdcs_timer *timer; // Lasts the entire life of the `kdu_client'
    kdcs_channel_monitor *monitor; // Lasts the entire life of the `kdu_client'
    kdu_client_notifier *notifier; // Reference to application-supplied object
    kdu_client_translator *context_translator; // Ref to app-supplied object
  private: // Members relevant to all request queues
    char *host_name; // Copy of the `server' string supplied to `connect' --
        // this string retains any hex-hex encoding from its supplier.
    char *resource_name; // Resource part of `request' supplied to `connect' --
        // this string retains any hex-hex encoding from its supplier.
    char *target_name; // NULL, or else derived from the query part of the
        // `request' supplied to connect; retains original hex-hex encoding.
    char *sub_target_name; // NULL, else derived from query part of `request'
        // supplied to connect; retains any (highly improbably) hex-hex coding
    char *processed_target_name; // Returned by `get_target_name' -- this name
        // has been hex-hex decoded from `resource_name' or target/sub-target.
    const char *one_time_query; // Points into `resource_name' buffer -- holds
        // original request fields which could not be interpreted; retains any
        // original hex-hex encoding.
    char *cache_path; // For `read_cache_contents' & `save_cache_contents'
    char target_id[256]; // Empty string until we know the target-id
    char requested_transport[41]; // The `cnew' request made for each channel
    bool initial_connection_window_non_empty; // Set inside `connect'
    bool check_for_local_cache; // False once we have tried to load cache file
    bool is_stateless; // True until a session is granted
    bool active_state; // True from `connect' until `close'
    bool non_interactive; // True from `connect' until `close' if appropriate
    bool image_done; // If the server has completely served the entire target
    bool close_requested; // Set by `close'; informs network management thread
    bool session_limit_reached; // If JPIP_EOR_SESSION_LIMIT_REACHED is found
  private: // Global statistics
    const char *final_status; // Status string used once all queues are closed
    kdu_long total_received_bytes; // Total bytes received from server
    kdu_long client_start_time_usecs; // Time client sent first request
    kdu_long last_start_time_usecs; // Time @ 1st request since all queues idle
    kdu_long active_usecs; // Total non-idle time, exlcuding any period since
                           // `last_start_time_usecs' became non-negative.
  private: // Communication state and resources
    kdc_request *free_requests; // List of recycled request structures
    kdc_primary *primary_channels; // List of primary communication channels
    kdc_cid *cids; // List of JPIP Channel-ID's and associated channel state
    kdc_request_queue *request_queues; // List of all alive request queues
    int next_request_queue_id; // Used to make sure request queues are unique
    kdc_model_manager *model_managers; // Active codestream cache managers
    kdu_long next_disconnect_usecs; // -ve if no queue waiting to disconnect
    bool have_queues_ready_to_close; // So `run' doesn't always have to check
  private: // Temporary resources
    int max_scratch_chars; // Includes space for any null-terminator
    char *scratch_chars;
    int max_scratch_ints;
    int *scratch_ints;
  };
  /* Implementation Notes:
        The implementation of this object involves a network management
     thread.  Most of the private helper functions are invoked only from
     within the network managment thread, while the application's thread (or
     threads) invoke calls only to the public member functions, such as
     `connect', `add_queue', `disconnect', `post_window',
     `get_window_in_progress', `is_active', `is_alive' and `is_idle'.  Calls
     to these application API functions generally hold a lock on the `mutex'
     while they are in progress, to avoid unexpected state changes.  However,
     to avoid delaying the application, the network management thread releases
     its lock on the mutex during operations which might take a while, such
     as waiting on network events and resolving network addresses.  In
     order to ensure that the context in which the network management thread
     is not accidentally corrupted by the application while the mutex is
     unlocked, the implementation of application API function must be
     careful never to release/delete any elements from lists.  In particular,
     no application call may result in the removal of an active primary
     channel and active request (one which is not on the `first_unrequested'
     list) or any request queue. */


#endif // KDU_CLIENT_H
