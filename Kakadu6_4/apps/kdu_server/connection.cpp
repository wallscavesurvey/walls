/*****************************************************************************/
// File: connection.cpp [scope = APPS/SERVER]
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
   Implements the session connection machinery, which talks HTTP with the
client in order to return the parameters needed to establish a persistent
connection -- it also brokers the persistent connection process itself
handing off the relevant sockets once connected.  This source file forms part
of the `kdu_server' application.
******************************************************************************/

#include <math.h>
#include "server_local.h"
#include "kdu_messaging.h"
#include "kdu_utils.h"

/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                  process_delegate_response                         */
/*****************************************************************************/

static int
  process_delegate_response(const char *reply, kdcs_message_block &par_block,
                            kdcs_sockaddr &address)
  /* This function essentially copies the `reply' paragraph from a delegated
     host server into the supplied `par_block', returning the value of the
     status code received from the delegated host.  However, it also
     introduces a number of modifications.
        Firstly, if the delegated host's reply did not include the
     "Connection: close" header, that header will be appended to the
     paragraph written to `par_block'.
        Secondly, if the reply paragraph includes a "JPIP-cnew:" header, the
     new channel details are scanned and the host address and port
     information are included in the list if not already in there.
        If `reply' is not a valid HTTP reply paragraph, the function returns
     error code 503, which allows the delegation function to try another
     host. */
{
  int code = 503;
  const char *end, *cp;
  const char *host_address = address.textualize(KDCS_ADDR_FLAG_LITERAL_ONLY);
  kdu_uint16 port = address.get_port();
  
  if (reply == NULL)
    return code;
  cp = strchr(reply,' ');
  if (cp == NULL)
    return code;
  while (*cp == ' ') cp++;
  if (sscanf(cp,"%d",&code) == 0)
    return code;

  par_block.restart();
  bool have_connection_close = false;
  while ((*reply != '\0') && (*reply != '\n'))
    {
      for (end=reply; (*end != '\n') && (*end != '\0'); end++);
      int line_length = (int)(end-reply);
      if (kdcs_has_caseless_prefix(reply,"JPIP-" JPIP_FIELD_CHANNEL_NEW ": "))
        {
          reply += strlen("JPIP-" JPIP_FIELD_CHANNEL_NEW ": ");
          while (*reply == ' ') reply++;
          par_block << "JPIP-" JPIP_FIELD_CHANNEL_NEW ": ";
          while (1)
            { // Scan through all the comma-delimited tokens
              cp = reply;
              while ((*cp != ',') && (*cp != ' ') &&
                     (*cp != '\0') && (*cp != '\n'))
                cp++;
              if (cp > reply)
                {
                  if (kdcs_has_caseless_prefix(reply,"host="))
                    host_address = NULL;
                  else if (kdcs_has_caseless_prefix(reply,"port="))
                    port = 0;
                  par_block.write_raw((kdu_byte *) reply,(int)(cp-reply));
                }
              if (*cp != ',')
                break;
              par_block << ",";
              reply = cp+1;
            }
          if (host_address != NULL)
            par_block << ",host=" << host_address;
          if (port != 0)
            par_block << ",port=" << port;
          par_block << "\r\n";
        }
      else
        {
          if (kdcs_has_caseless_prefix(reply,"Connection: "))
            {
              cp = strchr(reply,' ');
              while (*cp == ' ') cp++;
              if (kdcs_has_caseless_prefix(cp,"close"))
                have_connection_close = true;
            }
          par_block.write_raw((kdu_byte *) reply,line_length);
          par_block << "\r\n";
        }
      reply = (*end == '\0')?end:(end+1);
    }
  if (!have_connection_close)
    par_block << "Connection: close\r\n";
  par_block << "\r\n";
  return code;
}


/* ========================================================================= */
/*                              kd_request_fields                            */
/* ========================================================================= */

/*****************************************************************************/
/*                       kd_request_fields::write_query                      */
/*****************************************************************************/

void
  kd_request_fields::write_query(kdcs_message_block &block) const
{
  int n=0;

  if (target != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_TARGET "=" << target;
  if (sub_target != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_SUB_TARGET "="<<sub_target;
  if (target_id != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_TARGET_ID "="<<target_id;

  if (channel_id != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_CHANNEL_ID "="<<channel_id;
  if (channel_new != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_CHANNEL_NEW "="<<channel_new;
  if (channel_close != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_CHANNEL_CLOSE "="<<channel_close;
  if (request_id != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_REQUEST_ID "="<<request_id;

  if (full_size != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_FULL_SIZE "="<<full_size;
  if (region_size != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_REGION_SIZE "="<<region_size;
  if (region_offset != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_REGION_OFFSET "="<<region_offset;
  if (components != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_COMPONENTS "="<<components;
  if (codestreams != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_CODESTREAMS "="<<codestreams;
  if (contexts != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_CONTEXTS "="<<contexts;
  if (roi != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_ROI "="<<roi;
  if (layers != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_LAYERS "="<<layers;
  if (source_rate != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_SOURCE_RATE "="<<source_rate;

  if (meta_request != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_META_REQUEST "="<<meta_request;

  if (max_length != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_MAX_LENGTH "="<<max_length;
  if (max_quality != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_MAX_QUALITY "="<<max_quality;

  if (align != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_ALIGN "="<<align;
  if (type != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_TYPE "="<<type;
  if (delivery_rate != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_DELIVERY_RATE "="<<delivery_rate;

  if (model != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_MODEL "="<<model;
  if (tp_model != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_TP_MODEL "="<<tp_model;
  if (need != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_NEED "="<<need;

  if (capabilities != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_CAPABILITIES "="<<capabilities;
  if (preferences != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_PREFERENCES "="<<preferences;

  if (upload != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_UPLOAD "="<<upload;
  if (xpath_box != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_XPATH_BOX "="<<xpath_box;
  if (xpath_bin != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_XPATH_BIN "="<<xpath_bin;
  if (xpath_query != NULL)
    block << (((n++)==0)?"":"&")<<JPIP_FIELD_XPATH_QUERY "="<<xpath_query;

  if (unrecognized != NULL)
    block << (((n++)==0)?"":"&") << unrecognized;
}


/* ========================================================================= */
/*                                 kd_request                                */
/* ========================================================================= */

/*****************************************************************************/
/*                          kd_request::write_method                         */
/*****************************************************************************/

void
  kd_request::write_method(const char *string, int string_len)
{
  assert(string != NULL);
  resource = query = http_accept = NULL;
  set_cur_buf_len(string_len+1);
  memcpy(buf,string,string_len); buf[string_len]='\0';
  method = buf;
}

/*****************************************************************************/
/*                         kd_request::write_resource                        */
/*****************************************************************************/

void
  kd_request::write_resource(const char *string, int string_len)
{
  assert((string != NULL) && (method != NULL));
  int offset = cur_buf_len;
  set_cur_buf_len(offset+string_len+1);
  method = buf;
  memcpy(buf+offset,string,string_len); buf[offset+string_len]='\0';
  resource = buf+offset;
}

/*****************************************************************************/
/*                          kd_request::write_query                          */
/*****************************************************************************/

void
  kd_request::write_query(const char *string, int string_len)
{
  assert((string != NULL) && (method != NULL) && (resource != NULL));
  char *old_buf = buf;
  int offset = cur_buf_len;
  set_cur_buf_len(offset+string_len+1);
  method = buf;
  resource = buf + (resource-old_buf);
  if (http_accept != NULL)
    http_accept = buf + (http_accept-old_buf);
  memcpy(buf+offset,string,string_len); buf[offset+string_len]='\0';
  query = buf+offset;
}

/*****************************************************************************/
/*                       kd_request::write_http_accept                       */
/*****************************************************************************/

void
  kd_request::write_http_accept(const char *string, int string_len)
{
  assert((string != NULL) && (method != NULL) && (resource != NULL));
  char *old_buf = buf;
  int offset = cur_buf_len;
  set_cur_buf_len(offset+string_len+1);
  method = buf;
  resource = buf + (resource-old_buf);
  if (query != NULL)
    query = buf + (query-old_buf);
  memcpy(buf+offset,string,string_len); buf[offset+string_len]='\0';
  http_accept = buf+offset;
}

/*****************************************************************************/
/*                             kd_request::copy                              */
/*****************************************************************************/

void
  kd_request::copy(const kd_request *src)
{
  init();
  set_cur_buf_len(src->cur_buf_len);
  memcpy(buf,src->buf,cur_buf_len);
  if (src->method != NULL)
    method = buf + (src->method - src->buf);
  if (src->resource != NULL)
    resource = buf + (src->resource - src->buf);
  const char **src_ptr = (const char **) &(src->fields);
  const char **dst_ptr = (const char **) &fields;
  const char **lim_src_ptr = (const char **)(&(src->fields)+1);
  for (; src_ptr < lim_src_ptr; src_ptr++, dst_ptr++)
    { // Walk through all the request fields
      if (*src_ptr != NULL)
        {
          assert((*src_ptr >= src->buf) &&
                 (*src_ptr < (src->buf+src->cur_buf_len)));
          *dst_ptr = buf + (*src_ptr - src->buf);
        }
      else
        *dst_ptr = NULL;
    }
  close_connection = src->close_connection;
  preemptive = src->preemptive;
}


/* ========================================================================= */
/*                              kd_request_queue                             */
/* ========================================================================= */

/*****************************************************************************/
/*                     kd_request_queue::~kd_request_queue                   */
/*****************************************************************************/

kd_request_queue::~kd_request_queue()
{
  while ((tail=head) != NULL)
    {
      head=tail->next;
      delete tail;
    }
  while ((tail=free_list) != NULL)
    {
      free_list=tail->next;
      delete tail;
    }
  if (pending != NULL)
    delete pending;
}

/*****************************************************************************/
/*                           kd_request_queue::init                          */
/*****************************************************************************/

void
  kd_request_queue::init()
{
  while ((tail=head) != NULL)
    {
      head = tail->next;
      return_request(tail);
    }
  if (pending != NULL)
    return_request(pending);
  pending = NULL;
  pending_body_bytes = 0;
}

/*****************************************************************************/
/*                       kd_request_queue::read_request                      */
/*****************************************************************************/

bool
  kd_request_queue::read_request(kdcs_tcp_channel *channel)
{
  const char *cp, *text;
  
  if (pending_body_bytes == 0)
    {
      assert(pending == NULL);
      do {
          if ((text = channel->read_paragraph()) == NULL)
            return false;
          kd_msg_log.print(text,"<< ");
        } while (*text == '\n'); // Skip over blank lines

      pending = get_empty_request();

      if (kdcs_has_caseless_prefix(text,"GET "))
        {
          pending->write_method(text,3);
          text += 4;
        }
      else if (kdcs_has_caseless_prefix(text,"POST "))
        {
          pending->write_method(text,4);
          text += 5;
          if ((cp = kdcs_caseless_search(text,"\nContent-length:")) != NULL)
            {
              while (*cp == ' ') cp++;
              sscanf(cp,"%d",&pending_body_bytes);
              if ((cp = kdcs_caseless_search(text,"\nContent-type:")) != NULL)
                {
                  while (*cp == ' ') cp++;
                  if (!(kdcs_has_caseless_prefix(cp,
                                      "application/x-www-form-urlencoded") ||
                        kdcs_has_caseless_prefix(cp,"x-www-form-urlencoded")))
                    cp = NULL;
                }
            }
          if ((cp == NULL) || (pending_body_bytes < 0) ||
              (pending_body_bytes > 32000))
            { // Protocol not being followed; ignore query and kill connection
              pending_body_bytes = 0;
              pending->close_connection = true;        
            }
        }
      else
        { // Copy the method string only
          for (cp=text; (*cp != ' ') && (*cp != '\n') && (*cp != '\0'); cp++);
          pending->write_method(text,(int)(cp-text));
          pending->close_connection = true;
          complete_pending_request();
          return true; // Return true, even though it is an invalid request
        }

      // Find start of the resource string
      while (*text == ' ') text++;
      if (*text == '/')
        text++;
      else
        for (; (*text != ' ') && (*text != '\0') && (*text != '\n'); text++)
          if ((text[0] == '/') && (text[1] == '/'))
            { text += 2; break; }

      // Find end of the resource string
      cp = text;
      while ((*cp != '?') && (*cp != '\0') && (*cp != ' ') && (*cp != '\n'))
        cp++;
      pending->write_resource(text,(int)(cp-text));
      text = cp;

      // Look for query string
      if (pending_body_bytes == 0)
        {
          if (*text == '?')
            {
              text++; cp = text;
              while ((*cp != ' ') && (*cp != '\n') && (*cp != '\0'))
                cp++;
            }
          pending->write_query(text,(int)(cp-text));
          text = cp;
        }
      
      if (!pending->close_connection)
        { // See if we need to close the connection
          while (*text == ' ') text++;
          float version;
          if (kdcs_has_caseless_prefix(text,"HTTP/") &&
              (sscanf(text+5,"%f",&version) == 1) && (version < 1.1F))
            pending->close_connection = true;
          if ((cp=kdcs_caseless_search(text,"\nConnection:")) != NULL)
            {
              while (*cp == ' ') cp++;
              if (kdcs_has_caseless_prefix(cp,"CLOSE"))
                pending->close_connection = true;
            }
        }

      if ((cp=kdcs_caseless_search(text,"\nAccept:")) != NULL)
        {
          while (*cp == ' ') cp++;
          for (text=cp; (*cp != ' ') && (*cp != '\n') && (*cp != '\0'); cp++);
          pending->write_http_accept(text,(int)(cp-text));
        }
    }

  if (pending_body_bytes > 0)
    {
      int n;
      kdu_byte *body = channel->read_raw(pending_body_bytes);
      if (body == NULL)
        return false;

      text = (const char *) body;
      for (cp=text, n=0; n < pending_body_bytes; n++, cp++)
        if ((*cp == ' ') || (*cp == '\n') || (*cp == '\0'))
          break;
      pending->write_query(text,n);
      kd_msg_log.print(text,n,"<< ");
    }

  complete_pending_request();
  return true;
}

/*****************************************************************************/
/*                         kd_request_queue::push_copy                       */
/*****************************************************************************/

void
  kd_request_queue::push_copy(const kd_request *src)
{
  kd_request *req = get_empty_request();
  req->copy(src);
  if (tail == NULL)
    head = tail = req;
  else
    tail = tail->next = req;
}

/*****************************************************************************/
/*                      kd_request_queue::transfer_state                     */
/*****************************************************************************/

void
  kd_request_queue::transfer_state(kd_request_queue *src)
{
  assert(this->pending == NULL);
  if (src->head != NULL)
    {
      if (this->tail == NULL)
        this->head = src->head;
      else
        this->tail->next = src->head;
      if (src->tail != NULL)
        this->tail = src->tail;
      src->head = src->tail = NULL;
    }
  if (src->pending != NULL)
    {
      this->pending = src->pending;
      this->pending_body_bytes = src->pending_body_bytes;
      src->pending = NULL;
      src->pending_body_bytes = 0;
    }
}

/*****************************************************************************/
/*                 kd_request_queue::have_preempting_request                 */
/*****************************************************************************/

bool kd_request_queue::have_preempting_request(const char *channel_id)
{
  kd_request *req;
  for (req=head; req != NULL; req=req->next)
    if ((req->fields.channel_id != NULL) &&
        (strcmp(req->fields.channel_id,channel_id) == 0) &&
        (req->fields.channel_new == NULL) && req->preemptive)
      return true;
  return false;
}

/*****************************************************************************/
/*                         kd_request_queue::pop_head                        */
/*****************************************************************************/

const kd_request *
  kd_request_queue::pop_head()
{
  kd_request *req = head;
  if (req != NULL)
    {
      if ((head = req->next) == NULL)
        tail = NULL;
      return_request(req);
    }
  return req;
}

/*****************************************************************************/
/*                kd_request_queue::complete_pending_request                 */
/*****************************************************************************/

void
  kd_request_queue::complete_pending_request()
{
  assert(pending != NULL);
  if (tail == NULL)
    head = tail = pending;
  else
    tail = tail->next = pending;
  pending = NULL;
  pending_body_bytes = 0;

  if (tail->resource != NULL)
    kdu_hex_hex_decode((char *) tail->resource);
  else
    return;

  tail->preemptive = true; // Until proven otherwise
  const char *field_sep, *scan;
  for (field_sep=NULL, scan=tail->query; scan != NULL; scan=field_sep)
    {
      if (scan == field_sep)
        scan++;
      if (*scan == '\0')
        break;
      for (field_sep=scan; *field_sep != '\0'; field_sep++)
        if (*field_sep == '&')
          break;
      if (*field_sep == '\0')
        field_sep = NULL;
      kdu_hex_hex_decode((char *)scan,field_sep); // Decodes the request field
                       // and inserts null-terminator at or before `field_sep'.
      if (kdcs_parse_request_field(scan,JPIP_FIELD_WAIT))
        {
          if (strcmp(scan,"yes")==0)
            tail->preemptive = false;
        }
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_TARGET))
        tail->fields.target = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_SUB_TARGET))
        tail->fields.sub_target = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_TARGET_ID))
        tail->fields.target_id = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_CHANNEL_ID))
        tail->fields.channel_id = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_CHANNEL_NEW))
        tail->fields.channel_new = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_CHANNEL_CLOSE))
        tail->fields.channel_close = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_REQUEST_ID))
        tail->fields.request_id = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_FULL_SIZE))
        tail->fields.full_size = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_REGION_SIZE))
        tail->fields.region_size = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_REGION_OFFSET))
        tail->fields.region_offset = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_COMPONENTS))
        tail->fields.components = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_CODESTREAMS))
        tail->fields.codestreams = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_CONTEXTS))
        tail->fields.contexts = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_ROI))
        tail->fields.roi = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_LAYERS))
        tail->fields.layers = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_SOURCE_RATE))
        tail->fields.source_rate = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_META_REQUEST))
        tail->fields.meta_request = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_MAX_LENGTH))
        tail->fields.max_length = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_MAX_QUALITY))
        tail->fields.max_quality = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_ALIGN))
        tail->fields.align = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_TYPE))
        tail->fields.type = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_DELIVERY_RATE))
        tail->fields.delivery_rate = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_MODEL))
        tail->fields.model = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_TP_MODEL))
        tail->fields.tp_model = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_NEED))
        tail->fields.need = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_CAPABILITIES))
        tail->fields.capabilities = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_PREFERENCES))
        tail->fields.preferences = scan;

      else if (kdcs_parse_request_field(scan,JPIP_FIELD_UPLOAD))
        tail->fields.upload = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_XPATH_BOX))
        tail->fields.xpath_box = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_XPATH_BIN))
        tail->fields.xpath_bin = scan;
      else if (kdcs_parse_request_field(scan,JPIP_FIELD_XPATH_QUERY))
        tail->fields.xpath_query = scan;

      else if (kdcs_parse_request_field(scan,"admin_key"))
        tail->fields.admin_key = scan;
      else if (kdcs_parse_request_field(scan,"admin_command"))
        tail->fields.admin_command = scan;
      else
        tail->fields.unrecognized = scan;
    }

  if (tail->fields.target == NULL)
    tail->fields.target = tail->resource;

  if ((tail->fields.type == NULL) && (tail->http_accept != NULL))
    { // Parse acceptable types from the HTTP "Accept:" header
      tail->fields.type = tail->http_accept;
      const char *sp=tail->http_accept;
      char *cp = (char *)(tail->fields.type);
      for (; *sp != '\0'; sp++)
        { // It is sufficient to remove spaces, since "type" request bodies
          // have otherwise the same syntax as the "Accept:" header.
          if ((*sp != ' ') && (*sp != '\r') && (*sp != '\n') && (*sp != '\t'))
            *(cp++) = *sp;
        }
      *cp = '\0';
    }
}


/* ========================================================================= */
/*                              kds_jpip_channel                             */
/* ========================================================================= */

/*****************************************************************************/
/*                     kds_jpip_channel::kds_jpip_channel                    */
/*****************************************************************************/

kds_jpip_channel::kds_jpip_channel(kdcs_channel_monitor *monitor,
                                   kdcs_tcp_channel *channel,
                                   int max_chunk_bytes,
                                   float max_bytes_per_second)
{
  this->monitor = monitor;
  this->channel = channel;
  channel->set_channel_servicer(this);
  this->max_chunk_bytes = max_chunk_bytes;
  this->original_max_bytes_per_second = max_bytes_per_second;
  this->max_bytes_per_second = max_bytes_per_second;
  free_list = holding_head = holding_tail = NULL;
  completed_head = completed_tail = NULL;
  transmitted_head = transmitted_tail = NULL;
  num_unprocessed_eor_acks = 0;
  holding_bytes = window_bytes = 0;
  window_threshold = 1; // Wait for round trip before growing window
  
  need_chunk_trailer = false;
  waiting_to_transmit = false;
  is_auxiliary = false;
  no_more_data_expected = false;
  
  mutex.create();
  internal_event.create(false); // Auto-reset event might be a bit faster
  request_ready_event = queue_ready_event = idle_ready_event = NULL;
  monitor->synchronize_timing(timer);
  start_time = cur_time = timer.get_ellapsed_microseconds();
  delivery_gate = start_time; // Can delivery data immediately if desired
  timeout_gate = -1; // No scheduled timeout yet
  bunching_usecs = 1000000; // One second
  delivery_wakeup_scheduled = timeout_wakeup_scheduled = false;
  estimated_network_rate = max_bytes_per_second;
  float initial_epoch_bytes = (float)(KDCS_TARGET_SERVICE_EPOCH_SECONDS *
                                      max_bytes_per_second);
  if (initial_epoch_bytes < 1000.0F)
    suggested_epoch_bytes = 1000;
  else if (initial_epoch_bytes > 16000.0F)
    suggested_epoch_bytes = 16000;
  else
    suggested_epoch_bytes = (int) initial_epoch_bytes;
  
  avg_ht = avg_hb = 0.0;
  
  max_rtt_target = 1.0F; min_rtt_target = 0.0F; rtt_target = 0.5F;
  avg_1 = avg_t = avg_b = avg_bb = avg_tb = 0.0;
  estimated_network_delay = 0.5F; // Half a second until we know better
  nominal_window_threshold = 1;
  window_jitter = 0;
  
  total_transmitted_bytes = 0;
  total_acknowledged_bytes = 0;
  idle_start = -1;
  idle_usecs = 0;
  total_rtt_usecs = total_rtt_events = 0;
  
  next = NULL;

  if (!(mutex.exists() && internal_event.exists()))
    close_channel(); // In case we are all out of synchronization resources
}

/*****************************************************************************/
/*                   kds_jpip_channel::set_bandwidth_limit                   */
/*****************************************************************************/

void kds_jpip_channel::set_bandwidth_limit(float bw_limit)
{
  if ((bw_limit <= 0.0F) || (bw_limit > original_max_bytes_per_second))
    bw_limit = original_max_bytes_per_second;
  mutex.lock();
  this->max_bytes_per_second = bw_limit;
  float epoch_limit = (float)(KDCS_TARGET_SERVICE_EPOCH_SECONDS * bw_limit);
  if (epoch_limit < (float) suggested_epoch_bytes)
    suggested_epoch_bytes = (int) epoch_limit;
  mutex.unlock();
}

/*****************************************************************************/
/*                       kds_jpip_channel::set_timeout                       */
/*****************************************************************************/

void kds_jpip_channel::set_timeout(float seconds)
{
  mutex.lock();
  cur_time = timer.get_ellapsed_microseconds();
  timeout_gate = cur_time + (kdu_long) ceil(seconds * 1000000);
  if  (seconds < 0.0F)
    timeout_gate = -1; // Disable timeouts
  if (timeout_gate >= 0)
    {
      if ((timeout_gate <= cur_time) || (channel == NULL))
        wake_all_blocked_calls();
      else if ((!delivery_wakeup_scheduled) || (timeout_gate < delivery_gate))
        {
          delivery_wakeup_scheduled = false;
          timeout_wakeup_scheduled = true;
          channel->schedule_wakeup(timeout_gate,
                                   timeout_gate+(bunching_usecs>>2));
        }
    }
  mutex.unlock();
}

/*****************************************************************************/
/*                          kds_jpip_channel::close                          */
/*****************************************************************************/

bool kds_jpip_channel::close(bool send_queued_data_first)
{ 
  mutex.lock();
  if (send_queued_data_first)
    {
      kd_queue_elt *qelt, *qnext, *qprev=NULL;
      for (qelt=holding_head; qelt != NULL; qprev=qelt, qelt=qnext)
        { // Remove any elements from the holding queue which are still
          // waiting for a chunk of data ... assume this will never arrive
          qnext = qelt->next;
          if (qelt->reply_body_bytes < 0)
            {
              assert(qelt->chunk == NULL);
              if (qelt == holding_tail)
                holding_tail = qprev;
              else
                assert(0); // Incomplete elements should really be at the tail
              if (qprev == NULL)
                holding_head = qnext;
              else
                qprev->next = qnext;
              holding_bytes -= qelt->reply_bytes;
              return_to_free_list(qelt);
            }
        }
      while ((transmitted_head != NULL) || (holding_head != NULL))
        {
          if (channel == NULL)
            { // Should not happen if above lists are already non-empty
              close_channel(); // Empty above lists
              break;
            }
          if ((timeout_gate >= 0) && (cur_time >= timeout_gate))
            {
              mutex.unlock();
              return false;
            }
          idle_ready_event = &internal_event;
          internal_event.reset();
          internal_event.wait(mutex);
        }
    }
  
  close_channel();  // Moves all unclaimed chunks to the completed list for now
  mutex.unlock();
  return true;
}

/*****************************************************************************/
/*                      kds_jpip_channel::set_auxiliary                      */
/*****************************************************************************/

void kds_jpip_channel::set_auxiliary(float min_rtt_target,
                                     float max_rtt_target)
{
  if (is_auxiliary)
    return;
  mutex.lock();
  
  // Start by waiting for the holding queue to clear, if necessary
  while (holding_head != NULL)
    {
      assert(holding_tail->reply_body_bytes >= 0); // Waiting for chunks????
      if (((timeout_gate >= 0) && (cur_time >= timeout_gate)) ||
          (channel == NULL))
        close_channel();
      else
        {
          idle_ready_event = &internal_event;
          internal_event.reset();
          internal_event.wait(mutex);
        }
    }
  requests.init(); // We won't be using the request queue
  
  // At this point, it is just possible that the channel has already expired,
  // but that doesn't stop us configuring it as an auxiliary channel.  We
  // will find out soon enough if it has closed.
  if (min_rtt_target > max_rtt_target)
    min_rtt_target = max_rtt_target;
  this->min_rtt_target = min_rtt_target;
  this->max_rtt_target = max_rtt_target;
  this->rtt_target = 0.5F*(min_rtt_target + max_rtt_target);
  estimated_network_delay = max_rtt_target;
  estimated_network_rate = max_bytes_per_second;
  if (estimated_network_rate > 5000)
    estimated_network_rate = 5000; // Don't start too fast.
  
  kdu_long max_bunching_usecs = 1 + (kdu_long)(max_rtt_target * 500000.0F);
         // Limit to half the maximum RTT target
  if (max_bunching_usecs < bunching_usecs)
    bunching_usecs = max_bunching_usecs;
  is_auxiliary = true;
  mutex.unlock();
}

/*****************************************************************************/
/*                   kds_jpip_channel::get_suggested_bytes                   */
/*****************************************************************************/

int kds_jpip_channel::get_suggested_bytes(bool blocking,
                                          kdu_event *event_to_signal)
{
  int result = 0;
  mutex.lock();
  if (channel != NULL)
    {
      queue_ready_event = NULL; // Cancel any existing `queue_ready_event'
      while (!result)
        {
          if (holding_bytes < suggested_epoch_bytes)
            result = suggested_epoch_bytes;
          else
            {
              if (((queue_ready_event=event_to_signal) != NULL) || !blocking)
                break; // Return with 0, so caller can wait on event
              if ((timeout_gate < 0) || (cur_time < timeout_gate))
                break; // Don't block on timeout
              queue_ready_event = &internal_event;
              internal_event.reset();
              internal_event.wait(mutex);
            }
        }
    }
  mutex.unlock();
  return result;
}

/*****************************************************************************/
/*                      kds_jpip_channel::push_chunks                        */
/*****************************************************************************/

kds_chunk *kds_jpip_channel::push_chunks(kds_chunk *chunks,
                                         const char *content_type,
                                         bool finished_using_channel)
{  
  kds_chunk *chnk;
  for (chnk=chunks; chnk != NULL; chnk=chnk->next)
    {
      chnk->abandoned = true; // Until proven otherwise
      assert((chnk->num_bytes < (1<<16)) &&
             (chnk->num_bytes <= max_chunk_bytes));
      if (is_auxiliary)
        {
          assert(chnk->prefix_bytes == 8);
          chnk->data[0] = (kdu_byte)(chnk->num_bytes >> 8);
          chnk->data[1] = (kdu_byte) chnk->num_bytes;
          chnk->data[2] = chnk->data[3] = chnk->data[4] =
          chnk->data[5] = chnk->data[6] = chnk->data[7] = 0;
        }
    }
  mutex.lock();
  if (channel == NULL)
    {
      mutex.unlock();
      return chunks;
    }

  bool copy_chunks = !is_auxiliary;
  for (chnk=chunks; chnk != NULL; chnk=chnk->next)
    {
      if (chnk->num_bytes <= chnk->prefix_bytes)
        { // Nothing to transmit for this chunk; should not really happen.
          chnk->abandoned = false;
          if (!copy_chunks)
            append_to_completed_list(get_new_element())->chunk = chnk;
          continue;
        }
      
      kd_queue_elt *qelt = holding_tail;
      if ((qelt == NULL) || (qelt->chunk_bytes > 0) ||
          (qelt->started_bytes != 0))
        qelt = append_to_holding_list(get_new_element());
      assert(qelt->reply_body_bytes <= 0);
           // Otherwise caller forgot to push in reply before data chunk
      if (!copy_chunks)
        {
          qelt->chunk = chnk;
          qelt->chunk_bytes = chnk->num_bytes;
        }
      else
        { // Copy chunk
          chnk->abandoned = false;
          if (qelt->chunk_copy == NULL)
            qelt->chunk_copy = new kdu_byte[max_chunk_bytes];
          qelt->chunk_bytes = chnk->num_bytes - chnk->prefix_bytes;
          if (qelt->chunk_bytes <= 0)
            qelt->chunk_bytes = 0;
          else
            memcpy(qelt->chunk_copy,chnk->data+chnk->prefix_bytes,
                   (size_t) qelt->chunk_bytes);
          qelt->chunk = NULL;
        }
      assert(qelt->chunk_bytes > 0);
      holding_bytes += qelt->chunk_bytes;
      if (qelt->reply_bytes > 0)
        {
          assert((qelt->reply_body_bytes < 0) && !is_auxiliary);
          qelt->reply.backspace(2); // Backup over paragraph ending empty-line
          qelt->reply << "Transfer-Encoding: chunked\r\n";
          if (content_type != NULL)
            qelt->reply << "Content-Type: " << content_type << "\r\n\r\n";
        }
      qelt->reply_body_bytes = 0; // Because no longer waiting for the chunk
      if (!is_auxiliary)
        {
          qelt->reply.set_hex_mode(true);
          qelt->reply << qelt->chunk_bytes << "\r\n";
          qelt->reply.set_hex_mode(false);
          int new_reply_bytes = qelt->reply.get_remaining_bytes();
          assert(new_reply_bytes >= qelt->reply_bytes);
          holding_bytes += new_reply_bytes - qelt->reply_bytes;
          qelt->reply_bytes = new_reply_bytes;
          need_chunk_trailer = true;
        }
    }
      
  if (!waiting_to_transmit)
    {
      cur_time = timer.get_ellapsed_microseconds();
      if (window_bytes >= window_threshold)
        { // Need to reduce transmission window, or we can't send more data
          assert(is_auxiliary); // Primary channels have `window_threshold'=1
          process_acknowledgements();
        }
      send_data();
    }
  if (is_auxiliary && finished_using_channel)
    this->no_more_data_expected = true;
  mutex.unlock();
  return (copy_chunks)?chunks:NULL;
}

/*****************************************************************************/
/*                 kds_jpip_channel::terminate_chunked_data                  */
/*****************************************************************************/

void kds_jpip_channel::terminate_chunked_data(kdu_byte eor_code)
{
  if (channel == NULL)
    return;
  if (is_auxiliary && (eor_code == 0))
    return;
  mutex.lock();
  kd_queue_elt *qelt = holding_tail;
  if (is_auxiliary)
    {
      if ((qelt == NULL) || (qelt->chunk_bytes != 0) ||
          (qelt->eor_chunk[9] != eor_code))
        qelt = append_to_holding_list(get_new_element());
      qelt->eor_chunk[0] = 0;  qelt->eor_chunk[1] = 11;
      qelt->eor_chunk[2] = 0;  qelt->eor_chunk[3] = 0;
      qelt->eor_chunk[4] = 0;  qelt->eor_chunk[5] = 0;
      qelt->eor_chunk[6] = 0;  qelt->eor_chunk[7] = 0;
      qelt->eor_chunk[8] = 0;  qelt->eor_chunk[9] = eor_code;
      qelt->eor_chunk[10] = 0;
      qelt->num_eor_chunks++;
      qelt->num_pending_eor_chunk_acks++;
      holding_bytes += 11;
      if (qelt->started_bytes > 0)
        qelt->started_bytes += 11; // Started transmitting the EOR messages
                                   // and there is no data chunk so we know
                                   // we just need to correct the number of
                                   // `started_bytes'.
    }
  else
    {
      assert((qelt == NULL) || (qelt->chunk_bytes > 0));
      assert(need_chunk_trailer);
      qelt = append_to_holding_list(get_new_element());
      if (eor_code != 0)
        {
          kdu_byte buf[3] = {0,eor_code,0};
          qelt->reply.set_hex_mode(true);
          qelt->reply << 3 << "\r\n";
          qelt->reply.set_hex_mode(false);
          qelt->reply.write_raw(buf,3);
        }
      qelt->reply << "0\r\n\r\n"; // Write the chunk trailer
      need_chunk_trailer = false;
      qelt->reply_bytes = qelt->reply.get_remaining_bytes();
      holding_bytes += qelt->reply_bytes;
    }
  if (!waiting_to_transmit)
    {
      cur_time = timer.get_ellapsed_microseconds();
      send_data();
    }
  mutex.unlock();
}

/*****************************************************************************/
/*                        kds_jpip_channel::push_reply                       */
/*****************************************************************************/

void kds_jpip_channel::push_reply(kdcs_message_block &block, int body_bytes)
{
  if (is_auxiliary)
    {
      kdu_error e; e << "Attempting to push reply text into an "
      "auxiliary `kds_jpip_channel' object; this is appropriate only for "
      "primary transport channels.";
    }
  if (channel == NULL)
    return;
  mutex.lock();
  try {
    kd_queue_elt *qelt = holding_tail;
    if ((qelt == NULL) || (qelt->chunk_bytes > 0) ||
        (qelt->started_bytes != 0) || (qelt->reply_body_bytes != 0))
      qelt = append_to_holding_list(get_new_element());
    qelt->reply_body_bytes = body_bytes;
    if (need_chunk_trailer)
      {
        assert(qelt->reply_bytes == 0);
        need_chunk_trailer = false;
        qelt->reply << "0\r\n\r\n"; // Write the chunk trailer
      }
    qelt->reply.append(block);
    int new_reply_bytes = qelt->reply.get_remaining_bytes();
    holding_bytes += new_reply_bytes - qelt->reply_bytes;
    qelt->reply_bytes = new_reply_bytes;
    if ((qelt->reply_body_bytes >= 0) && !waiting_to_transmit)
      {
        cur_time = timer.get_ellapsed_microseconds();
        send_data();
      }
  } catch (...) {
    mutex.unlock();
    throw;
  }
  mutex.unlock();
}

/*****************************************************************************/
/*                kds_jpip_channel::retrieve_completed_chunks                */
/*****************************************************************************/

kds_chunk *
  kds_jpip_channel::retrieve_completed_chunks()
{
  if (completed_head == NULL)
    return NULL; // Quick, unguarded check
  kds_chunk *list=NULL, *tail=NULL;
  mutex.lock();
  while (completed_head != NULL)
    {
      kd_queue_elt *qelt = completed_head;
      if (qelt->chunk != NULL)
        {
          if (tail != NULL)
            tail = tail->next = qelt->chunk;
          else
            list = tail = qelt->chunk;
          tail->next = NULL;
          tail->abandoned = false;
          qelt->chunk = NULL;
        }
      if ((completed_head = qelt->next) == NULL)
        completed_tail = NULL;
      return_to_free_list(qelt);
    }
  mutex.unlock();
  return list;
}

/*****************************************************************************/
/*                     kds_jpip_channel::retrieve_requests                   */
/*****************************************************************************/

bool kds_jpip_channel::retrieve_requests(kd_request_queue *queue,
                                         bool blocking,
                                         kdu_event *event_to_signal)
{
  mutex.lock();
  if (is_auxiliary)
    {
      mutex.unlock();
      return false;
    }
  bool got_something = false;
  request_ready_event = NULL;
  while (!got_something)
    {
      got_something = (requests.peek_head() != NULL);
      if (channel == NULL)
        break;
      try {
        while (requests.read_request(channel))
          got_something = true;
      }
      catch (kdu_exception) { // Force premature closure of the channel
        close_channel();
        break;
      }
      if (!got_something)
        { // See if we need to block or install an event
          if (((request_ready_event = event_to_signal) != NULL) || !blocking)
            break;
          if ((timeout_gate >= 0) && (timeout_gate <= cur_time))
            break; // Timeout has already expired; don't blocking
          request_ready_event = &internal_event;
          internal_event.reset();
          internal_event.wait(mutex);
        }
    }
  if (got_something)
    {
      const kd_request *req;
      while ((req=requests.pop_head()) != NULL)
        queue->push_copy(req);
    }
  mutex.unlock();
  return got_something;
}

/*****************************************************************************/
/*                     kds_jpip_channel::return_requests                     */
/*****************************************************************************/

void kds_jpip_channel::return_requests(kd_request_queue *queue)
{
  mutex.lock();
  queue->transfer_state(&requests); // Append internal requests to `queue'
  requests.transfer_state(queue); // Move entire queue to internal `requests'
  if ((requests.peek_head() != NULL) && (request_ready_event != NULL))
    {
      request_ready_event->set();
      request_ready_event = NULL;
    }
  mutex.unlock();
}

/*****************************************************************************/
/*                      kds_jpip_channel::service_channel                    */
/*****************************************************************************/

void kds_jpip_channel::service_channel(kdcs_channel_monitor *monitor,
                                       kdcs_channel *chnl, int cond_flags)
{
  assert(monitor == this->monitor);
  mutex.lock();
  if (this->channel == NULL)
    { // Channel has already been closed
      mutex.unlock();
      return;
    }
  assert(channel == chnl);
  
  cur_time = timer.get_ellapsed_microseconds();
  if (cond_flags & KDCS_CONDITION_MONITOR_CLOSING)
    close_channel();
  
  bool acknowledgements_received = false;
  if (cond_flags & KDCS_CONDITION_READ)
    { // Some reading can be done
      if (is_auxiliary)
        acknowledgements_received = process_acknowledgements();
      else
        { // Read requests
          try {
            while ((channel != NULL) &&
                   requests.read_request(channel));
          }
          catch (kdu_exception) { // Force premature closure of the channel
            close_channel();
          }
          if ((request_ready_event != NULL) &&
              ((channel == NULL) || (requests.peek_head() != NULL)))
            {
              request_ready_event->set();
              request_ready_event = NULL;
            }
        }
    }
  
  if ((timeout_gate >= 0) && (cur_time >= timeout_gate))
    {
      timeout_wakeup_scheduled = false;
      delivery_wakeup_scheduled = false;
      wake_all_blocked_calls();
    }
  if ((waiting_to_transmit &&
       (cond_flags & (KDCS_CONDITION_WRITE | KDCS_CONDITION_WAKEUP))) ||
      (acknowledgements_received && (holding_head != NULL)))
    {
      delivery_wakeup_scheduled = false;
      send_data();
    }
  
  mutex.unlock();
}

/*****************************************************************************/
/*                         kds_jpip_channel::send_data                       */
/*****************************************************************************/

void kds_jpip_channel::send_data()
{
  if (channel == NULL)
    return;
  
  waiting_to_transmit = false; // Until we need to wait
  bool window_was_empty = (transmitted_head == NULL);
  kd_queue_elt *qp = holding_head;
  if ((idle_start >= 0) && (qp != NULL))
    { // Coming out of an idle period
      idle_usecs += cur_time - idle_start;
      idle_start = -1;
    }
  for (; (qp != NULL) && (qp->reply_body_bytes >= 0) &&
         (cur_time >= delivery_gate) && (window_bytes < window_threshold);
         qp=holding_head)
    {
      bool first_attempt = (qp->started_bytes == 0);
      if (first_attempt)
        {
          qp->started_bytes = qp->reply_bytes + qp->chunk_bytes +
            qp->num_eor_chunks * 11;
          if (qp->interval_start_time >= 0)
            {
              assert(!is_auxiliary);
              qp->interval_bytes += qp->started_bytes;
            }
          if (qp->reply_bytes > 0)
            {
              assert(!is_auxiliary);
              kd_msg_log.print((char *)(qp->reply.peek_block()),
                               qp->reply.get_remaining_bytes() -
                               qp->reply_body_bytes,"\t>> ");
            }
        }
      
      if (qp->reply_bytes > 0)
        {
          try {
            if (!channel->write_block(qp->reply))
              {
                waiting_to_transmit = true;
                if (first_attempt && (qp->interval_start_time < 0))
                  { // Implement mechanism for estimating HTTP channel rate
                    qp->interval_start_time = cur_time;
                    qp->interval_bytes = qp->started_bytes;
                  }
                break;
              }
          }
          catch (kdu_exception) {
            close_channel();
            return;
          }
          qp->reply.restart();
          holding_bytes -= qp->reply_bytes;
          qp->reply_bytes = 0;
        }
      
      while (qp->num_eor_chunks > 0)
        { // Send synthesized EOR chunk
          try {
            if (!channel->write_raw(qp->eor_chunk,11))
              {
                waiting_to_transmit = true;
                break;
              }
          }
          catch (kdu_exception) {
            close_channel();
            return;
          }
          holding_bytes -= 11;
          qp->num_eor_chunks--;
        }
      if (qp->num_eor_chunks > 0)
        break;
      
      if (qp->chunk_bytes > 0)
        { // Send data chunk
          try {
            kdu_byte *chunk_data =
              (qp->chunk!=NULL)?(qp->chunk->data):(qp->chunk_copy);
            assert((chunk_data!=NULL) && (qp->chunk_bytes<=max_chunk_bytes));
            if (!channel->write_raw(chunk_data,qp->chunk_bytes))
              {
                waiting_to_transmit = true;
                if (first_attempt && (!is_auxiliary) &&
                    (qp->interval_start_time < 0))
                  { // Implement mechanism for estimating HTTP channel rate
                    qp->interval_start_time = cur_time;
                    qp->interval_bytes = qp->chunk_bytes;
                  }
                break;
              }
          }
          catch (kdu_exception) {
            close_channel();
            return;
          }
          holding_bytes -= qp->chunk_bytes;
          qp->chunk_bytes = 0;
          if (is_auxiliary)
            qp->pending_chunk_ack = true;
          else
            {
              qp->reply << "\r\n"; // Terminate chunk with CRLF
              qp->reply_bytes = qp->reply.get_remaining_bytes();
              qp->started_bytes += qp->reply_bytes;
              if (qp->interval_start_time >= 0)
                qp->interval_bytes += qp->reply_bytes;
              holding_bytes += qp->reply_bytes;
              continue; // Causes us to go back and transmit chunk terminator
            }
        }
      
      // If we get here, the entire queue element has been transmitted
      total_transmitted_bytes += qp->started_bytes;
      if ((holding_head = qp->next) == NULL)
        {
          holding_tail = NULL;
          assert(holding_bytes == 0);
        }
      
      if (num_unprocessed_eor_acks > 0)
        { // We received some acknowledgements to EOR-only chunks while we
          // were still on the holding queue.  Deal with these here.
          assert(num_unprocessed_eor_acks <= qp->num_pending_eor_chunk_acks);
          qp->num_pending_eor_chunk_acks -= num_unprocessed_eor_acks;
          num_unprocessed_eor_acks = 0;
        }
            
      if (!is_auxiliary)
        return_to_free_list(qp);
      else if ((qp->num_pending_eor_chunk_acks == 0) && !qp->pending_chunk_ack)
        { // This element's contents have already been completely acknowledged
          // through previously unprocessed EOR acks -- extremely unlikely of
          // course.
          total_acknowledged_bytes += qp->started_bytes;
          return_to_free_list(qp);
        }
      else
        {
          append_to_transmitted_list(qp);
          window_bytes += qp->started_bytes;
          qp->transmit_time = cur_time;
          qp->window_bytes = window_bytes;
          qp->interval_start_time = transmitted_head->transmit_time;
          qp->interval_bytes = window_bytes +
            transmitted_head->window_bytes - transmitted_head->started_bytes;
        }
      
      // Determine the next delivery gate
      delivery_gate += 1 +
        (kdu_long)(qp->started_bytes * (1000000.0 / max_bytes_per_second));
      if ((delivery_gate + bunching_usecs) < cur_time)
        delivery_gate = cur_time - bunching_usecs;
            
      if (!is_auxiliary)
        {
          // See if HTTP-only transport channel is idle
          if (holding_head == NULL)
            {
              idle_start = cur_time;
              if (idle_ready_event != NULL)
                {
                  idle_ready_event->set();
                  idle_ready_event = NULL;
                }
            }
              
          // Implement HTTP-only network rate estimation algorithm to
          // adjust `estimated_network_rate' and hence `suggested_epoch_bytes'
          if (qp->interval_start_time >= 0)
            {
              if (holding_head != NULL)
                {
                  holding_head->interval_start_time = qp->interval_start_time;
                  holding_head->interval_bytes = qp->interval_bytes;
                }
              if (qp->interval_bytes > qp->started_bytes)
                { // Don't start estimating rate until the element after the
                  // one in which TCP queue overflow first held us up.
                  avg_ht = 0.9*avg_ht +
                    0.0000001 * (1+cur_time-qp->interval_start_time);
                  avg_hb = 0.9*avg_hb + 0.1 * qp->interval_bytes;
                  estimated_network_rate = (float)(avg_hb / avg_ht);
                  if (estimated_network_rate > max_bytes_per_second)
                    estimated_network_rate = max_bytes_per_second;
                  suggested_epoch_bytes = (int)
                    (KDCS_TARGET_SERVICE_EPOCH_SECONDS*estimated_network_rate);
                }
            }
          else
            {
              suggested_epoch_bytes += (qp->started_bytes >> 2); // Grow slowly
              float limit = (float)(KDCS_TARGET_SERVICE_EPOCH_SECONDS *
                                    max_bytes_per_second);
              if (limit < (float) suggested_epoch_bytes)
                suggested_epoch_bytes = (int) limit;
            }
          if (suggested_epoch_bytes < 1000)
            suggested_epoch_bytes = 1000;
        }

      // See if we need to wake up a blocked call to `get_suggested_bytes'
      if ((queue_ready_event != NULL) &&
          (holding_bytes < suggested_epoch_bytes))
        {
          queue_ready_event->set();
          queue_ready_event = NULL;
        }
      
      if (window_was_empty && (transmitted_head != NULL))
        {
          window_was_empty = false;
          process_acknowledgements(); // Unlikely to read any ack info, but
             // lets the `monitor' know to look out for acknowledgements.  If
             // the function does happen to succeed, it will adjust the
             // `window_bytes' value which may allow the loop to continue.
          if (channel == NULL)
            return; // `close_channel' was called from above function
        }
    }
  
  // See if we need to install a wakeup call or register for acknowledgements
  if ((!waiting_to_transmit) && (holding_head != NULL) &&
      (delivery_gate > cur_time) &&
      ((timeout_gate < 0) || (delivery_gate <= timeout_gate)))
    {
      channel->schedule_wakeup(delivery_gate,
                               delivery_gate+(bunching_usecs>>2));
      waiting_to_transmit = true;
      delivery_wakeup_scheduled = true;
      timeout_wakeup_scheduled = false;
    }
  else
    {
      delivery_wakeup_scheduled = false;
      if ((timeout_gate >= 0) && !timeout_wakeup_scheduled)
        {
          channel->schedule_wakeup(timeout_gate,
                                   timeout_gate+(bunching_usecs>>2));
          timeout_wakeup_scheduled = true;
        }
    }
}

/*****************************************************************************/
/*                 kds_jpip_channel::process_acknowledgements                */
/*****************************************************************************/

bool kds_jpip_channel::process_acknowledgements()
{
  bool received_ack = false;
  while ((channel != NULL) &&
         ((transmitted_head != NULL) || (holding_head != NULL) ||
          !no_more_data_expected))
    {
      try {
        kdu_byte *ack = channel->read_raw(8);
        if (ack == NULL)
          break;
      }
      catch (kdu_exception) {
        close_channel();
        break;
      }
      
      // If we get here, we have received an acknowledgement message.  We
      // will not check its contents here, for maximum compatibility with
      // various clients.
      received_ack = true;
      kd_queue_elt *qp = transmitted_head;
      if (qp == NULL)
        { // Could possibly happen if a synthesized EOR chunk has been
          // acknowledged before the remaining elements in the queue element
          // have been transmitted.
          if (((qp = holding_head) != NULL) &&
              (qp->num_pending_eor_chunk_acks > num_unprocessed_eor_acks) &&
              (qp->started_bytes > 0))
            {
              num_unprocessed_eor_acks++;
              continue;
            }
          // If we get here, the client has acknowledged more chunks than were
          // sent!!!!
          close_channel();
          break;
        }
      
      assert(num_unprocessed_eor_acks == 0);
      if (qp->num_pending_eor_chunk_acks > 0)
        {
          qp->num_pending_eor_chunk_acks--;
          if ((qp->num_pending_eor_chunk_acks > 0) || qp->pending_chunk_ack)
            continue; // More acknowledgement messages required for this chunk
        }
      else
        {
          assert(qp->pending_chunk_ack);
          qp->pending_chunk_ack = false;
        }
      
      total_acknowledged_bytes += qp->started_bytes;
      window_bytes -= qp->started_bytes;
      if ((transmitted_head = qp->next) == NULL)
        {
          transmitted_tail = NULL;
          assert(window_bytes == 0);
          if (holding_head == NULL)
            { // All available data now completely transmitted
              idle_start = cur_time; // Channel is now idle
              if (idle_ready_event != NULL)
                {
                  idle_ready_event->set();
                  idle_ready_event = NULL;
                }
            }
        }
      append_to_completed_list(qp);
      total_rtt_usecs += (cur_time - qp->transmit_time);
      total_rtt_events++;
      
      // Adjust the channel rate and delay estimates and the `window_threshold'
      kdu_long interval_usecs  = (1 + cur_time - qp->interval_start_time);
      kdu_long rtt_usecs = (1 + cur_time - qp->transmit_time);
      update_windowing_parameters(interval_usecs,qp->interval_bytes,
                                  rtt_usecs,qp->window_bytes);
      
      // Recompute the `suggested_epoch_bytes' value, based on channel stats
      float response_delay = 1.5F * estimated_network_delay;
      if (response_delay < (float) KDCS_TARGET_SERVICE_EPOCH_SECONDS)
        response_delay = (float) KDCS_TARGET_SERVICE_EPOCH_SECONDS;
      if (response_delay > 5.0F)
        response_delay = 5.0F;
      float rate_limit = 2000.0F;
      if (total_rtt_events > 3)
        rate_limit *= (total_rtt_events - 2);
      if (rate_limit > max_bytes_per_second)
        rate_limit = max_bytes_per_second;
      float byte_rate = estimated_network_rate;
      if (byte_rate > rate_limit)
        byte_rate = rate_limit;
      float suggested_val = response_delay * byte_rate;
      if (suggested_val > 1000000.0F)
        suggested_epoch_bytes = 1000000;
      else if (suggested_val < 1000.0F)
        suggested_epoch_bytes = 1000;
      else
        suggested_epoch_bytes = (int) suggested_val;
      if ((queue_ready_event != NULL) &&
          (holding_bytes < suggested_epoch_bytes))
        {
          queue_ready_event->set();
          queue_ready_event = NULL;
        }
    }
  
  return received_ack;
}

/*****************************************************************************/
/*               kds_jpip_channel::update_windowing_parameters               */
/*****************************************************************************/

void kds_jpip_channel::update_windowing_parameters(kdu_long interval_usecs,
                                                   int interval_bytes,
                                                   kdu_long acknowledge_usecs,
                                                   int acknowledge_window)
{
  avg_1   = 0.9*avg_1   + 0.1;
  avg_t   = 0.9*avg_t   + 0.0000001*interval_usecs;
  avg_b   = 0.9*avg_b   + 0.1*interval_bytes;
  avg_bb  = 0.9*avg_bb  + (0.1*interval_bytes)*interval_bytes;
  avg_tb  = 0.9*avg_tb  + (0.0000001*interval_usecs)*interval_bytes;
  double determinant = avg_1*avg_bb - avg_b*avg_b;
  if ((total_rtt_events > 2) && (determinant > 0.0))
    {
      double rate_denominator = avg_tb*avg_1 - avg_b*avg_t;
      double delay_numerator = avg_t*avg_bb - avg_tb*avg_b;
      
      if ((rate_denominator < (0.0000001*determinant)) ||
          (delay_numerator < 0.0))
        { // Can't believe the rate would be more than 1 Mbyte/s, while
          // negative rates or delays are clearly meaningless.  In such cases
          // we ignore the affine model below, and simply assume that RTT=0
          estimated_network_delay = 0.0F;
          if (avg_t < (0.0000001*avg_b))
            estimated_network_rate = 1000000.0F;
          else
            estimated_network_rate = (float)(avg_b / avg_t);
        }
      else
        { // Use affine estimator.  The following equation finds the
          // rate, R, and delay, D, which minimize the expected squared error
          // between interval time, T, and (W/R + D), where W is the
          // number of interval bytes.
          estimated_network_rate = (float)(determinant / rate_denominator);
          estimated_network_delay = (float)(delay_numerator / determinant);
        }
      
      if (estimated_network_rate <= 10.0F)
        { // Can't believe the rate is less than 10 bytes/second
          estimated_network_rate = 10.0F;
          estimated_network_delay = (float)
            (avg_t - estimated_network_rate*avg_b);
          if (estimated_network_delay < 0.0F)
            estimated_network_delay = 0.0F;
        }
      rtt_target = 2.0F * estimated_network_delay;
      if (rtt_target < min_rtt_target)
        rtt_target = min_rtt_target;
      else if (rtt_target > max_rtt_target)
        rtt_target = max_rtt_target;
    }
  
  // Adapt nominal window threshold with the aim of achieving the target RTT.
  float acknowledge_rtt = 0.000001F * (float) acknowledge_usecs;
  if (acknowledge_rtt < rtt_target)
    { // Increase the transmission window to make RTT larger
      if (acknowledge_window > nominal_window_threshold)
        nominal_window_threshold = acknowledge_window + (max_chunk_bytes>>2);
    }
  else
    { // Decrease the transmission window to make RTT smaller
      int suggested_threshold = 1 +
        (int)(acknowledge_window * rtt_target / acknowledge_rtt);
      if (suggested_threshold < nominal_window_threshold)
        nominal_window_threshold = suggested_threshold;
    }
  
  // Add jitter to get the actual window threshold.
  window_threshold = nominal_window_threshold + window_jitter;
  window_jitter += max_chunk_bytes;
  if (window_jitter > (max_chunk_bytes<<1))
    window_jitter = 0;
}

/*****************************************************************************/
/*                       kds_jpip_channel::close_channel                     */
/*****************************************************************************/

void kds_jpip_channel::close_channel()
{
  if (channel != NULL)
    {
      channel->close();
      delete channel;
      channel = NULL;
    }
  while ((transmitted_tail = transmitted_head) != NULL)
    {
      transmitted_head = transmitted_tail->next;
      if (transmitted_tail->chunk != NULL)
        append_to_completed_list(transmitted_tail);
      else
        return_to_free_list(transmitted_tail);
    }
  while ((holding_tail = holding_head) != NULL)
    {
      holding_head = holding_tail->next;
      if (holding_tail->chunk != NULL)
        append_to_completed_list(holding_tail);
      else
        return_to_free_list(holding_tail);
    }
  holding_bytes = window_bytes = 0;
  wake_all_blocked_calls();
  if (idle_start < 0)
    idle_start = cur_time;
}


/* ========================================================================= */
/*                          kd_target_description                            */
/* ========================================================================= */

/*****************************************************************************/
/*                  kd_target_description::parse_byte_range                  */
/*****************************************************************************/

bool
kd_target_description::parse_byte_range()
{
  range_start = 0;
  range_lim = KDU_LONG_MAX;
  const char *cp=byte_range;
  while (*cp == ' ') cp++;
  if (*cp == '\0')
    return true;
  
  while ((*cp >= '0') && (*cp <= '9'))
    range_start = (range_start*10) + (*(cp++) - '0');
  if (*(cp++) != '-')
    { range_start=0; return false; }
  range_lim = 0;
  while ((*cp >= '0') && (*cp <= '9'))
    range_lim = (range_lim*10) + (*(cp++) - '0');
  if ((range_lim <= range_start) || ((*cp != '\0') && (*cp != ' ')))
    { range_lim = KDU_LONG_MAX; return false; }
  range_lim++; // Since the upper end of the supplied range string is inclusive
  return true;
}


/* ========================================================================= */
/*                            kd_connection_thread                           */
/* ========================================================================= */

/*****************************************************************************/
/*                        connection_thread_start_proc                       */
/*****************************************************************************/

static kdu_thread_startproc_result KDU_THREAD_STARTPROC_CALL_CONVENTION
  connection_thread_start_proc(void *param)
{
  kd_connection_thread *obj = (kd_connection_thread *) param;
  obj->thread_entry();
  return KDU_THREAD_STARTPROC_ZERO_RESULT;
}

/*****************************************************************************/
/*                kd_connection_thread::kd_connection_thread                 */
/*****************************************************************************/

kd_connection_thread::kd_connection_thread(kd_connection_server *owner,
                                           kdcs_channel_monitor *monitor,
                                           kd_delegator *delegator,
                                           kd_source_manager *source_manager,
                                           int max_chunk_bytes,
                                           float max_bytes_per_second,
                                           float max_establishment_seconds,
                                           bool restrict_access)
{
  this->owner = owner;
  this->monitor = monitor;
  this->delegator = delegator;
  this->source_manager = source_manager;
  this->max_chunk_bytes = max_chunk_bytes;
  this->max_bytes_per_second = max_bytes_per_second;
  this->max_establishment_seconds = max_establishment_seconds;
  this->restrict_access = restrict_access;
  next = NULL;
}

/*****************************************************************************/
/*                        kd_connection_thread::start                        */
/*****************************************************************************/

bool kd_connection_thread::start()
{
  return thread.create(connection_thread_start_proc,this);
}

/*****************************************************************************/
/*                     kd_connection_thread::thread_entry                    */
/*****************************************************************************/

void kd_connection_thread::thread_entry()
{
  owner->mutex.lock();
  while (!owner->finish_requested)
    {
      kds_jpip_channel *jpip_channel = owner->returned_head;
      if (jpip_channel != NULL)
        {
          if ((owner->returned_head = jpip_channel->next) == NULL)
            owner->returned_tail = NULL;
          jpip_channel->next = NULL;
        }
      else
        {
          kdcs_tcp_channel *tcp_channel = NULL;
          try {
            if (!owner->shutdown_in_progress)
              tcp_channel = owner->listener->accept(monitor,NULL,true);   
          }
          catch (kdu_exception exc) {
            if (exc == KDU_ERROR_EXCEPTION)
              { // Thrown by `kdu_error' if new connection had to
                // be dropped due to lack of channel resources.
                kdcs_microsleep(1000000);
                continue;
              }
            break; // Treat all other exceptions as fatal
          }
          if (tcp_channel == NULL)
            {
              owner->num_idle_threads++;
              if (owner->shutdown_in_progress &&
                  (owner->num_idle_threads == owner->num_active_threads))
                monitor->request_closure();
              owner->thread_event.reset();
              owner->thread_event.wait(owner->mutex);
              owner->num_idle_threads--;
              continue;
            }
          jpip_channel = new kds_jpip_channel(monitor,tcp_channel,
                                              max_chunk_bytes,
                                              max_bytes_per_second);
          tcp_channel = NULL; // Now owned by the `jpip_channel'
        }
      
      // Now we can communicate with the `jpip_channel' to manage the
      // rest of the connection process, unlocking the `owner' mutex so
      // that other threads can process other incoming connections.
      owner->mutex.unlock();
      try {
        if (process_channel(jpip_channel))
          jpip_channel = NULL;
      }
      catch (kdu_exception) {  }
      if (jpip_channel != NULL)
        {
          jpip_channel->close();
          jpip_channel->release();
        }
      owner->mutex.lock();
    }
  
  owner->num_active_threads--;
  if (owner->num_active_threads == 0)
    {
      owner->finish_event.set();
      monitor->request_closure();
    }
  owner->mutex.unlock();
}

/*****************************************************************************/
/*                    kd_connection_thread::process_channel                  */
/*****************************************************************************/

bool kd_connection_thread::process_channel(kds_jpip_channel *channel)
{
  if (owner->shutdown_in_progress)
    return false;
  channel->set_timeout(max_establishment_seconds);
  request_queue.init();
  while ((request_queue.peek_head() != NULL) ||
         channel->retrieve_requests(&request_queue))
    {
      const kd_request *req = request_queue.pop_head();
      if (req == NULL)
        continue; // Should not happen
      send_par.restart();
      if ((req->resource == NULL) &&
          kdcs_has_caseless_prefix(req->method,"JPHT"))
        { // Check if we are connecting an auxiliary channel
          send_par << "JPHT/1.0";
          
          if (source_manager->install_channel(req->method,true,channel,
                                              NULL,send_par))
            return true; // Auxiliary channel successfully off loaded
          else
            return false; // Treat as fatal error; terminate connection.  It
                          // is dangerous to attempt to push `send_par' into
                          // the channel as a reply, because the channel may
                          // well be configured for auxiliary communication
                          // already; anyway, the request was not an HTTP
                          // request so it does not deserve an HTTP reply.
        }
      else if (req->resource == NULL)
        { // Unacceptable method
          send_par << "HTTP/1.1 405 Unsupported request method: \""
                   << req->method << "\"\r\n"
                   << "Connection: close\r\n";
          send_par << "Server: Kakadu JPIP Server "
                   << KDU_CORE_VERSION << "\r\n\r\n";
          channel->push_reply(send_par,0);
          return false; // Safest to terminate connection
        }
      else if (req->resource[0] == '\0')
        { // Empty resource string
          send_par << "HTTP/1.1 400 Request does not contain a valid URI or "
                      "absolute path component (perhaps you omitted the "
                      "leading \"/\" from an absolute path)\r\n"
                   << "Connection: close\r\n";
          send_par << "Server: Kakadu JPIP Server "
                   << KDU_CORE_VERSION << "\r\n\r\n";
          channel->push_reply(send_par,0);
          return false; // Safest to terminate connection
        }
      else if (req->fields.unrecognized != NULL)
        {
          send_par << "HTTP/1.1 400 Unrecognized query field: \""
                   << req->fields.unrecognized << "\"\r\n"
                   << "Connection: close\r\n";
          send_par << "Server: Kakadu JPIP Server "
                   << KDU_CORE_VERSION << "\r\n\r\n";
          channel->push_reply(send_par,0);
          return false; // Safest to terminate connnection
        }
      else if (req->fields.upload != NULL)
        {
          send_par << "HTTP/1.1 501 Upload functionality not "
                      "implemented.\r\n"
                   << "Connection: close\r\n";
          send_par << "Server: Kakadu JPIP Server "
                   << KDU_CORE_VERSION << "\r\n\r\n";
          channel->push_reply(send_par,0);
          return false; // Safest to terminate connection
        }
      else if (((req->fields.channel_id != NULL) ||
                (req->fields.channel_close != NULL)) &&
               (strcmp(req->resource,KD_MY_RESOURCE_NAME) == 0))
        { // Client trying to connect to existing session
          const char *cid = req->fields.channel_id;
          if (cid == NULL)
            cid = req->fields.channel_close;
          send_par << "HTTP/1.1 ";
          request_queue.replace_head(req);  req = NULL;
          if (source_manager->install_channel(cid,false,channel,
                                              &request_queue,send_par))
            return true; // Channel successfully off loaded.
          else
            { // Failure: reason already written to `send_par'
              req = request_queue.pop_head();
              send_par << "\r\nConnection: close\r\n";
              send_par << "Server: Kakadu JPIP Server "
                       << KDU_CORE_VERSION << "\r\n\r\n";
              channel->push_reply(send_par,0);
              return false; // Safest to terminate connection
            }
        }
      else if ((req->fields.admin_key != NULL) &&
               (strcmp(req->fields.admin_key,"0") == 0) &&
               (strcmp(req->resource,KD_MY_RESOURCE_NAME) == 0))
        { // Client requesting delivery of an admin key
          body_data.restart();
          if (source_manager->write_admin_id(body_data))
            {
              body_data << "\r\n";
              int body_bytes = body_data.get_remaining_bytes();
              send_par << "HTTP/1.1 200 OK\r\n";
              send_par << "Cache-Control: no-cache\r\n";
              send_par << "Content-Type: application/text\r\n";
              send_par << "Content-Length: " << body_bytes << "\r\n";
              if (req->close_connection)
                send_par << "Connection: close\r\n";
              send_par << "Server: Kakadu JPIP Server "
                       << KDU_CORE_VERSION << "\r\n\r\n";
              kd_msg << "\n\t"
                        "Key generated for remote admin request.\n\n";
              send_par.append(body_data);
              channel->push_reply(send_par,body_bytes);
            }
          else
            {
              send_par.restart();
              send_par << "HTTP/1.1 403 Remote administration not enabled.\r\n"
                          "Cache-Control: no-cache\r\n"
                          "Connection: close\r\n";
              kd_msg << "\n\t"
                        "Admin key requested -- feature not enabled.\n\n";
              send_par << "Server: Kakadu JPIP Server "
                       << KDU_CORE_VERSION << "\r\n\r\n";
              channel->push_reply(send_par,0);
              return false;
            }
        }
      else if ((req->fields.admin_key != NULL) &&
               (strcmp(req->resource,KD_MY_RESOURCE_NAME) == 0))
        {
          if (source_manager->compare_admin_id(req->fields.admin_key))
            process_admin_request(req,channel);
          else
            {
              send_par << "HTTP/1.1 400 Wrong or missing admin "
                          "key as first query field in admin request.\r\n"
                          "Connection: close\r\n";
              kd_msg << "\n\tWrong or missing admin key as "
                        "first query field in admin request.\n\n";
              send_par << "Server: Kakadu JPIP Server "
                       << KDU_CORE_VERSION << "\r\n\r\n";
              channel->push_reply(send_par,0);
              return false;
            }
        }
      else if (req->fields.target != NULL)
        {
          kd_target_description target;
          strncpy(target.filename,req->fields.target,255);
          target.filename[255] = '\0';
          if (req->fields.sub_target != NULL)
            {
              strncpy(target.byte_range,req->fields.sub_target,79);
              target.byte_range[79] = '\0';
            }
          if (restrict_access &&
              ((target.filename[0] == '/') ||
               (strchr(target.filename,':') != NULL) ||
               (strstr(target.filename,"..") != NULL)))
            {
              send_par << "HTTP/1.1 403 Attempting to access "
                          "privileged location on server.\r\n"
                          "Connection: close\r\n"
                          "Server: Kakadu JPIP Server "
                       << KDU_CORE_VERSION << "\r\n\r\n";
              channel->push_reply(send_par,0);
              return false;
            }
          else if (!target.parse_byte_range())
            {
              send_par << "HTTP/1.1 400 Illegal \""
                       << JPIP_FIELD_SUB_TARGET
                       << "\" value string.\r\n"
                          "Connection: close\r\n"
                          "Server: Kakadu JPIP Server "
                       << KDU_CORE_VERSION << "\r\n\r\n";
              channel->push_reply(send_par,0);
              return false;
            }
          else if (((req->fields.channel_new == NULL) &&
                    process_stateless_request(target,req,channel)) ||
                   ((req->fields.channel_new != NULL) &&
                    process_new_session_request(target,req,channel)))
            return true; // Channel has been donated to a source thread or
                         // has already been delegated and released
        }
      else
        {
          send_par << "HTTP/1.1 405 Unsupported request\r\n"
                      "Connection: close\r\n";
          send_par << "Server: Kakadu JPIP Server "
                   << KDU_CORE_VERSION << "\r\n\r\n";
          channel->push_reply(send_par,0);
          return false;
        }
      if (req->close_connection)
        return false;
    }
  return false; // The connection was dropped unexpectedly
}

/*****************************************************************************/
/*               kd_connection_thread::process_admin_request                 */
/*****************************************************************************/

void kd_connection_thread::process_admin_request(const kd_request *req,
                                                 kds_jpip_channel *channel)
{
  const char *command = req->fields.admin_command;
  
  // Extract commands
  bool shutdown = false;
  const char *next;
  char command_buf[80];
  body_data.restart();
  for (; command != NULL; command=next)
    {
      next = command;
      while ((*next != '\0') && (*next != ','))
        next++;
      memset(command_buf,0,80);
      if ((next-command) < 80)
        strncpy(command_buf,command,next-command);
      if (*next == ',')
        next++;
      else
        next = NULL;
      if (strcmp(command_buf,"shutdown") == 0)
        shutdown = true;
      else if (strcmp(command_buf,"stats") == 0)
        source_manager->write_stats(body_data);
      else if (strncmp(command_buf,"history",strlen("history")) == 0)
        {
          int max_records = INT_MAX;
          char *cp = command_buf + strlen("history");
          if (*cp == '=')
            sscanf(cp+1,"%d",&max_records);
          if (max_records > 0)
            source_manager->write_history(body_data,max_records);
        }
    }
  int body_bytes = body_data.get_remaining_bytes();
  
  // Send reply
  send_par.restart();
  send_par << "HTTP/1.1 200 OK\r\n";
  if (req->close_connection)
    send_par << "Connection: close\r\n";
  send_par << "Cache-Control: no-cache\r\n";
  if (body_bytes > 0)
    {
      send_par << "Content-Type: application/text\r\n";
      send_par << "Content-Length: " << body_bytes << "\r\n";
    }      
  send_par << "Server: Kakadu JPIP Server "
           << KDU_CORE_VERSION << "\r\n\r\n";
  send_par.append(body_data);
  channel->push_reply(send_par,body_bytes);
  if (shutdown)
    {
      owner->shutdown_in_progress = true;
      source_manager->close_gracefully();
    }
}

/*****************************************************************************/
/*               kd_connection_thread::process_stateless_request             */
/*****************************************************************************/

bool
  kd_connection_thread::process_stateless_request(kd_target_description &tgt,
                                                  const kd_request *req,
                                                  kds_jpip_channel *channel)
{
  char channel_id[KD_CHANNEL_ID_LEN+1];
  
  send_par.restart();
  send_par << "HTTP/1.1 ";
  request_queue.replace_head(req); // Put the request back on the queue
  if (source_manager->install_stateless_channel(tgt,channel,&request_queue) ||
      (source_manager->create_source_thread(tgt,NULL,owner,
                                            channel_id,send_par) &&
       source_manager->install_channel(channel_id,false,channel,
                                       &request_queue,send_par)))
    {
      kd_msg.start_message();
      kd_msg << "\n\tStateless request accepted\n";
      kd_msg << "\t\tInternal ID: " << channel_id << "\n";
      kd_msg.flush(true);
      return true;
    }

  req = request_queue.pop_head(); // Get the request back again
  if (req->close_connection)
    send_par << "Connection: close\r\n";
  send_par << "\r\nServer: Kakadu JPIP Server "
           << KDU_CORE_VERSION << "\r\n\r\n";
  channel->push_reply(send_par,0);
  kd_msg.start_message();
  kd_msg << "\n\t\tStateless request refused\n";
  kd_msg << "\t\t\tRequested target: \"" << tgt.filename << "\"";
  if (tgt.byte_range[0] != '\0')
    kd_msg << " (" << tgt.byte_range << ")";
  kd_msg << "\n";
  kd_msg.flush(true);
  return false;
}

/*****************************************************************************/
/*             kd_connection_thread::process_new_session_request             */
/*****************************************************************************/

bool
  kd_connection_thread::process_new_session_request(kd_target_description &tgt,
                                                    const kd_request *req,
                                                    kds_jpip_channel *channel)
{
  char channel_id[KD_CHANNEL_ID_LEN+1];
  const char *delegated_host =
    delegator->delegate(req,channel,monitor,max_establishment_seconds,
                        send_par,body_data);
  channel->set_timeout(max_establishment_seconds); // Give channel a new lease
                      // of life after waiting for delegation hosts to respond
  if (delegated_host != NULL)
    {
      kd_msg.start_message();
      kd_msg << "\n\t\tDelegated session request to host, \""
             << delegated_host << "\"\n";
      kd_msg << "\t\t\tRequested target: \"" << tgt.filename << "\"";
      if (tgt.byte_range[0] != '\0')
        kd_msg << " (" << tgt.byte_range << ")";
      kd_msg << "\n";
      kd_msg << "\t\t\tRequested channel type: \""
             << req->fields.channel_new << "\"\n";
      kd_msg.flush(true);
      channel->close();
      channel->release();
      return true;
    }
  
  send_par.restart();
  send_par << "HTTP/1.1 ";
  const char *cnew = req->fields.channel_new;
  request_queue.replace_head(req); // Put request back on the queue
  if (source_manager->create_source_thread(tgt,cnew,owner,
                                           channel_id,send_par) &&
      source_manager->install_channel(channel_id,false,channel,
                                      &request_queue,send_par))
    {
      kd_msg.start_message();
      kd_msg << "\n\tNew channel request accepted locally\n";
      kd_msg << "\t\tAssigned channel ID: " << channel_id << "\n";
      kd_msg.flush(true);      
      return true;
    }
  req = request_queue.pop_head(); // Get request back again
  send_par << "Cache-Control: no-cache\r\n";
  if (req->close_connection)
    send_par << "Connection: close\r\n";
  send_par << "Server: Kakadu JPIP Server "
           << KDU_CORE_VERSION << "\r\n\r\n";
  channel->push_reply(send_par,0);
  kd_msg.start_message();
  kd_msg << "\n\t\tNew channel request refused\n";
  kd_msg << "\t\t\tRequested target: \"" << tgt.filename << "\"";
  if (tgt.byte_range[0] != '\0')
    kd_msg << " (" << tgt.byte_range << ")";
  kd_msg << "\n";
  kd_msg << "\t\t\tRequested channel type: \""
         << req->fields.channel_new << "\"\n";
  kd_msg.flush(true);
  return false;
}


/* ========================================================================= */
/*                            kd_connection_server                           */
/* ========================================================================= */

/*****************************************************************************/
/*                         kd_connection_server::start                       */
/*****************************************************************************/

void
  kd_connection_server::start(kdcs_sockaddr &listen_address,
                              kdcs_channel_monitor *monitor,
                              kd_delegator *delegator,
                              kd_source_manager *source_manager,
                              int num_threads, int max_chunk_bytes,
                              float max_bytes_per_second,
                              float max_establishment_seconds,
                              bool restrict_access)

{
  assert((num_active_threads==0) && (num_idle_threads==0) && (threads==NULL));
  assert(listener == NULL);
  if (!(mutex.exists() && thread_event.exists() && finish_event.exists()))
    { kdu_error e; e << "Failed to create critical synchronization objects.  "
      "Too many threads, perhaps??"; }
  if (num_threads < 1)
    { kdu_error e; e << "The number of connection management threads "
      "must be at least 1!!"; }
  listener = new kdcs_tcp_channel(monitor,true);
  if (!listener->listen(listen_address,5+num_threads,this))
    { kdu_error e; e << "Unable to bind socket to listening address.  You "
      "may need to explicitly specify the IP address via the command line, "
      "or you may need to explicitly supply a port number which is not "
      "currently in use.  If the default HTTP port 80 is in use, port 8080 "
      "is frequently available on many systems."; }
  listener->get_listening_address(listen_address);
  mutex.lock();
  for (; num_threads > 0; num_threads--)
    {
      kd_connection_thread *thread =
        new kd_connection_thread(this,monitor,delegator,source_manager,
                                 max_chunk_bytes,max_bytes_per_second,
                                 max_establishment_seconds,
                                 restrict_access);
      thread->next = threads;
      threads = thread;
      if (!thread->start())
        {
          mutex.unlock();
          kdu_error e; e << "Unable to start requested connection threads!";
        }
      num_active_threads++;
    }
  mutex.unlock();
  
  // Print starting details for log file.
  time_t binary_time; time(&binary_time);
  struct tm *local_time = ::localtime(&binary_time);
  kd_msg << "\nKakadu Experimental JPIP Server "
         << KDU_CORE_VERSION << " started:\n"
         << "\tTime = " << asctime(local_time)
         << "\tListening address:port = "
         << listen_address.textualize(KDCS_ADDR_FLAG_LITERAL_ONLY |
                                      KDCS_ADDR_FLAG_BRACKETED_LITERALS)
         << ":" << (int) listen_address.get_port() << "\n";
  kd_msg.flush();
}

/*****************************************************************************/
/*                         kd_connection_server::finish                      */
/*****************************************************************************/

void kd_connection_server::finish()
{
  mutex.lock();
  finish_requested = true;
  while ((returned_tail=returned_head) != NULL)
    {
      returned_head = returned_tail->next;
      returned_tail->close();
      returned_tail->release();
    }
  thread_event.set(); // Wake up all the connection threads
  while (num_active_threads > 0)
    {
      finish_event.reset();
      finish_event.wait(mutex);
    }
  mutex.unlock();
  if (listener != NULL)
    {
      listener->close();
      delete listener;
      listener = NULL;
    }
  kd_connection_thread *thrd;
  while ((thrd=threads) != NULL)
    {
      threads = thrd->next;
      delete thrd;
    }  
}

/*****************************************************************************/
/*                    kd_connection_server::return_channel                   */
/*****************************************************************************/

void kd_connection_server::return_channel(kds_jpip_channel *channel)
{
  channel->set_bandwidth_limit(-1.0F); // Removes any local limits
  mutex.lock();
  if (finish_requested || shutdown_in_progress || (num_active_threads == 0))
    {
      mutex.unlock();
      channel->close();
      channel->release();
    }
  else
    {
      channel->next = NULL;
      if (returned_tail != NULL)
        returned_tail = returned_tail->next = channel;
      else
        returned_head = returned_tail = channel;
      thread_event.set();
      mutex.unlock();
    }
}

/*****************************************************************************/
/*                    kd_connection_server::service_channel                  */
/*****************************************************************************/

void
  kd_connection_server::service_channel(kdcs_channel_monitor *monitor,
                                        kdcs_channel *channel, int cond_flags)
{
  thread_event.set(); // Wake up connection thread to service the incoming call
}


/* ========================================================================= */
/*                                kd_delegator                               */
/* ========================================================================= */

/*****************************************************************************/
/*                           kd_delegator::add_host                          */
/*****************************************************************************/

void
kd_delegator::add_host(const char *hostname)
{
  kd_delegation_host *elt = new kd_delegation_host;
  elt->hostname = new char[strlen(hostname)+3];
  strcpy(elt->hostname,hostname);
  elt->load_share = 4;
  char *suffix = strrchr(elt->hostname,'*');
  if (suffix != NULL)
    {
      if ((sscanf(suffix+1,"%d",&(elt->load_share)) != 1) ||
          (elt->load_share < 1))
        {
          delete elt;
          kdu_error e;
          e << "Invalid delegate spec, \"" << hostname << "\", given "
          "on command line.  The `*' character must be followed by a "
          "positive integer load share value.";
        }
      *suffix = '\0';
    }
  elt->port = 80;
  suffix = strrchr(elt->hostname,':');
  if ((suffix != NULL) &&
      ((elt->hostname[0] != '[') || (strchr(suffix,']') == NULL)))
    {
      int port_val = 0;
      if ((sscanf(suffix+1,"%d",&port_val) != 1) ||
          (port_val <= 0) || (port_val >= (1<<16)))
        {
          delete elt;
          kdu_error e;
          e << "Illegal port number suffix found in "
          "delegated host address, \"" << hostname << "\".";
        }
      *suffix = '\0';
      elt->port = (kdu_uint16) port_val;
    }
  if ((elt->hostname[0] != '[') &&
      kdcs_sockaddr::test_ip_literal(elt->hostname,KDCS_ADDR_FLAG_IPV6_ONLY))
    { // Add brackets around IP literal to disambiguate HTTP "Host:" headers
      int c, hostlen = (int) strlen(elt->hostname);
      for (c=hostlen; c > 0; c--)
        elt->hostname[c] = elt->hostname[c-1];
      elt->hostname[0] = '[';
      elt->hostname[hostlen+1] = ']';
      elt->hostname[hostlen+2] = '\0';
    }
  
  elt->hostname_with_port = new char[strlen(elt->hostname)+16]; // Plenty
  sprintf(elt->hostname_with_port,"%s:%d",elt->hostname,elt->port);
  mutex.lock();
  elt->next = head;
  elt->prev = NULL;
  if (head != NULL)
    head = head->prev = elt;
  else
    head = tail = elt;
  num_delegation_hosts++;
  mutex.unlock();
}

/*****************************************************************************/
/*                           kd_delegator::delegate                          */
/*****************************************************************************/

const char *
  kd_delegator::delegate(const kd_request *req,
                         kds_jpip_channel *response_channel,
                         kdcs_channel_monitor *monitor,
                         float max_establishment_seconds,
                         kdcs_message_block &par_block,
                         kdcs_message_block &data_block)
{
  assert(req->resource != NULL);

  // Find the source to use
  int num_connections_attempted = 0;
  int num_resolutions_attempted = 0;
  kd_delegation_host *host = NULL;
  while (1)
    {
      mutex.lock();
      if ((num_resolutions_attempted >= num_delegation_hosts) ||
          (num_connections_attempted > num_delegation_hosts))
        {
          mutex.unlock();
          return NULL;
        }
      for (host = head; host != NULL; host=host->next)
        if ((!host->lookup_in_progress) && (host->load_counter > 0))
          break;
      if (host == NULL)
        { // All load counters down to 0; let's increment them all
          for (host=head; host != NULL; host=host->next)
            if (!host->lookup_in_progress)
              host->load_counter = host->load_share;
          for (host=head; host != NULL; host=host->next)
            if (!host->lookup_in_progress)
              break;
        }
      if (host == NULL)
        { // Waiting for another thread to finish resolving host address
          mutex.unlock();
          kdcs_microsleep(50000); // Sleep for 50 milliseconds
          continue;
        }
      
      // If we get here, we have a host to try.  First, move it to the tail
      // of the service list so that it will be the last host which we or
      // anybody else retries, if something goes wrong.
      if (host->prev == NULL)
        {
          assert(host == head);
          head = host->next;
        }
      else
        host->prev->next = host->next;
      if (host->next == NULL)
        {
          assert(host == tail);
          tail = host->prev;
        }
      else
        host->next->prev = host->prev;
      host->prev = tail;
      if (tail == NULL)
        {
          assert(head == NULL);
          head = tail = host;
        }
      else
        tail = tail->next = host;
      host->next = NULL;
      
      // Now see if we need to resolve the address
      if (host->resolution_counter > 1)
        { // The host has failed before.  Do our best to avoid it for a while.
          host->resolution_counter--;
          mutex.unlock();
          continue;
        }
      if (host->resolution_counter == 1)
        { // Attempt to resolve host name
          host->lookup_in_progress = true;
          mutex.unlock();
          bool success = host->address.init(host->hostname,
                                            KDCS_ADDR_FLAG_BRACKETED_LITERALS);
          mutex.lock();
          if (!success)
            { // Cannot resolve host address
              num_resolutions_attempted++;
              host->resolution_counter = KD_DELEGATE_RESOLUTION_INTERVAL;
              host->lookup_in_progress = false;
              mutex.unlock();
              continue;
            }
          host->address.set_port(host->port);
          host->resolution_counter = 0;
          host->load_counter = host->load_share;
          host->lookup_in_progress = false;
        }
      
      // Now we are ready to use the host
      num_connections_attempted++;
      if (host->load_counter > 0) // May have changed in another thread
        host->load_counter--;
      
      // Copy address before releasing lock, in case it is changed elsewhere.
      kdcs_sockaddr address = host->address;
      mutex.unlock();
      
      // Now try connecting to the host address and performing the
      // relevant transactions.
      kdcs_tcp_channel host_channel(monitor,true);
      host_channel.set_blocking_lifespan(max_establishment_seconds);
      try {
        if (!host_channel.connect(address,NULL)) // Set up for blocking I/O
          { // Cannot connect; may need to resolve address again later on
            fprintf(stderr,"Failed to connect to: %s:%d\n",
                    address.textualize(KDCS_ADDR_FLAG_LITERAL_ONLY |
                                       KDCS_ADDR_FLAG_BRACKETED_LITERALS),
                    (int) address.get_port());
            
            mutex.lock();
            host->resolution_counter = KD_DELEGATE_RESOLUTION_INTERVAL;
            host->load_counter = 0;
            mutex.unlock();
            continue;
          }
        host_channel.get_connected_address(address);
        data_block.restart();
        req->fields.write_query(data_block);
        int body_bytes = data_block.get_remaining_bytes();
        par_block.restart();
        par_block << "POST /" << req->resource << " HTTP/1.1\r\n";
        par_block << "Host: " << host->hostname << ":"
                  << host->port << "\r\n";
        par_block << "Content-Length: " << body_bytes << "\r\n";
        par_block << "Content-Type: application/x-www-form-urlencoded\r\n";
        par_block << "Connection: close\r\n\r\n";
        host_channel.write_block(par_block);
        host_channel.write_block(data_block);
        
        const char *reply, *header;
        
        par_block.restart();
        int code = 503; // Try back later by default
        if ((reply = host_channel.read_paragraph()) != NULL)
          code = process_delegate_response(reply,par_block,address);
        if (code == 503)
          continue; // Host may be too busy to answer right now
        
        // Forward the response incrementally
        bool more_chunks = false;
        int content_length = 0;
        if ((header =
             kdcs_caseless_search(reply,"\nContent-Length:")) != NULL)
          {
            while (*header == ' ') header++;
            sscanf(header,"%d",&content_length);
          }
        else if ((header =
                  kdcs_caseless_search(reply,"\nTransfer-encoding:")) != NULL)
          {
            while (*header == ' ') header++;
            if (kdcs_has_caseless_prefix(header,"chunked"))
              more_chunks = true;
          }
        data_block.restart();
        while (more_chunks || (content_length > 0))
          {
            if (content_length <= 0)
              {
                reply = host_channel.read_line();
                assert(reply != NULL); // Blocking call succeeds or throws
                if ((*reply == '\0') || (*reply == '\n'))
                  continue;
                if ((sscanf(reply,"%x",&content_length) == 0) ||
                    (content_length <= 0))
                  more_chunks = false;
                data_block.set_hex_mode(true);
                data_block << content_length << "\r\n";
                data_block.set_hex_mode(false);
              }
            if (content_length > 0)
              {
                host_channel.read_block(content_length,data_block);
                  // Blocking call succeeds or throws exception
                content_length = 0;
              }
            else
              data_block << "\r\n"; // End of chunked transfer
          }
        
        if ((content_length == 0) && !more_chunks)
          { // Communication with the host is complete
            body_bytes = data_block.get_remaining_bytes();
            par_block.append(data_block);
            response_channel->push_reply(par_block,body_bytes);
            return host->hostname_with_port;
          }
      }
      catch (...) {}
      host_channel.close(); // Not strictly necessary, but could help debugging
      break; // If we get here, we have failed to complete the transaction
    }
  return NULL;
}
