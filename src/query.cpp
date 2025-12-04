// $Id: query.cpp 279 2025-12-01 15:09:34Z  $

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
//void query_database::operator+=(const std::string& call)
void query_database::operator+=(const std::string_view call)
{ if (!_qdb.contains(call))
    _dynamic_qdb += call;
}

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         query matches for <i>key</i>

    The returned pair comprises: q1, qn
*/
pair<STRING_SET /* q1 */, STRING_SET /* qn */> query_database::operator[](const string_view key) const
{ STRING_SET rv_1 { };

  if (!key.contains('?'))
    return { rv_1, rv_1 };

  rv_1 = _query(replace_char(key, '?', '.'));

  STRING_SET rv_2 { _query(replace(key, "?"s, ".{1,}"s)) };

// remove any elements in rv_1 from rv_2
  rv_2 -= rv_1;

  return { rv_1, rv_2 };
}
