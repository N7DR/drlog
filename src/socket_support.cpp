// $Id: socket_support.cpp 61 2014-05-03 16:34:34Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file socket_support.h

    Objects and functions related to sockets. This code is based on
    a much larger codebase from IPfonix, Inc. for socket-related functions.
*/

#include "pthread_support.h"
#include "socket_support.h"
#include "string_functions.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

extern ofstream ost;

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
    \param  sock_p  Pointer to socket
  
    Acts as default constructor if passed pointer is NULL
*/
tcp_socket::tcp_socket(SOCKET* sp) :
  _destination_is_set(false),
  _preexisting_socket(true),
  _force_closure(false),
  _timeout_in_tenths(600)                     // 1 minute
{ if (sp)
    _sock = *sp;
  else
  { try
    { _sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// enable re-use
      static const int on = 1;
      int status = setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on) );    // char* cast is needed for Windows

      if (status)
        throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting SO_REUSEADDR");

      const struct linger lgr = { 1, 0 };

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

/*! \brief          Encapsulate a pre-existing socket
    \param  sp      socket
*/
tcp_socket::tcp_socket(SOCKET sp) :
  _destination_is_set(false),
  _preexisting_socket(true),
  _sock(sp),
  _force_closure(false),
  _timeout_in_tenths(600)                     // 1 minute
{ 
}

#include <chrono>
#include <thread>
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread
void alert(const string& msg);

/*! \brief  Construct and initialise with useful values
  \param  destination_ip_address  Destination dotted decimal address or FQDN
  \param  destination_port  Destination port
  \param  source_ip_address Address to which the socket is to be bound
*/
tcp_socket::tcp_socket(const string& destination_ip_address_or_fqdn, 
                       const unsigned int destination_port, 
                       const string& source_address) :
  _sock(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
  _destination_is_set(false),
  _preexisting_socket(false),
  _force_closure(false),
  _timeout_in_tenths(600)                    // 1 minute
{ 
  try
  { 
    ost << "Inside tcp_socket() constructor" << endl;

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
    { const int TIMEOUT = 10;
      bool connected = false;
      int n_timeouts = 0;

      while (!connected)
      { ost << "about to try to connect to " << destination_ip_address_or_fqdn << ":" << destination_port << endl;

        try
        { if (is_legal_ipv4_address(destination_ip_address_or_fqdn))
            destination(destination_ip_address_or_fqdn, destination_port, TIMEOUT);
          else                                                                // FQDN was passed instead of dotted decimal
          {
// resolve the name
            const string dotted_decimal = name_to_dotted_decimal(destination_ip_address_or_fqdn);
            
            ost << "name " << destination_ip_address_or_fqdn << " resolved to: " << dotted_decimal << endl;

            destination(dotted_decimal, destination_port, TIMEOUT );
          }

          connected = true;
        }

        catch (const socket_support_error& e)
        { ost << "caught socket_support_error exception" << endl;
          ost << "Socket support error number " << e.code() << "; " << e.reason() << endl;
//          n_timeouts++;
          if (n_timeouts++ % 10 == 0)
            alert("Error setting socket destination: " + destination_ip_address_or_fqdn +":" + to_string(destination_port));
          ost << "sleeping for 10 seconds" << endl;
          sleep_for(seconds(10));
        }
      }

      ost << "tcp_socket connected, constructed OK" << endl;

    }
  }

  catch (const socket_support_error& e)
  { if (e.code() == SOCKET_SUPPORT_CONNECT_ERROR)
    { ost << e.reason() << endl;
    }
    else
    { _close_the_socket();
      throw;
    }
  }

  catch (...)                       // to ensure that socket is properly closed
  { _close_the_socket();
    throw;
  }
}

// destructor
tcp_socket::~tcp_socket(void)
{ if (!_preexisting_socket)
    _close_the_socket();        // we have to close the socket if we are finished with it

  if (_preexisting_socket && _force_closure)
    _close_the_socket();
}

/*!     \brief  New socket
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

// bind the socket
void tcp_socket::bind(const sockaddr_storage& local_address)
{ SAFELOCK(_tcp_socket);
  const int status = ::bind(_sock, (sockaddr*)&local_address, sizeof(local_address));

  if (status)
    throw socket_support_error(SOCKET_SUPPORT_BIND_ERROR, "Errno = " + to_string(errno) + "; " + strerror(errno));

  _bound_address = local_address;
}

// bind the socket
void tcp_socket::bind(const string& dotted_decimal_address, const short port_nr)
{ bind(socket_address(dotted_decimal_address, port_nr)); 
}

// connect to the far-end
void tcp_socket::destination(const sockaddr_storage& adr)
{ SAFELOCK(_tcp_socket);

  ost << "inside destination() without timeout; about to connect" << endl;
  const int status = ::connect(_sock, (sockaddr*)(&adr), sizeof(adr));
  ost << "connect returned " << status << endl;

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



/*! \brief  Connect to the far-end, with explicit time-out when trying to make connection
    \param  adr Address/port of the far end
    \param  timeout timeout in seconds
*/
// https://www.linuxquestions.org/questions/programming-9/connect-timeout-change-145433/
//#if 0
void tcp_socket::destination(const sockaddr_storage& adr, const unsigned long timeout_secs)
{


  struct timeval timeout;
  timeout.tv_sec = timeout_secs;
  timeout.tv_usec = 0L;

  SAFELOCK(_tcp_socket);

  fd_set r_set, w_set;
  int flags;

  FD_ZERO(&r_set);
  FD_SET(_sock, &r_set);
  w_set = r_set;

//set socket nonblocking flag
  flags = fcntl(_sock, F_GETFL, 0);
  fcntl(_sock, F_SETFL, flags | O_NONBLOCK);

//  ost << "about to connect" << endl;
  ost << "inside destination() with timeout; about to connect" << endl;
  int status = ::connect(_sock, (sockaddr*)(&adr), sizeof(adr));
  ost << "connect returned " << status << endl;

  if (status == 0)        // all OK
  { _destination = adr;
    _destination_is_set = true;
  }
  else
  { _destination_is_set = false;
    ost << "errno = " << errno << endl;
    ost << "error string = " << strerror(errno) << endl;

    const string address = dotted_decimal_address(*(sockaddr*)(&adr));
          const unsigned int p = port(*(sockaddr*)(&adr));

    if (errno != EINPROGRESS)
    { //const string address = dotted_decimal_address(*(sockaddr*)(&adr));
      //const unsigned int p = port(*(sockaddr*)(&adr));

      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, "Status " + to_string(errno) + " received from ::connect; " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));
    }

    status = select(_sock + 1, &r_set, &w_set, NULL, (timeout_secs) ? &timeout : NULL);

    ost << "status from select in destination() = " << status << endl;

    if (status < 0)
    { ost << "about to throw exception" << endl;
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, "EINPROGRESS: " + to_string(errno) + " received from ::connect; " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));
    }

    if (status == 0)  // timed out
    { errno = ETIMEDOUT;
      throw socket_support_error(SOCKET_SUPPORT_CONNECT_ERROR, (string)"Timeout received from ::connect: " + strerror(errno) + " while trying to connect to address " + address + "; port " + to_string(p));
    }

// select is positive, which means that the file descriptor is OK
    _destination_is_set = true;
  }
}
//#endif

#if 0
if( (ret = connect(sock, (struct sockaddr *)&sa, 16)) < 0 )
    if (errno != EINPROGRESS)
        return -1;

if(ret == 0)    //then connect succeeded right away
    goto done;

//we are waiting for connect to complete now
if( (ret = select(sock + 1, &rset, &wset, NULL, (timeout) ? &ts : NULL)) < 0)
    return -1;
if(ret == 0){   //we had a timeout
    errno = ETIMEDOUT;
    return -1;
}
#endif



/*! \brief  Mark as connected to the far-end
    \param  adr Address/port of the far end

    Used for server sockets returned by .accept()
*/
void tcp_socket::connected(const sockaddr_storage& adr)
{ SAFELOCK(_tcp_socket);

  _destination = adr;
  _destination_is_set = true;
}

/*! \brief  Simple send
  \param  msg Message to send
  
  This is not very useful, since it doesn't look for a response
*/
void tcp_socket::send(const std::string& msg)
{ ost << "inside tcp_socket::send()" << endl;

  if (!_destination_is_set)
  { ost << "Error: destination is not set" << endl;
    throw tcp_socket_error(TCP_SOCKET_UNKNOWN_DESTINATION);
  }

  SAFELOCK(_tcp_socket);

  const int status = ::send(_sock, msg.c_str(), msg.length(), 0);

  ost << "send status = " << status << endl;
  
  if (status == -1)
    throw tcp_socket_error(TCP_SOCKET_ERROR_IN_WRITE);   
}

/*! \brief  Simple receive
  \return Received string
*/
#if 1
const string tcp_socket::read(void)
{ //pointer<char> pc(4096);                   // a reasonable sized buffer
  //char* cp = pc.ptr();
  char cp[4096];

  string rv;
  int status;
  
  SAFELOCK(_tcp_socket);

  do
  { ost << "calling ::recv in tcp_socket::read" << endl;
    status  = ::recv(_sock, cp, 4096, 0);

    ost << "tcp_socket::read status: " << status << endl;

    if (status == -1)
      throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);

    if (status == 0)
      throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);    
    
    rv += string(cp, status);
  } while (status == 4096);

  return rv;
}
#endif

/*! \brief                Simple receive
    \param  timeout_secs  Timeout in seconds
    \return               Received string

    Throws an exception if the read times out
*/
const string tcp_socket::read(const unsigned long timeout_secs)
{ string rv;

  struct timeval timeout;
  timeout.tv_sec = timeout_secs;
  timeout.tv_usec = 0L;

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
    { char cp[4096];    // a reasonable sized buffer
      int status;
  
      do
      { status  = ::recv(_sock, cp, 4096, 0);

        if (status == -1)
        { ost << "Throwing TCP_SOCKET_ERROR_IN_RECV" << endl;
          throw tcp_socket_error(TCP_SOCKET_ERROR_IN_RECV);
        }

        rv += string(cp, status);
      }     while (status == 4096);

      break;
    }
  }

  return rv;
}

// set the time before a keep-alive is sent
void tcp_socket::idle_time(const unsigned int seconds)
{ int optval = seconds;
  int optlen = sizeof(optval);
  
   SAFELOCK(_tcp_socket);

   const int status = setsockopt(socket(), IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting idle time");
}

// set the time between keep-alives
void tcp_socket::retry_time(const unsigned int seconds)
{ int optval = seconds;
  int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting retry time");
}

// set the maximum number of retries
void tcp_socket::max_retries(const unsigned int n)
{ int optval = n;
  int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error setting maximum number of retries");
}

// control keep-alive
void tcp_socket::keep_alive(const bool torf)
{ int optval = torf ? 1 : 0;
  int optlen = sizeof(optval);
  
  SAFELOCK(_tcp_socket);

  const int status = setsockopt(socket(), SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);

  if (status)
    throw tcp_socket_error(TCP_SOCKET_UNABLE_TO_SET_OPTION, "Error to control keep-alive");
}

void tcp_socket::keep_alive(const unsigned int idle, const unsigned int retry, const unsigned int n)
{ keep_alive();           // turn on keep-alive
  idle_time(idle);
  retry_time(retry);
  max_retries(n);
}

// ---------------------------------------------- generic socket functions ---------------------

// ----------------------------------  socket_error_name  -------------------------------

// get the name of a socket error
string socket_error_name(const int error_number)
{ return strerror(error_number);  
}

// read a message from a socket
string read_socket(SOCKET& in_socket, const int timeout_in_tenths, const int buffer_length_for_reply)
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
    { //log_message((string)"Socket timeout exceeded: " + to_string(timeout_in_tenths) + (string)" tenths of seconds", MSG_WARNING);
      throw socket_support_error(SOCKET_SUPPORT_TIMEOUT, (string)"Socket timeout exceeded: " + to_string(timeout_in_tenths) + (string)" tenths of seconds");
    }

    case SOCKET_ERROR:
    { //log_message((string)"Socket error", MSG_WARNING);
      throw socket_support_error(SOCKET_SUPPORT_SELECT_ERROR);
    }

    default:                   // response is waiting to be read
    { //pointer<char> socket_buffer_p(buffer_length_for_reply);
      //char* socket_buffer = socket_buffer_p.ptr();
      char socket_buffer[4096];

//      SOCKLEN_T from_length = sizeof(sockaddr_in);
      socklen_t from_length = sizeof(sockaddr_storage);
      sockaddr_storage ps_sockaddr;                    // unused but needed in recvfrom

      socket_status = recvfrom(in_socket, socket_buffer, buffer_length_for_reply, 0, (sockaddr*)&ps_sockaddr, &from_length); 
      if (socket_status == SOCKET_ERROR)
      { socket_status = errno;
        switch (socket_status)
        { 

          case EFAULT:
            //log_message((string)"EFAULT error", MSG_WARNING);
            throw socket_support_error(SOCKET_SUPPORT_EFAULT);
//              log_message((string)"Pointer error receiving message from provisioning server");
            break;

          default:
            //log_message((string)"RECVFROM Error", MSG_WARNING);
            throw socket_support_error(SOCKET_SUPPORT_RECVFROM_ERROR);
            break;
        }
      }
      else                                   // have read message from other end OK
      { const int octets_received = socket_status;
        return string(&(socket_buffer[0]), octets_received);
      }
    }
  }
}

// ---------------------------  flush_read_socket  ------------------------------

// flush a readable socket
void flush_read_socket(SOCKET& sock)
{ char socket_buffer[1024];                           // read 1024 octets at a time
  struct timeval timeout;
    
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  fd_set ps_set;

  FD_ZERO(&ps_set);
  FD_SET(sock, &ps_set);
  const int max_socket_number= sock + 1;               // see p. 292 of Linux socket programming

  socklen_t from_length = sizeof(sockaddr_storage);
  sockaddr_storage ps_sockaddr;                                // unused but needed in recvfrom

  while (select(max_socket_number, &ps_set, NULL, NULL, &timeout) == 1)
  { recvfrom(sock, socket_buffer, 1024, 0, (sockaddr*)&ps_sockaddr, &from_length); 

// reset the timeout
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

// and the set

    FD_ZERO(&ps_set);
    FD_SET(sock, &ps_set);


  }
}

// ---------------------------  socket_address  ------------------------------

// ip_address_as_long is in network order
// port_nr is in host order
// this works only in IPv4
sockaddr_storage socket_address(const unsigned long ip_address_as_long, const short port_nr)
{ 
  sockaddr_storage rv;
  sockaddr_in* sinp = (sockaddr_in*)(&rv);
  
  sinp->sin_family = AF_INET;
  sinp->sin_port = htons(port_nr);
  sinp->sin_addr.s_addr = ip_address_as_long;

  return rv;
}

// ---------------------------  process_socket_error  ------------------------------

// this encapsulates OS differences in a single place
void process_socket_error(const int original_socket_status)
{ if (original_socket_status == 0)                           // no error
    return;

  int socket_status = original_socket_status;

  if (socket_status == SOCKET_ERROR)
  { socket_status = errno;
    switch (socket_status)
    { case EAGAIN:                     // should only be used when in non-blocking mode
      //  log_message((string)"Error EAGAIN; This should never happen");
        break;
      default:
      //  log_message((string)"Socket error reading peer port");
        break;
    }
  }

}

/*! \brief  Convert a sockaddr_storage to a sockaddr_in
  \param  ss  Value to be converted
        \return <i>ss</i> as a sockaddr_in
  
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

/*!     \brief  Convert a name to a dotted decimal IP address
        \param  fqdn                    Name to be resolved
        \return Equivalent IP address in dotted decimal format

        Throws exception if the name cannot be resolved. Uses gethostbyname() to perform the lookup.
*/   
string name_to_dotted_decimal(const string& fqdn)
{ if (fqdn.empty())                  // gethostbyname (at least on Windows) is hosed if one calls it with null string
    throw socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL, "Asked to lookup empty name");

//  log_message((string)"Performing DNS lookup on name: " + fqdn, MSG_DEBUG);

  void* vp;
  
  { // SAFELOCK(gethostbyname);                    // reinstate if we need a thread-safe lookup

#if 0
    vp = gethostbyname(fqdn.c_str());

    if (vp == NULL)
      throw socket_support_error(SOCKET_SUPPORT_WRONG_PROTOCOL, "NULL returned by gethostbyname() looking for: " + (string)(fqdn.c_str()));

    hostent* hp = (hostent*)vp;
    char** h_addr_list = hp->h_addr_list;
    const uint32_t kdc_addr_long = htonl(*(uint32_t*)(*h_addr_list));    // host order

    return convert_to_dotted_decimal(kdc_addr_long);
#endif

//    struct hostent *ret, char *buf, size_t buflen,
//    struct hostent **result, int *h_errnop);

    const int BUF_LEN = 2048;

    struct hostent ret;
    char buf[BUF_LEN];
    size_t buflen(BUF_LEN);
    struct hostent* result;
    int h_errnop;

    const int status = gethostbyname_r(fqdn.c_str(), &ret, &buf[0], buflen, &result, &h_errnop);

    ost << "name_to_dotted_decimal status = " << status << endl;

    if ( (status == 0) and (result != nullptr) )    // success; the second test should be redundant
    { char** h_addr_list = result->h_addr_list;
      const uint32_t addr_long = htonl(*(uint32_t*)(*h_addr_list));    // host order

      return convert_to_dotted_decimal(addr_long);
    }
    else
    { ost << "Error return";

      throw(tcp_socket_error(TCP_SOCKET_UNABLE_TO_RESOLVE, (string)"Unable to resolve name: " + fqdn));
    }

  } 
}

