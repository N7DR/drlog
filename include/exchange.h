// $Id: exchange.h 170 2020-10-26 16:44:33Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   exchange.h

    Classes and functions related to processing exchanges
*/

#ifndef EXCHANGE_H
#define EXCHANGE_H

#include "macros.h"
#include "pthread_support.h"
#include "rules.h"

#include <string>
#include <vector>

class exchange_field_template;                  ///< forward declaration

extern pt_mutex exchange_field_database_mutex;  ///< mutex for the exchange field database

using TRIPLET = std::tuple<int                   /* field number wrt 0 */,
                           std::string           /* received value */,
                           std::set<std::string> /* unassigned field names */>;   ///< used in parsed_exchange

const std::set<char> legal_prec { 'A', 'B', 'M', 'Q', 'S', 'U' };     ///< legal values of the precedence for Sweepstakes

// -------------------------  exchange_field_prefill  ---------------------------

/*! \class  exchange_field_prefill
    \brief  Encapsulates external prefills for exchange fields
*/

class exchange_field_prefill
{
protected:

  std::map<std::string /* field-name */, std::unordered_map<std::string /* callsign */, std::string /* value */>> _db;  ///< all values are upper case

public:

/// default constructor
  inline exchange_field_prefill(void) = default;

/*! \brief                          Constructor
    \param  prefill_filename_map    map of fields to filenames
*/
  inline explicit exchange_field_prefill(const std::map<std::string /* field name */, std::string /* filename */>& prefill_filename_map)
    { insert_prefill_filename_map(prefill_filename_map); }

  READ(db);                                 ///< all the data

/*! \brief                          Populate with data taken from a prefill filename map
    \param  prefill_filename_map    map of fields to filenames
*/
  void insert_prefill_filename_map(const std::map<std::string, std::string>& prefill_filename_map);

/*! \brief              Do prefill data exist for a particular field name?
    \param  field_name  field name to test
    \return             whether prefill data exist for the field <i>field_name</i>
*/
  inline bool prefill_data_exists(const std::string& field_name) const
    { return ( _db.empty() ? false : (_db.count(field_name) == 1) ); }

/*! \brief              Get the prefill data for a particular field name and callsign
    \param  field_name  field name to test
    \param  callsign    callsign to test
    \return             the prefill data for the field <i>field_name</i> and callsign <i>callsign</i>

    Returns the empty string if there are no prefill data for the field <i>field_name</i> and
    callsign <i>callsign</i>
*/
  std::string prefill_data(const std::string& field_name, const std::string& callsign) const;
};

/// ostream << exchange_field_prefill
std::ostream& operator<<(std::ostream& ost, const exchange_field_prefill& epf);

// -------------------------  parsed_exchange_field  ---------------------------

/*! \class  parsed_exchange_field
    \brief  Encapsulates the name for an exchange field, its value after parsing an exchange, and whether it's a mult
*/

class parsed_exchange_field
{
protected:

  std::string    _name       { };           ///< field name
  std::string    _value      { };           ///< field value
  bool           _is_mult    { false };     ///< is this field a mult?
  std::string    _mult_value { };           ///< actual value of the mult (if it is a mult)

public:

/// default constructor
  parsed_exchange_field(void) = default;

/*! \brief      Constructor
    \param  nm  field name
    \param  v   field value
    \param  m   is this field a mult?
*/
  inline parsed_exchange_field(const std::string& nm, const std::string& v, const bool m) :
    _name(nm),
    _value(v),
    _is_mult(m),
    _mult_value(MULT_VALUE(nm, v))
  { }

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

/*! \brief          Write a <i>parsed_exchange_field</i> object to an output stream
    \param  ost     output stream
    \param  pef     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const parsed_exchange_field& pef);

// -------------------------  parsed_ss_exchange  ---------------------------

/*! \class  parsed_ss_exchange
    \brief  All the fields in the SS exchange, following parsing
*/

class parsed_ss_exchange
{
protected:

  unsigned int _serno;          ///< serial number
  char         _prec;           ///< precedence
  std::string  _callsign;       ///< callsign
  std::string  _check;          ///< check (2 digits)
  std::string  _section;        ///< section

/*! \brief          Does a string possibly contain a serial number?
    \param  str     string to check
    \return         whether <i>str</i> contains a possible serial number

    Currently returns true only for strings of the form:
      <i>n</i>
      <i>n</i><i>precedence</i>
*/
  bool _is_possible_serno(const std::string& str) const;

/*! \brief          Does a string possibly contain a precedence?
    \param  str     string to check
    \return         whether <i>str</i> contains a possible serial precedence

    Currently returns true only for strings of the form:
      <i>precedence</i>
      <i>n</i><i>precedence</i>
*/
  inline bool _is_possible_prec(const std::string& str) const
    { return ( (str.length() == 1) ? (legal_prec > last_char(str)) : (_is_possible_serno(str) and (legal_prec > last_char(str))) ); }

/*! \brief          Does a string possibly contain a check?
    \param  str     string to check
    \return         whether <i>str</i> is a (two-digit) check
*/
  inline bool _is_possible_check(const std::string& str) const
    { return ( (str.length() == 2) ? ( isdigit(str[0]) and isdigit(str[1]) ) : false ); }

/*! \brief          Does a string contain a possible callsign?
    \param  str     string to check
    \return         whether <i>str</i> is a reasonable callsign
*/
  inline bool _is_possible_callsign(const std::string& str) const
    { return ( (str.length() < 3) ? false : ( isalpha(str[0]) and contains_digit(str) ) ); }

public:

/*! \brief                      Constructor
    \param  call                callsign
    \param  received_fields     separated strings from the exchange
*/
  parsed_ss_exchange(const std::string& call, const std::vector<std::string>& received_fields);

  READ(serno);          ///< serial number
  READ(prec);           ///< precedence
  READ(callsign);       ///< callsign
  READ(check);          ///< check
  READ(section);        ///< section
};

/*! \brief          Write a <i>parsed_ss_exchange</i> object to an output stream
    \param  ost     output stream
    \param  pse     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const parsed_ss_exchange& pse);

// -------------------------  parsed_exchange  ---------------------------

/*! \class  parsed_exchange
    \brief  All the fields in the exchange, following parsing
*/

class parsed_exchange
{
protected:

  std::vector<parsed_exchange_field>    _fields;              ///< all the names, values and is_mult() indicators, in the same order as the exchange definition in the configuration file
  std::string                           _replacement_call;    ///< a new callsign, to replace the one in the CALL window
  bool                                  _valid;               ///< is the object valid? (i.e., was parsing successful?)

/*! \brief                          Assign all the received fields that match a single exchange field
    \param  unassigned_tuplest      all the unassigned fields
    \param  tuple_map_assignmens    the assignments
*/
  void _assign_unambiguous_fields(std::deque<TRIPLET>& unassigned_tuples, std::map<std::string, TRIPLET>& tuple_map_assignments);

/*! \brief                      Try to fill exchange fields with received field matches
    \param  matches             the names of the matching fields, for each received field number
    \param  received_values     the received values

    THIS IS CURRENTLY UNUSED
*/
  void _fill_fields(const std::map<int, std::set<std::string>>& matches, const std::vector<std::string>& received_values);

/*! \brief      Print the values of a <int, string, set<string>> tuple to the debug file
    \param  t   the tuple to print
*/
  void _print_tuple(const std::tuple<int, std::string, std::set<std::string>>& t) const;

public:

/*! \brief                              Constructor
    \param  from_callsign               callsign of the station from which the exchange was received
    \param  canonical_prefix            canonical prefix for <i>callsign</i>
    \param  rules                       rules for the contest
    \param  m                           mode
    \param  received_values             the received values, in the order that they were received
    \param  truncate_received_values    whether to stop parsing when matches have all been found  *** IS THIS EVER USED WITH THE VALUE <i>TRUE</i>? ***
*/
  parsed_exchange(const std::string& from_callsign, const std::string& canonical_prefix, const contest_rules& rules, const MODE m, const std::vector<std::string>& received_values /* , const bool truncate_received_values = false */);

  READ(fields);                        ///< all the names, values and is_mult() indicators, in the same order as the exchange definition in the configuration file
  READ(replacement_call);              ///< a new callsign, intended to replace the one in the CALL window
  READ(valid);                         ///< is the object valid? (i.e., was parsing successful?)

/// is the object valid? (i.e., was parsing successful?)
  inline bool is_valid(void) const
    { return valid(); }

/// is a replacement call present?
  inline bool has_replacement_call(void) const
    { return (!_replacement_call.empty() ); }

/*! \brief              Return the value of a particular field (addressed by name)
    \param  field_name  field for which the value is requested
    \return             value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
  std::string field_value(const std::string& field_name) const;

/// the number of fields
  inline size_t n_fields(void) const
    { return _fields.size(); }

/*! \brief      Return the name of a field
    \param  n   number of field for which the name is requested
    \return     name corresponding to <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline std::string field_name(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].name()); }

/*! \brief      Return the value of a particular field (addressed by number)
    \param  n   number of field for which the value is requested
    \return     value corresponding to field number <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline std::string field_value(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].value()); }

/*! \brief      Is a field a mult?
    \param  n   number of field for which the mult status is requested
    \return     whether field number <i>n</i> is a mult

    Returns false if <i>n</i> is out of range
*/
  inline bool field_is_mult(const size_t n) const
    { return (n >= _fields.size() ? false : _fields[n].is_mult()); }

/*! \brief      Return the mult value of a field
    \param  n   number of field for which the mult value is requested
    \return     mult value corresponding to <i>n</i>

    Returns empty string if <i>n</i> is out of range
*/
  inline std::string mult_value(const size_t n) const
    { return (n >= _fields.size() ? std::string() : _fields[n].mult_value()); }

/*! \brief          Return the names and values of matched fields
    \param  rules   rules for this contest
    \return         the actual matched names and values of the exchange fields

    Any field names that represent a choice are resolved to the name of the actual matched field in the returned object
*/
  std::vector<parsed_exchange_field> chosen_fields(const contest_rules& rules) const;

/*! \brief                  Given several possible field names, choose one that fits the data
    \param  choice_name     the name of the choice field (e.g., "SOCIETY+ITU_ZONE"
    \param  received_field  the value of the received field
    \param  rules           rules for this contest
    \return                 the individual name of a field in <i>choice_name</i> that fits the data

    Returns the first field name in <i>choice_name</i> that fits the value of <i>received_field</i>.
    If there is no fit, then returns the empty string.
*/
  std::string resolve_choice(const std::string& choice_name, const std::string& received_field,  const contest_rules& rules) const;
};

/*! \brief          Write a <i>parsed_exchange</i> object to an output stream
    \param  ost     output stream
    \param  pe      object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const parsed_exchange& pe);

// -------------------------  exchange_field_database  ---------------------------

/*! \class  exchange_field_database
    \brief  used for estimating the exchange field

    There can be only one of these, and it is thread safe
*/

class exchange_field_database
{
protected:

  std::map< std::pair< std::string /* callsign */, std::string /* field name */>, std::string /* value */>  _db;  ///< the actual database
//          --------------------------------  key  -----------------------------  --------  value  -------

public:

/*! \brief              Guess the value of an exchange field
    \param  callsign    callsign for the guess
    \param  field_name  name of the field for the guess
    \return             Guessed value of <i>field_name</i> for <i>callsign</i>

    Returns empty string if no sensible guess can be made
*/
  std::string guess_value(const std::string& callsign, const std::string& field_name);

/*! \brief              Set a value in the database
    \param  callsign    callsign for the new entry
    \param  field_name  name of the field for the new entry
    \param  value       the new entry
*/
  void set_value(const std::string& callsign, const std::string& field_name, const std::string& value);

/*! \brief              Set value of a field for multiple calls using a file
    \param  path        path for file
    \param  filename    name of file
    \param  field_name  name of the field

    Reads all the lines in the file, which is assumed to be in two columns:
      call  field_value
    Ignores the first line if the upper case version of the call in the first line is "CALL"
    Creates a database entry for calls as necessary
*/
  void set_values_from_file(const std::vector<std::string>& path, const std::string& filename, const std::string& field_name);

/// return number of calls in the database
  inline size_t size(void) const
    { return _db.size(); }
};

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

/*! \brief                      Constructor
    \param  rules               rules for this contest
    \param  callsign            callsign to which the exchange is to be attributed
    \param  received_exchange   exchange as received
*/
  sweepstakes_exchange(const contest_rules& rules, const std::string& callsign, const std::string& received_exchange);

  READ(serno);    ///< serial number
  READ(prec);     ///< precedence
  READ(call);     ///< callsign
  READ(check);    ///< check
  READ(section);  ///< section

/// does an instantiated object appear to be valid?
  inline bool valid(void) const
    { return (!_serno.empty() and !_prec.empty() and !_call.empty() and !_check.empty() and !_section.empty() ); }
};

#endif /* EXCHANGE_H */
