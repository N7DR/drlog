// $Id: fuzzy.cpp 40 2013-11-02 15:04:54Z  $

/*!     \file fuzzy.cpp

        Objects and functions related to the fuzzzy function
*/

#include "fuzzy.h"
#include "log_message.h"
#include "string_functions.h"

#include <iostream>
#include <regex>
#include <vector>

using namespace std;

extern message_stream    ost;

/// construct from filename
fuzzy_database::fuzzy_database(const string& filename)
{ const string file_contents = to_upper(remove_char(remove_char(read_file(filename), CR_CHAR), ' '));
  const vector<string> calls = to_lines(file_contents); 

  for_each(calls.begin(), calls.end(), [&] (const string& x) { this->add_call( x ); } );
}

/// construct from a drmaster object
fuzzy_database::fuzzy_database(const drmaster& drm)
{ const vector<string>& calls = drm.calls();

  for_each(calls.begin(), calls.end(), [&] (const string& x) { this->add_call( x ); } );
}

/// populate the database from a vector of calls
void fuzzy_database::init_from_calls(const vector<string>& calls)
{ for_each(calls.begin(), calls.end(), [&] (const string& this_call) { add_call(this_call); } );
}

/// add a call to the database
void fuzzy_database::add_call(const string& call)
{ _db[ _to_valid_size(call.length()) ].insert(call);
}

/// remove a call from the database; returns 0 or 1 depending on whether a call is actually removed (1 => a call was removed)
const unsigned int fuzzy_database::remove_call(const string& call)
{ return (_db[ _to_valid_size(call.length()) ].erase(call));
}

/// is a call in the database?
const bool fuzzy_database::contains(const std::string& call)
{ return (_db[ _to_valid_size(call.length()) ] < call);
}

///<return matches -- this would use regex, except that g++ doesn't support that yet :-( :-(
const set<string> fuzzy_database::operator[](const string& key) const
{ if (key.length() < 3)
    return set<string>();

//  unsigned int n_chars = key.length();
  
/*  vector<regex> expressions;
  for (unsigned int posn = 0; posn < n_chars; ++posn)
  { const string str = key.substr(0, posn) + '.' + key.substr(posn + 1);
    regex r(str);
    
    expressions.push_back(r);
  }
*/

  set<string> rv;
  const set<string>& ss = _db[ _to_valid_size(key.length()) ];
  
  for (const auto& str : ss)
  { for (unsigned int posn = 0; posn < key.length(); ++posn)
    { bool match = ( (posn == 0) or (substring(key, 0, posn) == substring(str, 0, posn)) );  // 2nd term is always true if posn = 0
      
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
}

// -----------  fuzzy_databases  ----------------

/// remove a call ... goes through databases in reverse priority order until a removal is successful
void fuzzy_databases::remove_call(const string& call)
{ bool removed = false;

  for (size_t n = 0; (n < _vec.size() and !removed); ++n)
  { const size_t rev = _vec.size() - 1 - n;

    removed = _vec[rev]->remove_call(call);
  }
}

/// return matches
const set<string> fuzzy_databases::operator[](const string& key)
{ if (key.length() < 3)
    return set<string>();

  set<string> rv;

  for (auto& db_p : _vec)
  { fuzzy_database& db = *db_p;

    const set<string>& calls = db[key];

    for (const auto& call : calls)
      rv.insert(call);
  }

  return rv;
}
