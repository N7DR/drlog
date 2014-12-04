// $Id: multiplier.cpp 85 2014-12-01 23:26:41Z  $

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

/// default constructor
multiplier::multiplier(void) :
  _per_band(false),
  _per_mode(false),
  _used(false)
{
}

#if defined(SINGLE_MODE)
/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
const bool multiplier::add_worked(const string& str, const int b)
{ if ((_used) and is_known(str))                                          // add only known mults
  { if (_per_band)
      return ( (_worked[b].insert(str)).second );
    else // not per-band; keep track of per-band anyway, in case of single-band entry
    { const bool rv = (_worked[b].insert(str)).second;

      _worked[N_BANDS].insert(str);
      return rv;
    }
  }
  else
    return false;
}
#endif    // SINGLE_MODE

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
const bool multiplier::add_worked(const std::string& str, const int b, const MODE m)
{ if ((_used) and is_known(str))                                          // add only known mults
  { auto& pb = _workedbm[static_cast<int>(m)];

    if (_per_mode)
    { //auto& pb = _workedbm[static_cast<int>(m)];

      return (_add_worked(str, b, pb));
    }
    else    // not per mode; keep track of per-mode anyway, in case of single-mode entry
    { //auto& pb = _workedbm[static_cast<int>(m)];

      const bool rv = _add_worked(str, b, pb);

      pb[N_BANDS].insert(str);
      return rv;
    }
  }

  return false;
}

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
const bool multiplier::unconditional_add_worked(const std::string& str, const int b)
{ add_known(str);

  return add_worked(str, b);
}

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
const bool multiplier::unconditional_add_worked(const std::string& str, const int b, const MODE m)
{ add_known(str);

  return add_worked(str, b, m);
}

#if defined(SINGLE_MODE)
/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
void multiplier::remove_worked(const string& str, const int b)
{ if (_used)
   _worked[ (_per_band ? b : N_BANDS) ].erase(str);
}
#endif    // SINGLE_MODE

/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed
    \param  m   mode on which <i>str</i> has been worked

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
void multiplier::remove_worked(const std::string& str, const int b, const MODE m)
{ if (_used)
  { auto& pb = _workedbm[ (_per_mode ? static_cast<int>(m) : N_MODES) ];

    pb[ (_per_band ? b : N_BANDS) ].erase(str);
  }
}

#if defined(SINGLE_MODE)
/*! \brief      Has a station been worked on a particular band?
    \param  str callsign to test
    \param  b   band to be tested
*/
const bool multiplier::is_worked(const string& str, const int b) const
{ if (!_used)
    return false;

  const set<string>& worked_this_band = _worked[ (_per_band ? b : N_BANDS) ];

  return (worked_this_band.find(str) != worked_this_band.cend());
}
#endif    // SINGLE_MODE

/*! \brief      Has a station been worked on a particular band and mode?
    \param  str callsign to test
    \param  b   band to be tested
*/
const bool multiplier::is_worked(const std::string& str, const int b, const MODE m) const
{ if (!_used)
    return false;

  auto& pb = _workedbm[ (_per_mode ? static_cast<int>(m) : N_MODES) ];

  const set<string>& worked_this_band = pb[ (_per_band ? b : N_BANDS) ];

  return (worked_this_band.find(str) != worked_this_band.cend());
}

/// Number of mults worked on a particular band and mode
const size_t multiplier::n_worked(const int b, const MODE m) const
{ if (!_used)
    return 0;

  auto& pb = _workedbm[ (_per_mode ? static_cast<int>(m) : N_MODES) ];

  return pb[ (_per_band ? b : N_BANDS) ].size();
}

/// All the mults worked on a particular band and mode
const set<string> multiplier::worked(const int b, const MODE m) const
{ if (!_used)
    return set<string>();

  auto& pb = _workedbm[ (_per_mode ? static_cast<int>(m) : N_MODES) ];

  return pb[ (_per_band ? b : N_BANDS) ];
}

#if defined(SINGLE_MODE)
// ostream << multiplier
ostream& operator<<(ostream& ost, const multiplier& m)
{ const auto flags = ost.flags();

  ost << "multiplier is per-band = " << boolalpha << m.per_band() << endl
      << "worked multipliers:" << endl;

  { for (size_t n = 0; n <= N_BANDS; ++n)
    { ost << "band = " << n << " : ";

      const set<string>& ss = m.worked(n);

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
#else
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
#endif    // SINGLE_MODE
