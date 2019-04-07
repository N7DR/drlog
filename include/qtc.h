// $Id: qtc.h 150 2019-04-05 16:09:55Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef QTC_H
#define QTC_H

/*! \file   qtc.h

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

using namespace std::literals::string_literals;

// error numbers
constexpr int QTC_INVALID_FORMAT            { -1 };   ///< error reading from file

constexpr bool QTC_SENT   { true };                     ///< QTC has been sent
constexpr bool QTC_UNSENT { false };                  ///< QTC has not been sent

// from http://www.kkn.net/~trey/cabrillo/qso-template.html:
//
//                             -qtc rcvd by - --------------qtc info received-----------------
//QTC: freq  mo date       time call          qserial    qtc sent by   qtim qcall         qexc
//QTC: ***** ** yyyy-mm-dd nnnn ************* nnn/nn     ************* nnnn ************* nnnn
//QTC:  3799 PH 2003-03-23 0711 YB1AQS        001/10     DL8WPX        0330 DL6RAI        1021

// -----------------------------------  qtc_entry  ----------------------------

/*! \class  qtc_entry
    \brief  An entry in a QTC
*/

class qtc_entry
{
protected:

  std::string _utc;          ///< time of QSO: HHMM
  std::string _callsign;     ///< other station
  std::string _serno;        ///< serial number sent by other station; width = 4

public:

/// default constructor
  inline qtc_entry(void) :
    _utc("0000"s),
    _callsign(),
    _serno("0000"s)
  { }

/// construct from a QSO
  inline explicit qtc_entry(const QSO& qso) :
    _utc(substring(qso.utc(), 0, 2) + substring(qso.utc(), 3, 2)),
    _callsign( (qso.continent() == "EU"s) ? qso.callsign() : std::string()),
    _serno(pad_string(qso.received_exchange("SERNO"s), 4, PAD_RIGHT))    // force width to 4
  { }

  READ_AND_WRITE(utc);          ///< time of QSO: HHMM
  READ_AND_WRITE(callsign);     ///< other station
  READ(serno);                  ///< serial number sent by other station; width = 4

/*! \brief          Explicitly set the serial number sent by the other station
    \param  str     new serial number
*/
  inline void serno(const std::string& str)
    { _serno = pad_string(str, 4, PAD_RIGHT); }

/// qtc_entry == qso
  inline const bool operator==(const QSO& qso) const
    { return ( *this == qtc_entry { qso } ); }

/// qtc_entry != qso
  inline const bool operator!=(const QSO& qso) const
    { return !(*this == qso); }

/// qtc_entry == qtc_entry
  inline const bool operator==(const qtc_entry& entry) const
    { return ( (_serno == entry._serno) and (_utc == entry._utc) and (_callsign == entry._callsign) ); }

/// qtc_entry < qtc_entry
  inline const bool operator<(const qtc_entry& entry) const
    { return ( (_serno < entry._serno) or (_utc < entry._utc) or (_callsign < entry._callsign) ); }

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

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _utc
         & _callsign
         & _serno;
    }
};

// -----------------------------------  qtc_series  ----------------------------

/*! \class  qtc_series
    \brief  A QTC series as defined by the WAE rules
*/

class qtc_series
{
protected:

  std::vector<std::pair<qtc_entry, bool>> _qtc_entries;    ///< the individual QTC entries, and whether each has been sent

  std::string _target;              ///< to whom is the QTC series to be sent?
  std::string _id;                  ///< QTC ID (e.g., "1/10")

  std::string _date;                ///< yyyy-mm-dd
  std::string _utc;                 ///< hh:mm:ss
  std::string _frequency;           ///< frequency in form xxxxx.y (kHz)
  std::string _mode;                ///< CW or PH
  std::string _source;              ///< my call

/*! \brief          Get all the entries that either have been sent or have not been sent
    \param  sent    whether to return the sent entries
    \return         if <i>sent</i> is <i>true</i>, return all the sent entries; otherwise return all the unsent entries
*/
  const std::vector<qtc_entry> _sent_or_unsent_qtc_entries(const bool sent) const;

public:

/// default constructor
  qtc_series(void) = default;

/*! \brief              Construct from a vector of qtc_entry
    \param  vec_qe      the vector of type <i>/qtc_entry</i>
    \param  mode_str    mode
    \param  my_call     my call
    \param  b           whether the QTC has been sent
*/
  qtc_series(const std::vector<qtc_entry>& vec_qe, const std::string& mode_str, const std::string& my_call, const bool b = QTC_UNSENT) :
    _mode(mode_str),
    _source(my_call)
    { FOR_ALL(vec_qe, [&] (const qtc_entry& qe) { (*this) += std::pair<qtc_entry, bool>( { qe, b } ); } ); }

  READ_AND_WRITE(target);             ///< to whom is the QTC series to be sent?
  READ_AND_WRITE(id);                 ///< QTC ID (e.g., "1/10")
  READ_AND_WRITE(qtc_entries);        ///< the individual QTC entries, and whether each has been sent
  READ_AND_WRITE(date);               ///< yyyy-mm-dd
  READ_AND_WRITE(utc);                ///< hh:mm:ss
  READ_AND_WRITE(mode);               ///< CW or PH
  READ_AND_WRITE(source);             ///< my call

/// synonym for getting target
  inline const decltype(_target) destination(void) const
    { return target(); }

/// synonym for setting target
  inline void destination(const decltype(_target) tgt)
    { target(tgt); }

/// reset to default-constructed state
  inline void clear(void)
    { *this = qtc_series(); }

/// return all the sent QTCs
  inline const std::vector<qtc_entry> sent_qtc_entries(void) const
    { return _sent_or_unsent_qtc_entries(true); }

/// return all the unsent QTCs
  inline const std::vector<qtc_entry> unsent_qtc_entries(void) const
    { return _sent_or_unsent_qtc_entries(false); }

/// return frequency in form xxxxx.y (kHz)
  inline const std::string frequency_str(void) const
    { return _frequency; }

/// set frequency in form xxxxx.y (kHz)
  inline void frequency_str(const std::string& s)
    { _frequency = s; }

/// set frequency from a frequency object
  inline void frequency_str(const frequency& f)
    { _frequency = f.display_string(); }

/// return the number of qtc_entries in the series
  inline const size_t size(void) const
    { return _qtc_entries.size(); }

/// are there zero qtc_entries in the series?
  inline const bool empty(void) const
    { return _qtc_entries.empty(); }

/*! \brief          Add a qtc_entry
    \param  param   entry to add, and whether the entry has been sent
    \return         whether <i>param</i> was actually added
*/
  const bool operator+=(const std::pair<qtc_entry, bool>& param);

/*! \brief      Return an entry
    \param  n   index number to return (wrt 0)
    \return     <i>n</i>th entry

    Returns empty pair if <i>n</i> is out of bounds.
*/
  const std::pair<qtc_entry, bool> operator[](const unsigned int n) const;

/*! \brief      Mark a particular entry as having been sent
    \param  n   index number to mark (wrt 0)

    Does nothing if entry number <i>n</i> does not exist
*/
  void mark_as_sent(const unsigned int n);

/*! \brief      Mark a particular entry as having NOT been sent
    \param  n   index number to mark (wrt 0)

    Does nothing if entry number <i>n</i> does not exist
*/
  void mark_as_unsent(const unsigned int n);

/*! \brief          Get first <i>qtc_entry</i> beyond a certain point that has not been sent
    \param  posn    index number (wrt 0) at which to start searching
    \return         first <i>qtc_entry</i> at or later than <i>posn</i> that has not been sent

    Returns an empty <i>qtc_entry</i> if no entries meet the criteria
*/
  const qtc_entry first_not_sent(const unsigned int posn = 0);

/*! \brief      Get a string representing a particular entry
    \param  n   number of the entry (wrt 0)
    \return     string describing <i>qtc_entry</i> number <i>n</i>

    Returns the empty string if entry number <i>n</i> does not exist
*/
  const std::string output_string(const unsigned int n) const;

/*! \brief      Get a string representing all the entries
    \return     string describing all the <i>qtc_entry</i> elements
*/
  const std::string complete_output_string(void) const;

/*! \brief      How many entries have been sent?
    \return     the number of entries that have been sent
*/
  const unsigned int n_sent(void) const;

/*! \brief      How many entries have not been sent?
    \return     the number of entries that have not been sent
*/
  const unsigned int n_unsent(void) const;

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _qtc_entries
         & _target
         & _id;
    }
};

/*! \brief          Write a <i>qtc_series</i> object to a window
    \param  win     destination window
    \param  qs      object to write
    \return         the window
*/
window& operator<(window& win, const qtc_series& qs);

// -----------------------------------  qtc_database  ----------------------------

/*! \class  qtc_database
    \brief  All QTCs
*/

extern pt_mutex qtc_database_mutex;         ///< the mutex to control access to the database

class qtc_database
{
protected:

  std::vector<qtc_series>    _qtc_db;       ///< the QTCs

public:

/// default constructor
  qtc_database(void) = default;

/*! \brief              Read from a file
    \param  filename    name of file that contains the database
*/
  explicit qtc_database(const std::string& filename);

  SAFEREAD(qtc_db, qtc_database);           ///< the QTCs

/*! \brief      Add a series of QTCs to the database
    \param  q   the series of QTCs to be added
*/
  void operator+=(const qtc_series& q);

/*! \brief      Get the number of QTCs in the database
    \return     the number of QTCs in the database
*/
  inline const size_t n_qtcs(void) const
    { SAFELOCK(qtc_database);

      return _qtc_db.size();
    }

/*! \brief      Get the number of QTCs in the database
    \return     the number of QTCs in the database

    Synonym for n_qtcs()
*/
  inline const size_t size(void) const
    { return n_qtcs(); }

/*! \brief      Get one of the series in the database
    \param  n   number of the series to return (wrt 0)
    \return     series number <i>n</i>

    Returns an empty series if <i>n</i> is out of bounds
*/
const qtc_series operator[](size_t n);

/*! \brief                          Get the number of QTCs that have been sent to a particular station
    \param   destination_callsign   the station to which the QTCs have been sent
    \return                         number of QTCs that have been sent to <i>destination_callsign</i>
*/
  const unsigned int n_qtcs_sent_to(const std::string& destination_callsign) const;

/*! \brief                          Get the total number of QTC entries that have been sent
    \return                         the number of QTC entries that have been sent
*/
  const unsigned int n_qtc_entries_sent(void) const;

// read from file
  void read(const std::string& filename);

/// serialise
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

/*! \brief                  Get a batch of QTC entries that may be sent to a particular destination
    \param  max_entries     maximum number of QTC entries to return
    \param  target          station to which the QTC entries are to be sent
    \return                 the sendable QTC entries
*/
  const std::vector<qtc_entry> get_next_unsent_qtc(const unsigned int max_entries = 10, const std::string& target = std::string());

/*! \brief          Add all unsent QSOs from a logbook to the buffer
    \param  logbk   logbook

    Does not add QSOs already in the buffer (either as sent or unsent).
    Does not add non-EU QSOs.
*/
  void operator+=(const logbook&);

/*! \brief          Add a QSO to the buffer
    \param  qso     QSO to add

    Does nothing if <i>qso</i> is already in the buffer (either as sent or unsent).
    Does nothing if the QSO is not with an EU station.
*/
  void operator+=(const QSO&);

/*! \brief          Remove a QTC if present in the unsent list
    \param  entry   QTC to remove
*/
  inline void operator-=(const qtc_entry& entry)
    { _unsent_qtcs.remove(entry); }

/*! \brief          Transfer a <i>qtc_entry</i> from unsent status to sent status
    \param  entry   <i>qtc_entry</i> to transfer
*/
  void unsent_to_sent(const qtc_entry& entry);

/*! \brief              Transfer a vector of <i>qtc_entry</i> objects from unsent status to sent status
    \param  entries     QTC entries to transfer
*/
  inline void unsent_to_sent(const std::vector<qtc_entry>& entries)
    { FOR_ALL(entries, [this] (const qtc_entry& entry) { unsent_to_sent(entry); }); }

/*! \brief      Transfer all the entries in a <i>qtc_series</i> from unsent status to sent status
    \param  qs  QTC entries to transfer
*/
  void unsent_to_sent(const qtc_series& qs);

/*! \brief      How many QTC QSOs have been sent?
    \return     the number of QSOs that have been sent in QTCs
*/
  inline const unsigned int n_sent_qsos(void) const
    { return _sent_qtcs.size(); }

/*! \brief      How many unsent QTC QSOs are there?
    \return     the number of QSOs that have not yet been sent in QTCs
*/
  inline const unsigned int n_unsent_qsos(void) const
    { return _unsent_qtcs.size(); }

/*! \brief      How large is the database?
    \return     the total number of QSOs, both sent and unsent, in the database
*/
  inline const unsigned int size(void) const
    { return ( n_sent_qsos() + n_unsent_qsos() ); }

/*! \brief          Recreate the unsent list
    \param  logbk   logbook
*/
  void rebuild(const logbook& logbk);

/// serialise
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
