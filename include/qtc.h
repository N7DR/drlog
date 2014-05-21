// $Id: qtc.h 63 2014-05-20 16:48:18Z  $

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

#include <list>
#include <set>
#include <string>
//#include <unordered_set>
#include <vector>

// -----------------------------------  qtc_entry  ----------------------------

/*!     \class qtc_entry
        \brief An entry in a QTC
*/

class qtc_entry
{
protected:

  std::string _utc;          ///< time of QSO
  std::string _callsign;     ///< other station
  std::string _serno;        ///< serial number sent by other station

public:

  explicit qtc_entry(const QSO& qso);

  READ(utc);
  READ(callsign);
  READ(serno);

  const bool operator==(const QSO& qso) const;

  inline const bool operator!=(const QSO& qso) const
    { return !(*this == qso); }

  const bool operator==(const qtc_entry& entry) const;

  const bool operator<(const qtc_entry&) const;

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _utc
         & _callsign
         & _serno;
    }

};

// -----------------------------------  qtc  ----------------------------

/*!     \class qtc
        \brief A QTC
*/

class qtc
{
protected:

  std::vector<qtc_entry> _qtcs;    ///< the QTCs

  std::string _target;             ///< to whom is the QTC to be sent?
  std::string _id;                 ///< QTC ID (e.g., "1/10")

public:

  READ(target);
  READ_AND_WRITE(id);

  inline const size_t size(void) const
    { return _qtcs.size(); }

  const bool operator+=(const qtc_entry& entry);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _qtcs
         & _target;
    }

};

// -----------------------------------  qtc_database  ----------------------------

/*!     \class qtc_database
        \brief All QTCs
*/

extern pt_mutex qtc_database_mutex;

class qtc_database
{
protected:

  std::vector<qtc>    _qtc_db;    ///< the QTCs

public:

  void operator+=(const qtc& q);

  inline const size_t n_qtcs(void) const
    { SAFELOCK(qtc_database);

      return _qtc_db.size();
    }

  inline const size_t size(void) const
    { return n_qtcs(); }

  inline const qtc operator[](size_t n)
    { SAFELOCK(qtc_database);

      return _qtc_db.at(n);
    }

  const unsigned int n_qtcs_sent_to(const std::string& destination_callsign) const;

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

  inline const size_t n_unsent(void) const
    { return _unsent_qtcs.size(); }

  const std::vector<qtc_entry> get_next_unsent_qtc(const std::string& target, const int max_entries = 10);

  void operator+=(const logbook& logbk);

  void operator-=(const qtc_entry& entry);

  void unsent_to_sent(const qtc_entry& entry);

  inline void unsent_to_sent(const std::vector<qtc_entry>& entries)
    { for_each(entries.cbegin(), entries.cend(), [this] (const qtc_entry& entry) { unsent_to_sent(entry); }); }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _unsent_qtcs
         & _sent_qtcs;
    }
};


#endif /* QTC_H */
