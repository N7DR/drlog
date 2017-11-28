// $Id: bandmap.h 140 2017-11-05 15:16:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef BANDMAP_H
#define BANDMAP_H

/*! \file   bandmap.h

    Classes and functions related to bandmaps
*/

#include "cluster.h"
#include "drlog_context.h"
#include "log.h"
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
                          };                                    ///< possible sources for bandmap entries

enum BANDMAP_DIRECTION { BANDMAP_DIRECTION_DOWN,
                         BANDMAP_DIRECTION_UP
                       };                                       ///< search directions for the bandmap

// forward declarations
class bandmap_filter_type;

extern const std::string   MY_MARKER;                           ///< the string that marks my position in the bandmap
extern const std::string   MODE_MARKER;                         ///< the string that marks the mode break in the bandmap
extern bandmap_filter_type BMF;                                 ///< the bandmap filter
extern old_log             olog;                                ///< old (ADIF) log containing QSO and QSL information

/*! \brief          Printable version of the name of a bandmap_entry source
    \param  bes     source of a bandmap entry
    \return         printable version of <i>bes</i>
*/
const std::string to_string(const BANDMAP_ENTRY_SOURCE bes);

// -----------   bandmap_buffer_entry ----------------

/*! \class  bandmap_buffer_entry
    \brief  Class for entries in the bandmap_buffer class
*/

class bandmap_buffer_entry
{
protected:
  std::set<std::string>  _posters;           // the posters

public:

  inline const unsigned int size(void) const
    { return _posters.size(); }

  const unsigned int add(const std::string& new_poster);
};

// -----------   bandmap_buffer ----------------

/*! \class  bandmap_buffer
    \brief  Class to control which cluster/RBN posts reach the bandmaps

    A single bandmap_buffer is used to control all bandmaps
*/

class bandmap_buffer
{
protected:
  unsigned int _min_posters;        ///< minumum number of posters needed to appear on bandmap, default = 1

  std::map<std::string /* call */, bandmap_buffer_entry>  _data;    ///< the database

  pt_mutex _bandmap_buffer_mutex;                                   ///< mutex for thread-safe access

public:

/// default constructor
  bandmap_buffer(const unsigned int n_posters = 1);

  READ_AND_WRITE(min_posters);      ///< minumum number of posters needed to appear on bandmap

/*! \brief              Get the number of posters associated with a call
    \param  callsign    the callsign to test
    \return             The number of posters associated with <i>callsign</i>
*/
  const unsigned int n_posters(const std::string& callsign);

/*! \brief              Associate a poster with a call
    \param  callsign    the callsign
    \param  poster      poster to associate with <i>callsign</i>
    \return             The number of posters associated with <i>callsign</i>, after this association is added

    Creates an entry in the buffer if no entry for </i>callsign</i> exists
*/
  const unsigned int add(const std::string& callsign, const std::string& poster);

/*! \brief              Are there sufficient posters of a call to allow it to appear on the bandmap?
    \param  callsign    the callsign to test
    \return             Whether the number of posters associated with <i>callsign</i> is equal to or greater than the necessary minimum
*/
  inline const bool sufficient_posters(const std::string& callsign)
    { return (n_posters(callsign) >= _min_posters); }
};


// -----------   needed_mult_details ----------------

/*! \class  needed_mult_details
    \brief  Encapsulate the details of a type of mult associated with a bandmap entry
*/

template<typename T>
class needed_mult_details
{
protected:
  bool           _is_needed;        ///< are any mult values needed?
  bool           _is_status_known;  ///< is the status known for sure?
  std::set<T>    _values;           ///< values that are needed

public:

/// default constructor
  needed_mult_details(void) :
    _is_needed(false),
    _is_status_known(true)          //  for backwards compatibility
  { }

/*! \brief      Constructor from a needed value
    \param  v   needed value
*/
  needed_mult_details(const T& v) :
    _is_needed(true)
  { _values.insert(v);
  }

/// is any value needed?
  inline const bool is_any_value_needed(void) const
    { return _is_needed; }

/// is the status known?
  inline const bool is_status_known(void) const
    { return _is_status_known; }

/// is sthe tatus known?
  inline void status_is_known(const bool torf)
    { _is_status_known = torf; }

/// return all the needed values (as a set)
  inline const std::set<T> values(void) const
    { return _values; }

/*! \brief      Add a needed value
    \param  v   needed value
    \return     whether <i>v</i> was actually inserted
*/
  const bool add(const T& v)
  { _is_needed = true;

    return (_values.insert(v)).second;
  }

/*! \brief      Is a particular value needed?
    \param  v   value to test
    \return     whether <i>v</i> is needed
*/
  const bool is_value_needed(const T& v) const
  { if (!_is_needed)
      return false;

    return (_values.find(v) == _values.cend());
  }

/*! \brief      Remove a needed value
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
  { _is_status_known = false;
    _is_needed = false;
    _values.clear();
  }

/// archive using boost serialization
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _is_needed
       & _is_status_known
       & _values;
  }
};

/// ostream << needed_mult_details<pair<>>
template<typename S>
std::ostream& operator<<(std::ostream& ost, const needed_mult_details<std::pair<S, S>>& nmd)
{ ost << "is needed: " << nmd.is_any_value_needed() << std::endl
      << "is status known: " << nmd.is_status_known() << std::endl
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
      << "is status known: " << nmd.is_status_known() << std::endl
      << "values: " << std::endl;

  const std::set<T> s = nmd.values();

  for (const auto& v : s)
    ost << "  value: " << v << std::endl;

  return ost;
}

// -----------   bandmap_filter_type ----------------

/*! \class  bandmap_filter_type
    \brief  Control bandmap filtering
*/

class bandmap_filter_type
{
protected:

  std::vector<std::string>  _continents;     ///< continents to filter
  bool                      _enabled;        ///< is bandmap filtering enabled?
  bool                      _hide;           ///< are we in hide mode? (as opposed to show)
  std::vector<std::string>  _prefixes;       ///< canonical country prefixes to filter

public:

/// default constructor
  bandmap_filter_type(void) :
    _enabled(false),
    _hide(true)
  { }

  READ(continents);                             ///< continents to filter
  READ_AND_WRITE(enabled);                      ///< is bandmap filtering enabled?
  READ_AND_WRITE(hide);                         ///< are we in hide mode? (as opposed to show)
  READ(prefixes);                               ///< canonical country prefixes to filter

/*! \brief      Get all the continents and canonical prefixes that are currently being filtered
    \return     all the continents and canonical prefixes that are currently being filtered

    The continents precede the canonical prefixes
*/
  const std::vector<std::string> filter(void) const;

/*!  \brief         Add a string to, or remove a string from, the filter
     \param str     string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed.
*/
  void add_or_subtract(const std::string& str);

/// archive using boost serialization
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _enabled
       & _hide
       & _prefixes
       & _continents;
  }
};

// -----------  bandmap_entry  ----------------

/*! \class  bandmap_entry
    \brief  An entry in a bandmap
*/

class bandmap_entry
{
protected:

  enum BAND                                                 _band;                              ///< band
  std::string                                               _callsign;                          ///< call
  std::string                                               _canonical_prefix;                  ///< canonical prefix corresponding to the call
  std::string                                               _continent;                         ///< continent corresponding to the call
  time_t                                                    _expiration_time;                   ///< time at which this entry expires (in seconds since the epoch)
  frequency                                                 _freq;                              ///< QRG
  std::string                                               _frequency_str;                     ///< QRG (kHz, to 1 dp)
  bool                                                      _is_needed;                         ///< do we need this call?
  needed_mult_details<std::pair<std::string, std::string>>  _is_needed_callsign_mult;           ///< details of needed callsign mults
  needed_mult_details<std::string>                          _is_needed_country_mult;            ///< details of needed country mults
  needed_mult_details<std::pair<std::string, std::string>>  _is_needed_exchange_mult;           ///< details of needed exchange mults
  enum MODE                                                 _mode;                              ///< mode
  bool                                                      _mult_status_is_known;              ///< true only after calculate_mult_status() has been called
//  std::set<std::string>                                     _posters;                   ///< stations that posted this entry
  enum BANDMAP_ENTRY_SOURCE                                 _source;                            ///< the source of this entry
  time_t                                                    _time;                              ///< time (in seconds since the epoch) at which the object was created
  time_t                                                    _time_of_earlier_bandmap_entry;     ///< time of bandmap_entry that this bandmap_entry replaced; 0 => not a replacement

public:

/*! \brief      Default constructor
    \param  s   source of the entry (default is BANDMAP_ENTRY_LOCAL)
*/
  bandmap_entry(const BANDMAP_ENTRY_SOURCE s = BANDMAP_ENTRY_LOCAL);

/*! \brief      Define the sorting criterion to be applied to a pair of bandmap entries: sort by frequency
    \param  be  comparison bandmap_entry
    \return     whether <i>this</i> should be sorted earlier than <i>be</i>
 *
 */
  inline const bool operator<(const bandmap_entry& be) const
    { return (_freq.hz() < be._freq.hz() ); }

  READ(band);                           ///< band
  READ(callsign);                       ///< call

/*! \brief          Set the callsign
    \param  call    the callsign to set
*/
  void callsign(const std::string& call);

  READ(canonical_prefix);               ///< canonical prefix corresponding to the call
  READ(continent);                      ///< continent corresponding to the call
  READ_AND_WRITE(expiration_time);      ///< time at which this entry expires (in seconds since the epoch)

  READ(freq);                           ///< QRG

/*! \brief      Set <i>_freq</i>, <i>_frequency_str</i>, <i>_band</i> and <i>_mode</i>
    \param  f   frequency used to set the values
*/
  void freq(const frequency& f);

  READ(frequency_str);                  ///< QRG (kHz, to 1 dp)

//  READ_AND_WRITE(is_needed);            ///< do we need this call?
  inline const bool is_needed(void) const
    { return ( _is_needed and !is_marker() ); }    // we never need a marker, regardless of the value of _is_needed

  WRITE(is_needed);                     ///< do we need this call?

  READ_AND_WRITE(mode);                 ///< mode
  READ(mult_status_is_known);
//  READ_AND_WRITE(posters);              ///< callsign(s) that posted the post(s) (if the source is RBN)
  READ_AND_WRITE(source);               ///< the source of this entry
  READ(time);                           ///< time (in seconds since the epoch) at which the object was created
  READ(time_of_earlier_bandmap_entry);  ///< time (in seconds since the epoch) of object that this object replaced

/// was this bandmap_entry generated from the RBN?
  inline const bool is_rbn(void) const
    { return (_source == BANDMAP_ENTRY_RBN); }

/// does the call in this bandmap_entry match the value <i>str</i>?
  inline const bool call_is(const std::string& str) const
    { return (_callsign == str); }

/// does this entry correspond to me?
  inline const bool is_my_marker(void) const
    { return call_is(MY_MARKER); }

/// does this entry correspond to the mode marker?
  inline const bool is_mode_marker(void) const
    { return call_is(MODE_MARKER); }

/// does this entry correspond to either kind of marker?
  inline const bool is_marker(void) const
    { return is_my_marker() or is_mode_marker(); }

/*! \brief              Calculate the mult status of this entry
    \param  rules       the rules for this contest
    \param  statistics  the current statistics

    Adjust the mult status in accordance with the passed parameters
*/
  void calculate_mult_status(contest_rules& rules, running_statistics& statistics);

/// return the details of any callsign mults
  inline const decltype(_is_needed_callsign_mult) is_needed_callsign_mult_details(void) const
    { return _is_needed_callsign_mult; }

/// return the details of a country mult
  inline const decltype(_is_needed_country_mult) is_needed_country_mult_details(void) const
    { return _is_needed_country_mult; }

/// return the details of any exchange mults
  inline const decltype(_is_needed_exchange_mult) is_needed_exchange_mult_details(void) const
    { return _is_needed_exchange_mult; }

/// is this a needed callsign mult?
  inline const bool is_needed_callsign_mult(void) const
    { return _is_needed_callsign_mult.is_any_value_needed(); }

/// is this a needed country mult?
  inline const bool is_needed_country_mult(void) const
    { return _is_needed_country_mult.is_any_value_needed(); }

/// is this a needed exchange mult?
  inline const bool is_needed_exchange_mult(void) const
    { return _is_needed_exchange_mult.is_any_value_needed(); }

/*! \brief          Add a value of callsign mult
    \param  name    name of the mult
    \param  value   value of the mult

    Does nothing if the value <i>value</i> is already known for the mult <i>name</i>
*/
  inline void add_callsign_mult(const std::string& name, const std::string& value)
    { _is_needed_callsign_mult.add( { name, value } ); }

/*! \brief          Add a value of country mult
    \param  value   value of the mult

    Does nothing if the value <i>value</i> is already known
*/
  inline void add_country_mult(const std::string& value)
    { _is_needed_country_mult.add(value); }

/*! \brief          Add a value of exchange mult
    \param  name    name of the mult
    \param  value   value of the mult
    \return         whether the mult was actually added
*/
  inline const bool add_exchange_mult(const std::string& name, const std::string& value)
    { return (_is_needed_exchange_mult.add( { name, value } ) ); }

/// remove all callsign mults
  inline void clear_callsign_mult(void)
    { _is_needed_callsign_mult.clear(); }

/// remove all country mults
  inline void clear_country_mult(void)
    { _is_needed_country_mult.clear(); }

/// remove all exchange mults
  inline void clear_exchange_mult(void)
    { _is_needed_exchange_mult.clear(); }

/*! \brief          Remove a particular value of a callsign mult
    \param  name    name of the mult
    \param  value   value of the mult

    Does nothing if the value <i>value</i> of mult <i>name</i> is unknown
*/
  inline void remove_callsign_mult(const std::string& name, const std::string& value)
    { _is_needed_callsign_mult.remove( { name, value } ); }

/*! \brief          Remove a particular value of country mult
    \param  value   value of the mult

    Does nothing if the value <i>value</i> is unknown
*/
  inline void remove_country_mult(const std::string& value)
    { _is_needed_country_mult.remove(value); }

/*! \brief          Remove a particular value of an exchange mult
    \param  name    name of the exchange mult
    \param  value   value of the mult

    Does nothing if the value <i>value</i> is unknown for the mult <i>name</i>
*/
  inline void remove_exchange_mult(const std::string& name, const std::string& value)
    { _is_needed_exchange_mult.remove( { name, value } ); }

/// is this a needed mult of any type?
  inline const bool is_needed_mult(void) const
    { return is_needed_callsign_mult() or is_needed_country_mult() or is_needed_exchange_mult(); }

// next three needed in order to pass as parameters to find_if, since I don't know how to choose among multiple overloaded functions

/// do we need to work this call?
  inline const bool is_stn_needed(void) const
    { return is_needed(); }

///  is this a needed mult?
  inline const bool is_a_needed_mult(void) const
    { return is_needed_mult(); }

/// does the _frequency_str match a target value?
  inline const bool is_frequency_str(const std::string& target) const
    { return (_frequency_str == target); }

/// set frequency string to particular number of decimal places (in kHz)
  inline void frequency_str_decimal_places(const int n)
    { _frequency_str = decimal_places(_frequency_str, n); }

/// a simple definition of whether there is no useful information in the object
  inline const bool empty(void) const
    { return _callsign.empty(); }

/// a simple definition of whether there is useful information in the object
  inline const bool valid(void) const
    { return !empty(); }

/*! \brief      Does this object match another bandmap_entry?
    \param  be  target bandmap entry
    \return     whether frequency_str or callsign match

    Used in += function.
*/
  const bool matches_bandmap_entry(const bandmap_entry& be) const;
   
/// how long (in seconds) has it been since this entry was inserted into a bandmap?
  inline const time_t time_since_inserted(void) const
    { return (::time(NULL) - _time); }

/// how long (in seconds) has it been since this entry or its predecessor was inserted into a bandmap?
  inline const time_t time_since_this_or_earlier_inserted(void) const
    { return ( ::time(NULL) - (_time_of_earlier_bandmap_entry ? _time_of_earlier_bandmap_entry : _time) ); }

/*! \brief          Should this bandmap_entry be removed?
    \param  now     current time
    \return         whether this bandmap_entry has expired
*/
  inline const bool should_prune(const time_t now = ::time(NULL)) const
    { return ( (_expiration_time < now) and !is_marker()); }

/*! \brief              Re-mark the need/mult status
    \param  rules       rules for the contest
    \param  q_history   history of all the QSOs
    \param  statistics  statistics for the contest so far
    \return             whether there were any changes in needed/mult status

    <i>statistics</i> must be updated to be current before this is called
*/
  const bool remark(contest_rules& rules, call_history& q_history, running_statistics& statistics);

/// the number of posters
//  inline const unsigned int n_posters(void) const
//    { return _posters.size(); }

/*! \brief      Return the (absolute) difference in frequency between two bandmap entries
    \param  be  other bandmap entry
    \return     difference in frequency between *this and <i>be</i>
*/
  inline const frequency frequency_difference(const bandmap_entry& be) const
    { return frequency(abs(be._freq.hz() - _freq.hz()), FREQ_HZ); }

/*! \brief      Return the difference in frequency between two bandmap entries, in +ve hertz
    \param  be  other bandmap entry
    \return     absolute difference in hertz between frequency of *this and fequency of <i>be</i>
*/
  inline const unsigned int absolute_frequency_difference(const bandmap_entry& be) const
    { return static_cast<unsigned int>(abs(frequency_difference(be).hz())); }

/*! \brief      Is this bandmap entry less than another one, using callsign order
    \param  be  other bandmap entry
    \return     whether *this is less than <i>be</i>, using callsign order
*/
  inline const bool less_by_callsign(const bandmap_entry& be) const
    { return (_callsign < be._callsign); }

/*! \brief      Is this bandmap entry less than another one, using frequency order
    \param  be  other bandmap entry
    \return     whether *this is less than <i>be</i>, using frequency order
*/
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
//  inline const bool is_poster(const std::string& call) const
//    { return (_posters < call); }

/// return all the posters as a space-separated string
  const std::string posters_string(void) const;

/// guess the mode
  const MODE putative_mode(void) const;

/// how many QSOs have we had (before this contest) with this callsign, band and mode?
  inline const unsigned int n_qsos(void) const
    { return olog.n_qsos(_callsign, _band, _mode); }

/// is this call+band+mode an all-time first?
  inline const bool is_all_time_first(void) const
    { return (n_qsos() == 0); }

/// is this call+band+mode an all-time first and also a needed QSO?
  inline const bool is_all_time_first_and_needed_qso(void) const
    { return (is_all_time_first() and is_needed()); }

/// is this a needed call for which the call+band+mode is an all-time first, or have we received a qsl for this call+band+mode
  inline const bool is_new_or_previously_qsled(void) const
//    { return (is_all_time_first() or olog.confirmed(_callsign, _band, _mode)); }
//    { return (is_all_time_first_and_needed_qso() or olog.confirmed(_callsign, _band, _mode)); }
    { return (is_needed() and (is_all_time_first() or olog.confirmed(_callsign, _band, _mode))); }

// set value from an earlier be
  void time_of_earlier_bandmap_entry(const bandmap_entry& old_be);

/// archive using boost serialization
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _band
         & _callsign
         & _canonical_prefix
         & _continent
         & _expiration_time
         & _freq
         & _frequency_str
         & _is_needed
         & _is_needed_callsign_mult
         & _is_needed_country_mult
         & _is_needed_exchange_mult
         & _mode
//         & _posters
         & _source
         & _time;
    }
};

/// ostream << bandmap_entry
std::ostream& operator<<(std::ostream& ost, const bandmap_entry& be);

// you'd think that BM_ENTRIES should be std::multiset<bandmap_entry>, but that's a royal pain with g++...
// remove_if internally calls the assignment operator, which is illegal... so basically means that in g++ set/multiset can't use remove_if
// one can use complex workarounds (remove_copy_if and then re-assign back to the original container), but that's ugly and in any case
// std::list seems to be fast enough
typedef std::list<bandmap_entry> BM_ENTRIES;
typedef const bool (bandmap_entry::* PREDICATE_FUN_P)(void) const;

// -----------  bandmap  ----------------

/*! \class  bandmap
    \brief  A bandmap
*/

class bandmap
{
protected:

  pt_mutex                  _bandmap_mutex;                             ///< mutex for this bandmap
  int                       _column_offset;                             ///< number of columns to offset start of displayed entries; used if there are two many entries to display them all
  std::set<std::string>     _do_not_add;                                ///< do not add these calls
  BM_ENTRIES                _entries;                                   ///< all the entries
  std::vector<int>          _fade_colours;                              ///< the colours to use as entries age
  decltype(_entries)        _filtered_entries;                          ///< entries, with the filter applied
  bool                      _filtered_entries_dirty;                    ///< is the filtered version dirty?
  bandmap_filter_type*      _filter_p;                                  ///< pointer to a bandmap filter
  frequency                 _mode_marker_frequency;                     ///< the frequency of the mode marker
  unsigned int              _rbn_threshold;                             ///< number of posters needed before a station appears in the bandmap
  decltype(_entries)        _rbn_threshold_and_filtered_entries;        ///< entries, with the filter and RBN threshold applied
  bool                      _rbn_threshold_and_filtered_entries_dirty;  ///< is the RBN threshold and filtered version dirty?
  std::set<std::string>     _recent_calls;                              ///< calls recently added
  int                       _recent_colour;                             ///< colour to use for entries < 120 seconds old (if black, then not used)

///  Mark filtered and rbn/filtered entries as dirty
  void _dirty_entries(void);

/*!  \brief     Insert a bandmap_entry
     \param be  entry to add
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

/*!  \brief                             Return the callsign closest to a particular frequency, if it is within the guard band
     \param bme                         band map entries
     \param target_frequency_in_khz     the target frequency, in kHz
     \param guard_band_in_hz            how far from the target to search, in Hz
     \return                            Callsign of a station within the guard band

     Returns the nearest station within the guard band, or the null string if no call is found.
     As currently implemented, assumes that the entries are in order of monotonically increasing or decreasing frequency
*/
  const std::string _nearest_callsign(const BM_ENTRIES& bme, const float target_frequency_in_khz, const int guard_band_in_hz);

public:

/// default constructor
  bandmap(void);

  SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX(mode_marker_frequency, _bandmap);             ///< the frequency of the mode marker

/// get the current bandmap filter
  inline const bandmap_filter_type bandmap_filter(void)
  { SAFELOCK (_bandmap);
    return *(_filter_p);
  }

/*!  \brief     Set the RBN threshold
     \param n   new value of the threshold
*/
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
  SAFEREAD_WITH_INTERNAL_MUTEX(entries, _bandmap);
    
/// the colours used as entries age
  SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX(fade_colours, _bandmap);

/// the colour used for recent entries
  SAFE_READ_AND_WRITE_WITH_INTERNAL_MUTEX(recent_colour, _bandmap);

/// set the colour used for recent entries
//  inline void recent_colour(const int rc)
//    { SAFELOCK(_bandmap);
//      _recent_colour = rc;
//    }

/*!  \brief     Add a bandmap_entry
     \param be  entry to add

     <i>_time_of_earlier_bandmap_entry</i> in <i>be</i> might be changed.
     Could change this by copying the be inside +=.
*/
  void operator+=(bandmap_entry& be);

/*! \brief              Return the entry for a particular call
    \param  callsign    call for which the entry should be returned
    \return             the bandmap_entry corresponding to <i>callsign</i>

    Returns the default bandmap_entry if <i>callsign</i> is not present in the bandmap
*/
  const bandmap_entry operator[](const std::string& callsign);

/*! \brief              Return the first entry for a partial call
    \param  callsign    partial call for which the entry should be returned
    \return             the first bandmap_entry corresponding to <i>callsign</i>

    Returns the null string if <i>callsign</i> matches no entries in the bandmap
*/
  const bandmap_entry substr(const std::string& callsign);

/*! \brief              Remove a call from the bandmap
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
                                const std::string& mult_type /* e.g., "WPXPX" */ , const std::string& callsign_mult_string /* e.g., "SM1" */);

/*! \brief                          Set the needed callsign mult status of all matching callsign mults to <i>false</i>
    \param  mult_type               name of mult type
    \param  callsign_mult_string    value of callsign mult value that is no longer a multiplier
*/
  void not_needed_callsign_mult(const std::string& mult_type /* e.g., "WPXPX" */ , const std::string& callsign_mult_string /* e.g., "SM1" */);

/*! \brief              Set the needed exchange mult status of a particular exchange mult to <i>false</i>
    \param  mult_name   name of exchange mult
    \param  mult_value  value of <i>mult_name</i> that is no longer a multiplier
*/
  void not_needed_exchange_mult(const std::string& mult_name, const std::string& mult_value);

/// prune the bandmap
  void prune(void);

// filter functions -- these affect all bandmaps, since there's just one (global) filter

/// is the filter enabled?
  inline const bool filter_enabled(void)
    { return _filter_p->enabled(); }

/*! \brief          Enable or disable the filter
    \param  torf    whether to enable the filter

    Disables the filter if <i>torf</i> is false.
*/
  void filter_enabled(const bool torf);

/// return all the countries and continents currently in the filter
  inline const std::vector<std::string> filter(void)
    { return _filter_p->filter(); }

/*!  \brief         Add a string to, or remove a string from, the filter associated with this bandmap
     \param str     string to add or subtract

     <i>str</i> may be either a continent identifier or a call or partial call. <i>str</i> is added
     if it's not already in the filter; otherwise it is removed. Currently, all bandmaps share a single
     filter.
*/
  void filter_add_or_subtract(const std::string& str);

/// is the filter in hide mode? (as opposed to show)
  inline const bool filter_hide(void) const
    { return _filter_p->hide(); }

/// set or unset the filter to hide mode (as opposed to show)
  void filter_hide(const bool torf);

/// is the filter in show mode? (as opposed to hide)
  inline const bool filter_show(void) const
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

/*!  \brief                             Find the station in the RBN threshold and filtered bandmap that is closest to a target frequency
     \param target_frequency_in_khz     target frequency, in kHz
     \param guard_band_in_hz            guard band, in Hz
     \return                            closest bandmap entry (if any) to the target frequency and within the guard band

     Applies filtering and the RBN threshold before searching for the station. Returns the
     empty string if no station was found within the guard band.
*/
    inline const std::string nearest_rbn_threshold_and_filtered_callsign(const float target_frequency_in_khz, const int guard_band_in_hz)
      { return _nearest_callsign(rbn_threshold_and_filtered_entries(), target_frequency_in_khz, guard_band_in_hz); }

/*!  \brief         Find the next needed station up or down in frequency from the current location
     \param fp      pointer to function to be used to determine whether a station is needed
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found.
     Applies filtering and the RBN threshold before searching for the next station.
*/
  const bandmap_entry needed(PREDICATE_FUN_P fp, const enum BANDMAP_DIRECTION dirn);

/*!  \brief         Find the next needed station (for a QSO) up or down in frequency from the current location
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed station for a QSO in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_qso(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_stn_needed, dirn); }

/*!  \brief         Find the next needed multiplier up or down in frequency from the current location
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed multiplier in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_mult(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_a_needed_mult, dirn); }

/*!  \brief         Find the next needed all-time new call+band+mode up or down in frequency from the current location
      param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next needed all-time call+band+mode in the direction <i>dirn</i>

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_all_time_new(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_all_time_first, dirn); }

/*!  \brief         Find the next needed stn that is also an all-time new call+band+mode, up or down in frequency from the current location
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next station in the direction <i>dirn</i> that is needed and an all-time new call+band+mode

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_all_time_new_and_needed_qso(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_all_time_first_and_needed_qso, dirn); }

/*!  \brief         Find the next stn that has QSLed and that is also an all-time new call+band+mode, up or down in frequency from the current location
     \param dirn    direction in which to search
     \return        bandmap entry (if any) corresponding to the next station in the direction <i>dirn</i> that has QSLed and is an all-time new call+band+mode

     The return value can be tested with .empty() to see if a station was found
*/
  inline const bandmap_entry needed_all_time_new_or_qsled(const enum BANDMAP_DIRECTION dirn)
    { return needed(&bandmap_entry::is_new_or_previously_qsled, dirn); }

/*! \brief          Find the next station up or down in frequency from a given frequency
    \param  f       starting frequency
    \param  dirn    direction in which to search
    \return         bandmap entry (if any) corresponding to the next needed station in the direction <i>dirn</i>

    The return value can be tested with .empty() to see if a station was found.
    Applies filtering and the RBN threshold before searching for the next station.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
  const bandmap_entry next_station(const frequency& f, const enum BANDMAP_DIRECTION dirn);

/*! \brief      Get lowest frequency on the bandmap
    \return     lowest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
  const frequency lowest_frequency(void);

/*! \brief      Get highest frequency on the bandmap
    \return     highest frequency on the bandmap

    Applies filtering and the RBN threshold before searching.
    As currently implemented, assumes that entries are in increasing order of frequency.
*/
  const frequency highest_frequency(void);

/*!  \brief             Was a call recently added?
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

/// is a particular call present?
  const bool is_present(const std::string& target_callsign);

/// convert to a printable string
  const std::string to_str(void);

/// serialize using boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_bandmap);

      ar & _column_offset
         & _do_not_add
         & _entries
         & _fade_colours
         & _filtered_entries
         & _filtered_entries_dirty    // filter_p ??
         & _rbn_threshold
         & _rbn_threshold_and_filtered_entries
         & _rbn_threshold_and_filtered_entries_dirty
         & _recent_calls
         & _recent_colour;
    }
};

/// window < bandmap
window& operator<(window& win, bandmap& bm);

/// ostream << bandmap
std::ostream& operator<<(std::ostream& ost, bandmap& bm);

typedef const bandmap_entry (bandmap::* BANDMAP_MEM_FUN_P)(const enum BANDMAP_DIRECTION);    // allow other files to access some functions in a useful, simple  manner; has to be at end, after bandmap defined

#endif    // BANDMAP_H
