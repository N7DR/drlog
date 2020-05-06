// $Id: grid.cpp 153 2019-09-01 14:27:02Z  $

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

#include <math.h>

using namespace std;

extern message_stream ost;                  ///< for debugging, info

constexpr float FIELD_WIDTH  { 20.0f };             ///< width of a Maidenhead field, in degrees
constexpr float FIELD_HEIGHT  { 10.0f };            ///< height of a Maidenhead field, in degrees
constexpr float SQUARE_WIDTH  { 2.0f };             ///< width of a Maidenhead "square", in degrees
constexpr float SQUARE_HEIGHT { 1.0f };            ///< width of a Maidenhead "square", in degrees

// -------------------------  grid_square  ---------------------------

/*! \class  grid_square
    \brief  Encapsulates a grid "square"
*/

/*! \brief      Construct from a designation
    \param  gs  four-character designation
*/
grid_square::grid_square(const string& gs)
{ if ( is_valid_grid_designation(gs) )
  { auto round_it { [] (const float f) { return static_cast<int>(round(f)); } };

    _designation = to_upper(gs);

// http://www.arrl.org/files/file/protected/Group/Members/Technology/tis/info/pdf/18929.pdf
// longitude
    const char c1 { _designation[0] };
    const int  l1 { round_it(FIELD_WIDTH * (c1 - 'J')) };
    const char c2 { _designation[2] };
    const int l2  { round_it(SQUARE_WIDTH * (c2 - '0')) };

    const float rounding_lon { ( SQUARE_WIDTH / 2 ) };
    const float lon          { l1 + l2 + rounding_lon };

    _longitude = lon;

// latitude
    const char  c3           { _designation[1] };
    const int   n3           { c3 - 'A' };
    const int   l3           { -90 + round_it(FIELD_HEIGHT * n3) };
    const char  c4           { _designation[3] };
    const int   n4           { c4 - '0' };
    const int   l4           { round_it(SQUARE_HEIGHT * n4) };
    const float rounding_lat { ( SQUARE_HEIGHT / 2 ) };
    const float lat          { l3 + l4 + rounding_lat };

    _latitude = lat;
  }
}

/*! \brief                          Is a string a valid designation for a Maidenhead square or subsquare?
    \param  putative_designation    the putative designation
    \return                         whether <i>putative_designation</i> is a valid designation of a Maidenhead square or subsquare
*/
const bool is_valid_grid_designation(const string& putative_designation)
{ if (putative_designation.length() < 4)
    return false;

  if (putative_designation.length() > 6)
    return false;

  if (putative_designation.length() == 5)
    return false;

  const string designation_copy { to_upper(putative_designation) };

// longitude; http://www.arrl.org/files/file/protected/Group/Members/Technology/tis/info/pdf/18929.pdf

  if ( (designation_copy[0] < 'A') or (designation_copy[0] > 'R') or
       (designation_copy[1] < 'A') or (designation_copy[1] > 'R') )
    return false;

  if ( (designation_copy[2] < '0') or (designation_copy[2] > '9') or
       (designation_copy[3] < '0') or (designation_copy[3] > '9') )
    return false;

  if (designation_copy.length() == 6)
  { if ( (designation_copy[4] < 'A') or (designation_copy[4] > 'X') or
         (designation_copy[5] < 'A') or (designation_copy[5] > 'X') )
      return false;
  }

  return true;
}

