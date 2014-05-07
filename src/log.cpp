// $Id: log.cpp 57 2014-04-06 23:20:03Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file log.cpp

        Classes and functions related to the log
*/

#include "cabrillo.h"
#include "log.h"
#include "statistics.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;

pt_mutex _log_mutex;

extern message_stream ost;
extern string VERSION;

// -----------  logbook  ----------------

const bool qso_sort_by_time(const QSO& q1, const QSO& q2)
  { return q1.earlier_than(q2); }

void logbook::operator+=(const QSO& q)
{ SAFELOCK(_log);
  _log.insert( { q.callsign(), q } );
  _log_vec.push_back(q);
}

// return qso number n (wrt 1)
const QSO logbook::operator[](const size_t n)
{ if (_log_vec.empty() or (_log_vec.size() < n))
    return QSO();

  return _log_vec[n - 1];
}

/// remove qso number n (wrt 1)
void logbook::operator-=(const unsigned int n)
{ SAFELOCK(_log);

  if (_log.empty() or (_log.size() < n))
    return;

// convert to chronological list, remove the pertinent entry, then convert back to map
//  list<QSO> qso_list = as_list();    // chronological order
  const size_t index = n - 1;    // because the interface is wrt 1

  auto it = _log_vec.begin();
  advance(it, index);

  _removed_qso = (*it);    // just in case we ever need it
  _log_vec.erase(it);
  _log.clear();

  for_each(_log_vec.cbegin(), _log_vec.cend(), [&](const QSO& qso) { _log.insert( {qso.callsign(), qso} ); } );
}

// all the QSOs with a particular call, returned in chronological order
const vector<QSO> logbook::worked(const string& call) const
{ vector<QSO> rv;

  { SAFELOCK(_log);
  
    for_each(_log.lower_bound(call), _log.upper_bound(call), [&rv] (const pair<string, QSO>& qso) { rv.push_back(qso.second); } );
  }

  sort(rv.begin(), rv.end(), qso_sort_by_time);    // put in chronological order

  return rv;
}

// number of times a call has been worked
const unsigned int logbook::n_worked(const string& call) const
{ SAFELOCK(_log);

  const auto range = _log.equal_range(call);

  return distance(range.first, range.second);
}

// has a call been worked on a particular band?
const bool logbook::qso_b4(const string& call, const BAND b) const
{ SAFELOCK(_log);
  
  for (auto cit = _log.lower_bound(call); cit != _log.upper_bound(call); ++cit)
    if (cit->second.band() == b)
      return true;
  
  return false;
}

// has a call been worked on a particular mode?
const bool logbook::qso_b4(const string& call, const enum MODE m) const
{ SAFELOCK(_log);
  
  for (auto cit = _log.lower_bound(call); cit != _log.upper_bound(call); ++cit)
    if (cit->second.mode() == m)
      return true;
  
  return false;
}

// has a call been worked on a particular band and mode?
const bool logbook::qso_b4(const string& call, const BAND b, const enum MODE m) const
{ SAFELOCK(_log);
  
  for (auto cit = _log.lower_bound(call); cit != _log.upper_bound(call); ++cit)
    if ((cit->second.band() == b) and (cit->second.mode() == m))
      return true;
  
  return false;
}

/// a string list of bands on which a call is needed
const string logbook::call_needed(const string& call, const contest_rules& rules) const
{ string rv;

  for (const auto& b : rules.permitted_bands())
    rv += ((qso_b4(call, b)) ? "   " :  BAND_NAME[static_cast<int>(b)]);
  
  return rv;
}

/// would a QSO be a dupe?
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

//    ost << "OK to work on different band? = " << rules.work_if_different_band() << endl;
//    ost << "OK to work on different mode? = " << rules.work_if_different_mode() << endl;

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

/// would a QSO be a dupe?
const bool logbook::is_dupe(const string& call, const BAND b, const enum MODE m, const contest_rules& rules) const
{ QSO qso;

// create a bare-bones QSO
  qso.callsign(call);
  qso.band(b);
  qso.mode(m);
  
  return is_dupe(qso, rules);
}

/// calculate QSO points (i.e., mults = 1)
const unsigned int logbook::qso_points(const contest_rules& rules, location_database& location_db) const
{ unsigned int rv = 0;
  //const list<QSO> qso_list = as_list();

  logbook tmp_log;                       // we are going to rebuild a log qso by qso, in order to mark dupes correctly; this assumes that we aren't keeping the dupes properly marked as we go along, which may be a better way to go
  
  for (vector<QSO>::const_iterator cit = _log_vec.cbegin(); cit != _log_vec.cend(); ++cit)
  { if (!tmp_log.is_dupe(*cit, rules))               // if this QSO is not a dupe
    { rv += rules.points(*cit, location_db);
      tmp_log += (*cit);                             // add this QSO to the temporary log
    }
  }
  
  return rv;
}

/// return time-ordered container of qsos
const list<QSO> logbook::as_list(void) const
{ list<QSO> rv;

  { SAFELOCK(_log);
  
    for (multimap<string, QSO>::const_iterator cit = _log.begin(); cit != _log.end(); ++cit)
      rv.push_back(cit->second);
  }

  rv.sort(earlier);    // sorts according to earlier(const QSO&, const QSO&)

  return rv;
}

/// return time-ordered container of qsos
const vector<QSO> logbook::as_vector(void) const
{ SAFELOCK(_log);

  return _log_vec;
}

/// recalculate the dupes
const logbook logbook::recalculate_dupes(const contest_rules& rules) const
{ //list<QSO> qso_list = as_list();          // put the log in chronological order
  logbook rv;
  
  SAFELOCK(_log);

//  for (list<QSO>::iterator it = qso_list.begin(); it != qso_list.end(); ++it)
  for (auto qso : _log_vec)
  { if (rv.is_dupe(qso, rules))
      qso.dupe();
    else
      qso.undupe();
      
    rv += qso;
  }
  
  return rv;
}

/*! \brief generate a Cabrillo log
    \param      context the drlog context
    \return     the Cabrillo log
    
    The Cabrillo format id supposedly "specified" at:
      http://www.kkn.net/~trey/cabrillo/
        
    Just for the record, the "specification" at the above URL is grossly incomplete
    (even though the ARRL at http://www.arrl.org/contests/fileform.html
    explicitly claims that it is complete). But it *is* incomplete, and there are
    places where an implementer has to to guess what to do.
*/
const string logbook::cabrillo_log(const drlog_context& context, const unsigned int score) const
{ string rv;

// this goes first
  rv += "START-OF-LOG: 3.0" + LF;
  
// call
  rv += "CALLSIGN: " + context.cabrillo_callsign() + LF;
  
// contest
  rv += "CONTEST: " + context.cabrillo_contest() + LF;
  
// drlog name / version
  rv += "CREATED-BY: drlog version " + VERSION + LF;
  
// name of operator
  if (context.cabrillo_name() != string())
    rv += "NAME: " + context.cabrillo_name() + LF;

// address lines http://www.kkn.net/~trey/cabrillo/tags.html: "A maximum of four ADDRESS: lines is permitted."
  if (context.cabrillo_address_1() != string())
  { rv += "ADDRESS: " + context.cabrillo_address_1() + LF;

    if (context.cabrillo_address_2() != string())
    { rv += "ADDRESS: " + context.cabrillo_address_2() + LF;

      if (context.cabrillo_address_3() != string())
      { rv += "ADDRESS: " + context.cabrillo_address_3() + LF;

        if (context.cabrillo_address_4() != string())
          rv += "ADDRESS: " + context.cabrillo_address_4() + LF;
      }
    }
  }
 
// address city
  if (context.cabrillo_address_city() != string())
    rv += "ADDRESS-CITY: " + context.cabrillo_address_city() + LF; 
  
// address state/province
  if (context.cabrillo_address_state_province() != string())
    rv += "ADDRESS-STATE-PROVINCE: " + context.cabrillo_address_state_province() + LF; 
  
// address postcode
  if (context.cabrillo_address_postalcode() != string())
    rv += "ADDRESS-POSTALCODE: " + context.cabrillo_address_postalcode() + LF; 
  
// address country
  if (context.cabrillo_address_country() != string())
    rv += "ADDRESS-COUNTRY: " + context.cabrillo_address_country() + LF;   
  
// list of operators
  if (context.cabrillo_operators() != string())
    rv += "OPERATORS: " + context.cabrillo_operators() + LF;
  
// Categories
// assisted
  rv += "CATEGORY-ASSISTED: " + context.cabrillo_category_assisted() + LF;
 
// band
  rv += "CATEGORY-BAND: " + context.cabrillo_category_band() + LF;

// operator
  rv += "CATEGORY-OPERATOR: " + context.cabrillo_category_operator() + LF;  

// overlay
  if (context.cabrillo_category_overlay() != string())
    rv += "CATEGORY-STATION: " + context.cabrillo_category_overlay() + LF;  

// power
  rv += "CATEGORY-POWER: " + context.cabrillo_category_power() + LF;   

// station
  if (context.cabrillo_category_station() != string())
    rv += "CATEGORY-STATION: " + context.cabrillo_category_station() + LF;  

// time
  if (context.cabrillo_category_time() != string())
    rv += "CATEGORY-TIME: " + context.cabrillo_category_time() + LF;  

// transmitter
  rv += "CATEGORY-TRANSMITTER: " + context.cabrillo_category_transmitter() + LF; 

// club
  if (context.cabrillo_club() != string())
    rv += "CLUB: " + context.cabrillo_club() + LF;  

// location
  if (context.cabrillo_location() != string())
    rv += "LOCATION: " + context.cabrillo_location() + LF;
    
// e-mail
  if (context.cabrillo_e_mail() != string())
    rv += "EMAIL: " + context.cabrillo_e_mail() + LF;    

// claimed score
  if (score)
    rv += "CLAIMED-SCORE: " + to_string(score) + LF;   

/* the QSOs. The Cabrillo so-called "specification" provides not even a semblance of a computer-parsable 
   grammar for QSOs, So a lot of this is guesswork.
   
  CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R TXID:81:1 
*/
  
// generate time-ordered container
  const list<QSO> qsos = as_list();
  const string cabrillo_qso_template = context.cabrillo_qso_template();
  
//  for (list<QSO>::const_iterator cit = qsos.begin(); cit != qsos.end(); ++cit)
//  { rv += cit->cabrillo_format(context.cabrillo_qso_template()) + LF;
//  }
  for_each(qsos.cbegin(), qsos.cend(), [&] (const QSO& q) { rv += q.cabrillo_format(cabrillo_qso_template) + LF; } );
  
// soapbox
  rv += "SOAPBOX: " + LF;
 
// this goes at the end
  rv += "END-OF-LOG:" + LF;
  
  return rv;
}

/// generate a trlog log
// just the main body with one QSO per line
const string logbook::trlog_log(const drlog_context& context, const unsigned int score) const
{ string rv;

// generate time-ordered container
  const list<QSO> qsos = as_list();
  logbook         cumulative;

  for (list<QSO>::const_iterator cit = qsos.begin(); cit != qsos.end(); ++cit)
  { const QSO& qso = *cit;
    string line(80, ' ');
    
// band
    line.replace(0, 3, pad_string(BAND_NAME[qso.band()], 3));
    
// mode    
    switch (qso.mode())
    { case MODE_CW :
        line.replace(3, 2, "CW");
        break;
  
      case MODE_SSB :
        line.replace(3, 3, "SSB");
        break;
  
//      case MODE_DIGI :
//        line.replace(3, 4, "RTTY");
        break;
    }

// date
    static const array<string, 12> MONTHS { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    const string drlog_date = qso.date();
    string trlog_date = drlog_date.substr(8, 2);
    
    trlog_date += "-";
    
    const int month_number = from_string<int>(drlog_date.substr(5, 2));

    if ((month_number >= 1) and (month_number <= 12))
      trlog_date += MONTHS[month_number - 1];
    else
      trlog_date += "UNK";

/*    switch (from_string<int>(drlog_date.substr(5, 2)))
    { case 1:
        trlog_date += "Jan";
        break;

      case 2:
        trlog_date += "Feb";
        break;

      case 3:
        trlog_date += "Mar";
        break;

      case 4:
        trlog_date += "Apr";
        break;

      case 5:
        trlog_date += "May";
        break;

      case 6:
        trlog_date += "Jun";
        break;

      case 7:
        trlog_date += "Jul";
       break;

      case 8:
        trlog_date += "Aug";
        break;

      case 9:
        trlog_date += "Sep";
        break;

      case 10:
        trlog_date += "Oct";
        break;

      case 11:
        trlog_date += "Nov";
        break;

      case 12:
        trlog_date += "Dec";
        break;
    }
*/
    
    trlog_date += "-";
    trlog_date += drlog_date.substr(2, 4);
    line.replace(7, 9, trlog_date);   
  
// time
    line.replace(17, 5, qso.utc().substr(0, 2) + ":" + qso.utc().substr(3, 2));

// frequency
    line.replace(23, 4, ":" + last(qso.freq(), 3));
    
// call
    line.replace(29, 15, qso.call());
    
// more here... it seems that the format changes as a function of contest, though  
  
  
//   rv += cit->cabrillo_format(context.cabrillo_qso_template()) + LF;
  }

  return rv;
}

/// read from a Cabrillo file
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
  for (unsigned int n = 0; n < lines.size(); ++n)
  { const string& line = lines[n];
  
//ost << n << ": " << line << endl;
//ost << "length of line: " << line.length() << endl;
  
    if (line.substr(0, 3) == "QSO" or line.substr(0, 3) == "   ")
    { QSO qso;
  
// go through the fields
      for (unsigned int n = 0; n < individual_values.size(); ++n)
      { const vector<string>& vec = individual_values[n];
        const string name = vec[0];
  
//ost << "Processing field: " << name << endl;
  
        const unsigned int posn = from_string<unsigned int>(vec[1]) - 1;

//ost << "posn: " << posn << endl;

        const unsigned int len  = from_string<unsigned int>(vec[2]);

//ost << "len: " << len << endl;

      const string value = (line.length() >= posn + 1 ? remove_peripheral_spaces(line.substr(posn, len)) : "");
  
//ost << name << ": " << value << endl;
      
// frequency
        if (name == "FREQ")
        { qso.freq(value);
    
  unsigned int _frequency = from_string<int>(value);
  BAND _band;
      
  if (_frequency >= 1800 and _frequency <= 2000)
      _band = BAND_160;
    if (_frequency >= 3500 and _frequency <= 4000)
      _band = BAND_80;
    if (_frequency >= 7000 and _frequency <= 7300)
      _band = BAND_40;
    if (_frequency >= 14000 and _frequency <= 14350)
      _band = BAND_20;
    if (_frequency >= 21000 and _frequency <= 21450)
      _band = BAND_15;
    if (_frequency >= 28000 and _frequency <= 29700)
      _band = BAND_10;  
        qso.band(_band);
  
  ost << "_frequency: " << _frequency << ", _band: " << _band << endl;
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
  { //ost << "TIME: " << value << endl;

    if (value.length() == 5)
      qso.utc(value.substr(0, 2) + value.substr(3, 2));    // handle hh:mm format
    else
      qso.utc(value);                                      // hhmm

    //ost << "UTC: " << qso.utc() << endl;
  }

// tcall
        if (name == "TCALL")
    qso.my_call(value);
    
// transmitted exchange
        if (name.substr(0, 5) == "TEXCH")
  { const string field_name = name.substr(6);
    vector<pair<string, string> > current_sent_exchange = qso.sent_exchange(); // do in two steps in order to remove constness of returned value
  
    qso.sent_exchange((current_sent_exchange.push_back(make_pair(field_name, value)), current_sent_exchange)); 
  }   

// rcall
        if (name == "RCALL")
    qso.callsign(value);

// received exchange
        if (name.substr(0, 5) == "REXCH")
  { const string field_name = name.substr(6);
//    vector<pair<string, string> > current_received_exchange = qso.received_exchange(); // do in two steps in order to remove constness of returned value
    vector<received_field> current_received_exchange = qso.received_exchange(); // do in two steps in order to remove constness of returned value

// should have a function in the QSO class to add a field to the exchange
//    map<string, string> current_received_exchange = qso.received_exchange();
//    current_received_exchange.insert(make_pair(field_name, value));
    current_received_exchange.push_back( { field_name, value, false, false });
    qso.received_exchange(current_received_exchange);

//    qso.received_exchange((current_received_exchange.push_back(make_pair(field_name, value)), current_received_exchange));

  
  }   

// don't do TXID since we don't really need that (and there's currently nowhere in QSO to put it)
      
      }
      
//ost << "Processed QSO" << endl;      
      qso.number(++last_qso_number);
      *this += qso;
    }
  }
}

/// read from a Cabrillo file using space-delimited fields
void logbook::read_cabrillo(const string& filename, const vector<string>& cabrillo_fields)
{ string file_contents = remove_char(read_file(filename), '\r');
  const vector<string> lines = to_lines(file_contents);

  unsigned int last_qso_number = 0; 

//ost << "Reading file" << endl;

// for each QSO line
  for (unsigned int n = 0; n < lines.size(); ++n)
  { const string& line = lines[n];
  
//ost << n << ": " << line << endl;
//ost << "length of line: " << line.length() << endl;
  
    if (line.substr(0, 3) == "QSO" or line.substr(0, 3) == "   ")
    { QSO qso;
  
      const vector<string> fields = split_string(remove_peripheral_spaces(squash(line.substr(4))), " ");
      
      for (unsigned int m = 0; m < fields.size(); ++m)
        ost << m << ": *" << fields[m] << "*" << endl; 

// go through the fields
      for (unsigned int n = 0; n < cabrillo_fields.size(); ++n)
      { const string name = cabrillo_fields[n];
        const string value = (n < fields.size() ? remove_peripheral_spaces(fields[n]) : "");
  
//ost << "name: " << name << endl;  
//ost << "value: " << value << endl;  
  
// frequency
      

        if (name == "FREQ")
  { qso.freq(value);
    
  unsigned int _frequency = from_string<int>(value);
  BAND _band = to_BAND(_frequency);

/*
  if (_frequency >= 1800 and _frequency <= 2000)
      _band = BAND_160;
    if (_frequency >= 3500 and _frequency <= 4000)
      _band = BAND_80;
    if (_frequency >= 7000 and _frequency <= 7300)
      _band = BAND_40;
    if (_frequency >= 14000 and _frequency <= 14350)
      _band = BAND_20;
    if (_frequency >= 21000 and _frequency <= 21450)
      _band = BAND_15;
    if (_frequency >= 28000 and _frequency <= 29700)
      _band = BAND_10;  
*/
        qso.band(_band);
  
  ost << "_frequency: " << _frequency << ", _band: " << _band << endl;
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
  
    qso.sent_exchange((current_sent_exchange.push_back(make_pair(field_name, value)), current_sent_exchange)); 
  }   

// rcall
        if (name == "RCALL")
    qso.callsign(value);

// received exchange
        if (name.substr(0, 5) == "REXCH")
  { const string field_name = name.substr(6);
//    vector<pair<string, string> > current_received_exchange = qso.received_exchange(); // do in two steps in order to remove constness of returned value
    vector<received_field> current_received_exchange = qso.received_exchange(); // do in two steps in order to remove constness of returned value
  
    //qso.received_exchange((current_received_exchange.push_back(make_pair(field_name, value)), current_received_exchange));

    // should have a function in the QSO class to add a field to the exchange
     //   map<string, string> current_received_exchange = qso.received_exchange();
        //current_received_exchange.insert(make_pair(field_name, value));
        current_received_exchange.push_back( { field_name, value, false, false } );
        qso.received_exchange(current_received_exchange);
  }   

// don't do TXID since we don't really need that (and there's currently nowhere in QSO to put it)
      
      }
      
//ost << "Processed QSO" << endl;      
      qso.number(++last_qso_number);
      *this += qso;
      
//      exit(0);
    }
  }
}

// get the value of an exchange field from the last QSO with someone; returns empty string if anything goes wrong
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

// -----------  log_extract  ----------------

/*!     \class log_extract
        \brief Support for bits of the log
*/

log_extract::log_extract(window& w) :
  _win(w),
  _win_size(0)
{
}

void log_extract::prepare(void)
{ _win_size = _win.height();
}

void log_extract::operator+=(const QSO& qso)
{ SAFELOCK(_extract);

  _qsos.push_back(qso);

  while (_qsos.size() > _win_size)
    _qsos.pop_front();
}

void log_extract::display(void)
{ vector<QSO> vec;

  { SAFELOCK(_extract);

    copy(_qsos.cbegin(), _qsos.cend(), back_inserter(vec));
  }

//  ost << "number of QSOS = " << vec.size() << endl;

  ost << "vec size = " << vec.size() << ", _win_size = " << _win_size << endl;

  if (vec.size() < _win_size)
  { ost << "clearing window" << endl;
    _win < WINDOW_CLEAR;
  }

  if (!vec.empty())
  { const size_t n_to_display = min(vec.size(), _win_size);

    ost << "n_to_display = " << n_to_display << endl;

    for (size_t n = 0; n < n_to_display; ++n)
    { const size_t index = vec.size() - 1 - n;
      _win < cursor(0, n) < vec[index].log_line();
    }
  }

  _win < WINDOW_REFRESH;
}

// get recent QSOs from a log, and possibly display them
void log_extract::recent_qsos(const logbook& lgbook, const bool to_display)
{
  ost << "logbook size = " << lgbook.size() << endl;

  const vector<QSO>& vec = lgbook.as_vector();

  ost << " vector size = " << vec.size() << endl;

  const size_t n_to_copy = min(vec.size(), _win_size);

  ost << "in recent_qsos(), n_to_copy = " << n_to_copy << endl;

  clear();    // empty the container of QSOs

  for (size_t n = 0; n < n_to_copy; ++n)
  	(*this) += vec.at(vec.size() - n_to_copy + n);

  if (to_display)
  	display();
}
