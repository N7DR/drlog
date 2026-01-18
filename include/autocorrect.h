// $Id: autocorrect.h 283 2026-01-18 16:41:22Z  $

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
    \brief  The database of good calls for the (non-dynamic) autocorrect function
*/

class autocorrect_database
{
protected:

  UNORDERED_STRING_SET _calls { };                              ///< known good calls

  mutable STRING_MAP<std::string /* output call */> _cache { }; ///< cache of input to output call mapping; key = input call; value = output call

public:

/// default constructor
  autocorrect_database(void) = default;

/*! \brief              Initialise the database from a container of known-good calls
    \param  callsigns   vector of known-good calls
*/
  inline void init_from_calls(const std::vector<std::string>& callsigns)
    { _calls += callsigns; }

/*! \brief                  Is a call a known-good call?
    \param  putative_call   target call
    \return                 whether <i>putative_call</i> is a known-good call
*/
  inline bool contains(const std::string_view putative_call) const
    { return _calls.contains(putative_call); }

/// return the number of known-good calls
  inline size_t n_calls(void) const
    { return _calls.size(); }

/// return the number of known-good calls
  inline size_t size(void) const
    { return n_calls(); }

/*! \brief          Obtain an output call from an input
    \param  str     input call
    \return         <i>str</i> or a corrected version of same
*/
//  std::string corrected_call(const std::string& str) const;
  std::string corrected_call(const std::string_view str) const;
};

// -----------  band_dynamic_autocorrect_database  ----------------

/*! \class  band_dynamic_autocorrect_database
    \brief  A single-band database for the dynamic autocorrection lookup
*/

class band_dynamic_autocorrect_database
{
protected:

  using F100_TYPE = uint32_t;   // type of frequency measured to 100 Hz

  BAND _b;                      ///< band to which this database applies

  F100_TYPE _f_min_100;         ///< minimum frequency in hundreds of Hz
  F100_TYPE _f_max_100;         ///< maximum frequency in hundreds of Hz

  std::map<time_t, std::map<F100_TYPE /* f_100 */, UNORDERED_STRING_MAP<size_t /* number of appearances */>>> _data_map_map_map; // time in minutes, f_100, callsign, number of times the

  STRING_SET _all_calls;

// this mutex introduces a lot of pain, as it is non-copyable and non-moveable; it means that instances
// have first to be default-created, then moved to the correct band, rather than creating in place.
// There's supposed to be a way to use .try_emplace(), but I can't find the correct magic incantation
// (https://en.cppreference.com/w/cpp/container/map/try_emplace)
  mutable std::recursive_mutex  _mtx;   ///< per-band mutex

public:

/// constructor
  band_dynamic_autocorrect_database(void) = default;

/// delete the copy constructor
  band_dynamic_autocorrect_database(const band_dynamic_autocorrect_database&) = delete;

/*! \brief              Prune the database by removing old minutes
    \param  n_minutes   remove all data older than <i>n_minutes</i> ago
*/
  void prune(const int n_minutes);

/*! \brief      Set the value of the band
    \param  b   the new value of the band
*/
  void to_band(const BAND b);

/*! \brief          Add a post to the database
    \param  post    post to add
*/
  void insert(const dx_post& post);

/*! \brief          Add a post to the database
    \param  post    post to add
*/
  inline void operator+=(const dx_post& post)
    { this -> insert(post); }

/*! \brief              Convert to a printable string
    \param  n_spaces    number of spaces to prepend to each line
    \return             printable string describing the database
*/
  std::string to_string(const int n_spaces = 0) const;

/*! \brief          Perform dynamic autocorrection on a call (if necessary)
    \param  post    post as received from the RBN
    \return         autocorrected call from the post
*/
  std::string autocorrect(const dx_post& post);
};

// -----------  dynamic_autocorrect_database  ----------------

/*! \class  dynamic_autocorrect_database
    \brief  A database for the dynamic lookup
*/

class dynamic_autocorrect_database
{
protected:

  std::map<BAND, band_dynamic_autocorrect_database> _per_band_db;   ///< per-band databases

  mutable std::recursive_mutex  _mtx;               ///< mutex

/*! \brief      Return the bands that are in the database
    \return     the bands containing data in the database
*/
  std::set<BAND> _known_bands(void) const;

public:

/*! \brief      Does the database contain data from a particular band?
    \param  b   band to test
    \return     whether the database contains data from band <i>b</i>
*/
  bool contains_band(const BAND b) const;

/*! \brief      Add a band to the database
    \param  b   band to add
*/
  void operator+=(const BAND b);

/*! \brief          Add a post to the database
    \param  post    post to add
*/
  void insert(const dx_post& post);

/*! \brief          Add a post to the database
    \param  post    post to add
*/
  inline void operator+=(const dx_post& post)
    { insert(post); }

/*! \brief              Prune the database by removing old minutes
    \param  n_minutes   remove all data older than <i>n_minutes</i> ago
*/
  void prune(const int n_minutes);

/*! \brief              Convert to a printable string
    \return             printable string describing the database
*/
  std::string to_string(void) const;

/*! \brief          Perform dynamic autocorrection on a call
    \param  post    post to subject to autocorrection
    \return         possibly-autocorrected call from <i>post</i>
*/
  std::string autocorrect(const dx_post& post);
};

// -----------  busts_database  ----------------

/*! \class  busts_database
    \brief  A database for caching bust and non-bust information

    This class is not thread-safe
*/

/*! \brief          Generate a single string from a pair of calls
    \param  call1   first call
    \param  call2   second call
    \return         single string to be used as index into sets

    Returns the same string for (call1, call2) and (call2, call1)
*/
inline std::string pair_index(const std::string& call1, const std::string& call2)
  { return (call1 < call2) ? (call1 + "+"s + call2) : (call2 + "+"s + call1); }

class busts_database
{
protected:

  UNORDERED_STRING_SET _known_busts;         ///< all the known bust-pairs
  UNORDERED_STRING_SET _known_non_busts;     ///< all the known non-bust pairs

public:

/// is a pair of calls a known bust pair? <i>index_string</i> is the result of executing pair_index() on the two calls
  inline bool is_known_bust(const std::string_view index_string) const
    { return _known_busts.contains(index_string); }

/// is a pair of calls a known non-bust pair? <i>index_string</i> is the result of executing pair_index() on the two calls
  inline bool is_known_non_bust(const std::string_view index_string) const
    { return _known_non_busts.contains(index_string); }

/// add a pair of calls to the set of known busts <i>index_string</i> is the result of executing pair_index() on the two calls
  inline void known_bust(const std::string_view index_string)
    { _known_busts += index_string; }

/// add a pair of calls to the set of known non-busts <i>index_string</i> is the result of executing pair_index() on the two calls
  inline void known_non_bust(const std::string_view index_string)
    { _known_non_busts += index_string; }
};

#endif    // AUTOCORRECT.H
