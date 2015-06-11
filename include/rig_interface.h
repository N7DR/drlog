// $Id: rig_interface.h 106 2015-06-06 16:11:23Z  $

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
          RIG_UNEXPECTED_RESPONSE  = -8,    ///< received unexpected response from rig
          RIG_MISC_ERROR           = -9;    ///< other error

const bool RESPONSE = true,                 ///< raw K3 command expects a response
           NO_RESPONSE = false;             ///< raw K3 command does not expect a response

enum VFO { VFO_A = 0,                       ///< VFO A
           VFO_B                            ///< VFO B
         };

extern std::map<std::pair<BAND, MODE>, frequency > DEFAULT_FREQUENCIES;    ///< default frequencies, per-band and per-mode

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
  frequency                                     _last_commanded_frequency;      ///< last frequency to which the rig was commanded to QSY
  frequency                                     _last_commanded_frequency_b;    ///< last frequency to which the VFO B was commanded to QSY
  MODE                                          _last_commanded_mode;           ///< last mode into which the rig was commanded
  std::map< std::pair <BAND, MODE>, frequency > _last_frequency;                ///< last-used frequencies on per-band, per-mode basis
  rig_model_t                                   _model;                         ///< hamlib model
  hamlib_port_t                                 _port;                          ///< hamlib port
  std::string                                   _port_name;                     ///< name of port
  RIG*                                          _rigp;                          ///< hamlib handle
  bool                                          _rig_connected;                 ///< is a rig connected?
  pt_mutex                                      _rig_mutex;                     ///< mutex for all operations
  unsigned int                                  _rig_poll_interval;             ///< interval between polling for rig status, in milliseconds
  rig_status                                    _status;                        ///< most recent rig frequency and mode from the periodic poll
  pthread_t                                     _thread_id;                     ///< ID for the thread that polls the rig for status

/*! \brief      Thread function to poll rig for status, forever
    \param  vp  unused (should be nullptr)
    \return     nullptr

    Sets the frequency and mode in the <i>_status</i> object
*/
  void* _poll_thread_function(void* vp);

/*! \brief      static wrapper for function to poll rig for status
    \param  vp  the this pointer, in order to allow static member access to a real object
    \return     nullptr
*/
  static void* _static_poll_thread_function(void* vp);

///< allow direct access to the underlying file descriptor used to communicate with the rig
  inline const int  _file_descriptor(void) const
    { return _rigp->state.rigport.fd; }

/*! \brief          Pointer to function used to alert the user to an error
    \param  msg     message to be presented to the user
*/
  void (*_error_alert_function)(const std::string& msg);

/*! \brief       Alert the user with a message
    \param  msg  message for the user

    Calls <i>_error_alert_function</i> to perform the actual alerting
*/
  void _error_alert(const std::string& msg);

public:

/// default constructor
  rig_interface (void);

/// destructor
  virtual ~rig_interface(void);

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
  void prepare(const drlog_context& context);

/*! \brief      Is a rig ready for use?
    \return     whether a rig is available
*/
  inline const bool valid(void) const
    { return (_rigp != nullptr); }

/// set baud rate
  void baud_rate(const unsigned int rate);

/// get baud rate
  const unsigned int baud_rate(void);

/// set number of data bits
  void data_bits(const unsigned int bits);

/// get number of data bits
  const unsigned int data_bits(void);

/// set number of stop bits
  void stop_bits(const unsigned int bits);

/// get number of stop bits
  const unsigned int stop_bits(void);

/*! \brief      Set frequency of VFO A
    \param  f   frequency to which to QSY

                Does nothing if <i>f</i> is not within a ham band
*/
  void rig_frequency(const frequency& f);

/// get the frequency of VFO A
  const frequency rig_frequency(void);

/*! \brief      Set frequency of VFO B
    \param  f   frequency to which to QSY

                Does nothing if <i>f</i> is not within a ham band
*/
  void rig_frequency_b(const frequency& f);

/// get frequency of VFO B
  const frequency rig_frequency_b(void);

/// set frequency of VFO B to match that of VFO A
  inline void rig_frequency_a_to_b(void)
    { rig_frequency_b(rig_frequency()); }

/*! \brief  Enable split operation

            hamlib has no good definition of exactly what split operation really means, and, hence,
            has no clear description of precisely what the hamlib rig_set_split_vfo() function is supposed
            to do for various values of the permitted parameters. There is general agreement on the reflector
            that the call contained herein *should* do the "right" thing -- but since there's no precise definition
            of any of this, not all backends are guaranteed to behave the same.

            Hence we use the explicit K3 command, since at least we know what that will do on that rig.
*/
  void split_enable(void);

/// disable split operation; see caveats under split_enable()
  void split_disable(void);

/// is split enabled?
  const bool split_enabled(void);

/// get mode
  const MODE rig_mode(void);

/*! \brief      Set mode
    \param  m   new mode

                Also sets the bandwidth (because it's easier to follow hamlib's model, even though I regard it as flawed)
*/
  void rig_mode(const MODE m);

/// is rig in TEST mode?
  const bool test(void);

/// set or unset test mode
  void test(const bool);

/// set rit offset (in Hz)
  void rit(const int hz);

/// get rit offset (in Hz)
  const int rit(void);

/// turn rit on
  void rit_enable(void);

/// turn rit off
  void rit_disable(void);

/// turn rit off
  inline void disable_rit(void)
    { rit_disable(); }

/// turn rit on
  inline void enable_rit(void)
    { rit_enable(); }

/// is rit enabled?
  const bool rit_enabled(void);

// set xit offset (in Hz)
  void xit(const int hz);

// get xit offset (in Hz)
  const int xit(void);

// turn xit on
// this is a kludge, since hamlib equates a zero offset with xit disabled (!)
  void xit_enable(void);

// turn xit off
// this is a kludge, since hamlib equates a zero offset with xit disabled (!)
  void xit_disable(void);

// turn xit off
// this is a kludge, since hamlib equates a zero offset with xit disabled (!)
  inline void disable_xit(void)
    { xit_disable(); }

  const bool xit_enabled(void);

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
#if !defined(NEW_RAW_COMMAND)
  const std::string raw_command(const std::string& cmd,
                                const bool expect_response = NO_RESPONSE);
#endif

#if defined(NEW_RAW_COMMAND)
  const std::string raw_command(const std::string& cmd,
                                const unsigned int expected_length);
#endif

  const frequency get_last_frequency(const BAND b, const MODE m);
  void set_last_frequency(const BAND b, const MODE m, const frequency& f) noexcept;

  const bool is_transmitting(void);

// register a function for alerting the user
  void register_error_alert_function(void (*error_alert_function)(const std::string&) );

  const VFO tx_vfo(void);

// get the band and mode from the rig, so that the status can be subsequently checked
//  inline void get_status(void)
//    { _poll_thread_function(nullptr); }
};

const std::string hamlib_error_code_to_string(const int e);

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
