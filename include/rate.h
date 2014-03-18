// $Id: rate.h 51 2014-02-22 18:00:47Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

#ifndef RATE_H
#define RATE_H

/*!     \file rate.h

        Classes and functions related to QSO and point rate
*/

#include "log_message.h"
extern message_stream    ost;


#include "pthread_support.h"

#include <chrono>
#include <map>

// -----------  rate_meter  ----------------

/*!     \class rate_meter
        \brief Keep track of rate information

        I think it is easier to use UNIX time rather than the chrono library here.
        Well, perhaps not easier here, but it's easier to interface with the rest
        of the program if we use time_t instead of timepoints.
*/

class rate_meter
{
protected:

  std::map<time_t /* epoch time */, std::pair<unsigned int /* n_qsos */, unsigned int /* points */> > _data;
//  time_t                                                                                              _earliest_time;  // time of first element
  pt_mutex _rate_mutex;

public:

//  rate_meter(void) :
//    _earliest_time(0)
//  { }

  inline const bool insert(const time_t t, const unsigned int nq, const unsigned int np)
  { SAFELOCK(_rate);

    ost << "Inserting <fn 1> into rate: " << t << ", " << nq << ", " << np << std::endl;

    return _data.insert( { t, { nq, np } } ).second;
  }

  inline const bool insert(const time_t t, const std::pair<unsigned int, unsigned int>& p)
  { SAFELOCK(_rate);

    ost << "Inserting <fn 2> into rate: " << t << ", " << p.first << ", " << p.second << std::endl;

    return _data.insert( { t, p } ).second;
  }

  const bool insert(const time_t t, const unsigned int np)
  { SAFELOCK(_rate);

    ost << "Inserting <fn 3> into rate: " << t << ", " << (_data.size() + 1) << ", " << np << std::endl;

    return _data.insert( { t, { (_data.size() + 1), np } } ).second;
  }

  inline const size_t size(void)
  { SAFELOCK(_rate);
    return _data.size();
  }

  inline void clear(void)
  { SAFELOCK(_rate);
    _data.clear();
  }

//  const unsigned int current_q_value = (time_n_qsos.empty() ? 0 : prev(time_n_qsos.cend())->second);
  inline const unsigned int current_qsos(void)
  { SAFELOCK(_rate);

    return (_data.empty() ? 0 : (prev(_data.cend())->second).first);
  }

  inline const unsigned int current_score(void)
  { SAFELOCK(_rate);

    return (_data.empty() ? 0 : (prev(_data.cend())->second).second);
  }

  const std::pair<unsigned int, unsigned int> current_qsos_and_score(void);

  //const auto qcit = time_n_qsos.lower_bound(now - rate * 60);
  //const unsigned int q_value = (qcit == time_n_qsos.cbegin() ? 0 : prev(qcit)->second);

  const unsigned int qsos(const time_t t);

  const unsigned int score(const time_t t);

  const std::pair<unsigned int, unsigned int> qsos_and_score(const time_t t);

  const std::pair<unsigned int, unsigned int> calculate_rate(const int seconds_in_past, const unsigned int normalisation_period = 3600);

  template<typename Archive>
  void serialize(Archive& ar, const unsigned version)
    { SAFELOCK(_rate);

      ar & _data;
    }

};

#endif /* RATE_H */
