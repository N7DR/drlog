// $Id: parallel_port.h 73 2014-08-30 14:44:01Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file parallel_port.h

        Classes and functions related to controlling a parallel port
        Uses the libieee1284 library
*/

#ifndef PARALLEL_PORT_H
#define PARALLEL_PORT_H

#include "macros.h"
#include "x_error.h"

#include <string>
#include <vector>

#include <ieee1284.h>

// error numbers
const int PARALLEL_PORT_NO_SUCH_PORT            = -1,    // port does not exist
          PARALLEL_PORT_MISC_ERROR              = -2,    // misc. error
          PARALLEL_PORT_UNABLE_TO_CLAIM         = -3,    // can't claim the port
          PARALLEL_PORT_UNABLE_TO_LIST          = -4;    // can't list the ports

/* From https://secure.wikimedia.org/wikipedia/en/wiki/Parallel_port:


Pin No (DB25)   Pin No (36 pin)     Signal name     Direction   Register - bit  Inverted
1               1                   Strobe          In/Out      Control-0       Yes
2               2                   Data0           Out         Data-0          No
3               3                   Data1           Out         Data-1          No
4               4                   Data2           Out         Data-2          No
5               5                   Data3           Out         Data-3          No
6               6                   Data4           Out         Data-4          No
7               7                   Data5           Out         Data-5          No
8               8                   Data6           Out         Data-6          No
9               9                   Data7           Out         Data-7          No
10              10                  Ack             In          Status-6        No
11              11                  Busy            In          Status-7        Yes
12              12                  Paper-Out       In          Status-5        No
13              13                  Select          In          Status-4        No
14              14                  Linefeed        In/Out      Control-1       Yes
15              32                  Error           In          Status-3        No
16              31                  Reset           In/Out      Control-2       No
17              36                  Select-Printer  In/Out      Control-3       Yes
18-25           19-30,33,17,16      Ground          -           -               -

*/

// PTT = DB25 16
// CW  = DB25 17


/*
  enum ieee1284_control_bits
{
  C1284_NSTROBE   = 0x01,
  C1284_NAUTOFD   = 0x02,
  C1284_NINIT     = 0x04,  == PTT
  C1284_NSELECTIN = 0x08,  == CW
  // To convert those values into PC-style register values, use this:
  C1284_INVERTED = (C1284_NSTROBE|
                    C1284_NAUTOFD|
                    C1284_NSELECTIN),
};
*/



/*
  struct parport {
  // An arbitrary name for the port.
  const char *name;

  // The base address of the port, if that has any meaning, or zero.
  unsigned long base_addr;

  // The ECR address of the port, if that has any meaning, or zero.
  unsigned long hibase_addr;

  // The filename associated with this port, if that has any meaning, or NULL.
  const char *filename;
};

  It turns out that one can't wrap a parport, because there's a void*
  that the user isn't supposed to use, but which seems to have to be carried along
  in order to maintain internal consistency... basically, once one gets a parport back
  from ieee1284_find_ports() one has to use it as-is; one can't copy the information
  and use it later (e.g., to open the port), after deleting the parport structure that was returned.

  With typical lack of helpfulness, none of this seems to be documented. Indeed, the very
  existence of the private void* which is the source of the problem isn't even mentioned
  on the parport man page.
*/

// ---------------------------------------- parallel_port -------------------------

/*!     \class parallel_port
        \brief Access and control a port
*/

class parallel_port
{
protected:
  struct parport_list   _list_from_library;     ///< list of parallel ports, from ieee1284 library
  int                   _port_nr;               ///< number of this port

public:

/*! \brief  Open a port
    \param  filename    name of the port to open
*/
  parallel_port(const std::string& filename);

/// destructor -- closes the port
  virtual ~parallel_port(void);

/*! \brief  Set control lines
    \param  char_to_assert  bit pattern to assert
*/
  void control(const char char_to_assert) const;
};

// -------------------------------------- Errors  -----------------------------------

/*! \class  parallel_port_error
    \brief  Errors related to parallel port processing
*/

class parallel_port_error : public x_error
{
protected:

public:

/*! \brief  Construct from error code and reason
  \param  n Error code
  \param  s Reason
*/
  inline parallel_port_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};


#endif    // PARALLEL_PORT_H
