// $Id: fuzzy.h 174 2020-11-30 20:28:40Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef FUZZY_H
#define FUZZY_H

/*! \file   fuzzy.h

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

constexpr size_t MIN_FUZZY_SIZE { 3 };               ///< any call with fewer than this number of characters is included with size MIN_FUZZY_SIZE
constexpr size_t MAX_FUZZY_SIZE { 8 };               ///< any call with more than this number of characters is included with size MAX_FUZZY_SIZE

class fuzzy_database
{
protected:

  std::array< std::set<std::string>, MAX_FUZZY_SIZE + 1 /* call size */>  _db;    ///< the database;
  
/*! \brief      Force a value to be within the legal range of sizes
    \param  sz  size that may need to be forced to change
    \return     <i>sz</i>, or a value within the legal range

    The purpose of this is to include calls with more or fewer characters than the boundaries
    into the correct element of the <i>_db</i> array
*/
  inline size_t _to_valid_size(const size_t sz) const
    { return std::max(std::min(sz, MAX_FUZZY_SIZE), MIN_FUZZY_SIZE); }

public:

/// default constructor
  inline fuzzy_database(void) = default;

/*! \brief              Construct from a file
    \param  filename    name of the file from which to construct the object

    The file <i>filename</i> is assumed to look similar to TRMASTER.ASC
*/
  explicit fuzzy_database(const std::string& filename);

/*! \brief          Construct from a <i>drmaster</i> object
    \param  drm     <i>drmaster</i> object from which to construct
*/
  explicit fuzzy_database(const drmaster& drm);

/*! \brief          Add the calls in a vector to the database
    \param  calls   calls to be added

    Does nothing for any calls already in the database
*/
  inline void init_from_calls(const std::vector<std::string>& calls)
    { FOR_ALL(calls, [&] (const std::string& this_call) { add_call(this_call); } ); }

/*! \brief          Add a call to the database
    \param  call    call to be added

    Does nothing if the call is already in the database
*/
  inline void add_call(const std::string& call)
    { _db[ _to_valid_size(call.length()) ] += call; }

/*! \brief          Remove a call from the database
    \param  call    call to be removed
    \return         whether <i>call</i> was actually removed

    Does nothing and returns <i>false</i> if <i>call</i> is not in the database
*/
  inline bool remove_call(const std::string& call)
    { return ( (_db[ _to_valid_size(call.length()) ].erase(call)) != 0 ); }

/*! \brief          Is a call in the database?
    \param  call    call to be removed
    \return         whether <i>call</i> is present in the database
*/
  inline bool contains(const std::string& call) const
    { return (_db[ _to_valid_size(call.length()) ] > call); }
  
/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         fuzzy matches for <i>key</i>

    This would use regex, except that g++ doesn't support that yet :-( :-(
*/
  std::set<std::string> operator[](const std::string& key) const;

/// empty the database
  inline void clear(void)
    { _db.fill( std::set<std::string> { } ); }
};

// -----------  fuzzy_databases  ----------------

/*! \class  fuzzy_databases
    \brief  Wrapper for multiple fuzzy databases
*/

class fuzzy_databases
{
protected:

  std::vector<fuzzy_database*> _vec;    ///< vector of pointers to databases in priority order, most important (i.e., the basic, static database) first

public:

/// default constructor
  fuzzy_databases(void) = default;

/// add a database to those that are consulted
  inline void add_db(fuzzy_database& db)
    { _vec += &db; }

/// add a database to those that are consulted
  inline void operator+=(fuzzy_database& db)
    { add_db(db); }

/// remove a call ... goes through databases in reverse priority order until a removal is successful
  void remove_call(const std::string& call);

/// remove a call ... goes through databases in reverse priority order until a removal is successful
  inline void operator-=(const std::string& call)
    { remove_call(call); }

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         all fuzzy matches in all databases for <i>key</i>
*/
  std::set<std::string> operator[](const std::string& key);
};

#endif    // FUZZY_H
