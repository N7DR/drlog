// $Id: cluster.h 206 2022-05-22 12:47:37Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CLUSTER_H
#define CLUSTER_H

/*! \file   cluster.h

    Classes and functions related to a DX cluster and the Reverse Beacon Network
*/

#include "drlog_context.h"
#include "macros.h"
#include "socket_support.h"

#include <atomic>
#include <string>

/// the source of a remote post
enum class POSTING_SOURCE { CLUSTER,                  ///< traditional cluster
                            RBN                       ///< Reverse Beacon Network
                          };

constexpr unsigned int MONITORED_POSTS_DURATION { 3600 };     ///< monitored posts are valid for one hour

// -----------  dx_cluster  ----------------

/*! \class  dx_cluster
    \brief  A DX cluster or reverse beacon network
*/

class dx_cluster
{ using TIME_POINT = std::chrono::time_point<std::chrono::system_clock>;

protected:

  tcp_socket          _connection;                  ///< TCP socket for communication with the network
  TIME_POINT          _last_data_received { };      ///< time point of last data received
  std::string         _login_id;                    ///< my login identifier
  std::string         _my_ip;                       ///< my IP address
  unsigned int        _port;                        ///< server port
  std::string         _server;                      ///< name or IP address of the server
  enum POSTING_SOURCE _source;                      ///< source for postings
  unsigned int        _timeout;                     ///< timeout in seconds (defaults to 1)
  std::string         _unprocessed_input;           ///< buffer for messages from the network

/// process a read error
  void _process_error(void);
    
public:

/*! \brief              Constructor
    \param  context     the drlog context
    \param  src         whether this is a real DX cluster or the RBN
*/
  dx_cluster(const drlog_context& context, const POSTING_SOURCE src);
  
/// destructor
  ~dx_cluster(void);

  dx_cluster(const dx_cluster&) = delete;       /// forbid copying

  READ(source);        ///< source for postings

/*! \brief      Read from the cluster socket
    \return     the current bytes waiting on the cluster socket
*/
  std::string read(void);
  
/*! \brief      Read from the cluster socket
    \return     the information that has been read from the socket but has not yet been processed
*/
  std::string get_unprocessed_input(void);
  
/*! \brief          Send a message to the cluster
    \param  msg     the message to be sent
    \return         whether transmission was successful
*/
  bool send(const std::string& msg = CRLF);
 //   { _connection.send(msg); }
//  inline void send(const std::string& msg = CRLF)
//    { _connection.send(msg); }

/*! \brief            Send a spot to the cluster
    \param  dx        callsign of DX station
    \param  freq      frequency of DX station
    \param  comment   comment to add to the post
    \return           whether the attemnpt to post was successful
*/
  bool spot(const std::string& dx, const std::string& freq, const std::string& comment = EMPTY_STR);

/*! \brief        Send a spot to the cluster
    \param  msg   the message to be sent
    \return       whether the attemnpt to post was successful
*/
  bool spot(const std::string& msg);

/// reset the cluster socket
  void reset_connection(void);

/*! \brief      The status of the connection, as a human-readable string
    \return     the status of the connection
*/
  inline std::string connection_status(void)
    { return _connection.to_string(); }

/*! \brief      Return the time since the last data were received on the connection
    \return     the time since the last data were received on the connection, in seconds
*/
  inline std::chrono::seconds time_since_data_last_received(void) const
    { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - _last_data_received); }
};

// -----------  dx_post  ----------------

/*! \class  dx_post
    \brief  Convert a line from the cluster to a DX posting
*/

class dx_post
{
protected:

  enum BAND             _band;              ///< band of post
  std::string           _callsign;          ///< callsign that was heard
  std::string           _canonical_prefix;  ///< canonical prefix corresponding to <i>callsign</i>
  std::string           _comment;           ///< comment supplied by poster
  std::string           _continent;         ///< continent of <i>_callsign</i>
  frequency             _freq;              ///< frequency at which <i>_callsign</i> was heard
  std::string           _frequency_str;     ///< frequency in format xxxxx.y [kHz]
  std::string           _mode_str;          ///< mode string from RBN post (empty if none)
  std::string           _poster;            ///< call of poster
  std::string           _poster_continent;  ///< continent of <i>_poster</i>
  enum POSTING_SOURCE   _source;            ///< source of the post (POSTING_CLUSTER or POSTING_RBN)
  time_t                _time_processed;    ///< time (relative to the UNIX epoch) at which we processed the post
  bool                  _valid;             ///< is it a valid post?
    
/// does the frequency appear to be valid? Nothing fancy needed here
  inline bool _valid_frequency(void) const
    { return ( (_freq.khz() >= 1'800) and (_freq.khz() <= 29'700) ); }

public:
    
/*! \brief                  Constructor
    \param  received_info   a line received from the cluster or RBN
    \param  db              the location database for this contest
    \param  post_source     the origin of the post
*/
  dx_post(const std::string& received_info, location_database& db, const enum POSTING_SOURCE post_source);

  READ(band);                   ///< band of post
  READ_AND_WRITE(callsign);     ///< callsign that was heard
  READ(canonical_prefix);       ///< canonical prefix corresponding to <i>callsign</i>
  READ(comment);                ///< comment supplied by poster
  READ(continent);              ///< continent of <i>_callsign</i>
  READ(freq);                   ///< frequency at which <i>_callsign</i> was heard
  READ(frequency_str);          ///< frequency in format xxxxx.y [kHz]
  READ(mode_str);               ///< mode string from RBN post (empty if none)
  READ(poster);                 ///< call of poster
  READ(poster_continent);       ///< continent of <i>_poster</i>
  READ(source);                 ///< source of the post (POSTING_CLUSTER or POSTING_RBN)
  READ(time_processed);         ///< time (relative to the UNIX epoch) at which we processed the post
  READ(valid);                  ///< is it a valid post?

// syntactic sugar

/// was the cluster the source of the post?
  inline bool from_cluster(void) const
    { return (_source == POSTING_SOURCE::CLUSTER); }

/// was the RBN the source of the post?
  inline bool from_rbn(void) const
    { return (_source == POSTING_SOURCE::RBN); }
};

/*! \brief          Write a <i>dx_post</i> object to an output stream
    \param  ost     output stream
    \param  dxp     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const dx_post& dxp);

// -----------  monitored_posts_entry  ----------------

/*! \class  monitored_posts_entry
    \brief  An entry in the container of monitored posts
*/

class monitored_posts_entry
{
protected:

  enum BAND     _band;              ///< band
  std::string   _callsign;          ///< callsign
  time_t        _expiration;        ///< time (relative to the UNIX epoch) at which entry will expire
  std::string   _frequency_str;     ///< frequency in format xxxxx.y [kHz]

public:

/*! \brief          Constructor
    \param  post    post from cluster or RBN
*/
  explicit monitored_posts_entry(const dx_post& post) :
    _callsign(post.callsign()),
    _frequency_str(post.frequency_str()),
    _expiration(post.time_processed() + MONITORED_POSTS_DURATION),
    _band(post.band())
  { }

  READ(band);               ///< band
  READ(callsign);           ///< callsign
  READ(expiration);         ///< time (relative to the UNIX epoch) at which entry will expire
  READ(frequency_str);      ///< frequency in format xxxxx.y [kHz]

/// convert to a string suitable for display in a window
  inline std::string to_string(void) const
    { return ( pad_left(_frequency_str, 7) + SPACE_STR + _callsign ); }
};

/*! \brief          Write a <i>monitored_posts_entry</i> object to an output stream
    \param  ost     output stream
    \param  mpe     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const monitored_posts_entry& mpe);

// -----------  monitored_posts  ----------------

/*! \class  monitored_posts
    \brief  Handle the monitoring of certain stations
*/

extern pt_mutex monitored_posts_mutex;         ///< mutex for the monitored posts

class monitored_posts
{
protected:

  std::set<std::string>             _callsigns   { };           ///< monitored calls
  std::deque<monitored_posts_entry> _entries;                   ///< calls monitored within past MONITORED_POSTS_DURATION seconds; basically a queue, but needs erase() capability
  bool                              _is_dirty    { false };     ///< whether info has changed since last output
  std::atomic<unsigned int>         _max_entries { 0 };         ///< number of displayable entries

public:

/// default constructor
  monitored_posts(void) = default;

  SAFEREAD(entries, monitored_posts);           ///< calls monitored within past MONITORED_POSTS_DURATION seconds
  READ(is_dirty);                               ///< whether info has changed since last output

  SAFE_READ_AND_WRITE(callsigns, monitored_posts);      ///< monitored calls

  inline void max_entries(const unsigned int v)
    { _max_entries = v; }

  unsigned int max_entries(void) const
    { return _max_entries; }

/*! \brief              Is a particular call monitored?
    \param  callsign    call to be tested
    \return             whether <i>callsign</i> is being monitored
*/
  bool is_monitored(const std::string& callsign) const;

/*! \brief          Test a post, and possibly add to <i>_entries</i>
    \param  post    post to be tested
*/
  void operator+=(const dx_post& post);

/*! \brief              Add a call to the set of those being monitored
    \param  new_call    call to be added
*/
  void operator+=(const std::string& new_call);

/*! \brief                  Remove a call from the set of those being monitored
    \param  call_to_remove  call to be removed
*/
  void operator-=(const std::string& call_to_remove);

/// prune <i>_entries</i>
  void prune(void);

/// convert to strings suitable for display in a window
  std::vector<std::string> to_strings(void) const;
};

#endif    // CLUSTER_H
