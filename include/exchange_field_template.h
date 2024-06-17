// $Id: exchange_field_template.h 234 2024-02-19 15:37:47Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   exchange_field_template.h

    Class for managing exchange fields
*/

#ifndef EFT_H
#define EFT_H

#include "drlog_context.h"

#include <regex>
#include <set>

// -------------------------  EFT  ---------------------------

/*! \class  EFT
    \brief  Manage a single exchange field

    <i>EFT</i> stands for "exchange field template"
*/

class EFT
{
protected:

// There are several ways to define a field:
//  1. regex
//  2. .values file

  bool                                  _is_mult { false };                   ///< is this field a mult?

  std::set<std::string>                 _legal_non_regex_values;    ///< all legal values not obtained from a regex

  std::string                           _name;                      ///< name of exchange field

// we simply store the expresison as a string, because the standard library version of regex is missing
// too many useful functions; in particular, there is no way to test whether a regex has been set [i.e.,
// whether it is equivalent to ""s]
  std::string                           _regex_str;          ///< regex expression to define field

  std::map<std::string,        /* a canonical field value */
          std::set             /* each equivalent value is a member of the set, including the canonical value */
            <std::string       /* indistinguishable legal values */
            >>                          _values;                    ///< the canonical and alternative values for the field

  std::map<std::string, std::string>    _value_to_canonical;        ///< key = value; value = corresponding canonical value

public:

/// default constructor
  EFT(void) = default;

/*! \brief      construct from name
    \param  nm  name

    Assumes not a mult. Object is not ready for use, except to test the name, after this constructor.
*/
  inline explicit EFT(const std::string& nm) :
    _name(nm)
  { }

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
  READ(legal_non_regex_values);         ///< all legal values not obtained from a regex
  READ_AND_WRITE(name);                 ///< name of exchange field
  READ(regex_str);                      ///< regex expression that defines field
  READ(values);                         ///< all the equivalent values, per canonical value
  READ(value_to_canonical);             ///< map of all value->canonical trsnaforms: key = value; value = corresponding canonical value

/*! \brief              Get regex expression from file
    \param  paths       paths to try
    \param  filename    name of file
    \return             whether a regex expression was read
*/
  bool read_regex_expression_file(const std::vector<std::string>& paths, const std::string& filename);

/*! \brief              Get info from .values file
    \param  path        paths to try
    \param  filename    name of file (without .values extension)
    \return             whether values were read
*/
//  bool read_values_file(const std::vector<std::string>& path, const std::string& filename);
  bool read_values_file(const std::vector<std::string>& path, const std::string_view filename);

/*! \brief              Parse and incorporate QTHX values from context
    \param  context     context for the contest
    \param  location_db location database
*/
  void parse_context_qthx(const drlog_context& context, location_database& location_db);

/*! \brief          Is a particular string a canonical value?
    \param  str     string to test
    \return         whether <i>str</i> is a canonical value
*/
  inline bool is_canonical_value(const std::string& str) const
    { return _values.contains(str); }

/*! \brief                          Add a canonical value
    \param  new_canonical_value     string to add

    Does nothing if <i>new_canonical_value</i> is already known
*/
  void add_canonical_value(const std::string& new_canonical_value);

/*! \brief              Add a legal value that corresponds to a canonical value
    \param  cv          canonical value
    \param  new_value   value that correspond to <i>cv</i>

    Does nothing if <i>new_value</i> is already known. Adds <i>cv</i> as a
    canonical value if necessary.
*/
  void add_legal_value(const std::string& cv, const std::string& new_value);

/*! \brief              Add legal values that corresponds to a canonical value
    \param  cv          canonical value
    \param  new_values  values that correspond to <i>cv</i>

    Adds <i>cv</i> as a canonical value if necessary. Ignores any elements of
    <i>cv</i> that are already known as being equivalent to <i>cv</i>.
*/
  inline void add_legal_values(const std::string& cv, const std::set<std::string>& new_values)
    { FOR_ALL(new_values, [&cv, this] (const std::string& str) { add_legal_value(cv, str); } ); }

/*! \brief          Is a string a legal value?
    \param  str     string to test
    \return         whether <i>str</i> is a legal value
*/
  bool is_legal_value(const std::string& str) const;

/*! \brief          What value should actually be logged for a given received value?
    \param  str     received value
    \return         value to be logged
*/
  std::string value_to_log(const std::string& str) const;

/*! \brief          Obtain canonical value corresponding to a given received value?
    \param  str     received value
    \return         canonical value equivalent to <i>str</i>

    Returns empty string if no equivalent canonical value can be found
*/
  std::string canonical_value(const std::string& str) const;

/// all the canonical values
  std::set<std::string> canonical_values(void) const;

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _is_mult
       & _legal_non_regex_values
       & _name
       & _regex_str
       & _values
       & _value_to_canonical;
  }
};

/*! \brief          Write an <i>EFT</i> object to an output stream
    \param  ost     output stream
    \param  eft     object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const EFT& eft);

#endif        // EFT_H
