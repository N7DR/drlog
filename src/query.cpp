// $Id: query.cpp 176 2020-12-13 18:28:41Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   query.cpp

    Objects and functions related to the query function
*/

#include "query.h"
#include "string_functions.h"

#include <regex>

using namespace std;

// -----------  query_database  ----------------

/*! \class  query_database
    \brief  The database for the query function
*/

/*! \brief      Return all calls that match a regex (string) expression
    \param      expression  expression against which to compare
    \return     all calls that matches <i>expression</i>
*/
set<string> query_database::_query(const string& expression) const
{ set<string> rv { };
  
  const regex rgx { expression };

  FOR_ALL(_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx))
                                                     rv += callsign;
                                                 } );

  FOR_ALL(_dynamic_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx))
                                                             rv += callsign;
                                                         } );
 
  return rv;
}

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
  for (const auto& el1 : rv_1)
//    rv_2.erase(el1);
    rv_2 -= el1;

  return { rv_1, rv_2 };
}
