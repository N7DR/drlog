// $Id: multiplier.cpp 43 2013-12-07 20:29:56Z  $

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

multiplier::multiplier(void) :
  _per_band(false),
  _used(false)
{
}

const bool multiplier::add_known(const string& str)
{ if (_used)
    return ( (_known.insert(str)).second );
  else
    return false;
}

const bool multiplier::add_worked(const std::string& str, const int b)
{ if ((_used) and is_known(str))                                          // add only known mults
    return ( (_worked[ (_per_band ? b : N_BANDS) ].insert(str)).second );
  else
    return false;
}

void multiplier::remove_worked(const string& str, const int b)
{ if (_used)
   _worked[ (_per_band ? b : N_BANDS) ].erase(str);
}

const bool multiplier::is_worked(const string& str, const int b) const
{ if (!_used)
    return false;

  const set<string>& worked_this_band = _worked[ (_per_band ? b : N_BANDS) ];

  return (worked_this_band.find(str) != worked_this_band.cend());
}

const size_t multiplier::n_worked(const int b) const
{ if (!_used)
    return 0;

  return  ( _worked[ (_per_band ? b : N_BANDS) ].size() );
}

const set<string> multiplier::worked(const int b) const
{ if (!_used)
    return set<string>();

  return _worked[ (_per_band ? b : N_BANDS) ];
}

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
