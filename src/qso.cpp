// $Id: qso.cpp 279 2025-12-01 15:09:34Z  $

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

extern contest_rules     rules;          ///< rules for this contest
extern drlog_context     context;        ///< configuration context
extern location_database location_db;    ///< location database
extern message_stream    ost;            ///< for debugging, info

extern void alert(const string_view msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);     ///< alert the user

bool         QSO_DISPLAY_COUNTRY_MULT { true };   ///< whether to display country mults in log window (may be changed in config file)
unsigned int QSO_MULT_WIDTH           { 5 };      ///< default width of QSO mult fields in log window

/*! \brief                      Is a particular field that might be received as part of the exchange optional?
    \param  field_name          the name of the field
    \param  fields_from_rules   the possible fields, taken from the rules
    \return                     whether field <i>field_name</i> is optional

    Works regardless of whether <i>field_name</i> includes an initial "received-" string
*/
bool QSO::_is_received_field_optional(const string_view field_name, const vector<exchange_field>& fields_from_rules) const
{ string_view name_copy { remove_from_start <std::string_view> (field_name, "received-"s) };

  for (const auto& ef : fields_from_rules)
  { if (ef.name() == name_copy)
      return ef.is_optional();
  }

// should never get here
  ost << "ERROR; unable to find field in _is_received_file_optional" << endl;

  return false;             // keep the compiler happy
}

/*! \brief      Process a name/value pair from a drlog log line to insert values in the QSO object
    \param  nv  name and value to be processed
    \return     whether <i>nv</i> was processed

    Does not process fields whose name begins with "received-"
*/
bool QSO::_process_name_value_pair(const pair<string, string>& nv)
{ bool processed { false };

  const auto& [ name, value ] { nv };

  if (!processed and (name == "number"sv))
    processed = ( _number = from_string<decltype(_number)>(value), true );

  if (!processed and (name == "date"sv))
    processed = ( _date = value, true );

  if (!processed and (name == "utc"sv))
    processed = ( _utc = value, true );

  if (!processed and (name == "mode"sv))
    processed = ( _mode = ( ( value == "CW"sv) ? MODE_CW : MODE_SSB), true );

  if (!processed and (name == "frequency"sv))               // old version
  { _frequency_tx = value;

    const double    f    { from_string<double>(_frequency_tx) };
    const frequency freq { f };

    processed = ( _band = static_cast<BAND>(freq), true );
  }

  if (!processed and (name == "frequency-tx"sv))
  { _frequency_tx = value;

    if (!_frequency_tx.empty())
    { const double    f    { from_string<double>(_frequency_tx) };
      const frequency freq { f };

      _band = static_cast<BAND>(freq);
    }
    processed = true;
  }

  if (!processed and (name == "frequency-rx"sv))
  { _frequency_rx = value;

    if (!_frequency_rx.empty())
    { }                                       // add something here when we actually use frequency-rx

    processed = true;
  }

  if (!processed and (name == "hiscall"sv))
  { _callsign = value;
    _canonical_prefix = location_db.canonical_prefix(_callsign);
    _continent = location_db.continent(_callsign);
    processed = true;
  }

  if (!processed and (name == "mycall"sv))
    processed = ( _my_call = value, true );

  if (!processed and name.starts_with("sent-"sv))
    processed = ( _sent_exchange += { to_upper(name.substr(5)), value }, true );

  if (!processed and (name == "sap"sv))
    processed = ( _is_sap = ((value == "true") ? true : false), true );

  return processed;
}

/*! \brief               Obtain the epoch time from a date and time in drlog format
    \param  date_str     date string in drlog format
    \param  utc_str      time string in drlog format
    \return              time in seconds since the UNIX epoch
*/
time_t QSO::_to_epoch_time(const string_view date_str, const string_view utc_str) const
{ struct tm time_struct;

  time_struct.tm_sec  = from_string<int>(utc_str.substr(6, 2));
  time_struct.tm_min  = from_string<int>(utc_str.substr(3, 2));
  time_struct.tm_hour = from_string<int>(utc_str.substr(0, 2));
  time_struct.tm_mday = from_string<int>(date_str.substr(8, 2));
  time_struct.tm_mon  = from_string<int>(date_str.substr(5, 2)) - 1;
  time_struct.tm_year = from_string<int>(date_str.substr(0, 4)) - 1900;

  return timegm(&time_struct);    // GNU function; see time_gm man page for portable alternative.
                                  // To implement alternative: declare a class whose constructor sets
                                  // TZ and calls tzset(), then call the constructor globally
}

/// constructor
QSO::QSO(void) :
  _epoch_time(time(NULL))           // now
{ constexpr int BUFSIZE { 26 };     // see man page for asctime_r

  struct tm    structured_time;

  gmtime_r(&_epoch_time, &structured_time);     // convert to UTC
  
  array<char, BUFSIZE> buf;                     // buffer to hold the ASCII date/time info; see man page for gmtime()
  
  asctime_r(&structured_time, buf.data());      // convert to ASCII

  const string ascii_time(buf.data(), BUFSIZE);
  
  _utc  = ascii_time.substr(11, 8);                                                     // hh:mm:ss
  _date = to_string(structured_time.tm_year + 1900) + "-"s + pad_leftz((structured_time.tm_mon + 1), 2) + "-"s + pad_leftz(structured_time.tm_mday, 2);   // yyyy-mm-dd
}

/*! \brief              Constructor from a line in the disk log
    \param  context     drlog context
    \param  str         string from log file
    \param  rules       rules for this contest
    \param  statistics  contest statistics

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    <i>statistics</i> might be changed by this function
*/
QSO::QSO(const drlog_context& context, const string_view str, const contest_rules& rules, running_statistics& statistics)
{ *this = QSO();

  populate_from_verbose_format(context, str, rules, statistics);
}

/// set TX frequency and band from a string of the form xxxxx.y
void QSO::freq_and_band(const string_view str)
{ freq(str);

  const unsigned int qrg      { from_string<unsigned int>(str) };
  const BAND         qrg_band { static_cast<BAND>(frequency(qrg)) };

  band(qrg_band);
}

/*! \brief              Read fields from a line in the disk log
    \param  context     drlog context
    \param  str         string from log file
    \param  rules       rules for this contest
    \param  statistics  contest statistics

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    <i>statistics</i> might be changed by this function
*/
void QSO::populate_from_verbose_format(const drlog_context& context, const string_view str, const contest_rules& rules, running_statistics& statistics)
{
// build a vector of name/value pairs
  size_t cur_posn { min(static_cast<size_t>(5), str.size()) };  // skip the "QSO: "

  vector<pair<string, string> > name_values;

  while (cur_posn != string_view::npos)
    name_values += next_name_value_pair( string { str }, cur_posn);

  _sent_exchange.clear();
  _received_exchange.clear();

  for (const auto& [field_name, field_value] : name_values)
  { bool processed { _process_name_value_pair(field_name, field_value) };

    if (!processed and field_name.starts_with("received-"s))
    { const string name_upper { to_upper(field_name.substr(9)) };

      if (!rules.all_known_field_names().contains(name_upper))
      { ost << "Warning: unknown exchange field: " << name_upper << " in QSO: " << *this << endl;
        alert("Unknown exch field: "s + name_upper);
      }

      const bool is_possible_mult { rules.is_exchange_mult(name_upper) };

      if (is_possible_mult and context.auto_remaining_exchange_mults(name_upper))
        statistics.add_known_exchange_mult(name_upper, field_value);

      const bool           is_mult { is_possible_mult ? statistics.is_needed_exchange_mult(name_upper, field_value, _band, _mode) : false };
      const received_field rf      { name_upper, field_value , is_possible_mult, is_mult };

      _received_exchange += rf;
      processed = true;
    }
  }

  _is_country_mult = statistics.is_needed_country_mult(_callsign, _band, _mode, rules);
  _epoch_time = _to_epoch_time(_date, _utc);
}

/*! \brief              Read fields from a line in the disk log
    \param  str         line from log file

    line in disk log looks like:
      QSO: number=    1 date=2013-02-18 utc=20:21:14 hiscall=GM100RSGB    mode=CW  band= 20 frequency=14036.0 mycall=N7DR         sent-RST=599 sent-CQZONE= 4 received-RST=599 received-CQZONE=14 points=1 dupe=false comment=

    Performs a skeletal setting of values, without using the rules for the contest; used by simulator
*/
void QSO::populate_from_verbose_format(const string_view str)
{
// build a vector of name/value pairs
  size_t cur_posn { min(static_cast<size_t>(5), str.size()) };  // skip the "QSO: "

  vector<pair<string, string> > name_values;

  while (cur_posn != string::npos)
    name_values += next_name_value_pair(str, cur_posn);

  _sent_exchange.clear();
  _received_exchange.clear();

  for (const auto& [field_name, field_value] : name_values)
  { bool processed { _process_name_value_pair(field_name, field_value) };

    if (!processed and field_name.starts_with("received-"s))
    { const string         name_upper { to_upper(field_name.substr(9)) };
      const received_field rf         { name_upper, field_value , false, false };    // just populate name and value

      _received_exchange += rf;
      processed = true;
    }
  }

  _epoch_time = _to_epoch_time(_date, _utc);
}

/*! \brief          Populate from a string (as visible in the log window)
    \param  str     string from visible log window

    Note: this might result in an incorrect value for _is_sap
*/
void QSO::populate_from_log_line(const string_view str)
{ ost << "Inside populate_from_log_line(); input string is:" << *this << endl;
  ost << "string = *" << str << "*" << endl;

// separate the line into fields
  const vector<string> vec { clean_split_string <string> (squash(str, ' '), ' ') };

  if (vec.size() > _log_line_fields.size())                        // output debugging info; this can be triggered if there are mults on the log line
  { ost << "populate_from_log_line parameter: " << str << endl;
    ost << "squashed: " << squash(str, ' ') << endl;

    ost << "Possible problem with number of fields in edited log line" << endl;
    ost << "vec size = " << vec.size() << "; _log_line_fields size = " << _log_line_fields.size() << endl;

    for (size_t n { 0 }; n < vec.size(); ++n)
      ost << "vec[" << n << "] = " << vec[n] << endl;

    for (size_t n { 0 }; n < _log_line_fields.size(); ++n)
      ost << "_log_line_fields[" << n << "] = " << _log_line_fields[n] << endl;
  }

  size_t sent_index     { 0 };
  size_t received_index { 0 };

  const vector<exchange_field> exchange_fields { rules.expanded_exch(_callsign, _mode) };

  ost << "number of received fields in string: " << _received_exchange.size() << endl;

  for (size_t idx { 0 }; idx < _received_exchange.size(); ++idx)
    ost << "  [" << idx << "] = " << _received_exchange[idx].value() << endl;

  for (size_t n { 0 }; ( (n < vec.size()) and (n < _log_line_fields.size()) ); ++n)
  { ost << "Processing log_line field number " << n << endl;
    const string& field_value { vec[n] };

    bool processed { false };

    const string& field { _log_line_fields[n] };

    ost << "log_line field field name " << field << endl;

    if (!processed and (field == "NUMBER"sv))
      processed = (_number = from_string<decltype(_number)>(field_value), true);

    if (!processed and (field == "DATE"sv))
      processed = (_date = field_value, true);

    if (!processed and (field == "UTC"sv))
      processed = (_utc = field_value, true);

    if (!processed and (field == "MODE"sv))
      processed = (_mode = ( (field_value == "CW"sv) ? MODE_CW : MODE_SSB), true);

    if (!processed and (field == "FREQUENCY"sv))
    { _frequency_tx = field_value;

      const double    f    { from_string<double>(_frequency_tx) };
      const frequency freq { f };

      processed = (_band = static_cast<BAND>(freq), true);
    }

    if (!processed and (field == "CALLSIGN"sv))
    { _callsign = field_value;
      _canonical_prefix = location_db.canonical_prefix(_callsign);
      _continent = location_db.continent(_callsign);

      processed = true;
    }

    if (!processed and field.starts_with("sent-"sv))
    { if (sent_index < _sent_exchange.size())
        _sent_exchange[sent_index++].second = field_value;
      processed = true;
    }

// need to handle the cases for an optional field:
//  not present in original and not present in new
//  not present in original and present in new
//  present in original and not present in new
//  present in original and present in new

    if (!processed and field.starts_with("received-"sv))
    { if (_is_received_field_optional(field, exchange_fields))
      { ost << "OPTIONAL received field = " << field << endl;

        const bool present_original { !_received_exchange[received_index].value().empty() };

        if (present_original)
          ost << "field is present in original: " << _received_exchange[received_index].value() << endl;
        else
          ost << "field is NOT present in original" << endl;

        const bool present_new { !field_value.empty() };

        if (present_new)
          ost << "field is present in new: " << field_value << endl;
        else
          ost << "field is NOT present in new" << endl;

// not present in either
        if (!present_original and !present_new)
        { ost << "field is not present in either original or new; not updated" << endl;
          received_index++;
        }

// if present in new
       if (present_new)
       { if (field_value == _received_exchange[received_index].value())
         { ost << "field is unchanged; not updated" << endl;
           received_index++;
         }
         else
         { const bool is_legal_value { rules.is_legal_value(substring <std::string_view> (field, 9), field_value) };

           if (is_legal_value)
             ost << "field value " << field_value << " is legal" << endl;
           else
             ost << "field value " << field_value << " is NOT legal" << endl;

           if (is_legal_value)
             _received_exchange[received_index++].value(field_value);
         }
       }
       else    // not present in new, but was present in original
       { ost << "field has been removed in new" << endl;
        _received_exchange[received_index++].value(string { });
       }
      }
      else
      { if (received_index < _received_exchange.size())
        { ost << "NOT OPTIONAL" << endl;
          ost << "About to assign: received_index = " << received_index << "; value = " << field_value << endl;

          ost << "Original received exchange[received_index]: " << _received_exchange[received_index] << endl << endl;

          const bool is_legal { rules.is_legal_value(substring <std::string> (field, 9), vec[n]) };

          ost << field_value << " IS " << (is_legal ? "" : "NOT ") << "a legal value for " << _received_exchange[received_index].name() << endl;
          ost << field_value << " IS " << (is_legal ? "" : "NOT ") << "a legal value for " <<  substring <std::string_view> (field, 9) << endl;

// if the field is a CHOICE and the value isn't legal for the original choice, see if it's valid for the other
          if (!rules.is_legal_value(substring <std::string> (field, 9), field_value))
          { const string&             original_field_name { _received_exchange[received_index].name() };
            const choice_equivalents& ec                  { rules.equivalents(_mode, _canonical_prefix) };

            if (ec.is_choice(original_field_name))
            { const string& alternative_choice_field_name { ec.other_choice(original_field_name) };

              ost << "original field name = " << original_field_name << ", alternative field name = " << alternative_choice_field_name << endl;

              _received_exchange[received_index].name(alternative_choice_field_name);

              if (!rules.is_legal_value(alternative_choice_field_name, field_value))
              { ost << "UNABLE TO PARSE REVISED EXCHANGE CORRECTLY" << endl;

                alert("Unable to parse exchange; making ad hoc decision");
              }
            }
          }

          _received_exchange[received_index++].value(field_value);

          ost << "Assigned: " << _received_exchange[received_index - 1] << endl;

          if (field.starts_with("received-PREC"sv))               // SS is, as always, special; received-CALL is not in the line, but it's in _received_exchange after PREC
            _received_exchange[received_index++].value(_callsign);
        }
      }

      processed = true;
    }
  }

  _epoch_time = _to_epoch_time(_date, _utc);

  ost << "Ending populate_from_log_line(); QSO is now: " << *this << endl;
}

/*! \brief              Set a field to be an exchange mult
    \param  field_name  name of field

    Does nothing if <i>field_name</i> is not a possible mult
*/
void QSO::set_exchange_mult(const string_view field_name)
{ for (auto& field : _received_exchange)
  { if (field.is_possible_mult() and (field.name() == field_name))
      field.is_mult(true);
  }
}

/*! \brief                          Re-format according to a Cabrillo template
    \param  cabrillo_qso_template   template for the QSO: line in a Cabrillo file, from configuration file
    \return                         QSO formatted in accordance with <i>cabrillo_qso_template</i>

    Example template:
      CABRILLO QSO = FREQ:6:5:L, MODE:12:2, DATE:15:10, TIME:26:4, TCALL:31:13:R, TEXCH-RST:45:3:R, TEXCH-CQZONE:49:6:R, RCALL:56:13:R, REXCH-RST:70:3:R, REXCH-CQZONE:74:6:R, TXID:81:1
*/
string QSO::cabrillo_format(const string_view cabrillo_qso_template) const
{ static unsigned int record_length { 0 };

  static vector< vector<string> > individual_values;
  
  if (!record_length)                                         // if we don't yet know the record length
  { const vector<string_view> template_fields { split_string <std::string_view> (cabrillo_qso_template, ',') };         // colon-delimited values
  
    for (const auto& template_field : template_fields)
      individual_values += split_string <std::string> (remove_peripheral_spaces <std::string_view> (template_field), ':');
  
    for (const auto& value : individual_values)
    { const unsigned int last_char_posn { from_string<unsigned int>(value[1]) + from_string<unsigned int>(value[2]) - 1 };

      record_length = max(last_char_posn, record_length);    
    }
  }
  
// create a record full of spaces
  string record(record_length, ' ');
  
  record.replace(0, 4, "QSO:"s);             // put into record
  
// go through every possibility for every field
  for (unsigned int n { 0 }; n < individual_values.size(); ++n)
  { const vector<string>& vec  { individual_values[n] };
    const string          name { vec[0] };
    const unsigned int    posn { from_string<unsigned int>(vec[1]) - 1 };
    const unsigned int    len  { from_string<unsigned int>(vec[2]) };

    PAD  pdirn    { PAD::LEFT };
    char pad_char { ' ' };
      
    if (vec.size() == 4)
    { if (vec[3][0] == 'R')
        pdirn = PAD::RIGHT;

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
    if (name == "FREQ"sv)
    { if (!_frequency_tx.empty())                                      // frequency is available
        value = to_string(from_string<unsigned int>(_frequency_tx));
      else                                                          // we have only the band; this should never be true
      { try
        { value = BOTTOM_OF_BAND.at(_band);
        }

        catch (...)
        { value = "1800"s;    // default
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
    if (name == "MODE"sv)
    { switch (_mode)
      { case MODE_CW:
          value = "CW"s;
         break;

        case MODE_SSB:
          value = "PH"s;
          break;
//  case MODE_DIGI:
//    value = "RY";
//    break;

        default:
          value = "XX"s;
          break;
      }
    }
    
/*
date is UTC date in yyyy-mm-dd form
*/      
    if (name == "DATE"sv)
      value = _date;
      
/*
time is UTC time in nnnn form; the "specification" doesn't bother to tell
us whether to round or to truncate.  Truncation is easier, so until the
specification tells us otherwise, that's what we do.
*/
    if (name == "TIME"sv)
      value = _utc.substr(0, 2) + _utc.substr(3, 2);
  
// TCALL == transmitted call == my call
    if (name == "TCALL"sv)
      value = _my_call;
      
// TEXCH-xxx
    if (name.starts_with("TEXCH-"sv))
    { const string field_name { name.substr(6) };
    
      if (field_name.contains('+'))                        // "+" indicates a CHOICE
      { for (const auto& name : clean_split_string <string> (field_name, '+'))
        { for (const auto& [nm, val] : _sent_exchange)
            if (nm == name)
              value = val;
        }
      }
      else
      { for (const auto& [nm, val] : _sent_exchange)
          if (nm == field_name)
            value = val;
      }
    }

// REXCH-xxx
    if (name.starts_with("REXCH-"sv))
    { const string field_name { remove_from_start <std::string> (name, "REXCH-"sv) };

//      if (contains(field_name, '+'))                        // "+" indicates a CHOICE
      if (field_name.contains('+'))                        // "+" indicates a CHOICE
      { for (const auto& name : clean_split_string <string> (field_name, '+'))
        { if (!received_exchange(name).empty())
            value = received_exchange(name);
        }
      }
      else
        value = received_exchange(field_name);  // don't use _received_exchange[field_name] because that's not const
    }
    
// RCALL
    if (name == "RCALL"sv)
      value = _callsign;  

// TXID
    if (name == "TXID"sv)
      value = "0"s;
  
    value = pad_string(value, len, pdirn, pad_char);
    record.replace(posn, len, value);             // put into record
  }

  return record;
}

/*! \brief      Obtain QSO in format for writing to disk (in the actual drlog log)
    \return     QSO formatted for writing to disk
*/
string QSO::verbose_format(void) const
{ constexpr int NUMBER_WIDTH    { 5 };
  constexpr int CALLSIGN_WIDTH  { 12 };
  constexpr int MODE_WIDTH      { 3 };
  constexpr int BAND_WIDTH      { 3 };
  constexpr int FREQUENCY_WIDTH { 7 };

  static const STRING_MAP</* tx field name */ pair< int /* width */, PAD> > TX_WIDTH { { "sent-RST"s,    { 3, PAD::LEFT } },
                                                                                        { "sent-CQZONE"s, { 2, PAD::LEFT } }
                                                                                      };

  static const STRING_MAP</* tx field name */ pair< int /* width */, PAD> > RX_WIDTH { { "received-RST"s,    { 3, PAD::LEFT } },
                                                                                        { "received-CQZONE"s, { 2, PAD::LEFT } }
                                                                                      };

  string rv;

  rv = "QSO: "s;
  
  rv += "number="s        + pad_left(_number, NUMBER_WIDTH);
  rv += " date="s         + _date;
  rv += " utc="s          + _utc;
  rv += " hiscall="s      + pad_right(_callsign, CALLSIGN_WIDTH);
  rv += " mode="s         + pad_right(remove_peripheral_spaces <std::string> (MODE_NAME[_mode]), MODE_WIDTH);
  rv += " band="s         + pad_right(remove_peripheral_spaces <std::string> (BAND_NAME[_band]), BAND_WIDTH);
  rv += " frequency-tx="s + pad_right(_frequency_tx, FREQUENCY_WIDTH);
  rv += " frequency-rx="s + pad_right( (_frequency_rx.empty() ? "0"s : _frequency_rx), FREQUENCY_WIDTH );
  rv += " mycall="s       + pad_right(_my_call, CALLSIGN_WIDTH);

  for (const auto& exch_field : _sent_exchange)
  { const string name  { "sent-"s + exch_field.first };
    const auto   cit   { TX_WIDTH.find(name) };
    const string value { (cit == TX_WIDTH.cend() ? exch_field.second : pad_string(exch_field.second, cit->second.first, cit->second.second)) };
  
    rv += (SPACE_STR + name + "="s + value);
  } 

  for (const auto& field : _received_exchange)
  { const string name  { "received-"s + field.name() };
    const auto   cit   { RX_WIDTH.find(name) };
    const string value { (cit == RX_WIDTH.cend() ? field.value() : pad_string(field.value(), cit->second.first, cit->second.second)) };

    rv += (SPACE_STR + name + "="s + value);
  }

  rv += " points="s + to_string(_points);
  rv += " dupe="s + (_is_dupe ? "true"s : "false"s);
  rv += " sap="s + (_is_sap ? "true"s : "false"s);
  rv += " comment="s + _comment;
    
  return rv; 
}

/*! \brief                  Does the QSO match an expression for a received exchange field?
    \param  rule_to_match   boolean rule to attempt to match
    \return                 whether the exchange in the QSO matches <i>rule_to_match</i>

    <i>rule_to_match is</i> from the configuration file, and looks like:
      [IOTA != -----]

    I don't think that this is currently used.
*/
bool QSO::exchange_match(const string_view rule_to_match) const
{
// remove the [] markers
  const string         target { rule_to_match.substr(1, rule_to_match.length() - 2) };
  const vector<string> tokens { split_string <std::string> (target, ' ') };

  if (tokens.size() != 3)
    ost << "Number of tokens = " << tokens.size() << endl;
  else                          // three tokens
  { //const string exchange_field_name { remove_peripheral_spaces <std::string> (tokens[0]) };                     // does not include the REXCH-, since it's taken directly from the logcfg.dat file
    const string_view exchange_field_name { remove_peripheral_spaces <std::string_view> (tokens[0]) };                     // does not include the REXCH-, since it's taken directly from the logcfg.dat file

// is this field present?
    const string exchange_field_value { received_exchange(exchange_field_name) };   // default is empty field

// now try the various legal operations
// !=
    if (!remove_leading_spaces <std::string_view> (exchange_field_value).empty())        // only check if we actually received something; catch the empty and all-spaces cases
    { const string_view op { remove_peripheral_spaces <std::string_view> (tokens[1]) };

      if (op == "!="sv)                                                // precise inequality
      { const string_view target { remove_trailing <std::string_view> (remove_leading <std::string_view> (remove_peripheral_spaces <std::string_view> (tokens[2]), '"'), '"') };  // strip any double quotation marks

        ost << "matched operator: " << op << endl;
        ost << "exchange field value: *" << exchange_field_value << "* " << endl;
        ost << "target: *" << target << "* " << endl;

        return (exchange_field_value != target);
      }
    }
  }

  return false;
}

/*! \brief              Return a single field from the received exchange
    \param  field_name  the name of the field
    \return             the value of <i>field_name</i> in the received exchange

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
string QSO::received_exchange(const string_view field_name) const
{ for (auto field : _received_exchange)
  { if (field.name() == field_name)
      return field.value();
  }

  return string { };
}

/*! \brief              Return a single field from the sent exchange
    \param  field_name  the name of the field
    \return             the value of <i>field_name</i> in the sent exchange

    Returns the empty string if <i>field_name</i> is not found in the exchange
*/
string QSO::sent_exchange(const string_view field_name) const
{ for (const auto& [name, value] : _sent_exchange)
    if (name == field_name)
      return value;

  return string { };
}

/*! \brief      Obtain string in format suitable for display in the LOG window
    \return     QSO formatted for writing into the LOG window

    Also populates <i>_log_line_fields</i> to match the returned string
*/
string QSO::log_line(void)
{ static const UNORDERED_STRING_MAP<unsigned int> field_widths { { "CHECK"s,     2 },
                                                                 { "CQZONE"s,    2 },
                                                                 { "CWPOWER"s,   3 },
                                                                 { "DOK"s,       1 },
                                                                 { "GRID"s,      4 },
                                                                 { "ITUZONE"s,   2 },
                                                                 { "NAME"s,      6 },
                                                                 { "PREC"s,      1 },
                                                                 { "RDA"s,       4 },
                                                                 { "RS"s,        2 },
                                                                 { "RST"s,       3 },
                                                                 { "SECTION"s,   3 },
                                                                 { "SKCCNO"s,    6 },
                                                                 { "SOCIETY"s,   5 },
                                                                 { "SPC"s,       3 },
                                                                 { "SSBPOWER"s,  4 },
                                                                 { "UKEICODE"s,  2 },
                                                                 { "160MSTATE"s, 2 },
                                                                 { "10MSTATE"s,  3 }
                                                               };
  constexpr size_t CALL_FIELD_LENGTH      { 12 };
  constexpr size_t DATE_FIELD_LENGTH      { 11 };
  constexpr size_t FREQUENCY_FIELD_LENGTH { 8 };
  constexpr size_t MODE_FIELD_LENGTH      { 4 };
  constexpr size_t NUMBER_FIELD_LENGTH    { 5 };
  constexpr size_t PREFIX_FIELD_LENGTH    { 5 };    // sufficient for VP2E
  constexpr size_t UTC_FIELD_LENGTH       { 9 };

  static const string empty_prefix_str { space_string(PREFIX_FIELD_LENGTH) };

  string rv { pad_left(to_string(number()), NUMBER_FIELD_LENGTH) };

  rv += pad_left(date(), DATE_FIELD_LENGTH);
  rv += pad_left(utc(), UTC_FIELD_LENGTH);
  rv += pad_left(MODE_NAME[mode()], MODE_FIELD_LENGTH);
  rv += pad_left(freq(), FREQUENCY_FIELD_LENGTH);
  rv += pad_left(pad_right(callsign(), CALL_FIELD_LENGTH), CALL_FIELD_LENGTH + 1);

  FOR_ALL(_sent_exchange, [&rv] (pair<string, string> se) { rv += (SPACE_STR + se.second); });

// print in same order they are present in the config file
  for (const auto& field : _received_exchange)
  { unsigned int field_width { QSO_MULT_WIDTH };

    const string name { field.name() };

// skip the CALL field from SS, since it's already on the line
    if ( (name != "CALL"sv) and (name != "CALLSIGN"sv) )
    { try
      { field_width = field_widths.at(name);
      }

      catch (...)
      {
      }

      rv += (SPACE_STR + pad_left(field.value(), field_width));
    }
  }

// mults

// callsign mult
  if (_is_prefix_mult)
    rv += pad_left(_prefix, PREFIX_FIELD_LENGTH);

// country mult
  if (QSO_DISPLAY_COUNTRY_MULT)                                                                         // set in drlog_context when parsing the config file
    rv += (_is_country_mult ? pad_left(_canonical_prefix, PREFIX_FIELD_LENGTH) : empty_prefix_str);     // sufficient for VP2E

// exchange mult
  for (const auto& field : _received_exchange)
  { unsigned int field_width { QSO_MULT_WIDTH };    // default width

    const string name { field.name() };

    try
    { field_width = field_widths.at(name);
    }

    catch (...)
    { }

    rv += (field.is_mult() ? pad_left(MULT_VALUE(name, field.value()), field_width + 1) : EMPTY_STR);  // TODO: think about a way to make this in a different colour
  }

  _log_line_fields.clear();    // make sure it's empty before we fill it

  static const vector<string> log_fields { "NUMBER"s, "DATE"s, "UTC"s, "MODE"s, "FREQUENCY"s, "CALLSIGN"s };

  FOR_ALL(log_fields,         [this] (const string& log_field) { _log_line_fields += log_field; } );
  FOR_ALL(_sent_exchange,     [this] (const auto& exch_field)  { _log_line_fields += ("sent-"s + exch_field.first); } );
  FOR_ALL(_received_exchange, [this] (const auto& field)       { if (field.name() != "CALL"sv)
                                                                   _log_line_fields += ("received-"s + field.name());
                                                               } );

  return rv;
}

/*! \brief      QSO == QSO
    \param  q   target QSO
    \return     whether the two QSOs are equal, for the purpose of comparison within a deque

    Only "important" members are compared.
*/
bool QSO::operator==(const QSO& q) const
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

  for (const auto& [sent_name, sent_value] : q.sent_exchange())
    ost << sent_name << " " << sent_value << " ";

  ost << ", Rcvd: ";

  for (const auto& received_exchange_field : q.received_exchange())
    ost << received_exchange_field << "  ";

  ost << ", SAP: " << (q.is_sap() ? "true" : "false");

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
pair<string, string> next_name_value_pair(const string_view str, size_t& posn)
{ static const pair<string, string> empty_pair { };

  if (posn >= str.size())
    return (posn = string::npos, empty_pair);

  const size_t first_char_posn { str.find_first_not_of(' ', posn) };

  if (first_char_posn == string::npos)
    return (posn = string::npos, empty_pair);

  const size_t equals_posn { str.find('=', first_char_posn) };

  if (equals_posn == string::npos)
    return (posn = string::npos, empty_pair);

  const string name                  { remove_peripheral_spaces <std::string> (str.substr(first_char_posn, equals_posn - first_char_posn)) };
  const size_t value_first_char_posn { str.find_first_not_of(' ', equals_posn + 1) };

  if (value_first_char_posn == string::npos)
    return (posn = string::npos, empty_pair);

  const size_t space_posn { str.find(' ', value_first_char_posn) };
  const string value      { (space_posn == string::npos) ? str.substr(value_first_char_posn)
                                                         : str.substr(value_first_char_posn, space_posn - value_first_char_posn) };

// handle "frequency_rx=     mycall=N7DR"
//  if (contains(value, '='))
  if (value.contains('='))
  { posn = value_first_char_posn;
    return { name, string { } };
  }

  posn = space_posn;
  return { name, value };
}
