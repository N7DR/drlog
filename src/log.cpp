// $Id: log.cpp 149 2019-01-03 19:24:01Z  $

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

pt_mutex _log_mutex;            ///< mutex for the log

extern message_stream ost;      ///< for debugging, info
extern string VERSION;          ///< version string

// -----------  logbook  ----------------

/*! \class  logbook
    \brief  The log
*/

/*! \brief      Is one QSO earlier than another?
    \param  q1  first QSO
    \param  q2  second QSO
    \return     whether <i>q1</i> is earlier than <i>q2</i>
*/
const bool qso_sort_by_time(const QSO& q1, const QSO& q2)
  { return q1.earlier_than(q2); }

/*! \brief      Add a QSO to the logbook
    \param  q   QSO to add
*/
void logbook::operator+=(const QSO& q)
{ SAFELOCK(_log);

  _log.insert( { q.callsign(), q } );
  _log_vec.push_back(q);
}

/*! \brief      Return an individual QSO by number (wrt 1)
    \param  n   QSO number to return
    \return     the <i>n</i>th QSO

    If <i>n</i> is out of range, then returns an empty QSO
*/
const QSO logbook::operator[](const size_t n) const
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

  const size_t index = n - 1;    // because the interface is wrt 1

  auto it = _log_vec.begin();

  advance(it, index);            // move to the correct QSO

  _log_vec.erase(it);
  _log.clear();              // empty preparatory to copying

  FOR_ALL(_log_vec, [&](const QSO& qso) { _log.insert( { qso.callsign(), qso } ); } );
}

/*! \brief          All the QSOs with a particular call, in chronological order
    \param  call    target callsign
    \return         vector of QSOs in chronological order

    If there are no QSOs with <i>call</i>, returns an empty vector
*/
const vector<QSO> logbook::worked(const string& call) const
{ vector<QSO> rv;

  { SAFELOCK(_log);
  
    for_each(_log.lower_bound(call), _log.upper_bound(call), [&rv] (const pair<string, QSO>& qso) { rv.push_back(qso.second); } );
  }

  sort(rv.begin(), rv.end(), qso_sort_by_time);    // put in chronological order

  return rv;
}

/*! \brief          The number of times that a particular call has been worked
    \param  call    target callsign
    \return         number of times that <i>call</i> has been worked
*/
const unsigned int logbook::n_worked(const string& call) const
{ SAFELOCK(_log);

  const auto range = _log.equal_range(call);

  return distance(range.first, range.second);
}

/*! \brief          Has a call been worked on a particular band?
    \param  call    target callsign
    \param  b       target band
    \return         whether <i>call</i> has been worked on <i>b</i>
*/
const bool logbook::qso_b4(const string& call, const BAND b) const
{ SAFELOCK(_log);
  
  for (auto cit = _log.lower_bound(call); cit != _log.upper_bound(call); ++cit)
    if (cit->second.band() == b)
      return true;
  
  return false;
}

/*! \brief          Has a call been worked on a particular mode?
    \param  call    target callsign
    \param  m       target mode
    \return         whether <i>call</i> has been worked on <i>m</i>
*/
const bool logbook::qso_b4(const string& call, const enum MODE m) const
{ SAFELOCK(_log);
  
  for (auto cit = _log.lower_bound(call); cit != _log.upper_bound(call); ++cit)
    if (cit->second.mode() == m)
      return true;
  
  return false;
}

/*! \brief          Has a call been worked on a particular band and mode?
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         whether <i>call</i> has been worked on <i>b</i> and <i>m</i>
*/
const bool logbook::qso_b4(const string& call, const BAND b, const enum MODE m) const
{ SAFELOCK(_log);
  
  for (auto cit = _log.lower_bound(call); cit != _log.upper_bound(call); ++cit)
    if ((cit->second.band() == b) and (cit->second.mode() == m))
      return true;
  
  return false;
}

/*! \brief          Get a string list of bands on which a call is needed
    \param  call    target callsign
    \param  rules   rules for the contest
    \return         string list of bands on which a call is needed (separated by three spaces)
*/
const string logbook::call_needed(const string& call, const contest_rules& rules) const
{ string rv;

  for (const auto& b : rules.permitted_bands())
    rv += ((qso_b4(call, b)) ? "   " :  BAND_NAME[static_cast<int>(b)]);
  
  return rv;
}

/*! \brief          Would a QSO be a dupe, according to the rules?
    \param  qso     target QSO
    \param  rules   rules for the contest
    \return         whether <i>qso</i> would be a dupe
*/
const bool logbook::is_dupe(const QSO& qso, const contest_rules& rules) const
{ bool rv = false;

  const string& call = qso.call();

  if (qso_b4(call))            // only check if we've worked this call before
  { 
// if we've worked on this band and mode, it is definitely a dupe
    const enum MODE& m = qso.mode();
    const BAND& b = qso.band();
  
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
const bool logbook::is_dupe(const string& call, const BAND b, const enum MODE m, const contest_rules& rules) const
{ QSO qso;

// create a bare-bones QSO
  qso.callsign(call);
  qso.band(b);
  qso.mode(m);
  
  return is_dupe(qso, rules);
}

/// return time-ordered list of qsos
const list<QSO> logbook::as_list(void) const
{ list<QSO> rv;

  { SAFELOCK(_log);
  
    FOR_ALL(_log, [&rv] (const pair<string, QSO>& q) { rv.push_back(q.second); } );
  }

  rv.sort(earlier);    // sorts according to earlier(const QSO&, const QSO&)

  return rv;
}

/// return time-ordered vector of qsos
const vector<QSO> logbook::as_vector(void) const
{ SAFELOCK(_log);

  return _log_vec;
}

/*! \brief          Recalculate the dupes
    \param  rules   rules for the contest
    \return         logbook with the dupes recalculated
*/
const logbook logbook::recalculate_dupes(const contest_rules& rules) const
{ logbook rv;
  
  SAFELOCK(_log);

  for (auto qso : _log_vec)
  { if (rv.is_dupe(qso, rules))
      qso.dupe();
    else
      qso.undupe();
      
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
const string logbook::cabrillo_log(const drlog_context& context, const unsigned int score) const
{ string rv;

// define a separate EOL for the Cabrillo file because JARL rejects logs that don't use CRLF, even though the
// specification is silent as to what constitutes an EOL marker. The FAQ (why would a specification need
// an FAQ if it were properly specified?) implies that the EOL of the underlying OS on the system that creates
// the Cabrillo file should be accepted by the contest sponsor.
  string EOL_STRING = LF;

  if (context.cabrillo_eol() == "CR")
    EOL_STRING = CR;

  if (context.cabrillo_eol() == "CRLF")
    EOL_STRING = CRLF;

// this goes first
  rv += "START-OF-LOG: 3.0" + EOL_STRING;
  
// call
  rv += "CALLSIGN: " + context.cabrillo_callsign() + EOL_STRING;
  
// contest
  rv += "CONTEST: " + context.cabrillo_contest() + EOL_STRING;
  
// drlog name / version
  rv += "CREATED-BY: drlog version " + VERSION + EOL_STRING;
  
// name of operator
  if (context.cabrillo_name() != string())
    rv += "NAME: " + context.cabrillo_name() + EOL_STRING;

// address lines http://www.kkn.net/~trey/cabrillo/tags.html: "A maximum of four ADDRESS: lines is permitted."
  if (context.cabrillo_address_1() != string())
  { rv += "ADDRESS: " + context.cabrillo_address_1() + EOL_STRING;

    if (context.cabrillo_address_2() != string())
    { rv += "ADDRESS: " + context.cabrillo_address_2() + EOL_STRING;

      if (context.cabrillo_address_3() != string())
      { rv += "ADDRESS: " + context.cabrillo_address_3() + EOL_STRING;

        if (context.cabrillo_address_4() != string())
          rv += "ADDRESS: " + context.cabrillo_address_4() + EOL_STRING;
      }
    }
  }
 
// address city
  if (context.cabrillo_address_city() != string())
    rv += "ADDRESS-CITY: " + context.cabrillo_address_city() + EOL_STRING;
  
// address state/province
  if (context.cabrillo_address_state_province() != string())
    rv += "ADDRESS-STATE-PROVINCE: " + context.cabrillo_address_state_province() + EOL_STRING;
  
// address postcode
  if (context.cabrillo_address_postalcode() != string())
    rv += "ADDRESS-POSTALCODE: " + context.cabrillo_address_postalcode() + EOL_STRING;
  
// address country
  if (context.cabrillo_address_country() != string())
    rv += "ADDRESS-COUNTRY: " + context.cabrillo_address_country() + EOL_STRING;
  
// list of operators
  if (context.cabrillo_operators() != string())
    rv += "OPERATORS: " + context.cabrillo_operators() + EOL_STRING;
  
// Categories
// assisted
  rv += "CATEGORY-ASSISTED: " + context.cabrillo_category_assisted() + EOL_STRING;
 
// band
  rv += "CATEGORY-BAND: " + context.cabrillo_category_band() + EOL_STRING;

// mode
  rv += "CATEGORY-MODE: " + context.cabrillo_category_mode() + EOL_STRING;

// operator
  rv += "CATEGORY-OPERATOR: " + context.cabrillo_category_operator() + EOL_STRING;

// overlay
  if (context.cabrillo_category_overlay() != string())
    rv += "CATEGORY-OVERLAY: " + context.cabrillo_category_overlay() + EOL_STRING;

// power
  rv += "CATEGORY-POWER: " + context.cabrillo_category_power() + EOL_STRING;

// station
  if (context.cabrillo_category_station() != string())
    rv += "CATEGORY-STATION: " + context.cabrillo_category_station() + EOL_STRING;

// time
  if (context.cabrillo_category_time() != string())
    rv += "CATEGORY-TIME: " + context.cabrillo_category_time() + EOL_STRING;

// transmitter
  rv += "CATEGORY-TRANSMITTER: " + context.cabrillo_category_transmitter() + EOL_STRING;

// club
  if (context.cabrillo_club() != string())
    rv += "CLUB: " + context.cabrillo_club() + EOL_STRING;

// location
  if (context.cabrillo_location() != string())
    rv += "LOCATION: " + context.cabrillo_location() + EOL_STRING;
    
// e-mail
  if (context.cabrillo_e_mail() != string())
    rv += "EMAIL: " + context.cabrillo_e_mail() + EOL_STRING;

// claimed score
  if (context.cabrillo_include_score())
    rv += "CLAIMED-SCORE: " + to_string(score) + EOL_STRING;

// certificate
    rv += "CERTIFICATE: " + context.cabrillo_certificate() + EOL_STRING;

/* the QSOs. The Cabrillo "specification" provides not even a semblance of a computer-parsable
   grammar for QSOs, so a lot of this is guesswork.
   
  CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R TXID:81:1 
*/
  
// generate time-ordered container
  const list<QSO> qsos = as_list();
  const string cabrillo_qso_template = context.cabrillo_qso_template();
  
  FOR_ALL(qsos, [&] (const QSO& q) { rv += q.cabrillo_format(cabrillo_qso_template) + EOL_STRING; } );
  
// soapbox
  rv += "SOAPBOX: " + EOL_STRING;
 
// this goes at the end
  rv += "END-OF-LOG:" + EOL_STRING;
  
  return rv;
}

/*! \brief                          Read from a Cabrillo file
    \param  filename                name of Cabrillo file
    \param  cabrillo_qso_template   template for the Cabrillo QSOs
*/
void logbook::read_cabrillo(const string& filename, const string& cabrillo_qso_template)
{ string file_contents = remove_char(read_file(filename), '\r');
  const vector<string> lines = to_lines(file_contents);
  
/*
  CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1 
*/
  
  vector< vector< string> > individual_values;
  const vector<string> template_fields = split_string(cabrillo_qso_template, ",");         // colon-delimited values
  
  for (unsigned int n = 0; n < template_fields.size(); ++n)
    individual_values.push_back(split_string(remove_peripheral_spaces(template_fields[n]), ":"));

  unsigned int last_qso_number = 0; 
   
// for each QSO line
  for (const auto& line : lines)
  { if (line.substr(0, 3) == "QSO" or line.substr(0, 3) == "   ")
    { QSO qso;
  
// go through the fields
      for (unsigned int n = 0; n < individual_values.size(); ++n)
      { const vector<string>& vec = individual_values[n];
        const string name = vec[0];
        const unsigned int posn = from_string<unsigned int>(vec[1]) - 1;
        const unsigned int len  = from_string<unsigned int>(vec[2]);
        const string value = (line.length() >= posn + 1 ? remove_peripheral_spaces(line.substr(posn, len)) : "");
      
// frequency
        if (name == "FREQ")
        { qso.freq(value);
    
          const unsigned int _frequency = from_string<unsigned int>(value);
          const BAND _band = static_cast<BAND>(frequency(_frequency));

          qso.band(_band);
        }
      
// mode
        if (name == "MODE")
        { if (value == "CW")
            qso.mode(MODE_CW);

          if (value == "SSB")
            qso.mode(MODE_SSB);
//    if (value == "RTTY")
//      qso.mode(MODE_DIGI);
        }

// date
        if (name == "DATE")
          qso.date(value);

// time
        if (name == "TIME")
        { if (value.length() == 5)
            qso.utc(value.substr(0, 2) + value.substr(3, 2));    // handle hh:mm format
          else
            qso.utc(value);                                      // hhmm
        }

// tcall
        if (name == "TCALL")
          qso.my_call(value);
    
// transmitted exchange
        if (name.substr(0, 5) == "TEXCH")
        { const string field_name = name.substr(6);
          vector<pair<string, string> > current_sent_exchange = qso.sent_exchange(); // do in two steps in order to remove constness of returned value
  
          qso.sent_exchange((current_sent_exchange.push_back( { field_name, value } ), current_sent_exchange));
        }

// rcall
        if (name == "RCALL")
          qso.callsign(value);

// received exchange
        if (name.substr(0, 5) == "REXCH")
        { const string field_name = name.substr(6);
          vector<received_field> current_received_exchange = qso.received_exchange(); // do in two steps in order to remove constness of returned value

// should have a function in the QSO class to add a field to the exchange
          current_received_exchange.push_back( { field_name, value, false, false });
          qso.received_exchange(current_received_exchange);
        }

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
{ const vector<string> lines = to_lines(remove_char(read_file(filename), CR_CHAR));

  unsigned int last_qso_number = 0; 

  for (const auto& line : lines)
  { if (line.substr(0, 3) == "QSO" or line.substr(0, 3) == "   ")
    { QSO qso;
  
      const vector<string> fields = split_string(remove_peripheral_spaces(squash(line.substr(4))), " ");
      
      for (unsigned int m = 0; m < fields.size(); ++m)
        ost << m << ": *" << fields[m] << "*" << endl; 

// go through the fields
      for (unsigned int n = 0; n < cabrillo_fields.size(); ++n)
      { const string name = cabrillo_fields[n];
        const string value = (n < fields.size() ? remove_peripheral_spaces(fields[n]) : "");

// frequency
        if (name == "FREQ")
        { qso.freq(value);
    
          unsigned int _frequency = from_string<unsigned int>(value);
          const BAND _band = to_BAND(_frequency);

          qso.band(_band);
        }

// mode
        if (name == "MODE")
        { if (value == "CW")
            qso.mode(MODE_CW);

          if (value == "SSB")
            qso.mode(MODE_SSB);
//    if (value == "RTTY")
//      qso.mode(MODE_DIGI);
        }

// date
        if (name == "DATE")
          qso.date(value);

// time
        if (name == "TIME")
        { if (value.find(":") == 2)
            qso.utc(value.substr(0, 2) + value.substr(3, 2));
          else
            qso.utc(value);
        }

// tcall
        if (name == "TCALL")
          qso.my_call(value);
    
// transmitted exchange
        if (name.substr(0, 5) == "TEXCH")
        { const string field_name = name.substr(6);
          vector<pair<string, string> > current_sent_exchange = qso.sent_exchange(); // do in two steps in order to remove constness of returned value
  
          qso.sent_exchange((current_sent_exchange.push_back( { field_name, value } ), current_sent_exchange));
        }

// rcall
        if (name == "RCALL")
          qso.callsign(value);

// received exchange
        if (name.substr(0, 5) == "REXCH")
        { const string field_name = name.substr(6);
          vector<received_field> current_received_exchange = qso.received_exchange(); // do in two steps in order to remove constness of returned value

// should have a function in the QSO class to add a field to the exchange
          current_received_exchange.push_back( { field_name, value, false, false } );
          qso.received_exchange(current_received_exchange);
        }

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
const string logbook::exchange_field_value(const string& callsign, const string& exchange_field_name)
{ const vector<QSO> qsos = worked(callsign);

  if (!qsos.empty())
  { const QSO& last_qso = qsos[qsos.size() - 1];
    const string received_exchange = last_qso.received_exchange(exchange_field_name);

    if (!received_exchange.empty())
      return received_exchange;
  }

  return string();
}

/*! \brief          Return all the QSOs that contain an exchange field that matches a target
    \param  target  target string for exchange fields
    \return         All the QSOs that contain an exchange field that matches a target

    If <i>n</i> is out of range, then returns an empty QSO
*/
const vector<QSO> logbook::match_exchange(const string& target) const
{ vector<QSO> rv;

  for (const auto& qso : _log_vec)
    if (qso.exchange_match_string(target))
      rv.push_back(qso);

  return rv;
}

/*! \brief                  Remove several recent QSOs
    \param  n_to_remove     number of QSOs to remove

    It is legal to call this function even if <i>n_to_remove</i> is greater than
    the number of QSOs in the logbook
*/
void logbook::remove_last_qsos(const unsigned int n_to_remove)
{ for (unsigned int n = 0; n < n_to_remove; ++n)
    remove_last_qso();
}

/*! \brief      Remove most-recent qso
    \return     the removed QSO

    Does nothing and returns an empty QSO if there are no QSOs in the log
*/
const QSO logbook::remove_last_qso(void)
{ if (empty())
    return QSO();

  SAFELOCK(_log);

  const QSO rv = _log_vec[size() - 1];

  *this -= size();    // remember, numbering is wrt 1

  return rv;
}

// -----------  log_extract  ----------------

/*! \class  log_extract
    \brief  Support for bits of the log
*/

/*! \brief  constructor
    \param  w               window to be used by this extract
*/
//log_extract::log_extract(window& w) :
//  _win(w),
//  _win_size(0)                                  // don't set the size yet, since the size of w may not be set
//{ }

/*! \brief          Add a QSO to the extract
    \param  qso     QSO to add

    Auto resizes the extract by removing old QSOs so that it does not exceed the window size
*/
void log_extract::operator+=(const QSO& qso)
{ SAFELOCK(_extract);

  _qsos.push_back(qso);

  while (_qsos.size() > _win_size)
    _qsos.pop_front();
}

/*! \brief  Display the extract in the associated window

    Displayed in order from oldest to newest. If the extract contains more QSOs than the window
    allows, only the most recent QSOs are displayed.
*/
void log_extract::display(void)
{ vector<QSO> vec;

  { SAFELOCK(_extract);

    copy(_qsos.cbegin(), _qsos.cend(), back_inserter(vec));
  }

  if (vec.size() < _win_size)
    _win < WINDOW_CLEAR;

  if (!vec.empty())
  { const size_t n_to_display = min(vec.size(), _win_size);

    for (size_t n = 0; n < n_to_display; ++n)
    { const size_t index = vec.size() - 1 - n;              // write so that most recent is at the bottom

      _win < cursor(0, n) < vec[index].log_line();
    }
  }

  _win < WINDOW_REFRESH;
}

/*! \brief              Get recent QSOs from a log, and possibly display them
    \param  lgbook      logbook to use
    \param  to_display  whether to display the extract

    Displayed in order from oldest to newest
*/
void log_extract::recent_qsos(const logbook& lgbook, const bool to_display)
{ const vector<QSO>& vec = lgbook.as_vector();
  const size_t n_to_copy = min(vec.size(), _win_size);

  clear();    // empty the container of QSOs

// extract the QSOs
  for (size_t n = 0; n < n_to_copy; ++n)
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
{ const vector<QSO> vec = lgbook.match_exchange(target);
  const size_t n_to_copy = min(vec.size(), _win_size);

  clear();    // empty the container of QSOs

  for (size_t n = 0; n < n_to_copy; ++n)
    (*this) += vec.at(vec.size() - n_to_copy + n);

  display();
}


// -----------  old_log  ----------------

/*! \class  old_log
    \brief  An old ADIF log

    Not thread safe, so create once and then never change.
*/

/*! \brief          Return total number of QSLs from a particular callsign
    \param  call    callsign
    \return         the number of QSLs from callsign <i>call</i>
*/
const unsigned int old_log::n_qsls(const string& call) const
{ const auto cit = _olog.find(call);

  return (cit == _olog.cend() ? 0 : get<0>(cit->second));
}

/*! \brief          Set the number of QSLs from a particular callsign
    \param  call    callsign
    \param  n       number of QSLs from <i>call</i>
*/
void old_log::n_qsls(const string& call, const unsigned int n)
{ auto it = _olog.find(call);

  if (it == _olog.end())
  { _olog[call];   // Josuttis, 2 ed. p.186 implies that this works rather than _olog[call] = { };
    it = _olog.find(call);
  }

  get<0>(it->second) = n;
}

/*! \brief          Increment the number of QSLs from a particular callsign
    \param  call    callsign
    \return         the new number of QSLs from callsign <i>call</i>
*/
const unsigned int old_log::increment_n_qsls(const string& call)
{ auto it = _olog.find(call);

  if (it == _olog.end())
  { _olog[call];
    n_qsls(call, 1);

    return 1;
  }

  const unsigned int rv = n_qsls(call) + 1;

  get<0>(it->second) = rv;

  return rv;
}

/*! \brief          Return total number of QSOs with a particular callsign
    \param  call    callsign
    \return         the number of QSOs with callsign <i>call</i>
*/
const unsigned int old_log::n_qsos(const string& call) const
{ const auto cit = _olog.find(call);

  return (cit == _olog.cend() ? 0 : get<1>(cit->second));
}

/*! \brief          Set the number of QSOs with a particular callsign
    \param  call    callsign
    \param  n       number of QSOs with <i>call</i>
*/
void old_log::n_qsos(const string& call, const unsigned int n)
{ auto it = _olog.find(call);

  if (it == _olog.end())
  { _olog[call];
    it = _olog.find(call);
  }

  get<1>(it->second) = n;
}

/*! \brief          increment the number of QSOs associated with a particular callsign
    \param  call    callsign for which the number of QSOs should be incremented
    \return         number of QSOs associated with with <i>call</i>
*/
const unsigned int old_log::increment_n_qsos(const string& call)
{ const auto it = _olog.find(call);

  if (it == _olog.end())
  { _olog[call];
    n_qsos(call, 1);

    return 1;
  }

  const unsigned int rv = n_qsos(call) + 1;

  get<1>(it->second) = rv;

  return rv;
}

/*! \brief          How many QSOs have taken place with a particular call on a a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         number of QSOs associated with with <i>call</i> on band <i>b</i> and mode <i>m</i>
*/
const unsigned int old_log::n_qsos(const string& call, const BAND b, const MODE m) const
{ const auto cit = _olog.find(call);

  return (cit == _olog.cend() ? 0 : get<3>(cit->second).count( { b, m } ));
}

/*! \brief          Increment the number of QSOs associated with a particular callsign, band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         number of QSOs associated with with <i>call</i> on band <i>b</i> and mode <i>m</i> (following the increment)
*/
const unsigned int old_log::increment_n_qsos(const string& call, const BAND b, const MODE m)
{ auto it = _olog.find(call);

  if (it == _olog.end())
  { _olog[call];
    it = _olog.find(call);
  }

  get<3>(it->second).insert( { b, m } );

  return n_qsos(call, b, m);
}

/*! \brief          Has a QSL ever been received for a particular call on a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
    \return         Has a QSL ever been received for a QSO with <i>call</i> on band <i>b</i> and mode <i>m</i>
*/
const bool old_log::confirmed(const string& call, const BAND b, const MODE m) const
{ const auto cit = _olog.find(call);

  if (cit == _olog.cend())
    return false;

  return (get<2>(cit->second) < ( pair<BAND, MODE>( { b, m } ) ) );
}

/*! \brief          Mark a QSL as being received for a particular call on a particular band and mode
    \param  call    target callsign
    \param  b       target band
    \param  m       target mode
*/
void old_log::qsl_received(const string& call, const BAND b, const MODE m)
{ auto it = _olog.find(call);

  if (it == _olog.end())
  { _olog[call];
    it = _olog.find(call);
  }

  get<2>(it->second).insert( { b, m } );
}
