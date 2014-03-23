// $Id: multiplier.h 55 2014-03-22 20:32:08Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef MULTIPLIER_H
#define MULTIPLIER_H

/*!     \file multiplier.h

        Classes and functions related to managing multipliers
*/

#include "log_message.h"
#include "macros.h"
#include "serialization.h"

#include <array>
#include <set>
#include <string>

extern message_stream    ost;

// -----------  multiplier  ----------------

/*!     \class multiplier
        \brief encapsulate all the necessary stuff for a mult
*/

class multiplier
{
protected:

  bool                                                             _per_band;  ///< is this multiplier accumulated per-band?
  std::array< std::set< std::string /* values */>, N_BANDS + 1>    _worked;    ///< last entry is the all-band set
  std::set<std::string>                                            _known;     ///< all the (currently) known possible values
  bool                                                             _used;      ///< is this object in use?

public:

/// default constructor
  multiplier(void);

  READ_AND_WRITE(per_band);                       ///< get/set _per_band
  READ_AND_WRITE(used);                           ///< get/set _used

/*! \brief  add a value to the set of known values
    \param  str value to add
    \return whether the addition was successful

    Returns false if the value <i>str</i> was already known
*/
  const bool add_known(const std::string& str);

/*! \brief  add a container of values to the set of known values
    \param  k   container of values to add
*/
  template<typename T>
  inline void add_known(const T& k)
    { if (_used)
        for_each(k.cbegin(), k.cend(), [&] (const std::string& str) { add_known(str); } );
    }

/// add the value <i>str</i> to the known values
  inline void operator+=(const std::string& str)
    { add_known(str); }

/// remove the value <i>str</i> from the known values (if it is known)
  inline void remove_known(const std::string& str)
    { if (_used)
        _known.erase(str);
    }

/// remove the value <i>str</i> from the known values (if it is known)
  inline void operator-=(const std::string& str)
    { remove_known(str); }

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
  const bool add_worked(const std::string& str, const int b);

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
  inline const bool add_worked(const std::string& str, const BAND b)
    { return (add_worked(str, static_cast<int>(b))); }

/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
  void remove_worked(const std::string& str, const int b);

/// Returns whether the value <i>str</i> is a known multiplier
  inline const bool is_known(const std::string& str) const
    { return (_used ? (_known < str) : false); }

  const bool is_worked(const std::string& str, const int b) const;

  const size_t n_worked(const int b) const;

  inline const size_t n_known(void) const
    { return _known.size(); }

  const std::set<std::string> worked(const int b) const;

  inline const std::set<std::string> known(void) const
    { return _known; }

  inline void clear(void)
  { for (auto& ss : _worked)    // this is clearer than using for_each here
      ss.clear();
  }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _known
       & _per_band
       & _used
       & _worked;
  }
};

std::ostream& operator<<(std::ostream& ost, const multiplier& m);

#endif /* MULTIPLIER_H */
