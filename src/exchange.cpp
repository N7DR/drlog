// $Id: exchange.cpp 73 2014-08-30 14:44:01Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file exchange.cpp

        Classes and functions related to processing exchanges
*/

#include "diskfile.h"
#include "exchange.h"
#include "string_functions.h"

using namespace boost;
using namespace std;

exchange_field_template EXCHANGE_FIELD_TEMPLATES;

#define NEW_CONSTRUCTOR

#if defined(NEW_CONSTRUCTOR)

/*!     \brief  constructor
        \param  callsign    callsign of the station from which the exchange was received
        \param  rules       rules for the contest
        \param  received_values     the received values, in the order that they were received
*/
parsed_exchange::parsed_exchange(const std::string& canonical_prefix, const contest_rules& rules, const vector<string>& received_values) :
  _replacement_call(),
  _valid(false)
{ static const string EMPTY_STRING("");

  ost << "Inside parsed_exchange constructor" << endl;

  const vector<exchange_field> exchange_template = rules.exch(canonical_prefix);
  const int size_difference = static_cast<int>(received_values.size()) - static_cast<int>(exchange_template.size());  // does not allow for optional fields

//  ost << "exchange_template: " << endl;
//  int index = 0;
//  for (const auto& field : exchange_template)
//  { ost << index++ << ": " << field.name() << ", " << field.is_mult() << ", " << field.is_optional() << ", " << field.is_choice() << endl;
//  }

  FOR_ALL(exchange_template, [=] (const exchange_field& ef) { _fields.push_back(parsed_exchange_field { ef.name(), EMPTY_STRING, ef.is_mult() }); } );

  for (auto& field : _fields)
  { ost << "field : " << field << endl;
  }

  if (size_difference == 0)    // correct number, although we don't assume that the order is the same as the template
  { for (auto& field : _fields)
    {
    }
  }



// in what follows, "source" refers to what has been received, "destination" refers to the fields that will be logged

// use vector<int> instead of vector<bool>
  vector<int> is_dest_mapped(exchange_template.size());    // all set to false (0)
  vector<int> is_source_used(exchange_template.size());    // all set to false (0)
  set<int>    sources_examined;
  set<int>    dest_mapped;

//  int number_of_excess_values = received_values.size() - exchange_template.size();
  unsigned int next_source = 0;
  unsigned int next_dest = 0;

// look at first field
// name of first field

  int n_attempts = 0;
  const int MAX_ATTEMPTS = 10;

// look for explicit marker for replacement call, which is a field that contains a dot
  vector<string> copy_received_values;

  for (const auto& received_value : received_values)  // assume only one field contains a dot
  { if (contains(received_value, "."))
      _replacement_call = remove_char(received_value, '.');
    else
      copy_received_values.push_back(received_value);
  }

//  ost << "number of source fields = " << copy_received_values.size() << endl;
//  ost << "number of dest fields = " << exchange_template.size() << endl;

//  for (size_t n = 0; n < copy_received_values.size(); ++n)
//    ost << "received value # " << n << " = " << copy_received_values[n] << endl;  // not canonical

  while ( ( (sources_examined.size() != copy_received_values.size()) or (dest_mapped.size() != exchange_template.size()) ) and (n_attempts++ < MAX_ATTEMPTS) )
  { //ost << "Attempt # " << n_attempts << endl;

    //ost << "number of sources examined = " << sources_examined.size() << endl;
    //ost << "number of dest mapped = " << dest_mapped.size() << endl;

    if (dest_mapped.size() == exchange_template.size() )    // have all fields been filled?
    { ost << "Testing after all fields have been filled" << endl;
      if (_replacement_call.empty() and (*validity_function("CALLSIGN", rules))(copy_received_values[next_source], rules))
      { ost << "Found a replacement call: " << copy_received_values[next_source] << endl;
        _replacement_call = copy_received_values[next_source];
        sources_examined.insert(next_source);
        is_source_used[next_source] = 1;
        next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
      }
      else    // re-map; find first destination field that this can go in
      { const string source = copy_received_values[next_source];
        bool remapped = false;

        for (size_t n = 0; !remapped and n < exchange_template.size(); ++n)
        { string destination_field_name = _fields[n].name();
          const bool is_choice = contains(destination_field_name, "+");
          string resolved_destination_field_name;
          bool resolved = false;

          if (is_choice)
          { resolved_destination_field_name = _resolve_choice(destination_field_name, source, rules);

            if (!resolved_destination_field_name.empty())
            { destination_field_name = resolved_destination_field_name;
              resolved = true;
            }
          }

          if (!is_choice or (is_choice and resolved))
          { const bool is_permitted_value_1 = (rules.exch_permitted_values(destination_field_name) < source);

//            ost << "is_permitted_value_1 = " << is_permitted_value_1
//                << "; source = " << source
//                << "; destination field name = " << destination_field_name
//                << "; number of permitted values = " << rules.exch_permitted_values(destination_field_name).size() << endl;

//              const std::set<std::string> exch_permitted_values(const std::string& field_name) const;

//            const bool is_permitted_value_1 = false;

            const bool is_permitted_value = ( (is_permitted_value_1) ? true : (*validity_function(destination_field_name, rules))(source, rules) );

            if (is_permitted_value)
            { _fields[n].value(source);
              _fields[n].name(destination_field_name);
              sources_examined.insert(next_source);
            }
          }
        }
      }
    }
    else               // not all the fields have yet been filled
    { string destination_field_name = _fields[next_dest].name();
      const bool is_choice = contains(destination_field_name, "+");
      string resolved_destination_field_name;
      bool resolved = false;

      if (is_choice)
      { resolved_destination_field_name = _resolve_choice(destination_field_name, copy_received_values[next_source], rules);

        if (!resolved_destination_field_name.empty())
        { destination_field_name = resolved_destination_field_name;
          resolved = true;
        }
      }

      ost << "is_choice = " << is_choice << "; resolved = " << resolved << endl;

      if (!is_choice or (is_choice and resolved))
      { const set<string>& permitted_values = rules.exch_permitted_values(destination_field_name);
        bool is_permitted_value_1 = false;
        bool is_permitted_value_2 = false;
        bool is_permitted_value;

        if (!permitted_values.empty())
        { is_permitted_value = (permitted_values < copy_received_values[next_source]);

//      ost << "is_permitted_value (1) = " << is_permitted_value
//          << "; source = " << copy_received_values[next_source]
//         << "; destination field name = " << destination_field_name
//          << "; number of permitted values = " << permitted_values.size() << endl;
        }
        else  // rules do not have an explicit set of permitted values
        { const VALIDITY_FUNCTION_TYPE vf = validity_function(destination_field_name, rules);

          is_permitted_value = (*vf)(copy_received_values[next_source], rules);
        }

//        ost << "destination field name = " << destination_field_name << ", tested value = " << copy_received_values[next_source] << ", is permitted value = " << is_permitted_value << endl;

        if (is_permitted_value)
        { _fields[next_dest].value(copy_received_values[next_source]);
          _fields[next_dest].name(destination_field_name);
          is_dest_mapped[next_dest] = 1;
          is_source_used[next_source] = 1;
          sources_examined.insert(next_source);
          dest_mapped.insert(next_dest);
          next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
          next_dest = ( (++next_dest < exchange_template.size()) ? next_dest : 0);    // wrap
        }
        else        // value does not match template
        {
// have we allocated a replacement call?
          if (_replacement_call.empty() and (*validity_function("CALLSIGN", rules))(copy_received_values[next_source], rules))
          { _replacement_call = copy_received_values[next_source];
            sources_examined.insert(next_source);
            is_source_used[next_source] = 1;
            next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
          }
          else    // we have allocated a replacement call
            next_dest = ( (++next_dest < exchange_template.size()) ? next_dest : 0);    // wrap
        }
      }
      else // unresolved choice
      {
// have we allocated a replacement call?
        if (_replacement_call.empty() and (*validity_function("CALLSIGN", rules))(copy_received_values[next_source], rules))
        { _replacement_call = copy_received_values[next_source];
          sources_examined.insert(next_source);
          is_source_used[next_source] = 1;
          next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
        }
        else    // we have allocated a replacement call
          next_dest = ( (++next_dest < exchange_template.size()) ? next_dest : 0);    // wrap
      }
    }
  }

  if (n_attempts < MAX_ATTEMPTS)
    _valid = true;

// normalize some of the exchange fields ... so that we don't mistakenly count each legitimate value more than once in statistics
  if (_valid)
  { for (auto it = _fields.begin(); it != _fields.end(); ++it)
    { //ost << "pexch field " << it->name() << " has value " << it->value() << endl;
      it->value(rules.canonical_value(it->name(), it->value()));
      //ost << "pexch field " << it->name() << " now has value " << it->value() << endl;
    }
  }
}

#endif

#if !defined(NEW_CONSTRUCTOR)
/*!     \brief  constructor
        \param  callsign    callsign of the station from which the exchange was received
        \param  rules       rules for the contest
        \param  received_values     the received values, in the order that they were received
*/
parsed_exchange::parsed_exchange(const std::string& canonical_prefix, const contest_rules& rules, const vector<string>& received_values) :
  _replacement_call(),
  _valid(false)
{
  ost << "Inside parsed_exchange constructor" << endl;

  const vector<exchange_field> exchange_template = rules.exch(canonical_prefix);

//  ost << "exchange_template: " << endl;
//  int index = 0;
//  for (const auto& field : exchange_template)
//  { ost << index++ << ": " << field.name() << ", " << field.is_mult() << ", " << field.is_optional() << ", " << field.is_choice() << endl;
//  }

  for (const auto& field : exchange_template)
    _fields.push_back(parsed_exchange_field { field.name(), "", field.is_mult() } );

// in what follows, "source" refers to what has been received, "destination" refers to the fields that will be logged

// use vector<int> instead of vector<bool>
  vector<int> is_dest_mapped(exchange_template.size());    // all set to false (0)
  vector<int> is_source_used(exchange_template.size());    // all set to false (0)
  set<int>    sources_examined;
  set<int>    dest_mapped;

//  int number_of_excess_values = received_values.size() - exchange_template.size();
  unsigned int next_source = 0;
  unsigned int next_dest = 0;

// look at first field
// name of first field

  int n_attempts = 0;
  const int MAX_ATTEMPTS = 10;

// look for explicit marker for replacement call, which is a field that contains a dot
  vector<string> copy_received_values;

  for (const auto& received_value : received_values)  // assume only one field contains a dot
  { if (contains(received_value, "."))
      _replacement_call = remove_char(received_value, '.');
    else
      copy_received_values.push_back(received_value);
  }

//  ost << "number of source fields = " << copy_received_values.size() << endl;
//  ost << "number of dest fields = " << exchange_template.size() << endl;

//  for (size_t n = 0; n < copy_received_values.size(); ++n)
//    ost << "received value # " << n << " = " << copy_received_values[n] << endl;  // not canonical

  while ( ( (sources_examined.size() != copy_received_values.size()) or (dest_mapped.size() != exchange_template.size()) ) and (n_attempts++ < MAX_ATTEMPTS) )
  { //ost << "Attempt # " << n_attempts << endl;

    //ost << "number of sources examined = " << sources_examined.size() << endl;
    //ost << "number of dest mapped = " << dest_mapped.size() << endl;

    if (dest_mapped.size() == exchange_template.size() )    // have all fields been filled?
    { ost << "Testing after all fields have been filled" << endl;
      if (_replacement_call.empty() and (*validity_function("CALLSIGN", rules))(copy_received_values[next_source], rules))
      { ost << "Found a replacement call: " << copy_received_values[next_source] << endl;
        _replacement_call = copy_received_values[next_source];
        sources_examined.insert(next_source);
        is_source_used[next_source] = 1;
        next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
      }
      else    // re-map; find first destination field that this can go in
      { const string source = copy_received_values[next_source];
        bool remapped = false;

        for (size_t n = 0; !remapped and n < exchange_template.size(); ++n)
        { string destination_field_name = _fields[n].name();
          const bool is_choice = contains(destination_field_name, "+");
          string resolved_destination_field_name;
          bool resolved = false;

          if (is_choice)
          { resolved_destination_field_name = _resolve_choice(destination_field_name, source, rules);

            if (!resolved_destination_field_name.empty())
            { destination_field_name = resolved_destination_field_name;
              resolved = true;
            }
          }

          if (!is_choice or (is_choice and resolved))
          { const bool is_permitted_value_1 = (rules.exch_permitted_values(destination_field_name) < source);

//            ost << "is_permitted_value_1 = " << is_permitted_value_1
//                << "; source = " << source
//                << "; destination field name = " << destination_field_name
//                << "; number of permitted values = " << rules.exch_permitted_values(destination_field_name).size() << endl;

//              const std::set<std::string> exch_permitted_values(const std::string& field_name) const;

//            const bool is_permitted_value_1 = false;

            const bool is_permitted_value = ( (is_permitted_value_1) ? true : (*validity_function(destination_field_name, rules))(source, rules) );

            if (is_permitted_value)
            { _fields[n].value(source);
              _fields[n].name(destination_field_name);
              sources_examined.insert(next_source);
            }
          }
        }
      }
    }
    else               // not all the fields have yet been filled
    { string destination_field_name = _fields[next_dest].name();
      const bool is_choice = contains(destination_field_name, "+");
      string resolved_destination_field_name;
      bool resolved = false;

      if (is_choice)
      { resolved_destination_field_name = _resolve_choice(destination_field_name, copy_received_values[next_source], rules);

        if (!resolved_destination_field_name.empty())
        { destination_field_name = resolved_destination_field_name;
          resolved = true;
        }
      }

      ost << "is_choice = " << is_choice << "; resolved = " << resolved << endl;

      if (!is_choice or (is_choice and resolved))
      { const set<string>& permitted_values = rules.exch_permitted_values(destination_field_name);
        bool is_permitted_value_1 = false;
        bool is_permitted_value_2 = false;
        bool is_permitted_value;

        if (!permitted_values.empty())
        { is_permitted_value = (permitted_values < copy_received_values[next_source]);

//      ost << "is_permitted_value (1) = " << is_permitted_value
//          << "; source = " << copy_received_values[next_source]
//         << "; destination field name = " << destination_field_name
//          << "; number of permitted values = " << permitted_values.size() << endl;
        }
        else  // rules do not have an explicit set of permitted values
        { const VALIDITY_FUNCTION_TYPE vf = validity_function(destination_field_name, rules);

          is_permitted_value = (*vf)(copy_received_values[next_source], rules);
        }

//        ost << "destination field name = " << destination_field_name << ", tested value = " << copy_received_values[next_source] << ", is permitted value = " << is_permitted_value << endl;

        if (is_permitted_value)
        { _fields[next_dest].value(copy_received_values[next_source]);
          _fields[next_dest].name(destination_field_name);
          is_dest_mapped[next_dest] = 1;
          is_source_used[next_source] = 1;
          sources_examined.insert(next_source);
          dest_mapped.insert(next_dest);
          next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
          next_dest = ( (++next_dest < exchange_template.size()) ? next_dest : 0);    // wrap
        }
        else        // value does not match template
        {
// have we allocated a replacement call?
          if (_replacement_call.empty() and (*validity_function("CALLSIGN", rules))(copy_received_values[next_source], rules))
          { _replacement_call = copy_received_values[next_source];
            sources_examined.insert(next_source);
            is_source_used[next_source] = 1;
            next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
          }
          else    // we have allocated a replacement call
            next_dest = ( (++next_dest < exchange_template.size()) ? next_dest : 0);    // wrap
        }
      }
      else // unresolved choice
      {
// have we allocated a replacement call?
        if (_replacement_call.empty() and (*validity_function("CALLSIGN", rules))(copy_received_values[next_source], rules))
        { _replacement_call = copy_received_values[next_source];
          sources_examined.insert(next_source);
          is_source_used[next_source] = 1;
          next_source = ( (++next_source < copy_received_values.size()) ? next_source : 0);    // wrap
        }
        else    // we have allocated a replacement call
          next_dest = ( (++next_dest < exchange_template.size()) ? next_dest : 0);    // wrap
      }
    }
  }

  if (n_attempts < MAX_ATTEMPTS)
    _valid = true;

// normalize some of the exchange fields ... so that we don't mistakenly count each legitimate value more than once in statistics
  if (_valid)
  { for (auto it = _fields.begin(); it != _fields.end(); ++it)
    { //ost << "pexch field " << it->name() << " has value " << it->value() << endl;
      it->value(rules.canonical_value(it->name(), it->value()));
      //ost << "pexch field " << it->name() << " now has value " << it->value() << endl;
    }
  }
}
#endif

/*! \brief  Return the value of a particular field
    \param  field_name  field for which the value is requested
    \return value corresponding to <i>field_name</i>

    Returns empty string if <i>field_name</i> does not exist
*/
const string parsed_exchange::field_value(const std::string& field_name) const
{ for (const auto& field : _fields)
    if (field.name() == field_name)
      return field.value();

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

#include "cty_data.h"
#include "drmaster.h"
#include "log.h"

extern drmaster* drm_p;
extern location_database location_db;
extern logbook logbk;
extern contest_rules     rules;

pt_mutex exchange_field_database_mutex;

// -------------------------  exchange_field_database  ---------------------------

/*!     \class exchange_field_database
        \brief used for estimating the exchange field

        There can be only one of these, and it is thread safe
*/

/*! \brief  Guess the value of an exchange field
    \param  callsign   callsign for the guess
    \param  field_name name of the field for the guess
    \return Guessed value of <i>field_name</i> for <i>callsign</i>

    Returns empty string if no sensible guess can be made
*/
const string exchange_field_database::guess_value(const string& callsign, const string& field_name)
{ ost << "guessing value of " << field_name << " exchange field for " << callsign << endl;

  SAFELOCK(exchange_field_database);

// first, check the database
  const auto it = _db.find( pair<string, string>( { callsign, field_name } ) ) ;

  if (it != _db.end())
    return it->second;

  ost << "not in database => no prior QSO" << endl;

// no prior QSO; is it in the drmaster database?
  const drmaster_line drm_line = (*drm_p)[callsign];

  if (field_name == "10MSTATE")
  { string rv;

    if (!drm_line.empty())
    { rv = drm_line.state_10();

      if (!rv.empty())
      { rv = rules.canonical_value("10MSTATE", rv);
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

  if ((field_name == "RDA") or (field_name == "RD2"))
  { static const set<string> countries { "UA", "UA2", "UA9" };
    string rv;

    if (!drm_line.empty() and (countries < location_db.canonical_prefix(callsign)))
    { rv = drm_line.qth();

      if (!rv.empty())
      { rv = rules.canonical_value(field_name, rv);
        _db.insert( { { callsign, field_name }, rv } );

        return rv;
      }
    }

// no entry in drmaster database; can we determine from the location database?

//    ost << "no entry in database" << endl;

    rv = location_db.region_abbreviation(callsign);

//    ost << "location_db.region_abbreviation() for " << callsign << " is: " << rv << endl;

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

  ost << "in exchange_field_database::set_value(); initial size = " << _db.size() << endl;
  ost << "callsign = " << callsign << ", field_name = " << field_name << ", value = " << value << endl;

  _db[ { callsign, field_name } ] = value;    // don't use insert, since we must overwrite

  ost << "final size = " << _db.size() << endl;
}

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

/// Return all the names of exchange fields in the database
const set<string> exchange_field_template::names(void) const
{ set<string> rv;

//  FOR_ALL(_db, [&rv] (const pair<string, boost::regex>& psr) { rv.insert(psr.first); } );
  FOR_ALL(_db, [&rv] (const decltype(_db)::value_type& psr) { rv.insert(psr.first); } );

  return rv;
}

/// Return regex for a name; returns empty regex if the name is invalid
const boost::regex exchange_field_template::expression(const string& str) const
{ if (!(names() < str))
    return boost::regex();

  return _db.at(str);
}

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

// -------------------------  EFT  ---------------------------

/*!     \class EFT (exchange_field_template)
        \brief Manage a single exchange field
*/

/// construct from name
EFT::EFT(const string& nm) :
  _is_mult(false),
  _name(nm)
{ }

/// construct from regex and values files
EFT::EFT(const string& nm, const vector<string>& path, const string& regex_filename /* , const string& values_filename */) :
  _is_mult(false),
  _name(nm)
{ read_regex_expression_file(path, regex_filename);
  read_values_file(path, nm);

  ost << (*this) << endl;
}

/// construct from regex and values files
EFT::EFT(const string& nm, const vector<string>& path, const string& regex_filename,
    const drlog_context& context, location_database& location_db) :
  _is_mult(false),
  _name(nm)
{ read_regex_expression_file(path, regex_filename);
  read_values_file(path, nm);







  ost << (*this) << endl;
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

/*! \brief  Get regex expression from file
    \param  paths      paths to try
    \param  filename   name of file
    \return whether a regex expression was read
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

/*! \brief  Get info from .values file
    \param  path      paths to try
    \param  filename   name of file (without .values extension)
    \return whether values were read
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
          }
        }
        else    // no "="
        { const string str = remove_peripheral_spaces(line);

          if (!str.empty())
            _values.insert( { str, /* set<string> */ { str } } );
        }
      }
    }
  }

  catch (...)
  { ost << "Failed to read file " << filename << ".values" << endl;
    return false;
  }

  return (!_values.empty());
}

void EFT::parse_context_qthx(const drlog_context& context, location_database& location_db)
{ const auto& context_qthx = context.qthx();

  if (context_qthx.empty())
    return;

//  SAFELOCK(rules);

  for (const auto this_qthx : context_qthx)
  { const string canonical_prefix = location_db.canonical_prefix(this_qthx.first);
    const set<string> ss = this_qthx.second;
//    exchange_field_values qthx;

//    qthx.name(string("QTHX[") + canonical_prefix + "]");

    for (const auto this_value : ss)
    { if (!contains(this_value, "|"))
        add_canonical_value(this_value);
      else
      { const vector<string> equivalent_values = remove_peripheral_spaces(split_string(this_value, "|"));

        if (!equivalent_values.empty())
          add_canonical_value(equivalent_values[0]);

        for (size_t n = 1; n < equivalent_values.size(); ++n)
          add_legal_value(equivalent_values[0], equivalent_values[n]);
      }
    }

//    _values.push_back(qthx);
  }
}


// maybe should keep this as a separate member, to save having to execute this so frequently
#if 0
const set<string> EFT::_all_legal_non_regex_values(void) const
{ set<string> rv;

  FOR_ALL(_values, [&rv] (const pair<string, set<string>>& pss) { copy(pss.second.cbegin(), pss.second.cend(), inserter(rv, rv.begin())); } );

  return rv;
}
#endif

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

const bool EFT::is_legal_value(const string& str) const
{ if (!_regex_expression.empty() and regex_match(str, _regex_expression))
    return true;

  if (!_values.empty())
    return (_legal_non_regex_values < str);

  return false;
}

const string EFT::value_to_log(const string& str) const
{ //if (is_legal_value(str))
  { const string rv = canonical_value(str);

    return (rv.empty() ? str : rv);

//    if (!rv.empty())
//      return rv;
//
//    return str;
  }
//  else
//    return string();
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

