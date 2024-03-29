// $Id: rate.cpp 187 2021-06-26 16:16:42Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file rate.cpp

    Classes and functions related to the calculation of rates
*/

#include "rate.h"
#include "string_functions.h"

using namespace std;

extern message_stream    ost;                ///< debugging/logging output

/*! \brief      Return the number of QSOs and points at the current epoch
    \return     pair.first is the number of QSOs; pair.second is the number of points
*/
rate_meter::PAIR_NQSOS_POINTS rate_meter::current_qsos_and_score(void) const
{ SAFELOCK(_rate);

  return _data.empty() ? PAIR_NQSOS_POINTS { 0, 0 } : prev(_data.cend())->second;
}

/// Return the number of QSOs at the epoch <i>t</i>
unsigned int rate_meter::qsos(const time_t t) const
{ const time_t now        { ::time(NULL) };                                     // time in seconds since the epoch
  const time_t query_time { min(now, t) };

  if (query_time == now)
    return current_qsos();

  SAFELOCK(_rate);

  return _data.empty() ? 0 : ((_data.lower_bound(query_time)->second).first);
}

/// Return the number of points at the epoch <i>t</i>
unsigned int rate_meter::score(const time_t t) const
{ const time_t now        { ::time(NULL) };                                     // time in seconds since the epoch
  const time_t query_time { min(now, t) };

  if (query_time == now)
    return current_score();

  SAFELOCK(_rate);

  return _data.empty() ? 0 : ((_data.lower_bound(query_time)->second).second);
}

/*! \brief      Return the number of QSOs and points at the epoch <i>t</i>
    \return     pair.first is the number of QSOs; pair.second is the number of points
*/
rate_meter::PAIR_NQSOS_POINTS rate_meter::qsos_and_score(const time_t t) const
{ const time_t now        { ::time(NULL) };                                     // time in seconds since the epoch
  const time_t query_time { min(now, t) };

  if (query_time == now)
    return current_qsos_and_score();

  SAFELOCK(_rate);

  if (_data.empty() or (t < _data.begin()->first))  // no data or we precede all the data
    return { 0, 0 };

  const auto ub { _data.upper_bound(query_time) };    // first element with key > query_time

  if (ub == _data.begin())                          // should never be true
    return { 0, 0 };

  return (prev(ub))->second;                        // ergo, prev(ub) should be last element with key <= query time
}

/*! \brief                              Return the difference in number of QSOs and points between now and some time in the past
    \param      seconds_in_past         Number of seconds in the past (i.e., from now) for which the result is desired
    \param      normalisation_period    period in seconds over which the returned values are to be normalised
    \return                             pair.first is the number of QSOs; pair.second is the number of points

    If <i>normalisation_period</i> is zero, then performs no normalisation, and returns merely the difference
    between now and the time represented by <i>seconds_in_past</i>. If <i>normalisation_period</i> is not zero,
    then normalises the differences to per <i>normalisation_rate</i> seconds.
*/
//auto rate_meter::calculate_rate(const int seconds_in_past, const unsigned int normalisation_period) -> PAIR_NQSOS_POINTS
rate_meter::PAIR_NQSOS_POINTS rate_meter::calculate_rate(const int seconds_in_past, const unsigned int normalisation_period) const
{ SAFELOCK(_rate);  // don't allow any changes while we're performing this calculation

  const time_t                           now            { ::time(NULL) };
  const pair<unsigned int, unsigned int> current_values { qsos_and_score(now) };

  if (seconds_in_past <= 0)
    return current_values;

  const PAIR_NQSOS_POINTS old_values { qsos_and_score(now - seconds_in_past) };

  if (normalisation_period == 0)
    return { current_values.first - old_values.first, current_values.second - old_values.second };

  return { (((current_values.first - old_values.first) * normalisation_period) / seconds_in_past),
           (((current_values.second - old_values.second) * normalisation_period) / seconds_in_past) };
}

/// convert to printable string
string rate_meter::to_string(void)
{ string rv    { "Number of points in rate = "s + ::to_string(_data.size()) + EOL };
  size_t index { 0 };

  for (auto datum : _data)
  { rv += "time = "s + ::to_string(datum.first) + "; n QSOs = "s + ::to_string(datum.second.first) + "; points = "s + ::to_string(datum.second.second);

    if (index++ != (_data.size() - 1) )
      rv += EOL;
  }

  return rv;
}

/// ostream << rate_meter
ostream& operator<<(ostream& ost, rate_meter& rate)
{ ost << rate.to_string();

  return ost;
}
