// $Id: adif3.h 259 2025-01-19 15:44:33Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

/*! \file   adif3.h

    Objects and functions related to ADIF version 3.1.0 at https://adif.org/310/ADIF_310.htm
    
    I refrain from comment on the self-evident quality of this soi-disant "specification"
*/

#ifndef ADIF3_H
#define ADIF3_H

// https://adif.org/310/ADIF_310.htm#ADIF_defined_Fields:
//   Fields of type IntlCharacter, IntlString, and IntlMultilineString cannot be used in ADI files.

// Yes, this is an utter mess... it's hard to be clean when what you're trying to model makes
// a pile of spaghetti look organised

#include "macros.h"
#include "string_functions.h"
#include "x_error.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std::literals::string_literals;

// error numbers
constexpr int ADIF3_INVALID_VALUE       { -1 },    ///< Invalid value
              ADIF3_INVALID_CHARACTER   { -2 },    ///< value contains invalid character
              ADIF3_INVALID_LENGTH      { -3 },    ///< value is incorrect length
              ADIF3_EMPTY_VALUE         { -4 },    ///< value is empty (implies incorrect length)
              ADIF3_UNKNOWN_TYPE        { -5 },    ///< unable to determine type
              ADIF3_DUPLICATE_FIELD     { -6 };    ///< duplicate field name

// should really be ADIF3_COUNTRY_STATUS, but that's too verbose until g++ supports "using enum", which is in g++ 11
enum class COUNTRY_STATUS { CURRENT,
                            DELETED
                          };

enum class ADIF3_DATA_TYPE { AWARD_LIST,
                             BOOLEAN,
                             CHARACTER,
                             CREDIT_LIST,
                             DATE,
                             DIGIT,
                             ENUMERATION,                       // as if these "enumerations" have anything to do with numbers
                             ENUMERATION_ANT_PATH,
                             ENUMERATION_ARRL_SECT,
                             ENUMERATION_BAND,
                             ENUMERATION_CONTINENT,
                             ENUMERATION_DARC_DOK,
                             ENUMERATION_DXCC_ENTITY_CODE,
                             ENUMERATION_MODE,
                             ENUMERATION_PRIMARY_ADMINISTRATIVE_SUBDIVISION,
                             ENUMERATION_PROPAGATION_MODE,
                             ENUMERATION_QSL_RECEIVED,
                             ENUMERATION_QSL_SENT,
                             ENUMERATION_QSL_VIA,
                             ENUMERATION_QSO_COMPLETE,
                             ENUMERATION_QSO_UPLOAD_STATUS,
                             ENUMERATION_REGION,
                             ENUMERATION_SECONDARY_ADMINISTRATIVE_SUBDIVISION,
                             GRID_SQUARE,
                             GRID_SQUARE_LIST,
                             INTEGER,
                             INTERNATIONAL_CHARACTER,           // the "INTERNATIONAL" things are sheer idiocy; Unicode has been essentially universal for at least 15 years
                             INTERNATIONAL_MULTILINE_STRING,
                             INTERNATIONAL_STRING,
                             IOTA_REFERENCE_NUMBER,
                             LOCATION,
                             MULTILINE_STRING,
                             NUMBER,
                             POSITIVE_INTEGER,
                             SECONDARY_SUBDIVISION_LIST,
                             SOTA_REFERENCE,
                             SPONSORED_AWARD_LIST,
                             STRING,
                             TIME,
                             N_TYPES
                           };                       ///< ADIF3 types of data

// ---------------------------------------------------  adif3_field  -----------------------------------------

/*! \class adif3_field
    \brief A single generic ADIF field
*/

class adif3_field
{
protected:

  std::string     _name;                    ///< name of the field
  ADIF3_DATA_TYPE _type;                    ///< type of the field
  std::string     _value;                   ///< value of the field
  
/*! \brief  place values into standardised forms as necessary

    Assumes that names are in upper case
*/
  void _normalise(void);

/// verify that values are legal
  void _verify(void) const;

// private objects and collections providing legal values

  const static UNORDERED_STRING_MAP<ADIF3_DATA_TYPE /* corresponding data type */>   _element_type;             ///< map from field name to type

  static STRING_MAP<std::pair<int, int> /* minimum and maximum permitted values */> _positive_integer_range;   ///< map from field name to permitted range of values

// soi-disant "enumeration" values (actually typically strings)
  const static UNORDERED_STRING_SET _ENUMERATION_BAND;     ///< band values
  const static std::unordered_map<int /* country number */, std::tuple<std::string /*country name */, 
                                                            std::string /* canonical prefix */, 
                                                            COUNTRY_STATUS /* whether deleted */>> _ENUMERATION_DXCC_ENTITY_CODE; ///< mapping between country code and country info

  static UNORDERED_STRING_SET _ENUMERATION_MODE;     ///< mode values
  static STRING_SET _ENUMERATION_QSL_RECEIVED;       ///< legal values of QSL_RCVD

public:

/// default constructor
  adif3_field(void) = default;
  
/*! \brief                  Construct from name and value
    \param  field_name      name of field
    \param  field_value     value of field

    <i>field_name</i> is converted to upper case when stored as <i>_name</i>
    <i>field_value</i> is validated and converted to standardised format (if applicable)
*/
  adif3_field(const std::string_view field_name, const std::string_view field_value);

  READ_AND_WRITE_STR(name);                    ///< name of the field
  READ_AND_WRITE(type);                        ///< type of the field
  READ_AND_WRITE(value);                       ///< value of the field

/*! \brief              Convert to printable string
    \param  append_str  string to append to the field
    \return             the canonical textual representation of the name and value of the field

    Returns empty string if either the name or the value is empty
*/
  inline std::string to_string(const std::string& append_str = "\n"s) const
    { return ( (_name.empty() or _value.empty()) ? std::string { } : ( "<"s + _name + ":"s + ::to_string(_value.length()) +">"s + _value + append_str) ); }
    
/*! \brief                  Import name and value from string, and return location past the end of the used part of the string
    \param  str             string from which to read
    \param  start_posn      position in <i>str</i> at which to start parsing
    \param  end_posn        one past the location at which to force an end to parsing, if necessary
    \param  accept_fields   ADIF fields to accept (all fields accepted if empty)
    \return                 one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/
  size_t import_and_eat(const std::string_view str, const size_t start_posn, const size_t end_posn /* one past <EOR> */, const STRING_SET& accept_fields = { });

/// is the field empty?
  inline bool empty(void) const
    { return _name.empty(); }
};

// ---------------------------------------------------  adif3_record -----------------------------------------

/*! \class  adif3_record
    \brief  a single ADIF3 record
*/

class adif3_record
{
protected:

  STRING_MAP<adif3_field>  _elements;       ///< map field name to the complete field; simplest to keep this ordered so that the fields are in alphabetical order

  static std::set<ADIF3_DATA_TYPE> _import_only;                     ///< fields that are not to be output

/*! \brief          Convert a string to an int, assuming that the string contains just digits
    \param  str     string to convert
    \return         <i>str</i> rendered as an int

    Result is valid ONLY if <i>str</i> contains only digits
*/  
  int _fast_string_to_int(const std::string_view str) const;            // convert a string to an int, assuming that the string contains just digits

public:

  adif3_record(void) = default;
 
/*! \brief              Import record from string, and return location past the end of the used part of the string
    \param  str         string from which to read
    \param  posn        position in <i>str</i> at which to start parsing
    \return             one past the last location to be used

    Returns string::npos if reads past the end of <i>str</i>
*/
  size_t import_and_eat(const std::string_view str, const size_t posn, const STRING_SET& accept_fields = { });

/*! \brief      Convert to printable string
    \return     the canonical textual representation of the record

    Returns just the end-of-record marker is the record is empty.
    Does not output import-only fields.
*/
  std::string to_string(void) const;

/*! \brief        Return the value of a field
    \param  str   of the field whose value is to be returned
    \return       the value of the field <i>str</i>

    Returns the empty string if the field <i>str</i> does not exist in the record 
*/
  inline std::string value(const std::string_view str) const
    { return MUM_VALUE(_elements, to_upper(str)).value(); }
  
/*! \brief                  Set the value of a field (which does not have to be extant in the record)
    \param  field_name      name of the field whose value is to be set
    \param  field_value     value to be set
    \return                 whether this was a new field

    <i>field_name</i> is converted to upper case when stored as <i>_name</i>
    <i>field_value</i> is validated and converted to standardised format (if applicable)
*/
  bool value(const std::string_view field_name, const std::string_view field_value);

/// return the ADIF3 value of the band (empty string if none) 
  inline std::string band(void) const
    { return value("BAND"s); }

/// return the ADIF3 value of the other station's callsign (empty string if none)
  inline std::string callsign(void) const
    { return value("CALL"s); }
    
/// return whether a QSL card is known to have been received
  inline bool confirmed(void) const
    { return (value("QSL_RCVD"s) == "Y"s); }

 /// return the ADIF3 value of the date [YYYYMMDD] (empty string if none)   
  inline std::string date(void) const
    { return value("QSO_DATE"s); }

/// return the ADIF3 value of the date [YYYYMMDD] (zero if none)
  inline int idate(void) const                                  // YYYYMMDD
    { return _fast_string_to_int(date()); }

/// return the ADIF3 value of the band (empty string if none)
  inline std::string mode(void) const
    { return value("MODE"s); }

/// return the ADIF3 value of the time (empty string if none)
  inline std::string time(void) const
    { return value("TIME_ON"s); }

/// return whether the record is empty    
  inline bool empty(void) const
    { return _elements.empty(); }
};

/// ostream << adif3_record
inline std::ostream& operator<<(std::ostream& ost, const adif3_record& rec)
  { return (ost << rec.to_string()); }

/*! \brief         Is one ADIF3 record chronologically earlier than another
    \param  rec1   first record
    \param  rec2   second record
    \return        whether <i>record1</i> is chronologically before <i>record2</i> 
*/
bool compare_adif3_records(const adif3_record& rec1, const adif3_record& rec2);

// ---------------------------------------------------  adif3_file -----------------------------------------

/*! \class  adif3_file
    \brief  all the ADIF3 records in a file
*/

class adif3_file : public std::vector<adif3_record>
{
protected:
  
  UNORDERED_STRING_MULTIMAP<adif3_record> _map_data;      ///< alternative access using a map; key = call

public:

/*! \brief              Construct from file name
    \param  filename    name of file to read

    Throws exception if something goes wrong when reading the file
*/
  adif3_file(const std::string_view filename, const STRING_SET& accept_fields = { });

/*! \brief              Construct from file name
    \param  path        vector of directories in which to look
    \param  filename    name of file to read

    Returns empty object if a problem occurs
*/
  adif3_file(const std::vector<std::string>& path, const std::string_view filename, const STRING_SET& accept_fields = { });

/*! \brief                  Return all the QSOs that match a call, band and mode
    \param      callsign    call to match
    \param      b           ADIF3 band to match
    \param      m           ADIF3 mode to match
    \return     filename    Return all the QSO records that match <i>callsign</i>, <i>b</i> and <i>m</i>
*/
  std::vector<adif3_record> matching_qsos(const std::string_view callsign, const std::string_view b, const std::string_view m) const;

/*! \brief                  Return all the QSOs that match a call
    \param      callsign    call to match
    \return     filename    Return all the QSO records that match <i>callsign</i>
*/
  std::vector<adif3_record> matching_qsos(const std::string_view callsign) const;
};

/*! \brief              Return position at which to start processing the body of the file
    \param      str     string of contents of file
    \return             position of first "<" after the end of the header
*/
size_t skip_adif3_header(const std::string_view str);

// -------------------------------------- Errors  -----------------------------------

/*! \class  adif3_error
    \brief  Errors for ADIF3 objects
*/

class adif3_error : public x_error
{
protected:
  
public:

/*!	\brief	Construct from error code and reason
	\param	n	Error code
	\param	s	Reason
*/
  inline adif3_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};

#endif    // ADIF3_H
