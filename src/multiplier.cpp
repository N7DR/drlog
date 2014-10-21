// $Id: multiplier.cpp 80 2014-10-20 18:47:10Z  $

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

/// default constructor
multiplier::multiplier(void) :
  _per_band(false),
  _used(false)
{
}

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
const bool multiplier::add_worked(const string& str, const int b)
{ if ((_used) and is_known(str))                                          // add only known mults
  { if (_per_band)
    { //const bool rv = (_worked[b].insert(str)).second;

      return ( (_worked[b].insert(str)).second );
    }
    else // not per-band; keep track of per-band anyway, in case of single-band entry
    { const bool rv = (_worked[b].insert(str)).second;

      _worked[N_BANDS].insert(str);
      return rv;
    }
  }
  else
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

/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
void multiplier::remove_worked(const string& str, const int b)
{ if (_used)
   _worked[ (_per_band ? b : N_BANDS) ].erase(str);
}

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

// ostream << multiplier
ostream& operator<<(ostream& ost, const multiplier& m)
{ const auto flags = ost.flags();

  ost << "multiplier is per-band = " << boolalpha << m.per_band() << endl
      << "worked multipliers:" << endl;

  for (size_t n = 0; n <= N_BANDS; ++n)
  { ost << "band = " << n << " : ";

    const set<string>& ss = m.worked(n);

    for (const auto& worked : ss)
      ost << worked << " ";

    ost << endl;
  }

  ost << "known multipliers: ";

  const set<string>& ss = m.known();

  for (const auto& known : ss)
    ost << known << " ";

  ost << "multiplier is used = " << m.used() << endl;

  ost.flags(flags);

  return ost;
}
