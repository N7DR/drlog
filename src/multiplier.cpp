// $Id: multiplier.cpp 279 2025-12-01 15:09:34Z  $

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
MULT_SET multiplier::_filter_asterisks(const MULT_SET& mv) const
{ //return SR::to<MULT_SET>(mv | SRV::filter([] (const string& str) { return !contains(str, '*'); }));
  return SR::to<MULT_SET>(mv | SRV::filter([] (const string& str) { return !str.contains('*'); }));

//  MULT_SET rv { mv };

//  erase_if(rv, [] (const string& str) { return contains(str, '*'); } );

//  return rv;
}

/*! \brief          Add a value to the set of known values
    \param  str     value to add
    \return         whether the addition was successful

    Returns false if the value <i>str</i> was already known
*/
//bool multiplier::add_known(const std::string& str)
bool multiplier::add_known(const string_view str)
{ SAFELOCK(multiplier);

  if (!_used)
    return false;

 // const string s       { str };
  const auto [_, rv] { _known.insert(string { str }) };

//  if (rv and _all_values_are_mults and contains(str, '*'))        // did we just add the first non-mult value?
  if (rv and _all_values_are_mults and str.contains('*'))        // did we just add the first non-mult value?
    _all_values_are_mults = false;

//  const auto [_, rv] { _known.insert(str) };
//
//  if (rv and _all_values_are_mults and contains(str, '*'))        // did we just add the first non-mult value?
//    _all_values_are_mults = false;

  return rv;
}

/*! \brief          Remove a value from the known values
    \param  str     value to be removed

    Does nothing if <i>str</i> is not known
*/
void multiplier::remove_known(const string_view str)
{ SAFELOCK(multiplier);

  if (_used)
  { STRC_ERASE(_known, str);

    //_known.erase(str);      // should work in C++23, but not yet supported (P2077R3): https://gcc.gnu.org/onlinedocs/gcc-14.2.0/libstdc++/manual/manual/status.html#status.iso.2023
                              // still not available: https://gcc.gnu.org/onlinedocs/gcc-15.1.0/libstdc++/manual/manual/status.html#status.iso.2020
  }

//  _all_values_are_mults = ALL_OF(_known, [] (const string& known_value) { return !contains(known_value, '*'); } );
  _all_values_are_mults = ALL_OF(_known, [] (const string& known_value) { return !known_value.contains('*'); } );
}

/*! \brief          Is a particular value a known multiplier value?
    \param  str     value to test
    \return         whether <i>str</i> is a known multiplier value
*/
bool multiplier::is_known(const string_view str) const
{ SAFELOCK(multiplier);

  return (_used ? _known.contains(str) : false);
}

/*! \brief          Add a worked multiplier
    \param  str     value that has been worked
    \param  b       band on which <i>str</i> has been worked
    \param  m       mode on which <i>str</i> has been worked
    \return         whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known.
    Adds even if it's NOT a mult value.
*/
bool multiplier::add_worked(const string_view str, const BAND b, const MODE m)
{ SAFELOCK(multiplier);

  if ((_used) and is_known(str))                                          // add only known mults
  { const int b_nr { static_cast<int>(b) };
    const int m_nr { static_cast<int>(m) };

    auto& pb       { _worked[m_nr] };
    auto [ _, rv ] { pb[b_nr].insert(string { str }) };  // BAND, MODE; the return value is required

    if (rv)
    { pb[ANY_BAND] += str;        // ANY_BAND, MODE

      auto& pb_any { _worked[ANY_MODE] };

      pb_any[b_nr] += str;        // BAND, ANY_MODE
      pb_any[ANY_BAND] += str;    // ANY_BAND, ANY_MODE
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
bool multiplier::unconditional_add_worked(const string_view str, const BAND b, const MODE m)
{ add_known(str);

  return add_worked(str, b, m);
}

/*! \brief          Remove a worked multiplier
    \param  str     value to be removed
    \param  b       band on which <i>str</i> is to be removed
    \param  m       mode on which <i>str</i> is to be removed

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
void multiplier::remove_worked(const string_view str, const BAND b, const MODE m)
{ SAFELOCK(multiplier);

  if (_used)
  { const int b_nr { static_cast<int>(b) };
    const int m_nr { static_cast<int>(m) };

//    _worked[m_nr][b_nr].erase(str);
    STRC_ERASE(_worked[m_nr][b_nr], str);

// is it still present in any band for this mode?
    bool present { false };

    for (int n {MIN_BAND}; !present and (n <= MAX_BAND); ++n)
      present = _worked[m_nr][n].contains(str);

    if (!present)
//      _worked[m_nr][ANY_BAND].erase(str);
      STRC_ERASE(_worked[m_nr][ANY_BAND], str);

// is it still present in any mode for this band?
    present = false;

    for (int n {MIN_MODE}; !present and (n <= MAX_MODE); ++n)
      present = _worked[n][b_nr].contains(str);

    if (!present)
//      _worked[ANY_MODE][b_nr].erase(str);
      STRC_ERASE(_worked[ANY_MODE][b_nr], str);

// is it still present in any band and any mode?
    present = (_worked[m_nr][ANY_BAND].contains(str) or _worked[ANY_MODE][b_nr].contains(str) );

    if (!present)
//      _worked[ANY_MODE][ANY_BAND].erase(str);
      STRC_ERASE(_worked[ANY_MODE][ANY_BAND], str);
  }
}

/*! \brief          Has a station been worked on a particular band and mode?
    \param  str     callsign to test
    \param  b       band to test
    \param  m       mode to test
    \return         whether <i>str</i> has been worked on band <i>b</i> and mode <i>m</i>
*/
bool multiplier::is_worked(const string_view str, const BAND b, const MODE m) const
{ SAFELOCK(multiplier);

  if (!_used)
    return false;

  const auto&     pb               { _worked[ (_per_mode ? static_cast<int>(m) : ANY_MODE) ] };
  const MULT_SET& worked_this_band { pb[ (_per_band ? b : ANY_BAND) ] };

  return worked_this_band.contains(str);
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
    return pb[ (_per_band ? static_cast<unsigned int>(b) : N_BANDS) ].size();

// some values are not mults (e.g., XXX* in UBA contest)
  return _filter_asterisks(pb[ (_per_band ? static_cast<unsigned int>(b) : N_BANDS) ]).size();
}

/*! \brief      All the mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     all the mults worked on band <i>b</i> and mode <i>m</i>

    Includes any non-mult values
*/
MULT_SET multiplier::worked(const int b, const int m) const
{ SAFELOCK(multiplier);

  if (!_used)
    return MULT_SET { };

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

  for (size_t nm { 0 }; nm <= N_MODES; ++nm)
  { for (size_t n { 0 }; n <= N_BANDS; ++n)
    { ost << "mode = " << nm << ", band = " << n << " : ";

      for (const auto& worked : m.worked(n, static_cast<MODE>(nm)))
        ost << worked << " ";

      ost << endl;
    }
  }

  ost << "known multipliers: ";

  for (const auto& known : m.known())
    ost << known << " ";

  ost << "multiplier is used = " << m.used() << endl;

  ost.flags(flags);

  return ost;
}
