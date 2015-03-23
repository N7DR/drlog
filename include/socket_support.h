// $Id: socket_support.h 100 2015-03-22 20:59:38Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef SOCKET_SUPPORT_H
#define SOCKET_SUPPORT_H

/*! \file socket_support.h

    Objects and functions related to sockets. This code is based on
    a much larger codebase from IPfonix, Inc. for socket-related functions.

    I spent some time trying to use the boost asio library to perform these
    TCP-related functions, but the lack of good documentation and examples led me
    to return to my old code.
*/

#include "macros.h"
#include "drlog_error.h"
#include "pthread_support.h"

#include <deque>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Errors
const int SOCKET_SUPPORT_UNABLE_TO_LISTEN    = -1,  ///< Unable to listen on socket
          SOCKET_SUPPORT_TIMEOUT             = -2,  ///< Socket timeout
          SOCKET_SUPPORT_SELECT_ERROR        = -3,  ///< Unable to select socket
          SOCKET_SUPPORT_RX_BUFFER_TOO_SMALL = -4,  ///< Socket receive buffer is too small
          SOCKET_SUPPORT_EFAULT              = -5,  ///< EFAULT error
          SOCKET_SUPPORT_RECVFROM_ERROR      = -6,  ///< Error receiving on socket
          SOCKET_SUPPORT_BIND_ERROR          = -7,  ///< Unable to bind socket
          SOCKET_SUPPORT_CONNECT_ERROR       = -8,  ///< Error received from connect()
          SOCKET_SUPPORT_LISTEN_ERROR        = -9,  ///< Error received from listen()
          SOCKET_SUPPORT_ACCEPT_ERROR        = -10, ///< Error received from accept()
          SOCKET_SUPPORT_WRONG_PROTOCOL      = -11; ///< Incorrect protocol (IPv4 vs. IPv6)

const int TCP_SOCKET_UNKNOWN_DESTINATION  = -1,     ///< Destination not set
          TCP_SOCKET_ERROR_IN_WRITE       = -2,     ///< Error received from write()
          TCP_SOCKET_ERROR_IN_RECV        = -3,     ///< Error received from recv()
          TCP_SOCKET_UNABLE_TO_SET_OPTION = -4,     ///< Error setting a socket option
          TCP_SOCKET_UNABLE_TO_CLOSE      = -5,     ///< Error closing socket
          TCP_SOCKET_UNABLE_TO_RESOLVE    = -6;     ///< Error resolving destination

/// TCP socket error messages
const std::string tcp_socket_error_string[7] = { "",
                                                 "Destination not set",
                                                 "Error return from write()",
                                                 "Error return from recv()",
                                                 "Error return from setsockopt()",
                                                 "Error closing socket",
                                                 "Error resolving destination"
                                               };

const int SOCKET_ERROR = -1;

/// Type that holds a socket -- syntactic sugar
typedef int SOCKET;

// used to create sockets of different protocol families
enum socket_family { IPV4_SOCKET,
                     IPV6_SOCKET
                   };

/*! \brief  Return the name of an error
    \param  error_number  Socket error number
    \return Error string that corresponds to <i>error_number</i>
*/
std::string socket_error_name(const int error_number);

/*! \brief  Read a socket
  \param  in_socket     Inbound socket
  \param  timeout_in_tenths         Timeout in tenths of a second
  \param  buffer_length_for_reply   Maximum allowed length of reply
  \return Response
  
  Throws socket_support_error(SOCKET_TIMEOUT) if the socket times out
*/
std::string read_socket(SOCKET& in_socket, const int timeout_in_tenths, const int buffer_length_for_reply);

/*! \brief  Flush a readable socket
  \param  sock  Socket to flush
*/
void flush_read_socket(SOCKET& sock);

/*! \brief  Generate a sockaddr_storage from an address and port
  \param  ip_address  IP address in network order
  \param  port_nr   Port number in host order
  \return Equivalent sockaddr_storage

        The returned sockaddr_storage is really a sockaddr_in, since this works only with IPv4
*/
sockaddr_storage socket_address(const unsigned long ip_address, const short port_nr = 0);

/*! \brief  Generate a sockaddr_storage from an address and port
  \param  dotted_decimal_address  IP address as a dotted decimal
  \param  port_nr   Port number in host order
  \return Equivalent sockaddr_storage

        The returned sockaddr_storage is really a sockaddr_in, since this works only with IPv4
*/
inline sockaddr_storage socket_address(const std::string& dotted_decimal_address, const short port_nr = 0)
  { return socket_address(inet_addr(dotted_decimal_address.c_str()), port_nr); }

/*! \brief  Extract port from a sockaddr_in
  \param  sin          sockaddr_in
  \return port number
*/
inline const unsigned int port(const sockaddr_in& sin)
  { return ntohs((unsigned int)(sin.sin_port)); }

/*! \brief  Extract port from a sockaddr
  \param  sin          sockaddr
  \return port number

  Assumes that the sockaddr is for the Internet family
*/
inline const unsigned int port(const sockaddr& sin)
  { return ntohs((unsigned int)(((sockaddr_in*)(&sin))->sin_port)); }

/*! \brief  Extract address from a sockaddr_in
  \param  sin          sockaddr_in
  \return dotted decimal string
*/
inline const std::string dotted_decimal_address(const sockaddr_in& sin)
  { return (inet_ntoa(sin.sin_addr)); }

/*! \brief  Extract address from a sockaddr
  \param  sin          sockaddr_in
  \return dotted decimal string

  Assumes that the sockaddr is for the Internet family
*/
inline const std::string dotted_decimal_address(const sockaddr& sin)
  { return (inet_ntoa(((sockaddr_in*)(&sin))->sin_addr)); }

// simple wrappers for stupid C-style functions
/*! \brief  Bind a socket to an address
  \param  sock    Socket to bind
  \param  local_address Address to which to bind <i>sock</i>
  \return Value returned from bind() system call
*/
inline int bind(SOCKET& sock, const sockaddr_in& local_address)
  { return bind(sock, (sockaddr*)&local_address, sizeof(sockaddr_in)); }

/*! \brief  Create a host-order 32-bit IP address
  \param  s IP address as a dotted decimal
  \return IP address in host order in a single variable
  
  Assumes that a long is 32 bits
*/
inline long host_order_inet_addr(const std::string& s)
  { return ntohl(inet_addr(s.c_str())); }             // inet_addr returns network order

/*! \brief  Process a socket error
  \param  socket_status Value returned by a system call to a socket function
  
  This routine encapsulates OS differences in a single place
*/
void process_socket_error(const int socket_status);

/*! \brief  Convert a sockaddr_storage to a sockaddr_in
  \param  ss  Value to be converted
        \return <i>ss</i> as a sockaddr_in
  
  Throws socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL) if <i>ss</i> is not an IPv4 address
*/
const sockaddr_in to_sockaddr_in(const sockaddr_storage& ss);

/// ostream << sockaddr_in
std::ostream& operator<<(std::ostream& ost, const struct sockaddr_in& pa);

// ------------------------------------  tcp_socket  ----------------------------------

/*! \class  tcp_socket
  \brief  Encapsulate and manage a TCP socket 
*/

class tcp_socket
{
protected:

  bool              _preexisting_socket;        ///< whether <i>_sock</i> exists outside the object
  SOCKET            _sock;                      ///< Encapsulated socket

  bool              _destination_is_set;        ///< Is the destination known?
  sockaddr_storage  _destination;               ///< Destination

  sockaddr_storage  _bound_address;                     ///< Address to which port is bound

  bool              _force_closure;                     ///< force closure of socket in destructor, even for a pre-existing socket

  unsigned int      _timeout_in_tenths;                 ///< timeout in tenths of a second (currently unimplemented)

  pt_mutex          _tcp_socket_mutex;

/*! \brief  Copy constructor
  \param  obj   Object to be copied
  
  Protected function ensures that the socket cannot be copied
*/
  tcp_socket(const tcp_socket& obj);

/*! \brief close the socket
*/
  void _close_the_socket(void);

public:

/*! \brief  Default constructor
*/
  tcp_socket(void);

/*! \brief  Construct and initialise with useful values
  \param  destination_ip_address_or_fqdn  Destination dotted decimal address or FQDN
  \param  destination_port          Destination port
  \param  source_address                  Address to which the socket is to be bound
*/
  tcp_socket(const std::string& destination_ip_address_or_fqdn, 
             const unsigned int destination_port, 
             const std::string& source_address);

/*! \brief  Encapsulate a pre-existing socket
  \param  sp  Pointer to socket
  
  Acts as default constructor if passed pointer is NULL
*/
  explicit tcp_socket(SOCKET* sp);

/*! \brief  Encapsulate a pre-existing socket
  \param  sock  Socket
*/
  explicit tcp_socket(SOCKET sock);

/*! \brief  Destructor
*/
  virtual ~tcp_socket(void);

/*!     \brief  Close the socket
*/
  inline void close(void)
    { return _close_the_socket(); }

/*!     \brief  New socket
        Switch to using a different underlying socket
*/
  void new_socket(void);

/*! \brief  Bind the socket
  \param  local_address Address/port to which the socket is to be bound
*/
  void bind(const sockaddr_storage& local_address);

/*! \brief  Bind the socket
  \param  dotted_decimal_address  Address to which the socket is to be bound
  \param  port_nr     Port to which the socket is to be bound
*/
  void bind(const std::string& dotted_decimal_address, const short port_nr = 0);

/*! \brief  Connect to the far-end
  \param  dotted_ip_address Address of the far end
  \param  port_nr     Port of the far end
*/
  inline void destination(const std::string& dotted_ip_address, const short port_nr)
    { destination(socket_address(dotted_ip_address, port_nr)); }

/*! \brief  Connect to the far-end
  \param  adr Address/port of the far end
*/
  void destination(const sockaddr_storage& adr);

/*! \brief  Connect to the far-end, with explicit time-out when trying to make connection
    \param  dotted_ip_address Address of the far end
    \param  port_nr     Port of the far end
    \param  timeout timeout in seconds
*/
  inline void destination(const std::string& dotted_ip_address, const short port_nr, const unsigned long timeout_secs)
    { destination(socket_address(dotted_ip_address, port_nr), timeout_secs); }

/*! \brief  Connect to the far-end, with explicit time-out when trying to make connection
    \param  adr Address/port of the far end
    \param  timeout timeout in seconds
*/
  void destination(const sockaddr_storage& adr, const unsigned long timeout_secs);

/*! \brief  Mark as connected to the far-end
  \param  adr Address/port of the far end

  Used for server sockets returned by .accept()
*/
  void connected(const sockaddr_storage& adr);

/*! \brief  Connect to the far-end
  \param  dotted_ip_address Address of the far end
  \param  port_nr     Port of the far end
*/
  inline void connect(const std::string& dotted_ip_address, const short port_nr)
    { destination(dotted_ip_address, port_nr); }

/// Return the encapsulated socket
  inline const SOCKET socket(void) const
    { return _sock; }

/// Force closure in destructor even if it's a pre-existing socket
  inline void force_closure(void)
    { _force_closure = true; }

/*! \brief  Simple send
  \param  msg Message to send
  
  This is not very useful, since it doesn't look for a response
*/
  void send(const std::string& msg);

/*! \brief  Simple receive
  \return Received string
*/
  const std::string read(void);

/*! \brief                Simple receive
        \param  timeout_secs  Timeout in seconds
  \return               Received string
        
        Throws an exception if the read times out
*/
const std::string read(const unsigned long timeout_secs);

  READ_AND_WRITE(timeout_in_tenths);                       ///< RW access to timeout_in_tenths
  
// keep-alive  
  
// set the time before a keep-alive is sent
  void idle_time(const unsigned int seconds);

// set the time between keep-alives
  void retry_time(const unsigned int seconds);

// set the maximum number of retries
  void max_retries(const unsigned int n); 
  
// control keep-alive
  void keep_alive(const bool torf = true);
  
  void keep_alive(const unsigned int idle, const unsigned int retry, const unsigned int n);
};

/*!     \brief  Convert a name to a dotted decimal IP address
        \param  fqdn                    Name to be resolved
        \return Equivalent IP address in dotted decimal format

        Throws exception if the name cannot be resolved. Uses gethostbyname() to perform the lookup.
*/   
std::string name_to_dotted_decimal(const std::string& fqdn);

// ---------------------------------------  Errors  ------------------------------------

/*! \class  socket_support_error
    \brief  Socket-related errors
*/

class socket_support_error : public drlog_error
{
protected:

public:

/*! \brief  Construct from error code and reason
    \param  n Error code
    \param  s Reason
*/
  socket_support_error(const int n, const std::string& s = (std::string)"") : 
    drlog_error(n, s) 
  { }
};

/*! \class  tcp_socket_error
    \brief  TCP socket-related errors
*/

class tcp_socket_error : public drlog_error
{
protected:

public:

/*! \brief  Construct from error code and reason
    \param  n Error code
    \param  s Reason
*/
  tcp_socket_error(const int n, const std::string& s = (std::string)"") : 
    drlog_error(n, s) 
  { }
};

#endif    // !SOCKET_SUPPORT_H
