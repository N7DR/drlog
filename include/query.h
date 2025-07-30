// $Id: query.h 273 2025-07-27 13:22:36Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef QUERY_H
#define QUERY_H

/*! \file   query.h

    Objects and functions related to generation of query matches
*/

#include "macros.h"
#include "string_functions.h"

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

// -----------  query_database  ----------------

/*! \class  query_database
    \brief  The database for the query function
*/

class query_database
{ using QUERY_DB_TYPE = UNORDERED_STRING_SET;

protected:

  QUERY_DB_TYPE _qdb;                // the basic container of calls;
  QUERY_DB_TYPE _dynamic_qdb;        // the dynamic container of worked calls;

/*! \brief                  Return all calls that match a regex (string) expression
    \param      expression  expression against which to compare
    \return                 all calls that matches <i>expression</i>
*/
  inline STRING_SET _query(const std::string& expression) const
    { return STRING_SET { regex_matches <STRING_SET> (_qdb, expression) + regex_matches <STRING_SET> (_dynamic_qdb, expression) }; }

public:

/// default constructor
  query_database(void) = default;

/// construct from a vector of calls
  explicit query_database(const std::vector<std::string>& calls) :
    _qdb(calls.cbegin(), calls.cend())
    { }

/// query_database = vector of calls
  inline void operator=(const std::vector<std::string>& calls)
    { _qdb.clear();
      std::ranges::copy(calls, std::inserter(_qdb, _qdb.end()));
    }

/// add a container of calls
  inline void operator+=(const decltype(_qdb)& calls)
    { std::ranges::copy(calls, std::inserter(_qdb, _qdb.end())); }

/*! \brief          Possibly add a call to the dynamic database
    \param  call    call to add
    
    <i>call<i> is added to the dynamic database iff it is not already present in either database
*/
//  void operator+=(const std::string& call);
  void operator+=(const std::string_view call);

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         query matches for <i>key</i>

    The returned pair comprises: q1, qn
*/
  std::pair<STRING_SET /* q1 */, STRING_SET /* qn */> operator[](const std::string_view key) const;

/// clear the dynamic database
  inline void clear_dynamic_database(void)
    { _dynamic_qdb.clear(); }
};

#endif    // QUERY_H
