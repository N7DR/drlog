// $Id: exchange.cpp 270 2025-05-26 01:09:07Z  $

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
#include "exchange_field_template.h"
#include "log.h"
#include "string_functions.h"

#include <array>

using namespace std;

extern contest_rules           rules;                               ///< the rules for this contest
extern EFT                     CALLSIGN_EFT;                        ///< exchange field template for a callsign
extern location_database       location_db;                         ///< the (global) location database
extern logbook                 logbk;                               ///< the (global) logbook
extern exchange_field_prefill  prefill_data;                        ///< exchange prefill data from external files
extern bool                    require_dot_in_replacement_call;     ///< whether a dot is required to mark a replacement callsign

extern const drmaster& drm_cdb;                                     ///< const version of the drmaster database

pt_mutex exchange_field_database_mutex { "EXCHANGE FIELD DATABASE"s }; ///< mutex for access to the exchange field database

// -------------------------  exchange_field_prefill  ---------------------------

/*! \class  exchange_field_prefill
    \brief  Encapsulates external prefills for exchange fields
*/

/*! \brief                          Populate with data taken from a prefill filename map
    \param  prefill_filename_map    map of fields to filenames
*/
void exchange_field_prefill::insert_prefill_filename_map(const STRING_MAP<string /* filename */>& prefill_filename_map)
{ for (const auto& this_pair : prefill_filename_map)
  { const string& field_name { this_pair.first };
    string_view   filename   { truncate_before_first <std::string_view> (this_pair.second, ':') };  // ":" is used to define the columns to read, if they aren't the first two

    try
    { UNORDERED_STRING_MAP<string /* prefill value */> call_value_map;  // key = call

// figure out the columns to be read; column numbers in the config file are wrt 1
      unsigned int call_column  { 0 };
      unsigned int field_column { 1 };

      if (contains(this_pair.second, ':'))
      { const vector<string_view> fields { split_string <std::string_view> (this_pair.second, ':') };

        if (fields.size() != 3)
        { ost << "Error in config file when defining prefill file: incorrect number of colons" << endl;
          exit(-1);
        }

        call_column = from_string<unsigned int>(fields[1]) - 1;      // adjust to wrt 0
        field_column = from_string<unsigned int>(fields[2]) - 1;     // adjust to wrt 0
      }

      const vector<string> lines { to_lines <std::string> ( to_upper( squash( replace_char( remove_char(read_file(filename), CR_CHAR ), '\t', ' ') ) ) ) }; // read, remove CRs, tabs to spaces, squash, to lines

      for (const auto& line : lines)                                // each line should now be space-separated columns
        if (const vector<string> this_pair { split_string <std::string> (line, ' ') }; this_pair.size() > max(call_column, field_column))
          call_value_map += { this_pair.at(call_column), this_pair.at(field_column) };

      _db += { to_upper(field_name), call_value_map };
    }

    catch (...)
    { ost << "ERROR CREATING PREFILL INFORMATION FROM FILE: " << filename << endl;
    }
  }
}

/*! \brief              Get the prefill data for a particular field name and callsign
    \param  field_name  field name to test
    \param  callsign    callsign to test
    \return             the prefill data for the field <i>field_name</i> and callsign <i>callsign</i>

    Returns the empty string if there are no prefill data for the field <i>field_name</i> and
    callsign <i>callsign</i>
*/
//string exchange_field_prefill::prefill_data(const string& field_name, const string_view callsign) const
string exchange_field_prefill::prefill_data(const string_view field_name, const string_view callsign) const
{ const auto opt { OPT_MUM_VALUE(_db, field_name) };

  return ( opt ? MUM_VALUE(opt.value(), callsign) : string { } );
}

/// ostream << exchange_field_prefill
ostream& operator<<(ostream& ost, const exchange_field_prefill& epf)
{
//  std::map<std::string /* field-name */, std::unordered_map<std::string /* callsign */, std::string /* value */>> _db;  ///< all values are upper case

  const auto& db { epf.db() };

  ost << "Number of field names = " << db.size() << endl;

  for (const auto& element : db)
  { ost << "Field name = " << element.first << endl;

    const auto& um { element.second };

    ost << "  Number of callsigns = " << um.size() << endl;

    for (const auto& ss : um)
      ost << "    Callsign: " << ss.first << "; Value: " << ss.second << endl;
  }

  return ost;
}

// -------------------------  parsed_exchange_field  ---------------------------

/*! \class  parsed_exchange_field
    \brief  Encapsulates the name for an exchange field, its value after parsing an exchange, whether it's a mult, and, if so, the value of that mult
*/

/*! \brief      Set the name and corresponding mult value
    \param  nm  field name
*/
//void parsed_exchange_field::name(const string& nm)
void parsed_exchange_field::name(const string_view nm)
{ _name = string { nm };
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
//bool parsed_ss_exchange::_is_possible_serno(const string& str) const
bool parsed_ss_exchange::_is_possible_serno(const string_view str) const
{ if (!contains_digit(str))
    return false;

  bool rv { true };

// check all except the last character
  for (size_t n { 0 }; n < str.length() - 1; ++n) // note the "<" and -1
    if (rv)
      rv = (isdigit(str[n]));

// now do the last character, which might be a digit or a precedence
  if (rv)
  { const char lchar { last_char(str) };

//    rv = isdigit(lchar) or (legal_prec > lchar);
    rv = isdigit(lchar) or legal_prec.contains(lchar);
  }

  return rv;
}

/*! \brief                      Constructor
    \param  call                callsign
    \param  received_fields     separated strings from the exchange
*/
//parsed_ss_exchange::parsed_ss_exchange(const string& call, const vector<string>& received_fields) :
parsed_ss_exchange::parsed_ss_exchange(const string_view call, const vector<string>& received_fields) :
  _callsign(call)
{ using INDEX_TYPE = uint8_t;

  if (received_fields.size() < 3)                    // at least 3 fields are required (<n><prec> <check> <sec>)
    return;

  vector<string> copy_received_fields { received_fields };
  bool           is_special           { false };

// deal with: B 71 CO 10 N7DR
// which might be a common case

  if ( (received_fields[0].length() == 1) and isalpha(received_fields[0][0]) )
  { if ( (received_fields[1].length() == 2) and isdigit(received_fields[1][0]) and isdigit(received_fields[1][1]))
    { _prec = received_fields[0][0];
      _check = received_fields[1];
      copy_received_fields = vector<string>(received_fields.cbegin() + 2, received_fields.cend());
      is_special = true;
    }
  }

// count the number fields that might be, or might contain, a serial number
// i.e., all digits, or digits followed by a single letter
  vector<INDEX_TYPE> possible_sernos;
  vector<INDEX_TYPE> possible_prec;
  vector<INDEX_TYPE> possible_check;
  vector<INDEX_TYPE> possible_callsigns;

  INDEX_TYPE index { 0 };

  for (const auto& field : copy_received_fields)
  { if (_is_possible_serno(field))
      possible_sernos += index;

    if (_is_possible_prec(field))
      possible_prec += index;

    if (_is_possible_check(field))
      possible_check += index;

    if (_is_possible_callsign(field))
      possible_callsigns += index;

    index++;
  }

// calculate number of entries that might be a check or a serial number
  vector<INDEX_TYPE> ambiguous_fields;

  for (const auto& possible_check_field : possible_check)
    if (contains(possible_sernos, possible_check_field))
      ambiguous_fields += possible_check_field;

  if (possible_prec.empty() and (_prec == DEFAULT_PREC) )  // _prec unchanged from default
  { ost << "ERROR: no possible precedence in exchange received from " << call << endl;
    for (const auto& field : received_fields)
      ost << field << " : " << endl;
  }
  else
  { if (!possible_prec.empty())
    { const unsigned int field_nr { possible_prec[possible_prec.size() - 1] };

      _prec = last_char(copy_received_fields[field_nr]);
    }
  }

// get the check; for this use the last field that is a possible check
  unsigned int check_field_nr { numeric_limits<unsigned int>::max() };

  if (possible_check.empty() and (_check == "XX"s) )  // _check unchanged from default
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

          case 2 :  // first should be serial number, second should check
          { const unsigned int field_nr { ambiguous_fields[1] };

            _check = copy_received_fields[field_nr];
            check_field_nr = static_cast<int>(field_nr);
            break;
          }

          default :
          { ost << "Too many ambiguous fields: " << ambiguous_fields.size() << "; doing my best" << endl; // NB no break; treat as 2

            { const unsigned int field_nr { ambiguous_fields[1] };

              _check = copy_received_fields[field_nr];
              check_field_nr = static_cast<int>(field_nr);
              break;
            }
          }
        }
      }
      else
      { const unsigned int field_nr { possible_check[possible_check.size() - 1] };

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
  { const size_t field_nr { possible_callsigns[possible_callsigns.size() - 1] };

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
  { unsigned int field_nr { possible_sernos[possible_sernos.size() - 1] };

    ost << "serno field number = " << field_nr << endl;
    ost << "check field number = " << check_field_nr << endl;

    if (field_nr == check_field_nr)    // what abt if check_field_nr == -1?
    { if (possible_sernos.size() == 1)
      { ost << "ERROR: insufficient possible sernos in exchange received from " << call << endl;

        for (const auto& field : received_fields)
          ost << field << " : " << endl;
        _serno = DEFAULT_SERNO;
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

        _serno = from_string<decltype(_serno)>(copy_received_fields[field_nr]);
      }
    }
    else    // field number is not same as check field number
    { _serno = from_string<decltype(_serno)>(copy_received_fields[field_nr]);  // stops processing when hits a letter
    }
  }

// get the section
  ost << "getting section" << endl;

  STRING_MAP<EFT> exchange_field_eft { rules.exchange_field_eft() };  // EFTs have the choices already expanded; key = field name

  index = 0;

  try
  { const EFT sec_eft { exchange_field_eft.at("SECTION"s) };

    int section_field_nr { -1 };

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

// I spent a lot of time trying to create a class that was a drop-in replacement for the
// tuple<int, string, set<string>> that is used when parsing the exchange, and eventually
// decided to cut my losses and give up.

// the following hack provides a little syntactic sugar
#define FIELD_NUMBER get<0>
#define RECEIVED_VALUE get<1>
#define FIELD_NAMES get<2>

// -------------------------  parsed_exchange  ---------------------------

/*! \class  parsed_exchange
    \brief  All the fields in the exchange, following parsing
*/

/*! \brief                      Try to fill exchange fields with received field matches
    \param  matches             the names of the matching fields, for each received field number
    \param  received_values     the received values

    THIS IS CURRENTLY UNUSED
*/
#if 0
void parsed_exchange::_fill_fields(const map<int, set<string>>& matches, const vector<string>& received_values)
{ set<int>          matched_field_numbers;
  set<string>       matched_field_names;
  decltype(matches) remaining_matches(matches);

  for (unsigned int field_nr { 0 }; field_nr < remaining_matches.size(); ++field_nr)
  { if (remaining_matches.at(field_nr).size() == 1)         // there is a single match between field number and name
    { const string& matched_field_name { *(remaining_matches.at(field_nr).cbegin()) };

      for (unsigned int n { 0 }; n < _fields.size(); ++n)
      { if (_fields[n].name() == matched_field_name)
          _fields[n].value(received_values[n]);
      }
    }
  }
}
#endif

/*! \brief      Print the values of a <int, string, set<string>> tuple to the debug file
    \param  t   the tuple to print
*/
void parsed_exchange::_print_tuple(const tuple<int, string, STRING_SET>& t) const
{ ost << "tuple:" << endl;
  ost << "  field number: " << get<0>(t) << endl;
  ost << "  field value: " << get<1>(t) << endl;

  const STRING_SET& ss { get<2>(t) };

  ost << "  { ";

  for (const auto& s : ss)
    ost << s << " ";

  ost << "}" << endl;
}

/*! \brief                          Assign all the received fields that match a single exchange field
    \param  unassigned_tuples       all the unassigned fields
    \param  tuple_map_assignmens    the assignments
*/
void parsed_exchange::_assign_unambiguous_fields(deque<TRIPLET>& unassigned_tuples, STRING_MAP<TRIPLET>& tuple_map_assignments)
{ size_t old_size_of_tuple_deque;

  do
  { old_size_of_tuple_deque = unassigned_tuples.size();  // this is changed if an assignment is made

    for (const auto& t : unassigned_tuples)
    { if (FIELD_NAMES(t).size() == 1)
      { const string& field_name { *(FIELD_NAMES(t).cbegin()) };    // syntactic sugar

        tuple_map_assignments -= field_name;        // erase any previous entry with this key
        tuple_map_assignments += { field_name, t };
      }
    }

// eliminate matched fields from sets of possible matches
// remove assigned tuples (changes tuple_deque)
    erase_if(unassigned_tuples,  [] (TRIPLET& t) { return (FIELD_NAMES(t).size() == 1); } );

    for (auto& t : unassigned_tuples)
    { STRING_SET& ss { FIELD_NAMES(t) };

      FOR_ALL(tuple_map_assignments, [&ss] (const auto& tm) { ss -= tm.first; });  // for each one that has been definitively assigned
    }
  } while (old_size_of_tuple_deque != unassigned_tuples.size());
}

/*! \brief                              Constructor
    \param  from_callsign               callsign of the station from which the exchange was received
    \param  canonical_prefix            canonical prefix for <i>callsign</i>
    \param  rules                       rules for the contest
    \param  m                           mode
    \param  received_values             the received values, in the order that they were received
    \param  truncate_received_values    whether to stop parsing when matches have all been found  *** IS THIS EVER USED? ***
*/
parsed_exchange::parsed_exchange(const string& from_callsign, const string& canonical_prefix, const contest_rules& rules, const MODE m, const vector<string>& received_values) :
  _replacement_call(),
  _valid(false)
{ static const string EMPTY_STRING { };

  extern bool is_ss;                    // Sweepstakes is special; this is set to the correct value by drlog.cpp

  vector<string> copy_received_values(received_values);

  const vector<exchange_field> exchange_template { rules.unexpanded_exch(canonical_prefix, m) };

  if (is_ss)                // SS is oh-so-special
  { const parsed_ss_exchange exch(from_callsign, received_values);

    if (exch.callsign() != from_callsign)
      _replacement_call = exch.callsign();

    _fields += { "SERNO"s, to_string(exch.serno()), false };
    _fields += { "PREC"s, create_string(exch.prec()), false };
    _fields += { "CALL"s, exch.callsign(), false };
    _fields += { "CHECK"s, exch.check(), false };
    _fields += { "SECTION"s, exch.section(), true };    // convert section to canonical form if necessary

    FOR_ALL(_fields, [rules] (parsed_exchange_field& pef) { pef.value(rules.canonical_value(pef.name(), pef.value())); } );

    _valid = ( (exch.serno() != 0) and (exch.section() != "AAA"s) and (exch.prec() != DEFAULT_PREC) );
    return;
  }

// how many fields are optional?
  STRING_SET optional_field_names;

  FOR_ALL(exchange_template, [&optional_field_names] (const exchange_field& ef) { if (ef.is_optional())
                                                                                    optional_field_names += ef.name();
                                                                                } );

// prepare output; includes optional fields and all choices  
  FOR_ALL(exchange_template, [this] (const exchange_field& ef) { _fields += { ef.name(), EMPTY_STRING, ef.is_mult() }; } );

// if there's an explicit . field, use it to replace the call
  FOR_ALL(received_values, [this] (const auto& received_value) { if (contains(received_value, '.'))
                                                                   _replacement_call = remove_char(received_value, '.');
                                                               });

  if (!_replacement_call.empty())    // remove the dotted field(s) from the received exchange
  { copy_received_values.clear();
    ranges::copy_if(received_values, back_inserter(copy_received_values), [] (const string& str) { return !contains(str, '.'); } );
  }

  {
// for each received field, which output fields does it match?
    map<int /* received field number */, STRING_SET> matches;

    const STRING_MAP<EFT> exchange_field_eft { rules.exchange_field_eft() };  // EFTs have the choices already expanded; key = field name

    int field_nr { 0 };

    for (const string& received_value : copy_received_values)
    { STRING_SET match;

      for (const auto& field : exchange_template)
      { const string& field_name { field.name() };

        try
        { if (contains(field_name, '+'))                                           // if it's a CHOICE
          { //const vector<string> choices_vec { split_string <std::string> (field_name, '+') };

//            STRING_SET choices(choices_vec.cbegin(), choices_vec.cend());
            const STRING_SET choices { SR::to<STRING_SET>( split_string <std::string> (field_name, '+') ) };

            for (auto it { choices.begin() }; it != choices.end(); )    // see Josuttis 2nd edition, p. 343
            { if (exchange_field_eft.at(*it).is_legal_value(received_value))
              { match += field_name;                 // insert the "+" version of the name
                it = choices.end();
              }
              else
                it++;
            }
          }
          else    // not a choice
          { if (exchange_field_eft.at(field_name).is_legal_value(received_value))
              match += field_name;
          }
        }

        catch (...)
        { ost << "Error: cannot find field name: " << field_name << endl;
        }
      }

      matches += { field_nr++, match };
    }

    deque<TRIPLET> tuple_deque;

    for (const auto& [ field_number, matching_names ] : matches)
      tuple_deque += { field_number, copy_received_values[field_number], matching_names };  // TRIPLET: field number, value, matching field names

    vector<TRIPLET>     tuple_vector_assignments;
    STRING_MAP<TRIPLET> tuple_map_assignments;

// find entries with only one entry in set
    _assign_unambiguous_fields(tuple_deque, tuple_map_assignments);

    if (tuple_deque.empty())
      _valid = true;

    bool processed_field_on_last_pass { true };     // whether we processed any fields on the last pass through the loop that we are baout to execute

    while (processed_field_on_last_pass and !_valid) // we aren't finished; if we processed a field last time, we should try again with the new head of the deque
    { processed_field_on_last_pass = false;

      const TRIPLET& t { tuple_deque[0] };    // first received field we haven't been able to use, even tentatively

// find first received field that's a match for any exchange field and that we haven't used
      const auto cit { FIND_IF(exchange_template, [&t] (const exchange_field& ef) { return ( FIELD_NAMES(t).contains(ef.name())); } ) };

      if (cit != exchange_template.cend())
      { processed_field_on_last_pass = true;

        const string& field_name { cit->name() };    // syntactic sugar
        const bool    inserted   { (tuple_map_assignments.insert( { field_name, t } )).second };

        if (!inserted)
          ost << "WARNING: Unable to insert map assignment for field: " << field_name << ". This should never happen" << endl;

// remove the tuple we just processed
        --tuple_deque;

// remove this possible match name from all remaining elements in tuple vector
        FOR_ALL(tuple_deque, [field_name] (TRIPLET& t) { FIELD_NAMES(t).erase(field_name); } );

        _assign_unambiguous_fields(tuple_deque, tuple_map_assignments);

// if tuple_deque is empty, it doesn't guarantee that we're OK:
// suppose that there's an optional field, and we include it BUT miss a mandatory field,
// then the deque will be empty, but the mapping will still be incomplete.
// So we provisionally set _valid here, but might revoke it when preparing the output
        if (tuple_deque.empty())
          _valid = true;
      }

      if (!_valid)          // we aren't finished -- tuple_vector is not empty
      { ost << "Not finished; tuple vector is not empty" << endl;

        FOR_ALL(tuple_deque, [this] (TRIPLET& t) { _print_tuple(t); } );

        const TRIPLET& t   { tuple_deque[0] };
        const auto     cit { FIND_IF(exchange_template, [t] (const exchange_field& ef) { return ( FIELD_NAMES(t).contains(ef.name())); } ) };

        if (cit == exchange_template.cend())
        { if ( !require_dot_in_replacement_call and (_replacement_call.empty()) )             // maybe test for replacement call
          { //const bool replace_callsign { CALLSIGN_EFT.is_legal_value(RECEIVED_VALUE(t)) };

//            if (replace_callsign)
            if ( CALLSIGN_EFT.is_legal_value(RECEIVED_VALUE(t)) )
            { _replacement_call = RECEIVED_VALUE(t);

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
    { const string& name { pef.name() };

      try
      { pef.value(RECEIVED_VALUE(tuple_map_assignments.at(name)));
      }

      catch (...)
      { bool found_map { false };

        if (contains(name, '+'))                                         // if it's a CHOICE
        { const vector<string> choices_vec { split_string <std::string> (name, '+') };

          for (unsigned int n { 0 }; n < choices_vec.size() and !found_map; ++n)    // typically just a choice of 2
          { try
            { pef.value(RECEIVED_VALUE( tuple_map_assignments.at(choices_vec[n]) ));
              found_map = true;
            }

            catch (...)
            {
            }
          }
        }

// we might revoke the validity flag here, if we spot a problem
//        if (!found_map and !(optional_field_names > name))
        if ( !found_map and !optional_field_names.contains(name) )
        { ost << "WARNING: unable to find map assignment for key = " << name << endl;
          _valid = false;
        }
      }
    }

// normalize exchange fields to use canonical value, so that we don't mistakenly count each legitimate value more than once in statistics
// this means that we can't use a DOK.values file, because the received DOK will get changed here
    if (_valid)
      FOR_ALL(_fields, [&rules] (parsed_exchange_field& pef) { pef.value(rules.canonical_value(pef.name(), pef.value())); } );

  }  // end of !truncate received values
}

#undef FIELD_NUMBER
#undef RECEIVED_VALUE
#undef FIELD_NAMES

/*! \brief              Return the value of a particular field
    \param  field_name  field for which the value is requested
    \return             value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
string parsed_exchange::field_value(const string_view field_name) const
{ const auto cit { FIND_IF(_fields, [field_name] (const auto& field) { return (field.name() == field_name); } ) };   // returns first match in the vector

  return (cit == _fields.cend()) ? string { } : cit -> value();
}

/*! \brief          Return the names and values of matched fields
    \param  rules   rules for this contest
    \return         returns the actual matched names and values of the exchange fields

    Any field names that represent a choice are resolved to the name of the actual matched field in the returned object
*/
vector<parsed_exchange_field> parsed_exchange::chosen_fields(const contest_rules& rules) const
{ vector<parsed_exchange_field> rv;

  for (const auto& pef : _fields)
  { if (!contains(pef.name(), '+'))             // not a CHOICE
      rv += pef;
    else                                        // is a CHOICE
    { parsed_exchange_field pef_chosen { pef };

      pef_chosen.name(resolve_choice(pef.name(), pef.value(), rules));

      if (pef_chosen.name().empty())
        ost << "ERROR in parsed_exchange::chosen_fields(): empty name for field: " << pef.name() << endl;
      else
        rv += pef_chosen;
    }
  }

// assign correct mult status
  FOR_ALL(rv, [rules] (parsed_exchange_field& pef) { pef.is_mult(rules.is_exchange_mult(pef.name())); } );

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
string parsed_exchange::resolve_choice(const string_view field_name, const string_view received_value, const contest_rules& rules) const
{ if (field_name.empty())
    return string { };

  if (!contains(field_name, '+'))   // if not a CHOICE
    return string { field_name };

  const vector<string_view> choices_vec        { split_string <std::string_view> (field_name, '+') };
  const STRING_MAP<EFT>     exchange_field_eft { rules.exchange_field_eft() };  // EFTs have the choices already expanded; key = field name

  for (const auto& choice: choices_vec)    // see Josuttis 2nd edition, p. 343
  { try
    { //if (exchange_field_eft.at(choice).is_legal_value(received_value))   // heterogeneous lookup doesn't support at()!!!
      //  return choice;

      if (const auto opt { OPT_MUM_VALUE(exchange_field_eft, choice) }; opt)
      { if (opt.value().is_legal_value(received_value))
          return string { choice };
      }
    }

    catch (...)
    { ost << "Cannot find EFT for choice: " << choice << endl;
    }
  }

  return string { };
}

/*! \brief          Write a <i>parsed_exchange</i> object to an output stream
    \param  ost     output stream
    \param  pe      object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const parsed_exchange& pe)
{ ost << "parsed exchange object IS " << (pe.valid() ? "" : "NOT ") << "valid" << endl
      << "replacement call = " << pe.replacement_call() << endl;

  const vector<parsed_exchange_field> vec { pe.fields() };

  ost << "parsed exchange fields: " << endl;

  for (size_t n { 0 }; n < vec.size(); ++n)
  { const parsed_exchange_field& pef { vec[n] };

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

    Returns empty string if no sensible guess can be made.
    The returned value is inserted into the database.
*/
string exchange_field_database::guess_value(const string_view callsign, const string_view field_name)
{ SAFELOCK(exchange_field_database);

// first, check the database
  if (const auto opt { OPT_MUM_VALUE(_db, pair<string, string> { callsign, field_name }) }; opt)
    return opt.value();

// see if there's a pre-fill entry
  const string prefill_datum { prefill_data.prefill_data(field_name, callsign) };

  if (!prefill_datum.empty())
  { _db += { { string { callsign }, string { field_name } }, prefill_datum };

    return prefill_datum;
  }

// if it's a QTHX, then don't go any further if the country doesn't match
  if (field_name.starts_with("QTHX["sv) or field_name.starts_with("QTH2X["sv))
  { const string canonical_prefix { delimited_substring <std::string> (field_name, '[', ']', DELIMITERS::DROP) };

    if (canonical_prefix != location_db.canonical_prefix(callsign))
    { _db += { { string { callsign }, string { field_name } }, EMPTY_STR };                     // so that it can be found immediately in future
 
      return EMPTY_STR;
    }
  }

// no prior QSO; is it in the drmaster database?
  const drmaster_line drm_line { drm_cdb[callsign] };

/*! \brief                          Given a value, insert the corresponding canonical or as-is value into the database
    \param  value                   value of a field
    \param  get_canonical_value     whether to convert <i>value</i> to its corresponding canonical value
    \return                         <i>value</i> or canonical value corresponding to <i>value</i>, whichever was inserted, as a string
*/
  auto insert_value = [&callsign, &field_name, this] (const auto value, const bool get_canonical_value = false)
    { const string rv { get_canonical_value ? rules.canonical_value(field_name, to_string(value)) : to_string(value) };     // empty -> empty

      _db += { { string { callsign }, string { field_name } }, rv };

      return rv;
    };

  constexpr bool INSERT_CANONICAL_VALUE { true };

  auto ve_area_to_province = [] (const char call_area)
    { if (!isdigit(call_area))
        return string { };

       static const array<string, 10> abbreviations { { string { }, "NS"s, "PQ"s, "ON"s, "MB"s, "SK"s, "AB"s, "BC"s, string { }, "NB"s} };

       return abbreviations[call_area - '0'];    // convert to number
    };

  static const STRING_MAP<string> province_map { { "VO1"s, "NF"s },  // prefix, province
                                                 { "VO2"s, "LB"s },
                                                 { "VY2"s, "PE"s }
                                               };

  string rv;

  auto value_from_ve_prefix = [&ve_area_to_province] (const string& pfx)
    { const auto opt { OPT_MUM_VALUE(province_map, pfx) };

      return ( opt ? opt.value() : ve_area_to_province( pfx[pfx.length() - 1] ) ); // call area is last character in prefix
    };

// currently identical to 10MSTATE, except look up different value on the drmaster line
  if (field_name == "160MSTATE"sv)
  { string rv;

    if (!drm_line.empty())
    { rv = to_upper(drm_line.state_160());

      if (rv.empty())                       ///< if no explicit 160MSTATE value, try the QTH value
        rv = to_upper(drm_line.qth());
    }

    if (rv.empty() and ( location_db.canonical_prefix(callsign) == "VE"sv) )  // can often guess province for VEs
      rv = value_from_ve_prefix(wpx_prefix(callsign));

    return insert_value(rv, INSERT_CANONICAL_VALUE);
  }

  if (field_name == "10MSTATE"sv)
  { string rv;

    if (!drm_line.empty())
    { rv = to_upper(drm_line.state_10());

      if (rv.empty())                       ///< if no explicit 10MSTATE value, try the QTH value
        rv = to_upper(drm_line.qth());
    }

    if (rv.empty() and ( location_db.canonical_prefix(callsign) == "VE"sv) )  // can often guess province for VEs
      rv = value_from_ve_prefix(wpx_prefix(callsign));

    return insert_value(rv, INSERT_CANONICAL_VALUE);
  }

  if (field_name == "AGE"sv)        // AA CW or SSB
  { if (!drm_line.empty())
    { // we don't have direct access to the contest mode, so assume CW unless the month is September
      const string      dts                   { date_time_string(SECONDS::NO_INCLUDE) };
      const string_view current_month         { substring <string_view> (dts, 5, 2) };
      const string      encoded_age_from_file { (current_month == "09") ? drm_line.age_aa_ssb() : drm_line.age_aa_cw() };  // YYYYMMDD:nn

      if (!encoded_age_from_file.empty())
      { const string_view current_year   { substring <string_view> (dts, 0, 4) };
        const string_view year_from_file { substring <string_view> (encoded_age_from_file, 0, 4) };
        const string_view age_from_file  { substring <string_view> (encoded_age_from_file, 9) };
        const string      current_age    { ((age_from_file == "00"sv) or (age_from_file == "01"sv))   // don't change if [old, YL] 00 or [new, anyone] 01
                                             ? age_from_file
                                             : ::to_string( from_string<int>(age_from_file) + from_string<int>(current_year) - from_string<int>(year_from_file) )
                                         };

        return insert_value(current_age);
      }
    }
  }

  if (field_name == "CHECK"sv)
    return insert_value(drm_line.check()); 

  if (field_name == "CQZONE"sv)
  { const string rv { drm_line.cq_zone() };

    if (!rv.empty())
      return insert_value(rv, INSERT_CANONICAL_VALUE);

// no entry in drmaster database; try the location database
    return insert_value(location_db.cq_zone(callsign), INSERT_CANONICAL_VALUE);
  }

  if (field_name == "CWPOWER"sv)
    return insert_value(drm_line.cw_power());

  if ( (field_name == "DOK"sv) and (location_db.canonical_prefix(callsign) == "DL"sv) )
    return insert_value(drm_line.qth());    // DL QTH is the DOK

  if (field_name == "FDEPT"sv)
    return insert_value(drm_line.qth());    // F (and French territories) QTH is the dept

  if (field_name == "GRID"sv)
  { const string grid_value { drm_line.grid() };

    return insert_value( (grid_value.length() > 4) ? substring <std::string> (grid_value, 0, 4) : grid_value );
  }

  if (field_name == "HADXC"sv)     // stupid HA DX membership number is (possibly) in the QTH field of an HA (making it useless for WAHUC)
    return insert_value(drm_line.qth());    // I think that this should work

  if (field_name == "IOTA"sv)
  { string rv { drm_line.iota() };

//   https://www.qsl.net/xe2nat/iotalist.htm; https://www.qsl.net/co8tw/iota.htm
  // up to FT/g, AF011

    if (rv.empty())
    { static const UNORDERED_STRING_MAP<string> iota_map { { "CM"s,   "NA015"s },  // key = cp, value = IOTA number
                                                           { "CY9"s,  "NA094"s },
                                                           { "CY0"s,  "NA063"s },
                                                           { "C6"s,   "NA001"s },
                                                           { "D4"s,   "AF005"s },
                                                           { "D6"s,   "AF007"s },
                                                           { "EA6"s,  "EU004"s },
                                                           { "EA8"s,  "AF004"s },
                                                           { "FG"s,   "NA102"s },
                                                           { "FJ"s,   "NA146"s },
                                                           { "FM"s,   "NA107"s },
                                                           { "FP"s,   "NA032"s },
                                                           { "FS"s,   "NA105"s },
                                                           { "FT/g"s, "AF011"s },
                                                           { "FT/w"s, "AF008"s },
                                                           { "G"s,    "EU005"s },
                                                           { "GJ"s,   "EU013"s },
                                                           { "GM"s,   "EU005"s },
                                                           { "GW"s,   "EU005"s },
                                                           { "HH"s,   "NA096"s },
                                                           { "HI"s,   "NA096"s },
                                                           { "HK0"s,  "NA033"s },
                                                           { "IS"s,   "EU024"s },
                                                           { "IT9"s,  "EU025"s }, // WAE country only
                                                           { "JW"s,   "EU026"s },
                                                           { "JX"s,   "EU022"s },
                                                           { "J3"s,   "NA024"s },
                                                           { "J6"s,   "NA108"s },
                                                           { "J7"s,   "NA101"s },
                                                           { "J8"s,   "NA109"s },
                                                           { "KP1"s,  "NA098"s },
                                                           { "KP2"s,  "NA106"s },
                                                           { "KP4"s,  "NA099"s },
                                                           { "KP5"s,  "NA095"s },
                                                           { "OH0"s,  "EU002"s },
                                                           { "OJ0"s,  "EU053"s },
                                                           { "OX"s,   "NA018"s },
                                                           { "OY"s,   "EU018"s },
                                                           { "PJ5"s,  "NA145"s },
                                                           { "R1FJ"s, "EU019"s },
                                                           { "SV5"s,  "EU001"s },
                                                           { "SV9"s,  "EU015"s },
                                                           { "TI9"s,  "NA012"s },
                                                           { "TF"s,   "EU021"s },
                                                           { "TK"s,   "EU014"s },
                                                           { "VO1"s,  "NA027"s },
                                                           { "VP2E"s, "NA022"s },
                                                           { "VP2M"s, "NA103"s },
                                                           { "VP2V"s, "NA023"s },
                                                           { "VP9"s,  "NA005"s },
                                                           { "VQ9"s,  "AF006"s },
                                                           { "VY2"s,  "NA029"s },
                                                           { "V2"s,   "NA100"s },
                                                           { "V4"s,   "NA104"s },
                                                           { "XE4"s,  "NA030"s },
                                                           { "YV0"s,  "NA020"s },
                                                           { "ZD8"s,  "AF003"s },
                                                           { "ZF"s,   "NA016"s },
                                                           { "6Y"s,   "NA097"s },
                                                           { "8P"s,   "NA021"s },
                                                           { "9H"s,   "EU023"s }
                                                        };


        rv = MUM_VALUE(iota_map, location_db.canonical_prefix(callsign)); 
      }

      return insert_value(rv);    // I think that this should work
  }

  if (field_name == "ITUZONE"sv)
  { const string rv { drm_line.itu_zone() };

    if (!rv.empty())
      return insert_value(rv, INSERT_CANONICAL_VALUE);

// no entry in drmaster database; try the location database
    return insert_value(location_db.itu_zone(callsign), INSERT_CANONICAL_VALUE);
  }

  static const STRING_SET JAs { "JA"s, "JD/M"s, "JD/O"s };

//  if ( (field_name == "JAPREF"sv) and ( (STRING_SET { "JA"s, "JD/M"s, "JD/O"s }).contains(location_db.canonical_prefix(callsign))) )
  if ( (field_name == "JAPREF"sv) and JAs.contains(location_db.canonical_prefix(callsign)) )
    return insert_value(drm_line.qth());

//  if ( (field_name == "KCJ"sv) and ( (set<string> { "JA"s, "JD/M"s, "JD/O"s }).contains(location_db.canonical_prefix(callsign))) )
//  if ( (field_name == "KCJ"sv) and ( (STRING_SET { "JA"s, "JD/M"s, "JD/O"s }).contains(location_db.canonical_prefix(callsign))) )
  if ( (field_name == "KCJ"sv) and JAs.contains(location_db.canonical_prefix(callsign)) )
    return insert_value(drm_line.qth2());    // I think that this should work

  if (field_name == "NAME"sv)
    return insert_value(drm_line.name());    // I think that this should work

  if (field_name == "PREC"sv)
    return insert_value(drm_line.precedence());    // I think that this should work 

  if (field_name.starts_with("QTHX["sv))     // by the time we get here, the call should match the canonical prefix in the name of the exchange field
  { const string_view canonical_prefix { delimited_substring <std::string_view> (field_name, '[', ']', DELIMITERS::DROP) };

    if (canonical_prefix != location_db.canonical_prefix(callsign))
    { ost << "QTHX: Failure to match callsign with canonical prefix in exchange_field_database::guess_value(); field name = " <<  field_name << ", callsign = " << callsign << endl;
      return string { };
    }

    return insert_value(drm_line.qth(), INSERT_CANONICAL_VALUE);    // I think that this should work, but not absolutely certain 
  }

  if ((field_name == "RDA"sv) or (field_name == "RD2"sv))
  { static const STRING_SET countries { "R1FJ"s, "UA"s, "UA2"s, "UA9"s };

    string rv;

    if (!drm_line.empty() and ( (countries.contains(location_db.canonical_prefix(callsign))) or callsign.starts_with("RI1AN"sv)) )
    { rv = drm_line.qth();

      if ( (field_name == "RD2"sv) and (rv.length() > 2) )     // allow for case when full 4-character RDA is in the drmaster file
        rv = substring <std::string> (rv, 0, 2);

      return insert_value(rv); 
    }

// no entry in drmaster database; can we determine from the location database?
    rv = location_db.region_abbreviation(callsign);

    return insert_value(rv);    // I think that this should work, but not absolutely certain 
  }

  if (field_name == "SECTION"sv)
    return insert_value(drm_line.section());

  if (field_name == "SKCCNO"sv)               // shouldn't really get here, because there should be a pre-fill file
    return insert_value(drm_line.skcc());

  if (field_name == "SOCIETY"sv)
    return insert_value(drm_line.society());

  if (field_name == "SPC"sv)
    return insert_value(drm_line.spc());

  if (field_name == "SSBPOWER"sv)
    return insert_value(drm_line.ssb_power());

  if (field_name == "UKEICODE"sv)
    return insert_value(drm_line.qth(), INSERT_CANONICAL_VALUE);

// choices
  if (field_name == "ITUZONE+SOCIETY"sv)    // IARU
  { string rv { drm_line.society() };

    if (rv.empty())
    { rv = drm_line.itu_zone();

      if (rv.empty())                                   // no entry in drmaster database; can we determine from the location database?
        rv = to_string(location_db.itu_zone(callsign));
    }

    return insert_value(rv, INSERT_CANONICAL_VALUE);    // I think that this should work, but not absolutely certain
  }

// give up
//  _db += { { string { callsign }, field_name }, EMPTY_STR };  // so we find it next time
  _db += { { string { callsign }, string { field_name } }, EMPTY_STR };  // so we find it next time

  return EMPTY_STR;
}

/*! \brief              Set a value in the database
    \param  callsign    callsign for the new entry
    \param  field_name  name of the field for the new entry
    \param  value       the new entry
*/
//void exchange_field_database::set_value(const string& callsign, const string& field_name, const string& value)
//void exchange_field_database::set_value(const string_view callsign, const string& field_name, const string& value)
void exchange_field_database::set_value(const string_view callsign, const string_view field_name, const string_view value)
{ SAFELOCK(exchange_field_database);

  _db[ { string { callsign }, string { field_name } } ] = value;    // don't use insert, since we must overwrite
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
//void exchange_field_database::set_values_from_file(const vector<string>& path, const string_view filename, const string& field_name)
void exchange_field_database::set_values_from_file(const vector<string>& path, const string_view filename, const string_view field_name)
{ try
  { const string contents { read_file(path, filename) };

    if (!contents.empty())
    { const vector<string> lines { to_lines <std::string> (to_upper(remove_char(contents, CR_CHAR))) };        // in case it's a silly Microsoft-format file

      for (int n { 0 }; n < ssize(lines); ++n)
      { const string line { squash(replace_char(lines[n], '\t', ' '), ' ') };

        if (const vector<string> tokens { clean_split_string <string> (line, ' ') }; tokens.size() == 2)
        { if ( (n == 0) and (tokens[0] == "CALL"sv) )   // possibly ignore this line
            continue;

          set_value(tokens[0] /* call */, field_name, tokens[1] /* value */);
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
string process_cut_digits(const string& input)
{ string rv { input };

  for (char& c : rv)  // must use reference, as we want to be able to change the value
  { switch (c)
    { case 'T' :
      case 't' :
        c = '0';
        break;

      case 'N' :
      case 'n' :
        c = '9';
        break;

      case 'A' :
      case 'a' :
        c = '1';
        break;
    }
  }

  return rv;
}

// -------------------------  sweepstakes_exchange  ---------------------------

/*!     \class sweepstakes_exchange
        \brief Encapsulates an exchange for Sweepstakes

        Sweepstakes is different because:
          1. Two fields might take the form of a two-digit number
          2. A call may be present as part of the exchange
          3. The order may be quite different from the canonical order if part of the exchange has come from drmaster
*/
//sweepstakes_exchange::sweepstakes_exchange(const contest_rules& rules, const string& callsign, const string& received_exchange) :
//sweepstakes_exchange::sweepstakes_exchange(const contest_rules& rules, const string_view callsign, const string& received_exchange) :
sweepstakes_exchange::sweepstakes_exchange(const contest_rules& rules, const string_view callsign, const string_view received_exchange) :
  _call(callsign)
{
/*
  static EFT  check_eft;
  static EFT  serno_eft;
  static EFT  prec_eft;
  static EFT  section_eft;
  static bool first_time { true };

  if (first_time)
  { check_eft = rules.exchange_field_eft("CHECK"s);
    serno_eft = rules.exchange_field_eft("SERNO"s);
    prec_eft = rules.exchange_field_eft("PREC"s);
    section_eft = rules.exchange_field_eft("SECTION"s);

    first_time = false;
  }
*/
  static const EFT check_eft   { rules.exchange_field_eft("CHECK"sv) };
  static const EFT serno_eft   { rules.exchange_field_eft("SERNO"sv) };
  static const EFT prec_eft    { rules.exchange_field_eft("PREC"sv) };
  static const EFT section_eft { rules.exchange_field_eft("SECTION"sv) };

  const vector<string> r_vec { remove_peripheral_spaces <std::string> (split_string <std::string> (received_exchange, SPACE_STR)) };

//  const static regex check_regex("^[[:digit:]][[:digit:]]$");
//  const static regex serno_regex("^([[:digit:]])+$");

//  auto is_check = [](const string& target) { return check_eft.is_legal_value(target); };
//  auto is_serno = [](const string& target) { return serno_eft.is_legal_value(target); };
//  auto is_prec = [](const string& target) { return prec_eft.is_legal_value(target); };
//  auto is_section = [](const string& target) { return section_eft.is_legal_value(target); };
}
