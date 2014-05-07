// $Id: log_message.cpp 3 2012-12-30 00:02:28Z  $

/*!     \file log_message.cpp

        Classes and functions related to log messages
*/

#include "log_message.h"

using namespace std;

// constructor from a file name
message_stream::message_stream(const std::string& filename) :
  _ost(filename.c_str())
{
}




