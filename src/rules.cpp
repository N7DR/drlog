// $Id: rules.cpp 250 2024-08-12 15:16:35Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   rules.cpp

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

pt_mutex rules_mutex { "RULES"s };      ///< mutex for the contest_rules object

extern const set<string> CONTINENT_SET; ///< the abbreviations for the continents

extern message_stream    ost;           ///< for debugging and logging
extern location_database location_db;   ///< location information

extern void alert(const string& msg, const bool show_time = true);  ///< Alert the user

using MSI = map<string, unsigned int>;                    ///< syntactic sugar

// -------------------------  choice_equivalents  ---------------------------

/*! \class  choice_equivalents
    \brief  Encapsulates the possibilities for a CHOICE received exchange

    Assumes that CHOICEs are in pairs
*/

/*! \brief              Add a pair of equivalent fields
    \param  ch1_ch2     pair: first element of choice, second element of choice
*/
void choice_equivalents::operator+=(const pair<string, string>& ch1_ch2)
{ _choices[ch1_ch2.first] = ch1_ch2.second;
  _choices[ch1_ch2.second] = ch1_ch2.first;
}

/*! \brief              Add a pair of equivalent fields in the form "FIELD1+FIELD2"
    \param  ch1_ch2     the two fields, separated by a plus sign

    Throws exception if <i>ch1_ch2<i> appears to be malformed
*/
void choice_equivalents::operator+=(const string& ch1_ch2)
{ if (number_of_occurrences(ch1_ch2, '+') != 1)
    throw exception();

  const vector<string> vec { clean_split_string <string> (ch1_ch2, '+') };

  *this += { vec[0], vec[1] };
}

/*! \brief              Add a pair of equivalent fields only if the form is "FIELD1+FIELD2"
    \param  ch1_ch2     the two fields, separated by a plus sign

    If <i>ch1_ch2</i> appears to be malformed, does not attempt to add.
*/
void choice_equivalents::add_if_choice(const string& ch1_ch2)  // add "FIELD1+FIELD2"
{ if (number_of_occurrences(ch1_ch2, '+') == 1)
    *this += ch1_ch2;
}

// -------------------------  exchange_field_values  ---------------------------

/*! \class  exchange_field_values
    \brief  Encapsulates the name and legal values for an exchange field
*/

/*! \brief      Add a canonical value
    \param  cv  canonical value to add

    Also adds <i>cv</i> as a possible value. Does nothing if <i>cv</i> is already
    present as a canonical value.
*/
void exchange_field_values::add_canonical_value(const string& cv)
{ //if (!(_values > cv))
  if (!(_values.contains(cv)))
    _values += { cv, set<string>( { cv } ) };
}

/*! \brief      Add a possible value
    \param  cv  canonical value to which <i>v</i> is to be added
    \param  v   value to be added

    Also adds <i>cv</i> as a canonical value if it does not already exist
*/
void exchange_field_values::add_value(const string& cv, const string& v)
{ add_canonical_value(cv);

  _values[cv] += v;
}

/*! \brief      Get all the canonical values
    \return     all the canonical values

    Returns empty set if there are no canonical values
*/
set<string> exchange_field_values::canonical_values(void) const
{ set<string> rv;

  for (const auto& [ cv, set_of_values ] : _values)
    rv += cv;

  return rv;
}

/*! \brief                  Is a particular value legal for a given canonical value?
    \param  cv              canonical value
    \param  putative_value  value to test
    \return                 whether <i>putative_value</i> is a legal value for the canonical value <i>cv</i>
*/
bool exchange_field_values::is_legal_value(const string& cv, const string& putative_value) const
{ if (!is_legal_canonical_value(cv))
    return false;

  const auto         posn { _values.find(cv) };               // must be != cend() if we get here
  const set<string>& ss   { posn->second };

  return ss.contains(putative_value);
}

// -------------------------  exchange_field  ---------------------------

/*! \class  exchange_field
    \brief  Encapsulates the name for an exchange field, and whether it's a mult/optional/choice
*/

/*! \brief      Follow all trees to their leaves
    \return     the exchange field, expanded recursively into all possible choices
*/
vector<exchange_field> exchange_field::expand(void) const
{ vector<exchange_field> rv;

  if (!is_choice())
    return ( rv += (*this), rv );

// it's a choice
  FOR_ALL(_choice, [&rv] (const auto& this_choice) { rv += this_choice.expand(); } );

  return rv;
}

string exchange_field::to_string(void) const
{ string rv { };

  rv += _name + " is mult: " + (_is_mult ? "true" : "false") + ", is optional: " + (_is_optional ? "true" : "false") + EOL;

  if (_choice.empty())
    rv += "No equivalents" + EOL;
  else
  { for (const auto& this_choice : _choice)
      rv += "  " + this_choice.name() + EOL;
  }

  return rv;
}

/*! \brief          Write an <i>exchange_field</i> object to an output stream
    \param  ost     output stream
    \param  exch_f  object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const exchange_field& exch_f)
{ ost << "exchange_field.name() = " << exch_f.name() << endl
      << boolalpha << "exchange_field.is_mult() = " << exch_f.is_mult() << endl
      << "exchange_field.is_optional() = " << exch_f.is_optional() << endl
      << "exchange_field.is_choice() = " << exch_f.is_choice() << noboolalpha;

  if (exch_f.is_choice())
  { ost << endl
        << "CHOICE: " << endl;

    for (size_t n { 0 }; n < exch_f.choice().size(); ++n)
    { ost << exch_f.choice()[n];

      if (n != exch_f.choice().size() - 1)
        ost << endl;
    }
  }

  return ost;
}

// -------------------------  contest_rules  ---------------------------

/*! \class  contest_rules
    \brief  A place to maintain all the rules

    This object should be created and initialized early, and from that
    point it should be treated as read-only. Being paranoid, though, there's still lots of
    internal locking, since there are ongoing debates about the true thread safety of
    strings in libstdc++.
*/

/*! \brief                  Parse and incorporate the "QTHX[xx] = " lines from context
    \param  context         drlog context
    \param  location_db     location database

    Incorporates the parsed information into _exch_values
*/
void contest_rules::_parse_context_qthx(const drlog_context& context, location_database& location_db)
{ const auto context_qthx { context.qthx() };

  SAFELOCK(rules);

  if (!context_qthx.empty())
  { //for (const auto& this_qthx : context_qthx)
    for (const auto& [str, ss] : context_qthx)
    { //const string      canonical_prefix { location_db.canonical_prefix(this_qthx.first) };
//      const set<string> ss               { this_qthx.second };
      const string canonical_prefix { location_db.canonical_prefix(str) };

      exchange_field_values qthx;

      qthx.name(string("QTHX["s) + canonical_prefix + "]"s);

      for (const auto& this_value : ss)
      { if (!contains(this_value, '|'))
          qthx.add_canonical_value(this_value);
        else
        { const vector<string> equivalent_values { clean_split_string <string> (this_value, '|') };

          if (!equivalent_values.empty())
            qthx.add_canonical_value(equivalent_values[0]);

          for_each(next(equivalent_values.cbegin()), equivalent_values.cend(), [cp = equivalent_values[0], &qthx] (const string& equivalent_value) { qthx += { cp, equivalent_value }; } ); // cbegin() corresponds to the canonical value
        }
      }

      _exch_values += qthx;
    }
  }
}

/*! \brief                      Get the expanded or unexpanded names of the exchange fields for a particular canonical prefix and mode
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \param  ch                  whether to expand choices
    \return                     the exchange field names associated with <i>canonical_prefix</i> and <i>m</i>
*/
vector<string> contest_rules::_exchange_field_names(const string& canonical_prefix, const MODE m, const CHOICES ch) const
{ vector<string> rv;

  FOR_ALL(_exchange_fields(canonical_prefix, m, ch), [&rv] (const exchange_field& ef) { rv += ef.name(); });

  return rv;
}

/*! \brief                      Get the expected exchange fields for a particular canonical prefix
    \param  canonical_prefix    canonical prefix
    \param  m                   mode
    \param  expand_choices      whether to expand CHOICE fields
    \return                     the exchange fields associated with <i>canonical_prefix</i>
*/
vector<exchange_field> contest_rules::_exchange_fields(const string& canonical_prefix, const MODE m, const CHOICES expand_choices) const
{ if (canonical_prefix.empty())
    return vector<exchange_field>();

  SAFELOCK(rules);

  try
  { const map<string, vector<exchange_field>>& exchange { ((expand_choices == CHOICES::EXPAND) ? _expanded_received_exchange.at(m) : _received_exchange.at(m) ) }; // this can throw exception if rig has been set to a mode that is not supported by the contest

    auto cit { exchange.find(canonical_prefix) };

    return ( (cit == exchange.cend()) ? MUM_VALUE(exchange, string { }) : cit->second );
  }

  catch (std::out_of_range& oor)
  { ost << "Out of Range error in contest_rules::_exchange_fields: " << oor.what() << endl;
    ost << "canonical prefix = " << canonical_prefix << ", mode = " << MODE_NAME[m] << ", expand_choices = " << ((expand_choices == CHOICES::EXPAND) ? "true" : "false") << endl;

    return vector<exchange_field> { };
  }
}

/*! \brief                      Parse exchange line from context
    \param  exchange_fields     container of fields taken from line in configuration file
    \param  exchange_mults_vec  container of fields that are mults
    \return                     container of detailed information about each exchange field in <i>exchange_fields</i>
*/
vector<exchange_field> contest_rules::_inner_parse(const vector<string>& exchange_fields , const vector<string>& exchange_mults_vec) const
{ vector<exchange_field> rv;

  for (const auto& field_name : exchange_fields)
  { const bool is_choice { contains(field_name, "CHOICE:"s) };
    const bool is_opt    { contains(field_name, "OPT:"s) };

    if (is_choice)
    { const vector<string> choice_vec { split_string <std::string> (field_name, ':') };

      if (choice_vec.size() == 2)       // true if legal
      { vector<string> choice_fields { clean_split_string <string> (choice_vec[1], '/') };

        vector<exchange_field> choices;

        FOR_ALL(choice_fields, [exchange_mults_vec, &choices] (auto& choice_field_name) { choices += exchange_field { choice_field_name, contains(exchange_mults_vec, choice_field_name) }; });

// put into alphabetical order
        SORT(choice_fields);

        string full_name;                 // A+B pseudo name of the choice

        FOR_ALL(choice_fields, [&full_name] (auto& choice_field_name) { full_name += (choice_field_name + '+'); });

        exchange_field this_field(substring <std::string> (full_name, 0, full_name.length() - 1), false);  // name is of form CHOICE1+CHOICE2

        this_field.choice(choices);
        rv += this_field;
      }
    }

    if (is_opt)
    { try
      { const string name { split_string <std::string> (field_name, ':').at(1) };

        rv += exchange_field(name, contains(exchange_mults_vec, name), is_opt);
      }

      catch (...)
      { ost << "Error parsing OPT exchange field: " << field_name << endl;
        exit(-1);
      }
    }

    if (!is_choice and !is_opt)
      rv += exchange_field { field_name, contains(exchange_mults_vec, field_name) };
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
  map<string /* canonical prefix */, vector<string> /* permitted values of exchange */> permitted_exchange_fields;  // use a map so that each value is inserted only once

  const auto& per_country_exchanges { context.exchange_per_country() };   // map, per-country exchanges; key = prefix-or-call; value = exchange

  for (const auto& [canonical_prefix, allowed_exchange_values] : per_country_exchanges)
    permitted_exchange_fields += { canonical_prefix, clean_split_string <string> (allowed_exchange_values) };   // unexpanded choice

//  for (const auto& pce : per_country_exchanges)
  for (const auto& [canonical_prefix, allowed_exchange_values] : per_country_exchanges)
  { set<string> ss;

    for (auto str : clean_split_string <string> (allowed_exchange_values))
    { str = remove_from_start <std::string> (str, "CHOICE:"sv);

      FOR_ALL(clean_split_string <string> (str, '/'), [&ss] (const string& s) { ss += s; } );
    }

    _per_country_exchange_fields += { canonical_prefix, ss };
  }

// add the ordinary exchange to the permitted exchange fields
  permitted_exchange_fields += { string { }, clean_split_string <std::string> (context.exchange()) };  // no canonical prefix for the default ordinary exchange

  const vector<string> exchange_mults_vec { clean_split_string <std::string> (context.exchange_mults()) };

  map<string, vector<exchange_field>> single_mode_rv_rst;
  map<string, vector<exchange_field>> single_mode_rv_rs;

//  for (const auto& mpef : permitted_exchange_fields)
  for (const auto& [ canonical_prefix, permitted_values ] : permitted_exchange_fields)
  { //const vector<string>& vs { mpef.second };
    const vector<string>& vs { permitted_values };

    vector<exchange_field> vef { _inner_parse(vs, exchange_mults_vec) };

// force the field name to <i>final_field_name</i> if it is <i>initial_field_name</i>
    auto vef_rs_rst = [&vef] (const string& initial_field_name, const string& final_field_name)
      { vector<exchange_field> rv;

        for (exchange_field ef : vef)
        { if (ef.name() == initial_field_name)
            ef.name(final_field_name);

          rv += move(ef);
        }

        return rv;
      };

// adjust RST/RS to match mode; ideally we would have done this later, when
// it would be far simpler to iterate through the map, changing values, but
// g++'s restriction that values of maps cannot be altered in-place
// makes it easier to do it now

//    single_mode_rv_rst += { mpef.first, vef_rs_rst("RS"s, "RST"s) }; // force to RST
//    single_mode_rv_rs += { mpef.first, vef_rs_rst("RST"s, "RS"s) };  // force to RS
    single_mode_rv_rst += { canonical_prefix, vef_rs_rst("RS"s, "RST"s) }; // force to RST
    single_mode_rv_rs += { canonical_prefix, vef_rs_rst("RST"s, "RS"s) };  // force to RS
  }

  for (const auto& m : _permitted_modes)
    _received_exchange += { m, ( (m == MODE_CW) ? single_mode_rv_rst : single_mode_rv_rs ) };
}

/*! \brief                  Initialize an object that was created from the default constructor
    \param  context         drlog context
    \param  location_db     location database

    After calling this function, the object is ready for use
*/
void contest_rules::_init(const drlog_context& context, location_database& location_db)
{ const vector<string> path { context.path() };

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

  _send_qtcs = context.qtcs() and (_my_continent != "EU"sv);
  _uba_bonus = context.uba_bonus();

  if (_uba_bonus)
    _bonus_countries += "ON"s;                  // weird UBA scoring adds bonus for QSOs with ON

// generate the country mults; the value from context is either "ALL" or "NONE" or a comma-separated list
  if (context.country_mults_filter() == "NONE"sv)
    _countries.clear();                             // remove concept of countries
  else
  { if (context.country_mults_filter() == "ALL"sv)
      ranges::copy(_countries, inserter(_country_mults, _country_mults.begin()));
    else
    { const vector<string> countries { clean_split_string <std::string> (context.country_mults_filter()) };

      FOR_ALL(countries, [this] (const string& prefix) { _country_mults += prefix; } );
    }
  }

  if (CONTINENT_SET.contains(context.country_mults_filter()))
  { const string target_continent { context.country_mults_filter() };

    ranges::copy_if(_countries, inserter(_country_mults, _country_mults.begin()), [target_continent, &location_db] (const string& cp) { return (location_db.continent(cp) == target_continent); } );
  }

// remove any country mults that are explicitly not allowed
  const vector<string> not_country_mults_vec { clean_split_string <std::string> (context.not_country_mults()) };  // may not be actual canonical prefixes

  FOR_ALL(not_country_mults_vec, [this, &location_db] (const string& not_country_mult) { _country_mults.erase(location_db.canonical_prefix(not_country_mult)); } );

// /MM stations
  _mm_country_mults = context.mm_country_mults();

// are any mults derived from callsigns?
  _callsign_mults = context.callsign_mults();                    // e.g., "WPXPX"
  _callsign_mults_per_band = context.callsign_mults_per_band();
  _callsign_mults_per_mode = context.callsign_mults_per_mode();
  _callsign_mults_used = !_callsign_mults.empty();

  _country_mults_per_band = context.country_mults_per_band();
  _country_mults_per_mode = context.country_mults_per_mode();
  _per_band_country_mult_factor = context.per_band_country_mult_factor();

// add the permitted modes
  const string modes { context.modes() };

  if (contains(modes, "CW"s))
    add_permitted_mode(MODE_CW);

//  if (contains(modes, "DIGI"))
//    add_permitted_mode(MODE_DIGI);

  if (contains(modes, "SSB"s))
    add_permitted_mode(MODE_SSB);

// the sent exchange
  if (_permitted_modes > MODE_CW)
    _sent_exchange_names += { MODE_CW, context.sent_exchange_cw().empty() ? context.sent_exchange_names() : context.sent_exchange_names(MODE_CW) };

  if (_permitted_modes > MODE_SSB)
    _sent_exchange_names += { MODE_SSB, context.sent_exchange_ssb().empty() ? context.sent_exchange_names() : context.sent_exchange_names(MODE_SSB) };

// add the permitted bands
  const vector<string> bands_vec { clean_split_string <std::string> (context.bands()) };

  for (const auto& str : bands_vec)
  { try
    { _permitted_bands += BAND_FROM_NAME.at(str);
    }

    catch (...)
    { }
  }

  _parse_context_exchange(context);                                                              // define the legal receive exchanges, and which fields are mults
  _exchange_mults = clean_split_string <std::string> (context.exchange_mults());

// DOKs are a single letter; create the complete set if they aren't in auto mode
  if ( contains(_exchange_mults, "DOK"s)  and !context.auto_remaining_exchange_mults("DOK"s) )
  { exchange_field_values dok_values { "DOK"s };

    FOR_ALL(UPPER_CASE_LETTERS, [&dok_values] (const char c) { dok_values.add_canonical_value(create_string(c)); } );

    _exch_values += dok_values;
  }

// create expanded version
  for (const string& unexpanded_exchange_mult_name : _exchange_mults)
  { if (!unexpanded_exchange_mult_name.starts_with("CHOICE:"s))            // not a choice
      _expanded_exchange_mults += unexpanded_exchange_mult_name;
    else
    { const vector<string> expanded_choice { clean_split_string <std::string> (remove_from_start <std::string> (unexpanded_exchange_mult_name, "CHOICE:"sv)) };

      FOR_ALL(expanded_choice, [this] (const string& ex_name) { if (!contains(_expanded_exchange_mults, ex_name))
                                                                  _expanded_exchange_mults += ex_name;
                                                              } );
    }
  }

  _exchange_mults_per_band = context.exchange_mults_per_band();
  _exchange_mults_per_mode = context.exchange_mults_per_mode();
  _exchange_mults_used = !_exchange_mults.empty();

// build expanded version of _received_exchange, and _choice_exchange_equivalents
  for (const auto& m : _permitted_modes)
  { map<string, vector<exchange_field>> expanded_exch;

    auto& unexpanded_exch              { _received_exchange.at(m) };
    auto& choice_equivalents_this_mode { _choice_exchange_equivalents[m] };     // map<prefix, choice_equivalents>

    for (const auto& qth_vec_field : unexpanded_exch)
    { const auto& [ prefix, vef ] { qth_vec_field };

      vector<exchange_field> expanded_vef;

      for (const auto& field : vef)
      { if (field.is_not_choice())
          expanded_vef += field;
        else
        { auto& choice_equivalents_this_mode_and_cp { choice_equivalents_this_mode[prefix] };     // map<prefix, choice_equivalents>; null prefix implies applies to all

          choice_equivalents_this_mode_and_cp += field.name();
          expanded_vef += field.expand();
        }
      }

      expanded_exch += { prefix, expanded_vef };
    }

    _expanded_received_exchange += { m, expanded_exch};

    for (const auto& [key, vef] : expanded_exch)
    { for (const exchange_field& ef : vef)
        _exchange_field_eft += { ef.name(), EFT(ef.name(), context.path(), context.exchange_fields_filename(), context, location_db) };
    }
  }

// define the points structure; this can be quite complex
  const points_structure EMPTY_PS { };

// fill the points structure
  for (const auto& m : _permitted_modes)
  { auto& pb { _points[m] };

    for (const auto& b : _permitted_bands)
    { pb += { b, EMPTY_PS };  // default is no points

      points_structure points_this_band;
      MSI              continent_points_this_band;
      MSI              country_points_this_band;

// parse the config file
      string context_points { context.points_string(b, m) };

      static const set<string> special_points_set { "IARU"s, "STEW"s };

      const bool is_special_points { ( special_points_set.contains(context_points) ) };

      if (is_special_points)
      { if (context_points == "IARU"sv)  // special
          pb[b].points_type(POINTS::IARU);

        if (context_points == "STEW"sv)  // special
          pb[b].points_type(POINTS::STEW);
      }
      else                              // ordinary points structure
      {
// remove commas inside the delimiters, because commas separate the point triplets
        context_points = remove_char_from_delimited_substrings(context_points, ',', '[', ']');

        const vector<string> points_str_vec { clean_split_string <string> (context_points) };

        for (const string& points_str : points_str_vec)
        { const vector<string> fields { split_string <std::string> (points_str, ':') };

// default?
          if (fields.size() != 2 and fields.size() != 3)
            ost << "Ignoring points field: " << points_str << endl;
          else
          { if (fields.size() == 3)                              // three fields
            { bool processed { false };

              if (fields[0].empty() and fields[1].empty())
              { points_this_band.default_points(from_string<unsigned int>(fields[2]));
                processed = true;
              }

// country
              if (!processed and !fields[1].empty())
              { if (contains(fields[1], '['))    // possible multiple countries
                { const string countries { delimited_substring <std::string> (fields[1], '[', ']', DELIMITERS::DROP) };  // delimiter is now spaces, as commas have been removed

                  if (!countries.empty())
                  { const vector<string> country_vec { clean_split_string <std::string> (remove_peripheral_spaces <std::string> (squash(countries)), ' ') };  // use space instead of comma because we've already split on commas

                    FOR_ALL(country_vec, [f2 = from_string<unsigned int>(fields[2]), &country_points_this_band, &location_db] (const string& country) 
                             { country_points_this_band += { location_db.canonical_prefix(country), f2 }; } );
                  }
                }
                else
                  country_points_this_band += { location_db.canonical_prefix(fields[1]), from_string<unsigned int>(fields[2]) };

                points_this_band.country_points(country_points_this_band);

                processed = true;
              }

              if (!processed)
              { continent_points_this_band[fields[0]] = from_string<unsigned int>(fields[2]);
                points_this_band.continent_points(continent_points_this_band);
              }
            }
          }

// [field-name-condition]:points // [IOTA != ------]:15 ... not yet supported
// [field-name]:points
          if (fields.size() == 2)
          { const string& f0                     { fields[0] };
            const string  inside_square_brackets { delimited_substring <std::string> (f0, '[', ']', DELIMITERS::DROP) };

            if (!inside_square_brackets.empty())
            { const set<string> all_exchange_field_names { exchange_field_names() };

              if (!all_exchange_field_names.empty())
              { const auto cit { all_exchange_field_names.find(inside_square_brackets) };

                if (cit != all_exchange_field_names.cend())              // if found a valid exchange field name (as opposed to a name with a condition)
                { const int n_points { from_string<int>(fields[1]) };    // the value after the colon

                  _exchange_present_points += { inside_square_brackets, n_points };    // there is just one value for all bands and modes; this will overwrite any previous value
                }
              }
            }
          }

          pb[b] = points_this_band;    // overwrite default
        }
      }  // not IARU
    }
  }

// legal values for the exchange fields
  _parse_context_qthx(context, location_db);  // qth-dependent fields

  vector<exchange_field> leaves_vec;

  for (const MODE m : _permitted_modes)
  { for (const auto& [ cp, vef ] : _received_exchange.at(m))                        // vef is unexpanded
      FOR_ALL(vef, [&leaves_vec] (const auto& ef) { leaves_vec += ef.expand(); });
  }

//  auto print_exch_field_names = [] (const auto& exch_values)
//    { for (auto& ev : exch_values)
//        ost << "exchange value name: " << ev.name() << endl;
//    };

//  print_exch_field_names(_exch_values);

  set<exchange_field> leaves(leaves_vec.cbegin(), leaves_vec.cend());

  for (const auto& ef : leaves)
  { static const set<string> no_canonical_values { "RS"s, "RST"s, "SERNO"s };    // some field values don't have canonical values

    const string& field_name { ef.name() };

 //   print_exch_field_names(_exch_values);

    string entire_file;

    bool read_file_ok { false };

    if (!no_canonical_values.contains(field_name))
    { try
      { entire_file = read_file(path, field_name + ".values"s);
        read_file_ok = true;
      }

      catch (...)
      {
      }
    }

// parse file
    if (read_file_ok)
    { const vector<string> lines { split_string <std::string> (entire_file, EOL_CHAR) };

      map<string /* canonical value */, set<string> > map_canonical_to_all;

      for (const auto& line : lines)
      { string lhs;
        set<string> equivalent_values;    // includes the canonical

        if (!line.empty() and (line[0] != ';') and !line.starts_with("//"s)) // ";" and "//" introduce comments
        { if (contains(line, '=') )
          { const vector<string> lhsrhs { split_string <std::string> (line, '=') };

            lhs = remove_peripheral_spaces <std::string> (lhsrhs[0]);
            equivalent_values += lhs;                  // canonical value

            if (lhsrhs.size() != 1)
            { const string&        rhs                         { lhsrhs[1] };
              const vector<string> remaining_equivalent_values { clean_split_string <string> (rhs) };

              FOR_ALL(remaining_equivalent_values, [&equivalent_values] (const string& rev) { equivalent_values += rev; });

              map_canonical_to_all += { lhs, equivalent_values };
            }
          }
          else    // no "="
          { const string str { remove_peripheral_spaces <std::string> (line) };

            if (!str.empty())
              map_canonical_to_all += { str, set<string>{ str } };
          }
        }
      }

// processed all lines
      _exch_values += { field_name, map_canonical_to_all };         // pair<string, map<string, set<string> > >
    }
    else
    { if (NONE_OF(_exch_values, [&field_name] (const auto& ev) { return (ev.name() == field_name); }))
        _exch_values += { field_name, MAP_STR_TO_SET { } };  // pair<string, map<string, set<string> > >
    }
  }

// generate the sets of all acceptable values
  FOR_ALL(all_known_field_names(), [this] (const string& field_name) { _permitted_exchange_values += { field_name, _all_exchange_values <set<string>> (field_name) }; });

// generate all the mappings from permitted to canonical values
  FOR_ALL(_exch_values, [this] (const exchange_field_values& efv) { _permitted_to_canonical += { efv.name(), INVERT_MAPPING(efv.values()) }; } );
}

/// default constructor
contest_rules::contest_rules(void) :        // can't go in rules.h because the defn of EFT is incomplete at that point
  _callsign_mults_used(false),
  _exchange_mults_used(false),
  _send_qtcs(false),
  _uba_bonus(false)
{ }

/*! \brief              Construct an object ready for use
    \param  context     context for this contest
    \param  location_db location database
*/
contest_rules::contest_rules(const drlog_context& context, location_database& location_db) :
  _callsign_mults_used(false),
  _countries(location_db.countries()),    // default is ALL countries
  _exchange_mults_used(false),
  _uba_bonus(false),
  _work_if_different_band(context.qso_multiple_bands()),
  _work_if_different_mode(context.qso_multiple_modes())
{ _init(context, location_db);
}

/*! \brief                  Prepare for use an object that was created from the default constructor
    \param  context         context for this contest
    \param  location_db     location database
*/
void contest_rules::prepare(const drlog_context& context, location_database& location_db)
{ _work_if_different_band = context.qso_multiple_bands();
  _work_if_different_mode = context.qso_multiple_modes();
  _countries = location_db.countries();

  _init(context, location_db);
}

/*! \brief      Get all the known names of exchange fields (for all modes)
    \return     all the known names of exchange fields (for all modes)
*/
set<string> contest_rules::all_known_field_names(void) const
{ set<string> rv;

  SAFELOCK(rules);

  for (const auto& m : _permitted_modes)
  { for (const auto& [ cp, vef] : _expanded_received_exchange.at(m))
      FOR_ALL(vef, [&rv] (const exchange_field& ef) { rv += ef.name(); } );
  }

  return rv;
}

/*! \brief              The exchange field template corresponding to a particular field
    \param  field_name  name of the field
    \return             the exchange field information associated with <i>field_name</i>

    Returned EFT("none") if <i>field_name</i> is unknown.
*/
EFT contest_rules::exchange_field_eft(const string& field_name) const
{ SAFELOCK(rules);

  return MUM_VALUE(_exchange_field_eft, field_name, EFT("none"s));
}

/*! \brief              All the canonical values for a particular exchange field
    \param  field_name  name of an exchange field (received)

    Returns empty vector if no acceptable values are found (e.g., RST, RS, SERNO)
*/
vector<string> contest_rules::exch_canonical_values(const string& field_name) const
{ vector<string> rv;

  { SAFELOCK(rules);

    for (const auto& ev : _exch_values)
    { if (ev.name() == field_name)
      { for (const auto& [ canonical_value, legal_values ] : ev.values())
          rv += canonical_value;

        SORT(rv);
      }
    }
  }

  return rv;
}

/*! \brief                          Add a canonical value for a particular exchange field
    \param  field_name              name of an exchange field (received)
    \param  new_canonical_value     the canonical value to add

    Also adds <i>new_canonical_value</i> to the legal values for the field <i>field_name</i>. Does nothing
    if <i>new_canonical_value</i> is already a canonical value.
*/
void contest_rules::add_exch_canonical_value(const string& field_name, const string& new_canonical_value)
{ SAFELOCK(rules);

  const auto it { FIND_IF(_exch_values, [&field_name] (const auto& ev) { return ev.name() == field_name; }) };

  if (it != _exch_values.end())
    it -> add_canonical_value(new_canonical_value);
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

/*! \brief                              Is a particular string a canonical value for a particular exchange field?
    \param  field_name                  name of an exchange field (received)
    \param  putative_canonical_value    the value to check

    Returns false if <i>field_name</i> is unrecognized
*/
bool contest_rules::is_canonical_value(const string& field_name, const string& putative_canonical_value) const
{ SAFELOCK(rules);

  const auto it { FIND_IF(_exch_values, [&field_name] (const auto& ev) { return ev.name() == field_name; }) };

  return (it == _exch_values.end()) ? false : it -> is_legal_canonical_value(putative_canonical_value);
}

/*! \brief                  Is a particular string a legal value for a particular exchange field?
    \param  field_name      name of an exchange field (received)
    \param  putative_value  the value to check

    Returns false if <i>field_name</i> is unrecognized. Supports regex exchanges.
*/
bool contest_rules::is_legal_value(const string& field_name, const string& putative_value) const
{ SAFELOCK(rules);

  try
  { const EFT& eft { _exchange_field_eft.at(field_name) };

    return eft.is_legal_value(putative_value);
  }

  catch (...)
  { ost << "ERROR: unknown field name in contest_rules::is_legal_value( " << field_name << ", " << putative_value << ")" << endl;

    return false;
  }
}

/*! \brief              Is a particular exchange field a regex?
    \param  field_name  name of an exchange field (received)
    \return             whether field <i>field_name</i> is a regex field

    Returns <i>false</i> if <i>field_name</i> is unknown.
*/
bool contest_rules::exchange_field_is_regex(const string& field_name) const
{ SAFELOCK(rules);

  try
  { const EFT& eft { _exchange_field_eft.at(field_name) };

    return eft.regex_str().empty();
  }

  catch (...)
  { ost << "ERROR: unknown field name in contest_rules::exchange_field_is_regex( " << field_name << ")" << endl;

    return false;
  }
}

/*! \brief              The permitted values for a field
    \param  field_name  name of an exchange field (received)

    Returns the empty set if the field <i>field_name</i> can take any value, or if it's a regex.
*/
set<string> contest_rules::exch_permitted_values(const string& field_name) const
{ SAFELOCK(rules);

  return MUM_VALUE(_permitted_exchange_values, field_name, set<string> { });
}

/*! \brief                  The canonical value for a field
    \param  field_name      name of an exchange field (received)
    \param  actual_value    actual received value of the field <i>field_name</i>
    \return                 Canonical value for the value <i>actual_value</i> for the field <i>field_name</i>

    Returns <i>actual_value</i> if there are no canonical values or if <i>actual_value</i> is empty
    Returns the empty string if <i>actual_value</i> is not a legal value for <i>field_name</i>.
*/
string contest_rules::canonical_value(const string& field_name, const string& actual_value) const
{ if (actual_value.empty())
    return actual_value;

  if (field_name == "DOK"sv)    // DOK is special because loads of actual_values map to the same value; keep the actual value
    return actual_value;       // we convert to the single letter version elsewhere

  if ((field_name == "IOTA"sv) and (actual_value.length() > 2))   // IOTA is special because there are so many possible received values, many of which are not canonical
    return (substring <std::string> (actual_value, 0, 2) + pad_leftz(substring <std::string_view> (actual_value, 2), 3));  // XXnnn

  const set<string> permitted_values { exch_permitted_values(field_name) };

  if (permitted_values.empty())                         // if no permitted values => anything allowed
    return actual_value;

  if ( !(permitted_values.contains(actual_value)) )               // is the actual value a permitted value for this field?
    return string { };
/*
  std::map
    <std::string,                                          // exch field name
      std::map
        <std::string,                                      // permitted value
          std::string                                      // canonical value
           > >                                  _permitted_to_canonical;
*/
  SAFELOCK(rules);
  
  return MUM_VALUE(_permitted_to_canonical.at(field_name), actual_value, actual_value);  // we don't get here if field_name isn't valid
}

/*! \brief                  Get the next mode in sequence
    \param  current_mode    the current mode
    \return                 the mode after <i>current_mode</i>

    Cycles through the available modes.
    Currently supports only MODE_CW and MODE_SSB
*/
MODE contest_rules::next_mode(const MODE current_mode) const
{ SAFELOCK(rules);

  auto cit { _permitted_modes.find(current_mode) };

  if (cit == _permitted_modes.cend())    // should never happen
    return *(_permitted_modes.cbegin());

  return (++cit == _permitted_modes.cend() ? *(_permitted_modes.cbegin()) : *cit);
}

/*! \brief      Add a band to those permitted in the contest
    \param  b   band to add

    Does nothing if <i>b</i> is already permitted
*/
void contest_rules::add_permitted_band(const BAND b)
{ SAFELOCK(rules);

  _permitted_bands += b;
}

/// get the next band up
BAND contest_rules::next_band_up(const BAND current_band) const
{ SAFELOCK(rules);

  auto cit { find(_permitted_bands.begin(), _permitted_bands.end(), current_band) };

  if (cit == _permitted_bands.cend())    // might happen if rig has been manually QSYed to a non-contest band
  { int band_nr { static_cast<int>(current_band) };

// find first permitted band higher than the current band
    const set<BAND> pbs { permitted_bands_set() };

    for (int counter { static_cast<int>(MIN_BAND) }; counter <= static_cast<int>(MAX_BAND); ++counter)    // the counter is not actually used
    { if (++band_nr > MAX_BAND)
        band_nr = MIN_BAND;

      if (const bool is_permitted { pbs.contains(static_cast<BAND>(band_nr)) }; is_permitted)
        return (static_cast<BAND>(band_nr));
    }

// should never get here
    return *(_permitted_bands.cbegin());
  }

  return (++cit == _permitted_bands.cend() ? *(_permitted_bands.cbegin()) : *cit);
}

/// get the next band down
BAND contest_rules::next_band_down(const BAND current_band) const
{ SAFELOCK(rules);

  auto cit { find(_permitted_bands.begin(), _permitted_bands.end(), current_band) };

  if (cit == _permitted_bands.end())    // might happen if rig has been manually QSYed to a non-contest band
  { int band_nr { static_cast<int>(current_band) };

// find first permitted band higher than the current one
    const set<BAND> pbs { permitted_bands_set()};

    for (int counter { static_cast<int>(MIN_BAND) }; counter <= static_cast<int>(MAX_BAND); ++counter)    // the counter is not actually used
    { if (--band_nr < MIN_BAND)
        band_nr = MAX_BAND;

      if (const bool is_permitted { pbs.contains(static_cast<BAND>(band_nr)) }; is_permitted)
        return (static_cast<BAND>(band_nr));
    }

// should never get here
    return *(prev(_permitted_bands.cend()));
  }

  return ( (cit == _permitted_bands.begin()) ? _permitted_bands[_permitted_bands.size() - 1] : *(--cit));
}

/*! \brief                  Points for a particular QSO
    \param  qso             QSO for which the points are to be calculated
    \param  location_db     location database
    \return                 the (location-based) points for <i>qso</i>
*/
unsigned int contest_rules::points(const QSO& qso, location_database& location_db) const
{ const BAND b { qso.band() };

  if (!_score_bands.contains(b))    // no points if we're not scoring this band
    return 0;

  const MODE m { qso.mode() };

  if (!_score_modes.contains(m))    // no points if we're not scoring this mode
    return 0;

// is an exchange field that will determine the number of points present?
  for (const auto& [ exchange_field_name, points ] : _exchange_present_points)
  { if (qso.is_exchange_field_present(exchange_field_name))
      return points;
  }

  const string call             { qso.callsign() };
  const string canonical_prefix { location_db.canonical_prefix(call) };

  if (canonical_prefix == "NONE"sv)      // unable to determine country
    return 0;

  const auto& pb { _points[m] };          // select the correct mode

  SAFELOCK(rules);

  if (!pb.contains(b))
  { ost << "Unable to find any points entries for band " << BAND_NAME[b] << endl;
    return 0;
  }

  switch (const points_structure& points_this_band { pb.at(b) }; points_this_band.points_type())
  { default :
    case POINTS::NORMAL :
    { const map<string, unsigned int>& country_points { points_this_band.country_points() };

      if (auto cit { country_points.find(canonical_prefix) }; cit != country_points.cend())    // if points are defined for this country
        return cit->second;

      const map<string, unsigned int>& continent_points { points_this_band.continent_points() };

      return MUM_VALUE(continent_points, location_db.continent(call), points_this_band.default_points());
    }

// IARU
//1 point per QSO with same zone or with HQ stations
//3 points per QSO with different zone on same continent
//5 points per QSO with different zone on different continent
    case POINTS::IARU :
    { unsigned int rv { 0 };

      const bool   same_zone     { (_my_itu_zone == location_db.itu_zone(call)) };
      const string his_continent { location_db.continent(call) };

      if (same_zone)
        rv = 1;

      if ( (!same_zone) and (_my_continent == his_continent) )
        rv = 3;

      if ( (!same_zone) and (_my_continent != his_continent) )
        rv = 5;

// is it an HQ station?
      const string society_value { qso.received_exchange("SOCIETY"s) };

      if (!society_value.empty())
        rv = 1;

      return rv;
    }

    case POINTS::STEW :
    { constexpr int STEW_PERRY_BONUS_DISTANCE { 500 };  // distance for bonus point(s) in Stew Perry, in km

      const string       grid_value { qso.received_exchange("GRID"s) };
      const unsigned int distance   { static_cast<unsigned int>( (grid_square(grid_value) - _my_grid) / STEW_PERRY_BONUS_DISTANCE ) + 1 };

      return distance;
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

/*! \brief      Do the country mults (if any) include a particular country?
    \param  cp  canonical prefix of country to test
    \return     whether cp is a country mult

    If <i>cp</i> is empty, then tests whether any countries are mults.
*/
bool contest_rules::country_mults_used(const string& cp) const
{ SAFELOCK(rules);

  if (cp.empty())
    return !_country_mults.empty();

  if (_country_mults.empty())
    return false;

  return _country_mults.contains(cp);
}

/*! \brief          Is an exchange field a mult?
    \param  name    name of exchange field
    \return         whether <i>name</i> is an exchange mult

    Returns <i>false</i> if <i>name</i> is unrecognised
*/
bool contest_rules::is_exchange_mult(const string& name) const
{ SAFELOCK(rules);

  return contains(_exchange_mults, name);
}

/*! \brief          Does the sent exchange include a particular field?
    \param  str     name of field to test
    \param  m       mode
    \return         whether the sent exchange for mode <i>m</i> includes a field with the name <i>str</i>
*/
bool contest_rules::sent_exchange_includes(const std::string& str, const MODE m) const
{ SAFELOCK(rules);

  try
  { //const auto& se { _sent_exchange_names.at(m) };

    return contains(_sent_exchange_names.at(m), str);
  }

  catch (...)
  { ost << "ERROR: mode " << m << " is not a key in contest_rules::_sent_exchange_names" << endl;
    return false;
  }
}

/*! \brief                      Is a particular field used for QSOs with a particular country?
    \param  field_name          name of exchange field to test
    \param  canonical_prefix    country to test
    \return                     whether the field <i>field_name</i> is used when the country's canonical prefix is <i>canonical_prefix</i>
*/
bool contest_rules::is_exchange_field_used_for_country(const string& field_name, const string& canonical_prefix) const
{ SAFELOCK(rules);

  if (!_exchange_field_eft.contains(field_name))
    return false;    // not a known field name

// if a field appears in the per-country rules, are we in the right country?
  bool is_a_per_country_field { false };

  for (const auto& [ cp, exchange_field_names_set ] : _per_country_exchange_fields)
  { if (exchange_field_names_set.contains(field_name))
      is_a_per_country_field = true;

    if (cp == canonical_prefix)
      return (exchange_field_names_set.contains(field_name));
  }

  return !is_a_per_country_field;       // a known field, and this is not a special country
}

/// the names of all the possible exchange fields
set<string> contest_rules::exchange_field_names(void) const
{ set<string> rv;

  for (const auto& [ field_name, unused_eft ] : _exchange_field_eft)      //std::map<std::string /* field name */, EFT>   _exchange_field_eft;
    rv += field_name;

  return rv;
}

/*! \brief      The equivalent choices of exchange fields for a given mode and country
    \param  m   mode
    \param  cp  canonical prefix of country
    \return     all the equivalent fields for mode <i>m</i> and country <i>cp</i>
*/
choice_equivalents contest_rules::equivalents(const MODE m, const string& cp) const
{ choice_equivalents rv;

  map<MODE, map<string, choice_equivalents>>::const_iterator cit_mode { _choice_exchange_equivalents.find(m) };

  if (cit_mode == _choice_exchange_equivalents.cend())     // no choice equivalents for this mode
    return rv;

  const map<string, choice_equivalents>& choice_equivalents_this_mode { cit_mode->second };                         // there are choice equivalents for this mode

  map<string, choice_equivalents>::const_iterator cit_cp { choice_equivalents_this_mode.find(cp) };

  if (cit_cp != choice_equivalents_this_mode.cend())
    return (cit_cp->second);

// if there's no choice for this particular cp, perhaps there's a choice for all cps
  return MUM_VALUE(choice_equivalents_this_mode, string { }, choice_equivalents { });   // I think that this is the same as the prior four lines
}

// The intention here is to follow the WPX definition. It would be
// a lot more helpful if they actually supplied a strict algorithm rather than
// an ambiguous definition. What follows attempts to implement my guess as to what
// they mean.

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
string wpx_prefix(const string& call)
{
// callsign has to contain three characters
  if (call.length() < 3)
    return string { };

  string callsign          { call };
  char   portable_district { 0 } ;   // portable call district

// make sure we deal with AA1AA/M/QRP

// /QRP -- deal with this first
  callsign = remove_from_end <std::string> (callsign, "/QRP"sv);

// remove portable designators
  if ((callsign.length() >= 2) and (penultimate_char(callsign) == '/'))
  { static const string portables { "AEJMP"sv };                 // concluding characters that might mean "portable"

    if (portables.find(last_char(callsign)) != string::npos)
      callsign = remove_from_end <std::string> (callsign, 2u);
    else
      if (callsign.find_last_of(DIGITS) == callsign.length() - 1)
      { portable_district = callsign[callsign.length() - 1];
        callsign = remove_from_end <std::string> (callsign, 2u);
      }
  }

// /MM, /MA, /AM
  if ((callsign.length() >= 3) and (antepenultimate_char(callsign) == '/'))
  { static const set<string> mobiles {"AM"s, "MA"s, "MM"s};

    if (mobiles.contains(last <std::string> (callsign, 2)))
      callsign = remove_from_end <std::string> (callsign, 3u);
  }

// trivial -- and almost unknown -- case first: no digits
  if (!contains_digit(callsign))
    return (substring <std::string> (callsign, 0, 2) + '0');

  size_t slash_posn { callsign.find('/') };

  if ( (slash_posn == string::npos) or (slash_posn == callsign.length() - 1) )
  { const size_t last_digit_posn { callsign.find_last_of(DIGITS) };

    if (portable_district)
      callsign[last_digit_posn] = portable_district;

    return substring <std::string> (callsign, 0, min(callsign.length(), last_digit_posn + 1));
  }

// we have a (meaningful) slash in the call
  const string left       { substring <std::string> (callsign, 0, slash_posn) };
  const size_t left_size  { left.size() };
  const string right      { substring <std::string> (callsign, slash_posn + 1) };
  const size_t right_size { right.size() };

  if (left_size == right_size)
  { const string left_canonical_prefix  { location_db.canonical_prefix(left) };
    const string right_canonical_prefix { location_db.canonical_prefix(right) };

    if (left_canonical_prefix == left)
      return left;

    if (right_canonical_prefix == right)
      return right;
  }

  string designator { (left_size < right_size) ? left : right };

  if (!contains_digit(designator))
    designator += '0';

  string rv { designator };

  if (rv.length() == 1)
  { if (rv[0] == call[0])  // if just the first character, add the next character (to deal with 7QAA)
      rv = substring <std::string> (call, 0, 2);
  }

  return rv;
}

/*! \brief          The SAC prefix for a particular call
    \param  call    call for which the prefix is to be calculated
    \return         the SAC prefix corresponding to <i>call</i>

    The SAC rules as written do not allow for weird prefixes such as LA100, etc.

From SAC rules, the relevant countries are:
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
string sac_prefix(const string& call)
{ static const unordered_set<string> scandinavian_countries { "JW"s, "JX"s, "LA"s, "OH"s, "OH0"s, "OJ0"s, "OX"s, "OY"s, "OZ"s, "SM"s, "TF"s };

  const string canonical_prefix { location_db.canonical_prefix(call) };

  if ( !(scandinavian_countries.contains(canonical_prefix)) )
    return string { };

// it is a scandinavian call
  const string wpx { wpx_prefix(call) };

// working from the end, find the first non-digit
  const size_t last_letter_posn { wpx.find_last_not_of(DIGITS) };
  const string digits           { substring <std::string> (wpx, last_letter_posn + 1) };

  if (digits.empty())
    return string { };    // to handle case of something like "SM" as the passed call, which happens as a call is being typed

  return ( ( (canonical_prefix != "OH0"sv) and (canonical_prefix != "OJ0"sv) ) ? (canonical_prefix + digits[0]) : canonical_prefix );   // use first digit
}

/*! \brief                  Given a received value of a particular multiplier field, what is the actual mult value?
    \param  field_name      name of the field
    \param  received_value  received value for field <i>field_name</i>
    \return                 the multiplier value for the field <i>field_name</i>

    For example, the mult value for a DOK field with the value A01 is A.
    Currently, the only field name that precipitates special processing is DOK.
    Adding IOTA.
*/
string MULT_VALUE(const string_view field_name, const string& received_value)
{ if (field_name == "DOK"sv)
  { if (!received_value.empty())
    { const auto posn { received_value.find_first_of(UPPER_CASE_LETTERS) };

      return ( (posn == string::npos) ? string { } : create_string(received_value[posn]) );
    }
    else        // should never happen: DOK with no value; might be empty if no value to guess
      return received_value;  // same as string()
  }

  if ( (field_name == "IOTA"sv) and (received_value.size() > 2) )
    return (substring <std::string> (received_value, 0, 2) + pad_leftz(substring <std::string> (received_value, 2), 3));  // XXnnn

  return received_value;
}
