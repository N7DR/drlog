// $Id$

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   internals.h

    Objects and functions related to GNU internals
*/

#ifndef INTERNALS_H
#define INTERNALS_H

#include <string>
#include <vector>

#include <execinfo.h>

// see https://www.gnu.org/software/libc/manual/html_node/Backtraces.html

// ------------------------------------  gcc_backtrace  ----------------------------------

/*! \class  gcc_backtrace
    \brief  Encapsulate and manage a GNU GCC backtrace
*/

class gcc_backtrace
{
protected:

  std::vector<std::string> _backtrace { };

  int _max_size { 10 };         ///< maximum number of elements in the trace

public:

  explicit gcc_backtrace(const bool acquire = false, const int max_sz = 10);

};

#endif    // INTERNALS_H
