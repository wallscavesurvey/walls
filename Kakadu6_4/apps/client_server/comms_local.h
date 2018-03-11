/*****************************************************************************/
// File: comms_local.h [scope = APPS/CLIENT-SERVER]
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
   This header file is designed to hide platform-specific definitions from
 the more portable definitions in "kdcs_comms.h".
 ******************************************************************************/

#ifndef COMMS_LOCAL_H
#define COMMS_LOCAL_H

#define FD_SETSIZE 1024

#if (defined WIN32) || (defined _WIN32) || (defined _WIN64)
#  include <winsock2.h>
#  include <direct.h>
#  include <Ws2tcpip.h>
#  define KDCS_WIN_SOCKETS
#  define KDCS_HOSTNAME_MAX 256
#else // not Windows
#  include <fcntl.h>
#  include <unistd.h>
#  include <errno.h>
#  include <netdb.h>
#  include <time.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  define KDCS_BSD_SOCKETS
#  ifdef _POSIX_HOST_NAME_MAX
#    define KDCS_HOSTNAME_MAX _POSIX_HOST_NAME_MAX
#  else
#    define KDCS_HOSTNAME_MAX 1024
#  endif // no _POSIX_HOST_NAME_MAX
#endif // not Windows

#include "kdcs_comms.h"

// Defined here:
struct kdcs_socket;
struct kdcs_fd_sets;
struct kdcs_channel_ref;


/*****************************************************************************/
/*                                kdcs_socket                                */
/*****************************************************************************/

#ifdef KDCS_WIN_SOCKETS
struct kdcs_socket {
  public: // Member functions
    kdcs_socket() { sock = INVALID_SOCKET; }
    kdcs_socket(kdcs_socket &xfer_src)
      { // Transfers the actual `sock' member from `xfer_src' leaving it invalid
        sock = xfer_src.sock; xfer_src.sock = INVALID_SOCKET;
      }
    ~kdcs_socket() { close(); }
    bool is_valid() { return (sock != INVALID_SOCKET); }
    void shutdown()
      { if (is_valid()) ::shutdown(sock,SD_BOTH); }
    void close()
      { if (is_valid()) { ::closesocket(sock); sock = INVALID_SOCKET; } }
    bool make_nonblocking()
      { unsigned long upar=1; return (ioctlsocket(sock,FIONBIO,&upar) == 0); }
    void disable_nagel() {}
    void reuse_address() {}
  public: // Static functions for testing errors
    static int get_last_error() { return (int) WSAGetLastError(); }
    static bool check_error_connected(int err) { return (err==WSAEISCONN); }
    static bool check_error_wouldblock(int err)
      { return ((err==WSAEWOULDBLOCK) || (err==WSAEALREADY) ||
                (err==WSAEINPROGRESS)); }
    static bool check_error_invalidargs(int err) { return (err==WSAEINVAL); };
  public: // Data
    SOCKET sock;
  };
#else // BSD sockets
struct kdcs_socket {
  public: // Member functions
    kdcs_socket() { sock = -1; }
    kdcs_socket(kdcs_socket &xfer_src)
      { // Transfers the actual `sock' member from `xfer_src' leaving it invalid
        sock = xfer_src.sock;  xfer_src.sock = -1;
      }
    ~kdcs_socket() { close(); }
    bool is_valid() { return (sock >= 0); }
    void shutdown()
      { if (is_valid()) ::shutdown(sock,SHUT_RDWR); }
    void close()
      { if (is_valid()) { ::close(sock); sock = -1; } }
    bool make_nonblocking()
      { int tmp = fcntl(sock,F_GETFL);
        return ((tmp != -1) && (fcntl(sock,F_SETFL,(tmp|O_NONBLOCK)) != -1)); }
    void disable_nagel()
      { int tval=1;
        setsockopt(sock,IPPROTO_TCP,TCP_NODELAY,(char *)&tval,sizeof(tval)); }
    void reuse_address()
      { int tval=1;
        setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char *)&tval,sizeof(tval)); }
  public: // Static functions for testing errors
    static int get_last_error() { return (int) errno; }
    static bool check_error_connected(int err) { return (err==EISCONN); }
    static bool check_error_wouldblock(int err)
      { return ((err==EWOULDBLOCK) || (err==EAGAIN) || (err==EALREADY) ||
                (err==EINPROGRESS)); }
    static bool check_error_invalidargs(int err) { return (err==EINVAL); }
  public: // Data
    int sock;
  };
#endif // BSD sockets

/*****************************************************************************/
/*                               kdcs_fd_sets                                */
/*****************************************************************************/

struct kdcs_fd_sets {
  public: // Functions
    kdcs_fd_sets() { clear(); }
    void clear()
      { FD_ZERO(&read_set); FD_ZERO(&write_set); FD_ZERO(&error_set); }
  public: // Data
    fd_set read_set;
    fd_set write_set;
    fd_set error_set;
  };

/*****************************************************************************/
/*                              kdcs_channel_ref                             */
/*****************************************************************************/

struct kdcs_channel_ref {
  kdcs_channel *channel;
  kdcs_socket *socket; // NULL if channel reference is marked for deletion
  kdcs_channel_servicer *servicer;
  int active_conditions; // Conditions passed to `select' at least once so far
  int queued_conditions; // Conditions which arrive while `select' in progress
  kdcs_channel_ref *next, *prev; // Used to build `channel_refs' list
  bool on_active_list; // True if on the `active_refs' list
  bool on_wakeup_list; // True if on the `wakeup_refs' list
  kdcs_channel_ref *active_next; // Used to build `active_refs' list
  kdcs_channel_ref *wakeup_next; // Used to build `wakeup_refs' list
  kdu_long earliest_wakeup; // Earliest time (usecs) to wakeup; -ve if none
  kdu_long latest_wakeup; // Latest time to schedule the wakeup; -ve if none
};



#endif // COMMS_LOCAL_H
