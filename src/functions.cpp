// $Id: functions.cpp 60 2014-04-26 22:11:23Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file functions.cpp

        Functions related to geography
*/

#include "functions.h"
#include "log_message.h"
#include "string_functions.h"

#include <math.h>

extern message_stream ost;  ///< debugging/logging output

using namespace std;

/*! \brief  Obtain distance in km between two locations
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return distance between source and target, in km

    See http://www.movable-type.co.uk/scripts/latlong.html:

    a = sin²(Δφ/2) + cos(φ1).cos(φ2).sin²(Δλ/2)
    c = 2.atan2(√a, √(1−a))
    d = R.c
    where   φ is latitude, λ is longitude, R is earth’s radius (mean radius = 6,371km)

    θ = atan2( sin(Δλ).cos(φ2), cos(φ1).sin(φ2) − sin(φ1).cos(φ2).cos(Δλ) )
*/
const float distance(const float& lat1, const float& long1, const float& lat2, const float& long2)
{ static const float r = 6371;                  // radius in km
  static const float pi = 3.14159265;
  static const float dtor = pi / 180.0;

  const float delta_phi = lat2 - lat1;
  const float delta_phi_2 = delta_phi / 2;

  const float delta_lambda = long2 - long1;
  const float delta_lambda_2 = delta_lambda / 2;

  const float a = sin(delta_phi_2 * dtor) * sin(delta_phi_2 * dtor) +
                  cos(lat1 * dtor) * cos(lat2 * dtor) * sin(delta_lambda_2 * dtor) * sin(delta_lambda_2 * dtor);

  const float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  const float d = r * c;

  return d;
}

/*! \brief  Obtain bearing of target from source
    \param  lat1    latitude of source, in degrees (+ve north)
    \param  long1   longitude of source, in degrees (+ve east)
    \param  lat2    latitude of target, in degrees (+ve north)
    \param  long2   longitude of target, in degrees (+ve east)
    \return bearing of target from source, in degrees

    See http://www.movable-type.co.uk/scripts/latlong.html:

    a = sin²(Δφ/2) + cos(φ1).cos(φ2).sin²(Δλ/2)
    c = 2.atan2(√a, √(1−a))
    d = R.c
    where   φ is latitude, λ is longitude, R is earth’s radius (mean radius = 6,371km)

    θ = atan2( sin(Δλ).cos(φ2), cos(φ1).sin(φ2) − sin(φ1).cos(φ2).cos(Δλ) )
*/
const float bearing(const float& lat1, const float& long1, const float& lat2, const float& long2)
{ static const float pi = 3.14159265;
  static const float dtor = pi / 180.0;
  static const float rtod = 1.0 / dtor;

  const float lat1_rad = lat1 * dtor;
  const float lat2_rad = lat2 * dtor;
  const float delta_lambda_rad = (long2 - long1) * dtor;

  const float theta = rtod * atan2( sin(delta_lambda_rad) * cos(lat2_rad),
                                    cos(lat1_rad) * sin(lat2_rad) - sin(lat1_rad) * cos(lat2_rad) * cos(delta_lambda_rad));

  return theta;
}

/*! \brief  Calculate the time of sunrise or sunset
    \param  lat    latitude of target, in degrees (+ve north)
    \param  long   longitude of target, in degrees (+ve east)
    \param  calc_sunset whether to calculate sunset instead of sunrise
    \return sunrise or sunset in the form HH:MM

    Default is to calculate sunrise if <i>calc_sunset</i> is absent.
    See http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
*/
const string sunrise(const float& lat, const float& lon, const bool calc_sunset)
{ static const float pi = 3.14159265;
  static const float dtor = pi / 180.0;
  static const float rtod = 1.0 / dtor;

  const string date_string = date_time_string().substr(0, 10);

  const int year = from_string<int>(date_string.substr(0, 4));
  const int month = from_string<int>(date_string.substr(5, 2));
  const int day = from_string<int>(date_string.substr(8, 2));

  const float n1 = floor((275.0 * month) / 9);
  const float n2 = floor((month + 9.0) / 12);
  const float n3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
  const float n = n1 - (n2 * n3) + day - 30;

  const float longitude = lon;     // change in sign not necessary
  const float lnghour = longitude / 15;
  const float t = n + (( (calc_sunset ? 18 : 6) - lnghour) / 24);

  const float m = (0.9856 * t) - 3.289;

  float l = m + (1.916 * sin(m * dtor)) + (0.020 * sin(2 * m * dtor) ) + 282.634;

  if (l < 0)
    l += 360;
  if (l > 360)
    l -= 360;

  float ra = atan(0.91764 * tan(l * dtor)) * rtod;

  if (ra < 0)
    ra += 360;
  if (ra > 360)
    ra -= 360;

  const float lquadrant = (floor(l / 90) ) * 90;
  const float raquadrant = (floor(ra / 90) ) * 90;

  ra += (lquadrant - raquadrant);
  ra /= 15;

  const float sindec = 0.39782 * sin(l * dtor);
  const float cosdec = cos(asin(sindec));

  float cos_h = (cos (90.9 * dtor) - (sindec * sin(lat * dtor))) / (cosdec * cos(lat * dtor));

  if (cos_h > 1)
    return "9999";    // always dark

  if (cos_h < -1)
    return "8888";    // always light

  float h = (calc_sunset ? (acos(cos_h) * rtod) : (360 - acos(cos_h) * rtod));

  h /= 15;

  float big_t = h + ra - (0.06571 * t) - 6.622;
  float ut = big_t - lnghour;

  if (ut > 24)
    ut -= 24;
  if (ut < 0)
    ut += 24;

  int hrs = floor(ut);
  int mins = floor(((ut - hrs) * 60) + 0.5);

  if (mins == 60)
  { mins = 0;
    hrs++;
  }

  if (hrs >= 24)
    hrs -= 24;

  return ( pad_string(to_string(hrs), 2, PAD_LEFT, '0') + ":" + pad_string(to_string(mins), 2, PAD_LEFT, '0') );
}

