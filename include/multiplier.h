// $Id: multiplier.h 205 2022-04-24 16:05:06Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef MULTIPLIER_H
#define MULTIPLIER_H

/*! \file   multiplier.h

    Classes and functions related to managing multipliers
*/

#include "bands-modes.h"
#include "log_message.h"
#include "macros.h"
#include "pthread_support.h"
#include "serialization.h"
#include "string_functions.h"

#include <array>
#include <set>
#include <string>

extern message_stream    ost;       ///< for debugging and logging
extern pt_mutex multiplier_mutex;   ///< mutex for multiplier objects

// -----------  multiplier  ----------------

/*! \class  multiplier
    \brief  encapsulate necessary stuff for a mult
*/

class multiplier
{
protected:

  bool         _all_values_are_mults { true };     ///< whether all the known values are actually mults

  MULT_SET _known      { };     ///< all the (currently) known possible values

//  std::set<std::string, decltype(&compare_mults)>  _known { compare_mults };

  bool         _per_band   { false };  ///< is this multiplier accumulated per band?
  bool         _per_mode   { false };  ///< is this multiplier accumulated per mode?

  bool         _used       { false };      ///< is this object in use?

/* Stored in the _worked array is the precise detail of what has been worked and where.
   However, "worked" as used as an access verb really means "do I need this mult"? Thus,
   writes and reads to/from _worked from outside the object are non-trivial.
*/
  std::array< std::array< MULT_SET /* values */, N_BANDS + 1>, N_MODES + 1 > _worked;  ///< the worked strings; the last entry in each row and column is for ANY_BAND/MODE

// SHOULD I HAVE distinct _worked_values AND _worked_mults??

/*! \brief      return only values that do NOT contain asterisks
    \param  mv  multiplier values
    \return     <i>mv</i>, but without any values that contain an asterisk
*/
  MULT_SET _filter_asterisks(const MULT_SET& mv) const;

public:

/// default constructor
  inline multiplier(void) = default;

  SAFE_READ(known, multiplier);                                     ///< all the (currently) known possible values
  SAFE_READ_AND_WRITE(per_band, multiplier);                        ///< is this multiplier accumulated per-band?
  SAFE_READ_AND_WRITE(per_mode, multiplier);                        ///< is this multiplier accumulated per-mode?
  SAFE_READ_AND_WRITE(used, multiplier);                            ///< is this object in use?

/*! \brief          Add a value to the set of known values
    \param  str     value to add
    \return         whether the addition was successful

    Returns false if the value <i>str</i> was already known
*/
  bool add_known(const std::string& str);

/*! \brief      Add a container of string values to the set of known values
    \param  k   container of values to add
    \return     number of values added
*/
  template<typename T>
    requires is_string<typename T::value_type>
  unsigned int add_known(const T& k)
    { unsigned int rv { 0 };

      SAFELOCK(multiplier);

      if (_used)
        FOR_ALL(k, [this, &rv] (const std::string& str) { if (add_known(str)) rv++; } );

      return rv;
    }

/*! \brief          Add a value to the known values
    \param  str     value to be added

    Does nothing if <i>str</i> is already known
*/
  inline void operator+=(const std::string& str)
    { add_known(str); }

/*! \brief          Remove a value from the known values
    \param  str     value to be removed

    Does nothing if <i>str</i> is not known
*/
  void remove_known(const std::string& str);

/*! \brief          Remove a value from the known values
    \param  str     value to be removed

    Does nothing if <i>str</i> is not known
*/
  inline void operator-=(const std::string& str)
    { remove_known(str); }

/*! \brief          Add a worked multiplier
    \param  str     value that has been worked
    \param  b       band on which <i>str</i> has been worked
    \param  m       mode on which <i>str</i> has been worked
    \return         whether <i>str</i> was successfully added to the worked multipliers

    Returns false if the value <i>str</i> is not known.
    Adds even if it's NOT a mult value.
*/
  bool add_worked(const std::string& str, const BAND b, const MODE m);

/*! \brief          Add a worked multiplier, even if it is unknown
    \param  str     value that has been worked
    \param  b       band on which <i>str</i> has been worked
    \param  m       mode on which <i>str</i> has been worked
    \return         whether <i>str</i> was successfully added to the worked multipliers

    Makes <i>str</i> known if it was previously unknown
*/
  bool unconditional_add_worked(const std::string& str, const BAND b, const MODE m);

/*! \brief          Remove a worked multiplier
    \param  str     value to be removed
    \param  b       band on which <i>str</i> is to be removed
    \param  m       mode on which <i>str</i> is to be removed

    Does nothing if <i>str</i> was not worked on <i>b</i>
*/
  void remove_worked(const std::string& str, const BAND b, const MODE m);

/*! \brief          Is a particular value a known multiplier value?
    \param  str     value to test
    \return         whether <i>str</i> is a known multiplier value
*/
//  bool is_known(const std::string& str) const;
  bool is_known(const std::string_view str) const;

/*! \brief          Has a station been worked on a particular band and mode?
    \param  str     callsign to test
    \param  b       band to be tested
    \param  m       mode to be tested
*/
  bool is_worked(const std::string& str, const BAND b, const MODE m) const;

/*! \brief      Number of mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     number of mults worked on band <i>b</i> and mode <i>m</i>
*/
  size_t n_worked(const BAND b, const MODE m) const;

/*! \brief      Number of mults worked on a particular band, regardless of mode
    \param  b   band
    \return     number of mults worked on band <i>b</i>
*/
  size_t n_worked(const BAND b) const;

/// Number of known mults
  inline size_t n_known_mults(void) const
    { SAFELOCK(multiplier);

      return _known.size();
    }

/*! \brief      All the mults worked on a particular band and mode
    \param  b   band
    \param  m   mode
    \return     all the mults worked on band <i>b</i> and mode <i>m</i>

    Includes any non-mult values
*/
  MULT_SET worked(const int b, const int m) const;

/// Set all bands and modes to state in which no mults have been worked
  inline void clear(void)
  { SAFELOCK(multiplier);

    for (auto& ass : _worked)    // this is, I think, clearer than using for_each here
      for (auto& ss : ass)
        ss.clear();
  }

/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
  { ar & _known
       & _per_band
       & _per_mode
       & _used
       & _worked;
  }
};

/*! \brief          Write a <i>multiplier</i> object to an output stream
    \param  ost     output stream
    \param  m       object to write
    \return         the output stream
*/
std::ostream& operator<<(std::ostream& ost, const multiplier& m);

#endif /* MULTIPLIER_H */
