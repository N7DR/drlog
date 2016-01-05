// $Id: parallel_port.cpp 112 2015-07-26 17:04:33Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file parallel_port.cpp

        Classes and functions related to controlling a parallel port
        Uses the libieee1284 library
*/


#include "log_message.h"
#include "parallel_port.h"

#include <iostream>

#include <errno.h>

using namespace std;

extern message_stream ost;                                  ///< for debugging, info

// ---------------------------------------- parallel_port -------------------------

/*!     \class parallel_port
        \brief Access and control a port
*/

/*! \brief              Open a port
    \param  filename    name of the port to open
*/
parallel_port::parallel_port(const string& filename)
{ int status = ieee1284_find_ports(&_list_from_library, 0);

  if (status != E1284_OK)
    throw parallel_port_error(PARALLEL_PORT_UNABLE_TO_LIST, "Unable to list parallel ports.");

  const unsigned int n_ports = _list_from_library.portc;

  int capabilities;
  bool found_match = false;

  for (unsigned int n = 0; !found_match and n < n_ports; ++n)
  { if (static_cast<string>(_list_from_library.portv[n]->filename) == filename)
    { const int status = ieee1284_open(_list_from_library.portv[n], 0, &capabilities);

      if (status == E1284_INIT)
        ost << "Error opening parallel port: E1284_INIT" << endl;

      if (status == E1284_NOMEM)
        ost << "Error opening parallel port: E1284_NOMEM" << endl;

      if (status == E1284_NOTAVAIL)
        ost << "Error opening parallel port: E1284_NOTAVAIL" << endl;

      if (status == E1284_INVALIDPORT)
        ost << "Error opening parallel port: E1284_INVALIDPORT" << endl;

      if (status == E1284_SYS)
        ost << "Error opening parallel port: E1284_SYS" << endl;

      if (status != E1284_OK)
        throw parallel_port_error(PARALLEL_PORT_MISC_ERROR, "Error trying to open parallel port " + filename + ".");

      found_match = true;
      _port_nr = n;
    }
  }

  if (!found_match)
    throw parallel_port_error(PARALLEL_PORT_NO_SUCH_PORT, "Parallel port " + filename + " does not exist on this system");

  status = ieee1284_claim(_list_from_library.portv[_port_nr]);

  if (status == E1284_NOMEM)
    ost << "Error claiming parallel port: E1284_NOMEM" << endl;

  if (status == E1284_INVALIDPORT)
    ost << "Error claiming parallel port: E1284_INVALIDPORT" << endl;

  if (status == E1284_SYS)
    ost << "Error claiming parallel port: E1284_SYS" << endl;

  if (status != E1284_OK)
    throw parallel_port_error(PARALLEL_PORT_UNABLE_TO_CLAIM, "Cannot claim parallel port " + filename + ".");
}

/// destructor -- closes the port
parallel_port::~parallel_port(void)
{ ieee1284_free_ports(&_list_from_library);
}

/*! \brief                  Set control lines
    \param  char_to_assert  bit pattern to assert
*/
//void parallel_port::control(const char c) const
//{ ieee1284_write_control(_list_from_library.portv[_port_nr], c);
//}
