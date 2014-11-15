// $Id: bandmap.cpp 84 2014-11-15 19:20:13Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file bandmap.cpp

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

extern message_stream      ost;                 ///< debugging/logging output
extern pt_mutex            bandmap_mutex;       ///< used when writing to the bandmap window

extern const set<string> CONTINENT_SET;         ///< two-letter abbreviations for all the continents

const string MY_MARKER("--------");             ///< the string that marks my position in the bandmap
bandmap_filter_type bmf;                        ///< the global bandmap filter

/// printable name of a source
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

// -----------   bandmap_filter_type ----------------

/*!     \class bandmap_filter_type
        \brief Control bandmap filtering
*/

/// return all the canonical prefixes and continents that are currently being filtered
const vector<string> bandmap_filter_type::filter(void) const
{ vector<string> rv = _continents;

  rv.insert(rv.end(), _prefixes.begin(), _prefixes.end());

  return rv;
}

/*!  \brief Add a string to, or remove a string from, the filter
     \param str string to add or subtract

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

extern const string callsign_mult_value(const string& callsign_mult_name, const string& callsign);
extern exchange_field_database exchange_db;                          ///< dynamic database of exchange field values for calls; automatically thread-safe

// default constructor
bandmap_entry::bandmap_entry(const BANDMAP_ENTRY_SOURCE s) :
  _time(::time(NULL)),
  _source(s),
  _is_needed(true),
  _expiration_time(0)
{ }

/// set _freq and _frequency_str
void bandmap_entry::freq(const frequency& f)
{ _freq = f;
  _frequency_str = _freq.display_string();
}

/*! \brief  Calculate the mult status of all entries
    \param  rules           the rules for this contest
    \param  statistics      the current statistics

    Adjust the mult status in accordance with the passed parameters
*/
void bandmap_entry::calculate_mult_status(contest_rules& rules, running_statistics& statistics)
{
// callsign mult status
  clear_callsign_mult();

  const set<string> callsign_mults = rules.callsign_mults();

  for (const auto& callsign_mult_name : callsign_mults)
  { const string callsign_mult_val = callsign_mult_value(callsign_mult_name, _callsign);

    if (!callsign_mult_val.empty())
    { if (statistics.is_needed_callsign_mult(callsign_mult_name, callsign_mult_val, _band))
        add_callsign_mult(callsign_mult_name, callsign_mult_val);
    }
  }

// country mult status
  clear_country_mult();

  const bool is_needed_country_mult = statistics.is_needed_country_mult(_callsign, _band);

  if (is_needed_country_mult)
    add_country_mult(_canonical_prefix);

// exchange mult status
  clear_exchange_mult();

  const vector<string> exch_mults = rules.exchange_mults();                                      ///< the exchange multipliers, in the same order as in the configuration file

  for (const auto& exch_mult_name : exch_mults)
  { string guess = exchange_db.guess_value(_callsign, exch_mult_name);

//    ost << "guess for " << _callsign << ", exchange mult " << exch_mult_name << " is: " << guess << endl;
    guess = rules.canonical_value(exch_mult_name, guess);
//    ost << "converted to canonical value: " << guess << endl;

    if (!guess.empty())
    { //ost << "is needed exchange mult = " << statistics.is_needed_exchange_mult(exch_mult_name, MULT_VALUE(exch_mult_name, guess), _band) << endl;

      if (statistics.is_needed_exchange_mult(exch_mult_name, MULT_VALUE(exch_mult_name, guess), _band))
      { //add_exchange_mult(exch_mult_name, MULT_VALUE(exch_mult_name, guess));

        const bool status = add_exchange_mult(exch_mult_name, MULT_VALUE(exch_mult_name, guess));

        //ost << "attempt to add returned: " << status << endl;
      }
    }
  }
}

/*! \brief  Does this object match another bandmap_entry?
    \param  be  target bandmap entry
    \return whether frequency_str or callsign match

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
{
// is needed?
  const bool original_is_needed = _is_needed;

// to do: allow for SS rules and per-mode contests
  _is_needed = !q_history.worked(_callsign, _band);

// callsign mult
  const bool original_is_needed_callsign_mult = is_needed_callsign_mult();
  const bool original_is_needed_country_mult = is_needed_country_mult();
  const bool original_is_needed_exchange_mult = is_needed_exchange_mult();

  calculate_mult_status(rules, statistics);

  return ( (original_is_needed != _is_needed) or (original_is_needed_callsign_mult != is_needed_callsign_mult()) or
           (original_is_needed_country_mult != is_needed_country_mult()) or (original_is_needed_exchange_mult != is_needed_exchange_mult()));
}

/// difference in frequency between two bandmap entries
/*! \brief              Return the difference in frequency between two bandmap entries
    \param  be          other bandmap entry
    \return             difference in frequency between *this and <i>be</i>
*/
const frequency bandmap_entry::frequency_difference(const bandmap_entry& be) const
{ frequency rv;

  rv.hz(abs(be._freq.hz() - _freq.hz()));

  return rv;
}

/*! \brief          Add a call to the associated posters
    \param  call    call to add
    \return         number of posters associated with this call, after adding <i>call</i>

    Does nothing if <i>call</i> is already a poster
*/
const unsigned int bandmap_entry::add_poster(const string& call)
{ _posters.insert(call);

  return n_posters();
}

/// return all the posters as a space-separated string
const string bandmap_entry::posters_string(void) const
{ string rv;

//  for_each(_posters.cbegin(), _posters.cend(), [&rv] (const string& p) { rv += (p + " "); } );
  FOR_ALL(_posters, [&rv] (const string& p) { rv += (p + " "); } );

  if (!rv.empty())
    rv = rv.substr(0, rv.length() - 1);  // skip the final space

  return rv;
}

/// ostream << bandmap_entry
ostream& operator<<(ostream& ost, const bandmap_entry& be)
{ ost << "frequency: " << to_string(be.freq()) << endl
      << "frequency_str: " << be.frequency_str() << endl
      << "callsign: " << be.callsign() << endl
      << "canonical_prefix: " << be.canonical_prefix() << endl
      << "continent: " << be.continent() << endl
      << "band: " << be.band() << endl
      << "time: " << be.time() << endl
      << "source: " << to_string(be.source()) << endl
      << "expiration_time: " << be.expiration_time() << endl
      << "is needed: " << be.is_needed() << endl
      << "is needed mult: " << be.is_needed_mult() << endl
      << "is needed callsign mult: " << be.is_needed_callsign_mult_details() << endl
      << "is needed country mult: " << be.is_needed_country_mult_details() << endl
      << "is needed exchange mult: " << be.is_needed_exchange_mult_details() << endl
      << "number of posters: " << be.n_posters() << endl;

  if (be.n_posters())
  { const set<string> posters = be.posters();

    for_each(posters.cbegin(), posters.cend(), [&ost] (const string& poster) { ost << "  " << poster << endl; } );
  }

  return ost;
}

// -----------  bandmap  ----------------

/*!     \class bandmap
        \brief A bandmap
*/

/*!  \brief Return the callsign closest to a particular frequency, if it is within the guard band
     \param bme                     band map entries
     \param target_frequency_in_khz the target frequency, in kHz
     \param guard_band_in_hz        how far from the target to search, in Hz
     \return                        Callsign of a station within the guard band

     Returns the nearest station within the guard band, or the null string if no call is found.
*/
const string bandmap::_nearest_callsign(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz)
{ // ost << "Inside _nearest callsign; target frequency (kHz) = " << target_frequency_in_khz << "; guard band (Hz) = " << guard_band_in_hz << endl;

  if (target_frequency_in_khz < 1800 or target_frequency_in_khz > 29700)
  { ost << "WARNING: bandmap::_nearest_callsign called with frequency in kHz = " << target_frequency_in_khz << endl;
    return string();
  }

  const float guard_band_in_khz = static_cast<float>(guard_band_in_hz) / 1000.0;
  bool finish_looking = false;
  float smallest_difference = 1000000;
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

    if (difference > guard_band_in_khz)
      finish_looking = true;
  }

//  if (finish_looking and !rv.empty())
//    ost << "nearest callsign to " << target_frequency_in_khz << " is " << rv << ", with abs(difference) = " << smallest_difference << endl;
//  else
//  { ost << "finish_looking = " << finish_looking << endl;
//    ost << "rv = " << rv << endl;
//  }

  return rv;
}

// insert an entry at the right place; also remove any current entry at the same frequency
void bandmap::_insert(const bandmap_entry& be)
{ SAFELOCK(_bandmap);

// remove any entry at the same frequency; now done in +=
//  if (be.n_posters() >= _rbn_threshold)
//    _entries.remove_if([=] (bandmap_entry& bme) { return ((bme.frequency_str() == be.frequency_str()) and (bme.callsign() != MY_MARKER)); } );  // remove any real entries at this QRG

  bool inserted = false;

  for (BM_ENTRIES::iterator it = _entries.begin(); !inserted and it != _entries.end(); ++it)
  { if (it->freq().hz() > be.freq().hz())
    { _entries.insert(it, be);                  // inserts before

       inserted = true;
    }
  }

  if (!inserted)
    _entries.push_back(be);    // this frequency is higher than any currently in the bandmap

//  ost << "inserted " << be.callsign() << " from " << be.source() << " with n posters = " << be.n_posters() << "; posters = " << be.posters_string() << endl;
}

/// default constructor
bandmap::bandmap(void) :
  _filter_p(&bmf),
  _column_offset(0),
  _rbn_threshold(1),
  _filtered_entries_dirty(false),
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
const bool bandmap::_mark_as_recent(const bandmap_entry& be)
{ if (_rbn_threshold == 1)
    return true;

  if ( (be.source() == BANDMAP_ENTRY_LOCAL) or (be.source() == BANDMAP_ENTRY_CLUSTER) )
    return true;

// RBN poster
  SAFELOCK(_bandmap);

  bandmap_entry old_be = (*this)[be.callsign()];

  if (!old_be.valid())    // not already present
    return false;

  if (be.frequency_difference(old_be).hz() > 250)
    return false;         // we're going to write a new entry

  const unsigned int n_new_posters = be.n_posters();

  if (n_new_posters != 1)
    ost << "in _mark_as_recent: Error: number of posters = " << n_new_posters << " for post for " << be.callsign() << endl;
  else
  { const string& poster = *(be.posters().cbegin());

    return (old_be.is_poster(poster));
  }

  return false;    // should never get here
}

/// add an entry to the bandmap
void bandmap::operator+=(const bandmap_entry& be)
{ ost << "inside += for " << be.callsign() << "; source = " << be.source() << endl;

//  if ((*this)[MY_MARKER].valid())
//    ost << "MY MARKER appears to be present on entry" << endl;
//  else
//    ost << "MY MARKER appears NOT to be present on entry" << endl;

  const string& callsign = be.callsign();

// do not add if it's already been done recently, or matches several other conditions
  bool add_it = !(_do_not_add < callsign);

  if (add_it)
    add_it = !((be.source() != BANDMAP_ENTRY_LOCAL) and is_recent_call(callsign));

  ost << "bandmap+=; callsign = " << callsign << " on " << be.frequency_str() << "; add_it = " << boolalpha << add_it << noboolalpha << endl;

  if (add_it)
  { const bool mark_as_recent = _mark_as_recent(be);  // keep track of whether we're going got mark this as a recent call

    if (mark_as_recent)
      ost << "marked as recent" << endl;
    else
      ost << "NOT marked as recent" << endl;

    SAFELOCK(_bandmap);

    bandmap_entry old_be;

    if (be.source() == BANDMAP_ENTRY_RBN)
    { //mark_as_recent = false;  // default for RBN entries, since a threshold may apply

      old_be = (*this)[callsign];

      if (old_be.valid())
      { if (be.frequency_difference(old_be).hz() > 250)  // if not within 250 Hz
        { (*this) -= callsign;
          _insert(be);
        }
        else    // different frequency
        { if (old_be.expiration_time() >= be.expiration_time())
          { const unsigned int n_new_posters = be.n_posters();

            if (n_new_posters != 1)
              ost << "Error: number of posters = " << n_new_posters << " for post for " << callsign << endl;
            else
            { const string& poster = *(be.posters().cbegin());

              old_be.add_poster(poster);

                ost << "added poster " << poster << " to " << callsign << "; number of posters = " << old_be.n_posters() << endl;
              (*this) -= callsign;
              _insert(old_be);
            }
          }
          else    // new expiration is later
          { old_be.source(BANDMAP_ENTRY_RBN);

            const unsigned int n_new_posters = be.n_posters();

            if (n_new_posters != 1)
              ost << "Error: number of posters = " << n_new_posters << " for post for " << callsign << endl;
            else
            { const string& poster = *(be.posters().cbegin());

              old_be.add_poster(poster);

              ost << "new expiration; added poster " << poster << " to " << callsign << "; number of posters = " << old_be.n_posters() << endl;

              old_be.expiration_time(be.expiration_time());

              (*this) -= callsign;
              _insert(old_be);
            }
          }
        }
      }
      else    // this call is not currently present
      { _entries.remove_if([=] (bandmap_entry& bme) { return ((bme.frequency_str() == be.frequency_str()) and (!bme.is_my_marker())); } );  // remove any real entries at this QRG
        _insert(be);
      }

// possibly remove all the other entries at this QRG
      if (callsign != MY_MARKER)
      { const bandmap_entry current_be = (*this)[callsign];  // the entry in the updated bandmap

        if (current_be.n_posters() >= _rbn_threshold)
        { _entries.remove_if([=] (bandmap_entry& bme) { bool rv = !bme.is_my_marker();

                                                        if (rv)
                                                        { rv = (bme.callsign() != current_be.callsign());

                                                          if (rv)
                                                            rv = bme.frequency_str() == current_be.frequency_str();
                                                        }

                                                        return rv; } );
        }
      }
    }
    else    // not RBN
    { _entries.remove_if([=] (bandmap_entry& bme) { return bme.matches_bandmap_entry(be); } );
      _insert(be);
    }

    if ((callsign != MY_MARKER) and mark_as_recent)
      _recent_calls.insert(callsign);

    _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
  }
}

/// prune the bandmap
void bandmap::prune(void)
{ SAFELOCK(_bandmap);

  const time_t now = ::time(NULL);             // get the time from the kernel
  const size_t initial_size = _entries.size();

  _entries.remove_if([=] (const bandmap_entry& be) { return (be.should_prune(now)); });  // OK for lists

  if (_entries.size() != initial_size)
  { _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
  }

  _recent_calls.clear();                       // empty the container of recent calls
}

/*! \brief return the entry for a particular call
    \param  callsign    call for which the entry should be returned
    \return the bandmap_entry corresponding to <i>callsign</i>

    Returns the default bandmap_entry if <i>callsign</i> is not present in the bandmap
*/
const bandmap_entry bandmap::operator[](const string& str)
{ SAFELOCK(_bandmap);

  const auto cit = find_if(_entries.cbegin(), _entries.cend(), [=] (const bandmap_entry& be) { return (be.callsign() == str); });

  return ( (cit == _entries.cend()) ? bandmap_entry() : *cit );
}

/*! \brief return the first entry for a partial call
    \param  callsign    partial call for which the entry should be returned
    \return             the first bandmap_entry corresponding to <i>callsign</i>

    Returns the null string if <i>callsign</i> matches no entries in the bandmap
*/
const bandmap_entry bandmap::substr(const string& str)
{ SAFELOCK(_bandmap);

  const auto cit = find_if(_entries.cbegin(), _entries.cend(), [=] (const bandmap_entry& be) { return be.is_substr(str); });

  return ( (cit == _entries.cend()) ? bandmap_entry() : *cit );
}

/*! \brief remove a call from the bandmap
    \param  callsign    call to be removed

    Does nothing if <i>callsign</i> is not in the bandmap
*/
void bandmap::operator-=(const string& callsign)
{ SAFELOCK(_bandmap);

  const size_t initial_size = _entries.size();

  _entries.remove_if([=] (const bandmap_entry& be) { return (be.callsign() == callsign); });        // OK for lists

  if (_entries.size() != initial_size)
  { _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
  }
}

/*! \brief set the needed status of a call to false
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

  _filtered_entries_dirty = true;
  _rbn_threshold_and_filtered_entries_dirty = true;
}

/*! \brief set the needed country mult status of all calls in a particular country to false
    \param  canonical_prefix    canonical prefix corresponding to country for which the status should be set

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
*/
void bandmap::not_needed_country_mult(const string& canonical_prefix)
{ SAFELOCK(_bandmap);

  for_each(_entries.begin(), _entries.end(), [&canonical_prefix] (decltype(*_entries.begin())& be) { be.remove_country_mult(canonical_prefix); } );

  _filtered_entries_dirty = true;
  _rbn_threshold_and_filtered_entries_dirty = true;
}

/*! \brief set the needed callsign mult status of all matching callsign mults to false
    \param  pf          pointer to function to return the callsign mult value
    \param  callsign_mult_string value of callsign mult value that is no longer a multiplier

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
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

      ost << "callsign = " << callsign << ", this_callsign_mult = " << this_callsign_mult << ", callsign_mult_string = " << callsign_mult_string << endl;

      if (this_callsign_mult == callsign_mult_string)
//        be.is_needed_callsign_mult(false);              // be.is_needed_callsign_mult(this_callsign_mult == callsign_mult_string) might work
      { be.remove_callsign_mult(mult_type, callsign_mult_string);
        _filtered_entries_dirty = true;
        _rbn_threshold_and_filtered_entries_dirty = true;
      }
    }
  }
}

void bandmap::not_needed_callsign_mult(const std::string& mult_type /* e.g., "WPXPX" */,
                                       const std::string& callsign_mult_string /* e.g., SM1 */)
{ if (callsign_mult_string.empty() or mult_type.empty())
    return;

  SAFELOCK(_bandmap);

// change status for all entries with this particular callsign mult
  for (auto& be : _entries)
    be.remove_callsign_mult(mult_type, callsign_mult_string);

  _filtered_entries_dirty = true;
  _rbn_threshold_and_filtered_entries_dirty = true;
}

void bandmap::not_needed_exchange_mult(const string& mult_name, const string& mult_value)
{ if (mult_name.empty() or mult_value.empty())
    return;

  SAFELOCK(_bandmap);

  for (auto& be : _entries)
    be.remove_exchange_mult(mult_name, mult_value);

  _filtered_entries_dirty = true;
  _rbn_threshold_and_filtered_entries_dirty = true;
}

/// enable or disable the filter
void bandmap::filter_enabled(const bool torf)
{ if (torf != filter_enabled())
  { SAFELOCK(_bandmap);

    _filter_p->enabled(torf);
    _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
  }
}

/*!  \brief Add a string to, or remove a string from, the filter associated with this bandmap
     \param str string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed. Currently, all bandmaps share a single
     filter.
*/
void bandmap::filter_add_or_subtract(const string& str)
{ if (!str.empty())
  { SAFELOCK(_bandmap);

    _filter_p->add_or_subtract(str);
    _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
  }
}

/// set or unset the filter to hide mode (as opposed to show)
void bandmap::filter_hide(const bool torf)
{ if (torf != filter_hide())
  { SAFELOCK(_bandmap);

    _filter_p->hide(torf);
    _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
  }
}

/// set or unset the filter to show mode (as opposed to hide)
void bandmap::filter_show(const bool torf)
{ if (torf != filter_show())
  { SAFELOCK(_bandmap);

    _filter_p->hide(!torf);
    _filtered_entries_dirty = true;
    _rbn_threshold_and_filtered_entries_dirty = true;
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
  { if (be.is_my_marker())
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

const BM_ENTRIES bandmap::rbn_threshold_and_filtered_entries(void)
{ // ost << "inside bandmap::rbn_threshold_and_filtered_entries()" << endl;

  { SAFELOCK (_bandmap);

    if (!_rbn_threshold_and_filtered_entries_dirty)
      return _rbn_threshold_and_filtered_entries;
  }

  const BM_ENTRIES filtered = filtered_entries();
  BM_ENTRIES rv;
  unsigned int threshold;

  SAFELOCK(_bandmap);

  threshold = _rbn_threshold;

//  ost << "filtering with rbn threshold = " << threshold << endl;

  for (const auto& be : filtered)
  { //ost << "source for " << be.callsign() << " is " << be.source() << endl;

    if (be.source() == BANDMAP_ENTRY_RBN)
    { if (be.n_posters() >= threshold)
        rv.push_back(be);
    }
    else  // not RBN
    { rv.push_back(be);
    }
  }

//  ost << "size of filtered = " << filtered.size() << endl;
//  ost << "size of returned = " << rv.size() << endl;

  _rbn_threshold_and_filtered_entries = rv;
  _rbn_threshold_and_filtered_entries_dirty = false;

  return rv;
}

/*!  \brief Find the next needed station up or down in frequency from the current loction
     \param fp      pointer to function to be used to determine whether a station is needed
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found.
     Applies filtering and the RBN threshold before searching for the next station.
*/
const bandmap_entry bandmap::needed(PREDICATE_FUN_P fp, const enum BANDMAP_DIRECTION dirn)
{ const BM_ENTRIES fe = rbn_threshold_and_filtered_entries();

//  BM_ENTRIES::const_iterator cit = find_if(fe.cbegin(), fe.cend(), [=] (const bandmap_entry& be) { return (be.is_my_marker()); } );  // find myself
//  BM_ENTRIES::const_iterator cit = FIND_IF(fe, [=] (const bandmap_entry& be) { return (be.is_my_marker()); } );  // find myself
  auto cit = FIND_IF(fe, [=] (const bandmap_entry& be) { return (be.is_my_marker()); } );  // find myself

  if (dirn == BANDMAP_DIRECTION_DOWN)
  { if (cit != fe.cend())                      // should always be true
    {  const string target_freq_str = cit->frequency_str();
       //BM_ENTRIES::const_reverse_iterator crit(cit);                    // NB points to position *before* cit; Josuttis First ed. p. 66f.

//       reverse_iterator<decltype(cit)> crit(cit);
      auto crit = prev(reverse_iterator<decltype(cit)>(cit));

//       crit = prev(crit);

       BM_ENTRIES::const_reverse_iterator crit2 = find_if(crit, fe.crend(), [=] (const bandmap_entry& be) { return (be.frequency_str() != target_freq_str); } ); // move away from my frequency

       if (crit2 != fe.crend())
       { BM_ENTRIES::const_reverse_iterator crit3 = find_if(crit2, fe.crend(), [=] (const bandmap_entry& be) { return (be.*fp)(); } );

         if (crit3 != fe.crend())
           return (*crit3);
       }
    }
  }

  if (dirn == BANDMAP_DIRECTION_UP)
  { if (cit != fe.end())                      // should always be true
    { const string target_freq_str = cit->frequency_str();
      BM_ENTRIES::const_iterator cit2 = find_if(cit, fe.cend(), [=] (const bandmap_entry& be) { return (be.frequency_str() != target_freq_str); }); // move away from my frequency

      if (cit2 != fe.end())
      { BM_ENTRIES::const_iterator cit3 = find_if(cit2, fe.cend(), [=] (const bandmap_entry& be) { return (be.*fp)(); });

        if (cit3 != fe.cend())
          return (*cit3);
      }
    }
  }

  return bandmap_entry();
}

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

/// window < bandmap
window& operator<(window& win, bandmap& bm)
{ static const unsigned int COLUMN_WIDTH = 19;                                // width of a column in the bandmap window
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
  
// change to the correct colour
      const time_t age = be.time_since_inserted();
      const time_t start_time = be.time();                  // time it was inserted
      const time_t expiration_time = be.expiration_time();
      const float fraction = static_cast<float>(age) / (expiration_time - start_time);

      const vector<int> fade_colours = bm.fade_colours();
      const int n_colours = fade_colours.size();
      const float interval = (1.0 / static_cast<float>(n_colours));

      int n_intervals = fraction / interval;
      n_intervals = min(n_intervals, n_colours - 1);

      int cpu = colours.add(fade_colours.at(n_intervals), win.bg());

// mark in GREEN if age is less than two minutes
      if (age < 120 and !be.is_my_marker() and (bm.recent_colour() != string_to_colour("BLACK")))
        cpu = colours.add(bm.recent_colour(), win.bg());

      if (be.is_my_marker())
        cpu = colours.add(COLOUR_WHITE, COLOUR_BLACK);    // marker for my current frequency

// work out where to start the display of this call
      const unsigned int x = ( (index  - start_entry) / win.height()) * COLUMN_WIDTH;
    
// check that there's room to display the entire entry
      if ((win.width() - x) < COLUMN_WIDTH)
        break;

      const unsigned int y = (win.height() - 1) - (index - start_entry) % win.height();
      int status_colour = colours.add(COLOUR_BLACK, COLOUR_BLACK);

      if (!be.is_my_marker())
      { if (be.is_needed())
          status_colour = colours.add(COLOUR_BLUE, COLOUR_BLUE);

        if (be.is_needed_mult())
          status_colour = colours.add(COLOUR_GREEN, COLOUR_GREEN);
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

ostream& operator<<(ostream& ost, bandmap& be)
{ ost << be.to_str(); // << endl;

  return ost;
}
