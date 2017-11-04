// $Id: socket_support.cpp 128 2016-04-16 15:47:23Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file socket_support.h

    Objects and functions related to sockets. This code is based on
    a much larger codebase from IPfonix, Inc. for socket-related functions.
*/

#include "log_message.h"
#include "pthread_support.h"
#include "socket_support.h"
#include "string_functions.h"

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

extern message_stream ost;                                              ///< for debugging and logging
extern void alert(const string& msg, const bool show_time = true);      ///< function to alert the user

const int SOCKET_ERROR = -1;            ///< error return from various socket-related system functions

// ---------------------------------  tcp_socket  -------------------------------

/// close the socket
void tcp_socket::_close_the_socket(void)
{ if (_sock)
  { SAFELOCK(_tcp_socket);

    const int status = ::close(_sock);
  
    if (status == -1)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_CLOSE, strerror(errno));
  }
}

/// default constructor
tcp_socket::tcp_socket(void)  :
  _sock(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
  _preexisting_socket(false),
  _force_closure(false),
  _timeout_in_tenths(600)                     // 1 minute
{ try
  { 
// enable re-use
    static const int on = 1;

    int status = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on) );  // char* cast is needed for Windows

    if (status)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_REUSEADDR");

    const struct linger lgr = { 1, 0 };

    status = setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&lgr, sizeof(lgr) );  // char* cast is needed for Windows

    if (status)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_LINGER");    
  }

  catch (...)
  { _close_the_socket();
    throw;
  }
}

/*! \brief          Encapsulate a pre-existing socket
    \param  sock_p  pointer to socket
  
    Acts as default constructor if passed pointer is nullptr
*/
tcp_socket::tcp_socket(SOCKET* sp) :
  _destination_is_set(false),
  _preexisting_socket(true),
  _force_closure(false),
  _timeout_in_tenths(600)                     // 1 minute
{ if (sp)
    _sock = *sp;
  else                          // sp is nullptr
  { try
    { _sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// enable re-use
      static const int on = 1;
      int status = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on) );    // char* cast is needed for Windows

      if (status)
        throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_REUSEADDR");

      const struct linger lgr { 1, 0 };

      status = setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&lgr, sizeof(lgr) );  // char* cast is needed for Windows

      if (status)
        throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_LINGER");    

      _preexisting_socket = false;
    }

    catch (...)
    { _close_the_socket();
      throw;
    }
  }
}

/*! \brief      Encapsulate a pre-existing socket
    \param  sp  socket
*/
tcp_socket::tcp_socket(SOCKET sp) :
  _destination_is_set(false),
  _preexisting_socket(true),
  _sock(sp),
  _force_closure(false),
  _timeout_in_tenths(600)                     // 1 minute
{  }

/*! \brief                          Construct and initialise with useful values
    \param  destination_ip_address  destination dotted decimal address or FQDN
    \param  destination_port        destination port
    \param  source_ip_address       address to which the socket is to be bound
*/
tcp_socket::tcp_socket(const string& destination_ip_address_or_fqdn, 
                       const unsigned int destination_port, 
                       const string& source_address,
                       const unsigned int retry_time_in_seconds) :
  _sock(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
  _destination_is_set(false),
  _preexisting_socket(false),
  _force_closure(false),
  _timeout_in_tenths(600)                    // 1 minute
{ 
  try
  { 
// enable re-use
    static const int on = 1;

    int status = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on) );      // char* cast is needed for Windows

    if (status)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_REUSEADDR");
    
    const struct linger lgr = { 1, 0 };

    status = setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&lgr, sizeof(lgr) );  // char* cast is needed for Windows

    if (status)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_LINGER");    

    bind(source_address);

// try to connect to the other end
    { const int TIMEOUT = 10;       // timeout in seconds
      bool connected = false;

      while (!connected)            // repeat until success
      { try
        { if (is_legal_ipv4_address(destination_ip_address_or_fqdn))
            destination(destination_ip_address_or_fqdn, destination_port, TIMEOUT);
          else                                                                // FQDN was passed instead of dotted decimal
          {
// resolve the name
            const string dotted_decimal = name_to_dotted_decimal(destination_ip_address_or_fqdn, 10);       // up to ten attempts at one-second intervals

            destination(dotted_decimal, destination_port, TIMEOUT );
          }

          connected = true;
        }

        catch (const socket_support_error& e)
        { ost << "caught socket_support_error exception while setting destination " << destination_ip_address_or_fqdn << " in tcp_socket constructor" << endl;
          ost << "Socket support error number " << e.code() << "; " << e.reason() << endl;

          alert("Error setting socket destination: " + destination_ip_address_or_fqdn +":" + to_string(destination_port));

          ost << "sleeping for " << retry_time_in_seconds << " seconds" << endl;
          sleep_for(seconds(retry_time_in_seconds));
        }
      }
    }
  }

  catch (const socket_support_error& e)
  { if (e.code() == SOCKET_SUPPORT_CONNECT_ERROR)
    { ost << "SOCKET_SUPPORT_CONNECT_ERROR" << e.reason() << endl;
    }
    else
    { ost << "socket_support_error: " << e.code() << ": " << e.reason() << endl;
      _close_the_socket();
      throw;
    }
  }

  catch (const tcp_socket_error& e)
  {
    { ost << "tcp_socket_error: " << e.code() << ": " << e.reason() << endl;
      _close_the_socket();
      throw;
    }
  }

  catch (...)                       // to ensure that socket is properly closed
  { ost << "Caught miscellaneous exception" << endl;
    _close_the_socket();
    throw;
  }
}

/// destructor
tcp_socket::~tcp_socket(void)
{ if (!_preexisting_socket)
    _close_the_socket();        // we have to close the socket if we are finished with it

  if (_preexisting_socket and _force_closure)
    _close_the_socket();
}

/*! \brief  New socket

    Switch to using a different underlying socket
*/
void tcp_socket::new_socket(void)
{ try
  { SAFELOCK(_tcp_socket);

    _sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// enable re-use
    static const int on = 1;
    int status = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on) );      // char* cast is needed for Windows

    if (status)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_REUSEADDR");
    
    const struct linger lgr = { 1, 0 };

    status = setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&lgr, sizeof(lgr) );  // char* cast is needed for Windows

    if (status)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_LINGER");      
  }
  
  catch (...)
  { throw;           // placeholder
  }
}

/*! \brief                  Bind the socket to a local address
    \param  local_address   address to which the socket is to be bound
*/
void tcp_socket::bind(const sockaddr_storage& local_address)
{ SAFELOCK(_tcp_socket);

  const int status = ::bind(_sock, (sockaddr*)&local_address, sizeof(local_address));

  if (status)
    throw socket_support_error(SOCKET_SUPPORT_BIND_ERROR, "Errno = " + to_string(errno) + "; " + strerror(errno));

  _bound_address = local_address;
}

/*! \brief          Connect to the far-end
    \param  adr     distal address
*/
void tcp_socket::destination(const sockaddr_storage& adr)
{ SAFELOCK(_tcp_socket);

  const int status = ::connect(_sock, (sockaddr*)(&adr), sizeof(adr));

  if (status == 0)
  { _destination = adr; 
    _destination_is_set = true;
  }
  else
  { _destination_is_set = false;
  
    const string address = dotted_decimal_address(*(sockaddr*)(&adr));
    const unsigned int p = port(*(sockaddr*)(&adr));
  
    throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, "Status " + to_string(errno) + " received from ::connect; " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));
  }
}

/*! \brief                  Connect to the far-end, with explicit time-out when trying to make connection
    \param  adr             address/port of the far end
    \param  timeout_secs    timeout in seconds

    See https://www.linuxquestions.org/questions/programming-9/connect-timeout-change-145433/
*/
void tcp_socket::destination(const sockaddr_storage& adr, const unsigned long timeout_secs)
{ struct timeval timeout { timeout_secs, 0L };
//  timeout.tv_sec = timeout_secs;
//  timeout.tv_usec = 0L;

  SAFELOCK(_tcp_socket);

  fd_set r_set, w_set;
  int flags;

  FD_ZERO(&r_set);
  FD_SET(_sock, &r_set);
  w_set = r_set;

//set socket nonblocking flag
  flags = fcntl(_sock, F_GETFL, 0);
  fcntl(_sock, F_SETFL, flags | O_NONBLOCK);

  int status = ::connect(_sock, (sockaddr*)(&adr), sizeof(adr));

  if (status == 0)        // all OK
  { _destination = adr;
    _destination_is_set = true;
  }
  else
  { _destination_is_set = false;

    const string address = dotted_decimal_address(*(sockaddr*)(&adr));
          const unsigned int p = port(*(sockaddr*)(&adr));

    if (errno != EINPROGRESS)
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, "Status " + to_string(errno) + " received from ::connect; " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));

    status = select(_sock + 1, &r_set, &w_set, NULL, (timeout_secs) ? &timeout : NULL);

    if (status < 0)
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, "EINPROGRESS: " + to_string(errno) + " received from ::connect; " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));

    if (status == 0)  // timed out
    { errno = ETIMEDOUT;
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, (string)"Timeout received from ::connect: " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));
    }

// select is positive, which means that the file descriptor is OK
    _destination_is_set = true;
  }
}

/*! \brief          Mark as connected to the far-end
    \param  adr     address/port of the far end

    Used for server sockets returned by .accept()
*/
void tcp_socket::connected(const sockaddr_storage& adr)
{ SAFELOCK(_tcp_socket);

  _destination = adr;
  _destination_is_set = true;
}

/*! \brief          Simple send
    \param  msg     message to send
  
    Does not look for a response
*/
void tcp_socket::send(const std::string& msg)
{ if (!_destination_is_set)
    throw tcp_socket_error(TCP_SOCKET_UNKNOWN_DESTINATION);

  SAFELOCK(_tcp_socket);

  const int status = ::send(_sock, msg.c_str(), msg.length(), 0);
  
  if (status == -1)
    throw tcp_socket_error(TCP_SOCKET_ERROR_IN_WRITE);   
}

/*! \brief      Simple receive
    \return     received string
*/
const string tcp_socket::read(void)
{ static const unsigned int BUFLEN = 4096;

  char cp[BUFLEN];
  string rv;
  int status;
  
  SAFELOCK(_tcp_socket);

  do
  { status  = ::recv(_sock, cp, BUFLEN, 0);

    if (status == -1)
      throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);

    if (status == 0)
      throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);    
    
    rv += string(cp, status);
  } while (status == BUFLEN);

  return rv;
}

/*! \brief                Simple receive
    \param  timeout_secs  timeout in seconds
    \return               received string

    Throws an exception if the read times out
*/
const string tcp_socket::read(const unsigned long timeout_secs)
{ string rv;

  struct timeval timeout { timeout_secs, 0L };
//  timeout.tv_sec = timeout_secs;
//  timeout.tv_usec = 0L;

  fd_set ps_set;

  FD_ZERO(&ps_set);
  FD_SET(_sock, &ps_set);

  SAFELOCK(_tcp_socket);

// check
  if (!FD_ISSET(_sock, &ps_set))                         // unable to set socket for listening
  { ost << "Throwing SOCKET_SUPPORT_UNABLE_TO_LISTEN" << endl;
    throw socket_support_error(SOCKET_SUPPORT_UNABLE_TO_LISTEN);
  }

  const int max_socket_number = _sock + 1;               // see p. 292 of Linux socket programming

  int socket_status = select(max_socket_number, &ps_set, NULL, NULL, &timeout);  // under Linux, timeout has the remaining time, but this is not to be relied on because it's not generally true in other systems. See Linux select() man page
  switch (socket_status)
  { case 0:                    // timeout
    { if (timeout_secs)        // don't signal timeout if we weren't given a duration to wait
      { ost << "Throwing SOCKET_SUPPORT_TIMEOUT; timeout in read() after " << to_string(timeout_secs) << " seconds" << endl;
        throw socket_support_error(SOCKET_SUPPORT_TIMEOUT, "Timeout after " + to_string(timeout_secs) + " seconds");
      }
      break;
    }

    case SOCKET_ERROR:
    { ost << "Throwing SOCKET_SUPPORT_SELECT_ERROR in read()" << endl;
      throw socket_support_error(SOCKET_SUPPORT_SELECT_ERROR);
    }

    default:                   // response is waiting to be read
    { const int BUFSIZE = 4096;

      char cp[BUFSIZE];    // a reasonable sized buffer
      int status;
  
      do
      { status  = ::recv(_sock, cp, BUFSIZE, 0);

        if (status == -1)
        { const string msg = "errno = " + to_string(errno) + ": " + strerror(errno);

          ost << "Throwing TCP_SOCKET_ERROR_IN_RECV;" << msg << endl;
          throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV, msg);
        }

        rv += string(cp, status);
      }     while (status == BUFSIZE);

      break;
    }
  }

  return rv;
}

/*! \brief              Set the idle time before a keep-alive is sent
    \param  seconds     time to wait idly before a keep-alive is sent
*/
void tcp_socket::idle_time(const unsigned int seconds)
{ const int optval = seconds;
  const int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting idle time");
}

/*! \brief              Set the time between keep-alives
    \param  seconds     time to wait idly before a keep-alive is sent
*/
void tcp_socket::retry_time(const unsigned int seconds)
{ const int optval = seconds;
  const int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting retry time");
}

/*! \brief      Set the maximum number of retries
    \param  n   maximum number of retries
*/
void tcp_socket::max_retries(const unsigned int n)
{ const int optval = n;
  const int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting maximum number of retries");
}

/*! \brief          Set or unset the use of keep-alives
    \param  torf    whether to use keep-alives
*/
void tcp_socket::keep_alive(const bool torf)
{ const int optval = torf ? 1 : 0;
  const int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error to control keep-alive");
}

/*! \brief          Set properties of the keep-alive
    \param  idle    idle time in seconds
    \param  retry   retry time in seconds
    \param  n       maximum number of retries
*/
void tcp_socket::keep_alive(const unsigned int idle, const unsigned int retry, const unsigned int n)
{ keep_alive();           // turn on keep-alive
  idle_time(idle);
  retry_time(retry);
  max_retries(n);
}

// ---------------------------------------------- generic socket functions ---------------------

/*! \brief                              Read a socket
    \param  in_socket                   inbound socket
    \param  timeout_in_tenths           timeout in tenths of a second
    \param  buffer_length_for_reply     maximum allowed length of reply
    \return                             response

    Throws socket_support_error(SOCKET_TIMEOUT) if the socket times out
*/
const string read_socket(SOCKET& in_socket, const int timeout_in_tenths, const int buffer_length_for_reply)
{
// wait for response
  struct timeval timeout;
  timeout.tv_sec = timeout_in_tenths / 10;
  timeout.tv_usec = (timeout_in_tenths - timeout.tv_sec * 10) * 100000L;

  fd_set ps_set;

  FD_ZERO(&ps_set);
  FD_SET(in_socket, &ps_set);

// check
  if (!FD_ISSET(in_socket, &ps_set))      // unable to set socket for listening
    throw socket_support_error(SOCKET_SUPPORT_UNABLE_TO_LISTEN);

  const int max_socket_number = in_socket + 1;               // see p. 292 of Linux socket programming

  int socket_status = select(max_socket_number, &ps_set, NULL, NULL, &timeout);  // under Linux, timeout has the remaining time, but this is not to be relied on because it's not generally true in other systems. See Linux select() man page

  switch (socket_status)
  { case 0:                    // timeout
      throw socket_support_error(SOCKET_SUPPORT_TIMEOUT, (string)"Socket timeout exceeded: " + to_string(timeout_in_tenths) + (string)" tenths of seconds");

    case SOCKET_ERROR:
      throw socket_support_error(SOCKET_SUPPORT_SELECT_ERROR);

    default:                   // response is waiting to be read
    { char socket_buffer[4096];
      socklen_t from_length = sizeof(sockaddr_storage);
      sockaddr_storage ps_sockaddr;                    // unused but needed in recvfrom

      socket_status = recvfrom(in_socket, socket_buffer, buffer_length_for_reply, 0, (sockaddr*)&ps_sockaddr, &from_length); 
      if (socket_status == SOCKET_ERROR)
      { socket_status = errno;
        switch (socket_status)
        { case EFAULT:
            throw socket_support_error(SOCKET_SUPPORT_EFAULT);

          default:
            throw socket_support_error(SOCKET_SUPPORT_RECVFROM_ERROR);
        }
      }
      else                                   // have read message from other end OK
      { const int octets_received = socket_status;

        return string(&(socket_buffer[0]), octets_received);
      }
    }
  }
}

/*! \brief          Flush a readable socket
    \param  sock    socket to flush
*/
void flush_read_socket(SOCKET& sock)
{ char socket_buffer[1024];                           // read 1024 octets at a time
  struct timeval timeout { 0, 0 };

  fd_set ps_set;

  FD_ZERO(&ps_set);
  FD_SET(sock, &ps_set);
  const int max_socket_number= sock + 1;               // see p. 292 of Linux socket programming

  socklen_t from_length = sizeof(sockaddr_storage);
  sockaddr_storage ps_sockaddr;                                // unused but needed in recvfrom

  while (select(max_socket_number, &ps_set, NULL, NULL, &timeout) == 1)
  { recvfrom(sock, socket_buffer, 1024, 0, (sockaddr*)&ps_sockaddr, &from_length); 

// reset the timeout
//    timeout.tv_sec = 0;
//    timeout.tv_usec = 0;
    timeout =  { 0, 0 };

// and the set
    FD_ZERO(&ps_set);
    FD_SET(sock, &ps_set);
  }
}

// ---------------------------  socket_address  ------------------------------

/*! \brief  Obtain sockaddr_storage corresponding to an address and port
    \param  ip_address_as_long  IP address, in network order
    \param  port_nr             port number, in host order
    \return                     Address and port as sockaddr_storage

    IPv4 only
*/
const sockaddr_storage socket_address(const unsigned long ip_address_as_long, const short port_nr)
{ sockaddr_storage rv;
  sockaddr_in* sinp = (sockaddr_in*)(&rv);
  
  sinp->sin_family = AF_INET;
  sinp->sin_port = htons(port_nr);
  sinp->sin_addr.s_addr = ip_address_as_long;

  return rv;
}

// ---------------------------  process_socket_error  ------------------------------

// this encapsulates OS differences in a single place
void process_socket_error(const int original_socket_status)
{

#if 0
  if (original_socket_status == 0)                           // no error
    return;

  int socket_status = original_socket_status;

  if (socket_status == SOCKET_ERROR)
  { socket_status = errno;
    switch (socket_status)
    { case EAGAIN:                     // should only be used when in non-blocking mode
        break;
      default:
        break;
    }
  }
#endif

}

/*! \brief      Convert a sockaddr_storage to a sockaddr_in
    \param  ss  value to be converted
    \return     <i>ss</i> as a sockaddr_in
  
    Throws socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL) if <i>ss</i> is not an IPv4 address
*/
const sockaddr_in to_sockaddr_in(const sockaddr_storage& ss)
{ sockaddr_in rv;

  if (ss.ss_family != AF_INET)
    throw socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL, "Attempt to convert non-IPv4 sockaddr_storage to sockaddr_in");

  rv.sin_family = AF_INET;
  rv.sin_port = ((sockaddr_in*)(&ss))->sin_port;
  rv.sin_addr = ((sockaddr_in*)(&ss))->sin_addr;

  return rv;
}

/*! \brief              Convert a name to a dotted decimal IP address
    \param  fqdn        name to be resolved
    \param  n_tries     maximum number of tries
    \return             equivalent IP address in dotted decimal format

    Throws exception if the name cannot be resolved. Uses gethostbyname_r() to perform the lookup.
    <i>n_tries</i> is present because gethostbyname_r() cannot be relied on to complete a remote
    lookup before deciding to return with an error.
*/
string name_to_dotted_decimal(const string& fqdn, const unsigned int n_tries)
{ if (fqdn.empty())                  // gethostbyname (at least on Windows) is hosed if one calls it with null string
    throw socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL, "Asked to lookup empty name");
  
  const int BUF_LEN = 2048;
  struct hostent ret;
  char buf[BUF_LEN];
  const size_t buflen(BUF_LEN);
  struct hostent* result;
  int h_errnop;

  remove_const_t<decltype(n_tries)> n_try = 0;
  bool success = false;
  int status;

  while (n_try++ < n_tries and !success)
  { status = gethostbyname_r(fqdn.c_str(), &ret, &buf[0], buflen, &result, &h_errnop);

    ost << "name_to_dotted_decimal status = " << status << endl;
    success = (status == 0) and (result != nullptr);    // the second test should be redundant

    if (!success and n_try != n_tries)
      sleep_for(seconds(1));
  }

  if (success)
  { char** h_addr_list = result->h_addr_list;
    const uint32_t addr_long = htonl(*(uint32_t*)(*h_addr_list));    // host order

    return convert_to_dotted_decimal(addr_long);
  }
  else
  { ost << "Error return" << endl;

    throw(tcp_socket_error(TCP_SOCKET_UNABLE_TO_RESOLVE, (string)"Unable to resolve name: " + fqdn));
  }
}
