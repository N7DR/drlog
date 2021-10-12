// $Id: log.h 192 2021-09-19 14:03:15Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef LOG_H
#define LOG_H

/*! \file   log.h

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

// -----------  logbook  ----------------

/*! \class  logbook
    \brief  The log
*/

extern pt_mutex _log_mutex;                 ///< mutex for log; keep this outside the class so that const objects can lock the mutex

class logbook
{
protected:
  
// we keep two copies, since we usually want to use a map, but it's
// not possible to put that in chronological order without including seconds...
// and even with seconds a change would be necessary should this ever be adapted for
// use in a multi station
  std::multimap<std::string, QSO>  _log;        ///< map version of log; key is callsign; cannot use unordered_multimap; we need call ordering
  std::vector<QSO>                 _log_vec;    ///< vector (chronological) version of log

  void _modify_qso_with_name_and_value(QSO& qso, const std::string& name, const std::string& value);  

public:
  
/*! \brief      Return an individual QSO by number (wrt 1)
    \param  n   QSO number to return
    \return     the <i>n</i>th QSO

    If <i>n</i> is out of range, then returns an empty QSO
*/
  QSO operator[](const size_t n) const;

/// return the most recent QSO
  inline QSO last_qso(void) const
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

/*! \brief      Remove most-recent QSO
    \return     the removed QSO

    Does nothing and returns an empty QSO if there are no QSOs in the log
*/
  QSO remove_last_qso(void);

/*! \brief                  Remove several recent QSOs
    \param  n_to_remove     number of QSOs to remove

    It is legal to call this function even if <i>n_to_remove</i> is greater than
    the number of QSOs in the logbook
*/
  void remove_last_qsos(const unsigned int n_to_remove);

/*! \brief          All the QSOs with a particular call, in chronological order
    \param  call    target callsign
    \return         vector of QSOs in chronological order

    If there are no QSOs with <i>call</i>, returns an empty vector
*/
  std::vector<QSO> worked(const std::string& call) const;
  
/*! \brief          The number of times that a particular call has been worked
    \param  call    target callsign
    \return         number of times that <i>call</i> has been worked
*/
  unsigned int n_worked(const std::string& call) const;

/*! \brief          Has a particular call been worked at all?
    \param  call    target callsign
    \return         whether <i>call</i> has been worked
*/
  inline bool qso_b4(const std::string& call) const
    { SAFELOCK(_log);

      return (_log.lower_bound(call) != _log.upper_bound(call)); 
    }
    
/*! \brief          Has a call been worked on a particular band?
    \param  call    target callsign
    \param  b       target band
    \return         whether <i>call</i> has been worked on <i>b</i>
*/
  bool qso_b4(const std::string& call, const BAND b) const;

/*! \brief          Has a call been worked on a particular mode?
    \param  call    target callsign
    \param  m       target mode
    \return         whether <i>call</i> has been worked on <i>m</i>
*/
  bool qso_b4(const std::string& call, const MODE m) const;

/*! \brief          Has a call been worked on a particular band and mode?
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         whether <i>call</i> has been worked on <i>b</i> and <i>m</i>
*/
  bool qso_b4(const std::string& call, const BAND b, const MODE m) const;
  
/*! \brief          Get a string list of bands on which a call is needed
    \param  call    target callsign
    \param  rules   rules for the contest
    \return         string list of bands on which a call is needed (separated by three spaces)
*/
  std::string call_needed(const std::string& call, const contest_rules& rules) const;
  
/*! \brief          Would a QSO be a dupe, according to the rules?
    \param  qso     target QSO
    \param  rules   rules for the contest
    \return         whether <i>qso</i> would be a dupe
*/
  bool is_dupe(const QSO& qso, const contest_rules& rules) const;

/*! \brief          Would a QSO be a dupe, according to the rules?
    \param  call    target call
    \param  b       target band
    \param  m       target mode
    \param  rules   rules for the contest
    \return         whether a QSO with <i>call</i> on band <i>b</i> and mode <i>m</i> would be a dupe
*/
  bool is_dupe(const std::string& call, const BAND b, const MODE m, const contest_rules& rules) const;

/// return time-ordered container of QSOs
  std::list<QSO> as_list(void) const;

/// return time-ordered container of QSOs
  std::vector<QSO> as_vector(void) const;
  
/*! \brief          Return the QSOs, filtered by some criterion
    \param  pred    predicate to apply
    \return         the QSOs for which <i>pred</i> is true

    The returned QSOs are in chronological order
*/
  template <class UnaryPredicate>
  std::vector<QSO> filter(UnaryPredicate pred) const
  { std::vector<QSO> rv;

    SAFELOCK(_log);

    copy_if(_log_vec.cbegin(), _log_vec.cend(), back_inserter(rv), pred);

    return rv;
  }
  
/*! \brief          Recalculate the dupes
    \param  rules   rules for the contest
    \return         logbook with the dupes recalculated
*/
  logbook recalculate_dupes(const contest_rules& rules) const;
  
/*! \brief              Generate a Cabrillo log
    \param  context     the drlog context
    \param  score       score to be claimed
    \return             the Cabrillo log
*/
  std::string cabrillo_log(const drlog_context& context, const unsigned int score) const;
  
/*! \brief                          Read from a Cabrillo file
    \param  filename                name of Cabrillo file
    \param  cabrillo_qso_template   template for the Cabrillo QSOs
*/
  void read_cabrillo(const std::string& filename, const std::string& cabrillo_qso_template);

/*! \brief                      Read from a Cabrillo file, using space-delimited fields
    \param  filename            name of Cabrillo file
    \param  cabrillo_fields     names of Cabrillo fields
*/
  void read_cabrillo(const std::string& filename, const std::vector<std::string>& cabrillo_fields);

/*! \brief              Read from a TRLOG file
    \param  filename    name of TRLOG file
*/
  void read_trlog_log(const std::string& filename);
  
/// clear the logbook
  inline void clear(void)
    { SAFELOCK(_log);
      _log.clear();
    }

/// how many QSOs are in the log?
  inline size_t size(void) const
    { SAFELOCK(_log);
      return _log.size();
    }

/// how many QSOs are in the log?
  inline size_t n_qsos(void) const
    { return size(); }

/// is the log empty?
  inline bool empty(void) const
    { SAFELOCK(_log);
      return _log.empty();
    }

/*! \brief                          Get the value of an exchange field from the most recent QSO with a station
    \param  callsign                call for which the information is required
    \param  exchange_field_name     name of the exchange field for which the information is required
    \return                         value received from <i>callsign</i> for the field <i>exchange_field_name</i> during the most recent QSO with <i>callsign</i>

    Returns empty string if anything goes wrong.
*/
  std::string exchange_field_value(const std::string& callsign, const std::string& exchange_field_name);

/*! \brief          Return all the QSOs that contain an exchange field that matches a target
    \param  target  target string for exchange fields
    \return         all the QSOs that contain an exchange field that matches a target
*/
  std::vector<QSO> match_exchange(const std::string& target) const;

/*! \brief          Return all the calls in the log
    \return         all the calls in the log
*/
  std::set<std::string> calls(void) const;

/// serialise logbook
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { SAFELOCK(_log);

    ar & _log
       & _log_vec;
  }
};

// -----------  log_extract  ----------------

/*! \class  log_extract
    \brief  Support for parts of the log
*/

class log_extract
{
protected:

  window& _win;                                 ///< window associated with the log extract  (NB, during construction, _win will be constructed first)
  size_t  _win_size { 0 };                      ///< height of the associated window

  std::deque<QSO> _qsos;                        ///< QSOs contained in the extract

  pt_mutex _extract_mutex { "LOG EXTRACT"s };   ///< mutex for thread safety

public:

/*! \brief  constructor
    \param  w               window to be used by this extract
*/
  explicit inline log_extract(window& w) :
    _win(w)
  { }                                       // don't set the size yet, since the size of w may not be set

/// prepare for use; this MUST be called before the object is used
  inline void prepare(void)
    { _win_size = _win.height(); }

/// number of QSOs in the extract
  inline size_t size(void)
  { SAFELOCK(_extract);
    return _qsos.size();
  }

/// is the extract empty?
  inline bool empty(void)
  { SAFELOCK(_extract);
    return _qsos.empty();
  }

/// height of the associated window
  inline unsigned int win_size(void)
  { SAFELOCK(_extract);
    return _win_size;
  }

/// QSOs contained in the extract
  inline std::deque<QSO> qsos(void)
  { SAFELOCK(_extract);
    return _qsos;
  }

/*! \brief          Add a QSO to the extract
    \param  qso     QSO to add

    Auto resizes the extract by removing old QSOs so that it does not exceed the window size
*/
  void operator+=(const QSO& qso);

/*! \brief          Unconditionally add a QSO to the extract
    \param  qso     QSO to add
*/
  inline void add_without_limit(const QSO& qso)
  { SAFELOCK(_extract);
    _qsos += qso;
  }

/// clear the extract
  inline void clear(void)
  { SAFELOCK(_extract);
    _qsos.clear();
  }

/*! \brief  Display the extract in the associated window

    Displayed in order from oldest to newest. If the extract contains more QSOs than the window
    allows, only the most recent QSOs are displayed.
*/
  void display(void);

/*! \brief              Get recent QSOs from a log, and possibly display them
    \param  lgbook      logbook to use
    \param  to_display  whether to display the extract

    Displayed in order from oldest to newest
*/
  void recent_qsos(const logbook& lgbook, const bool to_display = true);

/*! \brief          Display the QSOs from a log that match an exchange
    \param  lgbook  logbook to use
    \param  target  string to match in the QSO exchanges

    Displayed in order from oldest to newest. If the extract contains more QSOs than the window
    allows, only the most recent QSOs are displayed.
*/
  void match_exchange(const logbook& lgbook, const std::string& target);

/// log_extract = <i>container of QSOs</i>
template <typename C>
  void operator=(const C& t)
  requires (std::is_same_v<typename C::value_type, QSO>)
  { SAFELOCK(_extract);
    _qsos.clear();
    copy(t.cbegin(), t.cend(), back_inserter(_qsos));
  }
};

// -----------  old_log  ----------------

/*! \class old_log
    \brief An old ADIF3 log

    Not thread safe.
*/

class old_log
{
protected:

  std::unordered_map<std::string /* callsign */,
                     std::tuple< unsigned int /* qsls */,
                                 unsigned int /* qsos */,
                                 std::set< std::pair< BAND, MODE > >,     /* set of band/mode from which QSLs have been received */
                                 std::multiset< std::pair< BAND, MODE > > /* QSOs per band/mode */
                               >
                    > _olog;    ///< ADIF3 log of old QSOs (used for QSLs)
public:

/*! \brief          Return total number of QSLs from a particular callsign
    \param  call    callsign
    \return         the number of QSLs from callsign <i>call</i>
*/
  unsigned int n_qsls(const std::string& call) const;

/*! \brief          Set the number of QSLs from a particular callsign
    \param  call    callsign
    \param  n       number of QSLs from <i>call</i>
*/
  void n_qsls(const std::string& call, const unsigned int n);

/*! \brief          Increment the number of QSLs from a particular callsign
    \param  call    callsign
    \return         the new number of QSLs from callsign <i>call</i>
*/
  unsigned int increment_n_qsls(const std::string& call);

/*! \brief          Return total number of QSOs with a particular callsign
    \param  call    callsign
    \return         the number of QSOs with callsign <i>call</i>
*/
  unsigned int n_qsos(const std::string& call) const;

/*! \brief          Set the number of QSOs with a particular callsign
    \param  call    callsign
    \param  n       number of QSOs with <i>call</i>
*/
  void n_qsos(const std::string& call, const unsigned int n);

/*! \brief          Increment the number of QSOs associated with a particular callsign
    \param  call    callsign for which the number of QSOs should be incremented
    \return         number of QSOs associated with <i>call</i>
*/
  unsigned int increment_n_qsos(const std::string& call);

/*! \brief          How many QSOs have taken place with a particular call on a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         number of QSOs associated with <i>call</i> on band <i>b</i> and mode <i>m</i>
*/
  unsigned int n_qsos(const std::string& call, const BAND b, const MODE m) const;

/*! \brief          Increment the number of QSOs associated with a particular callsign, band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         number of QSOs associated with <i>call</i> on band <i>b</i> and mode <i>m</i> (following the increment)
*/
  unsigned int increment_n_qsos(const std::string& call, const BAND b, const MODE m);

/*! \brief          Has a QSL ever been received for a particular call on a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         Has a QSL ever been received for a QSO with <i>call</i> on band <i>b</i> and mode <i>m</i>
*/
  bool confirmed(const std::string& call, const BAND b, const MODE m) const;

/*! \brief          Mark a QSL as being received for a particular call on a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
*/
  void qsl_received(const std::string& call, const BAND b, const MODE m);
};

#endif    // LOG_H
