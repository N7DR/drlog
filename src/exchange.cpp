// $Id: exchange.cpp 111 2015-07-11 19:49:52Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file exchange.cpp

        Classes and functions related to processing exchanges
*/

#include "cty_data.h"
#include "diskfile.h"
#include "drmaster.h"
#include "exchange.h"
#include "log.h"
#include "string_functions.h"

using namespace boost;
using namespace std;

extern contest_rules     rules;         ///< the rules for this contest
extern drmaster* drm_p;                 ///< pointer to drmaster database
extern EFT CALLSIGN_EFT;                ///< exchange field template for a callsign
extern location_database location_db;   ///< the (global) location database
extern logbook logbk;                   ///< the (global) logbook

pt_mutex exchange_field_database_mutex; ///< mutex for access to the exchange field database

// -------------------------  parsed_exchange_field  ---------------------------

/*! \class parsed_exchange_field
    \brief Encapsulates the name for an exchange field, its value after parsing an exchange, whether it's a mult, and, if so, the value of that mult
*/

/// default constructor
parsed_exchange_field::parsed_exchange_field(void) :
  _name(),
  _value(),
  _is_mult(false),
  _mult_value()
{ }

/*! \brief      Constructor
    \param  nm  field name
    \param  v   field value
    \param  m   is this field a mult?
*/
parsed_exchange_field::parsed_exchange_field(const string& nm, const string& v, const bool m) :
    _name(nm),
    _value(v),
    _is_mult(m),
    _mult_value(MULT_VALUE(nm, v))
{ }

/*! \brief      Set the name and corresponding mult value
    \param  nm  field name
*/
void parsed_exchange_field::name(const string& nm)
{ _name = nm;
  _mult_value = MULT_VALUE(_name, _value);
}

/*! \brief      Set the value and corresponding mult value
    \param  v   new value
*/
void parsed_exchange_field::value(const string& v)
{ _value = v;
  _mult_value = MULT_VALUE(_name, _value);
}

/// ostream << parsed_exchange_field
ostream& operator<<(ostream& ost, const parsed_exchange_field& pef)
{ ost << "  name: " << pef.name() << endl
      << "  value: " << pef.value() << endl
      << "  is_mult: " << pef.is_mult() << endl
      << "  mult_value: " << pef.mult_value() << endl;

  return ost;
}

// -------------------------  parsed_exchange  ---------------------------

/*! \class parsed_exchange
    \brief All the fields in the exchange, following parsing
*/

/*! \brief                      Try to fill exchange fields with received field matches
    \param  matches             the names of the matching fields, for each received field number
    \param  received_values     the received values
*/
void parsed_exchange::_fill_fields(const map<int, set<string>>& matches, const vector<string>& received_values)
{ set<int> matched_field_numbers;
  set<string> matched_field_names;
  decltype(matches) remaining_matches(matches);

  for (unsigned int field_nr = 0; field_nr < remaining_matches.size(); ++field_nr)
  { if (remaining_matches.at(field_nr).size() == 1)  // there is a single match between field number and name
    { const string& matched_field_name = *(remaining_matches.at(field_nr).cbegin());

      for (unsigned int n = 0; n < _fields.size(); ++n)
      { if (_fields[n].name() == matched_field_name)
          _fields[n].value(received_values[n]);
      }
    }
  }
}

/*! \brief      Print the values of a <int, string, set<string>> tuple to the debug file
    \param  t   the tuple to print
*/
void parsed_exchange::_print_tuple(const tuple<int, string, set<string>>& t) const
{ ost << "tuple:" << endl;
  ost << "  field number: " << get<0>(t) << endl;
  ost << "  field value: " << get<1>(t) << endl;

  const set<string>& ss = get<2>(t);

  ost << "  { ";

  for (const auto& s : ss)
    ost << s << " ";

  ost << "}" << endl;
}

/*! \brief                      Constructor
    \param  callsign            callsign of the station from which the exchange was received
    \param  rules               rules for the contest
    \param  received_values     the received values, in the order that they were received
*/
parsed_exchange::parsed_exchange(const std::string& canonical_prefix, const contest_rules& rules, const MODE m, const vector<string>& received_values) :
  _replacement_call(),
  _valid(false)
{ static const string EMPTY_STRING("");
  static bool is_ss = false;                    // Sweepstakes is special
  static bool first_time = true;

  vector<string> copy_received_values(received_values);

  const vector<exchange_field> exchange_template = rules.exch(canonical_prefix, m);

// first time through, determine whether this is Sweepstakes
  if (first_time)
  { for (const auto& ef : exchange_template)
    { if (ef.name() == "PREC")
        is_ss = true;
    }

    first_time = false;
  }

// how many fields are optional?
  unsigned int n_optional_fields = 0;
  FOR_ALL(exchange_template, [&] (const exchange_field& ef) { if (ef.is_optional())
                                                                n_optional_fields++;
                                                            } );

// prepare output; includes optional fields and all choices
  FOR_ALL(exchange_template, [=] (const exchange_field& ef) { _fields.push_back(parsed_exchange_field(ef.name(), EMPTY_STRING, ef.is_mult())); } );

// if there's an explicit . field, use it to replace the call
  for (const auto& received_value : received_values)
  { if (contains(received_value, "."))
      _replacement_call = remove_char(received_value, '.');
  }

  if (!_replacement_call.empty())    // remove the dotted field(s) from the received exchange
  { copy_received_values.clear();
    copy_if(received_values.cbegin(), received_values.cend(), back_inserter(copy_received_values), [] (const string& str) { return !contains(str, "."); } );
  }

// for each received field, which output fields does it match?
  map<int /* received field number */, set<string>> matches;
  const map<string /* field name */, EFT>  exchange_field_eft = rules.exchange_field_eft();  // EFTs have the choices already expanded
  int field_nr = 0;

  for (const string& received_value : copy_received_values)
  { set<string> match;

    for (const auto& field : exchange_template)
    { const string& field_name = field.name();

      try
      { const bool is_choice = contains(field_name, "+");

        if (is_choice)
        { const vector<string> choices_vec = split_string(field_name, '+');
          set<string> choices(choices_vec.cbegin(), choices_vec.cend());

          for (auto it = choices.begin(); it != choices.end(); )    // see Josuttis 2nd edition, p. 343
          { const EFT& eft = exchange_field_eft.at(*it);

            if (eft.is_legal_value(received_value))
            { match.insert(field_name);                 // insert the "+" version of the name
              it = choices.end();
            }
            else
              it++;
          }
        }
        else    // not a choice
        { const EFT& eft = exchange_field_eft.at(field_name);

          if (eft.is_legal_value(received_value))
            match.insert(field_name);
        }
      }

      catch (...)
      { ost << "Error: cannot find field name: " << field_name << endl;
      }
    }

    matches.insert( { field_nr++, match } );
  }

// debug; print status
//  for (const auto& m : matches)
//  { ost << "field nr " << m.first << " [" << copy_received_values[m.first] << "]: ";
//    for (const string& str : m.second)
//      ost << str << "  ";
//
//    ost << endl;
//  }

  deque<tuple<int, string, set<string>>> tuple_deque;

  for (const auto& m : matches)
    tuple_deque.push_back(tuple<int, string, set<string>> { m.first, copy_received_values[m.first], m.second } );

  vector<tuple<int, string, set<string>>> tuple_vector_assignments;
  map<string, tuple<int, string, set<string>>> tuple_map_assignments;
  size_t old_size_of_tuple_deque;

// find entries with only one entry in set
  do
  { old_size_of_tuple_deque = tuple_deque.size();

    for (const auto& t : tuple_deque)
    { if (get<2>(t).size() == 1)
      { const auto it = tuple_map_assignments.find( *(get<2>(t).cbegin()) );

        if (it != tuple_map_assignments.end())
          tuple_map_assignments.erase(it);        // erase any previous entry with this key

        tuple_map_assignments.insert( { *(get<2>(t).cbegin()) /* field name */, t } );
      }
    }

// eliminate matched fields from sets of possible matches
// remove assigned tuples (changes tuple_deque)
    REMOVE_IF_AND_RESIZE(tuple_deque,  [] (tuple<int, string, set<string>>& t) { return (get<2>(t).size() == 1); } );

    for (auto& t : tuple_deque)
    { set<string>& ss = get<2>(t);

      for (const auto& tm : tuple_map_assignments)  // for each one that has been definitively assigned
        ss.erase(tm.first);
    }
  } while (old_size_of_tuple_deque != tuple_deque.size());

  if (tuple_deque.empty())
    _valid = true;

  if (!_valid) // we aren't finished
  { const tuple<int, string, set<string>>& t = tuple_deque[0];    // first received field we haven't been able to use, even tentatively

// find first received field that's a match for any exchange field and that we haven't used
    const auto cit = find_if(exchange_template.cbegin(), exchange_template.cend(), [=] (const exchange_field& ef) { return ( get<2>(t) < ef.name()); } );

    if (cit != exchange_template.cend())
    { const string& field_name = cit->name();    // syntactic sugar
      const bool inserted = (tuple_map_assignments.insert( { field_name, t } )).second;

      if (!inserted)
        ost << "WARNING: Unable to insert map assignment for field: " << field_name << ". This should never happen" << endl;

// remove the tuple we just processed
      tuple_deque.pop_front();

// remove this possible match name from all remaining elements in tuple vector
      FOR_ALL(tuple_deque, [=] (tuple<int, string, set<string>>& t) { get<2>(t).erase(field_name); } );

      do
      { old_size_of_tuple_deque = tuple_deque.size();

        for (const auto& t : tuple_deque)
        { if (get<2>(t).size() == 1)                             // if one element in set
          { const string& field_name = *(get<2>(t).cbegin());    // syntactic sugar
            const auto it = tuple_map_assignments.find( field_name );

            if (it != tuple_map_assignments.end())
              tuple_map_assignments.erase(it);

            tuple_map_assignments.insert( { field_name, t } );    // overwrite any previous entry with this key
          }
        }

// remove assigned tuples from tuple_vector
        REMOVE_IF_AND_RESIZE(tuple_deque,  [] (tuple<int, string, set<string>>& t) { return (get<2>(t).size() == 1); } );

        for (auto& t : tuple_deque)
        { set<string>& ss = get<2>(t);

          for (const auto& tm : tuple_map_assignments)  // for each one that has been definitively assigned
            ss.erase(tm.first);
        }

      } while (old_size_of_tuple_deque != tuple_deque.size());

      if (tuple_deque.empty())
        _valid = true;
    }

    if (!_valid) // we aren't finished -- tuple_vector is not empty
    { FOR_ALL(tuple_deque, [this] (tuple<int, string, set<string>>& t) { _print_tuple(t); } );

      const tuple<int, string, set<string>>& t = tuple_deque[0];
      const auto cit = find_if(exchange_template.cbegin(), exchange_template.cend(), [=] (const exchange_field& ef) { return ( get<2>(t) < ef.name()); } );

      if (cit == exchange_template.cend())
      { if (_replacement_call.empty())  // maybe test for replacement call
        { const bool replace_callsign = CALLSIGN_EFT.is_legal_value(get<1>(t));

          if (replace_callsign)
          { _replacement_call = get<1>(t);

            if (tuple_deque.size() == 1)
              _valid = true;
          }
        }
      }
    }

    if (!_valid)
      ost << "unable to parse exchange" << endl;
  }

// prepare output; includes optional fields and all choices
  for (auto& pef : _fields)
  { const string& name = pef.name();

    try
    { const auto& t = tuple_map_assignments.at(name);

      pef.value(get<1>(t));
    }

    catch (...)
    { const bool is_choice = contains(name, "+");
      bool found_map = false;

      if (is_choice)
      { const vector<string> choices_vec = split_string(name, '+');

        for (unsigned int n = 0; n < choices_vec.size() and !found_map; ++n)
        { try
          { const auto& t = tuple_map_assignments.at(choices_vec[n]);

            pef.value(get<1>(t));
            found_map = true;
          }

          catch (...)
          {
          }
        }
      }

      if (!found_map)
        ost << "WARNING: unable to find map assignment for key = " << name << endl;
    }
  }

// normalize exchange fields to use canonical value, so that we don't mistakenly count each legitimate value more than once in statistics
// this means that we can't use a DOK.values file, because the received DOK will get changed here
  if (_valid)
    FOR_ALL(_fields, [=] (parsed_exchange_field& pef) { pef.value(rules.canonical_value(pef.name(), pef.value())); } );
}

/*! \brief              Return the value of a particular field
    \param  field_name  field for which the value is requested
    \return             value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
const string parsed_exchange::field_value(const std::string& field_name) const
{ for (const auto& field : _fields)
    if (field.name() == field_name)
      return field.value();

  return string();
}

/*! \brief          Return the names and values of matched fields
    \param  rules   rules for this contest
    \return         returns the actual matched names and values of the exchange fields

    Any field names that represent a choice are resolved to the name of the actual matched field in the returned object
*/
const vector<parsed_exchange_field> parsed_exchange::chosen_fields(const contest_rules& rules) const
{ vector<parsed_exchange_field> rv;

  for (const auto& pef : _fields)
  { if (!contains(pef.name(), "+"))
      rv.push_back(pef);
    else
    { parsed_exchange_field pef_chosen = pef;

      pef_chosen.name(resolve_choice(pef.name(), pef.value(), rules));

      if (pef_chosen.name().empty())
      { ost << "ERROR in parsed_exchange::chosen_fields(): empty name for field: " << pef.name() << endl;
      }
      else
        rv.push_back(pef_chosen);
    }
  }

// assign correct mult status
  for (auto& pef : rv)
    pef.is_mult(rules.is_exchange_mult(pef.name()));

  return rv;
}

/*! \brief                  Given several possible field names, choose one that fits the data
    \param  choice_name     the name of the choice field (e.g., "SOCIETY+ITU_ZONE"
    \param  received_field  the value of the received field
    \return                 the individual name of a field in <i>choice_name</i> that fits the data

    Returns the first field name in <i>choice_name</i> that fits the value of <i>received_field</i>.
    If there is no fit, then returns the empty string.
*/
const string parsed_exchange::resolve_choice(const string& field_name, const string& received_value, const contest_rules& rules) const
{ if (field_name.empty())
    return string();

  const bool is_choice = contains(field_name, "+");

  if (!is_choice)
    return field_name;

  const vector<string> choices_vec = split_string(field_name, '+');
  const map<string /* field name */, EFT>  exchange_field_eft = rules.exchange_field_eft();  // EFTs have the choices already expanded

  for (const auto& choice: choices_vec)    // see Josuttis 2nd edition, p. 343
  { try
    { const EFT& eft = exchange_field_eft.at(choice);

      if (eft.is_legal_value(received_value))
        return choice;
    }

    catch (...)
    { ost << "Cannot find EFT for choice: " << choice << endl;
    }
  }

  return string();
}

/// ostream << parsed_exchange
ostream& operator<<(ostream& ost, const parsed_exchange& pe)
{ ost << "parsed exchange object IS " << (pe.valid() ? "" : "NOT ") << "valid" << endl
      << "replacement call = " << pe.replacement_call() << endl;

  const vector<parsed_exchange_field> vec = pe.fields();

  ost << "parsed exchange fields: " << endl;

  for (size_t n = 0; n < vec.size(); ++n)
  { const parsed_exchange_field& pef = vec[n];

    ost << n << ": " << "name: " << pef.name() << ", value: " << pef.value() << ", is mult = " << (pef.is_mult() ? "TRUE" : "FALSE");
    if (n != vec.size() - 1)
      ost << endl;
  }

  return ost;
}

// -------------------------  exchange_field_database  ---------------------------

/*! \class exchange_field_database
    \brief used for estimating the exchange field

    There can be only one of these, and it is thread safe
*/

/*! \brief              Guess the value of an exchange field
    \param  callsign    callsign for the guess
    \param  field_name  name of the field for the guess
    \return             guessed value of <i>field_name</i> for <i>callsign</i>

    Returns empty string if no sensible guess can be made
*/
const string exchange_field_database::guess_value(const string& callsign, const string& field_name)
{ SAFELOCK(exchange_field_database);

// first, check the database
  const auto it = _db.find( pair<string, string>( { callsign, field_name } ) ) ;

  if (it != _db.end())
    return it->second;

// no prior QSO; is it in the drmaster database?
  const drmaster_line drm_line = (*drm_p)[callsign];

  if (field_name == "10MSTATE")
  { string rv;

    if (!drm_line.empty())
    { rv = to_upper(drm_line.state_10());

      if (rv.empty())                       ///< if no explicit 10MSTATE value, try the QTH value
        rv = to_upper(drm_line.qth());
    }

      if (rv.empty() and ( location_db.canonical_prefix(callsign) == "VE") )  // can often guess province for VEs
      { const string pfx = wpx_prefix(callsign);

        if (pfx == "VY2")
          rv = "PE";

        if (rv.empty() and (pfx == "VO1"))
          rv = "NF";

        if (rv.empty())
        { const char call_area = pfx[pfx.length() - 1];

          switch (call_area)
          { case '1' :
              rv = "NS";
              break;

            case '2' :
              rv = "PQ";
              break;

            case '3' :
              rv = "ON";
              break;

            case '4' :
              rv = "MB";
              break;

            case '5' :
              rv = "SK";
              break;

            case '6' :
              rv = "MB";
              break;

            case '7' :
              rv = "BC";
              break;

            case '9' :
              rv = "NB";
              break;

            default :
              break;
          }
        }
      }

      if (!rv.empty())
      { rv = rules.canonical_value("10MSTATE", rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }

  }

  if (field_name == "CHECK")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.check();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "CQZONE")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.cq_zone();

      if (!rv.empty())
      { rv = rules.canonical_value("CQZONE", rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }

// no entry in drmaster database; can we determine from the location database?
    rv = to_string(location_db.cq_zone(callsign));

    if (!rv.empty())    // should always be true
    { _db.insert( { { callsign, field_name }, rv } );

      return rv;
    }
  }

  if (field_name == "CWPOWER")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.cw_power();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "DOK")
  { string rv;

    if (!drm_line.empty() and location_db.canonical_prefix(callsign) == "DL")
    { rv = drm_line.qth();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "FDEPT")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.qth();

      if (!rv.empty())
      { _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "HADXC")     // stupid HA DX membership number is (possibly) in the QTH field of an HA (making it useless for WAHUC)
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.qth();

      if (!rv.empty())
      { _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "ITUZONE")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.itu_zone();

      if (!rv.empty())
      { rv = rules.canonical_value("ITUZONE", rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }

// no entry in drmaster database; can we determine from the location database?
    rv = to_string(location_db.itu_zone(callsign));

    if (!rv.empty())    // should always be true
    { _db.insert( { { callsign, field_name }, rv } );

      return rv;
    }
  }

  if (field_name == "JAPREF")
  { string rv;

    if (!drm_line.empty() and location_db.canonical_prefix(callsign) == "JA")
    { rv = drm_line.qth();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "PREC")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.precedence();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (starts_with(field_name, "QTHX["))  // by the time we get here, the call should match the canoncial prefix in the name of the exchange field
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.qth();

      if (!rv.empty())
      { _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if ((field_name == "RDA") or (field_name == "RD2"))
  { static const set<string> countries { "R1FJ", "UA", "UA2", "UA9" };

    string rv;

    if (!drm_line.empty() and ( (countries < location_db.canonical_prefix(callsign)) or starts_with(callsign, "RI1AN")) )
    { rv = drm_line.qth();

      if (field_name == "RD2" and rv.length() > 2)      // allow for case when full 4-character RDA is in the drmaster file
        rv = substring(rv, 0, 2);

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }

// no entry in drmaster database; can we determine from the location database?
    rv = location_db.region_abbreviation(callsign);

    if (!rv.empty())    // should always be true
    { _db.insert( { { callsign, field_name }, rv } );

      return rv;
    }
  }

  if (field_name == "SOCIETY")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.society();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "SSBPOWER")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.ssb_power();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

// choices
  if (field_name == "ITUZONE+SOCIETY")    // IARU
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.society();

      if (rv.empty())
      { rv = drm_line.itu_zone();

        if (rv.empty())  // no entry in drmaster database; can we determine from the location database?
          rv = to_string(location_db.itu_zone(callsign));

        if (!rv.empty())
          rv = rules.canonical_value(field_name, rv);
      }

      if (!rv.empty())
      { _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

// give up
  static const string empty_string;

  _db.insert( { { callsign, field_name }, empty_string } );  // so we find it next time

  return empty_string;
}

/*! \brief  Set a value in the database
    \param  callsign   callsign for the new entry
    \param  field_name name of the field for the new entry
    \param  value      the new entry
*/
void exchange_field_database::set_value(const string& callsign, const string& field_name, const string& value)
{ SAFELOCK(exchange_field_database);

  _db[ { callsign, field_name } ] = value;    // don't use insert, since we must overwrite
}

/*! \brief              Set value of a field for multiple calls using a file
    \param  path        path for file
    \param  filename    name of file
    \param  field_name  name of the field

    Reads all the lines in the file, which is assumed to be in two columns:
      call  field_value
    Ignores the first line if the upper case version of the call in the first line is "CALL"
    Creates a database entry for calls as necessary
*/
void exchange_field_database::set_values_from_file(const vector<string>& path, const string& filename, const string& field_name)
{ try
  { string contents = read_file(path, filename);

    if (!contents.empty())
    { contents = remove_char(contents, CR_CHAR);        // in case it's a silly Microsoft-format file

      const vector<string> lines = to_lines(to_upper(contents));

      for (unsigned int n = 0; n < lines.size(); ++n)
      { string line = lines[n];
        line = replace_char(line, '\t', ' ');
        line = squash(line, ' ');

        const vector<string> tokens = remove_peripheral_spaces(split_string(line, " "));

        if (tokens.size() == 2)
        { if ( (n == 0) and (tokens[0] == "CALL") )
            continue;

          set_value(tokens[0], field_name, tokens[1]);
        }
      }
    }
  }

  catch (...)
  {
  }
}

#if !defined(NEW_CONSTRUCTOR)
// -------------------------  exchange_field_template  ---------------------------

/*!     \class exchange_field_template
        \brief used for managing and parsing exchange fields
*/

/*! \brief  Fill the database
    \param  path        directories to search (in order) for the filename
    \param  filename    name of the file that holds regex expressions for fields
*/
void exchange_field_template::prepare(const vector<string>& path, const string& filename)
{ if (filename.empty())
    return;

  try
  { const vector<string> lines = to_lines(read_file(path, filename));

    for (const auto& line : lines)
    { if (!line.empty())
      { const vector<string> fields = split_string(line, ":");

        if (fields.size() >= 2)
        { const string field_name = remove_peripheral_spaces(fields[0]);
          const size_t posn = line.find(":");
          const string regex_str = remove_peripheral_spaces(substring(line, posn + 1));
          const regex expression(regex_str);

          _db.insert( { field_name, expression } );
        }
      }
    }
  }

  catch (...)
  { ost << "error trying to read exchange field template file " << filename << endl;
  }
}

/*! \brief  Is a particular received string a valid value for a named field?
    \param  name        name of the exchange field
    \param  str         string to test for validity
*/
const bool exchange_field_template::is_valid(const string& name /* field name */, const string& str)
{ const auto it = _db.find(name);

  if (it == _db.end())
    return false;

  const bool is_match = regex_match(str, it->second);

  return is_match;
}

/*! \brief      What fields are a valid match for a particular received string?
    \param  str string to test
    \return     Names of fields for which <i>str</i> is a valid value
*/
const vector<string> exchange_field_template::valid_matches(const string& str)
{ vector<string> rv;

  for (const auto& db_entry : _db)
  { if (is_valid(db_entry.first, str))
      rv.push_back(db_entry.first);
  }

  return rv;
}

#endif

/*! \brief          Replace cut numbers with real numbers
    \param  input   string possibly containing cut numbers
    \return         <i.input</i> but with cut numbers replaced by actual digits

    Replaces [aA], [nN], [tT]
*/
const string process_cut_digits(const string& input)
{ string rv = input;

  for (char& c : rv)
  { if ((c == 'T') or (c == 't'))
      c = '0';

    if ((c == 'N') or (c == 'n'))
      c = '9';

    if ((c == 'A') or (c == 'a'))
      c = '1';
  }

  return rv;
}

#if !defined(NEW_CONSTRUCTOR)
/*! \brief  Is a string a valid CW power?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of CW power
*/
const bool is_valid_CWPOWER(const string& str, const contest_rules& rules)
  { return is_valid_ANYTHING(str, rules); }

const bool is_valid_RS(const string& str, const contest_rules& rules)
{ if (str.length() != 2)
    return false;

  if ( (str[0] < '1') or (str[0] > '5') )
    return false;

  if ( (str[1] < '1') or (str[1] > '9') )
    return false;

  return true;
}

// for now, include RS in RST, just to get slopbucket working
const bool is_valid_RST(const string& str, const contest_rules& rules)
{ if (str.length() != 3 and str.length() != 2)
    return false;

  if (str.length() == 3)
  { if ( (str[0] < '1') or (str[0] > '5') )
      return false;

    if ( (str[1] < '1') or (str[1] > '9') )
      return false;

    if ( (str[2] < '1') or (str[2] > '9') )
      return false;
  }

  if (str.length() == 3)
//    return true;
    return (str == string("599"));

  return is_valid_RS(str, rules);

//  return true;
}

const bool is_valid_CALLSIGN(const string& str, const contest_rules& rules)
{ if (str.length() < 3)
    return false;

  if (!contains_digit(str))
    return false;

  if (!contains_letter(str))
    return false;

  return true;
}

const bool is_valid_ANYTHING(const string&, const contest_rules& rules)
  { return true; }

const bool is_valid_NOTHING(const string&, const contest_rules& rules)
  { return false; }

const bool is_valid_SERNO(const string& str, const contest_rules& rules)
{ const string str_copy = process_cut_digits(str);
  const size_t posn = str_copy.find_first_not_of("0123456789");

  return (posn == string::npos);
}

const bool is_valid_SOCIETY(const string& str, const contest_rules& rules)
{ const string str_copy = to_upper(str);

  if ( (str_copy == "R1") or (str_copy == "R2") or (str_copy == "R3") )
    return true;

  const size_t posn = str_copy.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

  return (posn == string::npos);
}

const bool is_valid_CQZONE(const string& str, const contest_rules& rules)
{ const string str_copy = process_cut_digits(str);
  const size_t posn = str_copy.find_first_not_of("0123456789");

  if (posn == string::npos)  // string is now all digits
  { const int zone = from_string<int>(str_copy);

    if (zone > 0 and zone <= 40)
      return true;
  }

  return false;
}

/*! \brief  Is a string a valid RDA district?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of RDA district
*/
const bool is_valid_RDA(const string& str, const contest_rules& rules)
{ if (str.length() != 4)
    return false;

  const string str_copy = to_upper(str);
  const size_t posn = str_copy.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  if (posn != 2)
    return false;

  if (!isdigit(str_copy[2]) or !isdigit(str_copy[3]))
    return false;

  return true;
}

/*! \brief  Is a string a valid ITU zone?
    \param  str   the string to test
    \return whether <i>str</i> is a legal value of ITU zone
*/
const bool is_valid_ITUZONE(const string& str, const contest_rules& rules)
{ const string str_copy = process_cut_digits(str);
  const size_t posn = str_copy.find_first_not_of("0123456789");

  if (posn == string::npos)  // string is all digits
  { const unsigned int zone = from_string<unsigned int>(str_copy);

    return (zone > 0 and zone <= 90);
  }

  return false;
}

//const bool (*validity_function(const std::string& field_name))(const std::string&)

/*! \brief  Obtain the validity function corresponding to a particular exchange field name
    \param  field_name   name of the field for which the validity function is requested
    \return the validity function corresponding to <i>field_name</i>
*/
VALIDITY_FUNCTION_TYPE validity_function(const string& field_name, const contest_rules& rules)
{ ost << "setting validity function for field " << field_name << endl;

  if (field_name.length() >= 6 and starts_with(field_name, "QTHX["))
  {
  }

  if (field_name == "CALLSIGN")
    return &is_valid_CALLSIGN;

  if (field_name == "CQZONE")
    return &is_valid_CQZONE;

  if (field_name == "CWPOWER")
    return &is_valid_CWPOWER;

  if (field_name == "ITUZONE")
    return &is_valid_ITUZONE;

  if (field_name == "RDA")
    return &is_valid_RDA;

  if (field_name == "RS")
    return &is_valid_RS;

  if (field_name == "RST")
    return &is_valid_RST;

  if (field_name == "SERNO")
    return &is_valid_SERNO;

  if (field_name == "SOCIETY")
    return &is_valid_SOCIETY;

// some fields must be matched with an explicit value from a .values file
// since we get here only if such a match has failed, return false
  if (field_name == "10MSTATE")
    return &is_valid_NOTHING;

  return &is_valid_ANYTHING;    // default
}
#endif

#if !defined(NEW_CONSTRUCTOR)
/*! \brief  Given several possible field names, choose one that fits the data
    \param  choice_name   the name of the choice field (e.g., "SOCIETY+ITU_ZONE"
    \param  received_field the value of the received field
    \return the individual name of a field in <i>choice_name</i> that fits the data

    Returns the first field name in <i>choice_name</i> that fits the value of <i>received_field</i>.
    If there is no fit, then returns the empty string.
*/
const string parsed_exchange::_resolve_choice(const string& choice_name, const string& received_field, const contest_rules& rules)
{ const vector<string> choices_str = split_string(choice_name, "+");

  for (const auto& choice_str : choices_str)
  { const bool is_valid_1 = (rules.exch_permitted_values(choice_str) < received_field);

    if (is_valid_1)
      return choice_str;

    const bool is_valid = (*validity_function(choice_str, rules))(received_field, rules);

    if (is_valid)
      return choice_str;
  }

  return string();
}
#endif

// -------------------------  EFT  ---------------------------

/*!     \class EFT (exchange_field_template)
        \brief Manage a single exchange field
*/

/*! \brief      construct from name
    \param  nm  name

                Assumes not a mult. Object is not ready for use, except to test the name, after this constructor.
*/
EFT::EFT(const string& nm) :
  _is_mult(false),
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
EFT::EFT(const string& nm, const vector<string>& path, const string& regex_filename,
    const drlog_context& context, location_database& location_db) :
  _is_mult(false),
  _name(nm)
{ read_regex_expression_file(path, regex_filename);
  read_values_file(path, nm);
  parse_context_qthx(context, location_db);

  const vector<string> exchange_mults =  remove_peripheral_spaces(split_string(context.exchange_mults(), ","));

  _is_mult = (find(exchange_mults.cbegin(), exchange_mults.cend(), _name) != exchange_mults.cend());  // correct value of is_mult

 // ost << "constructed EFT: " << (*this) << endl;
}

#if 0
/// is an algorithm defined?
const bool EFT::defined(void) const
{ if (!_values.empty())
    return true;

  if (!_regex_expression.empty())
    return true;

  return false;
}
#endif

/*! \brief              Get regex expression from file
    \param  paths       paths to try
    \param  filename    name of file
    \return             whether a regex expression was read
*/
const bool EFT::read_regex_expression_file(const vector<string>& path, const string& filename)
{ if (filename.empty())
    return false;

  try
  { const vector<string> lines = to_lines(read_file(path, filename));
    bool found_it = false;

    for (const auto& line : lines)
    { if (!found_it and !line.empty())
      { const vector<string> fields = split_string(line, ":");

// a bit complex because ":" may appear in the regex
        if (fields.size() >= 2)
        { const string field_name = remove_peripheral_spaces(fields[0]);
          const size_t posn = line.find(":");
          const string regex_str = remove_peripheral_spaces(substring(line, posn + 1));

          if (field_name == _name)
          { _regex_expression = regex(regex_str);
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

  return (!_regex_expression.empty());
}

/*! \brief              Get info from .values file
    \param  path        paths to try
    \param  filename    name of file (without .values extension)
    \return             whether values were read
*/
const bool EFT::read_values_file(const vector<string>& path, const string& filename)
{ //ost << "EFT reading values file: " << filename << endl;

  try
  { const vector<string> lines = to_lines(read_file(path, filename + ".values"));

    //ost << "Read values file OK: " << lines.size() << " lines" << endl;

    for (const auto& line : lines)
    { set<string> equivalent_values;    // includes the canonical

      if (!line.empty() and line[0] != ';' and !starts_with(line, "//") ) // ";" and "//" introduce comments
      { if (contains(line, "=") )
        { const vector<string> lhsrhs = split_string(line, "=");
          const string lhs = remove_peripheral_spaces(lhsrhs[0]);

          equivalent_values.insert(lhs);                  // canonical value

          if (lhsrhs.size() != 1)
          { const string& rhs = lhsrhs[1];
            const vector<string> remaining_equivalent_values = remove_peripheral_spaces(split_string(rhs, ","));

            COPY_ALL(remaining_equivalent_values, inserter(equivalent_values, equivalent_values.begin()));

            _values.insert( { lhs, equivalent_values });
            add_legal_values(lhs, equivalent_values);
          }
        }
        else    // no "="
        { const string str = remove_peripheral_spaces(line);

          if (!str.empty())
          { _values.insert( { str, /* set<string> */ { str } } );
            add_canonical_value(str);
          }
        }
      }
    }
  }

  catch (...)
  { // ost << "Failed to read values file " << filename << ".values" << endl;
    return false;
  }

  return (!_values.empty());
}

/*! \brief              Parse and incorporate QTHX values from context
    \param  context     context for the contest
    \param  location_db location database
*/
void EFT::parse_context_qthx(const drlog_context& context, location_database& location_db)
{ if (!starts_with(_name, "QTHX["))
    return;

  const auto& context_qthx = context.qthx();  // map; key = canonical prefix; value = set of legal values

  if (context_qthx.empty())
    return;

  for (const auto& this_qthx : context_qthx)
  { const string canonical_prefix = location_db.canonical_prefix(this_qthx.first);

//    ost << "in EFT::parse_context_qthx, name = " << _name << ", canonical_prefix = " << canonical_prefix << endl;

    if (canonical_prefix == location_db.canonical_prefix(delimited_substring(_name, '[', ']')))
    { const set<string> ss = this_qthx.second;

      for (const auto this_value : ss)
      { if (!contains(this_value, "|"))
          add_canonical_value(this_value);
        else                                  // "|" is used to indicate alternative but equivalent values in the configuration file
        { const vector<string> equivalent_values = remove_peripheral_spaces(split_string(this_value, "|"));

          if (!equivalent_values.empty())
            add_canonical_value(equivalent_values[0]);

          for (size_t n = 1; n < equivalent_values.size(); ++n)
            add_legal_value(equivalent_values[0], equivalent_values[n]);
        }
      }
    }
  }
}

#if 0
const string EFT::_equivalent_canonical_value(const string& str) const
{ for (const auto& pss: _values)
  { if (pss.second < str)
      return pss.first;
  }

  return string();
}
#endif

void EFT::add_canonical_value(const string& new_canonical_value)
{ if (!is_canonical_value(new_canonical_value))
    _values.insert( { new_canonical_value, { new_canonical_value } } );

  _legal_non_regex_values.insert(new_canonical_value);
  _value_to_canonical.insert( { new_canonical_value, new_canonical_value } );
}

void EFT::add_legal_value(const string& cv, const string& new_value)
{ if (!is_canonical_value(cv))
    add_canonical_value(cv);

  const auto& it = _values.find(cv);
  auto& ss = it->second;

  ss.insert(new_value);

  _legal_non_regex_values.insert(new_value);
  _value_to_canonical.insert( { new_value, cv } );
}

void EFT::add_legal_values(const string& cv, const set<string>& new_values)
{ FOR_ALL(new_values, [cv, this] (const string& str) { add_legal_value(cv, str); } );
}

const bool EFT::is_legal_value(const string& str) const
{ ost << "testing whether " << str << " is a legal value for field " << name() << endl;
  ost << " ------------ " << endl;

  ost << "is legal value(): regex expression = " << _regex_expression << endl;
  ost << "str = " << str << endl;

  if (!_regex_expression.empty() and regex_match(str, _regex_expression))
  { ost << str << " IS a legal value for " << name() << endl;
    return true;
  }

  if (!_values.empty())
  { ost << "_values is not empty" << endl;
    ost << "  legal values: " << endl;

    for (const auto& v : _legal_non_regex_values )
      ost << v << " ";

    if (_legal_non_regex_values.empty())
      ost << "There are NO legal non-regex values" << endl;

    const bool b = (_legal_non_regex_values < str);

    if (b)
      ost << str << " IS a legal value for " << name() << endl;
    else
      ost << str << " IS NOT a legal value for " << name() << endl;

    return (_legal_non_regex_values < str);
  }

  ost << str << " IS NOT a legal value for " << name() << endl;

  return false;
}

const string EFT::value_to_log(const string& str) const
{ const string rv = canonical_value(str);

  return (rv.empty() ? str : rv);
}

// return canonical value for a received value
const string EFT::canonical_value(const std::string& str) const
{ const auto& it = _value_to_canonical.find(str);

  if (it != _value_to_canonical.cend())
    return it->second;

  if (regex_match(str, _regex_expression))
    return str;

  return string();
}

const set<string> EFT::canonical_values(void) const
{ set<string> rv;

  FOR_ALL(_values, [&rv] (const pair<string, set<string>>& pss) { rv.insert(pss.first); } );

  return rv;
}

ostream& operator<<(ostream& ost, const EFT& eft)
{ ost << "EFT name: " << eft.name() << endl
      << "  is_mult: " << eft.is_mult() << endl
      << "  regex_expression: " << eft.regex_expression() << endl;

//const map<string,                        /* a canonical field value */
//         set                             /* each equivalent value is a member of the set, including the canonical value */
//          <string                       /* indistinguishable legal values */
//          >> values = eft.values();
  const auto values = eft.values();

  for (const auto& sss : values)
  { ost << "  canonical value = " << sss.first << endl;

    const auto& ss = sss.second;

    for (const auto& s : ss)
      ost << "  value = " << s << endl;
  }

  const auto v = eft.legal_non_regex_values();

  if (!v.empty())
  { ost << "  legal_non_regex_values : ";
    for (const auto& str : v)
      ost << "  " << str;

    ost << endl;
  }

  const auto vcv = eft.value_to_canonical();

  if (!vcv.empty())
  { ost << "  v -> cv : " << endl;

    for (const auto& pss : vcv)
      ost << "    " << pss.first << " -> " << pss.second << endl;
  }

//      << endl;

  return ost;
}

//const vector<pair<int /* field number */, string /* value */>> fit_matches(const map<int /* received field number */, set<string>>& matches)
//{ vector<pair<int /* field number */, string /* value */>> rv;
//
//
//
//
//  return rv;
//}

// -------------------------  sweepstakes_exchange  ---------------------------

/*!     \class sweepstakes_exchange
        \brief Encapsulates an exchange for Sweepstakes

        Sweepstakes is different because:
          1. Two fields might take the form of a two-digit number
          2. A call may be present as part of the exchange
          3. The order may be quite different from the canonical order if part of the exchange has come from drmaster
*/

sweepstakes_exchange::sweepstakes_exchange(const contest_rules& rules, const string& callsign, const string& received_exchange) :
  _call(callsign)
{ static EFT check_eft;
  static EFT serno_eft;
  static EFT prec_eft;
  static EFT section_eft;
  static bool first_time = true;

  if (first_time)
  { check_eft = rules.exchange_field_eft("CHECK");
    serno_eft = rules.exchange_field_eft("SERNO");
    prec_eft = rules.exchange_field_eft("PREC");
    section_eft = rules.exchange_field_eft("SECTION");

    first_time = false;
  }

  const vector<string> r_vec = remove_peripheral_spaces(split_string(received_exchange, " "));

//  static const EFT check_eft = rules.exchange_field_eft("CHECK");
//  static const EFT serno_eft = rules.exchange_field_eft("SERNO");
//  static const EFT prec_eft = rules.exchange_field_eft("PREC");


//  const static regex check_regex("^[[:digit:]][[:digit:]]$");
//  const static regex serno_regex("^([[:digit:]])+$");

//  auto is_check = [](const string& target) { return regex_match(target, check_regex); };
//  auto is_serno = [](const string& target) { return regex_match(target, serno_regex); };

  auto is_check = [](const string& target) { return check_eft.is_legal_value(target); };
  auto is_serno = [](const string& target) { return serno_eft.is_legal_value(target); };
  auto is_prec = [](const string& target) { return prec_eft.is_legal_value(target); };
  auto is_section = [](const string& target) { return section_eft.is_legal_value(target); };


}
