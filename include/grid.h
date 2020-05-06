// $Id: grid.h 152 2019-08-21 20:23:38Z  $

// Released under the GNU Public License, version 2

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   grid.h

    Classes and functions related to grid "squares"
*/

#ifndef GRID_H
#define GRID_H

#include "functions.h"
#include "macros.h"
#include "serialization.h"

#include <string>

// -------------------------  grid_square  ---------------------------

/*! \class  grid_square
    \brief  Encapsulates a grid "square"
*/

class grid_square
{
protected:

  std::string   _designation;       ///< the designation of the square (e.g., "DN70")
  float         _latitude;          ///< latitude of centre (°N)
  float         _longitude;         ///< longitude of centre (°W)

public:

/// default constructor
  inline grid_square(void) = default;

/*! \brief      Construct from a designation
    \param  gs  four-character designation
*/
  grid_square(const std::string& gs);

  READ(designation);                ///< the designation of the square (e.g., "DN70")
  READ(latitude);                   ///< latitude of centre (°N)
  READ(longitude);                  ///< longitude of centre (°W)

/*! \brief      Calculate distance to another grid square
    \param  gs  other grid square
    \return     distance to <i>gs</i>, in kilometres

    Distance is between the centres of the two grid squares
*/
  inline const float operator-(const grid_square& gs) const
    { return distance(_latitude, _longitude, gs._latitude, gs._longitude); }

/*! \brief      Calculate bearing to another grid square
    \param  gs  other grid square
    \return     bearing of <i>gs</i>, in degrees from *this
*/
  inline const float bearing(const grid_square& gs) const
    { return ::bearing(_latitude, _longitude, gs._latitude, gs._longitude); }
  
/// serialise
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _designation
         & _latitude
         & _longitude;
    }
};

/*! \brief                          Is a string a valid designation for a Maidenhead square or subsquare?
    \param  putative_designation    the putative designation
    \return                         whether <i>putative_designation</i> is a valid designation of a Maidenhead square or subsquare
*/
const bool is_valid_grid_designation(const std::string& putative_designation);

#endif // GRID_H
