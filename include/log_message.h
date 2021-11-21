// $Id: log_message.h 196 2021-11-14 21:39:45Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

/*! \file   log_message.h

    Classes and functions related to log messages
*/

#include "pthread_support.h"

#include <fstream>
#include <iostream>
#include <string>

// -----------  message_stream  ----------------

/*! \class  message_stream
    \brief  A message stream to be written to a file
*/

class message_stream
{
protected:

  std::ofstream _ost;                       ///< the output stream
  std::ofstream _err;                       ///< the error stream
  
  pt_mutex      _message_stream_mutex;      ///< mutex for the stream

public:

/*! \brief              Constructor
    \param  filename    name of file to which output is to be written
    \param  error_name  name of file to which errors in message_stream operation are to be written

    The file <i>error_name</i> is used if a failure is detected when writing to <i>filename</i>.
    An extant file called <i>filename</i> is renamed, not overwritten
*/
  explicit message_stream(const std::string& filename, const std::string& error_name = "drlog-errors"s);

/*! \brief          Write a generic object to a <i>message_stream</i> object
    \param  obj     object to write
    \return         the <i>message_stream</i> object
*/
  template <typename T>
  message_stream& operator<<(const T obj)
  {
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

/*! \brief          Write a manipulator to a <i>message_stream</i> object (see "The C++ Standard Library", 13.6.1)
    \param  obj     manipulator to write
    \return         the <i>message_stream</i> object

    I have no idea why this is necessary, since it seems to me that the
    (identical) generic version should work just fine (since surely manipulators are constant?)
*/
  message_stream& operator<<(std::ostream&(*obj)(std::ostream&))
  {
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
