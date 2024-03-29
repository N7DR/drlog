// $Id: functions.h 189 2021-08-16 00:34:00Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   functions.h

    Functions related to geography
*/

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

constexpr float KM_PER_MILE { 1.609344f };                  ///< number of kilometres in a mile

/*! \brief              Convert miles to kilometres
    \param  dx_miles    distance in miles
    \return             <i>dx_miles</i> in kilometres
*/
inline float miles_to_kilometres(const float dx_miles)
  { return (dx_miles * KM_PER_MILE); }

/*! \brief          Convert kilometres to miles
    \param  dx_km   distance in kilometres
    \return         <i>dx_km</i> in miles
*/
inline float kilometres_to_miles(const float dx_km)
  { return (dx_km / KM_PER_MILE); }

/*! \brief          Obtain bearing of target from source
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return         bearing of target from source, in degrees

    See http://www.movable-type.co.uk/scripts/latlong.html
*/
float bearing(const float& lat1, const float& long1, const float& lat2, const float& long2);

/*! \brief          Obtain distance in km between two locations
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return         distance between source and target, in km

    See http://www.movable-type.co.uk/scripts/latlong.html:
*/
float distance(const float& lat1, const float& long1, const float& lat2, const float& long2);

enum class SRSS { SUNRISE,
                  SUNSET
                };

/*! \brief                  Calculate the time of sunrise or sunset
    \param  lat             latitude of target, in degrees (+ve north)
    \param  lon             longitude of target, in degrees (+ve east)
    \param  calc_sunset     whether to calculate sunset instead of sunrise
    \return                 sunrise or sunset in the form HH:MM

    See http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
    If there is no sunset or sunrise today, returns "DARK" or "LIGHT", according to whether is currently night
    or day at the given location
*/
std::string sunrise_or_sunset(const float& lat, const float& lon, const SRSS srss);

/*! \brief          Calculate the time of sunrise
    \param  lat     latitude of target, in degrees (+ve north)
    \param  lon     longitude of target, in degrees (+ve east)
    \return         sunrise in the form HH:MM

    If there is no sunrise today, returns "DARK" or "LIGHT", according to whether is currently night
    or day at the given location
*/
inline std::string sunrise(const float& lat, const float& lon)
  { return sunrise_or_sunset(lat, lon, SRSS::SUNRISE); }

/*! \brief          Calculate the time of sunset
    \param  lat     latitude of target, in degrees (+ve north)
    \param  lon     longitude of target, in degrees (+ve east)
    \return         sunset in the form HH:MM

    If there is no sunset today, returns "DARK" or "LIGHT", according to whether is currently night
    or day at the given location
*/
inline std::string sunset(const float& lat, const float& lon)
  { return sunrise_or_sunset(lat, lon, SRSS::SUNSET); }

#endif /* FUNCTIONS_H_ */
