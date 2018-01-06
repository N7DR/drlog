// $Id: grid.h 142 2018-01-01 20:56:52Z  $

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
  float         _latitude;          ///< latitude (°N)
  float         _longitude;         ///< longitude (°W)

public:

/// default constructor
  grid_square(void)
    { }

/*! \brief  Construct from a designation
    \param  Four-character designation
*/
  grid_square(const std::string& gs);

  READ(designation);                ///< the designation of the square (e.g., "DN70")
  READ(latitude);                   ///< latitude (°N)
  READ(longitude);                  ///< longitude (°W)

  inline const float operator-(const grid_square& gs) const
    { return distance(_latitude, _longitude, gs._latitude, gs._longitude); }

/// serialize with boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { ar & _designation
         & _latitude
         & _longitude;
    }
};

const bool is_valid_designation(const std::string& putative_designation);

#endif // GRID_H
