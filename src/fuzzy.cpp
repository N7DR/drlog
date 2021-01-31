// $Id: fuzzy.cpp 178 2020-12-27 16:26:16Z  $

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

/*! \brief              Construct from a file
    \param  filename    name of the file from which to construct the object

    The file <i>filename</i> is assumed to look similar to TRMASTER.ASC
*/
fuzzy_database::fuzzy_database(const string& filename)
{ const vector<string> calls { to_lines(to_upper(remove_char(remove_char(read_file(filename), CR_CHAR), ' '))) };

  FOR_ALL(calls, [&] (const string& x) { *this += x; } );
}

/*! \brief          Construct from a <i>drmaster</i> object
    \param  drm     <i>drmaster</i> object from which to construct
*/
fuzzy_database::fuzzy_database(const drmaster& drm)
{ const vector<string> calls { drm.calls() };

  FOR_ALL(calls, [&] (const string& x) { *this += x; } );
}

/*! \brief          Return matches
    \param  key     basic call against which to compare
    \return         fuzzy matches for <i>key</i>
*/
set<string> fuzzy_database::operator[](const string& key) const
{ set<string> rv { };

  if (key.length() < 3)
    return rv;

// if the key contains any characters that are interpreted by regex, we should return an empty set
  if (key.find_first_not_of(CALLSIGN_CHARS) != string::npos)
    return rv;

//  vector<string> regex_str_vec;

//  for (unsigned int posn { 0 }; posn < n_chars; ++posn)
//  { const string str { key.substr(0, posn) + '.' + key.substr(posn + 1) };
//    
//    regex_str_vec += str;
//  }

//  set<string> rv { };

  const set<string>& ss { _db[ _to_valid_size(key.length()) ] };

  for (size_t posn { 0 }; posn < key.length(); ++posn)
//  { const string str { key.substr(0, posn) + '.' + key.substr(posn + 1) };

    rv += regex_matches<set<string>>(ss, (key.substr(0, posn) + '.' + key.substr(posn + 1)) );
//  }

  return rv;

#if 0
  set<string> rv;

  const set<string>& ss { _db[ _to_valid_size(key.length()) ] };
  
  for (const auto& str : ss)
  { for (unsigned int posn = 0; posn < key.length(); ++posn)
    { bool match { ( (posn == 0) or (substring(key, 0, posn) == substring(str, 0, posn)) ) };  // 2nd term is always true if posn = 0
      
      if (match)
      { if (posn != key.length() -1 )
          match = (substring(key, posn + 1) == substring(str, posn + 1));

        if (match)
          if (key != str)           // don't include if the match is exact
            rv.insert(str);
      }
    }
  }
  
  return rv;
#endif
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

    Returns empty set if 
*/
set<string> fuzzy_databases::operator[](const string& key)
{ set<string> rv { };

  if (key.length() < 3)
    return rv;

//  set<string> rv;

  for (const auto& db_p : _vec)
  { const fuzzy_database& db { *db_p };

    rv += db[key];
//    const set<string>& calls { db[key] };

//    copy(calls.cbegin(), calls.cend(), inserter(rv, rv.begin()));
  }

  return rv;
}
