/*****************************************************************************/
// File: sources.cpp [scope = APPS/SERVER]
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
#include "kdcs_comms.h"
#include "server_local.h"
#include "kdu_messaging.h"

#ifdef KDU_WINDOWS_OS
#  define KD_PATH_MAX 400
#else // Assume Unix-like OS
# include <sys/stat.h>
# define KD_PATH_MAX PATH_MAX
#endif


/* ========================================================================= */
/*                             Internal Functions                            */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                      get_full_pathname                             */
/*****************************************************************************/

static bool get_full_pathname(char *full_pathname, const char *filename)
  /* Writes the full pathname associated with `filename' into the
     `full_pathname' buffer, which must be capable of holding at least
     `KD_PATH_MAX' characters, including the terminating null character.
     Returns false if unable to complete. */
{
  bool success = false;
#ifdef KDU_WINDOWS_OS
  char *tmp;
  if (GetFullPathName(filename,KD_PATH_MAX,full_pathname,&tmp))
    success = true;
#else // Assume Unix-like system
  if (realpath(filename,full_pathname) != NULL)
    success = true;
#endif
  return success;
}

/*****************************************************************************/
/* STATIC                     get_last_modify_time                           */
/*****************************************************************************/

static bool get_last_modify_time(kdu_long &value, const char *filename)
  /* Geneates 64-bit `value' which uniquely identifies the file's last
     modification time.  The meaning of this value is platform
     dependent, but we do not care about the actual interpretation for
     this application.  Returns false if the file does not exist. */
{
  value = 0;
#ifdef KDU_WINDOWS_OS
  HANDLE fhandle = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,
                              NULL,OPEN_EXISTING,0,NULL);
  if (fhandle == INVALID_HANDLE_VALUE)
    return false;
  FILETIME last_write_time;
  if (GetFileTime(fhandle,NULL,NULL,&last_write_time))
    {
      value = (kdu_long) last_write_time.dwHighDateTime;
      value <<= 32;
      value += (kdu_long) last_write_time.dwLowDateTime;
    }
  CloseHandle(fhandle);
#else // Assume Unix-like OS
  struct stat stat_buf;
  if (stat(filename,&stat_buf) != 0)
    return false;
  value = (kdu_long) stat_buf.st_mtime;
  value <<= 32;
  value += (kdu_long) stat_buf.st_ctime;
#endif
  return true;
}

/*****************************************************************************/
/*                               get_sys_address                             */
/*****************************************************************************/

void get_sys_address(kdu_byte address[])
  /* Fills in the first 6 bytes of the `address' array with the MAC
     address of our LAN card or with host-id/name dependent information. */
{
#ifdef KDU_WINDOWS_OS
  struct {
    ADAPTER_STATUS adapt;
    NAME_BUFFER name_buff[30];
  } adapter;
  NCB ncb;
  
  memset(&ncb,0,sizeof(ncb) );
  ncb.ncb_command = NCBRESET;
  ncb.ncb_lana_num = 0;
  Netbios(&ncb);
  memset(&ncb,0,sizeof(ncb));
  ncb.ncb_command = NCBASTAT;
  ncb.ncb_lana_num = 0;
  memset(ncb.ncb_callname,' ',NCBNAMSZ);
  ncb.ncb_callname[0] = '*';
  ncb.ncb_buffer = (unsigned char *) &adapter;
  ncb.ncb_length = sizeof(adapter);
  if (Netbios(&ncb) == 0)
    for (int i=0; i < 6; i++)
      address[i] = adapter.adapt.adapter_address[i];
#else // Not a Windows platform
  kdu_int32 host_id = gethostid();
  address[0] = (kdu_byte)(host_id>>0);
  address[1] = (kdu_byte)(host_id>>8);
  address[2] = (kdu_byte)(host_id>>16);  
  address[3] = (kdu_byte)(host_id>>24);
  address[4] = address[5] = 0;
  kdcs_sockaddr sockaddr;
  sockaddr.init(NULL,0); // Initialize with local hostname
  const char *hostname = sockaddr.textualize(0);
  if (hostname != NULL)
    {
      const char *cp;
      int i;
      for (cp=hostname, i=0; *cp != '\0'; cp++, i = 1-1)
        address[4+i] = (kdu_byte)(*cp);
    }
#endif // Non-Windows
}

/*****************************************************************************/
/* INLINE                        write_hex_val32                             */
/*****************************************************************************/

static inline void
  write_hex_val32(char *dest, kdu_uint32 val)
{
  int i;
  char v;
  for (i=8; i > 0; i--, val<<=4, dest++)
    {
      v = (char)((val >> 28) & 0x0000000F);
      *dest = (v < 10)?('0'+v):('A'+v-10);
    }
}

/*****************************************************************************/
/* INLINE                         read_hex_val32                             */
/*****************************************************************************/

static inline kdu_uint32
read_hex_val32(const char *src)
{
  kdu_uint32 val=0;
  int i;
  for (i=8; (i > 0) && (*src != '\0'); i--, src++)
    {
      val <<= 4;
      if ((*src >= '0') && (*src <= '9'))
        val += (kdu_uint32)(*src - '0');
      else if ((*src >= 'A') && (*src <= 'F'))
        val += 10 + (kdu_uint32)(*src - 'A');
    }
  return val;
}

/*****************************************************************************/
/* INLINE                read_val_or_wild (kdu_long)                         */
/*****************************************************************************/

static inline bool
  read_val_or_wild(const char * &string, kdu_long &val)
  /* Reads an unsigned decimal integer from the supplied `string', advancing
     the pointer immediately past the last character in the integer.  If
     `*string' is equal to `*' (wild-card), `val' is set to -1, again
     after advancing the string.  If a parsing error occurs, the function
     returns false. */
{
  if (*string == '*')
    { string++; val = -1; return true;}
  if ((*string >= '0') && (*string <= '9'))
    {
      val = 0;
      do {
        val = val*10 + (int)(*(string++) - '0');
      } while ((*string >= '0') && (*string <= '9'));
      return true;
    }
  return false;
}

/*****************************************************************************/
/* INLINE                   read_val_or_wild (int)                           */
/*****************************************************************************/

static inline bool
  read_val_or_wild(const char * &string, int &val)
  /* Same as above, but for regular integers. */
{
  if (*string == '*')
    { string++; val = -1; return true;}
  if ((*string >= '0') && (*string <= '9'))
    {
      val = 0;
      do {
        val = val*10 + (int)(*(string++) - '0');
      } while ((*string >= '0') && (*string <= '9'));
      return true;
    }
  return false;
}

/*****************************************************************************/
/* INLINE                read_val_range_or_wild (int)                        */
/*****************************************************************************/

static inline bool
  read_val_range_or_wild(const char * &string, kdu_long &min, kdu_long &max)
  /* Same as above, except that the function can also read a range of two
     values.  If there is only one value, `min' and `max' are both set to
     that value.  If the wildcard character `*' is encountered, both are set
     to -1. */
{
  if (*string == '*')
    { string++; min = max = -1; return true;}
  if ((*string >= '0') && (*string <= '9'))
    {
      min = 0;
      do {
        min = min*10 + (int)(*(string++) - '0');
      } while ((*string >= '0') && (*string <= '9'));
      if (*string == '-') 
        {
          max = 0; string++;
          while ((*string >= '0') && (*string <= '9'))
            max = max*10 + (int)(*(string++) - '0');
          if (string[-1] == '-')
            return false; // Failed to actually read a number
        }
      else
        max = min;
      return true;
    }
  return false;
}

/*****************************************************************************/
/* INLINE                read_val_range_or_wild (int)                        */
/*****************************************************************************/

static inline bool
  read_val_range_or_wild(const char * &string, int &min, int &max)
  /* Same as above, except that the range bounds are 32-bit integers. */
{
  if (*string == '*')
    { string++; min = max = -1; return true;}
  if ((*string >= '0') && (*string <= '9'))
    {
      min = 0;
      do {
        min = min*10 + (int)(*(string++) - '0');
      } while ((*string >= '0') && (*string <= '9'));
      if (*string == '-') 
        {
          max = 0; string++;
          while ((*string >= '0') && (*string <= '9'))
            max = max*10 + (int)(*(string++) - '0');
          if (string[-1] == '-')
            return false; // Failed to actually read a number
        }
      else
        max = min;
      return true;
    }
  return false;
}

/*****************************************************************************/
/* INLINE                      encode_var_length                             */
/*****************************************************************************/

static inline int
encode_var_length(kdu_byte *buf, int val)
/* Encodes the supplied value using a byte-oriented extension code,
 storing the results in the supplied data buffer and returning
 the total number of bytes used to encode the value. */
{
  int shift;
  kdu_byte byte;
  int num_bytes = 0;
  
  assert(val >= 0);
  for (shift=0; val >= (128<<shift); shift += 7);
  for (; shift >= 0; shift -= 7, num_bytes++)
    {
      byte = (kdu_byte)(val>>shift) & 0x7F;
      if (shift > 0)
        byte |= 0x80;
      *(buf++) = byte;
    }
  return num_bytes;
}



/* ========================================================================= */
/*                                 kd_source                                 */
/* ========================================================================= */

/*****************************************************************************/
/*                              kd_source::unlink                            */
/*****************************************************************************/

void kd_source::unlink()
{
  if (manager == NULL)
    return;
  if (prev == NULL)
    {
      assert(manager->sources == this);
      manager->sources = next;
    }
  else
    prev->next = next;
  if (next != NULL)
    next->prev = prev;
  manager->num_sources--;
  manager = NULL;
  next = prev = NULL;
}

/*****************************************************************************/
/*                              kd_source::open                              */
/*****************************************************************************/

bool kd_source::open(kd_target_description &tgt, bool caseless_target,
                     kdcs_message_block &explanation,
                     int phld_threshold, int per_client_cache,
                     const char *cache_directory,
                     const kdu_byte *sys_address)
{
  assert(this->target.filename[0] == '\0'); // Otherwise source is already open
  strcpy(this->target.filename,tgt.filename);
  strcpy(this->target.byte_range,tgt.byte_range);
  if (!this->target.parse_byte_range())
    assert(0); // Should already have been verified by the caller
  if (caseless_target)
    { // Convert target filename to lowercase
      char *cp;
      for (cp=this->target.filename; *cp != '\0'; cp++)
        *cp = (char) tolower(*cp);
    }
  
  // Find the time at which the file was last modified
  kdu_long file_time_val;
  if (!get_last_modify_time(file_time_val,this->target.filename))
    {
      explanation << "404 Image does not exist on server\r\n";
      return false;
    }
  
  kdu_byte last_modify_buf[8];
  for (int n=0; n < 8; n++, file_time_val>>=8)
    last_modify_buf[n] = (kdu_byte)(file_time_val);
  
  // Find the name for the cache file
  char *cp, cache_filename[320];
  if (cache_directory == NULL)
    strcpy(cache_filename,this->target.filename);
  else
    {
      strcpy(cache_filename,cache_directory);
      char *rel_path = this->target.filename;
      if ((strchr(rel_path,':') != NULL) ||
          (rel_path[0] == '/') || (rel_path[0] == '\\'))
        { // Strip back to the actual file name
          if ((cp = strrchr(rel_path,'/')) != NULL)
            rel_path = cp+1;
          if ((cp = strrchr(rel_path,'\\')) != NULL)
            rel_path = cp+1;
        }
      while ((rel_path[0]=='.') && (rel_path[1]=='.') &&
             ((rel_path[2]=='/') || (rel_path[2]=='\\')))
        rel_path += 3;
      strcat(cache_filename,"/");
      strcat(cache_filename,rel_path);
    }
  cp = strrchr(cache_filename,'.');
  if (cp != NULL)
    *cp = '_';
  if (this->target.byte_range[0] != '\0')
    {
      strcat(cache_filename,"(");
      strcat(cache_filename,this->target.byte_range);
      strcat(cache_filename,")");
    }
  strcat(cache_filename,".cache");
  
  bool cache_exists = false;
  target_id[0] = '\0';
  
  // See if an existing compatible cache file exists
  FILE *cache_fp = fopen(cache_filename,"rb");
  if (cache_fp != NULL)
    { // Read the target-ID
      cache_exists = true;
      target_id[32] = '\0';
      if (fread(target_id,1,32,cache_fp) != 32)
        cache_exists = false;
      for (int i=0; (i < 32) && cache_exists; i++)
        if (!(((target_id[i] >= '0') && (target_id[i] <= '9')) ||
              ((target_id[i] >= 'a') && (target_id[i] <= 'f')) ||
              ((target_id[i] >= 'A') && (target_id[i] <= 'F'))))
          cache_exists = false;
      char test_modify_buf[8];
      if ((fread(test_modify_buf,1,8,cache_fp) != 8) ||
          (memcmp(test_modify_buf,last_modify_buf,8) != 0))
        cache_exists = false;
      if (!cache_exists)
        { fclose(cache_fp); cache_fp = NULL; target_id[0] = '\0'; }
    }
  
  bool open_succeeded = false;
  do { // Loop so we can catch a bad cache file, and rewrite it
    try {
      if ((!cache_exists) && generate_target_id(sys_address))
        { // Try to create the cache file
          cache_fp = fopen(cache_filename,"wb");
          if ((cache_fp != NULL) &&
              ((fwrite(target_id,1,32,cache_fp) != 32) ||
               (fwrite(last_modify_buf,1,8,cache_fp) != 8)))
            { // Write failure (e.g., disk full)
              fclose(cache_fp); cache_fp = NULL; target_id[0] = '\0';
            }
        }
      serve_target.open(this->target.filename,phld_threshold,
                        per_client_cache,cache_fp,cache_exists,
                        this->target.range_start,this->target.range_lim);
      open_succeeded = true;
    }
    catch (...)
    {
      if (cache_fp != NULL)
        { // Delete the cache file
          fclose(cache_fp); cache_fp = NULL;
          remove(cache_filename);
        }
      if (cache_exists)
        { // Try regenerating the cache file
          cache_exists = false;
          target_id[0] = '\0';
        }
      else
        break; // Nothing more we can do
    }
  } while (!open_succeeded);
  
  if (cache_fp != NULL)
    fclose(cache_fp);
  if (!open_succeeded)
    explanation << "404 Cannot open target file on server\r\n";
  return open_succeeded;
}

/*****************************************************************************/
/*                      kd_source::generate_target_id                        */
/*****************************************************************************/

bool kd_source::generate_target_id(const kdu_byte *sys_address)
{
  memset(target_id,0,33);
  
  // Start by creating an initial 128-bit number from the last modification
  // date and the file name.
  kdu_long file_time_val;
  if (!get_last_modify_time(file_time_val,target.filename))
    return false;  
  union {
    kdu_byte id_data[16];
    kdu_uint32 id_data32[4];
  };
  id_data32[0] = (kdu_uint32)(file_time_val>>32);
  id_data32[1] = (kdu_uint32)(file_time_val);
  id_data32[2] = id_data32[3] = 0;
  
  char full_pathname[KD_PATH_MAX];
  memset(full_pathname,0,KD_PATH_MAX);
  if (!get_full_pathname(full_pathname,target.filename))
    strcpy(full_pathname,target.filename);
    
  int i;
  for (i=0; (i < KD_PATH_MAX) && (full_pathname[i] != '\0'); i++)
    {
      id_data[8+(i&7)] ^= (kdu_byte) target.filename[i];
      id_data[15-(i&7)] ^= ((kdu_byte) target.filename[i]) << 4;
    }
  for (i=0; target.byte_range[i] != '\0'; i++)
    {
      id_data[8+(i&7)] ^= (kdu_byte) target.byte_range[i];
      id_data[15-(i&7)] ^= ((kdu_byte) target.byte_range[i]) << 4;
    }
  
  // Now see if we can encrypt the 128-bit number using our own MAC address
  kdu_byte mac_key[16];
  for (i=0; i < 16; i++)
    mac_key[i] = sys_address[i % 6];
  kdu_rijndael cipher;
  cipher.set_key(mac_key);
  cipher.encrypt(id_data);
  
  for (i=0; i < 32; i++)
    {
      kdu_byte val = id_data[i>>1];
      if ((i & 1) == 0)
        val >>= 4;
      val &= 0x0F;
      if (val < 10)
        target_id[i] = (char)('0'+val);
      else
        target_id[i] = (char)('A'+val-10);
    }
  return true;
}


/* ========================================================================= */
/*                             kd_source_thread                              */
/* ========================================================================= */

/*****************************************************************************/
/* STATIC                  source_thread_start_proc                          */
/*****************************************************************************/

static kdu_thread_startproc_result KDU_THREAD_STARTPROC_CALL_CONVENTION
  source_thread_start_proc(void *param)
{
  kd_source_thread *obj = (kd_source_thread *) param;
  obj->run();
  obj->closing();
  return KDU_THREAD_STARTPROC_ZERO_RESULT;
}

/*****************************************************************************/
/*                      kd_source_thread::kd_source_thread                   */
/*****************************************************************************/

kd_source_thread::kd_source_thread(kd_source *source,
                                   kd_source_manager *manager,
                                   kd_connection_server *cserver,
                                   kdu_uint16 aux_listen_port, int max_cids,
                                   int max_chunk_bytes,
                                   bool ignore_relevance_info,
                                   float max_session_idle_seconds,
                                   float max_completion_seconds,
                                   float max_establishment_seconds,
                                   bool caseless_targets)
{
  if (max_chunk_bytes > 0xFFFF)  // Otherwise `kds_jpip_channel::push_chunks'
    max_chunk_bytes = 0xFFFF;    // can fail badly.

  mutex.create();
  event.create(false);
  this->source = source;
  this->manager = manager;
  this->cserver = cserver;
  this->max_completion_seconds = max_completion_seconds;
  this->max_establishment_seconds = max_establishment_seconds;
  this->max_chunk_bytes = max_chunk_bytes;
  this->ignore_relevance_info = ignore_relevance_info;
  this->caseless_targets = caseless_targets;
  
  ellapsed_microseconds = 0;
  last_idle_time = -1;
  max_idle_microseconds = (KDU_LONG_MAX - 100000000);
  if ((max_session_idle_seconds > 0.0F) &&
      (max_session_idle_seconds < (0.000001F * max_idle_microseconds)))
    max_idle_microseconds =
      1 + (kdu_long)(1000000.0F * max_session_idle_seconds);
  pref_max_bytes_per_second = -1.0F; // Until preference supplied by client
  
  finished = false;
  shutdown_requested = false;
  is_stateless = false;
  num_primary_channels = 0;
  primary_channels = NULL;
  this->aux_listen_port = aux_listen_port;
  this->max_cids = max_cids;
  num_cids = 0;
  cids = new kds_cid[max_cids];
  
  connection_established = false;
  num_primary_connections = 0;
  num_jpip_h_cids = num_jpip_ht_cids = 0;
  total_received_requests = total_serviced_requests = 0;
  num_serviced_with_byte_limits = 0;
  cumulative_byte_limit = 0;
  total_transmitted_bytes = 0;
  total_unacknowledged_bytes = 0;
  total_rtt_events = 0;
  total_rtt_seconds = 0.0F;
  auxiliary_channel_transmitted_bytes = 0;
  auxiliary_channel_active_seconds = 0.0F;
  prev = next = NULL;
  allow_dead_thread_delete = true;
}

/*****************************************************************************/
/*                     kd_source_thread::~kd_source_thread                   */
/*****************************************************************************/

kd_source_thread::~kd_source_thread()
{
  assert(num_cids == 0);
  if (thread.exists())
    thread.destroy(); // Waits for the thread to close
  kds_primary_channel *chn;
  while ((chn=primary_channels) != NULL)
    {
      primary_channels = chn->next;
      chn->channel->release();
      delete chn;
    }
  if (cids != NULL)
    {
      int p;
      for (p=0; p < num_cids; p++)
        if (cids[p].aux_channel != NULL)
          cids[p].aux_channel->release();
      delete[] cids;
    }
  event.destroy();
  mutex.destroy();
}

/*****************************************************************************/
/*                          kd_source_thread::start                          */
/*****************************************************************************/

void kd_source_thread::start()
{
  if (!thread.create(source_thread_start_proc,this))
    closing();
}

/*****************************************************************************/
/*                         kd_source_thread::closing                         */
/*****************************************************************************/

void kd_source_thread::closing()
{
  assert(finished); // Should have been set before calling this function
  
  // All all auxiliary channels a maximum of `max_completion_seconds' to
  // complete any outstanding transmission.  Do the same for the
  // primary channels which need to be released; however, we first recycle
  // any primary channels which are in a suitable state for it.
  int p;
  for (p=0; p < num_cids; p++)
    if (cids[p].aux_channel != NULL)
      cids[p].aux_channel->set_timeout(max_completion_seconds);
  kds_primary_channel *chn, *next_chn=NULL;
  for (chn=primary_channels; chn != NULL; chn=next_chn)
    {
      next_chn = chn->next;
      if ((chn->request_cid != NULL) && chn->reply_generated &&
          chn->release_channel_after_response)
        chn->channel->set_timeout(max_completion_seconds);
      else
        recycle_primary_channel(chn);
    }
  
  // Release all remaining channels
  while ((chn = primary_channels) != NULL)
    release_primary_channel(chn,true);
  for (p=0; p < num_cids; p++)
    release_auxiliary_channel(cids+p,true);
  
  // Print transmission statistics
  ellapsed_microseconds = timer.get_ellapsed_microseconds();
  float connected_seconds = 0.000001F * ellapsed_microseconds;
  float average_rtt = 0.0F;
  float approximate_aux_rate = 0.0F;
  float average_serviced_byte_limit = 0.0F;
  float average_service_bytes = 0.0F;

  time_t binary_time; time(&binary_time);
  struct tm *local_time = localtime(&binary_time);
  kd_msg.start_message();
  kd_msg << "\n\tDisconnecting client\n";
  kd_msg << "\t\tFile = \"" << source->target.filename << "\"\n";
  if (is_stateless)
    kd_msg << "\t\tChannel transport = <none> (stateless)\n";
  else if (num_jpip_h_cids == 0)
    kd_msg << "\t\tChannel transport = \"http-tcp\" ("
           << num_jpip_ht_cids << ")\n";
  else if (num_jpip_ht_cids == 0)
    kd_msg << "\t\tChannel transport = \"http\" ("
           << num_jpip_h_cids << ")\n";
  else
    kd_msg << "\t\tChannel transport = \"http\" ("
           << num_jpip_h_cids << "), \"http-tcp\" ("
           << num_jpip_ht_cids << ")\n";
  kd_msg << "\t\tDisconnect time = " << asctime(local_time);
  kd_msg << "\t\tConnected for " << connected_seconds << " seconds\n";
  kd_msg << "\t\tTotal bytes transmitted = " << total_transmitted_bytes;
  if (total_unacknowledged_bytes > 0)
    kd_msg << " (" << total_unacknowledged_bytes << " not acknowledged)";
  kd_msg << "\n";
  
  if (num_jpip_ht_cids > 0)
    {
      if (total_rtt_events == 0)
        kd_msg << "\t\tAverage RTT = ------\n";
      else
        {
          average_rtt = total_rtt_seconds/ total_rtt_events;
          approximate_aux_rate = auxiliary_channel_transmitted_bytes /
            (auxiliary_channel_active_seconds + 0.01F);
          kd_msg << "\t\tAverage auxiliary channel RTT = "
                 << average_rtt << " seconds\n";
          kd_msg << "\t\tApproximate auxiliary channel data rate = "
                 << approximate_aux_rate << " bytes/second\n";
        }
    }
  
  if (num_serviced_with_byte_limits > 0)
    average_serviced_byte_limit = ((float) cumulative_byte_limit) /
      ((float) num_serviced_with_byte_limits);
  if (total_serviced_requests > 0)
    average_service_bytes = ((float) total_transmitted_bytes) /
      ((float) total_serviced_requests);
  
  if (!connection_established)
    kd_msg << "\t\tClient failed to establish request and return channels.\n";
  else
    {
      kd_msg << "\t\tNumber of requests = "
             << total_received_requests << "\n";
      kd_msg << "\t\tNumber of requests not pre-empted = "
             << total_serviced_requests << "\n";
      kd_msg << "\t\tAverage bytes served per request not "
                "pre-empted = " << average_service_bytes << "\n";
      kd_msg << "\t\tNumber of primary channel attachment events = "
             << num_primary_connections << "\n";
    }
  kd_msg.flush(true);
  
  // Clean up resources
  this->destroy(); // Destroys the `kdu_serve' object's internal state
  if (source != NULL)
    source->serve_target.trim_resources();
  manager->delete_dead_threads();
  
  // Lock manager and update statistics
  manager->mutex.lock();
  kd_client_history *history = NULL;
  if (connection_established)
    {
      manager->total_transmitted_bytes += total_transmitted_bytes;
      manager->completed_connections++;
      history = manager->get_new_history_entry();
    }
  else
    manager->terminated_connections++;
  if (history != NULL)
    {
      history->num_jpip_h_channels += num_jpip_h_cids;
      history->num_jpip_ht_channels += num_jpip_ht_cids;
      strcpy(history->target.filename,source->target.filename);
      strcpy(history->target.byte_range,source->target.byte_range);
      history->num_primary_connections = num_primary_connections;
      history->total_transmitted_bytes = total_transmitted_bytes;
      history->connected_seconds = connected_seconds;
      history->num_requests = total_received_requests;
      history->rtt_events = total_rtt_events;
      history->average_rtt = average_rtt;
      history->approximate_auxiliary_channel_rate = approximate_aux_rate;
      history->average_serviced_byte_limit = average_serviced_byte_limit;
      history->average_service_bytes = average_service_bytes;
    }
  
  // Remove from list of threads for this source
  assert(source != NULL);
  if (prev == NULL)
    {
      assert(source->threads == this);
      source->threads = next;
    }
  else
    prev->next = next;
  if (next != NULL)
    next->prev = prev;
  
  // Add to list of dead threads in manager
  assert(manager->num_threads > 0);
  manager->num_threads--;
  next = manager->dead_threads;
  manager->dead_threads = this;
  prev = NULL;

  // See if we should remove the source from the manager's list of sources
  if (source->threads == NULL)
    {
      source->unlink();
      delete source;
    }
  source = NULL;

  // Signal and release the manager
  manager->event.set();
  manager->mutex.unlock();
}

/*****************************************************************************/
/* STATIC          kd_source_thread::translate_cnew_string                   */
/*****************************************************************************/

kd_jpip_channel_type
  kd_source_thread::translate_cnew_string(const char *string)
{
  kd_jpip_channel_type result = KD_CHANNEL_STATELESS;
  if (string != NULL)
    {
      const char *delim;
      for (; *string != '\0'; string=delim)
        {
          while (*string == ',') string++;
          for (delim=string; (*delim != '\0') && (*delim != ','); delim++);
          if (((delim-string)==8) &&
              kdcs_has_caseless_prefix(string,"http-tcp"))
            { result = KD_CHANNEL_HTTP_TCP; break; }
          if (((delim-string)==4) &&
              kdcs_has_caseless_prefix(string,"http"))
            { result = KD_CHANNEL_HTTP; break; }
        }
    }
  return result;
}

/*****************************************************************************/
/*                     kd_source_thread::assign_new_cid                      */
/*****************************************************************************/

const char *kd_source_thread::assign_new_cid(kd_jpip_channel_type channel_type,
                                             kds_cid *requesting_cid)
{
  manager->mutex.lock();
  if (is_stateless || (num_cids >= max_cids))
    {
      manager->mutex.unlock();
      return NULL;
    }
  if (channel_type == KD_CHANNEL_STATELESS)
    { // Calling this function after initial construction of source thread
      if (num_cids > 0)
        {
          manager->mutex.unlock();
          return NULL;
        }
      max_cids = 1;
      is_stateless = true;
    }
  kds_cid *cid = cids + num_cids;
  
  // Initialize
  cid->init();
  assert((cid->active_window == NULL) && (cid->active_model == NULL));
  cid->active_window = new kdu_window;
  cid->active_model = new kdu_window_model;
  cid->active_model->init(is_stateless);
  cid->active_prefs = new kdu_window_prefs;
  cid->channel_type = channel_type;
  if (requesting_cid != NULL)
    {
      cid->pref_bw_slice = requesting_cid->pref_bw_slice;
      cid->active_prefs->copy_from(*requesting_cid->active_prefs);
    }
  
  // Generate a unique channel-ID
  kd_source_thread *scan;
  do {
    kdu_uint32 client_id = (kdu_uint32)
      (manager->num_threads + (manager->num_threads<<16));
    client_id += (kdu_uint32)(manager->total_transmitted_bytes +
                              (manager->total_transmitted_bytes<<12));
    client_id ^= (kdu_uint32) rand();
    client_id ^= (kdu_uint32)(rand() << 11);
    cid->channel_id[0] = 'J';
    cid->channel_id[1] = 'P';
    cid->channel_id[2] = 'H';
    cid->channel_id[3] = (channel_type == KD_CHANNEL_HTTP_TCP)?'T':'_';
    write_hex_val32(cid->channel_id+4,client_id);
    write_hex_val32(cid->channel_id+12,source->source_id);
    cid->channel_id[20] = '\0';
    for (scan=source->threads; scan != NULL; scan=scan->next)
      {
        int p;
        for (p=0; p < scan->num_cids; p++)
          if (strcmp(scan->cids[p].channel_id,cid->channel_id) == 0)
            break;
        if (p < scan->num_cids)
          break;
      }
  } while (scan != NULL); // There is a random component which ensures
                          // that we keep generating new service ID's.
    
  // Find a spare `window_id' and advance counters
  for (cid->window_id=0; cid->window_id < num_cids; cid->window_id++)
    {
      int n;
      for (n=0; n < num_cids; n++)
        if (cids[n].window_id == cid->window_id)
          break;
      if (n == num_cids)
        break; // Found an unused `window_id'
    }
  
  num_cids++;
  manager->mutex.unlock();

  if (channel_type == KD_CHANNEL_HTTP)
    num_jpip_h_cids++;
  else if (channel_type == KD_CHANNEL_HTTP_TCP)
    num_jpip_ht_cids++;
  
  normalize_channel_bandwidths();

  return cid->channel_id;
}

/*****************************************************************************/
/*                     kd_source_thread::install_channel                     */
/*****************************************************************************/

bool kd_source_thread::install_channel(kds_jpip_channel *channel,
                                       kd_request_queue *transfer_queue,
                                       const char *auxiliary_channel_id)
{
  bool success = false;
  channel->set_timeout(-1.0F); // Disable timeouts
  mutex.lock();
  if ((!finished) && (num_cids > 0))
    {
      if (auxiliary_channel_id != NULL)
        {
          int which = find_channel_id(auxiliary_channel_id);
          if (which >= 0)
            {
              kds_cid *cid = cids + which;
              if ((cid->aux_channel == NULL) &&
                  (cid->channel_type == KD_CHANNEL_HTTP_TCP))
                { // Install the auxiliary channel
                  cid->aux_channel = channel;
                  cid->aux_timeout = -1;
                  success = true;
                  if (transfer_queue != NULL)
                    transfer_queue->init(); // Should not happen
                  if (pref_max_bytes_per_second > 0.0F)
                    channel->set_bandwidth_limit(pref_max_bytes_per_second *
                                                 cid->bw_slice);
                }
            }
        }
      else if (num_primary_channels < max_cids)
        {
          kds_primary_channel *chn = new kds_primary_channel;
          chn->next = primary_channels;
          primary_channels = chn;
          num_primary_channels++;
          chn->channel = channel;
          chn->initial_transmitted_bytes =
            channel->get_total_transmitted_bytes(true);
          if (transfer_queue != NULL)
            chn->pending_requests.transfer_state(transfer_queue);
          num_primary_connections++;
          success = true;
        }
      if (success)
        {
          last_idle_time = -1; // Session can no longer be considered idle
          event.set(); // Wake up the source thread if necessary
          if ((!connection_established) && (primary_channels != NULL))
            {
              int p;
              for (p=0; p < num_cids; p++)
                if ((cids[p].aux_channel != NULL) ||
                    (cids[p].channel_type != KD_CHANNEL_HTTP_TCP))
                  {
                    connection_established = true;
                    break;
                  }
            }
        }
    }
  mutex.unlock();
  return success;
}

/*****************************************************************************/
/*                     kd_source_thread::request_shutdown                    */
/*****************************************************************************/

void kd_source_thread::request_shutdown()
{ // Note: Must not lock the mutex here, since the source manager's mutex has
  // been locked by the caller.
  shutdown_requested = true;
  event.set();
}

/*****************************************************************************/
/*                            kd_source_thread::run                          */
/*****************************************************************************/

void kd_source_thread::run()
{
  try {
    initialize(&source->serve_target,max_chunk_bytes,8,ignore_relevance_info);
  }
  catch (...) {
    source->failed = shutdown_requested = true;
  }
  int p;
  kdu_byte final_eor_code = JPIP_EOR_UNSPECIFIED_REASON;
  kdu_long primary_attachment_timeout = -1;
  mutex.lock();
  while (!shutdown_requested)
    {
      event.reset();
      // Scan primary channels to get the most up-to-date request
      kds_primary_channel *chn, *next_chn=NULL;
      for (chn=primary_channels; chn != NULL; chn=next_chn)
        {
          primary_attachment_timeout = -1;
          next_chn = chn->next;
          if ((!chn->channel->retrieve_requests(&chn->pending_requests,
                                                false,&event)) &&
              (chn->request_cid == NULL) &&
              (chn->pending_requests.peek_head() == NULL))
            { // Check for unexpected closure, but not until all outstanding
              // requests have been processed; this ensures that `cclose'
              // requests issued immediately before closing a connection are
              // handled as robustly as possible.
              if (!chn->channel->is_active())
                { // Channel has been dropped unexpectedly
                  release_primary_channel(chn,false);
                  continue;
                }
            }
        }
      if (chn != NULL)
        { // Timeout encountered on a request channel; end of session!
          final_eor_code = JPIP_EOR_UNSPECIFIED_REASON;
          break;
        }
      
      ellapsed_microseconds = timer.get_ellapsed_microseconds();
      process_requests();
      if ((last_idle_time >= 0) &&
          (ellapsed_microseconds > (max_idle_microseconds+last_idle_time)))
        {
          final_eor_code = JPIP_EOR_SESSION_LIMIT_REACHED;
          break;
        }

      // Update the flow control state variables here to avoid all active
      // channels blocking on a `bucket_threshold' value.
      int num_active_cids = update_flow_control();

      // Now scan the JPIP channels for a request to serve
      bool processed_something = false;
      int suggested_bytes=2;
      kdu_long next_cid_timeout = -1;
      kds_jpip_channel *data_channel = NULL;
      kds_cid *sending_cid = NULL;
      bool error_encountered = false;
      for (p=0;
           (p < num_cids) && (sending_cid == NULL) && !error_encountered;
           p++)
        {
          kds_cid *cid = cids+p;
          if (cid->qid_timeout > 0)
            {
              if (cid->qid_timeout <= ellapsed_microseconds)
                cid->close_when_inactive = true;
              else if ((next_cid_timeout < 0) ||
                       (next_cid_timeout > cid->qid_timeout))
                next_cid_timeout = cid->qid_timeout;
            }
          if (!cid->is_active)
            continue; // Nothing to send on this channel
          if (cid->bucket_fulness > cid->bucket_threshold)
            { // No bandwidth availability for this channel yet
              if (cid->close_when_inactive)
                continue; // Don't block channels waiting to close
            }
          data_channel = ((cid->channel_type==KD_CHANNEL_HTTP_TCP)?
                          cid->aux_channel:(cid->active_chn->channel));
          suggested_bytes = -1; // Could help in debugging
          if (data_channel == NULL)
            { // Waiting for `aux_channel' to connect
              assert(cid->channel_type==KD_CHANNEL_HTTP_TCP);
              if (cid->active_chn == NULL)
                { // Already generated the initial reply
                  if (cid->aux_timeout < 0)
                    cid->aux_timeout = ellapsed_microseconds +
                    (kdu_long)(1000000.0F * max_establishment_seconds);
                  else if (cid->aux_timeout <= ellapsed_microseconds)
                    cid->close_when_inactive = cid->close_immediately = true;
                  else if ((next_cid_timeout < 0) ||
                           (next_cid_timeout > cid->aux_timeout))
                    next_cid_timeout = cid->aux_timeout;
                  continue;
                }
              suggested_bytes = 1 + (max_chunk_bytes/2); // To get us going
            }
          else if (cid->active_chunks == NULL)
            { // We will need to generate some data to send
              if ((suggested_bytes =
                   data_channel->get_suggested_bytes(false,&event)) <= 0)
                { // Channel closed or waiting for holding queue to empty
                  if (!data_channel->is_active())
                    cid->close_immediately = true;
                  continue;
                }
              kds_chunk *chunks = data_channel->retrieve_completed_chunks();
              if (chunks != NULL)
                release_chunks(chunks);
            }
          if (cid->active_chunks == NULL)
            {
              error_encountered = !generate_data_chunks(cid,suggested_bytes);
              processed_something = true;
            }
          if ((cid->active_chn != NULL) && !cid->active_chn->reply_generated)
            {
              if (error_encountered)
                {
                  replies.restart();
                  replies << "HTTP/1.1 500 Server encountered an unexpected "
                             "error while trying to generate data\r\n";
                  generate_error_reply(cid->active_chn,NULL);
                  assert((cid->active_chn==NULL) && cid->close_when_inactive);
                }
              else
                generate_reply(cid);
              processed_something = true;
            }
          if (error_encountered)
            break;
          if ((data_channel != NULL) && (cid->active_chunks != NULL))
            sending_cid = cid;
          else if (cid->active_chunks == NULL)
            {
              assert(cid->response_complete && (cid->active_chn == NULL));
              cid->is_active = cid->response_complete = false;
            }
          if (!cid->is_active)
            num_active_cids--;
        }
      
      if (error_encountered)
        {
          final_eor_code = JPIP_EOR_UNSPECIFIED_REASON;
          break;
        }
      
      if (sending_cid != NULL)
        { // Send data prepared in `sending_cid'
          processed_something = true;
          assert(data_channel != NULL);
          const char *content_type =
            ((sending_cid->active_extended_headers)?
             "image/jpp-stream;ptype=ext":"image/jpp-stream");
          kdu_long num_sent_bytes = 0;
          kds_chunk *chunks=sending_cid->active_chunks;
          while (sending_cid->active_chunks != NULL)
            {
              num_sent_bytes += sending_cid->active_chunks->num_bytes -
                sending_cid->active_chunks->prefix_bytes;
              sending_cid->active_chunks = sending_cid->active_chunks->next;
            }
          bool all_done_with_channel =
            (sending_cid->response_complete &&
             sending_cid->close_when_inactive &&
             (data_channel == sending_cid->aux_channel));
          if ((data_channel != sending_cid->aux_channel) &&
              (pref_max_bytes_per_second > 0.0F))
            data_channel->set_bandwidth_limit(pref_max_bytes_per_second *
                                              sending_cid->bw_slice);
          if ((chunks =
               data_channel->push_chunks(chunks,content_type,
                                         all_done_with_channel)) != NULL)
            release_chunks(chunks);
          if (sending_cid->response_complete)
            {
              if (sending_cid->channel_type != KD_CHANNEL_HTTP_TCP)
                sending_cid->active_chn->channel->terminate_chunked_data();
              detach_active_chn(sending_cid);
              sending_cid->is_active = sending_cid->response_complete = false;
            }
          if (!sending_cid->is_active)
            num_active_cids--;
          sending_cid->bucket_fulness += (float) num_sent_bytes;
          sending_cid->bucket_threshold =
            0.8F*sending_cid->bucket_threshold + 0.2F * num_sent_bytes;
        }
            
      // Check for JPIP channels which should be closed down
      do {
        for (p=0; p < num_cids; p++)
          if (cids[p].close_immediately ||
              (cids[p].close_when_inactive && !cids[p].is_active))
            {
              if (cids[p].is_active)
                num_active_cids--;
              close_cid(p,JPIP_EOR_RESPONSE_LIMIT_REACHED);
              processed_something = true;
              break; // Channels reshuffled; safest to search from scratch
            }
      } while (p < num_cids);
      
      if (num_cids == 0)
        break; // All JPIP session channels have been closed

      // See if the session is currently idle
      if (num_active_cids > 0)
        last_idle_time = -1;
      else if (last_idle_time < 0)
        last_idle_time = ellapsed_microseconds;
      
      if (processed_something)
        continue; // Go back to the start of the loop immediately; we made
                  // some changes which may have an immediate impact on our
                  // ability to process more channel requests or responses.
      
      // If we get here, we need to wait until a relevant condition occurs.
      if ((primary_channels == NULL) && (num_active_cids == 0))
        {
          if (primary_attachment_timeout < 0)
            primary_attachment_timeout = ellapsed_microseconds +
              (kdu_long)(1000000.0F * max_establishment_seconds);
          else if (primary_attachment_timeout <= ellapsed_microseconds)
            {
              final_eor_code = JPIP_EOR_RESPONSE_LIMIT_REACHED;
              break;
            }
        }
      kdu_long wait_usecs = ellapsed_microseconds+(1<<26); // Wait at most ~64s
      if ((last_idle_time >= 0) &&
          ((last_idle_time+max_idle_microseconds) < wait_usecs))
        wait_usecs = last_idle_time + max_idle_microseconds;
      if ((primary_attachment_timeout >= 0) &&
          (primary_attachment_timeout < wait_usecs))
        wait_usecs = primary_attachment_timeout;
      if ((next_cid_timeout >= 0) && (next_cid_timeout < wait_usecs))
        wait_usecs = next_cid_timeout;
      wait_usecs -= ellapsed_microseconds;
      if (wait_usecs > 0)
        event.timed_wait(mutex,(int) wait_usecs);
    }
  
  finished = true; // Important to set this before final calls to `close_cid'
                   // or unlocking the mutex.
  
  // Gracefully close any active response channels, generating outstanding
  // replies.
  while (num_cids > 0)
    {
      if (cids[0].is_active)
        cids[0].close_immediately = true;
      close_cid(0,final_eor_code);
    }

  mutex.unlock();
}

/*****************************************************************************/
/*                    kd_source_thread::process_requests                     */
/*****************************************************************************/

void kd_source_thread::process_requests()
{
  bool done = false;
  while ((num_cids > 0) && !done)
    {
      done = true; // Until proven otherwise
      kds_primary_channel *chn, *next_chn=NULL;
      for (chn=primary_channels; chn != NULL; chn=next_chn)
        {
          next_chn = chn->next;
          if ((chn->request_cid == NULL) &&
              !process_requests_to_stage_1(chn))
            continue;
          last_idle_time = -1; // Session cannot be considered idle
          kds_cid *cid = chn->request_cid;
          assert(cid != NULL);
          if ((chn->request_qid > 0) &&
              (cid->next_qid > chn->request_qid))
            { // Request arriving out of sequence!!
              cid->close_when_inactive = cid->close_immediately = true;
              continue;
            }
          if (cid->active_chn == chn)
            continue; // CID already knows about us
          if (cid->is_active)
            {
              if ((cid->active_chunks != NULL) || !chn->request_preemptive)
                continue; // Need to wait for ongoing request to finish
              preempt_request(cid);
              assert((cid->active_chn == NULL) && !cid->is_active);
              done = false; // Pre-empting a request may make another available
            }
          if (cid->close_when_inactive)
            continue; // Don't put new request into channel if we are closing
          if (chn->request_qid > 0)
            {
              if  (cid->next_qid != chn->request_qid)
                { // Need to wait until request with missing QID arrives
                  if (cid->qid_timeout < 0)
                    cid->qid_timeout = ellapsed_microseconds +
                      (kdu_long)(1000000.0F * max_completion_seconds);
                  continue; // Not ready for this request yet
                }
              cid->next_qid = (kdu_uint32)(chn->request_qid+1);
            }
          cid->qid_timeout = -1; // We have a correctly sequenced request
          if (!process_request_to_stage_2(chn))
            {
              done = false; // Go back through all primary channels again
              continue;
            }
          if (chn->request_cid != cid)
            { // A new channel has been allocated
              done = false; // Go back through all primary channels again
              continue;
            }
          assert(chn->request_processed);
          cid->active_chn = chn;
          cid->is_active = true;
          assert(cid->active_chunks == NULL);
          cid->active_window->copy_from(chn->window,false);
          if (is_stateless)
            cid->active_model->copy_from(chn->model);
          else
            cid->active_model->append(chn->model);
          chn->model.init(is_stateless);
          int pref_update_flags = -1;
          if (is_stateless)
            cid->active_prefs->copy_from(chn->window_prefs);
          else
            pref_update_flags = cid->active_prefs->update(chn->window_prefs);
          chn->window_prefs.init();
          if (pref_update_flags & (KDU_BANDWIDTH_SLICE_PREF |
                                   KDU_MAX_BANDWIDTH_PREF))
            { // Making some change to the absolute or relative bandwidth
              int pref_flags = (cid->active_prefs->preferred |
                                cid->active_prefs->required);
              if (pref_flags & KDU_BANDWIDTH_SLICE_PREF)
                cid->pref_bw_slice = cid->active_prefs->bandwidth_slice;
              if (pref_flags & KDU_MAX_BANDWIDTH_PREF)
                pref_max_bytes_per_second =
                  ((float) cid->active_prefs->max_bandwidth) * 0.125F;
              normalize_channel_bandwidths();
            }
          cid->active_extended_headers = chn->requested_extended_headers;
          cid->active_align = chn->requested_align;
          cid->active_remaining_bytes = chn->requested_max_bytes;    
          cid->response_complete = false;
          if ((!is_stateless) &&
              chn->pending_requests.have_preempting_request(cid->channel_id))
            {
              preempt_request(cid);
              assert(chn->request_cid == NULL);
              done = false; // Pre-empting request makes another available
            }
        }
    }
}

/*****************************************************************************/
/*                kd_source_thread::process_requests_to_stage_1              */
/*****************************************************************************/

bool kd_source_thread::process_requests_to_stage_1(kds_primary_channel *chn)
{
  replies.restart();
  const kd_request *req=chn->pending_requests.peek_head();
  if ((req = chn->pending_requests.peek_head()) == NULL)
    return false;

  if (req->resource == NULL)
    {
      replies << "HTTP/1.1 405 Unacceptable request method, \""
              << req->method << "\"\r\n";
      generate_error_reply(chn,req);
      return false;
    }
  
  // Check for target compatibility.  We will need to know this.
  chn->requested_target_compatible = true;
  if ((req->fields.target != NULL) &&
      (is_stateless ||
       (strcmp(req->fields.target,KD_MY_RESOURCE_NAME) != 0)))
    { // Have a non-trivial target
      const char *src_filename = source->target.filename;
      if ((strcmp(req->fields.target,src_filename) != 0) &&
          !(caseless_targets &&
            kdcs_caseless_compare(req->fields.target,src_filename)))
        chn->requested_target_compatible = false;
      else if (req->fields.sub_target == NULL)
        chn->requested_target_compatible =
          (source->target.byte_range[0] == '\0');
      else
        chn->requested_target_compatible =
          (strcmp(req->fields.sub_target,source->target.byte_range) == 0);
    }
  else if (is_stateless)
    chn->requested_target_compatible = false; // Stateless requests need target
  if ((req->fields.sub_target != NULL) &&
      (strcmp(req->fields.sub_target,source->target.byte_range) != 0))
    chn->requested_target_compatible = false;

  int p;
  if (is_stateless)
    { // Stateless request: target must agree
      assert(num_cids == 1);
      if (chn->requested_target_compatible)
        chn->request_cid = cids;
    }
  else if ((num_cids == 1) && cids[0].just_allocated)
    { // Very first request of the session
      chn->request_cid = cids;
      assert(chn->requested_target_compatible);
    }
  else if (req->fields.channel_id == NULL)
    { // May still be able to associate using cclose
      if ((req->fields.channel_close != NULL) &&
          ((p = find_channel_id(req->fields.channel_close)) >= 0))
        chn->request_cid = cids+p;
    }
  else if ((p = find_channel_id(req->fields.channel_id)) >= 0)
    chn->request_cid = cids+p;

  if (chn->request_cid == NULL)
    { // The request is not compatible with the current session; may
      // belong to another session, so hand it back to connection manager.
      recycle_primary_channel(chn);
      return false;
    }

  int val;
  chn->request_qid = -1;
  if ((req->fields.request_id != NULL) && !is_stateless)
    {
      if ((sscanf(req->fields.request_id,"%d",&val) == 0) || (val < 0))
        {
          replies << "HTTP/1.1 400 `qid' request field"
                     "has illegal parameter string.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      chn->request_qid = val;
    }

  assert(chn->request_cid != NULL);
  chn->release_channel_after_response = req->close_connection;
  chn->request_preemptive = req->preemptive;
  chn->request_processed = false;
  chn->reply_generated = false;
  return true;
}

/*****************************************************************************/
/*                kd_source_thread::process_request_to_stage_2               */
/*****************************************************************************/

bool kd_source_thread::process_request_to_stage_2(kds_primary_channel *chn)
{
  kds_cid *cid = chn->request_cid;
  const kd_request *req = chn->pending_requests.pop_head();
  assert((req != NULL) && (cid != NULL) && !chn->request_processed);
  replies.restart();
  chn->window.init();
  chn->model.init(is_stateless);
  chn->window_prefs.init();
  chn->requested_max_bytes = INT_MAX;
  chn->requested_align =
    ((req->fields.align != NULL) && (strcmp(req->fields.align,"yes") == 0));
  chn->requested_extended_headers = false;
  chn->reply_needs_target_id =
    ((req->fields.target_id != NULL) &&
     (strcmp(req->fields.target_id,source->target_id) != 0));
  chn->reply_needs_jpip_channel_id = (req->fields.channel_new != NULL);
  chn->reply_needs_roi_index = false;
  chn->reply_needs_quality_value = false;
  chn->reply_needs_min_len = false;
  chn->release_channel_after_response = req->close_connection;
  chn->reply_generated = false;

  bool first_request_on_channel = cid->just_allocated && !is_stateless;
  cid->just_allocated = false;
  
  if (!chn->requested_target_compatible)
    { // The session-ID demonstrates compatibility but target does not!
      replies << "HTTP/1.1 503 Request cannot be granted for access to "
                 "a different target (image source) within a JPIP "
                 "session.  The request might be legal (if `cnew' is also "
                 "being issued), but this server can only associate one "
                 "logical target with a given session.\r\n";
      generate_error_reply(chn,req);
      return false;
    }
    
  const char *scan, *delim;
  if (req->fields.unrecognized != NULL)
    {
      replies << "HTTP/1.1 400 Unrecognized request field, \""
              << req->fields.unrecognized << "\"\r\n";
      generate_error_reply(chn,req);
      return false;
    }
  if (req->fields.upload != NULL)
    {
      replies << "HTTP/1.1 501 Upload functionality not implemented.\r\n";
      generate_error_reply(chn,req);
      return false;
    }
  if ((req->fields.xpath_bin != NULL) ||
      (req->fields.xpath_box != NULL) ||
      (req->fields.xpath_query != NULL))
    {
      replies << "HTTP/1.1 501 Xpath functionality not implemented.\r\n";
      generate_error_reply(chn,req);
      return false;
    }
  if ((req->fields.max_length != NULL) &&
      ((sscanf(req->fields.max_length,"%d",&(chn->requested_max_bytes))==0) ||
       (chn->requested_max_bytes < 0)))
    {
      replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_MAX_LENGTH
              << "=\" request body.  Need non-negative integer.\r\n";
      generate_error_reply(chn,req);
      return false;
    }

  if (req->fields.type != NULL)
    {
      for (scan=req->fields.type; *scan != '\0'; scan=delim)
        {
          while (*scan == ',') scan++;
          for (delim=scan;
               (*delim != ',') && (*delim != ';') && (*delim != '\0');
               delim++);
          if ((((delim-scan) == 1)  && (*scan == '*')) ||
              (((delim-scan) == 10) &&
               (strncmp(scan,"jpp-stream",10) == 0)) ||
              (((delim-scan) == 16) &&
               (strncmp(scan,"image/jpp-stream",16)==0)))
            break;
          if (*delim == ';')
            while ((*delim != ',') && (*delim != '\0'))
              delim++;
        }
      if (*scan == '\0')
        {
          replies << "HTTP/1.1 415 Requested image return types are not "
                    "acceptable; can only return \"jpp-stream\".\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      if (*delim == ';')
        {
          for (scan=delim+1; (*delim != ',') && (*delim != '\0'); delim++);
          if (((delim-scan) == 9) && (strncmp(scan,"ptype=ext",9) == 0))
            chn->requested_extended_headers = true;
        }
    }
  
  int val1, val2;
  if (req->fields.full_size != NULL)
    {
      if ((sscanf(req->fields.full_size,"%d,%d",&val1,&val2) != 2) ||
          (val1 <= 0) || (val2 <= 0))
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_FULL_SIZE
                     "=\" request body.  Need two positive integers.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      chn->window.resolution.x = val1;
      chn->window.resolution.y = val2;
      for (scan=req->fields.full_size; *scan != ','; scan++);
      for (scan++; (*scan != ',') && (*scan != '\0'); scan++);
      chn->window.round_direction = -1; // Default is round-down
      if (strcmp(scan,",round-up") == 0)
        chn->window.round_direction = 1;
      else if (strcmp(scan,",closest") == 0)
        chn->window.round_direction = 0;
      else if (strcmp(scan,",round-down") == 0)
        chn->window.round_direction = -1;
      else if (*scan != '\0')
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_FULL_SIZE
                     "=\" request body.  Unrecognizable round-direction.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
    }
  else if ((req->fields.region_offset != NULL) ||
           (req->fields.region_size != NULL))
    {
      replies << "HTTP/1.1 400 Malformed JPIP request.  Neither the "
                 "\"" JPIP_FIELD_REGION_OFFSET "\", nor the "
                 "\"" JPIP_FIELD_REGION_SIZE "\" request field may "
                 "appear in a request which does not contain a "
                 "\"" JPIP_FIELD_FULL_SIZE "\" request field.\r\n";
      generate_error_reply(chn,req);
      return false;
    }
  if (req->fields.region_size != NULL)
    {
      if ((sscanf(req->fields.region_size,"%d,%d",&val1,&val2) != 2) ||
          (val1 <= 0) || (val2 <= 0))
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_REGION_SIZE
                     "=\" request body.  Need two positive integers.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      chn->window.region.size.x = val1;
      chn->window.region.size.y = val2;
    }
  else
    chn->window.region.size = chn->window.resolution;
  if (req->fields.region_offset != NULL)
    {
      if ((sscanf(req->fields.region_offset,"%d,%d",&val1,&val2) != 2) ||
          (val1 < 0) || (val2 < 0))
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_REGION_OFFSET
                     "=\" request body.  Need two non-negative integers.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      chn->window.region.pos.x = val1;
      chn->window.region.pos.y = val2;
    }
  if (req->fields.layers != NULL)
    {
      if ((sscanf(req->fields.layers,"%d",&val1) != 1) || (val1 < 0))
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_LAYERS
                     "=\" request body.  Need non-negative integer.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      chn->window.max_layers = val1;
    }
  if (req->fields.components != NULL)
    {
      bool failure = false;
      char *end_cp;
      int from, to;
      for (scan=req->fields.components; (*scan!='&') && (*scan!='\0'); )
        {
          while (*scan == ',')
            scan++;
          from = to = strtol(scan,&end_cp,10);
          if (end_cp == scan)
            { failure = true; break; }
          scan = end_cp;
          if (*scan == '-')
            {
              scan++;
              to = strtol(scan,&end_cp,10);
              if (end_cp == scan)
                to = INT_MAX;
              scan = end_cp;
            }
          if (((*scan != ',') && (*scan != '&') && (*scan != '\0')) ||
              (from < 0) || (from > to))
            { failure = true; break; }
          chn->window.components.add(from,to);
        }
      if (failure)
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_COMPONENTS
                     "=\" request body.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
    }
  if (req->fields.codestreams != NULL)
    {
      bool failure = false;
      char *end_cp;
      kdu_sampled_range range;
      for (scan=req->fields.codestreams; (*scan!='&') && (*scan!='\0'); )
        {
          while (*scan == ',')
            scan++;
          range.step = 1;
          range.from = range.to = strtol(scan,&end_cp,10);
          if (end_cp == scan)
            { failure = true; break; }
          scan = end_cp;
          if (*scan == '-')
            {
              scan++;
              range.to = strtol(scan,&end_cp,10);
              if (end_cp == scan)
                range.to = INT_MAX;
              scan = end_cp;
            }
          if (*scan == ':')
            {
              scan++;
              range.step = strtol(scan,&end_cp,10);
              if (end_cp == scan)
                { failure = true; break; }
              scan = end_cp;
            }
          if (((*scan != ',') && (*scan != '&') && (*scan != '\0')) ||
              (range.from < 0) || (range.from > range.to) ||
              (range.step < 1))
            { failure = true; break; }
          chn->window.codestreams.add(range);
        }
      if (failure)
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_CODESTREAMS
                     "=\" request body.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
    }
  
  int default_codestream_id = ((chn->window.codestreams.is_empty())?0:
                               chn->window.codestreams.get_range(0).from);
  
  if (req->fields.contexts != NULL)
    {
      for (scan=req->fields.contexts; (*scan!='&') && (*scan!='\0'); )
        {
          while (*scan == ',') scan++;
          scan = chn->window.parse_context(scan);
          if ((*scan != ',') && (*scan != '&') && (*scan != '\0'))
            {
              replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_CONTEXTS
                         "=\" request body.\r\n";
              generate_error_reply(chn,req);
              return false;
            }
        }
    }
  
  chn->reply_roi_index = 0;
  if (req->fields.roi != NULL)
    {
      chn->reply_needs_roi_index = true;
      chn->reply_roi_index =
        source->serve_target.find_roi(default_codestream_id,
                                      req->fields.roi);
      if (chn->reply_roi_index <= 0)
        chn->reply_roi_index = 0; // Still need to reply (no-ROI)
      else
        source->serve_target.get_roi_details(chn->reply_roi_index,
                                             chn->window.resolution,
                                             chn->window.region);
    }
  
  if (req->fields.max_quality != NULL)
    chn->reply_needs_quality_value = true;
  
  if (req->fields.meta_request != NULL)
    {
      const char *failure=chn->window.parse_metareq(req->fields.meta_request);
      if (failure != NULL)
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_META_REQUEST
                     "=\" request body.  Error at \"" << failure << "\".\r\n";
          generate_error_reply(chn,req);
          return false;
        }
    }
  
  if (req->fields.tp_model != NULL)
    {
      replies << "HTTP/1.1 400 \"" JPIP_FIELD_TP_MODEL "\" request field "
                 "is inappropriate in this context.\r\n";
      generate_error_reply(chn,req);
    }
  
  if ((req->fields.need != NULL) && is_stateless &&
      !chn->reply_needs_target_id)
    { 
      chn->model.init(true,true,default_codestream_id);
      if (!process_cache_specifiers(req->fields.need,chn->model,true))
        { 
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_NEED
                     "=\" request body.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
    }
  else if ((req->fields.model != NULL) && !chn->reply_needs_target_id)
    { 
      chn->model.init(is_stateless,false,default_codestream_id);
      if (!process_cache_specifiers(req->fields.model,chn->model,false))
        { 
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_MODEL
                     "=\" request body.\r\n";
          generate_error_reply(chn,req);
          return false;
        }
    }
  
  // Verify preferences before processing any state modifying requests
  kdu_window_prefs *prefs = NULL;
  if (req->fields.preferences != NULL)
    {
      prefs = &(chn->window_prefs);
      const char *failure = prefs->parse_prefs(req->fields.preferences);
      if (failure != NULL)
        {
          replies << "HTTP/1.1 400 Malformed \"" JPIP_FIELD_PREFERENCES
          "=\" request body.  Error at \"" << failure << "\".\r\n";
          generate_error_reply(chn,req);
          return false;
        }
      if (handle_unsupported_prefs(chn,req))
        return false;
    }
  
  // Process state modifying requests now
  if ((req->fields.channel_new != NULL) && !first_request_on_channel)
    { // Requesting a new channel-ID.
      int p;
      kd_jpip_channel_type ctp=translate_cnew_string(req->fields.channel_new);
      const char *new_channel_id = NULL;
      if (ctp != KD_CHANNEL_STATELESS)
        new_channel_id = assign_new_cid(ctp,chn->request_cid);
      if ((new_channel_id != NULL) &&
          ((p = find_channel_id(new_channel_id)) >= 0))
        {
          chn->request_cid = cids+p;
          if (chn->request_qid > 0)
            chn->request_cid->next_qid = (kdu_uint32)(chn->request_qid);
          chn->pending_requests.replace_head(req);
          return true;
        }
      chn->reply_needs_jpip_channel_id = false;
    }
  
  if (req->fields.channel_close != NULL)
    {
      int p;
      for (p=0; p < num_cids; p++)
        if ((strcmp(req->fields.channel_close,"*") == 0) ||
            (strstr(req->fields.channel_close,cids[p].channel_id) != NULL))
          cids[p].close_when_inactive = true; // Just mark them for closure
             // here.  Later, the `run' function will check for channels
             // which are idle or become idle with this flag set and close
             // them.
    }

  chn->request_processed = true;
  total_received_requests++;
  return true;
}

/*****************************************************************************/
/*                 kd_source_thread::process_cache_specifiers                */
/*****************************************************************************/

bool
  kd_source_thread::process_cache_specifiers(const char *string,
                                             kdu_window_model &model,
                                             bool need)
{
  while (*string != '\0')
    { 
      while (*string == ',') string++;
      if (*string == '[')
        { 
          string++;
          do { 
              int smin=-1, smax=-1; // Code-stream ID range
              if (!read_val_range_or_wild(string,smin,smax))
                return false;
              if (*string == ';')
                string++;
              else if (*string != ']')
                return false;
              if (smin > smax)
                return false;
              model.set_codestream_context(smin,smax);
            } while (*string != ']');
          string++;
          continue;
        }
      
      bool subtractive = need;
      if (*string == '-')
        {
          if (need)
            return false; // Can't have negative descriptors in "need="
          string++;
          subtractive = true;
        }
      
      int cls=-1; // Data-bin class code
      int tmin, tmax, cmin, cmax, rmin, rmax;
      kdu_long pmin, pmax, bin_id=-1;
      
      if (*string == 'H')
        {
          string++;
          if (*string == 'm')
            {
              string++;
              cls = KDU_MAIN_HEADER_DATABIN;
              bin_id = 0;
            }
          else
            {
              cls = KDU_TILE_HEADER_DATABIN;
              if (!read_val_or_wild(string,bin_id))
                return false;
            }
        }
      else if (*string == 'P')
        {
          string++;
          cls = KDU_PRECINCT_DATABIN;
          if (!read_val_or_wild(string,bin_id))
            return false;
        }
      else if (*string == 'M')
        {
          string++;
          cls = KDU_META_DATABIN;
          if (!read_val_or_wild(string,bin_id))
            return false;
        }
      else if (*string == 'T')
        {
          string++;
          cls = KDU_TILE_DATABIN;
          if (!read_val_or_wild(string,bin_id))
            return false;
        }
      else if ((*string=='t')||(*string=='c')||(*string=='r')||(*string=='p'))
        { // Scan for an implicit descriptor
          tmin=tmax=cmin=cmax=rmin=rmax=-1; pmin=pmax=-1;
          while (1)
            {
              if (*string=='t')
                {
                  string++;
                  if (!read_val_range_or_wild(string,tmin,tmax))
                    return false;
                }
              else if (*string=='c')
                {
                  string++;
                  if (!read_val_range_or_wild(string,cmin,cmax))
                    return false;
                }
              else if (*string=='r')
                {
                  string++;
                  if (!read_val_range_or_wild(string,rmin,rmax))
                    return false;
                }
              else if (*string=='p')
                {
                  string++;
                  if (!read_val_range_or_wild(string,pmin,pmax))
                    return false;
                }
              else
                break;
            }
          if ((tmin > tmax) || (cmin > cmax) || (rmin > rmax) || (pmin > pmax))
            return false;
        }
      else
        return false; // Unrecognized cache descriptor
      
      int flags=(subtractive)?KDU_WINDOW_MODEL_SUBTRACTIVE:0;
      int limit=(subtractive)?0:-1;
      if (*string == ':')
        { 
          string++;
          if (*string == 'L')
            { 
              flags |= KDU_WINDOW_MODEL_LAYERS;
              string++;
            }
          if (!read_val_or_wild(string,limit))
            return false;
          if (limit < 0)
            return false; // Wilds not allowed for qualifying byte/layer limits
        }
      if (cls < 0)
        model.add_instruction(tmin,tmax,cmin,cmax,rmin,rmax,pmin,pmax,
                              flags,limit);
      else
        model.add_instruction(cls,bin_id,flags,limit);
    }
  
  return true;
}

/*****************************************************************************/
/*                  kd_source_thread::generate_error_reply                   */
/*****************************************************************************/

void kd_source_thread::generate_error_reply(kds_primary_channel *chn,
                                            const kd_request *req)
{
  kds_cid *cid = chn->request_cid;
  if (cid != NULL)
    {
      cid->close_when_inactive = true;
      if (cid->active_chn == chn)
        cid->active_chn = NULL;
      chn->request_cid = NULL;
    }
  chn->request_processed = chn->reply_generated = false;
  assert(replies.get_remaining_bytes() > 0);
  bool close_connection =
    (req!=NULL)?req->close_connection:chn->release_channel_after_response;
  if (close_connection)
    replies << "Connection: close\r\n";
  replies << "Server: Kakadu JPIP Server "
          << KDU_CORE_VERSION << "\r\n\r\n";
  chn->channel->push_reply(replies,0);
  if ((req != NULL) && (req == chn->pending_requests.peek_head()))
    chn->pending_requests.pop_head();
  if (close_connection)
    {
      chn->channel->set_timeout(max_completion_seconds);
      release_primary_channel(chn,true);
    }
  else
    recycle_primary_channel(chn);
}

/*****************************************************************************/
/*                kd_source_thread::handle_unsupported_prefs                 */
/*****************************************************************************/

bool kd_source_thread::handle_unsupported_prefs(kds_primary_channel *chn,
                                                const kd_request *req)
{
  // First mask off the required preferences that we cannot support
  int required = chn->window_prefs.required;
  required &= (KDU_PLACEHOLDER_PREF_ORIG | KDU_PLACEHOLDER_PREF_EQUIV |
               KDU_COLOUR_METH_PREF | KDU_CONTRAST_SENSITIVITY_PREF);
  if (!required)
    return false;
  
  chn->window_prefs.required = required;
  chn->window_prefs.preferred = 0;
  int buf_size = chn->window_prefs.write_prefs(NULL);
  char *reply_buf = new char[buf_size+1];
  chn->window_prefs.write_prefs(reply_buf);
  
  kds_cid *cid = chn->request_cid;
  if (cid != NULL)
    {
      if (cid->active_chn == chn)
        cid->active_chn = NULL;
      chn->request_cid = NULL;
    }
  chn->request_processed = chn->reply_generated = false;
  replies.restart();
  replies << "HTTP/1.1 501 Request requires unsupported behaviour\r\n";
  replies << "JPIP-" JPIP_FIELD_PREFERENCES ": " << reply_buf << "\r\n";
  bool close_connection =
    (req!=NULL)?req->close_connection:chn->release_channel_after_response;
  if (close_connection)
    replies << "Connection: close\r\n";
  replies << "Server: Kakadu JPIP Server "
          << KDU_CORE_VERSION << "\r\n\r\n";
  delete[] reply_buf;
  
  chn->channel->push_reply(replies,0);
  if ((req != NULL) && (req == chn->pending_requests.peek_head()))
    chn->pending_requests.pop_head();
  if (close_connection)
    {
      chn->channel->set_timeout(max_completion_seconds);
      release_primary_channel(chn,true);
    }
  return true;
}

/*****************************************************************************/
/*                    kd_source_thread::preempt_request                      */
/*****************************************************************************/

void kd_source_thread::preempt_request(kds_cid *cid)
{
  assert(cid->is_active &&
         (!is_stateless) && // We don't pre-empt stateless requests
         (cid->active_chunks==NULL)); // We don't pre-empt request while
                                      // waiting for an auxiliary channel
  if ((cid->active_chn != NULL) && !cid->active_chn->reply_generated)
    {
      kds_primary_channel *chn = cid->active_chn;
      assert(chn->request_processed);
      replies.restart();
      replies << "HTTP/1.1 202 pre-empted\r\n";
      if (chn->reply_needs_target_id)
        replies << "JPIP-" JPIP_FIELD_TARGET_ID ": "
                << source->target_id << "\r\n";
      if (chn->reply_needs_jpip_channel_id)
        {
          assert(!is_stateless);
          replies << "JPIP-" JPIP_FIELD_CHANNEL_NEW ": "
                  << "cid=" << cid->channel_id
                  << ",path=" KD_MY_RESOURCE_NAME;
          if (cid->channel_type == KD_CHANNEL_HTTP)
            replies << ",transport=http";
          else
            replies << ",transport=http-tcp,auxport=" << aux_listen_port;
          replies << "\r\n";
        }
      replies << "Cache-Control: no-cache\r\n";
      if (chn->release_channel_after_response)
        replies << "Connection: close\r\n";
      replies << "\r\n";
      chn->channel->push_reply(replies,0);
      detach_active_chn(cid);
    }
  else if (cid->active_chn != NULL)
    {
      kds_primary_channel *chn = cid->active_chn;
      assert(cid->is_active && (chn->request_cid == cid) &&
             (cid->channel_type != KD_CHANNEL_HTTP_TCP));
      if (cid->channel_type != KD_CHANNEL_HTTP_TCP)
        chn->channel->terminate_chunked_data(JPIP_EOR_WINDOW_CHANGE);
      detach_active_chn(cid);      
    }
  if (cid->aux_channel != NULL)
    {
      assert(!cid->response_complete);
      cid->aux_channel->terminate_chunked_data(JPIP_EOR_WINDOW_CHANGE);
    }
  cid->is_active = false;
}

/*****************************************************************************/
/*                    kd_source_thread::detach_active_chn                    */
/*****************************************************************************/

void kd_source_thread::detach_active_chn(kds_cid *cid)
{
  kds_primary_channel *chn = cid->active_chn;
  cid->active_chn = NULL;
  if (chn != NULL)
    {
      chn->request_cid = NULL;
      chn->request_processed = chn->reply_generated = false;
      if (chn->release_channel_after_response)
        {
          chn->channel->set_timeout(max_completion_seconds);
          release_primary_channel(chn,true);
        }
    }
}

/*****************************************************************************/
/*                       kd_source_thread::close_cid                         */
/*****************************************************************************/

void kd_source_thread::close_cid(int which, kdu_byte eor_code)
{
  assert((which >= 0) && (which < num_cids));
  kds_cid *cid = cids + which;
  
  if (cid->is_active)
    {
      if (!(is_stateless || cid->active_model->is_empty()))
        { // Need to pass cache model information along
          cid->active_window->init();
          set_window(*(cid->active_window),cid->active_prefs,cid->active_model,
                     is_stateless,cid->window_id);
          cid->active_model->clear();
        }
      assert(cid->close_immediately);
      kds_primary_channel *chn = cid->active_chn;
      if (chn != NULL)
        {
          if (!chn->reply_generated)
            {
              assert(chn->request_processed);
              replies.restart();
              if (cid->aux_timeout > 0)
                replies << "HTTP/1.1 503 Server is unwilling to wait any "
                           "longer for attachment of auxiliary TCP channel "
                           "to complete HTTP-TCP transport connection; "
                           "closing down the JPIP channel.\r\n";
              else if (cid->qid_timeout > 0)
                replies << "HTTP/1.1 503 Server is unwilling to wait any "
                           "longer for arrival of request with an "
                           "outstanding QID value; closing down the JPIP "
                           "channel.\r\n";
              else
                replies << "HTTP/1/1 503 Closing down JPIP channel, "
                           "probably due to expiration of a Session time "
                           "limit.\r\n";
              replies << "Cache-Control: no-cache\r\n";
              if (chn->release_channel_after_response)
                replies << "Connection: close\r\n";
              chn->channel->push_reply(replies,0);
              detach_active_chn(cid);
            }
          else
            { // Must be in middle of HTTP-only request
              assert(cid->channel_type != KD_CHANNEL_HTTP_TCP);
              if (!cid->response_complete)
                chn->channel->terminate_chunked_data(eor_code);
              detach_active_chn(cid);
            }
        }
      if (cid->active_chunks != NULL)
        {
          if (cid->aux_channel != NULL)
            cid->active_chunks =
              cid->aux_channel->push_chunks(cid->active_chunks,NULL);
          else
            {
              kds_chunk *chnk;
              for (chnk=cid->active_chunks; chnk != NULL; chnk=chnk->next)
                chnk->abandoned = true;
            }
          if ((cid->active_chunks != NULL) && !finished)
            release_chunks(cid->active_chunks,true);
          cid->active_chunks = NULL;
        }
      if ((cid->aux_channel != NULL) && !cid->response_complete)
        cid->aux_channel->terminate_chunked_data(eor_code);
      cid->is_active = cid->response_complete = false;
    }

  assert((cid->active_chn == NULL) && !cid->is_active);
  if (cid->aux_channel != NULL)
    {
      cid->aux_channel->set_timeout(max_completion_seconds);
      release_auxiliary_channel(cid,true);
    }
  if (cid->active_chunks != NULL)
    release_chunks(cid->active_chunks,true);
  cid->active_chunks = NULL;
  
  // Shuffle the remaining channels down; need to
  // lock the manager's mutex for this
  manager->mutex.lock();
  int p;
  num_cids--;
  cids[which].init();
  for (p=which+1; p <= num_cids; p++)
    cids[p-1] = cids[p];
  cids[num_cids].active_window = NULL;
  cids[num_cids].active_model = NULL;
  cids[num_cids].active_prefs = NULL;
  cids[num_cids].init();
  manager->mutex.unlock();
  
  // Fix up references into the `cids' array from the primary channels list
  kds_primary_channel *chn;
  for (chn=primary_channels; chn != NULL; chn=chn->next)
    if (chn->request_cid != NULL)
      {
        if (chn->request_cid == cid)
          {
            assert(!chn->request_processed);
            chn->request_cid = NULL; // So call below does not try to access us
            recycle_primary_channel(chn); // Return to connection manager,
               // which will generate the error when it discovers that this
               // request does not correspond to a valid session's channel-ID.
          }
        else if ((p = (int)(chn->request_cid-cids)) > which)
          {
            assert(p <= num_cids);
            chn->request_cid--;
          }
      }
  
  // Fix up flow control members
  normalize_channel_bandwidths();
}

/*****************************************************************************/
/*                  kd_source_thread::generate_data_chunks                   */
/*****************************************************************************/

bool kd_source_thread::generate_data_chunks(kds_cid *cid, int suggested_bytes)
{
  if ((cid->active_chunks!=NULL) || cid->response_complete || !cid->is_active)
    return false; // Should not happen; treat these as error conditions.

  bool need_set_window =
    ((cid->active_chn != NULL) && !cid->active_chn->reply_generated);
    
  bool result = true;
  mutex.unlock();
  try {
    if (need_set_window)
      { 
        set_window(*(cid->active_window),cid->active_prefs,cid->active_model,
                   is_stateless,cid->window_id);
        cid->active_model->clear();
      }
    bool prohibit_response_data = ((cid->active_remaining_bytes == 0) &&
                                   (cid->channel_type != KD_CHANNEL_HTTP_TCP));
    if ((suggested_bytes < cid->active_remaining_bytes) &&
        (suggested_bytes > (cid->active_remaining_bytes-(suggested_bytes>>1))))
      suggested_bytes += (suggested_bytes>>1); // Avoid leaving a very small
                                               // number of bytes behind
    cid->active_chunks =
      generate_increments(suggested_bytes,cid->active_remaining_bytes,
                          cid->active_align,cid->active_extended_headers,
                          false,cid->window_id);
    assert(cid->active_chunks != NULL);
    bool window_done = !get_window(*(cid->active_window),cid->window_id);
    if (prohibit_response_data)
      {
        assert((cid->active_chunks->next == NULL) &&
               (cid->active_chunks->num_bytes <=
                cid->active_chunks->prefix_bytes));
        cid->response_complete = true;
        release_chunks(cid->active_chunks);
        cid->active_chunks = NULL;
      }
    else if (window_done || (cid->active_remaining_bytes <= 0))
      { // Append an EOR message
        kdu_byte eor_code = 0;
        if (get_image_done())
          eor_code = JPIP_EOR_IMAGE_DONE;
        else if (window_done)
          eor_code = JPIP_EOR_WINDOW_DONE;
        else
          {
            eor_code = JPIP_EOR_BYTE_LIMIT_REACHED;
            if ((cid->active_chunks->num_bytes ==
                 cid->active_chunks->prefix_bytes) &&
                (cid->active_chn != NULL) &&
                !cid->active_chn->reply_generated)
              { // Requested byte length must have been too small
                assert(cid->active_chunks->next == NULL);
                cid->active_chn->reply_needs_min_len = true;
              }
          }
        kdu_byte buf[3] = {0,eor_code,0};
        push_extra_data(buf,3,cid->active_chunks);
        cid->response_complete = true;
      }
  }
  catch (...) {
    result = false;
  }
  mutex.lock();
  return result;
}

/*****************************************************************************/
/*                      kd_source_thread::generate_reply                     */
/*****************************************************************************/

void kd_source_thread::generate_reply(kds_cid *cid)
{
  kds_primary_channel *chn = cid->active_chn;
  assert(chn != NULL);
  replies.restart();
  replies << "HTTP/1.1 200 ";
  if (chn->window.equals(*(cid->active_window)))
    replies << "OK\r\n";
  else
    replies << "OK, with modifications\r\n";
  
  if (chn->reply_needs_target_id)
    replies << "JPIP-" JPIP_FIELD_TARGET_ID ": "
            << source->target_id << "\r\n";
  
  if (chn->reply_needs_jpip_channel_id)
    {
      assert(!is_stateless);
      replies << "JPIP-" JPIP_FIELD_CHANNEL_NEW ": "
              << "cid=" << cid->channel_id
              << ",path=" KD_MY_RESOURCE_NAME;
      if (cid->channel_type == KD_CHANNEL_HTTP)
        replies << ",transport=http";
      else
        replies << ",transport=http-tcp,auxport=" << aux_listen_port;
      replies << "\r\n";
    }
  
  if (chn->request_qid >= 0)
    replies << "JPIP-" JPIP_FIELD_REQUEST_ID ":"
            << (kdu_uint32) chn->request_qid << "\r\n"; 
    
  if (chn->window.max_layers != cid->active_window->max_layers)
    replies << "JPIP-" JPIP_FIELD_LAYERS ": "
            << cid->active_window->max_layers << "\r\n";
  
  if (((chn->window.resolution != cid->active_window->resolution) &&
       (chn->window.resolution.x != 0) &&
       (chn->window.resolution.y != 0)) ||
      (chn->reply_roi_index > 0))
    replies << "JPIP-" JPIP_FIELD_FULL_SIZE ": "
            << cid->active_window->resolution.x << ","
            << cid->active_window->resolution.y << "\r\n";
  if (((chn->window.region.pos != cid->active_window->region.pos) &&
       (chn->window.resolution.x != 0) &&
       (chn->window.resolution.y != 0)) ||
      (chn->reply_roi_index > 0))
    replies << "JPIP-" JPIP_FIELD_REGION_OFFSET ": "
            << cid->active_window->region.pos.x << ","
            << cid->active_window->region.pos.y << "\r\n";
  if (((chn->window.region.size != cid->active_window->region.size) &&
       !chn->window.region.is_empty()) ||
      (chn->reply_roi_index > 0))
    replies << "JPIP-" JPIP_FIELD_REGION_SIZE ": "
            << cid->active_window->region.size.x << ","
            << cid->active_window->region.size.y << "\r\n";
  if (chn->reply_needs_roi_index)
    {
      if (chn->reply_roi_index > 0)
        {
          kdu_coords res;
          kdu_dims reg;
          const char *roi_name =
            source->serve_target.get_roi_details(chn->reply_roi_index,res,reg);
          assert(roi_name != NULL);
          replies << "JPIP-" JPIP_FIELD_ROI ": roi=" << roi_name
                  << ";fsiz=" << res.x << "," << res.y
                  << ";rsiz=" << reg.size.x << "," << reg.size.y
                  << ";roff=" << reg.pos.x << "," << reg.pos.y << "\r\n";
        }
      else
        replies << "JPIP-" JPIP_FIELD_ROI ": roi=no-roi\r\n";
    }
  if (chn->reply_needs_quality_value)
    replies << "JPIP-" JPIP_FIELD_MAX_QUALITY ": -1\r\n";
  if (chn->reply_needs_min_len)
    replies << "JPIP-" JPIP_FIELD_MAX_LENGTH ": 100\r\n";
  
  if ((chn->window.components != cid->active_window->components) &&
      !chn->window.components.is_empty())
    {
      kdu_sampled_range *rg;
      replies << "JPIP-" JPIP_FIELD_COMPONENTS ": ";
      for (int cn=0;
           (rg = cid->active_window->components.access_range(cn)) != NULL;
           cn++)
        {
          if (cn > 0)
            replies << ",";
          replies << rg->from;
          if (rg->to > rg->from)
            replies << "-" << rg->to;
        }
      replies << "\r\n";
    }
  
  bool need_codestream_header =
    (chn->window.codestreams != cid->active_window->codestreams);
  if (!(need_codestream_header || cid->active_window->contexts.is_empty()))
    { // See if any translated codestreams exist
      kdu_sampled_range *rg;
      for (int cn=0;
           (rg = cid->active_window->codestreams.access_range(cn)) != NULL;
           cn++)
        if (rg->context_type == KDU_JPIP_CONTEXT_TRANSLATED)
          { need_codestream_header = true; break; }
    }
  if (need_codestream_header)
    {
      kdu_sampled_range *rg;
      replies << "JPIP-" JPIP_FIELD_CODESTREAMS ": ";
      for (int cn=0;
           (rg = cid->active_window->codestreams.access_range(cn)) != NULL;
           cn++)
        {
          if (cn > 0)
            replies << ",";
          if (rg->context_type == KDU_JPIP_CONTEXT_TRANSLATED)
            replies << "<" << rg->remapping_ids[0]
                    << ":" << rg->remapping_ids[1] << ">";
          replies << rg->from;
          if (rg->to > rg->from)
            replies << "-" << rg->to;
          if ((rg->step > 1) && (rg->to > rg->from))
            replies << ":" << rg->step;
        }
      replies << "\r\n";
    }
  
  if (!cid->active_window->contexts.is_empty())
    {
      kdu_sampled_range *rg;
      bool range_written= false;
      for (int cn=0;
           (rg = cid->active_window->contexts.access_range(cn)) != NULL;
           cn++)
        {
          if ((rg->context_type != KDU_JPIP_CONTEXT_JPXL) &&
              (rg->context_type != KDU_JPIP_CONTEXT_MJ2T))
            continue;
          if (rg->expansion == NULL)
            continue;
          if (!range_written)
            replies << "JPIP-" JPIP_FIELD_CONTEXTS ": ";
          else
            replies << ";";
          range_written = true;
          if (rg->context_type == KDU_JPIP_CONTEXT_JPXL)
            replies << "jpxl";
          else if (rg->context_type == KDU_JPIP_CONTEXT_MJ2T)
            replies << "mj2t";
          else
            assert(0);
          replies << "<" << rg->from;
          if (rg->to > rg->from)
            replies << "-" << rg->to;
          if ((rg->step > 1) && (rg->to > rg->from))
            replies << ":" << rg->step;
          if ((rg->context_type == KDU_JPIP_CONTEXT_MJ2T) &&
              (rg->remapping_ids[1] == 0))
            replies << "+now";
          replies << ">";
          if (rg->context_type == KDU_JPIP_CONTEXT_JPXL)
            {
              if ((rg->remapping_ids[0] >= 0) && (rg->remapping_ids[1] >= 0))
                replies << "[s" << rg->remapping_ids[0]
                        << "i" << rg->remapping_ids[1] << "]";
            }
          else if (rg->context_type == KDU_JPIP_CONTEXT_MJ2T)
            {
              if (rg->remapping_ids[0] == 0)
                replies << "[track]";
              else if (rg->remapping_ids[0] == 1)
                replies << "[movie]";
            }
          replies << "=";
          kdu_range_set *expansion = rg->expansion;
          for (int en=0; (rg = expansion->access_range(en)) != NULL; en++)
            {
              if (en > 0)
                replies << ",";
              replies << rg->from;
              if (rg->to > rg->from)
                replies << "-" << rg->to;
              if ((rg->step > 1) && (rg->to > rg->from))
                replies << ":" << rg->step;
            }
        }
      if (range_written)
        replies << "\r\n";
    }
  
  if (!is_stateless)
    replies << "Cache-Control: no-cache\r\n";
  if (chn->release_channel_after_response)
    replies << "Connection: close\r\n";
  replies << "\r\n";
  
  if ((cid->active_chunks == NULL) ||
      (cid->channel_type == KD_CHANNEL_HTTP_TCP))
    chn->channel->push_reply(replies,0);
  else
    chn->channel->push_reply(replies,-1);
  
  chn->reply_generated = true;
  total_serviced_requests++;
  
  if ((cid->channel_type == KD_CHANNEL_HTTP_TCP) || (cid->active_chunks==NULL))
    detach_active_chn(cid);
}

/*****************************************************************************/
/*              kd_source_thread::normalize_channel_bandwidths               */
/*****************************************************************************/

void kd_source_thread::normalize_channel_bandwidths()
{
  if (num_cids == 0)
    return;
  int p;
  float slice_sum = 0.0F;
  for (p=0; p < num_cids; p++)
    {
      if (cids[p].pref_bw_slice < 1)
        cids[p].pref_bw_slice = 1; // Just a precaution
      slice_sum += (float) cids[p].pref_bw_slice;
    }
  float factor = 1.0F / slice_sum;
  for (p=0; p < num_cids; p++)
    cids[p].bw_slice = factor * (float) cids[p].pref_bw_slice;
  if (pref_max_bytes_per_second > 0.0F)
    for (p=0; p < num_cids; p++)
      if (cids[p].aux_channel != NULL)
        cids[p].aux_channel->set_bandwidth_limit(pref_max_bytes_per_second *
                                                cids[p].bw_slice);
}

/*****************************************************************************/
/*                   kd_source_thread::update_flow_control                   */
/*****************************************************************************/

int kd_source_thread::update_flow_control()
{
  if (num_cids == 0)
    return 0;
  int p;
  float total_fulness = 0.0F;
  for (p=0; p < num_cids; p++)
    {
      kds_cid *cid = cids + p;
      total_fulness += cid->bucket_fulness;
      if (cid->bucket_threshold < 1.0F)
        cid->bucket_threshold = 1.0F; // Without this, the algorithm below may
           // fail to satisfy the policy described with the declaration of
           // this function.
    }
  
  int num_active_cids = 0;
  kds_cid *best_active_channel = NULL;
  float best_active_channel_delta = 0.0F;
  float total_active_slice = 0.0F;
  float total_inactive_slack = 0.0F;
  float total_inactive_slice = 0.0F;
  float total_inactive_adjustment = 0.0F;
  for (p=0; p < num_cids; p++)
    {
      kds_cid *cid = cids + p;
      float delta = total_fulness * cid->bw_slice;
      cid->bucket_fulness -= delta;
      if (cid->is_active)
        {
          num_active_cids++;
          total_active_slice += cid->bw_slice;
          delta = cid->bucket_fulness+0.5F-cid->bucket_threshold;
          delta /= cid->bw_slice;
          if ((best_active_channel == NULL) ||
              (delta < best_active_channel_delta))
            {
              best_active_channel = cid;
              best_active_channel_delta = delta;
            }
        }
      else
        {
          total_inactive_slice += cid->bw_slice;
          if ((delta = cid->bucket_fulness) < 0.0F)
            total_inactive_slack -= delta;
          else if ((delta = delta+0.5F-cid->bucket_threshold) > 0.0F)
            total_inactive_adjustment += delta;
        }
    }
  if (best_active_channel != NULL)
    { // There are some active channels
      if (best_active_channel_delta > 0.0F)
        { // Need some adjustments
          assert(total_inactive_slack > 0.0F);
          float slack_fraction = (best_active_channel_delta *
                                  total_active_slice) / total_inactive_slack;
          assert(slack_fraction <= 1.0F);
          for (p=0; p < num_cids; p++)
            {
              kds_cid *cid = cids + p;
              float delta = cid->bucket_fulness;
              if (cid->is_active)
                {
                  assert((delta+0.5F-cid->bucket_threshold) > 0.0F);
                  cid->bucket_fulness -=
                    best_active_channel_delta * cid->bw_slice;
                }
              else if (delta < 0.0F)
                cid->bucket_fulness -= delta * slack_fraction;
            }
        }
    }
  else if (total_inactive_adjustment > 0.0F)
    { // All channels are inactive
      assert(total_inactive_adjustment <= total_inactive_slack);
      float slack_fraction = total_inactive_adjustment / total_inactive_slack;
      for (p=0; p < num_cids; p++)
        {
          kds_cid *cid = cids + p;
          assert(!cid->is_active);
          float delta = cid->bucket_fulness;
          if (delta < 0.0F)
            cid->bucket_fulness -= delta * slack_fraction;
          else if ((delta = delta+0.5F-cid->bucket_threshold) > 0.0F)
            cid->bucket_fulness -= delta;
        }
    }
  
  return num_active_cids;
}

/*****************************************************************************/
/*                 kd_source_thread::release_primary_channel                 */
/*****************************************************************************/

void kd_source_thread::release_primary_channel(kds_primary_channel *chn,
                                               bool send_queued_data_first)
{
  kds_primary_channel *prev=NULL;
  if (chn != primary_channels)
    {
      for (prev=primary_channels;
           (prev != NULL) && (prev->next != chn);
           prev = prev->next);
      assert((prev != NULL) && (prev->next == chn));
      chn = prev->next; // Safety precaution
    }
  if (chn->channel != NULL)
    {
      chn->channel->close(send_queued_data_first);
      if (send_queued_data_first)
        chn->channel->close(false);
      this->total_transmitted_bytes +=
        (chn->channel->get_total_transmitted_bytes() -
         chn->initial_transmitted_bytes);
      chn->channel->release();
      chn->channel = NULL;
    }
  if (prev == NULL)
    primary_channels = chn->next;
  else
    prev->next = chn->next;
  kds_cid *active_cid = chn->request_cid;
  if ((active_cid != NULL) && (!finished) && (active_cid >= cids) &&
      (active_cid < (cids+num_cids)) && (active_cid->active_chn == chn))
    { // Still waiting to generate a reply on this channel which is dead
      active_cid->is_active = false;
      active_cid->active_chn = NULL;
    }
  delete chn;
  num_primary_channels--;
}

/*****************************************************************************/
/*                 kd_source_thread::recycle_primary_channel                 */
/*****************************************************************************/

void kd_source_thread::recycle_primary_channel(kds_primary_channel *chn)
{
  kds_primary_channel *prev=NULL;
  if (chn != primary_channels)
    {
      for (prev=primary_channels;
           (prev != NULL) && (prev->next != chn);
           prev = prev->next);
      assert((prev != NULL) && (prev->next == chn));
      chn = prev->next; // Safety precaution
    }
  if (chn->channel != NULL)
    {
      this->total_transmitted_bytes +=
        (chn->channel->get_total_transmitted_bytes(true) -
         chn->initial_transmitted_bytes);
      chn->channel->return_requests(&(chn->pending_requests));
      cserver->return_channel(chn->channel);
      chn->channel = NULL;
    }  
  if (prev == NULL)
    primary_channels = chn->next;
  else
    prev->next = chn->next;
  delete chn;
  num_primary_channels--;
}

/*****************************************************************************/
/*                 kd_source_thread::release_auxiliary_channel               */
/*****************************************************************************/

void kd_source_thread::release_auxiliary_channel(kds_cid *cid,
                                                 bool send_queued_data_first)
{
  if (cid->aux_channel == NULL)
    return;
  cid->aux_channel->close(send_queued_data_first);
  if (send_queued_data_first)
    cid->aux_channel->close(false);
  kds_chunk *chunks;
  if ((chunks=cid->aux_channel->retrieve_completed_chunks()) != NULL)
    release_chunks(chunks);
  kdu_long num_bytes = cid->aux_channel->get_total_transmitted_bytes();
  this->total_transmitted_bytes += num_bytes;
  this->auxiliary_channel_transmitted_bytes += num_bytes;
  this->total_unacknowledged_bytes +=
    (num_bytes - cid->aux_channel->get_num_acknowledged_bytes());
  this->total_rtt_events +=
    cid->aux_channel->get_total_rtt_events();
  this->total_rtt_seconds +=
    cid->aux_channel->get_total_rtt_seconds();
  this->auxiliary_channel_active_seconds += (float)
    cid->aux_channel->get_active_seconds();
  cid->aux_channel->release();
  cid->aux_channel = NULL;
}


/* ========================================================================= */
/*                             kd_source_manager                             */
/* ========================================================================= */

/*****************************************************************************/
/*                  kd_source_manager::kd_source_manager                     */
/*****************************************************************************/

kd_source_manager::kd_source_manager(kdu_uint16 aux_listen_port,
                                     int max_sources,
                                     int max_threads,
                                     int max_cids_per_client,
                                     int max_chunk_bytes,
                                     bool ignore_relevance_info,
                                     int phld_threshold,
                                     int per_client_cache,
                                     int max_history_records,
                                     float max_session_idle_seconds,
                                     float max_completion_seconds,
                                     float max_establishment_seconds,
                                     float min_rtt_target,
                                     float max_rtt_target,
                                     const char *password,
                                     const char *cache_directory,
                                     bool caseless_targets)
{
  mutex.create();
  event.create(true); // Manual reset even
  get_sys_address(sys_address);
  
  this->aux_listen_port = aux_listen_port;
  this->max_chunk_bytes = max_chunk_bytes;
  this->ignore_relevance_info = ignore_relevance_info;
  this->phld_threshold = phld_threshold;
  this->per_client_cache = per_client_cache;
  this->max_per_client_cids = max_cids_per_client;
  this->max_session_idle_seconds = max_session_idle_seconds;
  this->max_completion_seconds = max_completion_seconds;
  this->max_establishment_seconds = max_establishment_seconds;
  this->min_rtt_target = min_rtt_target;
  this->max_rtt_target = max_rtt_target;
  shutdown_requested = false;
  this->cache_directory = cache_directory;
  this->caseless_targets = caseless_targets;
  
  this->max_sources = max_sources;
  num_sources = 0;
  sources = NULL;
  this->max_threads = max_threads;
  num_threads = 0;
  dead_threads = NULL;
  
  total_clients = 0;
  total_transmitted_bytes = 0;
  completed_connections = terminated_connections = 0;
  num_history_records = 0;
  this->max_history_records = max_history_records;  
  history_head = history_tail = NULL;
  memset(admin_id,0,16);
  if (password != NULL)
    admin_cipher.set_key(password);
}

/*****************************************************************************/
/*                    kd_source_manager::~kd_source_manager                  */
/*****************************************************************************/

kd_source_manager::~kd_source_manager()
{
  mutex.lock();
  max_threads = max_sources = 0; // Prevent new threads being started
  while (num_threads > 0)
    event.wait(mutex);
  mutex.unlock();
  assert(num_sources == 0);
  delete_dead_threads();
  mutex.destroy();
  event.destroy();
  while ((history_tail = history_head) != NULL)
    {
      history_head = history_tail->next;
      delete history_tail;
    }
}

/*****************************************************************************/
/*                   kd_source_manager::create_source_thread                 */
/*****************************************************************************/

bool kd_source_manager::create_source_thread(kd_target_description &target,
                                             const char *cnew_string,
                                             kd_connection_server *cserver,
                                             char channel_id[],
                                             kdcs_message_block &explanation)
{
  // Find out what type of channel we need initially
  kd_jpip_channel_type jpip_channel_type =
    kd_source_thread::translate_cnew_string(cnew_string);
  
  mutex.lock();
  
  if (shutdown_requested)
    {
      explanation << "503 Server is currently shutting down.\r\n";
      mutex.unlock();
      return false;
    }
  
  // Create the source
  kd_source *src;
  if (num_threads >= max_threads)
    {
      explanation << "503 Too many clients currently connected to server.\r\n";
      mutex.unlock();
      return false;
    }
  if (caseless_targets)
    {
      for (src=sources; src != NULL; src=src->next)
        if (kdcs_caseless_compare(src->target.filename,target.filename) &&
            (src->target.range_start == target.range_start) &&
            (src->target.range_lim == target.range_lim))
          break;
    }
  else
    {
      for (src=sources; src != NULL; src=src->next)
        if ((strcmp(src->target.filename,target.filename) == 0) &&
            (src->target.range_start == target.range_start) &&
            (src->target.range_lim == target.range_lim))
          break;
    }
  if (src == NULL)
    { // Need to open the image
      if (num_sources >= max_sources)
        {
          explanation << "503 Too many sources currently open on server.\r\n";
          mutex.unlock();
          return false;
        }
      src = new kd_source;
      if (!src->open(target,caseless_targets,explanation,phld_threshold,
                     per_client_cache,cache_directory,sys_address))
        {
          delete src;
          mutex.unlock();
          return false;
        }
      
      // Create the unique source identifier
      src->source_id =
        ((kdu_uint32 *)(&src))[0]; // Start with 32 LSB's of address
      kd_source *sscan;
      do {
        src->source_id ^= (kdu_uint32) rand();
        src->source_id ^= (kdu_uint32)(rand() << 16);
        for (sscan=sources; sscan != NULL; sscan=sscan->next)
          if (sscan->source_id == src->source_id)
            break;
      } while (sscan != NULL);
      
      // Link into the manager's list of active sources
      src->manager = this; // Link the new source in.
      src->next = sources;
      if (sources != NULL)
        sources->prev = src;
      sources = src;
      num_sources++;
    }
  
  // Create the source thread
  kd_source_thread *thrd =
    new kd_source_thread(src,this,cserver,aux_listen_port,max_per_client_cids,
                         max_chunk_bytes,ignore_relevance_info,
                         max_session_idle_seconds,max_completion_seconds,
                         max_establishment_seconds,caseless_targets);
  thrd->next = src->threads;
  if (src->threads != NULL)
    src->threads->prev = thrd;
  src->threads = thrd;
  num_threads++;
  total_clients++;
  mutex.unlock();
  
  // Assign a new channel-ID
  const char *new_channel_id = thrd->assign_new_cid(jpip_channel_type);
  assert(new_channel_id != NULL);
  memcpy(channel_id,new_channel_id,KD_CHANNEL_ID_LEN+1);
  thrd->start();
  return true;
}

/*****************************************************************************/
/*              kd_source_manager::install_stateless_channel                 */
/*****************************************************************************/

bool
  kd_source_manager::install_stateless_channel(kd_target_description &target,
                                               kds_jpip_channel *channel,
                                               kd_request_queue *xfer_queue)
{
  mutex.lock();
  
  if (shutdown_requested)
    {
      mutex.unlock();
      return false;
    }
  
  // Start by finding the source
  kd_source *src;
  for (src=sources; src != NULL; src=src->next)
    if ((strcmp(src->target.filename,target.filename) == 0) &&
        (src->target.range_start == target.range_start) &&
        (src->target.range_lim == target.range_lim))
      break;
  if (src == NULL)
    {
      mutex.unlock();
      return false;
    }
  
  // Next, find the best compatible source thread, if any
  int available_channels, max_available_channels=0;
  kd_source_thread *thrd, *best_thrd=NULL;
  for (thrd=src->threads; thrd != NULL; thrd=thrd->next)
    if ((available_channels = thrd->get_num_available_stateless_channels()) >
        max_available_channels)
      {
        max_available_channels = available_channels;
        best_thrd = thrd;
      }
  
  // Finally, install the channel
  bool success = false;
  if (best_thrd != NULL)
    {
      best_thrd->allow_dead_thread_delete = false;
      mutex.unlock();
      success = best_thrd->install_channel(channel,xfer_queue,NULL);
      mutex.lock();
      best_thrd->allow_dead_thread_delete = true;
    }
  mutex.unlock();
  return success;
}

/*****************************************************************************/
/*                    kd_source_manager::install_channel                     */
/*****************************************************************************/

bool kd_source_manager::install_channel(const char *channel_id, bool auxiliary,
                                        kds_jpip_channel *channel,
                                        kd_request_queue *transfer_queue,
                                        kdcs_message_block &explanation)
{
  int length = (int) strlen(channel_id);
  if (length <= 8)
    {
      explanation << "404 Request involves an invalid channel ID\r\n";
      return false;
    }
  kdu_uint32 source_id = read_hex_val32(channel_id+length-8);
  
  mutex.lock();
  
  if (shutdown_requested)
    {
      explanation << "503 Server is currently shutting down\r\n";
      mutex.unlock();
      return false;
    }
  
  kd_source *src;
  for (src=sources; src != NULL; src=src->next)
    if (src->source_id == source_id)
      break;
  if (src == NULL)
    {
      mutex.unlock();
      explanation << "503 Requested channel ID no longer exists\r\n";
      return false;
    }
  
  kd_source_thread *thrd;
  for (thrd=src->threads; thrd != NULL; thrd=thrd->next)
    if (thrd->find_channel_id(channel_id) >= 0)
      break;
  if (thrd == NULL)
    {
      mutex.unlock();
      explanation << "503 Requested channel ID no longer exists\r\n";
      return false;
    }
  thrd->allow_dead_thread_delete = false;
  mutex.unlock();
  if (auxiliary)
    channel->set_auxiliary(min_rtt_target,max_rtt_target);
  bool success =
    thrd->install_channel(channel,transfer_queue,(auxiliary)?channel_id:NULL);
  mutex.lock();
  thrd->allow_dead_thread_delete = true;
  mutex.unlock();
  
  if (!success)
    explanation << "409 Requested channel ID belongs to a session which "
                   "is already handling a full complement of connected "
                   "TCP channels or has just closed the JPIP channel "
                   "whose CID is supplied in this connection request.\r\n";
  return success;
}

/*****************************************************************************/
/*                    kd_source_manager::write_admin_id                      */
/*****************************************************************************/

bool kd_source_manager::write_admin_id(kdcs_message_block &block)
{
  if (!admin_cipher)
    return false;
  mutex.lock();
  admin_cipher.encrypt(admin_id);
  int i;
  for (i=0; i < 32; i++)
    {
      kdu_byte val = admin_id[i>>1];
      if ((i & 1) == 0)
        val >>= 4;
      val &= 0x0F;
      if (val < 10)
        block << (char)('0'+val);
      else
        block << (char)('A'+val-10);
    }
  mutex.unlock();
  return true;
}

/*****************************************************************************/
/*                   kd_source_manager::compare_admin_id                     */
/*****************************************************************************/

bool kd_source_manager::compare_admin_id(const char *string)
{
  if (!admin_cipher)
    return false;
  int i;
  bool matches = true;
  mutex.lock();
  admin_cipher.encrypt(admin_id);
  for (i=0; (i < 32) && matches; i++, string++)
    {
      kdu_byte val = admin_id[i>>1];
      if ((i & 1) == 0)
        val >>= 4;
      val &= 0x0F;
      if (((val < 10) && (*string != ('0'+val))) ||
          ((val >= 10) &&
           (*string != ('A'+val-10)) && (*string!= ('a'+val-10))))
        matches = false;
    }
    
  mutex.unlock();
  return matches;
}

/*****************************************************************************/
/*                      kd_source_manager::write_stats                       */
/*****************************************************************************/

void kd_source_manager::write_stats(kdcs_message_block &block)
{
  mutex.lock();
  block << "\nCURRENT STATS:\n";
  block << "    Current active sources = " << num_sources << "\n";
  block << "    Current active clients = " << num_threads << "\n";
  block << "    Total clients started = " << total_clients << "\n";
  block << "    Total clients successfully completed = "
        << completed_connections << "\n";
  block << "    Total clients terminated (channels not connected in time) = "
        << terminated_connections << "\n";
  block << "    Total bytes transmitted to completed clients = "
        << total_transmitted_bytes << "\n";
  mutex.unlock();
}

/*****************************************************************************/
/*                     kd_source_manager::write_history                      */
/*****************************************************************************/

void kd_source_manager::write_history(kdcs_message_block &block,
                                      int max_records)
{
  mutex.lock();
  int index = completed_connections;
  block << "\nHISTORY:\n";
  kd_client_history *scan;
  for (scan=history_head; (scan != NULL) && (max_records > 0);
       scan=scan->next, max_records--, index--)
    {
      block << "* " << index << ":\n";
      block << "    Target = \"" << scan->target.filename << "\"";
      if (scan->target.byte_range[0] != '\0')
        block << "(" << scan->target.byte_range << ")";
      block << "\n";
      if ((scan->num_jpip_h_channels + scan->num_jpip_ht_channels) == 0)
        block << "    Channel = <none> (stateless)\n";
      else if (scan->num_jpip_ht_channels == 0)
        block << "    Channel = \"http\" ("
              << scan->num_jpip_h_channels << ")\n";
      else if (scan->num_jpip_h_channels == 0)
        block << "    Channel = \"http-tcp\" ("
              << scan->num_jpip_ht_channels << ")\n";
      else
        block << "    Channel = \"http\" ("
              << scan->num_jpip_h_channels << "), \"http-tcp\" ("
              << scan->num_jpip_ht_channels << ")\n";
      block << "    Connected for " << scan->connected_seconds << " seconds\n";
      block << "    Transmitted bytes = "
            << scan->total_transmitted_bytes << "\n";
      block << "    Number of requests = " << scan->num_requests << "\n";
      block << "    Number of primary channel attachment events = "
            << scan->num_primary_connections << "\n";
      if (scan->num_jpip_ht_channels > 0)
        {
          block << "    Average aux channel RTT = " << scan->average_rtt
                << " seconds (over " << (int) scan->rtt_events
                << " RTT events)\n";
          block << "    Approximate aux channel rate = "
                << scan->approximate_auxiliary_channel_rate << " bytes/sec\n";
        }
      if ((scan->num_jpip_h_channels + scan->num_jpip_ht_channels) > 0)
        {
          block << "    Average byte limit in requests which were not "
                   "pre-empted = "
                << scan->average_serviced_byte_limit << "\n";
          block << "    Average bytes delivered per request which was "
                   "not pre-empted = " << scan->average_service_bytes << "\n";
          block << "    Average rate (not compensating for idle clients) = "
                << scan->total_transmitted_bytes /
                   (scan->connected_seconds+0.01F)
                << " bytes/sec\n";
        }
    }
  mutex.unlock();
}

/*****************************************************************************/
/*                  kd_source_manager::get_new_history_entry                 */
/*****************************************************************************/

kd_client_history *kd_source_manager::get_new_history_entry()
{
  kd_client_history *result;
  if (max_history_records <= 0)
    return NULL;
  if (num_history_records < max_history_records)
    result = new kd_client_history;
  else
    {
      result = history_tail;
      history_tail = result->prev;
    }
  memset(result,0,sizeof(kd_client_history));
  if ((result->next = history_head) != NULL)
    result->next->prev = result;
  history_head = result;
  if (history_tail == NULL)
    history_tail = history_head;
  history_tail->next = NULL;
  num_history_records++;
  return result;
}

/*****************************************************************************/
/*                    kd_source_manager::close_gracefully                    */
/*****************************************************************************/

void kd_source_manager::close_gracefully()
{
  mutex.lock();
  shutdown_requested = true;
  kd_source *src;
  kd_source_thread *thrd;
  for (src=sources; src != NULL; src=src->next)
    for (thrd=src->threads; thrd != NULL; thrd=thrd->next)
      thrd->request_shutdown(); // Must not call this while mutex locked
  while (num_threads > 0)
    {
      event.reset();
      event.wait(mutex);
    }
  mutex.unlock();
}
