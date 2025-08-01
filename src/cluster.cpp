// $Id: cluster.cpp 269 2025-05-19 22:42:59Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   cluster.cpp

    Classes and functions related to a DX cluster and the Reverse Beacon Network
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
using namespace   chrono;           // std::chrono
using namespace   chrono_literals;  // std::chrono_literals
using namespace   this_thread;      // std::this_thread

extern bool           exiting;              ///< is the program exiting?
extern message_stream ost;                  ///< for debugging and logging
extern pt_mutex       thread_check_mutex;   ///< mutex for controlling threads

pt_mutex monitored_posts_mutex { "MONITORED POSTS"s };         ///< mutex for the monitored posts
pt_mutex rbn_buffer_mutex      { "RBN BUFFER"s };              ///< mutex for the RBN buffer

// parameters for the TCP connection
constexpr unsigned int IDLE_SECS   { 60 };
constexpr unsigned int RETRY_SECS  { 60 };
constexpr unsigned int MAX_RETRIES { 5 };

constexpr unsigned int CLUSTER_TIMEOUT { 2 };       // seconds

// -----------  dx_cluster  ----------------

/*! \class  dx_cluster
    \brief  A DX cluster or reverse beacon network
*/

/// process a read error
void dx_cluster::_process_error(void)
{ ost << "Processing error on TCP socket: " << _connection.to_string() << endl;

new_socket:
  try
  { try
    { _connection.close();                     // releases the file descriptor, closes the fundamental TCP connection
      ost << "TCP connection closed OK" << endl;
    }

    catch (...)
    { ost << "Error closing socket" << endl;
    }

    _connection.new_socket();                                       // get a new socket
    _connection.keep_alive(IDLE_SECS, RETRY_SECS, MAX_RETRIES);     // set the keepalive option
    _connection.bind(_my_ip);                                       // bind it to the correct IP address  *** THIS IS FAILING

    ost << "New TCP socket: " << _connection.to_string() << endl;

// reconnect to the server
reconnect:
    try
    { ost << "Attempting to use the new socket to connect to server: " << _server << " on port " << _port << endl;

// resolve the name
      const string dotted_decimal { name_to_dotted_decimal(_server) };

      _connection.destination(dotted_decimal, _port);
      ost << "Connected to " << dotted_decimal << "; now need to log in" << endl;

      _connection.send(_login_id + CRLF);
      ost << "login ID [" << _login_id << "] sent" << endl;
    }

    catch (const socket_support_error& E)
    { ost << "socket support error " << E.code() << " while setting destination: " << E.reason() << endl;

// sleep for one minute before retrying
      for (int counter { 0 }; counter < 60; ++counter)
      { sleep_for(1s);

        SAFELOCK(thread_check);

        if (exiting)
          return;
      }

      ost << "will attempt to reconnect" << endl;
      goto reconnect;
    }
  }

  catch (const tcp_socket_error& E)    // complete failure; go back and try again
  { ost << "TCP socket error " << E.code() << " in close/new socket/bind recovery sequence: " << E.reason() << endl;

    for (int counter { 0 }; counter < 60; ++counter)
    { sleep_for(1s);

      SAFELOCK(thread_check);

      if (exiting)
        return;
    }

    ost << "will attempt to create new socket" << endl;
    goto new_socket;
  }

  catch (const socket_support_error& E)    // complete failure; go back and try again
  { ost << "socket support error " << E.code() << " in close/new socket/bind recovery sequence: " << E.reason() << endl;

    for (int counter { 0 }; counter < 60; ++counter)
    { sleep_for(1s);

      SAFELOCK(thread_check);

      if (exiting)
        return;
    }

    goto new_socket;
  }

  _last_data_received = NOW_TP();  // reset the reception timer
}

/*! \brief              Constructor
    \param  context     the drlog context
    \param  src         whether this is a real DX cluster or the RBN
*/
dx_cluster::dx_cluster(const drlog_context& context, const POSTING_SOURCE src) :
  _connection(src == POSTING_SOURCE::CLUSTER ? context.cluster_server() : context.rbn_server(),     // create the connection
              src == POSTING_SOURCE::CLUSTER ? context.cluster_port() :  context.rbn_port(),
              context.my_ip()),
  _login_id(src == POSTING_SOURCE::CLUSTER ? context.cluster_username() : context.rbn_username()),  // choose the correct login name
  _my_ip(context.my_ip()),                                                                          // set my IP address
  _port(src == POSTING_SOURCE::CLUSTER ? context.cluster_port() : context.rbn_port()),              // choose the correct port
  _server(src == POSTING_SOURCE::CLUSTER ? context.cluster_server() : context.rbn_server()),        // choose the correct server
  _source(src),                                                                                     // set the source
  _timeout(CLUSTER_TIMEOUT)                                                                         // two-second timeout
{ 
// set the keepalive option
  _connection.keep_alive(IDLE_SECS, RETRY_SECS, MAX_RETRIES);

  ost << (src == POSTING_SOURCE::CLUSTER ? "cluster " : "RBN ") << "connection: " << _connection.to_string() << endl;
  
  string buf;

// send the login information as soon as we receive anything from the server
  while (buf.empty())
  { try
    { buf = _connection.read(_timeout);

      if (!buf.empty())
      { SAFELOCK(rbn_buffer);
        _unprocessed_input += buf;
      }
      else
        sleep_for(2s);              // just in case we somehow read zero bytes from the connection: this *can* happen, even though it's not supposed to; match CLUSTER_TIMEOUT
    }

    catch (const socket_support_error& e)
    { if (e.code() == SOCKET_SUPPORT_TIMEOUT)
        ost << "Timeout in dx_cluster constructor" << endl;
      else
      { ost << "socket support error while reading cluster connection; error # = " << e.code() << "; reason = " << e.reason() << endl;
        throw;
      }
    }

    catch (const tcp_socket_error& e)
    { if (e.code() == TCP_SOCKET_ERROR_IN_RECV)                                     // for now, treat as a timeout
        ost << "Error reading from TCP socket in dx_cluster constructor" << endl;
      else
      { ost << "TCP socket error while reading cluster connection; error # = " << e.code() << "; reason = " << e.reason() << endl;
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
  if (_source == POSTING_SOURCE::CLUSTER)
    _connection.send("sh/dx/100"s + CRLF);
}

/// destructor
dx_cluster::~dx_cluster(void)
{ _connection.send("BYE"s + CRLF);
  
  { SAFELOCK(rbn_buffer);
    _unprocessed_input = _connection.read(_timeout / 2);  // add a delay before we tear down the connection
  }
}

/*! \brief          Send a message to the cluster
    \param  msg     the message to be sent
    \return         whether transmission was successful
*/
bool dx_cluster::send(const std::string_view msg)
{ try
  { _connection.send(msg);
    return true;
  }

  catch (const tcp_socket_error& e)
  { ost << "Caught TCP socket error in dx_cluster::send(): " << "error code = " << e.code() << ", reason = " << e.reason() << endl;

    return false;
  }

  catch (...)
  { ost << "Caught non TCP socket error in dx_cluster::send()" << endl;

    return false;
  }
}

/*! \brief            Send a spot to the cluster
    \param  dx        callsign of DX station
    \param  freq      frequency of DX station
    \param  comment   comment to add to the post
    \return           whether the attemnpt to post was successful

*/
//bool dx_cluster::spot(const string& dx, const string& freq, const string& comment)
//bool dx_cluster::spot(const string_view dx, const string& freq, const string& comment)
//bool dx_cluster::spot(const string_view dx, const string_view freq, const string& comment)
bool dx_cluster::spot(const string_view dx, const string_view freq, const string_view comment)
{ const string dx_cmd { _test_spots ? "DXT"s : "DX"s };
  const string msg    { dx_cmd + " "s + dx + " "s + freq + " "s + comment + CRLF };          // DXT is a test spot

  ost << "sending spot: " << msg;

  return send(msg);
}

/*! \brief        Send a spot to the cluster
    \param  msg   the message to be sent
    \return       whether the attempt to post was successful
*/
bool dx_cluster::spot(const std::string_view msg)
{ const string dx_cmd   { _test_spots ? "DXT"s : "DX"s };       // DXT is a test spot
  const string spot_msg { dx_cmd + " "s + msg + CRLF };

  ost << "sending spot: " << spot_msg;

  return send(spot_msg);
}

/// reset the cluster socket
void dx_cluster::reset_connection(void)
{ ost << "Attempting to reset connection" << endl;
  _process_error();
  ost << "Completed reset of connection" << endl;
}

/*! \brief      Read from the cluster socket

    Any data read from the socket are appended to <i>_unprocessed_input</i>
*/
void dx_cluster::read(void)
{ string buf;
    
  try
  { buf = _connection.read(_timeout);
  }
    
  catch (const socket_support_error& e)
  { if (e.code() != SOCKET_SUPPORT_TIMEOUT)    // an error that is not a timeout has to be handled
    {
// error reading; try to reconnect
      ost << "Error reading from socket; attempting reconnection" << endl;

      _process_error();
    }  // be silent if it's just a timeout
  }

  catch (const tcp_socket_error&)
  { _process_error();
  }
  
  if (!buf.empty())
  { SAFELOCK(rbn_buffer);

    _unprocessed_input += buf;
    _last_data_received = NOW_TP();
  }
}

/*! \brief      Read from the cluster socket
    \return     the information that has been read from the socket but has not yet been processed
*/
string dx_cluster::get_unprocessed_input(void)
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
dx_post::dx_post(const string_view received_info, location_database& db, const enum POSTING_SOURCE post_source) :
  _source(post_source),
  _valid(false)
{
// there seem to be two formats for postings: the sh/dx format and the ordinary post format; deal with the sh/dx first
// first non-space sharacter is a digit and last character is ">"
// parsing failures are trapped
  if (last_char(received_info) == '>')
  { string_view copy { remove_leading_spaces <std::string_view> (received_info) };
  
// 18073.1  P49V        29-Dec-2009 1931Z  nice signal NW            <N7XR> 
    if (!copy.empty() and isdigit(copy[0]))
    { try
      { size_t char_posn  { copy.find(' ') };
        size_t space_posn { };

        if (char_posn != string_view::npos)
        { _frequency_str = copy.substr(0, char_posn);
          _freq = frequency(_frequency_str);

          if (_valid_hf_frequency())                            // we are interested only in HF posts
          { char_posn = copy.find_first_not_of(' ', char_posn);
            space_posn = copy.find_first_of(' ', char_posn);

            _callsign = copy.substr(char_posn, space_posn - char_posn);

            const location_info li { db.info(_callsign) };

            _canonical_prefix = li.canonical_prefix();
            _continent = li.continent();

            char_posn = copy.find_first_not_of(' ', space_posn);
            space_posn = copy.find_first_of(' ', char_posn);

// skip the date
            char_posn = copy.find_first_not_of(' ', space_posn);
            space_posn = copy.find_first_of(' ', char_posn);
            char_posn = copy.find_first_not_of(' ', space_posn);

            if (const size_t bra_posn { copy.find_last_of('<') }; bra_posn != string_view::npos)
            { _comment = copy.substr(char_posn, bra_posn - char_posn);
              _poster = copy.substr(bra_posn + 1, copy.length() - (bra_posn + 1) - 1);
              _poster_continent = db.info(_poster).continent();

              _valid = true;
              _time_processed = ::time(NULL);
              _time_processed_1 = NOW_TP();
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

// new RBN posting:
// DX de DL8TG-#:    3517.9  UT7LW          CW    30 dB  27 WPM  CQ      0047Z

// ordinary posting
// DX de RX9WN:      3512.0  UE9WFJ       see UE9WURC  QRZ.COM           1932Z

// we treat everything after the call as a comment
  if (!_valid)
  { if (received_info.length() <= 70)
      ost << "received short post: " << received_info << endl;

    if ( (substring <std::string_view> (received_info, 0, 6) == "DX de "sv) and (received_info.length() > 70) )
    { try
      { if (post_source == POSTING_SOURCE::RBN)
        { const vector<string> fields { split_string <std::string> (squash(received_info), ' ') };

          if (fields.size() >= 6)
          { _poster = substring <std::string> (fields[2], 0, fields[2].length() - 1);      // remove colon
            _poster_continent = db.info(_poster).continent();

            _frequency_str = fields[3];
            _freq = frequency(_frequency_str);
            _frequency_str = _freq.display_string();  // normalise the _frequency_str; some posters use two decimal places

            if (_valid_hf_frequency())                           // we are interested only in HF posts
            { _callsign = fields[4];

              const location_info li { db.info(_callsign) };

              _canonical_prefix = li.canonical_prefix();
              _continent = li.continent();
              _mode_str = fields[5];
              _valid = true;
              _time_processed = ::time(NULL);
              _time_processed_1 = std::chrono::system_clock::now();
            }
          }
        }

        if (!_valid)
        { const string_view copy{ remove_leading_spaces <std::string_view> (substring <std::string_view> (received_info, 6)) };
  
          if (const size_t colon_posn { copy.find(':') }; colon_posn != string_view::npos)
          { _poster = copy.substr(0, colon_posn);
            _poster_continent = db.info(_poster).continent();

            size_t char_posn  { copy.find_first_not_of(' ', colon_posn + 1) };
            size_t space_posn { copy.find_first_of(' ', char_posn) };

            _frequency_str = copy.substr(char_posn, space_posn - char_posn);
            _freq = frequency(_frequency_str);
            _frequency_str = _freq.display_string();  // normalise the _frequency_str; some posters use two decimal places

            if (_valid_hf_frequency())                            // we are interested only in HF posts
            { char_posn = copy.find_first_not_of(' ', space_posn);
              space_posn = copy.find_first_of(' ', char_posn);

              _callsign = copy.substr(char_posn, space_posn - char_posn);

              const location_info li { db.info(_callsign) };

              _canonical_prefix = li.canonical_prefix();
              _continent = li.continent();

              char_posn = copy.find_first_not_of(' ', space_posn);
              space_posn = copy.find_last_of(' ');

              _comment = copy.substr(char_posn);
              _valid = true;
              _time_processed = ::time(NULL);
              _time_processed_1 = std::chrono::system_clock::now();
            }
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
    _valid = (_callsign.find_first_not_of(CALLSIGN_CHARS) == string::npos);

// add the band
  if (_valid)
    _band = static_cast<BAND>(_freq);
}

/*! \brief          Write a <i>dx_post</i> object to an output stream
    \param  ost     output stream
    \param  dxp     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const dx_post& dxp)
{ ost << "Post: " << endl
      << "  band: " << BAND_NAME[dxp.band()] << endl
      << "  callsign: " << dxp.callsign() << endl
      << "  canonical prefix: " << dxp.canonical_prefix() << endl
      << "  comment: " << dxp.comment() << endl
      << "  continent: " << dxp.continent() << endl
      << "  frequency: " << dxp.freq() << endl
      << "  frequency str: " << dxp.frequency_str() << endl
      << "  mode_str: " << dxp.mode_str() << endl
      << "  poster: " << dxp.poster() << endl
      << "  source: " << ( (dxp.source() == POSTING_SOURCE::RBN) ? "RBN" : "Cluster") << endl
      << "  time processed: " << dxp.time_processed() << endl
      << "  time processed 1: " << dxp.time_processed_1() << endl
      << "  valid: " << dxp.valid();

  return ost;
}

// -----------  monitored_posts_entry  ----------------

/*! \class  monitored_posts_entry
    \brief  An entry in the container of monitored posts
*/

/*! \brief          Write a <i>monitored_posts_entry</i> object to an output stream
    \param  ost     output stream
    \param  mpe     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const monitored_posts_entry& mpe)
{ ost << "Monitored: " << mpe.callsign()
      << ", absolute expiration = " << mpe.expiration()
      << ", relative expiration = " << mpe.expiration() - NOW_TP()
      << ", frequency = " << mpe.frequency_str();

  return ost;
}

// -----------  monitored_posts  ----------------

/*! \class  monitored_posts
    \brief  Handle the monitoring of certain stations
*/

/*! \brief              Is a particular call monitored?
    \param  callsign    call to be tested
    \return             whether <i>callsign</i> is being monitored
*/
bool monitored_posts::is_monitored(const std::string_view callsign) const
{ SAFELOCK(monitored_posts);

  return _callsigns.contains(callsign);
}

/*! \brief          Test a post, and possibly add to <i>_entries</i>
    \param  post    post to be tested
*/
void monitored_posts::operator+=(const dx_post& post)
{ const monitored_posts_entry mpe           { post };

  bool                  stop_search         { false };
  bool                  found_call_and_band { false };

  SAFELOCK(monitored_posts);

  for (deque<monitored_posts_entry>::iterator it { _entries.begin() }; (!stop_search and (it != _entries.end())); ++it)
  { const monitored_posts_entry& old_mpe { *it };

    if ( (mpe.callsign() == old_mpe.callsign()) and (mpe.band() == old_mpe.band()) )
    { if ( mpe.expiration() > old_mpe.expiration() )                // new expiration is later than expiration from old post
        _entries -= it;                                             // remove old entry
      else
        found_call_and_band = true;  // found entry, but it expires after the post we're testing

      stop_search = true;
    }
  }

  if (stop_search and !found_call_and_band)               // new expiration is later than expiration from old post
  { _entries += mpe;                                      // insert new entry
    _is_dirty = true;
  }

  if (!stop_search)                                       // no old post exists
  { _entries += mpe;
    _is_dirty = true;
  }

  while (_entries.size() > _max_entries)  // should happen only once at most
  { --_entries;
    _is_dirty = true;                     // should be unnecessary, since should already be true
  }
}

/*! \brief              Add a call to the set of those being monitored
    \param  new_call    call to be added
*/
void monitored_posts::operator+=(const string_view new_call)
{ SAFELOCK(monitored_posts);

  _callsigns += new_call;
}

/*! \brief                  Remove a call from the set of those being monitored
    \param  call_to_remove  call to be removed
*/
void monitored_posts::operator-=(const string_view call_to_remove)
{ SAFELOCK(monitored_posts);

  _callsigns -= call_to_remove;

// remove any entries that are for this call
  const size_t original_size { _entries.size() };

  erase_if(_entries, [call_to_remove] (monitored_posts_entry& mpe) { return (mpe.callsign() == call_to_remove); } );

  _is_dirty |= (original_size != _entries.size());
}

/// prune <i>_entries</i>
void monitored_posts::prune(void)
{ SAFELOCK(monitored_posts);

  const size_t original_size { _entries.size() };

  erase_if(_entries, [now = NOW_TP()] (monitored_posts_entry& mpe) { return (mpe.expiration() < now); } );

  _is_dirty |= (original_size != _entries.size());
}

/// convert to a vector of strings suitable for display in a window
vector<string> monitored_posts::to_strings(void) const
{ //    std::ranges::for_each(in | std::views::transform(rot13), show);

  SAFELOCK(monitored_posts);

//  return SR::to<vector<string>>(_entries | std::views::transform([] (const monitored_posts_entry& mpe) { return mpe.to_string(); }) );
  return SR::to<vector<string>>( _entries | std::views::transform(&monitored_posts_entry::to_string) );
//  vector<string> rv;
//  rv.reserve(_entries.size());

//  SAFELOCK(monitored_posts);

//  FOR_ALL(_entries, [&rv] (const monitored_posts_entry& mpe) { rv += (mpe.to_string()); } );

//  return rv;
}
