// $Id: log_message.cpp 166 2020-08-22 20:59:30Z  $

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
message_stream::message_stream(const string& filename, const string& error_name) :
  _message_stream_mutex("MESSAGE STREAM"s)
{ if (file_exists(filename))
  { int    index  { 0 };
    string target { filename + "-"s + to_string(index) };

    while (file_exists(target))
      target = filename + "-"s + to_string(++index);

    file_rename(filename, target);
  }

  _ost.open(filename);
  _err.open(error_name);

// we need to redirect stderr so that any messages sent there by libraries (ALSA, I'm looking at you)
// don't show up on the screen. Direct them error file defined in the message_stream object.
// there is a function -- int snd_lib_error_set_handler (snd_lib_error_handler_t handler) -- that
// can change the ALSA error handler, but it requires a function with the signature 
// typedef void(* snd_lib_error_handler_t) (const char *file, int line, const char *function, int err, const char *fmt,...) 
// and I don't know how easily to do this since it's a variadic C function.	

// so instead we'll just assume that the ALSA default writes to stderr, and will redirect that here

// https://stackoverflow.com/questions/998162/is-it-possible-to-disable-stderr-in-c
  freopen("stderr-output", "w", stderr);
}
