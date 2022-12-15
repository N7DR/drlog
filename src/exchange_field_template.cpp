// $Id: exchange_field_template.cpp 205 2022-04-24 16:05:06Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   exchange_field_template.cpp

    Class for managing exchange fields
*/

#include "exchange_field_template.h"

using namespace boost;                  // for regex
using namespace std;

// -------------------------  EFT  ---------------------------

/*! \class  EFT
    \brief  Manage a single exchange field

    <i>EFT</i> stands for "exchange field template"
*/

/*! \brief                  Construct from several parameters
    \param  nm              name
    \param  path            path for the regex and values files
    \param  regex_filename  name of file that contains the regex filter
    \param  context         context for the contest
    \param  location_db     location database

    Object is fully ready for use after this constructor.
*/
EFT::EFT(const string& nm, const vector<string>& path, const string& regex_filename,
    const drlog_context& context, location_database& location_db) :
  _name(nm)
{ read_regex_expression_file(path, regex_filename);
  read_values_file(path, nm);
  parse_context_qthx(context, location_db);

  _is_mult = contains(clean_split_string(context.exchange_mults(), ','), _name);  // correct value of is_mult
}

/*! \brief              Get regex expression from file
    \param  paths       paths to try
    \param  filename    name of file
    \return             whether a regex expression was read
*/
bool EFT::read_regex_expression_file(const vector<string>& paths, const string& filename)
{ if (filename.empty())
    return false;

  bool found_it { false };

  try
  { for (const auto& line : to_lines(read_file(paths, filename)))
    { if (!found_it and !line.empty())
      { const vector<string> fields { split_string(line, ':') };

// a bit complex because ":" may appear in the regex
        if (fields.size() >= 2)
        { const string field_name { remove_peripheral_spaces(fields[0]) };
          const size_t posn       { line.find(':') };
          const string regex_str  { remove_peripheral_spaces(substring(line, posn + 1)) };

          if ( (field_name == _name) and !regex_str.empty() )
          { _regex_str = regex_str;
            found_it = true;
          }
        }
      }
    }
  }

  catch (...)
  { ost << "error trying to read exchange field template file " << filename << endl;
    return false;
  }

  return found_it;
}

/*! \brief              Get info from .values file
    \param  path        paths to try
    \param  filename    name of file (without .values extension)
    \return             whether values were read
*/
bool EFT::read_values_file(const vector<string>& path, const string& filename)
{ try
  { for (const auto& line : to_lines(read_file(path, filename + ".values"s)))
    { set<string> equivalent_values;                    // includes the canonical value

      if (!line.empty() and line[0] != ';' and !line.starts_with("//"s)) // ";" and "//" introduce comments
      { if (contains(line, '=') )
        { const vector<string> lhsrhs { split_string(line, '=') };
          const string         lhs    { remove_peripheral_spaces(lhsrhs[0]) };

          equivalent_values += lhs;                  // canonical value

          if (lhsrhs.size() != 1)
          { const string& rhs { lhsrhs[1] };

            COPY_ALL(clean_split_string(rhs), inserter(equivalent_values, equivalent_values.begin()));

            _values += { lhs, equivalent_values };
            add_legal_values(lhs, equivalent_values);
          }
        }
        else    // no "="
        { if (const string str { remove_peripheral_spaces(line) }; !str.empty())
          { _values += { str, { str } };
            add_canonical_value(str);
          }
        }
      }
    }
  }

  catch (...)
  { return false;
  }

  return (!_values.empty());
}

/*! \brief                  Parse and incorporate QTHX values from context
    \param  context         context for the contest
    \param  location_db     location database
*/
void EFT::parse_context_qthx(const drlog_context& context, location_database& location_db)
{ if (!_name.starts_with("QTHX["s))
    return;

  const auto& context_qthx { context.qthx() };  // map; key = canonical prefix; value = set of legal values

  if (context_qthx.empty())
    return;

  for (const auto& this_qthx : context_qthx)
  { const string canonical_prefix { location_db.canonical_prefix(this_qthx.first) };

    if (canonical_prefix == location_db.canonical_prefix(delimited_substring(_name, '[', ']', DELIMITERS::DROP)))
    { const set<string>& ss { this_qthx.second };

      for (const auto& this_value : ss)
      { if (!contains(this_value, '|'))
          add_canonical_value(this_value);
        else                                  // "|" is used to indicate alternative but equivalent values in the configuration file
        { const vector<string> equivalent_values { clean_split_string(this_value, '|') };

          if (!equivalent_values.empty())
            add_canonical_value(equivalent_values[0]);

          for (size_t n { 1 }; n < equivalent_values.size(); ++n)
            add_legal_value(equivalent_values[0], equivalent_values[n]);
        }
      }
    }
  }
}

/*! \brief                          Add a canonical value
    \param  new_canonical_value     string to add

    Does nothing if <i>new_canonical_value</i> is already known
*/
void EFT::add_canonical_value(const string& new_canonical_value)
{ if (!is_canonical_value(new_canonical_value))
    _values += { new_canonical_value, { new_canonical_value } };

  _legal_non_regex_values += new_canonical_value;
  _value_to_canonical += { new_canonical_value, new_canonical_value };
}

/*! \brief              Add a legal value that corresponds to a canonical value
    \param  cv          canonical value
    \param  new_value   value that correspond to <i>cv</i>

    Does nothing if <i>new_value</i> is already known. Adds <i>cv</i> as a
    canonical value if necessary.
*/
void EFT::add_legal_value(const string& cv, const string& new_value)
{ if (!is_canonical_value(cv))
    add_canonical_value(cv);

  const auto& it { _values.find(cv) };  // this is now guaranteed not to be end()

  auto& ss { it->second };

  ss += new_value;      // add it to _values

  _legal_non_regex_values += new_value;
  _value_to_canonical += { new_value, cv };
}

/*! \brief          Is a string a legal value?
    \param  str     string to test
    \return         whether <i>str</i> is a legal value
*/
bool EFT::is_legal_value(const string& str) const
{
// test regex first
  if (!_regex_str.empty() and regex_match(str, regex(_regex_str)))
    return true;

//  if (!_values.empty())
//    return (_legal_non_regex_values > str);
//    return _legal_non_regex_values.contains(str);
//    return (str < _legal_non_regex_values);

//  return false;

  return (_values.empty() ? false : (str < _legal_non_regex_values));

}

/*! \brief          What value should actually be logged for a given received value?
    \param  str     received value
    \return         value to be logged
*/
string EFT::value_to_log(const string& str) const
{ const string rv { canonical_value(str) };

  return (rv.empty() ? str : rv);
}

/*! \brief          Obtain canonical value corresponding to a given received value
    \param  str     received value
    \return         canonical value equivalent to <i>str</i>

    Returns empty string if no equivalent canonical value can be found
*/
string EFT::canonical_value(const std::string& str) const
{ const string canonical { MUM_VALUE(_value_to_canonical, str) };

  if (!canonical.empty())
    return canonical;

  return ( regex_match(str, regex(_regex_str)) ? str : string() );  // by defn, a regex match is a canonical value
}

/// all the canonical values
set<string> EFT::canonical_values(void) const
{ set<string> rv;

  FOR_ALL(_values, [&rv] (const pair<string, set<string>>& pss) { rv += pss.first; } );

  return rv;
}

/// ostream << EFT
ostream& operator<<(ostream& ost, const EFT& eft)
{ ost << "EFT name: " << eft.name() << endl
      << "  is_mult: " << eft.is_mult() << endl
      << "  regex_str: " << eft.regex_str() << endl;

//const map<string,                        /* a canonical field value */
//         set                             /* each equivalent value is a member of the set, including the canonical value */
//          <string                       /* indistinguishable legal values */
//          >> values = eft.values();
//  const auto values { eft.values() };

//  for (const auto& sss : values)
  for (const auto& sss : eft.values())
  { ost << "  canonical value = " << sss.first << endl;

    const auto& ss { sss.second };

    for (const auto& s : ss)
      ost << "  value = " << s << endl;
  }

  const auto v { eft.legal_non_regex_values() };

  if (!v.empty())
  { ost << "  legal_non_regex_values : ";
    for (const auto& str : v)
      ost << "  " << str;

    ost << endl;
  }

  const auto vcv { eft.value_to_canonical() };

  if (!vcv.empty())
  { ost << "  v -> cv : " << endl;

    for (const auto& pss : vcv)
      ost << "    " << pss.first << " -> " << pss.second << endl;
  }

  return ost;
}
