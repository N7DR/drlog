// $Id: scp.h 140 2017-11-05 15:16:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef SCP_H
#define SCP_H

/*! \file   scp.h

    Objects and functions related to Super Check Partial
*/

#include "drmaster.h"

#include <map>
#include <set>
#include <string>
#include <unordered_set>

// forward declaration
class scp_databases;

typedef std::unordered_set<std::string> SCP_SET;    ///< define the type of set used in SCP functions

// -----------  scp_database  ----------------

/*! \class  scp_database
    \brief  The database for SCP

    We build our own database instead of trying to use the old K1EA
    memory layout
*/

class scp_database
{
protected:

  std::map<std::string  /* two characters */, SCP_SET /* calls that contain the two characters */ >  _db;    ///< the main database;

// a one-shot cache; I'm far from convinced that this is useful,
// because an ordinary cache-miss lookup is so fast
  std::string              _last_key;       ///< the key of the last lookup
  SCP_SET                  _last_result;    ///< the result from the last lookup

  std::vector<scp_databases*> _parents;     ///< a "parent" is an scp_databases object

public:

/// default constructor
  inline scp_database(void)
    { }

/// construct from filename; file is assumed to look similar to TRMASTER.ASC
  explicit scp_database(const std::string& filename);
  
/// construct from vector of calls
  explicit scp_database(const std::vector<std::string>& calls);

/// construct from a master_dta
  explicit scp_database(const master_dta& md);

/// construct from a drmaster object
  explicit scp_database(const drmaster& drm);

/// destructor
  inline virtual ~scp_database(void)
    { }

/// populate the database from a vector of calls
  void init_from_calls(const std::vector<std::string>& calls);

/// add a call to the database
  void add_call(const std::string& call);
  
/// remove a call from the database; returns 0 or 1 depending on whether a call is actually removed (1 => a call was removed)
  const unsigned int remove_call(const std::string& call);

/// is a call in the database?
  inline const bool contains(const std::string& call)
    { return (call.empty() ? false : (_db[substring(call, 0, 2)] < call) ); }

/// return SCP matches
  const SCP_SET operator[](const std::string& key);

/// empty the database; also clears the cache
  void clear(void);

/// clear the cache (without altering the database)
  void clear_cache(void);

/// add a parent scp_databases object
  inline void add_parent(scp_databases& dbs)
    { _parents.push_back(&dbs); }
};

// -----------  scp_databases  ----------------

/*! \class  scp_databases
    \brief  Wrapper for multiple SCP databases

    The idea is to have two databases, the static one that is read from an external file at start time,
    and a dynamic one that is altered as we encounter new calls that are not in the static database
*/

class scp_databases
{
protected:

  std::vector<scp_database*> _vec;    ///< in priority order, most important (i.e., the basic, static database) first.

  std::string                _last_key;       ///< the key of the last lookup
  SCP_SET                    _last_result;    ///< the result from the last lookup

public:

/// default constructor
  inline scp_databases(void)
    { }

/// destructor
  inline virtual ~scp_databases(void)
    { }

/// add a database to those that are consulted
  void add_db(scp_database& db);

/// add a database to those that are consulted
  inline void operator+=(scp_database& db)
    { add_db(db); }

/// remove a call ... goes through databases in *reverse* priority order until a removal is successful
  void remove_call(const std::string& call);

/// return matches
  const SCP_SET operator[](const std::string& key);

/// clear the cache; also clear the caches of any children
  void clear_cache(void);

/// clear the cache without clearing the caches of any children
  void clear_cache_no_children(void);
};

#endif    // SCP_H
