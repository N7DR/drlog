// $Id: log_message.cpp 64 2014-05-31 21:25:48Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file log_message.cpp

        Classes and functions related to log messages
*/

#include "diskfile.h"
#include "log_message.h"

using namespace std;

// constructor from a file name
message_stream::message_stream(const std::string& filename) //:
  //_ost(filename.c_str())
{ if (file_exists(filename))
  { int index = 0;

    string target = filename + "-" + to_string(index);

    while (file_exists(target))
      target = filename + "-" + to_string(++index);

    file_rename(filename, target);
  }

  _ost.open(filename);
}




