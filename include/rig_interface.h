// $Id: rig_interface.h 295 2026-05-17 12:40:09Z  $

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
#include "diskfile.h"
#include "drlog_context.h"
#include "hamlib.h"
#include "macros.h"
#include "pthread_support.h"
#include "string_functions.h"
#include "x_error.h"

#include <chrono>
#include <map>
#include <meta>
#include <string>
#include <utility>
#include <variant>

#include <hamlib/rig.h>

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

/// rig capabilities
enum class RIG_CAPABILITY { VFO_A = 0,          ///< has VFO A    // unbelievably, hamlib doesn't have standard naming; it calls this "Main" on some rigs
                            VFO_B,              ///< has VFO B    // unbelievably, hamlib doesn't have standard naming; it calls this "Sub" on some rigs
                            RIT,                ///< has RIT
                            XIT,                ///< has XIT
                            EQUAL_RIT_XIT_QRG,  ///< single frequency covers both RIT and XIT
                            SPLIT,              ///< can split; TX is VFO B; RX is VFO A
                            REVERSE_SPLIT,      ///< can split; TX is VFO A; RX is VFO B
                            LOCK_A,             ///< VFO A can be locked
                            LOCK_B,             ///< VFO B can be locked
                            SUB_RX,             ///< has a sub-receiver
                            TEST,               ///< has a TEST mode that inhibits transmission
                            RX_ANT,             ///< has a receive antenna
                            AUTO_NOTCH,         ///< has an automatic notch filter (for SSB)
                            AUDIO_BW,           ///< audio bandwidth can be controlled
                            AUDIO_CENTRE,       ///< centre frequency of audio can be controlled
                            BANDSCOPE           ///< has a controllable band scope
                          };

/*  So far, I have been unable to figure out a way to use reflection to generate a mapping between the name
    of a RIG_MODEL enum and its integer representation so that I can say, for example:
          auto value = name_to_enum_map[name]
    There are plenty of somewhat similar examples, but nothing that actually helps. The problem is that
    a consteval function can't return a map, because it's not a literal type; I'm unsure how to get around that.
    (And I've tried a LOT of attempted work-arounds.)

    https://stackoverflow.com/questions/21787488/can-have-definition-variable-of-non-literal-type-in-constexpr-function-body-c1

    (One can create and use a map, but it has to be deleted at compile time.)

    I'd be delighted to be told how to make this (or something equivalent) work.

    Actually, it's even worse: one can't return a simple ordinary std::string from a consteval function. This, as far
    as I can tell, means that a huge percentage of the cases where one might think that reflection would be of help
    in fact can't be implemented.

    So, far from being "a completely new language", C++ with reflection appears to be, in its current state, something not
    even worth thinking about except in exceptional cases. I have looked at the places where I (reluctantly) use macros in the current drlog
    code, and found only one place where the macro can be replaced by reflection -- and that's only because I'm forced to interface with
    the C-style programming in hamlib (which is ironic, as hamlib heavily uses macros... so one still ends up having to use macros *somewhere*).

    Reflection seems therefore to join the growing list of recently-added C++ features that sound great at first blush, but in the end turn out to
    be troublesome to the point of being not worth the effort except in a small percentage of cases where one would naively have expected
    them to be useful.

*/

using DRLOG_CLOCK = std::chrono::system_clock;

// ---------------------------------------- rig_status -------------------------

WRAPPER_2(rig_status, frequency, freq, MODE, mode);     ///< the status of a rig

// --------------------------------------- rig_capabilities ----------------------

/*! \class  rig_capabilities
    \brief  The capabilities of a rig

    All of this will greatly benefit from reflection when that is available (which should be gcc 16)
*/
class rig_capabilities
{ using INT_TYPE = uint32_t;    // 32 capabilities; could rework to use std::bitset if we ever need >64 capabilities

protected:

  INT_TYPE _caps { 0 };         // the capabilities as a mask; default to no capabilities

public:

  rig_capabilities(void) = default;

/*  \brief          Construct from Hamlib capabilities
    \param  hcaps   Hamlib capabilities


    There are several useful capabilities that Hamlib appears not to support:
      SPLIT
      RX_ANT
      TEST
      AUDIO_BW
      AUDIO_CENTRE

    I could well be wrong about some or all of these, however. I find much of the hamlib code to be, at best arcane,
    at worst incomprehensible to this non-C programmer. (The last C program I wrote would have been in 1988, immediately
    before I acquired a copy of the cfront (v 1.2) C++-to-C translator.
*/
  explicit rig_capabilities(const hamlib_capabilities& hcaps);

/*! \brief        Construct from a file
    \param  path  path(s) to search for the file <i>fn</i>
    \param  fn    filename containing capabilities

    The file <i>fn</i> should contain named rig capabilities, one per line: something like:
      VFO_A
      VFO_B
      LOCK_A
      LOCK_B
      etc.
*/
  rig_capabilities(const std::vector<std::string>& path, const std::string_view fn);

/*! \brief        Construct from a file
    \param  fn    filename containing capabilities

    The file <i>fn</i> should contain named rig capabilities, one per line: something like:
      VFO_A
      VFO_B
      LOCK_A
      LOCK_B
      etc.
*/
  explicit rig_capabilities(const std::string_view fn);

/*! \brief      Construct from a container of rig capabilities
    \param  s   container of capabilities
*/
  template <typename C>
    requires std::is_same_v<typename C::value_type, RIG_CAPABILITY>
  explicit inline rig_capabilities(const C& s)
    { FOR_ALL(s, [this] (const RIG_CAPABILITY rc) { set(rc); }); }

/*  I don't believe that there's any way to replace the following macro with reflection in C++ 26.
    I'd be happy to be proved wrong.
*/
#define FNS(y)  \
  inline bool y(void) const \
    { return ( _caps bitand (static_cast<INT_TYPE>(1) << static_cast<INT_TYPE>(RIG_CAPABILITY::y)) ); } \
\
  inline void y##_set(void) \
    { _caps = _caps bitor (static_cast<INT_TYPE>(1) << static_cast<INT_TYPE>(RIG_CAPABILITY::y)); } \
\
  inline void y##_clear(void) \
    { _caps = _caps ^ (static_cast<INT_TYPE>(1) << static_cast<INT_TYPE>(RIG_CAPABILITY::y)); }   // ^ is bitwise XOR

    FNS(VFO_A);
    FNS(VFO_B);
    FNS(RIT);
    FNS(XIT);
    FNS(EQUAL_RIT_XIT_QRG);
    FNS(SPLIT);
    FNS(REVERSE_SPLIT);
    FNS(LOCK_A);
    FNS(LOCK_B);
    FNS(SUB_RX);
    FNS(TEST);
    FNS(RX_ANT);
    FNS(AUTO_NOTCH);
    FNS(AUDIO_BW);
    FNS(AUDIO_CENTRE);
    FNS(BANDSCOPE);

#undef FNS

/*  \brief      Set a single capability
    \param  rc  capability to set
*/
  void set(const RIG_CAPABILITY rc);

/*  \brief      Clear a single capability
    \param  rc  capability to clear
*/
  void clear(const RIG_CAPABILITY rc);

/// Return whether all capabilities are unset
  inline bool empty(void) const
    { return !_caps; }

/// convert to human-readable string
  std::string to_string(void) const;
};

// --------------------------------------- polled_status ----------------------

/*! \class  polled_status
    \brief  The required status information for a rig, polled once per second in drlog.cpp
*/

class polled_status
{
protected:

  frequency _f_a { };      ///< frequency of VFO A
  frequency _f_b { };      ///< frequency of VFO B

  bool      _rit_enabled { false };   ///< whether RIT is enabled
  bool      _xit_enabled { false };   ///< whether XIT is enabled

  int       _rit_offset { 0 };        ///< RIT offset, in Hz
  int       _xit_offset { 0 };        ///< XIT offset, in Hz

  bool      _split_enabled  { false };   ///< whether split is enabled
  bool      _is_locked      { false };   ///< whether the VFO is locked
  bool      _sub_rx_enabled { false };   ///< whether the sub receiver is enabled
  bool      _test_mode      { false };   ///< whether the rig is in TEST mode
  bool      _rx_ant         { false };   ///< whether the receive antenna is enabled

  std::string _mode_str { "UNK"s };     ///< current mode of the rig, as a string

  MODE      _m;            ///< mode

  bool _notch { false };  ///< whether auto-notch is enabled

  std::string _bw_str     { };  ///< the audio bandwidth, in Hz
  std::string _centre_str { };  ///< the centre of the audio passband, in Hz

public:

  READ_AND_WRITE(f_a);               ///< frequency of VFO A
  READ_AND_WRITE(f_b);               ///< frequency of VFO B
  READ_AND_WRITE(m);                 ///< mode
  READ_AND_WRITE(notch);             ///< whether auto-notch is enabled
  READ_AND_WRITE(mode_str);          ///< current mode of the rig, as a string
  READ_AND_WRITE(rit_enabled);       ///< whether RIT is enabled
  READ_AND_WRITE(xit_enabled);       ///< whether XIT is enabled
  READ_AND_WRITE(rit_offset);        ///< RIT offset, in Hz
  READ_AND_WRITE(xit_offset);        ///< XIT offset, in Hz
  READ_AND_WRITE(split_enabled);     ///< whether split is enabled
  READ_AND_WRITE(bw_str);            ///< the audio bandwidth, in Hz
  READ_AND_WRITE(centre_str);        ///< the centre of the audio passband, in Hz
  READ_AND_WRITE(is_locked);         ///< whether the VFO is locked
  READ_AND_WRITE(sub_rx_enabled);    ///< whether the sub receiver is enabled
  READ_AND_WRITE(test_mode);         ///< whether the rig is in TEST mode
  READ_AND_WRITE(rx_ant);            ///< whether the receive antenna is enabled

/// polled_status == polled_status
  bool operator==(const polled_status& ps)  const = default;

/// other comparisons
  bool operator<=>(const polled_status& ps) const = default;

/// convert to human-readable string
  std::string to_string(void) const;
};

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

    This is a base class for interacting with rigs. It uses hamlib, despite
    my grave reservations about the quality of that library, as hamlib "supports" a vast number
    of different rigs. This allows at least basic control for those rigs.

    Derived classes with better implementations of various functions can then be used for particular rigs as
    the need arises (currently just the Elecraft K3, as that is the rig that I have).
*/

class rig_interface
{
protected:

  hamlib_capabilities                     _hcaps                       { };                             ///< capabilities from hamlib (which may or may not be populated and used)
  bool                                    _instrumented                { false };                       ///< whether to record all exchanges with rig
  frequency                               _last_commanded_frequency    { 14_MHz };                      ///< last frequency to which the rig was commanded to QSY
  frequency                               _last_commanded_frequency_b  { 14_MHz };                      ///< last frequency to which VFO B was commanded to QSY
  MODE                                    _last_commanded_mode         { MODE_CW };                     ///< last mode into which the rig was commanded
  std::unordered_map<bandmode, frequency> _last_frequency              { };                             ///< last-used frequencies on per-band, per-mode basis
  rig_model_t                             _model                       { RIG_MODEL_DUMMY };             ///< hamlib model
  hamlib_port_t                           _port;                                                        ///< hamlib port
  std::string                             _port_name                   { };                             ///< name of port
  rig_capabilities                        _rcaps                       { };                             ///< rig capabiliies
  RIG*                                    _rigp                        { nullptr };                     ///< hamlib handle
  bool                                    _rig_connected               { false };                       ///< is a rig connected?
  pthread_t                               _thread_id;                                                   ///< ID for the thread that polls the rig for status

  mutable pt_mutex                        _rig_mutex                   { "RIG INTERFACE"s };            ///< mutex for all operations

  polled_status                           _status                      { };                             ///< most recent result from a poll of the rig

// protected pointers to functions

/*! \brief          Pointer to function used to alert the user to an error
    \param  msg     message to be presented to the user
*/
  void (*_error_alert_function)(const std::string_view msg) { nullptr };

// protected functions

/*! \brief       Alert the user with a message
    \param  msg  message for the user

    Calls <i>_error_alert_function</i> to perform the actual alerting
*/
  void _error_alert(const std::string_view msg) const;

/*! \brief      Allow direct access to the underlying file descriptor used to communicate with the rig
    \return     the file descriptor associated with the rig
*/
  inline int _file_descriptor(void) const
    { return (_rigp -> state.rigport.fd); }

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band.
    Attempts to confirm that the frequency was actually set to <i>f</i>.
*/
  virtual void _rig_frequency(const frequency, const VFO v);

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
  virtual frequency _rig_frequency(const VFO v) const;

public:

/// default constructor
  rig_interface(void) = default;

// no copy constructor
  rig_interface(const rig_interface&) = delete;

// destructor
  inline virtual ~rig_interface(void)
    { }

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
  virtual void prepare(const drlog_context& context);

  SAFE_READ_AND_WRITE(hcaps, _rig);
  SAFE_READ_AND_WRITE(rcaps, _rig);


/*  I don't believe that there's any way to replace the following macro with reflection in C++ 26.
    I'd be happy to be proved wrong.
*/
// note that this does not lock the object, because there really doesn't seem to be any need for that
#define HAS_CAPABILITY(y) \
  inline bool y(void) const \
    { return _rcaps.y(); }

  HAS_CAPABILITY(VFO_A);
  HAS_CAPABILITY(VFO_B);
  HAS_CAPABILITY(RIT);
  HAS_CAPABILITY(XIT);
  HAS_CAPABILITY(EQUAL_RIT_XIT_QRG);
  HAS_CAPABILITY(SPLIT);
  HAS_CAPABILITY(REVERSE_SPLIT);
  HAS_CAPABILITY(LOCK_A);
  HAS_CAPABILITY(LOCK_B);
  HAS_CAPABILITY(SUB_RX);
  HAS_CAPABILITY(TEST);
  HAS_CAPABILITY(RX_ANT);
  HAS_CAPABILITY(AUTO_NOTCH);
  HAS_CAPABILITY(AUDIO_BW);
  HAS_CAPABILITY(AUDIO_CENTRE);
  HAS_CAPABILITY(BANDSCOPE);

#undef HAS_CAPABILITY

/*! \brief    Populate the hamlib capabilities, according to hamlib
    \return
*/
  inline void get_hamlib_capabilities(void)
    { _hcaps.get_capabilities(_rigp); }

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

/*! \brief            Set the span of the bandscope
    \param  khz_span  bandscope span, in kHz
*/
  inline virtual void bandscope_span([[maybe_unused]] const unsigned int khz_span) const
    { if (BANDSCOPE())                                                // do nothing if we don't have a bandscope
        _error_alert("Unimplemented function: rig_interface::bandscope_span(const unsigned int)"sv);
    }

/*! \brief      Record an image of the bandscope
    \param  fn  name of the image file
*/
  inline virtual void bandscope_screenshot([[maybe_unused]] const std::string fn)
    { if (BANDSCOPE())                                                // do nothing if we don't have a bandscope
        _error_alert("Unimplemented function: rig_interface::bandscope_screenshot(const string)"sv);
    }

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
  virtual void split_enable(void) const;

/// disable split operation; see caveats under split_enable()
  virtual void split_disable(void) const;

/*! \brief      Is split enabled?
    \return     whether split is enabled on the rig

    This interrogates the rig; it neither reads not writes the variable rig_is_split
*/
  virtual bool split_enabled(void) const;

/// get mode
  virtual MODE rig_mode(void) const;

/*! \brief      Set mode
    \param  m   new mode

    Also sets the bandwidth (because it's easier to follow hamlib's model, even though it is obviously flawed)
*/
  virtual void rig_mode(const MODE m);

/*! \brief      Is the rig in TEST mode?
    \return     whether the rig is currently in TEST mode
*/
  virtual bool test(void) const;

/*! \brief      Explicitly put the rig into or out of TEST mode
    \param  b   whether to enter TEST mode
*/
  virtual void test(const bool) const;

/*! \brief      Set rit offset (in Hz)
    \param  hz  offset in Hz
*/
  virtual void rit(const int hz) const;

/// get rit offset (in Hz)
  virtual int rit(void) const;

/*! \brief  Turn rit on

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  inline virtual void rit_enable(void) const
    { rit(1); }                      // 1 Hz offset, since a zero offset would disable RIT in hamlib [!]

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  inline virtual void rit_disable(void) const
    { rit(0); }                     // 0 Hz offset, which hamlib regards as disabling RIT [!]

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
  inline virtual void disable_rit(void) const
    { rit_disable(); }

/// turn rit on
  inline virtual void enable_rit(void) const
    { rit_enable(); }

/// is rit enabled? -- hamlib equates this to non-zero RIT offset
  inline virtual bool rit_enabled(void) const
    { return ( rit() != 0 ); }

/*! \brief      Set xit offset (in Hz)
    \param  hz  offset in Hz
*/
  virtual void xit(const int hz) const;

/// get xit offset (in Hz)
  int xit(void) const;

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline virtual void xit_enable(void) const
    { xit(1); }                 // 1 Hz offset

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline virtual void xit_disable(void) const
    { xit(0); }

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates a zero offset with xit disabled (!)
*/
  inline void disable_xit(void) const
    { xit_disable(); }

/// is xit enabled? -- hamlib equates this to non-zero XIT offset
  inline virtual bool xit_enabled(void) const
    { return ( xit() != 0 ); }

/*! \brief  Turn xit on
*/
  inline void enable_xit(void) const
    { xit_enable(); }

/// is the VFO locked?
  virtual bool is_locked(void) const;

/// lock the VFO
  virtual void lock(void) const;

/// unlock the VFO
  virtual void unlock(void) const;

/*! \brief      Turn sub-receiver on/off
    \param  b   turn sub-receiver on if TRUE, otherwise turn off
*/
  virtual void sub_receiver(const bool torf) const;

/// is sub-receiver on?
  virtual bool sub_receiver(void) const;

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

/// get the audio bandwidth in Hz
  virtual inline int bandwidth(void) const
    { return ( _error_alert("Unimplemented function: rig_interface::bandwidth()"sv), 0 ); }

/// get the audio bandwidth in hertz as a string
  inline std::string bandwidth_str(void) const
   { return ::to_string(bandwidth()); }

/*! \brief          Set the keyer speed
    \param  wpm     keyer speed in WPM
*/
  virtual void keyer_speed(const int wpm) const;

/// get the keyer speed in WPM
  virtual int keyer_speed(void) const;

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
*/
  inline virtual bool is_transmitting(void) const
    { return false; }   // don't call _error_alert(), because this can get called a lot

/// pause until the rig is no longer transmitting
  void wait_until_not_busy(void) const;

/*! \brief                          Register a function for alerting the user
    \param  error_alert_function    pointer to function for alerting the user
*/
  void register_error_alert_function(void (*error_alert_function) (const std::string_view));

/*! \brief      Which VFO is currently used for transmitting?
    \return     the VFO that is currently set to be used when transmitting
*/
  VFO tx_vfo(void) const;

/*! \brief      Set the audio bandwidth of VFO A
    \param  hz  desired audio bandwidth, in Hz
*/
  inline virtual void bandwidth_a([[maybe_unused]] const unsigned int hz) const
    { _error_alert("Unimplemented function: rig_interface::bandwidth_a(const unsigned int)"sv); }

/*! \brief      Set the audio bandwidth of VFO A
    \param  hz  desired audio bandwidth, in Hz
*/
  inline void bandwidth(const unsigned int hz) const
    { bandwidth_a(hz); }

/*! \brief      Set the audio bandwidth of VFO B
    \param  hz  desired audio bandwidth, in Hz
*/
  inline virtual void bandwidth_b([[maybe_unused]] const unsigned int hz) const
    { _error_alert("Unimplemented function: rig_interface::bandwidth_b(const unsigned int)"sv); }

/// set RIT, split and sub-rx off
  virtual void base_state(void) const;

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use
*/
  inline virtual bool rx_ant(void) const
    { return (_error_alert("Unimplemented function: rig_interface::rx_ant()"sv), false); }

/*! \brief          Control use of the RX antenna
    \param  torf    whether to use the RX antenna
*/
  inline virtual void rx_ant([[maybe_unused]] const bool torf) const
    { _error_alert("Unimplemented function: rig_interface::rx_ant(const bool)"sv); }

/// toggle whether the RX antenna is in use
  inline void rx_ant_toggle(void) const
    { rx_ant(!rx_ant()); }

/// toggle whether the RX antenna is in use
  inline void toggle_rx_ant(void) const
    { rx_ant_toggle(); }

/*! \brief      Get audio centre frequency, in Hz, as a printable string
    \return     The audio centre frequency, in Hz, as a printable string
*/
  inline virtual std::string centre_frequency_str(void) const
    { return ::to_string(centre_frequency()); }

/*! \brief      Get audio centre frequency, in Hz
    \return     The audio centre frequency, in Hz
*/
  inline virtual unsigned int centre_frequency(void) const
    { return (_error_alert("Unimplemented function: rig_interface::centre_frequency()"sv), 0); }

/*! \brief      Set audio centre frequency
    \param  fc  the audio centre frequency, in Hz
*/
  inline virtual void centre_frequency([[maybe_unused]] const unsigned int fc) const
    { _error_alert("Unimplemented function: rig_interface::centre_frequency(const unsigned int)"sv); }

/*! \brief              Is notch enabled?
    \param  ds_result   previously-obtained result of a K3 DS command, or empty string
    \return             whether notch is currently enabled
*/
  inline virtual bool notch_enabled([[maybe_unused]] const std::string_view ds_result = std::string { }) const
    { return (_error_alert("Unimplemented function: rig_interface::notch_enabled()"sv), false); }

/*! \brief              Toggle the notch status
*/
  virtual void toggle_notch_status(void) const;

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

/*! \brief                  Put the current VFOs into a rig memory
    \param  rig_memory_nr   the number of the rig memory into which the VFO frequencies are to be stored (0 <= value <= 99)

    This works only on a K3.

    The documentation for the K3 MC command doesn't explain the difference between setting and getting a memory
*/
//  void set_rig_memory(const int rig_memory_nr = 0) const;

/*! \brief                  Set the VFOs from a rig memory
    \param  rig_memory_nr   the number of the rig memory from which the VFO frequencies are to be (non-destructively) recalled (0 <= value <= 99)

    This works only on a K3.

    The documentation for the K3 MC command doesn't explain the difference between setting and getting a memory
*/
//  void get_rig_memory(const int rig_memory_nr = 0) const;

/*
 Table 7 from the K3 Programmers Reference Manual

 This version of the table is re-ordered in increasing value of nn:

TAP              HOLD        nn
DISP             METER       08
BAND-            VOX         09
BAND+            QSK         10
A/B (1)          BSET        11
REV (FM/rpt) (2) n/a         12
A->B (3)         SPLIT       13
MENU             CONFIG      14
V->M             AF REC      15
XMIT             TUNE        16
MODE-            ALT         17
MODE+            TEST        18
ATU              Tune ATU    19
M1               M1-RPT      21
M->V             AF PLAY     23
PRE (4)          ATT         24
RX ANT           n/a         25
ANT              ANT Name    26
AGC (5)          OFF         27
XFIL (6)         DUAL PB/APF 29
M2               M2-RPT      31
NTCH (9)         MANUAL      32
NB (7)           LEVEL       33
NR (8)           ADJ         34
M3               M3-RPT      35
REC              MSG Bank    37
M4               M4-RPT      39
CWT (0)          TEXT Dec    40
FREQ             ENT SCAN    41
SPOT (‘.’)       PITCH       42
AFX (<-)         DATA Md     43
RIT              PF1         45
XIT              PF2         47
SUB              DVRSTY*     48
FINE             COARSE      49
RATE             LOCK        50
CLR              n/a         53
CMP/PWR          MON         56
SPD/MIC          DELAY       57
SHIFT/LO         NORM        58
WIDTH/HI         I/II        59

*/

/// Poll the rig for the important status information
  virtual polled_status poll(void);

};

// ---------------------------------------- Elecraft K3 -------------------------

enum class K3_COMMAND_MODE { NORMAL,
                             EXTENDED
                           };

enum class K3_BUTTON_TAP { DISP       = 8,
                           BAND_MINUS = 9,
                           BAND_PLUS  = 10,
                           AB         = 11,
                           ONE        = 11,
                           REV        = 12,
                           TWO        = 12,
                           A_TO_B     = 13,
                           THREE      = 13,
                           MENU       = 14,
                           V_TO_M     = 15,
                           XMIT       = 16,
                           MODE_MINUS = 17,
                           MODE_PLUS  = 18,
                           ATU        = 19,
                           M1         = 21,
                           M_TO_V     = 23,
                           PRE        = 24,
                           FOUR       = 24,
                           RX_ANT     = 25,
                           ANT        = 26,
                           AGC        = 27,
                           FIVE       = 27,
                           XFIL       = 29,
                           SIX        = 29,
                           M2         = 31,
                           NTCH       = 32,
                           NOTCH      = NTCH,
                           NINE       = 32,
                           NB         = 33,
                           SEVEN      = 33,
                           NR         = 34,
                           EIGHT      = 34,
                           M3         = 35,
                           REC        = 37,
                           M4         = 39,
                           CWT        = 40,
                           ZERO       = 40,
                           FREQ       = 41,
                           SPOT       = 42,
                           DOT        = 42,
                           AFX        = 43,
                           LEFT_ARROW = 44,
                           RIT        = 45,
                           XIT        = 47,
                           SUB        = 48,
                           FINE       = 49,
                           RATE       = 50,
                           CLR        = 53,
                           CMP_PWR    = 56,
                           SPD_MIC    = 57,
                           SHIFT_LO   = 58,
                           WIDTH_Hi   = 59
                         };

enum class K3_BUTTON_HOLD { METER       = 8,
                            VOX         = 9,
                            QSK         = 10,
                            BSET        = 11,
                            SPLIT       = 13,
                            CONFIG      = 14,
                            AF_REC      = 15,
                            TUNE        = 16,
                            ALT         = 17,
                            TEST        = 18,
                            TUNE_ATU    = 19,
                            M1_RPT      = 21,
                            AF_PLAY     = 23,
                            ATT         = 24,
                            ANT_NAME    = 26,
                            OFF         = 27,
                            DUAL_PB_APF = 29,
                            M2_RPT      = 31,
                            MANUAL      = 32,
                            LEVEL       = 33,
                            ADJ         = 34,
                            M3_RPT      = 35,
                            MSG_BANK    = 37,
                            M4_RPT      = 39,
                            TEXT_DEC    = 40,
                            ENT_SCAN    = 41,
                            PITCH       = 42,
                            DATA_MD     = 43,
                            PF1         = 45,
                            PF2         = 47,
                            DVRSTY      = 48,
                            DIVERSITY   = DVRSTY,
                            COARSE      = 49,
                            LOCK        = 50,
                            MON         = 56,
                            DELAY       = 57,
                            NORM        = 58,
                            I_II        = 59
                         };

enum class K3_OPERATING_MODE { LSB = 1,         // 1 (LSB), 2 (USB), 3 (CW), 4 (FM), 5 (AM), 6 (DATA), 7 (CWREV), 9 (DATA-REV) [there is no 8]
                               USB = 2,
                               CW = 3,
                               FM = 4,
                               AM = 5,
                               DATA = 6,
                               CWREV = 7,
                               DATA_REV = 9
                             };

// --------------------------------------- k3_status ----------------------

/*! \class  k3_status
    \brief  The status of a K3, as determined by the response from an IF; command

From the K3 Programmer's Manual:

IF (Transceiver Information; GET only)
RSP format: IF[f]*****+yyyyrx*00tmvspbd1*; where the fields are defined as follows:
[f] Operating frequency, excluding any RIT/XIT offset (11 digits; see FA command format)
* represents a space (BLANK, or ASCII 0x20)
+ either "+" or "-" (sign of RIT/XIT offset)
yyyy RIT/XIT offset in Hz (range is -9999 to +9999 Hz when computer-controlled)
r 1 if RIT is on, 0 if off
x 1 if XIT is on, 0 if off
t 1 if the K3 is in transmit mode, 0 if receive
m operating mode (see MD command)
v receive-mode VFO selection, 0 for VFO A, 1 for VFO B
s 1 if scan is in progress, 0 otherwise
p 1 if the transceiver is in split mode, 0 otherwise
b Basic RSP format: always 0; K2 Extended RSP format (K22): 1 if present IF response
is due to a band change; 0 otherwise
d Basic RSP format: always 0; K3 Extended RSP format (K31): DATA sub-mode,
if applicable (0=DATA A, 1=AFSK A, 2= FSK D, 3=PSK D)
The fixed-value fields (space, 0, and 1) are provided for syntactic compatibility with existing software.
*/

class k3_status
{
protected:

  frequency         _freq           { };                        ///< "operating frequency, excluding any RIT/XIT offset"
  bool              _rit_positive   { true };                   ///< "(sign of RIT/XIT offset)"
  frequency         _abs_rit_offset { 0 };                      ///< "RIT/XIT offset in Hz (range is -9999 to +9999 Hz when computer-controlled)"
  bool              _rit_is_on      { false };                  ///< whether RIT is "on"
  bool              _xit_is_on      { false };                  ///< whether XIT is "on"
  bool              _transmit       { false };                  ///< whether rig is "in transmit mode"
  K3_OPERATING_MODE _op_mode        { K3_OPERATING_MODE::CW };  ///< "operating mode"
  VFO               _rx_vfo         { VFO::A };                 ///< "receive mode VFO"
  bool              _scanning       { false };                  ///< whether the rig is performing a scan
  bool              _is_split       { false };                  ///< whether the rig is split

  bool              _valid          { false };                  ///< only valid if the initialisation data look OK

public:

/*! Construct from the response to an "IF;" command
*/
  explicit k3_status(const std::string_view rsp);

  READ(freq);                 // "operating frequency, excluding any RIT/XIT offset"
  READ(rit_positive);         // "(sign of RIT/XIT offset)"
  READ(abs_rit_offset);       // "RIT/XIT offset in Hz (range is -9999 to +9999 Hz when computer-controlled)"
  READ(rit_is_on);            // whether RIT is "on"
  READ(xit_is_on);            // whether XIT is "on"
  READ(transmit);             // whether rig is "in transmit mode"
  READ(op_mode);              // "operating mode"
  READ(rx_vfo);               // "receive mode VFO"
  READ(scanning);             // whether the rig is performing a scan
  READ(is_split);             // whether the rig is split

  READ(valid);                // only valid if the initialisation data look OK

/// return whether RIT is off
  inline bool rit_is_off(void) const
    { return !rit_is_on(); }

/// return whether XIT is off
  inline bool xit_is_off(void) const
    { return !xit_is_on(); }

/// return whether the object is valid
  inline operator bool(void) const
    { return _valid; }

/// return whether the R/XIT direction is positive or negative
  inline char rit_dirn_char(void) const
    { return (_rit_positive ? '+' : '-'); }

/// return R/XIT value in Hz as a string, including the direction
  inline std::string rit_str(void) const
    { return rit_dirn_char() + ::to_string(_abs_rit_offset.hz()); }
};

// ---------------------------------------- elecraft_k3_interface -------------------------

/*! \class  elecraft_k3_interface
    \brief  The interface to a K3 rig
*/

class elecraft_k3_interface : public rig_interface
{
protected:

  bool _p3_ignore_checksum_error { false };         ///< should checksum errors be ignored when acquiring P3 screendumps?

/*! \brief      Thread used to record an image of the bandscope
    \param  fn  name of the image file
*/
  void _bandscope_screenshot_thread(const std::string fn);

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band.
    Attempts to confirm that the frequency was actually set to <i>f</i>.
*/
  void _rig_frequency(const frequency f, const VFO v);

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
  frequency _rig_frequency(const VFO v) const;

public:

/*! \brief                    Constructor
    \param  p3_ignore_error   whether to ignore checksum errors if a P3 is present
*/
  inline explicit elecraft_k3_interface(const bool p3_ignore_error = false) :
    rig_interface(),
    _p3_ignore_checksum_error(p3_ignore_error)
  { }

/// destructor
  inline ~elecraft_k3_interface(void)
    { }

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
  void prepare(const drlog_context& context);

/*! \brief      Set mode
    \param  m   new mode
*/
  void rig_mode(const MODE m);

/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  expectation     whether a response is expected
    \return                 the response from the rig, or the empty string
*/
  std::string raw_command(const std::string_view cmd, const RESPONSE expectation = RESPONSE::NOT_EXPECTED) const;

/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  timeout         the time to wait for a response
    \param  n_retries       number of retries
    \return                 the response from the rig, or the empty string
*/
  std::string _retried_raw_command(const std::string_view cmd, const std::chrono::milliseconds timeout = 250ms, const int n_retries = 0);

/*! \brief              Set the span of a P3
    \param  khz_span    span in kHz
*/
  void bandscope_span(const unsigned int khz_span) const;

/*! \brief      Record an image of the bandscope
    \param  fn  name of the image file
*/
  inline void bandscope_screenshot(const std::string fn)
    { std::jthread(&elecraft_k3_interface::_bandscope_screenshot_thread, this, fn).detach(); }    // obtain the screenshot in a separate thread

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

/*! \brief      Set rit offset (in Hz)
    \param  hz  offset in Hz
*/
  void rit(const int hz) const;

/// get rit offset (in Hz)
  int rit(void) const;

/*! \brief  Turn rit on
*/
  inline void rit_enable(void) const
    { raw_command("RT1;"sv); }          // proper enable for the K3

/*! \brief  Turn rit off
*/
  inline void rit_disable(void) const
    { raw_command("RT0;"sv); }          // proper disable for the K3

/// is rit enabled?
  bool rit_enabled(void) const;

/*! \brief  Turn xit on
*/
  inline void xit_enable(void) const
    { raw_command("XT1;"sv); }

/*! \brief  Turn xit off
*/
  inline void xit_disable(void) const
    { raw_command("XT0;"sv); }

/// is xit enabled?
  bool xit_enabled(void) const;

/*! \brief      Set xit offset (in Hz)
    \param  hz  offset in Hz

    On the K3 this also sets the RIT (!)
*/
  void xit(const int hz) const;

/// lock the VFO
  inline void lock(void) const
    { raw_command("LK1;"sv); }

/// unlock the VFO
  inline void unlock(void) const
    { raw_command("LK0;"sv); }

/*! \brief      Turn sub-receiver on/off
    \param  b   turn sub-receiver on if TRUE, otherwise turn off
*/
  inline void sub_receiver(const bool torf) const
    { raw_command(torf ? "SB1;"s : "SB0;"s); }

/// is sub-receiver on?
  bool sub_receiver(void) const;

/*! \brief          Set the keyer speed
    \param  wpm     keyer speed in WPM
*/
  inline void keyer_speed(const int wpm) const
    { raw_command("KS"s + pad_leftz(wpm, 3) + SEMICOLON); }

/// get the keyer speed in WPM
  int keyer_speed(void) const;

/// lock the VFO
  bool is_locked(void) const;

/// get the bandwidth in Hz
  int bandwidth(void) const;

/*! \brief Is the rig transmitting?

    With the K3, this is unreliable: the routine frequently takes the _error_alert() path, even if the rig is not transmitting.
    (This is, unfortunately, just one example of the basic unreliability of the K3 in responding to commands.)
*/
  bool is_transmitting(void) const;

/*! \brief      Is the rig in TEST mode?
    \return     whether the rig is currently in TEST mode
*/
  bool test(void) const;

/*! \brief      Explicitly put the rig into or out of TEST mode
    \param  b   whether to enter TEST mode
*/
  void test(const bool) const;

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
  void bandwidth_a(const unsigned int hz) const;

/*! \brief      Set the bandwidth of VFO B
    \param  hz  desired bandwidth, in Hz
*/
  void bandwidth_b(const unsigned int hz) const;

/*! \brief      Get audio centre frequency, in Hz, as a printable string
    \return     The audio centre frequency, in Hz, as a printable string
*/
  std::string centre_frequency_str(void) const;

/*! \brief      Get audio centre frequency, in Hz
    \return     The audio centre frequency, in Hz
*/
  unsigned int centre_frequency(void) const;

/*! \brief      Set audio centre frequency
    \param  fc  the audio centre frequency, in Hz
*/
  void centre_frequency(const unsigned int fc) const;

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use
*/
  bool rx_ant(void) const;

/*! \brief          Control use of the RX antenna
    \param  torf    whether to use the RX antenna
*/
  void rx_ant(const bool torf) const;

/*! \brief              Is notch enabled?
    \param  ds_result   previously-obtained result of a DS command, or empty string
    \return             whether notch is currently enabled
*/
  bool notch_enabled(const std::string_view ds_result = std::string { }) const;

/*! \brief              Toggle the notch status
*/
  void toggle_notch_status(void) const;

/*! \brief      Set the K3 command mode (either NORMAL or EXTENDED)
    \param  cm  command mode
*/
  void k3_command_mode(const K3_COMMAND_MODE cm);

/*! \brief      Get the K3 command mode (either NORMAL or EXTENDED)
    \return     the K3 command mode
*/
  K3_COMMAND_MODE k3_command_mode(void) const;

/*! \brief          Emulate the tapping or holding of a K3 button
    \param  button  the K3 button to tap or hold
*/
  void k3_press(const std::variant<K3_BUTTON_TAP, K3_BUTTON_HOLD>& button) const;

/*! \brief          Emulate double-tapping a K3 button
    \param  button  the K3 button to tap
*/
  void k3_double_tap(const K3_BUTTON_TAP button) const;

/// set RIT, split, sub-rx off
  void base_state(void) const;

/// Poll the rig for the important status information
  polled_status poll(void);
};

/*! \brief      Convert a hamlib error code to a printable string
    \param  e   hamlib error code
    \return     Printable string corresponding to error code <i>e</i>
*/
std::string hamlib_error_code_to_string(const int e);

// -------------------------------------- Errors  -----------------------------------

ERROR_CLASS(rig_interface_error);   ///< errors related to accessing the rig

#endif /* RIG_INTERFACE_H */
