// $Id: exchange.cpp 147 2018-04-20 21:32:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   exchange.cpp

    Classes and functions related to processing exchanges
*/

#include "cty_data.h"
#include "diskfile.h"
#include "drmaster.h"
#include "exchange.h"
#include "log.h"
#include "string_functions.h"

#include <array>

using namespace boost;                  // for regex
using namespace std;

extern contest_rules     rules;         ///< the rules for this contest
extern drmaster* drm_p;                 ///< pointer to drmaster database
extern EFT CALLSIGN_EFT;                ///< exchange field template for a callsign
extern location_database location_db;   ///< the (global) location database
extern logbook logbk;                   ///< the (global) logbook

pt_mutex exchange_field_database_mutex; ///< mutex for access to the exchange field database

static const set<char> legal_prec { 'A', 'B', 'M', 'Q', 'S', 'U' };     ///< legal values of the precedence for Sweepstakes

// -------------------------  parsed_exchange_field  ---------------------------

/*! \class  parsed_exchange_field
    \brief  Encapsulates the name for an exchange field, its value after parsing an exchange, whether it's a mult, and, if so, the value of that mult
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

/*! \brief          Write a <i>parsed_exchange_field</i> object to an output stream
    \param  ost     output stream
    \param  pef     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const parsed_exchange_field& pef)
{ ost << "  name: " << pef.name() << endl
      << "  value: " << pef.value() << endl
      << "  is_mult: " << pef.is_mult() << endl
      << "  mult_value: " << pef.mult_value() << endl;

  return ost;
}

// -------------------------  parsed_ss_exchange  ---------------------------

/*! \class  parsed_ss_exchange
    \brief  All the fields in the SS exchange, following parsing
*/

/*! \brief          Does a string possibly contain a serial number?
    \param  str     string to check
    \return         whether <i>str</i> contains a possible serial number

    Currently returns true only for strings of the form:
      <i>n</i>
      <i>n</i><i>precedence</i>
*/
const bool parsed_ss_exchange::_is_possible_serno(const string& str) const
{ if (!contains_digit(str))
    return false;

  bool possible = true;

// check all except the last character
  for (size_t n = 0; n < str.length() - 1; ++n)
    if (possible)
      possible = (isdigit(str[n]));

// now do the last character, which might be a digit or a precedence
  if (possible)
  { const char& lchar = last_char(str);

    possible = isdigit(lchar) or (legal_prec < lchar);
  }

  return possible;
}

/*! \brief          Does a string contain a precedence?
    \param  str     string to check
    \return         whether <i>str</i> contains a precedence

    Currently returns true only for strings of the form:
      <i>precedence</i>
      <i>n</i><i>precedence</i>
*/
const bool parsed_ss_exchange::_is_possible_prec(const string& str) const
{ //if (str.length() == 1)
  //  return (legal_prec < last_char(str));

  //return ( _is_possible_serno(str) and (legal_prec < last_char(str)) );
  return ( (str.length() == 1) ? (legal_prec < last_char(str)) : (_is_possible_serno(str) and (legal_prec < last_char(str))) );
}

/*! \brief          Does a string contain a possible check?
    \param  str     string to check
    \return         whether <i>str</i> is a (two-digit) check
*/
const bool parsed_ss_exchange::_is_possible_check(const string& str) const
{ if (str.length() != 2)
    return false;

  { for (size_t n = 0; n < str.length() - 1; ++n)
      if (!isdigit(str[n]))
        return false;

    return true;
  }

  return ( isdigit(str[0]) and isdigit(str[1]) );
}

/*! \brief          Does a string contain a possible callsign?
    \param  str     string to check
    \return         whether <i>str</i> is a reasonable callsign
*/
const bool parsed_ss_exchange::_is_possible_callsign(const string& str) const
{ //if (str.length() < 3)
  //  return false;

  //return (isalpha(str[0]) and contains_digit(str));

  return ( (str.length() < 3) ? false : ( isalpha(str[0]) and contains_digit(str) ) );
}

/*! \brief                      Constructor
    \param  call                callsign
    \param  received_fields     separated strings from the exchange
*/
parsed_ss_exchange::parsed_ss_exchange(const string& call, const vector<string>& received_fields) :
  _callsign(call),
  _prec('Z'),
  _check("XX"),
  _serno(0),
  _section("AAA")
{ if (received_fields.size() < 3)                    // at least 3 fields are required (<n><prec> <check> <sec>)
    return;

  vector<string> copy_received_fields = received_fields;
  bool is_special = false;

// deal with: B 71 CO 10 N7DR
// which might be a common case

  if (received_fields[0].length() == 1 and isalpha(received_fields[0][0]))
  { if (received_fields[1].length() == 2 and isdigit(received_fields[1][0]) and isdigit(received_fields[1][1]))
    { _prec = received_fields[0][0];
      _check = received_fields[1];
      copy_received_fields = vector<string>(received_fields.begin() + 2, received_fields.end());
      is_special = true;
    }
  }

//  count the number fields that might be, or might contain, a serial number
// i.e., all digits, or digits followed by a single letter
  vector<size_t> possible_sernos;
  size_t index = 0;

  for (const auto& field : copy_received_fields)
  { if (_is_possible_serno(field))
      possible_sernos.push_back(index);

    index++;
  }

  vector<size_t> possible_prec;
  index = 0;

  for (const auto& field : copy_received_fields)
  { if (_is_possible_prec(field))
      possible_prec.push_back(index);

    index++;
  }

  vector<size_t> possible_check;
  index = 0;

  for (const auto& field : copy_received_fields)
  { if (_is_possible_check(field))
      possible_check.push_back(index);

    index++;
  }

  vector<size_t> possible_callsigns;
  index = 0;

  for (const auto& field : copy_received_fields)
  { if (_is_possible_callsign(field))
      possible_callsigns.push_back(index);

    index++;
  }

// calculate number of entries that might be a check or a serial number
  vector<size_t> ambiguous_fields;

  for (const auto& possible_check_field : possible_check)
    if (find(possible_sernos.cbegin(), possible_sernos.cend(), possible_check_field) != possible_sernos.cend())
      ambiguous_fields.push_back(possible_check_field);

  if (possible_prec.empty() and (_prec == 'Z') )  // _prec unchanged from default
  { ost << "ERROR: no possible precedence in exchange received from " << call << endl;
    for (const auto& field : received_fields)
      ost << field << " : " << endl;
  }
  else
  { if (!possible_prec.empty())
    { const unsigned int field_nr = possible_prec[possible_prec.size() - 1];

      _prec = last_char(copy_received_fields[field_nr]);
    }
  }

// get the check; for this use the last field that is a possible check
  unsigned int check_field_nr = numeric_limits<unsigned int>::max();

  if (possible_check.empty() and (_check == "XX") )  // _check unchanged from default
  { ost << "ERROR: no possible check in exchange received from " << call << endl;
    for (const auto& field : received_fields)
      ost << field << " : " << endl;
  }
  else
  { if (!possible_check.empty() /* and !is_special */)
    { if (is_special)
      {  // deal with B 71 CO 10 N7DR 72
        switch (ambiguous_fields.size())
        { case 1 :  // if there's only one, it should be a serial number
            break;
          default :
            ost << "Too many ambiguous fields: " << ambiguous_fields.size() << "; doing my best" << endl; // NB no break
          case 2 :  // first should be serial number, second should check
          { const unsigned int field_nr = ambiguous_fields[1];

            _check = copy_received_fields[field_nr];
            check_field_nr = static_cast<int>(field_nr);
            break;
          }
        }
      }
      else
      { const unsigned int field_nr = possible_check[possible_check.size() - 1];

        _check = copy_received_fields[field_nr];
        check_field_nr = static_cast<int>(field_nr);
      }
    }
  }

// get the callsign, which may or may not be present
  ost << "getting callsign" << endl;

  if (possible_callsigns.empty())
    _callsign = call;    // default
  else
  { const unsigned int field_nr = possible_callsigns[possible_callsigns.size() - 1];

    _callsign = copy_received_fields[field_nr];
  }

// get the serno; for this use the last field that is a possible serno and hasn't been used as a check
  ost << "getting serno" << endl;

  if (possible_sernos.empty())
  { ost << "ERROR: no possible serno in exchange received from " << call << endl;
    for (const auto& field : received_fields)
      ost << field << " : " << endl;
  }
  else
  { unsigned int field_nr = possible_sernos[possible_sernos.size() - 1];

    ost << "serno field number = " << field_nr << endl;
    ost << "check field number = " << check_field_nr << endl;

    if (field_nr == check_field_nr)    // what abt if check_field_nr == -1?
    { if (possible_sernos.size() == 1)
      { ost << "ERROR: insufficient possible sernos in exchange received from " << call << endl;
        for (const auto& field : received_fields)
          ost << field << " : " << endl;
        _serno = 0;    // default
      }
      else
      { field_nr = possible_sernos[possible_sernos.size() - 2];

// if this is also a possible check number, we should go backwards some more if possible
        if (_is_possible_check(copy_received_fields[field_nr]))
        { ost << "moving backwards" << endl;

          if (possible_sernos.size() >= 3)
            field_nr = possible_sernos[possible_sernos.size() - 3];
          ost << "field number for serno = " << field_nr << endl;
        }

        _serno = from_string<unsigned int>(copy_received_fields[field_nr]);
      }
    }
    else    // field number is not same as check field number
    { _serno = from_string<unsigned int>(copy_received_fields[field_nr]);  // stops processing when hits a letter
    }
  }

// get the section
  ost << "getting section" << endl;
  map<string /* field name */, EFT>  exchange_field_eft = rules.exchange_field_eft();  // EFTs have the choices already expanded

//  vector<size_t> possible_sections;
  index = 0;

  try
  { const EFT sec_eft = exchange_field_eft.at("SECTION");
    int section_field_nr = -1;

    for (const auto& field : copy_received_fields)
    { if (sec_eft.is_legal_value(field))
        section_field_nr = index;

      index++;
    }

    if (section_field_nr == -1)
    { ost << "ERROR: no valid section in exchange received from " << call << endl;
      for (const auto& field : received_fields)
        ost << field << " : " << endl;
    }
    else
      _section = copy_received_fields[section_field_nr];
  }

  catch (...)
  { ost << "ERROR: no section information in rules " << endl;
  }
}

/*! \brief          Write a <i>parsed_ss_exchange</i> object to an output stream
    \param  ost     output stream
    \param  pse     object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const parsed_ss_exchange& pse)
{ ost << "serno: " << pse.serno()
      << ", prec: " << pse.prec()
      << ", callsign: " << pse.callsign()
      << ", check: " << pse.check()
      << ", section: " << pse.section();

  return ost;
}

// -------------------------  parsed_exchange  ---------------------------

/*! \class  parsed_exchange
    \brief  All the fields in the exchange, following parsing
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

/*! \brief                              Constructor
    \param  from_callsign               callsign of the station from which the exchange was received
    \param  canonical_prefix            canonical prefix for <i>callsign</i>
    \param  rules                       rules for the contest
    \param  m                           mode
    \param  received_values             the received values, in the order that they were received
    \param  truncate_received_values    whether to stop parsing when matches have all been found  *** IS THIS EVER USED? ***
*/
parsed_exchange::parsed_exchange(const string& from_callsign, const string& canonical_prefix, const contest_rules& rules, const MODE m, const vector<string>& received_values, const bool truncate_received_values) :
  _replacement_call(),
  _valid(false)
{ static const string EMPTY_STRING("");

  extern bool is_ss;                    // Sweepstakes is special; this is set to the correct value by drlog.cpp

  vector<string> copy_received_values(received_values);

  const vector<exchange_field> exchange_template = rules.unexpanded_exch(canonical_prefix, m);

  if (is_ss)                // SS is oh-so-special
  { const parsed_ss_exchange exch(from_callsign, received_values);

    if (exch.callsign() != from_callsign)
      _replacement_call = exch.callsign();

    _fields.push_back( { "SERNO", to_string(exch.serno()), false } );
    _fields.push_back( { "PREC", create_string(exch.prec()), false } );
    _fields.push_back( { "CALL", exch.callsign(), false } );
    _fields.push_back( { "CHECK", exch.check(), false } );
    _fields.push_back( parsed_exchange_field("SECTION", exch.section(), true) );    // convert section to canonical form if necessary

    FOR_ALL(_fields, [=] (parsed_exchange_field& pef) { pef.value(rules.canonical_value(pef.name(), pef.value())); } );

    _valid = ( (exch.serno() != 0) and (exch.section() != "AAA") and (exch.prec() != 'Z') );

    return;
  }

// how many fields are optional?
  set<string> optional_field_names;

  FOR_ALL(exchange_template, [&] (const exchange_field& ef) { if (ef.is_optional())
                                                              { //n_optional_fields++;
                                                                optional_field_names.insert(ef.name());
                                                              }
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

  if (truncate_received_values)
  { map<string /* field name */, EFT>  exchange_field_eft = rules.exchange_field_eft();  // EFTs have the choices already expanded

    ost << "exchange_field_eft map: " << exchange_field_eft << endl;

// remove any inappropriate RS(T)
    auto pred_fn = [m] (pair<const string, EFT>& psE) { return ( ( psE.first == "RST" and m != MODE_CW )  or
                                                                      ( psE.first == "RS" and m != MODE_SSB )
                                                                    );
                                                      };

    REMOVE_IF_AND_RESIZE(exchange_field_eft, pred_fn);

    ost << "NEW exchange_field_eft map: " << exchange_field_eft << endl;

//  std::map<std::string /* canonical prefix */, std::set<std::string> /* exchange field names */>  _per_country_exchange_fields;

    const vector<string> exchange_field_names = rules.unexpanded_exchange_field_names(canonical_prefix, m);

    for (const auto& efn : exchange_field_names)
      ost << " exchange field for " << canonical_prefix << " from rules: " << efn << endl;

    map<string /* field name */, string /* value */> result_map;

    const string rv0 = received_values[0];
    bool found_match = false;

    for (auto cit = exchange_field_names.cbegin(); !found_match and cit != exchange_field_names.cend(); ++cit)
    { const string exchange_field_name = *cit;

      if (contains(exchange_field_name, "+"))    // if a choice
      { const vector<string> choices_vec = split_string(exchange_field_name, '+');

        for (auto it = choices_vec.begin(); it != choices_vec.end(); )    // see Josuttis 2nd edition, p. 343
        { const EFT& eft = exchange_field_eft.at(*it);

          if (eft.is_legal_value(rv0))
          { result_map[*it] = rv0;
            found_match = true;
            it = choices_vec.end();
          }
          else
            it++;
        }
      }
      else        // not a choice
      { const EFT& eft = exchange_field_eft.at(exchange_field_name);

        if (eft.is_legal_value(rv0))
        { result_map[exchange_field_name] = rv0;
          found_match = true;
        }
      }
    }

    if (!found_match)
    { ost << "Error: cannot find match for exchange field: " << rv0 << endl;
    //alert("No match for exchange field: " + rv0);
    }
  }
  else        // !truncate received values
  {
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

//    ost << "preparing output for field: " << pef << endl;

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

      if (!found_map and !(optional_field_names < name))
        ost << "WARNING: unable to find map assignment for key = " << name << endl;
    }
  }

// normalize exchange fields to use canonical value, so that we don't mistakenly count each legitimate value more than once in statistics
// this means that we can't use a DOK.values file, because the received DOK will get changed here
    if (_valid)
      FOR_ALL(_fields, [=] (parsed_exchange_field& pef) { pef.value(rules.canonical_value(pef.name(), pef.value())); } );

  }  // end of !truncate received values
}

/*! \brief              Return the value of a particular field
    \param  field_name  field for which the value is requested
    \return             value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
const string parsed_exchange::field_value(const string& field_name) const
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
  { if (!contains(pef.name(), "+"))             // not a CHOICE
      rv.push_back(pef);
    else                                        // is a CHOICE
    { parsed_exchange_field pef_chosen = pef;

      pef_chosen.name(resolve_choice(pef.name(), pef.value(), rules));

      if (pef_chosen.name().empty())
        ost << "ERROR in parsed_exchange::chosen_fields(): empty name for field: " << pef.name() << endl;
      else
        rv.push_back(pef_chosen);
    }
  }

// assign correct mult status
  FOR_ALL(rv, [=] (parsed_exchange_field& pef) { pef.is_mult(rules.is_exchange_mult(pef.name())); } );

  return rv;
}

/*! \brief                  Given several possible field names, choose one that fits the data
    \param  choice_name     the name of the choice field (e.g., "SOCIETY+ITU_ZONE"
    \param  received_field  the value of the received field
    \param  rules           rules for this contest
    \return                 the individual name of a field in <i>choice_name</i> that fits the data

    Returns the first field name in <i>choice_name</i> that fits the value of <i>received_field</i>.
    If there is no fit, then returns the empty string.
*/
const string parsed_exchange::resolve_choice(const string& field_name, const string& received_value, const contest_rules& rules) const
{ if (field_name.empty())
    return string();

  if (!contains(field_name, "+"))   // if not a CHOICE
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

/*! \brief          Write a <i>parsed_exchange</i> object to an output stream
    \param  ost     output stream
    \param  pe      object to write
    \return         the output stream
*/
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

/*! \class  exchange_field_database
    \brief  used for estimating the exchange field

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

// if it's a QTHX, then don't go any further if the country doesn't match
  if (starts_with(field_name, "QTHX["))
  { const string canonical_prefix = delimited_substring(field_name, '[', ']');

    if (canonical_prefix != location_db.canonical_prefix(callsign))
    { _db.insert( { { callsign, field_name }, string() } );                     // so that it can be found immediately in future
      return string();
    }
  }

// no prior QSO; is it in the drmaster database?
  const drmaster_line drm_line = (*drm_p)[callsign];

  auto get_qth = [&] (void /* const drmaster_line& drm_line */)
    { if (drm_line.empty())
        return string();

      const string rv = drm_line.qth();

      if (rv.empty())
        return string();

      _db.insert( { { callsign, field_name }, rv } );

      return rv;
    };

// currently identical to 10MSTATE, except look up different value on the drmaster line
  if (field_name == "160MSTATE")
  { string rv;

    if (!drm_line.empty())
    { rv = to_upper(drm_line.state_160());

      if (rv.empty())                       ///< if no explicit 160MSTATE value, try the QTH value
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

        if (isdigit(call_area))
        { static const std::array<string, 10> abbreviations { { string(), "NS", "PQ", "ON", "MB", "SK", "AB", "BC", string(), "NB"} };  // std:: qualifier needed because we have boost here as well

          rv = abbreviations[call_area - '0'];    // convert to number
        }
      }
    }

    if (!rv.empty())
    { rv = rules.canonical_value("160MSTATE", rv);
      _db.insert( { { callsign, field_name }, rv } );

      return rv;
    }
  }

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
      { const char call_area_c = pfx[pfx.length() - 1];

        if (isdigit(call_area_c))
        { static const std::array<string, 10> abbreviations { { string(), "NS", "PQ", "ON", "MB", "SK", "AB", "BC", string(), "NB"} };  // std:: qualifier needed because we have boost here as well

          rv = abbreviations[call_area_c - '0'];    // convert to number
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
      { _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }
  }

  if (field_name == "FDEPT")
  { const string rv = get_qth();

    if (!rv.empty())
      return rv;
  }

  if (field_name == "GRID")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.grid();

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
    { rv = rules.canonical_value("ITUZONE", rv);
      _db.insert( { { callsign, field_name }, rv } );

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

  if (starts_with(field_name, "QTHX["))  // by the time we get here, the call should match the canonical prefix in the name of the exchange field
  { const string canonical_prefix = delimited_substring(field_name, '[', ']');

    if (canonical_prefix != location_db.canonical_prefix(callsign))
    { ost << "Failure to match callsign with canonical prefix in exchange_field_database::guess_value(); field name = " <<  field_name << ", callsign = " << callsign << endl;
      return string();
    }

    string rv;

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

  if (field_name == "SECTION")
  { string rv;

    if (!drm_line.empty())
    { rv = to_upper(drm_line.section());

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
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

  if (field_name == "UKEICODE")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.qth();

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

/*! \brief              Set a value in the database
    \param  callsign    callsign for the new entry
    \param  field_name  name of the field for the new entry
    \param  value       the new entry
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

/*! \brief          Replace cut numbers with real numbers
    \param  input   string possibly containing cut numbers
    \return         <i>input</i> but with cut numbers replaced by actual digits

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

// -------------------------  EFT  ---------------------------

/*! \class  EFT
    \brief  Manage a single exchange field

        <i>EFT</i> stands for "exchange field template"
*/

/*! \brief      construct from name
    \param  nm  name

                Assumes not a mult. Object is not ready for use, except to test the name, after this constructor.
*/
EFT::EFT(const string& nm) :
  _is_mult(false),
  _name(nm)
{ }

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
  _is_mult(false),
  _name(nm)
{ read_regex_expression_file(path, regex_filename);
  read_values_file(path, nm);
  parse_context_qthx(context, location_db);

  const vector<string> exchange_mults =  remove_peripheral_spaces(split_string(context.exchange_mults(), ","));

  _is_mult = (find(exchange_mults.cbegin(), exchange_mults.cend(), _name) != exchange_mults.cend());  // correct value of is_mult
}

/*! \brief              Get regex expression from file
    \param  paths       paths to try
    \param  filename    name of file
    \return             whether a regex expression was read
*/
const bool EFT::read_regex_expression_file(const vector<string>& paths, const string& filename)
{ if (filename.empty())
    return false;

  try
  { const vector<string> lines = to_lines(read_file(paths, filename));
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
{ try
  { const vector<string> lines = to_lines(read_file(path, filename + ".values"));

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
  { return false;
  }

  return (!_values.empty());
}

/*! \brief                  Parse and incorporate QTHX values from context
    \param  context         context for the contest
    \param  location_db     location database
*/
void EFT::parse_context_qthx(const drlog_context& context, location_database& location_db)
{ if (!starts_with(_name, "QTHX["))
    return;

  const auto& context_qthx = context.qthx();  // map; key = canonical prefix; value = set of legal values

  if (context_qthx.empty())
    return;

  for (const auto& this_qthx : context_qthx)
  { const string canonical_prefix = location_db.canonical_prefix(this_qthx.first);

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

/*! \brief                          Add a canonical value
    \param  new_canonical_value     string to add

    Does nothing if <i>new_canonical_value</i> is already known
*/
void EFT::add_canonical_value(const string& new_canonical_value)
{ if (!is_canonical_value(new_canonical_value))
    _values.insert( { new_canonical_value, { new_canonical_value } } );

  _legal_non_regex_values.insert(new_canonical_value);
  _value_to_canonical.insert( { new_canonical_value, new_canonical_value } );
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

  const auto& it = _values.find(cv);
  auto& ss = it->second;

  ss.insert(new_value);

  _legal_non_regex_values.insert(new_value);
  _value_to_canonical.insert( { new_value, cv } );
}

/*! \brief          Is a string a legal value?
    \param  str     string to test
    \return         whether <i>str</i> is a legal value
*/
const bool EFT::is_legal_value(const string& str) const
{
// test regex first
  if (!_regex_expression.empty() and regex_match(str, _regex_expression))
    return true;

  if (!_values.empty())
    return (_legal_non_regex_values < str);

  return false;
}

/*! \brief          What value should actually be logged for a given received value?
    \param  str     received value
    \return         value to be logged
*/
const string EFT::value_to_log(const string& str) const
{ const string rv = canonical_value(str);

  return (rv.empty() ? str : rv);
}

/*! \brief          Obtain canonical value corresponding to a given received value?
    \param  str     received value
    \return         canonical value equivalent to <i>str</i>

    Returns empty string if no equivalent canonical value can be found
*/
const string EFT::canonical_value(const std::string& str) const
{ const auto& it = _value_to_canonical.find(str);

  if (it != _value_to_canonical.cend())
    return it->second;

  if (regex_match(str, _regex_expression))
    return str;

  return string();
}

/// all the canonical values
const set<string> EFT::canonical_values(void) const
{ set<string> rv;

  FOR_ALL(_values, [&rv] (const pair<string, set<string>>& pss) { rv.insert(pss.first); } );

  return rv;
}

/// ostream << EFT
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

  return ost;
}

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

//  auto is_check = [](const string& target) { return check_eft.is_legal_value(target); };
//  auto is_serno = [](const string& target) { return serno_eft.is_legal_value(target); };
//  auto is_prec = [](const string& target) { return prec_eft.is_legal_value(target); };
//  auto is_section = [](const string& target) { return section_eft.is_legal_value(target); };


}
