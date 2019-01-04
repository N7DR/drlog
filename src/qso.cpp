// $Id: qso.cpp 149 2019-01-03 19:24:01Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   qso.cpp

    Classes and functions related to QSO information
*/

#include "bands-modes.h"
#include "cty_data.h"
#include "exchange.h"
#include "qso.h"
#include "statistics.h"
#include "string_functions.h"

#include <array>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>

using namespace std;

extern contest_rules rules;                 ///< rules for this contest
extern drlog_context context;               ///< configuration context
extern location_database location_db;       ///< location database
extern message_stream ost;                  ///< for debugging, info

extern void alert(const string& msg, const bool show_time = true);       ///< alert the user

bool QSO_DISPLAY_COUNTRY_MULT = true;       ///< whether to display country mults in log window (may be changed in config file)
unsigned int  QSO_MULT_WIDTH = 5;           ///< default width of QSO mult fields in log window

/*! \brief                      Is a particular field that might be received as part of the exchange optional?
    \param  field_name          the name of the field
    \param  fields_from_rules   the possible fields, taken from the rules
    \return                     whether field <i>field_name</i> is optional

    Works regardless of whether <i>field_name</i> includes an initial "received-" string
*/
const bool QSO::_is_received_field_optional(const string& field_name, const vector<exchange_field>& fields_from_rules) const
{ string name_copy = field_name;

  if (begins_with(name_copy, "received-"))
    name_copy = substring(name_copy, 9);

  for (const auto& ef : fields_from_rules)
  { if (ef.name() == name_copy)
      return ef.is_optional();
  }

// should never get here
  ost << "ERROR; unable to find field in _is_received_file_optional" << endl;

  return false;             // keep the compiler happy
}

/*! \brief               Obtain the epoch time from a date and time in drlog format
    \param  date_str     date string in drlog format
    \param  utc_str      time string in drlog format
    \return              time in seconds since the UNIX epoch
*/
const time_t QSO::_to_epoch_time(const string& date_str, const string& utc_str) const
{
  struct tm time_struct;

  time_struct.tm_sec = from_string<int>(utc_str.substr(6, 2));
  time_struct.tm_min = from_string<int>(utc_str.substr(3, 2));
  time_struct.tm_hour = from_string<int>(utc_str.substr(0, 2));
  time_struct.tm_mday = from_string<int>(date_str.substr(8, 2));
  time_struct.tm_mon = from_string<int>(date_str.substr(5, 2)) - 1;
  time_struct.tm_year = from_string<int>(date_str.substr(0, 4)) - 1900;

  return timegm(&time_struct);    // GNU function; see time_gm man page for portable alternative.
                                  // To implement alternative: declare a class whose constructor sets
                                  // TZ and calls tzset(), then call the constructor globally
}

/// constructor
QSO::QSO(void) :
  _epoch_time(time(NULL)),          // now
  _is_country_mult(false),          // not a country mult
  _is_dupe(false),                  // not a dupe
  _is_prefix_mult(false),           // not a prefix mult
  _points(1)                        // unused
{ struct tm    structured_time;

  gmtime_r(&_epoch_time, &structured_time);     // convert to UTC
  
  array<char, 26> buf;                          // buffer to hold the ASCII date/time info; see man page for gmtime()
  
  asctime_r(&structured_time, buf.data());      // convert to ASCII

  const string ascii_time(buf.data(), 26);
  
  _utc  = ascii_time.substr(11, 8);                                                     // hh:mm:ss
  _date = to_string(structured_time.tm_year + 1900) + "-" +
          pad_string(to_string(structured_time.tm_mon + 1), 2, PAD_LEFT, '0') + "-" +
          pad_string(to_string(structured_time.tm_mday), 2, PAD_LEFT, '0');             // yyyy-mm-dd
}

/*! \brief              Read fields from a line in the disk log
    \param  context     drlog context
    \param  str         string from log file
    \param  rules       rules for this contest
    \param  statistics  contest statistics

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=
*/
void QSO::populate_from_verbose_format(const drlog_context& context, const string& str, const contest_rules& rules, running_statistics& statistics)
{
// build a vector of name/value pairs
  size_t cur_posn = min(static_cast<size_t>(5), str.size());  // skip the "QSO: "
  vector<pair<string, string> > name_value;

  while (cur_posn != string::npos)
    name_value.push_back(next_name_value_pair(str, cur_posn));

  _sent_exchange.clear();
  _received_exchange.clear();

  //auto glambda = [] (auto a) { return a; };

//  auto set_value = [] (auto variable, const string& value)
//    { variable = from_string<decltype(variable)>(value);
//      return true;
//    };

  for (const auto& nv : name_value)
  { bool processed = false;
    const string& name = nv.first;
    const string& value = nv.second;

    if (!processed and (name == "number"))
    { //_number = from_string<decltype(_number)>(value);
      //processed = true;
       processed = ( _number = from_string<decltype(_number)>(value), true );
      //processed = set_value(_number, value);
    }

    if (!processed and (name == "date"))
    { //_date = value;
      //processed = true;
      processed = ( _date = value, true );
      //processed = set_value(_date, value);
    }

    if (!processed and (name == "utc"))
    { //_utc = value;
      //processed = true;
      processed = ( _utc = value, true );
    }

    if (!processed and (name == "mode"))
    { //_mode = ( ( value == "CW") ? MODE_CW : MODE_SSB);
      //processed = true;
      processed = ( _mode = ( ( value == "CW") ? MODE_CW : MODE_SSB), true );
    }

    if (!processed and (name == "frequency"))               // old version
    { _frequency_tx = value;

      const double f = from_string<double>(_frequency_tx);
      const frequency freq(f);

//      _band = static_cast<BAND>(freq);
//      processed = true;
      processed = ( _band = static_cast<BAND>(freq), true );
    }

    if (!processed and (name == "frequency-tx"))
    { _frequency_tx = value;

      if (!_frequency_tx.empty())
      { const double f = from_string<double>(_frequency_tx);
        const frequency freq(f);

        _band = static_cast<BAND>(freq);
      }
      processed = true;
    }

    if (!processed and (name == "frequency-rx"))
    { _frequency_rx = value;

      if (!_frequency_rx.empty())
      { }                                       // add something here when we actually use frequency-rx

      processed = true;
    }

    if (!processed and (name == "hiscall"))
    { _callsign = value;
      _canonical_prefix = location_db.canonical_prefix(_callsign);
      _continent = location_db.continent(_callsign);
      processed = true;
    }

    if (!processed and (name == "mycall"))
    { _my_call = value;
      processed = true;
    }

    if (!processed and (starts_with(name, "sent-")))
    { _sent_exchange.push_back( { to_upper(name.substr(5)), value } );
      processed = true;
    }

    if (!processed and (starts_with(name, "received-")))
    { const string name_upper = to_upper(name.substr(9));

      if (!(rules.all_known_field_names() < name_upper))
      { ost << "Warning: unknown exchange field: " << name_upper << " in QSO: " << *this << endl;
        alert("Unknown exch field: " + name_upper);
      }

      const bool is_possible_mult = rules.is_exchange_mult(name_upper);

      if (is_possible_mult and context.auto_remaining_exchange_mults(name_upper))
        statistics.add_known_exchange_mult(name_upper, value);

      const bool is_mult = is_possible_mult ? statistics.is_needed_exchange_mult(name_upper, value, _band, _mode) : false;
      const received_field rf { name_upper, value , is_possible_mult, is_mult };

      _received_exchange.push_back(rf);
      processed = true;
    }
  }

  _is_country_mult = statistics.is_needed_country_mult(_callsign, _band, _mode);
  _epoch_time = _to_epoch_time(_date, _utc);
}

/*! \brief          Populate from a string (as visible in the log window)
    \param  str     string from visible log window
*/
void QSO::populate_from_log_line(const string& str)
{ ost << "Inside populate_from_log_line(); initial QSO is:" << *this << endl;
  ost << "string = *" << str << "*" << endl;

// separate the line into fields
  const vector<string> vec = remove_peripheral_spaces(split_string(squash(str, ' '), " "));

  if (vec.size() > _log_line_fields.size())                        // output debugging info; this can be triggered if there are mults on the log line
  { ost << "populate_from_log_line parameter: " << str << endl;
    ost << "squashed: " << squash(str, ' ') << endl;

    ost << "Possible problem with number of fields in edited log line" << endl;
    ost << "vec size = " << vec.size() << "; _log_line_fields size = " << _log_line_fields.size() << endl;

    for (size_t n = 0; n < vec.size(); ++n)
      ost << "vec[" << n << "] = " << vec[n] << endl;

    for (size_t n = 0; n < _log_line_fields.size(); ++n)
      ost << "_log_line_fields[" << n << "] = " << _log_line_fields[n] << endl;
  }

  size_t sent_index = 0;
  size_t received_index = 0;

  const vector<exchange_field> exchange_fields = rules.expanded_exch(_callsign, _mode);

  for (size_t n = 0; ( (n < vec.size()) and (n < _log_line_fields.size()) ); ++n)
  { ost << "Processing log_line field number " << n << endl;

    bool processed = false;

    const string& field = _log_line_fields[n];

    ost << "log_line field field name " << field << endl;

    if (!processed and (field == "NUMBER"))
    { _number = from_string<decltype(_number)>(vec[n]);
      processed = true;
    }

    if (!processed and (field == "DATE"))
    { _date = vec[n];
      processed = true;
    }

    if (!processed and (field == "UTC"))
    { _utc = vec[n];
      processed = true;
    }

    if (!processed and (field == "MODE"))
    { _mode = ( (vec[n] == "CW") ? MODE_CW : MODE_SSB);
      processed = true;
    }

    if (!processed and (field == "FREQUENCY"))
    { _frequency_tx = vec[n];

      const double f = from_string<double>(_frequency_tx);
      const frequency freq(f);

      _band = static_cast<BAND>(freq);
      processed = true;
    }

    if (!processed and (field == "CALLSIGN"))
    { _callsign = vec[n];
      processed = true;
    }

    if (!processed and (starts_with(field, "sent-")))
    { if (sent_index < _sent_exchange.size())
        _sent_exchange[sent_index++].second = vec[n];
      processed = true;
    }

    if (!processed and (starts_with(field, "received-")))
    { if (_is_received_field_optional(field, exchange_fields) and !rules.is_legal_value(substring(field, 9), _received_exchange[received_index].value()))  // empty optional field
      { ost << "field = " << field << endl;
        ost << "OPTIONAL AND NOT LEGAL VALUE" << endl;
        ost << "Field to be tested = *" << substring(field, 9) << "*" << endl;
        ost << "Value to be tested = *" << _received_exchange[received_index].value()<< "*" << endl;
        ost << "Is legal value = " << boolalpha << rules.is_legal_value(substring(field, 9), _received_exchange[received_index].value()) << endl;

        _received_exchange[received_index++].value("");

        if (received_index < _received_exchange.size())
          _received_exchange[received_index++].value(vec[n]);  // assume that only one field is optional
      }
      else
      { if (received_index < _received_exchange.size())
        { ost << "NOT OPTIONAL" << endl;
          ost << "About to assign: received_index = " << received_index << "; value = " << vec[n] << endl;

          _received_exchange[received_index++].value(vec[n]);

          ost << "Assigned: " << _received_exchange[received_index - 1] << endl;

          if (starts_with(field, "received-PREC"))               // SS is, as always, special; received-CALL is not in the line, but it's in _received_exchange after PREC
          { //ost << "** received-PREC **" << endl;
            _received_exchange[received_index++].value(_callsign);
            //ost << "updated " << _received_exchange[received_index - 1].name() << " to value " << _received_exchange[received_index - 1].value() << endl;
          }
        }
      }

      processed = true;
    }
  }

  _canonical_prefix = location_db.canonical_prefix(_callsign);
  _continent = location_db.continent(_callsign);
  _epoch_time = _to_epoch_time(_date, _utc);

  ost << "Ending populate_from_log_line(); QSO is now: " << *this << endl;

}

/*! \brief          NEW - Populate from a string (as visible in the log window)
    \param  str     string from visible log window
    \param  mycall  my callsign

    Currently unused
*/
void QSO::new_populate_from_log_line(const string& str, const string& mycall)
{ if (str.empty())
    return;

  const vector<string> fields = remove_peripheral_spaces(split_string(squash(str, ' '), " "));

// number, date, UTC, mode, frequency
  number(from_string<unsigned int>(fields[0]));
  date(fields[1]);
  utc(fields[2]);
  mode( (fields[3] == string("CW") ? MODE_CW : MODE_SSB) );
  freq(fields[4]);

  const double f = from_string<double>(_frequency_tx);
  const frequency freq(f);

  band(static_cast<BAND>(freq));
  callsign(fields[5]);
  canonical_prefix(location_db.canonical_prefix(callsign()));
  continent(location_db.continent(callsign()));
  epoch_time(_to_epoch_time(date(), utc()));

  my_call(mycall);

// the sent exchange starts at fields[6]
  vector<pair<string, string> > tmp_sent_exchange = context.sent_exchange(mode());
  const size_t n_sent_fields = tmp_sent_exchange.size();

  for (size_t field_nr = 0; field_nr < n_sent_fields; ++field_nr)
    tmp_sent_exchange[field_nr].second = fields[6 + field_nr];

  sent_exchange(tmp_sent_exchange);

//  size_t index = 6 + n_sent_fields;      // index into received fields
  vector<string> exchange_field_values;

  for (size_t index = 6 + n_sent_fields; index < fields.size(); ++index)    // there must be a faster way to do this, via the constructor
    exchange_field_values.push_back(fields[index]);

// for now, the last parameter is ignored. Try it this way to see if the results are good; if they
// aren't, then we need to include a separate branch for "true" in the parsed_exchange constructor
  const parsed_exchange pexch(callsign(), canonical_prefix(), rules, mode(), exchange_field_values, true);  // this is relatively slow, but we can't send anything until we know that we have a valid exchange

  if (!pexch.valid())
  { alert("Unable to parse exchange in log line");
    return;
  }
}

/// is any of the exchange fields a mult?
const bool QSO::is_exchange_mult(void) const
{ for (const auto& field : _received_exchange)
    if (field.is_mult())
      return true;

  return false;
}

/*! \brief              Set a field to be an exchange mult
    \param  field_name  name of field

    Does nothing if <i>field_name</i> is not a possible mult
*/
void QSO::set_exchange_mult(const string& field_name)
{ for (auto& field : _received_exchange)
  { if (field.is_possible_mult() and field.name() == field_name)
      field.is_mult(true);
  }
}

/*! \brief                          Re-format according to a Cabrillo template
    \param  cabrillo_qso_template   template for the QSO: line in a Cabrillo file, from configuration file
    \return                         QSO formatted in accordance with <i>cabrillo_qso_template</i>

    Example template:
      CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1
*/
const string QSO::cabrillo_format(const string& cabrillo_qso_template) const
{ static unsigned int record_length = 0;
  static vector< vector< string> > individual_values;
  
  if (!record_length)                                         // if we don't yet know the record length
  { const vector<string> template_fields = split_string(cabrillo_qso_template, ",");         // colon-delimited values
  
    for (const auto& template_field : template_fields)
      individual_values.push_back(split_string(remove_peripheral_spaces(template_field), ":"));
  
    for (const auto& value : individual_values)
    { const unsigned int last_char_posn = from_string<unsigned int>(value[1]) + from_string<unsigned int>(value[2]) - 1;

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
    { if (vec[3][0] == 'R')
        pdirn = PAD_RIGHT;

      if (vec[3].size() == 2)    // if we define a pad char
        pad_char = vec[3][1];
    }

    string value;          // hold the value that we are going to insert
    
/* http://wwrof.org/cabrillo/cabrillo-qso-templates/ says:
    freq is frequency or band:

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

   Astonishingly, nowhere does it say *whose* "frequency or band" it is; we are left to guess.
   I plump for my TX frequency.
*/    
    if (name == "FREQ")
    { if (!_frequency_tx.empty())                                      // frequency is available
        value = to_string(from_string<unsigned int>(_frequency_tx));
      else                                                          // we have only the band; this should never be true
      { static const std::map<BAND, string> BOTTOM_OF_BAND { { BAND_160, "1800" },
                                                             { BAND_80,  "3500" },
                                                             { BAND_40,  "7100" },
                                                             { BAND_20,  "14000" },
                                                             { BAND_15,  "21000" },
                                                             { BAND_10,  "28000" }
                                                           };

        try
        { value = BOTTOM_OF_BAND.at(_band);
        }

        catch (...)
        { value = "1800";    // default
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

        default:
          value = "XX";
          break;
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
    if (starts_with(name, "TEXCH-"))
    { const string field_name = name.substr(6);
    
      if (contains(field_name, "+"))
      { const vector<string> vec = remove_peripheral_spaces(split_string(field_name, "+"));

        for (const auto& name : vec)
        { for (const auto& pss : _sent_exchange)
            if (pss.first == name)
              value = pss.second;
        }
      }
      else
      { for (const auto& pss : _sent_exchange)
          if (pss.first == field_name)
            value = pss.second;
      }
    }

// REXCH-xxx
    if (starts_with(name, "REXCH-"))
    { const string field_name = substring(name, 6);

      if (contains(field_name, "+"))                        // "+" indicates a CHOICE
      { const vector<string> vec = remove_peripheral_spaces(split_string(field_name, "+"));

        for (const auto& name : vec)
        { if (!received_exchange(name).empty())
            value = received_exchange(name);
        }
      }
      else
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
  }

  return record;
}

/*! \brief      Obtain QSO in format for writing to disk (in the actual drlog log)
    \return     QSO formatted for writing to disk
*/
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
  rv += " band=" + pad_string(remove_peripheral_spaces(BAND_NAME[_band]), BAND_WIDTH, PAD_RIGHT);
  rv += " frequency-tx=" + pad_string(_frequency_tx, FREQUENCY_WIDTH, PAD_RIGHT);
  rv += " frequency-rx=" + pad_string( (_frequency_rx.empty() ? "0" : _frequency_rx), FREQUENCY_WIDTH, PAD_RIGHT );
  rv += " mycall=" + pad_string(_my_call, CALLSIGN_WIDTH, PAD_RIGHT);

  for (const auto& exch_field : _sent_exchange)
  { const string name = "sent-" + exch_field.first;
    const auto cit = TX_WIDTH.find(name);
    const string value = (cit == TX_WIDTH.cend() ? exch_field.second : pad_string(exch_field.second, cit->second.first, cit->second.second));
  
    rv += " " + name + "=" + value;
  } 

  for (const auto& field : _received_exchange)
  { const string name = "received-" + field.name();
    const auto cit = RX_WIDTH.find(name);
    const string value = (cit == RX_WIDTH.cend() ? field.value() : pad_string(field.value(), cit->second.first, cit->second.second));

    rv += " " + name + "=" + value;
  }

  rv += " points=" + to_string(_points);
  rv += static_cast<string>(" dupe=") + (_is_dupe ? "true " : "false");
  rv += " comment=" + _comment;
    
  return rv; 
}

/*! \brief                  Does the QSO match an expression for a received exchange field?
    \param  rule_to_match   boolean rule to attempt to match
    \return                 whether the exchange in the QSO matches <i>rule_to_match</i>

    <i>rule_to_match is</i> from the configuration file, and looks like:
      [IOTA != -----]
*/
const bool QSO::exchange_match(const string& rule_to_match) const
{
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

  return false;
}

/*! \brief          Do the values of any of the exchange fields in the QSO match a target string?
    \param  target  target string
    \return         whether any of the exchange fields contain the value <i>target</i>
*/
const bool QSO::exchange_match_string(const string& target) const
{ for (const auto& field : _received_exchange)
    if (field.value() == target)
      return true;

  return false;
}

/*! \brief              Return a single field from the received exchange
    \param  field_name  the name of the field
    \return             the value of <i>field_name</i> in the received exchange

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
const string QSO::received_exchange(const string& field_name) const
{ for (const auto& field : _received_exchange)
  { if (field.name() == field_name)
      return field.value();
  }

  return string();
}

/*! \brief              Return a single field from the sent exchange
    \param  field_name  the name of the field
    \return             the value of <i>field_name</i> in the sent exchange

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
const string QSO::sent_exchange(const string& field_name) const
{ for (const auto& field : _sent_exchange)
  { if (field.first == field_name)
      return field.second;
  }

  return string();
}

/*! \brief              Does the sent exchange include a particular field?
    \param  field_name  the name of the field
    \return             whether <i>field_name</i> is present in the sent exchange
*/
const bool QSO::sent_exchange_includes(const string& field_name) const
{ for (const auto& field : _sent_exchange)
  { if (field.first == field_name)
      return true;
  }

  return false;
}

/*! \brief      Obtain string in format suitable for display in the LOG window
    \return     QSO formatted for writing in the LOG window

    Also populates <i>_log_line_fields</i> to match the returned string
*/
const string QSO::log_line(void)
{ static const map<string, unsigned int> field_widths { { "CHECK",     2 },
                                                        { "CQZONE",    2 },
                                                        { "CWPOWER",   3 },
                                                        { "DOK",       1 },
                                                        { "GRID",      4 },
                                                        { "ITUZONE",   2 },
                                                        { "NAME",      6 },
                                                        { "PREC",      1 },
                                                        { "RDA",       4 },
                                                        { "RS",        2 },
                                                        { "RST",       3 },
                                                        { "SECTION",   3 },
                                                        { "SKCCNO",    6 },
                                                        { "SPC",       3 },
                                                        { "SSBPOWER",  4 },
                                                        { "UKEICODE",  2 },
                                                        { "160MSTATE", 2 },
                                                        { "10MSTATE",  3 }
                                                      };
  static const size_t CALL_FIELD_LENGTH = 12;

  string rv = pad_string(to_string(number()), 5);

  rv += pad_string(date(), 11);
  rv += pad_string(utc(), 9);
  rv += pad_string(MODE_NAME[mode()], 4);
  rv += pad_string(freq(), 8);
  rv += pad_string(pad_string(callsign(), CALL_FIELD_LENGTH, PAD_RIGHT), CALL_FIELD_LENGTH + 1);

  FOR_ALL(_sent_exchange, [&] (pair<string, string> se) { rv += " " + se.second; });

// print in same order they are present in the config file
  for (const auto& field : _received_exchange)
  { unsigned int field_width = QSO_MULT_WIDTH;

    const string& name = field.name();

// skip the CALL field from SS, since it's already on the line
    if (name != "CALL" and name != "CALLSIGN")
    { try
      { field_width = field_widths.at(name);
      }

      catch (...)
      {
      }

      rv += " " + pad_string(field.value(), field_width);
    }
  }

// mults

// callsign mult
  if (_is_prefix_mult)
    rv += pad_string(_prefix, 5);

// country mult
  if (QSO_DISPLAY_COUNTRY_MULT)                                                         // set in drlog_context when parsing the config file
    rv += (_is_country_mult ? pad_string(_canonical_prefix, 5, PAD_LEFT) : "     ");    // sufficient for VP2E

// exchange mult
  for (const auto& field : _received_exchange)
  { unsigned int field_width = QSO_MULT_WIDTH;
    const string& name = field.name();

    try
    { field_width = field_widths.at(name);
    }

    catch (...)
    { }

    rv += (field.is_mult() ? pad_string(MULT_VALUE(name, field.value()), field_width + 1) : "");  // TODO: think about a way to make this in a different colour
  }

  _log_line_fields.clear();    // make sure it's empty before we fill it

  _log_line_fields.push_back("NUMBER");
  _log_line_fields.push_back("DATE");
  _log_line_fields.push_back("UTC");
  _log_line_fields.push_back("MODE");
  _log_line_fields.push_back("FREQUENCY");
  _log_line_fields.push_back("CALLSIGN");

  for (const auto& exch_field : _sent_exchange)
    _log_line_fields.push_back("sent-" + exch_field.first);

  for (const auto& field : _received_exchange)
  { if (field.name() != "CALL")                               // SS is special
      _log_line_fields.push_back("received-" + field.name());
  }

  return rv;
}

/*! \brief      QSO == QSO
    \param  q   target QSO
    \return     whether the two QSOs are equal, for the purpose of comparison within a deque

    Only "important" members are compared.
*/
const bool QSO::operator==(const QSO& q) const
{  if (_callsign != q._callsign)
    return false;

  if (_date != q._date)
    return false;

  if (_epoch_time != q._epoch_time)
    return false;

  if (_frequency_rx != q._frequency_rx)
      return false;

  if (_mode != q._mode)
      return false;

  if (_my_call != q._my_call)
      return false;

  if (_received_exchange != q._received_exchange)
      return false;

  if (_sent_exchange != q._sent_exchange)
      return false;

  if (_utc != q._utc)
      return false;

  return true;
}

/*! \brief          Write a <i>QSO</i> object to an output stream
    \param  ost     output stream
    \param  q       object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const QSO& q)
{ ost << "Number: " << q.number()
      << ", Date: " << q.date()
      << ", UTC: " << q.utc()
      << ", Call: " << q.callsign()
      << ", Mode: " << MODE_NAME[q.mode()]
      << ", Band: " << BAND_NAME[q.band()]
      << ", Freq: " << q.freq()
      << ", Sent: ";
      
  const vector<pair<string, string> > sent_exchange = q.sent_exchange();
  for (unsigned int n = 0; n < sent_exchange.size(); ++n)
    ost << sent_exchange[n].first << " " << sent_exchange[n].second << " ";    

  ost << ", Rcvd: ";

  const vector<received_field> received_exchange = q.received_exchange();

  for (const auto& received_exchange_field : received_exchange)
    ost << received_exchange_field << "  ";

  ost << ", Comment: " << q.comment()
      << ", Points: " << q.points();
  
  return ost;
}

/*! \brief          Obtain the next name and value from a drlog-format line
    \param  str     a drlog-format line
    \param  posn    character position within line
    \return         the next (<i>i.e.</i>, after <i>posn</i>) name and value separated by an "="

    Correctly handles extraneous spaces in <i>str</i>.
    <i>str</i> looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    The value of <i>posn</i> might be changed by this function.
*/
const pair<string, string> next_name_value_pair(const string& str, size_t& posn)
{ static const pair<string, string> empty_pair;

  if (posn >= str.size())
  { //posn = string::npos;
    //return empty_pair;
    return ( posn = string::npos, empty_pair);
  }

  const size_t first_char_posn = str.find_first_not_of(" ", posn);

  if (first_char_posn == string::npos)
  { //posn = string::npos;
    //return empty_pair;
    return ( posn = string::npos, empty_pair);
  }

  const size_t equals_posn = str.find("=", first_char_posn);

  if (equals_posn == string::npos)
  { //posn = string::npos;
    //return empty_pair;
    return ( posn = string::npos, empty_pair);
  }

  const string name = remove_peripheral_spaces(str.substr(first_char_posn, equals_posn - first_char_posn));
  const size_t value_first_char_posn = str.find_first_not_of(" ", equals_posn + 1);

  if (value_first_char_posn == string::npos)
  { //posn = string::npos;
    //return empty_pair;
    return ( posn = string::npos, empty_pair);
  }

  const size_t space_posn = str.find(" ", value_first_char_posn);
  const string value = (space_posn == string::npos) ? str.substr(value_first_char_posn)
                                                    : str.substr(value_first_char_posn, space_posn - value_first_char_posn);

// handle "frequency_rx=     mycall=N7DR"
  if (contains(value, "="))
  { posn = value_first_char_posn;
    return pair<string, string> { name, string() };
  }

  posn = space_posn;

  return pair<string, string> { name, value };
}


