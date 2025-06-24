// $Id: socket_support.cpp 269 2025-05-19 22:42:59Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   socket_support.cpp

    Objects and functions related to sockets. This code is based, with permission,  on
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

extern void alert(const string& msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);     ///< alert the user

constexpr size_t MIN_IN_BUFFER_SIZE { 4'096 };   // minumum size of a TCP input buffer, in bytes
//constexpr size_t MAX_IN_BUFFER_SIZE { 8'192 };   // maximum size of a TCP input buffer
//constexpr size_t MAX_IN_BUFFER_SIZE { 16'384 };   // maximum size of a TCP input buffer
constexpr size_t MAX_IN_BUFFER_SIZE { 32'768 };   // maximum size of a TCP input buffer, in bytes

constexpr int SOCKET_ERROR { -1 };            ///< error return from various socket-related system functions

/*! \brief          Set an option for a file descriptor
    \param  option  the option to set
    \param  fd      file descriptor on which the option <i>option</i> is to be set

    See https://www.linuxquestions.org/questions/programming-9/connect-timeout-change-145433/
    NB: a socket is a valid file descriptor
*/
void fd_set_option(const int opt, const int fd)  // e.g., O_NONBLOCK
{ //int flags;

  //flags = fcntl(fd, F_GETFL, 0);
  int flags { fcntl(fd, F_GETFL, 0) };

//  int status = fcntl(fd, F_SETFL, flags | opt);

  if (const int status { fcntl(fd, F_SETFL, flags | opt) }; status == -1)
    throw socket_support_error(SOCKET_SUPPORT_FLAG_ERROR);
}

// ---------------------------------  tcp_socket  -------------------------------

constexpr struct linger DEFAULT_TCP_LINGER { false, 0 };      // the default is not to linger

/// close the socket
void tcp_socket::_close_the_socket(void)
{ if (_sock)
  { SAFELOCK(_tcp_socket);
  
    if (const int status { ::close(_sock) }; status == -1)
      throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_CLOSE, strerror(errno));
  }
}

/*! \brief create an input buffer if one doesn't exist
*/
void tcp_socket::_create_buffer_if_necessary(void) const  // logically const
{ if (_in_buffer_p == nullptr)
  { _in_buffer_p    = new char[MIN_IN_BUFFER_SIZE];
    _in_buffer_size = MIN_IN_BUFFER_SIZE;
  }
}

/*! \brief    Resize the buffer
    \return   whether the buffer was actually resized

    Doubles the size of the input buffer, or sets it to MAX_IN_BUFFER_SIZE, whichever is less
*/
bool tcp_socket::_resize_buffer(void) const  // logically const
{ if (_in_buffer_size < MAX_IN_BUFFER_SIZE)
  { const size_t new_buffer_size { min(MAX_IN_BUFFER_SIZE, _in_buffer_size * 2) };

//    ost << "resizing buffer to " << new_buffer_size << " bytes" << endl;

    delete [] _in_buffer_p;

    _in_buffer_size = new_buffer_size;
    _in_buffer_p = new char[_in_buffer_size];

    return true;
  }

  return false;
}

/*! \brief            Resize the buffer
    \param  new_size  the new size of the buffer
    \return   whether the buffer was actually resized

    If the current size is not already MAX_IN_BUFFER_SIZE, then sets the size of the input buffer
    to <i>new_size</i>, or sets it to MAX_IN_BUFFER_SIZE, whichever is less
*/
bool tcp_socket::_resize_buffer(const size_t new_size) const   // logically const
{ if ( (_in_buffer_size < MAX_IN_BUFFER_SIZE) and (new_size > _in_buffer_size) )
  { const size_t new_buffer_size { min(MAX_IN_BUFFER_SIZE, new_size) };

    ost << "resizing buffer to " << new_buffer_size << " bytes" << endl;

    delete [] _in_buffer_p;

    _in_buffer_size = new_buffer_size;
    _in_buffer_p = new char[_in_buffer_size];

    return true;
  }

  return false;
}

/// default constructor
tcp_socket::tcp_socket(void) :
  _sock(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{ try
  { reuse();    // enable re-use
    linger(DEFAULT_TCP_LINGER);
  }

  catch (...)
  { _close_the_socket();
    throw;
  }
}

/*! \brief      Encapsulate a pre-existing socket
    \param  sp  pointer to socket
  
    Acts as default constructor if passed pointer is nullptr
*/
tcp_socket::tcp_socket(SOCKET* sp)
{ if (sp)
  { _sock = *sp;
    _preexisting_socket = true;
  }
  else                          // sp is nullptr
  { try
    { _sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      reuse();            // enable re-use
      linger(DEFAULT_TCP_LINGER);
    }

    catch (...)
    { _close_the_socket();
      throw;
    }
  }
}

/*! \brief                                  Construct and initialise with useful values
    \param  destination_ip_address_or_fqdn  destination dotted decimal address or FQDN
    \param  destination_port                destination port
    \param  source_address                  IP address to which the socket is to be bound
    \param  retry_time_in_seconds           time between retries, in seconds
*/
tcp_socket::tcp_socket(const string& destination_ip_address_or_fqdn, 
                       const unsigned int destination_port, 
                       const string& source_address,
                       const unsigned int retry_time_in_seconds) :
  _sock(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{ try
  { reuse();                        // enable re-use
    linger(DEFAULT_TCP_LINGER);

    bind(source_address);

// try to connect to the other end
    { constexpr int TIMEOUT { 10 };       // timeout in seconds

      bool connected { false };

      while (!connected)            // repeat until success
      { try
        { if (is_legal_ipv4_address(destination_ip_address_or_fqdn))
          { destination(destination_ip_address_or_fqdn, destination_port, TIMEOUT);
            rename_mutex("TCP: "s + destination_ip_address_or_fqdn + ":"s + ::to_string(destination_port));
          }
          else                                                                // FQDN was passed instead of dotted decimal
          {
// resolve the name
            const string dotted_decimal { name_to_dotted_decimal(destination_ip_address_or_fqdn, 10) };       // up to ten attempts at one-second intervals

            destination(dotted_decimal, destination_port, TIMEOUT );
            rename_mutex("TCP: "s + dotted_decimal + ":"s + ::to_string(destination_port));
          }

          connected = true;
        }

        catch (const socket_support_error& e)
        { alert("Error setting socket destination: "s + destination_ip_address_or_fqdn +":"s + ::to_string(destination_port));

          ost << "sleeping for " << retry_time_in_seconds << " seconds" << endl;
          sleep_for(seconds(retry_time_in_seconds));
        }
      }
    }
  }

  catch (const socket_support_error& e)
  { if (e.code() == SOCKET_SUPPORT_CONNECT_ERROR)
      ost << "SOCKET_SUPPORT_CONNECT_ERROR" << e.reason() << endl;
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
{ try
  { if ( (!_preexisting_socket) or (_preexisting_socket and _force_closure) )
      _close_the_socket();        // we have to close the socket if we are finished with it
  }

  catch (...)
  { try
    { ost << "UNABLE TO CLOSE SOCKET; " << strerror(errno) << endl;
    }

    catch (...)
    { }
  }

// free the storage associated with the input buffer
  if (_in_buffer_p != nullptr)
  { delete [] _in_buffer_p;

    _in_buffer_p = nullptr;
    _in_buffer_size = 0;
  }
}

/*! \brief  Create and use a different underlying socket
*/
void tcp_socket::new_socket(void)
{ try
  { SAFELOCK(_tcp_socket);

    _sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    reuse();    // enable re-use
    linger(DEFAULT_TCP_LINGER);

// probably should clear out pre-existing information; but we keep the old data buffer (that is removed only by the destructor)
    _bound_address = sockaddr_storage { };
    _destination = sockaddr_storage { };
    _destination_is_set = false;
    _preexisting_socket = false;
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

  if (const int status { ::bind(_sock, (sockaddr*)&local_address, sizeof(local_address)) }; status)
  { const string address { dotted_decimal_address(*(sockaddr*)(&local_address)) };

    ost << "Bind error; errno = " << ::to_string(errno) << "; "s << strerror(errno) << "; address = "s << address << endl;

    throw socket_support_error(SOCKET_SUPPORT_BIND_ERROR, "Bind error; errno = "s + ::to_string(errno) + "; "s + strerror(errno) + "; address = "s + address);
  }

  _bound_address = local_address;
}

/*! \brief          Connect to the far end
    \param  adr     distal address
*/
void tcp_socket::destination(const sockaddr_storage& adr)
{ SAFELOCK(_tcp_socket);

  if (const int status { ::connect(_sock, (sockaddr*)(&adr), sizeof(adr)) }; status == 0)
  { _destination = adr; 
    _destination_is_set = true;
  }
  else
  { ost << "unable to connect to destination in tcp_socket::destination()" << endl;
    _destination_is_set = false;
  
    const string       address { dotted_decimal_address(*(sockaddr*)(&adr)) };
    const unsigned int p       { port(*(sockaddr*)(&adr)) };
  
    throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, "Status "s + ::to_string(errno) + " received from ::connect; "s + strerror(errno) + " while trying to connect to address "s 
                                                               + address + "; port "s + ::to_string(p));
  }
}

/*! \brief                  Connect to the far-end, with explicit time-out when trying to make connection
    \param  adr             address/port of the far end
    \param  timeout_secs    timeout in seconds

    See https://www.linuxquestions.org/questions/programming-9/connect-timeout-change-145433/
*/
void tcp_socket::destination(const sockaddr_storage& adr, const unsigned long timeout_secs)
{ struct timeval timeout { static_cast<time_t>(timeout_secs), 0L };

  SAFELOCK(_tcp_socket);

  fd_set r_set, w_set;

  fd_set_value(r_set, _sock);
  w_set = r_set;

// set socket nonblocking flag
  int flags { fcntl(_sock, F_GETFL, 0) };

  fcntl(_sock, F_SETFL, flags | O_NONBLOCK);

  int status { ::connect(_sock, (sockaddr*)(&adr), sizeof(adr)) };

  if (status == 0)        // all OK
  { _destination = adr;
    _destination_is_set = true;
  }
  else
  { _destination_is_set = false;

    const string       address { dotted_decimal_address(*(sockaddr*)(&adr)) };
    const unsigned int p       { port(*(sockaddr*)(&adr)) };

    if (errno != EINPROGRESS)
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR,
                                   "Status "s + ::to_string(errno) + " received from ::connect; "s + strerror(errno) + " while trying to connect to address "s + address + "; port "s + ::to_string(p));

    status = select(_sock + 1, &r_set, &w_set, NULL, (timeout_secs) ? &timeout : NULL);

    if (status < 0)
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR,
                                   "EINPROGRESS: "s + ::to_string(errno) + " received from ::connect; "s + strerror(errno) + " while trying to connect to address "s + address + "; port "s + ::to_string(p));

    if (status == 0)  // timed out
    { errno = ETIMEDOUT;
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR,
                                   "Timeout received from ::connect: "s + strerror(errno) + " while trying to connect to address "s + address + "; port "s + ::to_string(p));
    }

// select is positive, which means that the file descriptor is OK
    _destination_is_set = true;
    _destination = adr;
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
  
    Does not look for a response. Throws an exception if there is any problem.
*/
void tcp_socket::send(const std::string_view msg)
{ if (!_destination_is_set)
  { ost << "Error in tcp_socket::send(); destination unknown" << endl;
    throw tcp_socket_error(TCP_SOCKET_UNKNOWN_DESTINATION);
  }

  SAFELOCK(_tcp_socket);

  const ssize_t status { ::send(_sock, msg.data(), msg.length(), 0) };

  if (status == -1)
  { ost << "Error in tcp_socket::send(); status -1 returned from ::send() when sending message: " << msg << endl;
    throw tcp_socket_error(TCP_SOCKET_ERROR_IN_WRITE);
  }
}

/*! \brief      Simple receive
    \return     received string
*/
string tcp_socket::read(void) const
{ _create_buffer_if_necessary();    // check that we have a place to put read data

  const char* cp { _in_buffer_p };

  string rv { };

  int    status;

  bool filled_buffer { false };       // indicate whether we filled the input buffer

  SAFELOCK(_tcp_socket);

  do
  { status = ::recv(_sock, (char*)cp, _in_buffer_size, 0);      // status is the number of bytes received; waits for at least some data to be present

    if (status == -1)
      throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);

    if (status == 0)
      throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);    
    
    if (status == static_cast<int>(_in_buffer_size))
    { ost << "Informative: TCP buffer (size = " << _in_buffer_size << ") filled in read without timeout" << endl;  // if this happens often, then should probably read more often or increase size of buffer

      filled_buffer = true;
    }

    rv += string { cp, static_cast<size_t>(status) };
  } while (status == static_cast<int>(_in_buffer_size));

  if (filled_buffer)
    _resize_buffer();

  return rv;
}

/*! \brief                Simple receive
    \param  timeout_secs  timeout in seconds
    \return               received string

    Throws an exception if the read times out
*/
string tcp_socket::read(const unsigned long timeout_secs) const
{ _create_buffer_if_necessary();    // check that we have a place to put read data

  string rv { };

  struct timeval timeout { static_cast<time_t>(timeout_secs), 0L };

  fd_set ps_set;

  fd_set_value(ps_set, _sock);

  SAFELOCK(_tcp_socket);

// check
  if (!FD_ISSET(_sock, &ps_set))                            // unable to set socket for listening
  { ost << "Throwing SOCKET_SUPPORT_UNABLE_TO_LISTEN" << endl;
    throw socket_support_error(SOCKET_SUPPORT_UNABLE_TO_LISTEN);
  }

  const int max_socket_number { _sock + 1 };               // see p. 292 of Linux socket programming

  int socket_status { select(max_socket_number, &ps_set, NULL, NULL, &timeout) };  // under Linux, timeout has the remaining time, but this is not to be relied on because it's not generally true in other systems. See Linux select() man page

  bool filled_buffer { false };       // indicate whether we filled the input buffer

  switch (socket_status)
  { case 0:                    // timeout
    { if (timeout_secs)        // don't signal timeout if we weren't given a duration to wait
        throw socket_support_error(SOCKET_SUPPORT_TIMEOUT, "Timeout after "s + ::to_string(timeout_secs) + " seconds"s);
      break;
    }

    case SOCKET_ERROR:
    { ost << "Throwing SOCKET_SUPPORT_SELECT_ERROR in read()" << endl;
      throw socket_support_error(SOCKET_SUPPORT_SELECT_ERROR);
    }

    default:                            // response is waiting to be read, at least in theory... sometimes it seems that in fact zero bytes are read
    { const char* cp { _in_buffer_p };

      int status;

      int error_count { 0 };
      bool retry { false };
  
      do
      { retry = false;

        status = ::recv(_sock, (char*)cp, _in_buffer_size, 0);

        if (status == static_cast<int>(_in_buffer_size))
        { ost << "Informative: TCP buffer (size = " << _in_buffer_size << ") filled in read with timeout" << endl;  // if this happens often, then should probably read more often or increase size of buffer

          filled_buffer = true;
        }

        if (status == -1)
        { switch (errno)
          { case 11 :         // Resource temporarily unavailable
            { error_count++;

              if (error_count <= 5)
              { const string msg { "errno = "s + ::to_string(errno) + ": "s + strerror(errno) + " error count = " + ::to_string(error_count)};

                ost << "ERROR in RECV: " << msg << endl;

                sleep_for(1s);  // insert a pause
                retry = true;
              }
              else
              { const string msg { "errno = "s + ::to_string(errno) + ": "s + strerror(errno) };

                ost << "Too many errors in read(): throwing TCP_SOCKET_ERROR_IN_RECV; " << msg << endl;
                throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV, msg);
              }

              break;
            }

            default :
            { const string msg { "errno = "s + ::to_string(errno) + ": "s + strerror(errno) };

              ost << "Throwing TCP_SOCKET_ERROR_IN_RECV; " << msg << endl;
              throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV, msg);
            }
          }
        }

        rv += ( (status == -1) ? EMPTY_STR : string { cp, static_cast<size_t>(status)} );
      } while ((status == static_cast<int>(_in_buffer_size)) or (retry == true));

      if (filled_buffer)
        _resize_buffer();

      break;
    }
  }

  return rv;
}

/*! \brief              Set the idle time before a keep-alive is sent
    \param  seconds     time to wait idly before a keep-alive is sent, in seconds
*/
void tcp_socket::keep_alive_idle_time(const unsigned int seconds)
{ const int optval { static_cast<int>(seconds) };
  
  SAFELOCK(_tcp_socket);

  if (const int status { setsockopt(_sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting idle time"s);
}

/*! \brief          Get the idle time before a keep-alive is sent
    \param  return  time to wait idly, in seconds, before a keep-alive is sent
*/
unsigned int tcp_socket::keep_alive_idle_time(void) const
{ int rv;

  socklen_t rv_len { sizeof(rv) };    // can't be const(!)

  SAFELOCK(_tcp_socket);

  if (const int status { getsockopt(_sock, IPPROTO_TCP, TCP_KEEPIDLE, &rv, &rv_len) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_GET_OPTION, "Error getting idle time"s);

  return static_cast<unsigned int>(rv);
}

/*! \brief    Get the time between keep-alives
    \return   time to wait idly between keep-alives, in seconds
*/
unsigned int tcp_socket::keep_alive_retry_time(void) const
{ int rv;

  socklen_t rv_len { sizeof(rv) };    // can't be const(!)

  SAFELOCK(_tcp_socket);

  if (const int status { getsockopt(_sock, IPPROTO_TCP, TCP_KEEPINTVL, &rv, &rv_len) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_GET_OPTION, "Error getting retry time"s);

  return static_cast<unsigned int>(rv);
}

/*! \brief              Set the time between keep-alives
    \param  seconds     time to wait idly before a keep-alive is sent
*/
void tcp_socket::keep_alive_retry_time(const unsigned int seconds)
{ const int optval { static_cast<int>(seconds) };
  
  SAFELOCK(_tcp_socket);

  if (const int status { setsockopt(_sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting retry time"s);
}

/*! \brief     Get the maximum number of retries
    \return   maximum number of retries before notifying upwards
*/
unsigned int tcp_socket::keep_alive_max_retries(void) const
{ int rv;

  socklen_t rv_len { sizeof(rv) };    // can't be const(!)

  SAFELOCK(_tcp_socket);

  if (const int status { getsockopt(_sock, IPPROTO_TCP, TCP_KEEPCNT, &rv, &rv_len) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_GET_OPTION, "Error getting maximum number of retries"s);

  return static_cast<unsigned int>(rv);
}

/*! \brief      Set the maximum number of retries
    \param  n   maximum number of retries before notifying upwards
*/
void tcp_socket::keep_alive_max_retries(const unsigned int n)
{ const int optval { static_cast<int>(n) };
  
  SAFELOCK(_tcp_socket);

  if (const int status { setsockopt(_sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting maximum number of retries"s);
}

/*  \brief    Is a keep-alive in use on this socket?
    \return   whether a keep-alive is in use
*/
bool tcp_socket::keep_alive(void) const
{ int rv;

  socklen_t rv_len { sizeof(rv) };    // can't be const(!)

  SAFELOCK(_tcp_socket);

  if (const int status { getsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, &rv, &rv_len) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_GET_OPTION, "Error getting SO_KEEPALIVE"s);

  return static_cast<unsigned int>(rv);
}

/*! \brief          Set or unset the use of keep-alives
    \param  torf    whether to use keep-alives

    Throws a tcp_socket_error if an error occurs
*/
void tcp_socket::keep_alive(const bool torf)
{ const int optval { torf ? 1 : 0 };
  
  SAFELOCK(_tcp_socket);

  if ( setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) )
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_KEEPALIVE"s);
}

/*! \brief          Set properties of the keep-alive
    \param  idle    idle time in seconds
    \param  retry   retry time in seconds
    \param  n       maximum number of retries
*/
void tcp_socket::keep_alive(const unsigned int idle, const unsigned int retry, const unsigned int n)
{ keep_alive(true);           // turn on keep-alive
  keep_alive_idle_time(idle);
  keep_alive_retry_time(retry);
  keep_alive_max_retries(n);
}

/*! \brief          Set or unset the re-use of the socket
    \param  torf    whether to allow re-use

    Throws a tcp_socket_error if an error occurs
*/
void tcp_socket::reuse(const bool torf)
{ const int optval { torf ? 1 : 0 };
  
  SAFELOCK(_tcp_socket);

  if ( setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) )
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_REUSEADDR"s);
}

/*! \brief    Get the lingering state of the socket
    \return   whether linger is enabled and, if so, the value in seconds

    Throws a tcp_socket_error if an error occurs
*/
optional<int> tcp_socket::linger(void) const
{ struct linger ling_val;

  socklen_t ling_len { sizeof(ling_val) };    // can't be const(!)

  SAFELOCK(_tcp_socket);

  if (const int status { getsockopt(_sock, SOL_SOCKET, SO_LINGER, &ling_val, &ling_len) }; status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_GET_OPTION, "Error getting SO_KEEPALIVE"s);

  optional<int> rv { };

  if (ling_val.l_onoff)
    rv = ling_val.l_linger;

  return rv;
}

/*! \brief          Set or unset lingering of the socket
    \param  lng     struct linger object

    Throws a tcp_socket_error if an error occurs
*/
void tcp_socket::linger(const struct linger& lngr)
{ SAFELOCK(_tcp_socket);

  if ( setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&lngr, sizeof(lngr) ) )
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_LINGER"s);
}

/*! \brief          Set or unset lingering of the socket
    \param  torf    whether to allow lingering
    \param  secs    linger time in seconds

    Throws a tcp_socket_error if an error occurs
*/
void tcp_socket::linger(const bool torf, const int secs)
{ const struct linger lgr { torf ? 1 : 0, secs };

  SAFELOCK(_tcp_socket);

  if ( setsockopt(_sock, SOL_SOCKET, SO_LINGER, (char*)&lgr, sizeof(lgr) ) )
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_LINGER"s);
}

/// Human-readable string description of the status of the socket
string tcp_socket::to_string(void) const
{ string rv { };

  rv += "bound address: "s + dotted_decimal_address(_bound_address) + EOL;

  if (_destination_is_set)
    rv += "destination is set to: "s + dotted_decimal_address(_destination) + EOL;
  else
    rv += "destination is not set"s + EOL;

  rv += "closure will "s + (_force_closure ? ""s : "NOT "s) + "be forced"s  + EOL;
  rv += "socket was "s + (_preexisting_socket ? ""s : "NOT "s) + "pre-existing"s  + EOL;
  rv += "encapsulated socket number: "s + ::to_string(_sock) + EOL;
  rv += "timout in tenths of a second: "s + ::to_string(_timeout_in_tenths) + EOL;

  if (keep_alive())
  { rv += "Keep-alive is set:"s + EOL;
    rv += " keep-alive idle time: "s + ::to_string(keep_alive_idle_time()) + " seconds"s + EOL;
    rv += " keep-alive retry time: "s + ::to_string(keep_alive_retry_time()) + " seconds"s + EOL;
    rv += " keep-alive max retries: "s + ::to_string(keep_alive_max_retries()) + EOL;
  }
  else
    rv += "Keep-alive is not set"s + EOL;

  const optional<int> ling_state { linger() };

  if (ling_state)
    rv += "linger is set to "s + ::to_string(ling_state.value()) + " seconds"s;
  else
    rv += "linger is not set"s;

  return rv;
}

// ---------------------------------  icmp_socket  -------------------------------

// see, for example, https://stackoverflow.com/questions/8290046/icmp-sockets-linux

/// default constructor
icmp_socket::icmp_socket(void) :
  _sock(::socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP))
{ if (_sock <= 0)
    throw icmp_socket_error(ICMP_SOCKET_UNABLE_TO_CREATE, strerror(errno));

//  set_nonblocking(_sock);
}

/*! \brief                                  Create and associate with a particular destination
    \param  destination_ip_address_or_fqdn  IPv4 address or an FQDN
*/
icmp_socket::icmp_socket(const string& destination_ip_address_or_fqdn) :
  _destination_str(destination_ip_address_or_fqdn),
  _sock(::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_ICMP))
{ if (_sock <= 0)
    throw icmp_socket_error(ICMP_SOCKET_UNABLE_TO_CREATE, strerror(errno));

//  set_nonblocking(_sock);

  _dest.sin_family = AF_INET;

  constexpr unsigned int N_RESOLUTION_TRIES { 10 };

  const string dest_str { is_legal_ipv4_address(destination_ip_address_or_fqdn) ? destination_ip_address_or_fqdn : name_to_dotted_decimal(destination_ip_address_or_fqdn, N_RESOLUTION_TRIES) };

  inet_aton(dest_str.c_str(), &_dest.sin_addr);   // do I have a function of my own to replace this?

  _icmp_hdr.type = ICMP_ECHO;
  _icmp_hdr.un.echo.id = 1234;  //arbitrary id

  rename_mutex("ICMP: "s + destination_ip_address_or_fqdn + ":"s);
}

/*! \brief                                  Create and associate with a particular destination and local address
    \param  destination_ip_address_or_fqdn  IPv4 address or an FQDN
    \param  dotted_decimal_address          local IPv4 address
*/
icmp_socket::icmp_socket(const string& destination_ip_address_or_fqdn, const string& dotted_decimal_address)
{ *this = icmp_socket(destination_ip_address_or_fqdn);

  rename_mutex("ICMP: "s + destination_ip_address_or_fqdn + ":"s);

  bind(dotted_decimal_address);
}

/*! \brief                  Bind the socket to a local address
    \param  local_address   address to which the socket is to be bound
*/
void icmp_socket::bind(const sockaddr_storage& local_address)
{ if (const int status { ::bind(_sock, (sockaddr*)&local_address, sizeof(local_address)) }; status)
  { const string address { dotted_decimal_address(*(sockaddr*)(&local_address)) };

    throw socket_support_error(SOCKET_SUPPORT_BIND_ERROR, "Bind error; errno = "s + ::to_string(errno) + "; "s + strerror(errno) + "; address = "s + address);
  }

  _bound_address = local_address;
}

/*! \brief                          Perform a ping

    This is ridiculously C-ish, basically taken from: https://stackoverflow.com/questions/8290046/icmp-sockets-linux

    And, of course, it's broken.
    250217: Now fixed, by forcing the socket to be non-blocking. The bug was in the original code from the above URL.
*/
bool icmp_socket::ping(void)
{ SAFELOCK(_icmp_socket);

  bool rv { false };

  try
  { unsigned char data[2048];

    const char* payload { "drlog" };

    fd_set read_set;
    struct icmphdr rcv_hdr;

    _icmp_hdr.un.echo.sequence = _sequence_nr++;

    memcpy(data, &_icmp_hdr, sizeof(_icmp_hdr));
    memcpy(data + sizeof(_icmp_hdr), payload, strlen(payload)); //icmp payload

    if (const ssize_t status { sendto(_sock, data, sizeof(_icmp_hdr) + strlen(payload), 0, (struct sockaddr*)&_dest, sizeof(_dest)) }; (status < 0))
    { //ost << "ping status 1 = " << status << endl;
      throw icmp_socket_error(ICMP_SOCKET_SEND_ERROR, "sendto error; errno = "s + ::to_string(errno) + ": "s + strerror(errno));
    }
//    else
//      ost << "ping status 1 = " << status << endl;

    FD_SET(_sock, &read_set);

    if (int status { select(_sock + 1, &read_set, NULL, NULL, &_socket_timeout) }; (status < 0))
    { //ost << "ping status 2 = " << status << endl;
      throw socket_support_error(SOCKET_SUPPORT_SELECT_ERROR, "select() error; errno = "s + ::to_string(errno) + ": "s + strerror(errno));
    }
//    else
//      ost << "ping status 2 = " << status << endl;

    socklen_t slen { 0 };

// try changing this to have several attempts one second apart if we get an EWOULDBLOCK

    if (ssize_t status { recvfrom(_sock, data, sizeof(data), 0, NULL, &slen) }; (status < 0))
    { //ost << "ping status 3 = " << status << endl;
      const auto recvfrom_error { errno };

      if ( (recvfrom_error == EAGAIN) or (recvfrom_error == EWOULDBLOCK) )
        ost << "ICMP timeout" << endl;
      else
        throw socket_support_error(SOCKET_SUPPORT_RECVFROM_ERROR, "recvfrom() error; errno = "s + ::to_string(errno) + ": "s + strerror(errno));
    }
//    else
//      ost << "ping status 3 = " << status << endl;

    memcpy(&rcv_hdr, data, sizeof rcv_hdr);

    if (rcv_hdr.type == ICMP_ECHOREPLY)
    { ost << "ping reply from " << _destination_str << ", id = " << _icmp_hdr.un.echo.id << ", sequence = " << _icmp_hdr.un.echo.sequence << endl;
      rv = true;
    }
    else
      ost << "Got ICMP packet with type: " << static_cast<int>(rcv_hdr.type) << endl;
  }

  catch (const socket_support_error& e)
  { ost << "in ping; socket support error number " << e.code() << ", reason = " << e.reason() << endl;

    return false;
  }

  catch (const icmp_socket_error& e)
  { ost << "in ping; ICMP socket error number " << e.code() << ", reason = " << e.reason() << endl;

    return false;
  }

  return rv;
}

#if 0
not ready for use
// ------------------------------------  epoller  ----------------------------------

/*! \class  epoller
    \brief  Encapsulate and manage Linux epoll functions
*/

epoller::epoller(void)
{ const int status { epoll_create1(0) };

  if (status == -1)
    throw epoll_error(EPOLL_UNABLE_TO_CREATE, "Error creating epoll object");

  _fd = status;

  _ev.events = EPOLLIN;     // temporary: just show interest in pollin; see man epoll(7)
//  _ev.data.fd = _fd;
//
//  status = epoll_ctl(_fd, EPOLL_CTL_ADD, listen_sock, &ev
 //
 //              if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
 //              perror("epoll_ctl: listen_sock");
//               exit(EXIT_FAILURE);
//           }

//  }

}

void epoller::operator+=(const int file_descriptor)
{ _ev.data.fd = file_descriptor;

  const int status { epoll_ctl(_fd, EPOLL_CTL_ADD, file_descriptor, &_ev) };

  if (status == -1)
    throw epoll_error(EPOLL_UNABLE_TO_ADD_DESCRIPTOR, "Error adding file descriptor to epoll");
}
#endif

// ---------------------------------------------- generic socket functions ---------------------

/*! \brief                              Read a socket
    \param  in_socket                   inbound socket
    \param  timeout_in_tenths           timeout in tenths of a second
    \param  buffer_length_for_reply     maximum allowed length of reply
    \return                             response

    Throws socket_support_error(SOCKET_TIMEOUT) if the socket times out
*/
string read_socket(SOCKET& in_socket, const int timeout_in_tenths, const int buffer_length_for_reply)
{
// wait for response
  struct timeval timeout { /* sec */ timeout_in_tenths / 10, /* usec */ (timeout_in_tenths - timeout.tv_sec * 10) * 100'000L };

  fd_set ps_set;

  fd_set_value(ps_set, in_socket);

// check
  if (!FD_ISSET(in_socket, &ps_set))      // unable to set socket for listening
    throw socket_support_error(SOCKET_SUPPORT_UNABLE_TO_LISTEN);

  const int max_socket_number { in_socket + 1 };               // see p. 292 of Linux socket programming

  int socket_status { select(max_socket_number, &ps_set, NULL, NULL, &timeout) };  // under Linux, timeout has the remaining time, but this is not to be relied on because it's not generally true in other systems. See Linux select() man page

  switch (socket_status)
  { case 0:                    // timeout
      throw socket_support_error(SOCKET_SUPPORT_TIMEOUT, (string)"Socket timeout exceeded: "s + to_string(timeout_in_tenths) + (string)" tenths of seconds"s);

    case SOCKET_ERROR:
      throw socket_support_error(SOCKET_SUPPORT_SELECT_ERROR);

    default:                   // response is waiting to be read
    { //char socket_buffer[4096];
//      char socket_buffer[buffer_length_for_reply];
//      array<char, buffer_length_for_reply> socket_buffer;
      vector<char> socket_buffer(buffer_length_for_reply);

      socklen_t from_length { sizeof(sockaddr_storage) };

      sockaddr_storage ps_sockaddr;                    // unused but needed in recvfrom

//      socket_status = recvfrom(in_socket, socket_buffer, buffer_length_for_reply, 0, (sockaddr*)&ps_sockaddr, &from_length);
      socket_status = recvfrom(in_socket, &(socket_buffer[0]), buffer_length_for_reply, 0, (sockaddr*)&ps_sockaddr, &from_length);

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
      { const int octets_received { socket_status };

        return string(&(socket_buffer[0]), octets_received);
      }
    }
  }
}

/*! \brief        Set an fd_set to contain a particular single value of a file descriptor
    \param  fds   set of file descriptors
    \param  fd    the file descriptor to place into fds
*/
void fd_set_value(fd_set& fds, const int fd)
{ FD_ZERO(&fds);
  FD_SET(fd, &fds);
}

/*! \brief          Flush a readable socket
    \param  sock    socket to flush
*/
void flush_read_socket(SOCKET& sock)
{ char socket_buffer[1024];                           // read 1024 octets at a time

  struct timeval timeout { 0, 0 };

  fd_set ps_set;

  fd_set_value(ps_set, sock);

  const int max_socket_number { sock + 1 };               // see p. 292 of Linux socket programming

  socklen_t from_length { sizeof(sockaddr_storage) };

  sockaddr_storage ps_sockaddr;                                // unused but needed in recvfrom

  while (select(max_socket_number, &ps_set, NULL, NULL, &timeout) == 1)
  { recvfrom(sock, socket_buffer, 1024, 0, (sockaddr*)&ps_sockaddr, &from_length); 

// reset the timeout
    timeout = { 0, 0 };

// and the set
    fd_set_value(ps_set, sock);
  }
}

// ---------------------------  socket_address  ------------------------------

/*! \brief              Generate a sockaddr_storage from an address and port
    \param  ip_address  IP address in network order
    \param  port_nr     port number in host order
    \return             equivalent sockaddr_storage

    The returned sockaddr_storage is really a sockaddr_in, since this works only with IPv4
*/
sockaddr_storage socket_address(const uint32_t ip_address, const short port_nr)
{ sockaddr_storage rv;

  sockaddr_in* sinp { (sockaddr_in*)(&rv) };    // static_cast results in compiler error

  sinp->sin_family = AF_INET;
  sinp->sin_port = htons(port_nr);
  sinp->sin_addr.s_addr = ip_address;

  return rv;
}

/*! \brief      Convert a sockaddr_storage to a sockaddr_in
    \param  ss  value to be converted
    \return     <i>ss</i> as a sockaddr_in
  
    Throws socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL) if <i>ss</i> is not an IPv4 address
*/
sockaddr_in to_sockaddr_in(const sockaddr_storage& ss)
{ sockaddr_in rv;

  if (ss.ss_family != AF_INET)
    throw socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL, "Attempt to convert non-IPv4 sockaddr_storage to sockaddr_in"s);

  rv.sin_family = AF_INET;
  rv.sin_port = ((sockaddr_in*)(&ss))->sin_port;    // static_cast results in compiler error
  rv.sin_addr = ((sockaddr_in*)(&ss))->sin_addr;    // static_cast results in compiler error

  return rv;
}

/*! \brief              Convert a name to a dotted decimal IP address
    \param  fqdn        name to be resolved
    \param  n_tries     maximum number of tries
    \return             equivalent IP address in dotted decimal format

    Throws exception if the name cannot be resolved. Uses gethostbyname_r() to perform the lookup.
    <i>n_tries</i> is present because gethostbyname_r() cannot be relied on to complete a remote
    lookup before deciding to return with an error.

    Cannot use string_view because gethostbyname_r() requires a null-terminated C-string
*/
string name_to_dotted_decimal(const string& fqdn, const unsigned int n_tries)
{ constexpr std::chrono::duration RETRY_TIME { 1s };       // period between retries

  if (fqdn.empty())                  // gethostbyname (at least on Windows) is hosed if one calls it with null string
    throw socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL, "Asked to look up empty name"s);

  constexpr int BUF_LEN { 2048 };
  
  const size_t buflen { BUF_LEN };

  struct hostent ret;
  char           buf[BUF_LEN];
  int            h_errnop;

  struct hostent* result { nullptr };

  remove_const_t<decltype(n_tries)> n_try   { 0 };
  bool                              success { false };

  while (n_try++ < n_tries and !success)
  { const int status { gethostbyname_r(fqdn.c_str(), &ret, &buf[0], buflen, &result, &h_errnop) };

    success = ( (status == 0) and (result != nullptr) );    // the second test should be redundant

    if (!success and n_try != n_tries)
      sleep_for(RETRY_TIME);
  }

  if (success)
  { char** h_addr_list { result->h_addr_list };

    const uint32_t addr_long { htonl(*(uint32_t*)(*h_addr_list)) };    // host order

    return convert_to_dotted_decimal(addr_long);
  }
  else
  { ost << "Error trying to resolve name: " << fqdn << endl;

    throw(tcp_socket_error(TCP_SOCKET_UNABLE_TO_RESOLVE, (string)"Unable to resolve name: "s + fqdn));
  }
}

/*! \brief      Extract address from a sockaddr_storage
    \param  ss  sockaddr_storage
    \return     dotted decimal string
*/
string dotted_decimal_address(const sockaddr_storage& ss)
{ try
  { return dotted_decimal_address(to_sockaddr_in(ss));
  }

  catch (...)
  { ost << "Caught exception when obtaining dotted decimal address" << endl;
    return "0.0.0.0"s;
  }
}
