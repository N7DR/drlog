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
#include "log_message.h"
#include "string_functions.h"

using namespace std;

extern message_stream    ost;       ///< for debugging and logging

// -----------  autocorrect_database  ----------------

/*! \class  autocorrect_database
    \brief  The database of good calls for the autocorrect function
*/

string autocorrect_database::corrected_call(const string& str) const
{ if (str.empty())
    return str;

  auto insert = [this] (const string& input, const string& output)
    { _cache += { input, output };

      if (output != input)
        ost << "  autocorrect: " << input << " -> " << output << endl;

      return output;
    };

  if (const string from_cache { MUM_VALUE(_cache, str) }; !from_cache.empty())
    return from_cache;

  if (contains(str))            // for now, assume that all the calls in the database are good; maybe change this later; note that this test is repeated in the tests below
    return insert(str, str);
//    return ( _cache += { str, str }, str );

// extraneous E in front of a US K call
  if (starts_with(str, "EK"s))
  { //const string call_to_test { substring(str, 1) };

    if (const string call_to_test { substring(str, 1) }; contains(call_to_test))
//      return ( _cache += { str, call_to_test }, call_to_test );
      return insert(str, call_to_test);
  }

// JA miscopied as JT
  if (starts_with(str, "JT"s))
  { if (!contains(str))
    { //const string call_to_test { "JA"s + substring(str, 2) };

      if (const string call_to_test { "JA"s + substring(str, 2) }; contains(call_to_test))
//        return ( _cache += { str, call_to_test }, call_to_test );
        return insert(str, call_to_test);
    }
  }

// initial W copied as an initial M
  if (starts_with(str, "M"s))
  { if (!contains(str))
    { //const string call_to_test { "W"s + substring(str, 1) };

      if (const string call_to_test { "W"s + substring(str, 1) }; contains(call_to_test))
//        return ( _cache += { str, call_to_test }, call_to_test );
        return insert(str, call_to_test);
    }
  }

// initial J copied as an initial O
  if (starts_with(str, "O"s) and (str.size() > 3))
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
 //         return ( _cache += { str, call_to_test }, call_to_test );
          return insert(str, call_to_test);

      default :
        break;
    }
  }

// extraneous T in front of a US K call
  if (starts_with(str, "TK"s))
  { const string call_to_test { substring(str, 1) };

    if (contains(call_to_test))
      //return ( _cache += { str, call_to_test }, call_to_test );
      return insert(str, call_to_test);
  }

// extraneous T in front of a US N call
  if (starts_with(str, "TN"s))
  { const string call_to_test { substring(str, 1) };

    if (contains(call_to_test))
//      return ( _cache += { str, call_to_test }, call_to_test );
      return insert(str, call_to_test);
  }

// initial PY copied as initial TM
  if (starts_with(str, "TM"s))
  { if (!contains(str))
    { const string call_to_test { "PY"s + substring(str, 2) };

      if (contains(call_to_test))
//        return ( _cache += { str, call_to_test }, call_to_test );
        return insert(str, call_to_test);
    }
  }

//  _cache += { str, str };

//  return str;
  return insert(str, str);
}
