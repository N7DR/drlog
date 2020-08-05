// $Id: procfs.cpp 153 2019-09-01 14:27:02Z  $

/*! \file procfs.cpp

    Class for accessing /proc/[pid] values
*/

#include "procfs.h"

using namespace std;
using namespace   chrono;

// -----------  procfs ----------------

/*! \class  procfs
    \brief  Information from the /proc/[pid] subsystem

    Currently, each access causes a new read from the procfs.
*/

vector<string> procfs::_statvec(void)
{ const system_clock::time_point now { system_clock::now() };

  if ( /* force or */ ( (now - _last_update_time) >  _minimum_interval) )      // update only if forced or if enough time has passed
  { _last_stat_vec = split_string(_stat(), ' ');
    _last_update_time = now;
  }

  return _last_stat_vec;
}
