// $Id: statistics.cpp 202 2022-03-07 21:01:02Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   statistics.cpp

    Classes and functions related to the statistics of an ongoing contest
*/

#include "bands-modes.h"
#include "log_message.h"
#include "rules.h"
#include "statistics.h"
#include "string_functions.h"

#include <algorithm>
#include <fstream>
#include <iostream>

using namespace std;

extern message_stream ost;      ///< for debugging and logging

extern bool scoring_enabled;

pt_mutex statistics_mutex { "STATISTICS"s };      ///< mutex for the (singleton) running_statistics object

constexpr unsigned int FIRST_FIELD_WIDTH  { 10 };   ///< width of first field
constexpr unsigned int FIELD_WIDTH        { 6 };    ///< width of other fields

// -----------  running_statistics  ----------------

/*! \class  running_statistics
    \brief  ongoing contest-related statistics
*/

/*! \brief              Add a callsign mult name, value and band to those worked
    \param  mult_name   name of callsign mult
    \param  mult_value  value of callsign mult
    \param  band_nr     band on which callsign mult was worked
    \param  mode_nr     mode on which callsign mult was worked

    <i>band_nr</i> = ALL_BANDS means add to *only* the global accumulator; otherwise add to a band AND to the global accumulator
    The information is inserted into the <i>_callsign_multipliers</i> object.
*/
void running_statistics::_insert_callsign_mult(const string& mult_name, const string& mult_value, const unsigned int band_nr, const unsigned int mode_nr)
{ if (_callsign_mults_used and !mult_value.empty())     // do we actually have to do anything?
  { SAFELOCK(statistics);

    if (known_callsign_mult_name(mult_name))            // do we already know about this mult name?
    { multiplier& mult { (_callsign_multipliers.find(mult_name))->second };

      mult.add_worked(mult_value, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr));                // add value and band for this mult name
    }
    else                                                   // unknown mult name
    { multiplier mult;                                     // create new mult

      mult.add_worked(mult_value, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr));                // we've worked it
      _callsign_multipliers += { mult_name, mult }; // store the info
    }
  }
}

/*! \brief          Generate a summary string for display
    \param  rules   rules for this contest
    \param  modes   the set of modes that are to be included in the summary string
    \return         summary string for modes in <i>modes</i>
*/
string running_statistics::_summary_string(const contest_rules& rules, const set<MODE>& modes)
{ const set<MODE>    permitted_modes { rules.permitted_modes() };
  const vector<BAND> permitted_bands { rules.permitted_bands() };

  string rv;

  { SAFELOCK(statistics);

// QSOs
    unsigned int qsos_all_bands { 0 };

    string line { pad_right("QSOs"s, FIRST_FIELD_WIDTH) };                                          // accumulator for total qsos

    auto add_all_bands = [&line] (const unsigned int sz, const unsigned int n) { if (sz != 1)
                                                                                   line += pad_left(to_string(n), FIELD_WIDTH);
                                                                               };

    for (const auto& b : permitted_bands)
    { unsigned int qsos { 0 };

      for (const auto& m : modes)
      { const auto& nq { _n_qsos[m] };

        if (modes.size() == 1)
          line += pad_left(to_string(nq[b]), FIELD_WIDTH);

        qsos += nq[b];
      }

      qsos_all_bands += qsos;

      if (modes.size() != 1)
        line += pad_left(to_string(qsos), FIELD_WIDTH);
    }

    add_all_bands(permitted_bands.size(), qsos_all_bands);
    rv += (line + LF);

// country mults
    if (_country_multipliers.used())                                         // if countries are mults
    { unsigned int total_countries_all_bands { 0 };

      line = pad_right("Countries"s, FIRST_FIELD_WIDTH);

      for (const auto& b : permitted_bands)
      { unsigned int countries { 0 };

        for (const auto& m : modes)
        { const auto n_countries { _country_multipliers.n_worked(b, m) };

          if (modes.size() == 1)
            line += pad_left(to_string(n_countries), FIELD_WIDTH);

          countries += n_countries;
        }

        total_countries_all_bands += countries;

        if (modes.size() != 1)
          line += pad_left(to_string(countries), FIELD_WIDTH);
      }

      add_all_bands(permitted_bands.size(), total_countries_all_bands);
      rv += (line + LF);
    }

// callsign mults
    const set<string>&  callsign_mults { rules.callsign_mults() };      // collection of types of mults based on callsign (e.g., "WPXPX")

    if (!callsign_mults.empty())
    { for (const auto mult_name : callsign_mults)
      { const MODE m { ((modes.size() == 1) ? *(modes.cbegin()) : ANY_MODE) };

        line = pad_right(mult_name, FIRST_FIELD_WIDTH);

 //       const auto& cit { _callsign_multipliers.find(mult_name) };

        if (const auto& cit { _callsign_multipliers.find(mult_name) }; cit != _callsign_multipliers.end())    // should always be true
        { const multiplier& mult { cit->second };

          FOR_ALL(permitted_bands, [=, &line] (const BAND& b) { line += pad_left(to_string(mult.n_worked(b, m)), FIELD_WIDTH); } );

          if (permitted_bands.size() != 1)
            line += pad_left(to_string(mult.n_worked(ANY_BAND, m)), FIELD_WIDTH);
        }
        else
        { ost << "Error: could not find mult name: " << mult_name << endl;
          ost << "Number of callsign multipliers in statistics = " << _callsign_multipliers.size() << endl;

          for (const auto& mult : _callsign_multipliers)
            ost << mult.first << endl;
        }
      }

      rv += line + LF;
    }

// Exchange mults
    const bool exchange_mults_per_band { rules.exchange_mults_per_band() };

    for (const auto& sm : _exchange_multipliers)
    { const string& field_name { sm.first };

      line = pad_right(field_name, FIRST_FIELD_WIDTH);

      const multiplier& mult { sm.second };

      unsigned int total { 0 };

      const MODE m { ((modes.size() == 1) ? *(modes.cbegin()) : ANY_MODE) };

      for (const auto& b : permitted_bands)
      { const auto n_exchange_mults { mult.n_worked(b, m) };

        line += pad_left(to_string(n_exchange_mults), FIELD_WIDTH);

        if (exchange_mults_per_band)
          total += n_exchange_mults;
      }

      if (permitted_bands.size() != 1)
      { if (exchange_mults_per_band)
          add_all_bands(permitted_bands.size(), total);
        else
          line += pad_left(to_string(mult.n_worked(ANY_BAND, m)), FIELD_WIDTH);
      }

      rv += line + LF;
    }

// dupes
    unsigned int dupes_all_bands { 0 };

    line = pad_right("Dupes"s, FIRST_FIELD_WIDTH);

    for (const auto& b : permitted_bands)
    { unsigned int dupes { 0 };

      for (const auto& m : modes)
      { const auto& nd { _n_dupes[m] };

        if (modes.size() == 1)
          line += pad_left(to_string(nd[b]), FIELD_WIDTH);

        dupes += nd[b];
      }

      dupes_all_bands += dupes;

      if (modes.size() != 1)
        line += pad_left(to_string(dupes), FIELD_WIDTH);
    }

    add_all_bands(permitted_bands.size(), dupes_all_bands);
    rv += (line + LF);

// QSO points
    if (scoring_enabled)
    { unsigned int points_all_bands { 0 };

      line = pad_right("Qpoints"s, FIRST_FIELD_WIDTH);

      for (const auto& b : permitted_bands)
      { unsigned int points { 0 };

        for (const auto& m : modes)
        { const auto& qp { _qso_points[m] };

          if (modes.size() == 1)
            line += pad_left(to_string(qp[b]), FIELD_WIDTH);

          points += qp[b];
        }

        points_all_bands += points;

        if (modes.size() != 1)
          line += pad_left(to_string(points), FIELD_WIDTH);
      }

      add_all_bands(permitted_bands.size(), points_all_bands);
      rv += line;
    }

    if (_include_qtcs)
    { line = LF + pad_right("QTC QSOs"s, FIRST_FIELD_WIDTH);
      line += pad_left((to_string(_qtc_qsos_sent) + '|' + to_string(_qtc_qsos_unsent)), FIELD_WIDTH * (permitted_bands.size() + 1));

      rv += line;
    }
  }

  return rv;
}

/*! \brief                  Prepare an object that was created with the default constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
void running_statistics::prepare(const cty_data& country_data, const drlog_context& context, const contest_rules& rules)
{ SAFELOCK(statistics);

  _callsign_mults_used = rules.callsign_mults_used();
  _country_mults_used = rules.country_mults_used();
  _auto_country_mults = context.auto_remaining_country_mults();
  _exchange_mults_used = rules.exchange_mults_used();
  _include_qtcs = rules.send_qtcs();

  _location_db.prepare(country_data, context.country_list());

  const vector<string>& exchange_mults { rules.exchange_mults() };

  _exch_mult_fields += exchange_mults;

// callsign mults
  if (_callsign_mults_used)
  { const set<string> callsign_mult_names { rules.callsign_mults() };

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
  { const unordered_set<string> country_mults { rules.country_mults() };

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
      const vector<string> canonical_values { rules.exch_canonical_values(exchange_mult_name) };

      if (!canonical_values.empty())
        em.add_known(canonical_values);

      _exchange_multipliers += { exchange_mult_name, em };
    }
  }
}

/*! \brief                                  Is a particular string a known callsign mult name?
    \param  putative_callsign_mult_name     string to test
    \return                                 whether <i>putative_callsign_mult_name</i> is a known callsign mult name
*/
bool running_statistics::known_callsign_mult_name(const string& putative_callsign_mult_name) const
{ SAFELOCK(statistics);

//  return ( _callsign_multipliers.find(putative_callsign_mult_name) != _callsign_multipliers.cend() );
  return contains(_callsign_multipliers, putative_callsign_mult_name);
}

/*! \brief              Do we still need to work a particular callsign mult on a particular band and mode?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
    \param  m           mode to test
    \return             whether the callsign mult <i>mult_name</i> with the value <i>mult_value</i> is still needed on band <i>b</i> and mode <i>m</i>
*/
bool running_statistics::is_needed_callsign_mult(const string& mult_name, const string& mult_value, const BAND b, const MODE m) const
{ SAFELOCK(statistics);

  if (!known_callsign_mult_name(mult_name))
    return false;

//  const auto cit { _callsign_multipliers.find(mult_name) };

  if (const auto cit { _callsign_multipliers.find(mult_name) }; cit != _callsign_multipliers.cend())                          // should always be true
  { const multiplier& mult   { cit->second };
//    const bool        worked { mult.is_worked(mult_value, b, m) };

//    return !(worked);
    return !(mult.is_worked(mult_value, b, m));
  }

  ost << "ERROR: Unknown multiplier name: " << mult_name << endl;
  return false;
}

/*! \brief              Do we still need to work a particular country as a mult on a particular band and mode?
    \param  callsign    call to test
    \param  b           band to test
    \param  m           mode to test
    \param  rules       rules for this contest
    \return             whether the country in which <i>callsign</i> is located is needed as a mult on band <i>b</i> and mode <i>m</i>
*/
bool running_statistics::is_needed_country_mult(const string& callsign, const BAND b, const MODE m, const contest_rules& rules)
{ try
  { SAFELOCK(statistics);

    const string canonical_prefix { _location_db.canonical_prefix(callsign) };

    if (!rules.is_country_mult(canonical_prefix))  // only determine whether actually a country mult if the country is a mult in the contest
      return false;

    bool is_needed_mult;

    if (_auto_country_mults)
      is_needed_mult = !(_country_multipliers.is_worked(canonical_prefix, b, m));       // we should count the mult even if it hasn't been seen enough times to be known yet
    else
    { if (const bool prefix_is_a_mult { _country_multipliers.is_known(canonical_prefix) }; prefix_is_a_mult)
        is_needed_mult = !(_country_multipliers.is_worked(canonical_prefix, b, m));
      else                                                                              // not a mult
        is_needed_mult = false;
    }

    return is_needed_mult;
  }
  
  catch (const location_error& e)
  { return false;
  }
}

/*! \brief          Add a known value of country mult
    \param  str     canonical prefix of mult
    \param  rules   rules for this contest
    \return         whether <i>str</i> was actually added

    Does nothing and returns <i>false</i> if <i>str</i> is already known
*/
bool running_statistics::add_known_country_mult(const string& str, const contest_rules& rules)
{ SAFELOCK(statistics);

  return ( (rules.country_mults() > str) ? _country_multipliers.add_known(str) : false );
}

/*! \brief          Add a QSO to the ongoing statistics
    \param  qso     QSO to add
    \param  log     logbook (without the qso <i>qso</i>)
    \param  rules   contest rules
*/
void running_statistics::add_qso(const QSO& qso, const logbook& log, const contest_rules& rules)
{ SAFELOCK(statistics);
  
  const BAND&        b       { qso.band() };
  const unsigned int band_nr { static_cast<unsigned int>(b) };
  const MODE&        mo      { qso.mode() };
  const unsigned int mode_nr { static_cast<unsigned int>(mo) };

// increment the number of QSOs
  auto& pb { _n_qsos[mode_nr] };

  pb[band_nr]++;

// multipliers

// callsign mults
// for now, just assume that there's at most one possible callsign mult, and the value is in qso.prefix()
  if (!_callsign_multipliers.empty() and !(qso.prefix().empty()) )
  { const string mult_name { _callsign_multipliers.begin()->first };

    multiplier& m { _callsign_multipliers.begin()->second };

    m.unconditional_add_worked(qso.prefix(), static_cast<BAND>(band_nr), static_cast<MODE>(mo));

    _callsign_multipliers[mult_name] = m;
  }

// country mults
  const string& call             { qso.callsign() };
  const string& canonical_prefix { _location_db.canonical_prefix(call) };

  _country_multipliers.add_worked(canonical_prefix, static_cast<BAND>(band_nr), static_cast<MODE>(mo));

// exchange mults
  for (auto& exchange_multiplier : _exchange_multipliers)
  { const string& field_name { exchange_multiplier.first };
    const string  value      { qso.received_exchange(field_name) };
    const string  mv         { MULT_VALUE(field_name, value) };            // the mult value of the received field

    if (!value.empty())
    { multiplier& mult { exchange_multiplier.second };

      mult.unconditional_add_worked(mv, static_cast<BAND>(band_nr), static_cast<MODE>(mo));
    }
  }
  
  const bool is_dupe { log.is_dupe(qso, rules) };

  if (is_dupe)
  { auto& pb { _n_dupes[mode_nr] };

    pb[band_nr]++;
  }
  else    // not a dupe; add qso points; this may not be a very clean algorithm; I should be able to do better
  {
// try to calculate the points for this QSO; start with a default value
    const unsigned int points_this_qso { rules.points(qso, _location_db) };             // points based on country; something like :G:3

    _qso_points[mode_nr][band_nr] += points_this_qso;

// if it's not a dupe, we may need to track whether it's an ON QSO in the UBA contest
    if (rules.uba_bonus())
    { if (rules.bonus_countries() > canonical_prefix)
        _n_ON_qsos[mode_nr][band_nr]++;
    }
  }
}

/*! \brief          Add a known legal value for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \param  value   new legal value for the exchange multiplier <i>name</i>
    \return         whether <i>value</i> was actually added
*/
bool running_statistics::add_known_exchange_mult(const string& name, const string& value)
{ SAFELOCK(statistics);

  for (auto& psm : _exchange_multipliers)
  { if (psm.first == name)
    { if (psm.second.add_known(MULT_VALUE(name, value)))
        return true;
    }
  }

  return false;
}

/*! \brief          Return all known legal values for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \return         all the known legal values of <i>name</i>
*/
MULTIPLIER_VALUES running_statistics::known_exchange_mult_values(const string& name)
{ SAFELOCK(statistics);

  for (const auto& psm : _exchange_multipliers)
  { if (psm.first == name)
      return psm.second.known();
  }

  return MULTIPLIER_VALUES();
}

/*! \brief                  Add a worked exchange mult
    \param  field_name      exchange mult field name
    \param  field_value     value of the field <i>field_name</i>
    \param  band_nr         number of the band on which worked mult is to be added
    \param  mode_nr         number of the mode on which worked mult is to be added
    \return                 whether the exchange mult was added

    Doesn't add if the value <i>field_value</i> is unknown.
*/
bool running_statistics::add_worked_exchange_mult(const string& field_name, const string& field_value, const int band_nr, const int mode_nr)
{ if (!field_value.empty())
  { const string mv { MULT_VALUE(field_name, field_value) };  // the mult value of the received field

    SAFELOCK(statistics);

    for (auto& psm : _exchange_multipliers)
    { if (psm.first == field_name)
        return ( psm.second.add_worked(mv, static_cast<BAND>(band_nr), static_cast<MODE>(mode_nr)) );
    }
  }

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
bool running_statistics::is_needed_exchange_mult(const string& exchange_field_name, const string& exchange_field_value, const BAND b, const MODE m) const
{ const string mv { MULT_VALUE(exchange_field_name, exchange_field_value) };  // the mult value of the received field

  SAFELOCK(statistics);

  for (const auto& psm : _exchange_multipliers)
  { if (psm.first == exchange_field_name and psm.second.is_known(mv) )
      return !(psm.second.is_worked(mv, b, m));
  }

  return false;
}

/*! \brief          A complete (multi-line) string that summarizes the statistics, for display in the SUMMARY window
    \param  rules   rules for this contest
    \return         summary string for display in the SUMMARY window
*/
string running_statistics::summary_string(const contest_rules& rules)
{ string rv;

// write the bands and underline them
  string line(FIRST_FIELD_WIDTH, ' ');

  const vector<BAND>& permitted_bands { rules.permitted_bands() };

  FOR_ALL(permitted_bands, [&line] (const BAND b) { line += pad_left(BAND_NAME[b], FIELD_WIDTH); } );

  if (permitted_bands.size() != 1)
    line += pad_left("All"s, FIELD_WIDTH);
  
  rv += (line + LF);
      
  line = string(FIRST_FIELD_WIDTH, ' ');

  FOR_ALL(permitted_bands, [&line] (const BAND) { line += pad_left("---"s, FIELD_WIDTH); } );

  if (permitted_bands.size() != 1)
    line += pad_left("---"s, FIELD_WIDTH);

  rv += (line + LF);

// now create the individual per-mode tables
  const set<MODE> sm { rules.permitted_modes() };

  vector<set<MODE>> vsm;

  for (const auto& m : sm)
    vsm += set<MODE> { m };

  if (vsm.size() > 1)
    vsm += sm;

  for (const auto& mode_set : vsm)
  { if (vsm.size() != 1)
      rv += ( ( (mode_set.size() == 1) ? MODE_NAME[*(mode_set.cbegin())] : "All"s ) + LF );
  
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
unsigned int running_statistics::points(const contest_rules& rules) const
{ unsigned int q_points { 0 };

  const set<BAND>& score_bands { rules.score_bands() };
  const set<MODE>  score_modes { rules.score_modes() };
  
  SAFELOCK(statistics);

  for (const auto& m : score_modes)
  { const auto& qp { _qso_points[m] };

    for (const auto& b : score_bands)
      q_points += qp[b];
  }

// QTC points
  q_points += _qtc_qsos_sent;

// mults
  const unsigned int callsign_mults { n_worked_callsign_mults(rules) };
  const unsigned int country_mults  { n_worked_country_mults(rules) };
  const unsigned int exchange_mults { n_worked_exchange_mults(rules) };

  unsigned int rv { q_points * (max((country_mults + exchange_mults + callsign_mults), static_cast<unsigned int>(1))) };    // force mult to be at least unity
  
// UBA has weird rules
  if (rules.uba_bonus())
  { constexpr unsigned int ON_QSO_POINTS { 10 };  // hardwire the ON points for now

    int bonus_qsos { 0 };

    for (const auto& m : score_modes)
    { const auto& qp { _n_ON_qsos[m] };

      for (const auto& b : score_bands)
        bonus_qsos += qp[b];
    }

// total QSOs
    int total_qsos { 0 };

    for (const auto& m : score_modes)
    { for (const auto& b : score_bands)
      { total_qsos += _n_qsos[m][b];
        total_qsos -= _n_dupes[m][b];
      }
    }

    const float        on_fraction  { static_cast<float>(bonus_qsos) / total_qsos };
    const unsigned int bonus_points { static_cast<unsigned int>(on_fraction * bonus_qsos * ON_QSO_POINTS) };

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
MULTIPLIER_VALUES running_statistics::worked_callsign_mults(const string& mult_name, const BAND b, const MODE m)
{ SAFELOCK(statistics);

//  if (mult_name == string() and _callsign_multipliers.size() == 1)
  if (mult_name.empty() and _callsign_multipliers.size() == 1)
    return _callsign_multipliers.cbegin()->second.worked(b, m);

  const auto& cit { _callsign_multipliers.find(mult_name) };

  return ( (cit == _callsign_multipliers.cend()) ? MULTIPLIER_VALUES() : cit->second.worked(b, m) );
}

/*! \brief      Worked exchange mults for a particular band and mode -- &&& THINK ABOUT THIS
    \param  b   target band
    \param  m   target mode
    \return     all the values of all country mults worked on band <i>b</i> and mode <i>m</i>
*/
map<string /* field name */, MULTIPLIER_VALUES /* values */ > running_statistics::worked_exchange_mults(const BAND b, const MODE m) const
{ map<string, MULTIPLIER_VALUES> rv;

  SAFELOCK(statistics);

  for (const auto& psm : _exchange_multipliers)
  { //const multiplier& mult { psm.second };

    //rv += { psm.first, mult.worked(b, m) };
    rv += { psm.first, psm.second.worked(b, m) };
  }

 return rv;
}

/// clear the dynamic information
void running_statistics::clear_info(void)
{ SAFELOCK(statistics);

  _n_dupes = move(decltype(_n_dupes)( { { } } ));
  _n_qsos = move(decltype(_n_qsos)( { { } } ));
  _qso_points = move(decltype(_qso_points)( { { } } ));

// clear the mults
  FOR_ALL(_callsign_multipliers, [] (pair<const string, multiplier>& callsign_m) { callsign_m.second.clear(); } );
  _country_multipliers.clear();
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

    There are many ways to come up with a plausible definition for the value of a mult
*/
float running_statistics::mult_to_qso_value(const contest_rules& rules, const BAND b, const MODE m) const
{ const unsigned int current_mults { n_worked_callsign_mults(rules) + n_worked_country_mults(rules) + n_worked_exchange_mults(rules) };
  const set<BAND>    score_bands   { rules.score_bands() };
  const set<MODE>    score_modes   { rules.score_modes() };
  const unsigned int current_qsos  { n_qsos(rules) };

  unsigned int current_qso_points { 0 };

  SAFELOCK(statistics);

// current QSO points
  for (const auto& m : score_modes)
  { const auto& qp { _qso_points[m] };

    FOR_ALL(score_bands, [&] (const BAND& b) { current_qso_points += qp[static_cast<int>(b)]; } );
  }

  const float        current_mean_qso_points { static_cast<float>(current_qso_points) / current_qsos };
  const unsigned int current_points          { current_qso_points * current_mults };                    // should be same as points(rules)

// add a notional mult
  unsigned int notional_mults { current_mults };

  const map<BAND, int>& per_band_country_mult_factor { rules.per_band_country_mult_factor() };

// include the correct factor for a mult on the current band
  notional_mults += ( _country_multipliers.used() ? per_band_country_mult_factor.at(b) : 1 );

// add a notional QSO; first calculate average qso point worth of a qso on the current band and mode
  const unsigned int& n_qso_points { _qso_points[m][b] };
  const unsigned int& n_qsos       { _n_qsos[m][b] };

  const float        mean_qso_points_band_mode { (n_qsos ? ( static_cast<float>(n_qso_points) / n_qsos ) : 0) };
  const float        new_point_average         { ( (current_qsos * current_mean_qso_points) + mean_qso_points_band_mode ) / (current_qsos + 1) };
  const unsigned int score_with_new_qso        { static_cast<unsigned int>((new_point_average * (current_qsos + 1) ) * current_mults) };
  const unsigned int score_with_new_mult       { static_cast<unsigned int>((new_point_average * (current_qsos + 1) ) * notional_mults) };
  const unsigned int new_qso_value             { score_with_new_qso - current_points };
  const unsigned int new_mult_value            { score_with_new_mult - current_points };

  return ( static_cast<float>(new_mult_value) / new_qso_value );
}

/*! \brief          How many QSOs have been made?
    \param  rules   rules for this contest

    Counts only those QSOs on bands and modes being used to calculate the score. Includes dupes.
*/
unsigned int running_statistics::n_qsos(const contest_rules& rules) const
{ const set<BAND>& score_bands { rules.score_bands() };
  const set<MODE>& score_modes { rules.score_modes() };

  unsigned int rv { 0 };

  SAFELOCK(statistics);

  for (const auto& m : score_modes)
  { const auto& nq { _n_qsos[m] };

    FOR_ALL(score_bands, [&] (const BAND& b) { rv += nq[static_cast<int>(b)]; } );
  }

  return rv;
}

/*! \brief          How many QSOs have been made in a particular mode?
    \param  rules   rules for this contest
    \param  m       target mode

    Counts only those QSOs on bands being used to calculate the score. Includes dupes.
*/
unsigned int running_statistics::n_qsos(const contest_rules& rules, const MODE m) const
{ const set<BAND>& score_bands { rules.score_bands() };
  const auto&      nq          { _n_qsos[m] };

  unsigned int rv { 0 };

  SAFELOCK(statistics);

  FOR_ALL(score_bands, [&] (const BAND& b) { rv += nq[static_cast<int>(b)]; } );

  return rv;
}

/*! \brief          Get the number of worked callsign mults
    \param  rules   rules for this contest
    \return         the number of callsign mults worked
*/
unsigned int running_statistics::n_worked_callsign_mults(const contest_rules& rules) const
{ const vector<BAND>& permitted_bands { rules.permitted_bands() };
  const set<BAND>&    score_bands     { rules.score_bands() };

  unsigned int rv { 0 };

  SAFELOCK(statistics);

  for (const auto& sm : _callsign_multipliers)
  { if (const multiplier& mult { sm.second }; mult.per_band())
    { //for (const auto& b : permitted_bands)
      //  if (score_bands > b)
      //    rv += mult.n_worked(b);
      FOR_ALL(permitted_bands, [=, &rv] (const auto& b) { if (score_bands > b) rv += mult.n_worked(b); });
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
unsigned int running_statistics::n_worked_country_mults(const contest_rules& rules) const
{ const set<BAND>      score_bands                  { rules.score_bands() };
  const map<BAND, int> per_band_country_mult_factor { rules.per_band_country_mult_factor() };

  unsigned int rv { 0 };

  SAFELOCK(statistics);

  FOR_ALL(score_bands, [&] (const BAND& b) { rv += (_country_multipliers.n_worked(b) * per_band_country_mult_factor.at(b)); } );

  return rv;
}

/*! \brief          Get the number of worked exchange mults
    \param  rules   rules for this contest
    \return         the number of exchange mults worked
*/
unsigned int running_statistics::n_worked_exchange_mults(const contest_rules& rules) const
{ const vector<BAND> permitted_bands { rules.permitted_bands() };
  const set<MODE>    permitted_modes { rules.permitted_modes() };

  unsigned int rv { 0 };

  SAFELOCK(statistics);

  for (const auto& em : _exchange_multipliers)
  { if (const multiplier& mult { em.second }; mult.per_mode())
    { for (const auto& m : permitted_modes)
      { if (mult.per_band())
        { //for (const auto& b : permitted_bands)
          //  rv += mult.n_worked(b, m);
          FOR_ALL(permitted_bands, [=, &rv] (const auto& b) { rv += mult.n_worked(b, m); });
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
unsigned int running_statistics::n_worked_exchange_mults(const BAND b, const MODE m) const
{ const map<string, MULTIPLIER_VALUES> worked { worked_exchange_mults(b, m) };

  unsigned int rv { 0 };

  for (const auto& psss : worked)
    rv += psss.second.size();

  return rv;
}

// -----------  call history  ----------------

/*! \class  call_history
    \brief  History of each call worked
*/

void call_history::rebuild(const logbook& logbk)
{ SAFELOCK(_history);

  _history.clear();

  FOR_ALL(logbk.as_vector(), [&] (const QSO& qso) { *this += qso; } );
}

/*! \brief          Add a QSO to the history
    \param  qso     QSO to add
*/
void call_history::operator+=(const QSO& qso)
{ SAFELOCK(_history);

//  _history[qso.callsign()].insert( { qso.band(), qso.mode() } );
  _history[qso.callsign()] += { qso.band(), qso.mode() };
}

/*! \brief      Has a call been worked on a particular band and mode?
    \param  s   callsign to test
    \param  b   band to test
    \param  m   mode to test
    \return     whether <i>s</i> has been worked on band <i>b</i> and mode <i>m</i>
*/
bool call_history::worked(const string& s, const BAND b , const MODE m)
{ SAFELOCK(_history);

  const auto& cit { _history.find(s) };

  if (cit == _history.cend())
    return false;

  const set<bandmode>&                sbm  { cit->second };
  const set<bandmode>::const_iterator scit { sbm.find( { b, m } ) };

  return (scit != sbm.cend());
}

/*! \brief      Has a call been worked on a particular band?
    \param  s   callsign to test
    \param  b   band to test
    \return     whether <i>s</i> has been worked on band <i>b</i>
*/
bool call_history::worked(const string& s, const BAND b)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { if (const string& call { pssbm.first }; s == call)
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
bool call_history::worked(const string& s, const MODE m)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { if (const string& call { pssbm.first }; s == call)
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
bool call_history::worked(const string& s)
{ SAFELOCK(_history);

  return (_history.find(s) != _history.end());
}

/*! \brief      Has a call been worked on any other band?
    \param  s   callsign to test
    \param  b   band NOT to test
    \return     whether <i>s</i> has been worked on a band other than <i>b</i>
*/
bool call_history::worked_on_another_band(const string& s, const BAND b)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { if (const string& call { pssbm.first }; s == call)
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
bool call_history::worked_on_another_mode(const string& s, const MODE m)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { //const string& call { pssbm.first };

    if (const string& call { pssbm.first }; s == call)
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
bool call_history::worked_on_another_band_and_mode(const string& s, const BAND b, const MODE m)
{ SAFELOCK(_history);

  for (const auto& pssbm : _history)
  { //const string& call { pssbm.first };

    if (const string& call { pssbm.first }; s == call)
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
