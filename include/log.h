// $Id: log.h 66 2014-06-14 19:22:10Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef LOG_H
#define LOG_H

/*!     \file log.h

        Classes and functions related to the log
*/

#include "cty_data.h"
#include "drlog_context.h"
#include "log_message.h"
#include "pthread_support.h"
#include "qso.h"
#include "rules.h"
#include "serialization.h"

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <fstream>
#include <iostream>

extern message_stream ost;

class running_statistics;

// -----------  logbook  ----------------

/*!     \class logbook
        \brief The log
*/

extern pt_mutex _log_mutex;  // keep this outside the class so that const objects can lock the mutex

class logbook
{
protected:
  
// we keep two copies, since we usually want to use a map, but it's
// not possible to put that in chronological order without including seconds...
// and even with seconds a change would be necessary should this ever be adapted for
// use in a multi station
  std::multimap<std::string, QSO>  _log;
  std::vector<QSO>                 _log_vec;
  
  QSO                              _removed_qso;   // most-recently removed QSO;  currently we don't do anything with this

public:
  
// return qso number n (wrt 1)
  const QSO operator[](const size_t n);

// return most recent qso
  inline const QSO last_qso(void)
    { return ( (*this)[size()] ); }

/// add a qso
  void operator+=(const QSO& q);
    
/// remove qso number n (wrt 1)
  void operator-=(const unsigned int n);

/// remove most-recent qso
  inline void remove_last_qso(void)
    { *this -= size(); }

/// all the QSOs with a particular call, returned in chronological order
  const std::vector<QSO> worked(const std::string& call) const;
  
/// number of times a call has been worked
  const unsigned int n_worked(const std::string& call) const;

/// has a particular call been worked at all?
  inline const bool qso_b4(const std::string& call) const
    { SAFELOCK(_log);
      return (_log.lower_bound(call) != _log.upper_bound(call)); 
    }
    
/// has a call been worked on a particular band?
  const bool qso_b4(const std::string& call, const BAND b) const;

/// has a call been worked on a particular mode?
  const bool qso_b4(const std::string& call, const MODE m) const;

/// has a call been worked on a particular band and mode?
  const bool qso_b4(const std::string& call, const BAND b, const MODE m) const;
  
/// a string list of bands on which a call is needed
  const std::string call_needed(const std::string& call, const contest_rules& rules) const;
  
/// would a QSO be a dupe?
  const bool is_dupe(const QSO& qso, const contest_rules& rules) const;

/// would a QSO be a dupe?
  const bool is_dupe(const std::string& call, const BAND b, const MODE m, const contest_rules& rules) const;
  
/// calculate total points
//  const unsigned int points(const contest_rules& rules, location_database& location_db) const;
  
/// calculate QSO points (i.e., mults = 1)
  const unsigned int qso_points(const contest_rules& rules, location_database& location_db) const;
  
/// return time-ordered container of qsos
  const std::list<QSO> as_list(void) const;

/// return time-ordered container of qsos
  const std::vector<QSO> as_vector(void) const;
  
//  template <typename T>
//  const T as(void) const
//  { T rv;
//
//    { SAFELOCK(_log);
//
//      copy(_log.cbegin(), _log.cend(), back_inserter(rv));
//    }
//
//    rv.sort(earlier);    // sorts according to earlier(const QSO&, const QSO&)
//
//    return rv;
//  }

template <class UnaryPredicate>
  const std::vector<QSO> filter(UnaryPredicate pred) const
  { std::vector<QSO> rv;

    copy_if(_log_vec.cbegin(), _log_vec.cend(), back_inserter(rv), pred);

    return rv;
  }
  
/// recalculate the dupes
  const logbook recalculate_dupes(const contest_rules& rules) const;
  
/// generate a Cabrillo log
  const std::string cabrillo_log(const drlog_context& context, const unsigned int score) const;
  
/// generate a trlog log
  const std::string trlog_log(const drlog_context& context, const unsigned int score) const; 
  
/// read from a Cabrillo file
  void read_cabrillo(const std::string& filename, const std::string& cabrillo_qso_template);

/// read from a Cabrillo file using space-delimited fields
  void read_cabrillo(const std::string& filename, const std::vector<std::string>& cabrillo_fields);

/// read a trlog log
  void read_trlog_log(const std::string& filename);
  
/// clear the logbook
  inline void clear(void)
    { _log.clear(); }

/// how many QSOs are in the log?
  inline size_t size(void) const
    { return _log.size(); }

  inline const bool empty(void) const
    { return _log.empty(); }

  inline size_t n_qsos(void) const
    { return size(); }

// modifies statistics
//  void populate_from_disk_file(const std::string& filename, const contest_rules& rules, running_statistics& statistics);

// get the value of an exchange field from the last QSO with someone; returns empty string if anything goes wrong
  const std::string exchange_field_value(const std::string& callsign, const std::string& exchange_field_name);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { SAFELOCK(_log);

    ar & _log
       & _log_vec;
  }
};

// -----------  log_extract  ----------------

/*!     \class log_extract
        \brief Support for bits of the log
*/

class log_extract
{
protected:

  window&      _win;
  unsigned int _win_size;

  std::deque<QSO> _qsos;

  pt_mutex _extract_mutex;

public:

  explicit log_extract(window& w);

  void prepare(void);

  inline const size_t size(void)
  { SAFELOCK(_extract);
    return _qsos.size();
  }

  inline const bool empty(void)
  { SAFELOCK(_extract);
    return _qsos.empty();
  }

  inline const unsigned int win_size(void)
  { SAFELOCK(_extract);
    return _win_size;
  }

  inline const std::deque<QSO> qsos(void)
  { SAFELOCK(_extract);
    return _qsos;
  }

  void operator+=(const QSO& qso);

// augment without limiting the size of the deque
  inline void add_without_limit(const QSO& qso)
  { SAFELOCK(_extract);
    _qsos.push_back(qso);
  }

  inline void clear(void)
  { SAFELOCK(_extract);
    _qsos.clear();
  }

  void display(void);

// get recent QSOs from a log
  void recent_qsos(const logbook&, const bool to_display = true);

template <typename T>
  void operator=(const T& t)
  { SAFELOCK(_extract);
    _qsos.clear();
    copy(t.cbegin(), t.cend(), back_inserter(_qsos));
  }

};

#endif    // LOG_H
