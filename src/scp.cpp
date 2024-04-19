// $Id: scp.cpp 223 2023-07-30 13:37:25Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file scp.cpp

        Objects and functions related to Super Check Partial
*/

#include "log_message.h"
#include "scp.h"
#include "string_functions.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

using namespace std;
using namespace      placeholders;

extern message_stream ost;

/// add a call
void scp_database::operator+=(const string& call)
{ if (call.length() >= 2)
    for ( auto start_index : RANGE<unsigned int>(0, call.length() - 1) )
//    for (unsigned int start_index = 0u; start_index < (call.length() - 1); ++start_index)
      (_db[substring <std::string> (call, start_index, 2)]) += call;
}

/*! \brief          Remove a call from the database
    \param  call    call to remove
    \return         whether <i>call</i> was actually removed
*/
bool scp_database::remove_call(const std::string& call)
{ bool rv { false };

  if (call.length() >= 2)
  { //for (const auto start_index : RANGE<size_t>(0, call.length() - 2))
//    for (const auto start_index : ranges::iota_view { 0u, call.length() - 2 } )
//    for ( auto start_index : RANGE<unsigned int>(0, call.length() - 2) )
    for ( auto start_index : RANGE<unsigned int>(0, call.length() - 1) )
    { SCP_SET& ss { _db[substring <std::string> (call, start_index, 2)] };

      rv = (ss.erase(call) == 1);       // key remains, regardless of whether the set of matches is empty
    }
  }

  return rv;
}

/*! \brief          Remove a call from the database
    \param  call    call to remove
*/
void scp_database::operator-=(const std::string& call)
{ if (call.length() >= 2)
//    for ( auto start_index : RANGE<unsigned int>(0, call.length() - 2) )
    for ( auto start_index : RANGE<unsigned int>(0, call.length() - 1) )
      _db[substring <std::string> (call, start_index, 2)].erase(call);       // key remains, regardless of whether the set of matches is empty
}

/*! \brief          Return all the matches for a partial call
    \param  key     partial call
    \return         whether <i>call</i> was actually removed
*/
SCP_SET scp_database::operator[](const string& key)
{ //ost << "Inside scp_database::operator[] for key = " << key << endl;

  if (key.length() < 2)
    return SCP_SET { };
  
  if (key.length() == 2)         // trivial lookup
  { _last_key = key;
    _last_result = _db[key];    // might change _db
    
    return _last_result;
  }
  

// key length is > 2, look to the cache first
  if (::contains(key, _last_key) and !_last_key.empty())    // cache hit
  { SCP_SET rv;
  
    for (const auto& cache_callsign : _last_result)
      if (::contains(cache_callsign, key))
        rv += cache_callsign;
      
    _last_key = key;
    _last_result = move(rv);

    return _last_result;
  }
  
// cache miss
  const SCP_SET& calls { _db[substring <std::string> (key, 0, 2)] };

  SCP_SET rv;

  for (const auto& callsign : calls)
    if (::contains(callsign, key))
      rv += callsign;
    
  _last_key = key;
  _last_result = move(rv);

  return _last_result;      
}

/// empty the database; also clears the cache
void scp_database::clear(void)
{ _db.clear();
  clear_cache();
}

/// clear the cache (without altering the database)
void scp_database::clear_cache(void)
{ _last_key.clear();
  _last_result.clear();

// must also clear the cache of any parents
  FOR_ALL(_parents, [] (scp_databases* parent_p) { parent_p->clear_cache_no_children(); } );
//  ranges::for_each(_parents, [] (scp_databases* parent_p) { parent_p->clear_cache_no_children(); } );
}

// -----------  scp_databases  ----------------

/// add a database to those that are consulted
void scp_databases::add_db(scp_database& db)
{ _vec += (&db);
  db.add_parent(*this);
}

/// return matches
SCP_SET scp_databases::operator[](const string& key)
{ //ost << "Inside scp_databases::operator[] for key = " << key << endl;

  if (key.length() < 2)
    return SCP_SET();

  if (key.length() == 2)         // trivial lookup
  { _last_key = key;
    _last_result.clear();

    for (auto& db_p : _vec)
    { scp_database& db { *(db_p) };

      _last_result += db[key];
    }

    return _last_result;
  }

//  ost << "key length = " << key.length() << endl;
//  ost << "_last key = " << _last_key << endl;

// key length is > 2; look to the cache first
  if (::contains(key, _last_key) and !_last_key.empty())    // cache hit
  { //ost << "CACHE HIT" << endl;
    //ost << "size of last result set = " << _last_result.size() << endl;

    //set<string> tmp_set;

    //FOR_ALL(_last_result, [&tmp_set] (const string& str) { tmp_set += str; } );
    //FOR_ALL(tmp_set, [] (const string& str) { ost << "  " << str << endl; } );

    SCP_SET rv;

    for (const auto& cache_callsign : _last_result)
      if (::contains(cache_callsign, key))
        rv += cache_callsign;

    _last_key = key;
    _last_result = move(rv);

    return _last_result;
  }

// key length is > 2; cache miss
  //ost << "CACHE MISS" << endl;

  SCP_SET rv;

  for (auto& db_p : _vec)
  { scp_database& db { *(db_p) };

    const SCP_SET& calls { db[substring <std::string> (key, 0, 2)] };

    for (const auto& callsign : calls)
      if (::contains(callsign, key))
        rv += callsign;
  }

  _last_key = key;
  _last_result = move(rv);

  return _last_result;
}

/// clear the cache; also clear the caches of any children
void scp_databases::clear_cache(void)
{ clear_cache_no_children();

  FOR_ALL(_vec, [] (scp_database* db_p) { db_p->clear_cache(); } );
}

/// clear the cache without clearing the caches of any children
void scp_databases::clear_cache_no_children(void)
{ _last_key.clear();
  _last_result.clear();
}
