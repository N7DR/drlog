// $Id: statistics.cpp 89 2015-01-03 13:59:15Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file statistics.cpp

        Classes and functions related to the statistics of an ongoing contest
*/

#include "bands-modes.h"
#include "log_message.h"
#include "rules.h"
#include "statistics.h"
#include "string_functions.h"

#include <fstream>
#include <iostream>

using namespace std;

extern message_stream ost;      ///< for debugging and logging

// there is just one running_statistics object, and it needs to be thread-safe
pt_mutex statistics_mutex;      ///< mutex for the running_statistics object

// -----------  running_statistics  ----------------

/*! \brief  add a callsign mult name, value and band to those worked
    \param  mult_name   name of callsign mult
    \param  mult_value  value of callsign mult
    \param  band_nr     band on which callsign mult was worked

    <i>band_nr</i> = ALL_BANDS means add to *only* the global accumulator; otherwise add to a band AND to the global accumulator
    The information is inserted into the <i>_callsign_multipliers</i> object.
*/
void running_statistics::_insert_callsign_mult(const string& mult_name, const string& mult_value, const unsigned int band_nr, const unsigned int mode_nr)
{ if (_callsign_mults_used and !mult_value.empty())     // do we actually have to do anything?
  { SAFELOCK(statistics);

    if (known_callsign_mult_name(mult_name))            // do we already know about this mult name?
    { //auto it = _callsign_multipliers.find(mult_name);
      //multiplier& mult = it->second;
      multiplier& mult = (_callsign_multipliers.find(mult_name))->second;

      mult.add_worked(mult_value, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr));                // add value and band for this mult name
    }
    else                                                // unknown mult name
    { multiplier mult;                                     // create new mult

      mult.add_worked(mult_value, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr));                // we've worked it
      _callsign_multipliers.insert( { mult_name, mult } ); // store the info
    }
  }
}

/// default constructor
running_statistics::running_statistics(void) :
  _n_qsos( { {} } ),              // Josuttis 2nd ed., p.262 -- initializes all elements with zero
  _n_dupes( { {} } ),
  _qso_points( { {} } ),
  _callsign_mults_used(false),
  _country_mults_used(false),
  _exchange_mults_used(false),
  _qtc_qsos_sent(0),
  _qtc_qsos_unsent(0),
  _include_qtcs(false)
{
}

/*! \brief  Constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
running_statistics::running_statistics(const cty_data& country_data, const drlog_context& context, const contest_rules& rules) :
  _n_qsos( { {} } ),              // Josuttis 2nd ed., p.262 -- initializes all elements with zero
  _n_dupes( { {} } ),
  _qso_points( { {} } ),
  _location_db(country_data, context.country_list()),
  _callsign_mults_used(rules.callsign_mults_used()),
  _country_mults_used(rules.country_mults_used()),
  _exchange_mults_used(rules.exchange_mults_used()),
  _qtc_qsos_sent(0),
  _qtc_qsos_unsent(0),
  _include_qtcs(rules.send_qtcs())
{ const vector<string>& exchange_mults = rules.exchange_mults();

  FOR_ALL(exchange_mults, [&] (const string& exchange_mult) { _exch_mult_fields.insert(exchange_mult); } );
}

/*! \brief  Prepare an object that was created with the default constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
void running_statistics::prepare(const cty_data& country_data, const drlog_context& context, const contest_rules& rules)
{ SAFELOCK(statistics);

  _include_qtcs = rules.send_qtcs();
  _callsign_mults_used = rules.callsign_mults_used();
  _country_mults_used  = rules.country_mults_used();
  _exchange_mults_used = rules.exchange_mults_used();

  _location_db.prepare(country_data, context.country_list());

  const vector<string>& exchange_mults = rules.exchange_mults();

  FOR_ALL(exchange_mults, [&] (const string& exchange_mult) { _exch_mult_fields.insert(exchange_mult); /* ost << "added _exch_mult_field: " << exchange_mult << endl; */} );

// callsign mults
  if (_callsign_mults_used)
  { const set<string> callsign_mult_names = rules.callsign_mults();

    if (!callsign_mult_names.empty())                              // should always be true
    { for (const auto& callsign_mult_name : callsign_mult_names)
      { multiplier em;

        em.used(true);
        em.per_band(rules.callsign_mults_per_band());
        em.per_mode(rules.callsign_mults_per_mode());

        _callsign_multipliers[callsign_mult_name] = em;
      }
    }
  }

// country mults
  if (_country_mults_used)
  { const set<string> country_mults = rules.country_mults();

    _country_multipliers.used(!country_mults.empty() or context.auto_remaining_country_mults());  // should always be true
    _country_multipliers.per_band(rules.country_mults_per_band());
    _country_multipliers.per_mode(rules.country_mults_per_mode());

    if (!context.auto_remaining_country_mults())
      _country_multipliers.add_known(country_mults);
  }

// exchange mults
  if (_exchange_mults_used)
  { for (const auto& exchange_mult_name : exchange_mults)
    { multiplier em;

      em.used(true);
      em.per_band(rules.exchange_mults_per_band());
      em.per_mode(rules.exchange_mults_per_mode());

// if values are obtained from grep, then the next line returns an empty vector
      const vector<string> canonical_values = rules.exch_canonical_values(exchange_mult_name);

      if (!canonical_values.empty())
        em.add_known(canonical_values);

//      ost << "statistics: adding exchange multiplier: " << exchange_mult_name << endl;

      _exchange_multipliers.push_back( { exchange_mult_name, em } );
    }
  }
}

/*! \brief                                  is a particular string a known callsign mult name?
    \param  putative_callsign_mult_name     string to test
    \return                                 whether <i>putative_callsign_mult_name</i> is a known callsign mult name
*/
const bool running_statistics::known_callsign_mult_name(const string& putative_callsign_mult_name) const
{ SAFELOCK(statistics);

  return ( _callsign_multipliers.find(putative_callsign_mult_name) != _callsign_multipliers.cend() );
}

/*! \brief              do we still need to work a particular callsign mult on a particular band (and any mode)?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
    \return             whether the mult <i>mult_name</i> with the value <i>mult_value</i> is needed on band <i>b</i>
*/
#if 0
const bool running_statistics::is_needed_callsign_mult(const string& mult_name, const string& mult_value, const BAND b) const
{ SAFELOCK(statistics);

  if (!known_callsign_mult_name(mult_name))
  { ost << "in running_statistics::is_needed_callsign_mult(), unknown callsign mult name = " << mult_name << endl;
    return false;
  }

  const auto cit = _callsign_multipliers.find(mult_name);
  const multiplier& mult = cit->second;
  const bool worked = mult.is_worked(mult_value, b);

  return !(worked);
}
#endif

/*! \brief              Do we still need to work a particular callsign mult on a particular band and mode?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
    \param  m           mode to test
*/
const bool running_statistics::is_needed_callsign_mult(const string& mult_name, const string& mult_value, const BAND b, const MODE m) const
{ SAFELOCK(statistics);

  if (!known_callsign_mult_name(mult_name))
  { ost << "in running_statistics::is_needed_callsign_mult(), unknown callsign mult name = " << mult_name << endl;
    return false;
  }

  const auto cit = _callsign_multipliers.find(mult_name);
  const multiplier& mult = cit->second;
  const bool worked = mult.is_worked(mult_value, b, m);

  return !(worked);
}

/*! \brief  do we still need to work a particular country as a mult on a particular band and mode?
    \param  callsign    call to test
    \param  b           band to test
*/
const bool running_statistics::is_needed_country_mult(const string& callsign, const BAND b, const MODE m)
{ try
  { SAFELOCK(statistics);

    const string canonical_prefix = _location_db.canonical_prefix(callsign);
    const bool is_needed = ( (_country_multipliers.is_known(canonical_prefix)) and !(_country_multipliers.is_worked(canonical_prefix, b, m)) );

    return is_needed;
  }
  
  catch (const location_error& e)
  { return false;
  }
}

/*! \brief          Add a known value of country mult
    \param  str     Canonical prefix of mult
    \return         Whether <i>str</i> was actually added

    Does nothing and returns false if <i>str</i> is already known
*/
const bool running_statistics::add_known_country_mult(const string& str)
{ SAFELOCK(statistics);

  return (_country_multipliers.add_known(str));
}

/*! \brief  On what bands is a country mult needed?
    \param  call    call to test
    \param  rules   Rules for the contest
    \return         Space-separated (actually, multiple spaces) string of band names on which
                    the country corresponding to <i>call</i> is needed.
*/
const string running_statistics::country_mult_needed(const string& call, const contest_rules& rules)
{ string rv;
  const vector<BAND> permitted_bands = rules.permitted_bands();
  
  for (const auto& b : permitted_bands)
    rv += ((is_needed_country_mult(call, b)) ?  BAND_NAME[b] : "   ");
  
  return rv;
}

/*! \brief  Add a QSO to the ongoing statistics
    \param  qso     QSO to add
    \param  log     Logbook (without the qso <i>qso</i>)
    \param  rules   Contest rules
*/
void running_statistics::add_qso(const QSO& qso, const logbook& log, const contest_rules& rules)
{ SAFELOCK(statistics);

  ost << "inside running_statistics::add_qso()" << endl;
  
  const BAND& b = qso.band();
  const unsigned int band_nr = static_cast<int>(b);
  const MODE& mo = qso.mode();
  const unsigned int mode_nr = static_cast<int>(mo);

// increment the number of QSOs
  auto& pb = _n_qsos[mode_nr];
  pb[band_nr]++;
  
  ost << "in add_qso(); pb[band_nr] = " << pb[band_nr] << endl;

// multipliers

// callsign mults
// for now, just assume that there's at most one possible callsign mult, and the value is in qso.prefix()

  if (!_callsign_multipliers.empty() and !(qso.prefix().empty()) )
  { const string mult_name = _callsign_multipliers.begin()->first;
    multiplier& m = _callsign_multipliers.begin()->second;

//    ost << "inside running_statistics::add_qso(), processing callsign mult " << mult_name << endl;
//    ost << "callsign = " << qso.callsign() << "; prefix = " << qso.prefix() << endl;
//    ost << "initial m = " << m << endl;

//    m.unconditional_add_worked(qso.prefix(), band_nr);
    m.unconditional_add_worked(qso.prefix(), static_cast<BAND>(band_nr), static_cast<MODE>(mo));

//    ost << "middle m = " << m << endl;
    _callsign_multipliers[mult_name] = m;
//    ost << "final m = " << m << endl;
  }

// country mults
  const string& call = qso.callsign();
  const string& canonical_prefix = _location_db.canonical_prefix(call);

//  ost << "adding worked country mult: " << canonical_prefix << " on band " << BAND_NAME[band_nr] << " and mode " << MODE_NAME[mo] << endl;
//  _country_multipliers.add_worked(canonical_prefix, band_nr);
  /* bool added = */ _country_multipliers.add_worked(canonical_prefix, static_cast<BAND>(band_nr), static_cast<MODE>(mo));

//  ost << "was added? = " << added << endl;

// exchange mults
//  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  // vector so we can keep the correct order
  for (auto& exchange_multiplier : _exchange_multipliers)
  { const string& field_name = exchange_multiplier.first;
    multiplier& mult = exchange_multiplier.second;
    const string value = qso.received_exchange(field_name);
    const string mv = MULT_VALUE(field_name, value);  // the mult value of the received field

//    ost << "Inside running_statistics::add_qso()" << endl;
//    ost << "QSO: " << qso << endl;
//    ost << "field_name = " << field_name << endl;
//    ost << "value: " << value << endl;
//    ost << "mult value: " << mv << endl;

    if (!value.empty())
    { //m.unconditional_add_worked(mv, band_nr);
      mult.unconditional_add_worked(mv, static_cast<BAND>(band_nr), static_cast<MODE>(mo));
    }

//    ost << "exchange multiplier object: " << m << endl;
  }
  
  const bool is_dupe = log.is_dupe(qso, rules);

  if (is_dupe)
  { //_n_dupes[band_nr]++;

    auto& pb = _n_dupes[mode_nr];
    pb[band_nr]++;
  }
  else    // not a dupe; add qso points; this may not be a very clean algorithm; I should be able to do better
  {
// ost << "Trying to calculate QSO points for statistics" << endl;
// try to calculate the points for this QSO; start with a default value
//    unsigned int points_this_qso = rules.points(qso.band(), qso.call(), _location_db);             // points based on country; something like ::3
    unsigned int points_this_qso = rules.points(qso, _location_db);             // points based on country; something like ::3

//    ost << "initial points this qso with " << qso.callsign() << " = " << points_this_qso << endl;

    const map<string, unsigned int>& exchange_points = rules.exchange_value_points();

    for (map<string, unsigned int>::const_iterator cit = exchange_points.begin(); cit != exchange_points.end(); ++cit)
    { const string condition = cit->first;
      const unsigned int points = cit->second;

// [IOTA != ------]:15 (15 points if the IOTA field is present and doesn't have the value "------")
      if (qso.exchange_match(condition))
      { points_this_qso = points;
        ost << "matched condition: " << condition << " ..... points = " << points << endl;
      }
    }

//    ost << "final points this qso with " << qso.callsign() << " = " << points_this_qso << endl;

//    _qso_points[band_nr] += points_this_qso;
    _qso_points[mode_nr][band_nr] += points_this_qso;
  }
}

/*! \brief          Add a known legal value for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \param  value   New legal value for the exchange multiplier <i>name</i>
    \return         Whether <i>value</i> was actually added
*/
const bool running_statistics::add_known_exchange_mult(const string& name, const string& value)
{ SAFELOCK(statistics);
//  ost << "in add_known_exchange_mult; name = " << name << ", value = " << value << endl;
//  ost << "size of _exchange_multipliers = " << _exchange_multipliers.size() << endl;

//  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  for (auto& psm : _exchange_multipliers)
  { //pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

    if (psm.first == name)
    { const bool added = psm.second.add_known(MULT_VALUE(name, value));

      if (added)
      { ost << "added known exchange mult: " << name << ", value = " << value << ", mult value = " << MULT_VALUE(name, value) << endl;
        return true;
      }
    }
  }

  return false;
}

/*! \brief          Return all known legal value for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \return         All the known legal values of <i>name</i>
*/
const set<string> running_statistics::known_exchange_mults(const string& name)
{ SAFELOCK(statistics);

//  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  for (const auto& psm : _exchange_multipliers)
  { //pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

    if (psm.first == name)
      return psm.second.known();
  }

  return set<string>();
}

/*! \brief  Add a worked exchange mult
    \param  field_name    Exchange mult field name
    \param  field_value   Value of the field <i>field_name</i>
    \param  band_nr       Number of the band on which worked mult is to be added

    Doesn't add if the value is unknown.
*/
void running_statistics::add_worked_exchange_mult(const string& field_name, const string& field_value, const int b, const int m)
{ if (field_value.empty())
    return;

  const string mv = MULT_VALUE(field_name, field_value);  // the mult value of the received field

  SAFELOCK(statistics);

  for (auto& psm : _exchange_multipliers)
    if (psm.first == field_name)
      psm.second.add_worked(mv, static_cast<BAND>(b), static_cast<MODE>(m));
}

/*! \brief          Perform a complete rebuild
    \param  log     logbook
    \param  rules   contest rules
*/
void running_statistics::rebuild(const logbook& log, const contest_rules& rules)
{ logbook l;

// done this way so as to account for dupes correctly
  for (const auto& qso : log.as_vector())
  { this->add_qso(qso, l, rules);
    l += qso;
  }
}

/// do we still need to work a particular exchange mult on a particular band and mode?
const bool running_statistics::is_needed_exchange_mult(const string& exchange_field_name, const string& exchange_field_value, const BAND b, const MODE m) const
{ const string mv = MULT_VALUE(exchange_field_name, exchange_field_value);  // the mult value of the received field

  SAFELOCK(statistics);

//  ost << "in is_needed_exchange_mult for field " << exchange_field_name << ", value = " << exchange_field_value << ", and band = " << b << ", mult value = " << mv << endl;

  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  { const pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

//    ost << "checking exchange field name = " << sm.first << endl;

    if (sm.first == exchange_field_name and sm.second.is_known(mv) /* sm.second.is_known(exchange_field_value) */)
    { //ost << "found field name " << sm.first << "; is needed: " << boolalpha << !(sm.second.is_worked(mv, b)) << noboolalpha << endl;;
//      return !(sm.second.is_worked(exchange_field_value, b));
      return !(sm.second.is_worked(mv, b, m));
    }
  }

  return false;
}

/// a string list of bands on which a particular exchange mult value is needed
//const std::string running_statistics::exchange_mult_needed(const string& exchange_field_name, const string& exchange_field_value, const contest_rules& rules)
//{ string rv;
//
//  for (const auto& b : rules.permitted_bands())
//    rv += ((is_needed_exchange_mult(exchange_field_name, exchange_field_value, b)) ?  BAND_NAME[static_cast<int>(b)] : "   ");
//
//  return rv;
//}

const string running_statistics::_summary_string(const contest_rules& rules, const set<MODE>& modes)
{ string rv;

  const unsigned int FIRST_FIELD_WIDTH = 10;
  const unsigned int FIELD_WIDTH       = 6;          // width of other fields
  const set<MODE> permitted_modes = rules.permitted_modes();
  const vector<BAND> permitted_bands = rules.permitted_bands();

  unsigned int qsos = 0;
  unsigned int countries = 0;
  unsigned int dupes = 0;
  unsigned int points = 0;

  string line;

  auto add_all_bands = [&line] (const unsigned int sz, const unsigned int n) { if (sz != 1)
                                                                                 line += pad_string(to_string(n), FIELD_WIDTH);
                                                                             };

//  for (const auto& m : modes)
  { SAFELOCK(statistics);

// QSOs
    line = pad_string("QSOs", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');                                          // accumulator for total qsos

    for (const auto& m : modes)
    { const auto& nq = _n_qsos[m];

//    for (unsigned int n = 0; n < permitted_bands.size(); ++n)
      for (const auto& b : permitted_bands)
      { //const int band_nr = static_cast<int>(permitted_bands[n]);

//        line += pad_string(to_string(nq[b]), FIELD_WIDTH);
        qsos += nq[b];

        ost << "adding " << nq[b] <<"; total = " << qsos << endl;
      }
    }

    line += pad_string(to_string(qsos), FIELD_WIDTH);

//    if (permitted_bands.size() != 1)
//       line += pad_string(to_string(qsos), FIELD_WIDTH);
    add_all_bands(permitted_bands.size(), qsos);

    rv += line + LF;

// country mults
    if (_country_multipliers.used())                                         // if countries are mults
    { line = pad_string("Countries", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

//      for (unsigned int n = 0; n < permitted_bands.size(); ++n)
      for (const auto& m : modes)
      { for (const auto& b : permitted_bands)
        { //const int band_nr = static_cast<int>(permitted_bands[n]);
          const unsigned int n_countries = _country_multipliers.n_worked(b, m);

          ost << "band " << BAND_NAME[b] << "; n_countries = " << n_countries << endl;

//          line += pad_string(to_string(n_countries), FIELD_WIDTH);
          countries += n_countries;
        }
      }

      line += pad_string(to_string(countries), FIELD_WIDTH);

        if (permitted_bands.size() != 1)
          line += pad_string(to_string(countries), FIELD_WIDTH);

      rv += line + LF;
    }

// callsign mults
    const set<string>&  callsign_mults = rules.callsign_mults();      // collection of types of mults based on callsign (e.g., "WPXPX")
    const bool callsign_mults_per_band = rules.callsign_mults_per_band();

//  ost << "about to test for callsign mults in summary_string()" << endl;

    if (!callsign_mults.empty())
    { for (const auto mult_name : callsign_mults)
      { //ost << "mult name = " << mult_name << endl;

        line = pad_string(mult_name, FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

        const auto& cit = _callsign_multipliers.find(mult_name);

        if (cit != _callsign_multipliers.end())    // should always be true
        { //ost << "found callsign multiplier: " << cit->first << endl;

          const multiplier& mult = cit->second;
          unsigned int total = 0;

          for (const auto& m : modes)
//          for (unsigned int n1 = 0; n1 < permitted_bands.size(); ++n1)
          { for (const auto& b : permitted_bands)
            { //const int band_nr = static_cast<int>(permitted_bands[n1]);
              const unsigned int n_callsign_mults = mult.n_worked(b, m);

//              line += pad_string(to_string(n_callsign_mults), FIELD_WIDTH);

              if (callsign_mults_per_band)
                total += n_callsign_mults;
            }


//          line += pad_string(to_string(n_callsign_mults), FIELD_WIDTH);

          if (!callsign_mults_per_band)
            total = mult.n_worked(ANY_BAND, m);
          }

          line += pad_string(to_string(total), FIELD_WIDTH);

//        ost << "line is: " << line << endl;
        }
        else
        { ost << "Error: did not find mult name: " << mult_name << endl;
          ost << "Number of callsign multipliers in statistics = " << _callsign_multipliers.size() << endl;

          for (const auto& mult : _callsign_multipliers)
            ost << mult.first << endl;
        }
      }

      rv += line + LF;
    }

// Exchange mults
    const bool exchange_mults_per_band = rules.exchange_mults_per_band();

    for (const auto& sm : _exchange_multipliers)
    { const string& field_name = sm.first;

      ost << "in summary string(), exchange field name = " << field_name << endl;

      line = pad_string(field_name, FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

      const multiplier& mult = sm.second;

      unsigned int total = 0;

//      for (unsigned int n1 = 0; n1 < permitted_bands.size(); ++n1)
      for (const auto& m : modes)
      { for (const auto& b : permitted_bands)
        { //const int band_nr = static_cast<int>(permitted_bands[n1]);
          const unsigned int n_exchange_mults = mult.n_worked(b, m);

//          line += pad_string(to_string(n_exchange_mults), FIELD_WIDTH);

          if (exchange_mults_per_band)
            total += n_exchange_mults;
        }

        if (!exchange_mults_per_band)
          total = mult.n_worked(ANY_BAND, m);
      }

      if (permitted_bands.size() != 1)
        line += pad_string(to_string(total), FIELD_WIDTH);
     }

// dupes
    line = pad_string("Dupes", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');



    for (const auto& m : modes)
    {     const auto& nd = _n_dupes[m];

//      for (unsigned int n = 0; n < permitted_bands.size(); ++n)
        for (const auto& b : permitted_bands)
      { //const int band_nr = static_cast<int>(permitted_bands[n]);

//        line += pad_string(to_string(nd.at(band_nr)), FIELD_WIDTH);
        dupes += nd[b];
      }
    }

    line += pad_string(to_string(dupes), FIELD_WIDTH);

    add_all_bands(permitted_bands.size(), dupes);

//    if (permitted_bands.size() != 1)
//      line += pad_string(to_string(dupes), FIELD_WIDTH);

    rv += line + LF;

// QSO points
    line = pad_string("Qpoints", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

    for (const auto& m : modes)
    { const auto& qp = _qso_points[m];

      for (const auto& b : permitted_bands)
//      for (unsigned int n = 0; n < permitted_bands.size(); ++n)
      { //const int band_nr = static_cast<int>(permitted_bands[n]);

//        line += pad_string(to_string(qp.at(band_nr)), FIELD_WIDTH);
        points += qp[b];
      }
    }

    line += pad_string(to_string(points), FIELD_WIDTH);


//    if (permitted_bands.size() != 1)
//      line += pad_string(to_string(points), FIELD_WIDTH);
    add_all_bands(permitted_bands.size(), points);


    rv += line;

    if (_include_qtcs)
    { line = LF + pad_string("QTC QSOs", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');
      line += pad_string((to_string(_qtc_qsos_sent) + "|" + to_string(_qtc_qsos_unsent)), FIELD_WIDTH * (permitted_bands.size() + 1));

      rv += line;
    }
  }

  return rv;
}


/// a (multi-line) string that summarizes the statistics
const string running_statistics::summary_string(const contest_rules& rules)
{ string rv;
  const unsigned int FIRST_FIELD_WIDTH = 10;
  const unsigned int FIELD_WIDTH       = 6;          // width of other fields

// write the bands and underline them
  string line(FIRST_FIELD_WIDTH, ' ');
  const vector<BAND>& permitted_bands = rules.permitted_bands();

  FOR_ALL(permitted_bands, [&line] (const BAND b) { line += pad_string(BAND_NAME[b], FIELD_WIDTH); } );

  if (permitted_bands.size() != 1)
    line += pad_string("All", FIELD_WIDTH);
  
  rv += (line + LF);
      
  line = string(FIRST_FIELD_WIDTH, ' ');

  FOR_ALL(permitted_bands, [&line] (const BAND) { line += pad_string("---", FIELD_WIDTH); } );

  if (permitted_bands.size() != 1)
    line += pad_string("---", FIELD_WIDTH);

  rv += line + LF;

// now create the individual per-mode tables
  const set<MODE> sm = rules.permitted_modes();
  vector<set<MODE>> vsm;

  for (const auto m : sm)
    vsm.push_back( { m } );

  if (vsm.size() > 1)
    vsm.push_back(sm);

//  ost << "number of mode sets = " << vsm.size() << endl;

  for (const auto& mode_set : vsm)
  { if (vsm.size()  != 1)
    { if (mode_set.size() == 1)
        rv += MODE_NAME[*(mode_set.cbegin())] + LF;
      else
        rv += "All" + LF;
    }
  
    rv += _summary_string(rules, mode_set);
    
    if ( (vsm.size() != 1) and (mode_set.size() == 1) )
      rv += LF;
  }
  
  return rv;
}
  
/// total points
const unsigned int running_statistics::points(const contest_rules& rules) const
{ unsigned int q_points = 0;
//  unsigned int callsign_mults = 0;
//  unsigned int country_mults = 0;
//  unsigned int exch_mults = 0;
  const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  const set<MODE> score_modes = rules.score_modes();
//  const map<BAND, int>& per_band_country_mult_factor = rules.per_band_country_mult_factor();
  
  SAFELOCK(statistics);

  for (const auto& m : score_modes)
  { const auto& qp = _qso_points[m];

    for (const auto& b : score_bands)
    { q_points += qp[b];
    }
  }

// callsign mults
  const unsigned int callsign_mults = n_worked_callsign_mults(rules);
  const unsigned int country_mults = n_worked_country_mults(rules);

//  for (const auto& b : permitted_bands)
//  { if (score_bands < b)
//    { q_points += _qso_points[b];
//      country_mults += (_country_multipliers.n_worked(b) * per_band_country_mult_factor.at(b));
//    }
//  }

// exchange mults
  const unsigned int exchange_mults = n_worked_exchange_mults(rules);

/*
  for (const auto& em : _exchange_multipliers)
  { const multiplier& m = em.second;

    if (m.per_band())
    { for (const auto& b : permitted_bands)
        if (score_bands < b)
          exch_mults += m.n_worked(b);
    }
    else
      exch_mults += m.n_worked(ALL_BANDS);
  }
*/

  const unsigned int rv = q_points * (country_mults + exchange_mults + callsign_mults);
  
  return rv;
}

/// worked callsign mults for a particular band
const set<string> running_statistics::worked_callsign_mults(const string& mult_name, const BAND b)
{ SAFELOCK(statistics);

  if (mult_name == string() and _callsign_multipliers.size() == 1)
    return _callsign_multipliers.cbegin()->second.worked(b);

  const auto& cit = _callsign_multipliers.find(mult_name);

  if (cit != _callsign_multipliers.cend())
    return cit->second.worked(b);

  return set<string>();
}

/// worked country mults for a particular band
const set<string> running_statistics::worked_country_mults(const BAND b, const MODE m)
{ SAFELOCK(statistics);

//  return ( _country_multipliers.worked(static_cast<int>(b)) );

  const auto tmp = _country_multipliers.worked(b, m);

  ost << "running_statistics::worked_country_mults; band = " << BAND_NAME[b] << ", mode = " << MODE_NAME[m] << endl;
  string str("  ");

  for (const auto& cm : tmp)
    str += cm + " ";

  ost << str << endl;

  return ( _country_multipliers.worked(b, m) );
}

/// worked exchange mults for a particular band
const map<string /* field name */, set<string> /* values */ >  running_statistics::worked_exchange_mults(const BAND b)
{ map<string, set<string> > rv;

  SAFELOCK(statistics);

  FOR_ALL(_exchange_multipliers, [=, &rv] (const pair<string, multiplier>& psm) { rv.insert( { psm.first, psm.second.worked(b) } ); } );


//  for (const auto& psm : _exchange_multipliers)
//  { const string& field_name = psm.first;
//    const multiplier& m = psm.second;
//
//    rv.insert( { field_name, m.worked(b) } );
//  }

  return rv;
}

const set<string> running_statistics::worked_exchange_mults(const string& exchange_field_name, const BAND b)
{ SAFELOCK(statistics);

  for (const auto& psm : _exchange_multipliers)
  { const string& field_name = psm.first;
    const multiplier& m = psm.second;

    if (field_name == exchange_field_name)
      return m.worked(b);
  }

  return set<string>();
}

const array<set<string>, N_BANDS> running_statistics::worked_exchange_mults(const string& exchange_field_name)
{ array<set<string>, N_BANDS> rv;

  for (size_t n = 0; n < N_BANDS; ++n)
    rv[n] = worked_exchange_mults(exchange_field_name, static_cast<BAND>(n));

  return rv;
}

// clear dynamic info
void running_statistics::clear_info(void)
{ SAFELOCK(statistics);

//  _n_dupes.fill(0);
//  _n_qsos.fill(0);
//  _qso_points.fill(0);

  _n_dupes = move(decltype(_n_dupes)( { { } } ));
  _n_qsos = move(decltype(_n_qsos)( { { } } ));
  _qso_points = move(decltype(_qso_points)( { { } } ));

  for (auto& callsign_m : _callsign_multipliers)
    callsign_m.second.clear();

  _country_multipliers.clear();

//  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  // vector so we can keep the correct order
  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  { pair<string, multiplier>& sm = _exchange_multipliers[n];

    sm.second.clear();
  }
}

void running_statistics::qtc_qsos_sent(const unsigned int n)
{ SAFELOCK(statistics);

  if (_include_qtcs)
    _qtc_qsos_sent = n;
}

void running_statistics::qtc_qsos_unsent(const unsigned int n)
{ SAFELOCK(statistics);

  if (_include_qtcs)
    _qtc_qsos_unsent = n;
}

// lots of ways to come up with a plausible definition for the value of a mult
const float running_statistics::mult_to_qso_value(const contest_rules& rules, const BAND b, const MODE m) const
{ const unsigned int current_mults = n_worked_callsign_mults(rules) + n_worked_country_mults(rules) + n_worked_exchange_mults(rules);
  const set<BAND> score_bands = rules.score_bands();
  const set<MODE> score_modes = rules.score_modes();
  unsigned int current_qso_points = 0;

  SAFELOCK(statistics);

// current QSO points
  for (const auto& m : score_modes)
  { const auto& qp = _qso_points[m];

    FOR_ALL(score_bands, [&] (const BAND& b) { current_qso_points += qp[static_cast<int>(b)]; } );
  }

  const unsigned int current_points = current_qso_points * current_mults;  // should be same as points(rules)

// add a notional mult
  unsigned int notional_mults = current_mults;
  const map<BAND, int>& per_band_country_mult_factor = rules.per_band_country_mult_factor();

// include the correct factor for a mult on the current band
  if (_country_multipliers.used())
  { notional_mults += per_band_country_mult_factor.at(b);
  }
  else
    notional_mults++;

// add a notional QSO; first calculate average qso point worth of a qso on the current band and mode
  const auto& qp = _qso_points[m];
  const unsigned int& n_qso_points = qp[b];
  const auto& nq = _n_qsos[m];
  const unsigned int& n_qsos = nq[b];

  const float mean_qso_points = (n_qsos ? ( static_cast<float>(n_qso_points) / n_qsos ) : 0);

  const unsigned int new_qso = static_cast<unsigned int>((mean_qso_points * (n_qsos + 1) ) * current_mults);
  const unsigned int new_mult = static_cast<unsigned int>((mean_qso_points * (n_qsos + 1) ) * notional_mults);

//  ost << "current points = " << current_points << endl;
//  ost << "new qso = " << new_qso << endl;
//  ost << "new mult = " << new_mult << endl;

  const unsigned int new_qso_value = new_qso - current_points;
  const unsigned int new_mult_value = new_mult - current_points;

//  ost << "new qso value = " << new_qso_value << endl;
//  ost << "new mult value = " << new_mult_value << endl;

//  ost << "returning: " << static_cast<float>(new_mult_value) / new_qso_value << endl;

  return ( static_cast<float>(new_mult_value) / new_qso_value );
}

/*! \brief                  How many QSOs have been made?
    \param  rules           rules for this contest

    Counts only those QSOs on bands and modes being used to calculate the score. Includes dupes.
*/
const unsigned int running_statistics::n_qsos(const contest_rules& rules) const
{ const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  const set<MODE> score_modes = rules.score_modes();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  for (const auto& m : score_modes)
  { const auto& nq = _n_qsos[m];

    FOR_ALL(score_bands, [&] (const BAND& b) { rv += nq[static_cast<int>(b)]; } );
  }

  return rv;
}

/*! \brief                  How many QSOs have been made in a particular mode?
    \param  rules           rules for this contest
    \param  m               target mode

    Counts only those QSOs on bands being used to calculate the score. Includes dupes.
*/
const unsigned int running_statistics::n_qsos(const contest_rules& rules, const MODE m) const
{ const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  unsigned int rv = 0;
  const auto& nq = _n_qsos[m];

  SAFELOCK(statistics);

  FOR_ALL(score_bands, [&] (const BAND& b) { rv += nq[static_cast<int>(b)]; } );

  return rv;
}

const unsigned int running_statistics::n_worked_callsign_mults(const contest_rules& rules) const
{ const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  for (const auto& sm : _callsign_multipliers)
  { const multiplier& mult = sm.second;

    if (mult.per_band())
    { for (const auto& b : permitted_bands)
        if (score_bands < b)
          rv += mult.n_worked(b);
    }
    else
      rv += mult.n_worked(ALL_BANDS);
  }

  return rv;
}

const unsigned int running_statistics::n_worked_country_mults(const contest_rules& rules) const
{ //const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  const map<BAND, int>& per_band_country_mult_factor = rules.per_band_country_mult_factor();
  unsigned int rv = 0;

  SAFELOCK(statistics);

//  for (const auto& b : permitted_bands)
//  { if (score_bands < b)
//      rv += (_country_multipliers.n_worked(b) * per_band_country_mult_factor.at(b));
//  }

  FOR_ALL(score_bands, [&] (const BAND& b) { rv += (_country_multipliers.n_worked(b) * per_band_country_mult_factor.at(b)); } );

  return rv;
}


const unsigned int running_statistics::n_worked_exchange_mults(const contest_rules& rules) const
{ const vector<BAND> permitted_bands = rules.permitted_bands();
  const set<MODE> permitted_modes = rules.permitted_modes();
//  const set<BAND> score_bands = rules.score_bands();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  for (const auto& em : _exchange_multipliers)
  { const multiplier& mult = em.second;

    if (mult.per_mode())
    { for (const auto& m : permitted_modes)
      { if (mult.per_band())
        { for (const auto& b : permitted_bands)
              rv += mult.n_worked(b, m);
        }
        else
          rv += mult.n_worked(ANY_BAND, m);
      }
    }
    else
    { for (const auto& m : permitted_modes)
      { if (mult.per_band())
        { for (const auto& b : permitted_bands)
              rv += mult.n_worked(b, ANY_MODE);
        }
        else
          rv += mult.n_worked(ANY_BAND, ANY_MODE);
      }
    }
  }

  return rv;
}

// -----------  call history  ----------------

/*!     \class call_history
        \brief History of each call worked
*/

void call_history::rebuild(const logbook& logbk)
{ SAFELOCK(_history);

  _history.clear();

  for (const auto& qso : logbk.as_vector())
    (*this) += qso;
}

void call_history::operator+=(const QSO& qso)
{ SAFELOCK(_history);

  _history[qso.callsign()].insert( { qso.band(), qso.mode() } );
}

const bool call_history::worked(const std::string& s, const BAND b , const MODE m)
{ SAFELOCK(_history);

  const auto& cit = _history.find(s);

  if (cit == _history.cend())
    return false;

  const set<bandmode>& sbm = cit->second;
  const set<bandmode>::const_iterator scit = sbm.find( { b, m } );

  return (scit != sbm.end());
}

const bool call_history::worked(const string& s)
{ SAFELOCK(_history);
  return (_history.find(s) != _history.end());
}

void call_history::clear(void)
{ SAFELOCK(_history);
  _history.clear();
}
