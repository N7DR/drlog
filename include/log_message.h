// $Id: log_message.h 14 2013-02-23 23:55:42Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

/*!     \file log_message.h

        Classes and functions related to log messages
*/

#include "pthread_support.h"

#include <fstream>
#include <iostream>
#include <string>

// -----------  message_stream  ----------------

/*!     \class message_stream
        \brief A message stream to be written to a file
*/

class message_stream
{
protected:
  std::ofstream     _ost;
  
  pt_mutex          _message_stream_mutex;

public:

// constructor from a file name
  explicit message_stream(const std::string& filename);

template <typename T>
  message_stream& operator<<(T obj)
  { SAFELOCK(_message_stream);
  
    _ost << obj;
    return *this;
  }

//template <typename T>
//  message_stream& operator<<(T& obj)
//  { SAFELOCK(_message_stream);
//
//    _ost << obj;
//    return *this;
//  }

// for manipulator (see "The C++ Standard Library", 13.6.1)
// I have no idea why this is necessary, since it seems to me that the
// (identical) generic version should work just fine
  message_stream& operator<<(std::ostream&(*obj)(std::ostream&))
  { SAFELOCK(_message_stream);
  
    _ost << obj;
    return *this;
  }
};


#endif    // LOG_MESSAGE_H

