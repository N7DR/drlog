// $Id: adif3.cpp 224 2023-08-03 20:54:02Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

/*! \file   adif3.cpp

    Objects and functions related to ADIF version 3.1.0 at https://adif.org/310/ADIF_310.htm
    
    I refrain from comment on the self-evident quality of this soi-disant "specification"
*/

#include "adif3.h"

#include <algorithm>
#include <iostream>

using namespace std;

/*! \brief  place values into standardised forms as necessary

    Assumes that names are in upper case
*/
void adif3_field::_normalise(void)
{ static const set<string> uc_fields { "CALL"s, "MODE"s, "STATION_CALLSIGN"s };   ///< fields that have UC values
  static const string      zero_zero { "00"s };

  switch (_type)
  { case ADIF3_DATA_TYPE::ENUMERATION_BAND :
      _value = to_lower(_value);
      return;
    
    case ADIF3_DATA_TYPE::ENUMERATION_QSL_RECEIVED :
      _value = to_upper(_value);
      return;
  
    case ADIF3_DATA_TYPE::GRID_SQUARE :             // AAnnaann
      _value[0] = toupper(_value[0]);
      _value[1] = toupper(_value[1]);
      
      if (_value.length() >= 6)
      { _value[4] = tolower(_value[4]);
        _value[5] = tolower(_value[5]);
      }
      return;
  
    case ADIF3_DATA_TYPE::TIME :                // six characters
      if (_value.length() == 4)
        _value += zero_zero;
      return;
      
    default :
      break;
  }

// force some values into upper case
//  if (uc_fields > _name)
  if (uc_fields.contains(_name))
  { _value = to_upper(_value);
    return;
  }
}

/// verify that values are legal
void adif3_field::_verify(void) const
{ switch (_type)
  { case ADIF3_DATA_TYPE::DATE :                                      // YYYYMMDD
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
      
    case ADIF3_DATA_TYPE::ENUMERATION_BAND :
    { if (!(_ENUMERATION_BAND > to_lower(_value)))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
    }
    break;

    case ADIF3_DATA_TYPE::ENUMERATION_DXCC_ENTITY_CODE :
    { if (_value.find_first_not_of(DIGITS) != string::npos)                         // check that it's an integer
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid character in "s + _name + ": "s + _value);

      if (!(_ENUMERATION_DXCC_ENTITY_CODE > from_string<int>(_value)))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid DXCC entity code in "s + _name + ": "s + _value);
    }
    break;
    
    case ADIF3_DATA_TYPE::ENUMERATION_MODE :
    { if (!(_ENUMERATION_MODE > to_upper(_value)))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
    }
    break;
 
    case ADIF3_DATA_TYPE::ENUMERATION_QSL_RECEIVED :
    { if (!( _ENUMERATION_QSL_RECEIVED > to_upper(_value)))
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
    }
    break;
 
/*
The first pair (a field) encodes with base 18 and the letters "A" to "R".
The second pair (square) encodes with base 10 and the digits "0" to "9".
The third pair (subsquare) encodes with base 24 and the letters "a" to "x".
The fourth pair (extended square) encodes with base 10 and the digits "0" to "9".
*/
    case ADIF3_DATA_TYPE::GRID_SQUARE :
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

    case ADIF3_DATA_TYPE::NUMBER :                               // a sequence of one or more Digits representing a decimal number, optionally preceded by a minus sign (ASCII code 45) and optionally including a single decimal point (ASCII code 46)
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
    
    case ADIF3_DATA_TYPE::POSITIVE_INTEGER :      // an unsigned sequence of one or more Digits representing a decimal integer that has a value greater than 0.  Leading zeroes are allowed. 
    { if (!is_digits(_value))
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);

// CQZ : 1..40
      if (const auto cit { _positive_integer_range.find(_name) }; cit != _positive_integer_range.cend())
      { //const int min_value { cit->second.first };
        //const int max_value { cit->second.second };
        const auto [min_value, max_value] = cit->second;
        
        if (const int zone { from_string<int>(_value) }; ((zone < min_value) or (zone > max_value)) )
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
      }
    }
    break; 

    case ADIF3_DATA_TYPE::STRING :        // amazingly (but nothing about this should amaze me), a "sequence of": ASCII character whose code lies in the range of 32 through 126, inclusive
    { for (const char c : _value)
//        if ( (static_cast<int>(c) < 32) or (static_cast<int>(c) > 126) )
        if ( int intval { static_cast<int>(c) }; (intval < 32) or (intval > 126) )
          throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
    }
    break;

    case ADIF3_DATA_TYPE::TIME :          // HHMMSS or HHMM
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
adif3_field::adif3_field(const string& field_name, const string& field_value)
{ _name = to_upper(field_name);
  
  const auto it { _element_type.find(_name) };

  if (it == _element_type.end())
    throw adif3_error(ADIF3_UNKNOWN_TYPE, "Cannot find type for element: "s + _name + " when creating ADIF3_FIELD with value: "s + field_value);
    
  _type = it->second;
  _value = field_value;
  
  _verify();
  _normalise();
}

/*! \brief              Import name and value from string, and return location past the end of the used part of the string
    \param  str         string from which to read
    \param  start_posn  position in <i>str</i> at which to start parsing
    \param  end_posn    one past the location at which to force and end to parsing, if necessary
    \return             one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/
size_t adif3_field::import_and_eat(const std::string& str, const size_t start_posn, const size_t end_posn /* last char of <EOR> */)
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
//  const string         descriptor_str { str.substr(posn_1 + 1, posn_2 - posn_1 -1) };
  const string_view         descriptor_str { substring <string_view> (str, posn_1 + 1, posn_2 - posn_1 -1) };
  const vector<string_view> fields         { split_string <std::string_view> (descriptor_str, ':') };
//  const vector<string> fields         { split_string <std::string> (descriptor_str, ':') };

  if ( (fields.size() < 2) or (fields.size() > 3) )     // wrong number of fields
    return string::npos;
    
  if (!_name.empty() and ( to_upper(fields[0]) != to_upper(name()) ))      // name mismatch
    return string::npos;
    
  if (_name.empty())
    _name = to_upper(fields[0]);                                // force name to UC
 
  const int    n_chars  { from_string<int>(fields[1]) };
  const string contents { str.substr(posn_2 + 1, n_chars) };
  const size_t rv       { posn_2 + 1 + n_chars };        // eat the used text; should be one past the end of the value of this field
    
// check validity
  const auto it { _element_type.find(_name) };

  if (it == _element_type.end())
    throw adif3_error(ADIF3_UNKNOWN_TYPE, "Cannot find type for element: "s + _name);
    
  _type = it->second;
  _value = contents;
    
  _verify();
  _normalise();
  
  return rv;
}  

// ---------------------------------------------------  adif3_record -----------------------------------------

/*! \brief          Convert a string to an int, assuming that the string contains just digits
    \param  str     string to convert
    \return         <i>str</i> rendered as an int

    Result is valid ONLY if <i>str</i> contains only digits
*/ 
int adif3_record::_fast_string_to_int(const string& str) const
{ static constexpr int zerochar { '0' };

  int rv { 0 };

  for (const char c : str)
//    rv = (rv * 10) + ((int)c - zerochar);
    rv = (rv * 10) + (static_cast<int>(c) - zerochar);

  return rv; 
}

/*! \brief          Import record from string, and return location past the end of the used part of the string
    \param  str     string from which to read
    \param  posn    position in <i>str</i> at which to start parsing
    \return         one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/ 
size_t adif3_record::import_and_eat(const std::string& str, const size_t posn)
{ 
// extract the text from the first "<" to the end of the first "eor"
  const auto posn_1 { str.find('<', posn) };

  if (posn_1 == string::npos)        // could not find initial delimiter
    return string::npos;
    
  const auto posn_2 { case_insensitive_find(str, "<EOR>"sv, posn)  + 4 };  // 4 = length("<EOR>") - 1; posn_2 points to last char: ">"
    
  if (posn_2 == string::npos)        // could not find end-of-record marker
    return string::npos;
  
  const size_t rv { ( (posn_2 + 1) >= str.length() ? string::npos : posn_2 + 1) };
  
  size_t start_posn { posn_1 };
  
  while (start_posn <= posn_2)
  { adif3_field element;

    start_posn = element.import_and_eat(str, start_posn, posn_2);             // name is forced to upper case
    
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
{ string rv;

  for (const auto& element : _elements)
  { if (const ADIF3_DATA_TYPE dt { element.second.type() }; !( _import_only > dt ) )       // don't output if this type is import-only
    { switch (dt)
      { default :
          rv += element.second.to_string();    // output without any checks
      }
    }
  }
  
  rv += "<EOR>\n";
  
  return rv;
}

/*! \brief                  Set the value of a field (which does not have to be extant in the record)
    \param  field_name      name of the field whose value is to be set
    \param  field_value     value to be set
    \return                 whether this was a new field

    <i>field_name</i> is converted to upper case when stored as <i>_name</i>
    <i>field_value</i> is validated and converted to standardised format (if applicable)
*/
bool adif3_record::value(const string& field_name, const string& field_value)
{ const adif3_field element        { field_name, field_value };
  const auto        [it, inserted] { _elements.insert( { element.name(), element } ) };
 
  return inserted;
}

/*! \brief         Is one ADIF3 record chronologically earlier than another
    \param  rec1   first record
    \param  rec2   second record
    \return        whether <i>record1</i> is chronologically before <i>record2</i> 
*/
bool compare_adif3_records(const adif3_record& rec1, const adif3_record& rec2)
{ if (rec1.date() < rec2.date())
    return true;
    
  if (rec2.date() < rec1.date())
    return false;
    
// same date
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
adif3_file::adif3_file(const string& filename)
{ const string contents { read_file(filename) };            // this might throw
  
  size_t start_posn { skip_adif3_header(contents) };
  
  while (start_posn != string::npos)
  { adif3_record rec;
    
    start_posn = rec.import_and_eat(contents, start_posn);
    
    if (start_posn != string::npos)
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
adif3_file::adif3_file(const vector<string>& path, const string& filename)
{ for (const auto& this_path : path)
  { try
    { *this = adif3_file { this_path + "/"s + filename };
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
std::vector<adif3_record> adif3_file::matching_qsos(const string& callsign, const string& b, const string& m) const
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
std::vector<adif3_record> adif3_file::matching_qsos(const string& callsign) const
{ vector<adif3_record> rv;

  const auto [begin_it, end_it] { _map_data.equal_range(callsign) };
  
  FOR_ALL(begin_it, end_it, [&rv](const auto& map_entry) { rv += map_entry.second; });
  
  return rv;
}

/*! \brief              Return position at which to start processing the body of the file
    \param      str     string of contents of filae
    \return             position of first "<" after the end of the header
*/
size_t skip_adif3_header(const std::string& str)
{ const auto posn_1 { case_insensitive_find(str, "<EOH>"s) };

  return ( (posn_1 == string::npos) ? 0 : (str.find('<', posn_1 + 1)) ); // either start of file or first "<" after "<EOH>"
}

/// map from field name to type -- too many of these are silly for it to be worth making comment on individual sillinesses
const unordered_map<string, ADIF3_DATA_TYPE> adif3_field::_element_type
{ { "ADDRESS"s,                   ADIF3_DATA_TYPE::MULTILINE_STRING },
  { "AGE"s,                       ADIF3_DATA_TYPE::NUMBER },
  { "A_INDEX"s,                   ADIF3_DATA_TYPE::NUMBER },
  { "ANT_AZ"s,                    ADIF3_DATA_TYPE::NUMBER },
  { "ANT_EL"s,                    ADIF3_DATA_TYPE::NUMBER },
  { "ANT_PATH"s,                  ADIF3_DATA_TYPE::ENUMERATION_ANT_PATH },
  { "ARRL_SECT"s,                 ADIF3_DATA_TYPE::ENUMERATION_ARRL_SECT },
  { "AWARD_SUBMITTED"s,           ADIF3_DATA_TYPE::SPONSORED_AWARD_LIST },
  { "AWARD_GRANTED"s,             ADIF3_DATA_TYPE::SPONSORED_AWARD_LIST },
  { "BAND"s,                      ADIF3_DATA_TYPE::ENUMERATION_BAND },
  { "BAND_RX"s,                   ADIF3_DATA_TYPE::ENUMERATION_BAND },
  { "CALL"s,                      ADIF3_DATA_TYPE::STRING },
  { "CHECK"s,                     ADIF3_DATA_TYPE::STRING },
  { "CLASS"s,                     ADIF3_DATA_TYPE::STRING },
  { "CLUBLOG_QSO_UPLOAD_DATE"s,   ADIF3_DATA_TYPE::DATE },
  { "CLUBLOG_QSO_UPLOAD_STATUS"s, ADIF3_DATA_TYPE::ENUMERATION_QSO_UPLOAD_STATUS },
  { "CNTY"s,                      ADIF3_DATA_TYPE::ENUMERATION_SECONDARY_ADMINISTRATIVE_SUBDIVISION },
  { "COMMENT"s,                   ADIF3_DATA_TYPE::STRING },
  { "COMMENT_INTL"s,              ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "CONT"s,                      ADIF3_DATA_TYPE::ENUMERATION_CONTINENT },
  { "CONTACTED_OP"s,              ADIF3_DATA_TYPE::STRING },
  { "CONTEST_ID"s,                ADIF3_DATA_TYPE::STRING },
  { "COUNTRY"s,                   ADIF3_DATA_TYPE::STRING },
  { "COUNTRY_INTL"s,              ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "CQZ"s,                       ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "CREDIT_SUBMITTED"s,          ADIF3_DATA_TYPE::CREDIT_LIST },                   // have to think about this, as it could be an import-only AWARD_LIST
  { "CREDIT_GRANTED"s,            ADIF3_DATA_TYPE::CREDIT_LIST },                   // have to think about this, as it could be an import-only AWARD_LIST
  { "DARC_DOK"s,                  ADIF3_DATA_TYPE::ENUMERATION_DARC_DOK },
  { "DISTANCE"s,                  ADIF3_DATA_TYPE::NUMBER },
  { "DXCC"s,                      ADIF3_DATA_TYPE::ENUMERATION_DXCC_ENTITY_CODE },
  { "EMAIL"s,                     ADIF3_DATA_TYPE::STRING },
  { "EQ_CALL"s,                   ADIF3_DATA_TYPE::STRING },                         // I have no idea where this name came from
  { "EQSL_QSLRDATE"s,             ADIF3_DATA_TYPE::DATE },
  { "EQSL_QSLSDATE"s,             ADIF3_DATA_TYPE::DATE },
  { "EQSL_QSL_RCVD"s,             ADIF3_DATA_TYPE::ENUMERATION_QSL_RECEIVED },
  { "EQSL_QSL_SENT"s,             ADIF3_DATA_TYPE::ENUMERATION_QSL_SENT },
  { "FISTS"s,                     ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "FISTS_CC"s,                  ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "FORCE_INIT"s,                ADIF3_DATA_TYPE::BOOLEAN },
  { "FREQ"s,                      ADIF3_DATA_TYPE::NUMBER },
  { "FREQ_RX"s,                   ADIF3_DATA_TYPE::NUMBER },
  { "GRIDSQUARE"s,                ADIF3_DATA_TYPE::GRID_SQUARE },
  { "GUEST_OP"s,                  ADIF3_DATA_TYPE::STRING },                         // import only
  { "HRDLOG_QSO_UPLOAD_DATE"s,    ADIF3_DATA_TYPE::DATE },
  { "HRDLOG_QSO_UPLOAD_STATUS"s,  ADIF3_DATA_TYPE::ENUMERATION_QSO_UPLOAD_STATUS },
  { "IOTA"s,                      ADIF3_DATA_TYPE::IOTA_REFERENCE_NUMBER },
  { "IOTA_ISLAND_ID"s,            ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "ITUZ"s,                      ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "K_INDEX"s,                   ADIF3_DATA_TYPE::INTEGER },                        // one wonders where...
  { "LAT"s,                       ADIF3_DATA_TYPE::LOCATION },
  { "LON"s,                       ADIF3_DATA_TYPE::LOCATION },
  { "LOTW_QSLRDATE"s,             ADIF3_DATA_TYPE::DATE },
  { "LOTW_QSLSDATE"s,             ADIF3_DATA_TYPE::DATE },
  { "LOTW_QSL_RCVD"s,             ADIF3_DATA_TYPE::ENUMERATION_QSL_RECEIVED },
  { "LOTW_QSL_SENT"s,             ADIF3_DATA_TYPE::ENUMERATION_QSL_SENT },
  { "MAX_BURSTS"s,                ADIF3_DATA_TYPE::NUMBER },
  { "MODE"s,                      ADIF3_DATA_TYPE::ENUMERATION_MODE },
  { "MS_SHOWER"s,                 ADIF3_DATA_TYPE::STRING },
  { "MY_ANTENNA"s,                ADIF3_DATA_TYPE::STRING },
  { "MY_ANTENNA_INTL"s,           ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_CITY"s,                   ADIF3_DATA_TYPE::STRING },
  { "MY_CITY_INTL"s,              ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_CNTY"s,                   ADIF3_DATA_TYPE::ENUMERATION_SECONDARY_ADMINISTRATIVE_SUBDIVISION },
  { "MY_COUNTRY"s,                ADIF3_DATA_TYPE::STRING },
  { "MY_COUNTRY_INTL"s,           ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_CQ_ZONE"s,                ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "MY_DXCC"s,                   ADIF3_DATA_TYPE::ENUMERATION_DXCC_ENTITY_CODE },
  { "MY_FISTS"s,                  ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "MY_GRIDSQUARE"s,             ADIF3_DATA_TYPE::GRID_SQUARE },
  { "MY_IOTA"s,                   ADIF3_DATA_TYPE::IOTA_REFERENCE_NUMBER },
  { "MY_IOTA_ISLAND_ID"s,         ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "MY_ITU_ZONE"s,               ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "MY_LAT"s,                    ADIF3_DATA_TYPE::LOCATION },
  { "MY_LON"s,                    ADIF3_DATA_TYPE::LOCATION },
  { "MY_NAME"s,                   ADIF3_DATA_TYPE::STRING },
  { "MY_NAME_INTL"s,              ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_POSTAL_CODE"s,            ADIF3_DATA_TYPE::STRING },
  { "MY_POSTAL_CODE_INTL"s,       ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_RIG"s,                    ADIF3_DATA_TYPE::STRING },
  { "MY_RIG_INTL"s,               ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_SIG"s,                    ADIF3_DATA_TYPE::STRING },
  { "MY_SIG_INTL"s,               ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_SIG_INFO"s,               ADIF3_DATA_TYPE::STRING },
  { "MY_SIG_INFO_INTL"s,          ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_SOTA_REF"s,               ADIF3_DATA_TYPE::SOTA_REFERENCE },
  { "MY_STATE"s,                  ADIF3_DATA_TYPE::ENUMERATION_PRIMARY_ADMINISTRATIVE_SUBDIVISION },
  { "MY_STREET"s,                 ADIF3_DATA_TYPE::STRING },                                            // !!
  { "MY_STREET_INTL"s,            ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "MY_USACA_COUNTIES"s,         ADIF3_DATA_TYPE::SECONDARY_SUBDIVISION_LIST },
  { "MY_VUCC_GRIDS"s,             ADIF3_DATA_TYPE::GRID_SQUARE_LIST },
  { "NAME"s,                      ADIF3_DATA_TYPE::STRING },
  { "NAME_INTL"s,                 ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "NOTES"s,                     ADIF3_DATA_TYPE::MULTILINE_STRING },
  { "NOTES_INTL"s,                ADIF3_DATA_TYPE::INTERNATIONAL_MULTILINE_STRING },
  { "NR_BURSTS"s,                 ADIF3_DATA_TYPE::INTEGER },
  { "NR_PINGS"s,                  ADIF3_DATA_TYPE::INTEGER },
  { "OPERATOR"s,                  ADIF3_DATA_TYPE::STRING },
  { "OWNER_CALLSIGN"s,            ADIF3_DATA_TYPE::STRING },
  { "PFX"s,                       ADIF3_DATA_TYPE::STRING },
  { "PRECEDENCE"s,                ADIF3_DATA_TYPE::STRING },
  { "PROP_MODE"s,                 ADIF3_DATA_TYPE::ENUMERATION_PROPAGATION_MODE },
  { "PUBLIC_KEY"s,                ADIF3_DATA_TYPE::STRING },
  { "QRZCOM_QSO_UPLOAD_DATE"s,    ADIF3_DATA_TYPE::DATE },
  { "QRZCOM_QSO_UPLOAD_STATUS"s,  ADIF3_DATA_TYPE::ENUMERATION_QSO_UPLOAD_STATUS },
  { "QSLMSG"s,                    ADIF3_DATA_TYPE::MULTILINE_STRING },
  { "QSLMSG"s,                    ADIF3_DATA_TYPE::INTERNATIONAL_MULTILINE_STRING },
  { "QSLRDATE"s,                  ADIF3_DATA_TYPE::DATE },
  { "QSLSDATE"s,                  ADIF3_DATA_TYPE::DATE },
  { "QSL_RCVD"s,                  ADIF3_DATA_TYPE::ENUMERATION_QSL_RECEIVED },
  { "QSL_RCVD_VIA"s,              ADIF3_DATA_TYPE::ENUMERATION_QSL_VIA },
  { "QSL_SENT"s,                  ADIF3_DATA_TYPE::ENUMERATION_QSL_SENT },
  { "QSL_SENT_VIA"s,              ADIF3_DATA_TYPE::ENUMERATION_QSL_VIA },
  { "QSL_VIA"s,                   ADIF3_DATA_TYPE::STRING },
  { "QSO_COMPLETE"s,              ADIF3_DATA_TYPE::ENUMERATION_QSO_COMPLETE },
  { "QSO_DATE"s,                  ADIF3_DATA_TYPE::DATE },
  { "QSO_DATE_OFF"s,              ADIF3_DATA_TYPE::DATE },
  { "QSO_RANDOM"s,                ADIF3_DATA_TYPE::BOOLEAN },
  { "QTH"s,                       ADIF3_DATA_TYPE::STRING },
  { "QTH_INTL"s,                  ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "REGION"s,                    ADIF3_DATA_TYPE::ENUMERATION_REGION },
  { "RIG"s,                       ADIF3_DATA_TYPE::MULTILINE_STRING },
  { "RIG_INTL"s,                  ADIF3_DATA_TYPE::INTERNATIONAL_MULTILINE_STRING },
  { "RST_RCVD"s,                  ADIF3_DATA_TYPE::STRING },
  { "RST_SENT"s,                  ADIF3_DATA_TYPE::STRING },
  { "RX_PWR"s,                    ADIF3_DATA_TYPE::NUMBER },                             // !!
  { "SAT_MODE"s,                  ADIF3_DATA_TYPE::STRING },
  { "SAT_NAME"s,                  ADIF3_DATA_TYPE::STRING },
  { "SFI_NAME"s,                  ADIF3_DATA_TYPE::INTEGER },
  { "SIG"s,                       ADIF3_DATA_TYPE::STRING },
  { "SIG_INTL"s,                  ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "SIG_INFO"s,                  ADIF3_DATA_TYPE::STRING },
  { "SIG_INFO_INTL"s,             ADIF3_DATA_TYPE::INTERNATIONAL_STRING },
  { "SILENT_KEY"s,                ADIF3_DATA_TYPE::BOOLEAN },
  { "SKCC"s,                      ADIF3_DATA_TYPE::STRING },
  { "SOTA_REF"s,                  ADIF3_DATA_TYPE::SOTA_REFERENCE },
  { "SRX"s,                       ADIF3_DATA_TYPE::INTEGER },                            // goodness knows how the abbreviation was derived
  { "SRX_STRING"s,                ADIF3_DATA_TYPE::STRING },
  { "STATE"s,                     ADIF3_DATA_TYPE::ENUMERATION_PRIMARY_ADMINISTRATIVE_SUBDIVISION },
  { "STATION_CALLSIGN"s,          ADIF3_DATA_TYPE::STRING },
  { "STX"s,                       ADIF3_DATA_TYPE::INTEGER },
  { "STX_STRING"s,                ADIF3_DATA_TYPE::STRING },
  { "SUBMODE"s,                   ADIF3_DATA_TYPE::STRING },                             // crazy... depends on mode... even though it's defined as a "String"
  { "SWL"s,                       ADIF3_DATA_TYPE::BOOLEAN },
  { "TEN_TEN"s,                   ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "TIME_OFF"s,                  ADIF3_DATA_TYPE::TIME },
  { "TIME_ON"s,                   ADIF3_DATA_TYPE::TIME },
  { "TX_PWR"s,                    ADIF3_DATA_TYPE::NUMBER },
  { "UKSMG"s,                     ADIF3_DATA_TYPE::POSITIVE_INTEGER },
  { "USACA_COUNTIES"s,            ADIF3_DATA_TYPE::SECONDARY_SUBDIVISION_LIST },
  { "VE_PROV"s,                   ADIF3_DATA_TYPE::STRING },                             // import only
  { "VUCC_GRIDS"s,                ADIF3_DATA_TYPE::GRID_SQUARE_LIST },
  { "WEB"s,                       ADIF3_DATA_TYPE::STRING }
};

/// map from field name to permitted range of values
map<std::string, pair<int, int>> adif3_field::_positive_integer_range
{ { "CQZ", { 1, 40 } }
};

/// band values
const unordered_set<string> adif3_field::_ENUMERATION_BAND
{ "2190m"s, "630m"s, "560m"s, "160m"s, "80m"s,    "60m"s, "40m"s,   "30m"s,   "20m"s,  "17m"s,
  "15m"s,   "12m"s,  "10m"s,  "6m"s,   "4m"s,     "2m"s,  "1.25m"s, "70cm"s,  "33cm"s, "23cm"s,
  "13cm"s,  "9cm"s,  "6cm"s,  "3cm"s,  "1.25cm"s, "6mm"s, "4mm"s,   "2.5mm"s, "2mm"s,  "1mm"s
};

/// mapping between country code and country info
//using enum COUNTRY_STATUS; not supported in g++ 10
//const unordered_map<int /* country number */, tuple<string /*country name */, string /* canonical prefix */, bool /* whether deleted */>> adif3_field::_ENUMERATION_DXCC_ENTITY_CODE
const unordered_map<int /* country number */, tuple<string /*country name */, string /* canonical prefix */, COUNTRY_STATUS /* whether deleted */>> adif3_field::_ENUMERATION_DXCC_ENTITY_CODE
{ //using enum COUNTRY_STATUS; not supported in g++ 10

  {  1  , { "CANADA"s,                                  "VE"s,    COUNTRY_STATUS::CURRENT } },
  {  2  , { "ABU AIL IS."s,                             ""s,      COUNTRY_STATUS::DELETED } },
  {  3  , { "AFGHANISTAN"s,                             "YA"s,    COUNTRY_STATUS::CURRENT } },
  {  4  , { "AGALEGA & ST. BRANDON IS."s,               "3B6"s,   COUNTRY_STATUS::CURRENT } },
  {  5  , { "ALAND IS."s,                               "OH0"s,   COUNTRY_STATUS::CURRENT } },
  {  6  , { "ALASKA"s,                                  "KL"s,    COUNTRY_STATUS::CURRENT } },
  {  7  , { "ALBANIA"s,                                 "ZA"s,    COUNTRY_STATUS::CURRENT } },
  {  8  , { "ALDABRA"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  9  , { "AMERICAN SAMOA"s,                          "KH8"s,   COUNTRY_STATUS::CURRENT } },
  {  10 , { "AMSTERDAM & ST. PAUL IS."s,                "FT5Z"s,  COUNTRY_STATUS::CURRENT } },
  {  11 , { "ANDAMAN & NICOBAR IS."s,                   "VU4"s,   COUNTRY_STATUS::CURRENT } },
  {  12 , { "ANGUILLA"s,                                "VP2E"s,  COUNTRY_STATUS::CURRENT } },
  {  13 , { "ANTARCTICA"s,                              "CE9"s,   COUNTRY_STATUS::CURRENT } },
  {  14 , { "ARMENIA"s,                                 "EK"s,    COUNTRY_STATUS::CURRENT } },
  {  15 , { "ASIATIC RUSSIA"s,                          "UA9"s,   COUNTRY_STATUS::CURRENT } },
  {  16 , { "NEW ZEALAND SUBANTARCTIC ISLANDS"s,        "ZL9"s,   COUNTRY_STATUS::CURRENT } },
  {  17 , { "AVES I."s,                                 "YV0"s,   COUNTRY_STATUS::CURRENT } },
  {  18 , { "AZERBAIJAN"s,                              "4J"s,    COUNTRY_STATUS::CURRENT } },
  {  19 , { "BAJO NUEVO"s,                              ""s,      COUNTRY_STATUS::DELETED } },
  {  20 , { "BAKER & HOWLAND IS."s,                     "KH1"s,   COUNTRY_STATUS::CURRENT } },
  {  21 , { "BALEARIC IS."s,                            "EA6"s,   COUNTRY_STATUS::CURRENT } },
  {  22 , { "PALAU"s,                                   "T8"s,    COUNTRY_STATUS::CURRENT } },
  {  23 , { "BLENHEIM REEF"s,                           ""s,      COUNTRY_STATUS::DELETED } },
  {  24 , { "BOUVET"s,                                  "3Y/b"s,  COUNTRY_STATUS::CURRENT } },
  {  25 , { "BRITISH NORTH BORNEO"s,                    ""s,      COUNTRY_STATUS::DELETED } },
  {  26 , { "BRITISH SOMALILAND"s,                      ""s,      COUNTRY_STATUS::DELETED } },
  {  27 , { "BELARUS"s,                                 "EU"s,    COUNTRY_STATUS::CURRENT } },
  {  28 , { "CANAL ZONE"s,                              ""s,      COUNTRY_STATUS::DELETED } },
  {  29 , { "CANARY IS."s,                              "EA8"s,   COUNTRY_STATUS::CURRENT } },
  {  30 , { "CELEBE & MOLUCCA IS."s,                    ""s,      COUNTRY_STATUS::DELETED } },
  {  31 , { "C. KIRIBATI (BRITISH PHOENIX IS.)"s,       "EA9"s,   COUNTRY_STATUS::CURRENT } },
  {  32 , { "CEUTA & MELILLA"s,                         "T31"s,   COUNTRY_STATUS::CURRENT } },
  {  33 , { "CHAGOS IS."s,                              "VQ9"s,   COUNTRY_STATUS::CURRENT } },
  {  34 , { "CHATHAM IS."s,                             "ZL7"s,   COUNTRY_STATUS::CURRENT } },
  {  35 , { "CHRISTMAS I."s,                            "VK9X"s,  COUNTRY_STATUS::CURRENT } },
  {  36 , { "CLIPPERTON I."s,                           "FO/c"s,  COUNTRY_STATUS::CURRENT } },
  {  37 , { "COCOS I."s,                                "TI9"s,   COUNTRY_STATUS::CURRENT } },
  {  38 , { "COCOS (KEELING) IS."s,                     "VK9C"s,  COUNTRY_STATUS::CURRENT } },
  {  39 , { "COMOROS"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  40 , { "CRETE"s,                                   "SV9"s,   COUNTRY_STATUS::CURRENT } },
  {  41 , { "CROZET I."s,                               "FT5W"s,  COUNTRY_STATUS::CURRENT } },
  {  42 , { "DAMAO, DIU"s,                              ""s,      COUNTRY_STATUS::DELETED } },
  {  43 , { "DESECHEO I."s,                             "KP5"s,   COUNTRY_STATUS::CURRENT } },
  {  44 , { "DESROCHES"s,                               ""s,      COUNTRY_STATUS::DELETED } },
  {  45 , { "DODECANESE"s,                              "SV5"s,   COUNTRY_STATUS::CURRENT } },
  {  46 , { "EAST MALAYSIA"s,                           "9M6"s,   COUNTRY_STATUS::CURRENT } },
  {  47 , { "EASTER I."s,                               "CE0Y"s,  COUNTRY_STATUS::CURRENT } },
  {  48 , { "E. KIRIBATI (LINE IS.)"s,                  "T32"s,   COUNTRY_STATUS::CURRENT } },
  {  49 , { "EQUATORIAL GUINEA"s,                       "3C"s,    COUNTRY_STATUS::CURRENT } },
  {  50 , { "MEXICO"s,                                  "XE"s,    COUNTRY_STATUS::CURRENT } },
  {  51 , { "ERITREA"s,                                 "E3"s,    COUNTRY_STATUS::CURRENT } },
  {  52 , { "ESTONIA"s,                                 "ES"s,    COUNTRY_STATUS::CURRENT } },
  {  53 , { "ETHIOPIA"s,                                "ET"s,    COUNTRY_STATUS::CURRENT } },
  {  54 , { "EUROPEAN RUSSIA"s,                         "UA"s,    COUNTRY_STATUS::CURRENT } },
  {  55 , { "FARQUHAR"s,                                ""s,      COUNTRY_STATUS::DELETED } },
  {  56 , { "FERNANDO DE NORONHA"s,                     "PY0F"s,  COUNTRY_STATUS::CURRENT } },
  {  57 , { "FRENCH EQUATORIAL AFRICA"s,                ""s,      COUNTRY_STATUS::DELETED } },
  {  58 , { "FRENCH INDO-CHINA"s,                       ""s,      COUNTRY_STATUS::DELETED } },
  {  59 , { "FRENCH WEST AFRICA"s,                      ""s,      COUNTRY_STATUS::DELETED } },
  {  60 , { "BAHAMAS"s,                                 "C6"s,    COUNTRY_STATUS::CURRENT } },
  {  61 , { "FRANZ JOSEF LAND"s,                        "R1FJ"s,  COUNTRY_STATUS::CURRENT } },
  {  62 , { "BARBADOS"s,                                "8P"s,    COUNTRY_STATUS::CURRENT } },
  {  63 , { "FRENCH GUIANA"s,                           "FY"s,    COUNTRY_STATUS::CURRENT } },
  {  64 , { "BERMUDA"s,                                 "VP9"s,   COUNTRY_STATUS::CURRENT } },
  {  65 , { "BRITISH VIRGIN IS."s,                      "VP2V"s,  COUNTRY_STATUS::CURRENT } },
  {  66 , { "BELIZE"s,                                  "V3"s,    COUNTRY_STATUS::CURRENT } },
  {  67 , { "FRENCH INDIA"s,                            ""s,      COUNTRY_STATUS::DELETED } },
  {  68 , { "KUWAIT/SAUDI ARABIA NEUTRAL ZONE"s,        ""s,      COUNTRY_STATUS::DELETED } },
  {  69 , { "CAYMAN IS."s,                              "ZF"s,    COUNTRY_STATUS::CURRENT } },
  {  70 , { "CUBA"s,                                    "CM"s,    COUNTRY_STATUS::CURRENT } },
  {  71 , { "GALAPAGOS IS."s,                           "HC8"s,   COUNTRY_STATUS::CURRENT } },
  {  72 , { "DOMINICAN REPUBLIC"s,                      "HI"s,    COUNTRY_STATUS::CURRENT } },
  {  74 , { "EL SALVADOR"s,                             "YS"s,    COUNTRY_STATUS::CURRENT } },
  {  75 , { "GEORGIA"s,                                 "4L"s,    COUNTRY_STATUS::CURRENT } },
  {  76 , { "GUATEMALA"s,                               "TG"s,    COUNTRY_STATUS::CURRENT } },
  {  77 , { "GRENADA"s,                                 "J3"s,    COUNTRY_STATUS::CURRENT } },
  {  78 , { "HAITI"s,                                   "HH"s,    COUNTRY_STATUS::CURRENT } },
  {  79 , { "GUADELOUPE"s,                              "FG"s,    COUNTRY_STATUS::CURRENT } },
  {  80 , { "HONDURAS"s,                                "HR"s,    COUNTRY_STATUS::CURRENT } },
  {  81 , { "GERMANY"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  82 , { "JAMAICA"s,                                 "6Y"s,    COUNTRY_STATUS::CURRENT } },
  {  84 , { "MARTINIQUE"s,                              "FM"s,    COUNTRY_STATUS::CURRENT } },
  {  85 , { "BONAIRE, CURACAO"s,                        ""s,      COUNTRY_STATUS::DELETED } },
  {  86 , { "NICARAGUA"s,                               "YN"s,    COUNTRY_STATUS::CURRENT } },
  {  88 , { "PANAMA"s,                                  "HP"s,    COUNTRY_STATUS::CURRENT } },
  {  89 , { "TURKS & CAICOS IS."s,                      "VP5"s,   COUNTRY_STATUS::CURRENT } },
  {  90 , { "TRINIDAD & TOBAGO"s,                       "9Y"s,    COUNTRY_STATUS::CURRENT } },
  {  91 , { "ARUBA"s,                                   "P4"s,    COUNTRY_STATUS::CURRENT } },
  {  93 , { "GEYSER REEF"s,                             ""s,      COUNTRY_STATUS::DELETED } },
  {  94 , { "ANTIGUA & BARBUDA"s,                       "V2"s,    COUNTRY_STATUS::CURRENT } },
  {  95 , { "DOMINICA"s,                                "J7"s,    COUNTRY_STATUS::CURRENT } },
  {  96 , { "MONTSERRAT"s,                              "VP2M"s,  COUNTRY_STATUS::CURRENT } },
  {  97 , { "ST. LUCIA"s,                               "J6"s,    COUNTRY_STATUS::CURRENT } },
  {  98 , { "ST. VINCENT"s,                             "J8"s,    COUNTRY_STATUS::CURRENT } },
  {  99 , { "GLORIOSO IS."s,                            "FR/g"s,  COUNTRY_STATUS::CURRENT } },
  {  100, { "ARGENTINA"s,                               "LU"s,    COUNTRY_STATUS::CURRENT } },
  {  101, { "GOA"s,                                     ""s,      COUNTRY_STATUS::DELETED } },
  {  102, { "GOLD COAST, TOGOLAND"s,                    ""s,      COUNTRY_STATUS::DELETED } },
  {  103, { "GUAM"s,                                    "KH2"s,   COUNTRY_STATUS::CURRENT } },
  {  104, { "BOLIVIA"s,                                 "CP"s,    COUNTRY_STATUS::CURRENT } },
  {  105, { "GUANTANAMO BAY"s,                          "KG4"s,   COUNTRY_STATUS::CURRENT } },
  {  106, { "GUERNSEY"s,                                "GU"s,    COUNTRY_STATUS::CURRENT } },
  {  107, { "GUINEA"s,                                  "3X"s,    COUNTRY_STATUS::CURRENT } },
  {  108, { "BRAZIL"s,                                  "PY"s,    COUNTRY_STATUS::CURRENT } },
  {  109, { "GUINEA-BISSAU"s,                           "J5"s,    COUNTRY_STATUS::CURRENT } },
  {  110, { "HAWAII"s,                                  "KH6"s,   COUNTRY_STATUS::CURRENT } },
  {  111, { "HEARD I."s,                                "VK0H"s,  COUNTRY_STATUS::CURRENT } },
  {  112, { "CHILE"s,                                   "CE"s,    COUNTRY_STATUS::CURRENT } },
  {  113, { "IFNI"s,                                    ""s,      COUNTRY_STATUS::DELETED } },
  {  114, { "ISLE OF MAN"s,                             "GD"s,    COUNTRY_STATUS::CURRENT } },
  {  115, { "ITALIAN SOMALILAND"s,                      ""s,      COUNTRY_STATUS::DELETED } },
  {  116, { "COLOMBIA"s,                                "HK"s,    COUNTRY_STATUS::CURRENT } },
  {  117, { "ITU HQ"s,                                  "4U1I"s,  COUNTRY_STATUS::CURRENT } },
  {  118, { "JAN MAYEN"s,                               "JX"s,    COUNTRY_STATUS::CURRENT } },
  {  119, { "JAVA"s,                                    ""s,      COUNTRY_STATUS::DELETED } },
  {  120, { "ECUADOR"s,                                 "HC"s,    COUNTRY_STATUS::CURRENT } },
  {  122, { "JERSEY"s,                                  "GJ"s,    COUNTRY_STATUS::CURRENT } },
  {  123, { "JOHNSTON I."s,                             "KH3"s,   COUNTRY_STATUS::CURRENT } },
  {  124, { "JUAN DE NOVA, EUROPA"s,                    "FR/j"s,  COUNTRY_STATUS::CURRENT } },
  {  125, { "JUAN FERNANDEZ IS."s,                      "CE0Z"s,  COUNTRY_STATUS::CURRENT } },
  {  126, { "KALININGRAD"s,                             "UA2"s,   COUNTRY_STATUS::CURRENT } },
  {  127, { "KAMARAN IS."s,                             ""s,      COUNTRY_STATUS::DELETED } },
  {  128, { "KARELO-FINNISH REPUBLIC"s,                 ""s,      COUNTRY_STATUS::DELETED } },
  {  129, { "GUYANA"s,                                  "8R"s,    COUNTRY_STATUS::CURRENT } },
  {  130, { "KAZAKHSTAN"s,                              "UN"s,    COUNTRY_STATUS::CURRENT } },
  {  131, { "KERGUELEN IS."s,                           "FT5X"s,  COUNTRY_STATUS::CURRENT } },
  {  132, { "PARAGUAY"s,                                "ZP"s,    COUNTRY_STATUS::CURRENT } },
  {  133, { "KERMADEC IS."s,                            "ZL8"s,   COUNTRY_STATUS::CURRENT } },
  {  134, { "KINGMAN REEF"s,                            "KH5K"s,  COUNTRY_STATUS::CURRENT } },
  {  135, { "KYRGYZSTAN"s,                              "EX"s,    COUNTRY_STATUS::CURRENT } },
  {  136, { "PERU"s,                                    "OA"s,    COUNTRY_STATUS::CURRENT } },
  {  137, { "REPUBLIC OF KOREA"s,                       "HK"s,    COUNTRY_STATUS::CURRENT } },
  {  138, { "KURE I."s,                                 "KH7K"s,  COUNTRY_STATUS::CURRENT } },
  {  139, { "KURIA MURIA I."s,                          ""s,      COUNTRY_STATUS::DELETED } },
  {  140, { "SURINAME"s,                                "PZ"s,    COUNTRY_STATUS::CURRENT } },
  {  141, { "FALKLAND IS."s,                            "VP8"s,   COUNTRY_STATUS::CURRENT } },
  {  142, { "LAKSHADWEEP IS."s,                         "VU7"s,   COUNTRY_STATUS::CURRENT } },
  {  143, { "LAOS"s,                                    "XW"s,    COUNTRY_STATUS::CURRENT } },
  {  144, { "URUGUAY"s,                                 "CX"s,    COUNTRY_STATUS::CURRENT } },
  {  145, { "LATVIA"s,                                  "YL"s,    COUNTRY_STATUS::CURRENT } },
  {  146, { "LITHUANIA"s,                               "LY"s,    COUNTRY_STATUS::CURRENT } },
  {  147, { "LORD HOWE I."s,                            "VK9L"s,  COUNTRY_STATUS::CURRENT } },
  {  148, { "VENEZUELA"s,                               "YV"s,    COUNTRY_STATUS::CURRENT } },
  {  149, { "AZORES"s,                                  "CU"s,    COUNTRY_STATUS::CURRENT } },
  {  150, { "AUSTRALIA"s,                               "VK"s,    COUNTRY_STATUS::CURRENT } },
  {  151, { "MALYJ VYSOTSKIJ I."s,                      "R1MV"s,  COUNTRY_STATUS::CURRENT } },
  {  152, { "MACAO"s,                                   "XX9"s,   COUNTRY_STATUS::CURRENT } },
  {  153, { "MACQUARIE I."s,                            "VK0M"s,  COUNTRY_STATUS::CURRENT } },
  {  154, { "YEMEN ARAB REPUBLIC"s,                     ""s,      COUNTRY_STATUS::DELETED } },
  {  155, { "MALAYA"s,                                  ""s,      COUNTRY_STATUS::DELETED } },
  {  157, { "NAURU"s,                                   "C2"s,    COUNTRY_STATUS::CURRENT } },
  {  158, { "VANUATU"s,                                 "YJ"s,    COUNTRY_STATUS::CURRENT } },
  {  159, { "MALDIVES"s,                                "8Q"s,    COUNTRY_STATUS::CURRENT } },
  {  160, { "TONGA"s,                                   "A3"s,    COUNTRY_STATUS::CURRENT } },
  {  161, { "MALPELO I."s,                              "HK0/m"s, COUNTRY_STATUS::CURRENT } },
  {  162, { "NEW CALEDONIA"s,                           "FK"s,    COUNTRY_STATUS::CURRENT } },
  {  163, { "PAPUA NEW GUINEA"s,                        "P2"s,    COUNTRY_STATUS::CURRENT } },
  {  164, { "MANCHURIA"s,                               ""s,      COUNTRY_STATUS::DELETED } },
  {  165, { "MAURITIUS"s,                               "3B8"s,   COUNTRY_STATUS::CURRENT } },
  {  166, { "MARIANA IS."s,                             "KH0",    COUNTRY_STATUS::CURRENT } },
  {  167, { "MARKET REEF"s,                             "OJ0",    COUNTRY_STATUS::CURRENT } },
  {  168, { "MARSHALL IS."s,                            "V7",     COUNTRY_STATUS::CURRENT } },
  {  169, { "MAYOTTE"s,                                 "FH",     COUNTRY_STATUS::CURRENT } },
  {  170, { "NEW ZEALAND"s,                             "ZL",     COUNTRY_STATUS::CURRENT } },
  {  171, { "MELLISH REEF"s,                            "VK9M",   COUNTRY_STATUS::CURRENT } },
  {  172, { "PITCAIRN I."s,                             "VP6",    COUNTRY_STATUS::CURRENT } },
  {  173, { "MICRONESIA"s,                              "V6",     COUNTRY_STATUS::CURRENT } },
  {  174, { "MIDWAY I."s,                               "KH4",    COUNTRY_STATUS::CURRENT } },
  {  175, { "FRENCH POLYNESIA"s,                        "FO",     COUNTRY_STATUS::CURRENT } },
  {  176, { "FIJI"s,                                    "3D",     COUNTRY_STATUS::CURRENT } },
  {  177, { "MINAMI TORISHIMA"s,                        "JD/m",   COUNTRY_STATUS::CURRENT } },
  {  178, { "MINERVA REEF"s,                            ""s,      COUNTRY_STATUS::DELETED } },
  {  179, { "MOLDOVA"s,                                 "ER",     COUNTRY_STATUS::CURRENT } },
  {  180, { "MOUNT ATHOS"s,                             "SV/a",   COUNTRY_STATUS::CURRENT } },
  {  181, { "MOZAMBIQUE"s,                              "C9",     COUNTRY_STATUS::CURRENT } },
  {  182, { "NAVASSA I."s,                              "KP1",    COUNTRY_STATUS::CURRENT } },
  {  183, { "NETHERLANDS BORNEO"s,                      ""s,      COUNTRY_STATUS::DELETED } },
  {  184, { "NETHERLANDS NEW GUINEA"s,                  ""s,      COUNTRY_STATUS::DELETED } },
  {  185, { "SOLOMON IS."s,                             "H4",     COUNTRY_STATUS::CURRENT } },
  {  186, { "NEWFOUNDLAND, LABRADOR"s,                  ""s,      COUNTRY_STATUS::DELETED } },
  {  187, { "NIGER"s,                                   "5U",     COUNTRY_STATUS::CURRENT } },
  {  188, { "NIUE"s,                                    "ZK2",    COUNTRY_STATUS::CURRENT } },
  {  189, { "NORFOLK I."s,                              "VK9N",   COUNTRY_STATUS::CURRENT } },
  {  190, { "SAMOA"s,                                   "E5/n",   COUNTRY_STATUS::CURRENT } },
  {  191, { "NORTH COOK IS."s,                          "E5/n",   COUNTRY_STATUS::CURRENT } },
  {  192, { "OGASAWARA"s,                               "JD/o",   COUNTRY_STATUS::CURRENT } },
  {  193, { "OKINAWA (RYUKYU IS.)"s,                    ""s,      COUNTRY_STATUS::DELETED } },
  {  194, { "OKINO TORI-SHIMA"s,                        ""s,      COUNTRY_STATUS::DELETED } },
  {  195, { "ANNOBON I."s,                              "3C0",    COUNTRY_STATUS::CURRENT } },
  {  196, { "PALESTINE"s,                               ""s,      COUNTRY_STATUS::DELETED } },
  {  197, { "PALMYRA & JARVIS IS."s,                    "KH5",    COUNTRY_STATUS::CURRENT } },
  {  198, { "PAPUA TERRITORY"s,                         ""s,      COUNTRY_STATUS::DELETED } },
  {  199, { "PETER 1 I."s,                              "3Y/p",   COUNTRY_STATUS::CURRENT } },
  {  200, { "PORTUGUESE TIMOR"s,                        ""s,      COUNTRY_STATUS::DELETED } },
  {  201, { "PRINCE EDWARD & MARION IS."s,              "ZS8"s,   COUNTRY_STATUS::CURRENT } },
  {  202, { "PUERTO RICO"s,                             "KP4"s,   COUNTRY_STATUS::CURRENT } },
  {  203, { "ANDORRA"s,                                 "C3"s,    COUNTRY_STATUS::CURRENT } },
  {  204, { "REVILLAGIGEDO"s,                           "XF4"s,   COUNTRY_STATUS::CURRENT } },
  {  205, { "ASCENSION I."s,                            "ZD8"s,   COUNTRY_STATUS::CURRENT } },
  {  206, { "AUSTRIA"s,                                 "OE"s,    COUNTRY_STATUS::CURRENT } },
  {  207, { "RODRIGUEZ I."s,                            "3B9"s,   COUNTRY_STATUS::CURRENT } },
  {  208, { "RUANDA-URUNDI"s,                           ""s,      COUNTRY_STATUS::DELETED } },
  {  209, { "BELGIUM"s,                                 "ON"s,    COUNTRY_STATUS::CURRENT } },
  {  210, { "SAAR"s,                                    ""s,      COUNTRY_STATUS::DELETED } },
  {  211, { "SABLE I."s,                                "CY0"s,   COUNTRY_STATUS::CURRENT } },
  {  212, { "BULGARIA"s,                                "LZ"s,    COUNTRY_STATUS::CURRENT } },
  {  213, { "SAINT MARTIN"s,                            "FS"s,    COUNTRY_STATUS::CURRENT } },
  {  214, { "CORSICA"s,                                 "TK"s,    COUNTRY_STATUS::CURRENT } },
  {  215, { "CYPRUS"s,                                  "5B"s,    COUNTRY_STATUS::CURRENT } },
  {  216, { "SAN ANDRES & PROVIDENCIA"s,                "HK0/a"s, COUNTRY_STATUS::CURRENT } },
  {  217, { "SAN FELIX & SAN AMBROSIO"s,                "CE0X"s,  COUNTRY_STATUS::CURRENT } },
  {  218, { "CZECHOSLOVAKIA"s,                          ""s,      COUNTRY_STATUS::DELETED } },
  {  219, { "SAO TOME & PRINCIPE"s,                     "S9"s,    COUNTRY_STATUS::CURRENT } },
  {  220, { "SARAWAK"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  221, { "DENMARK"s,                                 "OZ"s,    COUNTRY_STATUS::CURRENT } },
  {  222, { "FAROE IS."s,                               "OY"s,    COUNTRY_STATUS::CURRENT } },
  {  223, { "ENGLAND"s,                                 "G"s,     COUNTRY_STATUS::CURRENT } },
  {  224, { "FINLAND"s,                                 "OH"s,    COUNTRY_STATUS::CURRENT } },
  {  225, { "SARDINIA"s,                                "IS"s,    COUNTRY_STATUS::CURRENT } },
  {  226, { "SAUDI ARABIA/IRAQ NEUTRAL ZONE"s,          ""s,      COUNTRY_STATUS::DELETED } },
  {  227, { "FRANCE"s,                                  "F"s,     COUNTRY_STATUS::CURRENT } },
  {  228, { "SERRANA BANK & RONCADOR CAY"s,             ""s,      COUNTRY_STATUS::DELETED } },
  {  229, { "GERMAN DEMOCRATIC REPUBLIC"s,              ""s,      COUNTRY_STATUS::DELETED } },
  {  230, { "FEDERAL REPUBLIC OF GERMANY"s,             "DL"s,    COUNTRY_STATUS::CURRENT } },
  {  231, { "SIKKIM"s,                                  ""s,      COUNTRY_STATUS::DELETED } },
  {  232, { "SOMALIA"s,                                 "T5"s,    COUNTRY_STATUS::CURRENT } },
  {  233, { "GIBRALTAR"s,                               "ZB"s,    COUNTRY_STATUS::CURRENT } },
  {  234, { "SOUTH COOK IS."s,                          "E5/s"s,  COUNTRY_STATUS::CURRENT } },
  {  235, { "SOUTH GEORGIA I."s,                        "VP8/g"s, COUNTRY_STATUS::CURRENT } },
  {  236, { "GREECE"s,                                  "SV"s,    COUNTRY_STATUS::CURRENT } },
  {  237, { "GREENLAND"s,                               "OX"s,    COUNTRY_STATUS::CURRENT } },
  {  238, { "SOUTH ORKNEY IS."s,                        "VP8/o"s, COUNTRY_STATUS::CURRENT } },
  {  239, { "HUNGARY"s,                                 "HA"s,    COUNTRY_STATUS::CURRENT } },
  {  240, { "SOUTH SANDWICH IS."s,                      "VP8/s"s, COUNTRY_STATUS::CURRENT } },
  {  241, { "SOUTH SHETLAND IS."s,                      "VP8/h"s, COUNTRY_STATUS::CURRENT } },
  {  242, { "ICELAND"s,                                 "TF"s,    COUNTRY_STATUS::CURRENT } },
  {  243, { "PEOPLE'S DEMOCRATIC REP. OF YEMEN"s,       ""s,      COUNTRY_STATUS::DELETED } },
  {  244, { "SOUTHERN SUDAN"s,                          ""s,      COUNTRY_STATUS::DELETED } },
  {  245, { "IRELAND"s,                                 "EI"s,    COUNTRY_STATUS::CURRENT } },
  {  246, { "SOVEREIGN MILITARY ORDER OF MALTA"s,       "1A"s,    COUNTRY_STATUS::CURRENT } },
  {  247, { "SPRATLY IS."s,                             "1S"s,    COUNTRY_STATUS::CURRENT } },
  {  248, { "ITALY"s,                                   "I"s,     COUNTRY_STATUS::CURRENT } },
  {  249, { "ST. KITTS & NEVIS"s,                       "V4"s,    COUNTRY_STATUS::CURRENT } },
  {  250, { "ST. HELENA"s,                              "ZD7"s,   COUNTRY_STATUS::CURRENT } },
  {  251, { "LIECHTENSTEIN"s,                           "HB0"s,   COUNTRY_STATUS::CURRENT } },
  {  252, { "ST. PAUL I."s,                             "CY9"s,   COUNTRY_STATUS::CURRENT } },
  {  253, { "ST. PETER & ST. PAUL ROCKS"s,              "PY0S"s,  COUNTRY_STATUS::CURRENT } },
  {  254, { "LUXEMBOURG"s,                              "LX"s,    COUNTRY_STATUS::CURRENT } },
  {  255, { "ST. MAARTEN, SABA, ST. EUSTATIUS"s,        ""s,      COUNTRY_STATUS::DELETED } },
  {  256, { "MADEIRA IS."s,                             "CT3"s,   COUNTRY_STATUS::CURRENT } },
  {  257, { "MALTA"s,                                   "9H"s,    COUNTRY_STATUS::CURRENT } },
  {  258, { "SUMATRA"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  259, { "SVALBARD"s,                                "JW"s,    COUNTRY_STATUS::CURRENT } },
  {  260, { "MONACO"s,                                  "3A"s,    COUNTRY_STATUS::CURRENT } },
  {  261, { "SWAN IS."s,                                ""s,      COUNTRY_STATUS::DELETED } },
  {  262, { "TAJIKISTAN"s,                              "EY"s,    COUNTRY_STATUS::CURRENT } },
  {  263, { "NETHERLANDS"s,                             "PA"s,    COUNTRY_STATUS::CURRENT } },
  {  264, { "TANGIER"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  265, { "NORTHERN IRELAND"s,                        "GI"s,    COUNTRY_STATUS::CURRENT } },
  {  266, { "NORWAY"s,                                  "LA"s,    COUNTRY_STATUS::CURRENT } },
  {  267, { "TERRITORY OF NEW GUINEA"s,                 ""s,      COUNTRY_STATUS::DELETED } },
  {  268, { "TIBET"s,                                   ""s,      COUNTRY_STATUS::DELETED } },
  {  269, { "POLAND"s,                                  "SP"s,    COUNTRY_STATUS::CURRENT } },
  {  270, { "TOKELAU IS."s,                             "ZK3"s,   COUNTRY_STATUS::CURRENT } },
  {  271, { "TRIESTE"s,                                 ""s,      COUNTRY_STATUS::DELETED } },
  {  272, { "PORTUGAL"s,                                "CT"s,    COUNTRY_STATUS::CURRENT } },
  {  273, { "TRINDADE & MARTIM VAZ IS."s,               "PY0T"s,  COUNTRY_STATUS::CURRENT } },
  {  274, { "TRISTAN DA CUNHA & GOUGH I."s,             "ZD9"s,   COUNTRY_STATUS::CURRENT } },
  {  275, { "ROMANIA"s,                                 "YO"s,    COUNTRY_STATUS::CURRENT } },
  {  276, { "TROMELIN I."s,                             "FR/t"s,  COUNTRY_STATUS::CURRENT } },
  {  277, { "ST. PIERRE & MIQUELON"s,                   "FP"s,    COUNTRY_STATUS::CURRENT } },
  {  278, { "SAN MARINO"s,                              "T7"s,    COUNTRY_STATUS::CURRENT } },
  {  279, { "SCOTLAND"s,                                "GM"s,    COUNTRY_STATUS::CURRENT } },
  {  280, { "TURKMENISTAN"s,                            "EZ"s,    COUNTRY_STATUS::CURRENT } },
  {  281, { "SPAIN"s,                                   "EA"s,    COUNTRY_STATUS::CURRENT } },
  {  282, { "TUVALU"s,                                  "T2"s,    COUNTRY_STATUS::CURRENT } },
  {  283, { "UK SOVEREIGN BASE AREAS ON CYPRUS"s,       "ZC4"s,   COUNTRY_STATUS::CURRENT } },
  {  284, { "SWEDEN"s,                                  "SM"s,    COUNTRY_STATUS::CURRENT } },
  {  285, { "VIRGIN IS."s,                              "KP2"s,   COUNTRY_STATUS::CURRENT } },
  {  286, { "UGANDA"s,                                  "5X"s,    COUNTRY_STATUS::CURRENT } },
  {  287, { "SWITZERLAND"s,                             "HB"s,    COUNTRY_STATUS::CURRENT } },
  {  288, { "UKRAINE"s,                                 "UR"s,    COUNTRY_STATUS::CURRENT } },
  {  289, { "UNITED NATIONS HQ"s,                       "4U1U"s,  COUNTRY_STATUS::CURRENT } },
  {  291, { "UNITED STATES OF AMERICA"s,                "K"s,     COUNTRY_STATUS::CURRENT } },
  {  292, { "UZBEKISTAN"s,                              "UK"s,    COUNTRY_STATUS::CURRENT } },
  {  293, { "VIET NAM"s,                                "3W"s,    COUNTRY_STATUS::CURRENT } },
  {  294, { "WALES"s,                                   "GW"s,    COUNTRY_STATUS::CURRENT } },
  {  295, { "VATICAN"s,                                 "HV"s,    COUNTRY_STATUS::CURRENT } },
  {  296, { "SERBIA"s,                                  "YU"s,    COUNTRY_STATUS::CURRENT } },
  {  297, { "WAKE I."s,                                 "KH9"s,   COUNTRY_STATUS::CURRENT } },
  {  298, { "WALLIS & FUTUNA IS."s,                     "FW"s,    COUNTRY_STATUS::CURRENT } },
  {  299, { "WEST MALAYSIA"s,                           "9M2"s,   COUNTRY_STATUS::CURRENT } },
  {  301, { "W. KIRIBATI (GILBERT IS. )"s,              "T30"s,   COUNTRY_STATUS::CURRENT } },
  {  302, { "WESTERN SAHARA"s,                          "S0"s,    COUNTRY_STATUS::CURRENT } },
  {  303, { "WILLIS I."s,                               "VK9W"s,  COUNTRY_STATUS::CURRENT } },
  {  304, { "BAHRAIN"s,                                 "A9"s,    COUNTRY_STATUS::CURRENT } },
  {  305, { "BANGLADESH"s,                              "S2"s,    COUNTRY_STATUS::CURRENT } },
  {  306, { "BHUTAN"s,                                  "A5"s,    COUNTRY_STATUS::CURRENT } },
  {  307, { "ZANZIBAR"s,                                ""s,      COUNTRY_STATUS::DELETED } },
  {  308, { "COSTA RICA"s,                              "TI"s,    COUNTRY_STATUS::CURRENT } },
  {  309, { "MYANMAR"s,                                 "XZ"s,    COUNTRY_STATUS::CURRENT } },
  {  312, { "CAMBODIA"s,                                "XU"s,    COUNTRY_STATUS::CURRENT } },
  {  315, { "SRI LANKA"s,                               "4S"s,    COUNTRY_STATUS::CURRENT } },
  {  318, { "CHINA"s,                                   "BY"s,    COUNTRY_STATUS::CURRENT } },
  {  321, { "HONG KONG"s,                               "VR"s,    COUNTRY_STATUS::CURRENT } },
  {  324, { "INDIA"s,                                   "VU"s,    COUNTRY_STATUS::CURRENT } },
  {  327, { "INDONESIA"s,                               "YB"s,    COUNTRY_STATUS::CURRENT } },
  {  330, { "IRAN"s,                                    "EP"s,    COUNTRY_STATUS::CURRENT } },
  {  333, { "IRAQ"s,                                    "YI"s,    COUNTRY_STATUS::CURRENT } },
  {  336, { "ISRAEL"s,                                  "4X"s,    COUNTRY_STATUS::CURRENT } },
  {  339, { "JAPAN"s,                                   "JA"s,    COUNTRY_STATUS::CURRENT } },
  {  342, { "JORDAN"s,                                  "JY"s,    COUNTRY_STATUS::CURRENT } },
  {  344, { "DEMOCRATIC PEOPLE'S REP. OF KOREA "s,      "HM"s,    COUNTRY_STATUS::CURRENT } },
  {  345, { "BRUNEI DARUSSALAM"s,                       "V8"s,    COUNTRY_STATUS::CURRENT } },
  {  348, { "KUWAIT"s,                                  "9K"s,    COUNTRY_STATUS::CURRENT } },
  {  354, { "LEBANON"s,                                 "OD"s,    COUNTRY_STATUS::CURRENT } },
  {  363, { "MONGOLIA"s,                                "JT"s,    COUNTRY_STATUS::CURRENT } },
  {  369, { "NEPAL"s,                                   "9N"s,    COUNTRY_STATUS::CURRENT } },
  {  370, { "OMAN"s,                                    "A4"s,    COUNTRY_STATUS::CURRENT } },
  {  372, { "PAKISTAN"s,                                "AP"s,    COUNTRY_STATUS::CURRENT } },
  {  375, { "PHILIPPINES"s,                             "DU"s,    COUNTRY_STATUS::CURRENT } },
  {  376, { "QATAR"s,                                   "A7"s,    COUNTRY_STATUS::CURRENT } },
  {  378, { "SAUDI ARABIA"s,                            "HZ"s,    COUNTRY_STATUS::CURRENT } },
  {  379, { "SEYCHELLES"s,                              "S7"s,    COUNTRY_STATUS::CURRENT } },
  {  381, { "SINGAPORE"s,                               "9V"s,    COUNTRY_STATUS::CURRENT } },
  {  382, { "DJIBOUTI"s,                                "J2"s,    COUNTRY_STATUS::CURRENT } },
  {  384, { "SYRIA"s,                                   "YK"s,    COUNTRY_STATUS::CURRENT } },
  {  386, { "TAIWAN"s,                                  "BV"s,    COUNTRY_STATUS::CURRENT } },
  {  387, { "THAILAND"s,                                "HS"s,    COUNTRY_STATUS::CURRENT } },
  {  390, { "TURKEY"s,                                  "TA"s,    COUNTRY_STATUS::CURRENT } },
  {  391, { "UNITED ARAB EMIRATES"s,                    "A6"s,    COUNTRY_STATUS::CURRENT } },
  {  400, { "ALGERIA"s,                                 "7X"s,    COUNTRY_STATUS::CURRENT } },
  {  401, { "ANGOLA"s,                                  "D2"s,    COUNTRY_STATUS::CURRENT } },
  {  402, { "BOTSWANA"s,                                "A2"s,    COUNTRY_STATUS::CURRENT } },
  {  404, { "BURUNDI"s,                                 "9U"s,    COUNTRY_STATUS::CURRENT } },
  {  406, { "CAMEROON"s,                                "TJ"s,    COUNTRY_STATUS::CURRENT } },
  {  408, { "CENTRAL AFRICA"s,                          "TL"s,    COUNTRY_STATUS::CURRENT } },
  {  409, { "CAPE VERDE"s,                              "D4"s,    COUNTRY_STATUS::CURRENT } },
  {  410, { "CHAD"s,                                    "TT"s,    COUNTRY_STATUS::CURRENT } },
  {  411, { "COMOROS"s,                                 "D6"s,    COUNTRY_STATUS::CURRENT } },
  {  412, { "REPUBLIC OF THE CONGO"s,                   "9Q"s,    COUNTRY_STATUS::CURRENT } },
  {  414, { "DEMOCRATIC REPUBLIC OF THE CONGO"s,        "TN"s,    COUNTRY_STATUS::CURRENT } },
  {  416, { "BENIN"s,                                   "TY"s,    COUNTRY_STATUS::CURRENT } },
  {  420, { "GABON"s,                                   "TR"s,    COUNTRY_STATUS::CURRENT } },
  {  422, { "THE GAMBIA"s,                              "C5"s,    COUNTRY_STATUS::CURRENT } },
  {  424, { "GHANA"s,                                   "9G"s,    COUNTRY_STATUS::CURRENT } },
  {  428, { "COTE D'IVOIRE"s,                           "TU"s,    COUNTRY_STATUS::CURRENT } },
  {  430, { "KENYA"s,                                   "5Z"s,    COUNTRY_STATUS::CURRENT } },
  {  432, { "LESOTHO"s,                                 "7P"s,    COUNTRY_STATUS::CURRENT } },
  {  434, { "LIBERIA"s,                                 "EL"s,    COUNTRY_STATUS::CURRENT } },
  {  436, { "LIBYA"s,                                   "5A"s,    COUNTRY_STATUS::CURRENT } },
  {  438, { "MADAGASCAR"s,                              "5R"s,    COUNTRY_STATUS::CURRENT } },
  {  440, { "MALAWI"s,                                  "7Q"s,    COUNTRY_STATUS::CURRENT } },
  {  442, { "MALI"s,                                    "TZ"s,    COUNTRY_STATUS::CURRENT } },
  {  444, { "MAURITANIA"s,                              "5T"s,    COUNTRY_STATUS::CURRENT } },
  {  446, { "MOROCCO"s,                                 "CN"s,    COUNTRY_STATUS::CURRENT } },
  {  450, { "NIGERIA"s,                                 "5N"s,    COUNTRY_STATUS::CURRENT } },
  {  452, { "ZIMBABWE"s,                                "Z2"s,    COUNTRY_STATUS::CURRENT } },
  {  453, { "REUNION I."s,                              "FR"s,    COUNTRY_STATUS::CURRENT } },
  {  454, { "RWANDA"s,                                  "9X"s,    COUNTRY_STATUS::CURRENT } },
  {  456, { "SENEGAL"s,                                 "6W"s,    COUNTRY_STATUS::CURRENT } },
  {  458, { "SIERRA LEONE"s,                            "9L"s,    COUNTRY_STATUS::CURRENT } },
  {  460, { "ROTUMA I."s,                               "3D2/r"s, COUNTRY_STATUS::CURRENT } },
  {  462, { "SOUTH AFRICA"s,                            "ZS"s,    COUNTRY_STATUS::CURRENT } },
  {  464, { "NAMIBIA"s,                                 "V5"s,    COUNTRY_STATUS::CURRENT } },
  {  466, { "SUDAN"s,                                   "ST"s,    COUNTRY_STATUS::CURRENT } },
  {  468, { "SWAZILAND"s,                               "3DA"s,   COUNTRY_STATUS::CURRENT } },
  {  470, { "TANZANIA"s,                                "5H"s,    COUNTRY_STATUS::CURRENT } },
  {  474, { "TUNISIA"s,                                 "3V"s,    COUNTRY_STATUS::CURRENT } },
  {  478, { "EGYPT"s,                                   "SU"s,    COUNTRY_STATUS::CURRENT } },
  {  480, { "BURKINA FASO"s,                            "XT"s,    COUNTRY_STATUS::CURRENT } },
  {  482, { "ZAMBIA"s,                                  "9J"s,    COUNTRY_STATUS::CURRENT } },
  {  483, { "TOGO"s,                                    "5V"s,    COUNTRY_STATUS::CURRENT } },
  {  488, { "WALVIS BAY"s,                              ""s,      COUNTRY_STATUS::DELETED } },
  {  489, { "CONWAY REEF"s,                             "3D2/c"s, COUNTRY_STATUS::CURRENT } },
  {  490, { "BANABA I. (OCEAN I.)"s,                    "T33"s,   COUNTRY_STATUS::CURRENT } },
  {  492, { "YEMEN"s,                                   "7O"s,    COUNTRY_STATUS::CURRENT } },
  {  493, { "PENGUIN IS."s,                             ""s,      COUNTRY_STATUS::DELETED } },
  {  497, { "CROATIA"s,                                 "9A"s,    COUNTRY_STATUS::CURRENT } },
  {  499, { "SLOVENIA"s,                                "S5"s,    COUNTRY_STATUS::CURRENT } },
  {  501, { "BOSNIA-HERZEGOVINA"s,                      "E7"s,    COUNTRY_STATUS::CURRENT } },
  {  502, { "MACEDONIA"s,                               "Z3"s,    COUNTRY_STATUS::CURRENT } },
  {  503, { "CZECH REPUBLIC"s,                          "OK"s,    COUNTRY_STATUS::CURRENT } },
  {  504, { "SLOVAK REPUBLIC"s,                         "OM"s,    COUNTRY_STATUS::CURRENT } },
  {  505, { "PRATAS I."s,                               "BV9P"s,  COUNTRY_STATUS::CURRENT } },
  {  506, { "SCARBOROUGH REEF"s,                        "BS7"s,   COUNTRY_STATUS::CURRENT } },
  {  507, { "TEMOTU PROVINCE"s,                         "H40"s,   COUNTRY_STATUS::CURRENT } },
  {  508, { "AUSTRAL I."s,                              "FO/a"s,  COUNTRY_STATUS::CURRENT } },
  {  509, { "MARQUESAS IS."s,                           "FO/m"s,  COUNTRY_STATUS::CURRENT } },
  {  510, { "PALESTINE"s,                               "E4"s,    COUNTRY_STATUS::CURRENT } },
  {  511, { "TIMOR-LESTE"s,                             "4W"s,    COUNTRY_STATUS::CURRENT } },
  {  512, { "CHESTERFIELD IS."s,                        "FK/c"s,  COUNTRY_STATUS::CURRENT } },
  {  513, { "DUCIE I."s,                                "VP6/d"s, COUNTRY_STATUS::CURRENT } },
  {  514, { "MONTENEGRO"s,                              "4O"s,    COUNTRY_STATUS::CURRENT } },
  {  515, { "SWAINS I."s,                               "KH8/s"s, COUNTRY_STATUS::CURRENT } },
  {  516, { "SAINT BARTHELEMY"s,                        "FJ"s,    COUNTRY_STATUS::CURRENT } },
  {  517, { "CURACAO"s,                                 "PJ2"s,   COUNTRY_STATUS::CURRENT } },
  {  518, { "ST MAARTEN"s,                              "PJ7"s,   COUNTRY_STATUS::CURRENT } },
  {  519, { "SABA & ST. EUSTATIUS"s,                    "PJ5"s,   COUNTRY_STATUS::CURRENT } },
  {  520, { "BONAIRE"s,                                 "PJ4"s,   COUNTRY_STATUS::CURRENT } },
  {  521, { "SOUTH SUDAN (REPUBLIC OF)"s,               "Z8"s,    COUNTRY_STATUS::CURRENT } },
  {  522, { "REPUBLIC OF KOSOVO"s,                      "Z6",     COUNTRY_STATUS::CURRENT } }
};

/// mode values
unordered_set<string> adif3_field::_ENUMERATION_MODE
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
set<string> adif3_field::_ENUMERATION_QSL_RECEIVED { "Y"s, "N"s, "R"s, "I"s, "V"s };

/// fields that are not to be output
set<ADIF3_DATA_TYPE> adif3_record::_import_only
{ ADIF3_DATA_TYPE::AWARD_LIST };
