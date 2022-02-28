// $Id$

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   autocorrect.cpp

    Objects and functions related to automatically correcting calls in RBN posts
*/

#include "autocorrect.h"
#include "string_functions.h"

using namespace std;

// -----------  autocorrect_database  ----------------

/*! \class  autocorrect_database
    \brief  The database of good calls for the autocorrect function
*/

string autocorrect_database::corrected_call(const string& str) const
{ if (str.empty())
    return str;

  if (contains(str))            // for now, assume that all the calls in the database are good; maybe change this later
    return str;

// extraneous E in front of a US K call
  if (starts_with(str, "EK"s))
  { const string call_to_test { substring(str, 1) };

    if (contains(call_to_test))
      return call_to_test;
  }

// initial W copied as an initial M
  if (starts_with(str, "M"s))
  { if (!contains(str))
    { const string call_to_test { "W"s + substring(str, 1) };

      if (contains(call_to_test))
        return call_to_test;
    }
  }

// initial PY copied as initial TM
  if (starts_with(str, "TM"s))
  { if (!contains(str))
    { const string call_to_test { "PY"s + substring(str, 2) };

      if (contains(call_to_test))
        return call_to_test;
    }
  }

  return str;
}
