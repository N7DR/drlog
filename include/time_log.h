// $Id: time_log.h 251 2024-09-09 16:39:37Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   time_log.h

    Functions and classes related to measuring and logging time
*/

#ifndef TIME_LOG_H
#define TIME_LOG_H

#include "macros.h"

#include <chrono>
#include <ratio>

// -----------  time_log  ----------------

/*! \class  time_log
    \brief  Class for measuring and printing time
*/

template <class A = std::chrono::microseconds>
class time_log
{
protected:

//  std::chrono::time_point<std::chrono::system_clock> _start { std::chrono::system_clock::now() };   ///< starting time point
//  std::chrono::time_point<std::chrono::system_clock> _end   { };                                    ///< ending time point

  TIME_POINT _start { NOW_TP() };   ///< starting time point
  TIME_POINT _end   { };            ///< ending time point

public:

  READ_AND_WRITE(start);                            ///< starting time point
  READ_AND_WRITE(end);                              ///< ending time point
 
/*! \brief  Start the timer
*/ 
  inline void start_now(void)
//    { _start = std::chrono::system_clock::now(); }
    { _start = NOW_TP(); }

/*! \brief  Stop the timer
*/ 
  inline void end_now(void)
//    { _end = std::chrono::system_clock::now(); }
    { _end = NOW_TP(); }

/*! \brief  Reset the timer to initial state, except that the timer is started at the current time
*/
  void restart(void)
  { //_start = std::chrono::system_clock::now();
    _start = NOW_TP();
    _end = { };
  }

/*! \brief  Return the time between the start time and the end time, in units of the duration template (default microseconds)

    Performs no sanity checking on the values of the start and end times
*/ 
  template <class U = double, class T = A>
  inline U time_span(void) const
    { return static_cast<U>(duration_cast<T>(_end - _start).count()); }

/*! \brief  Return the time between the start time and now, in units of the duration template (default microseconds)

    Performs no sanity checking on the values of the start time and now
*/
  template <class U = double, class T = A>
  inline U click(void) const
//    { return static_cast<U>(duration_cast<T>(std::chrono::system_clock::now() - _start).count()); }
    { return static_cast<U>(duration_cast<T>(NOW_TP() - _start).count()); }

/*! \brief  Return the time between the start time and the end time, in units of the duration template (default microseconds)

    Performs no sanity checking on the values of the start and end times
*/
  template <class U = double, class T = A>
  inline U duration(void) const
    { return time_span<U, T>(); }

/*! \brief  Return the time between the start time and now, in units of the duration template (default microseconds), and set the start time to now

    Performs no sanity checking on the value of the start time before using it
*/
  template <class U = double, class T = A>
  U duration_restart(void)
  { end_now();

    const U rv { duration<U, T>() };

    restart();
    
    return rv;;
  } 

/*! \brief  Perform a basic sanity check that the start and end values reflect actual times
*/
  inline bool valid(void) const
  { const static std::chrono::time_point<std::chrono::system_clock> empty { };
    
    return ( (_start != empty) and (_end != empty) and (_start <= _end) );
  }
};

#endif      // TIME_LOG_H
