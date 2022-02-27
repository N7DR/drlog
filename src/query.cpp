// $Id: query.cpp 200 2022-01-16 14:48:14Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   query.cpp

    Objects and functions related to the query function
*/

#include "query.h"

using namespace std;

// -----------  query_database  ----------------

/*! \class  query_database
    \brief  The database for the query function
*/

/*! \brief          Possibly add a call to the dynamic database
    \param  call    call to add
    
    <i>call<i> is added to the dynamic database iff it is not already present in either database
*/
void query_database::operator+=(const std::string& call)
{ if (!( _qdb > call))
    _dynamic_qdb += call;
}

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         query matches for <i>key</i>

    The returned pair comprises: q1, qn
*/
pair<set<string> /* q1 */, set<string> /* qn */> query_database::operator[](const string& key) const
{ set<string> rv_1 { };

  if (!contains(key, '?'))
    return { rv_1, rv_1 };

  rv_1 = _query(replace_char(key, '?', '.'));

  set<string> rv_2 { _query(replace(key, "?"s, ".{1,}"s)) };

// remove any elements in rv_1 from rv_2
//  for (const auto& el1 : rv_1)
//    rv_2 -= el1;
  rv_2 -= rv_1;

  return { rv_1, rv_2 };
}
