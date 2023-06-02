// $Id: autocorrect.h 205 2022-04-24 16:05:06Z  $

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
};

// -----------  band_dynamic_autocorrect_database  ----------------

/*! \class  band_dynamic_autocorrect_database
    \brief  A single-band database for the dynamic lookup
*/

class band_dynamic_autocorrect_database
{
protected:

  BAND _b;

  uint32_t _f_min_100;
  uint32_t _f_max_100;

  std::vector<post_struct>  _data;

//  std::map<post_struct> _data_map;
  std::map<time_t, std::map<std::string, size_t>> _data_map_map;

public:

  band_dynamic_autocorrect_database(const BAND b);

  bool insert(const dx_post& post);
};

// -----------  dynamic_autocorrect_database  ----------------

/*! \class  dynamic_autocorrect_database
    \brief  A database for the dynamic lookup
*/

class dynamic_autocorrect_database
{
protected:

public:

  bool insert(const dx_post& post);
};

#endif    // AUTOCORRECT.H
