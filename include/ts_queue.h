// $Id: ts_queue.h 213 2022-12-15 17:11:46Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef TS_QUEUE_H
#define TS_QUEUE_H

/*! \file   ts_queue.h

    Classes and functions related to a very simple thread-safe queue.
    std::queue is NOT thread-safe!
*/

template <typename T>
class ts_queue
{
protected:

  std::queue<T> _q;                         ///< the unprotected queue

  mutable std::recursive_mutex _q_mutex;    ///< mutex to protect the std::queue

public:

/*! \brief              Append an element
    \param  element     the element to append
*/
  void operator+=(const T& element)
  { std::lock_guard<std::recursive_mutex> lock(_q_mutex);

    _q.push(element);
  }

/*! \brief   Pop the first element off the front of the queue
    \return  the element that was at the front of the queue, if any

    If the queue is empty, the value of the returned element is not set.
    If the queue is NOT empty, the front element is removed
*/
  std::optional<T> pop(void)
  { std::lock_guard<std::recursive_mutex> lock(_q_mutex);

    if (_q.empty())
      return { };

    T tmp { _q.front() };
  
    _q.pop();
  
    return tmp;
  }
};

#endif    // TS_QUEUE_H