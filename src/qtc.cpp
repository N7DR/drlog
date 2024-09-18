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
string qtc_entry::to_string(void) const
{ constexpr unsigned int CALL_WIDTH { 12 };

  return (_utc + SPACE_STR + pad_right(_callsign, CALL_WIDTH) + SPACE_STR + _serno);
}

// -----------------------------------  qtc_series  ----------------------------

/*! \class  qtc_series
    \brief  A QTC series as defined by the WAE rules
*/

/*! \brief              Get all the entries that either have been sent or have not been sent
    \param  qstatus     the status of the QTCs to return
    \return             all the QTCs whose status matches <i>qstatus</i>
*/
//vector<qtc_entry> qtc_series::_sent_or_unsent_qtc_entries(const bool sent) const
vector<qtc_entry> qtc_series::_sent_or_unsent_qtc_entries(const QTC_STATUS qstatus) const
{ vector<qtc_entry> rv;

  FOR_ALL(_qtc_entries, [qstatus, &rv] (const QTC_AND_STATUS& pqeb) { if (qstatus == pqeb.second) rv += pqeb.first; } );  // should use COPY_IF

  return rv;
}

/*! \brief          Add a qtc_entry
    \param  param   entry to add, and whether the entry has been sent
    \return         whether <i>param</i> was actually added
*/
bool qtc_series::operator+=(const QTC_AND_STATUS& p)
{ const qtc_entry& entry { p.first };

  if (entry.valid() and (entry.callsign() != _target))
  { _qtc_entries += p;
    return true;
  }

  return false;
}

/*! \brief      Return an entry
    \param  n   index number to return (wrt 0)
    \return     <i>n</i>th entry

    Returns empty pair if <i>n</i> is out of bounds.
*/
QTC_AND_STATUS qtc_series::operator[](const unsigned int n) const
{ static const QTC_AND_STATUS empty_return_value { { }, QTC_STATUS::UNSENT };

  return ( (n < _qtc_entries.size()) ? _qtc_entries[n] : empty_return_value );
}

/*! \brief      Mark a particular entry as having been sent
    \param  n   index number to mark (wrt 0)

    Does nothing if entry number <i>n</i> does not exist
*/
void qtc_series::mark_as_sent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = QTC_STATUS::SENT;
}

/*! \brief      Mark a particular entry as having NOT been sent
    \param  n   index number to mark (wrt 0)

    Does nothing if entry number <i>n</i> does not exist
*/
void qtc_series::mark_as_unsent(const unsigned int n)
{ if (n < _qtc_entries.size())
    _qtc_entries[n].second = QTC_STATUS::UNSENT;
}

/*! \brief          Get first <i>qtc_entry</i> beyond a certain point that has not been sent
    \param  posn    index number (wrt 0) at which to start searching
    \return         first <i>qtc_entry</i> at or later than <i>posn</i> that has not been sent

    Returns an empty <i>qtc_entry</i> if no entries meet the criteria
*/
qtc_entry qtc_series::first_not_sent(const unsigned int posn) const
{ unsigned int index { posn };

  while (index < _qtc_entries.size())
  { const auto& [ qe, status ] { _qtc_entries[index] };

    if (status == QTC_STATUS::UNSENT)
      return qe;

    index++;
  }

  return qtc_entry();
}

/*! \brief      How many entries have been sent?
    \return     the number of entries that have been sent
*/
unsigned int qtc_series::n_sent(void) const
{ unsigned int rv { 0 };

  FOR_ALL(_qtc_entries, [&rv] (const QTC_AND_STATUS& pqeb) { if (pqeb.second == QTC_STATUS::SENT) rv++; } );

  return rv;
}

/*! \brief      How many entries have not been sent?
    \return     the number of entries that have not been sent
*/
unsigned int qtc_series::n_unsent(void) const
{ unsigned int rv { 0 };

  FOR_ALL(_qtc_entries, [&rv] (const QTC_AND_STATUS& pqeb) { if (pqeb.second == QTC_STATUS::UNSENT) rv++; } );

  return rv;
}

/*! \brief      Get a string representing a particular entry
    \param  n   number of the entry (wrt 0)
    \return     string describing <i>qtc_entry</i> number <i>n</i>

    Returns the empty string if entry number <i>n</i> does not exist
*/
string qtc_series::output_string(const unsigned int n) const
{ if (n >= size())
    return string();

//  const qtc_entry qe { _qtc_entries[n].first };
  const qtc_entry qe { entry(n) };

  string rv { pad_left(_frequency, 5) + SPACE_STR };

  rv += (_mode + SPACE_STR + _date + SPACE_STR + _utc + SPACE_STR);
  rv += substring <std::string> (pad_right(_target, 13), 0, 13) + SPACE_STR;

  const vector<string_view> qtc_ser { split_string <std::string_view> (_id, '/') };

  rv += pad_leftz(qtc_ser[0], 3) + "/"s + pad_leftz(qtc_ser[1], 2) + create_string(' ', 5);
  rv += substring <std::string> (pad_right(_source, 13), 0, 13) + SPACE_STR;
  rv += qe.utc() + SPACE_STR;
  rv += substring <std::string> (pad_right(qe.callsign(), 13), 0, 13) + SPACE_STR;
  rv += qe.serno();       // this is right-padded with spaces to a width of four

  return rv;
}

/*! \brief      Get a string representing all the SENT entries
    \return     string describing all the SENT <i>qtc_entry</i> elements
*/
string qtc_series::complete_output_string(void) const
{ string rv { };

  for (size_t n { 0 }; n < size(); ++n)
  { if (_qtc_entries[n].second == QTC_STATUS::SENT)
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
  for (int y { 0 }; y < win.height(); ++y)
    win < cursor(COLUMN_WIDTH, y) < colour_pair(colours.add(GAP_COLOUR, GAP_COLOUR)) < "  ";

  size_t index { 0 };    // keep track of where we are in vector of entries

  for (const auto& [ qe, status ] : qs.qtc_entries())  //vector<pair<qtc_entry, QTC_STATUS>>
  { const string entry_str { qe.to_string() };
    const int    cpu       { static_cast<int>(colours.add(win.fg(), (status == QTC_STATUS::SENT) ? COLOUR_RED : win.bg())) };

// work out where to start the display of this call
    const unsigned int x { static_cast<unsigned int>( (index / win.height()) * (COLUMN_WIDTH + COLUMN_GAP) ) };
    const unsigned int y { static_cast<unsigned int>( (win.height() - 1) - (index % win.height()) ) };

    win < cursor(x, y) < colour_pair(cpu) < entry_str;

    index++;
  }

#if 0
  for (const auto& pr : qtc_entries)
  { const string entry_str { pr.first.to_string() };
    const int    cpu       { static_cast<int>(colours.add(win.fg(), (pr.second == QTC_STATUS::SENT) ? COLOUR_RED : win.bg())) };

// work out where to start the display of this call
    const unsigned int x { static_cast<unsigned int>( (index / win.height()) * (COLUMN_WIDTH + COLUMN_GAP) ) };
    const unsigned int y { static_cast<unsigned int>( (win.height() - 1) - (index % win.height()) ) };

    win < cursor(x, y) < colour_pair(cpu) < entry_str;

    index++;
  }
#endif

  return win;
}

// -----------------------------------  qtc_database  ----------------------------

/*!     \class qtc_database
        \brief All QTCs
*/

//pt_mutex qtc_database_mutex { "QTC DATABASE"s };                            ///< mutex to allow correct locking

/*! \brief      Add a series of QTCs to the database
    \param  q   the series of QTCs to be added
*/
void qtc_database::operator+=(const qtc_series& q)
{ qtc_series q_copy { q };

  SAFELOCK(_qtc_database);

  if (q_copy.id().empty())
    q_copy.id( to_string(_qtc_db.size() + 1) + "/"s + to_string(q.size()) );

  _qtc_db += q_copy;
}

/*! \brief      Get one of the series in the database
    \param  n   number of the series to return (wrt 0)
    \return     series number <i>n</i>

    Returns an empty series if <i>n</i> is out of bounds
*/
qtc_series qtc_database::operator[](size_t n) const
{ if (n >= size())
    return qtc_series();

  SAFELOCK(_qtc_database);

  return _qtc_db.at(n);
}

/*! \brief                          Get the number of QTCs that have been sent to a particular station
    \param   destination_callsign   the station to which the QTCs have been sent
    \return                         number of QTCs that have been sent to <i>destination_callsign</i>
*/
unsigned int qtc_database::n_qtcs_sent_to(const string& destination_callsign) const
{ unsigned int rv { 0 };

  SAFELOCK(_qtc_database);

  FOR_ALL(_qtc_db, [=, &rv] (const qtc_series& QTC) { if (QTC.target() == destination_callsign) rv += QTC.size(); } );

  return rv;
}

/*! \brief                          Get the total number of QTC entries that have been sent
    \return                         the number of QTC entries that have been sent
*/
unsigned int qtc_database::n_qtc_entries_sent(void) const
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

  const vector<string> lines { remove_peripheral_spaces <std::string> (to_lines <std::string_view> (read_file(filename))) };

  unsigned int line_nr { 0 };

  string     last_id;
  qtc_series series;

// 28100.0 CW 2014-07-18 15:32:42 G1AAA         001/10     N7DR          1516 YL2BJ         477
// 28100.0 CW 2014-07-18 15:32:42 G1AAA 001/10 N7DR 1516 YL2BJ 477
//    0    1      2         3       4      5     6    7    8    9

  constexpr int FREQ_FIELD      { 0 };
  constexpr int MODE_FIELD      { 1 };
  constexpr int DATE_FIELD      { 2 };
  constexpr int UTC_FIELD       { 3 };
  constexpr int DEST_FIELD      { 4 };
  constexpr int ID_FIELD        { 5 };
  constexpr int SRC_FIELD       { 6 };
  constexpr int QSO_UTC_FIELD   { 7 };
  constexpr int QSO_CALL_FIELD  { 8 };
  constexpr int QSO_SERNO_FIELD { 9 };

  while (line_nr < lines.size())
  { const string&        line   { lines[line_nr++] };
//    const vector<string> fields { remove_peripheral_spaces <std::string> (split_string <std::string> (squash(line), ' ')) };
    const vector<string> fields { remove_trailing_spaces <std::string> (split_string <std::string> (squash(line), ' ')) };

    if (fields.size() != 10)
      throw qtc_error(QTC_INVALID_FORMAT, "QTC has "s + to_string(fields.size()) + " fields (should be 10): "s + line);

    const string id { fields[ID_FIELD] };

    if (line_nr == 1)  // we've already incremented the line number
      last_id = id;

    if (id != last_id)       // new ID?
    { _qtc_db += series;

// do stuff, then:
      last_id = id;    // ready to process the new ID
      series.clear();
    }

    if (series.id().empty())
      series.id(id);

    if (series.frequency_str().empty())
      series.frequency_str(fields[FREQ_FIELD]);

    if (series.mode().empty())
      series.mode(fields[MODE_FIELD]);

    if (series.date().empty())
      series.date(fields[DATE_FIELD]);

    if (series.utc().empty())
      series.utc(fields[UTC_FIELD]);

    if (series.destination().empty())
      series.destination(fields[DEST_FIELD]);

    if (series.source().empty())
      series.source(fields[SRC_FIELD]);

    qtc_entry qe;

    qe.utc(fields[QSO_UTC_FIELD]);
    qe.callsign(fields[QSO_CALL_FIELD]);
    qe.serno(fields[QSO_SERNO_FIELD]);

    series += { qe, QTC_STATUS::SENT };
  }

// add the last series to the database
  if (!lines.empty())
    _qtc_db += series;
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
{ //const vector<QSO> qsos { logbk.as_vector() };

//  FOR_ALL(qsos, [this] (const QSO& qso) { (*this) += qso; } );
  FOR_ALL(logbk.as_vector(), [this] (const QSO& qso) { (*this) += qso; } );
}

/*! \brief          Add a QSO to the buffer
    \param  qso     QSO to add

    Does nothing if <i>qso</i> is already in the buffer (either as sent or unsent).
    Does nothing if the QSO is not with an EU station.
*/
void qtc_buffer::operator+=(const QSO& qso)
{ if (qso.continent() == "EU"sv)
  { if (const qtc_entry entry { qso }; !contains(_sent_qtcs, entry) and !contains(_unsent_qtcs, entry))
      _unsent_qtcs += entry;
  }
}

/*! \brief          Recreate the unsent list
    \param  logbk   logbook
*/
void qtc_buffer::rebuild_unsent_list(const logbook& logbk)
{ _unsent_qtcs.clear();

  *this += logbk;
}

/*! \brief                  Get a batch of QTC entries that may be sent to a particular destination
    \param  max_entries     maximum number of QTC entries to return
    \param  target          station to which the QTC entries are to be sent
    \return                 the sendable QTC entries
*/
vector<qtc_entry> qtc_buffer::get_next_unsent_qtc(const unsigned int max_entries, const string& target) const
{ vector<qtc_entry> rv;

  auto cit { _unsent_qtcs.cbegin() };

  while ( (rv.size() != max_entries) and (cit != _unsent_qtcs.cend()) )
  { if (cit->callsign() != target)
      rv += *cit;               // don't send a QTC back to the station of the original QSO    

    ++cit;
  }

  return rv;
}

/*! \brief          Transfer a <i>qtc_entry</i> from unsent status to sent status
    \param  entry   <i>qtc_entry</i> to transfer
*/
void qtc_buffer::unsent_to_sent(const qtc_entry& entry)
{ _unsent_qtcs -= entry;
  _sent_qtcs   += entry;
}

/*! \brief      Transfer all the (sent) entries in a <i>qtc_series</i> from unsent status to sent status
    \param  qs  QTC entries to transfer
*/
#if 0
void qtc_buffer::unsent_to_sent(const qtc_series& qs)
{ //const vector<qtc_entry> sent_qtc_entries { qs.sent_qtc_entries() };

  FOR_ALL(qs.sent_qtc_entries(), [this] (const qtc_entry& qe) { unsent_to_sent(qe); } );
}
#endif

/*! \brief      The unsent list in human-readable format
    \return     the unsent list as a string
*/
string qtc_buffer::unsent_list_as_string(void) const
{ string rv;

  size_t nr { 1 };

  for (const qtc_entry& qe : _unsent_qtcs)
  { rv += pad_string(to_string(nr), 4) + ": "s + qe.utc() + SPACE_STR + pad_string(qe.callsign(), 13, PAD::RIGHT) + SPACE_STR + qe.serno();

    if (nr++ != _unsent_qtcs.size())
      rv += EOL; 
  }

  return rv;
}
