// $Id: exchange.h 76 2014-09-21 20:33:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef EXCHANGE_H
#define EXCHANGE_H

#include "macros.h"
#include "pthread_support.h"
#include "rules.h"

#include <string>
#include <vector>

#include <boost/regex.hpp>    // because libstdc++ support for regex is essentially nonexistent

class exchange_field_template;

extern pt_mutex exchange_field_database_mutex;

#if !defined(NEW_CONSTRUCTOR)
extern exchange_field_template EXCHANGE_FIELD_TEMPLATES;
#endif

extern const std::string process_cut_digits(const std::string& input);

// -------------------------  parsed_exchange_field  ---------------------------

/*!     \class parsed_exchange_field
        \brief Encapsulates the name for an exchange field, its value after parsing an exchange, and whether it's a mult
*/

WRAPPER_3(parsed_exchange_field, std::string, name, std::string, value, bool, is_mult);

// -------------------------  parsed_exchange  ---------------------------

/*!     \class parsed_exchange
        \brief All the fields in the exchange, following parsing
*/

class parsed_exchange
{
protected:

  std::string                           _replacement_call;    ///< a new callsign, to replace the one in the CALL window
  std::vector<parsed_exchange_field>    _fields;              ///< all the names, values and is_mult() indicators, in the same order as the exchange definition in the configuration file
  bool                                  _valid;               ///< is the object valid? (i.e., was parsing successful?)


#if !defined(NEW_CONSTRUCTOR)
  /*! \brief  Given several possible field names, choose one that fits the data
    \param  choice_name   the name of the choice field (e.g., "SOCIETY+ITU_ZONE"
    \param  received_field the value of the received field
    \return the individual name of a field in <i>choice_name</i> that fits the data

    Returns the first field name in <i>choice_name</i> that fits the value of <i>received_field</i>.
    If there is no fit, then returns the empty string.
*/
  const std::string _resolve_choice(const std::string& canonical_prefix, const std::string& received_field, const contest_rules& rules);
#endif

  void _fill_fields(const std::map<int, std::set<std::string>>& matches, const std::vector<std::string>& received_values);

  void _print_tuple(const std::tuple<int, std::string, std::set<std::string>>& t) const;

public:

/*!     \brief  constructor
        \param  callsign    callsign of the station from which the exchange was received
        \param  rules       rules for the contest
        \param  received_values     the received values, in the order that they were received
*/
  parsed_exchange(const std::string& callsign, const contest_rules& rules, const std::vector<std::string>& received_values);

  READ(replacement_call);              ///< a new callsign, intended to replace the one in the CALL window
  READ(valid);                         ///< is the object valid? (i.e., was parsing successful?)
  READ(fields);                        ///< all the names, values and is_mult() indicators, in the same order as the exchange definition in the configuration file

/// is the object valid? (i.e., was parsing successful?)
  inline const bool is_valid(void) const
    { return valid(); }

/// is a replacement call present?
  inline const bool has_replacement_call(void) const
    { return (!_replacement_call.empty() ); }

/*! \brief  Return the value of a particular field
    \param  field_name  field for which the value is requested
    \return value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
  const std::string field_value(const std::string& field_name) const;

/// the number of fields
  inline const size_t n_fields(void) const
    { return _fields.size(); }

/*! \brief  Return the name of a field
    \param  n  number of field for which the name is requested
    \return name corresponding to <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline const std::string field_name(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].name()); }

/*! \brief  Return the value of a field
    \param  n  number of field for which the value is requested
    \return value corresponding to <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline const std::string field_value(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].value()); }

/*! \brief  Is a field a mult?
    \param  n  number of field for which the mult status is requested
    \return  whether field number <i>n</i> is a mult

    Returns false if <i>n</i> is out of range
*/
  inline const bool field_is_mult(const size_t n) const
    { return (n >= _fields.size() ? false : _fields[n].is_mult()); }
};

/// ostream << parsed_exchange
std::ostream& operator<<(std::ostream& ost, const parsed_exchange& pe);

// -------------------------  exchange_field_database  ---------------------------

/*!     \class exchange_field_database
        \brief used for estimating the exchange field

        There can be only one of these, and it is thread safe
*/

class exchange_field_database
{
protected:

  std::map< std::pair< std::string /* callsign */, std::string /* field name */>, std::string /* value */> _db;  ///< the actual database
//          --------------------------------  key  -----------------------------  --------  value  -------

public:

/*! \brief  Guess the value of an exchange field
    \param  callsign   callsign for the guess
    \param  field_name name of the field for the guess
    \return Guessed value of <i>field_name</i> for <i>callsign</i>

    Returns empty string if no sensible guess can be made
*/
  const std::string guess_value(const std::string& callsign, const std::string& field_name);

/*! \brief  Set a value in the database
    \param  callsign   callsign for the new entry
    \param  field_name name of the field for the new entry
    \param  value      the new entry
*/
  void set_value(const std::string& callsign, const std::string& field_name, const std::string& value);

  inline const size_t size(void) const
    { return _db.size(); }
};

#if !defined(NEW_CONSTRUCTOR)
// -------------------------  exchange_field_template  ---------------------------

/*!     \class exchange_field_template
        \brief used for managing and parsing exchange fields
*/
class exchange_field_template
{
protected:

  std::map<std::string /* name */, boost::regex> _db;       ///< database of regular expressions for named exchange fields

public:

/*! \brief  Fill the database
    \param  path        directories to search (in order) for the filename
    \param  filename    name of the file that holds regex expressions for fields
*/
  void prepare(const std::vector<std::string>& path, const std::string& filename);

  READ(db);

/// is <i>name</i> the name of a recognised field?
  inline const bool is_valid_field_name(const std::string& name) const
    { return (_db.find(name) != _db.cend()); }

/*! \brief  Is a particular received string a valid value for a named field?
    \param  name        name of the exchange field
    \param  str         string to test for validity
*/
  const bool is_valid(const std::string& name /* field name */, const std::string& str);

/*! \brief      What fields are a valid match for a particular received string?
    \param  str string to test
    \return     Names of fields for which <i>str</i> is a valid value
*/
  const std::vector<std::string> valid_matches(const std::string& str);

/// Return all the names of exchange fields in the database
  const std::set<std::string> names(void) const;

/// Return regex for a name; returns empty regex if the name is invalid
  const boost::regex expression(const std::string& str) const;
};
#endif

#if !defined(NEW_CONSTRUCTOR)
/*! \brief  Is a string a valid callsign?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of callsign
*/
const bool is_valid_CALLSIGN(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid CQ zone?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of CQ zone
*/
const bool is_valid_CQZONE(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid CW power?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of CW power
*/
const bool is_valid_CWPOWER(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid ITU zone?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of ITU zone
*/
const bool is_valid_ITUZONE(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid RDA district?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of RDA district
*/
const bool is_valid_RDA(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid RS value?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of RS
*/
const bool is_valid_RS(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid RST value?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of RST
*/
const bool is_valid_RST(const std::string& str, const contest_rules& rules);

/*! \brief  Is a string a valid IARU society?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of society

    Doesn't actually compare against known societies; merely tests whether <i>str</i> comprises only letters
*/
const bool is_valid_SOCIETY(const std::string& str, const contest_rules& rules);

/// match anything; simply returns true
const bool is_valid_ANYTHING(const std::string&, const contest_rules& rules);

typedef const bool (*VALIDITY_FUNCTION_TYPE)(const std::string& field_name, const contest_rules& rules);

/*! \brief  Obtain the validity function corresponding to a particular exchange field name
    \param  field_name   name of the field for which the validity function is requested
    \return the validity function corresponding to <i>field_name</i>
*/
VALIDITY_FUNCTION_TYPE validity_function(const std::string& field_name, const contest_rules& rules);
#endif

// -------------------------  EFT  ---------------------------

/*!     \class EFT (exchange_field_template)
        \brief Manage a single exchange field
*/

class EFT
{
protected:

// There are several ways to define a field:
//  1. regex
//  2. .values file

  std::string _name;                    ///< name of exchange field
  boost::regex _regex_expression;       ///< regex expression to define field

  std::map<std::string,                        /* a canonical field value */
          std::set                             /* each equivalent value is a member of the set, including the canonical value */
            <std::string                       /* indistinguishable legal values */
            >> _values;

  std::set<std::string>  _legal_non_regex_values;
  std::map<std::string, std::string>  _value_to_canonical;

  bool _is_mult;                       ///< is this field a mult?

public:

/// construct from name
  EFT(const std::string& nm);

/// construct from regex and values files
  EFT(const std::string& nm, const std::vector<std::string>& path, const std::string& regex_filename /*, const std::string& values_filename = nm */);
  EFT(const std::string& nm, const std::vector<std::string>& path,
      const std::string& regex_filename,
      const drlog_context& context, location_database& location_db);

  READ_AND_WRITE(is_mult);
  READ_AND_WRITE(name);
  READ(regex_expression);
  READ(values);
  READ(legal_non_regex_values);
  READ(value_to_canonical);

/*! \brief  Get regex expression from file
    \param  path      paths to try
    \param  filename   name of file
    \return whether a regex expression was read
*/
  const bool read_regex_expression_file(const std::vector<std::string>& path, const std::string& filename);

/*! \brief  Get info from .values file
    \param  path      paths to try
    \param  filename   name of file (without .values extension)
    \return whether values were read
*/
  const bool read_values_file(const std::vector<std::string>& path, const std::string& filename);

  void parse_context_qthx(const drlog_context& context, location_database& location_db);

  inline const bool is_canonical_value(const std::string& str) const
    { return (_values.find(str)  != _values.end()); }

  void add_canonical_value(const std::string& new_canonical_value);

  void add_legal_value(const std::string& cv, const std::string& new_value);

// check regex, then other values
  const bool is_legal_value(const std::string& str) const;

  const std::string value_to_log(const std::string& str) const;

// return canonical value for a received value
  const std::string canonical_value(const std::string& str) const;

// all canonical values
  const std::set<std::string> canonical_values(void) const;
};

std::ostream& operator<<(std::ostream& ost, const EFT& eft);

#endif /* EXCHANGE_H */

