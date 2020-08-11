<<<<<<< HEAD
// $Id: adif3.h 163 2020-08-06 19:46:33Z  $
=======
// $Id: adif3.h 158 2020-06-27 20:33:02Z  $
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9

// Released under the GNU Public License, version 2

// Principal author: N7DR

#ifndef ADIF3_H
#define ADIF3_H

// https://adif.org/310/ADIF_310.htm#ADIF_defined_Fields:
//   Fields of type IntlCharacter, IntlString, and IntlMultilineString cannot be used in ADI files.

// Yes, this is an utter mess... it's hard to be clean when what you're trying to model makes
// a pile of spaghetti look organised

/*! \file   adif3.h

    Objects and functions related to ADIF version 3.1.0 at https://adif.org/310/ADIF_310.htm
    
    I refrain from comment on the self-evident quality of this soi-disant "specification"
*/

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

enum class ADIF3_DATA_TYPE { AWARD_LIST,
                             CREDIT_LIST,
                             SPONSORED_AWARD_LIST,
                             BOOLEAN,
                             DIGIT,
<<<<<<< HEAD
=======
                             INTEGER,
                             NUMBER,
                             POSITIVE_INTEGER,
                             CHARACTER,
                             INTERNATIONAL_CHARACTER,
                             DATE,
                             TIME,
                             IOTA_REFERENCE_NUMBER,
                             STRING,
                             INTERNATIONAL_STRING,
                             MULTILINE_STRING,
                             INTERNATIONAL_MULTILINE_STRING,
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9
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
  
  void _normalise(void);
  void _verify(void) const;

// private objects and collections providing legal values

  static std::unordered_map<std::string, ADIF3_DATA_TYPE> _element_type;    // maps from field name to type

  static std::map<std::string, std::pair<int, int>> _positive_integer_range;

  static std::unordered_set<std::string> _ENUMERATION_BAND;
  static std::unordered_map<int /* country number */, std::tuple<std::string /*country name */, std::string /* canonical prefix */, bool /* whether deleted */>> _ENUMERATION_DXCC_ENTITY_CODE;
  static std::unordered_set<std::string> _ENUMERATION_MODE;
  static std::set<std::string> _ENUMERATION_QSL_RECEIVED;

public:

/// default constructor
  adif3_field(void) = default;
  
/// copy constructor
  adif3_field(const adif3_field&) = default;
  
  adif3_field(const std::string& field_name, const std::string& field_value);
  
  READ_AND_WRITE(name);                        ///< name of the field
  READ_AND_WRITE(type);                        ///< type of the field
  READ_AND_WRITE(value);                       ///< value of the field

/// convert to printable string
  inline const std::string to_string(const std::string& append_str = "\n"s) const
    { return ( (_name.empty() or _value.empty()) ? std::string() : ( "<"s + _name + ":"s + ::to_string(_value.length()) +">"s + _value + append_str) ); }
    
/// import from string, and eat string
  size_t import_and_eat(const std::string& str, const size_t start_posn, const size_t end_posn /* one past <EOR> */);

/// adif3_field < adif3_field (needed for maps/sets)  ??
  inline const bool operator<(const adif3_field& v2) const
    { return (_name < v2._name); }
};

// ---------------------------------------------------  adif3_record -----------------------------------------

/*! \class  adif3_record
    \brief  a single ADIF3 record
*/

class adif3_record
{
protected:

  std::map<std::string /* call */, adif3_field>  _elements;          // simplest to keep this ordered so that the fields are in alphabetical order
  
  static std::set<ADIF3_DATA_TYPE> _import_only;                            // fields that are not to be output
  
public:

  adif3_record(void) = default;
  
  size_t import_and_eat(const std::string& str, const size_t posn);
  
  const std::string to_string(void) const;
  
  const std::string value(const std::string& str) const;
  
  const bool value(const std::string& field_name, const std::string& field_value);     // set value of a field, replacing if already exists, adding if it does not; rv = whether inserted 
  
  inline const std::string band(void) const
    { return value("BAND"s); }

  inline const std::string callsign(void) const
    { return value("CALL"s); }
    
  inline const bool confirmed(void) const
    { return (value("QSL_RCVD"s) == "Y"s); }
    
  inline const std::string date(void) const
    { return value("QSO_DATE"s); }

  inline const int idate(void) const        // YYYYMMDD
    { return from_string<int>(date()); }

  inline const std::string mode(void) const
    { return value("MODE"s); }

  inline const std::string time(void) const
    { return value("TIME_ON"s); }
    
  inline const bool empty(void) const
    { return _elements.empty(); }
};

/*! \brief         Is one ADIF3 record chronologically earlier than another
    \param  rec1   first record
    \param  rec2   second record
    \return        whether <i>record1</i> is chronologically before <i>record2</i> 
*/
const bool compare_adif3_records(const adif3_record& rec1, const adif3_record& rec2);
  
// ---------------------------------------------------  adif3_file -----------------------------------------

/*! \class  adif3_file
    \brief  all the ADIF3 records in a file
*/

class adif3_file : public std::vector<adif3_record>
{
protected:
  
  std::unordered_multimap<std::string /* call */, adif3_record> _map_data;
  
public:

// construct from file name
  adif3_file(const std::string& filename);

  adif3_file(const std::vector<std::string>& path, const std::string& filename);
  
// is a QSO present? -1 => no, otherwise the index number
  const int is_present(const adif3_record& rec) const; 
  
  const adif3_record get_record(const adif3_record& rec) const; 
  
  const std::vector<adif3_record> matching_qsos(const std::string& callsign, const std::string& b, const std::string& m) const;

  const std::vector<adif3_record> matching_qsos(const std::string& callsign) const;

};

/// return position at which to start processing the body of the file
const size_t skip_adif3_header(const std::string& str);

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
