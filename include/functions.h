// $Id: functions.h 88 2014-12-27 15:19:42Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/*!     \file functions.h

        Functions related to geography
*/

#include <string>

/*! \brief          Obtain bearing of target from source
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return         bearing of target from source, in degrees

    See http://www.movable-type.co.uk/scripts/latlong.html
*/
const float bearing(const float& lat1, const float& long1, const float& lat2, const float& long2);

/*! \brief          Obtain distance in km between two locations
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return         distance between source and target, in km

    See http://www.movable-type.co.uk/scripts/latlong.html:
*/
const float distance(const float& lat1, const float& long1, const float& lat2, const float& long2);

/*! \brief              Calculate the time of sunrise or sunset
    \param  lat         latitude of target, in degrees (+ve north)
    \param  long        longitude of target, in degrees (+ve east)
    \param  calc_sunset whether to calculate sunset instead of sunrise
    \return             sunrise or sunset in the form HH:MM

    Default is to calculate sunrise if <i>calc_sunset</i> is absent.
    See http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
*/
const std::string sunrise(const float& lat, const float& lon, const bool calc_sunset = false);

/*! \brief          Calculate the time of sunset
    \param  lat     latitude of target, in degrees (+ve north)
    \param  long    longitude of target, in degrees (+ve east)
    \return         sunset in the form HH:MM
*/
inline const std::string sunset(const float& lat, const float& lon)
  { return sunrise(lat, lon, true); }

#endif /* FUNCTIONS_H_ */
