// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file qtc.cpp

        Classes and functions related to WAE QTCs
*/

#include "qtc.h"

#include <algorithm>

using namespace std;

// -----------------------------------  qtc_entry  ----------------------------

/*!     \class qtc_entry
        \brief An entry in a QTC
*/

qtc_entry::qtc_entry(const QSO& qso) :
  _utc(qso.utc()),
  _callsign(qso.callsign()),
  _serno(qso.received_exchange("SERNO"))
{
}

const bool qtc_entry::operator==(const QSO& qso) const
{ const qtc_entry target(qso);

  return (*this == target);
}

const bool qtc_entry::operator==(const qtc_entry& entry) const
{ if (_serno != entry._serno)
    return false;

  if (_utc != entry._utc)
    return false;

  if (_callsign != entry._callsign)
    return false;

  return true;
}

const bool qtc_entry::operator<(const qtc_entry& entry) const
{ if (_serno < entry._serno)
    return true;

  if (_utc < entry._utc)
    return true;

  if (_callsign < entry._callsign)
    return false;

  return false;
}

// -----------------------------------  qtc  ----------------------------

/*!     \class qtc
        \brief A QTC
*/

const bool qtc::operator+=(const qtc_entry& entry)
{ if (entry.callsign() != _target)
  { _qtcs.push_back(entry);
    return true;
  }

  return false;
}

// -----------------------------------  qtc_database  ----------------------------

/*!     \class qtc_database
        \brief All QTCs
*/

pt_mutex qtc_database_mutex;

void qtc_database::operator+=(const qtc& q)
{ qtc q_copy = q;

  SAFELOCK(qtc_database);

  if (q_copy.id().empty())
  { const string id = to_string(_qtc_db.size() + 1) + "/" + to_string(q.size());

    q_copy.id(id);
  }

  _qtc_db.push_back(q_copy);
}

const unsigned int qtc_database::n_qtcs_sent_to(const string& destination_callsign) const
{ unsigned int rv = 0;

  SAFELOCK(qtc_database);

  for_each(_qtc_db.cbegin(), _qtc_db.cend(), [=, &rv] (const qtc& QTC) { if (QTC.target() == destination_callsign) rv += QTC.size(); } );

  return rv;
}

// -----------------------------------  qtc_buffer  ----------------------------

/*!     \class qtc_buffer
        \brief Buffer to handle process of moving QTCs from unsent to sent
*/

void qtc_buffer::operator+=(const logbook& logbk)
{ const vector<QSO> qsos = logbk.as_vector();

  for (const auto& qso : qsos)
  { const qtc_entry entry(qso);

    if (_sent_qtcs.find(entry) == _sent_qtcs.end())
    { if (find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry) == _unsent_qtcs.end())
        _unsent_qtcs.push_back(entry);
    }
  }
}

const vector<qtc_entry> qtc_buffer::get_next_unsent_qtc(const string& target, const int max_entries)
{ vector<qtc_entry> rv;

  list<qtc_entry>::const_iterator cit = _unsent_qtcs.cbegin();

  while ( (rv.size() != max_entries) and (cit != _unsent_qtcs.cend()) )
  { rv.push_back(*cit);
    ++cit;
  }

  return rv;
}

void qtc_buffer::unsent_to_sent(const qtc_entry& entry)
{ const auto it = find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry);

  if (it != _unsent_qtcs.end())
    _unsent_qtcs.erase(it);

   _sent_qtcs.insert(entry);
}
