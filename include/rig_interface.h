// $Id: rig_interface.h 272 2025-07-13 22:28:31Z  $

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

enum class RESPONSE { EXPECTED,
                      NOT_EXPECTED
                    };

/// the two VFOs
enum class VFO { A,                       ///< VFO A
                 B                        ///< VFO B
               };

/// tap or hold
enum class PRESS { TAP,
                   HOLD
                 };

enum class K3_BUTTON { NOTCH = 32
                     };

enum class K3_COMMAND_MODE { NORMAL,
                             EXTENDED
                           };

using DRLOG_CLOCK = std::chrono::system_clock;

// ---------------------------------------- rig_status -------------------------

WRAPPER_2(rig_status, frequency, freq, MODE, mode);     ///< the status of a rig

// ---------------------------------------- audio_filter -------------------------

/*! \class  audio_filter
    \brief  Encapsulate the basic characteristics of an audio filter
*/

class audio_filter
{
protected:

  int _bandwidth   { 0 };     ///< some measure of bandwidth, in Hz
  int _centre      { 0 };     ///< centre frequency, in Hz
  int _granularity { 50 };    ///< granularity, in Hz

/*  \brief          convert a value to a rounded value with a particular granularity
    \param  value   the value to convert/round
    \return         <i>value</i> rounded with granularity <i>_granularity</i>
*/
  inline int _round(const int value) const
    { return ( ( (value + (_granularity / 2) ) / _granularity ) * _granularity ); }

public:

/// default constructor
  audio_filter(void) = default;

  READ(centre);                     ///< centre frequency, in Hz
  READ(bandwidth);                  ///< some measure of bandwidth, in Hz
  READ_AND_WRITE(granularity);      ///< granularity, in Hz

/*  \brief       set the bandwidth to a rounded value, in Hz
    \param  bw   the unrounded bandwidth, in Hz
*/
  inline void bandwidth(const int bw)
    { _bandwidth = _round(bw); }

/*  \brief       set the centre frequency to a rounded value, in Hz
    \param  bw   the unrounded bandwidth, in Hz
*/
  inline void centre(const int fc)
    { _centre = _round(fc); }
};

// ---------------------------------------- rig_interface -------------------------

/*! \class  rig_interface
    \brief  The interface to a rig

    A good argument can be made that this should be a base class, with specialisations occurring
    in derived classes. For now, that's not done.
*/

class rig_interface
{
protected:

  bool                                    _instrumented                { false };                       ///< whether to record all exchanges with rig
  frequency                               _last_commanded_frequency    { 14_MHz };                      ///< last frequency to which the rig was commanded to QSY
  frequency                               _last_commanded_frequency_b  { 14_MHz };                      ///< last frequency to which VFO B was commanded to QSY
  MODE                                    _last_commanded_mode         { MODE_CW };                     ///< last mode into which the rig was commanded
  std::unordered_map<bandmode, frequency> _last_frequency              { };                             ///< last-used frequencies on per-band, per-mode basis
  rig_model_t                             _model                       { RIG_MODEL_DUMMY };             ///< hamlib model
  hamlib_port_t                           _port;                                                        ///< hamlib port
  std::string                             _port_name                   { };                             ///< name of port
  RIG*                                    _rigp                        { nullptr };                     ///< hamlib handle
  bool                                    _rig_connected               { false };                       ///< is a rig connected?
  pthread_t                               _thread_id;                                                   ///< ID for the thread that polls the rig for status

  mutable pt_mutex                        _rig_mutex                   { "RIG INTERFACE"s };            ///< mutex for all operations

// protected pointers to functions

/*! \brief          Pointer to function used to alert the user to an error
    \param  msg     message to be presented to the user
*/
//  void (*_error_alert_function)(const std::string& msg) { nullptr };
  void (*_error_alert_function)(const std::string_view msg) { nullptr };

// protected functions

/*! \brief       Alert the user with a message
    \param  msg  message for the user

    Calls <i>_error_alert_function</i> to perform the actual alerting
*/
//  void _error_alert(const std::string& msg) const;
  void _error_alert(const std::string_view msg) const;

/*! \brief      Allow direct access to the underlying file descriptor used to communicate with the rig
    \return     the file descriptor associated with the rig
*/
  inline int _file_descriptor(void) const
    { return (_rigp->state.rigport.fd); }

/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  expectation     whether a response is expected
    \param  expected_len    expected length of response
    \return                 the response from the rig, or the empty string

    Currently any expected length is ignored; the routine looks for the concluding ";" instead
*/
  std::string _retried_raw_command(const std::string& cmd, /*const int expected_len = 0, */const int timeout_ms = 250, const int n_retries = 0);

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band.
    Attempts to confirm that the frequency was actually set to <i>f</i>.
*/
//  void _rig_frequency(const frequency&, const VFO v);
  void _rig_frequency(const frequency, const VFO v);

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
  frequency _rig_frequency(const VFO v) const;

public:

/// default constructor
  rig_interface(void) = default;

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
  inline unsigned int baud_rate(void) const;

/*! \brief          Set the number of data bits (7 or 8)
    \param  bits    the number of data bits to which the rig should be set

    Throws exception if <i>bits</i> is not 7 or 8
*/
  void data_bits(const unsigned int bits);

/*! \brief      Get the number of data bits
    \return     number of data bits
*/
  unsigned int data_bits(void) const;

/*! \brief          Set the number of stop bits (1 or 2)
    \param  bits    the number of stop bits to which the rig should be set

    Throws exception if <i>bits</i> is not 1 or 2
*/
  void stop_bits(const unsigned int bits);

/*! \brief      Get the number of stop bits
    \return     number of stop bits
*/
  unsigned int stop_bits(void) const;

/*! \brief      Set frequency of VFO A
    \param  f   new frequency of VFO A

    Does nothing if <i>f</i> is not within a ham band
*/
  inline void rig_frequency_a(const frequency f)
    { _rig_frequency(f, VFO::A); }

/*! \brief      Set frequency of VFO A
    \param  f   new frequency of VFO A

    Does nothing if <i>f</i> is not within a ham band
*/
  inline void rig_frequency(const frequency f)
    { rig_frequency_a(f); }

/*! \brief      Get the frequency of VFO A
    \return     frequency of VFO A
*/
  inline frequency rig_frequency_a(void) const
    { return _rig_frequency(VFO::A); }

/*! \brief      Get the frequency of VFO A
    \return     frequency of VFO A
*/
  inline frequency rig_frequency(void) const
    { return rig_frequency_a(); }

/*! \brief      Set frequency of VFO B
    \param  f   new frequency of VFO B

    Does nothing if <i>f</i> is not within a ham band
*/
  inline void rig_frequency_b(const frequency f)
    { _rig_frequency(f, VFO::B); }

/// get frequency of VFO B
  inline frequency rig_frequency_b(void) const
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
  void split_enable(void) const;

/// disable split operation; see caveats under split_enable()
  void split_disable(void) const;

/*! \brief      Is split enabled?
    \return     whether split is enabled on the rig

    This interrogates the rig; it neither reads not writes the variable rig_is_split
*/
  bool split_enabled(void) const;

/// get mode
  MODE rig_mode(void) const;

/*! \brief      Set mode
    \param  m   new mode

    If not a K3, then also sets the bandwidth (because it's easier to follow hamlib's model, even though it is obviously flawed)
*/
  void rig_mode(const MODE m);

/*! \brief      Is the rig in TEST mode?
    \return     whether the rig is currently in TEST mode
*/
  bool test(void) const;

/*! \brief      Explicitly put the rig into or out of TEST mode
    \param  b   whether to enter TEST mode

    This works only with the K3.
*/
  void test(const bool) const;

/*! \brief      Set rit offset (in Hz)
    \param  hz  offset in Hz
*/
  void rit(const int hz) const;

/// get rit offset (in Hz)
  int rit(void) const;

/*! \brief  Turn rit on

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  void rit_enable(void) const;

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  void rit_disable(void) const;

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  inline void disable_rit(void) const
    { rit_disable(); }

/// turn rit on
  inline void enable_rit(void) const
    { rit_enable(); }

/// is rit enabled?
  bool rit_enabled(void) const;

/*! \brief      Set xit offset (in Hz)
    \param  hz  offset in Hz

    On the K3 this also sets the RIT (!)
*/
  void xit(const int hz) const;

/// get xit offset (in Hz)
  int xit(void) const;

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  void xit_enable(void) const;

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  void xit_disable(void) const;

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline void disable_xit(void) const
    { xit_disable(); }

/// is xit enabled?
  bool xit_enabled(void) const;

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline void enable_xit(void) const
    { xit_enable(); }

/// get the rig's frequency and mode
  rig_status status(void);                              // most recent rig status

/// is the VFO locked?
  bool is_locked(void) const;

/// lock the VFO
  void lock(void) const;

/// unlock the VFO
  void unlock(void) const;

/*! \brief      Turn sub-receiver on/off
    \param  b   turn sub-receiver on if TRUE, otherwise turn off
*/
  void sub_receiver(const bool torf) const;

/// is sub-receiver on?
  bool sub_receiver(void) const;

/// is sub-receiver on?
  inline bool sub_receiver_enabled(void) const
    { return sub_receiver(); }

/// enable the sub-receiver
  inline void sub_receiver_enable(void) const
    { sub_receiver(true); }

/// disable the sub-receiver
  inline void sub_receiver_disable(void) const
    { sub_receiver(false); }

/// toggle sub-receiver between on and off
  inline void sub_receiver_toggle(void) const
    { sub_receiver_enabled() ? sub_receiver_disable() : sub_receiver_enable(); }

/// toggle sub-receiver between on and off
  inline void toggle_sub_receiver(void) const
    { sub_receiver_toggle(); }

/// get the bandwidth in Hz
  int bandwidth(void) const;

/// get the bandwidth as a string, in Hz
  std::string bandwidth_str(void) const;

/*! \brief          Set the keyer speed
    \param  wpm     keyer speed in WPM
*/
  void keyer_speed(const int wpm) const;

/// get the keyer speed in WPM
  int keyer_speed(void) const;

// explicit K3 commands
/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  expectation     whether a response is expected
    \param  expected_len    expected length of response
    \return                 the response from the rig, or the empty string

    Currently any expected length is ignored; the routine looks for the concluding ";" instead
*/
  std::string raw_command(const std::string_view cmd, const RESPONSE expectation = RESPONSE::NOT_EXPECTED) const;

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \return     the rig's most recent frequency for bandmode <i>bm</i>

     Returns empty frequency() if the most recent frequency for the band and mode has not been set (should never happen) 
*/
  frequency get_last_frequency(const bandmode bm) const;

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  b   band
    \param  m   mode
    \return     the rig's most recent frequency for band <i>b</i> and mode <i>m</i>.

    Returns empty frequency() if the most recent frequency for the band and mode has not been set (should never happen)  
*/
  inline frequency get_last_frequency(const BAND b, const MODE m) const
    { return get_last_frequency( { b, m } ); }

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \param  f   frequency
*/
  void set_last_frequency(const bandmode bm, const frequency f);

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  b   band
    \param  m   mode
    \param  f   frequency
*/
  inline void set_last_frequency(const BAND b, const MODE m, const frequency f)
    { set_last_frequency( { b, m }, f ); }

/*! \brief Is the rig transmitting?

    With the K3, this is unreliable: the routine frequently takes the _error_alert() path, even if the rig is not transmitting.
    (This is, unfortunately, just one example of the basic unreliability of the K3 in responding to commands.)
*/
  bool is_transmitting(void) const;

/// pause until the rig is no longer transmitting
  void wait_until_not_busy(void) const;

/*! \brief                          Register a function for alerting the user
    \param  error_alert_function    pointer to function for alerting the user
*/
//  void register_error_alert_function(void (*error_alert_function)(const std::string&));
  void register_error_alert_function(void (*error_alert_function)(const std::string_view));

/*! \brief      Which VFO is currently used for transmitting?
    \return     the VFO that is currently set to be used when transmitting
*/
  VFO tx_vfo(void) const;

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
  void bandwidth_a(const unsigned int hz) const;

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
  inline void bandwidth(const unsigned int hz) const
    { bandwidth_a(hz); }

/*! \brief      Set the bandwidth of VFO B
    \param  hz  desired bandwidth, in Hz
*/
  void bandwidth_b(const unsigned int hz) const;

/// set RIT, split, sub-rx off
  void base_state(void) const;

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use

    Works only with K3
*/
  bool rx_ant(void) const;

/*! \brief          Control use of the RX antenna
    \param  torf    whether to use the RX antenna

    Works only with K3
*/
  void rx_ant(const bool torf) const;

/// toggle whether the RX antenna is in use
  inline void rx_ant_toggle(void) const
    { rx_ant(!rx_ant()); }

/// toggle whether the RX antenna is in use
  inline void toggle_rx_ant(void) const
    { rx_ant_toggle(); }

/*! \brief      Get audio centre frequency, in Hz, as a printable string
    \return     The audio centre frequency, in Hz, as a printable string

    Works only with K3
*/
  std::string centre_frequency_str(void) const;

/*! \brief      Get audio centre frequency, in Hz
    \return     The audio centre frequency, in Hz

    Works only with K3
*/
  unsigned int centre_frequency(void) const;

/*! \brief      Set audio centre frequency
    \param  fc  the audio centre frequency, in Hz

    Works only with K3
*/
  void centre_frequency(const unsigned int fc) const;

/*! \brief              Is notch enabled?
    \param  ds_result   previously-obtained result of a DS command, or empty string
    \return             whether notch is currently enabled

    Works only with K3
*/
  bool notch_enabled(const std::string_view ds_result = std::string { }) const;

/*! \brief              Toggle the notch status

    Works only with K3
*/
  void toggle_notch_status(void) const;

/*! \brief      Set the K3 command mode (either NORMAL or EXTENDED)
    \param  cm  command mode

    Works only with K3
*/
  void k3_command_mode(const K3_COMMAND_MODE cm);

/*! \brief      Get the K3 command mode (either NORMAL or EXTENDED)
    \return     the K3 command mode

    Works only with K3
*/
  K3_COMMAND_MODE k3_command_mode(void) const;

/*! \brief          Emulate the tapping or holding of a K3 button
    \param  n       the K3 button to tap or hold
    \param  torh    whether to press or hold

    Works only with K3
*/
  void k3_press_button(const K3_BUTTON n, const PRESS torh) const;

/*! \brief      Emulate the tapping of a K3 button
    \param  n   the K3 button to tap

    Works only with K3
*/
  inline void k3_tap(const K3_BUTTON n) const
    { k3_press_button(n, PRESS::TAP); }

/*! \brief      Emulate the holding of a K3 button
    \param  n   the K3 button to hold

    Works only with K3
*/
  inline void k3_hold(const K3_BUTTON n) const
    { k3_press_button(n, PRESS::HOLD); }

/*! \brief      Set audio centre frequency and width
    \param  af  the characteristics to set 
*/
  void filter(const audio_filter& af) const;

/*! \brief      Turn on instrumentation
*/
  void instrument(void);

/*! \brief      Turn off instrumentation
*/
  void uninstrument(void);

/*! \brief      Is instrumentation turned on?
    \return     whether instrumentation is turned on
*/
  inline bool instrumented(void)
    { return _instrumented ; }
};

/*! \brief      Convert a hamlib error code to a printable string
    \param  e   hamlib error code
    \return     Printable string corresponding to error code <i>e</i>
*/
std::string hamlib_error_code_to_string(const int e);

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(rig_interface_error);   ///< errors related to accessing the rig

#endif /* RIG_INTERFACE_H */
