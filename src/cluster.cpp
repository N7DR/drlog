// $Id: cluster.cpp 61 2014-05-03 16:34:34Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file cluster.cpp

        Classes and functions related to a DX cluster
*/

#include "cluster.h"
#include "pthread_support.h"
#include "string_functions.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include <netinet/tcp.h>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

extern message_stream ost;
extern pt_mutex thread_check_mutex;

extern bool exiting;

pt_mutex buffer_mutex;
pt_mutex rbn_buffer_mutex;

// -----------  dx_cluster  ----------------

dx_cluster::dx_cluster(const drlog_context& context, const POSTING_SOURCE src) :
  _server(src == POSTING_CLUSTER ? context.cluster_server() : context.rbn_server()),
  _port(src == POSTING_CLUSTER ? context.cluster_port() : context.rbn_port()),
  _my_ip(context.my_ip()),
  _login_id(src == POSTING_CLUSTER ? context.cluster_username() : context.rbn_username()),
  _source(src),
  _timeout(2),
  _connection(src == POSTING_CLUSTER ? context.cluster_server() : context.rbn_server(),
              src == POSTING_CLUSTER ? context.cluster_port() :  context.rbn_port(),
              context.my_ip())
{ ost << "inside body of dx_cluster constructor" << endl;

// set the keepalive option
  _connection.keep_alive(300, 60, 2);
  
  string buf;

// send the login information as soon as we receive anything from the server
  while (buf.empty())
  { try
    { ost << "about to read cluster connection" << endl;

      buf = _connection.read(_timeout);

      { SAFELOCK(rbn_buffer);
        _unprocessed_input += buf;
      }
    }

    catch (const socket_support_error& e)
    { if (e.code() == SOCKET_SUPPORT_TIMEOUT)
        ost << "Timeout in dx_cluster constructor" << endl;
      else
      { ost << "socket support error while reading cluster connection; error # = " << e.code() << "; reason = " << e.reason() << endl;
        throw;
      }
    }

    catch (...)
    { ost << "caught unknown exception in dx_cluster constructor" << endl;
      throw;
    }
  }
  
  ost << "about to send to cluster connection" << endl;

  _connection.send(_login_id + CRLF);

// if it's a cluster, show the last 100 postings in order to populate bandmap
  if (_source == POSTING_CLUSTER)
    _connection.send("sh/dx/100" + CRLF);
}

dx_cluster::~dx_cluster(void)
{ _connection.send("BYE" + CRLF);
  
  { SAFELOCK(rbn_buffer);
    _unprocessed_input = _connection.read(_timeout / 2);  // add a delay before we tear down the connection
  }
}

void dx_cluster::_process_error(void)
{
new_socket:
  try
  { try
    { _connection.close();                     // releases the file descriptor
      ost << "Closed OK" << endl;
    }

    catch (...)
    { ost << "Error closing socket" << endl;
    }

    _connection.new_socket();                  // get a new socket
    ost << "Set connection to use new socket OK" << endl;

    _connection.keep_alive(300, 60, 2);        // set the keepalive option
    ost << "Keepalive parameters set" << endl;

    _connection.bind(_my_ip);                  // bind it to the correct IP address
    ost << "Bind OK" << endl;

// reconnect to the server
reconnect:
    try
    { ost << "Attempting to reconnect to server: " << _server << " on port " << _port << endl;

// resolve the name
      const string dotted_decimal = name_to_dotted_decimal(_server);

      _connection.destination(dotted_decimal, _port);
      ost << "Reconnected; now need to log in" << endl;

      _connection.send(_login_id + CRLF);
      ost << "login ID sent" << endl;
    }

    catch (const socket_support_error& E)
    { ost << "socket support error " << E.code() << " while setting destination: " << E.reason() << endl;
      sleep_for(minutes(1));    // sleep for one minute before retrying

      { SAFELOCK(thread_check);

        if (exiting)
        { //n_running_threads--;
          return;
        }
      }

      goto reconnect;
    }
  }

  catch (const tcp_socket_error& E)    // complete failure; go back and try again
  { ost << "TCP socket error " << E.code() << " in close/new socket/bind recovery sequence: " << E.reason() << endl;
    goto new_socket;
  }
}

void dx_cluster::reset(void)
{ ost << "Attempting to reset connection" << endl;
  _process_error();
  ost << "Completed reset of connection" << endl;
}

const string dx_cluster::read(void)
{ //try
  //{
//    ost << "About to read the connection" << endl;
  string buf;
    
  try
  { buf = _connection.read(_timeout);
//      ost << "Returned from reading the connection; number of read bytes = " << buf.length() << endl;
  }
    
  catch (const socket_support_error& e)
  { ost << "caught socket_support_error in read()" << endl;

    if (e.code() != SOCKET_SUPPORT_TIMEOUT)    // an error that is not a timeout has to be handled
    {
// error reading; try to reconnect
      ost << "Error reading from socket; attempting reconnection" << endl;

      _process_error();
    }
    else
      ost << "tcp_socket_error timeout in read: code = " << e.code() << "; reason = " << e.reason() << endl;
  }

  catch (const tcp_socket_error&)
  { _process_error();
  }
  
  if (!buf.empty())
  { //ost << "Data from server: " << buf << endl;

    SAFELOCK(rbn_buffer);
    _unprocessed_input += buf;
            
//    ost << "_unprocessed_input is now: " << _unprocessed_input << endl;
  }
  
  return _unprocessed_input;
}

void dx_cluster::send(const string& msg)
{ _connection.send(msg);
}

const string dx_cluster::get_unprocessed_input(void)
{ string rv;

  { SAFELOCK(rbn_buffer);
    rv = move(_unprocessed_input);
    _unprocessed_input.clear();
  }
  
  return rv;
}

// -----------  dx_post  ----------------

dx_post::dx_post(const std::string& received_info, location_database& db, const enum POSTING_SOURCE post_source) :
  _source(post_source),
  _valid(false)
{
// there seem to be two formats for postings: the sh/dx format and the ordinary post format; deal with the sh/dx first
// first non-space sharacter is a digit and last character is ">"
// parsing failures are trapped
  if (last_char(received_info) == '>')
  { const string copy = remove_leading_spaces(received_info);
  
// 18073.1  P49V        29-Dec-2009 1931Z  nice signal NW            <N7XR> 
    if (!copy.empty() and isdigit(copy[0]))
    { try
      { size_t char_posn = copy.find(" ");
        size_t space_posn;

        if (char_posn != string::npos)
        { _frequency_str = copy.substr(0, char_posn);
          _freq = frequency(_frequency_str);

          if (_valid_frequency())
          { char_posn = copy.find_first_not_of(" ", char_posn);
            space_posn = copy.find_first_of(" ", char_posn);

            _callsign = copy.substr(char_posn, space_posn - char_posn);

            const location_info li = db.info(_callsign);
            _canonical_prefix = li.canonical_prefix();
            _continent = li.continent();

            char_posn = copy.find_first_not_of(" ", space_posn);
            space_posn = copy.find_first_of(" ", char_posn);

            const string date = copy.substr(char_posn, space_posn - char_posn);     // we don't use this

            char_posn = copy.find_first_not_of(" ", space_posn);
            space_posn = copy.find_first_of(" ", char_posn);
            char_posn = copy.find_first_not_of(" ", space_posn);

            size_t bra_posn = copy.find_last_of("<");

            if (bra_posn != string::npos)
            { _comment = copy.substr(char_posn, bra_posn - char_posn);
              _poster = copy.substr(bra_posn + 1, copy.length() - (bra_posn + 1) - 1);

/*
              ost << "frequency: " << _frequency_str << endl
                   << "DX call: "   << _callsign << endl
                   << "Date: " << date << endl
                   << "Comment: " << _comment << endl
                   << "Poster: " << _poster << endl;
*/

              _valid = true;
              _time_processed = ::time(NULL);
            }
          }
        }
      }

      catch (...)
      { ost << "Error parsing cluster post (format 1): " << received_info << endl;
      }
    }
  }

// RBN posting
// DX de DK9IP-#:      7006.0  LA3MHA       CQ    21 dB 24 WPM             2257Z JN48

// ordinary posting
// DX de RX9WN:      3512.0  UE9WFJ       see UE9WURC  QRZ.COM           1932Z

// we treat everything after the call as a comment
  if (!_valid)
  { if (substring(received_info, 0, 6) == "DX de " and received_info.length() > 70)
    { try
      { const string copy = remove_leading_spaces(substring(received_info, 6));
        const size_t colon_posn = copy.find(":");

        if (colon_posn != string::npos)
        { _poster = copy.substr(0, colon_posn);

          size_t char_posn = copy.find_first_not_of(" ", colon_posn + 1);
          size_t space_posn = copy.find_first_of(" ", char_posn);

          _frequency_str = copy.substr(char_posn, space_posn - char_posn);
          _freq = frequency(_frequency_str);

          if (_valid_frequency())
          { char_posn = copy.find_first_not_of(" ", space_posn);
            space_posn = copy.find_first_of(" ", char_posn);

            _callsign = copy.substr(char_posn, space_posn - char_posn);

            const location_info li = db.info(_callsign);
            _canonical_prefix = li.canonical_prefix();
            _continent = li.continent();

            char_posn = copy.find_first_not_of(" ", space_posn);
            space_posn = copy.find_last_of(" ");

            _comment = copy.substr(char_posn);

/*
            ost << "DXPost format 2: " << endl
                  << "frequency: " << _frequency_str << endl
                   << "DX call: "   << _callsign << endl
 //                  << "Time: " << _time_claimed << endl
                   << "Comment: " << _comment << endl
                   << "Poster: " << _poster << endl;
*/
            
            _valid = true;
            _time_processed = ::time(NULL);
          }
        }        
      }

      catch (...)
      { ost << "Error parsing cluster post (format 2): " << received_info << endl;
      }
    }
  }
  
// just in case we've somehow got here and don't have a call, but think we're valid
  if (_callsign.empty())
    _valid = false;

// add the band
  if (_valid)
    _band = static_cast<BAND>(_freq);
}
