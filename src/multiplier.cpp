// $Id: multiplier.cpp 202 2022-03-07 21:01:02Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   multiplier.cpp

    Classes and functions related to managing multipliers
*/

#include "multiplier.h"

using namespace std;

pt_mutex multiplier_mutex { "MULTIPLIER"s };          ///< one mutex for all the multiplier objects

// -----------  multiplier  ----------------

/*! \class  multiplier
    \brief  encapsulate all the necessary stuff for a mult
*/

/*! \brief      return only values that do NOT contain asterisks
    \param  mv  multiplier values
    \return     <i>mv</i>, but without any values that contain an asterisk
*/
MULTIPLIER_VALUES multiplier::_filter_asterisks(const MULTIPLIER_VALUES& mv) const
{ MULTIPLIER_VALUES rv { mv };

//    REMOVE_IF_AND_RESIZE(rv, [] (const std::string& str) { return contains(str, '*'); } );
    erase_if(rv, [] (const std::string& str) { return contains(str, '*'); } );

//    FOR_ALL(mv, [&rv] (const std::string& str) { if (!contains(str, '*')) rv += str; } );
//    std::ranges::copy_if(mv.begin(), mv.end(), rv.begin(), [] (const auto& str) { return !contains(str, '*'); } );    // I don't know why this doesn't work

    return rv;
}

/*! \brief          Add a value to the set of known values
    \param  str     value to add
    \return         whether the addition was successful

    Returns false if the value <i>str</i> was already known
*/
bool multiplier::add_known(const std::string& str)
{ SAFELOCK(multiplier); 

  if (!_used)
    return false;

  const bool rv { _known.insert(str).second };

  if (rv and _all_values_are_mults and contains(str, '*'))        // did we just add the first non-mult value?
    _all_values_are_mults = false;

  return rv;
}

/*! \brief          Remove a value from the known values
    \param  str     value to be removed

    Does nothing if <i>str</i> is not known
*/
void multiplier::remove_known(const string& str)
{ SAFELOCK(multiplier);

  if (_used)
    _known.erase(str);

//  _all_values_are_mults = ANY_OF(_known, [] (const string& str) { return contains(str, '*'); } );
  _all_values_are_mults = ALL_OF(_known, [] (const string& str) { return !contains(str, '*'); } );
}

/*! \brief          Is a particular value a known multiplier value?
    \param  str     value to test
    \return         whether <i>str</i> is a known multiplier value
*/
bool multiplier::is_known(const string& str) const
{ SAFELOCK(multiplier);

  return (_used ? (_known > str) : false);
}

/*! \brief          Add a worked multiplier
    \param  str     value that has been worked
    \param  b       band on which <i>str</i> has been worked
    \param  m       mode on which <i>str</i> has been worked
    \return         whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known.
    Adds even if it's NOT a mult value.
*/
bool multiplier::add_worked(const string& str, const BAND b, const MODE m)
{ SAFELOCK(multiplier);

  if ((_used) and is_known(str))                                          // add only known mults
  { const int b_nr { static_cast<int>(b) };
    const int m_nr { static_cast<int>(m) };

    auto& pb { _worked[m_nr] };
    bool  rv { (pb[b_nr].insert(str)).second };  // BAND, MODE

    if (rv)
    { pb[ANY_BAND].insert(str);        // ANY_BAND, MODE

      auto& pb_any { _worked[ANY_MODE] };

      pb_any[b_nr].insert(str);        // BAND, ANY_MODE
      pb_any[ANY_BAND].insert(str);    // ANY_BAND, ANY_MODE
    }

    return rv;
  }

  return false;
}

/*! \brief          Add a worked multiplier, even if it is unknown
    \param  str     value that has been worked
    \param  b       band on which <i>str</i> has been worked
    \param  m       mode on which <i>str</i> has been worked
    \return         whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
bool multiplier::unconditional_add_worked(const string& str, const BAND b, const MODE m)
{ add_known(str);

  return add_worked(str, b, m);
}

/*! \brief          Remove a worked multiplier
    \param  str     value to be worked
    \param  b       band on which <i>str</i> is to be removed
    \param  m       mode on which <i>str</i> has been worked

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
void multiplier::remove_worked(const string& str, const BAND b, const MODE m)
{ SAFELOCK(multiplier);

  if (_used)
  { const int b_nr { static_cast<int>(b) };
    const int m_nr { static_cast<int>(m) };

    _worked[m_nr][b_nr].erase(str);

// is it still present in any band for this mode?
    bool present { false };

    for (int n {MIN_BAND}; n < MAX_BAND; ++n)
      present |= (_worked[m_nr][n] > str);

    if (!present)
      _worked[m_nr][ANY_BAND].erase(str);

// is it still present in any mode for this band?
    present = false;

    for (int n {MIN_MODE}; n < MAX_MODE; ++n)
      present |= (_worked[n][b_nr] > str);

    if (!present)
      _worked[ANY_MODE][b_nr].erase(str);

// is it still present in any band and any mode?
    present = ( (_worked[m_nr][ANY_BAND] > str) or (_worked[ANY_MODE][b_nr] > str) );

    if (!present)
      _worked[ANY_MODE][ANY_BAND].erase(str);
  }
}

/*! \brief          Has a station been worked on a particular band and mode?
    \param  str     callsign to test
    \param  b       band to test
    \param  m       mode to test
    \return         whether <i>str</i> has been worked on band <i>b</i> and mode <i>m</i>
*/
bool multiplier::is_worked(const string& str, const BAND b, const MODE m) const
{ SAFELOCK(multiplier);

  if (!_used)
    return false;

  const auto&              pb               { _worked[ (_per_mode ? static_cast<int>(m) : ANY_MODE) ] };
  const MULTIPLIER_VALUES& worked_this_band { pb[ (_per_band ? b : ANY_BAND) ] };

//  return (worked_this_band.find(str) != worked_this_band.cend());
  return contains(worked_this_band, str);
}

/*! \brief      Number of mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     number of mults worked on band <i>b</i> and mode <i>m</i>
*/
size_t multiplier::n_worked(const BAND b, const MODE m) const
{ SAFELOCK(multiplier);

  if (!_used)
    return 0;

  const int   b_nr { static_cast<int>(b) };
  const int   m_nr { static_cast<int>(m) };
  const auto& pb   { _worked[m_nr] };

  if (_all_values_are_mults)
    return pb[b_nr].size();

// some values are not mults (e.g., XXX* in UBA contest)
  return _filter_asterisks(pb[b_nr]).size();
}

/*! \brief      Number of mults worked on a particular band, regardless of mode
    \param  b   band
    \return     number of mults worked on band <i>b</i>
*/
size_t multiplier::n_worked(const BAND b) const
{ SAFELOCK(multiplier);

  if (!_used)
    return 0;

  const auto& pb { _worked[ N_MODES ] };

  if (_all_values_are_mults)
    return pb[ (_per_band ? b : N_BANDS) ].size();

// some values are not mults (e.g., XXX* in UBA contest)
  return _filter_asterisks(pb[ (_per_band ? b : N_BANDS) ]).size();
}

/*! \brief      All the mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     all the mults worked on band <i>b</i> and mode <i>m</i>

    Includes any non-mult values
*/
MULTIPLIER_VALUES multiplier::worked(const int b, const int m) const
{ SAFELOCK(multiplier);

  if (!_used)
    return MULTIPLIER_VALUES();

  const auto& pb { _worked[ (_per_mode ? static_cast<int>(m) : ANY_MODE) ] };

  return pb[ (_per_band ? b : ANY_BAND) ];
}

/*! \brief          Write a <i>multiplier</i> object to an output stream
    \param  ost     output stream
    \param  m       object to write
    \return         the output stream
*/
ostream& operator<<(ostream& ost, const multiplier& m)
{ const auto flags { ost.flags() };

  ost << "multiplier is per-mode = " << boolalpha << m.per_mode() << endl
      << "multiplier is per-band = " << boolalpha << m.per_band() << endl
      << "worked multipliers:" << endl;

  for (size_t nm = 0; nm <= N_MODES; ++nm)
  { for (size_t n = 0; n <= N_BANDS; ++n)
    { ost << "mode = " << nm << ", band = " << n << " : ";

      const MULTIPLIER_VALUES& ss { m.worked(n, static_cast<MODE>(nm)) };

      for (const auto& worked : ss)
        ost << worked << " ";

      ost << endl;
    }
  }

  ost << "known multipliers: ";

  const MULTIPLIER_VALUES& ss { m.known() };

  for (const auto& known : ss)
    ost << known << " ";

  ost << "multiplier is used = " << m.used() << endl;

  ost.flags(flags);

  return ost;
}
