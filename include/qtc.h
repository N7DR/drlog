// $Id: qtc.h 71 2014-08-10 22:56:10Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef QTC_H
#define QTC_H

/*!     \file qtc.h

        Classes and functions related to WAE QTCs
*/

#include "log.h"
#include "macros.h"
#include "qso.h"
#include "serialization.h"
#include "x_error.h"

#include <list>
#include <set>
#include <string>
#include <vector>

// error numbers
const int QTC_INVALID_FORMAT            = -1;    // error reading from file

const bool QTC_SENT = true,
           QTC_UNSENT = false;

// from http://www.kkn.net/~trey/cabrillo/qso-template.html:
//
//                             -qtc rcvd by - --------------qtc info received-----------------
//QTC: freq  mo date       time call          qserial    qtc sent by   qtim qcall         qexc
//QTC: ***** ** yyyy-mm-dd nnnn ************* nnn/nn     ************* nnnn ************* nnnn
//QTC:  3799 PH 2003-03-23 0711 YB1AQS        001/10     DL8WPX        0330 DL6RAI        1021

// -----------------------------------  qtc_entry  ----------------------------

/*!     \class qtc_entry
        \brief An entry in a QTC
*/

class qtc_entry
{
protected:

  std::string _utc;          ///< time of QSO: HHMM
  std::string _callsign;     ///< other station
  std::string _serno;        ///< serial number sent by other station; width = 4

public:

/// default constructor
  qtc_entry(void);

/// construct from a QSO
  explicit qtc_entry(const QSO& qso);

  READ_AND_WRITE(utc);          ///< time of QSO: HHMM
  READ_AND_WRITE(callsign);     ///< other station
  READ(serno);                  ///< serial number sent by other station; width = 4

/// set the serial number
  inline void serno(const std::string& str)
    { _serno = pad_string(str, 4, PAD_RIGHT); }

/// qtc_entry == qso
  const bool operator==(const QSO& qso) const;

/// qtc_entry != qso
  inline const bool operator!=(const QSO& qso) const
    { return !(*this == qso); }

/// qtc_entry == qtc_entry
  const bool operator==(const qtc_entry& entry) const;

/// qtc_entry < qtc_entry
  const bool operator<(const qtc_entry&) const;

/// convert to printable string
  const std::string to_string(void) const;

/// return the length of the printable string
  inline const size_t size(void) const
    { return to_string().size(); }

/// does a qtc_entry contain a non-empty call?
  inline const bool empty(void) const
    { return _callsign.empty(); }

/// is a qtc_entry valid?
  inline const bool valid(void) const
    { return !empty(); }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _utc
         & _callsign
         & _serno;
    }
};

// -----------------------------------  qtc_series  ----------------------------

/*!     \class qtc_series
        \brief A QTC series as defined by the WAE rules
*/

class qtc_series
{
protected:

  std::vector<std::pair<qtc_entry, bool>> _qtc_entries;    ///< the individual QTC entries, and whether each has been sent

  std::string _target;             ///< to whom is the QTC series to be sent?
  std::string _id;                 ///< QTC ID (e.g., "1/10")

  std::string  _date;              ///< yyyy-mm-dd
  std::string  _utc;               ///< hh:mm:ss
  std::string  _frequency;         ///< frequency in form xxxxx.y (kHz)
  std::string  _mode;              ///< CW or PH
  std::string  _source;            ///< my call

public:

  qtc_series(void)
    { }

  qtc_series(const std::vector<qtc_entry>& vec_qe, const std::string& mode_str, const std::string& my_call, const bool b = QTC_UNSENT) :
    _mode(mode_str),
    _source(my_call)
//    { for_each(vec_qe.cbegin(), vec_qe.cend(), [&] (const qtc_entry& qe) { (*this) += qe; } ); }
    { FOR_ALL(vec_qe, [&] (const qtc_entry& qe) { (*this) += std::pair<qtc_entry, bool>( { qe, b } ); } ); }

  READ_AND_WRITE(target);
  READ_AND_WRITE(id);
  READ_AND_WRITE(qtc_entries);
  READ_AND_WRITE(date);
  READ_AND_WRITE(utc);
  READ_AND_WRITE(mode);
  READ_AND_WRITE(source);

  inline const decltype(_target) destination(void) const
    { return target(); }

  inline void destination(const decltype(_target) tgt)
    { target(tgt); }

  inline void clear(void)
    { *this = qtc_series(); }

  const std::vector<qtc_entry> sent_qtc_entries(void) const;

  const std::vector<qtc_entry> unsent_qtc_entries(void) const;

  inline const std::string frequency_str(void) const
    { return _frequency; }

  inline void frequency_str(const std::string& s)
    { _frequency = s; }

  inline void frequency_str(const frequency& f)
    { _frequency = f.display_string(); }

  inline const size_t size(void) const
    { return _qtc_entries.size(); }

  inline const bool empty(void) const
    { return _qtc_entries.empty(); }

//  const bool operator+=(const qtc_entry& entry);
  const bool operator+=(const std::pair<qtc_entry, bool>&);

  inline std::pair<qtc_entry, bool>& operator[](const unsigned int n)
    { return _qtc_entries[n]; }

  const std::string to_string(const unsigned int n_rows) const;

// set a particular entry to sent
  void mark_as_sent(const unsigned int n);

// set a particular entry to unsent
  void mark_as_unsent(const unsigned int n);

// get first entry that has not been sent
  const qtc_entry first_not_sent(const unsigned int posn = 0);

  const std::string output_string(const unsigned int n) const;

  const std::string complete_output_string(void) const;

  const unsigned int n_sent(void) const;

  const unsigned int n_unsent(void) const;

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _qtc_entries
         & _target
         & _id;
    }
};

/// window < qtc
window& operator<(window& win, const qtc_series& qs);

// -----------------------------------  qtc_database  ----------------------------

/*!     \class qtc_database
        \brief All QTCs
*/

extern pt_mutex qtc_database_mutex;

class qtc_database
{
protected:

  std::vector<qtc_series>    _qtc_db;    ///< the QTCs

public:

  qtc_database(void)
    { }

// read from file
  qtc_database(const std::string& filename);

  SAFEREAD(qtc_db, qtc_database);

  void operator+=(const qtc_series& q);

  inline const size_t n_qtcs(void) const
    { SAFELOCK(qtc_database);

      return _qtc_db.size();
    }

  inline const size_t size(void) const
    { return n_qtcs(); }

  inline const qtc_series operator[](size_t n)
    { SAFELOCK(qtc_database);

      return _qtc_db.at(n);
    }

  const unsigned int n_qtcs_sent_to(const std::string& destination_callsign) const;

  const unsigned int n_qtc_entries_sent(void) const;

// write to file
//  void write(const std::string& filename);

// read from file
  void read(const std::string& filename);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _qtc_db;
    }
};

// -----------------------------------  qtc_buffer  ----------------------------

/*!     \class pending_qtcs
        \brief Buffer to handle process of moving QTCs from unsent to sent
*/

class qtc_buffer
{
protected:

  std::list<qtc_entry> _unsent_qtcs;    ///< the unsent QTCs

  std::set<qtc_entry> _sent_qtcs;       ///< unordered_set won't serialize

public:

  const std::vector<qtc_entry> get_next_unsent_qtc(const std::string& target, const unsigned int max_entries = 10);

  void operator+=(const logbook&);

  void operator+=(const QSO&);

  void operator-=(const qtc_entry& entry);

  void unsent_to_sent(const qtc_entry& entry);

  inline void unsent_to_sent(const std::vector<qtc_entry>& entries)
    { for_each(entries.cbegin(), entries.cend(), [this] (const qtc_entry& entry) { unsent_to_sent(entry); }); }

  void unsent_to_sent(const qtc_series& qs);

  inline const unsigned int n_sent_qsos(void) const
    { return _sent_qtcs.size(); }

  inline const unsigned int n_unsent_qsos(void) const
    { return _unsent_qtcs.size(); }

  inline const unsigned int size(void) const
    { return n_sent_qsos() + n_unsent_qsos(); }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _unsent_qtcs
         & _sent_qtcs;
    }
};

// -------------------------------------- Errors  -----------------------------------

/*! \class  qtc_error
    \brief  Errors related to QTC processing
*/

class qtc_error : public x_error
{
protected:

public:

/*! \brief  Construct from error code and reason
  \param  n Error code
  \param  s Reason
*/
  inline qtc_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

#endif /* QTC_H */
