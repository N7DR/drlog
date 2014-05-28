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

#if 0
multiplier::multiplier(const multiplier& obj)
{ obj._mult_mutex.lock();

  _per_band = obj._per_band;
  _worked = obj._worked;
  _known = obj._known;
  _used = obj._used;

  obj._mult_mutex.unlock();

//bool                                                             _per_band;  ///< is this multiplier accumulated per-band?
///std::array< std::set< std::string /* values */>, N_BANDS + 1>    _worked;    ///< last entry is the all-band set
//std::set<std::string>                                            _known;     ///< all the (currently) known possible values
//bool                                                             _used;      ///< is this object in use?

}
#endif

const bool multiplier::add_known(const string& str)
{ if (_used)
    return ( (_known.insert(str)).second );
  else
    return false;
}

const bool multiplier::add_worked(const std::string& str, const int b)
{
  ost << "add_worked: Adding " << str << " for band " << b << endl;

  if ((_used) and is_known(str))                                          // add only known mults
//    return ( (_worked[ (_per_band ? b : N_BANDS) ].insert(str)).second );
  { if (_per_band)
    { const bool rv = (_worked[b].insert(str)).second;
//      _worked[N_BANDS].insert(str);

      return rv;
    }
    else // not per-band
    { const bool rv = (_worked[b].insert(str)).second;  // keep track of per-band anyway, in case of single-band entry
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

//  return  ( _worked[ (_per_band ? b : N_BANDS) ].size() );
  return  ( _worked[b].size() );
}

const set<string> multiplier::worked(const int b) const
{ if (!_used)
    return set<string>();

//  return _worked[ (_per_band ? b : N_BANDS) ];
  return _worked[b];
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
