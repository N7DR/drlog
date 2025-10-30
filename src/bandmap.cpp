// $Id: bandmap.cpp 277 2025-10-19 15:57:37Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   bandmap.cpp

    Classes and functions related to bandmaps
*/

#include "bandmap.h"
#include "exchange.h"
#include "internals.h"
#include "log_message.h"
#include "statistics.h"
#include "string_functions.h"
#include "time_log.h"

#include <fstream>
#include <functional>

using namespace std;

namespace SR  = std::ranges;
namespace SRV = std::ranges::views;

extern bool                          bandmap_show_marked_frequencies;           ///< whether to display entries that would be marked
extern bool                          bandmap_frequency_up;                      ///< whether increasing frequency goes upwards in the bandmap
extern pt_mutex                      batch_messages_mutex;                      ///< mutex for batch messages
extern UNORDERED_STRING_MAP<string>  batch_messages;                            ///< batch messages associated with calls
extern drlog_context                 context;                                   ///< the context for the contest
extern exchange_field_database       exchange_db;                               ///< dynamic database of exchange field values for calls; automatically thread-safe
extern atomic<frequency>             last_update_frequency;                     ///< the frequency of the last bm window update
extern location_database             location_db;                               ///< location information
extern unsigned int                  max_qsos_without_qsl;                      ///< limit for the N7DR matches_criteria() algorithm
extern MINUTES_TYPE                  now_minutes;                               ///< access the current time in minutes
extern map<MODE, vector<pair<frequency, frequency>>> marked_frequency_ranges;   ///< the ranges of frequencies to be marked as red (i.e., not to be used in the ocntest)
extern message_stream                ost;                                       ///< debugging/logging output

extern const STRING_SET CONTINENT_SET;                 ///< two-letter abbreviations for all the continents

extern bool is_marked_frequency(const map<MODE, vector<pair<frequency, frequency>>>& marked_frequency_ranges, const MODE m, const frequency f); ///< Is a particular frequency within any marked range?

/*! \brief                      Obtain value corresponding to a type of callsign mult from a callsign
    \param  callsign_mult_name  the type of the callsign mult
    \param  callsign            the call for which the mult value is required

    Returns the empty string if no sensible result can be returned
*/
extern const string callsign_mult_value(const string_view callsign_mult_name, const string_view callsign);

constexpr unsigned int  MAX_CALLSIGN_WIDTH { 11 };        ///< maximum width of a callsign in the bandmap window
constexpr frequency     MAX_FREQUENCY_SKEW { 250_Hz };       ///< maximum separation to be treated as same frequency

constexpr string MODE_MARKER { "********"s };          ///< string to mark the mode break in the bandmap
constexpr string MY_MARKER   { "--------"s };          ///< the string that marks my position in the bandmap

bandmap_filter_type BMF;                            ///< the global bandmap filter

/*! \brief          Printable version of the name of a bandmap_entry source
    \param  bes     source of a bandmap entry
    \return         printable version of <i>bes</i>
*/
string to_string(const BANDMAP_ENTRY_SOURCE bes)
{ using enum BANDMAP_ENTRY_SOURCE;

  switch (bes)
  { case CLUSTER :
      return "BANDMAP_ENTRY_CLUSTER"s;

    case LOCAL :
      return "BANDMAP_ENTRY_LOCAL"s;

    case RBN :
      return "BANDMAP_ENTRY_RBN"s;

    default :
      return "UNKNOWN"s;
  }
}

// -----------  n_posters_database  ----------------

/*! \class  n_posters_database
    \brief  A database for the number of posters of stations
*/

/*! \brief      Add a call and poster to the database
    \param  pr  call and poster to be added
*/
void n_posters_database::operator+=(const pair<string /* call */, string /* poster */>& pr)
{ const time_t  now_m  { now_minutes };
  const string& call   { pr.first };
  const string& poster { pr.second };

  lock_guard<recursive_mutex> lg(_mtx);

  if (_min_posters > 1)
  { _data[now_m][call] += poster;

    test_call(call);
  }
}

/*! \brief      Get all the times in the database
    \return     all the times in <i>_data</i>
*/
set<time_t> n_posters_database::times(void) const
{ lock_guard<recursive_mutex> lg(_mtx); 

  return ALL_KEYS_SET(_data);
}

/*! \brief          Test whether a call appears enough times to be considered "good", and add to <i>_known_good_calls</i> if so
    \param  call    call to test
    \return         whether <i>call</i> is a known good call
*/
bool n_posters_database::test_call(const string_view call)
{ lock_guard<recursive_mutex> lg(_mtx); 

  if ( (_min_posters == 1) or _known_good_calls.contains(call) )
    return true;

  const set<time_t> all_times { times() };

  bool known_good      { false };
  int  count_n_posters { 0 };

  for (auto it { all_times.cbegin() }; !known_good and (it != all_times.end()); ++it)
  { const UNORDERED_STRING_MAP<UNORDERED_STRING_SET>& um { _data.at(*it) };    // key = call

    if (um.contains(call))
    { const string call_str { call };

      count_n_posters += um.at(call_str).size();

      if (count_n_posters >= _min_posters)
      { _known_good_calls += call_str;
        
        return true;
      }
    }
  }

  return false;
}

/// Prune the database
void n_posters_database::prune(void)
{ const time_t now_m { now_minutes };

  lock_guard<recursive_mutex> lg(_mtx);

  if (_min_posters == 1)
    return;

  const time_t target_time { now_m - _width };

  set<time_t> all_times { times() };

  bool need_to_clear_known_good_calls { false };

  for (const auto t : all_times)
  { if (t < target_time)
    { _data -= t;
      need_to_clear_known_good_calls = true;        // have to rebuild _known_good_calls if we've removed a minute
    }
  }

  if (need_to_clear_known_good_calls)
  { all_times = move(times());
    _known_good_calls.clear();

// rebuild _known_good_calls
    UNORDERED_STRING_SET all_calls { };

    FOR_ALL(all_times, [&all_calls, this] (const time_t t)     { all_calls += move(ALL_KEYS_USET(_data.at(t))); });
    FOR_ALL(all_calls, [this]             (const string& call) { test_call(call); });
  }
}

/// Convert to printable string
string n_posters_database::to_string(void) const
{ string rv;

  lock_guard<recursive_mutex> lg(_mtx);

  for (const auto& [t, cp] : _data)
  { rv += ::to_string(t) + ":"s + EOL;

    for (const auto& [c, p] : cp)
    { rv += "  "s + ::to_string(c) + "  :"s + EOL;

      for (const auto& poster : p)
        rv += "    "s + poster + " "s;
      
      rv += EOL;
      rv += "n_posters = "s + ::to_string(p.size()) + EOL;
    }
  }

  rv += EOL;

  if (_known_good_calls.empty())
    rv += "No known good calls"s + EOL;
  else
  { rv += "Known good calls: "s;

    CALL_SET ordered_known_good_calls;

    FOR_ALL(_known_good_calls,        [&ordered_known_good_calls] (const string& call) { ordered_known_good_calls += call; } );
    FOR_ALL(ordered_known_good_calls, [&rv]                       (const string& call) { rv += (call + " "s); } );

    rv += EOL + "Number of known good calls = "s + ::to_string(ordered_known_good_calls.size()) + EOL;
  }

  rv += EOL;

  return rv;
}

// -----------   bandmap_filter_type ----------------

/*! \class  bandmap_filter_type
    \brief  Control bandmap filtering
*/

/*!  \brief         Add a string to, or remove a string from, the filter
     \param str     string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed.
*/
void bandmap_filter_type::add_or_subtract(const string_view str)
{ const bool   is_continent { CONTINENT_SET.contains(str) };
  const string str_copy     { is_continent ? str : location_db.info(str).canonical_prefix() };  // convert to canonical prefix if necessary

//  GROUP_TYPE& g_ref { ( is_continent ? _continents : _prefixes ) };    // create reference to correct group

  if (GROUP_TYPE& g_ref { ( is_continent ? _continents : _prefixes ) }; g_ref.contains(str_copy))    // // create reference to correct group; remove a value
    g_ref -= str_copy;
  else
    g_ref += str_copy;
}

// -----------  bandmap_entry  ----------------

/*! \brief          Set the callsign
    \param  call    the callsign to set
*/
bandmap_entry& bandmap_entry::callsign(const string_view call)
{ _callsign = call;

  if (!is_marker())
  { const location_info li { location_db.info(_callsign)};

    _canonical_prefix = li.canonical_prefix();
    _continent = li.continent();
  }

  return *this;
}

/*! \brief      Set <i>_freq</i>, <i>_frequency_str</i>, <i>_band</i> and <i>_mode</i>
    \param  f   frequency used to set the values
*/
bandmap_entry& bandmap_entry::freq(const frequency f)
{ _freq = f;
  _frequency_str = _freq.display_string();
  _band = to_BAND(f);
  _mode = putative_mode();

  return *this;
}

/*! \brief              Calculate the mult status of this entry
    \param  rules       the rules for this contest
    \param  statistics  the current statistics

    Adjust the callsign, country and exchange mult status in accordance with the passed parameters
    Note that the parameters are NOT constant
*/
void bandmap_entry::calculate_mult_status(contest_rules& rules, running_statistics& statistics)
{
// callsign mult
  clear_callsign_mult();

//  for ( const auto& callsign_mult_name : rules.callsign_mults() )
  for ( string_view callsign_mult_name : rules.callsign_mults() )
  { if (const string callsign_mult_val { callsign_mult_value(callsign_mult_name, _callsign) }; !callsign_mult_val.empty())
    { if (statistics.is_needed_callsign_mult(callsign_mult_name, callsign_mult_val, _band, _mode))
        add_callsign_mult(callsign_mult_name, callsign_mult_val);
    }
  }

  _is_needed_callsign_mult.status_is_known(true);

// country mult
  if (rules.n_country_mults())        // if country mults are used
  { clear_country_mult();

    if (statistics.is_needed_country_mult(_callsign, _band, _mode, rules))
      add_country_mult(_canonical_prefix);

    _is_needed_country_mult.status_is_known(true);
  }
  else
  { // default _is_needed_country_mults instantiation of needed_mult_details should be: not needed, is known
  }

// exchange mult status
  clear_exchange_mult();

  const vector<string> exch_mults { rules.expanded_exchange_mults() };                                  // the exchange multipliers

// there can be an exchange mult only if one of the exchange field names matches an exchange mult field name
  bool exchange_mult_is_possible { false };

  for (const auto& exch_mult_name : exch_mults)
  { const vector<string> exchange_field_names       { rules.expanded_exchange_field_names(_canonical_prefix, _mode) };
    const bool           is_possible_exchange_field { contains(exchange_field_names, exch_mult_name) };

    if (is_possible_exchange_field)
    { exchange_mult_is_possible = true;

      if (const string guess { rules.canonical_value(exch_mult_name, exchange_db.guess_value(_callsign, exch_mult_name)) }; !guess.empty())
      { if ( statistics.is_needed_exchange_mult(exch_mult_name, guess, _band, _mode) )
          add_exchange_mult(exch_mult_name, guess);

        _is_needed_exchange_mult.status_is_known(true);
      }
    }
  }

  if (!exchange_mult_is_possible)                      // we now know that no exchange fields are exchange mult fields for this canonical prefix
    _is_needed_exchange_mult.status_is_known(true);

// it isn't trivial to know how to set _mult_status_is_known under various possibilities
// for now... if any mult type is used AND we know the status of that mult, we say that
// we know the total status
  _mult_status_is_known = false;

  if (rules.callsign_mults_used() and _is_needed_callsign_mult.is_status_known())
    _mult_status_is_known = true;

  if (!_mult_status_is_known and rules.country_mults_used(canonical_prefix()) and _is_needed_country_mult.is_status_known())
    _mult_status_is_known = true;

  if (!_mult_status_is_known and rules.exchange_mults_used() and _is_needed_exchange_mult.is_status_known())
    _mult_status_is_known = true;
}

/*! \brief      Does this object match another bandmap_entry?
    \param  be  target bandmap entry
    \return     whether frequency_str or callsign match

    Used in += function.
*/
bool bandmap_entry::matches_bandmap_entry(const bandmap_entry& be) const
{ if ((be.is_my_marker()) or is_my_marker())       // mustn't delete a valid call if we're updating my QRG
    return (_callsign == be._callsign);

  return ((_callsign == be._callsign) or (_frequency_str == be._frequency_str));  // neither bandmap_entry is at my QRG
}

/*! \brief              Re-mark the need/mult status
    \param  rules       rules for the contest
    \param  q_history   history of all the QSOs
    \param  statistics  statistics for the contest so far
    \return             whether there are any changes in needed/mult status

    <i>statistics</i> must be updated to be current before this is called
*/
bool bandmap_entry::remark(contest_rules& rules, const call_history& q_history, running_statistics& statistics)
{ const bool original_is_needed { _is_needed };

// if this contest allows only one QSO with a station (e.g., SS)
  if (!rules.work_if_different_band())
    _is_needed = NONE_OF(rules.permitted_bands(), [this, &q_history] (const BAND b) { return q_history.worked(_callsign, b); } );
  else
    _is_needed = !q_history.worked(_callsign, _band);

// multi-mode contests
  const bool original_is_needed_callsign_mult { is_needed_callsign_mult() };
  const bool original_is_needed_country_mult  { is_needed_country_mult() };
  const bool original_is_needed_exchange_mult { is_needed_exchange_mult() };

  calculate_mult_status(rules, statistics);

  return ( (original_is_needed != _is_needed) or (original_is_needed_callsign_mult != is_needed_callsign_mult()) or
           (original_is_needed_country_mult != is_needed_country_mult()) or (original_is_needed_exchange_mult != is_needed_exchange_mult()));
}

/// guess the mode, based on the frequency
MODE bandmap_entry::putative_mode(void) const
{ if (source() == BANDMAP_ENTRY_SOURCE::RBN)
    return MODE_CW;

  try
  { return ( (_freq < MODE_BREAK_POINT.at(band()) ) ? MODE_CW : MODE_SSB);
  }

  catch (...)
  { ost << "ERROR in putative_mode(); band = " << band() << endl;
    return MODE_CW;
  }
}

/*! \brief      Does this call match the N7DR custom criteria?
    \return     whether the call matches the N7DR custom criteria

    Matches criteria:
      0. is a needed QSO AND does not have an associated batch message; AND one of:

      1. not worked on this band/mode; OR
      2. worked and QSLed on this band/mode; OR
      3. worked and QSLed on another band/mode AND worked no more than 4 times in this band/mode
*/
bool bandmap_entry::matches_criteria(void) const
{ if (!is_needed())
    return false;               // skip any call that isn't needed

  { SAFELOCK(batch_messages);

    if (batch_messages.contains(_callsign))
      return false;             // skip any call with a batch message
  }

  if (is_all_time_first())
    return true;

  if (olog.confirmed(_callsign, _band, _mode))
    return true;

  if (olog.n_qsls(_callsign) and (olog.n_qsos(_callsign, _band, _mode) <= max_qsos_without_qsl))
    return true;

  return false;
}

/*! \brief          Write a <i>bandmap_entry</i> object to an output stream
    \param  ost     output stream
    \param  be      object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const bandmap_entry& be)
{ ost << "band: " << be.band() << endl
      << "callsign: " << be.callsign() << endl
      << "canonical_prefix: " << be.canonical_prefix() << endl
      << "continent: " << be.continent() << endl
      << "expiration_time: " << be.expiration_time() << endl
      << "frequency: " << to_string(be.freq()) << endl
      << "frequency_str: " << be.frequency_str() << endl
      << "is needed: " << be.is_needed() << endl
      << "is needed mult: " << be.is_needed_mult() << endl
      << "is needed callsign mult: " << be.is_needed_callsign_mult_details() << endl
      << "is needed country mult: " << be.is_needed_country_mult_details() << endl
      << "is needed exchange mult: " << be.is_needed_exchange_mult_details() << endl;



//      try
//      { ost << "mode: " << MODE_NAME[be.mode()] << endl;
//      }

//      catch (...)
//      { ost << "mode cannot be mapped to mode name: mode = " << be.mode() << endl;
//      }

      ost << "mult status is known: " << be.mult_status_is_known() << endl;

//      try
//      { ost << "putative mode: " << MODE_NAME[be.putative_mode()] << endl;
//      }

//      catch (...)
//      { ost << "putative mode cannot be mapped to mode name: putative mode = " << be.putative_mode() << endl;
//      }

      ost << "source: " << to_string(be.source()) << endl
          << "time: " << be.time() << endl;

  return ost;
}

// -----------  bandmap  ----------------

/*! \class  bandmap
    \brief  A bandmap
*/

/*!  \brief                             Return the callsign closest to a particular frequency, if it is within the guard band
     \param bme                         band map entries
     \param target_frequency_in_khz     the target frequency, in kHz
     \param guard_band_in_hz            how far from the target to search, in Hz
     \return                            Callsign of a station within the guard band

     Returns the nearest station within the guard band, or the null string if no call is found.
     As currently implemented, assumes that the entries are in order of monotonically increasing or decreasing frequency

     UNTESTED
*/
string bandmap::_nearest_callsign(const BM_ENTRIES& bme, const frequency target_frequency, const frequency guard_band) const
{ if ( !target_frequency.is_within_ham_band() )
  { ost << "WARNING: bandmap::_nearest_callsign called with frequency = " << target_frequency << endl;
    return string { };
  }

  bool      finish_looking      { false };
  frequency smallest_difference { 1_MHz };              // start with a big number

  string rv { };

//  bandmap_entry tmp_be { };

  for (BM_ENTRIES::const_iterator cit { bme.cbegin() }; (!finish_looking and cit != bme.cend()); ++cit)
  { const frequency Δf { target_frequency.difference(cit->freq()) };

    if ( (Δf <= guard_band) and (!cit -> is_marker()))
    { if (Δf < smallest_difference)
      { smallest_difference = Δf;
        rv = cit -> callsign();
      }
    }

    if ( (cit->freq() > target_frequency) and (Δf > guard_band) )  // no need to keep looking we if are past the allowed guard band
      finish_looking = true;
  }

  return rv;
}

/*!  \brief     Insert a bandmap_entry
     \param be  entry to add

    If <i>be</i> is my marker, the frequency is slightly decreased so that it will always appear below any
    other entries at the same QRG
*/
void bandmap::_insert(const bandmap_entry& be)
{ static bandmap_entry my_marker_copy;          // needed if be is my marker
 
  if (be.is_my_marker())
  { my_marker_copy = be;
    my_marker_copy.freq(my_marker_copy.freq() - MY_MARKER_BIAS);     // make it 1Hz less than actual value
  }

  const bandmap_entry& ber { be.is_my_marker() ? my_marker_copy : be };   // point to the right bandmap_entry object
  
  bool inserted { false };

  SAFELOCK(_bandmap);

// insert it in the right place in the list
  for (BM_ENTRIES::iterator it { _entries.begin() }; (!inserted and (it != _entries.end())); ++it)
  { if (it->freq().hz() > ber.freq().hz())
      inserted = ( _entries += { it, ber }, true );
  }

  if (!inserted)
    _entries += (ber);    // this frequency is higher than any currently in the bandmap

  _version++;
}

// a call will be marked as recent if:
// its source is LOCAL or CLUSTER
// or
// its source is RBN and the call is already present in the bandmap
// at the same QRG with this poster
// ASCII art created with asciio
/*
                  .-----.
                  | RBN |
                  '-----'
                     |
                     |
                     v
             .----------------.             .-----------.
             |                |             |           |
             |  Call present? |      NO     |   POST    |
             |                |------------>|           |
             |                |             |           |
             '----------------'             '-----------'
                      | YES
                      v
                .-----------.               .-----------.          .-----------.
                |           |               |           |          |           |
                |QRG approx |        NO     |Delete old |          |    POST   |
                |  same?    ---------------->   post    |--------->|           |
                |           |               |           |          |           |
                '-----------'               '-----------'          '-----------'
                      | YES
                      v
                .---------------------.     .---------------.
                | New expiration      |     | Add poster    |
                | later than          |  NO | to posters    |
                | current expiration? |---->|               |
                |                     |     |               |
                '---------------------'     '---------------'
                        |
                        | YES
                        v
                .--------------.
                | Mark current |
                |   as RBN     |
                |              |
                |              |
                '--------------'
                       |
                       |
                       v
                .------------.
                | Add poster |
                | to posters |
                |            |
                |            |
                '------------'
                      |
                      v
                .---------------.
                | POST with new |
                | expiration    |
                |               |
                |               |
                '---------------'
 */
/*!  \brief     Mark a bandmap_entry as recent
     \param be  entry to mark

    an entry will be marked as recent if:
        its source is LOCAL or CLUSTER
    or
        its source is RBN and the call is already present in the bandmap at the same QRG with the poster of <i>be</i>
*/
bool bandmap::_mark_as_recent(const bandmap_entry& be)
{ if (_rbn_threshold == 1)
    return true;

  if ( (be.source() == BANDMAP_ENTRY_SOURCE::LOCAL) or (be.source() == BANDMAP_ENTRY_SOURCE::CLUSTER) )
    return true;

  if (be.is_marker())           // don't mark markers as recent, if somehow we got here with a marker as parameter (which should nevre happen)
    return false;

  SAFELOCK(_bandmap);

  const bandmap_entry& old_be { (*this)[be.callsign()] };

  if (!old_be.valid())    // not already present
    return false;

  if (be.frequency_difference(old_be) > MAX_FREQUENCY_SKEW)       // treat anything within 250 Hz as the same frequency
    return false;         // we're going to write a new entry

  return false;    // should never get here
}

/*!  \brief     Add a bandmap_entry
     \param be  entry to add

     <i>_time_of_earlier_bandmap_entry</i> in <i>be</i> might be changed.
     Could change this by copying the be inside +=.

     Does not add if the frequency is outside the ham bands.
*/
void bandmap::operator+=(bandmap_entry& be)
{ SAFELOCK(_bandmap);

  const bool    mode_marker_is_present { (_mode_marker_frequency.hz() != 0) };
  const string& callsign               { be.callsign() };

  bool add_it { true };  

// do not add if it's already been done recently, or matches several other conditions
  { add_it = !(_do_not_add.contains(callsign));

    if (add_it)           // handle regex do-not-match conditions
    { smatch base_match;

      bool found_match { false };

      for ( auto it { _do_not_add_regex.begin() }; !found_match and (it != _do_not_add_regex.end()); ++it)
      { const regex& rgx { it -> second };

        found_match = regex_match(callsign, base_match, rgx);
      }

      add_it = !found_match;
    }

    if (add_it)
      add_it = be.freq().is_within_ham_band();

    if (add_it)
      add_it = !((be.source() != BANDMAP_ENTRY_SOURCE::LOCAL) and is_recent_call(callsign));

// could make this more efficient by having a global container of the mode-marker bandmap entries
    if (add_it and mode_marker_is_present)
    { const bandmap_entry mode_marker_be { (*this)[MODE_MARKER] };    // assumes only one mode marker

      add_it = (be.frequency_difference(mode_marker_be) > MAX_FREQUENCY_SKEW);
    }
  }

  if (add_it)                                           // actually add the entry
  { const bool mark_as_recent { _mark_as_recent(be) };  // keep track of whether we're going to mark this as a recent call

    bandmap_entry old_be;

    if (be.source() == BANDMAP_ENTRY_SOURCE::RBN)
    { old_be = (*this)[callsign];

      if (old_be.valid())
      { if (be.frequency_difference(old_be) > MAX_FREQUENCY_SKEW)  // add only if more than 250 Hz away
        { (*this) -= callsign;
          _insert(be);
        }
        else    // ~same frequency
        { if (old_be.expiration_time() >= be.expiration_time())
          { (*this) -= callsign;
            _insert(old_be);
          }
          else    // new expiration is later
          { old_be.source(BANDMAP_ENTRY_SOURCE::RBN);
            old_be.expiration_time(be.expiration_time()); // NB: don't change time(); this line causes duration to change, which means that we can't use value_maps for colour

            (*this) -= callsign;
            be.time_of_earlier_bandmap_entry(old_be);    // could change be

            _insert(old_be);
          }
        }
      }
      else    // this call is not currently present
      { _entries.remove_if( [&be] (bandmap_entry& bme) { return ((bme.frequency_str() == be.frequency_str()) and (bme.is_not_marker())); } );  // remove any real entries at this QRG
        _insert(be);
      }

// possibly remove all the other entries at this QRG
      if (be.is_not_marker())
      { const bandmap_entry current_be { (*this)[callsign] };  // the entry in the updated bandmap

        { _entries.remove_if( [&current_be] (bandmap_entry& bme) { bool rv { bme.is_not_marker() };

                                                                   if (rv)
                                                                   { rv = (bme.callsign() != current_be.callsign());

                                                                     if (rv)
                                                                       rv = bme.frequency_str() == current_be.frequency_str();
                                                                   }

                                                                   return rv;
                                                                 } );
        }
      }
    }
    else    // not RBN
    { _entries.remove_if( [&be] (bandmap_entry& bme) { return bme.matches_bandmap_entry(be); } );
      _insert(be);
    }

    if (be.is_not_marker() and mark_as_recent)
      _recent_calls += callsign;

// don't think I need to increment the version manually
  }

  if (mode_marker_is_present and !is_present(MODE_MARKER))
    ost << "*** ERROR: MODE MARKER HAS BEEN REMOVED BY BANDMAP_ENTRY: " << be << endl;
}

/// prune the bandmap
void bandmap::prune(void)
{ SAFELOCK(_bandmap);                                   // hold the lock for the entire process

  erase_if(_entries, [now = NOW()] (const bandmap_entry& be) { return (be.should_prune(now)); } );

  _recent_calls.clear();                       // empty the container of recent calls
  _version++;
}

/*! \brief              Return the entry for a particular call
    \param  callsign    call for which the entry should be returned
    \return             the bandmap_entry corresponding to <i>callsign</i>

    Returns the default bandmap_entry if <i>callsign</i> is not present in the bandmap
*/
bandmap_entry bandmap::operator[](const string_view str) const
{ SAFELOCK(_bandmap);

  return VALUE_IF(_entries, [&str] (const bandmap_entry& be) { return (be.callsign() == str); });
}

/*! \brief          Return the first entry for a partial call
    \param  pcall   partial call for which the entry should be returned
    \return         the first bandmap_entry corresponding to <i>callsign</i>

    Returns the default <i>bandmap_entry</i>> if <i>pcall</i> matches no entries in the bandmap
*/
bandmap_entry bandmap::substr(const string_view pcall) const
{ SAFELOCK(_bandmap);

  return VALUE_IF(_entries, [&pcall] (const bandmap_entry& be) { return contains(be.callsign(), pcall); });
}

/*! \brief              Remove a call from the bandmap
    \param  callsign    call to be removed

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::operator-=(const string_view callsign)
{ SAFELOCK(_bandmap);

  if (_is_regex(callsign))
    FOR_ALL(regex_matches(callsign), [this] (const string& matched_call) { *this -= matched_call; });   // sets dirty_entries and augments _version (perhaps multiple times) if executed
  else
  { const size_t initial_size { _entries.size() };

    erase_if(_entries, [callsign] (const bandmap_entry& be) { return (be.callsign() == callsign); } );

    if (_entries.size() != initial_size)  // mark as dirty if we removed any
      _version++;
  }
}

/*! \brief              Set the needed status of a call to <i>false</i>
    \param  callsign    call for which the status should be set

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::not_needed(const string_view callsign)
{ SAFELOCK(_bandmap);

  bandmap_entry be { (*this)[callsign] };

  if (!be.callsign().empty())        // did we get an entry?
  { be.is_needed(false);
    (*this) += be;                   // this will remove the pre-existing entry and increment the version
  }
}

/*! \brief                      Set the needed country mult status of all calls in a particular country to false
    \param  canonical_prefix    canonical prefix corresponding to country for which the status should be set

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
*/
void bandmap::not_needed_country_mult(const string_view canonical_prefix)
{ SAFELOCK(_bandmap);

  bool changed { false };

  FOR_ALL(_entries, [&canonical_prefix, &changed] (decltype(*_entries.begin())& be) { changed = ( be.remove_country_mult(canonical_prefix) or changed); } );

  if (changed)
    _version++;
}

/*! \brief                      Set the needed country mult status of all calls in a particular country and on a particular mode to false
    \param  canonical_prefix    canonical prefix corresponding to country for which the status should be set
    \param  m                   mode for which the status should be set

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap with the mode <i>m</i>
*/
void bandmap::not_needed_country_mult(const string_view canonical_prefix, const MODE m)
{ SAFELOCK(_bandmap);

  FOR_ALL(_entries, [m, &canonical_prefix] (decltype(*_entries.begin())& be) { if (be.mode() == m)
                                                                                 be.remove_country_mult(canonical_prefix);
                                                                             } );
}

/*! \brief                          Set the needed callsign mult status of all matching callsign mults to <i>false</i>
    \param  pf                      pointer to function to return the callsign mult value
    \param  mult_type               name of mult type
    \param  callsign_mult_string    value of callsign mult value that is no longer a multiplier
*/
//void bandmap::not_needed_callsign_mult(string (*pf)(const string& /* e.g. "WPXPX" */, const string& /* callsign */),
//                                       const string& mult_type /* e.g., "WPXPX" */,
//                                       const string& callsign_mult_string /* e.g., SM1 */)
//void bandmap::not_needed_callsign_mult(string (*pf)(const string_view /* e.g. "WPXPX" */, const string& /* callsign */),
//                                       const string& mult_type /* e.g., "WPXPX" */,
//                                       const string& callsign_mult_string /* e.g., SM1 */)
//void bandmap::not_needed_callsign_mult(string (*pf)(const string_view /* e.g. "WPXPX" */, const string_view /* callsign */),
//                                       const string& mult_type /* e.g., "WPXPX" */,
//                                       const string& callsign_mult_string /* e.g., SM1 */)
//void bandmap::not_needed_callsign_mult(string (*pf)(const string_view /* e.g. "WPXPX" */, const string_view /* callsign */),
//                                       const string_view mult_type /* e.g., "WPXPX" */,
//                                       const string& callsign_mult_string /* e.g., SM1 */)
void bandmap::not_needed_callsign_mult(string (*pf)(const string_view /* e.g. "WPXPX" */, const string_view /* callsign */),
                                       const string_view mult_type /* e.g., "WPXPX" */,
                                       const string_view callsign_mult_string /* e.g., SM1 */)
{ if (callsign_mult_string.empty() or mult_type.empty())
    return;

  SAFELOCK(_bandmap);

  bool changed { false };   // have we changed anything?

// change status for all entries with this particular callsign mult
  for (auto& be : _entries)
  { if (be.is_needed_callsign_mult())
    { const string& callsign           { be.callsign() };
      const string  this_callsign_mult { (*pf)(mult_type, callsign) };

      if (this_callsign_mult == callsign_mult_string)
        changed |= be.remove_callsign_mult(mult_type, callsign_mult_string);
    }
  }

  if (changed)
    _version++;
}

/*! \brief              Set the needed exchange mult status of a particular exchange mult to <i>false</i>
    \param  mult_name   name of exchange mult
    \param  mult_value  value of <i>mult_name</i> that is no longer a multiplier
*/
//void bandmap::not_needed_exchange_mult(const string& mult_name, const string& mult_value)
//void bandmap::not_needed_exchange_mult(const string_view mult_name, const string& mult_value)
void bandmap::not_needed_exchange_mult(const string_view mult_name, const string_view mult_value)
{ if (mult_name.empty() or mult_value.empty())
    return;

  bool changed { false };   // have we changed anything?

  SAFELOCK(_bandmap);

//  FOR_ALL(_entries, [mult_name, mult_value, &changed] (bandmap_entry& be) { changed = (be.remove_exchange_mult(mult_name, mult_value) or changed); } );
  FOR_ALL(_entries, [mult_name, mult_value, &changed] (bandmap_entry& be) { changed |= be.remove_exchange_mult(mult_name, mult_value); } );

  if (changed)
    _version++;
}

/*! \brief          Enable or disable the filter
    \param  torf    whether to enable the filter

    Disables the filter if <i>torf</i> is false.
*/
void bandmap::filter_enabled(const bool torf)
{ if (torf != filter_enabled())
  { SAFELOCK(_bandmap);

    _filter_p->enabled(torf);
    _version++;
  }
}

/*!  \brief         Add a string to, or remove a string from, the filter associated with this bandmap
     \param str     string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed. Currently, all bandmaps share a single
     filter.
*/
//void bandmap::filter_add_or_subtract(const string& str)
void bandmap::filter_add_or_subtract(const string_view str)
{ if (!str.empty())
  { SAFELOCK(_bandmap);

    _filter_p -> add_or_subtract(str);
    _version++;
  }
}

/*!  \brief         Set or unset the filter to hide mode (as opposed to show)
     \param torf    whether to set to hide mode
*/
void bandmap::filter_hide(const bool torf)
{ if (torf != filter_hide())
  { SAFELOCK(_bandmap);

    _filter_p -> hide(torf);
    _version++;
  }
}

/*!  \brief         Set or unset the filter to show mode (as opposed to hide)
     \param torf    whether to set to show mode
*/
void bandmap::filter_show(const bool torf)
{ if (torf != filter_show())
  { SAFELOCK(_bandmap);

    _filter_p -> hide(!torf);
    _version++;
  }
}

/// all the entries, after filtering has been applied
BM_ENTRIES bandmap::filtered_entries(void)
{ if (!filter_enabled())
    return entries();

  SAFELOCK(_bandmap);

// whether to include a particular bandmap entry
  auto include_be { [this] (const bandmap_entry& be) { if (be.is_marker())
                                                         return true;

                                                       const bool display_this_entry { (_filter_p -> continents()).contains(be.continent()) or (_filter_p -> prefixes()).contains(be.canonical_prefix()) };

                                                       return filter_hide() ? !display_this_entry : display_this_entry;
                                                     }
                  };

  _filtered_entries = to<BM_ENTRIES> (_entries | SRV::filter(include_be));

// is it correct that we don't increment _version?

  return _filtered_entries;
}

/// all the entries, after the RBN threshold and filtering have been applied
// threshold is now applied before an entry is put into _entries, so this is the same as just the filtered entries
BM_ENTRIES bandmap::rbn_threshold_and_filtered_entries(void)
{ SAFELOCK(_bandmap);

  _rbn_threshold_and_filtered_entries = filtered_entries();

  return _rbn_threshold_and_filtered_entries;
}

/// all the entries, after the RBN threshold, filtering and culling have been applied
BM_ENTRIES bandmap::rbn_threshold_filtered_and_culled_entries(void)
{ SAFELOCK (_bandmap);

  switch (_cull_function)
  { default :
    case 0 :
      return rbn_threshold_and_filtered_entries();

    case 1 :                                                        // N7DR criteria
      return SR::to<BM_ENTRIES> ( rbn_threshold_and_filtered_entries() | SRV::filter([] (const bandmap_entry& be) { return (be.is_marker() or be.matches_criteria()); }) );

    case 2 :                                                        // new on this band+mode
      return SR::to<BM_ENTRIES> ( rbn_threshold_and_filtered_entries() | SRV::filter([] (const bandmap_entry& be) { return (be.is_marker() or be.is_all_time_first_and_needed_qso()); }));

    case 3 :                                                        // never worked anywhere
      return SR::to<BM_ENTRIES> ( rbn_threshold_and_filtered_entries() | SRV::filter([] (const bandmap_entry& be) { return (be.is_marker() or (olog.n_qsos(be.callsign()) == 0)); }));
  }
}

/// the displayed calls, without any markers
BM_ENTRIES bandmap::displayed_entries_no_markers(void)
{ SAFELOCK (_bandmap);

  return SR::to<BM_ENTRIES> ( displayed_entries() | SRV::filter([] (const bandmap_entry& be) { return !be.is_marker(); }) );
}

/*!  \brief         Find the next needed station up or down in frequency from the current location
     \param fp      pointer to function to be used to determine whether a station is needed
     \param f       starting frequency
     \param dirn    direction in which to search
     \param nskip   number of matches to ignore
     \return        bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found.
     Applies filtering and the RBN threshold before searching for the next station.
     Assumes that the current location is correctly marked with MY_MARKER in the bandmap

     Should perhaps use guard band instead of MAX_FREQUENCY_SKEW
*/
bandmap_entry bandmap::needed(PREDICATE_FUN_P fp, const frequency f, const enum BANDMAP_DIRECTION dirn, const int16_t nskip)
{ static const frequency MAX_PERMITTED_SKEW { 95_Hz };

  SAFELOCK(_bandmap);    // hold the lock so nothing changes while we scan the bandmap

  bandmap_entry mbe_copy { my_bandmap_entry() };

  if (mbe_copy.freq() != (f - MY_MARKER_BIAS))
  {  mbe_copy.freq(f);
    (*this) += mbe_copy;      // NB this might change the bandmap; bias automatically applied during insertion
  }

  const BM_ENTRIES fe { displayed_entries() };

// why can't this be const?
  auto marker_it { FIND_IF(fe, [] (const bandmap_entry& be) { return (be.is_my_marker()); } ) };  // find myself

  if (marker_it == fe.cend())             // should never be true
    return bandmap_entry { };

  const frequency target_freq { f };

/*! \brief          From a starting iterator, move in a direction and find the first iterator that meets a condition, after skipping <i>nskip</i> iterators
    \param  it1     starting iterator
    \param  it2     one-past-the-end iterator
    \param  nskip   number of iterators to skip
    \return         iterator
*/
  auto get_be = [fp, &fe, &target_freq] (const auto it1, const auto it2, const auto nskip)                  // this returns a range containing one element
    { return SR::subrange(it1, it2) |
               SRV::filter([fp] (const bandmap_entry& be) { return (!be.is_marker() and (be.*fp)()); }) |
               SRV::drop(nskip) |
               SRV::take(1);
    };

/*! \brief          From a starting iterator, move down and find the first iterator that meets a condition, after skipping <i>nskip</i> iterators
    \param  foit    starting forward iterator
    \param  nskip   number of iterators to skip
    \return         iterator
*/
  auto gnd_r = [fp, &fe, &get_be, &target_freq] (auto foit, const auto nskip)
  { auto v { get_be(REVERSE_IT(foit), fe.crend(), nskip) };                   // can't be const, because can't use begin() on a constant range, and there is not yet a cbegin()

    if (!v.empty())
      return *(v.begin());

// get lowest non-marker
    for (auto it { fe.begin() }; it != fe.end(); ++it)
      if (!(it->is_marker()))
        return *it;

    return bandmap_entry { };
  };

/*! \brief          From a starting iterator, move up and find the first iterator that meets a condition, after skipping <i>nskip</i> iterators
    \param  foit    starting forward iterator
    \param  nskip   number of iterators to skip
    \return         iterator
*/
  auto gnu_r = [fp, get_be, &fe, &target_freq] (auto foit, const auto nskip)
  { const auto cit2 { find_if(foit, fe.cend(), [target_freq] (const bandmap_entry& be) { return (be.freq() > (target_freq + MAX_PERMITTED_SKEW) ); }) }; // move away from my frequency, in upwards direction

    auto v { get_be(cit2, fe.cend(), nskip) };

    if (!v.empty())
      return *(v.begin());

// get highest non-marker
    for (auto it { prev(fe.end()) }; it != fe.begin(); --it)
      if (!(it->is_marker()))
        return *it;

    if (!fe.front().is_marker())
      return fe.front();

    return bandmap_entry { };
  };

  switch (dirn)
  { case DOWN :
      return gnd_r(marker_it, nskip);

    case UP :
      return gnu_r(marker_it, nskip);

    default :                       // needed to keep the compiler happy
      return bandmap_entry { };
  }
}

/*! \brief          Find the next station up or down in frequency from a given frequency
    \param  f       starting frequency
    \param  dirn    direction in which to search
    \return         bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

    The return value can be tested with .empty() to see if a station was found.
    Applies filtering and the RBN threshold before searching for the next station.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
bandmap_entry bandmap::next_displayed_be(const frequency f, const enum BANDMAP_DIRECTION dirn, const int16_t nskip, const frequency max_skew)
{ const string dirn_str { (dirn == UP) ? "UP"s : "DOWN"s };

  bandmap_entry rv { };

  const BM_ENTRIES fe { displayed_entries_no_markers() };

  if (fe.empty())
    return rv;

// are we at the lowest or highest frequency already?
  if ( ((dirn == DOWN) and (f <= (fe.front().freq() + max_skew) )) or ((dirn == UP) and (f >= (fe.back().freq() - max_skew))) )
    return rv;

  switch (dirn)
  { case UP :
    { for (BM_ENTRIES::const_iterator cit { fe.cbegin() }; cit != fe.cend(); ++cit)
      { if ( (cit->freq() - max_skew) >= f)                 // stn
        { for (int16_t skip { 0 }; skip < nskip; ++skip)
          { ++cit;

            if (cit == fe.cend())
              return *(prev(cit));  // return highest-frequency be
          }

          return *cit;
        }
      }

      return rv;    // no frequency match, including nskip
    }

    case DOWN :
    { for (BM_ENTRIES::const_reverse_iterator crit { fe.crbegin() }; crit != fe.crend(); ++crit)
      { if ( (crit->freq() + max_skew) <= f)
        { for (int16_t skip { 0 }; skip < nskip; ++skip)
          { ++crit;

            if (crit == fe.crend())
              return *(prev(crit));  // return lowest-frequency be
          }

          return *crit;
        }
      }

      return rv;    // no frequency match, including nskip
    }
  }

  return bandmap_entry { };       // keep compiler happy
}

/*! \brief      Get lowest frequency on the bandmap
    \return     lowest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
frequency bandmap::lowest_frequency(void)
{ const BM_ENTRIES bme { displayed_entries() };

  return (bme.empty() ? frequency { } : bme.front().freq());
}

/*! \brief      Get highest frequency on the bandmap
    \return     highest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
frequency bandmap::highest_frequency(void)
{ const BM_ENTRIES bme { displayed_entries() };

  return (bme.empty() ? frequency { } : bme.back().freq());
}

/// convert to a printable string
string bandmap::to_str(void)
{ string     rv;
  BM_ENTRIES raw;
  BM_ENTRIES filtered;
  BM_ENTRIES threshold_and_filtered;
  BM_ENTRIES threshold_filtered_and_culled;

  int ver;

  { SAFELOCK(_bandmap);

    raw = entries();
    filtered = filtered_entries();
    threshold_and_filtered = rbn_threshold_and_filtered_entries();
    threshold_filtered_and_culled = rbn_threshold_filtered_and_culled_entries();

    ver = version();
  }

  rv += "bandmap version " + ::to_string(ver) + EOL;

  rv += "RAW bandmap:"s + EOL;
  rv += "number of entries = "s + to_string(raw.size());

  for (const auto& be : raw)
    rv += to_string(be) + EOL;

  rv += EOL + "FILTERED bandmap:"s + EOL;

  rv += "number of entries = "s + to_string(filtered.size());

  for (const auto& be : filtered)
    rv += to_string(be) + EOL;

  rv += EOL + "THRESHOLD AND FILTERED bandmap:"s + EOL;

  rv += "number of entries = "s + to_string(threshold_and_filtered.size());

  for (const auto& be : threshold_and_filtered)
    rv += to_string(be) + EOL;

  rv += EOL + "THRESHOLD, FILTERED and CULLED bandmap:"s + EOL;

  rv += "number of entries = "s + to_string(threshold_filtered_and_culled.size());

  for (const auto& be : threshold_filtered_and_culled)
    rv += to_string(be) + EOL;

  return rv;
}

/*!  \brief                     Is a particular call present?
     \param target_callsign     callsign to test
     \return                    whether <i>target_callsign</i> is present on the bandmap
*/
bool bandmap::is_present(const string_view target_callsign) const
{ SAFELOCK(_bandmap);

  return ANY_OF(_entries, [&target_callsign] (const bandmap_entry& be) { return (be.callsign() == target_callsign); });
}

/*! \brief         Process an insertion queue, adding the elements to the bandmap
    \param biq     insertion queue to process
    \return        whether any processing actually took place (i.e., was <i>biq</i> non-empty?)

    <i>biq</i> changes (is emptied) by this routine
    other threads MUST NOT access biq while this is executing
*/
bool bandmap::process_insertion_queue(BANDMAP_INSERTION_QUEUE& biq)
{ SAFELOCK(_bandmap);

  if (!biq.empty())
  { optional<bandmap_entry> obe { };

    while (obe = biq.pop(), obe)
      *this += obe.value();         // sets dirty_entries() and _version

    return true;
  }

  return false;
}

/*! \brief          Process an insertion queue, adding the elements to the bandmap, and writing to a window
    \param  biq     insertion queue to process
    \param  w       window to which to write the revised bandmap
     
    <i>biq</i> changes (is emptied) by this routine
    other threads MUST NOT access biq while this is executing
*/
void bandmap::process_insertion_queue(BANDMAP_INSERTION_QUEUE& biq, window& w)
{ SAFELOCK(_bandmap);

  if (process_insertion_queue(biq))       // don't need to output if nothing is done
  { // could put stuff here to make the following output optional -- if, for example, the bm has been recently written, or the version hasn't been incremented
    // although if we get here, it means that the version must have been incremented, I think

    write_to_window(w);
  }
}

/*! \brief          Write a <i>bandmap</i> object to a window
    \param  win     window to which to write
    \return         the window
*/
window& bandmap::write_to_window(window& win)
{ constexpr time_t      GREEN_TIME                 { 120 };             // time in seconds for which calls are marked in green
  constexpr COLOUR_TYPE NOT_NEEDED_COLOUR          { COLOUR_BLACK };
  constexpr COLOUR_TYPE MULT_COLOUR                { COLOUR_GREEN };
  constexpr COLOUR_TYPE NOT_MULT_COLOUR            { COLOUR_BLUE };
  constexpr COLOUR_TYPE UNKNOWN_MULT_STATUS_COLOUR { COLOUR_YELLOW };
  constexpr COLOUR_TYPE MARKED_FG_COLOUR           { COLOUR_WHITE };
  constexpr COLOUR_TYPE MARKED_BG_COLOUR           { COLOUR_RED };

  static const value_map bandap_vm_cluster { _fade_colours, 0U, context.bandmap_decay_time_cluster() }; // time in minutes
  static const value_map bandap_vm_local   { _fade_colours, 0U, context.bandmap_decay_time_local() };   // time in minutes
  static const value_map bandap_vm_rbn     { _fade_colours, 0U, context.bandmap_decay_time_rbn() };     // time in minutes

  SAFELOCK(_bandmap);                                        // in case multiple threads are trying to write a bandmap to the window

//  ost << "at time " << NOW_TP() << ": request to display bandmap version " << version_str() << " for band " << BAND_NAME[_band] << "; bandmap::write_to_window backtrace: " << endl << std_backtrace(BACKTRACE::ACQUIRE) << endl;

  if (_version <= _last_displayed_version)    // not an error, but indicate that it happened, and then do nothing
  { //ost << "Attempt to write old version of bandmap: last displayed version = " << last_version_str() << "; attempted to display version " << version_str() << endl;
    return win;
  }

// we are a more recent version, so display it
  const size_t     maximum_number_of_displayable_entries { (win.width() / COLUMN_WIDTH) * win.height() };
  const BM_ENTRIES entries                               { displayed_entries() };    // automatically filter
  const size_t     start_entry                           { (entries.size() > maximum_number_of_displayable_entries) ? column_offset() * win.height() : 0u };

#if 0
  if (!entries.empty())
  { //ost << "lowest frequency in bandmap: " << entries.cbegin() -> frequency_str() << "; band from be = " << entries.cbegin() -> band() << endl;

   // auto it = entries.begin();
//
   // for (int entry_nr { 0 }; entry_nr < static_cast<int>(entries.size()); ++entry_nr)
   // { ost << "entry number " << entry_nr << ": " << (it -> frequency_str()) << ": " <<  (it -> callsign()) << endl;
//
    //  it = next(it);
   // }

  }
  else
    ost << "bandmap is EMPTY" << endl;
#endif

  win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < (bandmap_frequency_up ? WINDOW_ATTRIBUTES::CURSOR_BOTTOM_LEFT : WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT);

  size_t index { 0 };    // keep track of where we are in the bandmap

  bool      found_my_marker { false };
  frequency my_marker_qrg   { };

  for (const auto& be : entries)
  { if ( (index >= start_entry) and (index < (start_entry + maximum_number_of_displayable_entries) ) )
    { const string      entry_str     { pad_right(pad_left(be.frequency_str(), 7) + SPACE_STR + substring <std::string> (be.callsign(), 0, MAX_CALLSIGN_WIDTH), COLUMN_WIDTH) };
      const string_view frequency_str { substring <std::string_view> (entry_str, 0, 7) };
      const string_view callsign_str  { substring <std::string_view> (entry_str, 8) };
      const bool        is_marker     { be.is_marker() };

// debug: write QRG of marker
//      if (be.is_my_marker())
 //       ost << "MY MARKER: " << be.to_brief_string() << endl;

      if (!found_my_marker and be.is_my_marker())
      { found_my_marker = true;
        my_marker_qrg = be.freq();
      }
  
// change to the correct colour
      const time_t age_since_original_inserted { be.time_since_this_or_earlier_inserted() };

      long unsigned int age { 0 };
      int               clr { 0 };

      if (!is_marker)       // we don't need to change the colour of the markers
      { switch (be.source())
        { case BANDMAP_ENTRY_SOURCE::CLUSTER :
            age = be.time_since_inserted() / 60;    // age in minutes
            clr = bandap_vm_cluster[age];
            break;

          case BANDMAP_ENTRY_SOURCE::LOCAL :
            age = be.time_since_inserted() / 60;    // age in minutes
            clr = bandap_vm_local[age];
            break;

          case BANDMAP_ENTRY_SOURCE::RBN :
            age = be.time_since_inserted() / 60;    // age in minutes
            clr = bandap_vm_rbn[age];
            break;
        }
      }

      PAIR_NUMBER_TYPE cpu { colours.add(clr, win.bg()) };

// mark in GREEN if less than two minutes since the original spot at this freq was inserted
      if ( (age_since_original_inserted < GREEN_TIME) and !be.is_marker() and (recent_colour() != COLOUR_BLACK) )
        cpu = colours.add(recent_colour(), win.bg());

      if (is_marker)
        cpu = colours.add(COLOUR_WHITE, COLOUR_BLACK);    // colours for markers

// work out where to start the display of this call
      const unsigned int x { static_cast<unsigned int>( ( (index  - start_entry) / win.height()) * COLUMN_WIDTH ) };
    
// check that there's room to display the entire entry
      if ((win.width() - x) < COLUMN_WIDTH)
        break;

// get the right y ordinate
      const unsigned int y { static_cast<unsigned int>( (bandmap_frequency_up ? 0 + (index - start_entry) % win.height()
                                                                              : (win.height() - 1) - (index - start_entry) % win.height() ) ) };

// now work out the status colour
      PAIR_NUMBER_TYPE status_colour { colours.add(NOT_NEEDED_COLOUR, NOT_NEEDED_COLOUR) };                      // default

      if (!is_marker)
      { if (be.is_needed())
        { if (be.mult_status_is_known())
            status_colour = (be.is_needed_mult() ? colours.add(MULT_COLOUR, MULT_COLOUR) : colours.add(NOT_MULT_COLOUR, NOT_MULT_COLOUR));
          else
            status_colour = colours.add(UNKNOWN_MULT_STATUS_COLOUR, UNKNOWN_MULT_STATUS_COLOUR);
        }
      }

// reverse the colour of the frequency if there are unseen entries lower or higher in frequency
      const bool reverse { ( (start_entry != 0) and (start_entry == index) ) or
                             (index == (start_entry + maximum_number_of_displayable_entries - 1) and (entries.size() > (index + 1)) ) };

      const bool is_marked_entry { bandmap_show_marked_frequencies and is_marked_frequency(marked_frequency_ranges, be.mode(), be.freq()) };

// switch to red if this is a marked frequency and we are showing marked frequencies
      win < cursor(x, y) < colour_pair( is_marked_entry ? colours.add(MARKED_FG_COLOUR, MARKED_BG_COLOUR) : cpu );

      if (reverse)
        win < WINDOW_ATTRIBUTES::WINDOW_REVERSE;

      win < frequency_str;

      if (reverse)
        win < WINDOW_ATTRIBUTES::WINDOW_NORMAL;

      win < colour_pair(status_colour) < SPACE_STR
          < colour_pair(cpu) < callsign_str;
    }

    index++;
  }

  win.refresh();    // force the visual update ... do we really want this?

  _last_displayed_version = static_cast<int>(_version);    // operator= is deleted
  _time_last_displayed = NOW_TP();

  last_update_frequency = (my_marker_qrg + MY_MARKER_BIAS);    // store as actual QRG rather than the marker QRG

  return win;
}

/*!  \brief             Add a call to the do-not-add list
     \param callsign    callsign or regex to add

     Calls in the do-not-add list are never added to the bandmap
*/
void bandmap::do_not_add(const string_view callsign)
{ SAFELOCK(_bandmap);

  if (_is_regex(callsign))
    _do_not_add_regex += { callsign, regex( string { callsign } ) };
  else
    _do_not_add += callsign;

// technically, we have changed the bandmap object; but we haven't changed anything visible, so don't mark dirty_entries() or _version
}

/*!  \brief             Remove a call from the do-not-add list
     \param callsign    callsign or regex to remove

     Calls in the do-not-add list are never added to the bandmap
*/
void bandmap::remove_from_do_not_add(const string_view callsign)
{ SAFELOCK(_bandmap);

  if (_is_regex(callsign))
    _do_not_add_regex -= callsign;      // removes the element with the key = callsign, if one exists
  else
    _do_not_add -= callsign;

// technically, we have changed the bandmap object; but we haven't changed anything visible, so don't mark dirty_entries() or _version
}

/*! \brief              Return all calls in the bandmap that match a regex string
    \param  regex_str   the target regex string

    See: https://www.reddit.com/r/cpp/comments/aqt7a0/status_of_string_view_and_regex/
*/
vector<string> bandmap::regex_matches(const string_view regex_str)
{ const BM_ENTRIES bme { displayed_entries() };
  const regex      rgx { string { regex_str } };

  smatch         base_match;
  vector<string> rv;

  FOR_ALL(bme, [&base_match, &rgx, &rv] (const bandmap_entry& displayed_entry) { if (const string& callsign { displayed_entry.callsign() }; regex_match(callsign, base_match, rgx))
                                                                                   rv += callsign;
                                                                               } );

  return rv;
}

/*! \brief              Return all calls in the bandmap that match a regex string
    \param  regex_str   the target regex string

    See: https://www.reddit.com/r/cpp/comments/aqt7a0/status_of_string_view_and_regex/
*/
vector<string> bandmap::regex_matches(const string& regex_str)
{ const BM_ENTRIES bme { displayed_entries() };
  const regex      rgx { regex_str };

  smatch         base_match;
  vector<string> rv;

  FOR_ALL(bme, [&base_match, &rgx, &rv] (const bandmap_entry& displayed_entry) { if (const string& callsign { displayed_entry.callsign() }; regex_match(callsign, base_match, rgx))
                                                                                   rv += callsign;
                                                                               } );

  return rv;
}

/*! \brief          Write a <i>bandmap</i> object to an output stream
    \param  ost     output stream
    \param  bm      object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, bandmap& bm)
{ ost << bm.to_str();

  return ost;
}
