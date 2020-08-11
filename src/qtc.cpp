// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   qtc.cpp

    Classes and functions related to WAE QTCs
*/

#include "diskfile.h"
#include "qtc.h"

#include <algorithm>

using namespace std;

// -----------------------------------  qtc_entry  ----------------------------

/*! \class  qtc_entry
    \brief  An entry in a QTC
*/

/// convert to printable string
const string qtc_entry::to_string(void) const
{ constexpr unsigned int CALL_WIDTH { 12 };

  return (_utc + SPACE_STR + pad_string(_callsign, CALL_WIDTH, PAD_RIGHT) + SPACE_STR + _serno /* + " [ " + pad_string(::to_string(_my_serno), 4) + " ]" */);
}

// -----------------------------------  qtc_series  ----------------------------

/*! \class  qtc_series
    \brief  A QTC series as defined by the WAE rules
*/

/*! \brief          Get all the entries that either have been sent or have not been sent
    \param  sent    whether to return the sent entries
    \return         if <i>sent</i> is <i>true</i>, return all the sent entries; otherwise retrun all the unsent entries
*/
const vector<qtc_entry> qtc_series::_sent_or_unsent_qtc_entries(const bool sent) const
{ vector<qtc_entry> rv;

  FOR_ALL(_qtc_entries, [&] (const pair<qtc_entry, bool>& pqeb) { if ( (sent ? pqeb.second : !pqeb.second) ) rv.push_back(pqeb.first); } );

  return rv;
}

/*! \brief          Add a qtc_entry
    \param  param   entry to add, and whether the entry has been sent
    \return         whether <i>param</i> was actually added
*/
const bool qtc_series::operator+=(const pair<qtc_entry, bool>& p)
{ const qtc_entry& entry { p.first };
  const bool       sent  { p.second };

  if (entry.valid() and (entry.callsign() != _target))
  { _qtc_entries.push_back( { entry, sent });
    return true;
  }

  return false;
}

/*! \brief      Return an entry
    \param  n   index number to return (wrt 0)
    \return     <i>n</i>th entry

    Returns empty pair if <i>n</i> is out of bounds.
*/
const pair<qtc_entry, bool> qtc_series::operator[](const unsigned int n) const
{ static const pair<qtc_entry, bool> empty_return_value { };

  return ( (n < _qtc_entries.size()) ? _qtc_entries[n] : empty_return_value );
}

/*! \brief      Mark a particular entry as having been sent
    \param  n   index number to mark (wrt 0)

    Does nothing if entry number <i>n</i> does not exist
*/
void qtc_series::mark_as_sent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = true;
}

/*! \brief      Mark a particular entry as having NOT been sent
    \param  n   index number to mark (wrt 0)

    Does nothing if entry number <i>n</i> does not exist
*/
void qtc_series::mark_as_unsent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = false;
}

/*! \brief          Get first <i>qtc_entry</i> beyond a certain point that has not been sent
    \param  posn    index number (wrt 0) at which to start searching
    \return         first <i>qtc_entry</i> at or later than <i>posn</i> that has not been sent

    Returns an empty <i>qtc_entry</i> if no entries meet the criteria
*/
const qtc_entry qtc_series::first_not_sent(const unsigned int posn)
{ unsigned int index { posn };

  while (index < _qtc_entries.size())
  { if (!_qtc_entries[index].second)
      return _qtc_entries[index].first;

    index++;
  }

  return qtc_entry();
}

/*! \brief      How many entries have been sent?
    \return     the number of entries that have been sent
*/
const unsigned int qtc_series::n_sent(void) const
{ unsigned int rv { 0 };

  FOR_ALL(_qtc_entries, [&rv] (const pair<qtc_entry, bool> & pqeb) { if (pqeb.second) rv++; } );

  return rv;
}

/*! \brief      How many entries have not been sent?
    \return     the number of entries that have not been sent
*/
const unsigned int qtc_series::n_unsent(void) const
{ unsigned int rv { 0 };

  FOR_ALL(_qtc_entries, [&rv] (const pair<qtc_entry, bool> & pqeb) { if (!pqeb.second) rv++; } );

  return rv;
}

/*! \brief      Get a string representing a particular entry
    \param  n   number of the entry (wrt 0)
    \return     string describing <i>qtc_entry</i> number <i>n</i>

    Returns the empty string if entry number <i>n</i> does not exist
*/
const string qtc_series::output_string(const unsigned int n) const
{ //static const string SPACE(" ");
  string rv;

  if (n >= size())
    return rv;

  const qtc_entry qe { _qtc_entries[n].first };

  rv = pad_string(_frequency, 5) + SPACE_STR;
  rv += (_mode + SPACE_STR + _date + SPACE_STR + _utc + SPACE_STR);
  rv += substring(pad_string(_target, 13, PAD_RIGHT, ' '), 0, 13) + SPACE_STR;

  const vector<string> qtc_ser { split_string(_id, "/"s) };

  rv += pad_string(qtc_ser[0], 3, PAD_LEFT, '0') + "/"s + pad_string(qtc_ser[1], 2, PAD_LEFT, '0') + create_string(' ', 5);
  rv += substring(pad_string(_source, 13, PAD_RIGHT, ' '), 0, 13) + SPACE_STR;
  rv += qe.utc() + SPACE_STR;
  rv += substring(pad_string(qe.callsign(), 13, PAD_RIGHT, ' '), 0, 13) + SPACE_STR;
  rv += qe.serno();

  return rv;
}

/*! \brief      Get a string representing all the entries
    \return     string describing all the <i>qtc_entry</i> elements
*/
const string qtc_series::complete_output_string(void) const
{ string rv;

  for (size_t n = 0; n < size(); ++n)
  { if (_qtc_entries[n].second)
      rv += (output_string(n) + EOL);
  }

  return rv;
}

// from http://www.kkn.net/~trey/cabrillo/qso-template.html:
//
//                             -qtc rcvd by - --------------qtc info received-----------------
//QTC: freq  mo date       time call          qserial    qtc sent by   qtim qcall         qexc
//QTC: ***** ** yyyy-mm-dd nnnn ************* nnn/nn     ************* nnnn ************* nnnn
//QTC:  3799 PH 2003-03-23 0711 YB1AQS        001/10     DL8WPX        0330 DL6RAI        1021

/// window < qtc_series
window& operator<(window& win, const qtc_series& qs)
{ const size_t COLUMN_WIDTH { qtc_entry().size() };                           // width of a column

  constexpr unsigned int COLUMN_GAP { 2 };                                              // gap between columns
  constexpr int          GAP_COLOUR { COLOUR_YELLOW };

  win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT;

// write the column separators
  for (int y = 0; y < win.height(); ++y)
    win < cursor(COLUMN_WIDTH, y) < colour_pair(colours.add(GAP_COLOUR, GAP_COLOUR)) < "  ";

  size_t index { 0 };    // keep track of where we are in vector of entries

  const auto qtc_entries { qs.qtc_entries() };

  for (const auto& pr : qtc_entries)
  { const string entry_str { pr.first.to_string() };

    int cpu { static_cast<int>(colours.add(win.fg(), pr.second ? COLOUR_RED : win.bg())) };

// work out where to start the display of this call
    const unsigned int x { static_cast<unsigned int>( (index / win.height()) * (COLUMN_WIDTH + COLUMN_GAP) ) };
    const unsigned int y { static_cast<unsigned int>( (win.height() - 1) - (index % win.height()) ) };

    win < cursor(x, y) < colour_pair(cpu) < entry_str;

    index++;
  }

  return win;
}

// -----------------------------------  qtc_database  ----------------------------

/*!     \class qtc_database
        \brief All QTCs
*/

pt_mutex qtc_database_mutex;                            ///< mutex to allow correct locking

/*! \brief      Add a series of QTCs to the database
    \param  q   the series of QTCs to be added
*/
void qtc_database::operator+=(const qtc_series& q)
{ qtc_series q_copy { q };

  SAFELOCK(qtc_database);

  if (q_copy.id().empty())
    q_copy.id( to_string(_qtc_db.size() + 1) + "/"s + to_string(q.size()) );

  _qtc_db.push_back(q_copy);
}

/*! \brief      Get one of the series in the database
    \param  n   number of the series to return (wrt 0)
    \return     series number <i>n</i>

    Returns an empty series if <i>n</i> is out of bounds
*/
const qtc_series qtc_database::operator[](size_t n)
{ if (n >= size())
    return qtc_series();

  SAFELOCK(qtc_database);

  return _qtc_db.at(n);
}

/*! \brief                          Get the number of QTCs that have been sent to a particular station
    \param   destination_callsign   the station to which the QTCs have been sent
    \return                         number of QTCs that have been sent to <i>destination_callsign</i>
*/
const unsigned int qtc_database::n_qtcs_sent_to(const string& destination_callsign) const
{ unsigned int rv { 0 };

  SAFELOCK(qtc_database);

  FOR_ALL(_qtc_db, [=, &rv] (const qtc_series& QTC) { if (QTC.target() == destination_callsign) rv += QTC.size(); } );

  return rv;
}

/*! \brief                          Get the total number of QTC entries that have been sent
    \return                         the number of QTC entries that have been sent
*/
const unsigned int qtc_database::n_qtc_entries_sent(void) const
{ unsigned int rv { 0 };

  FOR_ALL(_qtc_db, [&rv] (const qtc_series& QTC) { rv += QTC.size(); } );

  return rv;
}

/*! \brief              Read from a file
    \param  filename    name of file that contains the database
*/
void qtc_database::read(const string& filename)
{ if (!file_exists(filename))
    return;

  const vector<string> lines { to_lines(read_file(filename)) };

  unsigned int line_nr { 0 };

  string last_id;
  qtc_series series;

// 28100.0 CW 2014-07-18 15:32:42 G1AAA         001/10     N7DR          1516 YL2BJ         477
// 28100.0 CW 2014-07-18 15:32:42 G1AAA 001/10 N7DR 1516 YL2BJ 477
//    0    1      2         3       4      5     6    7    8    9
  while (line_nr < lines.size())
  { const string&        line   { lines[line_nr++] };
    const vector<string> fields { remove_peripheral_spaces(split_string(squash(line), ' ')) };

    if (fields.size() != 10)
      throw qtc_error(QTC_INVALID_FORMAT, "QTC has "s + to_string(fields.size()) + " fields; "s + line);

    const string id { fields[5] };

    if (line_nr == 1)  // we've already incremented the line number
      last_id = id;


    if (id != last_id)       // new ID?
    { _qtc_db.push_back(series);

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
}

// -----------------------------------  qtc_buffer  ----------------------------

/*!     \class qtc_buffer
        \brief Buffer to handle process of moving QTCs from unsent to sent
*/

/*! \brief          Add all unsent QSOs from a logbook to the buffer
    \param  logbk   logbook

    Does not add QSOs already in the buffer (either as sent or unsent).
    Does not add non-EU QSOs.
*/
void qtc_buffer::operator+=(const logbook& logbk)
{ const vector<QSO> qsos { logbk.as_vector() };

  FOR_ALL(qsos, [&] (const QSO& qso) { (*this) += qso; } );
}

/*! \brief          Add a QSO to the buffer
    \param  qso     QSO to add

    Does nothing if <i>qso</i> is already in the buffer (either as sent or unsent).
    Does nothing if the QSO is not with an EU station.
*/
void qtc_buffer::operator+=(const QSO& qso)
{ if (qso.continent() == "EU"s)
  { const qtc_entry entry { qso };

//    ost << "testing QSO to see if it's been sent: " << qso << endl;

//    if (_sent_qtcs.find(entry) == _sent_qtcs.end())
    if (find(_sent_qtcs.begin(), _sent_qtcs.end(), entry) == _sent_qtcs.end())
    { //ost << "QSO NOT IN SENT LIST" << endl;

      if (find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry) == _unsent_qtcs.end())
 //     if (_unsent_qtcs.find(entry) == _unsent_qtcs.end())
      { //ost << "QSO NOT IN UNSENT LIST" << endl;
        _unsent_qtcs.push_back(entry);
        //ost << "Added entry to unsent list: " << entry.to_string() << endl;
      }
    }
  }
}

/*! \brief          Recreate the unsent list
    \param  logbk   logbook
*/
void qtc_buffer::rebuild_unsent_list(const logbook& logbk)
{ //ost << "Inside qtc_buffer::rebuild()" << endl;
  _unsent_qtcs.clear();

  *this += logbk;
  //ost << "Exiting qtc_buffer::rebuild()" << endl;

  //ost << "unsent QTCs: " << endl << unsent_list_as_string() << endl;
}

/*! \brief                  Get a batch of QTC entries that may be sent to a particular destination
    \param  max_entries     maximum number of QTC entries to return
    \param  target          station to which the QTC entries are to be sent
    \return                 the sendable QTC entries
*/
const vector<qtc_entry> qtc_buffer::get_next_unsent_qtc(const unsigned int max_entries, const string& target)
{ vector<qtc_entry> rv;

  auto cit { _unsent_qtcs.cbegin() };

  while ( (rv.size() != max_entries) and (cit != _unsent_qtcs.cend()) )
  { if (cit->callsign() != target)
      rv.push_back(*cit);               // don't send a QTC back to the station of the original QSO
    ++cit;
  }

  return rv;
}

/*! \brief          Transfer a <i>qtc_entry</i> from unsent status to sent status
    \param  entry   <i>qtc_entry</i> to transfer
*/
void qtc_buffer::unsent_to_sent(const qtc_entry& entry)
{ //const auto it { find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry) };

  //if (it != _unsent_qtcs.end())
  //  _unsent_qtcs.erase(it);

//  ost << "FIRST UNSENT ENTRY = " << *(_unsent_qtcs.begin()) << endl;
//  ost << "THIS  UNSENT ENTRY = " << entry << endl;

//  if (_unsent_qtcs.find(entry) != _unsent_qtcs.end())
  if (find(_unsent_qtcs.begin(), _unsent_qtcs.end(), entry) != _unsent_qtcs.end())
  { _unsent_qtcs.remove(entry);
    //ost << "Removed entry from unsent set: " << entry.to_string() << endl;
  }  

  _sent_qtcs.push_back(entry);
  //ost << "Added entry to sent list: " << entry.to_string() << endl;
}

/*! \brief      Transfer all the entries in a <i>qtc_series</i> from unsent status to sent status
    \param  qs  QTC entries to transfer
*/
void qtc_buffer::unsent_to_sent(const qtc_series& qs)
{ //ost << "Inside qtc_buffer::unsent_to_sent(qtc_series)" << endl;

  //ost << "size of qtc_series = " << qs.size() << ", # sent = " << qs.n_sent() << ", # unsent = " << qs.n_unsent() << endl;

  const vector<qtc_entry> sent_qtc_entries { qs.sent_qtc_entries() };

  FOR_ALL(sent_qtc_entries, [&] (const qtc_entry& qe) { unsent_to_sent(qe); } );

  //ost << "finished qtc_buffer::unsent_to_sent(qtc_series)" << endl;
}

/*! \brief      The unsent list in human-readable format
    \return     the unsent list as a string
*/
string qtc_buffer::unsent_list_as_string(void) const
{ string rv;

  size_t nr { 1 };

  for (const qtc_entry& qe : _unsent_qtcs)
  { rv += pad_string(to_string(nr), 4) + ": "s + /* pad_string(to_string(qe.my_serno()), 4) + SPACE_STR + */ qe.utc() + SPACE_STR + pad_string(qe.callsign(), 13, PAD_RIGHT) + SPACE_STR + qe.serno();

    if (nr++ != _unsent_qtcs.size())
      rv += EOL; 
  }

  return rv;
}
