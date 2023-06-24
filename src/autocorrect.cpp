// $Id: autocorrect.cpp 221 2023-06-19 01:57:55Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   autocorrect.cpp

    Objects and functions related to automatically correcting calls in RBN posts
*/

#include "autocorrect.h"
#include "log_message.h"
#include "string_functions.h"

using namespace std;

extern message_stream ost;          ///< for debugging and logging
extern MINUTES_TYPE   now_minutes;  ///< access the current time in minutes

// -----------  autocorrect_database  ----------------

/*! \class  autocorrect_database
    \brief  The database of good calls for the autocorrect function
*/

/*! \brief          Obtain an output call from an input
    \param  str     input call
    \return         <i>str</i> or a corrected version of same
*/
string autocorrect_database::corrected_call(const string& str) const
{ if (str.empty())
    return str;

// insert a mapping into the cache
  auto insert = [this] (const string& input, const string& output)
    { _cache += { input, output };

      if (output != input)
        ost << "  autocorrect: " << input << " -> " << output << endl;

      return output;
    };

// return cached value
  if (const string from_cache { MUM_VALUE(_cache, str) }; !from_cache.empty())
    return from_cache;

  const bool present { contains(str) };
  const bool absent {!present };

// return known good call
  if (present)            // for now, assume that all the calls in the database are good; maybe change this later; note that this test is repeated in the tests below
    return insert(str, str);

// absent should always be true from this point on; but let's not assume it in case we change something later

// extraneous:
//   E in front of a US K call
//   T in front of a US K call
//   T in front of a US N call
  if (str.starts_with("EK"s) or str.starts_with("TK"s) or str.starts_with("TN"s))
  { if (const string call_to_test { substring(str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// PA copied as GA
  if (str.starts_with("GA"s))
  { if (absent)
    { if (const string call_to_test { "PA"s + substring(str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// US N call copied as I#
  if (absent and str.starts_with('I') and (str.length() > 1) and isdigit(str[1]))
  { if (const string call_to_test { "N"s + substring(str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// JA miscopied as JT
  if (str.starts_with("JT"s))
  { if (absent)
    { if (const string call_to_test { "JA"s + substring(str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial K copied as an initial M
  if (str.starts_with('M'))
  { if (absent)
    { if (const string call_to_test { "K"s + substring(str, 1) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial W copied as an initial M
  if (str.starts_with('M'))
  { if (absent)
    { if (const string call_to_test { "W"s + substring(str, 1) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// UA copied as MA
  if (str.starts_with("MA"s))
  { if (absent)
    { if (const string call_to_test { "UA"s + substring(str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial J copied as an initial O
  if (str.starts_with('O') and (str.size() > 3))
  { switch (str[1])
    { case 'A' :
      case 'E' :
      case 'F' :
      case 'G' :
      case 'H' :
      case 'I' :
      case 'J' :
      case 'K' :
      case 'L' :
      case 'M' :
      case 'N' :
      case 'O' :
      case 'P' :
      case 'Q' :
      case 'R' :
      case 'S' :
        
        if (const string call_to_test { "J"s + substring(str, 1) }; contains(call_to_test))
          return insert(str, call_to_test);

      default :
        break;
    }
  }

// US K call copied as TT#
  if (absent and str.starts_with("TT") and (str.length() > 2) and isdigit(str[2]))
  { if (const string call_to_test { "K"s + substring(str, 2) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// US N call copied as T#
  if (absent and str.starts_with('T') and (str.length() > 1) and isdigit(str[1]))
  { if (const string call_to_test { "N"s + substring(str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// initial PY copied as initial TM
  if (absent and str.starts_with("TM"s))
  { if (const string call_to_test { "PY"s + substring(str, 2) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// /P is quite often reported by the RBN as /W, especially in NFD
  if (absent and str.ends_with("/W"s))
  { const string base_call_to_test { substring(str, 0, str.length() - 2) };

    if (contains(base_call_to_test) or contains(base_call_to_test + "/P"s))
      return insert(str, base_call_to_test + "/P"s);
  }

  return insert(str, str);
}

// -----------  band_dynamic_autocorrect_database  ----------------

/*! \class  band_dynamic_autocorrect_database
    \brief  A single-band database for the dynamic lookup
*/

set<std::string> band_dynamic_autocorrect_database::_get_all_calls(void) const
{ set<std::string> rv;

  lock_guard<recursive_mutex> lg(_mtx);

// std::map<time_t, std::map<std::string /* call */, std::map<uint32_t /* f_100 */, size_t /* number of appearances */>>> _data_map_map_map; // time in minutes, callsign, number of times the call appears

  for (const auto& [t, cfn] : _data_map_map_map)
    for (const auto& [c, fn] : cfn)
      rv += c;
  
  return rv;
}

void band_dynamic_autocorrect_database::prune(const int n_minutes)
{ lock_guard<recursive_mutex> lg(_mtx);

  const auto now_min    { now_minutes };
  const auto target_min { now_min - n_minutes };

// in theory this is unnecessary, as there will be, at most, only one element. But be paranoid.
  set<int> keys_to_remove;

  for (auto it { _data_map_map_map.begin() }; it != _data_map_map_map.end(); ++it)
    if (it->first <= target_min)
      keys_to_remove += it->first;

 // ost << "number of keys to remove for band " << BAND_NAME.at(_b) << " = " << keys_to_remove.size() << endl;

  FOR_ALL(keys_to_remove, [this] (const int key_to_remove) { /* ost << "erasing key: " << key_to_remove << endl; */ _data_map_map_map.erase(key_to_remove); });
}

void band_dynamic_autocorrect_database::to_band(const BAND b)
{ lock_guard<recursive_mutex> lg(_mtx);

  _b = b;
  _f_min_100 = lower_edge(b).hz() / 100;
  _f_max_100 = upper_edge(b).hz() / 100;
  _data = vector<post_struct>(_f_max_100 - _f_min_100 + 1) ;     // create vector with correct number of elements, one for each 100 Hz in the band
}

bool band_dynamic_autocorrect_database::insert(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (post.band() != _b)    // check that the band is correct
    return false;

  const decltype(_f_min_100) f_100       { static_cast<decltype(_f_min_100)>(post.freq().hz() / 100) };    // frequency of the post in the correct units
  const string&              call        { post.callsign() };
  const auto                 now_min     { now_minutes };

  auto& data_this_minute          = _data_map_map_map[now_min];  // creates if doesn't exist
  auto& data_this_minute_and_call = data_this_minute[call];      // creates if doesn't exist
  auto& data_minute_call_f100     = data_this_minute_and_call[f_100];

  data_minute_call_f100++;

  return false;
}

string band_dynamic_autocorrect_database::autocorrect(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  string rv { post.callsign() };

  const decltype(_f_min_100) f_100 { static_cast<decltype(_f_min_100)>(post.freq().hz() / 100) };    // frequency of the post in the correct units
  const string&              call  { post.callsign() };

  return rv;
}

//   std::map<time_t, std::map<std::string /* call */, std::map<uint32_t /* f_100 */, size_t /* number of appearances */>>> _data_map_map_map; // time in minutes, callsign, number of times the call appears
string band_dynamic_autocorrect_database::to_string(const int n_spaces) const
{ string rv;

  const string leading_spaces { (n_spaces == 0) ? ""s : create_string(' ', n_spaces) };

  lock_guard<recursive_mutex> lg(_mtx);

  for (const auto& [t, cfn] : _data_map_map_map)
  { rv += leading_spaces + ::to_string(t) + ":"s + EOL;

    for (const auto& [c, fn] : cfn)
    { rv += leading_spaces + "  "s + ::to_string(c) + "  :"s + EOL;

      for (const auto& [f, n] : fn)
      { rv += leading_spaces + "    "s + ::to_string(f) + "    :"s + ::to_string(n) + EOL;
      }
    }
  }

  return rv;
}

// -----------  dynamic_autocorrect_database  ----------------

/*! \class  dynamic_autocorrect_database
    \brief  A database for the dynamic lookup
*/

set<BAND> dynamic_autocorrect_database::_known_bands(void) const
{ lock_guard<recursive_mutex> lg(_mtx);

  return ALL_KEYS <set<BAND>> (_per_band_db);
}

bool dynamic_autocorrect_database::contains_band(const BAND b) const
{ lock_guard<recursive_mutex> lg(_mtx);

  return _per_band_db.contains(b); 
}

void dynamic_autocorrect_database::operator+=(const BAND b)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (!_per_band_db.contains(b))
    _per_band_db[b].to_band(b);
}

bool dynamic_autocorrect_database::insert(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (_per_band_db.contains(post.band()))
    _per_band_db[post.band()].insert(post);

  return false;
}

void dynamic_autocorrect_database::prune(const int n_minutes)
{ lock_guard<recursive_mutex> lg(_mtx);

  FOR_ALL(_known_bands(), [this, n_minutes] (const BAND b) { _per_band_db[b].prune(n_minutes); }); 
}

string dynamic_autocorrect_database::to_string(void) const
{ string rv;

//  lock_guard<recursive_mutex> lg(_mtx);

//  for (const auto& [b, tcfn] : _per_band_db)
  const set<BAND> bands { _known_bands() };

  for (const BAND b : bands)
  { rv += "band: "s + BAND_NAME.at(b) + "m"s + EOL;

    rv += _per_band_db.at(b).to_string(2) + EOL;
  }

  return rv;
}

#if 0

// -----------  band_n_posters_database  ----------------

/*! \class  band_n_posters_database
    \brief  A single-band database for the number of posters  of stations

    
*/

void band_n_posters_database::operator+=(const dx_post& post)
{ if (post.band() != _b)
    return;

  const auto now_min { now_minutes };

  lock_guard<recursive_mutex> lg(_mtx);

  auto& data_time_call { _data[now_min][post.callsign()] };

  data_time_call += post.poster();
}

//   std::map<time_t, std::unordered_map<std::string /* call */, std::unordered_set<std::string>>> _data; // time in minutes, callsign, number of posters
int band_n_posters_database::n_posters(const string& call) const
{ int rv { 0 };

  lock_guard<recursive_mutex> lg(_mtx);

  const set<time_t> times { all_keys<set<time_t>>(_data) };

  for (const auto t : times)
  { if (_data.at(t).contains(call))
      rv += _data.at(t).at(call).size();
  }

  return rv;
}

void band_n_posters_database::prune(const int n_minutes)
{ lock_guard<recursive_mutex> lg(_mtx);

  const auto now_min    { now_minutes };
  const auto target_min { now_min - n_minutes };

// in theory this is unnecessary, as there will be, at most, only one element. But be paranoid.
  set<int> keys_to_remove;

  for (auto it { _data.begin() }; it != _data.end(); ++it)
    if (it->first <= target_min)
      keys_to_remove += it->first;

 // ost << "number of keys to remove for band " << BAND_NAME.at(_b) << " = " << keys_to_remove.size() << endl;

  FOR_ALL(keys_to_remove, [this] (const int key_to_remove) { /* ost << "erasing key: " << key_to_remove << endl; */ _data.erase(key_to_remove); });
}

// -----------  n_posters_database  ----------------

/*! \class  n_posters_database
    \brief  database for the number of posters  of stations
*/

set<BAND> n_posters_database::_known_bands(void) const
{ lock_guard<recursive_mutex> lg(_mtx);

  return all_keys<set<BAND>>(_per_band_db);
}

bool n_posters_database::contains_band(const BAND b) const
{ lock_guard<recursive_mutex> lg(_mtx);

  return _per_band_db.contains(b); 
}

void n_posters_database::operator+=(const BAND b)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (!_per_band_db.contains(b))
    _per_band_db[b].to_band(b);
}

void n_posters_database::operator+=(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (_per_band_db.contains(post.band()))
    _per_band_db[post.band()] += post;
}

void n_posters_database::prune(const int n_minutes)
{ lock_guard<recursive_mutex> lg(_mtx);

  FOR_ALL(_known_bands(), [this, n_minutes] (const BAND b) { _per_band_db[b].prune(n_minutes); }); 
}

#endif     // 0
