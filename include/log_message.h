// $Id: log_message.h 88 2014-12-27 15:19:42Z  $

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
  std::ofstream     _ost;                       ///< the output stream
  std::ofstream     _err;                       ///< the error stream
  
  pt_mutex          _message_stream_mutex;      ///< mutex for the stream

public:

/// constructor from a file name
message_stream(const std::string& filename, const std::string& error_name = "drlog-errors");

/// message_stream << <generic object>
template <typename T>
  message_stream& operator<<(const T obj)
  { //SAFELOCK(_message_stream);
    try
    { SAFELOCK(_message_stream);

      _ost << obj;
    }

    catch (...)
    { _err << "Error writing to stream" << std::endl;
      _err << obj;
    }

    return *this;
  }

// for manipulator (see "The C++ Standard Library", 13.6.1)
// I have no idea why this is necessary, since it seems to me that the
// (identical) generic version should work just fine
  message_stream& operator<<(std::ostream&(*obj)(std::ostream&))
  { //SAFELOCK(_message_stream);
  
    try
    { SAFELOCK(_message_stream);

      _ost << obj;
    }

    catch (...)
    { _err << "Error writing to stream" << std::endl;
      _err << obj;
    }

    return *this;
  }
};

#endif    // LOG_MESSAGE_H

