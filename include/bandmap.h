// $Id: bandmap.h 61 2014-05-03 16:34:34Z  $

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

const std::string to_string(const BANDMAP_ENTRY_SOURCE);


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

/*! \brief  add a needed value
    \param  v   needed value
*/
  void add(const T& v)
  { _is_needed = true;
    _values.insert(v);
  }

  const bool is_value_needed(const T& v) const
  { if (!_is_needed)
      return false;

    return (_values.find(v) == _values.cend());
  }

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

  void clear(void)
  { _is_needed = false;
    _values.clear();
  }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _is_needed
       & _values;
  }

};

template<typename S>
std::ostream& operator<<(std::ostream& ost, const needed_mult_details<std::pair<S, S>>& nmd)
{ ost << "is needed: " << nmd.is_any_value_needed() << std::endl
      << "values: " << std::endl;

  std::set<std::pair<S, S>> s = nmd.values();

  for (const auto& v : s)
    ost << "  value: " << v.first << ", " << v.second << std::endl;

  return ost;
}

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

  READ_AND_WRITE(enabled);                          ///< is bandmap filtering enabled?
  READ_AND_WRITE(hide);                             ///< are we in hide mode? (as opposed to show)
  READ(continents);  ///< continents to filter
  READ(prefixes);    ///< canonical country prefixes to filter

/// return all the canonical prefixes and continents that are currently being filtered
  const std::vector<std::string> filter(void) const;

/*!  \brief Add a string to, or remove a string from, the filter
     \param str string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; ortherwise it is removed.
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
//  enum MODE                 _mode;                    ///< mode
  time_t                    _time;                    ///< time (in seconds since the epoch) at which the object was created
  enum BANDMAP_ENTRY_SOURCE _source;                  ///< the source of this entry
  time_t                    _expiration_time;         ///< time at which this entry expires (in seconds since the epoch)

  bool                      _is_needed;               ///< do we need this call?
  bool                      _is_needed_mult;          ///< is this a needed mult?

  std::set<std::string>     _posters;                 ///< stations that posted this entry

  typedef std::pair<std::string, std::string> pss_type;

  needed_mult_details<std::pair<std::string, std::string>> _is_needed_callsign_mult;
  needed_mult_details<std::string>                         _is_needed_country_mult;
  needed_mult_details<std::pair<std::string, std::string>> _is_needed_exchange_mult;

public:

/// default constructor
//  bandmap_entry(void);

/// construct from particular source
  bandmap_entry(const BANDMAP_ENTRY_SOURCE s = BANDMAP_ENTRY_LOCAL);

/*! \brief  Construct from some useful stuff
    \param  post            a post from a cluster/RBN
    \param  expiration      time at which this entry will expire
    \param  is_needed       do we need to work this station on this band?
    \param  is_needed_mult  would this station be a mult on this band?
*/
//  bandmap_entry(const dx_post& post, const time_t expiration, const bool is_needed, const bool is_needed_mult);

//  bandmap_entry(const dx_post& post, const time_t expiration, const bool is_needed,
//                const bool is_needed_callsign_mult, const bool is_needed_country_mult, const bool is_needed_exchange_mult);

  bandmap_entry(const dx_post& post, const time_t expiration, const bool is_needed,
                const needed_mult_details<std::pair<std::string, std::string>>& is_needed_callsign_mult,
                const needed_mult_details<std::string>& is_needed_country_mult,
                const needed_mult_details<std::pair<std::string, std::string>>& is_needed_exchange_mult);

/// define the sorting criterion to be applied to a pair of bandmap entries
  inline const bool operator<(const bandmap_entry& be) const
    { return (_freq.hz() < be._freq.hz() ); }

  READ(freq);                    ///< QRG
  READ(frequency_str);           ///< QRG (kHz)

  void freq(const frequency& f);           ///< set _freq and _frequency_str

  READ_AND_WRITE(callsign);                ///< call
  READ_AND_WRITE(canonical_prefix);        ///< canonical prefix corresponding to the call
  READ_AND_WRITE(continent);               ///< continent corresponding to the call
  READ_AND_WRITE(band);                    ///< band
  READ_AND_WRITE(posters);                 ///< source(s) of posting (if the source is RBN)
  READ_AND_WRITE(time);                    ///< time (in seconds since the epoch) at which the object was created
  READ_AND_WRITE(expiration_time);         ///< time at which this entry expires (in seconds since the epoch)
  READ_AND_WRITE(source);                  ///< the source of this entry

  READ_AND_WRITE(is_needed);                      ///< do we need this call?

  void calculate_mult_status(contest_rules& rules, running_statistics& statistics);

  inline const needed_mult_details<pss_type> is_needed_callsign_mult_details(void) const
    { return _is_needed_callsign_mult; }

  inline const needed_mult_details<std::string> is_needed_country_mult_details(void) const
    { return _is_needed_country_mult; }

  inline const needed_mult_details<pss_type> is_needed_exchange_mult_details(void) const
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

  inline void add_exchange_mult(const std::string& name, const std::string& value)
    { _is_needed_exchange_mult.add( { name, value } ); }

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
   
/// how long (in seconds) has it been since this enty was inserted into a bandmap?
  inline const time_t time_since_inserted(void) const
    { return ::time(NULL) - _time; }

/*! \brief  Should this bandmap_entry be removed?
    \param  now current time
    \return whether this bandmap_entry has expired
*/
  inline const bool should_prune(const time_t now = ::time(NULL)) const
    { return ( (_expiration_time < now) and (_callsign != "--------")); }

/// needed for a functor in const bandmap_entry bandmap::operator[](const std::string& str)
  const bool is_callsign(const std::string& str) const
    { return _callsign == str; }

/// needed for a functor in const bandmap_entry bandmap::substr(const std::string& str)
  const bool is_substr(const std::string& str) const
    { return contains(_callsign, str); }

// re-mark as to the need/mult status
  const bool remark(contest_rules& rules, call_history& q_history, running_statistics& statistics);

/// the number of posters
  inline const unsigned int n_posters(void) const
    { return _posters.size(); }

/// difference in frequency between two bandmap entries
  const frequency frequency_difference(const bandmap_entry& be) const;

  inline const bool less_by_callsign(const bandmap_entry& be) const
    { return (_callsign < be._callsign); }

  inline const bool less_by_frequency(const bandmap_entry& be) const
    { return (_freq.hz() < be._freq.hz()); }

  const unsigned int add_poster(const std::string& call);

  inline const bool is_poster(const std::string& call) const
    { return (_posters < call); }

  const std::string posters_string(void) const;

//  const std::string to_string(void) const;

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
  
  BM_ENTRIES                _entries;          ///< all the entries
  
//  auto lt = [] (const bandmap_entry& be1, const bandmap_entry& be2) { return be1.callsign() < be2.callsign(); };

//  std::set<bandmap_entry, decltype<lt>>  _call_entries;

  // recommended
//  std::set< int, std::function< bool(int,int) > > set_two(
//                                          [] ( int x, int y ) { return x>y ; } ) ; // fine

  std::set<std::string>     _recent_calls;     ///< calls recently added
  std::set<std::string>     _do_not_add;       ///< do not add these calls
  std::vector<int>          _fade_colours;     ///< the colours to use as entries age

  bandmap_filter_type*      _filter_p;         ///< pointer to a bandmap filter
  int                       _column_offset;    ///< number of columns to offset start of displayed entries; used if there are two many entries to display them all

  pt_mutex                  _bandmap_mutex;    ///< mutex for this bandmap

  unsigned int _rbn_threshold;

/*!  \brief Return a callsign close to a particular frequency
     \param bme                     band map entries
     \param target_frequency_in_khz the target frequency, in kHz
     \param guard_band_in_hz        how far from the target to search, in Hz
     \return                        Callsign of a station within the guard band

     Returns the lowest-frequency station within the guard band, or the null string if no call is found.
*/
  const std::string _nearby_callsign(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz);

  const std::vector<std::string> _nearby_callsigns(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz);

// insert an entry at the right place
  void _insert(const bandmap_entry& be);

  const bool _mark_as_recent(const bandmap_entry& be);

public:

/// default constructor
  bandmap(void);

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

/// add an entry to the bandmap
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

/*! \brief set the needed status of a call to false
    \param  callsign    call for which the status should be set

    Does nothing if <i>callsign</i> is not in the bandmap
*/
  void not_needed(const std::string& callsign);

/*! \brief set the needed country mult status of all calls in a particular country to false
    \param  canonical_prefix    canonical prefix corresponding to country for which the status should be set
    \param  location_db         location database derived from CTY file

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
*/
//  void not_needed_country_mult(const std::string& canonical_prefix, location_database& location_db);

  void not_needed_country_mult(const std::string& canonical_prefix);

//  typedef

/*! \brief set the needed callsign mult status of all calls in a particular country to false
    \param  pf          pointer to function to return the callsign mult value
    \param  callsign_mult_string value of callsign mult value that is no longer a multiplier

    Does nothing if no calls from the country identified by <i>canonical_prefix</i> are in the bandmap
*/
  void not_needed_callsign_mult(const std::string (*pf)(const std::string& /* e.g., "WPXPX" */, const std::string& /* callsign */),
                                const std::string& mult_type /* e.g., "WPXPX" */ , const std::string& callsign_mult_string /* e.g., SM1 */);

  void not_needed_callsign_mult(const std::string (*pf)(const std::string& /* e.g., "WPXPX" */, const std::string& /* callsign */),
                                const std::vector<std::pair<std::string /* e.g., "WPXPX" */, std::string /* e.g., SM1 */>>& target_values);

  void not_needed_callsign_mult(const std::string& mult_type /* e.g., "WPXPX" */ , const std::string& callsign_mult_string /* e.g., SM1 */);

  void not_needed_exchange_mult(const std::string& mult_name, const std::string& mult_value);

/// prune the bandmap
  void prune(void);

// filter functions -- these affect all bandmaps, since there's just one filter object

/// is the filter enabled?
  inline const bool filter_enabled(void)
    { return _filter_p->enabled(); }

/// enable or disable the filter
  inline void filter_enabled(const bool torf)
    { _filter_p->enabled(torf); }

/// return all the countries and continents currently in the filter
  inline const std::vector<std::string> filter(void)
    { return _filter_p->filter(); }

/*!  \brief Add a string to, or remove a string from, the filter associated with this bandmap
     \param str string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed. Currently, all bandmaps share a single
     filter.
*/
  inline void filter_add_or_subtract(const std::string& str)
    { _filter_p->add_or_subtract(str); }

/// is the filter in hide mode? (as opposed to show)
  inline const bool filter_hide(void)
    { return _filter_p->hide(); }

/// set or unset the filter to hide mode (as opposed to show)
  inline void filter_hide(const bool torf)
    { _filter_p->hide(torf); }

/// is the filter in show mode? (as opposed to hide)
  inline const bool filter_show(void)
    { return !_filter_p->hide(); }

/// set or unset the filter to show mode (as opposed to hide)
  inline void filter_show(const bool torf)
    { _filter_p->hide(!torf); }

/// all the entries, after filtering has been applied
  const BM_ENTRIES filtered_entries(void);

  const BM_ENTRIES rbn_threshold_and_filtered_entries(void);

/// the current column offset
  inline int column_offset(void) const
    { return _column_offset; }

/// set the column offset
  inline void column_offset(const int n)
    { SAFELOCK(_bandmap);
      _column_offset = n;
    }

/*!  \brief Return a callsign close to a particular frequency
     \param target_frequency_in_khz the target frequency, in kHz
     \param guard_band_in_hz        how far from the target to search, in Hz
     \return                        Callsign of a station within the guard band

     Returns the lowest-frequency station within the guard band, or the null string if no call is found.
*/
//  const std::string nearby_callsign(const float target_frequency_in_khz, const int guard_band_in_hz);
    inline const std::string nearby_callsign(const float target_frequency_in_khz, const int guard_band_in_hz)
      {  return _nearby_callsign(_entries, target_frequency_in_khz, guard_band_in_hz); }

    inline const std::vector<std::string> nearby_callsigns(const float target_frequency_in_khz, const int guard_band_in_hz)
      {  return _nearby_callsigns(_entries, target_frequency_in_khz, guard_band_in_hz); }

/*!  \brief Return a callsign close to a particular frequency, using the filtered version of the bandmap
     \param target_frequency_in_khz the target frequency, in kHz
     \param guard_band_in_hz        how far from the target to search, in Hz
     \return                        Callsign of a station within the guard band

     Returns the lowest-frequency station within the guard band, or the null string if no call is found.
*/
//    const std::string nearby_filtered_callsign(const float target_frequency_in_khz, const int guard_band_in_hz);
    inline const std::string nearby_filtered_callsign(const float target_frequency_in_khz, const int guard_band_in_hz)
      { return _nearby_callsign(filtered_entries(), target_frequency_in_khz, guard_band_in_hz); }

    inline const std::vector<std::string> nearby_filtered_callsigns(const float target_frequency_in_khz, const int guard_band_in_hz)
      { return _nearby_callsigns(filtered_entries(), target_frequency_in_khz, guard_band_in_hz); }

/*!  \brief Find the next needed station up or down in frequency from the current loction
     \param fp      pointer to function to be used to determine whather a station is needed
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

std::ostream& operator<<(std::ostream& ost, bandmap& be);

#endif    // BANDMAP_H
