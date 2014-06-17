// $Id: qso.cpp 66 2014-06-14 19:22:10Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file qso.cpp

        Classes and functions related to QSO information
*/

#include "bands-modes.h"
#include "cty_data.h"
#include "qso.h"
#include "statistics.h"
#include "string_functions.h"

#include <array>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>

using namespace std;

extern message_stream ost;

//QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=
const pair<string, string> QSO::_next_name_value_pair(const string& str, size_t& posn)
{ static const pair<string, string> empty_pair;

  if (posn >= str.size())
  { posn = string::npos;
    return empty_pair;
  }

  size_t first_char_posn = str.find_first_not_of(" ", posn);

  if (first_char_posn == string::npos)
  { posn = string::npos;
    return empty_pair;
  }

  size_t equals_posn = str.find("=", first_char_posn);

  if (equals_posn == string::npos)
  { posn = string::npos;
    return empty_pair;
  }

  const string name = remove_peripheral_spaces(str.substr(first_char_posn, equals_posn - first_char_posn));

  size_t value_first_char_posn = str.find_first_not_of(" ", equals_posn + 1);

  if (value_first_char_posn == string::npos)
  { posn = string::npos;
    return empty_pair;
  }

  size_t space_posn = str.find(" ", value_first_char_posn);

  const string value = (space_posn == string::npos) ? str.substr(value_first_char_posn)
                                                    : str.substr(value_first_char_posn, space_posn - value_first_char_posn);

  posn = space_posn;

  return pair<string, string> { name, value };
}

const time_t QSO::_to_epoch_time(const string& date_str, const string& utc_str) const
{
  struct tm time_struct;

  time_struct.tm_sec = from_string<int>(utc_str.substr(6, 2));
  time_struct.tm_min = from_string<int>(utc_str.substr(3, 2));
  time_struct.tm_hour = from_string<int>(utc_str.substr(0, 2));
  time_struct.tm_mday = from_string<int>(date_str.substr(8, 2));
  time_struct.tm_mon = from_string<int>(date_str.substr(5, 2)) - 1;
  time_struct.tm_year = from_string<int>(date_str.substr(0, 4)) - 1900;

//  ost << "_utc = " << _utc << endl;
//  ost << "_date = " << _date << endl;

//  _utc = 15:46:59
//  _date = 2013-05-25

//  return (mktime(&time_struct));
  return timegm(&time_struct);    // GNU function; see time_gm man page for portable alternative.
                                  // To implement alternative: declare a class whose constructor sets
                                  // TZ and calls tzset(), then call the constructor globally
}

/// constructor
QSO::QSO(void) :
  _points(1),
  _is_dupe(false),
  _is_country_mult(false),
  _is_prefix_mult(false),
  _epoch_time(time(NULL))        // now
{ struct tm    structured_time;

  gmtime_r(&_epoch_time, &structured_time);          // convert to UTC
  
  array<char, 26> buf;                           // buffer to hold the ASCII date/time info; see man page for gmtime()
  
  asctime_r(&structured_time, buf.data());                   // convert to ASCII

  const string ascii_time(buf.data(), 26);                   // this is a modern language
  
  _utc  = ascii_time.substr(11, 8);                                                     // hh:mm:ss
  _date = to_string(structured_time.tm_year + 1900) + "-" +
          pad_string(to_string(structured_time.tm_mon + 1), 2, PAD_LEFT, '0') + "-" +
          pad_string(to_string(structured_time.tm_mday), 2, PAD_LEFT, '0');             // yyyy-mm-dd
}

/// read fields from a line in the disk log
//QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=
void QSO::populate_from_verbose_format(const string& str, const contest_rules& rules, running_statistics& statistics)
{
// build a vector of name/value pairs
  size_t cur_posn = min(static_cast<size_t>(5), str.size());  // skip the "QSO: "

//  string str_copy = str.substr(5);    // remove the "QSO: "
  vector<pair<string, string> > name_value;

//  if (posn != string::npos)
  while (cur_posn != string::npos)
    name_value.push_back(_next_name_value_pair(str, cur_posn));

//  for (size_t n = 0; n < name_value.size(); ++n)
//    ost << "name = " << name_value[n].first << ", value = " << name_value[n].second << endl;

  _sent_exchange.clear();
  _received_exchange.clear();

  for (size_t n = 0; n < name_value.size(); ++n)
  { bool processed = false;

    if (!processed and (name_value[n].first == "number"))
    { _number = from_string<decltype(_number)>(name_value[n].second);
      processed = true;
    }

    if (!processed and (name_value[n].first == "date"))
    { _date = name_value[n].second;
      processed = true;
    }

    if (!processed and (name_value[n].first == "utc"))
    { _utc = name_value[n].second;
      processed = true;
    }

    if (!processed and (name_value[n].first == "mode"))
    { _mode = ( ( name_value[n].second == "CW") ? MODE_CW : MODE_SSB);
      processed = true;
    }

    if (!processed and (name_value[n].first == "frequency"))
    { _frequency = name_value[n].second;

      const double f = from_string<double>(_frequency);
      const frequency freq(f);

      _band = static_cast<BAND>(freq);
      processed = true;
    }

    if (!processed and (name_value[n].first == "hiscall"))
    { _callsign = name_value[n].second;

extern location_database location_db;

      _canonical_prefix = location_db.canonical_prefix(_callsign);
      _continent = location_db.continent(_callsign);
      processed = true;
    }

    if (!processed and (name_value[n].first == "mycall"))
    { _my_call = name_value[n].second;
      processed = true;
    }

    if (!processed and (name_value[n].first.substr(0, 5) == "sent-"))
    { _sent_exchange.push_back( { to_upper(name_value[n].first.substr(5)), name_value[n].second } );
      processed = true;
    }

 //   ost << "restoring QSO for " << _callsign << endl;

    if (!processed and (name_value[n].first.substr(0, 9) == "received-"))
    { const string& name = to_upper(name_value[n].first.substr(9));
      const string& value = name_value[n].second;
      const bool is_possible_mult = rules.is_exchange_mult(name);
      const bool is_mult = is_possible_mult ? statistics.is_needed_exchange_mult(name, value, _band) : false;
      const received_field rf { name, value , is_possible_mult, is_mult };

//      ost << "exchange field " << name << " has value " << value << " and is_possible_mult is " << is_possible_mult << " and is_mult is " << is_mult << endl;

      _received_exchange.push_back(rf);
      processed = true;
    }
  }

  const bool qso_is_country_mult = statistics.is_needed_country_mult(_callsign, _band);
  _is_country_mult = qso_is_country_mult;

//  ost << _callsign << " is_country_mult = " << _is_country_mult << endl;

  _epoch_time = _to_epoch_time(_date, _utc);
}

/// read fields from a log_line
void QSO::populate_from_log_line(const string& str)
{
//  ost << "populate_from_log_line parameter: " << str << endl;
//  ost << "squashed: " << squash(str, ' ') << endl;

// separate the line into fields
  const vector<string> vec = remove_peripheral_spaces(split_string(squash(str, ' '), " "));

//  for (size_t n = 0; n < vec.size(); ++n)
//    vec[n] = remove_peripheral_spaces(vec[n]);

  if (vec.size() != _log_line_fields.size())
  { ost << "populate_from_log_line parameter: " << str << endl;
    ost << "squashed: " << squash(str, ' ') << endl;

    ost << "Problem with number of fields in edited log line" << endl;
    ost << "vec size = " << vec.size() << "; _log_line_fields size = " << _log_line_fields.size() << endl;

    for (size_t n = 0; n < vec.size(); ++n)
      ost << "vec[" << n << "] = " << vec[n] << endl;

    for (size_t n = 0; n < _log_line_fields.size(); ++n)
          ost << "_log_line_fields[" << n << "] = " << _log_line_fields[n] << endl;
  }

  size_t sent_index = 0;
  size_t received_index = 0;

  for (size_t n = 0; ( (n < vec.size()) and (n < _log_line_fields.size()) ); ++n)
  { bool processed = false;

    if (!processed and (_log_line_fields[n] == "NUMBER"))
    { _number = from_string<decltype(_number)>(vec[n]);
      processed = true;
    }

    if (!processed and (_log_line_fields[n] == "DATE"))
    { _date = vec[n];
      processed = true;
    }

    if (!processed and (_log_line_fields[n] == "UTC"))
    { _utc = vec[n];
      processed = true;
    }

    if (!processed and (_log_line_fields[n] == "MODE"))
    { _mode = ( (vec[n] == "CW") ? MODE_CW : MODE_SSB);
      processed = true;
    }

    if (!processed and (_log_line_fields[n] == "FREQUENCY"))
    { _frequency = vec[n];

      const double f = from_string<double>(_frequency);
      const frequency freq(f);

      _band = static_cast<BAND>(freq);
      processed = true;
    }

    if (!processed and (_log_line_fields[n] == "CALLSIGN"))
    { _callsign = vec[n];
      processed = true;
    }

//    ost << "sent exchange size = " << _sent_exchange.size() << endl;

    if (!processed and (_log_line_fields[n].substr(0, 5) == "sent-"))
    { if (sent_index < _sent_exchange.size())
        _sent_exchange[sent_index++].second = vec[n];
      processed = true;
    }

    if (!processed and (_log_line_fields[n].substr(0, 9) == "received-"))
    { if (received_index < _received_exchange.size())
        _received_exchange[received_index++].value(vec[n]);
      processed = true;
    }
  }

  extern location_database location_db;

  _canonical_prefix = location_db.canonical_prefix(_callsign);
  _continent = location_db.continent(_callsign);

//ost << "in populate_from_log_line; about to set _epoch_time for " << _callsign << endl;

  _epoch_time = _to_epoch_time(_date, _utc);

//  ost << "_epoch_time = " << _epoch_time << endl;
  ost << "in populate_from_log_line; _is_country_mult = " << _is_country_mult << endl;
}

/// are any of the exchange fields a mult?
const bool QSO::is_exchange_mult(void) const
{ for (const auto& field : _received_exchange)
    if (field.is_mult())
      return true;

  return false;
}

//std::vector<received_field>   _received_exchange;              // names do not include the REXCH-


/// re-format according to a Cabrillo template
/*
  CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1 
*/
const string QSO::cabrillo_format(const string& cabrillo_qso_template) const
{ static unsigned int record_length = 0;
  static vector< vector< string> > individual_values;
  
  if (!record_length)                                         // if we don't yet know the record length
  { const vector<string> template_fields = split_string(cabrillo_qso_template, ",");         // colon-delimited values
  
//    for (unsigned int n = 0; n < template_fields.size(); ++n)
    for (const auto& template_field : template_fields)
      individual_values.push_back(split_string(remove_peripheral_spaces(template_field), ":"));
  
//    for (unsigned int n = 0; n < individual_values.size(); ++n)
    for (const auto& value : individual_values)
    { //const vector<string>& vec = individual_values[n];
      const unsigned int last_char_posn = from_string<unsigned int>(value[1]) + from_string<unsigned int>(value[2]) - 1;

      record_length = max(last_char_posn, record_length);    
    }
  }
  
// create a record full of spaces
  string record(record_length, ' ');
  
  record.replace(0, 4, "QSO:");             // put into record
  
// go through every possibility for every field
  for (unsigned int n = 0; n < individual_values.size(); ++n)
  { const vector<string>& vec = individual_values[n];
    const string name = vec[0];
    const unsigned int posn = from_string<unsigned int>(vec[1]) - 1;
    const unsigned int len  = from_string<unsigned int>(vec[2]);
    pad_direction pdirn = PAD_LEFT;

    char pad_char = ' ';
      
    if (vec.size() == 4)
    { // ost << "looking for padding for " << name << " => " << vec[3] << endl;

      if (vec[3][0] == 'R')
    { // ost << "PADDING TO RIGHT" << endl;
      pdirn = PAD_RIGHT;
    }
//      else
//        ost << "Pad direction = " << vec[3][0] << endl;

      if (vec[3].size() == 2)    // if we define a pad char
        pad_char = vec[3][1];
    }

    string value;          // hold the value that we are going to insert
    
/*
    * freq is frequency/band:
          o 1800 or actual frequency in KHz
          o 3500 or actual frequency in KHz
          o 7000 or actual frequency in KHz
          o 14000 or actual frequency in KHz
          o 21000 or actual frequency in KHz
          o 28000 or actual frequency in KHz
          o 50
          o 144
          o 222
          o 432
          o 902
          o 1.2G
          o 2.3G
          o 3.4G
          o 5.7G
          o 10G
          o 24G
          o 47G
          o 75G
          o 119G
          o 142G
          o 241G
          o 300G 
*/    
    if (name == "FREQ")
    { if (!_frequency.empty())                                      // frequency is available
        value = to_string(from_string<unsigned int>(_frequency));
      else                                                          // we have only the band
      { switch (_band)
        { case BAND_160:
            value = "1800";
            break;
          case BAND_80:
            value = "3500";
            break;
          case BAND_40:
            value = "7000";
            break;
          case BAND_20:
            value = "14000";
            break;
          case BAND_15:
            value = "21000";
            break;
          case BAND_10:
            value = "28000";
            break;
          default:
            value = "UNK";
            break;
        }
      }
    }

/*
mo is mode:

    * CW
    * PH
    * FM
    * RY
    * (in the case of cross-mode QSOs, indicate the transmitting mode) 
*/
    if (name == "MODE")
    { switch (_mode)
      { case MODE_CW:
          value = "CW";
    break;
  case MODE_SSB:
    value = "PH";
    break;
//  case MODE_DIGI:
//    value = "RY";
//    break;
      }
    }
    
/*
date is UTC date in yyyy-mm-dd form
*/      
    if (name == "DATE")
      value = _date;
      
/*
time is UTC time in nnnn form; the "specification" doesn't bother to tell
us whether to round or to truncate.  Truncation is easier, so until the
specification tells us otherwise, that's what we do.
*/
    if (name == "TIME")
      value = _utc.substr(0, 2) + _utc.substr(3, 2);
  
// TCALL == transmitted call == my call
    if (name == "TCALL")
      value = _my_call;
      
// TEXCH-xxx
    if (name.substr(0, 6) == "TEXCH-")
    { const string field_name = name.substr(6);
    
      for (unsigned int n = 0; n < _sent_exchange.size(); ++n)
        if (_sent_exchange[n].first == field_name)
          value = _sent_exchange[n].second;
    }

// REXCH-xxx
    if (name.substr(0, 6) == "REXCH-")
    { const string field_name = name.substr(6);
    
      value = received_exchange(field_name);  // don't use _received_exchange[field_name] because that's not const
    }
    
// RCALL
    if (name == "RCALL")
      value = _callsign;  

// TXID
    if (name == "TXID")
      value = "0";  
  
    value = pad_string(value, len, pdirn, pad_char);
    record.replace(posn, len, value);             // put into record
    
    ost << "record: " << record << endl;
//    ost << "record length: " << record.length() << endl;
  }

  return record;
}

/// format for writing to disk
const string QSO::verbose_format(void) const
{ static const int NUMBER_WIDTH   = 5;
  static const int CALLSIGN_WIDTH = 12;
  static const int MODE_WIDTH = 3;
  static const int BAND_WIDTH = 3;
  static const int FREQUENCY_WIDTH = 7;

  static const map<string /* tx field name */, pair< int /* width */, pad_direction> > TX_WIDTH { { "sent-RST",    { 3, PAD_LEFT } },
                                                                                                  { "sent-CQZONE", { 2, PAD_LEFT } }
                                                                                                };

  static const map<string /* tx field name */, pair< int /* width */, pad_direction> > RX_WIDTH { { "received-RST",    { 3, PAD_LEFT } },
                                                                                                  { "received-CQZONE", { 2, PAD_LEFT } }
                                                                                                };

  string rv;

  rv = "QSO: ";
  
  rv += "number=" + pad_string(to_string(_number), NUMBER_WIDTH);
  rv += " date=" + _date;
  rv += " utc=" + _utc;
  rv += " hiscall=" + pad_string(_callsign, CALLSIGN_WIDTH, PAD_RIGHT);
  rv += " mode=" + pad_string(remove_peripheral_spaces(MODE_NAME[_mode]), MODE_WIDTH, PAD_RIGHT);
  rv += " band=" + pad_string(remove_peripheral_spaces(BAND_NAME[_band]), BAND_WIDTH);
  rv += " frequency=" + pad_string(_frequency, FREQUENCY_WIDTH);
  rv += " mycall=" + pad_string(_my_call, CALLSIGN_WIDTH, PAD_RIGHT);

//  for (size_t n = 0; n < _sent_exchange.size(); ++n)
  for (const auto& exch_field : _sent_exchange)
  { //const pair<string, string>& exch_field = _sent_exchange[n];
    const string name = "sent-" + exch_field.first;
    const auto cit = TX_WIDTH.find(name);
    const string value = (cit == TX_WIDTH.cend() ? exch_field.second : pad_string(exch_field.second, cit->second.first, cit->second.second));
  
//    rv += " sent-" + exch_field.first + "=" + exch_field.second;
    rv += " " + name + "=" + value;
  } 

  for (const auto& field : _received_exchange)
  { const string name = "received-" + field.name();
    const auto cit = RX_WIDTH.find(name);
    const string value = (cit == RX_WIDTH.cend() ? field.value() : pad_string(field.value(), cit->second.first, cit->second.second));

    rv += " " + name + "=" + value;
  }
//    rv += (" received-" + field.name() + "=" + field.value());


  rv += " points=" + to_string(_points);
  rv += static_cast<string>(" dupe=") + (_is_dupe ? "true " : "false");

  rv += " comment=" + _comment;

//  ost << "verbose_format() returning: " << rv << endl;
    
  return rv; 
}

/// does the QSO match an expression for a received exchange field?
/// [IOTA != -----]
const bool QSO::exchange_match(const string& rule_to_match) const
{
  ost << "testing exchange match rule: " << rule_to_match << " on QSO: " << *this << endl;

// remove the [] markers
  const string target = rule_to_match.substr(1, rule_to_match.length() - 2);
  const vector<string> tokens = split_string(target, " ");

  if (tokens.size() != 3)
  { ost << "Number of tokens = " << tokens.size() << endl;
  }
  else                          // three tokens
  { const string& exchange_field_name = remove_peripheral_spaces(tokens[0]);                     // does not include the REXCH-, since it's taken directly from the logcfg.dat file
    string exchange_field_value;                                   // default is empty field

// is this field present?
  //  for (unsigned int n = 0; n < _received_exchange.size(); ++n)
  //  { ost << "comparing " << _received_exchange[n].first << ", which has the value: " << _received_exchange[n].second << " with " << exchange_field_name << endl;

  //    if (_received_exchange[n].first == exchange_field_name)
  //      exchange_field_value = _received_exchange[n].second;
  //  }

      exchange_field_value = received_exchange(exchange_field_name);



// now try the various legal operations
    const string op = remove_peripheral_spaces(tokens[1]);
    string target = remove_peripheral_spaces(tokens[2]);
    target = remove_leading(target, '"');
    target = remove_trailing(target, '"');                  // strip any double quotation marks

// !=
    if (!remove_leading_spaces(exchange_field_value).empty())        // only check if we actually received something; catch the empty and all-spaces cases
    { if (op == "!=")                                                // precise inequality
      { ost << "matched operator: " << op << endl;
        ost << "exchange field value: *" << exchange_field_value << "* " << endl;
        ost << "target: *" << target << "* " << endl;

        return exchange_field_value != target;
      }
    }

  }
//  std::vector<std::pair<std::string, std::string> >   _received_exchange;  // vector<name, value>

  return false;

}

/*!     \brief  Return a single field from the received exchange
        \param  field_name  The name of the field
        \return The value of <i>field_name</i> in the received exchange

        Returns the empty string if <i>field_name</i> is not found in the exchange
 */
const string QSO::received_exchange(const string& field_name) const
{ for (const auto& field : _received_exchange)
  { if (field.name() == field_name)
      return field.value();
  }

  return string();
}

/*!     \brief  Return a single field from the sent exchange
        \param  field_name  The name of the field
        \return The value of <i>field_name</i> in the sent exchange

        Returns the empty string if <i>field_name</i> is not found in the exchange
 */
const string QSO::sent_exchange(const string& field_name) const
{ for (const auto& field : _sent_exchange)
  { if (field.first == field_name)
      return field.second;
  }

  return string();
}

/*!     \brief  Does the sent exchange include a particular field?
        \param  field_name  The name of the field
        \return Whether <i>field_name</i> is present in the sent exchange
 */
const bool QSO::sent_exchange_includes(const std::string& field_name)
{ for (const auto& field : _sent_exchange)
  { if (field.first == field_name)
      return true;
  }

  return false;
}

/// for use in the log window
const string QSO::log_line(void)
{ static const size_t CALL_FIELD_LENGTH = 12;
  string rv;

  rv  = pad_string(to_string(number()), 5);
  rv += pad_string(date(), 11);
  rv += pad_string(utc(), 9);
  rv += pad_string(MODE_NAME[mode()], 4);
  rv += pad_string(freq(), 8);

//  ost << "rv = *" << rv << "*" << endl;

  rv += pad_string(pad_string(callsign(), CALL_FIELD_LENGTH, PAD_RIGHT), CALL_FIELD_LENGTH + 1);

//  ost << "rv = *" << rv << "*" << endl;

//  for (unsigned int n = 0; n < _sent_exchange.size(); ++n)
//    rv += " " + _sent_exchange[n].second;

  for_each(_sent_exchange.cbegin(), _sent_exchange.cend(), [&] (pair<string, string> se) { rv += " " + se.second; });

// print in same order the are present in the config file
//    for (size_t n = 0; n < _received_exchange.size(); ++n)
    for (const auto& field : _received_exchange)
    { unsigned int field_width = 5;
      const string& name = field.name();

      ost << "field name in log line: " << name << endl;

      if (name == "CQZONE")
        field_width = 2;

      if (name == "CWPOWER")
        field_width = 3;

      if (name == "ITUZONE")
        field_width = 2;

      if (name == "RS")
        field_width = 2;

      if (name == "RST")
        field_width = 3;

      if (name == "RDA")
        field_width = 4;

      if (name == "SERNO")
        field_width = 4;

      if (name == "10MSTATE")
        field_width = 3;

      if (QSO_MULT_WIDTH)
        field_width = QSO_MULT_WIDTH;

      rv += " " + pad_string(field.value(), field_width);
    }

// mults

// callsign mult
   if (_is_prefix_mult)
     rv += pad_string(_prefix, 5);

// country mult
   if (QSO_DISPLAY_COUNTRY_MULT)                                            // set in drlog_context when parsing the config file
     rv += (_is_country_mult ? pad_string(_canonical_prefix, 5) : "     "); // sufficient for VP2E

// exchange mult
   for (const auto& field : _received_exchange)
   { unsigned int field_width = 5;
     const string& name = field.name();

     if (name == "CQZONE" or name == "ITUZONE")
       field_width = 2;

     if (name == "RS")
       field_width = 2;

     if (name == "RST")
       field_width = 3;

     if (name == "RDA")
       field_width = 4;

     if (name == "SERNO")
       field_width = 4;

     if (name == "10MSTATE")
       field_width = 3;

     if (QSO_MULT_WIDTH)
       field_width = QSO_MULT_WIDTH;

     rv += (field.is_mult() ? pad_string(field.value(), field_width + 1) : "");

//     ost << "field = " << name << "; is_mult = " << field.is_mult() << endl;
//     ost << "length of log line = " << rv.size() << endl;
   }

  _log_line_fields.clear();    // make sure it's empty before we fill it

  _log_line_fields.push_back("NUMBER");
  _log_line_fields.push_back("DATE");
  _log_line_fields.push_back("UTC");
  _log_line_fields.push_back("MODE");
  _log_line_fields.push_back("FREQUENCY");
  _log_line_fields.push_back("CALLSIGN");

  for (size_t n = 0; n < _sent_exchange.size(); ++n)
  { const pair<string, string>& exch_field = _sent_exchange[n];

    _log_line_fields.push_back("sent-" + exch_field.first);
  }

  for (const auto& field : _received_exchange)
    _log_line_fields.push_back("received-" + field.name());

// possible mults
//  log_line += (is_country_mult ? pad_string(location_db_p->canonical_prefix(qso.callsign()), 5) : "     ");
//  _log_line_fields.push_back(_is_country_mult ? pad_string(_canonical_prefix, 5) : "     ");

  return rv;
}

/// ostream << QSO
std::ostream& operator<<(std::ostream& ost, QSO& q)
{ ost << "Number: " << q.number()
      << ", Date: " << q.date()
      << ", UTC: " << q.utc()
      << ", Call: " << q.callsign()
      << ", Mode: " << (int)(q.mode())
      << ", Band: " << (int)(q.band())
      << ", Freq: " << q.freq()
      << ", Sent: ";
      
  const vector<pair<string, string> > sent_exchange = q.sent_exchange();
  for (unsigned int n = 0; n < sent_exchange.size(); ++n)
    ost << sent_exchange[n].first << " " << sent_exchange[n].second << " ";    

    ost << ", Rcvd: ";

//  const vector<pair<string, string> > received_exchange = q.received_exchange();
  const vector<received_field> received_exchange = q.received_exchange();

//  for (unsigned int n = 0; n < received_exchange.size(); ++n)
//    ost << received_exchange[n].name() << " " << received_exchange[n].value() << " ";

    for (const auto& received_exchange_field : received_exchange)
      ost << received_exchange_field.name() << " " << received_exchange_field.value() << " ";

//    const map<string, string> received_exchange = q.received_exchange();
//    for (map<string, string>::const_iterator cit = received_exchange.begin(); cit != received_exchange.end(); ++cit)
//      ost << (cit->first) << " " << (cit->second) << " ";

      
//  ost << ", Rcvd: " << q.received_exchange()
  ost << ", Comment: " << q.comment()
      << ", Points: " << q.points();
  
  return ost;
}

bool QSO_DISPLAY_COUNTRY_MULT = true;
int  QSO_MULT_WIDTH = 0;
