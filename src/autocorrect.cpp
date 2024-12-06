// $Id: autocorrect.cpp 241 2024-06-02 19:59:44Z  $

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

void alert(const string& msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);   ///< Alert the user

busts_database busts_db;

// -----------  autocorrect_database  ----------------

/*! \class  autocorrect_database
    \brief  The database of good calls for the (non-dynamic) autocorrect function
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
  const bool absent  {!present };

// return known good call
  if (present)            // for now, assume that all the calls in the database are good; maybe change this later; note that this test is repeated in the tests below
    return insert(str, str);

// absent should always be true from this point on; but let's not assume it in case we change something later

// long call ends with a bust of "TEST"
  static const set<string> broken_TEST { "EST"s, "NST"s, "TEAT"s, "TEIT"s, "TENT"s, "TETT"s, "TRT"s, "TUT"s };

  for ( const auto& broken_suffix : broken_TEST )
  { const size_t broken_length { broken_suffix.size() };

    if ( absent and (str.size() >= (broken_length + 3)) and str.ends_with(broken_suffix) )
    { if (const string call_to_test { substring <std::string> (str, 0, str.size() - broken_length) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// extraneous:
//   E in front of a US K call
//   T in front of a US K call
//   T in front of a US N call
  if (absent and (str.starts_with("EK"sv) or str.starts_with("TK"sv) or str.starts_with("TN"sv)))
  { if (const string call_to_test { substring <std::string> (str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// PA copied as GA
  if (absent and str.starts_with("GA"sv))
  { if (absent)
    { if (const string call_to_test { "PA"s + substring <std::string> (str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// US N call copied as I#
  if (absent and str.starts_with('I') and (str.length() > 1) and isdigit(str[1]))
  { if (const string call_to_test { "N"s + substring <std::string> (str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// JA miscopied as JT
  if (str.starts_with("JT"sv))
  { if (absent)
    { if (const string call_to_test { "JA"s + substring <std::string> (str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial K or initial W copied as an initial M; note that if both K and W are valid, then we simply choose K
  if (str.starts_with('M'))
  { if (absent)
    { const string sub { substring <std::string> (str, 1) };

      if (const string call_to_test { "K"s + sub }; contains(call_to_test))
        return insert(str, call_to_test);

      if (const string call_to_test { "W"s + sub }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial L copied as an initial D
  if (str.starts_with('D'))
  { if (absent)
    { if (const string call_to_test { "L"s + substring <std::string> (str, 1) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial W copied as an initial M
//  if (str.starts_with('M'))
//  { if (absent)
//    { if (const string call_to_test { "W"s + substring <std::string> (str, 1) }; contains(call_to_test))
//        return insert(str, call_to_test);
//    }
//  }

// UA copied as MA
  if (str.starts_with("MA"sv))
  { if (absent)
    { if (const string call_to_test { "UA"s + substring <std::string> (str, 2) }; contains(call_to_test))
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
        
        if (const string call_to_test { "J"s + substring <std::string> (str, 1) }; contains(call_to_test))
          return insert(str, call_to_test);

      default :
        break;
    }
  }

// US K call copied as TT#
  if (absent and str.starts_with("TT"sv) and (str.length() > 2) and isdigit(str[2]))
  { if (const string call_to_test { "K"s + substring <std::string> (str, 2) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// US N call copied as T#
  if (absent and str.starts_with('T') and (str.length() > 1) and isdigit(str[1]))
  { if (const string call_to_test { "N"s + substring <std::string> (str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// initial PY copied as initial TM
  if (absent and str.starts_with("TM"sv))
  { if (const string call_to_test { "PY"s + substring <std::string> (str, 2) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// /P is quite often reported by the RBN as /W, especially in NFD
  if (absent and str.ends_with("/W"sv))
  { const string base_call_to_test { substring <std::string> (str, 0, str.length() - 2) };

    if (contains(base_call_to_test) or contains(base_call_to_test + "/P"s))
      return insert(str, base_call_to_test + "/P"s);
  }

  return insert(str, str);
}

// -----------  band_dynamic_autocorrect_database  ----------------

/*! \class  band_dynamic_autocorrect_database
    \brief  A single-band database for the dynamic lookup
*/

/*! \brief              Prune the database by removing old minutes
    \param  n_minutes   remove all data older than <i>n_minutes</i> ago
*/
void band_dynamic_autocorrect_database::prune(const int n_minutes)
{ const auto now_min    { now_minutes };
  const auto target_min { now_min - n_minutes };

  set<time_t> keys_to_remove;               // in theory this is unnecessary, as there will be, at most, only one element. But be paranoid.

  lock_guard<recursive_mutex> lg(_mtx);

//  for (auto it { _data_map_map_map.begin() }; it != _data_map_map_map.end(); ++it)      // remove old keys (which are times in minutes)
//    if (it->first <= target_min)
//      keys_to_remove += it->first;
//  for (const auto& [ time_in_minutes, map_element ] : _data_map_map_map)
  for (const auto& [ time_in_minutes, _ ] : _data_map_map_map)
    if (time_in_minutes <= target_min)
      keys_to_remove += time_in_minutes;

  FOR_ALL(keys_to_remove, [this] (const time_t key_to_remove) { _data_map_map_map -= key_to_remove; });
}

/*! \brief      Set the value of the band
    \param  b   the new value of the band
*/
void band_dynamic_autocorrect_database::to_band(const BAND b)
{ lock_guard<recursive_mutex> lg(_mtx);

  _b = b;
  _f_min_100 = lower_edge(b).hz() / 100;
  _f_max_100 = upper_edge(b).hz() / 100;
}

/*! \brief          Add a post to the database
    \param  post    post to add
*/
void band_dynamic_autocorrect_database::insert(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (post.band() != _b)    // check that the band is correct
    return;

  const F100_TYPE f_100   { static_cast<F100_TYPE>(post.freq().hz() / 100) };    // frequency of the post in the correct units
  const string&   call    { post.callsign() };
  const auto      now_min { now_minutes };

  auto& data_this_minute          { _data_map_map_map[now_min] };           // creates if doesn't exist
  auto& data_this_minute_and_freq { data_this_minute[f_100] };              // creates if doesn't exist
  auto& data_minute_f100_call     { data_this_minute_and_freq[call] };

  data_minute_f100_call++;
}

/*! \brief          Perform dynamic autocorrection on a call (if necessary)
    \param  post    post as received from the RBN
    \return         autocorrected call from the post
*/
string band_dynamic_autocorrect_database::autocorrect(const dx_post& post)
{ string rv { post.callsign() };            // default is to return an unchanged call

  const string post_call { post.callsign() };

  lock_guard<recursive_mutex> lg(_mtx);

  const F100_TYPE f_100       { static_cast<F100_TYPE>(post.freq().hz() / 100) };    // frequency of the post in the correct units
  const F100_TYPE low_target  { f_100 - 2 };            // target - 200 Hz
  const F100_TYPE high_target { f_100 + 3 };            // target + 200 Hz

  unordered_map<string /* call */, int /* n_occurrences */> hits;

  for (auto t_it { _data_map_map_map.cbegin() }; t_it != _data_map_map_map.cend(); ++t_it)
  {
// test only if frequency is in range
    const auto lb { (t_it->second).lower_bound(low_target) };
    const auto ub { (t_it->second).upper_bound(high_target) };

    for (auto f_it { lb }; f_it != ub; ++f_it)
    { for (auto c_it { f_it->second.cbegin() }; c_it != f_it->second.cend(); ++c_it)
      { const auto& c { c_it->first };
        const auto& n { c_it->second };

// check it if it's a match or a bust of a match
        if (post_call != c)
        { const string calls { pair_index(post_call, c) };

          if (busts_db.is_known_bust(calls))
            hits[c] += n;
          else
          { if (!busts_db.is_known_non_bust(calls))    // if neither known bust nor known non-bust
            { const bool bust { is_bust_call(post_call, c) };

              if (bust)
              { busts_db.known_bust(calls);
                hits[c] += n;
              }
              else
                busts_db.known_non_bust(calls);
            }
          }
        }
        else    // posted call matches one that's been seen before (post_call == c)
          hits[c] += n;
      }
    }
  }

// find the best match
  if (hits.empty())     // should never happen
  { alert("ERROR: hits is EMPTY");

    ost << "ERROR: hits is EMPTY" << endl;
    ost << this->to_string() << endl;
  }
  else
  { string best_match { hits.begin()->first };
    int    highest_n  { hits.begin()->second };

    for (auto it { next(hits.begin()) }; it != hits.end(); ++it)
    { if (it->second > highest_n)
      { best_match = it->first;
        highest_n = it->second;
      }
    }

    if (highest_n > 1)      // don't change if it's a 50/50 chance, since there's just no way to tell if we should do so
      rv = best_match;
  }

  return rv;
}

/*! \brief              Convert to a printable string
    \param  n_spaces    number of spaces to prepend to each line
    \return             printable string describing the database
*/
string band_dynamic_autocorrect_database::to_string(const int n_spaces) const
{ string rv;

  const string leading_spaces { (n_spaces == 0) ? ""s : create_string(' ', n_spaces) };

  lock_guard<recursive_mutex> lg(_mtx);

  for (const auto& [t, fcn] : _data_map_map_map)
  { rv += leading_spaces + ::to_string(t) + ":"s + EOL;

    for (const auto& [f, cn] : fcn)
    { rv += leading_spaces + "  "s + ::to_string(f) + "  :"s + EOL;

      for (const auto& [c, n] : cn)
        rv += leading_spaces + "    "s + ::to_string(c) + "    : "s + ::to_string(n) + EOL;
    }
  }

  return rv;
}

// -----------  dynamic_autocorrect_database  ----------------

/*! \class  dynamic_autocorrect_database
    \brief  A database for the dynamic lookup
*/

/*! \brief      Return the bands that are in the database
    \return     the bands containing data in the database
*/
set<BAND> dynamic_autocorrect_database::_known_bands(void) const
{ lock_guard<recursive_mutex> lg(_mtx);

  return ALL_KEYS_SET(_per_band_db);
}

/*! \brief      Does the database contain data from a particular band?
    \param  b   band to test
    \return     whether the database contains data from band <i>b</i>
*/
bool dynamic_autocorrect_database::contains_band(const BAND b) const
{ lock_guard<recursive_mutex> lg(_mtx);

  return _per_band_db.contains(b); 
}

/*! \brief      Add a band to the database
    \param  b   band to add
*/
void dynamic_autocorrect_database::operator+=(const BAND b)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (!_per_band_db.contains(b))
    _per_band_db[b].to_band(b);
}

/*! \brief          Add a post to the database
    \param  post    post to add
*/
void dynamic_autocorrect_database::insert(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (_per_band_db.contains(post.band()))
    _per_band_db[post.band()] += post;
}

/*! \brief              Prune the database by removing old minutes
    \param  n_minutes   remove all data older than <i>n_minutes</i> ago
*/
void dynamic_autocorrect_database::prune(const int n_minutes)
{ lock_guard<recursive_mutex> lg(_mtx);

  FOR_ALL(_known_bands(), [n_minutes, this] (const BAND b) { _per_band_db[b].prune(n_minutes); }); 
}

/*! \brief          Perform dynamic autocorrection on a call
    \param  post    post to subject to autocorrection
    \return         possibly-autocorrected call from <i>post</i>
*/
string dynamic_autocorrect_database::autocorrect(const dx_post& post)
{ lock_guard<recursive_mutex> lg(_mtx);

  if (_per_band_db.contains(post.band()))
    return _per_band_db[post.band()].autocorrect(post);

  alert( "ERROR in dynamic autocorrection");
  ost << "ERROR in dynamic autocorrection" << endl;
  ost << this->to_string() << endl;

  return post.callsign();
}

/*! \brief              Convert to a printable string
    \return             printable string describing the database
*/
string dynamic_autocorrect_database::to_string(void) const
{ string rv;

  const set<BAND> bands { _known_bands() };

  for (const BAND b : bands)
  { rv += "band: "s + BAND_NAME.at(b) + "m"s + EOL;
    rv += _per_band_db.at(b).to_string(2) + EOL;    // the "2" means to prepend each line with two spaces
  }

  return rv;
}
