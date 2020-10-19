// $Id: rig_interface.h 169 2020-10-18 17:16:44Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   rig_interface.h

    Classes and functions related to transferring information between the computer and the rig.

    The user must be a member of the dialout group in order to use the serial port.
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
constexpr int RIG_UNABLE_TO_OPEN       { -1 },    ///< unable to access rig
              RIG_UNABLE_TO_INITIALISE { -2 },    ///< unable to initialise rig structure
              RIG_NO_SUCH_RIG          { -3 },    ///< unable to map rig name to hamlib model number
              RIG_INVALID_DATA_BITS    { -4 },    ///< data bits must be 7 or 8
              RIG_INVALID_STOP_BITS    { -5 },    ///< stop bits must be 1 or 2
              RIG_NO_RESPONSE          { -6 },    ///< no response received
              RIG_HAMLIB_ERROR         { -7 },    ///< unexpected event in hamlib
              RIG_UNEXPECTED_RESPONSE  { -8 },    ///< received unexpected response from rig
              RIG_MISC_ERROR           { -9 };    ///< other error

constexpr bool RESPONSE_EXPECTED    { true },               ///< raw K3 command expects a response
               NO_RESPONSE_EXPECTED { !RESPONSE_EXPECTED }; ///< raw K3 command does not expect a response

/// the two VFOs
enum class VFO { A,                       ///< VFO A
                 B                        ///< VFO B
               };

// ---------------------------------------- rig_status -------------------------

WRAPPER_2(rig_status, frequency, freq, MODE, mode);     ///< the status of a rig

// ---------------------------------------- rig_interface -------------------------

/*! \class  rig_interface
    \brief  The interface to a rig
*/

class rig_interface
{
protected:

  frequency                               _last_commanded_frequency    { };                             ///< last frequency to which the rig was commanded to QSY
  frequency                               _last_commanded_frequency_b  { };                             ///< last frequency to which VFO B was commanded to QSY
  MODE                                    _last_commanded_mode         { MODE_CW };                     ///< last mode into which the rig was commanded
  std::unordered_map<bandmode, frequency> _last_frequency;                                              ///< last-used frequencies on per-band, per-mode basis
  rig_model_t                             _model                       { RIG_MODEL_DUMMY };             ///< hamlib model
  hamlib_port_t                           _port;                                                        ///< hamlib port
  std::string                             _port_name                   { };                             ///< name of port
  RIG*                                    _rigp                        { nullptr };                     ///< hamlib handle
  bool                                    _rig_connected               { false };                       ///< is a rig connected?
  pt_mutex                                _rig_mutex                   { "RIG INTERFACE"s };            ///< mutex for all operations
  unsigned int                            _rig_poll_interval           { 1'000 };                       ///< interval between polling for rig status, in milliseconds
  rig_status                              _status                      { frequency(14'000), MODE_CW };  ///< most recent rig frequency and mode from the periodic poll
  pthread_t                               _thread_id;                                                   ///< ID for the thread that polls the rig for status

// protected pointers to functions

/*! \brief          Pointer to function used to alert the user to an error
    \param  msg     message to be presented to the user
*/
  void (*_error_alert_function)(const std::string& msg) { nullptr };

// protected functions

/*! \brief      Thread function to poll rig for status, forever
    \param  vp  unused (should be nullptr)
    \return     nullptr

    Sets the frequency and mode in the <i>_status</i> object
*/
  void* _poll_thread_function(void* vp);

/*! \brief          Static wrapper for function to poll rig for status
    \param  this_p  the this pointer, in order to allow static member access to a real object
    \return         nullptr
*/
  static void* _static_poll_thread_function(void* this_p);

/*! \brief      Allow direct access to the underlying file descriptor used to communicate with the rig
    \return     the file descriptor associated with the rig
*/
  inline int _file_descriptor(void) const
    { return (_rigp->state.rigport.fd); }

/*! \brief       Alert the user with a message
    \param  msg  message for the user

    Calls <i>_error_alert_function</i> to perform the actual alerting
*/
  void _error_alert(const std::string& msg);

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band
*/
  void _rig_frequency(const frequency&, const VFO v);

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
  frequency _rig_frequency(const VFO v);

public:

/// default constructor
  rig_interface(void) = default;
//  rig_interface (void) :
//    _last_commanded_frequency(),          // no last-commanded frequency
//    _last_commanded_frequency_b(),        // no last-commanded frequency for VFO B
//    _port_name()                         // no default port
//  { }

// no copy constructor
  rig_interface(const rig_interface&) = delete;

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
  void prepare(const drlog_context& context);

/*! \brief      Is a rig ready for use?
    \return     whether a rig is available
*/
  inline bool valid(void) const
    { return (_rigp != nullptr); }

/*! \brief          Set baud rate
    \param  rate    baud rate to which the rig should be set
*/
  void baud_rate(const unsigned int rate);

/*! \brief      Get baud rate
    \return     rig baud rate
*/
  unsigned int baud_rate(void);

/*! \brief          Set the number of data bits (7 or 8)
    \param  bits    the number of data bits to which the rig should be set

    Throws exception if <i>bits</i> is not 7 or 8
*/
  void data_bits(const unsigned int bits);

/*! \brief      Get the number of data bits
    \return     number of data bits
*/
  unsigned int data_bits(void);

/*! \brief          Set the number of stop bits (1 or 2)
    \param  bits    the number of stop bits to which the rig should be set

    Throws exception if <i>bits</i> is not 1 or 2
*/
  void stop_bits(const unsigned int bits);

/*! \brief      Get the number of stop bits
    \return     number of stop bits
*/
  unsigned int stop_bits(void);

/*! \brief      Set frequency of VFO A
    \param  f   new frequency of VFO A

    Does nothing if <i>f</i> is not within a ham band
*/
  inline void rig_frequency_a(const frequency& f)
    { _rig_frequency(f, VFO::A); }

/*! \brief      Set frequency of VFO A
    \param  f   new frequency of VFO A

    Does nothing if <i>f</i> is not within a ham band
*/
  inline void rig_frequency(const frequency& f)
    { rig_frequency_a(f); }

/*! \brief      Get the frequency of VFO A
    \return     frequency of VFO A
*/
  inline frequency rig_frequency_a(void)
    { return _rig_frequency(VFO::A); }

/*! \brief      Get the frequency of VFO A
    \return     frequency of VFO A
*/
  inline frequency rig_frequency(void)
    { return rig_frequency_a(); }

/*! \brief      Set frequency of VFO B
    \param  f   new frequency of VFO B

    Does nothing if <i>f</i> is not within a ham band
*/
  inline void rig_frequency_b(const frequency& f)
    { _rig_frequency(f, VFO::B); }

/// get frequency of VFO B
  inline frequency rig_frequency_b(void)
    { return _rig_frequency(VFO::B); }

/// set frequency of VFO B to match that of VFO A
  inline void rig_frequency_a_to_b(void)
    { rig_frequency_b(rig_frequency()); }

/*! \brief  Enable split operation

            hamlib has no good definition of exactly what split operation really means, and, hence,
            has no clear description of precisely what the hamlib rig_set_split_vfo() function is supposed
            to do for various values of the permitted parameters. There is general agreement on the reflector
            that the call contained herein *should* do the "right" thing -- but since there's no precise definition
            of any of this, not all backends are guaranteed to behave the same.

            Hence we use the explicit K3 command, since at least we know what that is supposed to do on that rig.
            (With the caveat that, because there is no proper transactional processing of K3 commands, any or all of
            this could fail silently. All we can do is to throw the commands at the rig and hope that they work.)
*/
  void split_enable(void);

/// disable split operation; see caveats under split_enable()
  void split_disable(void);

/*! \brief      Is split enabled?
    \return     whether split is enabled on the rig

                This interrogates the rig; it neither reads not writes the variable rig_is_split
*/
  bool split_enabled(void);

/// get mode
  MODE rig_mode(void);

/*! \brief      Set mode
    \param  m   new mode

    Also sets the bandwidth (because it's easier to follow hamlib's model, even though I regard it as flawed)
*/
  void rig_mode(const MODE m);

/*! \brief      Is the rig in TEST mode?
    \return     whether the rig is currently in TEST mode
*/
  bool test(void);

/*! \brief      Explicitly put the rig into or out of TEST mode
    \param  b   whether to enter TEST mode

    This works only with the K3.
*/
  void test(const bool);

/*! \brief      Set rit offset (in Hz)
    \param  hz  offset in Hz
*/
  void rit(const int hz);

/// get rit offset (in Hz)
  int rit(void);

/*! \brief  Turn rit on

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  void rit_enable(void);

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  void rit_disable(void);

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  inline void disable_rit(void)
    { rit_disable(); }

/// turn rit on
  inline void enable_rit(void)
    { rit_enable(); }

/// is rit enabled?
  bool rit_enabled(void);

/*! \brief      Set xit offset (in Hz)
    \param  hz  offset in Hz

    On the K3 this also sets the RIT (!)
*/
  void xit(const int hz);

/// get xit offset (in Hz)
  int xit(void);

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  void xit_enable(void);

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  void xit_disable(void);

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline void disable_xit(void)
    { xit_disable(); }

/// is xit enabled?
  bool xit_enabled(void);

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline void enable_xit(void)
    { xit_enable(); }

/// get the rig's frequency and mode
  rig_status status(void);                              // most recent rig status

/// is the VFO locked?
  bool is_locked(void);

/// lock the VFO
  void lock(void);

/// unlock the VFO
  void unlock(void);

/*! \brief      Turn sub-receiver on/off
    \param  b   turn sub-receiver on if TRUE, otherwise turn off
*/
  void sub_receiver(const bool);

/// is sub-receiver on?
  bool sub_receiver(void);

/// is sub-receiver on?
  inline bool sub_receiver_enabled(void)
    { return sub_receiver(); }

/// enable the sub-receiver
  inline void sub_receiver_enable(void)
    { sub_receiver(true); }

/// disable the sub-receiver
  inline void sub_receiver_disable(void)
    { sub_receiver(false); }

/// toggle sub-receiver between on and off
  void sub_receiver_toggle(void);

/// toggle sub-receiver between on and off
  inline void toggle_sub_receiver(void)
    { sub_receiver_toggle(); }

/// get the bandwidth in Hz
  int bandwidth(void);

/*! \brief          Set the keyer speed
    \param  wpm     keyer speed in WPM
*/
  void keyer_speed(const int wpm);

/// get the keyer speed in WPM
  int keyer_speed(void);

// explicit K3 commands
#if !defined(NEW_RAW_COMMAND)

/*! \brief                      Send a raw command to the rig
    \param  cmd                 the command to send
    \param  response_expected   whether a response is expected
    \return                     the response from the rig, or the empty string
*/
  const std::string raw_command(const std::string& cmd,
                                const bool response_expected = NO_RESPONSE_EXPECTED);

#endif

#if defined(NEW_RAW_COMMAND)
  const std::string raw_command(const std::string& cmd,
                                const unsigned int expected_length);
#endif

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \return     the rig's most recent frequency for bandmode <i>bm</i>

     Returns empty frequency() if the most recent frequency for the band and mode has not been set (should never happen) 
*/
  frequency get_last_frequency(const bandmode bm);

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the rig's most recent frequency for band <i>b</i> and mode <i>m</i>.

    Returns empty frequency() if the most recent frequency for the band and mode has not been set (should never happen)  
*/
  inline frequency get_last_frequency(const BAND b, const MODE m)
    { return get_last_frequency( { b, m } ); }

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \param  f   frequency
*/
  void set_last_frequency(const bandmode bm, const frequency& f);

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  b   band
    \param  m   mode
    \param  f   frequency
*/
  inline void set_last_frequency(const BAND b, const MODE m, const frequency& f)
    { set_last_frequency( { b, m }, f ); }

/*! \brief Is the rig transmitting?

    With the K3, this is unreliable: the routine frequently takes the _error_alert() path, even if the rig is not transmitting.
    (This is, unfortunately, just one example of the basic unreliability of the K3 in responding to commands.)
*/
  bool is_transmitting(void);

/*! \brief                          Register a function for alerting the user
    \param  error_alert_function    pointer to function for alerting the user
*/
  void register_error_alert_function(void (*error_alert_function)(const std::string&) );

/*! \brief      Which VFO is currently used for transmitting?
    \return     the VFO that is currently set to be used when transmitting
*/
  VFO tx_vfo(void);

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
  void bandwidth_a(const unsigned int hz);

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
  inline void bandwidth(const unsigned int hz)
    { bandwidth_a(hz); }

/*! \brief      Set the bandwidth of VFO B
    \param  hz  desired bandwidth, in Hz
*/
  void bandwidth_b(const unsigned int hz);

/// set RIT, split, sub-rx off
  void base_state(void);

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use

    Works only with K3
*/
  bool rx_ant(void);

/*! \brief          Control use of the RX antenna
    \param  torf    whether to use the RX antenna

    Works only with K3
*/
  void rx_ant(const bool torf);

/// toggle whether the RX antenna is in use
  inline void rx_ant_toggle(void)
    { rx_ant(!rx_ant()); }

/// toggle whether the RX antenna is in use
  inline void toggle_rx_ant(void)
    { rx_ant_toggle(); }
};

/*! \brief      Convert a hamlib error code to a printable string
    \param  e   hamlib error code
    \return     Printable string corresponding to error code <i>e</i>
*/
std::string hamlib_error_code_to_string(const int e);

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(rig_interface_error);   ///< errors related to accessing the rig

#endif /* RIG_INTERFACE_H */
