// $Id: log_message.cpp 153 2019-09-01 14:27:02Z  $

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

/*! \brief  Constructor
    \param  filename    name of file to which output is to be written
    \param  error_name  name of file to which errors in message_stream operation are to be written

    The file <i>error_name</i> is used if a failure is detected when writing to <i>filename</i>.
    An extant file called <i>filename</i> is renamed, not overwritten
*/
message_stream::message_stream(const string& filename, const string& error_name)
{ if (file_exists(filename))
  { int    index  { 0 };
    string target { filename + "-"s + to_string(index) };

    while (file_exists(target))
      target = filename + "-"s + to_string(++index);

    file_rename(filename, target);
  }

  _ost.open(filename);
  _err.open(error_name);
}
