// $Id$

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

  std::chrono::time_point<std::chrono::system_clock> _start { std::chrono::system_clock::now() };   ///< starting time point
  std::chrono::time_point<std::chrono::system_clock> _end   { };                                    ///< ending time point
  
public:

  READ_AND_WRITE(start);                            ///< starting time point
  READ_AND_WRITE(end);                              ///< ending time point
 
/*! \brief  Start the timer
*/ 
  void start_now(void)
    { _start = std::chrono::system_clock::now(); }

/*! \brief  Stop the timer
*/ 
  void end_now(void)
    { _end = std::chrono::system_clock::now(); }

/*! \brief  Reset the timer to initial state, except that the timer is started at the current time
*/
  void restart(void)
    { _start = std::chrono::system_clock::now();
      _end = { };
    }
  
  template <class U = double, class T = A>
  inline U time_span(void) const
    { return static_cast<U>(duration_cast<T>(_end - _start).count()); }

  template <class U = double, class T = A>
  inline U duration(void) const
    { return time_span<U, T>(); }

  template <class U = double, class T = A>
  inline U split(void)
  { end_now();
    
    return time_span<U, T>();
  }

  template <class U = double, class T = A>
  inline U duration_restart(void)
  { end_now();

    const U rv { duration<U, T>() };

    restart();
    
    return rv;;
  } 

  bool valid(void) const
    { const static std::chrono::time_point<std::chrono::system_clock> empty { };
    
      return ( (_start != empty) and (_end != empty) and (_start <= _end) ); 
    }
};

#endif      // TIME_LOG_H
