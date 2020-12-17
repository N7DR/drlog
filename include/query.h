// $Id$

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

#include <set>
#include <string>
#include <vector>

// -----------  query_database  ----------------

/*! \class  query_database
    \brief  The database for the query function
*/

class query_database
{
protected:

  std::set<std::string> _qdb;                // the basic container of calls;
  std::set<std::string> _dynamic_qdb;        // the dynamic container of worked calls;

  std::set<std::string> _query(const std::string& expression) const;

public:

  query_database(void) = default;

  explicit query_database(const std::vector<std::string>& calls) :
    _qdb(calls.cbegin(), calls.cend())
    { }

  inline void operator=(const std::vector<std::string>& calls)
    { _qdb.clear();
      std::copy(calls.cbegin(), calls.cend(), std::inserter(_qdb, _qdb.end())); 
    }

  inline void operator+=(const decltype(_qdb)& calls)
    { std::copy(calls.cbegin(), calls.cend(), std::inserter(_qdb, _qdb.end())); }

  void operator+=(const std::string& call);

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         query matches for <i>key</i>
*/
  std::pair<std::set<std::string> /* q1 */, std::set<std::string> /* qn */> operator[](const std::string& key) const;

  std::set<std::string> q_1(const std::string& key) const;

  std::set<std::string> q_n(const std::string& key) const;
};

#endif    // QUERY_H
