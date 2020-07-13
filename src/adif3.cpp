#include "adif3.h"

#include <algorithm>
#include <iostream>

using namespace std;

/*! \brief  place values into standardised forms as necessary

    Assumes that names are in upper case
*/
void adif3_field::_normalise(void)
{ static const string      zero_zero { "00"s };
  static const set<string> uc_fields { "CALL"s, "MODE"s, "STATION_CALLSIGN"s };   ///< fields that have UC values
  
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
  if (uc_fields > _name)
  { _value = to_upper(_value);
    return;
  }
}

/// verify that values are legal
void adif3_field::_verify(void) const
{ switch (_type)
  { case ADIF3_DATA_TYPE::DATE :                                      // YYYYMMDD
    { if (_value.find_first_not_of("01234567879"s) != string::npos)
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
        
      if (_value.length() != 8)
        throw adif3_error(ADIF3_INVALID_LENGTH, "Invalid length in "s + _name + ": "s + _value);
        
      const string year { _value.substr(0, 4) };
      
      if (year < "1930"s)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid year in "s + _name + ": "s + _value);
        
      const int month { from_string<int>(_value.substr(4, 2)) };
      
      if ( (month < 1) or (month > 12) )
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid month in "s + _name + ": "s + _value);

      const int day { from_string<int>(_value.substr(6, 2)) };
      
      int days_in_month { 31 };
      
      if ( (month == 4) or (month == 6) or (month == 9) or (month == 11) )
        days_in_month = 30;
          
      if (month == 2)
      { days_in_month = 29;
        if (from_string<int>(year) % 4)
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

    case ADIF3_DATA_TYPE::ENUMERATION_DXCC_ENTITY_CODE :  // first treat as an integer
    { if (_value.find_first_not_of("01234567879"s) != string::npos)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid character in "s + _name + ": "s + _value);

      if (const int code { from_string<int>(_value) }; _ENUMERATION_DXCC_ENTITY_CODE.find(code) == _ENUMERATION_DXCC_ENTITY_CODE.cend())
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

    case ADIF3_DATA_TYPE::NUMBER :                               // a sequence of one or more Digits representing a decimal number, optionally preceded by a minus sign (ASCII code 45) and optionally including a single decimal point  (ASCII  code 46) 
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
    { if (_value.find_first_not_of("01234567879"s) != string::npos)
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);

// CQZ : 1..40
      if (const auto cit = _positive_integer_range.find(_name); cit != _positive_integer_range.cend())
      { const int& min_value { cit->second.first };
        const int& max_value { cit->second.second };   
        
        if (const int zone { from_string<int>(_value) }; ((zone < min_value) or (zone > max_value)) )
          throw adif3_error(ADIF3_INVALID_VALUE, "Invalid value in "s + _name + ": "s + _value);
      }
    }
    break; 

    case ADIF3_DATA_TYPE::STRING :        // amazingly (but nothing about this should amaze me), a "sequence of": ASCII character whose code lies in the range of 32 through 126, inclusive
    { for (const char c : _value)
        if ( (static_cast<int>(c) < 32) or (static_cast<int>(c)  > 126) )
          throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
    }
    break;

    case ADIF3_DATA_TYPE::TIME :          // HHMMSS or HHMM
    { const string& utc { _value };
      
      if (utc.find_first_not_of("01234567879"s) != string::npos)
        throw adif3_error(ADIF3_INVALID_CHARACTER, "Invalid character in "s + _name + ": "s + _value);
 
      if ( (utc.length() != 4) and (utc.length() != 6) )
        throw adif3_error(ADIF3_INVALID_LENGTH, "Invalid length in "s + _name + ": "s + _value);
 
      if (const int hours { from_string<int>(utc.substr(0, 2)) }; hours > 23)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid hours value in "s + _name + ": "s + _value);

      if (const int minutes { from_string<int>(utc.substr(2, 2)) }; minutes > 59)
        throw adif3_error(ADIF3_INVALID_VALUE, "Invalid minutes value in "s + _name + ": "s + _value);

      if (utc.length() == 6)
      { if (const int seconds { from_string<int>(utc.substr(4, 2)) }; seconds > 59)
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
{ _name = (to_upper(field_name));
  
  const auto it = _element_type.find(_name);

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
  const string         descriptor_str { str.substr(posn_1 + 1, posn_2 - posn_1 -1) };
  const vector<string> fields         { split_string(descriptor_str, ":"s) };

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
const int adif3_record::_fast_string_to_int(const string& str) const
{ static constexpr int zerochar { '0' };

  int rv { 0 };

  for (const char c : str)
    rv = (rv * 10) + ((int)c - zerochar);

  return rv; 
}

/*! \brief              Import record from string, and return location past the end of the used part of the string
    \param  str         string from which to read
    \param  posn        position in <i>str</i> at which to start parsing
    \return             one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/ 
size_t adif3_record::import_and_eat(const std::string& str, const size_t posn)
{ 
// extract the text from the first "<" to the end of the first "eor"
  const auto posn_1 { str.find('<', posn) };

  if (posn_1 == string::npos)        // could not find initial delimiter
    return string::npos;
    
  const auto posn_2 { case_insensitive_find(str, "<EOR>"s, posn)  + 4 };  // 4 = length("<EOR>") - 1; posn_2 points to last char: ">"
    
  if (posn_2 == string::npos)        // could not find end-of-record marker
    return string::npos;
  
  const size_t rv { ( (posn_2 + 1) >= str.length() ? string::npos : posn_2 + 1) };
  
  size_t start_posn { posn_1 };
  
  while (start_posn <= posn_2)
  { adif3_field element;

    start_posn = element.import_and_eat(str, start_posn, posn_2);             // name is forced to upper case
    
    if ( auto [it, inserted] = _elements.insert( { element.name(), element } ); !inserted)     // should always be inserted
      throw adif3_error(ADIF3_DUPLICATE_FIELD, "Duplicated field name: "s  + element.name());
  }
  
  return rv;
}

/*! \brief      Convert to printable string
    \return     the canonical textual representation of the record

    Returns just the end-of-record marker is the record is empty.
    Does not output import-only fields.
*/
const string adif3_record::to_string(void) const
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

/*! \brief      Return the value of a field
    \param  str name of the field whose value is to be returned
    \return     the value of the field <i>str</i>

    Returns the empty string if the field <i>str</i> does not exist in the record 
*/  
const string adif3_record::value(const string& str) const               // str is field name
{ //return (_elements > to_upper(str)).second;

  const string uc { to_upper(str) };

  const auto cit { _elements.find( to_upper(str) ) };     // name is always stored in upper case

  return ( (cit == _elements.cend()) ? string() : cit->second.value() );
}

/*! \brief                  Set the value of a field (which does not have to be extant in the record)
    \param  field_name      name of the field whose value is to be set
    \param  field_value     value to be set
    \return                 whether this was a new field

    <i>field_name</i> is converted to upper case when stored as <i>_name</i>
    <i>field_value</i> is validated and converted to standardised format (if applicable)
*/
const bool adif3_record::value(const string& field_name, const string& field_value)
{ const adif3_field element        { field_name, field_value };
  const auto        [it, inserted] { _elements.insert( { element.name(), element } ) };
 
  return inserted;
}

/*! \brief         Is one ADIF3 record chronologically earlier than another
    \param  rec1   first record
    \param  rec2   second record
    \return        whether <i>record1</i> is chronologically before <i>record2</i> 
*/
const bool compare_adif3_records(const adif3_record& rec1, const adif3_record& rec2)
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
      _map_data.insert( { rec.callsign(), rec } );
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
    { *this = move(adif3_file(this_path + "/"s + filename));
      return;
    }

    catch (...)
    {
    }
  }
}

#if 0
const adif3_record adif3_file::get_record(const adif3_record& rec) const
{ const auto [begin_it, end_it] = _map_data.equal_range(rec.callsign());

  for (auto it = begin_it; it != end_it; ++it)
  { const adif3_record& this_rec { it->second };
    
    const bool found_it { ( (this_rec.callsign() == rec.callsign()) and
                            (this_rec.date() == rec.date()) and
                            (this_rec.time() == rec.time()) and
                            (this_rec.band() == rec.band()) and
                            (this_rec.mode() == rec.mode()) ) };

    if (found_it)
      return this_rec;
  }
  
  return adif3_record();        // return empty record
}
#endif

/*! \brief                  Return all the QSOs that match a call, band and mode
    \param      callsign    call to match
    \param      b           ADIF3 band to match
    \param      m           ADIF3 mode to match
    \return     filename    Return all the QSO records that match <i>callsign</i>, <i>b</i> and <i>m</i>
*/
const std::vector<adif3_record> adif3_file::matching_qsos(const string& callsign, const string& b, const string& m) const
{ vector<adif3_record> rv;

  const auto [begin_it, end_it] { _map_data.equal_range(callsign) };
  
  for_each(begin_it, end_it, [=, &rv](const auto& map_entry) 
    { if ( (map_entry.second.band() == b) and (map_entry.second.mode() == m) ) 
        rv.push_back(map_entry.second); 
    });
  
  return rv;
}

/*! \brief                  Return all the QSOs that match a call
    \param      callsign    call to match
    \return     filename    Return all the QSO records that match <i>callsign</i>
*/
const std::vector<adif3_record> adif3_file::matching_qsos(const string& callsign) const
{ vector<adif3_record> rv;

  const auto [begin_it, end_it] { _map_data.equal_range(callsign) };
  
  for_each(begin_it, end_it, [=, &rv](const auto& map_entry) { rv.push_back(map_entry.second); });
  
  return rv;
}

/*! \brief              Return position at which to start processing the body of the file
    \param      str     string of contents of filae
    \return             position of first "<" after the end of the header
*/
const size_t skip_adif3_header(const std::string& str)
{ const auto posn_1 { case_insensitive_find(str, "<EOH>"s) };

  return ( (posn_1 == string::npos) ? 0 : (str.find("<"s, posn_1 + 1)) ); // either start of file or first "<" after "<EOH>"
}

/// map from field name to type -- too many of these are silly for it to be worth making comment on individual sillinesses
unordered_map<string, ADIF3_DATA_TYPE> adif3_field::_element_type
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
unordered_set<string> adif3_field::_ENUMERATION_BAND
{ "2190m"s, "630m"s, "560m"s, "160m"s, "80m"s,    "60m"s, "40m"s,   "30m"s,   "20m"s,  "17m"s,
  "15m"s,   "12m"s,  "10m"s,  "6m"s,   "4m"s,     "2m"s,  "1.25m"s, "70cm"s,  "33cm"s, "23cm"s,
  "13cm"s,  "9cm"s,  "6cm"s,  "3cm"s,  "1.25cm"s, "6mm"s, "4mm"s,   "2.5mm"s, "2mm"s,  "1mm"s
};

/// mapping between country code and country info
unordered_map<int /* country number */, tuple<string /*country name */, string /* canonical prefix */, bool /* whether deleted */>> adif3_field::_ENUMERATION_DXCC_ENTITY_CODE
{ {  1  , { "CANADA"s,                                  "VE"s,    false } },
  {  2  , { "ABU AIL IS."s,                             ""s,      true  } },
  {  3  , { "AFGHANISTAN"s,                             "YA"s,    false } },
  {  4  , { "AGALEGA & ST. BRANDON IS."s,               "3B6"s,   false } },
  {  5  , { "ALAND IS."s,                               "OH0"s,   false } },
  {  6  , { "ALASKA"s,                                  "KL"s,    false } },
  {  7  , { "ALBANIA"s,                                 "ZA"s,    false } },
  {  8  , { "ALDABRA"s,                                 ""s,      true  } },
  {  9  , { "AMERICAN SAMOA"s,                          "KH8"s,   false } },
  {  10 , { "AMSTERDAM & ST. PAUL IS."s,                "FT5Z"s,  false } },
  {  11 , { "ANDAMAN & NICOBAR IS."s,                   "VU4"s,   false } },
  {  12 , { "ANGUILLA"s,                                "VP2E"s,  false } },
  {  13 , { "ANTARCTICA"s,                              "CE9"s,   false } },
  {  14 , { "ARMENIA"s,                                 "EK"s,    false } },
  {  15 , { "ASIATIC RUSSIA"s,                          "UA9"s,   false } },
  {  16 , { "NEW ZEALAND SUBANTARCTIC ISLANDS"s,        "ZL9"s,   false } },
  {  17 , { "AVES I."s,                                 "YV0"s,   false } },
  {  18 , { "AZERBAIJAN"s,                              "4J"s,    false } },
  {  19 , { "BAJO NUEVO"s,                              ""s,      true  } },
  {  20 , { "BAKER & HOWLAND IS."s,                     "KH1"s,   false } },
  {  21 , { "BALEARIC IS."s,                            "EA6"s,   false } },
  {  22 , { "PALAU"s,                                   "T8"s,    false } },
  {  23 , { "BLENHEIM REEF"s,                           ""s,      true  } },
  {  24 , { "BOUVET"s,                                  "3Y/b"s,  false } },
  {  25 , { "BRITISH NORTH BORNEO"s,                    ""s,      true  } },
  {  26 , { "BRITISH SOMALILAND"s,                      ""s,      true  } },
  {  27 , { "BELARUS"s,                                 "EU"s,    false } },
  {  28 , { "CANAL ZONE"s,                              ""s,      true  } },
  {  29 , { "CANARY IS."s,                              "EA8"s,   false } },
  {  30 , { "CELEBE & MOLUCCA IS."s,                    ""s,      true  } },
  {  31 , { "C. KIRIBATI (BRITISH PHOENIX IS.)"s,       "EA9"s,   false } },
  {  32 , { "CEUTA & MELILLA"s,                         "T31"s,   false } },
  {  33 , { "CHAGOS IS."s,                              "VQ9"s,   false } },
  {  34 , { "CHATHAM IS."s,                             "ZL7"s,   false } },
  {  35 , { "CHRISTMAS I."s,                            "VK9X"s,  false } },
  {  36 , { "CLIPPERTON I."s,                           "FO/c"s,  false } },
  {  37 , { "COCOS I."s,                                "TI9"s,   false } },
  {  38 , { "COCOS (KEELING) IS."s,                     "VK9C"s,  false } },
  {  39 , { "COMOROS"s,                                 ""s,      true  } },
  {  40 , { "CRETE"s,                                   "SV9"s,   false } },
  {  41 , { "CROZET I."s,                               "FT5W"s,  false } },
  {  42 , { "DAMAO, DIU"s,                              ""s,      true  } },
  {  43 , { "DESECHEO I."s,                             "KP5"s,   false } },
  {  44 , { "DESROCHES"s,                               ""s,      true  } },
  {  45 , { "DODECANESE"s,                              "SV5"s,   false } },
  {  46 , { "EAST MALAYSIA"s,                           "9M6"s,   false } },
  {  47 , { "EASTER I."s,                               "CE0Y"s,  false } },
  {  48 , { "E. KIRIBATI (LINE IS.)"s,                  "T32"s,   false } },
  {  49 , { "EQUATORIAL GUINEA"s,                       "3C"s,    false } },
  {  50 , { "MEXICO"s,                                  "XE"s,    false } },
  {  51 , { "ERITREA"s,                                 "E3"s,    false } },
  {  52 , { "ESTONIA"s,                                 "ES"s,    false } },
  {  53 , { "ETHIOPIA"s,                                "ET"s,    false } },
  {  54 , { "EUROPEAN RUSSIA"s,                         "UA"s,    false } },
  {  55 , { "FARQUHAR"s,                                ""s,      true  } },
  {  56 , { "FERNANDO DE NORONHA"s,                     "PY0F"s,  false } },
  {  57 , { "FRENCH EQUATORIAL AFRICA"s,                ""s,      true  } },
  {  58 , { "FRENCH INDO-CHINA"s,                       ""s,      true  } },
  {  59 , { "FRENCH WEST AFRICA"s,                      ""s,      true  } },
  {  60 , { "BAHAMAS"s,                                 "C6"s,    false } },
  {  61 , { "FRANZ JOSEF LAND"s,                        "R1FJ"s,  false } },
  {  62 , { "BARBADOS"s,                                "8P"s,    false } },
  {  63 , { "FRENCH GUIANA"s,                           "FY"s,    false } },
  {  64 , { "BERMUDA"s,                                 "VP9"s,   false } },
  {  65 , { "BRITISH VIRGIN IS."s,                      "VP2V"s,  false } },
  {  66 , { "BELIZE"s,                                  "V3"s,    false } },
  {  67 , { "FRENCH INDIA"s,                            ""s,      true  } },
  {  68 , { "KUWAIT/SAUDI ARABIA NEUTRAL ZONE"s,        ""s,      true  } },
  {  69 , { "CAYMAN IS."s,                              "ZF"s,    false } },
  {  70 , { "CUBA"s,                                    "CM"s,    false } },
  {  71 , { "GALAPAGOS IS."s,                           "HC8"s,   false } },
  {  72 , { "DOMINICAN REPUBLIC"s,                      "HI"s,    false } },
  {  74 , { "EL SALVADOR"s,                             "YS"s,    false } },
  {  75 , { "GEORGIA"s,                                 "4L"s,    false } },
  {  76 , { "GUATEMALA"s,                               "TG"s,    false } },
  {  77 , { "GRENADA"s,                                 "J3"s,    false } },
  {  78 , { "HAITI"s,                                   "HH"s,    false } },
  {  79 , { "GUADELOUPE"s,                              "FG"s,    false } },
  {  80 , { "HONDURAS"s,                                "HR"s,    false } },
  {  81 , { "GERMANY"s,                                 ""s,      true  } },
  {  82 , { "JAMAICA"s,                                 "6Y"s,    false } },
  {  84 , { "MARTINIQUE"s,                              "FM"s,    false } },
  {  85 , { "BONAIRE, CURACAO"s,                        ""s,      true  } },
  {  86 , { "NICARAGUA"s,                               "YN"s,    false } },
  {  88 , { "PANAMA"s,                                  "HP"s,    false } },
  {  89 , { "TURKS & CAICOS IS."s,                      "VP5"s,   false } },
  {  90 , { "TRINIDAD & TOBAGO"s,                       "9Y"s,    false } },
  {  91 , { "ARUBA"s,                                   "P4"s,    false } },
  {  93 , { "GEYSER REEF"s,                             ""s,      true  } },
  {  94 , { "ANTIGUA & BARBUDA"s,                       "V2"s,    false } },
  {  95 , { "DOMINICA"s,                                "J7"s,    false } },
  {  96 , { "MONTSERRAT"s,                              "VP2M"s,  false } },
  {  97 , { "ST. LUCIA"s,                               "J6"s,    false } },
  {  98 , { "ST. VINCENT"s,                             "J8"s,    false } },
  {  99 , { "GLORIOSO IS."s,                            "FR/g"s,  false } },
  {  100, { "ARGENTINA"s,                               "LU"s,    false } },
  {  101, { "GOA"s,                                     ""s,      true  } },
  {  102, { "GOLD COAST, TOGOLAND"s,                    ""s,      true  } },
  {  103, { "GUAM"s,                                    "KH2"s,   false } },
  {  104, { "BOLIVIA"s,                                 "CP"s,    false } },
  {  105, { "GUANTANAMO BAY"s,                          "KG4"s,   false } },
  {  106, { "GUERNSEY"s,                                "GU"s,    false } },
  {  107, { "GUINEA"s,                                  "3X"s,    false } },
  {  108, { "BRAZIL"s,                                  "PY"s,    false } },
  {  109, { "GUINEA-BISSAU"s,                           "J5"s,    false } },
  {  110, { "HAWAII"s,                                  "KH6"s,   false } },
  {  111, { "HEARD I."s,                                "VK0H"s,  false } },
  {  112, { "CHILE"s,                                   "CE"s,    false } },
  {  113, { "IFNI"s,                                    ""s,      true  } },
  {  114, { "ISLE OF MAN"s,                             "GD"s,    false } },
  {  115, { "ITALIAN SOMALILAND"s,                      ""s,      true  } },
  {  116, { "COLOMBIA"s,                                "HK"s,    false } },
  {  117, { "ITU HQ"s,                                  "4U1I"s,  false } },
  {  118, { "JAN MAYEN"s,                               "JX"s,    false } },
  {  119, { "JAVA"s,                                    ""s,      true  } },
  {  120, { "ECUADOR"s,                                 "HC"s,    false } },
  {  122, { "JERSEY"s,                                  "GJ"s,    false } },
  {  123, { "JOHNSTON I."s,                             "KH3"s,   false } },
  {  124, { "JUAN DE NOVA, EUROPA"s,                    "FR/j"s,  false } },
  {  125, { "JUAN FERNANDEZ IS."s,                      "CE0Z"s,  false } },
  {  126, { "KALININGRAD"s,                             "UA2"s,   false } },
  {  127, { "KAMARAN IS."s,                             ""s,      true  } },
  {  128, { "KARELO-FINNISH REPUBLIC"s,                 ""s,      true  } },
  {  129, { "GUYANA"s,                                  "8R"s,    false } },
  {  130, { "KAZAKHSTAN"s,                              "UN"s,    false } },
  {  131, { "KERGUELEN IS."s,                           "FT5X"s,  false } },
  {  132, { "PARAGUAY"s,                                "ZP"s,    false } },
  {  133, { "KERMADEC IS."s,                            "ZL8"s,   false } },
  {  134, { "KINGMAN REEF"s,                            "KH5K"s,  false } },
  {  135, { "KYRGYZSTAN"s,                              "EX"s,    false } },
  {  136, { "PERU"s,                                    "OA"s,    false } },
  {  137, { "REPUBLIC OF KOREA"s,                       "HK"s,    false } },
  {  138, { "KURE I."s,                                 "KH7K"s,  false } },
  {  139, { "KURIA MURIA I."s,                          ""s,      true  } },
  {  140, { "SURINAME"s,                                "PZ"s,    false } },
  {  141, { "FALKLAND IS."s,                            "VP8"s,   false } },
  {  142, { "LAKSHADWEEP IS."s,                         "VU7"s,   false } },
  {  143, { "LAOS"s,                                    "XW"s,    false } },
  {  144, { "URUGUAY"s,                                 "CX"s,    false } },
  {  145, { "LATVIA"s,                                  "YL"s,    false } },
  {  146, { "LITHUANIA"s,                               "LY"s,    false } },
  {  147, { "LORD HOWE I."s,                            "VK9L"s,  false } },
  {  148, { "VENEZUELA"s,                               "YV"s,    false } },
  {  149, { "AZORES"s,                                  "CU"s,    false } },
  {  150, { "AUSTRALIA"s,                               "VK"s,    false } },
  {  151, { "MALYJ VYSOTSKIJ I."s,                      "R1MV"s,  false } },
  {  152, { "MACAO"s,                                   "XX9"s,   false } },
  {  153, { "MACQUARIE I."s,                            "VK0M"s,  false } },
  {  154, { "YEMEN ARAB REPUBLIC"s,                     ""s,      true  } },
  {  155, { "MALAYA"s,                                  ""s,      true  } },
  {  157, { "NAURU"s,                                   "C2"s,    false } },
  {  158, { "VANUATU"s,                                 "YJ"s,    false } },
  {  159, { "MALDIVES"s,                                "8Q"s,    false } },
  {  160, { "TONGA"s,                                   "A3"s,    false } },
  {  161, { "MALPELO I."s,                              "HK0/m"s, false } },
  {  162, { "NEW CALEDONIA"s,                           "FK"s,    false } },
  {  163, { "PAPUA NEW GUINEA"s,                        "P2"s,    false } },
  {  164, { "MANCHURIA"s,                               ""s,      true  } },
  {  165, { "MAURITIUS"s,                               "3B8"s,   false } },
  {  166, { "MARIANA IS."s,                             "KH0",    false } },
  {  167, { "MARKET REEF"s,                             "OJ0",    false } },
  {  168, { "MARSHALL IS."s,                            "V7",     false } },
  {  169, { "MAYOTTE"s,                                 "FH",     false } },
  {  170, { "NEW ZEALAND"s,                             "ZL",     false } },
  {  171, { "MELLISH REEF"s,                            "VK9M",   false } },
  {  172, { "PITCAIRN I."s,                             "VP6",    false } },
  {  173, { "MICRONESIA"s,                              "V6",     false } },
  {  174, { "MIDWAY I."s,                               "KH4",    false } },
  {  175, { "FRENCH POLYNESIA"s,                        "FO",     false } },
  {  176, { "FIJI"s,                                    "3D",     false } },
  {  177, { "MINAMI TORISHIMA"s,                        "JD/m",   false } },
  {  178, { "MINERVA REEF"s,                            ""s,      true  } },
  {  179, { "MOLDOVA"s,                                 "ER",     false } },
  {  180, { "MOUNT ATHOS"s,                             "SV/a",   false } },
  {  181, { "MOZAMBIQUE"s,                              "C9",     false } },
  {  182, { "NAVASSA I."s,                              "KP1",    false } },
  {  183, { "NETHERLANDS BORNEO"s,                      ""s,      true  } },
  {  184, { "NETHERLANDS NEW GUINEA"s,                  ""s,      true  } },
  {  185, { "SOLOMON IS."s,                             "H4",     false } },
  {  186, { "NEWFOUNDLAND, LABRADOR"s,                  ""s,      true  } },
  {  187, { "NIGER"s,                                   "5U",     false } },
  {  188, { "NIUE"s,                                    "ZK2",    false } },
  {  189, { "NORFOLK I."s,                              "VK9N",   false } },
  {  190, { "SAMOA"s,                                   "E5/n",   false } },
  {  191, { "NORTH COOK IS."s,                          "E5/n",   false } },
  {  192, { "OGASAWARA"s,                               "JD/o",   false } },
  {  193, { "OKINAWA (RYUKYU IS.)"s,                    ""s,      true  } },
  {  194, { "OKINO TORI-SHIMA"s,                        ""s,      true  } },
  {  195, { "ANNOBON I."s,                              "3C0",    false } },
  {  196, { "PALESTINE"s,                               ""s,      true  } },
  {  197, { "PALMYRA & JARVIS IS."s,                    "KH5",    false } },
  {  198, { "PAPUA TERRITORY"s,                         ""s,      true  } },
  {  199, { "PETER 1 I."s,                              "3Y/p",   false } },
  {  200, { "PORTUGUESE TIMOR"s,                        ""s,      true  } },
  {  201, { "PRINCE EDWARD & MARION IS."s,              "ZS8"s,   false } },
  {  202, { "PUERTO RICO"s,                             "KP4"s,   false } },
  {  203, { "ANDORRA"s,                                 "C3"s,    false } },
  {  204, { "REVILLAGIGEDO"s,                           "XF4"s,   false } },
  {  205, { "ASCENSION I."s,                            "ZD8"s,   false } },
  {  206, { "AUSTRIA"s,                                 "OE"s,    false } },
  {  207, { "RODRIGUEZ I."s,                            "3B9"s,   false } },
  {  208, { "RUANDA-URUNDI"s,                           ""s,      true  } },
  {  209, { "BELGIUM"s,                                 "ON"s,    false } },
  {  210, { "SAAR"s,                                    ""s,      true  } },
  {  211, { "SABLE I."s,                                "CY0"s,   false } },
  {  212, { "BULGARIA"s,                                "LZ"s,    false } },
  {  213, { "SAINT MARTIN"s,                            "FS"s,    false } },
  {  214, { "CORSICA"s,                                 "TK"s,    false } },
  {  215, { "CYPRUS"s,                                  "5B"s,    false } },
  {  216, { "SAN ANDRES & PROVIDENCIA"s,                "HK0/a"s, false } },
  {  217, { "SAN FELIX & SAN AMBROSIO"s,                "CE0X"s,  false } },
  {  218, { "CZECHOSLOVAKIA"s,                          ""s,      true  } },
  {  219, { "SAO TOME & PRINCIPE"s,                     "S9"s,    false } },
  {  220, { "SARAWAK"s,                                 ""s,      true  } },
  {  221, { "DENMARK"s,                                 "OZ"s,    false } },
  {  222, { "FAROE IS."s,                               "OY"s,    false } },
  {  223, { "ENGLAND"s,                                 "G"s,     false } },
  {  224, { "FINLAND"s,                                 "OH"s,    false } },
  {  225, { "SARDINIA"s,                                "IS"s,    false } },
  {  226, { "SAUDI ARABIA/IRAQ NEUTRAL ZONE"s,          ""s,      true  } },
  {  227, { "FRANCE"s,                                  "F"s,     false } },
  {  228, { "SERRANA BANK & RONCADOR CAY"s,             ""s,      true  } },
  {  229, { "GERMAN DEMOCRATIC REPUBLIC"s,              ""s,      true  } },
  {  230, { "FEDERAL REPUBLIC OF GERMANY"s,             "DL"s,    false } },
  {  231, { "SIKKIM"s,                                  ""s,      true  } },
  {  232, { "SOMALIA"s,                                 "T5"s,    false } },
  {  233, { "GIBRALTAR"s,                               "ZB"s,    false } },
  {  234, { "SOUTH COOK IS."s,                          "E5/s"s,  false } },
  {  235, { "SOUTH GEORGIA I."s,                        "VP8/g"s, false } },
  {  236, { "GREECE"s,                                  "SV"s,    false } },
  {  237, { "GREENLAND"s,                               "OX"s,    false } },
  {  238, { "SOUTH ORKNEY IS."s,                        "VP8/o"s, false } },
  {  239, { "HUNGARY"s,                                 "HA"s,    false } },
  {  240, { "SOUTH SANDWICH IS."s,                      "VP8/s"s, false } },
  {  241, { "SOUTH SHETLAND IS."s,                      "VP8/h"s, false } },
  {  242, { "ICELAND"s,                                 "TF"s,    false } },
  {  243, { "PEOPLE'S DEMOCRATIC REP. OF YEMEN"s,       ""s,      true  } },
  {  244, { "SOUTHERN SUDAN"s,                          ""s,      true  } },
  {  245, { "IRELAND"s,                                 "EI"s,    false } },
  {  246, { "SOVEREIGN MILITARY ORDER OF MALTA"s,       "1A"s,    false } },
  {  247, { "SPRATLY IS."s,                             "1S"s,    false } },
  {  248, { "ITALY"s,                                   "I"s,     false } },
  {  249, { "ST. KITTS & NEVIS"s,                       "V4"s,    false } },
  {  250, { "ST. HELENA"s,                              "ZD7"s,   false } },
  {  251, { "LIECHTENSTEIN"s,                           "HB0"s,   false } },
  {  252, { "ST. PAUL I."s,                             "CY9"s,   false } },
  {  253, { "ST. PETER & ST. PAUL ROCKS"s,              "PY0S"s,  false } },
  {  254, { "LUXEMBOURG"s,                              "LX"s,    false } },
  {  255, { "ST. MAARTEN, SABA, ST. EUSTATIUS"s,        ""s,      true  } },
  {  256, { "MADEIRA IS."s,                             "CT3"s,   false } },
  {  257, { "MALTA"s,                                   "9H"s,    false } },
  {  258, { "SUMATRA"s,                                 ""s,      true  } },
  {  259, { "SVALBARD"s,                                "JW"s,    false } },
  {  260, { "MONACO"s,                                  "3A"s,    false } },
  {  261, { "SWAN IS."s,                                ""s,      true  } },
  {  262, { "TAJIKISTAN"s,                              "EY"s,    false } },
  {  263, { "NETHERLANDS"s,                             "PA"s,    false } },
  {  264, { "TANGIER"s,                                 ""s,      true  } },
  {  265, { "NORTHERN IRELAND"s,                        "GI"s,    false } },
  {  266, { "NORWAY"s,                                  "LA"s,    false } },
  {  267, { "TERRITORY OF NEW GUINEA"s,                 ""s,      true  } },
  {  268, { "TIBET"s,                                   ""s,      true  } },
  {  269, { "POLAND"s,                                  "SP"s,    false } },
  {  270, { "TOKELAU IS."s,                             "ZK3"s,   false } },
  {  271, { "TRIESTE"s,                                 ""s,      true  } },
  {  272, { "PORTUGAL"s,                                "CT"s,    false } },
  {  273, { "TRINDADE & MARTIM VAZ IS."s,               "PY0T"s,  false } },
  {  274, { "TRISTAN DA CUNHA & GOUGH I."s,             "ZD9"s,   false } },
  {  275, { "ROMANIA"s,                                 "YO"s,    false } },
  {  276, { "TROMELIN I."s,                             "FR/t"s,  false } },
  {  277, { "ST. PIERRE & MIQUELON"s,                   "FP"s,    false } },
  {  278, { "SAN MARINO"s,                              "T7"s,    false } },
  {  279, { "SCOTLAND"s,                                "GM"s,    false } },
  {  280, { "TURKMENISTAN"s,                            "EZ"s,    false } },
  {  281, { "SPAIN"s,                                   "EA"s,    false } },
  {  282, { "TUVALU"s,                                  "T2"s,    false } },
  {  283, { "UK SOVEREIGN BASE AREAS ON CYPRUS"s,       "ZC4"s,   false } },
  {  284, { "SWEDEN"s,                                  "SM"s,    false } },
  {  285, { "VIRGIN IS."s,                              "KP2"s,   false } },
  {  286, { "UGANDA"s,                                  "5X"s,    false } },
  {  287, { "SWITZERLAND"s,                             "HB"s,    false } },
  {  288, { "UKRAINE"s,                                 "UR"s,    false } },
  {  289, { "UNITED NATIONS HQ"s,                       "4U1U"s,  false } },
  {  291, { "UNITED STATES OF AMERICA"s,                "K"s,     false } },
  {  292, { "UZBEKISTAN"s,                              "UK"s,    false } },
  {  293, { "VIET NAM"s,                                "3W"s,    false } },
  {  294, { "WALES"s,                                   "GW"s,    false } },
  {  295, { "VATICAN"s,                                 "HV"s,    false } },
  {  296, { "SERBIA"s,                                  "YU"s,    false } },
  {  297, { "WAKE I."s,                                 "KH9"s,   false } },
  {  298, { "WALLIS & FUTUNA IS."s,                     "FW"s,    false } },
  {  299, { "WEST MALAYSIA"s,                           "9M2"s,   false } },
  {  301, { "W. KIRIBATI (GILBERT IS. )"s,              "T30"s,   false } },
  {  302, { "WESTERN SAHARA"s,                          "S0"s,    false } },
  {  303, { "WILLIS I."s,                               "VK9W"s,  false } },
  {  304, { "BAHRAIN"s,                                 "A9"s,    false } },
  {  305, { "BANGLADESH"s,                              "S2"s,    false } },
  {  306, { "BHUTAN"s,                                  "A5"s,    false } },
  {  307, { "ZANZIBAR"s,                                ""s,      true  } },
  {  308, { "COSTA RICA"s,                              "TI"s,    false } },
  {  309, { "MYANMAR"s,                                 "XZ"s,    false } },
  {  312, { "CAMBODIA"s,                                "XU"s,    false } },
  {  315, { "SRI LANKA"s,                               "4S"s,    false } },
  {  318, { "CHINA"s,                                   "BY"s,    false } },
  {  321, { "HONG KONG"s,                               "VR"s,    false } },
  {  324, { "INDIA"s,                                   "VU"s,    false } },
  {  327, { "INDONESIA"s,                               "YB"s,    false } },
  {  330, { "IRAN"s,                                    "EP"s,    false } },
  {  333, { "IRAQ"s,                                    "YI"s,    false } },
  {  336, { "ISRAEL"s,                                  "4X"s,    false } },
  {  339, { "JAPAN"s,                                   "JA"s,    false } },
  {  342, { "JORDAN"s,                                  "JY"s,    false } },
  {  344, { "DEMOCRATIC PEOPLE'S REP. OF KOREA "s,      "HM"s,    false } },
  {  345, { "BRUNEI DARUSSALAM"s,                       "V8"s,    false } },
  {  348, { "KUWAIT"s,                                  "9K"s,    false } },
  {  354, { "LEBANON"s,                                 "OD"s,    false } },
  {  363, { "MONGOLIA"s,                                "JT"s,    false } },
  {  369, { "NEPAL"s,                                   "9N"s,    false } },
  {  370, { "OMAN"s,                                    "A4"s,    false } },
  {  372, { "PAKISTAN"s,                                "AP"s,    false } },
  {  375, { "PHILIPPINES"s,                             "DU"s,    false } },
  {  376, { "QATAR"s,                                   "A7"s,    false } },
  {  378, { "SAUDI ARABIA"s,                            "HZ"s,    false } },
  {  379, { "SEYCHELLES"s,                              "S7"s,    false } },
  {  381, { "SINGAPORE"s,                               "9V"s,    false } },
  {  382, { "DJIBOUTI"s,                                "J2"s,    false } },
  {  384, { "SYRIA"s,                                   "YK"s,    false } },
  {  386, { "TAIWAN"s,                                  "BV"s,    false } },
  {  387, { "THAILAND"s,                                "HS"s,    false } },
  {  390, { "TURKEY"s,                                  "TA"s,    false } },
  {  391, { "UNITED ARAB EMIRATES"s,                    "A6"s,    false } },
  {  400, { "ALGERIA"s,                                 "7X"s,    false } },
  {  401, { "ANGOLA"s,                                  "D2"s,    false } },
  {  402, { "BOTSWANA"s,                                "A2"s,    false } },
  {  404, { "BURUNDI"s,                                 "9U"s,    false } },
  {  406, { "CAMEROON"s,                                "TJ"s,    false } },
  {  408, { "CENTRAL AFRICA"s,                          "TL"s,    false } },
  {  409, { "CAPE VERDE"s,                              "D4"s,    false } },
  {  410, { "CHAD"s,                                    "TT"s,    false } },
  {  411, { "COMOROS"s,                                 "D6"s,    false } },
  {  412, { "REPUBLIC OF THE CONGO"s,                   "9Q"s,    false } },
  {  414, { "DEMOCRATIC REPUBLIC OF THE CONGO"s,        "TN"s,    false } },
  {  416, { "BENIN"s,                                   "TY"s,    false } },
  {  420, { "GABON"s,                                   "TR"s,    false } },
  {  422, { "THE GAMBIA"s,                              "C5"s,    false } },
  {  424, { "GHANA"s,                                   "9G"s,    false } },
  {  428, { "COTE D'IVOIRE"s,                           "TU"s,    false } },
  {  430, { "KENYA"s,                                   "5Z"s,    false } },
  {  432, { "LESOTHO"s,                                 "7P"s,    false } },
  {  434, { "LIBERIA"s,                                 "EL"s,    false } },
  {  436, { "LIBYA"s,                                   "5A"s,    false } },
  {  438, { "MADAGASCAR"s,                              "5R"s,    false } },
  {  440, { "MALAWI"s,                                  "7Q"s,    false } },
  {  442, { "MALI"s,                                    "TZ"s,    false } },
  {  444, { "MAURITANIA"s,                              "5T"s,    false } },
  {  446, { "MOROCCO"s,                                 "CN"s,    false } },
  {  450, { "NIGERIA"s,                                 "5N"s,    false } },
  {  452, { "ZIMBABWE"s,                                "Z2"s,    false } },
  {  453, { "REUNION I."s,                              "FR"s,    false } },
  {  454, { "RWANDA"s,                                  "9X"s,    false } },
  {  456, { "SENEGAL"s,                                 "6W"s,    false } },
  {  458, { "SIERRA LEONE"s,                            "9L"s,    false } },
  {  460, { "ROTUMA I."s,                               "3D2/r"s, false } },
  {  462, { "SOUTH AFRICA"s,                            "ZS"s,    false } },
  {  464, { "NAMIBIA"s,                                 "V5"s,    false } },
  {  466, { "SUDAN"s,                                   "ST"s,    false } },
  {  468, { "SWAZILAND"s,                               "3DA"s,   false } },
  {  470, { "TANZANIA"s,                                "5H"s,    false } },
  {  474, { "TUNISIA"s,                                 "3V"s,    false } },
  {  478, { "EGYPT"s,                                   "SU"s,    false } },
  {  480, { "BURKINA FASO"s,                            "XT"s,    false } },
  {  482, { "ZAMBIA"s,                                  "9J"s,    false } },
  {  483, { "TOGO"s,                                    "5V"s,    false } },
  {  488, { "WALVIS BAY"s,                              ""s,      true  } },
  {  489, { "CONWAY REEF"s,                             "3D2/c"s, false } },
  {  490, { "BANABA I. (OCEAN I.)"s,                    "T33"s,   false } },
  {  492, { "YEMEN"s,                                   "7O"s,    false } },
  {  493, { "PENGUIN IS."s,                             ""s,      true  } },
  {  497, { "CROATIA"s,                                 "9A"s,    false } },
  {  499, { "SLOVENIA"s,                                "S5"s,    false } },
  {  501, { "BOSNIA-HERZEGOVINA"s,                      "E7"s,    false } },
  {  502, { "MACEDONIA"s,                               "Z3"s,    false } },
  {  503, { "CZECH REPUBLIC"s,                          "OK"s,    false } },
  {  504, { "SLOVAK REPUBLIC"s,                         "OM"s,    false } },
  {  505, { "PRATAS I."s,                               "BV9P"s,  false } },
  {  506, { "SCARBOROUGH REEF"s,                        "BS7"s,   false } },
  {  507, { "TEMOTU PROVINCE"s,                         "H40"s,   false } },
  {  508, { "AUSTRAL I."s,                              "FO/a"s,  false } },
  {  509, { "MARQUESAS IS."s,                           "FO/m"s,  false } },
  {  510, { "PALESTINE"s,                               "E4"s,    false } },
  {  511, { "TIMOR-LESTE"s,                             "4W"s,    false } },
  {  512, { "CHESTERFIELD IS."s,                        "FK/c"s,  false } },
  {  513, { "DUCIE I."s,                                "VP6/d"s, false } },
  {  514, { "MONTENEGRO"s,                              "4O"s,    false } },
  {  515, { "SWAINS I."s,                               "KH8/s"s, false } },
  {  516, { "SAINT BARTHELEMY"s,                        "FJ"s,    false } },
  {  517, { "CURACAO"s,                                 "PJ2"s,   false } },
  {  518, { "ST MAARTEN"s,                              "PJ7"s,   false } },
  {  519, { "SABA & ST. EUSTATIUS"s,                    "PJ5"s,   false } },
  {  520, { "BONAIRE"s,                                 "PJ4"s,   false } },
  {  521, { "SOUTH SUDAN (REPUBLIC OF)"s,               "Z8"s,    false } },
  {  522, { "REPUBLIC OF KOSOVO"s,                      "Z6",     false } }
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
