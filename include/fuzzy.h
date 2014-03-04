// $Id: fuzzy.h 40 2013-11-02 15:04:54Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef FUZZY_H
#define FUZZY_H

/*!     \file fuzzy.h

        Objects and functions related to generation of fuzzy matches
*/

#include "drmaster.h"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <unordered_set>

// typedef std::unordered_set<std::string> FUZZY_SET; not really necessary, since the big user of time is to iterate through the set

// -----------  fuzzy_database  ----------------

/*!     \class fuzzy_database
        \brief The database for the fuzzy function
*/

const size_t MIN_FUZZY_SIZE = 3;               // any call with fewer than this number of characters is included with size MIN_FUZZY_SIZE
const size_t MAX_FUZZY_SIZE = 8;               // any call with more than this number of characters is included with size MAX_FUZZY_SIZE

class fuzzy_database
{
protected:
  std::array< std::set<std::string>, MAX_FUZZY_SIZE + 1 /* call size */>  _db;    ///< the database;
  
  inline const size_t _to_valid_size(const size_t sz) const
    { return std::max(std::min(sz, MAX_FUZZY_SIZE), MIN_FUZZY_SIZE); }

public:

/// default constructor
  fuzzy_database(void)
    { }

/// construct from filename; file is assumed to look similar to TRMASTER.ASC
  explicit fuzzy_database(const std::string& filename);

/// construct from a drmaster object
  explicit fuzzy_database(const drmaster& drm);
  
/// destructor
  virtual ~fuzzy_database(void)
    { }

/// populate the database from a vector of calls
  void init_from_calls(const std::vector<std::string>& calls);

/// add a call to the database
  void add_call(const std::string& call);

/// remove a call from the database; returns 0 or 1 depending on whether a call is actually removed (1 => a call was removed)
  const unsigned int remove_call(const std::string& call);

/// is a call in the database?
  const bool contains(const std::string& call);
  
/// return fuzzy matches
  const std::set<std::string> operator[](const std::string& key) const;

/// empty the database
  inline void clear(void)
    { _db.fill(std::set<std::string>()); }
};

// -----------  fuzzy_databases  ----------------

/*!     \class fuzzy_databases
        \brief Wrapper for multiple fuzzy databases
*/

class fuzzy_databases
{
protected:

  std::vector<fuzzy_database*> _vec;    // in priority order, most important (i.e., the basic, static database) first

public:

/// default constructor
  fuzzy_databases(void)
    { }

/// destructor
  virtual ~fuzzy_databases(void)
    { }

/// add a database to those that are consulted
  inline void add_db(fuzzy_database& db)
    { _vec.push_back(&db); }

/// add a database to those that are consulted
  inline void operator+=(fuzzy_database& db)
    { add_db(db); }

/// remove a call ... goes through databases in reverse priority order until a removal is successful
  void remove_call(const std::string& call);

/// return matches
  const std::set<std::string> operator[](const std::string& key);
};

#endif    // FUZZY_H
