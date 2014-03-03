/*
 * functions.h
 *
 *  Created on: Dec 11, 2012
 *      Author: n7dr
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

// obtain bearing, given two locations
// http://www.movable-type.co.uk/scripts/latlong.html
const float bearing(const float& lat1, const float& long1, const float& lat2, const float& long2);
const float distance(const float& lat1, const float& long1, const float& lat2, const float& long2);
const std::string sunrise(const float& lat, const float& lon, const bool calc_sunset = false);
inline const std::string sunset(const float& lat, const float& lon)
  { return sunrise(lat, lon, true); }

#endif /* FUNCTIONS_H_ */
