// $Id: fuzzy.cpp 215 2023-01-23 19:37:41Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   fuzzy.cpp

    Objects and functions related to the fuzzy function
*/

#include "fuzzy.h"
#include "log_message.h"
#include "string_functions.h"

#include <iostream>
#include <regex>
#include <vector>

using namespace std;

extern message_stream    ost;       ///< debugging/logging output

// -----------  fuzzy_database  ----------------

/*! \class  fuzzy_database
    \brief  The database for the fuzzy function
*/

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         fuzzy matches for <i>key</i>
*/
FUZZY_SET fuzzy_database::operator[](const string& key) const
{ FUZZY_SET rv { };

  if (key.length() < 3)
    return rv;

// if the key contains any characters that are interpreted by regex, we should return an empty set
  if (key.find_first_not_of(CALLSIGN_CHARS) != string::npos)
    return rv;

//  const set<string>& ss { _db[ _to_valid_size(key.length()) ] };
  const FUZZY_SET& ss { _db[ _to_valid_size(key.length()) ] };

  for (size_t posn { 0 }; posn < key.length(); ++posn)
 //   rv += regex_matches<set<string>>(ss, (key.substr(0, posn) + '.' + key.substr(posn + 1)) );
//    rv += regex_matches<unordered_set<string>>(ss, (key.substr(0, posn) + '.' + key.substr(posn + 1)) );
    rv += regex_matches<base_type<decltype(ss)>>(ss, (key.substr(0, posn) + '.' + key.substr(posn + 1)) );

// 230116 do not include the key in the output set
  rv -= key;

  return rv;
}

// -----------  fuzzy_databases  ----------------

/*! \class  fuzzy_databases
    \brief  Wrapper for multiple fuzzy databases
*/

/// remove a call ... goes through databases in reverse priority order until a removal is successful
void fuzzy_databases::remove_call(const string& call)
{ bool removed { false };

  for (size_t n { 0 }; (!removed and (n < _vec.size())); ++n)
    removed = ( _vec[ (_vec.size() - 1 - n) ] -> remove_call(call) );
}

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         all fuzzy matches in all databases for <i>key</i>

    Returns empty set if the call is too short 
*/
FUZZY_SET fuzzy_databases::operator[](const string& key) const
{ FUZZY_SET rv { };

  if (key.length() < 3)
    return rv;

  for (const auto& db_p : _vec)
  { const fuzzy_database& db { *db_p };

    rv += db[key];
  }

  return rv;
}
