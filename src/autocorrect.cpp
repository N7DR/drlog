// $Id: autocorrect.cpp 205 2022-04-24 16:05:06Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   autocorrect.cpp

    Objects and functions related to automatically correcting calls in RBN posts
*/

#include "autocorrect.h"
#include "log_message.h"
#include "string_functions.h"

using namespace std;

extern message_stream    ost;       ///< for debugging and logging

// -----------  autocorrect_database  ----------------

/*! \class  autocorrect_database
    \brief  The database of good calls for the autocorrect function
*/

/*! \brief          Obtain an output call from an input
    \param  str     input call
    \return         <i>str</i> or a corrected version of same
*/
string autocorrect_database::corrected_call(const string& str) const
{ if (str.empty())
    return str;

// insert a mapping into the cache
  auto insert = [this] (const string& input, const string& output)
    { _cache += { input, output };

      if (output != input)
        ost << "  autocorrect: " << input << " -> " << output << endl;

      return output;
    };

// return cached value
  if (const string from_cache { MUM_VALUE(_cache, str) }; !from_cache.empty())
    return from_cache;

// return known good call
  if (contains(str))            // for now, assume that all the calls in the database are good; maybe change this later; note that this test is repeated in the tests below
    return insert(str, str);

// extraneous E in front of a US K call
//  if (str.starts_with("EK"s))
 // { if (const string call_to_test { substring(str, 1) }; contains(call_to_test))
 //     return insert(str, call_to_test);
//  }

// extraneous:
//   E in front of a US K call
//   T in front of a US K call
//   T in front of a US N call
  if (str.starts_with("EK"s) or str.starts_with("TK"s) or str.starts_with("TN"s))
  { if (const string call_to_test { substring(str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// JA miscopied as JT
  if (str.starts_with("JT"s))
  { if (!contains(str))
    { if (const string call_to_test { "JA"s + substring(str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial W copied as an initial M
  if (str.starts_with('M'))
  { if (!contains(str))
    { if (const string call_to_test { "W"s + substring(str, 1) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

// initial J copied as an initial O
  if (str.starts_with('O') and (str.size() > 3))
  { switch (str[1])
    { case 'A' :
      case 'E' :
      case 'F' :
      case 'G' :
      case 'H' :
      case 'I' :
      case 'J' :
      case 'K' :
      case 'L' :
      case 'M' :
      case 'N' :
      case 'O' :
      case 'P' :
      case 'Q' :
      case 'R' :
      case 'S' :
        
        if (const string call_to_test { "J"s + substring(str, 1) }; contains(call_to_test))
          return insert(str, call_to_test);

      default :
        break;
    }
  }

// extraneous T in front of a US N call
//  if (str.starts_with("TN"s))
//  { if (const string call_to_test { substring(str, 1) }; contains(call_to_test))
//      return insert(str, call_to_test);
//  }

// US N call copied as T#
  if (!contains(str) and str.starts_with('T') and (str.length() > 1) and isdigit(str[1]))
  { if (const string call_to_test { "N"s + substring(str, 1) }; contains(call_to_test))
      return insert(str, call_to_test);
  }

// initial PY copied as initial TM
  if (str.starts_with("TM"s))
  { if (!contains(str))
    { if (const string call_to_test { "PY"s + substring(str, 2) }; contains(call_to_test))
        return insert(str, call_to_test);
    }
  }

  return insert(str, str);
}
