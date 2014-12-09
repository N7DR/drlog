// $Id: statistics.cpp 83 2014-11-10 21:31:02Z  $

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
void running_statistics::_insert_callsign_mult(const string& mult_name, const string& mult_value, const unsigned int band_nr)
{ if (_callsign_mults_used and !mult_value.empty())     // do we actually have to do anything?
  { SAFELOCK(statistics);

    if (known_callsign_mult_name(mult_name))            // do we already know about this mult name?
    { auto it = _callsign_multipliers.find(mult_name);
      multiplier& m = it->second;

      m.add_worked(mult_value, band_nr);                // add value and band for this mult name
    }
    else                                                // unknown mult name
    { multiplier m;                                     // create new mult

      m.add_worked(mult_value, band_nr);                // we've worked it
      _callsign_multipliers.insert( { mult_name, m } ); // store the info
    }
  }
}

/// default constructor
running_statistics::running_statistics(void) :
  _n_qsos( {} ),                                 // Josuttis 2nd ed., p.262 -- initializes all elements with zero
  _n_dupes( {} ),
  _qso_points( {} ),
  _n_qsosbm( { {} } ),
  _n_dupesbm( { {} } ),
  _qso_pointsbm( { {} } ),
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
  _n_qsos( {} ),                                 // Josuttis 2nd ed., p.262 -- initializes all elements with zero
  _n_dupes( {} ),
  _qso_points( {} ),
  _n_qsosbm( { {} } ),
  _n_dupesbm( { {} } ),
  _qso_pointsbm( { {} } ),
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

  FOR_ALL(exchange_mults, [&] (const string& exchange_mult) { _exch_mult_fields.insert(exchange_mult); } );

// callsign mults
  if (_callsign_mults_used)
  { const set<string> callsign_mult_names = rules.callsign_mults();

    if (!callsign_mult_names.empty())                              // should always be true
    { for (const auto& callsign_mult_name : callsign_mult_names)
      { multiplier em;

        if (rules.callsign_mults_per_band())
          em.per_band(true);

        if (rules.callsign_mults_per_mode())
          em.per_mode(true);

        em.used(true);

        _callsign_multipliers[callsign_mult_name] = em;
      }
    }
  }

// country mults
  if (_country_mults_used)
  { const set<string> country_mults = rules.country_mults();

    if (!country_mults.empty() or context.auto_remaining_country_mults())  // should always be true
      _country_multipliers.used(true);

    if (rules.country_mults_per_band())
      _country_multipliers.per_band(true);

    if (rules.country_mults_per_mode())
      _country_multipliers.per_mode(true);

    if (!context.auto_remaining_country_mults())
      _country_multipliers.add_known(country_mults);
  }

// exchange mults
  if (_exchange_mults_used)
  { for (const auto& exchange_mult_name : exchange_mults)
    { multiplier em;

      em.used(true);

      if (rules.exchange_mults_per_band())
        em.per_band(true);

      if (rules.exchange_mults_per_mode())
        em.per_mode(true);

// if values are obtained from grep, then the next line returns an empty vector
      const vector<string> canonical_values = rules.exch_canonical_values(exchange_mult_name);

      if (!canonical_values.empty())
        em.add_known(canonical_values);

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

/*! \brief              do we still need to work a particular callsign mult on a particular band?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
    \return             whether the mult <i>mult_name</i> with the value <i>mult_value</i> is needed on band <i>b</i>
*/
const bool running_statistics::is_needed_callsign_mult(const string& mult_name, const string& mult_value, const BAND b) const
{ SAFELOCK(statistics);

  if (!known_callsign_mult_name(mult_name))
  { ost << "in running_statistics::is_needed_callsign_mult(), unknown callsign mult name = " << mult_name << endl;
    return false;
  }

  const auto cit = _callsign_multipliers.find(mult_name);
  const multiplier& m = cit->second;
  const bool worked = m.is_worked(mult_value, b);

  return !(worked);
}

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

/*! \brief  do we still need to work a particular country as a mult on a particular band?
    \param  callsign    call to test
    \param  b           band to test
*/
const bool running_statistics::is_needed_country_mult(const string& callsign, const BAND band_nr)
{ try
  { SAFELOCK(statistics);

    const string canonical_prefix = _location_db.canonical_prefix(callsign);
    const bool is_needed = ( (_country_multipliers.is_known(canonical_prefix)) and !(_country_multipliers.is_worked(canonical_prefix, band_nr)) );

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
  
  const BAND& b = qso.band();
  const unsigned int band_nr = static_cast<int>(b);
  const MODE& mo = qso.mode();
  const unsigned int mode_nr = static_cast<int>(mo);

// increment the number of QSOs
  _n_qsos[band_nr]++;

  auto& pb = _n_qsosbm[mode_nr];
  pb[band_nr]++;
  
// multipliers

// callsign mults
// for now, just assume that there's at most one possible callsign mult, and the value is in qso.prefix()

  if (!_callsign_multipliers.empty() and !(qso.prefix().empty()) )
  { const string mult_name = _callsign_multipliers.begin()->first;
    multiplier& m = _callsign_multipliers.begin()->second;

//    ost << "inside running_statistics::add_qso(), processing callsign mult " << mult_name << endl;
//    ost << "callsign = " << qso.callsign() << "; prefix = " << qso.prefix() << endl;
//    ost << "initial m = " << m << endl;

    m.unconditional_add_worked(qso.prefix(), band_nr);
    m.unconditional_add_worked(qso.prefix(), band_nr, mo);

//    ost << "middle m = " << m << endl;
    _callsign_multipliers[mult_name] = m;
//    ost << "final m = " << m << endl;
  }

// country mults
  const string& call = qso.callsign();
  const string& canonical_prefix = _location_db.canonical_prefix(call);

//  ost << "adding worked country mult: " << canonical_prefix << " on band " << band_nr << endl;
  _country_multipliers.add_worked(canonical_prefix, band_nr);
  _country_multipliers.add_worked(canonical_prefix, band_nr, mo);

///  ost << "was added? = " << added << endl;

// exchange mults
//  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  // vector so we can keep the correct order
  for (auto& exchange_multiplier : _exchange_multipliers)
  { const string& field_name = exchange_multiplier.first;
    multiplier& m = exchange_multiplier.second;
    const string value = qso.received_exchange(field_name);
    const string mv = MULT_VALUE(field_name, value);  // the mult value of the received field

//    ost << "Inside running_statistics::add_qso()" << endl;
//    ost << "QSO: " << qso << endl;
//    ost << "field_name = " << field_name << endl;
//    ost << "value: " << value << endl;
//    ost << "mult value: " << mv << endl;

    if (!value.empty())
    { m.unconditional_add_worked(mv, band_nr);
      m.unconditional_add_worked(mv, band_nr, mo);
    }

//    ost << "exchange multiplier object: " << m << endl;
  }
  
  const bool is_dupe = log.is_dupe(qso, rules);

  if (is_dupe)
  { _n_dupes[band_nr]++;

    auto& pb = _n_dupesbm[mode_nr];
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

    _qso_points[band_nr] += points_this_qso;
    _qso_pointsbm[mode_nr][band_nr] += points_this_qso;
  }
}

//std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  ///< exchange multipliers; vector so we can keep the correct order

const bool running_statistics::add_known_exchange_mult(const string& name, const string& value)
{ SAFELOCK(statistics);
//  ost << "in add_known_exchange_mult; name = " << name << ", value = " << value << endl;
//  ost << "size of _exchange_multipliers = " << _exchange_multipliers.size() << endl;

  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  { pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

    if (sm.first == name)
    { const bool added = sm.second.add_known(MULT_VALUE(name, value));

      if (added)
      { ost << "added known exchange mult: " << name << ", value = " << value << ", mult value = " << MULT_VALUE(name, value) << endl;
        return true;
      }
    }
  }

  return false;
}

const set<string> running_statistics::known_exchange_mults(const string& name)
{ SAFELOCK(statistics);

  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  { pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

    if (sm.first == name)
      return sm.second.known();
  }

  return set<string>();
}

/*! \brief  Add a worked exchange mult
    \param  field_name    Exchange mult field name
    \param  field_value   Value of the field <i>field_name</i>
    \param  band_nr       Number of the band on which worked mult is to be added

    Doesn't add if the value is unknown.
*/
void running_statistics::add_worked_exchange_mult(const string& field_name, const string& field_value, const int b)
{ if (field_value.empty())
    return;

  const string mv = MULT_VALUE(field_name, field_value);  // the mult value of the received field

  SAFELOCK(statistics);

  for (auto& psm : _exchange_multipliers)
    if (psm.first == field_name)
      psm.second.add_worked(mv, b);
}

/// rebuild
void running_statistics::rebuild(const logbook& log, const contest_rules& rules)
{ logbook l;

// done this way so as to account for dupes correctly
  for (const auto& qso : log.as_vector())
  { this->add_qso(qso, l, rules);
    l += qso;
  }
}

/// do we still need to work a particular exchange mult on a particular band?
const bool running_statistics::is_needed_exchange_mult(const string& exchange_field_name, const string& exchange_field_value, const BAND b) const
{ const string mv = MULT_VALUE(exchange_field_name, exchange_field_value);  // the mult value of the received field

  SAFELOCK(statistics);

//  ost << "in is_needed_exchange_mult for field " << exchange_field_name << ", value = " << exchange_field_value << ", and band = " << b << ", mult value = " << mv << endl;

  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  { const pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

//    ost << "checking exchange field name = " << sm.first << endl;

    if (sm.first == exchange_field_name and sm.second.is_known(mv) /* sm.second.is_known(exchange_field_value) */)
    { ost << "found known field name; returning " << !(sm.second.is_worked(mv, b)) << endl;;
//      return !(sm.second.is_worked(exchange_field_value, b));
      return !(sm.second.is_worked(mv, b));
    }
  }

  return false;
}

/// a string list of bands on which a particular exchange mult value is needed
const std::string running_statistics::exchange_mult_needed(const string& exchange_field_name, const string& exchange_field_value, const contest_rules& rules)
{ string rv;

  for (const auto& b : rules.permitted_bands())
    rv += ((is_needed_exchange_mult(exchange_field_name, exchange_field_value, b)) ?  BAND_NAME[static_cast<int>(b)] : "   ");
   
  return rv;
}

const string _summary_string(const contest_rules& rules, const unsigned int n_mode)
{ string rv;

  return rv;
}

/// a (multi-line) string that summarizes the statistics
const string running_statistics::summary_string(const contest_rules& rules)
{ string rv;
  const unsigned int FIRST_FIELD_WIDTH = 10;
  const unsigned int FIELD_WIDTH       = 6;          // width of other fields
//  const set<MODE> permitted_modes = rules.permitted_modes();
//  const unsigned int n_modes = permitted_modes.size();

//  for (unsigned int n_mode = 0; n_mode < n_modes; ++n_mode)
//  { auto cit = permitted_modes.cbegin();
//    for (unsigned int nxt = 0; nxt < n_mode; ++nxt)
//      cit = next(cit);
//    const MODE m = *(cit);
//    const string mode_name = ( m == MODE_CW ? "CW" : "SSB" );

//    rv += mode_name + LF;

// first line is the bands
    string line(FIRST_FIELD_WIDTH, ' ');
    const vector<BAND>& permitted_bands = rules.permitted_bands();
  
    for (unsigned int permitted_band_nr = 0; permitted_band_nr < permitted_bands.size(); ++permitted_band_nr)
      line += pad_string(BAND_NAME.at(permitted_bands[permitted_band_nr]), FIELD_WIDTH);

    if (permitted_bands.size() != 1)
      line += pad_string("All", FIELD_WIDTH);
  
    rv += (line + LF);
      
// underline the bands
  line = string(FIRST_FIELD_WIDTH, ' ');

  for (unsigned int permitted_band_nr = 0; permitted_band_nr < permitted_bands.size(); ++permitted_band_nr)
    line += pad_string("---", FIELD_WIDTH);

  if (permitted_bands.size() != 1)
    line += pad_string("---", FIELD_WIDTH);

  rv += line + LF;

  SAFELOCK(statistics);
  
// QSOs
  line = pad_string("QSOs", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');
  
  unsigned int qsos = 0;                                          // accumulator for total qsos
  for (unsigned int n = 0; n < permitted_bands.size(); ++n)
  { const int band_nr = static_cast<int>(permitted_bands[n]);

    line += pad_string(to_string(_n_qsos.at(band_nr)), FIELD_WIDTH);
    qsos += _n_qsos[band_nr];
  }

  if (permitted_bands.size() != 1)
    line += pad_string(to_string(qsos), FIELD_WIDTH);

  rv += line + LF; 
  
// country mults
  if (_country_multipliers.used())                                         // if countries are mults
  { line = pad_string("Countries", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

    unsigned int countries = 0;
    
    for (unsigned int n = 0; n < permitted_bands.size(); ++n)
    { const int band_nr = static_cast<int>(permitted_bands[n]);
      const unsigned int n_countries = _country_multipliers.n_worked(band_nr);

      ost << "band nr " << n << "; n_countries = " << n_countries << endl;

      line += pad_string(to_string(n_countries), FIELD_WIDTH); 
      countries += n_countries;
    }
  
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

      //const array<set<string>, N_BANDS>& ar = _worked_callsign_mults;
      //std::map<std::string /* mult name */, multiplier>                 _callsign_multipliers;
      const auto& cit = _callsign_multipliers.find(mult_name);

      if (cit != _callsign_multipliers.end())    // should always be true
      { //ost << "found callsign multiplier: " << cit->first << endl;

        const multiplier& m = cit->second;
        unsigned int total = 0;

        for (unsigned int n1 = 0; n1 < permitted_bands.size(); ++n1)
        { const int band_nr = static_cast<int>(permitted_bands[n1]);
          const unsigned int n_callsign_mults = m.n_worked(band_nr);

          line += pad_string(to_string(n_callsign_mults), FIELD_WIDTH);

          if (callsign_mults_per_band)
            total += n_callsign_mults;
        }

        if (!callsign_mults_per_band)
          total = m.n_worked(N_BANDS);

 //       if (permitted_bands.size() != 1)
          line += pad_string(to_string(total), FIELD_WIDTH);

//        ost << "line is: " << line << endl;
      }
      else
      { ost << "Error: did not find mult name: " << mult_name << endl;
        ost << "Number of callsign multipliers in statistics = " << _callsign_multipliers.size() << endl;

        for (const auto& m : _callsign_multipliers)
          ost << m.first << endl;
      }
    }

    rv += line + LF;
  }

// Exchange mults
  const bool exchange_mults_per_band = rules.exchange_mults_per_band();

  for (const auto& sm : _exchange_multipliers)
  { const string& field_name = sm.first;

//    ost << "in summary string(), exchange field name = " << field_name << endl;

    line = pad_string(field_name, FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

    const multiplier& m = sm.second;

    unsigned int total = 0;

    for (unsigned int n1 = 0; n1 < permitted_bands.size(); ++n1)
    { const int band_nr = static_cast<int>(permitted_bands[n1]);
      const unsigned int n_exchange_mults = m.n_worked(band_nr);

      line += pad_string(to_string(n_exchange_mults), FIELD_WIDTH);

      if (exchange_mults_per_band)
        total += n_exchange_mults;
    }

    if (!exchange_mults_per_band)
      total = m.n_worked(N_BANDS);

//    if (permitted_bands.size() != 1)
      line += pad_string(to_string(total), FIELD_WIDTH);

    rv += line + LF;
  }

// dupes
  line = pad_string("Dupes", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');
  
  unsigned int dupes = 0;
  for (unsigned int n = 0; n < permitted_bands.size(); ++n)
  { const int band_nr = static_cast<int>(permitted_bands[n]);

    line += pad_string(to_string(_n_dupes.at(band_nr)), FIELD_WIDTH);
    dupes += _n_dupes[band_nr];
  }

  if (permitted_bands.size() != 1)
    line += pad_string(to_string(dupes), FIELD_WIDTH);

  rv += line + LF; 

// QSO points  
  line = pad_string("Qpoints", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');
  
  unsigned int points = 0;
  for (unsigned int n = 0; n < permitted_bands.size(); ++n)
  { const int band_nr = static_cast<int>(permitted_bands[n]);

    line += pad_string(to_string(_qso_points.at(band_nr)), FIELD_WIDTH);
    points += _qso_points[band_nr];
  }

  if (permitted_bands.size() != 1)
    line += pad_string(to_string(points), FIELD_WIDTH);

  rv += line; 

  if (_include_qtcs)
  { line = LF + pad_string("QTC QSOs", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');
    line += pad_string((to_string(_qtc_qsos_sent) + "|" + to_string(_qtc_qsos_unsent)), FIELD_WIDTH * (permitted_bands.size() + 1));

    rv += line;
  }

//  }
  
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
//  const map<BAND, int>& per_band_country_mult_factor = rules.per_band_country_mult_factor();
  
  SAFELOCK(statistics);

  for (const auto& b : permitted_bands)
  { if (score_bands < b)
    { q_points += _qso_points[b];
//      country_mults += (_country_multipliers.n_worked(b) * per_band_country_mult_factor.at(b));
    }
  }

// callsign mults
  const unsigned int callsign_mults = n_worked_callsign_mults(rules);

/*
  for (const auto& sm : _callsign_multipliers)
  { const multiplier& m = sm.second;

    if (m.per_band())
    { for (const auto& b : permitted_bands)
        if (score_bands < b)
          callsign_mults += m.n_worked(b);
    }
    else
      callsign_mults += m.n_worked(ALL_BANDS);
  }
*/

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
const set<string> running_statistics::worked_country_mults(const BAND b)
{ SAFELOCK(statistics);

  return ( _country_multipliers.worked(static_cast<int>(b)) );
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

  _n_dupes.fill(0);
  _n_qsos.fill(0);
  _qso_points.fill(0);

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
const float running_statistics::mult_to_qso_value(const contest_rules& rules, const BAND b) const
{ //const unsigned int current_points = points(rules);
  const unsigned int current_mults = n_worked_callsign_mults(rules) + n_worked_country_mults(rules) + n_worked_exchange_mults(rules);
  const set<BAND>& score_bands = rules.score_bands();
  unsigned int current_qso_points = 0;

  SAFELOCK(statistics);

// current QSO points
  FOR_ALL(score_bands, [&] (const BAND& b) { current_qso_points += _qso_points[static_cast<int>(b)]; } );

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

// add a notional QSO; first calculate average qso point worth of a qso on the current band
  const unsigned int& n_qso_points = _qso_points[b];
  const unsigned int& n_qsos = _n_qsos[b];

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

const unsigned int running_statistics::n_qsos(const contest_rules& rules) const
{ const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  FOR_ALL(score_bands, [&] (const BAND& b) { rv += _n_qsos[static_cast<int>(b)]; } );

  return rv;
}

const unsigned int running_statistics::n_worked_callsign_mults(const contest_rules& rules) const
{ const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  for (const auto& sm : _callsign_multipliers)
  { const multiplier& m = sm.second;

    if (m.per_band())
    { for (const auto& b : permitted_bands)
        if (score_bands < b)
          rv += m.n_worked(b);
    }
    else
      rv += m.n_worked(ALL_BANDS);
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
{ const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  for (const auto& em : _exchange_multipliers)
  { const multiplier& m = em.second;

    if (m.per_band())
    { for (const auto& b : permitted_bands)
        if (score_bands < b)
          rv += m.n_worked(b);
    }
    else
      rv += m.n_worked(ALL_BANDS);
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
