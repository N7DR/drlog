// $Id: socket_support.h 260 2025-01-27 18:44:34Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef SOCKET_SUPPORT_H
#define SOCKET_SUPPORT_H

/*! \file socket_support.h

    Objects and functions related to sockets. This code is based, with permission, on
    a much larger codebase from IPfonix, Inc. for socket-related functions.

    I spent some time trying to use the boost asio library to perform these
    TCP-related functions, but the lack of good documentation and examples led me
    to return to my old code.
*/

#include "drlog_error.h"
#include "macros.h"
#include "pthread_support.h"

#include <chrono>
#include <deque>
#include <string>
#include <string_view>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

// Errors
constexpr int SOCKET_SUPPORT_UNABLE_TO_LISTEN    { -1 },  ///< Unable to listen on socket
              SOCKET_SUPPORT_TIMEOUT             { -2 },  ///< Socket timeout
              SOCKET_SUPPORT_SELECT_ERROR        { -3 },  ///< Unable to select socket
              SOCKET_SUPPORT_RX_BUFFER_TOO_SMALL { -4 },  ///< Socket receive buffer is too small
              SOCKET_SUPPORT_EFAULT              { -5 },  ///< EFAULT error
              SOCKET_SUPPORT_RECVFROM_ERROR      { -6 },  ///< Error receiving on socket
              SOCKET_SUPPORT_BIND_ERROR          { -7 },  ///< Unable to bind socket
              SOCKET_SUPPORT_CONNECT_ERROR       { -8 },  ///< Error received from connect()
              SOCKET_SUPPORT_LISTEN_ERROR        { -9 },  ///< Error received from listen()
              SOCKET_SUPPORT_ACCEPT_ERROR        { -10 }, ///< Error received from accept()
              SOCKET_SUPPORT_WRONG_PROTOCOL      { -11 }, ///< Incorrect protocol (IPv4 vs. IPv6)
              SOCKET_SUPPORT_FLAG_ERROR          { -12 }; ///< Error setting flag

constexpr int TCP_SOCKET_UNKNOWN_DESTINATION  { -1 },     ///< Destination not set
              TCP_SOCKET_ERROR_IN_WRITE       { -2 },     ///< Error received from write()
              TCP_SOCKET_ERROR_IN_RECV        { -3 },     ///< Error received from recv()
              TCP_SOCKET_UNABLE_TO_SET_OPTION { -4 },     ///< Error setting a socket option
              TCP_SOCKET_UNABLE_TO_CLOSE      { -5 },     ///< Error closing socket
              TCP_SOCKET_UNABLE_TO_RESOLVE    { -6 },     ///< Error resolving destination
              TCP_SOCKET_UNABLE_TO_GET_OPTION { -7 };     ///< Error getting a socket option

constexpr int ICMP_SOCKET_UNABLE_TO_CREATE    { -1 },     ///< Unable to create socket
              ICMP_SOCKET_SEND_ERROR          { -2 };     ///< Error when sending

constexpr int EPOLL_UNABLE_TO_CREATE          { -1 },     ///< unable to create epoll
              EPOLL_UNABLE_TO_ADD_DESCRIPTOR  { -2 };     ///< unable to add a file descriptor to an epoll

/// TCP socket error messages
const std::string tcp_socket_error_string[8] { std::string { },
                                               "Destination not set"s,
                                               "Error return from write()"s,
                                               "Error return from recv()"s,
                                               "Error return from setsockopt()"s,
                                               "Error closing socket"s,
                                               "Error resolving destination"s,
                                               "Error return from getsockopt()"s
                                             };

/// ICMP socket error messages
const std::string icmp_socket_error_string[3] { std::string { },
                                                "Unable to create"s,
                                                "Error when sending"s
                                              };

/// Type that holds a socket -- syntactic sugar
using SOCKET = int;

void fd_set_option(const int opt, const int fd);

/*! \brief        Set an fd_set to contain a particular single value of a file descriptor
    \param  fds   set of file descriptors
    \param  fd    the file descriptor to place into fds
*/
void fd_set_value(fd_set& fds, int fd);

/*! \brief        Set an fd_set to contain a particular single value of a file descriptor
    \param  fds   set of file descriptors
    \param  fd    the file descriptor to place into fds
*/
inline void operator+=(fd_set& fds, const int fd)
  { fd_set_value(fds, fd); }

/*! \brief                  Return the name of an error
    \param  error_number    socket error number
    \return                 error string that corresponds to <i>error_number</i>
*/
inline std::string socket_error_name(const int error_number)
  { return strerror(error_number); }

/*! \brief                              Read a socket
    \param  in_socket                   inbound socket
    \param  timeout_in_tenths           timeout in tenths of a second
    \param  buffer_length_for_reply     maximum allowed length of reply
    \return                             response
  
    Throws socket_support_error(SOCKET_TIMEOUT) if the socket times out
*/
std::string read_socket(SOCKET& in_socket, const int timeout_in_tenths, const int buffer_length_for_reply = 4096);

/*! \brief          Flush a readable socket
    \param  sock    socket to flush
*/
void flush_read_socket(SOCKET& sock);

/*! \brief              Generate a sockaddr_storage from an address and port
    \param  ip_address  IP address in network order
    \param  port_nr     port number in host order
    \return             equivalent sockaddr_storage

    The returned sockaddr_storage is really a sockaddr_in, since this works only with IPv4
*/
sockaddr_storage socket_address(const uint32_t ip_address, const short port_nr = 0);

/*! \brief                          Generate a sockaddr_storage from an address and port
    \param  dotted_decimal_address  IP address as a dotted decimal
    \param  port_nr                 port number in host order
    \return                         equivalent sockaddr_storage

    The returned sockaddr_storage is really a sockaddr_in, since this works only with IPv4
*/
inline sockaddr_storage socket_address(const std::string& dotted_decimal_address, const short port_nr = 0)
  { return socket_address(inet_addr(dotted_decimal_address.c_str()), port_nr); }

/*! \brief          Extract port from a sockaddr_in
    \param  sin     sockaddr_in
    \return         port number
*/
inline unsigned int port(const sockaddr_in& sin)
  { return ntohs((unsigned int)(sin.sin_port)); }

/*! \brief          Extract port from a sockaddr
    \param  sin     sockaddr
    \return         port number

    Assumes that the sockaddr is for the Internet family
*/
inline unsigned int port(const sockaddr& sin)
  { return ntohs((unsigned int)(((sockaddr_in*)(&sin))->sin_port)); }

/*! \brief          Extract address from a sockaddr_in
    \param  sin     sockaddr_in
    \return         dotted decimal string
*/
inline std::string dotted_decimal_address(const sockaddr_in& sin)
  { return (inet_ntoa(sin.sin_addr)); }

/*! \brief          Extract address from a sockaddr
    \param  sin     sockaddr_in
    \return         dotted decimal string

    Assumes that the sockaddr is for the Internet family
*/
inline std::string dotted_decimal_address(const sockaddr& sin)
  { return (inet_ntoa(((sockaddr_in*)(&sin))->sin_addr)); }

/*! \brief      Extract address from a sockaddr_storage
    \param  ss  sockaddr_storage
    \return     dotted decimal string
*/
std::string dotted_decimal_address(const sockaddr_storage& ss);

/*! \brief                  Bind a socket to an address
    \param  sock            socket to bind
    \param  local_address   address to which to bind <i>sock</i>
    \return                 value returned from bind() system call
*/
inline int bind(SOCKET& sock, const sockaddr_in& local_address)
  { return bind(sock, (sockaddr*)&local_address, sizeof(sockaddr_in)); }

/*! \brief      Create a host-order 32-bit IP address
    \param  s   IP address as a dotted decimal
    \return     IP address in host order in a single variable
*/
inline uint32_t host_order_inet_addr(const std::string& s)
  { return ntohl(inet_addr(s.c_str())); }             // inet_addr returns network order

/*! \brief      Convert a sockaddr_storage to a sockaddr_in
    \param  ss  value to be converted
    \return     <i>ss</i> as a sockaddr_in
  
    Throws socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL) if <i>ss</i> is not an IPv4 address
*/
sockaddr_in to_sockaddr_in(const sockaddr_storage& ss);

/*! \brief          Write a <i>sockaddr_in</i> object to an output stream
    \param  ost     output stream
    \param  pa      object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const struct sockaddr_in& pa);

// ------------------------------------  tcp_socket  ----------------------------------

/*! \class  tcp_socket
    \brief  Encapsulate and manage a TCP socket
*/

class tcp_socket
{

protected:

  sockaddr_storage  _bound_address      { };                        ///< address to which socket is bound
  sockaddr_storage  _destination        { };                        ///< destination
  bool              _destination_is_set { false };                  ///< is the destination known?
  bool              _force_closure      { false };                  ///< force closure of socket in destructor, even for a pre-existing socket
  bool              _preexisting_socket { false };                  ///< whether <i>_sock</i> exists outside the object
  SOCKET            _sock               { 0 };                      ///< encapsulated socket
  mutable pt_mutex  _tcp_socket_mutex   { "UNNAMED TCP SOCKET"s };  ///< mutex to control access
  unsigned int      _timeout_in_tenths  { 600 };                    ///< timeout in tenths of a second = 1 minute (currently unimplemented)

  mutable char*     _in_buffer_p        { nullptr };                ///< used to store data read from the network, if this is a read socket
  mutable size_t    _in_buffer_size     { 0 };

/*! \brief close the socket
*/
  void _close_the_socket(void);

/*! \brief create an input buffer if one doesn't exist
*/
  void _create_buffer_if_necessary(void) const;   // logically const

/*! \brief    Resize the buffer
    \return   whether the buffer was actually resized

    Doubles the size of the input buffer, or sets it to MAX_IN_BUFFER_SIZE, whichever is less
*/
  bool _resize_buffer(void) const;   // logically const

/*! \brief            Resize the buffer
    \param  new_size  the new size of the buffer
    \return   whether the buffer was actually resized

    If the current size is not already MAX_IN_BUFFER_SIZE, then sets the size of the input buffer
    to <i>new_size</i>, or sets it to MAX_IN_BUFFER_SIZE, whichever is less
*/
  bool _resize_buffer(const size_t new_size) const;   // logically const

public:

/*! \brief  Default constructor
*/
  tcp_socket(void);

/*! \brief                                  Construct and initialise with useful values
    \param  destination_ip_address_or_fqdn  destination dotted decimal address or FQDN
    \param  destination_port                destination port
    \param  source_address                  IP address to which the socket is to be bound
    \param  retry_time_in_seconds           time between retries, in seconds
*/
  tcp_socket(const std::string& destination_ip_address_or_fqdn, 
             const unsigned int destination_port, 
             const std::string& source_address,
             const unsigned int retry_time_in_seconds = 10);

/*! \brief      Encapsulate a pre-existing socket
    \param  sp  pointer to socket
  
    Acts as default constructor if passed pointer is NULL
*/
  explicit tcp_socket(SOCKET* sp);

/*! \brief          Encapsulate a pre-existing socket
    \param  sock    socket
*/
  inline explicit tcp_socket(SOCKET sock) :
    _preexisting_socket(true),
    _sock(sock)
  { }

/*! \brief  Destructor
*/
  ~tcp_socket(void);

/// copy constructor
  tcp_socket(const tcp_socket& obj) = delete;

  READ_AND_WRITE(timeout_in_tenths);                       ///< RW access to timeout_in_tenths

/*! \brief  Close the socket
*/
  inline void close(void)
    { _close_the_socket(); }

/*! \brief  Create and use a different underlying socket
*/
  void new_socket(void);

/*! \brief                  Bind the socket
    \param  local_address   address/port to which the socket is to be bound
*/
  void bind(const sockaddr_storage& local_address);

/*! \brief                          Bind the socket
    \param  dotted_decimal_address  address to which the socket is to be bound
    \param  port_nr                 port to which the socket is to be bound
*/
  inline void bind(const std::string& dotted_decimal_address, const short port_nr = 0)
    { bind(socket_address(dotted_decimal_address, port_nr)); }

/*! \brief                      Connect to the far-end
    \param  dotted_ip_address   address of the far end
    \param  port_nr             port of the far end
*/
  inline void destination(const std::string& dotted_ip_address, const short port_nr)
    { destination(socket_address(dotted_ip_address, port_nr)); }

/*! \brief          Connect to the far-end
    \param  adr     distal address
*/
  void destination(const sockaddr_storage& adr);

/*! \brief                      Connect to the far-end, with explicit time-out when trying to make connection
    \param  dotted_ip_address   address of the far end
    \param  port_nr             port of the far end
    \param  timeout_secs        timeout in seconds
*/
  inline void destination(const std::string& dotted_ip_address, const short port_nr, const unsigned long timeout_secs)
    { destination(socket_address(dotted_ip_address, port_nr), timeout_secs); }

/*! \brief                  Connect to the far-end, with explicit time-out when trying to make connection
    \param  adr             address/port of the far end
    \param  timeout_secs    timeout in seconds

    See https://www.linuxquestions.org/questions/programming-9/connect-timeout-change-145433/
*/
  void destination(const sockaddr_storage& adr, const unsigned long timeout_secs);

/*! \brief          Mark as connected to the far-end
    \param  adr     address/port of the far end

    Used for server sockets returned by .accept()
*/
  void connected(const sockaddr_storage& adr);

/*! \brief                      Connect to the far-end
    \param  dotted_ip_address   address of the far end
    \param  port_nr             port of the far end
*/
  inline void connect(const std::string& dotted_ip_address, const short port_nr)
    { destination(dotted_ip_address, port_nr); }

/*! \brief   Get the encapsulated socket
    \return  the socket that is encapsulated by the object
*/
  inline SOCKET socket(void) const
    { return _sock; }

/// force closure in destructor even if it's a pre-existing socket
  inline void force_closure(void)
    { _force_closure = true; }

/*! \brief          Simple send
    \param  msg     message to send
  
    Does not look for a response. Throws an exception if there is any problem.
*/
  void send(const std::string_view msg);

/*! \brief      Simple receive
    \return     received string
*/
  std::string read(void) const;

/*! \brief                  Simple receive
    \param  timeout_secs    timeout in seconds
    \return                 received string
        
    Throws an exception if the read times out
*/
  std::string read(const unsigned long timeout_secs) const;

/*! \brief                  Simple receive
    \param  timeout_secs    timeout
    \return                 received string

    Throws an exception if the read times out
*/
  inline std::string read(const std::chrono::system_clock::duration t) const
    { return read(static_cast<unsigned long>(duration_cast<std::chrono::seconds>(t).count())); }
  
/*! \brief              Set the idle time before a keep-alive is sent
    \param  seconds     time to wait idly before a keep-alive is sent
*/
  void keep_alive_idle_time(const unsigned int seconds);

/*! \brief              Set the idle time before a keep-alive is sent
    \param  seconds     time to wait idly before a keep-alive is sent
*/
  inline void keep_alive_idle_time(const std::chrono::system_clock::duration t)
    { keep_alive_idle_time(duration_cast<std::chrono::seconds>(t).count()); }

/*! \brief    Get the idle time before a keep-alive is sent
    \return   time to wait idly, in seconds, before a keep-alive is sent
*/
  unsigned int keep_alive_idle_time(void) const;

/*! \brief    Get the time between keep-alives
    \return   time to wait idly between keep-alives, in seconds
*/
  unsigned int keep_alive_retry_time(void) const;

/*! \brief              Set the time between keep-alives
    \param  seconds     time to wait idly before a keep-alive is sent
*/
  void keep_alive_retry_time(const unsigned int seconds);

/*! \brief      Get the maximum number of retries
    \param  n   maximum number of retries before notifying upwards
*/
  unsigned int keep_alive_max_retries(void) const;

/*! \brief      Set the maximum number of retries
    \param  n   maximum number of retries before notifying upwards
*/
  void keep_alive_max_retries(const unsigned int n);
  
/*! \brief          Set or unset the use of keep-alives
    \param  torf    whether to use keep-alives

    Throws a tcp_socket_error if an error occurs
*/
  void keep_alive(const bool torf = true);

/*  \brief    Is a keep-alive in use on this socket?
    \return   whether a keep-alive is in use
*/
  bool keep_alive(void) const;

/*! \brief          Set properties of the keep-alive
    \param  idle    idle time in seconds
    \param  retry   retry time in seconds
    \param  n       maximum number of retries
*/
  void keep_alive(const unsigned int idle, const unsigned int retry, const unsigned int n);

/*! \brief          Set or unset the re-use of the socket
    \param  torf    whether to allow re-use

    Throws a tcp_socket_error if an error occurs
*/
  void reuse(const bool torf = true);

/*! \brief          Set or unset lingering of the socket
    \param  torf    whether to allow lingering
    \param  secs    linger time in seconds

    Throws a tcp_socket_error if an error occurs
*/
  void linger(const bool torf = true, const int secs = 0);

/*! \brief          Set or unset lingering of the socket
    \param  lng     struct linger object

    Throws a tcp_socket_error if an error occurs
*/
  void linger(const struct linger& lngr);

/*! \brief    Get the lingering state of the socket
    \return   whether linger is enabled and, if so, the value in seconds

    Throws a tcp_socket_error if an error occurs
*/
  std::optional<int> linger(void) const;

/*! \brief              Rename the mutex associated with the socket
    \param  new_name    the new name for the mutex
*/
  inline void rename_mutex(const std::string& new_name)
    { _tcp_socket_mutex.rename(new_name); }

/// Human-readable string description of the status of the socket
  std::string to_string(void) const;

/*! \brief  Enable reuse of the socket

    Throws a tcp_socket_error if an error occurs
*/
  void enable_reuse(void);
};

// ------------------------------------  icmp_socket  ----------------------------------

/*! \class  icmp_socket
    \brief  Encapsulate and manage a Linux kernel ICMP socket

    https://opennms.discourse.group/t/how-to-allow-unprivileged-users-to-use-icmp-ping/1573
*/

class icmp_socket
{
protected:

  sockaddr_storage  _bound_address     { };                           ///< address to which socket is bound
  sockaddr_in       _dest              { };                           ///< destination
  std::string       _destination_str   { };                           ///< destination as string
  icmphdr           _icmp_hdr          { };                           ///< header for the ping packet
  pt_mutex          _icmp_socket_mutex { "UNNAMED ICMP SOCKET"s };    ///< mutex to control access
  int               _sequence_nr       { 0 };                         ///< sequence number
  SOCKET            _sock              { };                           ///< encapsulated socket
  struct timeval    _socket_timeout    { 5, 0 };                      ///< seconds, microseconds; collision with curses timeout macro if just use _timeout

public:

/*! \brief  Default constructor
*/
  icmp_socket(void);

/*! \brief                                  Create and associate with a particular destination
    \param  destination_ip_address_or_fqdn  IPv4 address or an FQDN
*/
  explicit icmp_socket(const std::string& destination_ip_address_or_fqdn);

/*! \brief                                  Create and associate with a particular destination
    \param  destination_ip_address_or_fqdn  IPv4 address or an FQDN
*/
  inline explicit icmp_socket(const std::string_view destination_ip_address_or_fqdn)
    { icmp_socket( std::string { destination_ip_address_or_fqdn } ); }

/*! \brief                                  Create and associate with a particular destination and local address
    \param  destination_ip_address_or_fqdn  IPv4 address or an FQDN
    \param  dotted_decimal_address          local IPv4 address
*/
  icmp_socket(const std::string& destination_ip_address_or_fqdn, const std::string& dotted_decimal_address);

/// copy constructor
  icmp_socket(const icmp_socket& obj) = delete;

  READ_AND_WRITE(socket_timeout);   // seconds, microseconds; collision with curses timeout macro if just use _timeout

/*! \brief                  Bind the socket
    \param  local_address   address/port to which the socket is to be bound
*/
  void bind(const sockaddr_storage& local_address);

/*! \brief                          Bind the socket
    \param  dotted_decimal_address  address to which the socket is to be bound
    \param  port_nr                 port to which the socket is to be bound
*/
  inline void bind(const std::string& dotted_decimal_address)
    { bind(socket_address(dotted_decimal_address, 0 /* port nr */)); }

/*! \brief                          Perform a ping

    This is ridiculously C-ish, basically taken from: https://stackoverflow.com/questions/8290046/icmp-sockets-linux
*/
  bool ping(void);

/*! \brief              Rename the mutex associated with the socket
    \param  new_name    the new name for the mutex
*/
  inline void rename_mutex(const std::string& new_name)
    { _icmp_socket_mutex.rename(new_name); }
};

#if 0

not yet ready for use

// ------------------------------------  epoller  ----------------------------------

/*! \class  epoller
    \brief  Encapsulate and manage Linux epoll functions
*/

class epoller
{
protected:

  int                _fd;

  struct epoll_event _ev;

public:

  epoller(void);

  void operator+=(const int file_descriptor);   // fd of interest

};
#endif

/*! \brief              Convert a name to a dotted decimal IP address
    \param  fqdn        name to be resolved
    \param  n_tries     maximum number of tries
    \return             equivalent IP address in dotted decimal format

    Throws exception if the name cannot be resolved. Uses gethostbyname_r() to perform the lookup.
    <i>n_tries</i> is present because gethostbyname_r() cannot be relied on to complete a remote
    lookup before deciding to return with an error.

    Cannot use string_view because gethostbyname_r() requires a null-terminated C-string
*/   
std::string name_to_dotted_decimal(const std::string& fqdn, const unsigned int n_tries = 1);

// ---------------------------------------  Errors  ------------------------------------

ERROR_CLASS(socket_support_error);  ///< general socket-related errors

ERROR_CLASS(tcp_socket_error);      ///< errors related to TCP sockets

ERROR_CLASS(icmp_socket_error);     ///< errors related to ICMP sockets

//ERROR_CLASS(epoll_error);           ///< errors related to epolls

#endif    // !SOCKET_SUPPORT_H
