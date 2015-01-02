// $Id: multiplier.cpp 87 2014-12-20 18:29:59Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file multiplier.cpp

        Classes and functions related to managing multipliers
*/

#include "multiplier.h"

using namespace std;

// -----------  multiplier  ----------------

/*!     \class multiplier
        \brief encapsulate all the necessary stuff for a mult
*/

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  worked_for_a_mode   reference to array of worked information for a single mode
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
#if 0
const bool multiplier::_add_worked(const std::string& str, const int b, std::array< std::set< std::string /* values */>, N_BANDS + 1>& worked_for_a_mode)
{ if ((_used) and is_known(str))                                          // add only known mults
  { if (_per_band)
      return ( (worked_for_a_mode[b].insert(str)).second );
    else // not per-band; keep track of per-band anyway, in case of single-band entry
    { const bool rv = (worked_for_a_mode[b].insert(str)).second;

      worked_for_a_mode[N_BANDS].insert(str);
      return rv;
    }
  }
  else
    return false;
}
#endif

/// default constructor
multiplier::multiplier(void) :
  _per_band(false),
  _per_mode(false),
  _used(false)
{
}

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
const bool multiplier::add_worked(const string& str, const BAND b, const MODE m)
{ if ((_used) and is_known(str))                                          // add only known mults
  { const int b_nr = static_cast<int>(b);
    const int m_nr = static_cast<int>(m);

    ost << "multiplier; adding worked " << str << ", band = " << BAND_NAME[b] << ", mode = " << MODE_NAME[m] << endl;

    auto& pb = _worked[m_nr];

    bool rv = (pb[b_nr].insert(str)).second;  // BAND, MODE

    if (rv)
    { pb[ANY_BAND].insert(str);   // ANY_BAND, MODE

      auto& pb_any = _worked[ANY_MODE];

      pb_any[b_nr].insert(str);    // BAND, ANY_MODE
      pb[ANY_BAND].insert(str);    // ANY_BAND, ANY_MODE
    }

    return rv;
  }

  return false;
}

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
//const bool multiplier::unconditional_add_worked(const std::string& str, const int b)
//{ add_known(str);
//
//  return add_worked(str, b);
//}

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
const bool multiplier::unconditional_add_worked(const string& str, const BAND b, const MODE m)
{ add_known(str);

  return add_worked(str, b, m);
}

/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed
    \param  m   mode on which <i>str</i> has been worked

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
void multiplier::remove_worked(const string& str, const BAND b, const MODE m)
{ if (_used)
  { const int b_nr = static_cast<int>(b);
    const int m_nr = static_cast<int>(m);

    _worked[m_nr][b_nr].erase(str);

// is it still present in any band for this mode?
    auto& pb = _worked[m_nr];
    bool present = false;

    for (int n = MIN_BAND; n < MAX_BAND; ++n)
      present = present or (_worked[m_nr][n] < str);

    if (!present)
      _worked[m_nr][ANY_BAND].erase(str);

// is it still present in any mode for this band?
    present = false;

    for (int n = MIN_MODE; n < MAX_MODE; ++n)
      present = present or (_worked[n][b_nr] < str);

    if (!present)
      _worked[ANY_MODE][b_nr].erase(str);

// is it still present in any band and any mode?
    present = ( (_worked[m_nr][ANY_BAND] < str) or (_worked[ANY_MODE][b_nr] < str) );

    if (!present)
      _worked[ANY_MODE][ANY_BAND].erase(str);
  }
}

/*! \brief      Has a station been worked on a particular band, regardless of mode?
    \param  str callsign to test
    \param  b   band to be tested
*/
#if 0
const bool multiplier::is_worked(const string& str, const int b) const
{ if (!_used)
    return false;

  auto& pb = _worked[ N_MODES ];

  const set<string>& worked_this_band = pb[ (_per_band ? b : N_BANDS) ];

  return (worked_this_band.find(str) != worked_this_band.cend());
}
#endif

/*! \brief      Has a station been worked on a particular band and mode?
    \param  str callsign to test
    \param  b   band to be tested
*/
const bool multiplier::is_worked(const std::string& str, const int b, const MODE m) const
{ if (!_used)
    return false;

  auto& pb = _worked[ (_per_mode ? static_cast<int>(m) : N_MODES) ];

  const set<string>& worked_this_band = pb[ (_per_band ? b : N_BANDS) ];

  return (worked_this_band.find(str) != worked_this_band.cend());
}

/*! \brief      Number of mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     number of mults worked on band <i>b</i> and mode <i>m</i>
*/
const size_t multiplier::n_worked(const BAND b, const MODE m) const
{ if (!_used)
    return 0;

  const int b_nr = static_cast<int>(b);
  const int m_nr = static_cast<int>(m);

  const auto& pb = _worked[m_nr];

  return pb[b_nr].size();
}

const size_t multiplier::n_worked(const int b) const
{ if (!_used)
    return 0;

  auto& pb = _worked[ N_MODES ];

  return pb[ (_per_band ? b : N_BANDS) ].size();
}

/*! \brief      All the mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     all the mults worked on band <i>b</i> and mode <i>m</i>
*/
const set<string> multiplier::worked(const int b, const int m) const
{ if (!_used)
    return set<string>();

  auto& pb = _worked[ (_per_mode ? static_cast<int>(m) : ANY_MODE) ];

  ost << "multiplier::worked for band = " << BAND_NAME[b] << ", mode = " << MODE_NAME[m] << " is:" << endl;
  ost << "_per_mode = " << boolalpha << _per_mode << noboolalpha << endl;

  set<string> tmp = pb[ (_per_band ? b : ANY_BAND) ];
  string str("  ");

  for (auto& s : tmp)
    str += s + " ";

  ost << str << endl;

  return pb[ (_per_band ? b : ANY_BAND) ];
//  auto& pb = _worked[m];
//  return pb[b];
}

/// All the mults worked on a particular band, regardless of mode
const set<string> multiplier::worked(const int b) const
{ if (!_used)
    return set<string>();

//  set<string> rv;

  auto& pb = _worked[ N_MODES ];

  return pb[ (_per_band ? b : N_BANDS) ];
}

// ostream << multiplier
ostream& operator<<(ostream& ost, const multiplier& m)
{ const auto flags = ost.flags();

  ost << "multiplier is per-mode = " << boolalpha << m.per_mode() << endl
      << "multiplier is per-band = " << boolalpha << m.per_band() << endl
      << "worked multipliers:" << endl;

  for (size_t nm = 0; nm <= N_MODES; ++nm)
  { for (size_t n = 0; n <= N_BANDS; ++n)
    { ost << "mode = " << nm << ", band = " << n << " : ";

      const set<string>& ss = m.worked(n, static_cast<MODE>(nm));

      for (const auto& worked : ss)
        ost << worked << " ";

      ost << endl;
    }
  }

  ost << "known multipliers: ";

  const set<string>& ss = m.known();

  for (const auto& known : ss)
    ost << known << " ";

  ost << "multiplier is used = " << m.used() << endl;

  ost.flags(flags);

  return ost;
}

