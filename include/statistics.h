// $Id: statistics.h 149 2019-01-03 19:24:01Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef STATISTICS_H
#define STATISTICS_H

/*! \file   statistics.h

    Classes and functions related to the statistics of an ongoing contest
*/

#include "cty_data.h"
#include "drlog_context.h"
#include "log.h"
#include "multiplier.h"
#include "pthread_support.h"
#include "qso.h"
#include "rules.h"
#include "serialization.h"

#include <array>
#include <map>
#include <set>
#include <string>

extern pt_mutex statistics_mutex;       ///< mutex for statistics

// -----------  running_statistics  ----------------

/*! \class  running_statistics
    \brief  ongoing contest-related statistics
*/

class running_statistics
{
protected:
  
  std::map<std::string /* mult name */, multiplier>                 _callsign_multipliers;  ///< callsign multipliers (supports more than one)
  bool                                                              _callsign_mults_used;   ///< are callsign mults used? Copied from rules

  multiplier                                                        _country_multipliers;   ///< country multipliers
  bool                                                              _country_mults_used;    ///< are country mults used? Copied from rules
  bool                                                              _auto_country_mults;    ///< can the list of country multipliers change?

  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  ///< exchange multipliers; vector so we can keep the correct order
  bool                                                              _exchange_mults_used;   ///< are country mults used? Copied from rules
  std::set<std::string>                                             _exch_mult_fields;      ///< names of the exch fields that are mults

  bool                                                              _include_qtcs;          ///< do we include QTC information?

  location_database                                                 _location_db;           ///< database for location-based lookups

  std::array<std::array<unsigned int, N_BANDS>, N_MODES>            _n_dupes;               ///< number of dupes, per band and mode
  std::array<std::array<unsigned int, N_BANDS>, N_MODES>            _n_qsos;                ///< number of QSOs, per band and mode
  std::array<std::array<unsigned int, N_BANDS>, N_MODES>            _n_ON_qsos;             ///< number of ON QSOs, per band and mode -- for UBA
  std::array<std::array<unsigned int, N_BANDS>, N_MODES>            _qso_points;            ///< number of QSO points, per band and mode

  unsigned int                                                      _qtc_qsos_sent;         ///< total number of QSOs sent in QTCs
  unsigned int                                                      _qtc_qsos_unsent;       ///< total number of (legal) QSOs available but not yet sent in QTCs

/*! \brief              Add a callsign mult name, value and band to those worked
    \param  mult_name   name of callsign mult
    \param  mult_value  value of callsign mult
    \param  band_nr     band on which callsign mult was worked
    \param  mode_nr     mode on which callsign mult was worked

    <i>band_nr</i> = ALL_BANDS means add to *only* the global accumulator; otherwise add to a band AND to the global accumulator
    The information is inserted into the <i>_callsign_multipliers</i> object.
*/
  void _insert_callsign_mult(const std::string& mult_name, const std::string& mult_value, const unsigned int band_nr = ALL_BANDS, const unsigned int mode_nr = ALL_MODES);

/*! \brief          Generate a summary string for display
    \param  rules   rules for this contest
    \param  modes   the set of modes that are to be included in the summary string
    \return         summary string for modes in <i>modes</i>
*/
  const std::string _summary_string(const contest_rules& rules, const std::set<MODE>& modes);

public:

/// default constructor
  inline running_statistics(void) :
    _auto_country_mults(false),
    _callsign_mults_used(false),
    _country_mults_used(false),
    _exchange_mults_used(false),
    _include_qtcs(false),
    _n_dupes( { {} } ),
    _n_qsos( { {} } ),              // Josuttis 2nd ed., p.262 -- initializes all elements with zero
    _qso_points( { {} } ),
    _qtc_qsos_sent(0),
    _qtc_qsos_unsent(0)
  { }

/*! \brief                  Constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
  inline running_statistics(const cty_data& country_data, const drlog_context& context, /* const */ contest_rules& rules) :
    _auto_country_mults(false),
    _callsign_mults_used(rules.callsign_mults_used()),
    _country_mults_used(rules.country_mults_used()),
    _exchange_mults_used(rules.exchange_mults_used()),
    _exch_mult_fields(rules.exchange_mults().cbegin(), rules.exchange_mults().cend()),
    _include_qtcs(rules.send_qtcs()),
    _location_db(country_data, context.country_list()),
    _n_dupes( { {} } ),
    _n_qsos( { {} } ),              // Josuttis 2nd ed., p.262 -- initializes all elements with zero
    _qso_points( { {} } ),
    _qtc_qsos_sent(0),
    _qtc_qsos_unsent(0)
  { }
  
/*! \brief                  Prepare an object that was created with the default constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
  void prepare(const cty_data& country_data, const drlog_context& context, const contest_rules& rules);

  SAFEREAD(callsign_mults_used, statistics);                ///< are callsign mults used?

  SAFEREAD(country_multipliers, statistics);                ///< country multipliers
  SAFEREAD(country_mults_used, statistics);                 ///< are country mults used?

  SAFEREAD(exchange_mults_used, statistics);                ///< are exchange mults used?

/*! \brief          How many QSOs have been made?
    \param  rules   rules for this contest

    Counts only those QSOs on bands being used to calculate the score. Includes dupes.
*/
  const unsigned int n_qsos(const contest_rules& rules) const;

/*! \brief          How many QSOs have been made in a particular mode?
    \param  rules   rules for this contest
    \param  m       target mode
    \return         number of qsos on mode <i>m</i>

    Counts only those QSOs on bands being used to calculate the score. Includes dupes.
*/
  const unsigned int n_qsos(const contest_rules& rules, const MODE m) const;

/*! \brief              Do we still need to work a particular callsign mult on a particular band and mode?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
    \param  m           mode to test
    \return             whether the mult <i>mult_name</i> with value <i>mult_value</i> is a needed callsign mult on band <i>b</i> and mode <i>m</i>
*/
  const bool is_needed_callsign_mult(const std::string& mult_name, const std::string& mult_value, const BAND b, const MODE m) const;

/*! \brief          Add a known value of country mult
    \param  str     canonical prefix of mult
    \param  rules   rules for this contest
    \return         Whether <i>str</i> was actually added

    Does nothing and returns false if <i>str</i> is already known
*/
  const bool add_known_country_mult(const std::string& str, const contest_rules& rules);

/*! \brief              Do we still need to work a particular country as a mult on a particular band and a particular mode?
    \param  callsign    call to test
    \param  b           band to test
    \param  m           mode to test
    \return             whether the country corresponding <i>callsign</i> still needs to be worked on band <i>b</i> and mode <i>m</i>.
*/
  const bool is_needed_country_mult(const std::string& callsign, const BAND b, const MODE m);
  
/*! \brief          Add a QSO to the ongoing statistics
    \param  qso     QSO to add
    \param  log     logbook (without the qso <i>qso</i>)
    \param  rules   contest rules
*/
  void add_qso(const QSO& qso, const logbook& log, const contest_rules& rules);
  
/*! \brief          Perform a complete rebuild
    \param  log     logbook
    \param  rules   rules for this contest
*/
  void rebuild(const logbook& log, const contest_rules& rules);

/*! \brief          Add a known legal value for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \param  value   new legal value for the exchange multiplier <i>name</i>
    \return         whether <i>value</i> was actually added
*/
  const bool add_known_exchange_mult(const std::string& name, const std::string& value);

/*! \brief          Return all known legal values for a particular exchange multiplier
    \param  name    name of the exchange multiplier
    \return         All the known legal values of <i>name</i>
*/
  const std::set<std::string> known_exchange_mult_values(const std::string& name);

/*! \brief                          Do we still need to work a particular exchange mult on a particular band and mode?
    \param  exchange_field_name     name of the target exchange field
    \param  exchange_field_value    value of the target exchange field
    \param  b                       target band
    \param  m                       target mode
    \return                         whether reception of exchange field <i>exchange_field_name</i> with value <i>exchange_field_value</i> on band <i>b</i> and mode <i>m</i> would be a multiplier
*/
  const bool is_needed_exchange_mult(const std::string& exchange_field_name, const std::string& exchange_field_value, const BAND b, const MODE m) const;

/*! \brief                  Add a worked exchange mult
    \param  field_name      exchange mult field name
    \param  field_value     value of the field <i>field_name</i>
    \param  band_nr         number of the band on which worked mult is to be added
    \param  mode_nr         number of the mode on which worked mult is to be added
    \return                 whether the exchange mult was added

    Doesn't add if the value <i>field_value</i> is unknown.
*/
  const bool add_worked_exchange_mult(const std::string& field_name, const std::string& field_value, const int band_nr = ALL_BANDS, const int mode_nr = ALL_MODES);

/*! \brief          A complete (multi-line) string that summarizes the statistics, for display in the SUMMARY window
    \param  rules   rules for this contest
    \return         summary string for display in the SUMMARY window
*/
  const std::string summary_string(const contest_rules& rules);

/*! \brief          Total points
    \param  rules   rules for this contest
    \return         current point total
*/
  const unsigned int points(const contest_rules& rules) const;

/*! \brief              Worked callsign mults for a particular band and mode
    \param  mult_name   name of mult
    \param  b           band
    \param  m           mode
    \return             callsign mults worked on band <i>b</i> and mode <i>m</i>
*/
  const std::set<std::string> worked_callsign_mults(const std::string& mult_name, const BAND b, const MODE m);

/*! \brief      Worked country mults for a particular band and mode
    \param  b   target band
    \param  m   target mode
    \return     all the worked country mults on band <i>b</i> and mode <i>m</i>
*/
  inline const std::set<std::string> worked_country_mults(const BAND b, const MODE m)
  { SAFELOCK(statistics);
    return ( _country_multipliers.worked(b, m) );
  }

/// all the known country mults
  inline const std::set<std::string> known_country_mults(void)
  { SAFELOCK(statistics);
    return _country_multipliers.known();
  }

/// the number of known country mults
  inline const size_t n_known_country_mults(void) const
  { SAFELOCK(statistics);
    return _country_multipliers.n_known();
  }

/*! \brief      Worked exchange mults for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     all the exchange mults worked on band <i>b</i> and mode <i>m</i>
*/
  const std::map<std::string /* field name */, std::set<std::string> /* values */ > worked_exchange_mults(const BAND b, const MODE m) const;

/*! \brief                                  Is a particular string a known callsign mult name?
    \param  putative_callsign_mult_name     string to test
    \return                                 whether <i>putative_callsign_mult_name</i> is a known callsign mult name
*/
  const bool known_callsign_mult_name(const std::string& putative_callsign_mult_name) const;

/// clear the dynamic information
  void clear_info(void);

/*! \brief      Set the number of sent QTC QSOs
    \param  n   number of sent QTC QSOs
*/
  void qtc_qsos_sent(const unsigned int n);

/*! \brief      Set the number of unsent QTC QSOs
    \param  n   number of unsent QTC QSOs
*/
  void qtc_qsos_unsent(const unsigned int n);

/*! \brief          Get the number of worked callsign mults
    \param  rules   rules for this contest
    \return         the number of callsign mults worked
*/
  const unsigned int n_worked_callsign_mults(const contest_rules& rules) const;

/*! \brief          Get the number of worked country mults
    \param  rules   rules for this contest
    \return         the number of country mults worked
*/
  const unsigned int n_worked_country_mults(const contest_rules& rules) const;

/*! \brief          Get the number of worked exchange mults
    \param  rules   rules for this contest
    \return         the number of exchange mults worked
*/
  const unsigned int n_worked_exchange_mults(const contest_rules& rules) const;

/*! \brief      Get the number of exchange mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the number of exchange mults worked on band <i>b</i> and mode <i>m</i>
*/
  const unsigned int n_worked_exchange_mults(const BAND b, const MODE m) const;

/*! \brief          What is the ratio of the value of a new mult to the value of a new (non-mult) QSO?
    \param  rules   rules for this contest
    \param  b       band
    \param  m       mode
    \return         the ratio of the value of a new mult QSO on band <i>b</i> and mode <i>m</i> to the value of a new non-mult QSO on the same band and mode
*/
  const float mult_to_qso_value(const contest_rules& rules, const BAND b, const MODE m) const;

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(statistics);

      ar & _callsign_multipliers
         & _callsign_mults_used
         & _country_multipliers
         & _country_mults_used
         & _exchange_multipliers
         & _exchange_mults_used
         & _exch_mult_fields
         & _include_qtcs
         & _location_db
         & _n_dupes
         & _n_qsos
         & _n_ON_qsos
         & _qso_points
         & _qtc_qsos_sent
         & _qtc_qsos_unsent;
    }
};

// -----------  call history  ----------------

/*! \class  call_history
    \brief  History of each call worked

    Instantiations of this class are automatically thread-safe.
*/

class call_history
{
protected:

  std::map<std::string, std::set<bandmode> > _history;              ///< container for the history
  pt_mutex                                   _history_mutex;        ///< mutex for the container

public:

/*! \brief          Add a QSO to the history
    \param  qso     QSO to add
*/
  void operator+=(const QSO& qso);

/*! \brief      Has a call been worked on a particular band and mode?
    \param  s   callsign to test
    \param  b   band to test
    \param  m   mode to test
    \return     whether <i>s</i> has been worked on band <i>b</i> and mode <i>m</i>
*/
  const bool worked(const std::string& s, const BAND b, const MODE m);

/*! \brief      Has a call been worked on a particular band?
    \param  s   callsign to test
    \param  b   band to test
    \return     whether <i>s</i> has been worked on band <i>b</i>
*/
  const bool worked(const std::string& s, const BAND b);

/*! \brief      Has a call been worked on a particular mode?
    \param  s   callsign to test
    \param  m   mode to test
    \return     whether <i>s</i> has been worked on mode <i>m</i>
*/
  const bool worked(const std::string& s, const MODE m);

/*! \brief      Has a call been worked?
    \param  s   callsign to test
    \return     whether <i>s</i> has been worked
*/
  const bool worked(const std::string& s);

/*! \brief      Has a call been worked on any other band?
    \param  s   callsign to test
    \param  b   band NOT to test
    \return     whether <i>s</i> has been worked on a band other than <i>b</i>
*/
  const bool worked_on_another_band(const std::string& s, const BAND b);

/*! \brief      Has a call been worked on any other mode?
    \param  s   callsign to test
    \param  m   mode NOT to test
    \return     whether <i>s</i> has been worked on a mode other than <i>m</i>
*/
  const bool worked_on_another_mode(const std::string& s, const MODE m);

/*! \brief      Has a call been worked on any other band and mode combination?
    \param  s   callsign to test
    \param  b   band not to include
    \param  m   mode not to include
    \return     whether <i>s</i> has been worked on a band and mode other than <i>b</i> and <i>m</i>
*/
  const bool worked_on_another_band_and_mode(const std::string& s, const BAND b, const MODE m);

/*! \brief          Perform a complete rebuild
    \param  logbk   logbook
*/
  void rebuild(const logbook& logbk);

/// clear the history
  void clear(void);

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_history);
      ar & _history;
    }
};

#endif    // STATISTICS_H
