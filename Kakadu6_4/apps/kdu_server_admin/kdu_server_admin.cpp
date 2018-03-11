/*****************************************************************************/
// File: kdu_server_admin.cpp [scope = APPS/SERVER-ADMIN]
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
   Implements a rudimentary administration tool for use with the "kdu_server"
application.
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "kdcs_comms.h"
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_args.h"
#include "kdu_security.h"

/* ========================================================================= */
/*                         Set up messaging services                         */
/* ========================================================================= */

class kdu_stream_message : public kdu_message {
  public: // Member classes
    kdu_stream_message(FILE *dest)
      { this->dest = dest; }
    void put_text(const char *string)
      { fprintf(dest,"%s",string); }
    void flush(bool end_of_message=false)
      { fflush(dest); }
  private: // Data
    FILE *dest;
  };

static kdu_stream_message cout_message(stdout);
static kdu_stream_message cerr_message(stderr);
static kdu_message_formatter pretty_cout(&cout_message);
static kdu_message_formatter pretty_cerr(&cerr_message);


/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                         print_usage                                */
/*****************************************************************************/

static void
  print_usage(char *prog, bool comprehensive=false)
{
  kdu_message_formatter out(&cout_message);

  out << "Usage:\n  \"" << prog << " ...\n";
  out.set_master_indent(3);
  out << "-host <host name, or IP address w/ optional brackets>[:<port>]\n";
  if (comprehensive)
    out << "\tTo avoid ambiguity, IPv6 literal addresses at least should "
           "generally be enclosed within square brackets, so as to "
           "disambiguate the optional \":<port>\" suffix.  This is "
           "the same policy adopted for the representation of IP literals in "
           "URI's (see RFC 3986).  The `-host' argument is parsed in such a "
           "way, however, that square brackets can be used around any host "
           "name or IP address literal.\n";
  out << "-passwd <access password>\n";
  if (comprehensive)
    out << "\tYou must supply the same password which was supplied to the "
           "\"kdu_server\" application when it was started.  If there was "
           "no password, remote administration will not be possible.\n";
  out << "-proxy <proxy name, or IP addressk w/ optional brackets>[:<port>]\n";
  if (comprehensive)
    out << "\tThis argument is optional.  It may be used to specify a "
           "proxy server which should be used to communicate with the "
           "host machine indirectly.  The format for this argument is "
           "identical to that of the `-host' argument.\n";
  out << "-history <max records>\n";
  if (comprehensive)
    out << "\tRequests that the server send historical information regarding "
           "recent transactions.  It is up to the server to decide what "
           "information to supply.  The argument takes a single parameter "
           "indicating the maximum number of records which the client wishes "
           "to receive.  The server may or may not have remembered this much "
           "historical information.\n";
  out << "-shutdown\n";
  if (comprehensive)
    out << "\tSends a shutdown request to the server.\n";
  out << "-usage -- print a comprehensive usage statement.\n";
  out << "-u -- print a brief usage statement.\"\n\n";
  out.flush();
  exit(0);
}

/*****************************************************************************/
/* STATIC                       parse_host_spec                              */
/*****************************************************************************/

static char *
  parse_host_spec(const char *string, kdu_uint16 &port, const char *arg_name)
  /* Parses the a host name or IP literal from `string', stripping out
     containing brackets and the decoding any :<port> suffix, returning a
     newly allocated string with the host name/IP literal. */
{
  char *host = new char[strlen(string)+1];
  const char *port_delim = strrchr(string,':');
  if (*string == '[')
    { // Extract the host name or IP literal part
      const char *cp = strrchr(string,']');
      if (cp != NULL)
        { 
          if (port_delim < cp)
            port_delim = NULL; // No port number
          strcpy(host,string+1); // Skip over open bracket
          host[cp-(string+1)] = '\0';
          string = NULL;
        }
    }
  if (string != NULL)
    { // Still need to copy host name out of `string'
      strcpy(host,string);
      if (port_delim != NULL)
        host[port_delim-string] = '\0';
    }
  port = 80;
  if (port_delim != NULL)
    { 
      int port_val;
      if ((sscanf(port_delim+1,"%d",&port_val) == 0) ||
          (port_val < 0) || (port_val >= (1<<16)))
        { kdu_error e; e <<
          "Illegal port suffix found with host name, "
          "\"" << host << "\", supplied via `" << arg_name << "'.";
        }
      port = (kdu_uint16) port_val;
    }
  return host;
}

/*****************************************************************************/
/* STATIC                       parse_arguments                              */
/*****************************************************************************/

static void
  parse_arguments(kdu_args &args, char * &host, kdu_uint16 &host_port,
                  char * &proxy, kdu_uint16 &proxy_port,
                  char * &password, int &history_length, bool &shutdown)
{
  password = host = proxy = NULL;
  host_port = proxy_port = 80;
  history_length = 0;
  shutdown = false;

  if (args.find("-u") != NULL)
    print_usage(args.get_prog_name());
  if (args.find("-usage") != NULL)
    print_usage(args.get_prog_name(),true);
  if (args.find("-passwd") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "\"Missing or empty password string supplied "
          "with the \"-passwd\" argument."; }
      if (password != NULL)
        delete[] password;
      password = new char[strlen(string)+1];
      strcpy(password,string);
      args.advance();
    }
  if (args.find("-host") != NULL)
    {
      const char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "\"Missing or empty host string supplied "
          "with the \"-host\" argument."; }
      if (host != NULL)
        { delete[] host; host = NULL; }
      host = parse_host_spec(string,host_port,"-host");
      args.advance();
    }
  if (args.find("-proxy") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (*string == '\0'))
        { kdu_error e; e << "\"Missing or empty proxy name string supplied "
          "with the \"-proxy\" argument."; }
      if (proxy != NULL)
        { delete[] proxy; proxy = NULL; }
      proxy = parse_host_spec(string,proxy_port,"-proxy");
      args.advance();
    }
  if (args.find("-history") != NULL)
    {
      char *string = args.advance();
      if ((string == NULL) || (sscanf(string,"%d",&history_length) == 0) ||
          (history_length < 0))
        { kdu_error e; e << "The `-history' argument requires a non-negative "
          "integer parameter, indicating the maximum number of historical "
          "records which are being requested."; }
      args.advance();
    }
  if (args.find("-shutdown") != NULL)
    {
      shutdown = true;
      args.advance();
    }

  if (host == NULL)
    { kdu_error e; e << "You must supply a `-host' argument."; }
  if (proxy == NULL)
    { proxy = host; proxy_port = host_port; }
  if (password == NULL)
    { kdu_error e; e << "You must supply a `-passwd' argument."; }
}

/*****************************************************************************/
/* STATIC                       write_request                                */
/*****************************************************************************/

static void
  write_request(kdcs_message_block &block, const char *host,
                kdu_uint16 host_port, const char *query)
{
  block << "GET /jpip?" << query << " HTTP/1.1\r\n";
  if ((host[0] != '[') &&
      kdcs_sockaddr::test_ip_literal(host,KDCS_ADDR_FLAG_IPV6_ONLY))
    block << "Host: [" << host << "]";
  else
    block << "Host: " << host;
  if (host_port != 80)
    block << ":" << host_port;
  block << "\r\n";
  block << "Cache-Control: no-cache\r\n";
}

/*****************************************************************************/
/* STATIC                     parse_http_reply                               */
/*****************************************************************************/

static int
  parse_http_reply(const char *reply)
  /* Parses HTTP reply, generating error messages as appropriate and
     returning the value of the Content-Length header, if one is present. */
{
  // Parse status line
  const char *header = strchr(reply,' ');
  int status_code = 0;
  if ((header == NULL) || (sscanf(header+1,"%d",&status_code) != 1) ||
      (status_code != 200))
    { kdu_error e;
      e << "Server returned error status code " << status_code
        << ".  Complete response was:\n\n" << reply;
    }

  // Parse content-length
  int content_length = 0;
  if ((header = kdcs_caseless_search(reply,"\nContent-Length:")) != NULL)
    {
      if (*header == ' ') header++;
      sscanf(header,"%d",&content_length);
    }

  return content_length;
}

/*****************************************************************************/
/* STATIC                      read_admin_key                                */
/*****************************************************************************/

static void
  read_admin_key(const char *text, kdu_byte key[])
{
  const char *scan = (text == NULL)?"":text;
  memset(key,0,16);
  for (int i=0; (i < 32) && (*scan != '\0'); i++, scan++)
    {
      kdu_byte val;
      if ((*scan  >= '0') && (*scan <= '9'))
        val = (kdu_byte)(*scan - '0');
      else if ((*scan >= 'A') && (*scan <= 'F'))
        val = 10 + (kdu_byte)(*scan - 'A');
      else if ((*scan >= 'a') && (*scan <= 'f'))
        val = 10 + (kdu_byte)(*scan - 'a');
      else
        { kdu_error e;
          e << "Illegal admin key string received from server:\n\t\t"
            << text;
        }
      if ((i & 1) == 0)
        key[i>>1] = val << 4;
      else
        key[i>>1] |= val;
    }
}

/*****************************************************************************/
/* STATIC                   convert_key_to_string                            */
/*****************************************************************************/

static void
  convert_key_to_string(kdu_byte key[], char string[])
{
  memset(string,0,33);
  for (int i=0; i < 32; i++)
    {
      kdu_byte val = key[i>>1];
      if ((i & 1) == 0)
        val >>= 4;
      val &= 0x0F;
      if (val < 10)
        *(string++) = (char)('0'+val);
      else
        *(string++) = (char)('A'+val-10);
    }
}

/*****************************************************************************/
/*                                   main                                    */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  // Prepare console I/O and argument parsing.
  kdu_customize_warnings(&pretty_cout);
  kdu_customize_errors(&pretty_cerr);
  kdu_args args(argc,argv,"-s");

  // Collect arguments.
  char *password, *host, *proxy;
  kdu_uint16 host_port, proxy_port;
  int history_length;
  bool shutdown;
  try {
    parse_arguments(args,host,host_port,proxy,proxy_port,password,
                    history_length,shutdown);
    if (args.show_unrecognized(pretty_cout) != 0)
      { kdu_error e; e << "There were unrecognized command line arguments!"; }
  }
  catch (kdu_exception)
    { return -1; }

  // Establish connection
  kdcs_channel_monitor monitor;
  pretty_cout << "Looking up host name...\n";
  kdcs_sockaddr address;
  if (!address.init(proxy,KDCS_ADDR_FLAG_NEED_PORT))
    { kdu_error e;
      e << "Unable to resolve host address, \"" << proxy << "\"";
    }  
  address.set_port(proxy_port);
  
  // Perform the admin transactions
  try {
    kdcs_tcp_channel channel(&monitor,false); // No separate monitor thread
    kdcs_message_block block;

    pretty_cout << "Connecting to host...\n";
    channel.set_blocking_lifespan(3.0F); // 3 seconds should be plenty
    bool success = false;
    try {
      success = channel.connect(address,NULL);
    }
    catch (kdu_exception exc)
      {
        if (exc != KDU_ERROR_EXCEPTION)
          { kdu_error e; e << "Connection attempt timed out."; }
      }
    if (!success)
      { kdu_error e;
        e << "Unable to open TCP connection to host or proxy.";
      }
    else
      {
        channel.get_connected_address(address); // Get actual address
        pretty_cout << "Connection established to: "
        << address.textualize(KDCS_ADDR_FLAG_LITERAL_ONLY |
                              KDCS_ADDR_FLAG_BRACKETED_LITERALS) << ":"
        << (int) address.get_port() << "\n";
      }

    block.restart();
    write_request(block,host,host_port,"admin_key=0");
    block << "\r\n";
    pretty_cout << "Getting admin key...\n";
    channel.write_block(block);
    block.restart();
    const char *reply = channel.read_paragraph();
    int content_length = parse_http_reply(reply);
    if (content_length != 0)
      channel.read_block(content_length,block);

    kdu_rijndael security;
    security.set_key(password);

    kdu_byte admin_id[16];
    read_admin_key(block.read_line(),admin_id);
    security.encrypt(admin_id);
    char hex_string[33];
    convert_key_to_string(admin_id,hex_string);

    char query_string[256];
    sprintf(query_string,"admin_key=%s&admin_command=",hex_string);

    strcat(query_string,"stats");
    if (history_length > 0)
      sprintf(query_string+strlen(query_string),
              ",history=%d",history_length);
    if (shutdown)
      strcat(query_string,",shutdown");

    block.restart();
    write_request(block,host,host_port,query_string);
    block << "Connection: close\r\n\r\n";
    
    pretty_cout << "Issuing admin request...\n";
    channel.write_block(block);
    block.restart();
    reply = channel.read_paragraph();
    content_length = parse_http_reply(reply);
    if (content_length != 0)
      channel.read_block(content_length,block);
    pretty_cout << "\n";
    block.keep_whitespace();
    while ((reply = block.read_line()) != NULL)
      pretty_cout << reply;
    
    channel.close();
  }
  catch (kdu_exception) {
    pretty_cerr << "Network connection closed unexpectedly.\n";
  }

  // Clean up
  delete[] host;
  delete[] password;
  if (proxy != host)
    delete[] proxy;
  return 0;
}
