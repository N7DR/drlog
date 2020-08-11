// $Id: rate.h 163 2020-08-06 19:46:33Z  $

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

<<<<<<< HEAD
  using PAIR_NQSOS_POINTS = std::pair<unsigned int /* n_qsos */, unsigned int /* points */>;

  std::map<time_t /* epoch time */, PAIR_NQSOS_POINTS > _data;    ///< number of QSOs and points at a particular time
=======
  std::map<time_t /* epoch time */, std::pair<unsigned int /* n_qsos */, unsigned int /* points */> > _data;    ///< number of QSOs and points at a particular time
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9

  pt_mutex _rate_mutex;                                  ///< In order to lock the object

public:

/*! \brief      Insert information into <i>_data</i>
    \param  t   epoch
    \param  nq  number of qsos at epoch <i>t</i>
    \param  np  number of points at epoch <i>t</i>
    \return     Whether insertion was successful
*/
  inline const bool insert(const time_t t, const unsigned int nq, const unsigned int np)
  { SAFELOCK(_rate);
    return _data.insert( { t, { nq, np } } ).second;
  }

/*! \brief      Insert information into <i>_data</i>
    \param  t   epoch
    \param  p   number of qsos and points at epoch <i>t</i>
    \return     Whether insertion was successful
*/
<<<<<<< HEAD
  inline bool insert(const time_t t, const PAIR_NQSOS_POINTS& p)
=======
  inline const bool insert(const time_t t, const std::pair<unsigned int, unsigned int>& p)
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9
  { SAFELOCK(_rate);
    return _data.insert( { t, p } ).second;
  }

/*! \brief      Insert information into <i>_data</i>
    \param  t   epoch
    \param  np  number of points at epoch <i>t</i>
    \return     Whether insertion was successful

    Assumes that the number of QSOs is one greatert han the current number in <i>_data</i>
*/
  const bool insert(const time_t t, const unsigned int np)
  { SAFELOCK(_rate);
    return _data.insert( { t, { (_data.size() + 1), np } } ).second;
  }

/// number of values in <i>_data</i>
  inline const size_t size(void)
  { SAFELOCK(_rate);
    return _data.size();
  }

/// Empty <i>_data</i>
  inline void clear(void)
  { SAFELOCK(_rate);
    _data.clear();
  }

/// Return the number of QSOs at the current epoch
  inline const unsigned int current_qsos(void)
  { SAFELOCK(_rate);
    return (_data.empty() ? 0 : (prev(_data.cend())->second).first);
  }

/// Return the number of points at the current epoch
  inline const unsigned int current_score(void)
  { SAFELOCK(_rate);

    return (_data.empty() ? 0 : (prev(_data.cend())->second).second);
  }

/*! \brief      Return the number of QSOs and points at the current epoch
    \return     pair.first is the number of QSOs; pair.second is the number of points
*/
<<<<<<< HEAD
  /* std::pair<unsigned int, unsigned int> */ PAIR_NQSOS_POINTS current_qsos_and_score(void);
=======
  const std::pair<unsigned int, unsigned int> current_qsos_and_score(void);
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9

/// Return the number of QSOs at the epoch <i>t</i>
  const unsigned int qsos(const time_t t);

/// Return the number of points at the epoch <i>t</i>
  const unsigned int score(const time_t t);

/*! \brief      Return the number of QSOs and points at the epoch <i>t</i>
    \return     pair.first is the number of QSOs; pair.second is the number of points
*/
<<<<<<< HEAD
  PAIR_NQSOS_POINTS qsos_and_score(const time_t t);
=======
  const std::pair<unsigned int, unsigned int> qsos_and_score(const time_t t);

>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9

/*! \brief                          Return the difference in number of QSOs and points between now and some time in the past
    \param  seconds_in_past         number of seconds in the past (i.e., from now) for which the result is desired
    \param  normalisation_period    period in seconds over which the returned values are to be normalised
    \return                         pair.first is the number of QSOs; pair.second is the number of points

    If <i>normalisation_period</i> is zero, then performs no normalisation, and returns merely the difference
    between now and the time represented by <i>seconds_in_past</i>. If <i>normalisation_period</i> is not zero,
    then normalises the differences to per <i>normalisation_rate</i> seconds.
*/
<<<<<<< HEAD
  PAIR_NQSOS_POINTS calculate_rate(const int seconds_in_past, const unsigned int normalisation_period = 3600);
=======
  const std::pair<unsigned int, unsigned int> calculate_rate(const int seconds_in_past, const unsigned int normalisation_period = 3600);
>>>>>>> 0a43fe059e6587fe915f47631dbfa4e529ab7fa9

/// convert to printable string
  const std::string to_string(void);

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
