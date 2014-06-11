// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file qtc.cpp

        Classes and functions related to WAE QTCs
*/

#include "diskfile.h"
#include "qtc.h"

#include <algorithm>

using namespace std;

// -----------------------------------  qtc_entry  ----------------------------

/*!     \class qtc_entry
        \brief An entry in a QTC
*/

qtc_entry::qtc_entry(void) :
  _utc("0000"),
  _callsign(),
  _serno("0000")
{
}

qtc_entry::qtc_entry(const QSO& qso) :
  _utc(substring(qso.utc(), 0, 2) + substring(qso.utc(), 3, 2)),
  _callsign( (qso.continent() == "EU") ? qso.callsign() : string()),
  _serno(pad_string(qso.received_exchange("SERNO"), 4, PAD_RIGHT))
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

const string qtc_entry::to_string(void) const
{ static const unsigned int CALL_WIDTH = 12;
  static const string SPACE(" ");
  string rv;

  rv = _utc + SPACE + pad_string(_callsign, CALL_WIDTH, PAD_RIGHT) + SPACE + _serno;

  return rv;
}

// -----------------------------------  qtc  ----------------------------

/*!     \class qtc
        \brief A QTC
*/

const bool qtc::operator+=(const qtc_entry& entry)
{ if (entry.valid() and (entry.callsign() != _target))
  { _qtc_entries.push_back( { entry, false });
    return true;
  }

  return false;
}

// set a particular entry to sent
void qtc::mark_as_sent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = true;
}

// get first entry that has not been sent
const qtc_entry qtc::first_not_sent(const unsigned int posn)
{ unsigned int index = posn;

  while (index < _qtc_entries.size())
  { if (!_qtc_entries[index].second)
      return _qtc_entries[index].first;

    index++;
  }

  return qtc_entry();
}

#include <array>

const string qtc::to_string(const unsigned int n_rows) const
{
#if 0
  unsigned int column = 1;
  string rv;
  unsigned int index = 0;
  const size_t n_entries = _qtc_entries.size();
  bool finished = (index == n_entries);
  vector<vector<string>> rv_columns;

  while (!finished)
  { vector<string> rv_column;

    unsigned int row = 0;

    while ((row < n_rows) and !finished)
    { rv_column.push_back(_qtc_entries[index++]);
      finished = (index == n_entries);
      row++;
    }

    rv_columns.push_back(rv_column);
  }

  const size_t n_columns = rv_columns.size();
#endif

  return string();

}

/// window < qtc
window& operator<(window& win, const qtc& q)
{ static const unsigned int COLUMN_WIDTH = qtc_entry().size();                                // width of a column

  win < WINDOW_CLEAR < CURSOR_TOP_LEFT;

  size_t index = 0;    // keep track of where we are in vector of entries
  const auto qtc_entries = q.qtc_entries();

  for (const auto& pr : qtc_entries)
  { const string entry_str = pr.first.to_string();
    int cpu = colours.add(win.fg(), pr.second ? COLOUR_RED : COLOUR_BLACK);

// work out where to start the display of this call
    const unsigned int x = (index / win.height()) * COLUMN_WIDTH;
    const unsigned int y = (win.height() - 1) - (index % win.height());

    win < cursor(x, y) < colour_pair(cpu);
    win < entry_str;

    index++;
  }

  return win;
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

// write to file
void qtc_database::write(const string& filename)
{ string str;

  { SAFELOCK(qtc_database);

    for (const auto& QTC : _qtc_db)
    { str += "ID=" + QTC.id() + " ";
      str += "TARGET=" + QTC.target() + EOL;

      const vector<pair<qtc_entry, bool>> qtc_entries = QTC.qtc_entries();    ///< the individual QTC entries, and whether each has been sent

//    for_each(qtc_entries.cbegin(), qtc_entries.cend(), [&] (const auto& qeb) { str +=
      for (const auto& qeb : qtc_entries)
      { str += qeb.first.to_string() + EOL;
      }
    }
  }

  write_file(str, filename);
}

// read from file
void qtc_database::read(const string& filename)
{ if (!file_exists(filename))
    return;

  const vector<string> lines = to_lines(read_file(filename));
  unsigned int line_nr = 0;


  while (line_nr < lines.size())
  { qtc QTC;
    const string& id_line = lines[line_nr];
    const vector<string> fields = split_string(id_line, ' ');

    QTC.id(split_string(fields[0], '=')[1]);
    QTC.target(split_string(fields[1], '=')[1]);

    vector<pair<qtc_entry, bool>> qtc_entries;    ///< the individual QTC entries, and whether each has been sent

    while ((line_nr != lines.size() - 1) and (!contains(lines[line_nr + 1], "=")))
    { const string& line = lines[++line_nr];

      qtc_entry qe;

      qe.utc(substring(line, 0, 4));
      qe.callsign(remove_trailing_spaces(substring(line, 5, line.length() - 4)));
      qe.serno(last(line, 4));

      qtc_entries.push_back( { qe, true } );
    }

    QTC.qtc_entries(qtc_entries);
  }
}

// -----------------------------------  qtc_buffer  ----------------------------

/*!     \class qtc_buffer
        \brief Buffer to handle process of moving QTCs from unsent to sent
*/

void qtc_buffer::operator+=(const logbook& logbk)
{ const vector<QSO> qsos = logbk.as_vector();

  for (const auto& qso : qsos)
  { if (qso.continent() == "EU")
    { const qtc_entry entry(qso);

      if (_sent_qtcs.find(entry) == _sent_qtcs.end())
      { if (find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry) == _unsent_qtcs.end())
          _unsent_qtcs.push_back(entry);
      }
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
