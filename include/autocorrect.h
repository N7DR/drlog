// $Id: autocorrect.h 221 2023-06-19 01:57:55Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef AUTOCORRECT_H
#define AUTOCORRECT_H

/*! \file   autocorrect.h

    Objects and functions related to automatically correcting calls in RBN posts
*/

#include "cluster.h"
#include "macros.h"

#include <string>
#include <unordered_set>
#include <vector>

using MINUTES_TYPE = int64_t;                               // type for holding absolute minutes

// -----------  autocorrect_database  ----------------

/*! \class  autocorrect_database
    \brief  The database of good calls for the autocorrect function
*/

class autocorrect_database
{
protected:

  std::unordered_set<std::string> _calls;                                               ///< known good calls

  mutable std::map<std::string /* input call */, std::string /* output call */> _cache; ///< cache of input to output call mapping

public:

// default constructor
  autocorrect_database(void) = default;

/*! \brief              Initialise the database from a container of known-good calls
    \param  callsigns   vector of known-good calls
*/
  inline void init_from_calls(const std::vector<std::string>& callsigns)
    { FOR_ALL(callsigns, [this] (const std::string& str) { _calls += str; } ); }

/*! \brief                  Is a call a known-good call?
    \param  putative_call   target call
    \return                 whether <i>putative_call</i> is a known-good call
*/
  inline bool contains(const std::string& putative_call) const
    { return _calls.contains(putative_call); }

  inline unsigned int n_calls(void) const
    { return _calls.size(); }

  inline unsigned int size(void) const
    { return n_calls(); }

/*! \brief          Obtain an output call from an input
    \param  str     input call
    \return         <i>str</i> or a corrected version of same
*/
  std::string corrected_call(const std::string& str) const;
};

// -----------  post_struct  ----------------

/*! \class  post_struct
    \brief  Information from a <i>dx_post</i>
*/

class post_struct
{
protected:

  std::string _call;
  size_t      _n_posts;
  time_t      _post_time;

public:

  post_struct(void) = default;

  inline explicit post_struct(const dx_post& dx) :
    _call(dx.callsign()),
    _n_posts(1),
    _post_time(dx.time_processed())
  { }

};

// -----------  band_dynamic_autocorrect_database  ----------------

/*! \class  band_dynamic_autocorrect_database
    \brief  A single-band database for the dynamic lookup

    
*/

class band_dynamic_autocorrect_database
{
protected:

  BAND _b;                  ///< band to which this database applies

  uint32_t _f_min_100;      ///< minimum frequency in hundreds of Hz
  uint32_t _f_max_100;      ///< maximum frequency in hundreds of Hz

  std::vector<post_struct>  _data;  // arranged according to frequency

  std::map<time_t, std::map<std::string /* call */, std::map<uint32_t /* f_100 */, size_t /* number of appearances */>>> _data_map_map_map; // time in minutes, callsign, number of times the call appears

  std::set<std::string> _all_calls;

// this introduces a lot of pain, as it is non-copyable and non-moveable; it means that instances
// have first to be default-created, then moved to the correct band, rather than creating in place.
// There's supposed to be a way to use .try_emplace(), but I can't find the correct magic incantation
// (https://en.cppreference.com/w/cpp/container/map/try_emplace)
  mutable std::recursive_mutex  _mtx;   ///< per-band mutex

  std::set<std::string> _get_all_calls(void) const;

public:

  band_dynamic_autocorrect_database(void) = default;

  band_dynamic_autocorrect_database(const band_dynamic_autocorrect_database&) = delete;

  void prune(const int n_minutes);

  void to_band(const BAND b);

  bool insert(const dx_post& post);

  std::string to_string(const int n_spaces = 0) const;

  std::string autocorrect(const dx_post& post);
};

// -----------  dynamic_autocorrect_database  ----------------

/*! \class  dynamic_autocorrect_database
    \brief  A database for the dynamic lookup
*/

class dynamic_autocorrect_database
{
protected:

  std::map<BAND, band_dynamic_autocorrect_database> _per_band_db;

  mutable std::recursive_mutex  _mtx;   ///< mutex

  std::set<BAND> _known_bands(void) const;

public:

  bool contains_band(const BAND b) const;

  void operator+=(const BAND b);

  bool insert(const dx_post& post);

  inline void operator+=(const dx_post& post)
    { insert(post); }

  void prune(const int n_minutes);
//  inline void prune(const int n_minutes)
//    { FOR_ALL(_known_bands(), [this, n_minutes] (const BAND b) { _per_band_db[b].prune(n_minutes); }); }

  std::string to_string(void) const;
};

#if 0
// -----------  band_n_posters_database  ----------------

/*! \class  band_n_posters_database
    \brief  A single-band database for the number of posters  of stations
*/

class band_n_posters_database
{
protected:

  BAND _b;                  ///< band to which this database applies

  std::map<time_t, std::unordered_map<std::string /* call */, std::unordered_set<std::string>>> _data; // time in minutes, callsign, number of posters

  mutable std::recursive_mutex  _mtx;   ///< mutex

public:

  band_n_posters_database(void) = default;

  inline explicit band_n_posters_database(const BAND b) :
    _b(b)
  { }

  inline void to_band(const BAND b)
    { _b = b; }

  void operator+=(const dx_post& post);

  std::set<time_t> times(void) const
  { std::lock_guard<std::recursive_mutex> lg(_mtx); 

    return all_keys<std::set<time_t>>(_data);
  }

  int n_posters(const std::string& call) const; // in all minutes

  void prune(const int n_minutes);
};

// -----------  n_posters_database  ----------------

/*! \class  n_posters_database
    \brief  database for the number of posters  of stations
*/

class n_posters_database
{
protected:

  std::map<BAND, band_n_posters_database> _per_band_db;

  mutable std::recursive_mutex  _mtx;   ///< mutex

  std::set<BAND> _known_bands(void) const;

public:

  bool contains_band(const BAND b) const;

  void operator+=(const BAND b);

  void operator+=(const dx_post& post);

  void prune(const int n_minutes);
};
#endif    // 0

#endif    // AUTOCORRECT.H
