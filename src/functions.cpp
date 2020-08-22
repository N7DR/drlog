// $Id: functions.cpp 165 2020-08-22 16:19:06Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   functions.cpp

    Functions related to geography
*/

#include "functions.h"
#include "log_message.h"
#include "string_functions.h"

#include <math.h>

extern message_stream ost;  ///< debugging/logging output

using namespace std;

constexpr float PI   { 3.14159265f };
constexpr float R    { 6371.0f };                  // radius in km

constexpr float DTOR { PI / 180.0f };
constexpr float RTOD { 1.0f / DTOR };

/*! \brief          Obtain distance in km between two locations
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return         distance between source and target, in km

    See http://www.movable-type.co.uk/scripts/latlong.html:

    a = sin²(Δφ/2) + cos(φ1).cos(φ2).sin²(Δλ/2)
    c = 2.atan2(√a, √(1−a))
    d = R.c
    where   φ is latitude, λ is longitude, R is earth’s radius (mean radius = 6,371km)

    θ = atan2( sin(Δλ).cos(φ2), cos(φ1).sin(φ2) − sin(φ1).cos(φ2).cos(Δλ) )
*/
float distance(const float& lat1, const float& long1, const float& lat2, const float& long2)
{ const float delta_phi   { lat2 - lat1 };
  const float delta_phi_2 { delta_phi / 2 };

  const float delta_lambda   { long2 - long1 };
  const float delta_lambda_2 { delta_lambda / 2 };

  const float a { sin(delta_phi_2 * DTOR) * sin(delta_phi_2 * DTOR) +
                  cos(lat1 * DTOR) * cos(lat2 * DTOR) * sin(delta_lambda_2 * DTOR) * sin(delta_lambda_2 * DTOR) };

  const float c { 2 * atan2(sqrt(a), sqrt(1 - a)) };
  const float d { R * c };

  return d;
}

/*! \brief          Obtain bearing of target from source
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return         bearing of target from source, in degrees

    See http://www.movable-type.co.uk/scripts/latlong.html:

    a = sin²(Δφ/2) + cos(φ1).cos(φ2).sin²(Δλ/2)
    c = 2.atan2(√a, √(1−a))
    d = R.c
    where   φ is latitude, λ is longitude, R is earth’s radius (mean radius = 6,371km)

    θ = atan2( sin(Δλ).cos(φ2), cos(φ1).sin(φ2) − sin(φ1).cos(φ2).cos(Δλ) )
*/
float bearing(const float& lat1, const float& long1, const float& lat2, const float& long2)
{ const float lat1_rad         { lat1 * DTOR };
  const float lat2_rad         { lat2 * DTOR };
  const float delta_lambda_rad { (long2 - long1) * DTOR };

  const float theta { RTOD * atan2( sin(delta_lambda_rad) * cos(lat2_rad),
                                    cos(lat1_rad) * sin(lat2_rad) - sin(lat1_rad) * cos(lat2_rad) * cos(delta_lambda_rad)) };

  return theta;
}

/*! \brief                  Calculate the time of sunrise or sunset
    \param  lat             latitude of target, in degrees (+ve north)
    \param  lon             longitude of target, in degrees (+ve east)
    \param  calc_sunset     whether to calculate sunset instead of sunrise
    \return                 sunrise or sunset in the form HH:MM

    See http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
    If there is no sunset or sunrise today, returns "DARK" or "LIGHT", according to whether is currently night
    or day at the given location
*/
//string sunrise_or_sunset(const float& lat, const float& lon, const bool calc_sunset)
string sunrise_or_sunset(const float& lat, const float& lon, const SRSS srss)
{ const string date_string { date_time_string().substr(0, 10) };

  const int year  { from_string<int>(date_string.substr(0, 4)) };
  const int month { from_string<int>(date_string.substr(5, 2)) };
  const int day   { from_string<int>(date_string.substr(8, 2)) };

  const float n1 { ffloor((275.0 * month) / 9) };
  const float n2 { ffloor((month + 9.0) / 12) };
  const float n3 { (1 + ffloor((year - 4 * floor(year / 4) + 2) / 3)) };
  const float n  { n1 - (n2 * n3) + day - 30 };

  const float longitude { lon };     // change in sign not necessary
  const float lnghour   { longitude / 15 };
 // const float t         { n + (( (calc_sunset ? 18 : 6) - lnghour) / 24) };
  const float t         { n + (( ((srss == SRSS::SUNSET) ? 18 : 6) - lnghour) / 24) };

  const float m { (0.9856f * t) - 3.289f };

  float l { static_cast<float>(m + (1.916 * sin(m * DTOR)) + (0.020 * sin(2 * m * DTOR) ) + 282.634) };

  if (l < 0)
    l += 360;
  if (l >= 360)
    l -= 360;

  float ra { static_cast<float>(atan(0.91764 * tan(l * DTOR)) * RTOD) };

  if (ra < 0)
    ra += 360;
  if (ra >= 360)
    ra -= 360;

  const float lquadrant  { (floor(l / 90) ) * 90 };
  const float raquadrant { (floor(ra / 90) ) * 90 };

  ra += (lquadrant - raquadrant);
  ra /= 15;

  const float sindec { 0.39782f * sin(l * DTOR) };
  const float cosdec { cos(asin(sindec)) };

  float cos_h { static_cast<float>((cos (90.9 * DTOR) - (sindec * sin(lat * DTOR))) / (cosdec * cos(lat * DTOR))) };

//  ost << "cos_h = " << cos_h << endl;

// 171128 swap DARK and LIGHT
// 190528 swap back: OX3XR in CQ WPX CW was returning DARK
  if (cos_h > 1)
    return "DARK"s;      // always dark

  if (cos_h < -1)
    return "LIGHT"s;     // always light

//  float h { (calc_sunset ? (acos(cos_h) * RTOD) : (360 - acos(cos_h) * RTOD)) };
  float h { (srss == SRSS::SUNSET ? (acos(cos_h) * RTOD) : (360 - acos(cos_h) * RTOD)) };

  h /= 15;

  const float big_t { h + ra - (0.06571f * t) - 6.622f };

  float ut    { big_t - lnghour };

  if (ut > 24)
    ut -= 24;
  if (ut < 0)
    ut += 24;

  int hrs  { ifloor(ut) };
  int mins { ifloor(((ut - hrs) * 60) + 0.5) };

  if (mins == 60)
  { mins = 0;
    hrs++;
  }

  if (hrs >= 24)
    hrs -= 24;

  return ( pad_string(to_string(hrs), 2, PAD_LEFT, '0') + ":"s + pad_string(to_string(mins), 2, PAD_LEFT, '0') );
}
