// $Id: cluster.h 55 2014-03-22 20:32:08Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef CLUSTER_H
#define CLUSTER_H

/*!     \file cluster.h

        Classes and functions related to a DX cluster and the reverse beacon network
*/

#include "drlog_context.h"
#include "macros.h"
#include "socket_support.h"

#include <string>

enum POSTING_SOURCE { POSTING_CLUSTER,
                      POSTING_RBN
                    };

// -----------  dx_cluster  ----------------

/*!     \class dx_cluster
        \brief A reverse beacon network
*/

class dx_cluster
{
protected:
  std::string     _server;                        ///< name or IP address of the server
  unsigned int    _port;                          ///< server port
  std::string     _my_ip;                         ///< my IP address
  unsigned int    _timeout;                       ///< timeout in seconds (defaults to 1)
  std::string     _login_id;                      ///< my login identifier
  enum POSTING_SOURCE _source;                    ///< source for postings
  
  std::string     _unprocessed_input;             ///< buffer for messages from the network
  
  tcp_socket      _connection;                    ///< TCP socket for communication with the network
  
  void _process_error(void);

//  pt_mutex        _rbn_mutex;

/// forbid copying
  dx_cluster(const dx_cluster&);
    
public:
//  tcp_socket cluster_socket(context.cluster_server(), context.cluster_port(), context.my_ip());
//  dx_cluster(const std::string& server_name, const unsigned int server_port, const std::string& ip_address, const std::string& id);
  dx_cluster (const drlog_context& context, const POSTING_SOURCE src);
  
  virtual ~dx_cluster(void);
  
  const std::string read(void);
  
  const std::string get_unprocessed_input(void);
  
  void send(const std::string& msg = "\r\n"); 

  READ(source);

  void reset(void);
};


// -----------  dx_post  ----------------

/*!     \class dx_post
        \brief Convert a line from the cluster to a DX posting
*/

class dx_post
{
  
protected:
    bool         _valid;                 ///< is it a valid post?
    
    std::string  _poster;
    frequency    _freq;
    std::string  _frequency_str;
    std::string  _callsign;
    std::string  _canonical_prefix;
    std::string  _continent;
    std::string  _comment;
    enum BAND    _band;
    time_t       _time_processed;
    enum POSTING_SOURCE _source;
    
    inline const bool _valid_frequency(void) const
      { return (_freq.khz() >= 1800 and _freq.khz() <= 29700); }

public:
    
  dx_post(const std::string& received_info, location_database& db, const enum POSTING_SOURCE post_source);
  
  inline virtual ~dx_post(void)
    { }

  READ(valid);
  READ(poster);
  READ(freq);
  READ(frequency_str);
  READ(callsign);
  READ(canonical_prefix);
  READ(continent);
  READ(comment);
  READ(band);
  READ(time_processed);
  READ(source);
};

#endif    // CLUSTER_H
