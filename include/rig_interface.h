// $Id: rig_interface.h 43 2013-12-07 20:29:56Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*!     \file rig_interface.h

        Classes and functions related to transferring information
        between the computer and the rig.

        The more I spend time here, the more I think that it was a mistake to use hamlib :-(
*/

#ifndef RIG_INTERFACE_H
#define RIG_INTERFACE_H

#include "bands-modes.h"
#include "drlog_context.h"
#include "macros.h"
#include "pthread_support.h"
#include "x_error.h"

#include <hamlib/rig.h>

#include <map>
#include <string>
#include <utility>

// Errors
const int RIG_UNABLE_TO_OPEN       = -1,    ///< unable to access rig
          RIG_UNABLE_TO_INITIALISE = -2,    ///< unable to initialise rig structure
          RIG_NO_SUCH_RIG          = -3,    ///< unable to map rig name to hamlib model number
          RIG_INVALID_DATA_BITS    = -4,    ///< data bits must be 7 or 8
          RIG_INVALID_STOP_BITS    = -5,    ///< stop bits must be 1 or 2
          RIG_NO_RESPONSE          = -6,    ///< no response received
          RIG_HAMLIB_ERROR         = -7,    ///< unexpected event in hamlib
          RIG_UNEXPECTED_RESPONSE  = -8;    ///< received unexpected response from rig

const bool RESPONSE = true,
           NO_RESPONSE = false;

extern std::map<std::pair<BAND, MODE>, frequency > DEFAULT_FREQUENCIES;

// ---------------------------------------- rig_status -------------------------

/*!     \class rig_status
        \brief The status of a rig .... a trivial wrapper
*/

WRAPPER_2(rig_status, frequency, freq, MODE, mode);

// ---------------------------------------- rig_interface -------------------------

/*!     \class rig_interface
        \brief The interface to a rig
*/

class rig_interface
{
protected:
  hamlib_port_t    _port;
  char*            _port_name;                           // ugh!

  RIG*             _rigp;
  rig_model_t      _model;

  rig_status       _status;                              // most recent rig status from the periodic poll
  pt_mutex         _rig_mutex;                           // mutex for all operations

  unsigned int     _rig_poll_interval;                   // interval between polling for rig status, in milliseconds
  pthread_t        _thread_id;                           // ID for the thread that polls the rig for status

  void*            _poll_thread_function(void*);         // function to poll rig for status
  static void*     _static_poll_thread_function(void*);  // function to poll rig for status

// sleep for a designated number of milliseconds
//  void              _msec_sleep(const unsigned int msec);

// allow direct access to the file descriptor
  inline const int  _file_descriptor(void) const
    { return _rigp->state.rigport.fd; }

// keep track of frequencies on per-band, per-mode basis
  std::map< std::pair <BAND, MODE>, frequency > _last_frequency;

  bool              _rig_connected;
  frequency         _last_commanded_frequency;
  MODE              _last_commanded_mode;

  frequency         _last_commanded_frequency_b;

  void (*_error_alert_function)(const std::string&);

  void _error_alert(const std::string& msg);

public:

// constructor
  rig_interface ( void /* const std::string& serial_port */);

// destructor
  ~rig_interface(void);

/// prepare rig for use
  void prepare(const drlog_context& context);

  const bool valid(void) const
    { return (_rigp != nullptr); }

// get/set baud rate
  void baud_rate(const unsigned int rate);

  const unsigned int baud_rate(void);

// get/set number of data bits
  void data_bits(const unsigned int bits);

  const unsigned int data_bits(void);

// get/set number of stop bits
  void stop_bits(const unsigned int bits);

  const unsigned int stop_bits(void);

// get/set frequency
  void rig_frequency(const frequency& f);

  const frequency rig_frequency(void);

// get/set frequency, VFO B
  void rig_frequency_b(const frequency& f);

  const frequency rig_frequency_b(void);

  const MODE rig_mode(void);

  void rig_mode(const MODE m);

// is rig in TEST mode?
  const bool test(void);

  void test(const bool);

// set rit offset
  void rit(const int hz);

// get rit offset
  const int rit(void);

// turn rit on/off
  void rit_enable(void);

  void rit_disable(void);

  inline void disable_rit(void)
    { rit_disable(); }

  inline void enable_rit(void)
    { rit_enable(); }

  const bool rit_enabled(void);

// set xit offset
  void xit(const int hz);

// get xit offset
  const int xit(void);

// turn xit on/off
// this is a kludge, since hamlib equates an offset with xit turned off (!)
  void xit_enable(void);

  void xit_disable(void);

  inline void disable_xit(void)
    { xit_disable(); }

  const bool xit_enabled(void);
//    { return (xit() == 0); }

  inline void enable_xit(void)
    { xit_enable(); }

  const rig_status status(void);                              // most recent rig status

// is the VFO locked?
  const bool is_locked(void);

// lock the VFO
  void lock(void);

// unlock the VFO
  void unlock(void);

// sub-receiver on/off
  void sub_receiver(const bool);

// is sub-receiver on?
  const bool sub_receiver(void);

  inline const bool sub_receiver_enabled(void)
    { return sub_receiver(); }

  inline void sub_receiver_enable(void)
    { sub_receiver(true); }

  inline void sub_receiver_disable(void)
    { sub_receiver(false); }



// get the bandwidth in Hz
  const int bandwidth(void);

// set the keyer speed in WPM
  void keyer_speed(const int wpm);

// get the keyer speed in WPM
  const int keyer_speed(void);

// explicit K3 commands
  const std::string raw_command(const std::string& cmd,
                                const bool expect_response = NO_RESPONSE);

  const frequency get_last_frequency(const BAND b, const MODE m);
  void set_last_frequency(const BAND b, const MODE m, const frequency& f) noexcept;

  const bool is_transmitting(void);

// register a function for alerting the user
  void register_error_alert_function(void (*error_alert_function)(const std::string&) );

// get the band and mode from the rig, so that the status can be subsequently checked
//  inline void get_status(void)
//    { _poll_thread_function(nullptr); }
};

// -------------------------------------- Errors  -----------------------------------

/*! \class  rig_interface_error
    \brief  Errors related to accessing the rig
*/

class rig_interface_error : public x_error
{
protected:

public:

/*! \brief  Construct from error code and reason
  \param  n Error code
  \param  s Reason
*/
  inline rig_interface_error(const int n, const std::string& s) :
    x_error(n, s)
  { }
};


#endif /* RIG_INTERFACE_H */
