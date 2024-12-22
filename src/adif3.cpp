// $Id: adif3.cpp 249 2024-07-28 16:44:41Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

/*! \file   adif3.cpp

    Objects and functions related to ADIF version 3.1.0 at https://adif.org/310/ADIF_310.htm
    
    I refrain from comment on the self-evident quality of this soi-disant "specification"
*/

#include "adif3.h"
#include "log_message.h"

#include <algorithm>
#include <iostream>

using namespace std;

extern message_stream    ost;       ///< debugging/logging output

/*! \brief  place values into standardised forms as necessary

    Assumes that names are in upper case
*/
void adif3_field::_normalise(void)
{ using enum ADIF3_DATA_TYPE;

  static const STRING_SET uc_fields { "CALL"s, "MODE"s, "STATION_CALLSIGN"s };   ///< fields that have UC values
  static const string     zero_zero { "00"s };

  switch (_type)
  { case ENUMERATION_BAND :
      _value = to_lower(_value);
      return;
    
    case ENUMERATION_QSL_RECEIVED :
      _value = to_upper(_value);
      return;
  
    case GRID_SQUARE :             // AAnnaann
      _value[0] = toupper(_value[0]);
      _value[1] = toupper(_value[1]);
      
      if (_value.length() >= 6)
      { _value[4] = tolower(_value[4]);
        _value[5] = tolower(_value[5]);
      }
      return;
  
    case TIME :                // six characters
      if (_value.length() == 4)
        _value += zero_zero;
      return;
      
    default :
      break;
  }

// force some values into upper case
  if (uc_fields.contains(_name))
  { _value = to_upper(_value);
    return;
  }
}

/// verify that values are legal
void adif3_field::_verify(void) const
{ using enum ADIF3_DATA_TYPE;

  switch (_type)
  { case DATE :                                      // YYYYMMDD
    { if (_value.find_first_not_of(DIGITS) != string::npos)
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
        
      if (_value.length() != 8)
        throw adif3_error(ADIF3_INVALID_LENGTH, "Invalid length in "s + _name + ": "s + _value);

      const string_view year { substring <string_view> (_value, 0, 4) };
      
      if (year < "1930"sv)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid year in "s + _name + ": "s + _value);
        
      const int month { from_string<int>(substring <string_view> (_value, 4, 2)) };
      
      if ( (month < 1) or (month > 12) )
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid month in "s + _name + ": "s + _value);

      const int day { from_string<int>(substring <string_view> (_value, 6, 2)) };
      
      int days_in_month { 31 };
      
      if ( (month == 4) or (month == 6) or (month == 9) or (month == 11) )
        days_in_month = 30;
          
      if (month == 2)
      { days_in_month = 29;
        if (from_string<int>(year) % 4)   // fails in year 2100
          days_in_month = 28;
      }
        
      if ( (day < 1) or (day > days_in_month) )
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid day number in "s + _name + ": "s + _value);
    }
    break;
      
    case ENUMERATION_BAND :
    { if (!_ENUMERATION_BAND.contains(to_lower(_value)))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
    }
    break;

    case ENUMERATION_DXCC_ENTITY_CODE :
    { if (_value.find_first_not_of(DIGITS) != string::npos)                         // check that it's an integer
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid character in "s + _name + ": "s + _value);

      if (!(_ENUMERATION_DXCC_ENTITY_CODE.contains(from_string<int>(_value))))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid DXCC entity code in "s + _name + ": "s + _value);
    }
    break;
    
    case ENUMERATION_MODE :
    { if (!(_ENUMERATION_MODE.contains(to_upper(_value))))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
    }
    break;
 
    case ENUMERATION_QSL_RECEIVED :
    { if (!( _ENUMERATION_QSL_RECEIVED.contains(to_upper(_value))))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
    }
    break;
 
/*
The first pair (a field) encodes with base 18 and the letters "A" to "R".
The second pair (square) encodes with base 10 and the digits "0" to "9".
The third pair (subsquare) encodes with base 24 and the letters "a" to "x".
The fourth pair (extended square) encodes with base 10 and the digits "0" to "9".
*/
    case GRID_SQUARE :
    { if ( (_value.size() != 2) and (_value.size() != 4) and (_value.size() != 6) and (_value.size() != 8) )
        throw adif3_error(ADIF3_INVALID_LENGTH, "Invalid length in "s + _name + ": "s + _value);
        
      const string uc { to_upper(_value) };      // ADIF spec says case-insensitive!
        
      if ( (uc[0] < 'A' or uc[0] > 'R') or
            (uc[1] < 'A' or uc[1] > 'R') )
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);

      if (_value.size() > 2)
      { if ( (uc[2] < '0' or uc[2] > '9') or
              (uc[3] < '0' or uc[3] > '9') )
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
      }
        
      if (_value.size() > 4)
      { if ( (uc[4] < 'A' or uc[4] > 'X') or
              (uc[5] < 'A' or uc[5] > 'X') )
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
      }
 
      if (_value.size() > 6)
      { if ( (uc[6] < '0' or uc[6] > '9') or
              (uc[7] < '0' or uc[7] > '9') )
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
      }
    }
    break;

    case NUMBER :                               // a sequence of one or more Digits representing a decimal number, optionally preceded by a minus sign (ASCII code 45) and optionally including a single decimal point (ASCII code 46)
    { if (_value.empty())
        throw adif3_error(ADIF3_EMPTY_VALUE, "Empty value in "s + _name + ": "s + _value);   
        
      if (!isdigit(_value[0]) and (_value[0] != '-') and (_value[0] != '.'))
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid initial character in "s + _name + ": "s + _value);

// only one decimal point is allowed
      if (count(_value.begin(), _value.end(), '.') > 1)
        throw adif3_error(ADIF3_INVALID_VALUE, "More than one decimal point in "s + _name + ": "s + _value);
        
// check that only legal characters occur        
      if (_value.find_first_not_of("01234567879."s, 1) != string::npos)
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
    }
    break;
    
    case POSITIVE_INTEGER :      // an unsigned sequence of one or more Digits representing a decimal integer that has a value greater than 0.  Leading zeroes are allowed.
    { if (!is_digits(_value))
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);

// CQZ : 1..40
      if (const auto cit { _positive_integer_range.find(_name) }; cit != _positive_integer_range.cend())
      { const auto [min_value, max_value] = cit->second;
        
        if (const int zone { from_string<int>(_value) }; ((zone < min_value) or (zone > max_value)) )
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
      }
    }
    break; 

    case STRING :        // amazingly (but nothing about this should amaze me), a "sequence of": ASCII character whose code lies in the range of 32 through 126, inclusive
    { for (const char c : _value)
        if ( int intval { static_cast<int>(c) }; (intval < 32) or (intval > 126) )
          throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
    }
    break;

    case TIME :          // HHMMSS or HHMM
    { const string& utc { _value };
      
      if (!is_digits(utc))
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
 
      if ( (utc.length() != 4) and (utc.length() != 6) )
        throw adif3_error(ADIF3_INVALID_LENGTH, "Invalid length in "s + _name + ": "s + _value);
 
      if (const int hours { from_string<int>(substring <string_view> (utc, 0, 2)) }; hours > 23)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid hours value in "s + _name + ": "s + _value);

      if (const int minutes { from_string<int>(substring <string_view> (utc, 2, 2)) }; minutes > 59)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid minutes value in "s + _name + ": "s + _value);

      if (utc.length() == 6)
      { if (const int seconds { from_string<int>(substring <string_view> (utc, 4, 2)) }; seconds > 59)
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid seconds value in "s + _name + ": "s + _value);   
      }
    }
    break;

    default :     // assume OK
      cout << "default (i.e., no) field checking for field " << name() << " with value: " << value() << endl;
    break;
  }
}

/*! \brief                  Construct from name and value
    \param  field_name      name of field
    \param  field_value     value of field

    <i>field_name</i> is converted to upper case when stored as <i>_name</i>
    <i>field_value</i> is validated and converted to standardised format (if applicable)
*/
adif3_field::adif3_field(const string_view field_name, const string_view field_value)
{ _name = to_upper(field_name);
  
  const auto opt { OPT_MUM_VALUE(_element_type, _name) };

  if (!opt)
    throw adif3_error(ADIF3_UNKNOWN_TYPE, "Cannot find type for element: "s + _name + " when creating ADIF3_FIELD with value: "s + field_value);

  _type = opt.value();
  _value = field_value;
  
  _verify();
  _normalise();
}

/*! \brief                  Import name and value from string, and return location past the end of the used part of the string
    \param  str             string from which to read
    \param  start_posn      position in <i>str</i> at which to start parsing
    \param  end_posn        one past the location at which to force an end to parsing, if necessary
    \param  accept_fields   ADIF fields to accept (all fields accepted if empty)
    \return                 one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/
size_t adif3_field::import_and_eat(const string_view str, const size_t start_posn, const size_t end_posn /* one past <EOR> */, const STRING_SET& accept_fields)
{ const auto posn_1 { str.find('<', start_posn) };

  if (posn_1 == string::npos)        // could not find initial delimiter
    return string::npos;

  const auto posn_2          { str.find('>', posn_1) };
  const bool pointing_at_eor { (posn_2 == end_posn) };

  if (pointing_at_eor)
    return (posn_2 + 1);

  if (posn_2 == string::npos)        // could not find ending delimiter
    return string::npos;

// if it's the EOR, then jump out
  const string_view         descriptor_str { substring <string_view> (str, posn_1 + 1, posn_2 - posn_1 -1) };
  const vector<string_view> fields         { split_string <std::string_view> (descriptor_str, ':') };

  if ( (fields.size() < 2) or (fields.size() > 3) )     // wrong number of fields
    return string::npos;

  const int    n_chars { from_string<int>(fields[1]) };
  const size_t rv      { posn_2 + 1 + n_chars };        // eat the used text; should be one past the end of the value of this field
  const string uc_name { to_upper(fields[0]) };

  if (accept_fields.empty() or accept_fields.contains(uc_name))
  { if (!_name.empty() and ( to_upper(fields[0]) != to_upper(name()) ))      // name mismatch
      return string::npos;

    if (_name.empty())
      _name = uc_name;                                // force name to UC

    const string contents { str.substr(posn_2 + 1, n_chars) };
    const auto   atype    { OPT_MUM_VALUE(_element_type, _name) };

    if (!atype)
      throw adif3_error(ADIF3_UNKNOWN_TYPE, "Cannot find type for element: "s + _name);

    _type = atype.value();
    _value = contents;

    _verify();
    _normalise();
  }

  return rv;
}

// ---------------------------------------------------  adif3_record -----------------------------------------

/*! \brief          Convert a string to an int, assuming that the string contains just digits
    \param  str     string to convert
    \return         <i>str</i> rendered as an int

    Result is valid ONLY if <i>str</i> contains only digits
*/ 
int adif3_record::_fast_string_to_int(const string_view str) const
{ static constexpr int zerochar { '0' };

  int rv { 0 };

  for (const char c : str)
    rv = (rv * 10) + (static_cast<int>(c) - zerochar);

  return rv; 
}

/*! \brief          Import record from string, and return location past the end of the used part of the string
    \param  str     string from which to read
    \param  posn    position in <i>str</i> at which to start parsing
    \return         one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/ 
size_t adif3_record::import_and_eat(const string_view str, const size_t posn, const STRING_SET& accept_fields)
{ 
// extract the text from the first "<" to the end of the first "eor"
  const auto posn_1 { str.find('<', posn) };

  if (posn_1 == string::npos)        // could not find initial delimiter
    return string::npos;

#if 0
  const auto posn_2 { case_insensitive_find(str, "<EOR>"sv, posn)  + 4 };  // 4 = length("<EOR>") - 1; posn_2 points to last char: ">"
    
  if (posn_2 == string::npos)        // could not find end-of-record marker
    return string::npos;
#endif

// this is a little faster than the above
#if 1
//  static int counter { 0 };

  size_t posn_2 { posn_1 };

  auto posn_3 { str.find('>', posn_1) };

  if (posn_3 == string::npos)        // could not find end-of-record marker
    return string::npos;

  bool found_it { false };

  while (!found_it)
  { while ( (posn_3 - posn_2  != 4) )
    { posn_2 = str.find('<', posn_3 + 1);

      if (posn_2 == string::npos)        // could not find end-of-record marker
        return string::npos;

      posn_3 = { str.find('>', posn_2 + 1) };

      if (posn_3 == string::npos)        // could not find end-of-record marker
        return string::npos;
    }

    found_it = (to_upper(substring <string_view> (str, posn_2, 5)) == "<EOR>");
  }

  posn_2 = posn_3;  // so that stuff after endif works
#endif

  const size_t rv { ( (posn_2 + 1) >= str.length() ? string::npos : posn_2 + 1) };
  
  size_t start_posn { posn_1 };
  
  while (start_posn <= posn_2)
  { adif3_field element;

    start_posn = element.import_and_eat(str, start_posn, posn_2, accept_fields);             // name is forced to upper case
    
    if (!element.empty())
      if ( auto [it, inserted] { _elements.insert( { element.name(), element } ) }; !inserted)     // should always be inserted
        throw adif3_error(ADIF3_DUPLICATE_FIELD, "Duplicated field name: "s  + element.name());
  }
  
  return rv;
}

/*! \brief      Convert to printable string
    \return     the canonical textual representation of the record

    Returns just the end-of-record marker if the record is empty.
    Does not output import-only fields.
*/
string adif3_record::to_string(void) const
{ string rv { };

  for (const auto& [name, field] : _elements)
  { if (const ADIF3_DATA_TYPE dt { field.type() }; !( _import_only.contains(dt) ) )       // don't output if this type is import-only
      rv += field.to_string();    // output without any checks
  }

  rv += "<EOR>\n"s;
  
  return rv;
}

/*! \brief                  Set the value of a field (which does not have to be extant in the record)
    \param  field_name      name of the field whose value is to be set
    \param  field_value     value to be set
    \return                 whether this was a new field

    <i>field_name</i> is converted to upper case when stored as <i>_name</i>
    <i>field_value</i> is validated and converted to standardised format (if applicable)
*/
bool adif3_record::value(const string_view field_name, const string_view field_value)
{ const adif3_field element        { field_name, field_value };
  const auto        [it, inserted] { _elements.insert( { element.name(), element } ) };
 
  return inserted;
}

/*! \brief         Is one ADIF3 record chronologically earlier than another?
    \param  rec1   first record
    \param  rec2   second record
    \return        whether <i>record1</i> is chronologically before <i>record2</i> 
*/
bool compare_adif3_records(const adif3_record& rec1, const adif3_record& rec2)
{ if (rec1.date() < rec2.date())
    return true;
    
  if (rec2.date() < rec1.date())
    return false;
    
// same date; check time
  return (rec1.time() < rec2.time());
}

// ---------------------------------------------------  adif3_file -----------------------------------------

/*! \class  adif3_file
    \brief  all the ADIF3 records in a file
*/


/*! \brief              Construct from file name
    \param  filename    name of file to read

    Throws exception if something goes wrong when reading the file
*/
adif3_file::adif3_file(const string_view filename, const STRING_SET& accept_fields)
{ const string contents { read_file(filename) };            // this might throw
  
  size_t start_posn { skip_adif3_header(contents) };
  
  while (start_posn != string::npos)
  { adif3_record rec;
    
    start_posn = rec.import_and_eat(contents, start_posn, accept_fields);
    
    if ( !rec.empty() and (start_posn != string::npos) )
    { push_back(rec);
      _map_data += { rec.callsign(), rec };
    }
  }
}

/*! \brief              Construct from file name
    \param  path        vector of directories in which to look
    \param  filename    name of file to read

    Returns empty object if a problem occurs
*/
adif3_file::adif3_file(const vector<string>& path, const string_view filename, const STRING_SET& accept_fields)
{ for (const auto& this_path : path)
  { try
    { *this = adif3_file { this_path + "/"s + filename, accept_fields };
      return;
    }

    catch (...)
    {
    }
  }
}

/*! \brief                  Return all the QSOs that match a call, band and mode
    \param      callsign    call to match
    \param      b           ADIF3 band to match
    \param      m           ADIF3 mode to match
    \return     filename    Return all the QSO records that match <i>callsign</i>, <i>b</i> and <i>m</i>
*/
std::vector<adif3_record> adif3_file::matching_qsos(const string_view callsign, const string_view b, const string_view m) const
{ vector<adif3_record> rv;

  const auto [begin_it, end_it] { _map_data.equal_range(callsign) };
  
  FOR_ALL(begin_it, end_it, [b, m, &rv] (const auto& map_entry)
    { const adif3_record& rec { map_entry.second };

      if ( (rec.band() == b) and (rec.mode() == m) )
        rv += rec;
    });
  
  return rv;
}

/*! \brief                  Return all the QSOs that match a call
    \param      callsign    call to match
    \return     filename    Return all the QSO records that match <i>callsign</i>
*/
std::vector<adif3_record> adif3_file::matching_qsos(const string_view callsign) const
{ vector<adif3_record> rv;

  const auto [begin_it, end_it] { _map_data.equal_range(callsign) };
  
  FOR_ALL(begin_it, end_it, [&rv] (const auto& map_entry) { rv += map_entry.second; });
  
  return rv;
}

/*! \brief              Return position at which to start processing the body of the file
    \param      str     string of contents of filae
    \return             position of first "<" after the end of the header
*/
size_t skip_adif3_header(const std::string_view str)
{ const auto posn_1 { case_insensitive_find(str, "<EOH>"sv) };

  return ( (posn_1 == string::npos) ? 0 : (str.find('<', posn_1 + 1)) ); // either start of file or first "<" after "<EOH>"
}

using enum ADIF3_DATA_TYPE;

/// map from field name to type -- too many of these are silly for it to be worth making comment on individual sillinesses
const UNORDERED_STRING_MAP<ADIF3_DATA_TYPE> adif3_field::_element_type
{ { "ADDRESS"s,                   MULTILINE_STRING },
  { "AGE"s,                       NUMBER },
  { "A_INDEX"s,                   NUMBER },
  { "ANT_AZ"s,                    NUMBER },
  { "ANT_EL"s,                    NUMBER },
  { "ANT_PATH"s,                  ENUMERATION_ANT_PATH },
  { "ARRL_SECT"s,                 ENUMERATION_ARRL_SECT },
  { "AWARD_SUBMITTED"s,           SPONSORED_AWARD_LIST },
  { "AWARD_GRANTED"s,             SPONSORED_AWARD_LIST },
  { "BAND"s,                      ENUMERATION_BAND },
  { "BAND_RX"s,                   ENUMERATION_BAND },
  { "CALL"s,                      STRING },
  { "CHECK"s,                     STRING },
  { "CLASS"s,                     STRING },
  { "CLUBLOG_QSO_UPLOAD_DATE"s,   DATE },
  { "CLUBLOG_QSO_UPLOAD_STATUS"s, ENUMERATION_QSO_UPLOAD_STATUS },
  { "CNTY"s,                      ENUMERATION_SECONDARY_ADMINISTRATIVE_SUBDIVISION },
  { "COMMENT"s,                   STRING },
  { "COMMENT_INTL"s,              INTERNATIONAL_STRING },
  { "CONT"s,                      ENUMERATION_CONTINENT },
  { "CONTACTED_OP"s,              STRING },
  { "CONTEST_ID"s,                STRING },
  { "COUNTRY"s,                   STRING },
  { "COUNTRY_INTL"s,              INTERNATIONAL_STRING },
  { "CQZ"s,                       POSITIVE_INTEGER },
  { "CREDIT_SUBMITTED"s,          CREDIT_LIST },                   // have to think about this, as it could be an import-only AWARD_LIST
  { "CREDIT_GRANTED"s,            CREDIT_LIST },                   // have to think about this, as it could be an import-only AWARD_LIST
  { "DARC_DOK"s,                  ENUMERATION_DARC_DOK },
  { "DISTANCE"s,                  NUMBER },
  { "DXCC"s,                      ENUMERATION_DXCC_ENTITY_CODE },
  { "EMAIL"s,                     STRING },
  { "EQ_CALL"s,                   STRING },                         // I have no idea where this name came from
  { "EQSL_QSLRDATE"s,             DATE },
  { "EQSL_QSLSDATE"s,             DATE },
  { "EQSL_QSL_RCVD"s,             ENUMERATION_QSL_RECEIVED },
  { "EQSL_QSL_SENT"s,             ENUMERATION_QSL_SENT },
  { "FISTS"s,                     POSITIVE_INTEGER },
  { "FISTS_CC"s,                  POSITIVE_INTEGER },
  { "FORCE_INIT"s,                BOOLEAN },
  { "FREQ"s,                      NUMBER },
  { "FREQ_RX"s,                   NUMBER },
  { "GRIDSQUARE"s,                GRID_SQUARE },
  { "GUEST_OP"s,                  STRING },                         // import only
  { "HRDLOG_QSO_UPLOAD_DATE"s,    DATE },
  { "HRDLOG_QSO_UPLOAD_STATUS"s,  ENUMERATION_QSO_UPLOAD_STATUS },
  { "IOTA"s,                      IOTA_REFERENCE_NUMBER },
  { "IOTA_ISLAND_ID"s,            POSITIVE_INTEGER },
  { "ITUZ"s,                      POSITIVE_INTEGER },
  { "K_INDEX"s,                   INTEGER },                        // one wonders where...
  { "LAT"s,                       LOCATION },
  { "LON"s,                       LOCATION },
  { "LOTW_QSLRDATE"s,             DATE },
  { "LOTW_QSLSDATE"s,             DATE },
  { "LOTW_QSL_RCVD"s,             ENUMERATION_QSL_RECEIVED },
  { "LOTW_QSL_SENT"s,             ENUMERATION_QSL_SENT },
  { "MAX_BURSTS"s,                NUMBER },
  { "MODE"s,                      ENUMERATION_MODE },
  { "MS_SHOWER"s,                 STRING },
  { "MY_ANTENNA"s,                STRING },
  { "MY_ANTENNA_INTL"s,           INTERNATIONAL_STRING },
  { "MY_CITY"s,                   STRING },
  { "MY_CITY_INTL"s,              INTERNATIONAL_STRING },
  { "MY_CNTY"s,                   ENUMERATION_SECONDARY_ADMINISTRATIVE_SUBDIVISION },
  { "MY_COUNTRY"s,                STRING },
  { "MY_COUNTRY_INTL"s,           INTERNATIONAL_STRING },
  { "MY_CQ_ZONE"s,                POSITIVE_INTEGER },
  { "MY_DXCC"s,                   ENUMERATION_DXCC_ENTITY_CODE },
  { "MY_FISTS"s,                  POSITIVE_INTEGER },
  { "MY_GRIDSQUARE"s,             GRID_SQUARE },
  { "MY_IOTA"s,                   IOTA_REFERENCE_NUMBER },
  { "MY_IOTA_ISLAND_ID"s,         POSITIVE_INTEGER },
  { "MY_ITU_ZONE"s,               POSITIVE_INTEGER },
  { "MY_LAT"s,                    LOCATION },
  { "MY_LON"s,                    LOCATION },
  { "MY_NAME"s,                   STRING },
  { "MY_NAME_INTL"s,              INTERNATIONAL_STRING },
  { "MY_POSTAL_CODE"s,            STRING },
  { "MY_POSTAL_CODE_INTL"s,       INTERNATIONAL_STRING },
  { "MY_RIG"s,                    STRING },
  { "MY_RIG_INTL"s,               INTERNATIONAL_STRING },
  { "MY_SIG"s,                    STRING },
  { "MY_SIG_INTL"s,               INTERNATIONAL_STRING },
  { "MY_SIG_INFO"s,               STRING },
  { "MY_SIG_INFO_INTL"s,          INTERNATIONAL_STRING },
  { "MY_SOTA_REF"s,               SOTA_REFERENCE },
  { "MY_STATE"s,                  ENUMERATION_PRIMARY_ADMINISTRATIVE_SUBDIVISION },
  { "MY_STREET"s,                 STRING },                                            // !!
  { "MY_STREET_INTL"s,            INTERNATIONAL_STRING },
  { "MY_USACA_COUNTIES"s,         SECONDARY_SUBDIVISION_LIST },
  { "MY_VUCC_GRIDS"s,             GRID_SQUARE_LIST },
  { "NAME"s,                      STRING },
  { "NAME_INTL"s,                 INTERNATIONAL_STRING },
  { "NOTES"s,                     MULTILINE_STRING },
  { "NOTES_INTL"s,                INTERNATIONAL_MULTILINE_STRING },
  { "NR_BURSTS"s,                 INTEGER },
  { "NR_PINGS"s,                  INTEGER },
  { "OPERATOR"s,                  STRING },
  { "OWNER_CALLSIGN"s,            STRING },
  { "PFX"s,                       STRING },
  { "PRECEDENCE"s,                STRING },
  { "PROP_MODE"s,                 ENUMERATION_PROPAGATION_MODE },
  { "PUBLIC_KEY"s,                STRING },
  { "QRZCOM_QSO_UPLOAD_DATE"s,    DATE },
  { "QRZCOM_QSO_UPLOAD_STATUS"s,  ENUMERATION_QSO_UPLOAD_STATUS },
  { "QSLMSG"s,                    MULTILINE_STRING },
  { "QSLMSG"s,                    INTERNATIONAL_MULTILINE_STRING },
  { "QSLRDATE"s,                  DATE },
  { "QSLSDATE"s,                  DATE },
  { "QSL_RCVD"s,                  ENUMERATION_QSL_RECEIVED },
  { "QSL_RCVD_VIA"s,              ENUMERATION_QSL_VIA },
  { "QSL_SENT"s,                  ENUMERATION_QSL_SENT },
  { "QSL_SENT_VIA"s,              ENUMERATION_QSL_VIA },
  { "QSL_VIA"s,                   STRING },
  { "QSO_COMPLETE"s,              ENUMERATION_QSO_COMPLETE },
  { "QSO_DATE"s,                  DATE },
  { "QSO_DATE_OFF"s,              DATE },
  { "QSO_RANDOM"s,                BOOLEAN },
  { "QTH"s,                       STRING },
  { "QTH_INTL"s,                  INTERNATIONAL_STRING },
  { "REGION"s,                    ENUMERATION_REGION },
  { "RIG"s,                       MULTILINE_STRING },
  { "RIG_INTL"s,                  INTERNATIONAL_MULTILINE_STRING },
  { "RST_RCVD"s,                  STRING },
  { "RST_SENT"s,                  STRING },
  { "RX_PWR"s,                    NUMBER },                             // !!
  { "SAT_MODE"s,                  STRING },
  { "SAT_NAME"s,                  STRING },
  { "SFI_NAME"s,                  INTEGER },
  { "SIG"s,                       STRING },
  { "SIG_INTL"s,                  INTERNATIONAL_STRING },
  { "SIG_INFO"s,                  STRING },
  { "SIG_INFO_INTL"s,             INTERNATIONAL_STRING },
  { "SILENT_KEY"s,                BOOLEAN },
  { "SKCC"s,                      STRING },
  { "SOTA_REF"s,                  SOTA_REFERENCE },
  { "SRX"s,                       INTEGER },                            // goodness knows how the abbreviation was derived
  { "SRX_STRING"s,                STRING },
  { "STATE"s,                     ENUMERATION_PRIMARY_ADMINISTRATIVE_SUBDIVISION },
  { "STATION_CALLSIGN"s,          STRING },
  { "STX"s,                       INTEGER },
  { "STX_STRING"s,                STRING },
  { "SUBMODE"s,                   STRING },                             // crazy... depends on mode... even though it's defined as a "String"
  { "SWL"s,                       BOOLEAN },
  { "TEN_TEN"s,                   POSITIVE_INTEGER },
  { "TIME_OFF"s,                  TIME },
  { "TIME_ON"s,                   TIME },
  { "TX_PWR"s,                    NUMBER },
  { "UKSMG"s,                     POSITIVE_INTEGER },
  { "USACA_COUNTIES"s,            SECONDARY_SUBDIVISION_LIST },
  { "VE_PROV"s,                   STRING },                             // import only
  { "VUCC_GRIDS"s,                GRID_SQUARE_LIST },
  { "WEB"s,                       STRING }
};

/// map from field name to permitted range of values
STRING_MAP<pair<int, int>> adif3_field::_positive_integer_range
{ { "CQZ", { 1, 40 } }
};

/// band values
const UNORDERED_STRING_SET adif3_field::_ENUMERATION_BAND
{ "2190m"s, "630m"s, "560m"s, "160m"s, "80m"s,    "60m"s, "40m"s,   "30m"s,   "20m"s,  "17m"s,
  "15m"s,   "12m"s,  "10m"s,  "6m"s,   "4m"s,     "2m"s,  "1.25m"s, "70cm"s,  "33cm"s, "23cm"s,
  "13cm"s,  "9cm"s,  "6cm"s,  "3cm"s,  "1.25cm"s, "6mm"s, "4mm"s,   "2.5mm"s, "2mm"s,  "1mm"s
};

/// mapping between country code and country info
using enum COUNTRY_STATUS; //not supported in g++ 10
const unordered_map<int /* country number */, tuple<string /*country name */, string /* canonical prefix */, COUNTRY_STATUS /* whether deleted */>> adif3_field::_ENUMERATION_DXCC_ENTITY_CODE
{ {  1  , { "CANADA"s,                                  "VE"s,    CURRENT } },
  {  2  , { "ABU AIL IS."s,                             ""s,      DELETED } },
  {  3  , { "AFGHANISTAN"s,                             "YA"s,    CURRENT } },
  {  4  , { "AGALEGA & ST. BRANDON IS."s,               "3B6"s,   CURRENT } },
  {  5  , { "ALAND IS."s,                               "OH0"s,   CURRENT } },
  {  6  , { "ALASKA"s,                                  "KL"s,    CURRENT } },
  {  7  , { "ALBANIA"s,                                 "ZA"s,    CURRENT } },
  {  8  , { "ALDABRA"s,                                 ""s,      DELETED } },
  {  9  , { "AMERICAN SAMOA"s,                          "KH8"s,   CURRENT } },
  {  10 , { "AMSTERDAM & ST. PAUL IS."s,                "FT5Z"s,  CURRENT } },
  {  11 , { "ANDAMAN & NICOBAR IS."s,                   "VU4"s,   CURRENT } },
  {  12 , { "ANGUILLA"s,                                "VP2E"s,  CURRENT } },
  {  13 , { "ANTARCTICA"s,                              "CE9"s,   CURRENT } },
  {  14 , { "ARMENIA"s,                                 "EK"s,    CURRENT } },
  {  15 , { "ASIATIC RUSSIA"s,                          "UA9"s,   CURRENT } },
  {  16 , { "NEW ZEALAND SUBANTARCTIC ISLANDS"s,        "ZL9"s,   CURRENT } },
  {  17 , { "AVES I."s,                                 "YV0"s,   CURRENT } },
  {  18 , { "AZERBAIJAN"s,                              "4J"s,    CURRENT } },
  {  19 , { "BAJO NUEVO"s,                              ""s,      DELETED } },
  {  20 , { "BAKER & HOWLAND IS."s,                     "KH1"s,   CURRENT } },
  {  21 , { "BALEARIC IS."s,                            "EA6"s,   CURRENT } },
  {  22 , { "PALAU"s,                                   "T8"s,    CURRENT } },
  {  23 , { "BLENHEIM REEF"s,                           ""s,      DELETED } },
  {  24 , { "BOUVET"s,                                  "3Y/b"s,  CURRENT } },
  {  25 , { "BRITISH NORTH BORNEO"s,                    ""s,      DELETED } },
  {  26 , { "BRITISH SOMALILAND"s,                      ""s,      DELETED } },
  {  27 , { "BELARUS"s,                                 "EU"s,    CURRENT } },
  {  28 , { "CANAL ZONE"s,                              ""s,      DELETED } },
  {  29 , { "CANARY IS."s,                              "EA8"s,   CURRENT } },
  {  30 , { "CELEBE & MOLUCCA IS."s,                    ""s,      DELETED } },
  {  31 , { "C. KIRIBATI (BRITISH PHOENIX IS.)"s,       "EA9"s,   CURRENT } },
  {  32 , { "CEUTA & MELILLA"s,                         "T31"s,   CURRENT } },
  {  33 , { "CHAGOS IS."s,                              "VQ9"s,   CURRENT } },
  {  34 , { "CHATHAM IS."s,                             "ZL7"s,   CURRENT } },
  {  35 , { "CHRISTMAS I."s,                            "VK9X"s,  CURRENT } },
  {  36 , { "CLIPPERTON I."s,                           "FO/c"s,  CURRENT } },
  {  37 , { "COCOS I."s,                                "TI9"s,   CURRENT } },
  {  38 , { "COCOS (KEELING) IS."s,                     "VK9C"s,  CURRENT } },
  {  39 , { "COMOROS"s,                                 ""s,      DELETED } },
  {  40 , { "CRETE"s,                                   "SV9"s,   CURRENT } },
  {  41 , { "CROZET I."s,                               "FT5W"s,  CURRENT } },
  {  42 , { "DAMAO, DIU"s,                              ""s,      DELETED } },
  {  43 , { "DESECHEO I."s,                             "KP5"s,   CURRENT } },
  {  44 , { "DESROCHES"s,                               ""s,      DELETED } },
  {  45 , { "DODECANESE"s,                              "SV5"s,   CURRENT } },
  {  46 , { "EAST MALAYSIA"s,                           "9M6"s,   CURRENT } },
  {  47 , { "EASTER I."s,                               "CE0Y"s,  CURRENT } },
  {  48 , { "E. KIRIBATI (LINE IS.)"s,                  "T32"s,   CURRENT } },
  {  49 , { "EQUATORIAL GUINEA"s,                       "3C"s,    CURRENT } },
  {  50 , { "MEXICO"s,                                  "XE"s,    CURRENT } },
  {  51 , { "ERITREA"s,                                 "E3"s,    CURRENT } },
  {  52 , { "ESTONIA"s,                                 "ES"s,    CURRENT } },
  {  53 , { "ETHIOPIA"s,                                "ET"s,    CURRENT } },
  {  54 , { "EUROPEAN RUSSIA"s,                         "UA"s,    CURRENT } },
  {  55 , { "FARQUHAR"s,                                ""s,      DELETED } },
  {  56 , { "FERNANDO DE NORONHA"s,                     "PY0F"s,  CURRENT } },
  {  57 , { "FRENCH EQUATORIAL AFRICA"s,                ""s,      DELETED } },
  {  58 , { "FRENCH INDO-CHINA"s,                       ""s,      DELETED } },
  {  59 , { "FRENCH WEST AFRICA"s,                      ""s,      DELETED } },
  {  60 , { "BAHAMAS"s,                                 "C6"s,    CURRENT } },
  {  61 , { "FRANZ JOSEF LAND"s,                        "R1FJ"s,  CURRENT } },
  {  62 , { "BARBADOS"s,                                "8P"s,    CURRENT } },
  {  63 , { "FRENCH GUIANA"s,                           "FY"s,    CURRENT } },
  {  64 , { "BERMUDA"s,                                 "VP9"s,   CURRENT } },
  {  65 , { "BRITISH VIRGIN IS."s,                      "VP2V"s,  CURRENT } },
  {  66 , { "BELIZE"s,                                  "V3"s,    CURRENT } },
  {  67 , { "FRENCH INDIA"s,                            ""s,      DELETED } },
  {  68 , { "KUWAIT/SAUDI ARABIA NEUTRAL ZONE"s,        ""s,      DELETED } },
  {  69 , { "CAYMAN IS."s,                              "ZF"s,    CURRENT } },
  {  70 , { "CUBA"s,                                    "CM"s,    CURRENT } },
  {  71 , { "GALAPAGOS IS."s,                           "HC8"s,   CURRENT } },
  {  72 , { "DOMINICAN REPUBLIC"s,                      "HI"s,    CURRENT } },
  {  74 , { "EL SALVADOR"s,                             "YS"s,    CURRENT } },
  {  75 , { "GEORGIA"s,                                 "4L"s,    CURRENT } },
  {  76 , { "GUATEMALA"s,                               "TG"s,    CURRENT } },
  {  77 , { "GRENADA"s,                                 "J3"s,    CURRENT } },
  {  78 , { "HAITI"s,                                   "HH"s,    CURRENT } },
  {  79 , { "GUADELOUPE"s,                              "FG"s,    CURRENT } },
  {  80 , { "HONDURAS"s,                                "HR"s,    CURRENT } },
  {  81 , { "GERMANY"s,                                 ""s,      DELETED } },
  {  82 , { "JAMAICA"s,                                 "6Y"s,    CURRENT } },
  {  84 , { "MARTINIQUE"s,                              "FM"s,    CURRENT } },
  {  85 , { "BONAIRE, CURACAO"s,                        ""s,      DELETED } },
  {  86 , { "NICARAGUA"s,                               "YN"s,    CURRENT } },
  {  88 , { "PANAMA"s,                                  "HP"s,    CURRENT } },
  {  89 , { "TURKS & CAICOS IS."s,                      "VP5"s,   CURRENT } },
  {  90 , { "TRINIDAD & TOBAGO"s,                       "9Y"s,    CURRENT } },
  {  91 , { "ARUBA"s,                                   "P4"s,    CURRENT } },
  {  93 , { "GEYSER REEF"s,                             ""s,      DELETED } },
  {  94 , { "ANTIGUA & BARBUDA"s,                       "V2"s,    CURRENT } },
  {  95 , { "DOMINICA"s,                                "J7"s,    CURRENT } },
  {  96 , { "MONTSERRAT"s,                              "VP2M"s,  CURRENT } },
  {  97 , { "ST. LUCIA"s,                               "J6"s,    CURRENT } },
  {  98 , { "ST. VINCENT"s,                             "J8"s,    CURRENT } },
  {  99 , { "GLORIOSO IS."s,                            "FR/g"s,  CURRENT } },
  {  100, { "ARGENTINA"s,                               "LU"s,    CURRENT } },
  {  101, { "GOA"s,                                     ""s,      DELETED } },
  {  102, { "GOLD COAST, TOGOLAND"s,                    ""s,      DELETED } },
  {  103, { "GUAM"s,                                    "KH2"s,   CURRENT } },
  {  104, { "BOLIVIA"s,                                 "CP"s,    CURRENT } },
  {  105, { "GUANTANAMO BAY"s,                          "KG4"s,   CURRENT } },
  {  106, { "GUERNSEY"s,                                "GU"s,    CURRENT } },
  {  107, { "GUINEA"s,                                  "3X"s,    CURRENT } },
  {  108, { "BRAZIL"s,                                  "PY"s,    CURRENT } },
  {  109, { "GUINEA-BISSAU"s,                           "J5"s,    CURRENT } },
  {  110, { "HAWAII"s,                                  "KH6"s,   CURRENT } },
  {  111, { "HEARD I."s,                                "VK0H"s,  CURRENT } },
  {  112, { "CHILE"s,                                   "CE"s,    CURRENT } },
  {  113, { "IFNI"s,                                    ""s,      DELETED } },
  {  114, { "ISLE OF MAN"s,                             "GD"s,    CURRENT } },
  {  115, { "ITALIAN SOMALILAND"s,                      ""s,      DELETED } },
  {  116, { "COLOMBIA"s,                                "HK"s,    CURRENT } },
  {  117, { "ITU HQ"s,                                  "4U1I"s,  CURRENT } },
  {  118, { "JAN MAYEN"s,                               "JX"s,    CURRENT } },
  {  119, { "JAVA"s,                                    ""s,      DELETED } },
  {  120, { "ECUADOR"s,                                 "HC"s,    CURRENT } },
  {  122, { "JERSEY"s,                                  "GJ"s,    CURRENT } },
  {  123, { "JOHNSTON I."s,                             "KH3"s,   CURRENT } },
  {  124, { "JUAN DE NOVA, EUROPA"s,                    "FR/j"s,  CURRENT } },
  {  125, { "JUAN FERNANDEZ IS."s,                      "CE0Z"s,  CURRENT } },
  {  126, { "KALININGRAD"s,                             "UA2"s,   CURRENT } },
  {  127, { "KAMARAN IS."s,                             ""s,      DELETED } },
  {  128, { "KARELO-FINNISH REPUBLIC"s,                 ""s,      DELETED } },
  {  129, { "GUYANA"s,                                  "8R"s,    CURRENT } },
  {  130, { "KAZAKHSTAN"s,                              "UN"s,    CURRENT } },
  {  131, { "KERGUELEN IS."s,                           "FT5X"s,  CURRENT } },
  {  132, { "PARAGUAY"s,                                "ZP"s,    CURRENT } },
  {  133, { "KERMADEC IS."s,                            "ZL8"s,   CURRENT } },
  {  134, { "KINGMAN REEF"s,                            "KH5K"s,  CURRENT } },
  {  135, { "KYRGYZSTAN"s,                              "EX"s,    CURRENT } },
  {  136, { "PERU"s,                                    "OA"s,    CURRENT } },
  {  137, { "REPUBLIC OF KOREA"s,                       "HK"s,    CURRENT } },
  {  138, { "KURE I."s,                                 "KH7K"s,  CURRENT } },
  {  139, { "KURIA MURIA I."s,                          ""s,      DELETED } },
  {  140, { "SURINAME"s,                                "PZ"s,    CURRENT } },
  {  141, { "FALKLAND IS."s,                            "VP8"s,   CURRENT } },
  {  142, { "LAKSHADWEEP IS."s,                         "VU7"s,   CURRENT } },
  {  143, { "LAOS"s,                                    "XW"s,    CURRENT } },
  {  144, { "URUGUAY"s,                                 "CX"s,    CURRENT } },
  {  145, { "LATVIA"s,                                  "YL"s,    CURRENT } },
  {  146, { "LITHUANIA"s,                               "LY"s,    CURRENT } },
  {  147, { "LORD HOWE I."s,                            "VK9L"s,  CURRENT } },
  {  148, { "VENEZUELA"s,                               "YV"s,    CURRENT } },
  {  149, { "AZORES"s,                                  "CU"s,    CURRENT } },
  {  150, { "AUSTRALIA"s,                               "VK"s,    CURRENT } },
  {  151, { "MALYJ VYSOTSKIJ I."s,                      "R1MV"s,  CURRENT } },
  {  152, { "MACAO"s,                                   "XX9"s,   CURRENT } },
  {  153, { "MACQUARIE I."s,                            "VK0M"s,  CURRENT } },
  {  154, { "YEMEN ARAB REPUBLIC"s,                     ""s,      DELETED } },
  {  155, { "MALAYA"s,                                  ""s,      DELETED } },
  {  157, { "NAURU"s,                                   "C2"s,    CURRENT } },
  {  158, { "VANUATU"s,                                 "YJ"s,    CURRENT } },
  {  159, { "MALDIVES"s,                                "8Q"s,    CURRENT } },
  {  160, { "TONGA"s,                                   "A3"s,    CURRENT } },
  {  161, { "MALPELO I."s,                              "HK0/m"s, CURRENT } },
  {  162, { "NEW CALEDONIA"s,                           "FK"s,    CURRENT } },
  {  163, { "PAPUA NEW GUINEA"s,                        "P2"s,    CURRENT } },
  {  164, { "MANCHURIA"s,                               ""s,      DELETED } },
  {  165, { "MAURITIUS"s,                               "3B8"s,   CURRENT } },
  {  166, { "MARIANA IS."s,                             "KH0"s,   CURRENT } },
  {  167, { "MARKET REEF"s,                             "OJ0"s,   CURRENT } },
  {  168, { "MARSHALL IS."s,                            "V7"s,    CURRENT } },
  {  169, { "MAYOTTE"s,                                 "FH"s,    CURRENT } },
  {  170, { "NEW ZEALAND"s,                             "ZL"s,    CURRENT } },
  {  171, { "MELLISH REEF"s,                            "VK9M"s,  CURRENT } },
  {  172, { "PITCAIRN I."s,                             "VP6"s,   CURRENT } },
  {  173, { "MICRONESIA"s,                              "V6"s,    CURRENT } },
  {  174, { "MIDWAY I."s,                               "KH4"s,   CURRENT } },
  {  175, { "FRENCH POLYNESIA"s,                        "FO"s,    CURRENT } },
  {  176, { "FIJI"s,                                    "3D"s,    CURRENT } },
  {  177, { "MINAMI TORISHIMA"s,                        "JD/m"s,  CURRENT } },
  {  178, { "MINERVA REEF"s,                            ""s,      DELETED } },
  {  179, { "MOLDOVA"s,                                 "ER"s,    CURRENT } },
  {  180, { "MOUNT ATHOS"s,                             "SV/a"s,  CURRENT } },
  {  181, { "MOZAMBIQUE"s,                              "C9"s,    CURRENT } },
  {  182, { "NAVASSA I."s,                              "KP1"s,   CURRENT } },
  {  183, { "NETHERLANDS BORNEO"s,                      ""s,      DELETED } },
  {  184, { "NETHERLANDS NEW GUINEA"s,                  ""s,      DELETED } },
  {  185, { "SOLOMON IS."s,                             "H4"s,    CURRENT } },
  {  186, { "NEWFOUNDLAND, LABRADOR"s,                  ""s,      DELETED } },
  {  187, { "NIGER"s,                                   "5U"s,    CURRENT } },
  {  188, { "NIUE"s,                                    "ZK2"s,   CURRENT } },
  {  189, { "NORFOLK I."s,                              "VK9N"s,  CURRENT } },
  {  190, { "SAMOA"s,                                   "E5/n"s,   CURRENT } },
  {  191, { "NORTH COOK IS."s,                          "E5/n"s,   CURRENT } },
  {  192, { "OGASAWARA"s,                               "JD/o"s,   CURRENT } },
  {  193, { "OKINAWA (RYUKYU IS.)"s,                    ""s,      DELETED } },
  {  194, { "OKINO TORI-SHIMA"s,                        ""s,      DELETED } },
  {  195, { "ANNOBON I."s,                              "3C0"s,    CURRENT } },
  {  196, { "PALESTINE"s,                               ""s,      DELETED } },
  {  197, { "PALMYRA & JARVIS IS."s,                    "KH5"s,    CURRENT } },
  {  198, { "PAPUA TERRITORY"s,                         ""s,      DELETED } },
  {  199, { "PETER 1 I."s,                              "3Y/p"s,   CURRENT } },
  {  200, { "PORTUGUESE TIMOR"s,                        ""s,      DELETED } },
  {  201, { "PRINCE EDWARD & MARION IS."s,              "ZS8"s,   CURRENT } },
  {  202, { "PUERTO RICO"s,                             "KP4"s,   CURRENT } },
  {  203, { "ANDORRA"s,                                 "C3"s,    CURRENT } },
  {  204, { "REVILLAGIGEDO"s,                           "XF4"s,   CURRENT } },
  {  205, { "ASCENSION I."s,                            "ZD8"s,   CURRENT } },
  {  206, { "AUSTRIA"s,                                 "OE"s,    CURRENT } },
  {  207, { "RODRIGUEZ I."s,                            "3B9"s,   CURRENT } },
  {  208, { "RUANDA-URUNDI"s,                           ""s,      DELETED } },
  {  209, { "BELGIUM"s,                                 "ON"s,    CURRENT } },
  {  210, { "SAAR"s,                                    ""s,      DELETED } },
  {  211, { "SABLE I."s,                                "CY0"s,   CURRENT } },
  {  212, { "BULGARIA"s,                                "LZ"s,    CURRENT } },
  {  213, { "SAINT MARTIN"s,                            "FS"s,    CURRENT } },
  {  214, { "CORSICA"s,                                 "TK"s,    CURRENT } },
  {  215, { "CYPRUS"s,                                  "5B"s,    CURRENT } },
  {  216, { "SAN ANDRES & PROVIDENCIA"s,                "HK0/a"s, CURRENT } },
  {  217, { "SAN FELIX & SAN AMBROSIO"s,                "CE0X"s,  CURRENT } },
  {  218, { "CZECHOSLOVAKIA"s,                          ""s,      DELETED } },
  {  219, { "SAO TOME & PRINCIPE"s,                     "S9"s,    CURRENT } },
  {  220, { "SARAWAK"s,                                 ""s,      DELETED } },
  {  221, { "DENMARK"s,                                 "OZ"s,    CURRENT } },
  {  222, { "FAROE IS."s,                               "OY"s,    CURRENT } },
  {  223, { "ENGLAND"s,                                 "G"s,     CURRENT } },
  {  224, { "FINLAND"s,                                 "OH"s,    CURRENT } },
  {  225, { "SARDINIA"s,                                "IS"s,    CURRENT } },
  {  226, { "SAUDI ARABIA/IRAQ NEUTRAL ZONE"s,          ""s,      DELETED } },
  {  227, { "FRANCE"s,                                  "F"s,     CURRENT } },
  {  228, { "SERRANA BANK & RONCADOR CAY"s,             ""s,      DELETED } },
  {  229, { "GERMAN DEMOCRATIC REPUBLIC"s,              ""s,      DELETED } },
  {  230, { "FEDERAL REPUBLIC OF GERMANY"s,             "DL"s,    CURRENT } },
  {  231, { "SIKKIM"s,                                  ""s,      DELETED } },
  {  232, { "SOMALIA"s,                                 "T5"s,    CURRENT } },
  {  233, { "GIBRALTAR"s,                               "ZB"s,    CURRENT } },
  {  234, { "SOUTH COOK IS."s,                          "E5/s"s,  CURRENT } },
  {  235, { "SOUTH GEORGIA I."s,                        "VP8/g"s, CURRENT } },
  {  236, { "GREECE"s,                                  "SV"s,    CURRENT } },
  {  237, { "GREENLAND"s,                               "OX"s,    CURRENT } },
  {  238, { "SOUTH ORKNEY IS."s,                        "VP8/o"s, CURRENT } },
  {  239, { "HUNGARY"s,                                 "HA"s,    CURRENT } },
  {  240, { "SOUTH SANDWICH IS."s,                      "VP8/s"s, CURRENT } },
  {  241, { "SOUTH SHETLAND IS."s,                      "VP8/h"s, CURRENT } },
  {  242, { "ICELAND"s,                                 "TF"s,    CURRENT } },
  {  243, { "PEOPLE'S DEMOCRATIC REP. OF YEMEN"s,       ""s,      DELETED } },
  {  244, { "SOUTHERN SUDAN"s,                          ""s,      DELETED } },
  {  245, { "IRELAND"s,                                 "EI"s,    CURRENT } },
  {  246, { "SOVEREIGN MILITARY ORDER OF MALTA"s,       "1A"s,    CURRENT } },
  {  247, { "SPRATLY IS."s,                             "1S"s,    CURRENT } },
  {  248, { "ITALY"s,                                   "I"s,     CURRENT } },
  {  249, { "ST. KITTS & NEVIS"s,                       "V4"s,    CURRENT } },
  {  250, { "ST. HELENA"s,                              "ZD7"s,   CURRENT } },
  {  251, { "LIECHTENSTEIN"s,                           "HB0"s,   CURRENT } },
  {  252, { "ST. PAUL I."s,                             "CY9"s,   CURRENT } },
  {  253, { "ST. PETER & ST. PAUL ROCKS"s,              "PY0S"s,  CURRENT } },
  {  254, { "LUXEMBOURG"s,                              "LX"s,    CURRENT } },
  {  255, { "ST. MAARTEN, SABA, ST. EUSTATIUS"s,        ""s,      DELETED } },
  {  256, { "MADEIRA IS."s,                             "CT3"s,   CURRENT } },
  {  257, { "MALTA"s,                                   "9H"s,    CURRENT } },
  {  258, { "SUMATRA"s,                                 ""s,      DELETED } },
  {  259, { "SVALBARD"s,                                "JW"s,    CURRENT } },
  {  260, { "MONACO"s,                                  "3A"s,    CURRENT } },
  {  261, { "SWAN IS."s,                                ""s,      DELETED } },
  {  262, { "TAJIKISTAN"s,                              "EY"s,    CURRENT } },
  {  263, { "NETHERLANDS"s,                             "PA"s,    CURRENT } },
  {  264, { "TANGIER"s,                                 ""s,      DELETED } },
  {  265, { "NORTHERN IRELAND"s,                        "GI"s,    CURRENT } },
  {  266, { "NORWAY"s,                                  "LA"s,    CURRENT } },
  {  267, { "TERRITORY OF NEW GUINEA"s,                 ""s,      DELETED } },
  {  268, { "TIBET"s,                                   ""s,      DELETED } },
  {  269, { "POLAND"s,                                  "SP"s,    CURRENT } },
  {  270, { "TOKELAU IS."s,                             "ZK3"s,   CURRENT } },
  {  271, { "TRIESTE"s,                                 ""s,      DELETED } },
  {  272, { "PORTUGAL"s,                                "CT"s,    CURRENT } },
  {  273, { "TRINDADE & MARTIM VAZ IS."s,               "PY0T"s,  CURRENT } },
  {  274, { "TRISTAN DA CUNHA & GOUGH I."s,             "ZD9"s,   CURRENT } },
  {  275, { "ROMANIA"s,                                 "YO"s,    CURRENT } },
  {  276, { "TROMELIN I."s,                             "FR/t"s,  CURRENT } },
  {  277, { "ST. PIERRE & MIQUELON"s,                   "FP"s,    CURRENT } },
  {  278, { "SAN MARINO"s,                              "T7"s,    CURRENT } },
  {  279, { "SCOTLAND"s,                                "GM"s,    CURRENT } },
  {  280, { "TURKMENISTAN"s,                            "EZ"s,    CURRENT } },
  {  281, { "SPAIN"s,                                   "EA"s,    CURRENT } },
  {  282, { "TUVALU"s,                                  "T2"s,    CURRENT } },
  {  283, { "UK SOVEREIGN BASE AREAS ON CYPRUS"s,       "ZC4"s,   CURRENT } },
  {  284, { "SWEDEN"s,                                  "SM"s,    CURRENT } },
  {  285, { "VIRGIN IS."s,                              "KP2"s,   CURRENT } },
  {  286, { "UGANDA"s,                                  "5X"s,    CURRENT } },
  {  287, { "SWITZERLAND"s,                             "HB"s,    CURRENT } },
  {  288, { "UKRAINE"s,                                 "UR"s,    CURRENT } },
  {  289, { "UNITED NATIONS HQ"s,                       "4U1U"s,  CURRENT } },
  {  291, { "UNITED STATES OF AMERICA"s,                "K"s,     CURRENT } },
  {  292, { "UZBEKISTAN"s,                              "UK"s,    CURRENT } },
  {  293, { "VIET NAM"s,                                "3W"s,    CURRENT } },
  {  294, { "WALES"s,                                   "GW"s,    CURRENT } },
  {  295, { "VATICAN"s,                                 "HV"s,    CURRENT } },
  {  296, { "SERBIA"s,                                  "YU"s,    CURRENT } },
  {  297, { "WAKE I."s,                                 "KH9"s,   CURRENT } },
  {  298, { "WALLIS & FUTUNA IS."s,                     "FW"s,    CURRENT } },
  {  299, { "WEST MALAYSIA"s,                           "9M2"s,   CURRENT } },
  {  301, { "W. KIRIBATI (GILBERT IS. )"s,              "T30"s,   CURRENT } },
  {  302, { "WESTERN SAHARA"s,                          "S0"s,    CURRENT } },
  {  303, { "WILLIS I."s,                               "VK9W"s,  CURRENT } },
  {  304, { "BAHRAIN"s,                                 "A9"s,    CURRENT } },
  {  305, { "BANGLADESH"s,                              "S2"s,    CURRENT } },
  {  306, { "BHUTAN"s,                                  "A5"s,    CURRENT } },
  {  307, { "ZANZIBAR"s,                                ""s,      DELETED } },
  {  308, { "COSTA RICA"s,                              "TI"s,    CURRENT } },
  {  309, { "MYANMAR"s,                                 "XZ"s,    CURRENT } },
  {  312, { "CAMBODIA"s,                                "XU"s,    CURRENT } },
  {  315, { "SRI LANKA"s,                               "4S"s,    CURRENT } },
  {  318, { "CHINA"s,                                   "BY"s,    CURRENT } },
  {  321, { "HONG KONG"s,                               "VR"s,    CURRENT } },
  {  324, { "INDIA"s,                                   "VU"s,    CURRENT } },
  {  327, { "INDONESIA"s,                               "YB"s,    CURRENT } },
  {  330, { "IRAN"s,                                    "EP"s,    CURRENT } },
  {  333, { "IRAQ"s,                                    "YI"s,    CURRENT } },
  {  336, { "ISRAEL"s,                                  "4X"s,    CURRENT } },
  {  339, { "JAPAN"s,                                   "JA"s,    CURRENT } },
  {  342, { "JORDAN"s,                                  "JY"s,    CURRENT } },
  {  344, { "DEMOCRATIC PEOPLE'S REP. OF KOREA "s,      "HM"s,    CURRENT } },
  {  345, { "BRUNEI DARUSSALAM"s,                       "V8"s,    CURRENT } },
  {  348, { "KUWAIT"s,                                  "9K"s,    CURRENT } },
  {  354, { "LEBANON"s,                                 "OD"s,    CURRENT } },
  {  363, { "MONGOLIA"s,                                "JT"s,    CURRENT } },
  {  369, { "NEPAL"s,                                   "9N"s,    CURRENT } },
  {  370, { "OMAN"s,                                    "A4"s,    CURRENT } },
  {  372, { "PAKISTAN"s,                                "AP"s,    CURRENT } },
  {  375, { "PHILIPPINES"s,                             "DU"s,    CURRENT } },
  {  376, { "QATAR"s,                                   "A7"s,    CURRENT } },
  {  378, { "SAUDI ARABIA"s,                            "HZ"s,    CURRENT } },
  {  379, { "SEYCHELLES"s,                              "S7"s,    CURRENT } },
  {  381, { "SINGAPORE"s,                               "9V"s,    CURRENT } },
  {  382, { "DJIBOUTI"s,                                "J2"s,    CURRENT } },
  {  384, { "SYRIA"s,                                   "YK"s,    CURRENT } },
  {  386, { "TAIWAN"s,                                  "BV"s,    CURRENT } },
  {  387, { "THAILAND"s,                                "HS"s,    CURRENT } },
  {  390, { "TURKEY"s,                                  "TA"s,    CURRENT } },
  {  391, { "UNITED ARAB EMIRATES"s,                    "A6"s,    CURRENT } },
  {  400, { "ALGERIA"s,                                 "7X"s,    CURRENT } },
  {  401, { "ANGOLA"s,                                  "D2"s,    CURRENT } },
  {  402, { "BOTSWANA"s,                                "A2"s,    CURRENT } },
  {  404, { "BURUNDI"s,                                 "9U"s,    CURRENT } },
  {  406, { "CAMEROON"s,                                "TJ"s,    CURRENT } },
  {  408, { "CENTRAL AFRICA"s,                          "TL"s,    CURRENT } },
  {  409, { "CAPE VERDE"s,                              "D4"s,    CURRENT } },
  {  410, { "CHAD"s,                                    "TT"s,    CURRENT } },
  {  411, { "COMOROS"s,                                 "D6"s,    CURRENT } },
  {  412, { "REPUBLIC OF THE CONGO"s,                   "9Q"s,    CURRENT } },
  {  414, { "DEMOCRATIC REPUBLIC OF THE CONGO"s,        "TN"s,    CURRENT } },
  {  416, { "BENIN"s,                                   "TY"s,    CURRENT } },
  {  420, { "GABON"s,                                   "TR"s,    CURRENT } },
  {  422, { "THE GAMBIA"s,                              "C5"s,    CURRENT } },
  {  424, { "GHANA"s,                                   "9G"s,    CURRENT } },
  {  428, { "COTE D'IVOIRE"s,                           "TU"s,    CURRENT } },
  {  430, { "KENYA"s,                                   "5Z"s,    CURRENT } },
  {  432, { "LESOTHO"s,                                 "7P"s,    CURRENT } },
  {  434, { "LIBERIA"s,                                 "EL"s,    CURRENT } },
  {  436, { "LIBYA"s,                                   "5A"s,    CURRENT } },
  {  438, { "MADAGASCAR"s,                              "5R"s,    CURRENT } },
  {  440, { "MALAWI"s,                                  "7Q"s,    CURRENT } },
  {  442, { "MALI"s,                                    "TZ"s,    CURRENT } },
  {  444, { "MAURITANIA"s,                              "5T"s,    CURRENT } },
  {  446, { "MOROCCO"s,                                 "CN"s,    CURRENT } },
  {  450, { "NIGERIA"s,                                 "5N"s,    CURRENT } },
  {  452, { "ZIMBABWE"s,                                "Z2"s,    CURRENT } },
  {  453, { "REUNION I."s,                              "FR"s,    CURRENT } },
  {  454, { "RWANDA"s,                                  "9X"s,    CURRENT } },
  {  456, { "SENEGAL"s,                                 "6W"s,    CURRENT } },
  {  458, { "SIERRA LEONE"s,                            "9L"s,    CURRENT } },
  {  460, { "ROTUMA I."s,                               "3D2/r"s, CURRENT } },
  {  462, { "SOUTH AFRICA"s,                            "ZS"s,    CURRENT } },
  {  464, { "NAMIBIA"s,                                 "V5"s,    CURRENT } },
  {  466, { "SUDAN"s,                                   "ST"s,    CURRENT } },
  {  468, { "SWAZILAND"s,                               "3DA"s,   CURRENT } },
  {  470, { "TANZANIA"s,                                "5H"s,    CURRENT } },
  {  474, { "TUNISIA"s,                                 "3V"s,    CURRENT } },
  {  478, { "EGYPT"s,                                   "SU"s,    CURRENT } },
  {  480, { "BURKINA FASO"s,                            "XT"s,    CURRENT } },
  {  482, { "ZAMBIA"s,                                  "9J"s,    CURRENT } },
  {  483, { "TOGO"s,                                    "5V"s,    CURRENT } },
  {  488, { "WALVIS BAY"s,                              ""s,      DELETED } },
  {  489, { "CONWAY REEF"s,                             "3D2/c"s, CURRENT } },
  {  490, { "BANABA I. (OCEAN I.)"s,                    "T33"s,   CURRENT } },
  {  492, { "YEMEN"s,                                   "7O"s,    CURRENT } },
  {  493, { "PENGUIN IS."s,                             ""s,      DELETED } },
  {  497, { "CROATIA"s,                                 "9A"s,    CURRENT } },
  {  499, { "SLOVENIA"s,                                "S5"s,    CURRENT } },
  {  501, { "BOSNIA-HERZEGOVINA"s,                      "E7"s,    CURRENT } },
  {  502, { "MACEDONIA"s,                               "Z3"s,    CURRENT } },
  {  503, { "CZECH REPUBLIC"s,                          "OK"s,    CURRENT } },
  {  504, { "SLOVAK REPUBLIC"s,                         "OM"s,    CURRENT } },
  {  505, { "PRATAS I."s,                               "BV9P"s,  CURRENT } },
  {  506, { "SCARBOROUGH REEF"s,                        "BS7"s,   CURRENT } },
  {  507, { "TEMOTU PROVINCE"s,                         "H40"s,   CURRENT } },
  {  508, { "AUSTRAL I."s,                              "FO/a"s,  CURRENT } },
  {  509, { "MARQUESAS IS."s,                           "FO/m"s,  CURRENT } },
  {  510, { "PALESTINE"s,                               "E4"s,    CURRENT } },
  {  511, { "TIMOR-LESTE"s,                             "4W"s,    CURRENT } },
  {  512, { "CHESTERFIELD IS."s,                        "FK/c"s,  CURRENT } },
  {  513, { "DUCIE I."s,                                "VP6/d"s, CURRENT } },
  {  514, { "MONTENEGRO"s,                              "4O"s,    CURRENT } },
  {  515, { "SWAINS I."s,                               "KH8/s"s, CURRENT } },
  {  516, { "SAINT BARTHELEMY"s,                        "FJ"s,    CURRENT } },
  {  517, { "CURACAO"s,                                 "PJ2"s,   CURRENT } },
  {  518, { "ST MAARTEN"s,                              "PJ7"s,   CURRENT } },
  {  519, { "SABA & ST. EUSTATIUS"s,                    "PJ5"s,   CURRENT } },
  {  520, { "BONAIRE"s,                                 "PJ4"s,   CURRENT } },
  {  521, { "SOUTH SUDAN (REPUBLIC OF)"s,               "Z8"s,    CURRENT } },
  {  522, { "REPUBLIC OF KOSOVO"s,                      "Z6"s,    CURRENT } }
};

/// mode values
UNORDERED_STRING_SET adif3_field::_ENUMERATION_MODE
{ "AM"s,       "ARDOP"s,    "ATV"s,     "C4FM"s,   "CHIP"s,    "CLO"s,     "CONTESTI"s, "CW"s,       "DIGITALVOICE"s, "DOMINO"s, 
  "DSTAR"s,    "FAX"s,      "FM"s,      "FSK441"s, "FT8"s,     "HELL"s,    "ISCAT"s,    "JT4"s,      "JT6M"s,         "JT9"s, 
  "JT44"s,     "JT65"s,     "MFSK"s,    "MSK144"s, "MT63"s,    "OLIVIA"s,  "OPERA"s,    "PAC"s,      "PAX"s,          "PKT"s, 
  "PSK"s,      "PSK2K"s,    "Q15"s,     "QRA64"s,  "ROS"s,     "RTTY"s,    "RTTYM"s,    "SSB"s,      "SSTV"s,         "T10"s, 
  "THOR"s,     "THRB"s,     "TOR"s,     "V4"s,     "VOI"s,     "WINMOR"s,  "WSPR"s,     "AMTORFEC"s, "ASCI"s,         "CHIP64"s, 
  "CHIP128"s,  "DOMINOF"s,  "FMHELL"s,  "FSK31"s,  "GTOR"s,    "HELL80"s,  "HFSK"s,     "JT4A"s,     "JT4B"s,         "JT4C"s, 
  "JT4D"s,     "JT4E"s,     "JT4F"s,    "JT4G"s,   "JT65 "s,   "JT65 "s,   "JT65C"s,    "MFSK8"s,    "MFSK16"s,       "PAC2"s,
  "PAC3"s,     "PAX2"s,     "PCW"s,     "PSK10"s,  "PSK31"s,   "PSK63"s,   "PSK63F"s,   "PSK125"s,   "PSKAM10"s,      "PSKAM31"s, 
  "PSKAM50"s,  "PSKFEC31"s, "PSKHELL"s, "QPSK31"s, "QPSK63"s,  "QPSK125"s, "THRBX"s
};

/// legal values of QSL_RCVD
STRING_SET adif3_field::_ENUMERATION_QSL_RECEIVED { "Y"s, "N"s, "R"s, "I"s, "V"s };

/// fields that are not to be output
set<ADIF3_DATA_TYPE> adif3_record::_import_only
{ ADIF3_DATA_TYPE::AWARD_LIST };
