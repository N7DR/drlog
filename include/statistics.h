// $Id: statistics.h 79 2014-10-11 15:09:04Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef STATISTICS_H
#define STATISTICS_H

/*!     \file statistics.h

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

extern pt_mutex statistics_mutex;

// -----------  running_statistics  ----------------

/*!     \class running_statistics
        \brief ongoing statistics
*/

class running_statistics
{
protected:
  
  location_database                                  _location_db;          ///< database for location-based lookups
  
  std::array<unsigned int, N_BANDS>                  _n_dupes;              ///< number of dupes, per band
  std::array<unsigned int, N_BANDS>                  _n_qsos;               ///< number of QSOs, per band
  std::array<unsigned int, N_BANDS>                  _qso_points;           ///< number of QSO points, per band

  std::set<std::string>                              _exch_mult_fields;     ///< names of the exch fields that are mults

/*! \brief  add a callsign mult name, value and band to those worked
    \param  mult_name   name of callsign mult
    \param  mult_value  value of callsign mult
    \param  band_nr     band on which callsign mult was worked

    <i>band_nr</i> = ALL_BANDS means add to *only* the global accumulator; otherwise add to a band AND to the global accumulator
    The information is inserted into the <i>_callsign_multipliers</i> object.
*/
  void _insert_callsign_mult(const std::string& mult_name, const std::string& mult_value, const unsigned int band_nr = ALL_BANDS);

  std::map<std::string /* mult name */, multiplier>                 _callsign_multipliers;  ///< callsign multipliers (supports more than one)
  multiplier                                                        _country_multipliers;   ///< country multipliers
  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  ///< exchange multipliers; vector so we can keep the correct order

// these are copied from rules
  bool                            _callsign_mults_used;      ///< are callsign mults used?
  bool                            _country_mults_used;       ///< are country mults used?
  bool                            _exchange_mults_used;      ///< are country mults used?

  bool          _include_qtcs;                  ///< do we include QTC information?
  unsigned int  _qtc_qsos_sent;                 ///< total number of QSOs sent in QTCs
  unsigned int  _qtc_qsos_unsent;               ///< total number of (legal) QSOs available but not yet sent in QTCs

public:

/// default constructor
  running_statistics(void);

/*! \brief  Constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
  running_statistics(const cty_data& country_data, const drlog_context& context, const contest_rules& rules);
  
/*! \brief  Prepare an object that was created with the default constructor
    \param  country_data    data from cty.dat file
    \param  context         drlog context
    \param  rules           rules for this contest
*/
  void prepare(const cty_data& country_data, const drlog_context& context, const contest_rules& rules);

  SAFEREAD(callsign_mults_used, statistics);                  ///< are callsign mults used?
  SAFEREAD(country_mults_used, statistics);                   ///< are country mults used?
  SAFEREAD(exchange_mults_used, statistics);                  ///< are exchange mults used?

/*! \brief  do we still need to work a particular callsign mult on a particular band?
    \param  mult_name   name of mult
    \param  mult_value  value of mult to test
    \param  b           band to test
*/
  const bool is_needed_callsign_mult(const std::string& mult_name, const std::string& mult_value, const BAND b) const;

/*! \brief          Add a known value of country mult
    \param  str     Canonical prefix of mult
    \return         Whether <i>str</i> was actually added

    Does nothing and returns false if <i>str</i> is already known
*/
  const bool add_known_country_mult(const std::string& str);

/*! \brief  do we still need to work a particular country as a mult on a particular band?
    \param  callsign    call to test
    \param  b           band to test
    \return         Whether the country corresponding <i>callsign</i> still needs to be worked on band <i>b</i>.
*/
  const bool is_needed_country_mult(const std::string& callsign, const BAND b);

/*! \brief  On what bands is a country mult needed?
    \param  call    call to test
    \param  rules   Rules for the contest
    \return         Space-separated (actually, multiple spaces) string of band names on which
                    the country corresponding to <i>call</i> is needed.
*/
  const std::string country_mult_needed(const std::string& call, const contest_rules& rules);
  
/*! \brief  Add a QSO to the ongoing statistics
    \param  qso     QSO to add
    \param  log     Logbook (without the qso <i>qso</i>)
    \param  rules   Contest rules
*/
  void add_qso(const QSO& qso, const logbook& log, const contest_rules& rules);
  
/// rebuild
  void rebuild(const logbook& log, const contest_rules& rules);

  const bool add_known_exchange_mult(const std::string& name, const std::string& value);

  const std::set<std::string> known_exchange_mults(const std::string& name);

/// a string list of bands on which a particular exchange mult value is needed
  const std::string exchange_mult_needed(const std::string& exchange_field_name, const std::string& exchange_field_value, const contest_rules& rules); 

/// do we still need to work a particular exchange mult on a particular band?
  const bool is_needed_exchange_mult(const std::string& exchange_field_name, const std::string& exchange_field_value, const BAND b) const;

  const std::array<std::set<std::string>, N_BANDS> worked_exchange_mults(const std::string& exchange_field_name);

  const std::set<std::string> worked_exchange_mults(const std::string& exchange_field_name, const BAND b);

/*! \brief  Add a worked exchange mult
    \param  field_name    Exchange mult field name
    \param  field_value   Value of the field <i>field_name</i>
    \param  band_nr       Number of the band on which worked mult is to be added
*/
  void add_worked_exchange_mult(const std::string& field_name, const std::string& field_value, const int band_nr = ALL_BANDS);

/// a (multi-line) string that summarizes the statistics
  const std::string summary_string(const contest_rules& rules);

/// total points
  const unsigned int points(const contest_rules& rules) const;

/// worked callsign mults for a particular band
  const std::set<std::string> worked_callsign_mults(const std::string& mult_name, const BAND b);

/// worked country mults for a particular band
  const std::set<std::string> worked_country_mults(const BAND b);

  inline const std::set<std::string> known_country_mults(void)
    { SAFELOCK(statistics);
      return _country_multipliers.known();
    }

  inline const size_t n_known_country_mults(void) const
    { return _country_multipliers.n_known(); }

/// worked exchange mults for a particular band
  const std::map<std::string /* field name */, std::set<std::string> /* values */ >   worked_exchange_mults(const BAND b);

/*! \brief  is a particular string a known callsign mult name?
    \param  putative_callsign_mult_name     string to test
    \return whether <i>putative_callsign_mult_name</i> is a known callsign mult name
*/
  const bool known_callsign_mult_name(const std::string& putative_callsign_mult_name) const;

  void clear_info(void);

  void qtc_qsos_sent(const unsigned int n);
  void qtc_qsos_unsent(const unsigned int n);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(statistics);

      ar & _callsign_multipliers
         & _callsign_mults_used
         & _country_multipliers
         & _country_mults_used
         & _exchange_multipliers
         & _exchange_mults_used
         & _location_db
         & _n_dupes
         & _n_qsos
         & _qso_points;
    }
};

// -----------  call history  ----------------

/*!     \class call_history
        \brief History of each call worked

        Instantiations of this class are automatically thread-safe.
*/

class call_history
{
protected:
  std::map<std::string, std::set<bandmode> > _history;

  pt_mutex _history_mutex;

public:

  void operator+=(const QSO& qso);

  const bool worked(const std::string& s, const BAND b, const MODE m);

  inline const bool worked(const std::string& s, const BAND b)
    { return (worked(s, b, MODE_CW) or worked(s, b, MODE_SSB)); }

  const bool worked(const std::string& s);

  void rebuild(const logbook& logbk);

  void clear(void);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_history);
      ar & _history;
    }

};

#endif    // STATISTICS_H
