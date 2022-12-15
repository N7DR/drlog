// $Id: log.cpp 213 2022-12-15 17:11:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   log.cpp

    Classes and functions related to the log
*/

#include "cabrillo.h"
#include "log.h"
#include "statistics.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;

pt_mutex _log_mutex { "LOG"s };            ///< mutex for the log

extern message_stream ost;      ///< for debugging, info
extern string VERSION;          ///< version string

// -----------  logbook  ----------------

/*! \class  logbook
    \brief  The log
*/

/*! \brief          Modify a passed QSO with a new value for a named field
    \param  qso     QSO to modify
    \param  name    name of the field to modify
    \param  value   the new value to give to field <i>name</i>
*/
void logbook::_modify_qso_with_name_and_value(QSO& qso, const string& name, const string& value)
{ 
// frequency
  if (name == "FREQ"sv)
    qso.freq_and_band(value);

// mode
  if (name == "MODE"sv)
  { if (value == "CW"sv)
      qso.mode(MODE_CW);

    if (value == "SSB"sv)
      qso.mode(MODE_SSB);
//    if (value == "RTTY")
//      qso.mode(MODE_DIGI);
  }

// date
  if (name == "DATE"sv)
    qso.date(value);

// time
  if (name == "TIME"sv)
    qso.utc( (value.length() == 5) ? (value.substr(0, 2) + value.substr(3, 2)) : value ); // handle hh:mm and hhmm formats

// tcall
  if (name == "TCALL"sv)
    qso.my_call(value);

// transmitted exchange
//  if (name.starts_with("TEXCH-"s))
//   qso.sent_exchange(qso.sent_exchange() + pair<string, string> { name.substr(6), value });    // remove "TEXCH-" before adding the field and value
  if (const string_view str { "TEXCH-"sv }; name.starts_with(str))
    qso.sent_exchange(qso.sent_exchange() + pair<string, string> { remove_from_start(name, str), value });    // remove "TEXCH-" before adding the field and value

// rcall
  if (name == "RCALL"sv)
    qso.callsign(value);

// received exchange
//  if (name.starts_with("REXCH-"s))
//    qso.received_exchange(qso.received_exchange() + received_field { name.substr(6), value, false, false });    // remove "REXCH-" before adding the field and value
  if (const string_view str { "TEXCH-"sv }; name.starts_with(str))
    qso.received_exchange(qso.received_exchange() + received_field { remove_from_start(name, str), value, false, false });    // remove "REXCH-" before adding the field and value
}

/*! \brief      Add a QSO to the logbook
    \param  q   QSO to add
*/
void logbook::operator+=(const QSO& q)
{ SAFELOCK(_log);

  _log += { q.callsign(), q };
  _log_vec += q;
}

/*! \brief      Return an individual QSO by number (wrt 1)
    \param  n   QSO number to return
    \return     the <i>n</i>th QSO

    If <i>n</i> is out of range, then returns an empty QSO
*/
QSO logbook::operator[](const size_t n) const
{ const static QSO empty_qso;

  SAFELOCK(_log);

  return ( (_log_vec.empty() or (_log_vec.size() < n)) ? empty_qso : _log_vec[n - 1] ) ;
}

/*! \brief      Remove an individual QSO by number (wrt 1)
    \param  n   QSO number to remove

    If <i>n</i> is out of range, then does nothing
*/
void logbook::operator-=(const unsigned int n)
{ SAFELOCK(_log);

  if (_log.empty() or (_log.size() < n))
    return;

  const size_t index { n - 1 };    // because the interface is wrt 1
  auto  it           { _log_vec.begin() + index };     // move to the correct QSO

  _log_vec.erase(it);
  _log.clear();              // empty preparatory to copying

  FOR_ALL(_log_vec, [this] (const QSO& qso) { _log += { qso.callsign(), qso }; } );     // rebuild the log
}

/*! \brief          All the QSOs with a particular call, in chronological order
    \param  call    target callsign
    \return         vector of QSOs in chronological order

    If there are no QSOs with <i>call</i>, returns an empty vector
*/
vector<QSO> logbook::worked(const string& call) const
{ vector<QSO> rv;

  { SAFELOCK(_log);
  
    for_each(_LB(call), _UB(call), [&rv] (const pair<string, QSO>& qso) { rv += qso.second; } );
  }

// https://www.youtube.com/watch?v=SYLgG7Q5Zws  39:40
  ranges::sort(rv, { }, [] (const QSO& q) { return q.epoch_time(); });    // use a projection to put in chronological order

  return rv;
}

/*! \brief          The number of times that a particular call has been worked
    \param  call    target callsign
    \return         number of times that <i>call</i> has been worked
*/
unsigned int logbook::n_worked(const string& call) const
{ SAFELOCK(_log);

  const auto range { _log.equal_range(call) };

  return distance(range.first, range.second);
}

/*! \brief          Has a call been worked on a particular band?
    \param  call    target callsign
    \param  b       target band
    \return         whether <i>call</i> has been worked on <i>b</i>
*/
bool logbook::qso_b4(const string& call, const BAND b) const
{ SAFELOCK(_log);
  
  return ANY_OF(_LB(call), _UB(call), [b] (const auto& pr) { return (pr.second.band() == b); });
}

/*! \brief          Has a call been worked on a particular mode?
    \param  call    target callsign
    \param  m       target mode
    \return         whether <i>call</i> has been worked on <i>m</i>
*/
bool logbook::qso_b4(const string& call, const enum MODE m) const
{ SAFELOCK(_log);

  return ANY_OF(_LB(call), _UB(call), [=] (const auto& pr) { return (pr.second.mode() == m); });
}

/*! \brief          Has a call been worked on a particular band and mode?
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         whether <i>call</i> has been worked on <i>b</i> and <i>m</i>
*/
bool logbook::qso_b4(const string& call, const BAND b, const enum MODE m) const
{ SAFELOCK(_log);

  return ANY_OF(_LB(call), _UB(call), [=] (const auto& pr) { return (pr.second.band() == b) and (pr.second.mode() == m); });
}

/*! \brief          Get a string list of bands on which a call is needed
    \param  call    target callsign
    \param  rules   rules for the contest
    \return         string list of bands on which a call is needed (separated by three spaces)
*/
string logbook::call_needed(const string& call, const contest_rules& rules) const
{ string rv;

  for (const auto& b : rules.permitted_bands())
    rv += ((qso_b4(call, b)) ? "   "s :  BAND_NAME[static_cast<int>(b)]);
  
  return rv;
}

/*! \brief          Would a QSO be a dupe, according to the rules?
    \param  qso     target QSO
    \param  rules   rules for the contest
    \return         whether <i>qso</i> would be a dupe
*/
bool logbook::is_dupe(const QSO& qso, const contest_rules& rules) const
{ bool rv { false };

  const string call { qso.call() };

  if (qso_b4(call))            // only check if we've worked this call before
  { 
// if we've worked on this band and mode, it is definitely a dupe
    const MODE m { qso.mode() };
    const BAND b { qso.band() };
  
    if (qso_b4(call, b, m))
      rv = true;

// it's a dupe if we've worked before on a different mode and we're not allowed to re-work
    if (!rv and !rules.work_if_different_band())
    { if (qso_b4(call, m))
        rv = true;
    }

// it's a dupe if we've worked before on a different mode and we're not allowed to re-work
    if (!rv and !rules.work_if_different_mode())
    { if (qso_b4(call, b))
        rv = true;
    }
  }

  return rv;
}

/*! \brief          Would a QSO be a dupe, according to the rules?
    \param  call    callsign
    \param  b       band
    \param  m       mode
    \param  rules   rules for the contest
    \return         whether a QSO with <i>call</i> on band <i>b</i> and mode <i>m</i> would be a dupe
*/
bool logbook::is_dupe(const string& call, const BAND b, const enum MODE m, const contest_rules& rules) const
{ QSO qso;

// create a bare-bones QSO
  qso.callsign(call);
  qso.band(b);
  qso.mode(m);
  
  return is_dupe(qso, rules);
}

/// return time-ordered list of qsos
list<QSO> logbook::as_list(void) const
{ list<QSO> rv;

  { SAFELOCK(_log);
  
    FOR_ALL(_log, [&rv] (const pair<string, QSO>& q) { rv += q.second; } );
  }

  rv.sort(earlier);    // sorts according to earlier(const QSO&, const QSO&); should be a NOP, though

  return rv;
}

/// return time-ordered vector of qsos
vector<QSO> logbook::as_vector(void) const
{ SAFELOCK(_log);

  return _log_vec;
}

/*! \brief          Recalculate the dupes
    \param  rules   rules for the contest
    \return         logbook with the dupes recalculated
*/
logbook logbook::recalculate_dupes(const contest_rules& rules) const
{ logbook rv;
  
  SAFELOCK(_log);

  for (auto qso : _log_vec)
  { auto fn_p { ( (rv.is_dupe(qso, rules)) ? &QSO::dupe : &QSO::undupe ) };  // find the correct function to apply

    (qso.*fn_p)();  // apply it
      
    rv += qso;
  }
  
  return rv;
}

/*! \brief              Generate a Cabrillo log
    \param  context     the drlog context
    \param  score       score to be claimed
    \return             the Cabrillo log
    
    The Cabrillo format is supposedly "specified" at:
      http://www.kkn.net/~trey/cabrillo/
        
    Just for the record, the "specification" at the above URL is grossly incomplete
    (even though the ARRL at http://www.arrl.org/contests/fileform.html
    explicitly claims that it is complete). But it *is* incomplete, and there are
    many areas in which an implementer has to to guess what to do.

    The format "specification" has been moved to:
      http://wwrof.org/cabrillo/
    It remains thoroughly deficient.
*/
string logbook::cabrillo_log(const drlog_context& context, const unsigned int score) const
{ string rv;

// define a separate EOL for the Cabrillo file because JARL rejects logs that don't use CRLF, even though the
// specification is silent as to what constitutes an EOL marker. The FAQ (why would a specification need
// an FAQ if it were properly specified?) implies that the EOL of the underlying OS on the system that creates
// the Cabrillo file should be accepted by the contest sponsor.
//  string EOL_STRING { LF };

  const string EOL_STRING { (context.cabrillo_eol() == "CR"sv) ? CR : ( (context.cabrillo_eol() == "CRLF"sv) ? CRLF : LF ) };

// this goes first
  rv += "START-OF-LOG: 3.0"s + EOL_STRING;
  
// call
  rv += "CALLSIGN: "s + context.cabrillo_callsign() + EOL_STRING;
  
// contest
  rv += "CONTEST: "s + context.cabrillo_contest() + EOL_STRING;
  
// drlog name / version
  rv += "CREATED-BY: drlog version "s + VERSION + EOL_STRING;
  
// name of operator
  if (!context.cabrillo_name().empty())
    rv += "NAME: "s + context.cabrillo_name() + EOL_STRING;

// address lines http://www.kkn.net/~trey/cabrillo/tags.html: "A maximum of four ADDRESS: lines is permitted."
  if (!context.cabrillo_address_1().empty())
  { rv += "ADDRESS: "s + context.cabrillo_address_1() + EOL_STRING;

    if (!context.cabrillo_address_2().empty())
    { rv += "ADDRESS: "s + context.cabrillo_address_2() + EOL_STRING;

      if (!context.cabrillo_address_3().empty())
      { rv += "ADDRESS: "s + context.cabrillo_address_3() + EOL_STRING;

        if (!context.cabrillo_address_4().empty())
          rv += "ADDRESS: "s + context.cabrillo_address_4() + EOL_STRING;
      }
    }
  }
 
// address city
  if (!context.cabrillo_address_city().empty())
    rv += "ADDRESS-CITY: "s + context.cabrillo_address_city() + EOL_STRING;
  
// address state/province
  if (!context.cabrillo_address_state_province().empty())
    rv += "ADDRESS-STATE-PROVINCE: "s + context.cabrillo_address_state_province() + EOL_STRING;
  
// address postcode
  if (!context.cabrillo_address_postalcode().empty())
    rv += "ADDRESS-POSTALCODE: "s + context.cabrillo_address_postalcode() + EOL_STRING;
  
// address country
  if (!context.cabrillo_address_country().empty())
    rv += "ADDRESS-COUNTRY: "s + context.cabrillo_address_country() + EOL_STRING;
  
// list of operators
  if (!context.cabrillo_operators().empty())
    rv += "OPERATORS: "s + context.cabrillo_operators() + EOL_STRING;
  
// Categories
// assisted
  rv += "CATEGORY-ASSISTED: "s + context.cabrillo_category_assisted() + EOL_STRING;
 
// band
  rv += "CATEGORY-BAND: "s + context.cabrillo_category_band() + EOL_STRING;

// mode
  rv += "CATEGORY-MODE: "s + context.cabrillo_category_mode() + EOL_STRING;

// operator
  rv += "CATEGORY-OPERATOR: "s + context.cabrillo_category_operator() + EOL_STRING;

// overlay
  if (!context.cabrillo_category_overlay().empty())
    rv += "CATEGORY-OVERLAY: "s + context.cabrillo_category_overlay() + EOL_STRING;

// power
  rv += "CATEGORY-POWER: "s + context.cabrillo_category_power() + EOL_STRING;

// station
  if (!context.cabrillo_category_station().empty())
    rv += "CATEGORY-STATION: "s + context.cabrillo_category_station() + EOL_STRING;

// time
  if (!context.cabrillo_category_time().empty())
    rv += "CATEGORY-TIME: "s + context.cabrillo_category_time() + EOL_STRING;

// transmitter
  rv += "CATEGORY-TRANSMITTER: "s + context.cabrillo_category_transmitter() + EOL_STRING;

// club
  if (!context.cabrillo_club().empty())
    rv += "CLUB: "s + context.cabrillo_club() + EOL_STRING;

// location
  if (!context.cabrillo_location().empty())
    rv += "LOCATION: "s + context.cabrillo_location() + EOL_STRING;
    
// e-mail
  if (!context.cabrillo_e_mail().empty())
    rv += "EMAIL: "s + context.cabrillo_e_mail() + EOL_STRING;

// claimed score
  if (context.cabrillo_include_score())
    rv += "CLAIMED-SCORE: "s + to_string(score) + EOL_STRING;

// certificate
    rv += "CERTIFICATE: "s + context.cabrillo_certificate() + EOL_STRING;

/* the QSOs. The Cabrillo "specification" provides not even a semblance of a computer-parsable
   grammar for QSOs, so a lot of this is guesswork.
   
  CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R TXID:81:1 
*/
  
// generate time-ordered container
  const string    cabrillo_qso_template { context.cabrillo_qso_template() };
  
  FOR_ALL(as_list(), [&] (const QSO& q) { rv += q.cabrillo_format(cabrillo_qso_template) + EOL_STRING; } );
 
// soapbox
  rv += "SOAPBOX: "s + EOL_STRING;
 
// this goes at the end
  rv += "END-OF-LOG:"s + EOL_STRING;
  
  return rv;
}

/*! \brief                          Read from a Cabrillo file
    \param  filename                name of Cabrillo file
    \param  cabrillo_qso_template   template for the Cabrillo QSOs
*/
void logbook::read_cabrillo(const string& filename, const string& cabrillo_qso_template)
{ static const vector<string> qso_markers { "QSO"s, "   "s };   // lines that represent QSOs start with one of these strings

  const vector<string> lines { to_lines(remove_char(read_file(filename), CR_CHAR)) };
  
/*
  CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1 
*/
  
  vector< vector< string> > individual_values;

  const vector<string> template_fields { clean_split_string(cabrillo_qso_template) };         // colon-delimited values
  
  for (unsigned int n { 0 }; n < template_fields.size(); ++n)
    individual_values += split_string(template_fields[n], ':');

  unsigned int last_qso_number { 0 };
   
// for each QSO line
  for (const auto& line : lines)
  { if (starts_with(line, qso_markers))
    { QSO qso;
  
// go through the fields
      for (unsigned int n { 0 }; n < individual_values.size(); ++n)
      { const vector<string>& vec   { individual_values[n] };
        const string&         name  { vec[0] };
        const unsigned int    posn  { from_string<unsigned int>(vec[1]) - 1 };
        const unsigned int    len   { from_string<unsigned int>(vec[2]) };
        const string          value { ( line.length() >= (posn + 1) ? remove_peripheral_spaces(line.substr(posn, len)) : string()) };
      
        _modify_qso_with_name_and_value(qso, name, value);

// don't do TXID since we don't really need that (and there's currently nowhere in QSO to put it)
      }

      qso.number(++last_qso_number);
      *this += qso;
    }
  }
}

/*! \brief                      Read from a Cabrillo file, using space-delimited fields
    \param  filename            name of Cabrillo file
    \param  cabrillo_fields     names of Cabrillo fields
*/
void logbook::read_cabrillo(const string& filename, const vector<string>& cabrillo_fields)
{ static const vector<string> qso_markers { "QSO"s, "   "s };   // lines that represent QSOs start with one of these strings

  const vector<string> lines { to_lines(remove_char(read_file(filename), CR_CHAR)) };

  unsigned int last_qso_number { 0 };

  for (const auto& line : lines)
  { if (starts_with(line, qso_markers))
    { QSO qso;
  
      const vector<string> fields { split_string(squash(line.substr(4)), ' ') }; // skip first four characters
      
      for (unsigned int m { 0 }; m < fields.size(); ++m)
        ost << m << ": *" << fields[m] << "*" << endl; 

// go through the fields
      for (unsigned int n { 0 }; n < cabrillo_fields.size(); ++n)
      { const string name  { cabrillo_fields[n] };
        const string value { (n < fields.size() ? remove_peripheral_spaces(fields[n]) : string()) };

        _modify_qso_with_name_and_value(qso, name, value);

// don't do TXID since we don't really need that (and there's currently nowhere in QSO to put it)
      }

      qso.number(++last_qso_number);
      *this += qso;
    }
  }
}

/*! \brief                          Get the value of an exchange field from the last QSO with a station
    \param  callsign                target call
    \param  exchange_field_name     name of target exchange field
    \return                         value of the most recent instance of the exchange field <i>exchange_field_name</i> received from <i>callsign</i>

    Returns empty string if no returnable instance can be found
*/
string logbook::exchange_field_value(const string& callsign, const string& exchange_field_name)
{ const vector<QSO> qsos { worked(callsign) };

  if (!qsos.empty())
  { const QSO&   last_qso          { qsos[qsos.size() - 1] };
    const string received_exchange { last_qso.received_exchange(exchange_field_name) };

    if (!received_exchange.empty())
      return received_exchange;
  }

  return string();
}

/*! \brief          Return all the QSOs that contain an exchange field that matches a target
    \param  target  target string for exchange fields
    \return         All the QSOs that contain an exchange field that matches a target

    If <i>n</i> is out of range, then returns an empty vector
*/
vector<QSO> logbook::match_exchange(const string& target) const
{ vector<QSO> rv;

  FOR_ALL(_log_vec, [&target, &rv] (const auto& qso) { if (qso.exchange_match_string(target)) rv += qso; });

  return rv;
}

/*! \brief                  Remove several recent QSOs
    \param  n_to_remove     number of QSOs to remove

    It is legal to call this function even if <i>n_to_remove</i> is greater than
    the number of QSOs in the logbook
*/
void logbook::remove_last_qsos(const unsigned int n_to_remove)
{ for (unsigned int n { 0 }; n < n_to_remove; ++n)
    remove_last_qso();
}

/*! \brief      Remove most-recent qso
    \return     the removed QSO

    Does nothing and returns an empty QSO if there are no QSOs in the log
*/
QSO logbook::remove_last_qso(void)
{ if (empty())
    return QSO();

  SAFELOCK(_log);

  const QSO rv { _log_vec[size() - 1] };

  *this -= size();    // remember, numbering is wrt 1

  return rv;
}

/*! \brief          Return all the calls in the log
    \return         all the calls in the log
*/
set<string> logbook::calls(void) const
{ SAFELOCK(_log);

  set<string> rv { };

// https://stackoverflow.com/questions/11554932/how-can-i-get-all-the-unique-keys-in-a-multimap
  for (auto it { _log.cbegin() }; it != _log.cend(); it = _log.upper_bound(it->first))
    rv += it->first;

  return rv;
}

/*! \brief      Return the most recent EU call worked
    \return     the most recently worked EU call

    Returns the empty string if no European calls are in the log
*/
string logbook::last_worked_eu_call(void) const
{ static const string_view EU { "EU"sv };

  SAFELOCK(_log);

  const auto cit { find_if(_log_vec.rbegin(), _log_vec.rend(), [] (const QSO& q) { return (q.continent() == EU); } ) };

  return ( (cit == _log_vec.rend()) ? string { } : cit->callsign() );
}

// -----------  log_extract  ----------------

/*! \class  log_extract
    \brief  Support for bits of the log
*/

/*! \brief          Add a QSO to the extract
    \param  qso     QSO to add

    Auto resizes the extract by removing old QSOs so that it does not exceed the window size
*/
void log_extract::operator+=(const QSO& qso)
{ SAFELOCK(_extract);

  _qsos += qso;

  while (_qsos.size() > _win_size)
    --_qsos;
}

/*! \brief  Display the extract in the associated window

    Displayed in order from oldest to newest. If the extract contains more QSOs than the window
    allows, only the most recent QSOs are displayed.
*/
void log_extract::display(void)
{ vector<QSO> vec;

  { SAFELOCK(_extract);

    ranges::copy(_qsos, back_inserter(vec));
  }

  if (vec.size() < _win_size)
    _win < WINDOW_ATTRIBUTES::WINDOW_CLEAR;

  if (!vec.empty())
  { const size_t n_to_display { min(vec.size(), _win_size) };

    for (size_t n { 0 }; n < n_to_display; ++n)
    { const size_t index { vec.size() - 1 - n };              // write so that most recent is at the bottom

      _win < cursor(0, n) < WINDOW_ATTRIBUTES::WINDOW_CLEAR_TO_EOL < cursor(0, n) < vec[index].log_line();
    }
  }

  _win < WINDOW_ATTRIBUTES::WINDOW_REFRESH;
}

/*! \brief              Get recent QSOs from a log, and possibly display them
    \param  lgbook      logbook to use
    \param  to_display  whether to display the extract

    Displayed in order from oldest to newest
*/
void log_extract::recent_qsos(const logbook& lgbook, const bool to_display)
{ const vector<QSO>& vec       { lgbook.as_vector() };
  const size_t       n_to_copy { min(vec.size(), _win_size) };

  clear();    // empty the container of QSOs

// extract the QSOs
  for (size_t n { 0 }; n < n_to_copy; ++n)
  	(*this) += vec.at(vec.size() - n_to_copy + n);

  if (to_display)
  	display();
}

/*! \brief          Get the QSOs that match an exchange from a log, and display them
    \param  lgbook  logbook to use
    \param  target  string to match in the QSO exchanges

    Displayed in order from oldest to newest. If the extract contains more QSOs than the window
    allows, only the most recent QSOs are displayed.
*/
void log_extract::match_exchange(const logbook& lgbook, const string& target)
{ const vector<QSO> vec       { lgbook.match_exchange(target) };
  const size_t      n_to_copy { min(vec.size(), _win_size) };

  clear();    // empty the container of QSOs

  for (size_t n { 0 }; n < n_to_copy; ++n)
    (*this) += vec.at(vec.size() - n_to_copy + n);

  display();
}

// -----------  old_log  ----------------

/*! \class  old_log
    \brief  An old ADIF3 log

    Not thread safe, so create once and then never change.
*/

/*! \brief          Return an iterator to the data for a particular callsign
    \param  call    callsign
    \return         An iterator to the data for <i>call</i>

    The data for <i>call</i> are created if they don't already exist, and the corresponding iterator returned
*/
auto old_log::_find_or_create(const string& call) -> decltype(_olog)::iterator
{ auto it { _olog.find(call) };

  return ( (it == _olog.end()) ? (_olog[call], _olog.find(call)) : it );    // Josuttis, 2 ed. p.186 implies that this works rather than _olog[call] = { };
}

/*! \brief          Return total number of QSLs from a particular callsign
    \param  call    callsign
    \return         the number of QSLs from callsign <i>call</i>
*/
unsigned int old_log::n_qsls(const string& call) const
{ const auto cit { _olog.find(call) };

  return (cit == _olog.cend() ? 0 : get<0>(cit->second));
}

/*! \brief          Increment the number of QSLs from a particular callsign
    \param  call    callsign
    \return         the new number of QSLs from callsign <i>call</i>
*/
unsigned int old_log::increment_n_qsls(const string& call)
{ auto it { _olog.find(call) };

  if (it == _olog.end())    // never worked before; => current number of QSLs must be zero
  { _olog[call];
    n_qsls(call, 1);

    return 1;
  }

  const unsigned int rv { n_qsls(call) + 1 };

  get<0>(it->second) = rv;

  return rv;
}

/*! \brief          Return total number of QSOs with a particular callsign
    \param  call    callsign
    \return         the number of QSOs with callsign <i>call</i>
*/
unsigned int old_log::n_qsos(const string& call) const
{ const auto cit { _olog.find(call) };

  return (cit == _olog.cend() ? 0 : get<1>(cit->second));
}

/*! \brief          increment the number of QSOs associated with a particular callsign
    \param  call    callsign for which the number of QSOs should be incremented
    \return         number of QSOs associated with with <i>call</i>
*/
unsigned int old_log::increment_n_qsos(const string& call)
{ const auto it { _olog.find(call) };

  if (it == _olog.end())    // no prior QSOs with this call
  { _olog[call];
    n_qsos(call, 1);

    return 1;
  }

  const unsigned int rv { n_qsos(call) + 1 };

  get<1>(it->second) = rv;

  return rv;
}

/*! \brief          How many QSOs have taken place with a particular call on a a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         number of QSOs associated with with <i>call</i> on band <i>b</i> and mode <i>m</i>
*/
unsigned int old_log::n_qsos(const string& call, const BAND b, const MODE m) const
{ const auto cit { _olog.find(call) };

  return (cit == _olog.cend() ? 0 : get<3>(cit->second).count( { b, m } ));
}

/*! \brief          Increment the number of QSOs associated with a particular callsign, band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         number of QSOs associated with with <i>call</i> on band <i>b</i> and mode <i>m</i> (following the increment)
*/
unsigned int old_log::increment_n_qsos(const string& call, const BAND b, const MODE m)
{ get<3>(_find_or_create(call) -> second) += { b, m };

  return n_qsos(call, b, m);
}

/*! \brief          Has a QSL ever been received for a particular call on a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         Has a QSL ever been received for a QSO with <i>call</i> on band <i>b</i> and mode <i>m</i>
*/
bool old_log::confirmed(const string& call, const BAND b, const MODE m) const
{ const auto cit { _olog.find(call) };

  return ( cit == _olog.cend() ? false : get<2>(cit->second) > pair<BAND, MODE>( { b, m } ) );
}
