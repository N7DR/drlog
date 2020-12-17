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

void query_database::operator+=(const std::string& call)
{ if (!( _qdb > call))
    _dynamic_qdb += call;
}

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         query matches for <i>key</i>
*/
pair<set<string> /* q1 */, set<string> /* qn */> query_database::operator[](const string& key) const
{ set<string> rv_1 { };

  if (!contains(key, '?'))
    return { rv_1, rv_1 };

  rv_1 = _query(replace_char(key, '?', '.'));

  set<string> rv_2 { _query(replace(key, "?"s, ".{1,}"s)) };

// remove any elements in rv_1 from rv_2
  for (const auto& el1 : rv_1)
    rv_2.erase(el1);

  return { rv_1, rv_2 };

//return ( q_1(key) + q_n(key) );

#if 0
  set<string> rv { };

  if (!contains(key, '?'))
    return rv;

// start with ? == a single character
  const string target { replace_char(key, '?', '.') };
  const regex  rgx_1  { target };

  FOR_ALL(_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_1))
                                                     rv += callsign;
                                                 } );

  FOR_ALL(_dynamic_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_1))
                                                             rv += callsign;
                                                         } );

  const regex rgx_2 { replace(key, "?"s, ".{2,}"s) };     // two or more characters

  FOR_ALL(_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_2))
                                                     rv += callsign;
                                                 } );

  FOR_ALL(_dynamic_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_2))
                                                             rv += callsign;
                                                         } );
 
  return rv;
#endif
}

set<string> query_database::q_1(const string& key) const
{ set<string> rv { };

  if (!contains(key, '?'))
    return rv;

// start with ? == a single character
  const string target { replace_char(key, '?', '.') };
  const regex  rgx_1  { target };

  FOR_ALL(_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_1))
                                                     rv += callsign;
                                                 } );

  FOR_ALL(_dynamic_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_1))
                                                             rv += callsign;
                                                         } );
 
  return rv;
}

set<string> query_database::q_n(const string& key) const
{ set<string> rv { };

  if (!contains(key, '?'))
    return rv;

  const regex rgx_2 { replace(key, "?"s, ".{2,}"s) };     // two or more characters

  FOR_ALL(_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_2))
                                                     rv += callsign;
                                                 } );

  FOR_ALL(_dynamic_qdb, [=, &rv](const string& callsign) { if (regex_match(callsign, rgx_2))
                                                             rv += callsign;
                                                         } );
 
  return rv;
}
