// $Id: fuzzy.h 137 2016-12-15 20:07:54Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef FUZZY_H
#define FUZZY_H

/*! \file fuzzy.h

    Objects and functions related to generation of fuzzy matches
*/

#include "drmaster.h"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <unordered_set>

// -----------  fuzzy_database  ----------------

/*! \class  fuzzy_database
    \brief  The database for the fuzzy function
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
  inline void init_from_calls(const std::vector<std::string>& calls)
    { FOR_ALL(calls, [&] (const std::string& this_call) { add_call(this_call); } ); }

/// add a call to the database
  inline void add_call(const std::string& call)
    { _db[ _to_valid_size(call.length()) ].insert(call); }

/// remove a call from the database; returns whether the removal was successful
  inline const bool remove_call(const std::string& call)
    { return ( (_db[ _to_valid_size(call.length()) ].erase(call)) != 0 ); }

/// is a call in the database?
  inline const bool contains(const std::string& call)
    { return (_db[ _to_valid_size(call.length()) ] < call); }
  
/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         fuzzy matches for <i>key</i>

    This would use regex, except that g++ doesn't support that yet :-( :-(
*/
  const std::set<std::string> operator[](const std::string& key) const;

/// empty the database
  inline void clear(void)
    { _db.fill(std::set<std::string>()); }
};

// -----------  fuzzy_databases  ----------------

/*! \class  fuzzy_databases
    \brief  Wrapper for multiple fuzzy databases
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

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         all fuzzy matches in all databases for <i>key</i>
*/
  const std::set<std::string> operator[](const std::string& key);
};

#endif    // FUZZY_H
