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

  const string from_cache { MUM_VALUE(_cache, str) };

  if (!from_cache.empty())
    return from_cache;

  if (contains(str))            // for now, assume that all the calls in the database are good; maybe change this later; note that this test is repeated in the tests below
    return ( _cache += { str, str }, str );

// extraneous E in front of a US K call
  if (starts_with(str, "EK"s))
  { const string call_to_test { substring(str, 1) };

    if (contains(call_to_test))
      return ( _cache += { str, call_to_test }, call_to_test );
  }

// initial W copied as an initial M
  if (starts_with(str, "M"s))
  { if (!contains(str))
    { const string call_to_test { "W"s + substring(str, 1) };

      if (contains(call_to_test))
        return ( _cache += { str, call_to_test }, call_to_test );
    }
  }

// initial J copied as an initial O
  if (starts_with(str, "O"s))
  { if (starts_with(str, "OA"s))
    { if (!contains(str))
      { const string call_to_test { "JA"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OE"s))
    { if (!contains(str))
      { const string call_to_test { "JE"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

   if (starts_with(str, "OF"s))
    { if (!contains(str))
      { const string call_to_test { "JF"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OG"s))
    { if (!contains(str))
      { const string call_to_test { "JG"s + substring(str, 2) };

        if (contains(call_to_test))
          return call_to_test;
      }
    }

    if (starts_with(str, "OH"s))
    { if (!contains(str))
      { const string call_to_test { "JH"s + substring(str, 2) };

        if (contains(call_to_test))
          return call_to_test;
      }
    }

    if (starts_with(str, "OI"s))
    { if (!contains(str))
      { const string call_to_test { "JI"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OJ"s))
    { if (!contains(str))
      { const string call_to_test { "JJ"s + substring(str, 2) };

        if (contains(call_to_test))
          return call_to_test;
      }
    }

    if (starts_with(str, "OK"s))
    { if (!contains(str))
      { const string call_to_test { "JK"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OL"s))
    { if (!contains(str))
      { const string call_to_test { "JL"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OM"s))
    { if (!contains(str))
      { const string call_to_test { "JM"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "ON"s))
    { if (!contains(str))
      { const string call_to_test { "JN"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OO"s))
    { if (!contains(str))
      { const string call_to_test { "JO"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OP"s))
    { if (!contains(str))
      { const string call_to_test { "JP"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OQ"s))
    { if (!contains(str))
      { const string call_to_test { "JQ"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OR"s))
    { if (!contains(str))
      { const string call_to_test { "JR"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }

    if (starts_with(str, "OS"s))
    { if (!contains(str))
      { const string call_to_test { "JS"s + substring(str, 2) };

        if (contains(call_to_test))
          return ( _cache += { str, call_to_test }, call_to_test );
      }
    }
  }

// extraneous T in front of a US K call
  if (starts_with(str, "TK"s))
  { const string call_to_test { substring(str, 1) };

    if (contains(call_to_test))
      return ( _cache += { str, call_to_test }, call_to_test );
  }

// extraneous T in front of a US N call
  if (starts_with(str, "TN"s))
  { const string call_to_test { substring(str, 1) };

    if (contains(call_to_test))
      return ( _cache += { str, call_to_test }, call_to_test );
  }

// initial PY copied as initial TM
  if (starts_with(str, "TM"s))
  { if (!contains(str))
    { const string call_to_test { "PY"s + substring(str, 2) };

      if (contains(call_to_test))
        return ( _cache += { str, call_to_test }, call_to_test );
    }
  }

  _cache += { str, str };

  return str;
}
