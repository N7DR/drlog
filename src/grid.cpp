// $Id: grid.cpp 141 2017-12-16 21:19:10Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   grid.cpp

    Classes and functions related to grid squares
*/

#include "grid.h"
#include "log_message.h"
#include "string_functions.h"

using namespace std;

extern message_stream ost;                  ///< for debugging, info

// -------------------------  grid_square  ---------------------------

/*! \class  grid_square
    \brief  Encapsulates a grid "square"
*/

/*! \brief  Construct from a designation
    \param  Four-character designation
*/
grid_square::grid_square(const string& gs)
{ if ( is_valid_designation(gs) )
  { _designation = to_upper(gs);

// http://www.arrl.org/files/file/protected/Group/Members/Technology/tis/info/pdf/18929.pdf
// longitude
    const char c1 = _designation[0];
    const int l1 = 20 * (c1 - 'I');


//    const int n1 = c1 - 'A';
//    const int l1 = -180 + 20 * n1;

//    ost << "l1 = " << l1 << endl;

    const char c2 = _designation[2];

    int l2;

    if (l1 < 0)
    { l2 = -2 * ('9' - c2);
    }
    else
    { l2 = 2 * (c2 - '0');
    }

//    const int l2 = l1 + q;


//    const int n2 = c2 - '0';
//    const int l2_base = (l1 < 0 ? -20 : 0);

//    ost << "l2_base = " << l2_base << endl;

//    const int l2 = l2_base + 2 * n2;

//    ost << "l2 = " << l2 << endl;

    const float rounding_lon = (l1 < 0 ? -1 : 1);

    const float lon = l1 + l2 + rounding_lon;

//    ost << "lon = " << lon << endl;

    _longitude = lon;

// latitude
    const char c3 = _designation[1];
    const int n3 = c3 - 'A';
    const int l3 = -90 + 10 * n3;

//    ost << "l3 = " << l3 << endl;

    const char c4 = _designation[3];
    const int n4 = c4 - '0';
    const int l4_base = (l3 < 0 ? -10 : 0);

//    ost << "l4_base = " << l4_base << endl;

    const int l4 = l4_base + 1 * n4;

//    ost << "l4 = " << l4 << endl;

    const float rounding_lat = (l4 < 0 ? -0.5 : 0.5);

    const float lat = l3 + l4 + rounding_lat;

//    ost << "lat = " << lat << endl;

    _latitude = lat;
  }
}

const bool is_valid_designation(const string& putative_designation)
{ if (putative_designation.length() < 4)
    return false;

  if (putative_designation.length() > 6)
    return false;

  if (putative_designation.length() == 5)
    return false;

  string designation_copy = to_upper(putative_designation);

// longitude; http://www.arrl.org/files/file/protected/Group/Members/Technology/tis/info/pdf/18929.pdf

  if ( (putative_designation[0] < 'A') or (putative_designation[0] > 'R') or
       (putative_designation[1] < 'A') or (putative_designation[1] > 'R') )
    return false;

  if ( (putative_designation[2] < '0') or (putative_designation[2] > '9') or
       (putative_designation[3] < '0') or (putative_designation[3] > '9') )
    return false;

  if (putative_designation.length() == 6)
  { if ( (putative_designation[4] < 'A') or (putative_designation[4] > 'X') or
         (putative_designation[5] < 'A') or (putative_designation[5] > 'X') )
      return false;
  }

  return true;
}


#if 0
http://www.mike-willis.com/Equations/coordinates.html

MaidenheadToDeg(AnsiString locator, double &latitude, double &longitude)
{
// Converts Maidenhead to Lat Long, returns 1 on success

locator = locator.UpperCase();

if (locator.Length() == 4)
locator += "IL";

if (locator.Length() != 6 && locator.Length() != 8)
return 0;

if (locator[1] < 'A' || locator[1] > 'R' ||
locator[2] < 'A' || locator[2] > 'R' ||
locator[3] < '0' || locator[3] > '9' ||
locator[4] < '0' || locator[4] > '9' ||
locator[5] < 'A' || locator[5] > 'X' ||
locator[6] < 'A' || locator[6] > 'X')
return 0;

if (locator.Length() == 8 &&
(locator[7] <'0' || locator[7] > '9' ||
locator[8] < '0' || locator[8] > '9'))
return 0;

longitude = -180.0 + FIELD_WIDTH * (locator[1] - 'A') +
SQUARE_WIDTH * (locator[3] - '0') +
SUB_WIDTH * (locator[5] - 'A');

latitude = -90.0 + FIELD_HEIGHT * (locator[2] - 'A') +
SQUARE_HEIGHT * (locator[4] - '0') +
SUB_HEIGHT * (locator[6] - 'A');

if (locator.Length() == 8) {
longitude += SUB_SUB_WIDTH * (locator[7] - '0') + SUB_SUB_WIDTH / 2.0;
latitude += SUB_SUB_HEIGHT * (locator[8] - '0') + SUB_SUB_HEIGHT / 2.0;
} else {
longitude += SUB_SUB_WIDTH * 5.0;
latitude += SUB_SUB_HEIGHT * 5.0;
locator = locator + "55";
}
return 1;
}
#endif
