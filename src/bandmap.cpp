// $Id: bandmap.cpp 142 2018-01-01 20:56:52Z  $

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

extern pt_mutex                 bandmap_mutex;      ///< used when writing to the bandmap window
extern bandmap_buffer           bm_buffer;          ///< global control buffer for all the bandmaps
extern exchange_field_database  exchange_db;        ///< dynamic database of exchange field values for calls; automatically thread-safe
extern location_database        location_db;        ///< location information
extern message_stream           ost;                ///< debugging/logging output

extern const set<string> CONTINENT_SET;             ///< two-letter abbreviations for all the continents

/*! \brief                      Obtain value corresponding to a type of callsign mult from a callsign
    \param  callsign_mult_name  the type of the callsign mult
    \param  callsign            the call for which the mult value is required

    Returns the empty string if no sensible result can be returned
*/
extern const string callsign_mult_value(const string& callsign_mult_name, const string& callsign);

const string        MY_MARKER("--------");          ///< the string that marks my position in the bandmap
const string        MODE_MARKER("********");        ///< string to mark the mode break in the bandmap
const unsigned int  MAX_FREQUENCY_SKEW = 250;       ///< maximum separation, in hertz, to be treated as same frequency

bandmap_filter_type BMF;                            ///< the global bandmap filter

/*! \brief          Printable version of the name of a bandmap_entry source
    \param  bes     source of a bandmap entry
    \return         printable version of <i>bes</i>
*/
const string to_string(const BANDMAP_ENTRY_SOURCE bes)
{ switch (bes)
  { case BANDMAP_ENTRY_LOCAL :
      return "BANDMAP_ENTRY_LOCAL";

    case BANDMAP_ENTRY_CLUSTER :
      return "BANDMAP_ENTRY_CLUSTER";

    case BANDMAP_ENTRY_RBN :
      return "BANDMAP_ENTRY_RBN";

    default :
      return "UNKNOWN";
  }
}

// -----------   bandmap_buffer_entry ----------------

/*! \class  bandmap_buffer_entry
    \brief  Class for entries in the bandmap_buffer class
*/

const unsigned int bandmap_buffer_entry::add(const std::string& new_poster)
{ _posters.insert(new_poster);

  return _posters.size();
}

// -----------   bandmap_buffer ----------------

/*! \class  bandmap_buffer
    \brief  Class to control which cluster/RBN posts reach the bandmaps

    A single bandmap_buffer is used to control all bandmaps
*/

/// default constructor
bandmap_buffer::bandmap_buffer(const unsigned int n_posters) :
  _min_posters(n_posters)
{ }

/*! \brief              Get the number of posters associated with a call
    \param  callsign    the callsign to test
    \return             The number of posters associated with <i>callsign</i>
*/
const unsigned int bandmap_buffer::n_posters(const string& callsign)
{ SAFELOCK(_bandmap_buffer);

  const auto cit = _data.find(callsign);

  return ( ( cit == _data.end() ) ? 0 : cit->second.size() );
}

/*! \brief              Associate a poster with a call
    \param  callsign    the callsign
    \param  poster      poster to associate with <i>callsign</i>
    \return             The number of posters associated with <i>callsign</i>, after this association is added

    Creates an entry in the buffer if no entry for </i>callsign</i> exists
*/
const unsigned int bandmap_buffer::add(const string& callsign, const string& poster)
{ SAFELOCK(_bandmap_buffer);

  bandmap_buffer_entry& bfe = _data[callsign];

  return bfe.add(poster);
}

// -----------   bandmap_filter_type ----------------

/*! \class  bandmap_filter_type
    \brief  Control bandmap filtering
*/

/*! \brief      All the continents and canonical prefixes that are currently being filtered
    \return     all the continents and canonical prefixes that are currently being filtered

    The continents precede the canonical prefixes
*/
const vector<string> bandmap_filter_type::filter(void) const
{ vector<string> rv = _continents;

  rv.insert(rv.end(), _prefixes.begin(), _prefixes.end());

  return rv;
}

/*!  \brief         Add a string to, or remove a string from, the filter
     \param str     string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed.
*/
void bandmap_filter_type::add_or_subtract(const string& str)
{ vector<string>* vs_p = ( (CONTINENT_SET < str) ? &_continents : &_prefixes );          // create pointer to correct vector
  set<string> ss;                                                                        // temporary place to build new container of strings

  for_each(vs_p->cbegin(), vs_p->cend(), [&ss] (const string& continent_or_prefix) { ss.insert(continent_or_prefix); } );  // create a copy of current values

  if (ss < str)              // remove a value
    ss.erase(str);
  else                       // add a value
    ss.insert(str);

  vs_p->clear();                                        // empty it
  copy(ss.begin(), ss.end(), back_inserter(*vs_p));     // copy the new strings to the correct destination
  sort((*vs_p).begin(), (*vs_p).end(), compare_calls);  // make it easy for humans
}

// -----------  bandmap_entry  ----------------

/*! \brief      Default constructor
    \param  s   source of the entry (default is BANDMAP_ENTRY_LOCAL)
*/
bandmap_entry::bandmap_entry(const BANDMAP_ENTRY_SOURCE s) :
  _expiration_time(0),
  _is_needed(true),
  _mult_status_is_known(false),
  _source(s),
  _time(::time(NULL)),
  _time_of_earlier_bandmap_entry(0)
{ }

/*! \brief          Set the callsign
    \param  call    the callsign to set
*/
void bandmap_entry::callsign(const string& call)
{ _callsign = call;

//  if (_callsign != MY_MARKER)
  if (!is_marker())
  { const location_info li = location_db.info(_callsign);

    _canonical_prefix = li.canonical_prefix();
    _continent = li.continent();
  }
}

/*! \brief      Set <i>_freq</i>, <i>_frequency_str</i>, <i>_band</i> and <i>_mode</i>
    \param  f   frequency used to set the values
*/
void bandmap_entry::freq(const frequency& f)
{ _freq = f;
  _frequency_str = _freq.display_string();
  _band = to_BAND(f);
  _mode = putative_mode();
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

  const set<string> callsign_mults = rules.callsign_mults();

  for (const auto& callsign_mult_name : callsign_mults)
  { const string callsign_mult_val = callsign_mult_value(callsign_mult_name, _callsign);

    if (!callsign_mult_val.empty())
    { if (statistics.is_needed_callsign_mult(callsign_mult_name, callsign_mult_val, _band, _mode))
        add_callsign_mult(callsign_mult_name, callsign_mult_val);
    }
  }

  _is_needed_callsign_mult.status_is_known(true);

// country mult
  if (rules.n_country_mults())        // if country mults are used
  { clear_country_mult();

    if (statistics.is_needed_country_mult(_callsign, _band, _mode))
      add_country_mult(_canonical_prefix);

    _is_needed_country_mult.status_is_known(true);
  }
  else
  { // default _is_needed_country_mults instantiation of needed_mult_details should be: not needed, is known
  }

// exchange mult status
  clear_exchange_mult();

  const vector<string> exch_mults = rules.expanded_exchange_mults();                                  // the exchange multipliers

// there can only be an exchange mult if one of the exchange field names matches an exchange mult field name
  bool exchange_mult_is_possible = false;

  for (const auto& exch_mult_name : exch_mults)
  { const vector<string> exchange_field_names = rules.expanded_exchange_field_names(_canonical_prefix, _mode);
    const bool is_possible_exchange_field = ( find(exchange_field_names.cbegin(), exchange_field_names.cend(), exch_mult_name) != exchange_field_names.cend() );

    if (is_possible_exchange_field)
    { exchange_mult_is_possible = true;
      string guess = exchange_db.guess_value(_callsign, exch_mult_name);

      guess = rules.canonical_value(exch_mult_name, guess);

      if (!guess.empty())
        _is_needed_exchange_mult.status_is_known(true);
    }
  }

  if (!exchange_mult_is_possible)                      // we now know that no exchange fields are exchange mult fields for this canonical prefix
  { _is_needed_exchange_mult.status_is_known(true);
  }

// for debugging HA contest, in which many stns were marked on bm in green, even though already worked
//  if (is_needed_callsign_mult() or is_needed_country_mult() or is_needed_exchange_mult())
//    ost << "+ve mult status for " << callsign() << ": " << (is_needed_callsign_mult() ? "T" : "F")
//        << (is_needed_country_mult() ? "T" : "F") << (is_needed_exchange_mult() ? "T" : "F")
//        << endl;

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
const bool bandmap_entry::matches_bandmap_entry(const bandmap_entry& be) const
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
const bool bandmap_entry::remark(contest_rules& rules, call_history& q_history, running_statistics& statistics)
{ const bool original_is_needed = _is_needed;

// if this contest allows only one QSO with a station (e.g., SS)
  if (!rules.work_if_different_band())
  { _is_needed = true;

    for (auto& b : rules.permitted_bands())
      _is_needed = ( _is_needed and !q_history.worked(_callsign, b) );
  }
  else
   _is_needed = !q_history.worked(_callsign, _band);

// multi-mode contests ***
  const bool original_is_needed_callsign_mult = is_needed_callsign_mult();
  const bool original_is_needed_country_mult = is_needed_country_mult();
  const bool original_is_needed_exchange_mult = is_needed_exchange_mult();

  calculate_mult_status(rules, statistics);

  return ( (original_is_needed != _is_needed) or (original_is_needed_callsign_mult != is_needed_callsign_mult()) or
           (original_is_needed_country_mult != is_needed_country_mult()) or (original_is_needed_exchange_mult != is_needed_exchange_mult()));
}

/*! \brief          Add a call to the associated posters
    \param  call    call to add
    \return         number of posters associated with this call, after adding <i>call</i>

    Does nothing if <i>call</i> is already a poster
*/
//const unsigned int bandmap_entry::add_poster(const string& call)
//{ _posters.insert(call);
//
//  return n_posters();
//}

/// return all the posters as a space-separated string
//const string bandmap_entry::posters_string(void) const
//{ string rv;
//
//  FOR_ALL(_posters, [&rv] (const string& p) { rv += (p + " "); } );
//
//  if (!rv.empty())
//    rv = substring(rv, 0, rv.length() - 1);  // remove the final space
//
//  return rv;
//}

/// guess the mode, based on the frequency
const MODE bandmap_entry::putative_mode(void) const
{ if (source() == BANDMAP_ENTRY_RBN)
    return MODE_CW;

  try
  { return ( (_freq < MODE_BREAK_POINT.at(band()) ) ? MODE_CW : MODE_SSB);
  }

  catch (...)
  { ost << "ERROR in putative_mode(); band = " << band() << endl;
    return MODE_CW;
  }
}

// set value from an earlier be
//void bandmap_entry::time_of_earlier_bandmap_entry(const bandmap_entry& old_be)
//{ if (old_be.time_of_earlier_bandmap_entry())
//  { _time_of_earlier_bandmap_entry = old_be._time_of_earlier_bandmap_entry;
//  }
//  else
//  { _time_of_earlier_bandmap_entry = old_be._time;
//  }

//  _time_of_earlier_bandmap_entry = ( old_be.time_of_earlier_bandmap_entry() ? old_be._time_of_earlier_bandmap_entry : old_be._time );
//}

/// ostream << bandmap_entry
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
//      << "number of posters: " << be.n_posters() << endl;

//  if (be.n_posters())
//  { ost << "posters:" << endl;
//
//    const set<string> posters = be.posters();
//
//    FOR_ALL(posters, [&ost] (const string& poster) { ost << "  " << poster << endl; } );
//  }

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
const string bandmap::_nearest_callsign(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz)
{ if (target_frequency_in_khz < 1800 or target_frequency_in_khz > 29700)
  { ost << "WARNING: bandmap::_nearest_callsign called with frequency in kHz = " << target_frequency_in_khz << endl;
    return string();
  }

  const float guard_band_in_khz = static_cast<float>(guard_band_in_hz) / 1000.0;
  bool finish_looking = false;
  float smallest_difference = 1000000;              // start with a big number
  string rv;

  for (BM_ENTRIES::const_iterator cit = bme.cbegin(); (!finish_looking and cit != bme.cend()); ++cit)
  { const float difference = cit->freq().kHz() - target_frequency_in_khz;
    const float abs_difference = fabs(difference);

    if ( (abs_difference <= guard_band_in_khz) and (cit->callsign() != MY_MARKER))
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
*/
void bandmap::_insert(const bandmap_entry& be)
{ SAFELOCK(_bandmap);

  bool inserted = false;

  for (BM_ENTRIES::iterator it = _entries.begin(); !inserted and it != _entries.end(); ++it)
  { if (it->freq().hz() > be.freq().hz())
    { _entries.insert(it, be);                  // inserts before

       inserted = true;
    }
  }

  if (!inserted)
    _entries.push_back(be);    // this frequency is higher than any currently in the bandmap
}

/*!  \brief     Mark filtered and rbn/filtered entries as dirty
*/
void bandmap::_dirty_entries(void)
{ SAFELOCK (_bandmap);              // should be unnecessary, since if the entries are dirty we should already have the lock

  _filtered_entries_dirty = true;
  _rbn_threshold_and_filtered_entries_dirty = true;
}

/// default constructor
bandmap::bandmap(void) :
  _column_offset(0),
  _filtered_entries_dirty(false),
  _filter_p(&BMF),
  _mode_marker_frequency(frequency(0)),
  _rbn_threshold(1),
  _rbn_threshold_and_filtered_entries_dirty(false),
  _recent_colour(string_to_colour("BLACK"))
{ }

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
const bool bandmap::_mark_as_recent(const bandmap_entry& be)
{ if (_rbn_threshold == 1)
    return true;

  if ( (be.source() == BANDMAP_ENTRY_LOCAL) or (be.source() == BANDMAP_ENTRY_CLUSTER) )
    return true;

  if (be.is_marker())           // don't mark markers as recent, if somehow we got here with a marker as parameter (which should nevre happen)
    return false;

  SAFELOCK(_bandmap);

  bandmap_entry old_be = (*this)[be.callsign()];

  if (!old_be.valid())    // not already present
    return false;

  if (be.absolute_frequency_difference(old_be) > MAX_FREQUENCY_SKEW)       // treat anything within 250 Hz as the same frequency
    return false;         // we're going to write a new entry

// RBN poster
//  const unsigned int n_new_posters = be.n_posters();

//  if (n_new_posters != 1)
//    ost << "in _mark_as_recent: Error: number of posters = " << n_new_posters << " for post for " << be.callsign() << endl;
//  else
//    return (old_be.is_poster( *(be.posters().cbegin()) ));

  return false;    // should never get here
}

/*!  \brief     Add a bandmap_entry
     \param be  entry to add

     <i>_time_of_earlier_bandmap_entry</i> in <i>be</i> might be changed.
     Could change this by copying the be inside +=.

     Does not add if the frequency is outside the ham bands.
*/
void bandmap::operator+=(bandmap_entry& be)
{ //const bool mode_marker_is_present = is_present(MODE_MARKER);
//  ost << "Adding bandmap entry: " << be << endl;
//  ost << "current size = " << this->size() << endl;

// possibly add poster to the bandmap buffer
//  if (be.source() != BANDMAP_ENTRY_LOCAL)
//    bm_buffer.add(be.callsign(), be.p)



  const bool mode_marker_is_present = (_mode_marker_frequency.hz() != 0);
  const string& callsign = be.callsign();

// do not add if it's already been done recently, or matches several other conditions
  bool add_it = !(_do_not_add < callsign);

  if (add_it)
    add_it = be.freq().is_within_ham_band();

  if (add_it)
    add_it = !((be.source() != BANDMAP_ENTRY_LOCAL) and is_recent_call(callsign));

// could make this more efficient by having a global container of the mode-marker bandmap entries
  if (add_it and mode_marker_is_present)
  { const bandmap_entry mode_marker_be = (*this)[MODE_MARKER];    // assumes only one mode marker

    add_it = (be.absolute_frequency_difference(mode_marker_be) > MAX_FREQUENCY_SKEW);
  }

  if (add_it)
  { const bool mark_as_recent = _mark_as_recent(be);  // keep track of whether we're going to mark this as a recent call
    bandmap_entry old_be;

    SAFELOCK(_bandmap);

    if (be.source() == BANDMAP_ENTRY_RBN)
    { old_be = (*this)[callsign];

      if (old_be.valid())
      { if (be.absolute_frequency_difference(old_be) > MAX_FREQUENCY_SKEW)  // if not within 250 Hz
        { (*this) -= callsign;
          _insert(be);
        }
        else    // ~same frequency
        { if (old_be.expiration_time() >= be.expiration_time())
          { (*this) -= callsign;
            _insert(old_be);
          }
          else    // new expiration is later
          { old_be.source(BANDMAP_ENTRY_RBN);
            old_be.expiration_time(be.expiration_time());

            (*this) -= callsign;
            be.time_of_earlier_bandmap_entry(old_be);    // could change be

            _insert(old_be);
          }
        }
      }
      else    // this call is not currently present
      { _entries.remove_if([=] (bandmap_entry& bme) { return ((bme.frequency_str() == be.frequency_str()) and (!bme.is_marker())); } );  // remove any real entries at this QRG
        _insert(be);
      }

// possibly remove all the other entries at this QRG
      if (callsign != MY_MARKER and callsign != MODE_MARKER)
      { const bandmap_entry current_be = (*this)[callsign];  // the entry in the updated bandmap

//        if (current_be.n_posters() >= _rbn_threshold)
        { _entries.remove_if([=] (bandmap_entry& bme) { bool rv = !bme.is_marker();

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

    if ((callsign != MY_MARKER) and (callsign != MODE_MARKER) and mark_as_recent)
      _recent_calls.insert(callsign);

    _dirty_entries();
  }

  if (mode_marker_is_present and !is_present(MODE_MARKER))
    ost << "*** ERROR: MODE MARKER HAS BEEN REMOVED BY BANDMAP_ENTRY: " << be << endl;
}

/// prune the bandmap
void bandmap::prune(void)
{ SAFELOCK(_bandmap);

  const time_t now = ::time(NULL);             // get the time from the kernel
  const size_t initial_size = _entries.size();

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
const bandmap_entry bandmap::operator[](const string& str)
{ SAFELOCK(_bandmap);

  const auto cit = FIND_IF(_entries, [=] (const bandmap_entry& be) { return (be.callsign() == str); });

  return ( (cit == _entries.cend()) ? bandmap_entry() : *cit );
}

/*! \brief              Return the first entry for a partial call
    \param  callsign    partial call for which the entry should be returned
    \return             the first bandmap_entry corresponding to <i>callsign</i>

    Returns the null string if <i>callsign</i> matches no entries in the bandmap
*/
const bandmap_entry bandmap::substr(const string& str)
{ SAFELOCK(_bandmap);

  const auto cit = FIND_IF(_entries, [=] (const bandmap_entry& be) { return contains(be.callsign(), str); });

  return ( (cit == _entries.cend()) ? bandmap_entry() : *cit );
}

/*! \brief              Remove a call from the bandmap
    \param  callsign    call to be removed

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::operator-=(const string& callsign)
{ SAFELOCK(_bandmap);

  const size_t initial_size = _entries.size();

  _entries.remove_if([=] (const bandmap_entry& be) { return (be.callsign() == callsign); });        // OK for lists

  if (_entries.size() != initial_size)
    _dirty_entries();
}

/*! \brief              Set the needed status of a call to <i>false</i>
    \param  callsign    call for which the status should be set

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::not_needed(const string& callsign)
{ SAFELOCK(_bandmap);

  bandmap_entry be = (*this)[callsign];

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
void bandmap::not_needed_callsign_mult(const std::string (*pf)(const std::string& /* e.g. "WPXPX" */, const std::string& /* callsign */),
                                       const std::string& mult_type /* e.g., "WPXPX" */,
                                       const std::string& callsign_mult_string /* e.g., SM1 */)
{ if (callsign_mult_string.empty() or mult_type.empty())
    return;

  SAFELOCK(_bandmap);

// change status for all entries with this particular callsign mult
  for (auto& be : _entries)
  { if (be.is_needed_callsign_mult())
    { const string& callsign = be.callsign();
      const string this_callsign_mult = (*pf)(mult_type, callsign);

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
const BM_ENTRIES bandmap::filtered_entries(void)
{ if (!filter_enabled())
    return entries();

  { SAFELOCK (_bandmap);

    if (!_filtered_entries_dirty)
      return _filtered_entries;
  }

  SAFELOCK(_bandmap);

  const BM_ENTRIES tmp = entries();
  BM_ENTRIES rv;

  for (const auto& be : tmp)
  { if (be.is_my_marker() or be.is_mode_marker())
      rv.push_back(be);
    else                                              // start by assuming that we are in show mode
    { const string& canonical_prefix = be.canonical_prefix();
      const string& continent = be.continent();
      bool display_this_entry = false;
      const vector<string>& fil_continent = _filter_p->continents();

      for (size_t n = 0; n < fil_continent.size() and !display_this_entry; ++n)
        if (continent == fil_continent[n])
          display_this_entry = true;

      const vector<string>& fil_prefix = _filter_p->prefixes();

      for (size_t n = 0; n < fil_prefix.size() and !display_this_entry; ++n)
        if (canonical_prefix == fil_prefix[n])
          display_this_entry = true;

      if (filter_hide())                              // hide is the opposite of show
        display_this_entry = !display_this_entry;

      if (display_this_entry)
        rv.push_back(be);
    }
  }

  _filtered_entries = rv;
  _filtered_entries_dirty = false;

  return rv;
}

/// all the entries, after the RBN threshold and filtering have been applied
const BM_ENTRIES bandmap::rbn_threshold_and_filtered_entries(void)
{
  { SAFELOCK (_bandmap);

    if (!_rbn_threshold_and_filtered_entries_dirty)
      return _rbn_threshold_and_filtered_entries;
  }

  const BM_ENTRIES filtered = filtered_entries();
  BM_ENTRIES rv;

//  SAFELOCK(_bandmap);

//  const unsigned int threshold = _rbn_threshold;

  for (const auto& be : filtered)
  { //if (be.source() == BANDMAP_ENTRY_RBN)
    //{ //if (be.n_posters() >= threshold)
        //rv.push_back(be);
    //}
    //else  // not RBN
      //rv.push_back(be);
    rv.push_back(be);
  }

  SAFELOCK(_bandmap);

  _rbn_threshold_and_filtered_entries = rv;
  _rbn_threshold_and_filtered_entries_dirty = false;

  return rv;
}

/*!  \brief         Find the next needed station up or down in frequency from the current location
     \param fp      pointer to function to be used to determine whether a station is needed
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found.
     Applies filtering and the RBN threshold before searching for the next station.
*/
const bandmap_entry bandmap::needed(PREDICATE_FUN_P fp, const enum BANDMAP_DIRECTION dirn)
{ const BM_ENTRIES fe = rbn_threshold_and_filtered_entries();
  auto cit = FIND_IF(fe, [=] (const bandmap_entry& be) { return (be.is_my_marker()); } );  // find myself

  if (dirn == BANDMAP_DIRECTION_DOWN)
  { if (cit != fe.cend())                      // should always be true
    { const string target_freq_str = cit->frequency_str();
      auto crit = prev(reverse_iterator<decltype(cit)>(cit));             // Josuttis First ed. p. 66f.

      //BM_ENTRIES::const_reverse_iterator crit2 = find_if(crit, fe.crend(), [=] (const bandmap_entry& be) { return (be.frequency_str() != target_freq_str); } ); // move away from my frequency, in downwards direction
      const auto crit2 = find_if(crit, fe.crend(), [=] (const bandmap_entry& be) { return (be.frequency_str() != target_freq_str); } ); // move away from my frequency, in downwards direction

      if (crit2 != fe.crend())
      { //BM_ENTRIES::const_reverse_iterator crit3 = find_if(crit2, fe.crend(), [=] (const bandmap_entry& be) { return (be.*fp)(); } );
        const auto crit3 = find_if(crit2, fe.crend(), [=] (const bandmap_entry& be) { return (be.*fp)(); } );

        if (crit3 != fe.crend())
          return (*crit3);
      }
    }
  }

  if (dirn == BANDMAP_DIRECTION_UP)
  { if (cit != fe.cend())                      // should always be true
    { const string target_freq_str = cit->frequency_str();
//      BM_ENTRIES::const_iterator cit2 = find_if(cit, fe.cend(), [=] (const bandmap_entry& be) { return (be.frequency_str() != target_freq_str); }); // move away from my frequency, in upwards direction
      const auto cit2 = find_if(cit, fe.cend(), [=] (const bandmap_entry& be) { return (be.frequency_str() != target_freq_str); }); // move away from my frequency, in upwards direction

      if (cit2 != fe.cend())
      { //BM_ENTRIES::const_iterator cit3 = find_if(cit2, fe.cend(), [=] (const bandmap_entry& be) { return (be.*fp)(); });
        const auto cit3 = find_if(cit2, fe.cend(), [=] (const bandmap_entry& be) { return (be.*fp)(); });

        if (cit3 != fe.cend())
          return (*cit3);
      }
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
const bandmap_entry bandmap::next_station(const frequency& f, const enum BANDMAP_DIRECTION dirn)
{ bandmap_entry rv;

  const BM_ENTRIES fe = rbn_threshold_and_filtered_entries();

  if (fe.empty())
    return rv;

  if (dirn == BANDMAP_DIRECTION_DOWN and f <= fe.front().freq())
    return rv;

  if (dirn == BANDMAP_DIRECTION_UP and f >= fe.back().freq())
    return rv;

  if (dirn == BANDMAP_DIRECTION_DOWN)
  { if (f <= fe.front().freq())         // all frequencies are higher than the target
      return rv;

    rv = fe.front();

    for (BM_ENTRIES::const_iterator cit = next(fe.cbegin()); cit != fe.cend(); ++cit)
    { if (cit->freq() >= f)
        return rv;

      rv = *cit;
    }

// get here only if all frequencies are below the target
    return rv;
  }

  if (dirn == BANDMAP_DIRECTION_UP)
  { if (f >= fe.back().freq())         // all frequencies are lower than the target
      return rv;

    rv = fe.back();

    for (BM_ENTRIES::const_reverse_iterator crit = next(fe.crbegin()); crit != fe.crend(); ++crit)
    { if (crit->freq() <= f)
        return rv;

      rv = *crit;
    }

// get here only if all frequencies are above the target
    return rv;
  }

  return bandmap_entry();       // keep compiler happy
}

/*! \brief      Get lowest frequency on the bandmap
    \return     lowest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
const frequency bandmap::lowest_frequency(void)
{ const BM_ENTRIES bme = rbn_threshold_and_filtered_entries();

  return (bme.empty() ? frequency() : bme.front().freq());
}

/*! \brief      Get highest frequency on the bandmap
    \return     highest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
const frequency bandmap::highest_frequency(void)
{ const BM_ENTRIES bme = rbn_threshold_and_filtered_entries();

  return (bme.empty() ? frequency() : bme.back().freq());
}

/// convert to a printable string
const string bandmap::to_str(void)
{ string rv;
  BM_ENTRIES raw;
  BM_ENTRIES filtered;
  BM_ENTRIES threshold_and_filtered;

  { SAFELOCK(_bandmap);

    raw = entries();
    filtered = filtered_entries();
    threshold_and_filtered = rbn_threshold_and_filtered_entries();
  }

  rv += "RAW bandmap:" + EOL;
  rv += "number of entries = " + to_string(raw.size());

  for (const auto& be : raw)
    rv += to_string(be) + EOL;

  rv += EOL + "FILTERED bandmap:" + EOL;

  rv += "number of entries = " + to_string(filtered.size());

  for (const auto& be : filtered)
    rv += to_string(be) + EOL;

  rv += EOL + "THRESHOLD AND FILTERED bandmap:" + EOL;

  rv += "number of entries = " + to_string(threshold_and_filtered.size());

  for (const auto& be : threshold_and_filtered)
    rv += to_string(be) + EOL;

  return rv;
}

/// is a particular call present?
const bool bandmap::is_present(const string& target_callsign)
{ SAFELOCK(_bandmap);

  return !(_entries.cend() == FIND_IF(_entries, [=] (const bandmap_entry& be) { return (be.callsign() == target_callsign); }));
}

/// window < bandmap
window& operator<(window& win, bandmap& bm)
{ static const unsigned int COLUMN_WIDTH = 19;                                // width of a column in the bandmap window

  static int NOT_NEEDED_COLOUR = COLOUR_BLACK;
//  static int DO_NOT_WORK_COLOUR = NOT_NEEDED_COLOUR;
  static int MULT_COLOUR = COLOUR_GREEN;
  static int NOT_MULT_COLOUR = COLOUR_BLUE;
  static int UNKNOWN_MULT_STATUS_COLOUR = COLOUR_YELLOW;

  const size_t maximum_number_of_displayable_entries = (win.width() / COLUMN_WIDTH) * win.height();

  SAFELOCK(bandmap);                                        // in case multiple threads are trying to write a bandmap to the window

  const BM_ENTRIES entries = bm.rbn_threshold_and_filtered_entries();    // automatically filter
  const size_t start_entry = (entries.size() > maximum_number_of_displayable_entries) ? bm.column_offset() * win.height() : 0;

  win < WINDOW_CLEAR < CURSOR_TOP_LEFT;

  size_t index = 0;    // keep track of where we are in the bandmap

  for (const auto& be : entries)
  { if ( (index >= start_entry) and (index < (start_entry + maximum_number_of_displayable_entries) ) )
    { const string entry_str = pad_string(pad_string(be.frequency_str(), 7)  + " " + be.callsign(), COLUMN_WIDTH, PAD_RIGHT);
      const string frequency_str = substring(entry_str, 0, 7);
      const string callsign_str = substring(entry_str, 8);
      const bool is_marker = be.is_marker();
  
// change to the correct colour
//      const time_t age = be.time_since_inserted();
      const time_t age_since_original_inserted = be.time_since_this_or_earlier_inserted();
      const time_t age_since_this_inserted = be.time_since_inserted();

//      ost << "age of this or earlier insertion for " << be.callsign() << " = " << age_since_original_inserted << endl;
//      ost << "time in entry: " << be.time() << endl;
//      ost << "earlier time in entry: " << be.time_of_earlier_bandmap_entry() << endl;
//      ost << "be: " << be << endl;

      const time_t start_time = be.time();                  // time it was inserted
      const time_t expiration_time = be.expiration_time();
      const float fraction = static_cast<float>(age_since_this_inserted) / (expiration_time - start_time);

      const vector<int> fade_colours = bm.fade_colours();
      const int n_colours = fade_colours.size();
      const float interval = (1.0 / static_cast<float>(n_colours));

      int n_intervals = fraction / interval;
      n_intervals = min(n_intervals, n_colours - 1);

      int cpu = colours.add(fade_colours.at(n_intervals), win.bg());

// mark in GREEN if less than two minutes since the original spot at this freq was inserted
      if (age_since_original_inserted < 120 and !be.is_my_marker() and !be.is_mode_marker() and (bm.recent_colour() != string_to_colour("BLACK")))
        cpu = colours.add(bm.recent_colour(), win.bg());

//      if (be.is_my_marker() or be.is_mode_marker())
      if (is_marker)
        cpu = colours.add(COLOUR_WHITE, COLOUR_BLACK);    // colours for markers

// work out where to start the display of this call
      const unsigned int x = ( (index  - start_entry) / win.height()) * COLUMN_WIDTH;
    
// check that there's room to display the entire entry
      if ((win.width() - x) < COLUMN_WIDTH)
        break;

      const unsigned int y = (win.height() - 1) - (index - start_entry) % win.height();

// now work out the status colour
      int status_colour = colours.add(NOT_NEEDED_COLOUR, NOT_NEEDED_COLOUR);                      // default

      if (!is_marker)
      { if (be.is_needed())
        { if (be.mult_status_is_known())
            status_colour = (be.is_needed_mult() ? colours.add(MULT_COLOUR, MULT_COLOUR) : colours.add(NOT_MULT_COLOUR, NOT_MULT_COLOUR));
          else
            status_colour = colours.add(UNKNOWN_MULT_STATUS_COLOUR, UNKNOWN_MULT_STATUS_COLOUR);
        }
      }

// reverse the colour of the frequency if there are unseen entries lower or higher in frequency
      const bool reverse = ( (start_entry != 0) and (start_entry == index) ) or
                             (index == (start_entry + maximum_number_of_displayable_entries - 1) and (entries.size() > (index + 1))) ;

      win < cursor(x, y) < colour_pair(cpu);

      if (reverse)
        win < WINDOW_REVERSE;

      win < frequency_str;

      if (reverse)
        win < WINDOW_NORMAL;

      win < colour_pair(status_colour) < " "
          < colour_pair(cpu) < callsign_str;
    }

    index++;
  }

  return win;
}

/// ostream << bandmap
ostream& operator<<(ostream& ost, bandmap& bm)
{ ost << bm.to_str();

  return ost;
}
