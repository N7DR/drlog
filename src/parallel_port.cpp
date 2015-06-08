// $Id: parallel_port.cpp 106 2015-06-06 16:11:23Z  $

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

extern message_stream ost;

// ---------------------------------------- parallel_port -------------------------

/*!     \class parallel_port
        \brief Access and control a port
*/

/*! \brief  Open a port
    \param  filename    name of the port to open
*/
parallel_port::parallel_port(const string& filename)
{ //struct parport_list   list_from_library;     ///< list of parallel ports, from ieee1284 library
  int status = ieee1284_find_ports(&_list_from_library, 0);

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

/* man libieee1284 says:

  The normal sequence of events will be that the application
   1.  calls ieee1284_find_ports to get a list of available ports
   2.  then ieee1284_get_deviceid to look for a device on each port that it is interested in
   3.  and then ieee1284_open to open each port it finds a device it can control on.
   4.  The list of ports returned from ieee1284_find_ports can now be disposed of using ieee1284_free_ports.
   5.  Then when it wants to control the device, it will call ieee1284_claim to prevent other drivers from using the port
   6.  then perhaps do some data transfers
   7.  and then ieee1284_release when it is finished that that particular command. This claim-control-release sequence will be repeated each time it wants to tell the device to do something.
   8.  Finally when the application is finished with the device it will call ieee1284_close.

   We don't perform the get_deviceid, because it makes no sense to do so.
*/

  status = ieee1284_claim(_list_from_library.portv[_port_nr]);

  if (status == E1284_NOMEM)
    ost << "Error claiming parallel port: E1284_NOMEM" << endl;

  if (status == E1284_INVALIDPORT)
    ost << "Error claiming parallel port: E1284_INVALIDPORT" << endl;

  if (status == E1284_SYS)
    ost << "Error claiming parallel port: E1284_SYS" << endl;

  if (status != E1284_OK)
    throw parallel_port_error(PARALLEL_PORT_UNABLE_TO_CLAIM, "Cannot claim parallel port " + filename + ".");

//  ieee1284_free_ports(&list_from_library);
}

/// destructor -- closes the port
parallel_port::~parallel_port(void)
{ ieee1284_free_ports(&_list_from_library);
}

/*! \brief  Set control lines
    \param  char_to_assert  bit pattern to assert
*/
void parallel_port::control(const char c) const
{ ieee1284_write_control(_list_from_library.portv[_port_nr], c);
}

const int parallel_port::status(void) const
{ int rv = ieee1284_read_status(_list_from_library.portv[_port_nr]);

  return rv;
}

//  There are five status lines, one of which is usually inverted on PC-style ports. Where they differ, libieee1284
//  operates on the IEEE 1284 values, not the PC-style inverted values. The status lines are represented by the
//  following enumeration:
//  enum ieee1284_status_bits
//  {
//  S1284_NFAULT = 0x08,
//  S1284_SELECT = 0x10,
//  S1284_PERROR = 0x20,
//  S1284_NACK = 0x40,
//  S1284_BUSY = 0x80,
//  /* To convert those values into PC-style register values, use this: */
//  S1284_INVERTED = S1284_BUSY,
//  };

const string parallel_port::status_string(void) const
{ const int status = this->status();
  string rv;

  if (status >= 0)
  { if (status bitand S1284_NFAULT)
      rv += " FAULT";

    if (status bitand S1284_NFAULT)
      rv += " SELECT";

    if (status bitand S1284_PERROR)
      rv += " ERROR";

    if (status bitand S1284_NACK)
      rv += " NACK";

    if (status bitand S1284_BUSY)
      rv += " BUSY";

    return (rv.empty() ? rv : rv.substr(1));    // remove the initial space
  }

/*
  Possible error codes:

   E1284_NOTIMPL
       The port lacks the required capability. This could be due to a limitation of this version of libieee1284, or a hardware limitation.

   E1284_NOTAVAIL
       Access to the status lines is not available on this port type.

   E1284_TIMEDOUT
       The timeout has elapsed.

   E1284_INVALIDPORT
       The port parameter is invalid (for instance, perhaps the port is not claimed).
*/

  switch (status)
  { case E1284_NOTIMPL :
      return "E1284_NOTIMPL error";

    case E1284_NOTAVAIL :
      return "E1284_NOTAVAIL error";

    case E1284_TIMEDOUT :
      return "E1284_TIMEDOUT error";

    case E1284_INVALIDPORT :
      return "E1284_INVALIDPORT error";

    default:
      return ( string("Unknown status error: ") + to_string(status) );
  }
}

//
//There are four control lines, three of which are usually inverted on PC-style ports. Where they differ, libieee1284 operates on the IEEE 1284 values, not the PC-style inverted values. The control lines are represented by the
//following enumeration:
//
//    enum ieee1284_control_bits
//    {
//      C1284_NSTROBE   = 0x01,
//      C1284_NAUTOFD   = 0x02,
//      C1284_NINIT     = 0x04,
//      C1284_NSELECTIN = 0x08,
//      /* To convert those values into PC-style register values, use this: */
//      C1284_INVERTED = (C1284_NSTROBE|
//                        C1284_NAUTOFD|
//                        C1284_NSELECTIN),
//    };




const int parallel_port::control_status(void) const
{ const int rv = ieee1284_read_control(_list_from_library.portv[_port_nr]);

  return rv;
}

const string parallel_port::control_status_string(void) const
{ const int status = this->control_status();
  string rv;

  if (status >= 0)
  { if (status bitand C1284_NSTROBE)
      rv += " STROBE";

    if (status bitand C1284_NAUTOFD)
      rv += " AUTOFD";

    if (status bitand C1284_NINIT)
      rv += " INIT";

    if (status bitand C1284_NSELECTIN)
      rv += " SELECTIN";

    return (rv.empty() ? rv : rv.substr(1));    // remove the initial space
  }

/*
  Possible error codes for ieee1284_read_control:

  E1284_NOTAVAIL
      The control lines of this port are not accessible by the application.

  E1284_INVALIDPORT
      The port parameter is invalid (for instance, perhaps it is not claimed).

*/

  switch (status)
  { case E1284_NOTAVAIL :
      return "E1284_NOTAVAIL error";

    case E1284_INVALIDPORT :
      return "E1284_INVALIDPORT error";

    default:
      return ( string("Unknown control_status error: ") + to_string(status) );
  }
}

