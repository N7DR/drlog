// $Id: multiplier.h 85 2014-12-01 23:26:41Z  $

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
#include "pthread_support.h"
#include "serialization.h"

#include <array>
#include <set>
#include <string>

extern message_stream    ost;

// -----------  multiplier  ----------------

/*!     \class multiplier
        \brief encapsulate necessary stuff for a mult

        Not thread-safe; I don't think that that matters
*/

class multiplier
{
protected:

  bool                                                             _per_band;  ///< is this multiplier accumulated per-band?
  bool                                                             _per_mode;  ///< is this multiplier accumulated per-mode?

  std::array< std::set< std::string /* values */>, N_BANDS + 1>    _worked;    ///< last entry is the all-band set

  std::array< std::array< std::set< std::string /* values */>, N_BANDS + 1>, N_MODES + 1 > _workedbm;  // will replace _worked eventually

  std::set<std::string>                                            _known;     ///< all the (currently) known possible values
  bool                                                             _used;      ///< is this object in use?

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  worked_for_a_mode   reference to array of worked information for a single mode
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
  const bool _add_worked(const std::string& str, const int b, std::array< std::set< std::string /* values */>, N_BANDS + 1>& worked_for_a_mode);

public:

/// default constructor
  multiplier(void);

  READ_AND_WRITE(per_band);                       ///< get/set _per_band
  READ_AND_WRITE(per_mode);                       ///< get/set _per_mode
  READ_AND_WRITE(used);                           ///< get/set _used

/*! \brief  add a value to the set of known values
    \param  str value to add
    \return whether the addition was successful

    Returns false if the value <i>str</i> was already known
*/
  inline const bool add_known(const std::string& str)
    { return ( _used ? ( (_known.insert(str)).second ) : false ); }

/*! \brief  add a container of string values to the set of known values
    \param  k   container of values to add
    \return     Number of values added
*/
  template<typename T>
  inline const unsigned int add_known(const T& k)
    { unsigned int rv = 0;

      if (_used)
//        for_each(k.cbegin(), k.cend(), [&] (const std::string& str) { if (add_known(str)) rv++; } );
        FOR_ALL(k, [&] (const std::string& str) { if (add_known(str)) rv++; } );

      return rv;
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
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
    const bool add_worked(const std::string& str, const int b, const MODE m);

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
  inline const bool add_worked(const std::string& str, const BAND b)
    { return (add_worked(str, static_cast<int>(b))); }

/*! \brief  add a worked multiplier
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known
*/
  inline const bool add_worked(const std::string& str, const BAND b, const MODE m)
    { return (add_worked(str, static_cast<int>(b), m)); }

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
  const bool unconditional_add_worked(const std::string& str, const int b);

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
  const bool unconditional_add_worked(const std::string& str, const int b, const MODE m);

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
  inline const bool unconditional_add_worked(const std::string& str, const BAND b)
    { return (unconditional_add_worked(str, static_cast<int>(b))); }

/*! \brief  add a worked multiplier, even if it is unknown
    \param  str value that has been worked
    \param  b   band on which <i>str</i> has been worked
    \param  m   mode on which <i>str</i> has been worked
    \return whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
  inline const bool unconditional_add_worked(const std::string& str, const BAND b, const MODE m)
    { return (unconditional_add_worked(str, static_cast<int>(b), m)); }

/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
  void remove_worked(const std::string& str, const int b);

/*! \brief  remove a worked multiplier
    \param  str value to be worked
    \param  b   band on which <i>str</i> is to be removed
    \param  m   mode on which <i>str</i> has been worked

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
  void remove_worked(const std::string& str, const int b, const MODE m);

/// Returns whether the value <i>str</i> is a known multiplier
  inline const bool is_known(const std::string& str) const
    { return (_used ? (_known < str) : false); }

/*! \brief      Has a station been worked on a particular band?
    \param  str callsign to test
    \param  b   band to be tested
*/
  const bool is_worked(const std::string& str, const int b) const;

/*! \brief      Has a station been worked on a particular band and mode?
    \param  str callsign to test
    \param  b   band to be tested
*/
  const bool is_worked(const std::string& str, const int b, const MODE m) const;

/// Number of mults worked on a particular band
  inline const size_t n_worked(const int b) const
    { return (_used ? _worked[b].size() : 0); }

/// Number of known mults
  inline const size_t n_known(void) const
    { return _known.size(); }

/// All the mults worked on a particular band
  inline const std::set<std::string> worked(const int b) const
    { return (_used ? _worked[b] : std::set<std::string>() ); }

/// All the known mults
  inline const std::set<std::string> known(void) const
    { return _known; }

/// Set all bands to state in which no mults have been worked
  inline void clear(void)
  { for (auto& ss : _worked)    // this is, I think, clearer than using for_each here
      ss.clear();
  }

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _known
       & _per_band
       & _per_mode
       & _used
       & _worked
       & _workedbm;
  }
};

/// ostream << multiplier
std::ostream& operator<<(std::ostream& ost, const multiplier& m);

#endif /* MULTIPLIER_H */
