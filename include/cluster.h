// $Id: cluster.h 137 2016-12-15 20:07:54Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CLUSTER_H
#define CLUSTER_H

/*! \file cluster.h

    Classes and functions related to a DX cluster and the reverse beacon network
*/

#include "drlog_context.h"
#include "macros.h"
#include "socket_support.h"

#include <string>

enum POSTING_SOURCE { POSTING_CLUSTER,
                      POSTING_RBN
                    };                          ///< the source of a remote posting

const unsigned int MONITORED_POSTS_DURATION = 3600;     ///< monitored posts are valid for one hour

// -----------  dx_cluster  ----------------

/*! \class  dx_cluster
    \brief  A DX cluster or reverse beacon network
*/

class dx_cluster
{
protected:
  tcp_socket          _connection;                    ///< TCP socket for communication with the network
  std::string         _login_id;                      ///< my login identifier
  std::string         _my_ip;                         ///< my IP address
  unsigned int        _port;                          ///< server port
  std::string         _server;                        ///< name or IP address of the server
  enum POSTING_SOURCE _source;                        ///< source for postings
  unsigned int        _timeout;                       ///< timeout in seconds (defaults to 1)
  std::string         _unprocessed_input;             ///< buffer for messages from the network

/// process a read error
  void _process_error(void);

/// forbid copying: declared but never defined
  dx_cluster(const dx_cluster&);
    
public:

/*! \brief              Constructor
    \param  context     the drlog context
    \param  src         whether this is a real DX cluster or the RBN
*/
  dx_cluster (const drlog_context& context, const POSTING_SOURCE src);
  
/// destructor
  virtual ~dx_cluster(void);
  
/*! \brief      Read from the cluster socket
    \return     the current bytes waiting on the cluster socket
*/
  const std::string read(void);
  
/*! \brief      Read from the cluster socket
    \return     the information that has been read from the socket but has not yet been processed
*/
  const std::string get_unprocessed_input(void);
  
/*! \brief          Send a message to the cluster
    \return msg     the message to be sent
*/
  inline void send(const std::string& msg = "\r\n")
   { _connection.send(msg); }

  READ(source);        ///< source for postings

/// reset the cluster socket
  void reset(void);
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
  std::string           _poster;            ///< call of poster
  enum POSTING_SOURCE   _source;            ///< source of the post (POSTING_CLUSTER or POSTING_RBN)
  time_t                _time_processed;    ///< time (relative to the UNIX epoch) at which we processed the post
  bool                  _valid;             ///< is it a valid post?
    
/// does the frequency appear to be valid? Nothing fancy needed here
  inline const bool _valid_frequency(void) const
    { return (_freq.khz() >= 1800 and _freq.khz() <= 29700); }

public:
    
/*! \brief                  Constructor
    \param  received_info   a line received from the cluster or RBN
    \param  db              the location database for this contest
    \param  post_source     the origin of the post
*/
  dx_post(const std::string& received_info, location_database& db, const enum POSTING_SOURCE post_source);
  
/// destructor
  inline virtual ~dx_post(void)
    { }

  READ(band);                   ///< band of post
  READ(callsign);               ///< callsign that was heard
  READ(canonical_prefix);       ///< canonical prefix corresponding to <i>callsign</i>
  READ(comment);                ///< comment supplied by poster
  READ(continent);              ///< continent of <i>_callsign</i>
  READ(freq);                   ///< frequency at which <i>_callsign</i> was heard
  READ(frequency_str);          ///< frequency in format xxxxx.y [kHz]
  READ(poster);                 ///< call of poster
  READ(source);                 ///< source of the post (POSTING_CLUSTER or POSTING_RBN)
  READ(time_processed);         ///< time (relative to the UNIX epoch) at which we processed the post
  READ(valid);                  ///< is it a valid post?
};

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
  monitored_posts_entry(const dx_post& post);

  READ(band);               ///< band
  READ(callsign);           ///< callsign
  READ(expiration);
  READ(frequency_str);

/// convert to a string suitable for display in a window
  inline const std::string to_string(void) const
    { return ( pad_string(_frequency_str, 7, PAD_LEFT) + " " + _callsign ); }
};

/// ostream << monitored_posts_entry
std::ostream& operator<<(std::ostream& ost, const monitored_posts_entry& mpe);

// -----------  monitored_posts  ----------------

/*! \class  monitored_posts
    \brief  Handle the monitoring of certain stations
*/

extern pt_mutex monitored_posts_mutex;         ///< mutex for the monitored posts

class monitored_posts
{
protected:

  std::set<std::string> _callsigns;     ///< monitored calls
  bool                  _is_dirty;      ///< whether info has changed since last output
  unsigned int          _max_entries;   ///< number of displayable entries

  std::deque<monitored_posts_entry> _entries;          ///< calls monitored within past MONITORED_POSTS_DURATION seconds

public:

/// constructor
  monitored_posts(void);

  SAFEREAD(entries, monitored_posts);           ///< calls monitored within past MONITORED_POSTS_DURATION seconds
  READ(is_dirty);                               ///< whether info has changed since last output

  SAFE_READ_AND_WRITE(callsigns, monitored_posts);      ///< monitored calls
  SAFE_READ_AND_WRITE(max_entries, monitored_posts);    ///< number of displayable entries

/*! \brief              Is a particular call monitored?
    \param  callsign    call to be tested
    \return             whether <i>callsign</i> is being monitored
*/
  const bool is_monitored(const std::string& callsign) const;

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
  const std::vector<std::string> to_strings(void) const;
};

#endif    // CLUSTER_H
