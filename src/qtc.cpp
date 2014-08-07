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

/// default constructor
qtc_entry::qtc_entry(void) :
  _utc("0000"),
  _callsign(),
  _serno("0000")
{
}

/// construct from a QSO
qtc_entry::qtc_entry(const QSO& qso) :
  _utc(substring(qso.utc(), 0, 2) + substring(qso.utc(), 3, 2)),
  _callsign( (qso.continent() == "EU") ? qso.callsign() : string()),
  _serno(pad_string(qso.received_exchange("SERNO"), 4, PAD_RIGHT))    // force width to 4
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

// -----------------------------------  qtc_series  ----------------------------

/*!     \class qtc_series
        \brief A QTC series as defined by the WAE rules
*/

const vector<qtc_entry> qtc_series::sent_qtc_entries(void) const
{ vector<qtc_entry> rv;

  for_each(_qtc_entries.cbegin(), _qtc_entries.cend(), [&] (const pair<qtc_entry, bool>& pqeb) { if (pqeb.second) rv.push_back(pqeb.first); } );

  return rv;
}

const vector<qtc_entry> qtc_series::unsent_qtc_entries(void) const
{ vector<qtc_entry> rv;

  for_each(_qtc_entries.cbegin(), _qtc_entries.cend(), [&] (const pair<qtc_entry, bool>& pqeb) { if (!pqeb.second) rv.push_back(pqeb.first); } );

  return rv;
}

//const bool qtc_series::operator+=(const qtc_entry& entry)
//{ if (entry.valid() and (entry.callsign() != _target))
//  { _qtc_entries.push_back( { entry, QTC_UNSENT });
//    return true;
//  }
//
//  return false;
//}

const bool qtc_series::operator+=(const std::pair<qtc_entry, bool>& p)
{ const qtc_entry& entry = p.first;
  const bool sent = p.second;

  if (entry.valid() and (entry.callsign() != _target))
  { _qtc_entries.push_back( { entry, sent });
    return true;
  }

  return false;
}


// set a particular entry to sent
void qtc_series::mark_as_sent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = true;
}

// set a particular entry to unsent
void qtc_series::mark_as_unsent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = false;
}

// get first entry that has not been sent
const qtc_entry qtc_series::first_not_sent(const unsigned int posn)
{ unsigned int index = posn;

  while (index < _qtc_entries.size())
  { if (!_qtc_entries[index].second)
      return _qtc_entries[index].first;

    index++;
  }

  return qtc_entry();
}

const unsigned int qtc_series::n_sent(void) const
{ unsigned int rv = 0;

//  for (const auto qe : _qtc_entries)
//  { if (qe.second)
//      rv++;
//  }

  for_each(_qtc_entries.cbegin(), _qtc_entries.cend(), [&rv] (const pair<qtc_entry, bool> & pqeb) { if (pqeb.second) rv++; } );

  return rv;
}

const unsigned int qtc_series::n_unsent(void) const
{ unsigned int rv = 0;

  for (const auto qe : _qtc_entries)
  { if (!qe.second)
      rv++;
  }

  return rv;
}

const string qtc_series::output_string(const unsigned int n) const
{ static const string SPACE(" ");
  string rv;

  if (n >= size())
    return rv;

  const qtc_entry qe = _qtc_entries[n].first;

//  rv = pad_string(remove_from_end(_frequency, 2), 5) + SPACE;
  rv = pad_string(_frequency, 5) + SPACE;
  rv += (_mode + SPACE + _date + SPACE + _utc + SPACE);
  rv += substring(pad_string(_target, 13, PAD_RIGHT, ' '), 0, 13) + SPACE;

  const vector<string> qtc_ser = split_string(_id, "/");

  rv += pad_string(qtc_ser[0], 3, PAD_LEFT, '0') + "/" + pad_string(qtc_ser[1], 2, PAD_LEFT, '0') + create_string(' ', 5);
  rv += substring(pad_string(_source, 13, PAD_RIGHT, ' '), 0, 13) + SPACE;
  rv += qe.utc() + SPACE;
  rv += substring(pad_string(qe.callsign(), 13, PAD_RIGHT, ' '), 0, 13) + SPACE;
  rv += qe.serno();

  return rv;
}

const string qtc_series::complete_output_string(void) const
{ string rv;

  ost << "series size = " << size() << endl;

  for (size_t n = 0; n < size(); ++n)
  { if (_qtc_entries[n].second)
      rv += (output_string(n) + EOL);
  }

  ost << "complete_output_string returning: " << rv << endl;

  return rv;
}

// from http://www.kkn.net/~trey/cabrillo/qso-template.html:
//
//                             -qtc rcvd by - --------------qtc info received-----------------
//QTC: freq  mo date       time call          qserial    qtc sent by   qtim qcall         qexc
//QTC: ***** ** yyyy-mm-dd nnnn ************* nnn/nn     ************* nnnn ************* nnnn
//QTC:  3799 PH 2003-03-23 0711 YB1AQS        001/10     DL8WPX        0330 DL6RAI        1021

#include <array>

const string qtc_series::to_string(const unsigned int n_rows) const
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

/// window < qtc_series
window& operator<(window& win, const qtc_series& qs)
{ static const unsigned int COLUMN_WIDTH = qtc_entry().size();                           // width of a column
  static const unsigned int COLUMN_GAP = 2;                                              // gap between columns
  static const int GAP_COLOUR = COLOUR_YELLOW;

  win < WINDOW_CLEAR < CURSOR_TOP_LEFT;

// write the column separators
  for (int y = 0; y < win.height(); ++y)
    win < cursor(COLUMN_WIDTH, y) < colour_pair(colours.add(GAP_COLOUR, GAP_COLOUR)) <= "  ";

  size_t index = 0;    // keep track of where we are in vector of entries
  const auto qtc_entries = qs.qtc_entries();

  for (const auto& pr : qtc_entries)
  { const string entry_str = pr.first.to_string();
    int cpu = colours.add(win.fg(), pr.second ? COLOUR_RED : win.bg());

// work out where to start the display of this call
    const unsigned int x = (index / win.height()) * (COLUMN_WIDTH + COLUMN_GAP);
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

void qtc_database::operator+=(const qtc_series& q)
{ qtc_series q_copy = q;

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

  for_each(_qtc_db.cbegin(), _qtc_db.cend(), [=, &rv] (const qtc_series& QTC) { if (QTC.target() == destination_callsign) rv += QTC.size(); } );

  return rv;
}

const unsigned int qtc_database::n_qtc_entries_sent(void) const
{ unsigned int rv = 0;

  for_each(_qtc_db.cbegin(), _qtc_db.cend(), [&rv] (const qtc_series& QTC) { rv += QTC.size(); } );

  return rv;
}

// read from file
void qtc_database::read(const string& filename)
{  ost << "in qtc_database::read()" << endl;

  if (!file_exists(filename))
    return;

  const vector<string> lines = to_lines(read_file(filename));
  unsigned int line_nr = 0;
  string last_id;
  qtc_series series;

// 28100.0 CW 2014-07-18 15:32:42 G1AAA         001/10     N7DR          1516 YL2BJ         477
// 28100.0 CW 2014-07-18 15:32:42 G1AAA 001/10 N7DR 1516 YL2BJ 477
//    0    1      2         3       4      5     6    7    8    9
  while (line_nr < lines.size())
  { const string& line = lines[line_nr++];
    const vector<string> fields = remove_peripheral_spaces(split_string(squash(line), ' '));

    if (fields.size() != 10)
      throw qtc_error(QTC_INVALID_FORMAT, string("QTC has ") + to_string(fields.size()) + " fields; " + line);

    const string id = fields[5];

    if (line_nr == 1)  // we've already incremented the line number
      last_id = id;

     ost << "id = " << id << endl;

    if (id != last_id)       // new ID?
    {  ost << "id != last_id" << endl;

      _qtc_db.push_back(series);

// do stuff, then:
      last_id = id;    // ready to process the new ID
      series.clear();
    }
//    else
    { if (series.id().empty())
        series.id(id);

      if (series.frequency_str().empty())
        series.frequency_str(fields[0]);

      if (series.mode().empty())
        series.mode(fields[1]);

      if (series.date().empty())
        series.date(fields[2]);

      if (series.utc().empty())
        series.utc(fields[3]);

      if (series.destination().empty())
        series.destination(fields[4]);

      if (series.source().empty())
        series.source(fields[6]);

      qtc_entry qe;

      qe.utc(fields[7]);
      qe.callsign(fields[8]);
      qe.serno(fields[9]);

      series += pair<qtc_entry, bool>( { qe, QTC_SENT } );
    }
  }

// add the last series to the database
  if (!lines.empty())
    _qtc_db.push_back(series);

  ost << "size of _qtc_db = " << _qtc_db.size() << endl;
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

const vector<qtc_entry> qtc_buffer::get_next_unsent_qtc(const string& target, const unsigned int max_entries)
{ vector<qtc_entry> rv;

  list<qtc_entry>::const_iterator cit = _unsent_qtcs.cbegin();

  while ( (rv.size() != max_entries) and (cit != _unsent_qtcs.cend()) )
  { rv.push_back(*cit);
    ++cit;
  }

  return rv;
}

void qtc_buffer::unsent_to_sent(const qtc_entry& entry)
{ ost << "attempting to transfer: " << entry.to_string() << endl;

  const auto it = find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry);

  ost << "size of _unsent_qtcs = " << _unsent_qtcs.size() << endl;

  if (it != _unsent_qtcs.end())
  { ost << "Erasing: " << it->to_string() << endl;
    _unsent_qtcs.erase(it);
  }

  _sent_qtcs.insert(entry);
}

void qtc_buffer::unsent_to_sent(const qtc_series& qs)
{ //const vector<pair<qtc_entry, bool>>& vec_qe = qs.qtc_entries();    ///< the individual QTC entries, and whether each has been sent

  const vector<qtc_entry> sent_qtc_entries = qs.sent_qtc_entries();

  ost << "number of sent qtc entries in vector<qtc_entry> = " << sent_qtc_entries.size() << endl;

  for (const auto& qe : sent_qtc_entries)
    unsent_to_sent(qe);
}
