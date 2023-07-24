// $Id: scp.h 215 2023-01-23 19:37:41Z  $

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

using SCP_SET = std::unordered_set<std::string>;    ///< define the type of set used in SCP functions

// -----------  scp_database  ----------------

/*! \class  scp_database
    \brief  The database for SCP

    We build our own database instead of trying to use the old K1EA
    memory layout
*/

class scp_database
{
protected:

  std::unordered_map<std::string /* two characters */, SCP_SET /* calls that contain the two characters */ > _db;   ///< the main database;

// a one-shot cache; I'm far from convinced that this is useful,
// because an ordinary cache-miss lookup is so fast
  std::string              _last_key;       ///< the key of the last lookup
  SCP_SET                  _last_result;    ///< the result from the last lookup

  std::vector<scp_databases*> _parents;     ///< a "parent" is an scp_databases object

public:

/// default constructor
  inline scp_database(void) = default;

/// construct from filename; file is assumed to look similar to TRMASTER.ASC
  inline explicit scp_database(const std::string& filename)
    { init_from_calls(to_lines <std::string> (to_upper(remove_char(remove_char(read_file(filename), CR_CHAR), ' ')))); }
  
/// construct from vector of calls
  inline explicit scp_database(const std::vector<std::string>& calls)
    { init_from_calls(calls); }

/// construct from a master_dta
  inline explicit scp_database(const master_dta& md)
    { init_from_calls(md.calls()); }

/// construct from a drmaster object
  inline explicit scp_database(const drmaster& drm)
    { init_from_calls(drm.calls()); }

/// populate the database from a vector of calls
  inline void init_from_calls(const std::vector<std::string>& calls)
    { FOR_ALL(calls, [this] (const std::string& this_call) { *this += this_call; } ); }

/// add a call to the database
  void operator+=(const std::string& call);
  
/*! \brief          Remove a call from the database
    \param  call    call to remove
    \return         whether <i>call</i> was actually removed
*/
  bool remove_call(const std::string& call);

/*! \brief          Remove a call from the database
    \param  call    call to remove
*/
  void operator-=(const std::string& call);

/// is a call in the database?
  inline bool contains(const std::string& call)
    { return (call.empty() ? false : (_db[substring <std::string> (call, 0, 2)].contains(call))); }

/*! \brief          Return all the matches for a partial call
    \param  key     partial call
    \return         whether <i>call</i> was actually removed
*/
  SCP_SET operator[](const std::string& key);

/// empty the database; also clears the cache
  void clear(void);

/// clear the cache (without altering the database)
  void clear_cache(void);

/// add a parent scp_databases object
  inline void add_parent(scp_databases& dbs)
    { _parents += &dbs; }
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
  inline scp_databases(void) = default;

/// add a database to those that are consulted
  void add_db(scp_database& db);

/// add a database to those that are consulted
  inline void operator+=(scp_database& db)
    { add_db(db); }

/// remove a call ... goes through databases in *reverse* priority order until a removal is successful
  inline void remove_call(const std::string& call)
    { FIND_IF(_vec, [&call] (auto& db) { return (db->remove_call(call)); } ); }

/// remove a call ... goes through databases in *reverse* priority order until a removal is successful
  inline void operator-=(const std::string& call)
    { remove_call(call); }

/// return matches
  SCP_SET operator[](const std::string& key);

/// clear the cache; also clear the caches of any children
  void clear_cache(void);

/// clear the cache without clearing the caches of any children
  void clear_cache_no_children(void);
};

#endif    // SCP_H
