// $Id: exchange.h 92 2015-01-24 22:36:02Z  $

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

class exchange_field_template;                  ///< forward declaration

extern pt_mutex exchange_field_database_mutex;  ///< mutex for the exchange field database

// -------------------------  parsed_exchange_field  ---------------------------

/*!     \class parsed_exchange_field
        \brief Encapsulates the name for an exchange field, its value after parsing an exchange, and whether it's a mult
*/

class parsed_exchange_field
{
protected:
  std::string    _name;                 ///< field name
  std::string    _value;                ///< field value
  bool           _is_mult;              ///< is this field a mult?
  std::string    _mult_value;           ///< actual value of the mult (if it is a mult)

public:

/// default constructor
  parsed_exchange_field(void);

/*! \brief      Constructor
    \param  nm  field name
    \param  v   field value
    \param  b   is this field a mult?
*/
  parsed_exchange_field(const std::string& nm, const std::string& v, const bool m);

  READ(name);                   ///< field name
  READ(value);                  ///< field value
  READ_AND_WRITE(is_mult);      ///< is this field a mult?
  READ(mult_value);             ///< actual value of the mult (if it is a mult)

/*! \brief      Set the name and corresponding mult value
    \param  nm  field name
*/
  void name(const std::string& nm);

/*! \brief      Set the value and corresponding mult value
    \param  v   new value
*/
  void value(const std::string& v);
};

/// ostream << parsed_exchange_field
std::ostream& operator<<(std::ostream& ost, const parsed_exchange_field& pef);

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

/*! \brief                      Try to fill exchange fields with received field matches
    \param  matches             the names of the matching fields, for each received field number
    \param  received_values     the received values
*/
  void _fill_fields(const std::map<int, std::set<std::string>>& matches, const std::vector<std::string>& received_values);

/*! \brief      Print the values of a <int, string, set<string>> tuple to the debug file
    \param  t   the tuple to print
*/
  void _print_tuple(const std::tuple<int, std::string, std::set<std::string>>& t) const;

public:

/*! \brief                      Constructor
    \param  callsign            callsign of the station from which the exchange was received
    \param  rules               rules for the contest
    \param  received_values     the received values, in the order that they were received
*/
  parsed_exchange(const std::string& callsign, const contest_rules& rules, const MODE m, const std::vector<std::string>& received_values);

  READ(replacement_call);              ///< a new callsign, intended to replace the one in the CALL window
  READ(valid);                         ///< is the object valid? (i.e., was parsing successful?)
  READ(fields);                        ///< all the names, values and is_mult() indicators, in the same order as the exchange definition in the configuration file

/// is the object valid? (i.e., was parsing successful?)
  inline const bool is_valid(void) const
    { return valid(); }

/// is a replacement call present?
  inline const bool has_replacement_call(void) const
    { return (!_replacement_call.empty() ); }

/*! \brief              Return the value of a particular field (addressed by name)
    \param  field_name  field for which the value is requested
    \return             value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
  const std::string field_value(const std::string& field_name) const;

/// the number of fields
  inline const size_t n_fields(void) const
    { return _fields.size(); }

/*! \brief      Return the name of a field
    \param  n   number of field for which the name is requested
    \return     name corresponding to <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline const std::string field_name(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].name()); }

/*! \brief      Return the value of a particular field (addressed by number)
    \param  n   number of field for which the value is requested
    \return     value corresponding to field number <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline const std::string field_value(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].value()); }

/*! \brief      Is a field a mult?
    \param  n   number of field for which the mult status is requested
    \return     whether field number <i>n</i> is a mult

    Returns false if <i>n</i> is out of range
*/
  inline const bool field_is_mult(const size_t n) const
    { return (n >= _fields.size() ? false : _fields[n].is_mult()); }

/*! \brief      Return the mult value of a field
    \param  n   number of field for which the mult value is requested
    \return     mult value corresponding to <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline const std::string mult_value(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].mult_value()); }

/*! \brief          Return the names and values of matched fields
    \param  rules   rules for this contest
    \return         the actual matched names and values of the exchange fields

    Any field names that represent a choice are resolved to the name of the actual matched field in the returned object
*/
  const std::vector<parsed_exchange_field> chosen_fields(const contest_rules& rules) const;

/*! \brief                  Given several possible field names, choose one that fits the data
    \param  choice_name     the name of the choice field (e.g., "SOCIETY+ITU_ZONE"
    \param  received_field  the value of the received field
    \return                 the individual name of a field in <i>choice_name</i> that fits the data

    Returns the first field name in <i>choice_name</i> that fits the value of <i>received_field</i>.
    If there is no fit, then returns the empty string.
*/
    const std::string resolve_choice(const std::string& choice_name, const std::string& received_field,  const contest_rules& rules) const;
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

/*! \brief              Guess the value of an exchange field
    \param  callsign    callsign for the guess
    \param  field_name  name of the field for the guess
    \return             Guessed value of <i>field_name</i> for <i>callsign</i>

    Returns empty string if no sensible guess can be made
*/
  const std::string guess_value(const std::string& callsign, const std::string& field_name);

/*! \brief              Set a value in the database
    \param  callsign    callsign for the new entry
    \param  field_name  name of the field for the new entry
    \param  value       the new entry
*/
  void set_value(const std::string& callsign, const std::string& field_name, const std::string& value);

  inline const size_t size(void) const
    { return _db.size(); }
};

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

  std::set<std::string>  _legal_non_regex_values;           ///< all legal values not obtained from a regex
  std::map<std::string, std::string>  _value_to_canonical;  ///< key = value; value = corresponding canonical value

  bool _is_mult;                                            ///< is this field a mult?

public:

/// default constructor
  EFT(void)
    { }

/*! \brief      construct from name
    \param  nm  name

                Assumes not a mult. Object is not ready for use, except to test the name, after this constructor.
*/
  EFT(const std::string& nm);

/*! \brief                  construct from several parameters
    \param  nm              name
    \param  path            path for the regex and values files
    \param  regex_filename  name of file that contains the regex filter
    \param  context         context for the contest
    \param  location_db     location database

    Object is fully ready for use after this constructor.
*/
  EFT(const std::string& nm, const std::vector<std::string>& path,
      const std::string& regex_filename,
      const drlog_context& context, location_database& location_db);

  READ_AND_WRITE(is_mult);              ///< is this field a mult?
  READ_AND_WRITE(name);                 ///< name of exchange field
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

/*! \brief              Parse and incorporate QTHX values from context
    \param  context     context for the contest
    \param  location_db location database
*/
  void parse_context_qthx(const drlog_context& context, location_database& location_db);

  inline const bool is_canonical_value(const std::string& str) const
    { return (_values.find(str)  != _values.end()); }

  void add_canonical_value(const std::string& new_canonical_value);

  void add_legal_value(const std::string& cv, const std::string& new_value);

  void add_legal_values(const std::string& cv, const std::set<std::string>& new_values);

// check regex, then other values
  const bool is_legal_value(const std::string& str) const;

  const std::string value_to_log(const std::string& str) const;

// return canonical value for a received value
  const std::string canonical_value(const std::string& str) const;

// all canonical values
  const std::set<std::string> canonical_values(void) const;
};

std::ostream& operator<<(std::ostream& ost, const EFT& eft);

// -------------------------  sweepstakes_exchange  ---------------------------

/*!     \class sweepstakes_exchange
        \brief Encapsulates an exchange for Sweepstakes

        Sweepstakes is different because:
          1. Two fields might take the form of a two-digit number
          2. A call may be present as part of the exchange
          3. The order may be quite different from the canonical order if part of the exchange has come from drmaster
*/

class sweepstakes_exchange
{
protected:

  std::string _serno;    ///< serial number
  std::string _prec;     ///< precedence
  std::string _call;     ///< callsign
  std::string _check;    ///< check
  std::string _section;  ///< section

public:

  sweepstakes_exchange(const contest_rules& rules, const std::string& callsign, const std::string& received_exchange);

  READ(serno);    ///< serial number
  READ(prec);     ///< precedence
  READ(call);     ///< callsign
  READ(check);    ///< check
  READ(section);  ///< section

  inline const bool valid(void) const
    { return (!_serno.empty() and !_prec.empty() and !_call.empty() and !_check.empty() and !_section.empty() ); }
};

#endif /* EXCHANGE_H */

