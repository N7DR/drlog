// $Id: cluster.cpp 129 2016-09-29 21:13:34Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file cluster.cpp

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

extern message_stream ost;              ///< for debugging and logging
extern pt_mutex thread_check_mutex;     ///< mutex for controlling threads

extern bool exiting;                    ///< is the program exiting?

pt_mutex buffer_mutex;                  ///< mutex for the cluster buffer
pt_mutex monitored_posts_mutex;         ///< mutex for the monitored posts
pt_mutex rbn_buffer_mutex;              ///< mutex for the RBN buffer

// -----------  dx_cluster  ----------------

/*! \brief              Constructor
    \param  context     the drlog context
    \param  src         whether this is a real DX cluster or the RBN
*/
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
{
// set the keepalive option
  _connection.keep_alive(300, 60, 2);
  
  string buf;

// send the login information as soon as we receive anything from the server
  while (buf.empty())
  { try
    { buf = _connection.read(_timeout);

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

  _connection.send(_login_id + CRLF);

// if it's a cluster, show the last 100 postings in order to populate bandmap
  if (_source == POSTING_CLUSTER)
    _connection.send("sh/dx/100" + CRLF);
}

/// destructor
dx_cluster::~dx_cluster(void)
{ _connection.send("BYE" + CRLF);
  
  { SAFELOCK(rbn_buffer);
    _unprocessed_input = _connection.read(_timeout / 2);  // add a delay before we tear down the connection
  }
}

/// process a read error
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
    _connection.keep_alive(300, 60, 2);        // set the keepalive option
    _connection.bind(_my_ip);                  // bind it to the correct IP address

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
          return;
      }

      goto reconnect;
    }
  }

  catch (const tcp_socket_error& E)    // complete failure; go back and try again
  { ost << "TCP socket error " << E.code() << " in close/new socket/bind recovery sequence: " << E.reason() << endl;
    goto new_socket;
  }
}

/// reset the cluster socket
void dx_cluster::reset(void)
{ ost << "Attempting to reset connection" << endl;
  _process_error();
  ost << "Completed reset of connection" << endl;
}

/*! \brief      Read from the cluster socket
    \return     the current bytes waiting on the cluster socket
*/
const string dx_cluster::read(void)
{ string buf;
    
  try
  { buf = _connection.read(_timeout);
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
  { SAFELOCK(rbn_buffer);

    _unprocessed_input += buf;
  }
  
  return _unprocessed_input;
}

/*! \brief      Read from the cluster socket
    \return     the information that has been read from the socket but has not yet been processed
*/
const string dx_cluster::get_unprocessed_input(void)
{ string rv;

  { SAFELOCK(rbn_buffer);
    rv = move(_unprocessed_input);
    _unprocessed_input.clear();
  }
  
  return rv;
}

// -----------  dx_post  ----------------

/*! \class  dx_post
    \brief  Convert a line from the cluster to a DX posting
*/

/*! \brief                  Constructor
    \param  received_info   a line received from the cluster or RBN
    \param  db              the location database for this contest
    \param  post_source     the origin of the post
*/
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

            const size_t bra_posn = copy.find_last_of("<");

            if (bra_posn != string::npos)
            { _comment = copy.substr(char_posn, bra_posn - char_posn);
              _poster = copy.substr(bra_posn + 1, copy.length() - (bra_posn + 1) - 1);
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

// sanity check for the callsign
  if (_valid)
    _valid = (_callsign.find_first_not_of("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ/") == string::npos);

// add the band
  if (_valid)
    _band = static_cast<BAND>(_freq);
}

// -----------  monitored_posts_entry  ----------------

/*! \class  monitored_posts_entry
    \brief  An entry in the container of monitored posts
*/

monitored_posts_entry::monitored_posts_entry(const dx_post& post)
{ _callsign = post.callsign();
//  _freq = post.freq();
  _frequency_str = post.frequency_str();
  _expiration = post.time_processed() + 3600;  // valid for one hour
  _band = post.band();
}

const string monitored_posts_entry::to_string(void) const
{ string rv;

  rv = pad_string(_frequency_str, 7, PAD_LEFT) + " " + _callsign;

  return rv;
}

/// ostream << monitored_posts_entry
ostream& operator<<(ostream& ost, const monitored_posts_entry& mpe)
{ ost << "Monitored: " << mpe.callsign()
      << ", absolute expiration = " << mpe.expiration()
      << ", relative expiration = " << mpe.expiration() - ::time(NULL)
      << ", frequency = " << mpe.frequency_str();

  return ost;
}

// -----------  monitored_posts  ----------------

/*! \class  monitored_posts
    \brief  Handle the monitoring of certain stations
*/

monitored_posts::monitored_posts(void) :
  _is_dirty(false)
{ }

void monitored_posts::callsigns(const set<string>& calls_to_be_monitored)
{ SAFELOCK(monitored_posts);

  _callsigns = calls_to_be_monitored;
}

const bool monitored_posts::is_monitored(const std::string& callsign) const
{ SAFELOCK(monitored_posts);

  return (_callsigns < callsign);
}

void monitored_posts::max_entries(const unsigned int n)
{ SAFELOCK(monitored_posts);

  _max_entries = n;
}

//void monitored_posts::add(const string& call, const enum BAND b, const time_t& tm)
//{ SAFELOCK(monitored_posts);
//
//}

const deque<monitored_posts_entry> monitored_posts::entries(void)
{ SAFELOCK(monitored_posts);

  return _entries;
}

void monitored_posts::operator+=(const dx_post& post)
{ monitored_posts_entry mpe(post);
  bool stop_search = false;
  bool found_call_and_band = false;

  SAFELOCK(monitored_posts);

  for (deque<monitored_posts_entry>::iterator it = _entries.begin(); (!stop_search and (it != _entries.end())); ++it)
  { monitored_posts_entry& old_mpe = *it;

    if ( (mpe.callsign() == old_mpe.callsign()) and (mpe.band() == old_mpe.band()) )
    { if ( mpe.expiration() > old_mpe.expiration() )
      { _entries.erase(it);
        stop_search = true;
      }
      else
      { found_call_and_band = true;  // found entry, but it expires after the post we're testing
        stop_search = true;
      }
    }
  }

  if (stop_search and !found_call_and_band)
  { _entries.push_back(mpe);
    _is_dirty = true;
  }

  if (!stop_search)
  { _entries.push_back(mpe);
    _is_dirty = true;
  }

  while (_entries.size() > _max_entries)  // should happen only once at most
  { _entries.pop_front();
    _is_dirty = true;                     // should be unnecessary, since should already be true
  }
}

void monitored_posts::operator+=(const string& new_call)
{ SAFELOCK(monitored_posts);

  _callsigns.insert(new_call);
}

void monitored_posts::operator-=(const string& call_to_remove)
{ SAFELOCK(monitored_posts);

  _callsigns.erase(call_to_remove);

// remove any entries that have this call

  const size_t original_size = _entries.size();

  _entries.erase(remove_if(_entries.begin(), _entries.end(),
                   [=] (monitored_posts_entry& mpe) { return (mpe.callsign() == call_to_remove); } ),
                 _entries.end() );

  if (original_size != _entries.size())
    _is_dirty = true;
}

void monitored_posts::prune(void)
{ const time_t now = ::time(NULL);

  SAFELOCK(monitored_posts);

  const size_t original_size = _entries.size();

  _entries.erase(remove_if(_entries.begin(), _entries.end(),
                   [=] (monitored_posts_entry& mpe) { return (mpe.expiration() < now); } ),
                 _entries.end() );

  if (original_size != _entries.size())
    _is_dirty = true;
}

const vector<string> monitored_posts::to_strings(void)
{ vector<string> rv;

  SAFELOCK(monitored_posts);

  for (const auto& mpe: _entries)
    rv.push_back(mpe.to_string());

  return rv;
}
