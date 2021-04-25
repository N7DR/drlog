// $Id: version.cpp 147 2018-04-20 21:32:50Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   version.cpp

    version information
*/

#include "version.h"

#include <string>

using namespace std;

const string VERSION_TYPE("γ");   ///< level of release: α, β, γ, &c.
const string DATE_STR(__DATE__);  ///< Mmm dd yyyy
const string TIME_STR(__TIME__);  ///< hh:mm:ss
