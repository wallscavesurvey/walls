/*****************************************************************************/
// File: client_local.h [scope = APPS/KDU_CLIENT]
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
  Private definitions used in the implementation of the "kdu_client" class.
******************************************************************************/

#ifndef CLIENT_LOCAL_H
#define CLIENT_LOCAL_H

#include "kdcs_comms.h" // Must include this first
#include "kdu_client.h"

// Defined here:
struct kdc_request;
class kdc_flow_regulator;
class kdc_model_manager;
class kdc_primary;
class kdc_cid;
struct kdc_request_queue;

/*****************************************************************************/
/*                                kdc_request                                */
/*****************************************************************************/

struct kdc_request {
  public: // Member functions
    void init(kdc_request_queue *queue)
      {
        qid = -1; this->queue = queue;
        window.init(); extra_query_fields=NULL;
        received_body_bytes = received_message_bytes = 0; byte_limit = 0;
        preemptive = true;
        response_terminated = reply_received = window_completed = false;
        quality_limit_reached = byte_limit_reached = false;
        new_elements = true; copy_created = is_copy = false; next = NULL;
      }
  public: // Data
    int qid; // -1 if request is issued without a JPIP QID field
    kdc_request_queue *queue; // identifies the queue to which request belongs
    kdu_window window;
    const char *extra_query_fields; // For first request issued after `connect'
    int received_body_bytes; // Message body bytes received (excludes EOR body)
    int received_message_bytes; // Message bytes received (excludes EOR body)
    int byte_limit; // If 0, there is no limit specified in the request
    bool preemptive; // True if this request preempts earlier ones
    bool new_elements; // True if not a known subset of a previous request
    bool response_terminated; // If response empty or received EOR message
    bool window_completed; // True if all data for the window has been sent
    bool quality_limit_reached; // True if response terminated at quality limit
    bool byte_limit_reached; // True if response terminated at byte limit
    bool reply_received; // True if server has replied to the request
    bool copy_created; // If this request has been auto-duplicated already
    bool is_copy; // If this request was copied by auto-duplication
    kdc_request *next; // Used to build linked lists
  };

/*****************************************************************************/
/*                             kdc_flow_regulator                            */
/*****************************************************************************/

class kdc_flow_regulator {
  public: // Member functions
    kdc_flow_regulator() { enable(false); }
    bool enable(bool set_to_enabled=true)
      /* The object must be enabled before `get_request_byte_limit' will
         return anything other than 0.  All other calls have no effect
         either unless the object is enabled.  You may disable an enabled
         object by setting the `set_to_enable' flag to false.  The function
         returns the previous enabled state. */
      {
        bool result = enabled; enabled = set_to_enabled;
        response_completion_time = reply_received_time = -1;
        requested_bytes = 0; byte_limit = min_request_byte_limit = 2000;
        return result;
      }
    void reply_received(int requested_bytes, kdu_long ellapsed_usecs)
      {
        this->requested_bytes = requested_bytes;
        reply_received_time = ellapsed_usecs;
      }
      /* Call this when you receive the reply text for a non-empty response.
         The response will follow.  The function allows the gap between the
         end of a previous response and the start of a new response to be
         timed.  The `request_limit' argument indicates the number of bytes
         to which the request was limited (this may have been overridden by
         the server).  The `ellapsed_usecs' argument should be obtained from
         an external `kdcs_timer' object that the caller uses for all
         timing. */
    void response_complete(int received_bytes, bool pause,
                           kdu_long ellapsed_usecs);
      /* Call this when you receive the completed response corresponding to
         the reply for which `reply_received' was called.  The `received_bytes'
         argument indicates the total number of bytes which were received.
         The `pause' argument should be true only if there are not currently
         any queued requests in the pipeline.  The `ellapsed_usecs' argument
         should be obtained from an external `kdcs_timer' object that the
         caller uses for all timing. */
    void set_min_request_byte_limit(int val)
      {
        if (val > min_request_byte_limit)
          min_request_byte_limit = val;
      }
    int get_request_byte_limit()
      {
        return (enabled)?byte_limit:0;
      }
      /* Returns a suggested byte limit for the next request.  An adaptive
         algorithm uses timing statistics derived from the above two 
         functions to come up with a reasonable limit.  The function
         returns 0, meaning that there should be no limit, if the object
         is not currently enabled.  Otherwise, it uses the following
         algorithm:
             Two quantities are derived from the reply and response
         completion events signalled by the above functions.  These are:
         1) Tgap = the time between the end of a non-paused response and
         the receipt of the next reply; and
         2) Tdat = the time between the receipt of the reply text and the
         end of the response data belonging to that reply.  The object
         adaptively adjusts its suggested request byte limit so that
                   Tgap / (Tdat+Tgap) \approx (Tdat+Tgap) / 10 seconds.
           * If Tgap / (Tdat+Tgap) > (Tdat+Tgap)/10, the request size is
         increased, unless the request size was already so large that the
         server pre-empted the last part of the request, reducing the
         response length substantially below the suggested byte limit.
         Evidently, the algorithm will not push Tdat+Tgap beyond 10 seconds.
           * If Tgap / (Tdat+Tgap) < (Tdat+Tgap)/10, the request size is
         decreased, unless Tdat+Tgap is already less than 1 second, in
         which case it will be left as is.  Also, the request byte limit
         will not be decreased below 2 kBytes. */
  private: // Data
    bool enabled;
    kdu_long response_completion_time; // -ve if paused after last response
    kdu_long reply_received_time; // -ve if no reply received yet
    int requested_bytes; // Set when reply is received.
    int min_request_byte_limit; // Lower bound (may be server-supplied)
    int byte_limit;
  };

/*****************************************************************************/
/*                             kdc_model_manager                             */
/*****************************************************************************/

  // This class manages objects which are used to determine and signal
  // cache model information for a single code-stream.
class kdc_model_manager {
  public: // Member functions
    kdc_model_manager()
      { codestream_id=-1; next = NULL; }
    ~kdc_model_manager()
      { if (codestream.exists()) codestream.destroy(); }
  public: // Data
    kdu_long codestream_id;
    kdu_cache aux_cache;
    kdu_codestream codestream;
    kdc_model_manager *next; // For linked list of code-streams
  };

/*****************************************************************************/
/*                               kdc_primary                                 */
/*****************************************************************************/

class kdc_primary : public kdcs_channel_servicer {
  protected: // Destructor may not be invoked directly
    virtual ~kdc_primary()
      {
        if (immediate_server != NULL) delete[] immediate_server;
        if (channel != NULL) delete channel;
      }
  public: // Member functions
    kdc_primary(kdu_client *client)
      {
        immediate_server=NULL; immediate_port=0;
        channel_connected=false; channel_reconnect_allowed = false;
        channel=NULL; channel_timeout_set = false; using_proxy=false;
        is_persistent=true; keep_alive=false; is_released=false;
        num_http_tcp_cids = num_http_only_cids = 0;
        active_requester=NULL; first_active_cid = last_active_cid = NULL;
        waiting_to_read = false; in_http_body = false;
        chunked_transfer = false; chunk_length = 0; total_chunk_bytes = 0;
        this->client = client; next = NULL;
      }
    void release() { release_ref(); }
      /* Call this function in place of the destructor. */
    void set_last_active_cid(kdc_cid *cid);
      /* Appends `cid' to the tail of the object's active-CID list managed by
         `first_active_cid' and `last_active_cid'.  This function may be
         called even if the `cid' is already on the active list, in which
         case it verifies that `cid' is already the last active CID. */
    void remove_active_cid(kdc_cid *cid);
      /* Always use this function to remove the `cid' from the object's
         active-CID list, managed by `first_active_cid' and `last_active_cid'.
         If this leaves the active-CID list empty and the channel is not
         persistent, this function also closes the TCP channel, but otherwise
         leaves everything intact.  If `cid' has been newly created in
         response to a JPIP-cnew response from the server, this function
         calls `cid->assign_ongoing_primary_channel' as soon as it is
         removed from the initial primary channel's active-CID list. */
    void resolve_address();
      /* This function is called to resolve the `immediate_address', if
         `channel' is found to be NULL.  If the address cannot be resolved,
         an error is generated.  If the address resolves to the same address
         as that of an existing primary channel which has no users (no CID's),
         that primary channel is released, after taking control of its TCP
         connection.  Otherwise, a new TCP `channel' is created here, but
         the connection process is not itself initiated. */
    void send_active_request();
      /* This function is called whenever there is non-NULL `active_requester'
         and the `send_block' is non-empty.  If the immediate server address
         has not yet been resolved, it is done here.  If the channel is not
         yet connected, an attempt is made to connect it here.  If the
         attempt to send the request over a (supposedly) connected channel
         fails and `allow_channel_reconnect' is true, this function initiates
         another connection attempt, but sets `allow_channel_reconnect' to
         false, so that this will not happen again, at least until a reply
         is successfully received on the channel.  If the request is sent
         and `waiting_to_read' is false, the function also initiates the
         process of reading reply/body data so as to ensure that the
         `service_channel' function gets called when data is available for
         receiving. */
    bool read_reply();
      /* This function is invoked to read HTTP reply text into the
         `recv_block'.  If `in_http_body' is true, this function should not
         be used (but will return false anyway), since the data next to be
         read belongs to an HTTP response body, as opposed to the reply
         paragraph.  The function also return false if the first active CID
         is not expecting a reply.  It returns true if and only if it reads
         a complete reply paragraph.  The `waiting_to_read' member is set to
         true if an attempt is made to read a reply paragraph, but not all the
         required characters are available yet from the channel. */
    bool read_body_chunk();
      /* This function is invoked to read a single chunk of HTTP response
         data (in chunked transfer mode) or the complete response (for
         non-chunked transfer).  It returns true if it succeeds in reading a
         chunk.  In chunked transfer mode, if the function enters with
         `chunk_length'=0, the function first attempts to read the length
         of the next chunk.  If it encounters the chunked transfer terminator,
         or if it finds that `chunk_length'=0 when the transfer mode is
         not chunked, the function returns true but leaves `in_http_body'
         equal to false.  As with `read_reply', the `waiting_to_read' flag
         is set to true if an attempt is made to read a chunk of data or
         its header/trailer and we fail to get all the way through.
            Note that this function may duplicate the current request
         so as to implement the HTTP-only flow control strategy.  Duplicated
         requests are marked as such so that they are not duplicated again.
         This might otherwise be a risk if the request being duplicated
         is non-preemptive, since duplicates may be inserted ahead of other
         non-preemptive requests. */
    void signal_status(const char *text);
      /* Adjusts the `status_string' of all request queues which use this
         primary channel, then invokes the `client->notifier->notify'
         function. */
    protected: // Implementation of `kdcs_channel_servicer' interface
    virtual void service_channel(kdcs_channel_monitor *monitor,
                                 kdcs_channel *channel, int cond_flags);
      /* This function is invoked by the channel monitor when the `channel'
         object needs to be serviced. */
  public: // Data
    char *immediate_server; // Name or IP address of server or proxy
    kdu_uint16 immediate_port; // Port to use with `immediate_server'
    kdcs_sockaddr immediate_address; // Resolved address from above members
    kdcs_tcp_channel *channel; // NULL if address resolution not yet complete
    bool channel_connected; // True once `channel' is connected
    bool channel_reconnect_allowed; // See `send_active_request'
    bool channel_timeout_set; // If a scheduled timeout has been set already
    bool using_proxy; // True if `immediate_server' is actually a proxy
    bool is_persistent; // True if `channel' persists to the next request
    bool is_released; // Set by `client->release_primary_channel'
    bool keep_alive; // Set if the channel is to be preserved beyond `close'
  public: // Members used to keep track of the channel's users
    int num_http_tcp_cids; // Number of HTTP-TCP CID's using me (0, 1, 2, ...)
    int num_http_only_cids; // Number of HTTP-only CID's using me (0 or 1)
    kdc_request_queue *active_requester;
    kdc_cid *first_active_cid, *last_active_cid; // See below
  public: // Members used to handle requests & responses for active CID's
    bool waiting_to_read; // If channel monitor knows we have more data to read
    bool in_http_body; // If next bytes to be read belong to HTTP response body
    bool chunked_transfer; // If body data to be transfered in chunks
    int chunk_length; // Length of next chunk or entire response if not chunked
    int total_chunk_bytes; // Accumulates chunk lengths from single response
    kdcs_message_block query_block;
    kdcs_message_block send_block; // If non-empty, request is still outgoing
    kdcs_message_block recv_block;
    kdc_flow_regulator flow_regulator; // See below
  public: // Links
    kdu_client *client;
    kdc_primary *next; // Used to build list of primary HTTP transport channels
  };
  /* Notes:
        This object represents a single HTTP transport, for issuing requests
     and receiving replies and or response data.  Each JPIP channel
     (represented by a `kdc_cid' object) uses a single primary HTTP channel
     (i.e., a single `kdc_primary' object).
        Each JPIP channel which uses the HTTP-only transport has its own
     primary channel.  However, HTTP-TCP transported JPIP channels share
     a common HTTP channel wherever possible.
        The `active_requester' member, if non-NULL, points to the request
     queue associated with a currently scheduled request.  This pointer
     remains valid at least until the request has been delivered over the
     HTTP channel.  For HTTP-only transported CID's, the `active_requester'
     member remains non-NULL until the reply is actually received.  For
     HTTP-TCP transported CID's, the `active_requester' member can become
     NULL as soon as the request has been delivered.  The `active_requester'
     member blocks new requests from being delivered, so we sometimes leave
     it non-NULL for longer so as to temporarily prevent unwanted request
     interleaving.  For a new request queue (one with
     `kdc_request_queue::just_started'=true) we always leave the queue as
     the active requester until the reply is received, which allows
     persistence to be determined and also allows for the fact that the
     first reply may assign a new CID for the queue whose transport type
     could affect the policy described above.  If the channel turns out to
     be non-persistent, we leave the `active_requester' non-NULL
     until the HTTP response (reply and any response data) are received.  We
     do the same thing for stateless communications (i.e., where there is no
     channel-ID), since we cannot formulate a comprehensive set of cache
     model statements until we have received all outstanding data.  Of course,
     this makes stateless communication less responsive, but that is why
     JPIP is designed with stateful sessions in mind.
        `first_active_cid' points to a list of all CID's which are or have
     been the `active_requester', but for which the reply (including HTTP
     response data for HTTP-only CID's) is still outstanding.  This means
     that as soon as a CID supplies a valid request, its active request queue
     becomes the `active_requester' and the CID itself is appended to the
     tail of the active CID list (identified by `last_active_cid').  If a
     CID issues multiple requests consecutively, for which there are multiple
     outstanding replies, that CID's replies (and responses, if
     HTTP-transported) must all be cleared before we can advance the list.
        For HTTP-only communications, there is only one CID associated with
     any given primary transport channel.  For HTTP-only communications,
     the `flow_regulator' is also used to estimate the channel/server
     behaviour and to generate reasonable length limits to apply to outgoing
     requests, so that the primary HTTP transport does not become clogged with
     the responses to past requests, damaging responsiveness to new requests
     which may need to pre-empt existing ones.  The `flow_regulator'
     expects us to use the policy of not sending new requests until at least
     the reply to a past request has been received.  This is why we leave
     `active_requester' non-NULL until the point at which the reply has been
     received.  Once it becomes non-NULL, a new request can be scheduled.
        For HTTP-TCP communications, the `flow_regulator' is not used and
     the server manages flow control.  For this reason, we allow new requests
     from different CID's to be dispatched before existing requests have even
     been replied.  In the process, the active CID's list may grow to
     represent the CID's for which requests have been delivered but replies
     have not yet been received.  No CID may appear in multiple
     locations on the list of active CID's, so a CID may not pre-empt
     its own requests while other CID's are still waiting to send their
     requests ... in practice, this should not be much of a limitation,
     since the server is unlikely to be able to de-interleave pre-empting
     requests from later in the TCP request pipe while there are
     outstanding requests to be processed from other CID's, earlier in the
     pipe.
  */

/*****************************************************************************/
/*                                 kdc_cid                                   */
/*****************************************************************************/

class kdc_cid : public kdcs_channel_servicer {
  protected: // Destructor may not be invoked directly
    virtual ~kdc_cid()
      {
        if (channel_id != NULL) delete[] channel_id;
        if (resource != NULL) delete[] resource;
        if (server != NULL) delete[] server;
        if (aux_channel != NULL) delete aux_channel;
      }
  public: // Member functions
    kdc_cid(kdu_client *client)
      {
        channel_id = resource = server = NULL;
        request_port = return_port = 0; primary_channel = NULL;
        aux_channel = NULL; uses_aux_channel = aux_channel_connected = false;
        aux_timeout_set = false; on_primary_active_list = false;
        newly_assigned_by_server = false; channel_close_requested = false;
        is_released = false; next_qid = 1; last_requester = NULL;
        first_active_receiver = last_active_receiver = NULL;
        num_request_queues = 0; have_unsent_ack = false; aux_chunk_length = 0;
        total_aux_chunk_bytes = 0; have_new_data_since_last_alert=false;
        last_msg_class_id = 0; last_msg_stream_id = 0;
        this->client = client; next = NULL; next_active_cid = NULL;
      }  
    void release() { release_ref(); } // Call this instead of destructor
    void set_last_active_receiver(kdc_request_queue *queue);
      /* Appends `queue' to the tail of the object's active-CID list managed by
        `first_active_receiver' and `last_active_receiver'.  This function may
        be called even if the `queue' is already on the active receiver list,
        in which case it verifies that `queue' it is already the last active
        receiver. */
    void remove_active_receiver(kdc_request_queue *queue);
      /* Convenience function to remove the `queue' from the object's
         active receiver list, managed by `first_active_receiver' and
         `last_active_receiver'. */
    kdc_request_queue *find_next_requester();
      /* This function scans for request queues which are using this CID to
         determine one which can transmit next.  The function returns NULL
         if no request queue is ready or allowed to transmit on this CID.
            If `last_requester' is non-NULL, the function tries to choose
         a different next requester, if there is one with a request
         available.  It is important that `last_requester' be either NULL
         or point to a valid entry in the `client's `request_queues' list
         since the function searches from the queue immediately following
         the last requester in that list.
            The function takes care of implementing the request scheduling
         rules.  Specifically, a request queue can issue a request if has
         one available, if it is not already on its CID's active receiver
         list in any position other than that of last active receiver, if
         the primary channel's `active_requester' member is NULL, and if
         any other queue on the CID's active receiver list has received
         replies to all outstanding requests.
      */
    void process_return_data(kdcs_message_block &block, kdc_request *req);
      /* Absorbs as much as possible of the return data contained in the
         supplied message block.  The function extracts only data which
         pertains to a single request (identified by the `req' argument).  If
         it encounters an EOR message, the `req->response_terminated' flag is
         set, along with other flags which might be derived from the EOR
         reason code (e.g., `req->window_done' and `req->byte_limit_reached'),
         after which the function returns.  If you are using this function to
         process HTTP response data chunks, the message block should end after
         any EOR message is encountered.  In the case of data returned over an
         HTTP-TCP transport's auxiliary channel, each chunk of data might
         possibly contain multiple EOR messages, though.  In this case, the
         function returns after parsing the first EOR message and the caller
         may need to invoke it again on a subsequent `kdc_request' object.
         Along the way, the function updates `req->received_body_bytes' to hold
         the total number of body bytes from all JPIP messages parsed in
         response to the request (except for EOR messages).  It also updates
         `req->received_message_bytes' to hold the total number of message
         bytes (bodies + headers) from all JPIP messages parsed in response
         to the request (except for EOR messages). */
    void assign_ongoing_primary_channel();
      /* This function is called on a CID which was newly assigned by the
         server when it is first removed from its original primary channel's
         active CID list.  The function also disables flow control for
         primary channels which are used with HTTP-TCP communications.
         The function assumes that the `server_address' has already been
         resolved, if required.  The function does not itself throw
         exceptions or generate calls to `kdu_error'. */
    bool connect_aux_channel();
      /* Called if the auxiliary channel still needs to be connected.  May
         throw an exception.  If successful, the function sets
         `aux_channel_connected' to true and returns true; returns false if
         the connection cannot yet be completed. */
    bool read_aux_chunk();
      /* Reads a single auxiliary response data chunk, processes its contents
         and sends the corresponding acknowledgement message.  If all of this
         succeeds, the function returns true.  Otherwise it returns false and
         leaves at least one network condition monitoring event behind which
         will eventually result in a call to `service_channel' re-invoking
         this function.  You should always call this function repeatedly
         until it returns false or throws an exception.  If an exception is
         thrown, the current object should generally be released by calling
         `kdu_client::release_cid'. */
    void signal_status(const char *text);
      /* Adjusts the `status_string' of all request queues which use this
         CID, then invokes the `client->notifier->notify' function. */
    void alert_app_if_new_data()
      {
        if (!have_new_data_since_last_alert) return;
        client->signal_status();
        have_new_data_since_last_alert = false;
      }
      /* Invokes `client->notifier->notify' if new data has been entered into
         the cache from this CID's `process_return_data' function, since
         this function was last called. */
  protected: // Implementation of `kdcs_channel_servicer' interface
    virtual void service_channel(kdcs_channel_monitor *monitor,
                                 kdcs_channel *channel, int cond_flags);
      /* This function is invoked by the channel monitor when the
         `aux_channel' needs to be serviced in one way or another. */
  public: // Basic communication members
    char *channel_id; // NULL if "CID" is stateless (at least so far)
    char *resource; // Name of resource to be used in requests
    char *server; // Name or IP address of server to use for next request
    kdu_uint16 request_port; // Port associated with `server'
    kdu_uint16 return_port; // Port associated with aux return channel
    kdcs_sockaddr server_address; // Used to store any resolved address
    kdc_primary *primary_channel; // We bind each CID to a single HTTP channel
    kdcs_tcp_channel *aux_channel; // Used with HTTP-TCP transport
    bool uses_aux_channel; // True as soon as HTTP-TCP transport identified
    bool aux_channel_connected; // True once `aux_channel' has been connected
    bool aux_timeout_set; // True if wakeup has been scheduled for aux_channel
    bool on_primary_active_list; // If on `primary_channel' active CID list
    bool newly_assigned_by_server; // See below
    bool channel_close_requested; // If a cclose request field has been issued
    bool is_released; // Set by `client->release_cid'
    kdu_uint32 next_qid; // Don't really need this yet
    kdu_window_prefs prefs;
  public: // Request queue association members
    int num_request_queues; // Number of request queues using this CID
    kdc_request_queue *last_requester; // See below
    kdc_request_queue *first_active_receiver; // See below
    kdc_request_queue *last_active_receiver; // See below
  public: // Members used to buffer auxiliary channel data
    kdcs_message_block aux_recv_block; // Used to receive data chunks
    kdu_byte ack_buf[8];
    bool have_unsent_ack;
    int aux_chunk_length; // 0 if not yet ready to read a chunk
    int total_aux_chunk_bytes; // Mainly for debugging purposes
  public: // Message decoding state
    bool have_new_data_since_last_alert; // Used by `alert_app_if_new_data'
    int last_msg_class_id;
    kdu_long last_msg_stream_id;
  public: // Links
    kdu_client *client;
    kdc_cid *next; // Used to build a list of JPIP channels
    kdc_cid *next_active_cid; // For `kdc_primary' active CID lists
  };
  /* Notes:
        CID objects are used to represent JPIP channels (the name comes from
     the fact that JPIP channels are identified by a `cid' query field in
     the JPIP request syntax).  There is exactly one CID for each channel
     assigned by the server, but for stateless communications, we use a
     single `kdc_cid' object with a NULL `channel_id' member.  All JPIP
     communication starts stateless, but may become stateful if the server
     grants a `cnew' request, in which case the `channel_id' member becomes
     non-NULL and stores the server-assigned unique Channel-ID.
        All CID's are associated with a primary HTTP transport, over which
     requests are delivered and replies received.  As already discussed under
     `kdc_primary', HTTP-only CID's (those where `uses_aux_channel' is false)
     each have their own primary HTTP channel (i.e., `primary_channel' points
     to a unique channel for each such CID).  HTTP-TCP transported CID's,
     however, have use the `aux_channel' to receive response data.  These
     CID's share a common primary HTTP transport wherever they can (i.e.,
     wherever the IP address and port assigned for the primary HTTP
     communications by the server are consistent), so as to conserve
     resources.  There are no real efficiency benefits to separating the
     HTTP channels used by HTTP-TCP transported JPIP channels.
        The `first_active_receiver' and `last_active_receiver' members manage
     a list of all request queues which have at least begun to issue a
     request, but have not yet received all replies and response data to
     their outstanding requests.  A request queue may appear on this list
     only once, so it is not possible for a request queue to issue a new
     request if it is already in any position on this list other than the
     tail.  This principle is embodied in the implementation of the
     `find_next_requester' function.
        The `last_requester' member points to the request queue which most
     recently issued a request.  This member is set by the
     `kdc_request_queue::issue_request' function, which also appends the
     requester to the end of the active receiver list.
        The `newly_assigned_by_server' flag is true if this CID was created
     in response to a JPIP-cnew response from the server.  In this case,
     communication over the new CID proceeds using the primary channel on
     which the JPIP-cnew response was received, but only for the request.  No
     new requests are accepted over the CID until the primary channel
     connection details can be verified (possibly reassigned), which takes
     place when the CID is first removed as the active-CID on its original
     primary channel.  The `kdc_primary::remove_active_cid' watches out for
     this condition and calls `assign_ongoing_primary_channel'.
  */

/*****************************************************************************/
/*                             kdc_request_queue                             */
/*****************************************************************************/

struct kdc_request_queue {
  public: // Member functions
    kdc_request_queue(kdu_client *client)
      {
        queue_id = 0; cid=NULL;
        request_head = request_tail = first_incomplete = NULL;
        first_unreplied = first_unrequested = NULL;
        is_active_receiver = false; just_started = is_idle = true;
        close_when_idle = false; disconnect_timeout_usecs = 0;
        status_string = "Request queue created"; received_bytes = 0;
        queue_start_time_usecs = last_start_time_usecs = -1; active_usecs = 0;
        this->client = client; next = NULL; next_active_receiver = NULL;
      }
    kdc_request *add_request();
      /* Appends a new request to the internal queue, leaving the caller
         to fill out the window parameters and the `new_elements' flag.
         A side-effect of this function, is that it leaves `is_idle'=false. */
    void duplicate_request(kdc_request *req);
      /* Makes a copy of the request on the queue.  It is the caller's
         responsibility to ensure that `req' is already on the queue in
         question.  The new request is inserted immediately after `req'. */
    void remove_request(kdc_request *req);
      /* Can be used to remove any request in the queue and rearrange things
         as required. */
    void request_completed(kdc_request *req);
      /* This function is called when `req' (which must be identical to
         `first_incomplete' on entry) is found to have been completed,
         meaning that the reply has been received and the response is also
         terminated, for one reason or another.  Accordingly,
         `req->reply_received' and `req->response_terminated' must be true.
            The function advances `first_incomplete', discards all but
         the first complete response from the request queue (we always try
         to keep one around to see if new requests are redundant or not),
         and potentially adjusts the `is_idle' and `status_string' members.
            The function also checks whether or not any as-yet unsent requests
         are made redundant by the new completed request, removing them
         if required -- except that the final request of a queue, whose
         `close_when_idle' flag is set, is never removed (this is the one
         on which the "cclose" message is sent).
            If this was a stateless request, the function updates the set
         of model managers, so that there is one for each code-stream for
         which we have received a main header. */
    void issue_request();
      /* Called from the management thread when we are ready for a request
         to be issued.  This function should not be called until
         `cid->get_next_requester' returns the current queue.
             Moreover, the function should not be called until
         `cid->newly_assigned_by_server' is false, since we should not be
         issuing new requests over a CID until its primary
         connection details have been validated and/or reassigned.
            The function also takes responsibility for adding the queue onto
         its CID's active-receiver list, if it is not already there, since
         a queue must enter the active receiver list from the moment that
         a request is first commenced, even if it takes a while to go out
         through the relevant TCP channel.
            If this function issues a request over a CID which already has an
         incomplete request from one or more other request queues (i.e.,
         request queues other than the current one, which are on the CID's
         active receiver list) and the current request is preemptive, it is
         likely to pre-empt these other request queues.  To conceal this
         from the application, the present function synthesizes a copy of
         each such potentially pre-empted request.  A copy is made if the
         potentially pre-empted request is the final one on its queue or
         if the ensuing request is non-preemptive.  Request copies have
         the same pre-emptability as the source request from which they
         are copied.  Moreover, when a request is duplicated, it is marked
         as having been duplicated so that no attempt is made to duplicate
         it again, either here or inside the `kdc_primary::read_body_chunk'
         function.
            The function does not actually send the request, mainly because
         this might generate an exception that could be hard to back out of
         without killing the entire client connection.  Instead, the
         caller should later scan the primary request channels, looking for
         any which have `kdc_primary::active_requester' non-NULL and
         `kdc_primary::send_block' non-empty, invoking the corresponding
         `kdc_primary::send_active_request' funcion if so.  It is easier
         to catch network-generated exceptions at that point.  If the
         request needs to be re-sent due to channel queue fulness or
         anything like that, it will happen automatically from within
         the `kdc_primary::service_channel' function which is invoked by
         the client's channel monitor object. */
    kdc_request *process_reply(const char *reply);
      /* This function processes an HTTP reply paragraph recovered from the
         server.  If the paragraph is empty or contains a 100-series return
         code, the function returns NULL, meaning that the actual reply
         paragraph is still expected.  Otherwise, the function processes
         the reply headers, using them to adjust the state of the
         `first_unreplied' entry on the request queue and returning a
         pointer to this entry.  When the function returns non-NULL, the
         `first_unreplied' pointer is advanced and the returned request
         has its `reply_received' entry set to non-NULL.  The function
         parses and sets channel persistence information, window fields,
         new-channel (JPIP-cnew) parameters and any received information
         about the global target-ID.  However, it does not parse
         content-length or transfer encoding information; this is left to
         the caller. */
    void transfer_to_new_cid(kdc_cid *new_cid);
      /* This is a delicate function, which is used during the establishment
         of a new JPIP channel to associate the request queue with a new
         CID.  The new CID has exactly the same primary request channel as
         the old one (at least for now).  Moreover, the function is called
         from within the `process_reply' function, so the queue is
         currently the existing CID's active requester, as well as being on
         its active receiver list.  As a result, there are quite a few things
         to move around.
            Once done, the queue's previous CID may possibly be left
         with no request queues.  This is possible only if all other request
         queues (there must originally have been at least one) using the CID
         were closed since the call to `kdu_client::add_queue' but before
         the new queue was established -- this is actually quite a likely
         occurrence for some navigation patterns.  In this event, the
         function assigns a temporary request queue for the old CID, which
         is marked with `close_when_inactive' and given a single empty
         request so that it will issue the JPIP `cclose' request.  Otherwise
         we would be left with an orphan request queue on the client side
         as well as the server side, consuming valuable server resources until
         the session is closed. */
    void adjust_active_usecs_on_idle();
      /* This function is called if the queue goes idle.  It may be called
         from the application thread or from the network management thread.
         The function adjusts the `last_start_time_usecs' and `active_usecs'
         members in the current object and, if all queues are idle, the
         main `client' object as well. */
    void signal_status(const char *text)
      {
        this->status_string = text;
        client->signal_status();
      }
  public: // Data
    int queue_id; // Identifier presented by `kdu_client' to the application
    kdu_window_prefs prefs; // Maintains service prefs for this queue
    kdc_cid *cid; // Each request queue is associated with one CID
    kdc_request *request_head; // List of all requests on the queue
    kdc_request *request_tail; // Tail of above list
    kdc_request *first_incomplete; // First request on the queue for which a
                 // complete response is not yet available.
    kdc_request *first_unreplied; // First request on the queue for which the
                 // reply has not yet been received.
    kdc_request *first_unrequested; // First request on the queue for which the
                 // process of issuing the request of the CID has not yet
                 // started; this member is advanced by `issue_request', even
                 // though it may take some time for the corresponding call to
                 // `kdc_primary::send_active_request' to completely push the
                 // request out on the relevant primary HTTP channel.
  public: // Status
    bool is_active_receiver; // If on `cid's active receiver list
    bool just_started; // True until the reply to the first request is received
    bool is_idle; // See `kdu_client::is_idle'
    bool close_when_idle; // Set by `kdu_client::disconnect'
    kdu_long disconnect_timeout_usecs; // Absolute timeout, set by `disconnect'
    const char *status_string; // See `kdu_client::get_status'
    kdu_long received_bytes;
    kdu_long queue_start_time_usecs; // Time first request was sent, or -1
    kdu_long last_start_time_usecs; // Time of first request since idle, or -1
    kdu_long active_usecs; // Total non-idle time, excluding any period since
                           // `last_start_time_usecs' became non-negative.
  public: // Links
    kdu_client *client;
    kdc_request_queue *next; // For list of all request queues
    kdc_request_queue *next_active_receiver; // For CID's active receiver list
  };

#endif // CLIENT_LOCAL_H
