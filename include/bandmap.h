// $Id: bandmap.h 89 2015-01-03 13:59:15Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef BANDMAP_H
#define BANDMAP_H

/*!     \file bandmap.h

        Classes and functions related to bandmaps
*/

#include "cluster.h"
#include "drlog_context.h"
#include "pthread_support.h"
#include "rules.h"
#include "screen.h"
#include "serialization.h"
#include "statistics.h"

#include <array>
#include <list>
#include <string>
#include <utility>

enum BANDMAP_ENTRY_SOURCE { BANDMAP_ENTRY_LOCAL,
                            BANDMAP_ENTRY_CLUSTER,
                            BANDMAP_ENTRY_RBN
                          };                                       ///< possible sources for bandmap entries

enum BANDMAP_DIRECTION { BANDMAP_DIRECTION_DOWN,
                         BANDMAP_DIRECTION_UP
                       };                                          ///< search directions for the bandmap

extern const std::string MY_MARKER;                                ///< the string that marks my position in the bandmap

/*! \brief          Printable version of the name of a bandmap_entry source
    \param  bes     source of a bandmap entry
    \return         Printable version of <i>bes</i>
*/
const std::string to_string(const BANDMAP_ENTRY_SOURCE bes);

// -----------   needed_mult_details ----------------

/*!     \class needed_mult_details
        \brief Encapsulate the details of a type of mult associated with a bandmap entry
*/

template<typename T>
class needed_mult_details
{
protected:
  bool           _is_needed;     ///< are any mult values needed?
  std::set<T>    _values;        ///< values that are needed

public:

/// default constructor
  needed_mult_details(void) :
    _is_needed(false)
  { }

/*! \brief  constructor from a needed value
    \param  v   needed value
*/
  needed_mult_details(const T& v) :
    _is_needed(true)
  { _values.insert(v);
  }

/// is any value needed?
  inline const bool is_any_value_needed(void) const
    { return _is_needed; }

/// return all the needed values (as a set)
  inline const std::set<T> values(void) const
    { return _values; }

/*! \brief      add a needed value
    \param  v   needed value
    \return     whether <i>v</i> was actually inserted
*/
  const bool add(const T& v)
  { _is_needed = true;

    return (_values.insert(v)).second;
  }

/*! \brief      is a particular value needed?
    \param  v   value to test
    \return     whether <i>v</i> is needed
*/
  const bool is_value_needed(const T& v) const
  { if (!_is_needed)
      return false;

    return (_values.find(v) == _values.cend());
  }

/*! \brief      remove a needed value
    \param  v   value to remove
    \return     whether <i>v</i> was actually removed

    Doesn't remove <i>v</i> if no values are needed; does nothing if <i>v</i> is unknown
*/
  const bool remove(const T& v)
  { if (!_is_needed)
      return false;

    if (_values.find(v) == _values.cend())
      return false;

    const bool rv = (_values.erase(v) == 1);

    if (_values.empty())
      _is_needed = false;

    return rv;
  }

/// remove knowledge of all needed values
  void clear(void)
  { _is_needed = false;
    _values.clear();
  }

/// archive using boost serialization
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _is_needed
       & _values;
  }
};

/// ostream << needed_mult_details<pair<>>
template<typename S>
std::ostream& operator<<(std::ostream& ost, const needed_mult_details<std::pair<S, S>>& nmd)
{ ost << "is needed: " << nmd.is_any_value_needed() << std::endl
      << "values: " << std::endl;

  std::set<std::pair<S, S>> s = nmd.values();

  for (const auto& v : s)
    ost << "  value: " << v.first << ", " << v.second << std::endl;

  return ost;
}

/// ostream << needed_mult_details<>
template<typename T>
std::ostream& operator<<(std::ostream& ost, const needed_mult_details<T>& nmd)
{ ost << "is needed: " << nmd.is_any_value_needed() << std::endl
      << "values: " << std::endl;

  std::set<T> s = nmd.values();

  for (const auto& v : s)
    ost << "  value: " << v << std::endl;

  return ost;
}

// -----------   bandmap_filter_type ----------------

/*!     \class bandmap_filter_type
        \brief Control bandmap filtering
*/

class bandmap_filter_type
{
protected:

  bool                      _enabled;        ///< is bandmap filtering enabled?
  bool                      _hide;           ///< are we in hide mode? (as opposed to show)
  std::vector<std::string>  _prefixes;       ///< canonical country prefixes to filter
  std::vector<std::string>  _continents;     ///< continents to filter

public:

/// default constructor
  bandmap_filter_type(void) :
    _enabled(false),
    _hide(true)
  { }

  READ_AND_WRITE(enabled);                      ///< is bandmap filtering enabled?
  READ_AND_WRITE(hide);                         ///< are we in hide mode? (as opposed to show)
  READ(continents);                             ///< continents to filter
  READ(prefixes);                               ///< canonical country prefixes to filter

/*! \brief  All the continents and canonical prefixes that are currently being filtered
    \return all the continents and canonical prefixes that are currently being filtered

            The continents precede the canonical prefixes
*/
  const std::vector<std::string> filter(void) const;

/*!  \brief     Add a string to, or remove a string from, the filter
     \param str string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed.
*/
  void add_or_subtract(const std::string& str);
};

// -----------  bandmap_entry  ----------------

/*!     \class bandmap_entry
        \brief An entry in a bandmap
*/

class bandmap_entry
{
protected:

  frequency                 _freq;                    ///< QRG
  std::string               _frequency_str;           ///< QRG (kHz)
  std::string               _callsign;                ///< call
  std::string               _canonical_prefix;        ///< canonical prefix corresponding to the call
  std::string               _continent;               ///< continent corresponding to the call
  enum BAND                 _band;                    ///< band
  enum MODE                 _mode;                    ///< mode
  time_t                    _time;                    ///< time (in seconds since the epoch) at which the object was created
  enum BANDMAP_ENTRY_SOURCE _source;                  ///< the source of this entry
  time_t                    _expiration_time;         ///< time at which this entry expires (in seconds since the epoch)

  bool                      _is_needed;               ///< do we need this call?
  bool                      _is_needed_mult;          ///< is this a needed mult?

  std::set<std::string>     _posters;                 ///< stations that posted this entry

  needed_mult_details<std::pair<std::string, std::string>> _is_needed_callsign_mult;    ///< details of needed callsign mults
  needed_mult_details<std::string>                         _is_needed_country_mult;     ///< details of needed country mults
  needed_mult_details<std::pair<std::string, std::string>> _is_needed_exchange_mult;    ///< details of needed exchange mults

public:

/*! \brief      Default constructor
    \param  s   source of the entry (default is BANDMAP_ENTRY_LOCAL)
*/
  bandmap_entry(const BANDMAP_ENTRY_SOURCE s = BANDMAP_ENTRY_LOCAL);

/// define the sorting criterion to be applied to a pair of bandmap entries
  inline const bool operator<(const bandmap_entry& be) const
    { return (_freq.hz() < be._freq.hz() ); }

  READ(freq);                    ///< QRG
  READ(frequency_str);           ///< QRG (kHz)

/*! \brief      Set _freq and _frequency_str
    \param  f   frequency used to set the values
*/
  void freq(const frequency& f);           ///< set _freq and _frequency_str

  READ_AND_WRITE(callsign);                ///< call
  READ_AND_WRITE(canonical_prefix);        ///< canonical prefix corresponding to the call
  READ_AND_WRITE(continent);               ///< continent corresponding to the call
  READ_AND_WRITE(band);                    ///< band
  READ_AND_WRITE(mode);                    ///< mode
  READ_AND_WRITE(posters);                 ///< source(s) of posting (if the source is RBN)
  READ_AND_WRITE(time);                    ///< time (in seconds since the epoch) at which the object was created
  READ_AND_WRITE(expiration_time);         ///< time at which this entry expires (in seconds since the epoch)
  READ_AND_WRITE(source);                  ///< the source of this entry

  READ_AND_WRITE(is_needed);               ///< do we need this call?

/// was this bandmap_entry generated from the RBN?
  inline const bool is_rbn(void) const
    { return (_source == BANDMAP_ENTRY_RBN); }

/// does the call in this bandmap_entry match the value <i>str</i>?
  inline const bool call_is(const std::string& str) const
    { return (_callsign == str); }

/// does this entry correspond to me?
  inline const bool is_my_marker(void) const
    { return call_is(MY_MARKER); }

/*! \brief              Calculate the mult status of this entry
    \param  rules       the rules for this contest
    \param  statistics  the current statistics

    Adjust the mult status in accordance with the passed parameters
*/
  void calculate_mult_status(contest_rules& rules, running_statistics& statistics);

  inline const decltype(_is_needed_callsign_mult) is_needed_callsign_mult_details(void) const
    { return _is_needed_callsign_mult; }

  inline const decltype(_is_needed_country_mult) is_needed_country_mult_details(void) const
    { return _is_needed_country_mult; }

  inline const decltype(_is_needed_exchange_mult) is_needed_exchange_mult_details(void) const
    { return _is_needed_exchange_mult; }

  inline const bool is_needed_callsign_mult(void) const
    { return _is_needed_callsign_mult.is_any_value_needed(); }

  inline const bool is_needed_country_mult(void) const
    { return _is_needed_country_mult.is_any_value_needed(); }

  inline const bool is_needed_exchange_mult(void) const
    { return _is_needed_exchange_mult.is_any_value_needed(); }

  inline void add_callsign_mult(const std::string& name, const std::string& value)
    { _is_needed_callsign_mult.add( { name, value } ); }

  inline void add_country_mult(const std::string& value)
    { _is_needed_country_mult.add(value); }

  inline const bool add_exchange_mult(const std::string& name, const std::string& value)
    { return (_is_needed_exchange_mult.add( { name, value } ) ); }

  inline void clear_callsign_mult(void)
    { _is_needed_callsign_mult.clear(); }

  inline void clear_country_mult(void)
    { _is_needed_country_mult.clear(); }

  inline void clear_exchange_mult(void)
    { _is_needed_exchange_mult.clear(); }

  inline void remove_callsign_mult(const std::string& name, const std::string& value)
    { _is_needed_callsign_mult.remove( { name, value } ); }

  inline void remove_country_mult(const std::string& value)
    { _is_needed_country_mult.remove(value); }

  inline void remove_exchange_mult(const std::string& name, const std::string& value)
    { _is_needed_exchange_mult.remove( { name, value } ); }

  inline const bool is_needed_mult(void) const
    { return is_needed_callsign_mult() or is_needed_country_mult() or is_needed_exchange_mult(); }

// next three needed in order to pass as parameters to find_if, since I don't know how to choose among multiple overloaded functions

/// do we need this call?
  inline const bool is_stn_needed(void) const
    { return is_needed(); }

///  is this a needed mult?
  inline const bool is_a_needed_mult(void) const
    { return is_needed_mult(); }

/// does the _frequency_str match a target value?
  inline const bool is_frequency_str(const std::string& target) const
    { return (_frequency_str == target); }

/// a simple definition of whether there's useful information in the object
  inline const bool empty(void) const
    { return _callsign.empty(); }

  inline const bool valid(void) const
    { return !empty(); }

/*! \brief  Does this object match another bandmap_entry?
    \param  be  target bandmap entry
    \return whether frequency_str or callsign match

    Used in += function.
*/
  const bool matches_bandmap_entry(const bandmap_entry& be) const;
   
/// how long (in seconds) has it been since this entry was inserted into a bandmap?
  inline const time_t time_since_inserted(void) const
    { return ::time(NULL) - _time; }

/*! \brief  Should this bandmap_entry be removed?
    \param  now current time
    \return whether this bandmap_entry has expired
*/
  inline const bool should_prune(const time_t now = ::time(NULL)) const
    { return ( (_expiration_time < now) and (_callsign != MY_MARKER)); }

/// needed for a functor in const bandmap_entry bandmap::operator[](const std::string& str)
  const bool is_callsign(const std::string& str) const
    { return _callsign == str; }

/// needed for a functor in const bandmap_entry bandmap::substr(const std::string& str)
  const bool is_substr(const std::string& str) const
    { return contains(_callsign, str); }

/*! \brief              Re-mark the need/mult status
    \param  rules       rules for the contest
    \param  q_history   history of all the QSOs
    \param  statistics  statistics for the contest so far
    \return             whether there are any changes in needed/mult status

    <i>statistics</i> must be updated to be current before this is called
*/
  const bool remark(contest_rules& rules, call_history& q_history, running_statistics& statistics);

/// the number of posters
  inline const unsigned int n_posters(void) const
    { return _posters.size(); }

/// difference in frequency between two bandmap entries
  const frequency frequency_difference(const bandmap_entry& be) const;

/// order two bandmap entries, in order of callsign
  inline const bool less_by_callsign(const bandmap_entry& be) const
    { return (_callsign < be._callsign); }

/// order two bandmap entries, in order of frequency
  inline const bool less_by_frequency(const bandmap_entry& be) const
    { return (_freq.hz() < be._freq.hz()); }

/*! \brief          Add a call to the associated posters
    \param  call    call to add
    \return         number of posters associated with this call, after adding <i>call</i>

    Does nothing if <i>call</i> is already a poster
*/
  const unsigned int add_poster(const std::string& call);

/*! \brief          Is a particular call one of the posters?
    \param  call    call to test
    \return         Whether <i>call</i> is a poster
*/
  inline const bool is_poster(const std::string& call) const
    { return (_posters < call); }

/// return all the posters as a space-separated string
  const std::string posters_string(void) const;

/// guess the mode
  const MODE putative_mode(void) const;

/// archive using boost serialization
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _callsign
         & _canonical_prefix
         & _freq
         & _frequency_str
         & _posters
         & _continent
         & _band
//         & _mode
         & _time
         & _source
         & _expiration_time
         & _is_needed
         & _is_needed_mult
         & _is_needed_callsign_mult
         & _is_needed_country_mult
         & _is_needed_exchange_mult;
    }
};

std::ostream& operator<<(std::ostream& ost, const bandmap_entry& be);

// you'd think that BM_ENTRIES should be std::multiset<bandmap_entry>, but that's a royal pain with g++...
// remove_if internally calls the assignment operator, which is illegal... so basically means that in g++ set/multiset can't use remove_if
// one can use complex workarounds (remove_copy_if and then re-assign back to the original container), but that's ugly and in any case
// std::list seems to be fast enough
typedef std::list<bandmap_entry> BM_ENTRIES;
typedef const bool (bandmap_entry::* PREDICATE_FUN_P)(void) const;

// -----------  bandmap  ----------------

/*!     \class bandmap
        \brief A bandmap
*/

class bandmap
{
protected:

  bool                      _filtered_entries_dirty;                               ///< is the filtered version dirty?
  bool                      _rbn_threshold_and_filtered_entries_dirty;             ///< is the RBN threshold and filtered version dirty?
  
  BM_ENTRIES                _entries;                             ///< all the entries
  BM_ENTRIES                _filtered_entries;                    ///< entries, with the filter applied
  BM_ENTRIES                _rbn_threshold_and_filtered_entries;  ///< entries, with the filter and RBN threshold applied
  
//  auto lt = [] (const bandmap_entry& be1, const bandmap_entry& be2) { return be1.callsign() < be2.callsign(); };

//  std::set<bandmap_entry, decltype<lt>>  _call_entries;

  // recommended
//  std::set< int, std::function< bool(int,int) > > set_two(
//                                          [] ( int x, int y ) { return x>y ; } ) ; // fine

  std::set<std::string>     _recent_calls;     ///< calls recently added
  std::set<std::string>     _do_not_add;       ///< do not add these calls
  std::vector<int>          _fade_colours;     ///< the colours to use as entries age
  int                       _recent_colour;    ///< colour to use for entries < 120 seconds old (if black, then not used)

  bandmap_filter_type*      _filter_p;         ///< pointer to a bandmap filter
  int                       _column_offset;    ///< number of columns to offset start of displayed entries; used if there are two many entries to display them all

  pt_mutex                  _bandmap_mutex;    ///< mutex for this bandmap

  unsigned int _rbn_threshold;                 ///< number of posters needed before a station appears in the bandmap

/*!  \brief                         Return the callsign closest to a particular frequency, if it is within the guard band
     \param bme                     band map entries
     \param target_frequency_in_khz the target frequency, in kHz
     \param guard_band_in_hz        how far from the target to search, in Hz
     \return                        Callsign of a station within the guard band

     Returns the nearest station within the guard band, or the null string if no call is found.
*/
  const std::string _nearest_callsign(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz);

/*!  \brief     Insert a bandmap_entry
     \param be  entry to add

     Removes any extant entry at the same frequency as <i>be</i>
*/
  void _insert(const bandmap_entry& be);

/*!  \brief     Mark a bandmap_entry as recent
     \param be  entry to mark

     an entry will be marked as recent if:
       its source is LOCAL or CLUSTER
     or
       its source is RBN and the call is already present in the bandmap at the same QRG with the poster of <i>be</i>
*/
  const bool _mark_as_recent(const bandmap_entry& be);

/*!  \brief Mark filtered and rbn/filtered entries as dirty
*/
  void _dirty_entries(void);

public:

/// default constructor
  bandmap(void);

/// set the RBN threshold
  inline void rbn_threshold(const unsigned int n)
  { SAFELOCK (_bandmap);
    _rbn_threshold = n;
  }

/// the number of entries in the bandmap
  inline const size_t size(void)
    { SAFELOCK(_bandmap);
      return _entries.size(); 
    }
  
/// all the entries in the bandmap
  inline const BM_ENTRIES entries(void)
    { SAFELOCK(_bandmap);
      return _entries;
    }
    
/// the colours used as entries age
  inline const std::vector<int> fade_colours(void)
    { SAFELOCK(_bandmap);
      return _fade_colours;
    }

/// set the colours to use as entries age
  inline void fade_colours(const std::vector<int> fc)
    { _fade_colours = fc; }

/// the colour used for recent entries
  inline const int recent_colour(void)
    { SAFELOCK(_bandmap);
      return _recent_colour;
    }

/// set the colour used for recent entries
  inline void recent_colour(const int rc)
    { SAFELOCK(_bandmap);
      _recent_colour = rc;
    }

/*!  \brief                         Add a bandmap_entry
     \param be                      entry to add
*/
  void operator+=(const bandmap_entry& be);

/*! \brief return the entry for a particular call
    \param  callsign    call for which the entry should be returned
    \return the bandmap_entry corresponding to <i>callsign</i>

    Returns the default bandmap_entry if <i>callsign</i> is not present in the bandmap
*/
  const bandmap_entry operator[](const std::string& callsign);

/*! \brief return the first entry for a partial call
    \param  callsign    partial call for which the entry should be returned
    \return             the first bandmap_entry corresponding to <i>callsign</i>

    Returns the null string if <i>callsign</i> matches no entries in the bandmap
*/
  const bandmap_entry substr(const std::string& callsign);

/*! \brief remove a call from the bandmap
    \param  callsign    call to be removed

    Does nothing if <i>callsign</i> is not in the bandmap
*/
  void operator-=(const std::string& callsign);

/*! \brief set the needed status of a call to <i>false</i>
    \param  callsign    call for which the status should be set

    Does nothing if <i>callsign</i> is not in the bandmap
*/
  void not_needed(const std::string& callsign);

/*! \brief set the needed country mult status of all calls in a particular country to false
    \param  canonical_prefix    canonical prefix corresponding to country for which the status should be set

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
*/
  void not_needed_country_mult(const std::string& canonical_prefix);

/*! \brief                          set the needed callsign mult status of all matching callsign mults to <i>false</i>
    \param  pf                      pointer to function to return the callsign mult value
    \param  mult_type               name of mult type
    \param  callsign_mult_string    value of callsign mult value that is no longer a multiplier
*/
  void not_needed_callsign_mult(const std::string (*pf)(const std::string& /* e.g., "WPXPX" */, const std::string& /* callsign */),
                                const std::string& mult_type /* e.g., "WPXPX" */ , const std::string& callsign_mult_string /* e.g., SM1 */);

/*! \brief set the needed callsign mult status of all calls in a particular country to false
    \param  target_values   vector of target multiplier names and values
*/
  void not_needed_callsign_mult(const std::string (*pf)(const std::string& /* e.g., "WPXPX" */, const std::string& /* callsign */),
                                const std::vector<std::pair<std::string /* e.g., "WPXPX" */, std::string /* e.g., SM1 */>>& target_values);

  void not_needed_callsign_mult(const std::string& mult_type /* e.g., "WPXPX" */ , const std::string& callsign_mult_string /* e.g., SM1 */);

  void not_needed_exchange_mult(const std::string& mult_name, const std::string& mult_value);

/// prune the bandmap
  void prune(void);

// filter functions -- these affect all bandmaps, since there's just one (global) filter

/// is the filter enabled?
  inline const bool filter_enabled(void)
    { return _filter_p->enabled(); }

/// enable or disable the filter
  void filter_enabled(const bool torf);

/// return all the countries and continents currently in the filter
  inline const std::vector<std::string> filter(void)
    { return _filter_p->filter(); }

/*!  \brief Add a string to, or remove a string from, the filter associated with this bandmap
     \param str string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed. Currently, all bandmaps share a single
     filter.
*/
  void filter_add_or_subtract(const std::string& str);

/// is the filter in hide mode? (as opposed to show)
  inline const bool filter_hide(void)
    { return _filter_p->hide(); }

/// set or unset the filter to hide mode (as opposed to show)
  void filter_hide(const bool torf);

/// is the filter in show mode? (as opposed to hide)
  inline const bool filter_show(void)
    { return !_filter_p->hide(); }

/// set or unset the filter to show mode (as opposed to hide)
  void filter_show(const bool torf);

/// all the entries, after filtering has been applied
  const BM_ENTRIES filtered_entries(void);

/// all the entries, after the RBN threshold and filtering have been applied
  const BM_ENTRIES rbn_threshold_and_filtered_entries(void);

/// get the column offset
  inline int column_offset(void) const
    { return _column_offset; }

/// set the column offset
  inline void column_offset(const int n)
    { SAFELOCK(_bandmap);
      _column_offset = n;
    }

/*!  \brief Find the station in the RBN threshold and filtered bandmap that is closest to a target frequency
     \param target_frequency_in_khz target frequency, in kHz
     \param guard_band_in_hz        guard band, in Hz
     \return    Closest bandmap entry (if any) to the target frequency and within the guard band

     Applies filtering and the RBN threshold before searching for the station. Returns the
     empty string if no station was found within the guard band.
*/
    inline const std::string nearest_rbn_threshold_and_filtered_callsign(const float target_frequency_in_khz, const int guard_band_in_hz)
      { return _nearest_callsign(rbn_threshold_and_filtered_entries(), target_frequency_in_khz, guard_band_in_hz); }

/*!  \brief Find the next needed station up or down in frequency from the current location
     \param fp      pointer to function to be used to determine whether a station is needed
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found.
     Applies filtering and the RBN threshold before searching for the next station.
*/
  const bandmap_entry needed(PREDICATE_FUN_P fp, const enum BANDMAP_DIRECTION dirn);

/*!  \brief Find the next needed station (for a QSO) up or down in frequency from the current location
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station for a QSO in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_qso(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_stn_needed, dirn); }

/*!  \brief Find the next needed multiplier up or down in frequency from the current location
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed multiplier in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_mult(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_a_needed_mult, dirn); }

/*!  \brief Was a call recently added?
     \param callsign    callsign to test
     \return            whether <i>callsign</i> was added since the bandmap was last pruned
*/
  inline const bool is_recent_call(const std::string& callsign)
    { SAFELOCK(_bandmap);
      return (_recent_calls < callsign);
    }

/*!  \brief Add a call to the do-not-add list
     \param callsign    callsign to add

     Calls in the do-not-add list are never added to the bandmap
*/
  inline void do_not_add(const std::string& callsign)
    { SAFELOCK(_bandmap);
      _do_not_add.insert(callsign);
    }

/*!  \brief Remove a call from the do-not-add list
     \param callsign    callsign to add

     Calls in the do-not-add list are never added to the bandmap
*/
  inline void remove_from_do_not_add(const std::string& callsign)
    { SAFELOCK(_bandmap);
      _do_not_add.erase(callsign);
    }

/// convert to a printable string
  const std::string to_str(void);

/// serialize using boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_bandmap);

      ar & _entries
         & _do_not_add;
    }
};

/// window < bandmap
window& operator<(window& win, bandmap& bm);

/// ostream << bandmap
std::ostream& operator<<(std::ostream& ost, bandmap& bm);

#endif    // BANDMAP_H
