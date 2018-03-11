/*****************************************************************************/
// File: kdu_server.cpp [scope = APPS/SERVER]
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
   A simple JPEG2000 interactive image server application, built around the
support offered by the Kakadu core system for region-of-interest access,
in-place transcoding, on-demand packet construction, and caching compressed
data sources.
******************************************************************************/

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "kdcs_comms.h"
#include "kdu_compressed.h"
#include "kdu_messaging.h"
#include "kdu_args.h"
#include "server_local.h"

#ifdef KDU_WINDOWS_OS
#  define KDS_ASSUME_CASELESS_FILENAMES
#endif

/* ========================================================================= */
/*                         Set up messaging services                         */
/* ========================================================================= */

static kd_message_lock message_lock;
static kd_message_sink sys_msg(stdout,&message_lock,false);
static kd_message_sink sys_msg_with_exceptions(stdout,&message_lock,true);

static kdu_message_formatter kd_msg_with_exceptions(&sys_msg_with_exceptions);

kdu_message_formatter kd_msg(&sys_msg);
kd_message_log kd_msg_log;


/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                        print_version                               */
/*****************************************************************************/

static void
  print_version()
{
  kdu_message_formatter &out = kd_msg;
  out.start_message();
  out << "This is Kakadu's \"kdu_server\" application.\n";
  out << "\tCompiled against the Kakadu core system, version "
      << KDU_CORE_VERSION << "\n";
  out << "\tCurrent core system version is "
      << kdu_get_core_version() << "\n";
  out.flush(true);
  exit(0);
}

/*****************************************************************************/
/* STATIC                         print_usage                                */
/*****************************************************************************/

static void
  print_usage(char *prog, bool comprehensive=false)
{
  kdu_message_formatter &out = kd_msg;
  out.start_message();

  out << "Usage:\n  \"" << prog << " ...\n";
  out.set_master_indent(3);
  out << "-wd <working directory>\n";
  if (comprehensive)
    out << "\tSpecifies the working directory for the kakadu server; this can "
           "work in conjunction with the -restrict parameter if required.\n";
  out << "-cd <directory in which to store \".cache\" files>\n";
  if (comprehensive)
    out << "\tThe server creates a \".cache\" file for each source file "
           "that it serves, which contains a digest of the metadata "
           "structure and the selected placeholder partitioning (see "
           "`-phld_threshold').  So long as the \".cache\" file persists, "
           "the image will presented in the same way to clients.  This means "
           "that clients can reliably share information with each other "
           "and that a client may reconnect to an image at some later "
           "point (perhaps days or weeks) and fully reuse the information "
           "cached from previous browsing sessions.  Once the \".cache\" "
           "file is created, it will not be changed when clients later "
           "connect to the same image.  By default, the \".cache\" file "
           "is written to the same directory as the original image file.  "
           "The present argument allows you to specify an alternate "
           "directory for the \".cache\" files.  The cache file's path name "
           "is formed by appending the requested image file name, "
           "including all relative path segments, to the supplied directory "
           "name.  If the cache directory is specified with a relative "
           "path, that path is relative to the working directory, which "
           "may be explicitly specified via the `-wd' argument.  This "
           "argument and the `-wd' argument are both most reliably used "
           "in conjunction with the `-restrict' argument. \n";
  out << "-log <log file>\n";
  if (comprehensive) 
    out << "\tRedirects console messages to the specified log file.\n";
  out << "-passwd <administration password>\n";
  if (comprehensive)
    out << "\tIf this argument is used, you will be able to perform various "
           "remote administration tasks by using the \"kdu_server_admin\" "
           "application, with the same password.  The password is any "
           "non-empty character string, although long strings, containing "
           "uncommon characters and sub-strings are best, as usual.\n";
  out << "-port <listen port number>\n";
  if (comprehensive)
    out << "\tBy default, the server listens for HTML connection reqests "
           "on port 80.  To override this, you may specify a different port "
           "number here.\n";
  out << "-address localhost|<listen IP address>\n";
  if (comprehensive)
    out << "\tYou should not need to use this command unless you wish to "
           "serve images from a multi-homed host or force the server to use "
           "only the standard \"localhost\" address (127.0.0.1) for "
           "clients located on the same machine.  By default, the server "
           "uses the first valid IP address on which the machine is "
           "capable of listening.  The parameter string supplied with this "
           "option should either be a valid IPv4 or IPv6 address, in the "
           "familiar dotted (IPv4) or colon-separated (IPv6) format, or else "
           "the word \"localhost\".\n";
  out << "-delegate <host, IP or bracketed IP>[:<port>][*<load share>]\n";
  if (comprehensive)
    out << "\tDelegate the task of actually serving clients to one or "
           "more other servers, usually running on different machines.  This "
           "argument may be supplied multiple times, once for each host to "
           "which service may be delegated.  When multiple delegates are "
           "supplied, the load is distributed on the basis of the optional "
           "load share specifier.  The load share is a positive integer, "
           "which defaults to 4, if not explicitly provided.  Clients are "
           "delegated to the available hosts on a round-robin basis until "
           "each host has received its load share, after which all the "
           "load share counters are initialized to the load share value "
           "and round robin delegation continues from there.  Note that "
           "literal host IP addresses are best enclosed in square brackets, "
           "especially for IPv6 addresses, since this eliminates ambiguity "
           "associated with the decoding of the optional port suffix.\n";
  out << "-max_idle_time <max session idle seconds>\n";
  if (comprehensive)
    out << "\tBy default, clients will be automatically disconnected after "
           "being continuously connected, yet idle, for a period of 5 "
           "minutes.  A different maximum idle time (expressed in seconds) "
           "may be specified here.\n";
  out << "-clients <max client connections>\n";
  if (comprehensive)
    out << "\tBy default, the server permits two client connections to "
           "be serviced at once.  This argument allows the limit to be "
           "adjusted.  If you select a very large value here, you may "
           "find that the internal implementation will refuse to grant "
           "sufficient connections anyway, in which case it may need to "
           "be recompiled with a larger value for `FD_SETSIZE' in "
           "\"kdcs_comms.h\".\n";
  out << "-sources <max open sources>\n";
  if (comprehensive)
    out << "\tBy default, the server permits one open JPEG2000 source file "
           "for each client connection.  This argument may "
           "be used to reduce the number of allowed source files.  Clients "
           "browsing the same image share a single open source file, which "
           "leads to a number of efficiencies.  New client connections will "
           "be refused, even if the total number of clients does not exceed "
           "the limit supplied using `-clients', if the total number of open "
           "files would exceed the limit.  If the `-clients' argument is "
           "missing, the value supplied to a `-sources' argument also becomes "
           "the maximum number of connected clients.\n";
  out << "-cids <max JPIP channel-ID's per session>\n";
  if (comprehensive)
    out << "\tBy default, each stateful session involves at most two (2) JPIP "
           "channels, the first of which is assigned when the client first "
           "issues a JPIP \"cnew\" request.  JPIP allows for clients to "
           "request additional channel-ID's, so that multiple window "
           "requests can be served simultaneously with a single cache model.  "
           "This can lead to very efficient interaction with large sources, "
           "but at the expense of additional socket and memory resources.  "
           "This argument allows you to configure the maximum number of "
           "JPIP channels which can be managed on behalf of each client, "
           "in a stateful communication session.\n";
  out << "-max_rate <max bytes/second>\n";
  if (comprehensive)
    out << "\tBy default, transmission of JPEG2000 packet data to the client "
           "is limited to a maximum rate of 10 kBytes/second.  "
           "A different maximum transmission rate (expressed in bytes/second) "
           "may be specified here.  Data is transmitted at the maximum "
           "rate until certain queuing constraints are encountered, if any.\n";
  out << "-max_rtt <max target RTT, in seconds>\n";
  if (comprehensive)
    out << "\tMaximum value to be used as the server's target round trip time "
           "(RTT).  The actual instantaneous RTT may be somewhat larger than "
           "this, depending upon network conditions.  The default value for "
           "this argument is 2 seconds.\n";
  out << "-min_rtt <min target RTT, in seconds>\n";
  if (comprehensive)
    out << "\tMinimum value to be used as the server's target round trip time "
           "(RTT). The actual instantaneous RTT may be smaller than this "
           "value, but the server endeavours to queue sufficient messages "
           "onto the network so as to realize at least this RTT.  The "
           "default value for this argument is 0.5 seconds.  Whenever "
           "the minimum target RTT is lower than the maximum target RTT, the "
           "server will attempt to hunt for the smallest target RTT which is "
           "consistent with these bounds and also maximizes network "
           "utilization.\n";
  out << "-max_chunk <max transfer chunk bytes>\n";
  if (comprehensive)
    out << "\tBy default, the maximum size of the image data chunks shipped "
           "by the server is 1024 bytes.  Flow control at the server or "
           "client is generally performed on chunk boundaries, so smaller "
           "chunks may give you finer granularity for estimating network "
           "conditions, at the expense of higher computational overhead, "
           "and some loss of transport efficiency.  In any event, you may "
           "not specify chunk sizes smaller than 128 bytes here.  "
           "Values smaller than about 32 bytes could cause some fundamental "
           "assumptions in the \"kdu_serve\" object to fail.\n";
  out << "-ignore_relevance\n";
  if (comprehensive)
    out << "\tBy supplying this flag, you force the server to ignore the "
           "degree to which a precinct overlaps with the spatial window "
           "requested by the client, serving up compressed data from all "
           "precincts which have any relevance at all, layer by layer, "
           "with the lowest frequency subbands appearing first within each "
           "layer.  By contrast, the default behaviour is to schedule "
           "precinct data in such a way that more information is provided "
           "for those precincts which have a larger overlap with the "
           "window of interest.  If the source code-stream contains a "
           "COM marker segment which identifies the distortion-length slope "
           "thresholds which were originally used to form the quality layers, "
           "and hence packets, this information is used to schedule precinct "
           "data in a manner which is approximately optimal in the same "
           "rate-distortion sense as that used to form the original "
           "code-stream layers, taking into account the degree to which "
           "each precinct is actually relevant to the window of interest.\n";
  out << "-phld_threshold <JP2 box partitioning threshold>\n";
  if (comprehensive)
    out << "\tThe threshold represents the maximum size for any JP2 box "
           "before that box is replaced by a placeholder in its containing "
           "data-bin, where the placeholder identifies a separate data-bin "
           "which holds the box's contents.  Selecting a large value for the "
           "threshold allows all meta-data to be appear in meta data-bin 0, "
           "with placeholders used only for the contiguous codestream boxes.  "
           "Selecting a small value tends to distribute the meta-data over "
           "numerous data-bins, each of which is delivered to the client "
           "only if its contents are deemed relevant to the client request.  "
           "The default value for the partitioning threshold is 32 bytes.\n"
           "\t   Note carefully that this argument will have no affect on the "
           "partitioning of meta-data into data-bins for target files whose "
           "representation has already been cached in a file having the "
           "suffix, \".cache\".  This is done whenever a target file is first "
           "opened by an instance of the server so as to ensure the delivery "
           "of a consistent image every time the client requests it.  If you "
           "delete the cache file, the server will generate a new target ID "
           "which will prevent the client from re-using any information "
           "it recovered during previous browsing sessions.\n";
  out << "-cache <cache bytes per client>\n";
  if (comprehensive)
    out << "\tWhen serving a client, the JPEG2000 source file is managed by "
           "a persistent Kakadu codestream object.  This object loads "
           "compressed data on demand from the source file.  Data which is "
           "not in use can also be temporarily unloaded from memory, so "
           "long as the JPEG2000 code-stream contains appropriate "
           "PLT (packet length) marker segments and a packet order in which "
           "all packets of any given precinct appear contiguously.  If you "
           "are unsure whether a particular image has an appropriate "
           "structure to permit dynamic unloading and reloading of the "
           "compressed data, try opening it with \"kdu_show\" and monitoring "
           "the compressed data memory using the appropriate status mode.\n"
           "\t   Under these conditions, the system employs a FIFO "
           "(first-in first-out) cachine strategy to unload compressed "
           "data which is not in use once a cache size threshold is "
           "exceeded.  The default cache size "
           "used by this application is 2 Megabytes per client attached to "
           "the same code-stream.  You may specify an alternate per-client "
           "cache size here, which may be as low as 0.  Kakadu applications "
           "should work well even if the cache size is 0, but the server "
           "application may become disk bound if the cache size gets too "
           "small.\n";
  out << "-case_sensitive [yes|no]\n";
#ifdef KDS_ASSUME_CASELESS_FILENAMES
  if (comprehensive)
    out << "\tBy default, on this operating system, filenames are assumed to "
           "be case insensitive and converted to lower case before opening "
           "files.  This helps with efficiently sharing resources of files "
           "which are already open between multiple clients.  However, if "
           "the file system is not caseless, you can override the default "
           "policy by specifying \"-case_sensitive yes\".\n";
#else
  if (comprehensive)
    out << "\tBy default, on this operating system, filenames are assumed to "
           "be case sensitive.  However, if you know that the file system "
           "to be used is case insensitive, you can override the default "
           "policy by specifying \"-case_sensitive no\".  This may help in "
           "efficiently sharing the resources associated with open sources "
           "amongst multiple clients.\n";
#endif
  out << "-history <max history records>\n";
  if (comprehensive)
    out << "\tIndicates the maximum number of client records which are "
           "maintained in memory for serving up to the remote administrator "
           "application.  Each record contains simple statistics concerning "
           "the behaviour of a recent client connection.  The default "
           "limit is 100, but there is no harm in increasing this "
           "considerably.\n";
  out << "-establishment_timeout <seconds>\n";
  if (comprehensive)
    out << "\tSpecifies the timeout value to use for the handshaking which "
           "is used to establish connection channels.  Each time "
           "a TCP channel is accepted by the server, it allows this amount "
           "of time for the client to pass in the connection message.  In "
           "the case of the initial HTTP connection, the client must send "
           "its HTTP GET request within the timeout period.  In the case of "
           "persistent TCP session channels, the client must connect the "
           "channel to the session within the timeout period.  The reason "
           "for timing these events is to guard against malicious behaviour "
           "in denial of service attacks.  The default timeout is 10 "
           "seconds, but this might not be enough when operating over very "
           "slow links.\n";
  out << "-completion_timeout <seconds>\n";
  if (comprehensive)
    out << "\tSpecifies the time allowed for completion of ongoing "
           "communication within a channel before the channel is closed "
           "down.  This timeout applies to the sending of left-over data "
           "on channels which are being closed down.  The default timeout "
           "value is 3 seconds.\n";
  out << "-connection_threads <max threads for managing new connections>\n";
  if (comprehensive)
    out << "\tSpecifies the maximum number of threads which can be "
           "dedicated to managing the establishment of new connections.  "
           "The new connections are handed off to dedicated per-client "
           "threads as soon as possible, but connection threads are "
           "responsible for initially opening files and managing all "
           "tasks associated with delegating services to other servers.  By "
           "allowing multiple connection requests to be processed "
           "simultaneously, the performance of the server need not be "
           "compromised by clients with slow channels.  The default "
           "maximum number of connection management threads is 5.\n";
  out << "-restrict -- restrict access to images in the working directory\n";
  out << "-record -- print all HTTP requests and replies to stdout.\n";
  out << "-version -- print core system version I was compiled against.\n";
  out << "-v -- abbreviation of `-version'\n";
  out << "-usage -- print a comprehensive usage statement.\n";
  out << "-u -- print a brief usage statement.\"\n\n";
  out.flush(true);
  exit(0);
}

/*****************************************************************************/
/* STATIC                       parse_arguments                              */
/*****************************************************************************/

static char *
  parse_arguments(kdu_args &args, kdcs_sockaddr &listen_address,
                  kdu_uint16 &aux_listen_port, int &max_clients,
                  int &max_sources, int &max_cids_per_client,
                  kd_delegator &delegator,
                  float &max_session_idle_seconds,
                  float &max_completion_seconds,
                  float &max_rtt_target, float &min_rtt_target,
                  float &max_bytes_per_second,
                  int &max_chunk_size, bool &ignore_relevance_info,
                  int &phld_threshold, int &per_client_cache,
                  int &max_history_records,
                  float &max_establishment_seconds,
                  int &num_connection_threads,
                  bool &restrict_access, char *&cache_directory,
                  bool &caseless_targets)
{
  int listen_port = aux_listen_port = 80;
  listen_address.reset();
  max_clients = max_sources = 0; // Default policy established later.
  max_cids_per_client = 2;
  max_rtt_target = 2.0F;
  min_rtt_target = 0.5F;
  max_bytes_per_second = 10000.0F;
  max_session_idle_seconds = 300.0F;
  max_chunk_size = 1024;
  ignore_relevance_info = false;
  phld_threshold = 32;
  per_client_cache = 2000000; // 2 Megabytes per client.
  max_history_records = 100;
  max_completion_seconds = 3.0F;
  max_establishment_seconds = 10.0F;
  num_connection_threads = 5;
  restrict_access = false;
  cache_directory = NULL;
  char *passwd = NULL;
#ifdef KDS_ASSUME_CASELESS_FILENAMES
  caseless_targets = true;
#else
  caseless_targets = false;
#endif

  if (args.find("-u") != NULL)
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if ((args.find("-version") != NULL) || (args.find("-v") != NULL))
    print_version();

  if(args.find("-wd") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "Missing or empty working directory supplied "
          "with the \"-wd\" argument."; }
      if (kdcs_chdir(string) != 0)
        { kdu_error e; e << "Unable to change working directory to \""
          << string << "\"."; }
      args.advance();
    }

  if (args.find("-cd") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "Missing or empty cache directory supplied "
          "with the \"-cd\" argument."; }
      size_t len = strlen(string);
      cache_directory = new char[len+1];
      strcpy(cache_directory,string);
      args.advance();
      if ((cache_directory[len-1] == '/') || (cache_directory[len-1] == '\\'))
        cache_directory[len-1] = '\0';
    }

  if (args.find("-passwd") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "Missing or empty password string supplied "
          "with the \"-passwd\" argument."; }
      if (passwd != NULL)
        delete[] passwd;
      passwd = new char[strlen(string)+1];
      strcpy(passwd,string);
      args.advance();
    }
  if (args.find("-address") != NULL)
    { 
      const char *string = args.advance();
      if ((string != NULL) && (strcmp(string,"localhost") == 0))
        string = "127.0.0.1";
      if (!listen_address.init(string,0))
        { kdu_error e; e << "Missing or illegal IP address supplied with "
          "the \"-address\" argument."; }
      args.advance();
    }
  if (args.find("-port") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&listen_port) != 1) ||
          (listen_port <= 0) || (listen_port >= (1<<16)))
        { kdu_error e; e << "\"-port\" argument requires a positive 16-bit "
          "value for the port number on which to listen for connections."; }
      args.advance();
    }
  if (args.find("-clients") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_clients) != 1) ||
          (max_clients <= 0))
        { kdu_error e; e << "\"-clients\" argument requires a positive "
          "value for the maximum number of supported clients."; }
      args.advance();
    }
  if (args.find("-sources") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&max_sources) != 1) ||
          (max_sources <= 0))
        { kdu_error e; e << "\"-sources\" argument requires a positive "
          "value for the maximum number of simultaneously open sources."; }
      args.advance();
    }
  if (args.find("-cids") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&max_cids_per_client) != 1) ||
          (max_cids_per_client <= 0))
        { kdu_error e; e << "\"-cids\" argument requires a positive "
          "value for the maximum number of JPIP channel-ID's which can "
          "be assigned in each stateful session."; }
      args.advance();
    }
  if (args.find("-max_rtt") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%f",&max_rtt_target) != 1) ||
          (max_rtt_target <= 0.0F))
        { kdu_error e; e << "\"-max_rtt\" argument requires a positive "
          "maximum value for the target datagram RTT."; }
      args.advance();
    }
  if (args.find("-min_rtt") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%f",&min_rtt_target) != 1) ||
          (min_rtt_target < 0.0F) || (min_rtt_target > max_rtt_target))
        { kdu_error e; e << "\"-min_rtt\" argument requires a non-negative "
          "minimum value for the target datagram RTT.  The value may not "
          "exceed the specified (or default) maximum target RTT value of "
          << max_rtt_target << " seconds."; }
      args.advance();
    }
  if (args.find("-max_rate") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%f",&max_bytes_per_second) != 1) ||
          (max_bytes_per_second <= 0.0F))
        { kdu_error e; e << "\"-max_rate\" argument requires a positive "
          "maximum transmission rate (bytes/second) for JPEG2000 packet "
          "data."; }
      args.advance();
    }
  if (args.find("-max_idle_time") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%f",&max_session_idle_seconds) != 1) ||
          (max_session_idle_seconds <= 0.0F))
        { kdu_error e; e << "\"-max_idle_time\" argument requires a "
          "positive time limit (seconds) for client connections!"; }
      args.advance();
    }
  if (args.find("-max_chunk") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&max_chunk_size) != 1) ||
          (max_chunk_size < 128) || (max_chunk_size >= (1<<16)))
        { kdu_error e; e << "The `-max_chunk' argument requires an integer "
          "parameter, no smaller than 128 and strictly smaller than 2^16."; }
      args.advance();
    }
  if (args.find("-ignore_relevance") != NULL)
    {
      ignore_relevance_info = true;
      args.advance();
    }
  if (args.find("-phld_threshold") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&phld_threshold) != 1) ||
          (phld_threshold <= 0))
        { kdu_error e; e << "The `-phld_threshold' argument requires a "
          "positive integer parameter."; }
      args.advance();
    }
  if (args.find("-cache") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&per_client_cache) != 1) ||
          (per_client_cache < 0))
        { kdu_error e; e << "The `-cache' argument requires a non-negative "
          "integer parameter."; }
      args.advance();
    }
  if (args.find("-history") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&max_history_records) != 1) ||
          (max_history_records < 0))
        { kdu_error e; e << "The `-history' argument requires a non-negative "
          "integer parameter."; }
      args.advance();
    }
  if (args.find("-completion_timeout") != NULL)
    {
      const char *string = args.advance();
      float fval;
      if ((string == NULL) || (sscanf(string,"%f",&fval)!=1) || (fval <= 0.0F))
        { kdu_error e; e << "The `-completion_timeout' argument requires a "
          "positive number of seconds."; }
      args.advance();
      max_completion_seconds = fval;
    }
  if (args.find("-establishment_timeout") != NULL)
    {
      const char *string = args.advance();
      float fval;
      if ((string == NULL) || (sscanf(string,"%f",&fval)!=1) || (fval <= 0.0F))
        { kdu_error e; e << "The `-establishment_timeout' argument requires a "
        "positive number of seconds."; }
      args.advance();
      max_establishment_seconds = fval;
    }
  if (args.find("-connection_threads") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          (sscanf(string,"%d",&num_connection_threads) != 1) ||
          (num_connection_threads < 1))
        { kdu_error e; e << "The `-connection_threads' argument requires a "
          "positive integer parameter."; }
      args.advance();
    }
  if (args.find("-restrict") != NULL)
    {
      restrict_access = true;
      args.advance();
    }
  if (args.find("-record") != NULL)
    {
      kd_msg_log.init(&kd_msg);
      args.advance();
    }
  while (args.find("-delegate") != NULL)
    {
      const char *string = args.advance();
      if (string == NULL)
        { kdu_error e; e << "The `-delegate' argument requires a string "
          "parameter."; }
      delegator.add_host(string);
      args.advance();
    }
  if (args.find("-case_sensitive") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) ||
          ((strcmp(string,"yes") != 0) && (strcmp(string,"no") != 0)))
        { kdu_error e; e << "The `-case_sensitive' argument requires a "
          "single string parameter, equal to either \"yes\" or \"no\"."; }
      caseless_targets = (strcmp(string,"no") == 0);
      args.advance();
    }

  // Implement the default policy for `max_clients' and `max_sources'
  if (max_clients == 0)
    max_clients = 2;
  if (max_clients < max_sources)
    max_clients = max_sources;
  if (max_sources == 0)
    max_sources = max_clients;
  
  // Now configure the `listen_address'
  if (!listen_address.is_valid())
    { // Explicit IP address not supplied on command-line
      if (!listen_address.init(NULL,0))
        { kdu_error e;
          e << "Unable to obtain local host address for server to use!"; }        
    }
  listen_address.set_port((kdu_uint16) listen_port);
  aux_listen_port = (kdu_uint16) listen_port;
  
  // Adjust the maximum number of clients, if necessary, to ensure that
  // we never need more sockets than `FD_SETSIZE'
  int safe_max_clients = FD_SETSIZE-2*num_connection_threads-1;
     // Allows one socket for each source thread.  In practice, we may
     // need more than that if multiple JPIP channel-ID's are assigned, or
     // if HTTP-TCP communication is involved.
  if (safe_max_clients < 1)
    { kdu_error e; e << "The current value of FD_SETSIZE, defined in "
        "\"kdcs_comms.h\" is too small to safely accommodate even one "
        "client, given the `-connection_threads' value.  You probably only "
        "need a few connection threads.  If necessary, increase the value "
        "of FD_SETSIZE and recompile the server application.";
    }
  if (max_clients > safe_max_clients)
    {
      max_clients = safe_max_clients;
      kdu_warning w; w << "The maximum number of clients has been reduced "
        "to " << safe_max_clients << ", since this is the maximum number "
        "of clients which can be allocated sockets, in the simplest "
        "HTTP-only transport mode.  The actual number of clients might "
        "be limited further in practice if some of them use richer "
        "transport modes, involving more sockets.  If you really need a "
        "larger number of clients, you should consider increasing the value "
        "of the FD_SETSIZE macro in \"kdcs_comms.h\".";
    }
  
  return passwd;
}


/* ========================================================================= */
/*                               kd_message_log                              */
/* ========================================================================= */

/*****************************************************************************/
/*                            kd_message_log::print                          */
/*****************************************************************************/

void
  kd_message_log::print(const char *buffer, int num_chars, const char *prefix,
                        bool leave_blank_line)
{
  if (output == NULL)
    return;
  output->start_message();
  if (leave_blank_line)
    output->put_text("\n");
  while (num_chars > 0)
    {
      int xfer_len = 0;
      for (xfer_len=0; xfer_len < num_chars; xfer_len++)
        if (buffer[xfer_len] == '\n')
          break;
      if (buf_len < (xfer_len+2))
        { // Need to reallocate buffer.
          if (buf != NULL)
            delete[] buf;
          buf_len = xfer_len*2 + 20;
          buf = new char[buf_len];
        }
      memcpy(buf,buffer,(size_t) xfer_len);
      buf[xfer_len++] = '\n';
      buf[xfer_len] = '\0';
      (*output) << prefix << buf;
      buffer += xfer_len;
      num_chars -= xfer_len;
    }
  output->flush(true);
}


/* ========================================================================= */
/*                             External Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* EXTERN                            main                                    */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  // Prepare console I/O and argument parsing.
  kdu_customize_warnings(&kd_msg);
  kdu_customize_errors(&kd_msg_with_exceptions);
  kdu_args args(argc,argv,"-s");

  FILE *log_file = NULL;
  if (args.find("-log") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "Missing or empty file name supplied with "
          "the \"-log\" argument."; }
      log_file = fopen(string,"w");
      if (log_file == NULL)
        { kdu_error e;
          e << "Unable to open supplied log file, \"" << string << "\"."; }
      sys_msg.redirect(log_file);
      sys_msg_with_exceptions.redirect(log_file);
      args.advance();
    }

  kd_delegator delegator;
  kdcs_channel_monitor monitor;
  kd_connection_server *connection_server = new kd_connection_server;
  kd_source_manager *source_manager = NULL;
  char *passwd=NULL;
  char *cache_directory=NULL;
  
  try { // Catch otherwise uncaught exceptions.
    kdcs_sockaddr listen_address;
    kdu_uint16 aux_listen_port; // Listening port for auxiliary TCP channels
    int max_clients, max_sources, max_cids_per_client;
    float max_session_idle_seconds, max_completion_seconds;
    float max_rtt_target, min_rtt_target;
    float max_establishment_seconds, max_bytes_per_second;
    int max_chunk_bytes, phld_threshold, per_client_cache;
    int max_history_records, num_connection_threads;
    bool ignore_relevance_info, restrict_access, caseless_targets;
    passwd =
      parse_arguments(args,listen_address,aux_listen_port,
                      max_clients,max_sources,max_cids_per_client,
                      delegator,max_session_idle_seconds,
                      max_completion_seconds,max_rtt_target,min_rtt_target,
                      max_bytes_per_second,max_chunk_bytes,
                      ignore_relevance_info,phld_threshold,
                      per_client_cache,max_history_records,
                      max_establishment_seconds,num_connection_threads,
                      restrict_access,cache_directory,caseless_targets);
    if (args.show_unrecognized(kd_msg) != 0)
      { kdu_error e;
        e << "There were unrecognized command line arguments!"; }
    
    source_manager =
      new kd_source_manager(aux_listen_port,max_sources,max_clients,
                            max_cids_per_client,max_chunk_bytes,
                            ignore_relevance_info,phld_threshold,
                            per_client_cache,max_history_records,
                            max_session_idle_seconds,max_completion_seconds,
                            max_establishment_seconds,min_rtt_target,
                            max_rtt_target,passwd,cache_directory,
                            caseless_targets);

    // Start up network and channel listening service
    connection_server->start(listen_address,&monitor,&delegator,
                             source_manager,num_connection_threads,
                             max_chunk_bytes,max_bytes_per_second,
                             max_establishment_seconds,restrict_access);

    
    
    
    // Run the channel monitoring loop until `request_closure' is called
    while (monitor.run_once(5000,-1));
    
    // Cleanup
    connection_server->finish();
    if (source_manager != NULL)
      delete source_manager;
    connection_server->release();
    if (passwd != NULL)
      delete[] passwd;
    if (cache_directory != NULL)
      delete[] cache_directory;    
  }
  catch (kdu_exception) {
    if (log_file != NULL)
      { fclose(log_file); log_file = NULL; }
    return -1;
  }
    
  // Shut down services
  if (log_file != NULL)
    { fclose(log_file); log_file = NULL; }
  return 0;
}
