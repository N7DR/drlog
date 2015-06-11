// $Id: rules.cpp 104 2015-05-20 16:59:12Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file rules.cpp

        Classes and functions related to the contest rules
*/

#include "exchange.h"
#include "macros.h"
#include "qso.h"
#include "rules.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>
#include <utility>

using namespace std;
using namespace std::placeholders;

pt_mutex rules_mutex;                   ///< mutex for the contest_rules object

extern const set<string> CONTINENT_SET; ///< the abbreviations for the continents
extern message_stream ost;              ///< for debugging and logging

typedef std::map<std::string, unsigned int> MSI;        // syntactic sugar

// -------------------------  exchange_field_values  ---------------------------

/*!     \class exchange_field_values
        \brief Encapsulates the name and legal values for an exchange field
*/

/*! \brief      Add a canonical value
    \param  cv  canonical value to add

    Also adds <i>cv</i> as a possible value
*/
void exchange_field_values::add_canonical_value(const string& cv)
{ if (_values.find(cv) == _values.end())
    _values.insert( { cv, set<string>( { cv } ) } );
}

/*! \brief      Add a possible value
    \param  cv  canonical value to which <i>v</i> is to be added
    \param  v   value to be added

    Also adds <i>cv</i> as a canonical value if it does not already exist
*/
void exchange_field_values::add_value(const string& cv, const string& v)
{ add_canonical_value(cv);

  set<string>& ss = _values[cv];

  ss.insert(v);
}

/*! \brief      Number of possible values for a particular canonical value
    \param  cv  canonical value
    \return     number of possible values for the canonical value <i>cv</i>

    Returns 0 if the canonical value does not exist
*/
const size_t exchange_field_values::n_values(const string& cv) const
{ const auto posn = _values.find(cv);

  return ( (posn == _values.end()) ? 0 : posn->second.size() );
}

/*! \brief      Get all the legal values for a single canonical value
    \param  cv  canonical value
    \return     all the legal values corresponding to the canonical value <i>cv</i>

    Returns empty set if the canonical value does not exist
*/
const set<std::string> exchange_field_values::values(const string& cv) const
{ const auto posn = _values.find(cv);

  return ( (posn == _values.end()) ? set<string>() : posn->second );
}

/*! \brief      Get all the canonical values
    \return     all the canonical values

    Returns empty set if there are no canonical values
*/
const set<string> exchange_field_values::canonical_values(void) const
{ set<string> rv;

  FOR_ALL(_values, [&rv] (const pair<string, set<string>>& psss) { rv.insert(psss.first); } );

  return rv;
}

/*! \brief      Get all the legal values (for all canonical values)
    \return     all possible legal values for all canonical values

    Returns empty set if there are no canonical values
*/
const set<string> exchange_field_values::all_values(void) const
{ set<string> rv;

  for (const auto& cvv : _values)
    COPY_ALL(cvv.second, inserter(rv, rv.begin()));

  return rv;
}

/*! \brief          Is a string a legal value (for any canonical value)
    \param  value   value to be tested
    \return         whether <i>value</i> is a legal value of any canonical value
*/
const bool exchange_field_values::is_legal_value(const string& value) const
{ for (const auto& cv : canonical_values())  // for each canonical value
  { if (is_legal_value(cv, value))
        return true;
  }

  return false;
}

/*! \brief                  Is a particular value legal for a given canonical value?
    \param  cv              canonical value
    \param  putative_value  value to test
    \return                 whether <i>putative_value</i> is a legal value for the canonical value <i>cv</i>
*/
const bool exchange_field_values::is_legal_value(const string& cv, const string& putative_value) const
{ if (!is_legal_canonical_value(cv))
    return false;

  const auto posn = _values.find(cv);
  const set<string>& ss = posn->second;

  return ( ss.find(putative_value) != ss.cend() );
}

// -------------------------  exchange_field  ---------------------------

/*!     \class exchange_field
        \brief Encapsulates the name for an exchange field, and whether it's a mult
*/

/*! \brief          Construct from name, multiplier and optional status
    \param  nm      name of field
    \param  mult    whether field is a mult
    \param  opt     whether field is optional

    Also the default constructor
*/
exchange_field::exchange_field(const string& nm, const bool mult, const bool opt) :
  _name(nm),
  _is_mult(mult),
  _is_optional(opt)
{ }

/*! \brief  Follow all trees to their leaves
    \return The exchange field, expanded recursively into all possible choices
*/
const vector<exchange_field> exchange_field::expand(void) const
{ vector<exchange_field> rv;

  if (!is_choice())
  { rv.push_back(*this);
    return rv;
  }

// it's a choice
  for (const auto& this_choice : _choice)
  { const vector<exchange_field>& vec = this_choice.expand();   // recursive

    COPY_ALL(vec, back_inserter(rv));          // append to rv
  }

  return rv;
}

/// ostream << exchange_field
ostream& operator<<(ostream& ost, const exchange_field& exch_f)
{ ost << "exchange_field.name() = " << exch_f.name() << endl
      << boolalpha << "exchange_field.is_mult() = " << exch_f.is_mult() << endl
      << "exchange_field.is_optional() = " << exch_f.is_optional() << endl
      << "exchange_field.is_choice() = " << exch_f.is_choice() << noboolalpha;

  if (exch_f.is_choice())
  { ost << endl
        << "CHOICE: " << endl;

    for (size_t n = 0; n < exch_f.choice().size(); ++n)
    { ost << exch_f.choice()[n];

      if (n != exch_f.choice().size() - 1)
        ost << endl;
    }
  }

  return ost;
}

// -------------------------  points_structure  ---------------------------

/*!     \class points_structure
        \brief Encapsulate the vagaries of points-per-QSO rules
*/

/// constructor
points_structure::points_structure(void) :
  _points_type(POINTS_NORMAL)
{ }

// -------------------------  contest_rules  ---------------------------

/*!     \class contest_rules
        \brief A place to maintain all the rules

        This object should be created and initialized early, and from that
        point it should be treated as read-only. Being paranoid, though, there's still lots of
        internal locking, since there are ongoing debates about the true thread safety of
        strings in libstdc++.
*/

/*! \brief              Parse and incorporate the "QTHX[xx] = " lines from context
    \param  context     drlog context
    \param  location_db location database

    Incorporates the parsed information into _exch_values
*/
void contest_rules::_parse_context_qthx(const drlog_context& context, location_database& location_db)
{ const auto& context_qthx = context.qthx();

  if (context_qthx.empty())
    return;

  SAFELOCK(rules);

  for (const auto this_qthx : context_qthx)
  { const string canonical_prefix = location_db.canonical_prefix(this_qthx.first);
    const set<string> ss = this_qthx.second;
    exchange_field_values qthx;

    qthx.name(string("QTHX[") + canonical_prefix + "]");

    for (const auto this_value : ss)
    { if (!contains(this_value, "|"))
        qthx.add_canonical_value(this_value);
      else
      { const vector<string> equivalent_values = remove_peripheral_spaces(split_string(this_value, "|"));

        if (!equivalent_values.empty())
          qthx.add_canonical_value(equivalent_values[0]);

        for (size_t n = 1; n < equivalent_values.size(); ++n)           // n = 0 corresponds to the canonical value
          qthx.add_value(equivalent_values[0], equivalent_values[n]);
      }
    }

    _exch_values.push_back(qthx);
  }
}

/*! \brief              Private function used to obtain all the understood values for a particular exchange field
    \param  field_name  name of the field for which the understood values are required
    \return             set of all the legal values for the field <i>field_name</i>

    Uses the variable <i>_exch_values</i> to obtain the returned value
*/
const set<string> contest_rules::_all_exchange_values(const string& field_name) const
{ SAFELOCK(rules);

  const auto cit = FIND_IF(_exch_values, [=] (const exchange_field_values& efv) { return (efv.name() == field_name); } );

  return ( (cit == _exch_values.cend()) ? set<string>() : cit->all_values() );
}

/*! \brief                      Parse exchange line from context
    \param  exchange_fields     container of fields taken from line in configuration file
    \param  exchange_mults_vec  container of fields that are mults
    \return                     container of detailed information about each exchange field in <i>exchange_fields</i>
*/
const vector<exchange_field> contest_rules::_inner_parse(const vector<string>& exchange_fields , const vector<string>& exchange_mults_vec) const
{ vector<exchange_field> rv;

  for (const auto& field_name : exchange_fields)
  { const bool is_choice = contains(field_name, "CHOICE:");
    const bool is_opt = contains(field_name, "OPT:");

    if (is_choice)
    { const vector<string> choice_vec = split_string(field_name, ":");
      string full_name;  // pseudo name of the choice

       if (choice_vec.size() == 2)    // true if legal
       { vector<string> choice_fields = remove_peripheral_spaces(split_string(choice_vec[1], "/"));
         vector<exchange_field> choices;

         for (auto& choice_field_name : choice_fields)
         { const bool is_mult = find(exchange_mults_vec.cbegin(), exchange_mults_vec.cend(), choice_field_name) != exchange_mults_vec.cend();
           exchange_field this_choice(choice_field_name, is_mult);

           choices.push_back(this_choice);
         }

// put into alphabetical order
         sort(choice_fields.begin(), choice_fields.end());

         for (auto& choice_field_name : choice_fields)
           full_name += choice_field_name + "+";

         exchange_field this_field(substring(full_name, 0, full_name.length() - 1), false);  // name is of form CHOICE1+CHOICE2

         this_field.choice(choices);
         rv.push_back(this_field);
       }
     }

    if (is_opt)
    { try
      { const string name = split_string(field_name, ":")[1];
        const bool is_mult = find(exchange_mults_vec.cbegin(), exchange_mults_vec.cend(), name) != exchange_mults_vec.cend();

        rv.push_back( exchange_field(name, is_mult, is_opt) );
      }

      catch (...)
      { ost << "Error parsing OPT exchange field: " << field_name << endl;
        exit(-1);
      }
    }

    if (!is_choice and !is_opt)
    { const bool is_mult = find(exchange_mults_vec.cbegin(), exchange_mults_vec.cend(), field_name) != exchange_mults_vec.cend();

      rv.push_back( exchange_field(field_name, is_mult) );
    }
  }

  return rv;
}

/*! \brief              Parse the "exchange = " and all the "exchange [xx] = " lines from context
    \param  context     drlog context
    \return             name/mult/optional/choice status for exchange fields

    Puts correct values in _received_exchange
*/
void contest_rules::_parse_context_exchange(const drlog_context& context)
{
// generate vector of all permitted exchange fields
  map<string, vector<string>> permitted_exchange_fields;  // use a map so that each value is inserted only once

  const auto& per_country_exchanges = context.exchange_per_country();

  for (const auto& pce : per_country_exchanges)
  { const vector<string> vs = remove_peripheral_spaces(split_string(pce.second, ","));

    permitted_exchange_fields.insert( { pce.first, vs } );   // unexpanded choice
  }

  for (const auto& pce : per_country_exchanges)
  { const vector<string> vs = remove_peripheral_spaces(split_string(pce.second, ","));
    set<string> ss;

    for (auto str : vs)
    { if (begins_with(str, "CHOICE:"))
        str = substring(str, 7);

      const vector<string> expanded_choice = remove_peripheral_spaces(split_string(str, "/"));

      for (const auto& s : expanded_choice)
        ss.insert(s);
    }

    _per_country_exchange_fields.insert( { pce.first, ss } );
  }

// add the ordinary exchange to the permitted exchange fields
  const vector<string> exchange_vec = remove_peripheral_spaces(split_string(context.exchange(), ","));
  permitted_exchange_fields.insert( { "", exchange_vec } );

  const vector<string> exchange_mults_vec = remove_peripheral_spaces(split_string(context.exchange_mults(), ","));
  map<string, vector<exchange_field>> single_mode_rv_rst;
  map<string, vector<exchange_field>> single_mode_rv_rs;

  for (const auto& mpef : permitted_exchange_fields)
  { const vector<string>& vs = mpef.second;
    vector<exchange_field> vef = _inner_parse(vs, exchange_mults_vec);
    vector<exchange_field> vef_rst;
    vector<exchange_field> vef_rs;

// adjust RST/RS to match mode; ideally we would have done this later, when
// it would be far simpler to iterate through the map, changing values, but
// g++'s bizarre restriction that values of maps cannot be altered in-place
// makes it easier to do it now

// RST
    for (auto ef : vef)
    { if (ef.name() == "RS")
        ef.name("RST");

      vef_rst.push_back(ef);
    }

// RS
    for (auto ef : vef)
    { if (ef.name() == "RST")
        ef.name("RS");

      vef_rs.push_back(ef);
    }

    single_mode_rv_rst.insert( {  mpef.first, vef_rst } );
    single_mode_rv_rs.insert( {  mpef.first, vef_rs } );
  }

  for (const auto& m : _permitted_modes)
    _received_exchange.insert( { m, ( (m == MODE_CW) ? single_mode_rv_rst : single_mode_rv_rs ) } );
}

/*! \brief              Initialize an object that was created from the default constructor
    \param  context     drlog context
    \param  location_db location database

    After calling this function, the object is ready for use
*/
void contest_rules::_init(const drlog_context& context, location_database& location_db)
{ const vector<string> path = context.path();

// personal information, taken from context
  _my_continent = context.my_continent();
  _my_country = location_db.canonical_prefix(context.my_call());
  _my_cq_zone = context.my_cq_zone();
  _my_grid = context.my_grid();
  _my_itu_zone = context.my_itu_zone();

// on which band(s) and mode(s) are we scoring?
  _score_bands = context.score_bands();
  _original_score_bands = _score_bands;
  _score_modes = context.score_modes();
  _original_score_modes = _score_modes;

  _send_qtcs = context.qtcs() and (_my_continent != "EU");

// generate the country mults; the value from context is either "ALL" or "NONE"
  if (context.country_mults_filter() == "ALL")
    copy(_countries.cbegin(), _countries.cend(), inserter(_country_mults, _country_mults.begin()));

  if (CONTINENT_SET < context.country_mults_filter())
  { const string target_continent = context.country_mults_filter();

    copy_if(_countries.cbegin(), _countries.cend(), inserter(_country_mults, _country_mults.begin()), [=, &location_db] (const string& cp) {return (location_db.continent(cp) == target_continent); } );
  }

// remove any that are explicitly not allowed
  vector<string> not_country_mults_vec = remove_peripheral_spaces(split_string(context.not_country_mults(), ","));  // may not be actual canonical prefixes

  FOR_ALL(not_country_mults_vec, [&] (const string& not_country_mult) { _country_mults.erase(location_db.canonical_prefix(not_country_mult)); } );

// are any mults derived from callsigns?
  _callsign_mults = context.callsign_mults();                    // e.g., "WPXPX"
  _callsign_mults_per_band = context.callsign_mults_per_band();
  _callsign_mults_per_mode = context.callsign_mults_per_mode();
  _callsign_mults_used = !_callsign_mults.empty();

  _country_mults_used = !_country_mults.empty();
  _country_mults_per_band = context.country_mults_per_band();
  _country_mults_per_mode = context.country_mults_per_mode();
  _per_band_country_mult_factor = context.per_band_country_mult_factor();

// add the permitted modes
  const string modes = context.modes();

  if (contains(modes, "CW"))
    add_permitted_mode(MODE_CW);

//  if (contains(modes, "DIGI"))
//    add_permitted_mode(MODE_DIGI);

  if (contains(modes, "SSB"))
    add_permitted_mode(MODE_SSB);

// the sent exchange
  if (_permitted_modes < MODE_CW)
  { if (!context.sent_exchange_cw().empty())
      _sent_exchange_names.insert( { MODE_CW, context.sent_exchange_cw_names() } );
    else
      _sent_exchange_names.insert( { MODE_CW, context.sent_exchange_names() } );
  }

  if (_permitted_modes < MODE_SSB)
  { if (!context.sent_exchange_ssb().empty())
      _sent_exchange_names.insert( { MODE_SSB, context.sent_exchange_ssb_names() } );
    else
      _sent_exchange_names.insert( { MODE_SSB, context.sent_exchange_names() } );
  }

// add the permitted bands
  const vector<string> bands_vec = remove_peripheral_spaces( split_string(context.bands(), ",") );

  for (const auto& str : bands_vec)
  { try
    { _permitted_bands.push_back(BAND_FROM_NAME.at(str));
    }

    catch (...)
    { }
  }

  _parse_context_exchange(context);                     // define the legal receive exchanges, and which fields are mults
  _exchange_mults = remove_peripheral_spaces( split_string(context.exchange_mults(), ",") );
  _exchange_mults_per_band = context.exchange_mults_per_band();
  _exchange_mults_per_mode = context.exchange_mults_per_mode();
  _exchange_mults_used = !_exchange_mults.empty();

//  ost << "exchange mults: " << context.exchange_mults() << endl;

// build expanded version of _received_exchange
  for (const auto& m : _permitted_modes)
  { map<string, vector<exchange_field>> expanded_exch;
    auto& unexpanded_exch = _received_exchange.at(m);

    for (const auto& qth_vec_field : unexpanded_exch)
    { const string& prefix = qth_vec_field.first;
      const vector<exchange_field>& vef = qth_vec_field.second;
      vector<exchange_field> expanded_vef;

      for (const auto& field : vef)
      { if (!field.is_choice())
          expanded_vef.push_back(field);
        else
        { const vector<exchange_field> vec = field.expand();

          copy(vec.cbegin(), vec.cend(), back_inserter(expanded_vef));
        }
      }

      expanded_exch.insert( { prefix, expanded_vef } );
    }

    _expanded_received_exchange.insert( { m, expanded_exch} );

    for (const auto& psvef : expanded_exch)
    { const vector<exchange_field>& vef = psvef.second;

      for (const auto& ef : vef)
        _exchange_field_eft.insert( { ef.name(), EFT(ef.name(), context.path(), context.exchange_fields_filename(), context, location_db) } );
    }
  }

// define the points structure; this can be quite complex
  const points_structure empty_ps { };

// fill the points structure
  for (const auto& m : _permitted_modes)
  { auto& pb = _points[m];

    for (const auto& b : _permitted_bands)
    { pb.insert( {b, empty_ps} );  // default is no points

      points_structure points_this_band;
      MSI     country_points_this_band;
      MSI     continent_points_this_band;

// parse the config file
      const string context_points = context.points(b, m);

      if (context_points == "IARU")  // special
      { points_structure ps = pb[b];

        ps.points_type(POINTS_IARU);
        pb[b] = ps;
      }
      else
      { const vector<string> points_str_vec = remove_peripheral_spaces( split_string(context_points, ",") );

        for (unsigned int n = 0; n < points_str_vec.size(); ++n)
        { const string points_str = points_str_vec[n];
          const vector<string> fields = split_string(points_str, ":");

// default?
          if (fields.size() != 2 and fields.size() != 3)
            ost << "Ignoring points field: " << points_str << endl;
          else
          { if (fields.size() == 3)                              // three fields
            { bool processed = false;

              if (fields[0].empty() and fields[1].empty())
              { points_this_band.default_points(from_string<unsigned int>(fields[2]));
                processed = true;
              }

// country
              if (!processed and !fields[1].empty())
              { if (contains(fields[1], "["))    // possible multiple countries
                { const string countries = delimited_substring(fields[1], '[', ']');

                  if (!countries.empty())
                  { const vector<string> country_vec = remove_peripheral_spaces(split_string(remove_peripheral_spaces(squash(countries)), ' '));  // use space instead of comma because we've already split on commas

                    FOR_ALL(country_vec, [&] (const string& country) { country_points_this_band.insert( { location_db.canonical_prefix(country), from_string<unsigned int>(fields[2]) } ); } );
                  }
                }
                else
                  country_points_this_band.insert( { location_db.canonical_prefix(fields[1]), from_string<unsigned int>(fields[2]) } );

                points_this_band.country_points(country_points_this_band);

                processed = true;
              }

              if (!processed)
              { continent_points_this_band[fields[0]] = from_string<unsigned int>(fields[2]);
                points_this_band.continent_points(continent_points_this_band);
              }
            }
          }

// [field-name-condition]:points // [IOTA != ------]:15
          if (fields.size() == 2)
          { const string& f0 = fields[0];

            if ((f0.find("[") != string::npos) and (f0.find("]") != string::npos))
              _exchange_value_points.insert(make_pair(f0, from_string<unsigned int>(fields[1])));  // we keep the delimiters here; they are stripped before the comparison
          }

          pb[b] = points_this_band;    // overwrite default
        }
      }  // not IARU
    }
  }

// legal values for the exchange fields
  _parse_context_qthx(context, location_db);  // qth-dependent fields

  vector<exchange_field> leaves_vec;

  for (const auto& m : _permitted_modes)
  { const map<string, vector<exchange_field>>& unexpanded_exch = _received_exchange.at(m);

    for (auto cit = unexpanded_exch.cbegin(); cit != unexpanded_exch.cend(); ++cit)
    { const vector<exchange_field> vec_1 = cit->second;

      for (auto cit3 = vec_1.cbegin(); cit3 != vec_1.cend(); ++cit3)
      { const vector<exchange_field> vec = cit3->expand();

        copy(vec.cbegin(), vec.cend(), back_inserter(leaves_vec));
      }
    }
  }

  set<exchange_field> leaves(leaves_vec.cbegin(), leaves_vec.cend());

  for (const auto& ef : leaves)
  { static const set<string> no_canonical_values( { "RS", "RST", "SERNO" } );    // some field values don't have canonical values

    const string& field_name = ef.name();
    string entire_file;
    bool   read_file_ok = false;

    if (!(no_canonical_values < field_name))
    { try
      { entire_file = read_file(path, field_name + ".values");
        read_file_ok = true;
      }

      catch (...)
      {
      }
    }

// parse file
    if (read_file_ok)
    { vector<string> lines = split_string(entire_file, EOL_CHAR);
      map<string /* canonical value */, set<string> > map_canonical_to_all;

      for (const auto& line : lines)
      { string lhs;
        set<string> equivalent_values;    // includes the canonical

        if (!line.empty() and line[0] != ';' and !starts_with(line, "//") ) // ";" and "//" introduce comments
        { if (contains(line, "=") )
          { vector<string> lhsrhs = split_string(line, "=");

            lhs = remove_peripheral_spaces(lhsrhs[0]);
            equivalent_values.insert(lhs);                  // canonical value

            if (lhsrhs.size() != 1)
            { const string& rhs = lhsrhs[1];
              const vector<string> remaining_equivalent_values = remove_peripheral_spaces(split_string(rhs, ","));

              FOR_ALL(remaining_equivalent_values, [&] (const string& rev) { equivalent_values.insert(rev); });

              map_canonical_to_all.insert( { lhs, equivalent_values });
            }
          }
          else    // no "="
          { const string str = remove_peripheral_spaces(line);

            if (!str.empty())
              map_canonical_to_all.insert( { str, set<string>{ str } } );
          }
        }
      }
// processed all lines
      _exch_values.push_back( { field_name, map_canonical_to_all } );    // pair<string, map<string, set<string> > >
    }
    else
      _exch_values.push_back( { field_name, map<string, set<string> >() } );    // pair<string, map<string, set<string> > >
  }

// generate the sets of all acceptable values
  const set<string> field_names = all_known_field_names();

  for (auto field_name : field_names)
    _permitted_exchange_values.insert( { field_name, _all_exchange_values(field_name) } );

// generate all the mappings from permitted to canonical values
  for (auto cit = _exch_values.cbegin(); cit != _exch_values.cend(); ++cit)
    _permitted_to_canonical.insert( { cit->name(), INVERT_MAPPING(cit->values()) } );
}

/// default constructor
contest_rules::contest_rules(void) :
  _callsign_mults_used(false),
  _country_mults_used(false),
  _exchange_mults_used(false),
  _send_qtcs(false)
{ }

/*! \brief              Construct an object ready for use
    \param  context     context for this contest
    \param  location_db location database
*/
contest_rules::contest_rules(const drlog_context& context, location_database& location_db) :
  _work_if_different_band(context.qso_multiple_bands()),
  _work_if_different_mode(context.qso_multiple_modes()),
  _countries(location_db.countries()),
  _callsign_mults_used(false),
  _country_mults_used(false),
  _exchange_mults_used(false)
{ _init(context, location_db);
}

/*! \brief              Prepare for use an object that was created from the default constructor
    \param  context     context for this contest
    \param  location_db location database
*/
void contest_rules::prepare(const drlog_context& context, location_database& location_db)
{ _work_if_different_band = context.qso_multiple_bands();
  _work_if_different_mode = context.qso_multiple_modes();
  _countries = location_db.countries();

  _init(context, location_db);
}

/*! \brief                      Get the expected exchange fields for a particular canonical prefix
    \param  canonical_prefix    canonical prefix
    \return                     the exchange fields associated with <i>canonical_prefix</i>

    CHOICE fields are NOT expanded
*/
const vector<exchange_field> contest_rules::exch(const string& canonical_prefix, const MODE m) const
{ if (canonical_prefix.empty())
    return vector<exchange_field>();

  SAFELOCK(rules);

  const map<string, vector<exchange_field>>& unexpanded_exchange = _received_exchange.at(m);
  auto cit = unexpanded_exchange.find(canonical_prefix);

  if (cit != unexpanded_exchange.cend())
    return cit->second;

  cit = unexpanded_exchange.find(string());

  return ( (cit == unexpanded_exchange.cend()) ? vector<exchange_field>() : cit->second );
}

/*! \brief                      Get the expected exchange fields for a particular canonical prefix
    \param  canonical_prefix    canonical prefix
    \return                     the exchange fields associated with <i>canonical_prefix</i>

    CHOICE fields ARE expanded
*/
const vector<exchange_field> contest_rules::expanded_exch(const string& canonical_prefix, const MODE m) const
{ if (canonical_prefix.empty())
    return vector<exchange_field>();

  SAFELOCK(rules);

  const map<string, vector<exchange_field>>& expanded_exchange = _expanded_received_exchange.at(m);
  auto cit = expanded_exchange.find(canonical_prefix);

  if (cit != expanded_exchange.cend())
    return cit->second;

  cit = expanded_exchange.find(string());

  return ( (cit == expanded_exchange.cend()) ? vector<exchange_field>() : cit->second );
}

/// Get all the known names of exchange fields (for all modes)
const set<string> contest_rules::all_known_field_names(void) const
{ set<string> rv;

  SAFELOCK(rules);

  for (const auto& m : _permitted_modes)
  { const map<string, vector<exchange_field>>& expanded_exchange = _expanded_received_exchange.at(m);

    for (const auto& msvef : expanded_exchange)
    { const vector<exchange_field>& vef = msvef.second;

      for (const auto& ef : vef)
        rv.insert(ef.name());
    }
  }

  return rv;
}

/*! \brief              The exchange field template corresponding to a particular field
    \param  field_name  name of the field
    \return             the exchange field information associated with <i>field_name</i>

    Returned EFT("none") if <i>field_name</i> is unknown.
*/
const EFT contest_rules::exchange_field_eft(const string& field_name) const
{ SAFELOCK(rules);

  const auto v = _exchange_field_eft.find(field_name);

  if (v == _exchange_field_eft.end())
    return EFT("none");

  return (v->second);
}

// returns empty vector if no acceptable values are found (e.g., RST, RS, SERNO)
const vector<string> contest_rules::exch_canonical_values(const string& field_name) const
{ vector<string> rv;

  { SAFELOCK(rules);

    for (unsigned int n = 0; n < _exch_values.size(); ++n)
    { if (_exch_values[n].name() == field_name)
      { const map<string, set<string> >& m = _exch_values[n].values();

        for_each(m.cbegin(), m.cend(), [&rv] (const map<string, set<string> >::value_type& mss) { rv.push_back(mss.first); } );  // Josuttis 2nd ed., p. 338

        sort(rv.begin(), rv.end());
      }
    }
  }

  return rv;
}

/*! \brief                      Add a canonical value for a particular exchange field
    \param  field_name          name of an exchange field (received)
    \param  new_canonical_value the canonical value to add

    Also adds <i>new_canonical_value</i> to the legal values for the field <i>field_name</i>. Does nothing
    if <i>new_canonical_value</i> is already a canonical value.
*/
void contest_rules::add_exch_canonical_value(const string& field_name, const string& new_canonical_value)
{ SAFELOCK(rules);

  bool found_it = false;

  for (unsigned int n = 0; n < _exch_values.size() and !found_it; ++n)
  { if (_exch_values[n].name() == field_name)
    { found_it = true;

      _exch_values[n].add_canonical_value(new_canonical_value);
    }
  }
}

#if 0
/// all the equivalent values for exchange fields with such equivalences
  std::vector                                              /* one entry for each exchange field */
    <std::pair
      <std::string,                                        /* exch field name */
          std::map
           <std::string,                                   /* a canonical field value */
             std::set                                      /* each equivalent value is a member of the vector, including the canonical value */
               <std::string                                /* indistinguishable legal values */
                  > > > >                       _exch_values;
#endif

/*! \brief                           Is a particular string a canonical value for a particular exchange field?
    \param  field_name               name of an exchange field (received)
    \param  putative_canonical_value the value to check

    Returns false if <i>field_name</i> is unrecognized
*/
const bool contest_rules::is_canonical_value(const string& field_name, const string& putative_canonical_value) const
{ SAFELOCK(rules);

  for (unsigned int n = 0; n < _exch_values.size(); ++n)
  { if (_exch_values[n].name() == field_name)
      return _exch_values[n].is_legal_canonical_value(putative_canonical_value);
  }

  return false;
}

/*! \brief                           Is a particular string a legal value for a particular exchange field?
    \param  field_name               name of an exchange field (received)
    \param  putative_value           the value to check

    Returns false if <i>field_name</i> is unrecognized
*/
const bool contest_rules::is_legal_value(const string& field_name, const string& putative_value) const
{ SAFELOCK(rules);

  for (const auto& pev : _permitted_exchange_values)
  { const string& this_field_name = pev.first;

    if (this_field_name == field_name)
      return (pev.second < putative_value);
  }

  return false;
}

/*! \brief                           The permitted values for a field
    \param  field_name               name of an exchange field (received)

    Returns the empty set if the field <i>field_name</i> can take any value
*/
const set<string> contest_rules::exch_permitted_values(const string& field_name) const
{ static const set<string> empty_set( { } );

  SAFELOCK(rules);

  const auto it = _permitted_exchange_values.find(field_name);

  return (it == _permitted_exchange_values.cend() ? empty_set : it->second);
}

/*! \brief                  The canonical value for a field
    \param  field_name      name of an exchange field (received)
    \param  actual_value    actual received value of the field <i>field_name</i>
    \return                 Canonical value for the value <i>actual_value</i> for the field <i>field_name</i>

    Returns <i>actual_value</i> if there are no canonical values.
    Returns the empty string if <i>actual_value</i> is not a legal value for <i>field_name</i>
*/
const string contest_rules::canonical_value(const string& field_name, const string& actual_value) const
{ set<string> ss = exch_permitted_values(field_name);

  if (exch_permitted_values(field_name).empty())                         // if no permitted values => anything allowed
    return actual_value;

  if (!(exch_permitted_values(field_name) < actual_value))               // is the actual value a permitted value for this field?
    return string();
/*
  std::map
    <std::string,                                          // exch field name
      std::map
        <std::string,                                      // permitted value
          std::string                                      // canonical value
           > >                                  _permitted_to_canonical;
*/
  SAFELOCK(rules);

  const map<string, string>& p_to_c = _permitted_to_canonical.at(field_name);  // we don't get here if field_name isn't valid
  const auto cit = p_to_c.find(actual_value);

  return (cit == p_to_c.cend() ? actual_value : (cit->second));
}

/*! \brief                          Get the next mode
    \param  current_mode            Current mode
    \return                         Next mode in sequence

    Cycles through the available modes.
    Currently supports only MODE_CW and MODE_SSB
*/
const MODE contest_rules::next_mode(const MODE current_mode) const
{ SAFELOCK(rules);

  auto cit = _permitted_modes.find(current_mode);

  if (cit == _permitted_modes.cend())    // should never happen
    return *(_permitted_modes.cbegin());

  ++cit;

  if (cit == _permitted_modes.cend())
    return *(_permitted_modes.cbegin());

  return *cit;
}

/*! \brief      Add a band to the list of permitted bands
    \param  b   band to add

    Does nothing if <i>b</i> is already permitted
*/
void contest_rules::add_permitted_band(const BAND b)
{ SAFELOCK(rules);

  if (find(_permitted_bands.begin(), _permitted_bands.end(), b) == _permitted_bands.end())
    _permitted_bands.push_back(b);
}

/// get the next band up
const BAND contest_rules::next_band_up(const BAND current_band) const
{ SAFELOCK(rules);

  auto cit = find(_permitted_bands.begin(), _permitted_bands.end(), current_band);

  if (cit == _permitted_bands.cend())    // should never happen
    return *(_permitted_bands.cbegin());

  cit++;

  if (cit == _permitted_bands.cend())
    return *(_permitted_bands.cbegin());

  return *cit;
}

/// get the next band down
const BAND contest_rules::next_band_down(const BAND current_band) const
{ SAFELOCK(rules);

  auto cit = find(_permitted_bands.begin(), _permitted_bands.end(), current_band);

  if (cit == _permitted_bands.end())
    return *(_permitted_bands.begin());

  if (cit == _permitted_bands.begin())
    return _permitted_bands[_permitted_bands.size() - 1];

  cit--;

  return *cit;
}

const unsigned int contest_rules::points(const QSO& qso, location_database& location_db) const
{ const BAND b = qso.band();

  if (!(_score_bands < b))    // no points if we're not scoring this band
    return 0;

  const string call = qso.callsign();
  const string canonical_prefix = location_db.canonical_prefix(call);

  if (canonical_prefix == "NONE")      // unable to determine country
    return 0;

  const MODE m = qso.mode();
  const auto& pb = _points[m];          // select the correct mode

  SAFELOCK(rules);

  if (pb.find(b) == pb.cend())
  { ost << "Unable to find any points entries for band " << BAND_NAME[b] << endl;
    return 0;
  }

  const points_structure& points_this_band = pb.at(b);           // valid because of the prior check

  switch (points_this_band.points_type())
  { default :
    case POINTS_NORMAL :
    { const map<string, unsigned int>& country_points = points_this_band.country_points();
      auto cit = country_points.find(canonical_prefix);

      if (cit != country_points.cend())    // if points are defined for this country
        return cit->second;

      const map<string, unsigned int>& continent_points = points_this_band.continent_points();
      cit = continent_points.find(location_db.continent(call));

      if (cit != continent_points.cend())  // if points are defined for this continent
        return cit->second;

      return points_this_band.default_points();
    }

// IARU
//1 point per QSO with same zone or with HQ stations
//3 points per QSO with different zone on same continent
//5 points per QSO with different zone on different continent
    case POINTS_IARU :
    { unsigned int rv = 0;
      const bool same_zone = (_my_itu_zone == location_db.itu_zone(call));
      const string his_continent = location_db.continent(call);

      if (same_zone)
        rv = 1;

      if ( (!same_zone) and (_my_continent == his_continent) )
        rv = 3;

      if ( (!same_zone) and (_my_continent != his_continent) )
        rv = 5;

// is is an HQ station?
//      ost << "Trying to score QSO: " << qso << endl;

      const string society_value = qso.received_exchange("SOCIETY");

      ost << "SOCIETY field is " << (society_value.empty() ? "NOT" : "") << " present" << endl;

      if (!society_value.empty())
          ost << "SOCIETY value = " << society_value << endl;

      if (!society_value.empty())
        rv = 1;

      return rv;
    }
  }
}

/*! \brief              Define a new set of bands to be scored
    \param  new_bands   the set of bands to be scored

    Does nothing if <i>new_bands</i> is empty
*/
void contest_rules::score_bands(const set<BAND>& new_bands)
{ if (!new_bands.empty())
  { SAFELOCK(rules);

    _score_bands = new_bands;
  }
}

/*! \brief              Define a new set of modes to be scored
    \param  new_modes   the set of modes to be scored

    Does nothing if <i>new_modes</i> is empty
*/
void contest_rules::score_modes(const set<MODE>& new_modes)
{ if (!new_modes.empty())
  { SAFELOCK(rules);

    _score_modes = new_modes;
  }
}

/*! \brief          Is an exchange field a mult?
    \param  name    name of exchange field
    \return         whether <i>name</i> is an exchange mult

    Returns <i>false</i> if <i>name</i> is unrecognised
*/
const bool contest_rules::is_exchange_mult(const string& name) const
{ SAFELOCK(rules);

  return ( find(_exchange_mults.cbegin(), _exchange_mults.cend(), name) != _exchange_mults.cend() );
}

/*! \brief          Does the sent exchange include a particular field?
    \param  str     name of field to test
    \param  m       mode
    \return         whether the sent exchange for mode <i>m</i> includes a field with the name <i>str</i>
*/
const bool contest_rules::sent_exchange_includes(const std::string& str, const MODE m) const
{ SAFELOCK(rules);

  try
  { const auto& se = _sent_exchange_names.at(m);

    return (find(se.cbegin(), se.cend(), str) != se.cend());
  }

  catch (...)
  { ost << "ERROR: mode " << m << " is not a key in contest_rules::_sent_exchange_names" << endl;
    return false;
  }
}

const bool contest_rules::is_exchange_field_used_for_country(const string& field_name, const string& canonical_prefix) const
{  SAFELOCK(rules);

//  ost << "in contest_rules::is_exchange_field_used_for_country(): field_name = " << field_name << ", prefix = " << canonical_prefix << endl;

//     std::map<std::string /* field name */, EFT>   _exchange_field_eft;        ///< new place ( if NEW_CONSTRUCTOR is defined) for exchange field information
//  if (find(_exchange_field_eft.cbegin(), _exchange_field_eft.cend(), canonical_prefix) == _exchange_field_eft.cend())
//    return false;    // not a known field name
//   for (const auto& tmp : _exchange_field_eft)
//     ost << "exchange_field_eft name = " << tmp.first << endl;

    if (_exchange_field_eft.find(field_name)  == _exchange_field_eft.cend())
    { //ost << "could not find field name in _exchange_field_eft; returning false" << endl;
      return false;    // not a known field name
    }


// if a field appears in the per-country rules, are we in the right country?
//    std::map<std::string /* canonical prefix */, std::set<std::string> /* exchange field names */>  _per_country_exchange_fields;
    bool is_a_per_country_field = false;

    for (const auto& ssets : _per_country_exchange_fields)
    { //ost << "ssets.first = " << ssets.first << endl;

      if (ssets.second < field_name)
        is_a_per_country_field = true;

      if (ssets.first == canonical_prefix)
      { //for (const auto& str : ssets.second)
        //  ost << "set member = " << str << endl;

        return (ssets.second < field_name);
      }
    }

    if (is_a_per_country_field)
      return false;

    return true;  // a known field, and this is not a special country
}

// The intention here is to follow the WPX definition. It would be
// a lot more helpful if they actually supplied a strict algorithm rather than
// an ambiguous definition. What follows attempts to implement my guess as to what
// they mean.

extern location_database location_db;

/*
 *  1. A PREFIX is the letter/numeral combination which forms the first part of the amateur call.
 *  Examples: N8, W8, WD8, HG1, HG19, KC2, OE2, OE25, LY1000, etc. Any difference in the numbering,
 *  lettering, or order of same shall count as a separate prefix. A station operating from a DXCC
 *  entity different from that indicated by its call sign is required to sign portable. The portable
 *  prefix must be an authorized prefix of the country/call area of operation. In cases of portable
 *  operation, the portable designator will then become the prefix. Example: N8BJQ operating from
 *  Wake Island would sign N8BJQ/KH9 or N8BJQ/NH9. KH6XXX operating from Ohio must use an authorized
 *  prefix for the U.S. 8th district (/W8, /AD8, etc.). Portable designators without numbers will be
 *  assigned a zero (Ø) after the second letter of the portable designator to form the prefix.
 *  Example: PA/N8BJQ would become PAØ. All calls without numbers will be assigned a zero (Ø) after
 *  the first two letters to form the prefix. Example: XEFTJW would count as XEØ. Maritime mobile,
 *  mobile, /A, /E, /J, /P, or other license class identifiers do not count as prefixes.
 *
 *  I wish that they would provide an algorithm; the above is pretty silly (what about G/<call>; I'm sure that's
 *  supposed to be G0, but the above doesn't say that, since there is only one letter in the "portable designator";
 *  it's also easy to construct cases to which the above isn't clearly intended to apply: surely they can't really
 *  intend that HB/<call> is to be scored as HB0, or OH/<call> as OH0, etc. But that's what it says. (Well, actually,
 *  it doesn't even say that, since it says "zero" (i.e., U+0030), but then uses U+00D8 in the example; one is left
 *  trying to guess from this mess what they might have meant.)
 *
 *  They must have an algorithm in use in the log checking software, so why isn't public? I don't understand
 *  what is served by keeping the actual algorithm secret and publishing instead an ambiguous and incomplete
 *  description.
 *
 */

/*! \brief          Return the WPX prefix of a call
    \param  call    callsign for which the WPX prefix is desired
    \return         the WPX prefix corresponding to <i>call</i>
*/
const string wpx_prefix(const string& call)
{
// callsign has to contain three characters
  if (call.length() < 3)
    return string();

  static const string digits( { "0123456789" } );
  string callsign = call;
  char portable_district { 0 } ;   // portable call district

// make sure we deal with AA1AA/M/QRP

// /QRP -- deal with this first
  if (last(callsign, 4) == "/QRP")
    callsign = substring(callsign, 0, callsign.length() - 4);

// remove portable designators
  if (penultimate_char(callsign) == '/')
  { static const string portables( { "AEJMP" } );

    if (portables.find(last_char(callsign)) != string::npos)
      callsign = substring(callsign, 0, callsign.length() - 2);
    else
      if (callsign.find_last_of(digits) == callsign.length() - 1)
      { portable_district = callsign[callsign.length() - 1];
        callsign = substring(callsign, 0, callsign.length() - 2);
      }
  }

// /MM, /MA, /AM
  if (antepenultimate_char(callsign) == '/')
  { static const set<string> mobiles( {"AM", "MA", "MM"} );

    if (mobiles < last(callsign, 2))
      callsign = substring(callsign, 0, callsign.length() - 3);
  }

// trivial -- and almost unknown -- case first: no digits
  if (callsign.find_first_of(digits) == string::npos)
    return (substring(callsign, 0, 2) + "0");

  size_t slash_posn = callsign.find('/');

  if ( (slash_posn == string::npos) or (slash_posn == callsign.length() - 1) )
  { const size_t last_digit_posn = callsign.find_last_of(digits);

    if (portable_district)
      callsign[last_digit_posn] = portable_district;

    return substring(callsign, 0, min(callsign.length(), last_digit_posn + 1));
  }

// we have a (meaningful) slash in the call
  const string left = substring(callsign, 0, slash_posn);
  const string right = substring(callsign, slash_posn + 1);

  const size_t left_size = left.size();
  const size_t right_size = right.size();

  if (left_size == right_size)
  { const string left_canonical_prefix = location_db.canonical_prefix(left);
    const string right_canonical_prefix = location_db.canonical_prefix(right);

    if (left_canonical_prefix == left)
      return left;

    if (right_canonical_prefix == right)
      return right;
  }

  string designator = (left_size < right_size ? left : right);

//  ost << "designator = " << designator << endl;

  if (designator.find_first_of(digits) == string::npos)
    designator += "0";

//  const size_t last_digit_posn = designator.find_last_of(digits);
//  const string rv = substring(designator, 0, min(callsign.length(), last_digit_posn + 1));

  string rv = designator;

  if (rv.length() == 1)
  { if (rv[0] == call[0])  // if just the first character, add the next character (to deal with 7QAA)
      rv = substring(call, 0, 2);
  }

//  ost << "WPX prefix for " << callsign << " is: " << rv << endl;

//  return substring(designator, 0, min(callsign.length(), last_digit_posn + 1));
  return rv;
}

/*! \brief          The SAC prefix for a particular call
    \param  call    call for which the prefix is to be calculated
    \return         the SAC prefix corresponding to <i>call</i>

    The SAC rules as written do not allow for weird prefixes such as LA100, etc.
*/
const string sac_prefix(const string& call)
{ static const set<string> scandinavian_countries { "JW", "JX", "LA", "OH", "OH0", "OJ0", "OX", "OY", "OZ", "SM", "TF" };
  const string canonical_prefix = location_db.canonical_prefix(call);

  if ( !(scandinavian_countries < canonical_prefix) )
    return string();

// it is a scandinavian call
  const string wpx = wpx_prefix(call);

// working from the end, find the first non-digit
  size_t last_letter_posn = wpx.find_last_not_of("0123456789");
  const string digits = substring(wpx, last_letter_posn + 1);

  if (digits.empty())
    return string();    // to handle case of something like "SM" as the passed call, which happens as a call is being typed

  if (canonical_prefix != "OH0" and canonical_prefix != "OJ0")
  { const string prefix = canonical_prefix + digits;

    return prefix;
  }
  else
  { return canonical_prefix;
  }
}

/*
Svalbard and Bear Island JW
Jan Mayen JX
Norway LA – LB – LG – LJ – LN
Finland OF – OG – OH – OI
Aland Islands OFØ – OGØ – OHØ
Market Reef OJØ
Greenland OX – XP
Faroe Islands OW – OY
Denmark 5P – 5Q – OU – OV – OZ
Sweden 7S – 8S – SA – SB – SC – SD – SE – SF – SG – SH – SI – SJ – SK – SL – SM
Iceland TF
*/

/*! \brief  Given a received value of a particular multiplier field, what is the actual mult value?
    \param  field_name         name of the field
    \param  received_value     received value for field <i>field_name</i>
    \return                    The multiplier value for the field <i>field_name</i>

    For example, the mult value for a DOK field with the value A01 is A.
*/
const string MULT_VALUE(const string& field_name, const string& received_value)
{ if (field_name == "DOK")
  { if (!received_value.empty())
    { const auto posn = received_value.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

      if (posn == string::npos)
        return string();
      else
        return (create_string(received_value[posn]));
    }
    else        // should never happen: DOK with no value
    { ost << "Error: DOK with no value" << endl;
      return string();
    }
  }
  else
    return received_value;
}
