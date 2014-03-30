// $Id: statistics.h 55 2014-03-22 20:32:08Z  $

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
  
  location_database                                        _location_db;
  
  std::array<unsigned int, N_BANDS>                        _n_dupes;
  std::array<unsigned int, N_BANDS>                        _n_qsos;

  std::array<unsigned int, N_BANDS>                        _qso_points;

  std::set<std::string>                                    _exch_mult_fields;    ///< names of the exch fields that are mults

// ALL_BANDS means add to *only* the global set; otherwise add to a band AND to the global set
  void _insert_callsign_mult(const std::string& mult_name, const std::string& mult_value, const unsigned int band_nr = ALL_BANDS);

  std::map<std::string /* mult name */, multiplier>                 _callsign_multipliers;
  multiplier                                                        _country_multipliers;
  std::vector<std::pair<std::string /* field name */, multiplier> > _exchange_multipliers;  // vector so we can keep the correct order

// copied from rules
  bool                                _callsign_mults_used;      ///< are callsign mults used?
  bool                                _country_mults_used;      ///< are country mults used?
  bool                                _exchange_mults_used;      ///< are country mults used?

public:

  running_statistics(void);

  running_statistics(const cty_data& country_data, const drlog_context& context, const contest_rules& rules);
  
  void prepare(const cty_data& country_data, const drlog_context& context, const contest_rules& rules);

  SAFEREAD(callsign_mults_used, statistics);                  ///< are callsign mults used?
  SAFEREAD(country_mults_used, statistics);                   ///< are country mults used?
  SAFEREAD(exchange_mults_used, statistics);                  ///< are exchange mults used?

// return true if actually added
  const bool add_known_country_mult(const std::string& str);

/// do we still need to work a particular country as a mult on a particular band?
  const bool is_needed_country_mult(const std::string& callsign, const BAND);

  const bool is_needed_callsign_mult(const std::string& mult_name, const std::string& mult_value, const BAND b) const;

/// a string list of bands on which a country is needed
  const std::string country_mult_needed(const std::string& call, const contest_rules& rules);
  
/// add a qso
  void add_qso(const QSO& qso, const logbook& log, const contest_rules& rules);
  
/// rebuild
  void rebuild(const logbook& log, const contest_rules& rules);

/// a string list of bands on which a particular exchange mult value is needed
  const std::string exchange_mult_needed(const std::string& exchange_field_name, const std::string& exchange_field_value, const contest_rules& rules); 

/// do we still need to work a particular exchange mult on a particular band?
  const bool is_needed_exchange_mult(const std::string& exchange_field_name, const std::string& exchange_field_value, const BAND b) const;

  const std::array<std::set<std::string>, N_BANDS> worked_exchange_mults(const std::string& exchange_field_name);

  const std::set<std::string> worked_exchange_mults(const std::string& exchange_field_name, const BAND b);
  
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

  const bool known_callsign_mult_name(const std::string& putative_callsign_mult_name) const;

  void clear_info(void);

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

//  void add(const std::string& callsign, const BAND b, const MODE m);
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
