// $Id: autocorrect.h 202 2022-03-07 21:01:02Z  $

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

  std::unordered_set<std::string> _calls;

// we should add a cache of input to output calls
  mutable std::map<std::string /* input call */, std::string /* output call */> _cache;

public:

  autocorrect_database(void) = default;

  inline void init_from_calls(const std::vector<std::string>& callsigns)
    { FOR_ALL(callsigns, [this] (const std::string& str) { _calls += str; } ); }

  inline bool contains(const std::string& putative_call) const
    { return ::contains(_calls, putative_call); }

  inline unsigned int n_calls(void) const
    { return _calls.size(); }

  inline unsigned int size(void) const
    { return n_calls(); }

  std::string corrected_call(const std::string& str) const;
};

inline bool contains(const autocorrect_database& db, const std::string& str)
  { return db.contains(str); }

#endif    // AUTOCORRECT.H
