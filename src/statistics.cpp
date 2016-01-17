// $Id: statistics.cpp 119 2016-01-16 18:32:13Z  $

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

pt_mutex statistics_mutex;      ///< mutex for the (singleton) running_statistics object

// -----------  running_statistics  ----------------

/*! \brief              Add a callsign mult name, value and band to those worked
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
    { multiplier& mult = (_callsign_multipliers.find(mult_name))->second;

      mult.add_worked(mult_value, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr));                // add value and band for this mult name
    }
    else                                                   // unknown mult name
    { multiplier mult;                                     // create new mult

      mult.add_worked(mult_value, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr));                // we've worked it
      _callsign_multipliers.insert( { mult_name, mult } ); // store the info
    }
  }
}

/*! \brief          Generate a summary string for display
    \param  rules   rules for this contest
    \param  modes   the set of modes that are to be included in the summary string
    \return         summary string for modes in <i>modes</i>
*/
const string running_statistics::_summary_string(const contest_rules& rules, const set<MODE>& modes)
{ string rv;

  const unsigned int FIRST_FIELD_WIDTH = 10;
  const unsigned int FIELD_WIDTH       = 6;          // width of other fields
  const set<MODE> permitted_modes = rules.permitted_modes();
  const vector<BAND> permitted_bands = rules.permitted_bands();

  unsigned int qsos_all_bands = 0;
  unsigned int countries_all_bands = 0;
  unsigned int dupes_all_bands = 0;
  unsigned int points_all_bands = 0;

  string line;

  auto add_all_bands = [&line] (const unsigned int sz, const unsigned int n) { if (sz != 1)
                                                                                 line += pad_string(to_string(n), FIELD_WIDTH);
                                                                             };

  { SAFELOCK(statistics);

// QSOs
    line = pad_string("QSOs", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');                                          // accumulator for total qsos

    for (const auto& b : permitted_bands)
    { unsigned int qsos = 0;

      for (const auto& m : modes)
      { const auto& nq = _n_qsos[m];

        if (modes.size() == 1)
          line += pad_string(to_string(nq[b]), FIELD_WIDTH);
        qsos += nq[b];
      }

      qsos_all_bands += qsos;

      if (modes.size() != 1)
        line += pad_string(to_string(qsos), FIELD_WIDTH);
    }

    add_all_bands(permitted_bands.size(), qsos_all_bands);

    rv += line + LF;

// country mults
    if (_country_multipliers.used())                                         // if countries are mults
    { line = pad_string("Countries", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

      for (const auto& b : permitted_bands)
      { unsigned int countries = 0;

        for (const auto& m : modes)
        { const unsigned int n_countries = _country_multipliers.n_worked(b, m);

          if (modes.size() == 1)
            line += pad_string(to_string(n_countries), FIELD_WIDTH);

          countries += n_countries;
        }

        countries_all_bands += countries;

        if (modes.size() != 1)
          line += pad_string(to_string(countries), FIELD_WIDTH);
      }

      add_all_bands(permitted_bands.size(), countries_all_bands);
      rv += line + LF;
    }

// callsign mults
    const set<string>&  callsign_mults = rules.callsign_mults();      // collection of types of mults based on callsign (e.g., "WPXPX")

    if (!callsign_mults.empty())
    { for (const auto mult_name : callsign_mults)
      { const MODE m = ((modes.size() == 1) ? *(modes.cbegin()) : ANY_MODE);

        line = pad_string(mult_name, FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

        const auto& cit = _callsign_multipliers.find(mult_name);

        if (cit != _callsign_multipliers.end())    // should always be true
        { const multiplier& mult = cit->second;

          for (const auto& b : permitted_bands)
          {
            { const unsigned int n_callsign_mults = mult.n_worked(b, m);

              line += pad_string(to_string(n_callsign_mults), FIELD_WIDTH);
            }
          }

          if (permitted_bands.size() != 1)
            line += pad_string(to_string(mult.n_worked(ANY_BAND, m)), FIELD_WIDTH);
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

//    ost << "Number of exchange multipliers = " << _exchange_multipliers.size() << endl;

    for (const auto& sm : _exchange_multipliers)
    { const string& field_name = sm.first;

      line = pad_string(field_name, FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

      const multiplier& mult = sm.second;
      unsigned int total = 0;

      const MODE m = ((modes.size() == 1) ? *(modes.cbegin()) : ANY_MODE);

      for (const auto& b : permitted_bands)
      {
        { const unsigned int n_exchange_mults = mult.n_worked(b, m);

            line += pad_string(to_string(n_exchange_mults), FIELD_WIDTH);

          if (exchange_mults_per_band)
            total += n_exchange_mults;

        }
      }

      if (permitted_bands.size() != 1)
      { if (exchange_mults_per_band)
          add_all_bands(permitted_bands.size(), total);
        else
          line += pad_string(to_string(mult.n_worked(ANY_BAND, m)), FIELD_WIDTH);
      }

      rv += line + LF;

//      ost << "line = " << line << endl;

     }

// dupes
     line = pad_string("Dupes", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

     for (const auto& b : permitted_bands)
     { unsigned int dupes = 0;

       for (const auto& m : modes)
       { const auto& nd = _n_dupes[m];

         if (modes.size() == 1)
           line += pad_string(to_string(nd[b]), FIELD_WIDTH);

         dupes += nd[b];
       }

       dupes_all_bands += dupes;

       if (modes.size() != 1)
         line += pad_string(to_string(dupes), FIELD_WIDTH);
     }

    add_all_bands(permitted_bands.size(), dupes_all_bands);
    rv += line + LF;

// QSO points
    line = pad_string("Qpoints", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');

    for (const auto& b : permitted_bands)
    { unsigned int points = 0;

      for (const auto& m : modes)
      { const auto& qp = _qso_points[m];

        if (modes.size() == 1)
          line += pad_string(to_string(qp[b]), FIELD_WIDTH);
        points += qp[b];
      }

      points_all_bands += points;

      if (modes.size() != 1)
        line += pad_string(to_string(points), FIELD_WIDTH);
    }

    add_all_bands(permitted_bands.size(), points_all_bands);
    rv += line;

    if (_include_qtcs)
    { line = LF + pad_string("QTC QSOs", FIRST_FIELD_WIDTH, PAD_RIGHT, ' ');
      line += pad_string((to_string(_qtc_qsos_sent) + "|" + to_string(_qtc_qsos_unsent)), FIELD_WIDTH * (permitted_bands.size() + 1));

      rv += line;
    }
  }

  return rv;
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

/*! \brief                  Constructor
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

/*! \brief                  Prepare an object that was created with the default constructor
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

      _exchange_multipliers.push_back( { exchange_mult_name, em } );
    }
  }
}

/*! \brief                                  Is a particular string a known callsign mult name?
    \param  putative_callsign_mult_name     string to test
    \return                                 whether <i>putative_callsign_mult_name</i> is a known callsign mult name
*/
const bool running_statistics::known_callsign_mult_name(const string& putative_callsign_mult_name) const
{ SAFELOCK(statistics);

  return ( _callsign_multipliers.find(putative_callsign_mult_name) != _callsign_multipliers.cend() );
}

/*! \brief              Do we still need to work a particular callsign mult on a particular band and mode?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
    \param  m           mode to test
    \return             whether the callsign mult <i>mult_name</i> with the value <i>mult_value</i> is still needed on band <i>b</i> and mode <i>m</i>
*/
const bool running_statistics::is_needed_callsign_mult(const string& mult_name, const string& mult_value, const BAND b, const MODE m) const
{ SAFELOCK(statistics);

  if (!known_callsign_mult_name(mult_name))
    return false;

  const auto cit = _callsign_multipliers.find(mult_name);

  if (cit != _callsign_multipliers.cend())                          // should always be true
  { const multiplier& mult = cit->second;
    const bool worked = mult.is_worked(mult_value, b, m);

    return !(worked);
  }

  ost << "ERROR: Unknown multiplier name: " << mult_name << endl;
  return false;
}

/*! \brief              Do we still need to work a particular country as a mult on a particular band and mode?
    \param  callsign    call to test
    \param  b           band to test
    \param  m           mode to test
    \return             whether the country in which <i>callsign</i> is located is needed as a mult on band <i>b</i> and mode <i>m</i>
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
    \param  str     canonical prefix of mult
    \return         whether <i>str</i> was actually added

    Does nothing and returns <i>false<i> if <i>str</i> is already known
*/
const bool running_statistics::add_known_country_mult(const string& str, const contest_rules& rules)
{ SAFELOCK(statistics);

  if (rules.country_mults() < str)                  // all the legal country mults
    return (_country_multipliers.add_known(str));

  return false;
}

/*! \brief          Add a QSO to the ongoing statistics
    \param  qso     QSO to add
    \param  log     logbook (without the qso <i>qso</i>)
    \param  rules   contest rules
*/
void running_statistics::add_qso(const QSO& qso, const logbook& log, const contest_rules& rules)
{ SAFELOCK(statistics);
  
  const BAND& b = qso.band();
  const unsigned int band_nr = static_cast<int>(b);
  const MODE& mo = qso.mode();
  const unsigned int mode_nr = static_cast<int>(mo);

// increment the number of QSOs
  auto& pb = _n_qsos[mode_nr];
  pb[band_nr]++;

// multipliers

// callsign mults
// for now, just assume that there's at most one possible callsign mult, and the value is in qso.prefix()
  if (!_callsign_multipliers.empty() and !(qso.prefix().empty()) )
  { const string mult_name = _callsign_multipliers.begin()->first;
    multiplier& m = _callsign_multipliers.begin()->second;

    m.unconditional_add_worked(qso.prefix(), static_cast<BAND>(band_nr), static_cast<MODE>(mo));

    _callsign_multipliers[mult_name] = m;
  }

// country mults
  const string& call = qso.callsign();
  const string& canonical_prefix = _location_db.canonical_prefix(call);

  _country_multipliers.add_worked(canonical_prefix, static_cast<BAND>(band_nr), static_cast<MODE>(mo));

// exchange mults
  for (auto& exchange_multiplier : _exchange_multipliers)
  { const string& field_name = exchange_multiplier.first;
    multiplier& mult = exchange_multiplier.second;
    const string value = qso.received_exchange(field_name);
    const string mv = MULT_VALUE(field_name, value);            // the mult value of the received field

    if (!value.empty())
      mult.unconditional_add_worked(mv, static_cast<BAND>(band_nr), static_cast<MODE>(mo));
  }
  
  const bool is_dupe = log.is_dupe(qso, rules);

  if (is_dupe)
  { auto& pb = _n_dupes[mode_nr];
    pb[band_nr]++;
  }
  else    // not a dupe; add qso points; this may not be a very clean algorithm; I should be able to do better
  {
// try to calculate the points for this QSO; start with a default value
    const unsigned int points_this_qso = rules.points(qso, _location_db);             // points based on country; something like ::3

    _qso_points[mode_nr][band_nr] += points_this_qso;

// if it's not a dupe, we may need to track whether it's an ON QSO in the UBA contest
    if (rules.uba_bonus())
    { if (rules.bonus_countries() < canonical_prefix)
        _n_ON_qsos[mode_nr][band_nr]++;
    }
  }
}

/*! \brief          Add a known legal value for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \param  value   new legal value for the exchange multiplier <i>name</i>
    \return         whether <i>value</i> was actually added
*/
const bool running_statistics::add_known_exchange_mult(const string& name, const string& value)
{ SAFELOCK(statistics);

//  ost << "adding known legal value of " << value << " to exchange mult: " << name << endl;

  for (auto& psm : _exchange_multipliers)
  { if (psm.first == name)
    { //ost << "MULT_VALUE = " << MULT_VALUE(name, value) << endl;

      const bool added = psm.second.add_known(MULT_VALUE(name, value));

      if (added)
        return true;
    }
  }

  return false;
}

/*! \brief          Return all known legal values for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \return         all the known legal values of <i>name</i>
*/
const set<string> running_statistics::known_exchange_mult_values(const string& name)
{ SAFELOCK(statistics);

  for (const auto& psm : _exchange_multipliers)
  { if (psm.first == name)
      return psm.second.known();
  }

  return set<string>();
}

/*! \brief                  Add a worked exchange mult
    \param  field_name      exchange mult field name
    \param  field_value     value of the field <i>field_name</i>
    \param  band_nr         number of the band on which worked mult is to be added
    \return                 whether the exchange mult was added

    Doesn't add if the value is unknown.
*/
const bool running_statistics::add_worked_exchange_mult(const string& field_name, const string& field_value, const int b, const int m)
{ if (field_value.empty())
    return false;

  const string mv = MULT_VALUE(field_name, field_value);  // the mult value of the received field

  SAFELOCK(statistics);

  for (auto& psm : _exchange_multipliers)
    if (psm.first == field_name)
      return (psm.second.add_worked(mv, static_cast<BAND>(b), static_cast<MODE>(m)));

  return false;
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

/*! \brief                          Do we still need to work a particular exchange mult on a particular band and mode?
    \param  exchange_field_name     name of the target exchange field
    \param  exchange_field_value    value of the target exchange field
    \param  b                       target band
    \param  m                       target mode
    \return                         whether reception of exchange field <i>exchange_field_name</i> with value <i>exchange_field_value</i> on band <i>b</i> and mode <i>m</i> would be a multiplier
*/
const bool running_statistics::is_needed_exchange_mult(const string& exchange_field_name, const string& exchange_field_value, const BAND b, const MODE m) const
{ const string mv = MULT_VALUE(exchange_field_name, exchange_field_value);  // the mult value of the received field

  SAFELOCK(statistics);

  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
  { const pair<string /* field name */, multiplier>& sm = _exchange_multipliers[n];

    if (sm.first == exchange_field_name and sm.second.is_known(mv) )
      return !(sm.second.is_worked(mv, b, m));
  }

  return false;
}

/*! \brief          A complete (multi-line) string that summarizes the statistics, for display in the SUMMARY window
    \param  rules   rules for this contest
    \return         summary string for display in the SUMMARY window
*/
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
  
/*! \brief          Total points
    \param  rules   rules for this contest
    \return         current point total
*/
const unsigned int running_statistics::points(const contest_rules& rules) const
{ unsigned int q_points = 0;
  const vector<BAND>& permitted_bands = rules.permitted_bands();
  const set<BAND>& score_bands = rules.score_bands();
  const set<MODE> score_modes = rules.score_modes();
  
  SAFELOCK(statistics);

  for (const auto& m : score_modes)
  { const auto& qp = _qso_points[m];

    for (const auto& b : score_bands)
      q_points += qp[b];
  }

// callsign mults
  const unsigned int callsign_mults = n_worked_callsign_mults(rules);
  const unsigned int country_mults = n_worked_country_mults(rules);

// exchange mults
  const unsigned int exchange_mults = n_worked_exchange_mults(rules);
  unsigned int rv = q_points * (country_mults + exchange_mults + callsign_mults);
  
// UBA has weird rules
  if (rules.uba_bonus())
  { int bonus_qsos = 0;

    for (const auto& m : score_modes)
    { const auto& qp = _n_ON_qsos[m];

      for (const auto& b : score_bands)
        bonus_qsos += qp[b];
    }

// total QSOs
    int total_qsos = 0;

    for (const auto& m : score_modes)
    { for (const auto& b : score_bands)
      { total_qsos += _n_qsos[m][b];
        total_qsos -= _n_dupes[m][b];
      }
    }

    const float on_fraction = static_cast<float>(bonus_qsos) / total_qsos;
    const unsigned int points_ON_qso = 10;                                      // hardwire the ON points for now
    const unsigned int bonus_points = on_fraction * bonus_qsos * points_ON_qso;

    rv += bonus_points;
  }

  return rv;
}

/*! \brief              Worked callsign mults for a particular band and mode
    \param  mult_name   name of the callsign mult
    \param  b           target band
    \param  m           target mode
    \return             all the worked values of the callsign mult <i>mult_name</i> on band <i>b</i> and mode <i>m</i>
*/
const set<string> running_statistics::worked_callsign_mults(const string& mult_name, const BAND b, const MODE m)
{ SAFELOCK(statistics);

  if (mult_name == string() and _callsign_multipliers.size() == 1)
    return _callsign_multipliers.cbegin()->second.worked(b, m);

  const auto& cit = _callsign_multipliers.find(mult_name);

  if (cit != _callsign_multipliers.cend())
    return cit->second.worked(b, m);

  return set<string>();
}

/*! \brief      Worked country mults for a particular band and mode
    \param  b   target band
    \param  m   target mode
    \return     all the worked country mults on band <i>b</i> and mode <i>m</i>
*/
const set<string> running_statistics::worked_country_mults(const BAND b, const MODE m)
{ SAFELOCK(statistics);

  return ( _country_multipliers.worked(b, m) );
}

/*! \brief      Worked exchange mults for a particular band and mode -- &&& THINK ABOUT THIS
    \param  b   target band
    \param  m   target mode
    \return     all the values of all country mults worked on band <i>b</i> and mode <i>m</i>
*/
const map<string /* field name */, set<string> /* values */ >  running_statistics::worked_exchange_mults(const BAND b, const MODE m) const
{ map<string, set<string> > rv;

  SAFELOCK(statistics);

  for (const auto& psm : _exchange_multipliers)
  { const string& field_name = psm.first;
    const multiplier& mult = psm.second;
    const set<string> mult_values = mult.worked(b, m);

    rv.insert( { field_name, mult_values } );
  }

 return rv;
}

/// clear the dynamic information
void running_statistics::clear_info(void)
{ SAFELOCK(statistics);

  _n_dupes = move(decltype(_n_dupes)( { { } } ));
  _n_qsos = move(decltype(_n_qsos)( { { } } ));
  _qso_points = move(decltype(_qso_points)( { { } } ));

//  for (auto& callsign_m : _callsign_multipliers)
//    callsign_m.second.clear();

  FOR_ALL(_callsign_multipliers, [] (pair<const string, multiplier>& callsign_m) { callsign_m.second.clear(); } );

  _country_multipliers.clear();

//  for (size_t n = 0; n < _exchange_multipliers.size(); ++n)
//  { pair<string, multiplier>& sm = _exchange_multipliers[n];  //  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers
//
//    sm.second.clear();
//  }

  FOR_ALL(_exchange_multipliers, [] (pair<string, multiplier>& sm) { sm.second.clear(); } );
}

/*! \brief      Set the number of sent QTC QSOs
    \param  n   number of sent QTC QSOs
*/
void running_statistics::qtc_qsos_sent(const unsigned int n)
{ SAFELOCK(statistics);

  if (_include_qtcs)
    _qtc_qsos_sent = n;
}

/*! \brief      Set the number of unsent QTC QSOs
    \param  n   number of unsent QTC QSOs
*/
void running_statistics::qtc_qsos_unsent(const unsigned int n)
{ SAFELOCK(statistics);

  if (_include_qtcs)
    _qtc_qsos_unsent = n;
}

/*! \brief          What is the ratio of the value of a new mult to the value of a new (non-mult) QSO?
    \param  rules   rules for this contest
    \param  b       band
    \param  m       mode
    \return         the ratio of the value of a new mult QSO on band <i>b</i> and mode <i>m</i> to the value of a new non-mult QSO on the same band and mode
*/

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

/*! \brief          How many QSOs have been made?
    \param  rules   rules for this contest

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

/*! \brief          How many QSOs have been made in a particular mode?
    \param  rules   rules for this contest
    \param  m       target mode

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

/*! \brief          Get the number of worked callsign mults
    \param  rules   rules for this contest
    \return         the number of callsign mults worked
*/
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

/*! \brief          Get the number of worked country mults
    \param  rules   rules for this contest
    \return         the number of country mults worked
*/
const unsigned int running_statistics::n_worked_country_mults(const contest_rules& rules) const
{ const set<BAND>& score_bands = rules.score_bands();
  const map<BAND, int>& per_band_country_mult_factor = rules.per_band_country_mult_factor();
  unsigned int rv = 0;

  SAFELOCK(statistics);

  FOR_ALL(score_bands, [&] (const BAND& b) { rv += (_country_multipliers.n_worked(b) * per_band_country_mult_factor.at(b)); } );

  return rv;
}

/*! \brief          Get the number of worked exchange mults
    \param  rules   rules for this contest
    \return         the number of exchange mults worked
*/
const unsigned int running_statistics::n_worked_exchange_mults(const contest_rules& rules) const
{ const vector<BAND> permitted_bands = rules.permitted_bands();
  const set<MODE> permitted_modes = rules.permitted_modes();
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
    { if (mult.per_band())
      { for (const auto& b : permitted_bands)
          rv += mult.n_worked(b, ANY_MODE);
      }
      else
        rv += mult.n_worked(ANY_BAND, ANY_MODE);
    }
  }

  return rv;
}


/*! \brief      Get the number of exchange mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the number of exchange mults worked on band <i>b</i> and mode <i>m</i>
*/
const unsigned int running_statistics::n_worked_exchange_mults(const BAND b, const MODE m) const
{ const map<string, set<string> > worked = worked_exchange_mults(b, m);
  unsigned int rv = 0;

  for (const auto& psss : worked)
    rv += psss.second.size();

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

/*! \brief          Add a QSO to the history
    \param  qso     QSO to add
*/
void call_history::operator+=(const QSO& qso)
{ SAFELOCK(_history);

  _history[qso.callsign()].insert( { qso.band(), qso.mode() } );
}

/*! \brief      Has a call been worked on a particular band and mode?
    \param  s   callsign to test
    \param  b   band to test
    \param  m   mode to test
    \return     whether <i>s</i> has been worked on band <i>b</i> and mode <i>m</i>
*/
const bool call_history::worked(const string& s, const BAND b , const MODE m)
{ SAFELOCK(_history);

  const auto& cit = _history.find(s);

  if (cit == _history.cend())
    return false;

  const set<bandmode>& sbm = cit->second;
  const set<bandmode>::const_iterator scit = sbm.find( { b, m } );

  return (scit != sbm.end());
}

/*! \brief      Has a call been worked on a particular band?
    \param  s   callsign to test
    \param  b   band to test
    \return     whether <i>s</i> has been worked on band <i>b</i>
*/
const bool call_history::worked(const string& s, const BAND b)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { const string& call = pssbm.first;

    if (s == call)
    { for (const auto& bm : pssbm.second)
      { if (bm.first == b)
          return true;
      }
    }
  }

  return false;
}

/*! \brief      Has a call been worked on a particular mode?
    \param  s   callsign to test
    \param  m   mode to test
    \return     whether <i>s</i> has been worked on mode <i>m</i>
*/
const bool call_history::worked(const string& s, const MODE m)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { const string& call = pssbm.first;

    if (s == call)
    { for (const auto& bm : pssbm.second)
      { if (bm.second == m)
          return true;
      }
    }
  }

  return false;
}

/*! \brief      Has a call been worked?
    \param  s   callsign to test
    \return     whether <i>s</i> has been worked
*/
const bool call_history::worked(const string& s)
{ SAFELOCK(_history);
  return (_history.find(s) != _history.end());
}

/*! \brief      Has a call been worked on any other band?
    \param  s   callsign to test
    \param  b   band NOT to test
    \return     whether <i>s</i> has been worked on a band other than <i>b</i>
*/
const bool call_history::worked_on_another_band(const string& s, const BAND b)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { const string& call = pssbm.first;

    if (s == call)
    { for (const auto& bm : pssbm.second)
      { if (bm.first != b)
          return true;
      }
    }
  }

  return false;
}

/*! \brief      Has a call been worked on any other mode?
    \param  s   callsign to test
    \param  m   mode NOT to test
    \return     whether <i>s</i> has been worked on a mode other than <i>m</i>
*/
const bool call_history::worked_on_another_mode(const string& s, const MODE m)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { const string& call = pssbm.first;

    if (s == call)
    { for (const auto& bm : pssbm.second)
      { if (bm.second != m)
          return true;
      }
    }
  }

  return false;
}

/*! \brief      Has a call been worked on any other band and mode combination?
    \param  s   callsign to test
    \param  b   band not to include
    \param  m   mode not to include
    \return     whether <i>s</i> has been worked on a band and mode other than <i>b</i> and <i>m</i>
*/
const bool call_history::worked_on_another_band_and_mode(const std::string& s, const BAND b, const MODE m)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { const string& call = pssbm.first;

    if (s == call)
    { for (const auto& bm : pssbm.second)
      { if ( (bm.first != b) and (bm.second != m) )
          return true;
      }
    }
  }

  return false;
}

/// clear the history
void call_history::clear(void)
{ SAFELOCK(_history);
  _history.clear();
}
