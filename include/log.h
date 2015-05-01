// $Id: log.h 94 2015-02-07 15:06:10Z  $

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

extern message_stream ost;                  ///< for debugging, info

class running_statistics;                   // forward declaration

// -----------  logbook  ----------------

/*!     \class logbook
        \brief The log
*/

extern pt_mutex _log_mutex;                 ///< mutex for log; keep this outside the class so that const objects can lock the mutex

class logbook
{
protected:
  
// we keep two copies, since we usually want to use a map, but it's
// not possible to put that in chronological order without including seconds...
// and even with seconds a change would be necessary should this ever be adapted for
// use in a multi station
  std::multimap<std::string, QSO>  _log;        ///< map version of log
  std::vector<QSO>                 _log_vec;    ///< vector (chronological) version of log

public:
  
/*! \brief      Return an individual QSO by number (wrt 1)
    \param  n   QSO number to return
    \return     the <i>n</i>th QSO

    If <i>n</i> is out of range, then returns an empty QSO
*/
  const QSO operator[](const size_t n) const;

/// return the most recent qso
  inline const QSO last_qso(void) const
    { SAFELOCK(_log);

      return ( (*this)[size()] );
    }

/*! \brief      Add a QSO to the logbook
    \param  q   QSO to add
*/
  void operator+=(const QSO& q);
    
/*! \brief      Remove an individual QSO by number (wrt 1)
    \param  n   QSO number to remove

    If <i>n</i> is out of range, then does nothing
*/
  void operator-=(const unsigned int n);

/// remove most-recent qso
  inline void remove_last_qso(void)
    { SAFELOCK(_log);

      *this -= size();    // remember, numbering is wrt 1
    }

/*! \brief          All the QSOs with a particular call, in chronological order
    \param  call    target callsign
    \return         vector of QSOs in chronological order

    If there are no QSOs with <i>call</i>, returns an empty vector
*/
  const std::vector<QSO> worked(const std::string& call) const;
  
/*! \brief          The number of times that a particular call has been worked
    \param  call    target callsign
    \return         number of times that <i>call</i> has been worked
*/
  const unsigned int n_worked(const std::string& call) const;

/// has a particular call been worked at all?
  inline const bool qso_b4(const std::string& call) const
    { SAFELOCK(_log);

      return (_log.lower_bound(call) != _log.upper_bound(call)); 
    }
    
/*!     \brief          Has a call been worked on a particular band?
        \param  call    target callsign
        \param  b       target band
        \return         whether <i>call</i> has been worked on <i>b</i>
*/
  const bool qso_b4(const std::string& call, const BAND b) const;

/*!     \brief          Has a call been worked on a particular mode?
        \param  call    target callsign
        \param  m       target mode
        \return         whether <i>call</i> has been worked on <i>m</i>
*/
  const bool qso_b4(const std::string& call, const MODE m) const;

/*!     \brief          Has a call been worked on a particular band and mode?
        \param  call    target callsign
        \param  b       target band
        \param  m       target mode
        \return         whether <i>call</i> has been worked on <i>b</i> and <i>m</i>
*/
  const bool qso_b4(const std::string& call, const BAND b, const MODE m) const;
  
/*!     \brief          Get a string list of bands on which a call is needed
        \param  call    target callsign
        \param  rules   rules for the contest
        \return         string list of bands on which a call is needed (separated by three spaces)
*/
  const std::string call_needed(const std::string& call, const contest_rules& rules) const;
  
/*!     \brief          Would a QSO be a dupe, according to the rules?
        \param  qso     target QSO
        \param  rules   rules for the contest
        \return         whether <i>qso</i> would be a dupe
*/
  const bool is_dupe(const QSO& qso, const contest_rules& rules) const;

/*!     \brief          Would a QSO be a dupe, according to the rules?
        \param  call    target call
        \param  b       target band
        \param  m       target mode
        \param  rules   rules for the contest
        \return         whether a QSO with <i>call</i> on band <i>b</i> and mode <i>m</i> would be a dupe
*/
  const bool is_dupe(const std::string& call, const BAND b, const MODE m, const contest_rules& rules) const;

/// return time-ordered container of QSOs
  const std::list<QSO> as_list(void) const;

/// return time-ordered container of QSOs
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

/*!     \brief          Return the QSOs, filtered by some criterion
        \param  pred    predicate to apply
        \return         The QSOs for which <i>pred</i> is true

        The returned QSOs are in chronological order
*/
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
  inline const size_t size(void) const
    { return _log.size(); }

/// how many QSOs are in the log?
  inline const size_t n_qsos(void) const
    { return size(); }

/// is the log empty?
  inline const bool empty(void) const
    { return _log.empty(); }

// get the value of an exchange field from the last QSO with someone; returns empty string if anything goes wrong
/*!     \brief                          Get the value of an exchange field from the most recent QSO with a station
        \param  callsign                call for which the information is required
        \param  exchange_field_name     name of the exchange field for which the information is required
        \return                         Value received from <i>callsign</i> for the field <i>exchange_field_name</i> during the most recent QSO with <i>callsign</i>

        Returns empty string if anything goes wrong.
*/
  const std::string exchange_field_value(const std::string& callsign, const std::string& exchange_field_name);

/*! \brief          Return all the QSOs that contain an exchange field that matches a target
    \param  target  target string for exchange fields
    \return         All the QSOs that contain an exchange field that matches a target
*/
  const std::vector<QSO> match_exchange(const std::string& target) const;

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

  window&      _win;                        ///< window associated with the log extract  (NB, during construction, _win will be constructed first)
  unsigned int _win_size;                   ///< height of the associated window

  std::deque<QSO> _qsos;                    ///< QSOs contained in the extract

  pt_mutex _extract_mutex;                  ///< mutex for thread safety

public:

/*! \brief  constructor
    \param  w               window to be used by this extract
*/
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
  void recent_qsos(const logbook& lgbook, const bool to_display = true);

  void match_exchange(const logbook& lgbook, const std::string& target);

template <typename T>
  void operator=(const T& t)
  { SAFELOCK(_extract);
    _qsos.clear();
    copy(t.cbegin(), t.cend(), back_inserter(_qsos));
  }

};

#endif    // LOG_H
