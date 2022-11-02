// $Id: bandmap.cpp 210 2022-10-31 17:26:13Z  $

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
#include "log_message.h"
#include "statistics.h"
#include "string_functions.h"

#include <fstream>
#include <functional>

using namespace std;

extern pt_mutex                      batch_messages_mutex;              ///< mutex for batch messages
extern unordered_map<string, string> batch_messages;                    ///< batch messages associated with calls
extern bandmap_buffer                bm_buffer;                         ///< global control buffer for all the bandmaps
extern bool                          bandmap_show_marked_frequencies;   ///< whether to display entries that would be marked
extern bool                          bandmap_frequency_up;              ///< whether increasing frequency goes upwards in the bandmap
//extern drlog_context                 context;                           ///< context
extern exchange_field_database       exchange_db;                       ///< dynamic database of exchange field values for calls; automatically thread-safe
extern location_database             location_db;                       ///< location information
extern unsigned int                  max_qsos_without_qsl;              ///< limit for the N7DR matches_criteria() algorithm
extern map<MODE, vector<pair<frequency, frequency>>> marked_frequency_ranges;
extern message_stream                ost;                               ///< debugging/logging output

extern const set<string> CONTINENT_SET;                 ///< two-letter abbreviations for all the continents

extern bool is_marked_frequency(const map<MODE, vector<pair<frequency, frequency>>>& marked_frequency_ranges, const MODE m, const frequency f); ///< Is a particular frequency within any marked range?

/*! \brief                      Obtain value corresponding to a type of callsign mult from a callsign
    \param  callsign_mult_name  the type of the callsign mult
    \param  callsign            the call for which the mult value is required

    Returns the empty string if no sensible result can be returned
*/
extern const string callsign_mult_value(const string& callsign_mult_name, const string& callsign);

constexpr unsigned int  MAX_CALLSIGN_WIDTH { 11 };        ///< maximum width of a callsign in the bandmap window
constexpr unsigned int  MAX_FREQUENCY_SKEW { 250 };       ///< maximum separation, in hertz, to be treated as same frequency

constexpr int MY_MARKER_BIAS { 1 };                       /// shift (downward), in Hz, that is applied to MY_MARKER before inserting it 

const string        MODE_MARKER { "********"s };          ///< string to mark the mode break in the bandmap
const string        MY_MARKER   { "--------"s };          ///< the string that marks my position in the bandmap

bandmap_filter_type BMF;                            ///< the global bandmap filter

/*! \brief          Printable version of the name of a bandmap_entry source
    \param  bes     source of a bandmap entry
    \return         printable version of <i>bes</i>
*/
string to_string(const BANDMAP_ENTRY_SOURCE bes)
{ switch (bes)
  { case BANDMAP_ENTRY_SOURCE::LOCAL :
      return "BANDMAP_ENTRY_LOCAL"s;

    case BANDMAP_ENTRY_SOURCE::CLUSTER :
      return "BANDMAP_ENTRY_CLUSTER"s;

    case BANDMAP_ENTRY_SOURCE::RBN :
      return "BANDMAP_ENTRY_RBN"s;

    default :
      return "UNKNOWN"s;
  }
}

// -----------   bandmap_buffer ----------------

/*! \class  bandmap_buffer
    \brief  Class to control which cluster/RBN posts reach the bandmaps

    A single bandmap_buffer is used to control all bandmaps
*/

/*! \brief              Get the number of posters associated with a call
    \param  callsign    the callsign to test
    \return             The number of posters associated with <i>callsign</i>
*/
unsigned int bandmap_buffer::n_posters(const string& callsign) const
{ SAFELOCK(_bandmap_buffer);

//  const auto cit { _data.find(callsign) };

//  return ( ( cit == _data.end() ) ? 0 : cit->second.size() );

// std::map<std::string /* call */, bandmap_buffer_entry>  _data;

  return MUMF_VALUE(_data, callsign, &bandmap_buffer_entry::size);  // is this correct??? I think that it is
}

/*! \brief              Associate a poster with a call
    \param  callsign    the callsign
    \param  poster      poster to associate with <i>callsign</i>
    \return             The number of posters associated with <i>callsign</i>, after this association is added

    Creates an entry in the buffer if no entry for <i>callsign</i> exists
*/
unsigned int bandmap_buffer::add(const string& callsign, const string& poster)
{ SAFELOCK(_bandmap_buffer);

  return _data[callsign].add(poster);
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
void bandmap_filter_type::add_or_subtract(const string& str)
{ const bool   is_continent { CONTINENT_SET.contains(str) };
  const string str_copy     { is_continent ? str : location_db.info(str).canonical_prefix() };  // convert to canonical prefix

  vector<string>* vs_p { ( is_continent ? &_continents : &_prefixes ) };        // create pointer to correct vector
  set<string> ss;                                                                        // temporary place to build new container of strings

  for_each(vs_p->cbegin(), vs_p->cend(), [&ss] (const string& continent_or_prefix) { ss += continent_or_prefix; } );  // create a copy of current values

  if (ss.contains(str_copy))              // remove a value
    ss -= str_copy;
  else                       // add a value
    ss += str_copy;

  vs_p->clear();                                        // empty it
  copy(ss.begin(), ss.end(), back_inserter(*vs_p));     // copy the new strings to the correct destination
  SORT(*vs_p, compare_calls);                           // make it easy for humans
}

// -----------  bandmap_entry  ----------------

/*! \brief          Set the callsign
    \param  call    the callsign to set
*/
bandmap_entry& bandmap_entry::callsign(const string& call)
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
bandmap_entry& bandmap_entry::freq(const frequency& f)
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
*/
void bandmap_entry::calculate_mult_status(contest_rules& rules, running_statistics& statistics)
{
// callsign mult
  clear_callsign_mult();

  const set<string> callsign_mults { rules.callsign_mults() };

  for (const auto& callsign_mult_name : callsign_mults)
  { const string callsign_mult_val { callsign_mult_value(callsign_mult_name, _callsign) };

    if (!callsign_mult_val.empty())
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

      const string guess { rules.canonical_value(exch_mult_name, exchange_db.guess_value(_callsign, exch_mult_name)) };

      if (!guess.empty())
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
      2. worked and QSLed on this band/mode;
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

  if (olog.n_qsls(_callsign) and olog.n_qsos(_callsign, _band, _mode) <= max_qsos_without_qsl)
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
      << "is needed exchange mult: " << be.is_needed_exchange_mult_details() << endl
      << "mode: " << MODE_NAME[be.mode()] << endl
      << "mult status is known: " << be.mult_status_is_known() << endl
      << "putative mode: " << MODE_NAME[be.putative_mode()] << endl
      << "source: " << to_string(be.source()) << endl
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
     \return                            callsign of a station within the guard band

     Returns the nearest station within the guard band, or the null string if no call is found.
     As currently implemented, assumes that the entries are in order of monotonically increasing or decreasing frequency
*/
string bandmap::_nearest_callsign(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz)
{ if ( (target_frequency_in_khz < 1800) or (target_frequency_in_khz > 29700) )
  { ost << "WARNING: bandmap::_nearest_callsign called with frequency in kHz = " << target_frequency_in_khz << endl;
    return string();
  }

  const float guard_band_in_khz { static_cast<float>(guard_band_in_hz) / 1000.0f };

  bool  finish_looking      { false };
  float smallest_difference { 1'000'000};              // start with a big number

  string rv;

  for (BM_ENTRIES::const_iterator cit { bme.cbegin() }; (!finish_looking and cit != bme.cend()); ++cit)
  { const float difference     { cit->freq().kHz() - target_frequency_in_khz };
    const float abs_difference { fabs(difference) };

    if ( (abs_difference <= guard_band_in_khz) and (!cit->is_my_marker()))
    { if (abs_difference < smallest_difference)
      { smallest_difference = abs_difference;
        rv = cit->callsign();
      }
    }

    if (difference > guard_band_in_khz)  // no need to keep looking we if are past the allowed guard band
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

    my_marker_copy.freq(frequency(my_marker_copy.freq().hz() - MY_MARKER_BIAS));     // make it 1Hz less than actual value 
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
}

/*!  \brief     Mark filtered and rbn/filtered entries as dirty
*/
void bandmap::_dirty_entries(void)
{ SAFELOCK (_bandmap);              // should be unnecessary, since if the entries are dirty we should already have the lock

  _filtered_entries_dirty = true;
  _rbn_threshold_and_filtered_entries_dirty = true;
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

  if (be.absolute_frequency_difference(old_be) > MAX_FREQUENCY_SKEW)       // treat anything within 250 Hz as the same frequency
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
{ const bool    mode_marker_is_present { (_mode_marker_frequency.hz() != 0) };
  const string& callsign               { be.callsign() };

// do not add if it's already been done recently, or matches several other conditions
  bool add_it { !_do_not_add.contains(callsign) };

  if (add_it)
    add_it = be.freq().is_within_ham_band();

  if (add_it)
    add_it = !((be.source() != BANDMAP_ENTRY_SOURCE::LOCAL) and is_recent_call(callsign));

// could make this more efficient by having a global container of the mode-marker bandmap entries
  if (add_it and mode_marker_is_present)
  { const bandmap_entry mode_marker_be { (*this)[MODE_MARKER] };    // assumes only one mode marker

    add_it = (be.absolute_frequency_difference(mode_marker_be) > MAX_FREQUENCY_SKEW);
  }

  if (add_it)
  { const bool mark_as_recent { _mark_as_recent(be) };  // keep track of whether we're going to mark this as a recent call

    bandmap_entry old_be;

    SAFELOCK(_bandmap);

    if (be.source() == BANDMAP_ENTRY_SOURCE::RBN)
    { old_be = (*this)[callsign];

      if (old_be.valid())
      { if (be.absolute_frequency_difference(old_be) > MAX_FREQUENCY_SKEW)  // add only if more than 250 Hz away
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
            old_be.expiration_time(be.expiration_time());

            (*this) -= callsign;
            be.time_of_earlier_bandmap_entry(old_be);    // could change be

            _insert(old_be);
          }
        }
      }
      else    // this call is not currently present
      { _entries.remove_if([=] (bandmap_entry& bme) { return ((bme.frequency_str() == be.frequency_str()) and (bme.is_not_marker())); } );  // remove any real entries at this QRG
        _insert(be);
      }

// possibly remove all the other entries at this QRG
      if (be.is_not_marker())
      { const bandmap_entry current_be { (*this)[callsign] };  // the entry in the updated bandmap

        { _entries.remove_if([=] (bandmap_entry& bme) { bool rv { bme.is_not_marker() };

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
    { _entries.remove_if([=] (bandmap_entry& bme) { return bme.matches_bandmap_entry(be); } );
      _insert(be);
    }

    if (be.is_not_marker() and mark_as_recent)
      _recent_calls += callsign;

    _dirty_entries();
  }

  if (mode_marker_is_present and !is_present(MODE_MARKER))
    ost << "*** ERROR: MODE MARKER HAS BEEN REMOVED BY BANDMAP_ENTRY: " << be << endl;
}

/// prune the bandmap
void bandmap::prune(void)
{ SAFELOCK(_bandmap);                                   // hold the lock for the entire process

  const time_t now          { ::time(NULL) };             // get the time from the kernel
  const size_t initial_size { _entries.size() };

  _entries.remove_if([=] (const bandmap_entry& be) { return (be.should_prune(now)); });  // OK for lists

  if (_entries.size() != initial_size)
    _dirty_entries();

  _recent_calls.clear();                       // empty the container of recent calls
}

/*! \brief              Return the entry for a particular call
    \param  callsign    call for which the entry should be returned
    \return             the bandmap_entry corresponding to <i>callsign</i>

    Returns the default bandmap_entry if <i>callsign</i> is not present in the bandmap
*/
bandmap_entry bandmap::operator[](const string& str)
{ SAFELOCK(_bandmap);

  return VALUE_IF(_entries, [=] (const bandmap_entry& be) { return (be.callsign() == str); });
}

/*! \brief              Return the first entry for a partial call
    \param  callsign    partial call for which the entry should be returned
    \return             the first bandmap_entry corresponding to <i>callsign</i>

    Returns the null string if <i>callsign</i> matches no entries in the bandmap
*/
bandmap_entry bandmap::substr(const string& str)
{ SAFELOCK(_bandmap);

  return VALUE_IF(_entries, [=] (const bandmap_entry& be) { return contains(be.callsign(), str); });
}

/*! \brief              Remove a call from the bandmap
    \param  callsign    call to be removed

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::operator-=(const string& callsign)
{ SAFELOCK(_bandmap);

  const size_t initial_size { _entries.size() };

  _entries.remove_if([=] (const bandmap_entry& be) { return (be.callsign() == callsign); });        // OK for lists

  if (_entries.size() != initial_size)  // mark as dirty if we removed any
    _dirty_entries();
}

/*! \brief              Set the needed status of a call to <i>false</i>
    \param  callsign    call for which the status should be set

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::not_needed(const string& callsign)
{ SAFELOCK(_bandmap);

  bandmap_entry be { (*this)[callsign] };

  if (!be.callsign().empty())        // did we get an entry?
  { be.is_needed(false);
    (*this) += be;                   // this will remove the pre-existing entry
  }

  _dirty_entries();
}

/*! \brief                      Set the needed country mult status of all calls in a particular country to false
    \param  canonical_prefix    canonical prefix corresponding to country for which the status should be set

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
*/
void bandmap::not_needed_country_mult(const string& canonical_prefix)
{ SAFELOCK(_bandmap);

  FOR_ALL(_entries, [&canonical_prefix] (decltype(*_entries.begin())& be) { be.remove_country_mult(canonical_prefix); } );

  _dirty_entries();
}

/*! \brief                          Set the needed callsign mult status of all matching callsign mults to <i>false</i>
    \param  pf                      pointer to function to return the callsign mult value
    \param  mult_type               name of mult type
    \param  callsign_mult_string    value of callsign mult value that is no longer a multiplier
*/
void bandmap::not_needed_callsign_mult(/*const*/ std::string (*pf)(const std::string& /* e.g. "WPXPX" */, const std::string& /* callsign */),
                                       const std::string& mult_type /* e.g., "WPXPX" */,
                                       const std::string& callsign_mult_string /* e.g., SM1 */)
{ if (callsign_mult_string.empty() or mult_type.empty())
    return;

  SAFELOCK(_bandmap);

// change status for all entries with this particular callsign mult
  for (auto& be : _entries)
  { if (be.is_needed_callsign_mult())
    { const string& callsign           { be.callsign() };
      const string  this_callsign_mult { (*pf)(mult_type, callsign) };

      if (this_callsign_mult == callsign_mult_string)
      { be.remove_callsign_mult(mult_type, callsign_mult_string);
        _dirty_entries();
      }
    }
  }
}

/*! \brief              Set the needed exchange mult status of a particular exchange mult to <i>false</i>
    \param  mult_name   name of exchange mult
    \param  mult_value  value of <i>mult_name</i> that is no longer a multiplier
*/
void bandmap::not_needed_exchange_mult(const string& mult_name, const string& mult_value)
{ if (mult_name.empty() or mult_value.empty())
    return;

  SAFELOCK(_bandmap);

  FOR_ALL(_entries, [=] (bandmap_entry& be) { be.remove_exchange_mult(mult_name, mult_value); } );

  _dirty_entries();
}

/*! \brief          Enable or disable the filter
    \param  torf    whether to enable the filter

    Disables the filter if <i>torf</i> is false.
*/
void bandmap::filter_enabled(const bool torf)
{ if (torf != filter_enabled())
  { SAFELOCK(_bandmap);

    _filter_p->enabled(torf);
    _dirty_entries();
  }
}

/*!  \brief         Add a string to, or remove a string from, the filter associated with this bandmap
     \param str     string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed. Currently, all bandmaps share a single
     filter.
*/
void bandmap::filter_add_or_subtract(const string& str)
{ if (!str.empty())
  { SAFELOCK(_bandmap);

    _filter_p->add_or_subtract(str);
    _dirty_entries();
  }
}

/*!  \brief         Set or unset the filter to hide mode (as opposed to show)
     \param torf    whether to set to hide mode
*/
void bandmap::filter_hide(const bool torf)
{ if (torf != filter_hide())
  { SAFELOCK(_bandmap);

    _filter_p->hide(torf);
    _dirty_entries();
  }
}

/*!  \brief         Set or unset the filter to show mode (as opposed to hide)
     \param torf    whether to set to show mode
*/
void bandmap::filter_show(const bool torf)
{ if (torf != filter_show())
  { SAFELOCK(_bandmap);

    _filter_p->hide(!torf);
    _dirty_entries();
  }
}

/// all the entries, after filtering has been applied
BM_ENTRIES bandmap::filtered_entries(void)
{ if (!filter_enabled())
    return entries();

  { SAFELOCK (_bandmap);

    if (!_filtered_entries_dirty)
      return _filtered_entries;
  }

  SAFELOCK(_bandmap);

  const BM_ENTRIES tmp { entries() };

  BM_ENTRIES rv;

  for (const auto& be : tmp)
  { if (be.is_my_marker() or be.is_mode_marker())
      rv += be;
    else                                              // start by assuming that we are in show mode
    { bool display_this_entry { contains(_filter_p->continents(), be.continent()) or contains(_filter_p->prefixes(),  be.canonical_prefix()) };

      if (filter_hide())                              // hide is the opposite of show
        display_this_entry = !display_this_entry;

      if (display_this_entry)
        rv += be;
    }
  }

  _filtered_entries = move(rv);
  _filtered_entries_dirty = false;

  return _filtered_entries;
}

/// all the entries, after the RBN threshold and filtering have been applied
BM_ENTRIES bandmap::rbn_threshold_and_filtered_entries(void)
{
  { SAFELOCK (_bandmap);

    if (!_rbn_threshold_and_filtered_entries_dirty)
      return _rbn_threshold_and_filtered_entries;
  }

  BM_ENTRIES filtered { filtered_entries() };  // splice is going to change this

  BM_ENTRIES rv;

  rv.splice(rv.end(), filtered);

  SAFELOCK(_bandmap);

  _rbn_threshold_and_filtered_entries = move(rv);
  _rbn_threshold_and_filtered_entries_dirty = false;

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
    { BM_ENTRIES rv { rbn_threshold_and_filtered_entries() };       // only slow if dirty, although does perform copy

 //     REMOVE_IF_AND_RESIZE(rv, [] (bandmap_entry& be) { return ( !( be.is_marker() or be.matches_criteria() ) ); });
      erase_if(rv, [] (bandmap_entry& be) { return ( !( be.is_marker() or be.matches_criteria() ) ); });

      return rv;
    }

    case 2 :                                                        // new on this band+mode
    { BM_ENTRIES rv { rbn_threshold_and_filtered_entries() };       // only slow if dirty, although does perform copy

 //     REMOVE_IF_AND_RESIZE(rv, [] (bandmap_entry& be) { return ( !( be.is_marker() or be.is_all_time_first_and_needed_qso() ) ); });
      erase_if(rv, [] (bandmap_entry& be) { return ( !( be.is_marker() or be.is_all_time_first_and_needed_qso() ) ); });

      return rv;
    }

    case 3 :                                                        // never worked anywhere
    { BM_ENTRIES rv { rbn_threshold_and_filtered_entries() };       // only slow if dirty, although does perform copy

 //     REMOVE_IF_AND_RESIZE(rv, [] (bandmap_entry& be) { return ( !( be.is_marker() or (olog.n_qsos(be.callsign()) == 0) ) ); });
      erase_if(rv, [] (bandmap_entry& be) { return ( !( be.is_marker() or (olog.n_qsos(be.callsign()) == 0) ) ); });

      return rv;
    }    
  }
}

/*!  \brief         Find the next needed station up or down in frequency from the current location
     \param fp      pointer to function to be used to determine whether a station is needed
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found.
     Applies filtering and the RBN threshold before searching for the next station.

     Should perhaps use guard band instead of MAX_FREQUENCY_SKEW
*/
bandmap_entry bandmap::needed(PREDICATE_FUN_P fp, const enum BANDMAP_DIRECTION dirn)
{ SAFELOCK(_bandmap);    // hold the lock so nothing changes while we scan the bandmap

  const int        max_permitted_skew { 95 };                   // 95 Hz
  const BM_ENTRIES fe                 { displayed_entries() };

// why can't this be const?
  auto marker_it { FIND_IF(fe, [] (const bandmap_entry& be) { return (be.is_my_marker()); } ) };  // find myself

  if (marker_it == fe.cend())             // should never be true
    return bandmap_entry();

  const frequency target_freq { marker_it->freq() };

  if (dirn == BANDMAP_DIRECTION::DOWN)
  { const auto crit  { prev(reverse_iterator<decltype(marker_it)>(marker_it)) };             // Josuttis first ed. p. 66f.
    const auto crit2 { find_if(crit, fe.crend(), [=] (const bandmap_entry& be) { return ( be.freq().hz() < (target_freq.hz() - max_permitted_skew) ); } ) }; // move away from my frequency, in downwards direction

    if (crit2 != fe.crend())
    { if (const auto crit3 { find_if(crit2, fe.crend(), [=] (const bandmap_entry& be) { return (be.*fp)(); } ) }; crit3 != fe.crend())
        return (*crit3);
    }
  }

  if (dirn == BANDMAP_DIRECTION::UP)
  { const auto cit2 { find_if(marker_it, fe.cend(), [=] (const bandmap_entry& be) { return (be.freq().hz() > (target_freq.hz() + max_permitted_skew) ); }) }; // move away from my frequency, in upwards direction

    if (cit2 != fe.cend())
    { const auto cit3 { find_if(cit2, fe.cend(), [=] (const bandmap_entry& be) { return (be.*fp)(); }) };

      if (cit3 != fe.cend())
        return (*cit3);
    }
  }

  return bandmap_entry();
}

/*! \brief          Find the next station up or down in frequency from a given frequency
    \param  f       starting frequency
    \param  dirn    direction in which to search
    \return         bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

    The return value can be tested with .empty() to see if a station was found.
    Applies filtering and the RBN threshold before searching for the next station.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
bandmap_entry bandmap::next_station(const frequency& f, const enum BANDMAP_DIRECTION dirn)
{ bandmap_entry rv;

  const BM_ENTRIES fe { displayed_entries() };

  if (fe.empty())
    return rv;

  if ( (dirn == BANDMAP_DIRECTION::DOWN and f <= fe.front().freq()) or (dirn == BANDMAP_DIRECTION::UP and f >= fe.back().freq()) )
    return rv;

// the logic here (for DOWN; UP is just the inverse) is to mark the highest frequency
// and then step down through the bandmap; if bm freq is >= the target then mark the
// frequency and keep going; if bm freq < the target, return the most recently marked
// frequency

  switch (dirn)
  { case BANDMAP_DIRECTION::DOWN :
      if (f <= fe.front().freq())         // all frequencies are higher than the target
        return rv;

      rv = fe.front();

      for (BM_ENTRIES::const_iterator cit { next(fe.cbegin()) }; cit != fe.cend(); ++cit)
      { if (cit->freq() >= f)
          return rv;

        rv = *cit;
      }

      return rv;            // get here only if all frequencies are above the target

    case BANDMAP_DIRECTION::UP :
      if (f >= fe.back().freq())         // all frequencies are lower than the target
        return rv;

      rv = fe.back();

      for (BM_ENTRIES::const_reverse_iterator crit { next(fe.crbegin()) }; crit != fe.crend(); ++crit)
      { if (crit->freq() <= f)
          return rv;

        rv = *crit;
      }
      
      return rv;            // get here only if all frequencies are below the target
  }

  return bandmap_entry();       // keep compiler happy
}

/*! \brief      Get lowest frequency on the bandmap
    \return     lowest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
frequency bandmap::lowest_frequency(void)
{ const BM_ENTRIES bme { displayed_entries() };

  return (bme.empty() ? frequency() : bme.front().freq());
}

/*! \brief      Get highest frequency on the bandmap
    \return     highest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
frequency bandmap::highest_frequency(void)
{ const BM_ENTRIES bme { displayed_entries() };

  return (bme.empty() ? frequency() : bme.back().freq());
}

/// convert to a printable string
string bandmap::to_str(void)
{ string rv;
  BM_ENTRIES raw;
  BM_ENTRIES filtered;
  BM_ENTRIES threshold_and_filtered;
  BM_ENTRIES threshold_filtered_and_culled;

  { SAFELOCK(_bandmap);

    raw = entries();
    filtered = filtered_entries();
    threshold_and_filtered = rbn_threshold_and_filtered_entries();
    threshold_filtered_and_culled = rbn_threshold_filtered_and_culled_entries();
  }

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
bool bandmap::is_present(const string& target_callsign) const
{ SAFELOCK(_bandmap);

  return ANY_OF(_entries, [target_callsign] (const bandmap_entry& be) { return (be.callsign() == target_callsign); });
}

/*! \brief         Process an insertion queue, adding the elements to the bandmap
    \param biq     insertion queue to process
     
    <i>biq</i> changes (is emptied) by this routine
    other threads MUST NOT access biq while this is executing
*/
void bandmap::process_insertion_queue(BANDMAP_INSERTION_QUEUE& biq)
{ SAFELOCK(_bandmap);

  while (!biq.empty())
  { *this += biq.front();

    biq.pop();
  }
}

/*! \brief          Process an insertion queue, adding the elements to the bandmap, and writing to a window
    \param  biq     insertion queue to process
    \param  w       window to which to write the revised bandmap
     
    <i>biq</i> changes (is emptied) by this routine
    other threads MUST NOT access biq while this is executing
*/
void bandmap::process_insertion_queue(BANDMAP_INSERTION_QUEUE& biq, window& w)
{ process_insertion_queue(biq);

  w <= (*this);
}

/*! \brief          Write a <i>bandmap</i> object to a window
    \param  win     window to which to write
    \return         the window
*/
window& bandmap::write_to_window(window& win)
{ constexpr COLOUR_TYPE NOT_NEEDED_COLOUR          { COLOUR_BLACK };
  constexpr COLOUR_TYPE MULT_COLOUR                { COLOUR_GREEN };
  constexpr COLOUR_TYPE NOT_MULT_COLOUR            { COLOUR_BLUE };
  constexpr COLOUR_TYPE UNKNOWN_MULT_STATUS_COLOUR { COLOUR_YELLOW };

  const size_t maximum_number_of_displayable_entries { (win.width() / COLUMN_WIDTH) * win.height() };

  SAFELOCK(_bandmap);                                        // in case multiple threads are trying to write a bandmap to the window

  BM_ENTRIES entries { displayed_entries() };    // automatically filter

  const size_t start_entry { (entries.size() > maximum_number_of_displayable_entries) ? column_offset() * win.height() : 0u };

  win < WINDOW_ATTRIBUTES::WINDOW_CLEAR < (bandmap_frequency_up ? WINDOW_ATTRIBUTES::CURSOR_BOTTOM_LEFT : WINDOW_ATTRIBUTES::CURSOR_TOP_LEFT);

  size_t index { 0 };    // keep track of where we are in the bandmap

  for (const auto& be : entries)
  { if ( (index >= start_entry) and (index < (start_entry + maximum_number_of_displayable_entries) ) )
    { const string entry_str     { pad_right(pad_left(be.frequency_str(), 7) + SPACE_STR + substring(be.callsign(), 0, MAX_CALLSIGN_WIDTH), COLUMN_WIDTH) };
      const string frequency_str { substring(entry_str, 0, 7) };
      const string callsign_str  { substring(entry_str, 8) };
      const bool   is_marker     { be.is_marker() };
  
// change to the correct colour
      const time_t age_since_original_inserted { be.time_since_this_or_earlier_inserted() };
      const time_t age_since_this_inserted     { be.time_since_inserted() };
      const time_t start_time                  { be.time() };                  // time it was inserted
      const time_t expiration_time             { be.expiration_time() };
      const float  fraction                    { static_cast<float>(age_since_this_inserted) / (expiration_time - start_time) };
      const int    n_colours                   { static_cast<int>(fade_colours().size()) };
      const float  interval                    { (1.0f / static_cast<float>(n_colours)) };
      const int    n_intervals                 { min(static_cast<int>(fraction / interval), n_colours - 1) };

      PAIR_NUMBER_TYPE cpu { colours.add(fade_colours().at(n_intervals), win.bg()) };

// mark in GREEN if less than two minutes since the original spot at this freq was inserted
      if (age_since_original_inserted < 120 and !be.is_marker() and (recent_colour() != /* 16 */ COLOUR_BLACK))
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
                             (index == (start_entry + maximum_number_of_displayable_entries - 1) and (entries.size() > (index + 1))) };

      const bool is_marked_entry { bandmap_show_marked_frequencies and is_marked_frequency(marked_frequency_ranges, be.mode(), be.freq()) };

// switch to red if marked frequency and we are showing marked frequencies
      win < cursor(x, y) < colour_pair( is_marked_entry ? colours.add(COLOUR_WHITE, COLOUR_RED) : cpu );

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

  return win;
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
