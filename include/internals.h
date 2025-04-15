// $Id: internals.h 265 2025-03-31 01:32:02Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   internals.h

    Objects and functions related to GNU internals
*/

#ifndef INTERNALS_H
#define INTERNALS_H

#include "macros.h"

#include <stacktrace>
#include <string>
#include <vector>

#include <execinfo.h>

/// whether to acquire a backtrace during construction
enum class BACKTRACE { ACQUIRE,
                       NO_ACQUIRE
                     };

// see https://www.gnu.org/software/libc/manual/html_node/Backtraces.html

#if 0
// ------------------------------------  gcc_backtrace  ----------------------------------

/*! \class  gcc_backtrace
    \brief  Encapsulate and manage a GNU GCC backtrace
*/

class gcc_backtrace
{
protected:

  std::vector<std::string> _backtrace { };      ///< the names of functions
  int                      _max_size  { 10 };   ///< maximum number of elements in the trace

public:

/*! \brief            Constructor
    \param  acquire   whether to acquire a backtrace during construction
    \param  max_sz    maximum number of functions in the backtrace
*/
  explicit gcc_backtrace(const BACKTRACE acq = BACKTRACE::NO_ACQUIRE, const int max_sz = 10);

  READ_AND_WRITE(max_size);         // maximum number of elements in the trace

/// Get the number of functions in the backtrace
  inline size_t size(void) const
    { return _backtrace.size(); }

/*! \brief      Get a function in the backtrace
    \param  n   get the nth (wrt 0) function in the backtrace
    \return     the name of the function at the <i>n</i>th position in the backtrace
*/
  inline std::string operator[](const size_t n) const
    { return _backtrace[n]; }

/// Acquire a backtrace
  void acquire(void);

/// Return as human-readable string; one level per line
  std::string to_string(void) const;
};

/*! \brief          Write a <i>gcc_backtrace</i> object to an output stream
    \param  ost     output stream
    \param  bt      object to write
    \return         the output stream
*/
inline std::ostream& operator<<(std::ostream& ost, const gcc_backtrace& bt)
  { return (ost << bt.to_string()); }

// this appears to be non-working in gcc 14.2:
//  even if I add stdc++_libbacktrace to the cmake file, the link fails as the library can't be found
#endif

// ------------------------------------  backtrace  ----------------------------------

/*! \class  std_backtrace
    \brief  Encapsulate and manage std::stacktrace

    For reasons I don't understand, we can't just call this class "backtrace"
*/

class std_backtrace
{
protected:

  std::stacktrace _backtrace { };      ///< the names of functions

public:

/*! \brief        Constructor
    \param  acq   whether to acquire a backtrace during construction
*/
  explicit std_backtrace(const BACKTRACE acq = BACKTRACE::NO_ACQUIRE);

/// Get the number of functions in the backtrace
  inline size_t size(void) const
    { return _backtrace.size(); }

/*! \brief      Get a function in the backtrace
    \param  n   get the nth (wrt 0) function in the backtrace
    \return     the name of the function at the <i>n</i>th position in the backtrace
*/
  inline std::string operator[](const size_t n)
    { return std::to_string(_backtrace.at(n)); }      // there is no operator[] for std::stacktrace

/// Acquire a backtrace
  inline void acquire(void)
    { _backtrace = std::stacktrace::current(); }

/// Return as human-readable string; one level per line
  std::string to_string(void) const
    { return std::to_string(_backtrace); }
};

/*! \brief          Write a <i>backtrace</i> object to an output stream
    \param  ost     output stream
    \param  bt      object to write
    \return         the output stream
*/
inline std::ostream& operator<<(std::ostream& ost, const std_backtrace& bt)   // I don't know why the "class" is necessary
  { return (ost << bt.to_string()); }

#endif    // INTERNALS_H
