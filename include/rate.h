// $Id: rate.h 189 2021-08-16 00:34:00Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef RATE_H
#define RATE_H

/*! \file rate.h

    Classes and functions related to QSO and point rate
*/

#include "log_message.h"
#include "pthread_support.h"

#include <chrono>
#include <map>

extern message_stream    ost;                ///< debugging/logging output

// -----------  rate_meter  ----------------

/*! \class rate_meter
    \brief Keep track of rate information

    I think it is easier to use UNIX time rather than the chrono library here.
    Well, perhaps not easier here, but it's easier to interface with the rest
    of the program if we use time_t instead of timepoints.
*/

class rate_meter
{
protected:

  using PAIR_NQSOS_POINTS = std::pair<unsigned int /* n_qsos */, unsigned int /* points */>;

  std::map<time_t /* epoch time */, PAIR_NQSOS_POINTS > _data;    ///< number of QSOs and points at a particular time

  mutable pt_mutex _rate_mutex { "DEFAULT RATE METER"s };                                  ///< In order to lock the object

public:

/*! \brief      Insert information into <i>_data</i>
    \param  t   epoch
    \param  nq  number of qsos at epoch <i>t</i>
    \param  np  number of points at epoch <i>t</i>
    \return     Whether insertion was successful
*/
  inline bool insert(const time_t t, const unsigned int nq, const unsigned int np)
  { SAFELOCK(_rate);
    return _data.insert( { t, { nq, np } } ).second;
  }

/*! \brief      Insert information into <i>_data</i>
    \param  t   epoch
    \param  p   number of qsos and points at epoch <i>t</i>
    \return     Whether insertion was successful
*/
  inline bool insert(const time_t t, const PAIR_NQSOS_POINTS& p)
  { SAFELOCK(_rate);
    return _data.insert( { t, p } ).second;
  }

/*! \brief      Insert information into <i>_data</i>
    \param  tp  epoch, number of qsos and points at epoch
*/
  void operator+=(const std::pair<time_t, PAIR_NQSOS_POINTS>& tp)
  { SAFELOCK(_rate);
    _data += tp;
  }

/*! \brief      Insert information into <i>_data</i>
    \param  t   epoch
    \param  np  number of points at epoch <i>t</i>
    \return     Whether insertion was successful

    Assumes that the number of QSOs is one greater than the current number in <i>_data</i>
*/
  bool insert(const time_t t, const unsigned int np)
  { SAFELOCK(_rate);
    return _data.insert( { t, { (_data.size() + 1), np } } ).second;
  }

/*! \brief          Insert information into <i>_data</i>
    \param  t_np    epoch, points at epoch
*/
  void operator+=(const std::pair<time_t, const unsigned int>&& t_np)
  { SAFELOCK(_rate);
    _data += { t_np.first, { (_data.size() + 1), t_np.second } };
  }

/// number of values in <i>_data</i>
  inline size_t size(void)
  { SAFELOCK(_rate);
    return _data.size();
  }

/// Empty <i>_data</i>
  inline void clear(void)
  { SAFELOCK(_rate);
    _data.clear();
  }

/// Return the number of QSOs at the current epoch
  inline unsigned int current_qsos(void) const
  { SAFELOCK(_rate);
    return _data.empty() ? 0 : (prev(_data.cend())->second).first;
  }

/// Return the number of points at the current epoch
  inline unsigned int current_score(void) const
  { SAFELOCK(_rate);

    return _data.empty() ? 0 : (prev(_data.cend())->second).second;
  }

/*! \brief      Return the number of QSOs and points at the current epoch
    \return     pair.first is the number of QSOs; pair.second is the number of points
*/
  PAIR_NQSOS_POINTS current_qsos_and_score(void) const;

/// Return the number of QSOs at the epoch <i>t</i>
  unsigned int qsos(const time_t t) const;

/// Return the number of points at the epoch <i>t</i>
  unsigned int score(const time_t t) const;

/*! \brief      Return the number of QSOs and points at the epoch <i>t</i>
    \return     pair.first is the number of QSOs; pair.second is the number of points
*/
  PAIR_NQSOS_POINTS qsos_and_score(const time_t t) const;

/*! \brief                          Return the difference in number of QSOs and points between now and some time in the past
    \param  seconds_in_past         number of seconds in the past (i.e., from now) for which the result is desired
    \param  normalisation_period    period in seconds over which the returned values are to be normalised
    \return                         pair.first is the number of QSOs; pair.second is the number of points

    If <i>normalisation_period</i> is zero, then performs no normalisation, and returns merely the difference
    between now and the time represented by <i>seconds_in_past</i>. If <i>normalisation_period</i> is not zero,
    then normalises the differences to per <i>normalisation_rate</i> seconds.
*/
  PAIR_NQSOS_POINTS calculate_rate(const int seconds_in_past, const unsigned int normalisation_period = 3600) const;

/// convert to printable string
  std::string to_string(void);

/// serialize using boost
  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_rate);

      ar & _data;
    }
};

/// ostream << rate_meter
std::ostream& operator<<(std::ostream& ost, rate_meter& rate);

#endif /* RATE_H */
