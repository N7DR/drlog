// $Id: rig_interface.cpp 289 2026-03-15 19:15:54Z  $

// Released under the GNU Public License, version 2
//   see: https://www.gnu.org/licenses/gpl-2.0.html

// Principal author: N7DR

// Copyright owners:
//    N7DR

/*! \file   rig_interface.cpp

    Classes and functions related to transferring information between the computer and the rig.

    The user must be a member of the dialout group in order to use the serial port.
*/

#include "bands-modes.h"
#include "drlog_context.h"
#include "rig_interface.h"
#include "string_functions.h"

//#include "hamlib/serial.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <cstring>

#include <termios.h>
#include <cstdio>
#include <fcntl.h>

#include <hamlib/rig.h>

// Hamlib changed the macro FILPATHLEN to HAMLIB_FILPATHLEN at some point
#if (!defined(HAMLIB_FILPATHLEN))

#define HAMLIB_FILPATHLEN FILPATHLEN

#endif

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

using namespace std::literals::chrono_literals;

extern bool debug;                             ///< debug frequency changes
extern bool rig_is_split;

extern void alert(const string_view msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);     ///< alert the user (not used for errors)

constexpr char SEMICOLON { ';' };

// --------------------------------------- polled_status ----------------------

/*! \class  polled_status
    \brief  The required status information for a rig, polled once per second in drlog.cpp
*/

string polled_status::to_string(void) const
{ string rv { };

  rv += "  VFO A: " + ::to_string(_f_a) + EOL;
  rv += "  VFO B: " + ::to_string(_f_b) + EOL;

  rv += "  RIT IS " + (_rit_enabled ? ""s : "NOT "s) + "enabled" + EOL;
  rv += "  XIT IS " + (_xit_enabled ? ""s : "NOT "s) + "enabled" + EOL;

  rv += "  split IS " + (_split_enabled ? ""s : "NOT "s) + "enabled" + EOL;
  rv += "  rig IS " + (_is_locked ? ""s : "NOT "s) + "locked" + EOL;
  rv += "  sub RX IS " + (_sub_rx_enabled ? ""s : "NOT "s) + "enabled" + EOL;
  rv += "  rig IS " + (_test_mode ? ""s : "NOT "s) + "in TEST mode" + EOL;
  rv += "  RX ant IS " + (_rx_ant ? ""s : "NOT "s) + "in USE" + EOL;

  rv += "  mode_str: " + _mode_str + EOL;
  rv += "  mode name: " + MODE_NAME[_m] + EOL;

  rv += "  notch IS " + (_notch ? ""s : "NOT "s) + "enabled" + EOL;

  rv += "  bw: " + _bw_str + EOL;
  rv += "  centre: " + _centre_str + EOL;



  return rv;
}


/* The current version of Hamlib seems to be both slow and unreliable with the K3. Anent unreliability, for example, the is_locked() function
 * as written below causes the entire program to freeze (presumably some kind of blocking or threading issue in the current version of hamlib).
 *
 * Anyway, as a result of this, we use explicit K3 control commands where appropriate. This may be changed if/when hamlib proves
 * itself sufficiently robust.
 *
 * Another issue is that there is simply no good detailed description of the expected behaviour corresponding to many hamlib
 * functions (for example, the split-related functions -- e.g., CURR_VFO is defined simply as "the current VFO", but it plainly is not,
 * as a few test function calls quickly demonstrate); nor, indeed, is there even a description of the theoretical transceiver that is modelled by
 * hamlib. There doesn't even appear to be a guarantee that if a function is "successful" (i.e., does not return an error), it
 * will behave identically on all rigs.
 *
 * There are other weirdnesses: polling the split status on the K3 actually SWAPS the frequencies of the two VFOs, even though
 * it is a *get* function. Life is too short to try to sort out all this stuff and to get it to work correctly, if that's even possible,
 * even just for the K3 -- especially as there is no guarantee that other rigs will respond the same way to the same (successful) calls.
 *
 * So, unfortunately, we end up using a lot of K3-specific calls. This was not my desire. But I got fed up of trying to make sense
 * of the current implementation of hamlib.
 */

/* But there are many problems with the K3 soi-disant protocol as well. Even after trying to get clear answers from Elecraft, the following
 * issues (at least) remain (after incorporating the Elecraft responses, which mostly did not address the questions I asked):
 *
 *   1. Which commands might cause a "?;" response if the K3 is "busy"?
 *   2. Do these include commands that don't normally return a response?
 *   3. Which commands do eventually get executed even if the K3 is "busy"?
 *   4. How long might one have to wait for some commands to execute if the K3 is "busy"?
 *   5. Which commands are subject to delay if the K3 is "busy"?
 *   6. What is the precise definition of "busy"?
 *
 *   All in all, the only thing to do seems to be to throw commands at the K3 and hope that they stick. It is impractical to
 *   check each time whether a command was executed, because the documentation says that certain commands (which it does not define)
 *   might take as long as half a second to execute if the K3 is "busy".
 *
 *   Sometimes, there is simply no alternative to sending a command and then getting the value to see if the command has actually
 *   executed on the rig. This is particularly so for the set/get frequency commands, as drlog requires a correct view of the band,
 *   including the rig's frequency, in order to build an accurate, stable bandmap.
 *
 *   The fundamental problem in all this is that the protocol has no concept of a transaction. It is unclear why simple 1980s-era
 *   protocols are still being used to exchange information with rigs. Baud rates now are fast enough that, even for serial lines, real
 *   protocols could be run over, for example, SLIP.
*/

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

k3_status::k3_status(const string_view rsp)
{ constexpr size_t FREQUENCY_POSN    { 2 };
  constexpr size_t RIT_POSITIVE_POSN { 18 };
  constexpr size_t ABS_RIT_POSN      { 19 };
  constexpr size_t RIT_ON_POSN       { 23 };
  constexpr size_t XIT_ON_POSN       { 24 };
  constexpr size_t TRANSMIT_POSN     { 28 };
  constexpr size_t OP_MODE_POSN      { 29 };
  constexpr size_t RX_VFO_POSN       { 30 };
  constexpr size_t SCANNING_POSN     { 31 };
  constexpr size_t SPLIT_POSN        { 32 };

  if (rsp.size() != K3_STATUS_REPLY_LENGTH)
    return;                                   // invalid

  _freq = frequency { from_string<double>(substring <string_view> (rsp, FREQUENCY_POSN, 11)) };

  if ( (_freq < 1800_kHz) or (_freq > 51_MHz) )
    return;                                   // invalid

  const char rit_positive_char { rsp[RIT_POSITIVE_POSN] };

  switch (rit_positive_char)
  { case '+' :
      _rit_positive = true;   // not actually necessary
      break;

    case '-' :
      _rit_positive = false;
      break;

    default :
      return;                                   // invalid
  }

  _abs_rit_offset = frequency { from_string<double>(substring <string_view> (rsp, ABS_RIT_POSN, 4)), FREQUENCY_UNIT::HZ };

  const char rit_on_char { rsp[RIT_ON_POSN] };

  switch (rit_on_char)
  { case '1' :
      _rit_is_on = true;
      break;

    case '0' :
      _rit_is_on = false;
      break;

    default :
      return;                                   // invalid
  }

  const char xit_on_char { rsp[XIT_ON_POSN] };

  switch (xit_on_char)
  { case '1' :
      _xit_is_on = true;
      break;

    case '0' :
      _xit_is_on = false;
      break;

    default :
      return;                                   // invalid
  }

  const char transmit_char { rsp[TRANSMIT_POSN] };

  switch (transmit_char)
  { case '1' :
      _transmit = true;
      break;

    case '0' :
      _transmit = false;
      break;

    default :
      return;                                   // invalid
  }

  const char op_mode_char { rsp[OP_MODE_POSN] };

  switch (op_mode_char)
  { case '1' :
      _op_mode = K3_OPERATING_MODE::LSB;
      break;

    case '2' :
      _op_mode = K3_OPERATING_MODE::USB;
      break;

    case '3' :
      _op_mode = K3_OPERATING_MODE::CW;
      break;

    case '4' :
      _op_mode = K3_OPERATING_MODE::FM;
      break;

    case '5' :
      _op_mode = K3_OPERATING_MODE::AM;
      break;

    case '6' :
      _op_mode = K3_OPERATING_MODE::DATA;
      break;

    case '7' :
      _op_mode = K3_OPERATING_MODE::CWREV;
      break;

    case '9' :                                  // there is no '8'
      _op_mode = K3_OPERATING_MODE::DATA_REV;
      break;

    default :
      return;                                   // invalid
  }

  const char rx_vfo_char { rsp[RX_VFO_POSN] };

  switch (rx_vfo_char)
  { case '0' :
      _rx_vfo = VFO::A;
      break;

    case '1' :
      _rx_vfo = VFO::B;
      break;

    default :
      return;                                   // invalid
  }

  const char scanning_char { rsp[SCANNING_POSN] };

  switch (scanning_char)
  { case '0' :
      _scanning = false;
      break;

    case '1' :
      _scanning = true;
      break;

    default :
      return;                                   // invalid
  }

  const char split_char { rsp[SPLIT_POSN] };

  switch (split_char)
  { case '0' :
      _is_split = false;
      break;

    case '1' :
      _is_split = true;
      break;

    default :
      return;                                   // invalid
  }

// ignore the last two fields

  _valid = true;
}

// ---------------------------------------- rig_interface -------------------------

/*! \class  rig_interface
    \brief  The interface to a rig

    A good argument can be made that this should be a base class, with specialisations occurring
    in derived classes. For now, that's not done.
*/

/*! \brief       Alert the user with a message
    \param  msg  message for the user

    Calls <i>_error_alert_function</i> to perform the actual alerting
*/
void rig_interface::_error_alert(const string_view msg) const
{ if (_error_alert_function)
    (*_error_alert_function)(msg);
}

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band.
    Attempts to confirm that the frequency was actually set to <i>f</i>.
*/
void rig_interface::_rig_frequency(const frequency f, const VFO v)
{ constexpr std::chrono::duration RETRY_TIME { 100ms };      // period between retries

  if (f.is_within_ham_band())
  { switch (v)
    { case VFO::A :
        _last_commanded_frequency = f;
        break;

      case VFO::B :
        _last_commanded_frequency_b = f;
        break;
    }

    if (_rig_connected)
    { SAFELOCK(_rig);           // hold the lock until we have received confirmation that the frequency is correct

      if (_model == RIG_MODEL_K3)
      { bool retry { true };

        constexpr int MAX_RETRIES { 2 };

        int n_retries { 0 };

        while ( (retry) and (n_retries++ <= MAX_RETRIES) )
        { raw_command( ((v == VFO::A) ? "FA"s : "FB"s) + pad_leftz(to_string(f.hz()), 11) + ';');

          const frequency rf { _rig_frequency(v) };

          if (f != rf)
          { const string msg { "frequency mismatch: commanded = "s + to_string(f.hz()) + "; actual = "s + to_string(rf.hz()) };

            _error_alert(msg);
            ost << msg << "; retrying" << endl;


            if (n_retries <= MAX_RETRIES)
              sleep_for(RETRY_TIME);
            else
            ost << "Maximum number of retries exceeded; quitting attempt to set frequency" << endl;
          }
          else
            retry = false;
        }
      }
      else      // not K3
      { bool retry { true };

        constexpr int MAX_RETRIES { 2 };

        int n_retries { 0 };

        while ( (retry) and (n_retries++ <= MAX_RETRIES) )
        { if (const int status { rig_set_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_A : RIG_VFO_B ), f.hz()) }; status != RIG_OK)
              _error_alert("Error setting frequency of VFO "s + ((v == VFO::A) ? "A"s : "B"s));

          if (debug)
            ost << "commanded frequency = " << to_string(f.hz()) << "; actual frequency = " << to_string(_rig_frequency(v).hz()) << endl;

          if (const auto rf {_rig_frequency(v)}; rf != f)     // explicitly check the frequency -- hamlib, however, will simply assume for a period of one second that the SET worked!!!

          { const string msg { "frequency mismatch: commanded = "s + to_string(f.hz()) + "; actual = "s + to_string(rf.hz()) };

            _error_alert(msg);
            ost << msg << "; retrying" << endl;
//          ost << "frequency mismatch: commanded = " << f << "; actual = " << rf << "; retrying" << endl;        // DEBUG for when we get stuck

            if (n_retries <= MAX_RETRIES)
              sleep_for(RETRY_TIME);
            else
              ost << "Maximum number of retries exceeded; quitting attempt to set frequency" << endl;
          }
          else
            retry = false;
        }
      }
    }
  }
  else
    _error_alert("Commanded frequency is not within ham band: "s + to_string(f.hz()) + " Hz"s);
}

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
frequency rig_interface::_rig_frequency(const VFO v) const
{ if (!_rig_connected)
    return ( (v == VFO::A) ? _last_commanded_frequency : _last_commanded_frequency_b);

  if (_model == RIG_MODEL_K3)
  { constexpr int                   MAX_RETRIES { 2 };
    constexpr std::chrono::duration RETRY_TIME  { 100ms };      // period between retries

    int       counter { 0 };
    frequency rv      { 0 };

    while ( (counter++ < MAX_RETRIES) and !rv )
    { const string response { raw_command( (v == VFO::A) ? "FA;"sv : "FB;"sv, RESPONSE::EXPECTED) };

      rv = frequency(from_string<uint32_t>(substring <string_view> (response, 2, 11)));

      if (!rv)
        sleep_for(RETRY_TIME);
    }

    if (!rv)
      ost << "Warning: returning 0 from _rig_frequency, VFO " << ((v == VFO::A) ? "A" : "B") << endl;

    return rv;
  }
  else          // not K3
  { freq_t hz;

    SAFELOCK(_rig);

    if (const int status { rig_get_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_CURR : RIG_VFO_B ), &hz) }; status != RIG_OK)
    { _error_alert("Error getting frequency of VFO "s + ((v == VFO::A) ? 'A' : 'B'));             // written to screen and to output file
      return ( (v == VFO::A) ? _last_commanded_frequency : _last_commanded_frequency_b) ;
    }

    return frequency { hz };
  }
}

// ---------------------------------------- rig_interface -------------------------

/*! \class  rig_interface
    \brief  The interface to a rig
*/

/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  expectation     whether a response is expected
    \param  expected_len    expected length of response
    \return                 the response from the rig, or the empty string

    Currently any expected length is ignored; the routine looks for the concluding ";" instead
    C++ does not allow a generic std::chrono::duration to be a parameter
*/
string rig_interface::_retried_raw_command(const string_view cmd, const milliseconds timeout, const int n_retries)
{ string rv;

  int counter    { 0 };
  bool completed { false };

  while (!completed and (counter != n_retries))  
  { rv = raw_command(cmd, RESPONSE::EXPECTED/*, expected_len*/);    // issue the command each time

    completed = (rv.empty() ? false : last_char(rv) == ';');

    if (!completed)
    { counter++;
      sleep_for(timeout);
    }
  }

  if (!completed)
  { ost << "no response from command: " << cmd << endl;
    rv.clear();             // don't give a partial reponse
  }

  return rv;
}

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
void rig_interface::prepare(const drlog_context& context)
{ const STRING_MAP<rig_model_t> type_name_to_hamlib_model { { "K3"s, RIG_MODEL_K3 }
                                                          };

  _port_name = context.rig1_port();
  rig_set_debug(RIG_DEBUG_NONE);
  rig_load_all_backends();              // this function returns an int -- in true Linux fashion, there is no documentation as to possible values and their meaning
                                        // see: http://hamlib.sourceforge.net/manuals/1.2.15/group__rig.html

  const string rig_type { to_upper(context.rig1_type()) };

#define       STR_TO_MODEL(y) \
  { #y, y }

    const STRING_MAP<rig_model_t> tmap
    { STR_TO_MODEL(RIG_MODEL_NETRIGCTL),
      STR_TO_MODEL(RIG_MODEL_ARMSTRONG),
      STR_TO_MODEL(RIG_MODEL_FLRIG),
      STR_TO_MODEL(RIG_MODEL_TRXMANAGER_RIG),
      STR_TO_MODEL(RIG_MODEL_DUMMY_NOVFO),
      STR_TO_MODEL(RIG_MODEL_TCI1X),
      STR_TO_MODEL(RIG_MODEL_ACLOG),
      STR_TO_MODEL(RIG_MODEL_SDRSHARP),
      STR_TO_MODEL(RIG_MODEL_QUISK),
      STR_TO_MODEL(RIG_MODEL_FT847),
      STR_TO_MODEL(RIG_MODEL_FT1000),
      STR_TO_MODEL(RIG_MODEL_FT1000D),
      STR_TO_MODEL(RIG_MODEL_FT1000MPMKV),
      STR_TO_MODEL(RIG_MODEL_FT747),
      STR_TO_MODEL(RIG_MODEL_FT757),
      STR_TO_MODEL(RIG_MODEL_FT757GXII),
      STR_TO_MODEL(RIG_MODEL_FT575),
      STR_TO_MODEL(RIG_MODEL_FT767),
      STR_TO_MODEL(RIG_MODEL_FT736R),
      STR_TO_MODEL(RIG_MODEL_FT840),
      STR_TO_MODEL(RIG_MODEL_FT820),
      STR_TO_MODEL(RIG_MODEL_FT900),
      STR_TO_MODEL(RIG_MODEL_FT920),
      STR_TO_MODEL(RIG_MODEL_FT890),
      STR_TO_MODEL(RIG_MODEL_FT990),
      STR_TO_MODEL(RIG_MODEL_FRG100),
      STR_TO_MODEL(RIG_MODEL_FRG9600),
      STR_TO_MODEL(RIG_MODEL_FRG8800),
      STR_TO_MODEL(RIG_MODEL_FT817),
      STR_TO_MODEL(RIG_MODEL_FT100),
      STR_TO_MODEL(RIG_MODEL_FT857),
      STR_TO_MODEL(RIG_MODEL_FT897),
      STR_TO_MODEL(RIG_MODEL_FT1000MP),
      STR_TO_MODEL(RIG_MODEL_FT1000MPMKVFLD),
      STR_TO_MODEL(RIG_MODEL_VR5000),
      STR_TO_MODEL(RIG_MODEL_FT450),
      STR_TO_MODEL(RIG_MODEL_FT950),
      STR_TO_MODEL(RIG_MODEL_FT2000),
      STR_TO_MODEL(RIG_MODEL_FT9000),
      STR_TO_MODEL(RIG_MODEL_FT980),
      STR_TO_MODEL(RIG_MODEL_FTDX5000),
      STR_TO_MODEL(RIG_MODEL_VX1700),
      STR_TO_MODEL(RIG_MODEL_FTDX1200),
      STR_TO_MODEL(RIG_MODEL_FT991),
      STR_TO_MODEL(RIG_MODEL_FT891),
      STR_TO_MODEL(RIG_MODEL_FTDX3000),
      STR_TO_MODEL(RIG_MODEL_FT847UNI),
      STR_TO_MODEL(RIG_MODEL_FT600),
      STR_TO_MODEL(RIG_MODEL_FTDX101D),
      STR_TO_MODEL(RIG_MODEL_FT818),
      STR_TO_MODEL(RIG_MODEL_FTDX10),
      STR_TO_MODEL(RIG_MODEL_FT897D),
      STR_TO_MODEL(RIG_MODEL_FTDX101MP),
      STR_TO_MODEL(RIG_MODEL_MCHFQRP),
      STR_TO_MODEL(RIG_MODEL_FT450D),
      STR_TO_MODEL(RIG_MODEL_FT650),
      STR_TO_MODEL(RIG_MODEL_FT990UNI),
      STR_TO_MODEL(RIG_MODEL_FT710),
      STR_TO_MODEL(RIG_MODEL_FT9000OLD),
      STR_TO_MODEL(RIG_MODEL_Q900),
      STR_TO_MODEL(RIG_MODEL_PMR171),
      STR_TO_MODEL(RIG_MODEL_TS50),
      STR_TO_MODEL(RIG_MODEL_TS440),
      STR_TO_MODEL(RIG_MODEL_TS450S),
      STR_TO_MODEL(RIG_MODEL_TS570D),
      STR_TO_MODEL(RIG_MODEL_TS690S),
      STR_TO_MODEL(RIG_MODEL_TS711),
      STR_TO_MODEL(RIG_MODEL_TS790),
      STR_TO_MODEL(RIG_MODEL_TS811),
      STR_TO_MODEL(RIG_MODEL_TS850),
      STR_TO_MODEL(RIG_MODEL_TS870S),
      STR_TO_MODEL(RIG_MODEL_TS940),
      STR_TO_MODEL(RIG_MODEL_TS950S),
      STR_TO_MODEL(RIG_MODEL_TS950SDX),
      STR_TO_MODEL(RIG_MODEL_TS2000),
      STR_TO_MODEL(RIG_MODEL_R5000),
      STR_TO_MODEL(RIG_MODEL_TS570S),
      STR_TO_MODEL(RIG_MODEL_THD7A),
      STR_TO_MODEL(RIG_MODEL_THD7AG),
      STR_TO_MODEL(RIG_MODEL_THF6A),
      STR_TO_MODEL(RIG_MODEL_THF7E),
      STR_TO_MODEL(RIG_MODEL_K2),
      STR_TO_MODEL(RIG_MODEL_TS930),
      STR_TO_MODEL(RIG_MODEL_THG71),
      STR_TO_MODEL(RIG_MODEL_TS680S),
      STR_TO_MODEL(RIG_MODEL_TS140S),
      STR_TO_MODEL(RIG_MODEL_TMD700),
      STR_TO_MODEL(RIG_MODEL_TMV7),
      STR_TO_MODEL(RIG_MODEL_TS480),
      STR_TO_MODEL(RIG_MODEL_K3),
      STR_TO_MODEL(RIG_MODEL_TRC80),
      STR_TO_MODEL(RIG_MODEL_TS590S),
      STR_TO_MODEL(RIG_MODEL_TRANSFOX),
      STR_TO_MODEL(RIG_MODEL_THD72A),
      STR_TO_MODEL(RIG_MODEL_TMD710),
      STR_TO_MODEL(RIG_MODEL_TMV71),
      STR_TO_MODEL(RIG_MODEL_F6K),
      STR_TO_MODEL(RIG_MODEL_TS590SG),
      STR_TO_MODEL(RIG_MODEL_XG3),
      STR_TO_MODEL(RIG_MODEL_TS990S),
      STR_TO_MODEL(RIG_MODEL_HPSDR),
      STR_TO_MODEL(RIG_MODEL_TS890S),
      STR_TO_MODEL(RIG_MODEL_THD74),
      STR_TO_MODEL(RIG_MODEL_K3S),
      STR_TO_MODEL(RIG_MODEL_KX2),
      STR_TO_MODEL(RIG_MODEL_KX3),
      STR_TO_MODEL(RIG_MODEL_PT8000A),
      STR_TO_MODEL(RIG_MODEL_K4),
      STR_TO_MODEL(RIG_MODEL_POWERSDR),
      STR_TO_MODEL(RIG_MODEL_MALACHITE),
      STR_TO_MODEL(RIG_MODEL_LAB599_TX500),
      STR_TO_MODEL(RIG_MODEL_SDRUNO),
      STR_TO_MODEL(RIG_MODEL_QRPLABS),
      STR_TO_MODEL(RIG_MODEL_FX4),
      STR_TO_MODEL(RIG_MODEL_THETIS),
      STR_TO_MODEL(RIG_MODEL_TRUSDX),
      STR_TO_MODEL(RIG_MODEL_SDRCONSOLE),
      STR_TO_MODEL(RIG_MODEL_QRPLABS_QMX),
      STR_TO_MODEL(RIG_MODEL_IC1271),
      STR_TO_MODEL(RIG_MODEL_IC1275),
      STR_TO_MODEL(RIG_MODEL_IC271),
      STR_TO_MODEL(RIG_MODEL_IC275),
      STR_TO_MODEL(RIG_MODEL_IC375),
      STR_TO_MODEL(RIG_MODEL_IC471),
      STR_TO_MODEL(RIG_MODEL_IC475),
      STR_TO_MODEL(RIG_MODEL_IC575),
      STR_TO_MODEL(RIG_MODEL_IC706),
      STR_TO_MODEL(RIG_MODEL_IC706MKII),
      STR_TO_MODEL(RIG_MODEL_IC706MKIIG),
      STR_TO_MODEL(RIG_MODEL_IC707),
      STR_TO_MODEL(RIG_MODEL_IC718),
      STR_TO_MODEL(RIG_MODEL_IC725),
      STR_TO_MODEL(RIG_MODEL_IC726),
      STR_TO_MODEL(RIG_MODEL_IC728),
      STR_TO_MODEL(RIG_MODEL_IC729),
      STR_TO_MODEL(RIG_MODEL_IC731),
      STR_TO_MODEL(RIG_MODEL_IC735),
      STR_TO_MODEL(RIG_MODEL_IC736),
      STR_TO_MODEL(RIG_MODEL_IC737),
      STR_TO_MODEL(RIG_MODEL_IC738),
      STR_TO_MODEL(RIG_MODEL_IC746),
      STR_TO_MODEL(RIG_MODEL_IC751),
      STR_TO_MODEL(RIG_MODEL_IC751A),
      STR_TO_MODEL(RIG_MODEL_IC756),
      STR_TO_MODEL(RIG_MODEL_IC756PRO),
      STR_TO_MODEL(RIG_MODEL_IC761),
      STR_TO_MODEL(RIG_MODEL_IC765),
      STR_TO_MODEL(RIG_MODEL_IC775),
      STR_TO_MODEL(RIG_MODEL_IC781),
      STR_TO_MODEL(RIG_MODEL_IC820),
      STR_TO_MODEL(RIG_MODEL_IC821H),
      STR_TO_MODEL(RIG_MODEL_IC970),
      STR_TO_MODEL(RIG_MODEL_ICR10),
      STR_TO_MODEL(RIG_MODEL_ICR71),
      STR_TO_MODEL(RIG_MODEL_ICR72),
      STR_TO_MODEL(RIG_MODEL_ICR75),
      STR_TO_MODEL(RIG_MODEL_ICR7000),
      STR_TO_MODEL(RIG_MODEL_ICR7100),
      STR_TO_MODEL(RIG_MODEL_ICR8500),
      STR_TO_MODEL(RIG_MODEL_ICR9000),
      STR_TO_MODEL(RIG_MODEL_IC910),
      STR_TO_MODEL(RIG_MODEL_IC78),
      STR_TO_MODEL(RIG_MODEL_IC746PRO),
      STR_TO_MODEL(RIG_MODEL_IC756PROII),
      STR_TO_MODEL(RIG_MODEL_ICID1),
      STR_TO_MODEL(RIG_MODEL_IC703),
      STR_TO_MODEL(RIG_MODEL_IC7800),
      STR_TO_MODEL(RIG_MODEL_IC756PROIII),
      STR_TO_MODEL(RIG_MODEL_ICR20),
      STR_TO_MODEL(RIG_MODEL_IC7000),
      STR_TO_MODEL(RIG_MODEL_IC7200),
      STR_TO_MODEL(RIG_MODEL_IC7700),
      STR_TO_MODEL(RIG_MODEL_IC7600),
      STR_TO_MODEL(RIG_MODEL_IC92D),
      STR_TO_MODEL(RIG_MODEL_ICR9500),
      STR_TO_MODEL(RIG_MODEL_IC7410),
      STR_TO_MODEL(RIG_MODEL_IC9100),
      STR_TO_MODEL(RIG_MODEL_ICRX7),
      STR_TO_MODEL(RIG_MODEL_IC7100),
      STR_TO_MODEL(RIG_MODEL_ID5100),
      STR_TO_MODEL(RIG_MODEL_IC2730),
      STR_TO_MODEL(RIG_MODEL_IC7300),
      STR_TO_MODEL(RIG_MODEL_PERSEUS),
      STR_TO_MODEL(RIG_MODEL_IC785x),
      STR_TO_MODEL(RIG_MODEL_X108G),
      STR_TO_MODEL(RIG_MODEL_ICR6),
      STR_TO_MODEL(RIG_MODEL_IC7610),
      STR_TO_MODEL(RIG_MODEL_ICR8600),
STR_TO_MODEL(RIG_MODEL_ICR30),
STR_TO_MODEL(RIG_MODEL_IC9700),
STR_TO_MODEL(RIG_MODEL_ID4100),
STR_TO_MODEL(RIG_MODEL_ID31),
STR_TO_MODEL(RIG_MODEL_ID51),
STR_TO_MODEL(RIG_MODEL_IC705),
STR_TO_MODEL(RIG_MODEL_ICF8101),
STR_TO_MODEL(RIG_MODEL_X6100),
STR_TO_MODEL(RIG_MODEL_G90),
STR_TO_MODEL(RIG_MODEL_X5105),
STR_TO_MODEL(RIG_MODEL_IC905),
STR_TO_MODEL(RIG_MODEL_X6200),
STR_TO_MODEL(RIG_MODEL_IC7760),
STR_TO_MODEL(RIG_MODEL_MINISCOUT),
STR_TO_MODEL(RIG_MODEL_XPLORER),
STR_TO_MODEL(RIG_MODEL_OS535),
STR_TO_MODEL(RIG_MODEL_OS456),
STR_TO_MODEL(RIG_MODEL_OMNIVI),
STR_TO_MODEL(RIG_MODEL_OMNIVIP),
STR_TO_MODEL(RIG_MODEL_PARAGON2),
STR_TO_MODEL(RIG_MODEL_DELTAII),
STR_TO_MODEL(RIG_MODEL_PCR1000),
STR_TO_MODEL(RIG_MODEL_PCR100),
STR_TO_MODEL(RIG_MODEL_PCR1500),
STR_TO_MODEL(RIG_MODEL_PCR2500),
STR_TO_MODEL(RIG_MODEL_AR8200),
STR_TO_MODEL(RIG_MODEL_AR8000),
STR_TO_MODEL(RIG_MODEL_AR7030),
STR_TO_MODEL(RIG_MODEL_AR5000),
STR_TO_MODEL(RIG_MODEL_AR3030),
STR_TO_MODEL(RIG_MODEL_AR3000A),
STR_TO_MODEL(RIG_MODEL_AR3000),
STR_TO_MODEL(RIG_MODEL_AR2700),
STR_TO_MODEL(RIG_MODEL_AR2500),
STR_TO_MODEL(RIG_MODEL_AR16),
STR_TO_MODEL(RIG_MODEL_SDU5500),
STR_TO_MODEL(RIG_MODEL_SDU5000),
STR_TO_MODEL(RIG_MODEL_AR8600),
STR_TO_MODEL(RIG_MODEL_AR5000A),
STR_TO_MODEL(RIG_MODEL_AR7030P),
STR_TO_MODEL(RIG_MODEL_SR2200),
STR_TO_MODEL(RIG_MODEL_JST145),
STR_TO_MODEL(RIG_MODEL_JST245),
STR_TO_MODEL(RIG_MODEL_CMH530),
STR_TO_MODEL(RIG_MODEL_NRD345),
STR_TO_MODEL(RIG_MODEL_NRD525),
STR_TO_MODEL(RIG_MODEL_NRD535),
STR_TO_MODEL(RIG_MODEL_NRD545),
STR_TO_MODEL(RIG_MODEL_RS64),
STR_TO_MODEL(RIG_MODEL_RS2005),
STR_TO_MODEL(RIG_MODEL_RS2006),
STR_TO_MODEL(RIG_MODEL_RS2035),
STR_TO_MODEL(RIG_MODEL_RS2042),
STR_TO_MODEL(RIG_MODEL_RS2041),
STR_TO_MODEL(RIG_MODEL_BC780),
STR_TO_MODEL(RIG_MODEL_BC245),
STR_TO_MODEL(RIG_MODEL_BC895),
STR_TO_MODEL(RIG_MODEL_PRO2052),
STR_TO_MODEL(RIG_MODEL_BC235),
STR_TO_MODEL(RIG_MODEL_BC250),
STR_TO_MODEL(RIG_MODEL_BC785),
STR_TO_MODEL(RIG_MODEL_BC786),
STR_TO_MODEL(RIG_MODEL_BCT8),
STR_TO_MODEL(RIG_MODEL_BCD396T),
STR_TO_MODEL(RIG_MODEL_BCD996T),
STR_TO_MODEL(RIG_MODEL_BC898),
STR_TO_MODEL(RIG_MODEL_DKR8),
STR_TO_MODEL(RIG_MODEL_DKR8A),
STR_TO_MODEL(RIG_MODEL_DKR8B),
STR_TO_MODEL(RIG_MODEL_HF150),
STR_TO_MODEL(RIG_MODEL_HF225),
STR_TO_MODEL(RIG_MODEL_HF250),
STR_TO_MODEL(RIG_MODEL_HF235),
STR_TO_MODEL(RIG_MODEL_RA3790),
STR_TO_MODEL(RIG_MODEL_RA3720),
STR_TO_MODEL(RIG_MODEL_RA6790),
STR_TO_MODEL(RIG_MODEL_RA3710),
STR_TO_MODEL(RIG_MODEL_RA3702),
STR_TO_MODEL(RIG_MODEL_HF1000),
STR_TO_MODEL(RIG_MODEL_HF1000A),
STR_TO_MODEL(RIG_MODEL_WJ8711),
STR_TO_MODEL(RIG_MODEL_WJ8888),
STR_TO_MODEL(RIG_MODEL_ESM500),
STR_TO_MODEL(RIG_MODEL_EK890),
STR_TO_MODEL(RIG_MODEL_EK891),
STR_TO_MODEL(RIG_MODEL_EK895),
STR_TO_MODEL(RIG_MODEL_EK070),
STR_TO_MODEL(RIG_MODEL_TRP7000),
STR_TO_MODEL(RIG_MODEL_TRP8000),
STR_TO_MODEL(RIG_MODEL_TRP9000),
STR_TO_MODEL(RIG_MODEL_TRP8255),
STR_TO_MODEL(RIG_MODEL_WR1000),
STR_TO_MODEL(RIG_MODEL_WR1500),
STR_TO_MODEL(RIG_MODEL_WR1550),
STR_TO_MODEL(RIG_MODEL_WR3100),
STR_TO_MODEL(RIG_MODEL_WR3150),
STR_TO_MODEL(RIG_MODEL_WR3500),
STR_TO_MODEL(RIG_MODEL_WR3700),
STR_TO_MODEL(RIG_MODEL_G303),
STR_TO_MODEL(RIG_MODEL_G313),
STR_TO_MODEL(RIG_MODEL_G305),
STR_TO_MODEL(RIG_MODEL_G315),
STR_TO_MODEL(RIG_MODEL_TT550),
STR_TO_MODEL(RIG_MODEL_TT538),
STR_TO_MODEL(RIG_MODEL_RX320),
STR_TO_MODEL(RIG_MODEL_RX340),
STR_TO_MODEL(RIG_MODEL_RX350),
STR_TO_MODEL(RIG_MODEL_TT526),
STR_TO_MODEL(RIG_MODEL_TT516),
STR_TO_MODEL(RIG_MODEL_TT565),
STR_TO_MODEL(RIG_MODEL_TT585),
STR_TO_MODEL(RIG_MODEL_TT588),
STR_TO_MODEL(RIG_MODEL_RX331),
STR_TO_MODEL(RIG_MODEL_TT599),
STR_TO_MODEL(RIG_MODEL_DX77),
STR_TO_MODEL(RIG_MODEL_DXSR8),
STR_TO_MODEL(RIG_MODEL_505DSP),
STR_TO_MODEL(RIG_MODEL_GNURADIO),
STR_TO_MODEL(RIG_MODEL_MC4020),
STR_TO_MODEL(RIG_MODEL_GRAUDIO),
STR_TO_MODEL(RIG_MODEL_GRAUDIOIQ),
STR_TO_MODEL(RIG_MODEL_USRP_G),
STR_TO_MODEL(RIG_MODEL_MICROTUNE_4937),
STR_TO_MODEL(RIG_MODEL_MICROTUNE_4702),
STR_TO_MODEL(RIG_MODEL_MICROTUNE_4707),
STR_TO_MODEL(RIG_MODEL_DSP10),
STR_TO_MODEL(RIG_MODEL_SDR1000),
STR_TO_MODEL(RIG_MODEL_SDR1000RFE),
STR_TO_MODEL(RIG_MODEL_DTTSP),
STR_TO_MODEL(RIG_MODEL_DTTSP_UDP),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_A),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_B),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_C),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_D),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_E),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_F),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_G),
STR_TO_MODEL(RIG_MODEL_SMARTSDR_H),
STR_TO_MODEL(RIG_MODEL_EKD500),
STR_TO_MODEL(RIG_MODEL_ELEKTOR304),
STR_TO_MODEL(RIG_MODEL_DRT1),
STR_TO_MODEL(RIG_MODEL_DWT),
STR_TO_MODEL(RIG_MODEL_USRP0),
STR_TO_MODEL(RIG_MODEL_USRP),
STR_TO_MODEL(RIG_MODEL_DDS60),
STR_TO_MODEL(RIG_MODEL_ELEKTOR507),
STR_TO_MODEL(RIG_MODEL_MINIVNA),
STR_TO_MODEL(RIG_MODEL_SI570AVRUSB),
STR_TO_MODEL(RIG_MODEL_PMSDR),
STR_TO_MODEL(RIG_MODEL_SI570PICUSB),
STR_TO_MODEL(RIG_MODEL_FIFISDR),
STR_TO_MODEL(RIG_MODEL_FUNCUBEDONGLE),
STR_TO_MODEL(RIG_MODEL_HIQSDR),
STR_TO_MODEL(RIG_MODEL_FASDR),
STR_TO_MODEL(RIG_MODEL_SI570PEABERRY1),
STR_TO_MODEL(RIG_MODEL_SI570PEABERRY2),
STR_TO_MODEL(RIG_MODEL_FUNCUBEDONGLEPLUS),
STR_TO_MODEL(RIG_MODEL_RSHFIQ),
STR_TO_MODEL(RIG_MODEL_V4L),
STR_TO_MODEL(RIG_MODEL_V4L2),
STR_TO_MODEL(RIG_MODEL_ESMC),
STR_TO_MODEL(RIG_MODEL_EB200),
STR_TO_MODEL(RIG_MODEL_XK2100),
STR_TO_MODEL(RIG_MODEL_EK89X),
STR_TO_MODEL(RIG_MODEL_XK852),
STR_TO_MODEL(RIG_MODEL_PRM8060),
STR_TO_MODEL(RIG_MODEL_PRM8070),
STR_TO_MODEL(RIG_MODEL_ADT_200A),
STR_TO_MODEL(RIG_MODEL_IC_M700PRO),
STR_TO_MODEL(RIG_MODEL_IC_M802),
STR_TO_MODEL(RIG_MODEL_IC_M710),
STR_TO_MODEL(RIG_MODEL_IC_M803),
STR_TO_MODEL(RIG_MODEL_DORJI_DRA818V),
STR_TO_MODEL(RIG_MODEL_DORJI_DRA818U),
STR_TO_MODEL(RIG_MODEL_BARRETT_2050),
STR_TO_MODEL(RIG_MODEL_BARRETT_950),
STR_TO_MODEL(RIG_MODEL_BARRETT_4050),
STR_TO_MODEL(RIG_MODEL_BARRETT_4100),
STR_TO_MODEL(RIG_MODEL_ELAD_FDM_DUO),
STR_TO_MODEL(RIG_MODEL_CODAN_ENVOY),
STR_TO_MODEL(RIG_MODEL_CODAN_NGT),
STR_TO_MODEL(RIG_MODEL_GS100),
STR_TO_MODEL(RIG_MODEL_MDS4710),
STR_TO_MODEL(RIG_MODEL_MDS9710),
STR_TO_MODEL(RIG_MODEL_ATD578UVIII),
STR_TO_MODEL(RIG_MODEL_MICOM2),
STR_TO_MODEL(RIG_MODEL_CTX10)


    };

#undef STR_TO_MODEL

    FOR_ALL(tmap, [] (const auto& pr) { ost << "tmap: " << pr.first << ": " << pr.second << endl; });

  { if (rig_type.starts_with("HAMLIB_"sv))
    { const string hamlib_name { "RIG_MODEL_"s + remove_from_start <string> (rig_type, "HAMLIB_"sv) };

      ost << "HAMLIB name = " << hamlib_name << endl;

      _model = MUM_VALUE(tmap, hamlib_name, RIG_MODEL_DUMMY);

      ost << "HAMLIB _model = " << _model << endl;
    }
  }

  if (_model == RIG_MODEL_DUMMY)
    _model = MUM_VALUE(type_name_to_hamlib_model, rig_type, RIG_MODEL_DUMMY);   // get the hamlib type of rig

  if ( (_model == RIG_MODEL_DUMMY) and !rig_type.empty())
    _error_alert("Unknown rig type: "s + rig_type);

  _rigp = rig_init(_model);

  if (!_rigp)
    throw rig_interface_error(RIG_UNABLE_TO_INITIALISE, "Unable to initialise rig structure for rig type "s + (rig_type.empty() ? "DUMMY"s : rig_type));

  if (_model != RIG_MODEL_DUMMY)        // hamlib documentation is unclear as to whether this is necessary; probably not
  { baud_rate(context.rig1_baud());
    data_bits(context.rig1_data_bits());
    stop_bits(context.rig1_stop_bits());

    if (_port_name.size() >= (HAMLIB_FILPATHLEN - 2))                        // ridiculous C-ism
    { const string msg { "Port name is too long: "s + _port_name };

      _error_alert(msg);
      ost << msg << endl;
      exit(-1);
    }

    strncpy(_rigp->state.rigport.pathname, _port_name.c_str(), HAMLIB_FILPATHLEN - 1);     // !! -1 to remove compiler warning about length
  }

  const int status { rig_open(_rigp) };

  if (status != RIG_OK)
  { const string msg { "Unable to open the rig on port "s + _port_name + ": Error = "s + hamlib_error_code_to_string(status) };

    _error_alert(msg);
    ost << msg << endl;
    throw rig_interface_error(RIG_UNABLE_TO_OPEN, "Error opening rig: " + rig_type);
  }

  if ((status == RIG_OK) and (_model != RIG_MODEL_DUMMY))
    _rig_connected = true;

// if it's a K3, enable extended mode
  k3_command_mode(K3_COMMAND_MODE::EXTENDED);
}

/*! \brief      Set mode
    \param  m   new mode

    If not a K3, then also sets the bandwidth (because it's easier to follow hamlib's model, even though it is obviously flawed)
*/
void rig_interface::rig_mode(const MODE m)
{ constexpr chrono::duration RETRY_TIME          { 10ms };   // wait time if a retry is necessary; decreasing this makes little difference
  constexpr chrono::duration K3_MODE_CHANGE_TIME { 500ms };  // time for a K3 to change mode

  _last_commanded_mode = m;

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { int counter { 0 };

      MODE last_tested_mode;

      while ( (counter++ <= 10) and (last_tested_mode = rig_mode(), last_tested_mode != m) ) // don't change mode if we're already in the correct mode
      { if (counter != 1)                               // no pause the first time through
          sleep_for(RETRY_TIME);
        
        switch(m)
        { case MODE_CW :
            raw_command("MD3;"sv);
            sleep_for(K3_MODE_CHANGE_TIME);
            break;

          case MODE_SSB :
            { const string k3_mode_cmd { (rig_frequency() < 10_MHz) ? "MD1;"sv : "MD2;"sv };  // select LSB or USB

              raw_command(k3_mode_cmd);
              sleep_for(K3_MODE_CHANGE_TIME);
              break;
            }

          default :
            break;
        }
      }

      if (counter > 10)
        _error_alert("Error setting mode: commanded mode = "s + MODE_NAME[m] + "; but rig is in mode: " + MODE_NAME[last_tested_mode]);
    }
    else    // not K3
    { static pbwidth_t last_cw_bandwidth  { 200 };
      static pbwidth_t last_ssb_bandwidth { 1800 };

// set correct hamlib mode 
      rmode_t hamlib_m { RIG_MODE_CW };

      if (m == MODE_SSB)
        hamlib_m = ( (rig_frequency() < 10_MHz) ? RIG_MODE_LSB : RIG_MODE_USB );  // select LSB or USB

      int status;

// hamlib, for reasons I can't guess, sets both the mode and the bandwidth in a single command
      pbwidth_t tmp_bandwidth;
      rmode_t   tmp_mode;

      { SAFELOCK(_rig);
        status = rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);
      }

      if (status != RIG_OK)
        _error_alert("Error getting mode prior to setting mode");
      else
      { switch (tmp_mode)
        { case RIG_MODE_CW:
            last_cw_bandwidth = tmp_bandwidth;
            break;

          case RIG_MODE_LSB:
          case RIG_MODE_USB:
            last_ssb_bandwidth = tmp_bandwidth;
            break;

          default:
            break;
        }

        bool retry { true };

        SAFELOCK(_rig);       // hold the lock until we receive positive confirmation that the rig is properly set to correct mode and bandwidth

        while (retry)
        { const pbwidth_t new_bandwidth    { ( (m == MODE_SSB) ? last_ssb_bandwidth : last_cw_bandwidth ) };
          const pbwidth_t bandwidth_to_set { (tmp_mode == hamlib_m) ? tmp_bandwidth : new_bandwidth };

          status = rig_set_mode(_rigp, RIG_VFO_CURR, hamlib_m, bandwidth_to_set) ;

          if (status != RIG_OK)
            _error_alert("Error setting mode"s);

          status = rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);

          if (status != RIG_OK)
            _error_alert("Error getting mode after setting mode"s);

          if ( (tmp_mode != hamlib_m) or (tmp_bandwidth != new_bandwidth) )   // explicitly check the mode and bandwidth
            sleep_for(RETRY_TIME);
          else
            retry = false;                                                    // we're done
        }
      }
    }
  }
}

/*! \brief  Enable split operation

    hamlib has no good definition of exactly what split operation really means, and, hence,
    has no clear description of precisely what the hamlib rig_set_split_vfo() function is supposed
    to do for various values of the permitted parameters. There is general agreement on the reflector
    that the call contained herein *should* do the "right" thing -- but since there's no precise definition
    of any of this, not all backends are guaranteed to behave the same.

    Hence we use the explicit K3 command, since at least we know what that will do on that rig.
*/
void rig_interface::split_enable(void) const
{ if (!_rig_connected)
    return;

  SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { raw_command("FT1;"sv);
    rig_is_split = true;
    return;
  }

// not a K3
  if (const int status { rig_set_split_vfo(_rigp, RIG_VFO_CURR, RIG_SPLIT_ON, RIG_VFO_B) }; status != RIG_OK)  // magic parameters
    _error_alert("Error executing SPLIT command"s);
  else
    rig_is_split = true;
}

/// disable split operation; see caveats under split_enable()
void rig_interface::split_disable(void) const
{ if (!_rig_connected)
    return;

  SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { raw_command("FR0;"sv);
    rig_is_split = false;
    return;
  }

// not a K3
//  const int status = rig_set_split_vfo(_rigp, RIG_VFO_CURR, RIG_SPLIT_OFF, RIG_VFO_A);    // do not delete this line, in case we ever need to use this version instead of the following line
//  const int status { rig_set_split_vfo(_rigp, RIG_VFO_A, RIG_SPLIT_OFF, RIG_VFO_A) };     // the line above also works

  if (const int status { rig_set_split_vfo(_rigp, RIG_VFO_A, RIG_SPLIT_OFF, RIG_VFO_A) }; status != RIG_OK)
    _error_alert("Error executing SPLIT command"s);
  else
    rig_is_split = false;
}

/*! \brief      Is split enabled?
    \return     whether split is enabled on the rig

    This interrogates the rig; it neither reads not writes the variable rig_is_split
*/
bool rig_interface::split_enabled(void) const
{ if (!_rig_connected)
    return false;

  if (_model == RIG_MODEL_K3)
  { SAFELOCK(_rig);

    if ( const string transmit_vfo { raw_command("FT;"sv, RESPONSE::EXPECTED) }; contains_at(transmit_vfo, ';', 3) and contains_at(transmit_vfo, "FT"sv, 0) )
      return (transmit_vfo[2] == '1');

    _error_alert("Unable to determine whether rig is SPLIT"s);
    return false;
  }

// not a K3
  split_t split_mode;
  vfo_t   tx_vfo;

  SAFELOCK(_rig);

  if (const int status { rig_get_split_vfo(_rigp, RIG_VFO_B, &split_mode, &tx_vfo) }; status != RIG_OK)
  { _error_alert("Error getting SPLIT"s);
    return false;
  }

  return (split_mode == RIG_SPLIT_ON);
}

/*! \brief          Set baud rate
    \param  rate    baud rate to which the rig should be set
*/
void rig_interface::baud_rate(const unsigned int rate)
{ if (_rigp)
  { SAFELOCK(_rig);

    _rigp->state.rigport.parm.serial.rate = rate;
  }
}

/*! \brief      Get baud rate
    \return     rig baud rate
*/
unsigned int rig_interface::baud_rate(void) const
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.rate : 0);
}

/*! \brief          Set the number of data bits (7 or 8)
    \param  bits    the number of data bits to which the rig should be set

    Throws exception if <i>bits</i> is not 7 or 8
*/
void rig_interface::data_bits(const unsigned int bits)
{ if ( (bits < 7) or (bits > 8) )
    throw rig_interface_error(RIG_INVALID_DATA_BITS, "Attempt to set invalid number of data bits: "s + to_string(bits));

  SAFELOCK(_rig);

  if (_rigp)
    _rigp->state.rigport.parm.serial.data_bits = bits;
}

/*! \brief      Get the number of data bits
    \return     number of data bits
*/
unsigned int rig_interface::data_bits(void) const
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.data_bits : 0);
}

/*! \brief          Set the number of stop bits (1 or 2)
    \param  bits    the number of stop bits to which the rig should be set

    Throws exception if <i>bits</i> is not 1 or 2
*/
void rig_interface::stop_bits(const unsigned int bits)
{ if ( (bits < 1) or (bits > 2) )
    throw rig_interface_error(RIG_INVALID_STOP_BITS, "Attempt to set invalid number of stop bits: "s + to_string(bits));

  SAFELOCK(_rig);

  if (_rigp)
    _rigp->state.rigport.parm.serial.stop_bits = bits;
}

/*! \brief      Get the number of stop bits
    \return     number of stop bits
*/
unsigned int rig_interface::stop_bits(void) const
{ SAFELOCK(_rig);

  return (_rigp ? _rigp->state.rigport.parm.serial.stop_bits : 0);
}

/// get the rig's mode
MODE rig_interface::rig_mode(void) const
{ if (!_rig_connected)
    return _last_commanded_mode;
  else
  { rmode_t   m;
    pbwidth_t w;

    SAFELOCK(_rig);

    if (const int status { rig_get_mode(_rigp, RIG_VFO_CURR, &m, &w) }; status != RIG_OK)
    { _error_alert("Error getting mode"s);
      return _last_commanded_mode;
    }

    switch (m)
    { case RIG_MODE_CW :
        return MODE_CW;

      case RIG_MODE_LSB :
      case RIG_MODE_USB :
        return MODE_SSB;

      default :
        return MODE_CW;
    }
  }
}

/*! \brief      Set rit offset (in Hz)
    \param  hz  offset in Hz
*/
void rig_interface::rit(const int hz) const
{ if (!_rig_connected)                          // do nothing if no rig is connected
    return;

// hamlib's behaviour anent the K3 is brain dead if hz == 0
  if (_model == RIG_MODEL_K3)
  { if (hz == 0)                                // just clear the RIT/XIT
      raw_command("RC;"sv);
    else
    { const int    positive_hz { abs(hz) };
      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s) + pad_leftz(positive_hz, 4) };

      raw_command("RO"s + hz_str + SEMICOLON);
    }
  }
  else
  { SAFELOCK(_rig);

    const int status { rig_set_rit(_rigp, RIG_VFO_CURR, hz) };

    if (status != RIG_OK)
      _error_alert("Hamlib error while setting RIT offset"s);
  }
}

/// get rit offset (in Hz)
int rig_interface::rit(void) const
{ if (_model == RIG_MODEL_K3)
  { if ( const string value { raw_command("RO;"sv, RESPONSE::EXPECTED) }; contains_at(value, ';', 7) and contains_at(value, "RO"sv, 0) )
      return from_string<int>(substring <std::string> (value, 2, 5));
    else
    { _error_alert("Invalid rig response in rit(): "s + value);
      return 0;
    }
  }
  else
  { SAFELOCK(_rig);

    shortfreq_t hz;

    if (const int status { rig_get_rit(_rigp, RIG_VFO_CURR, &hz) }; status != RIG_OK)
    { _error_alert("Hamlib error while getting RIT offset"s);
      return 0;
    }

    return static_cast<int>(hz);
  }
}

/*! \brief  Turn rit on

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
void rig_interface::rit_enable(void) const
{ if (_model == RIG_MODEL_K3)
    raw_command("RT1;"sv);          // proper enable for the K3
  else
    rit(1);                         // 1 Hz offset, since a zero offset would disable RIT in hamlib
}

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
void rig_interface::rit_disable(void) const
{ if (_model == RIG_MODEL_K3)
    raw_command("RT0;"sv);      // proper disable for the K3
  else
    rit(0);                     // 0 Hz offset, which hamlib regards as disabling RIT
}

/// is rit enabled?
bool rig_interface::rit_enabled(void) const
{ switch (_model)
  { case RIG_MODEL_K3 :
    { if ( const string response { raw_command("RT;"sv, RESPONSE::EXPECTED) }; contains_at(response, SEMICOLON, 3) and contains_at(response, "RT"sv, 0) )
        return (response[2] == '1');
      else
      { _error_alert("Invalid length in rit_enabled(): "s + response);
        return false;
      }
    }
    
    case RIG_MODEL_DUMMY :
      return false;
  }
  
  return false;         // default response
}

/*! \brief  Turn xit on

    This is a kludge, as hamlib equates an offset of zero with xit turned off (!)
*/
void rig_interface::xit_enable(void) const
{ if (_model == RIG_MODEL_K3)
    raw_command("XT1;"sv);
  else
    xit(1);                 // 1 Hz offset
}

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates an offset of zero with xit turned off (!)
*/
void rig_interface::xit_disable(void) const
{ if (_model == RIG_MODEL_K3)
    raw_command("XT0;"sv);
  else
    xit(1);                 // 1 Hz offset
}

/// is xit enabled?
bool rig_interface::xit_enabled(void) const
{ switch (_model)
  { case RIG_MODEL_K3 :
    { if ( const string response { raw_command("XT;"sv, RESPONSE::EXPECTED) }; contains_at(response, ';', 3) and contains_at(response, "XT"sv, 0) )
        return (response[2] == '1');
      else
      { _error_alert("Invalid length in xit_enabled(): "s + response);  // handle this error upstairs
        return false;
      }
    }
    
    case RIG_MODEL_DUMMY :
      return false;
  }
  
  return false;         // default response
}

/*! \brief      Set xit offset (in Hz)
    \param  hz  offset in Hz

    On the K3 this also sets the RIT offset
*/
void rig_interface::xit(const int hz) const
{ if (_model == RIG_MODEL_K3)                   // hamlib's behaviour anent the K3 is not what we want, have K3-specific code
  { if (hz == 0)                                // just clear the RIT/XIT
      raw_command("RC;"sv);
    else
    { const int    positive_hz { abs(hz) };
      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s ) + pad_leftz(positive_hz, 4) };

      raw_command("RO"s + hz_str + SEMICOLON);
    }
  }
  else
  { SAFELOCK(_rig);

    if (const int status { rig_set_xit(_rigp, RIG_VFO_CURR, hz) }; status != RIG_OK)
      _error_alert("Hamlib error in xit(int)");  // handle this error upstairs
  }
}

/// get xit offset (in Hz)
int rig_interface::xit(void) const
{ shortfreq_t hz;

  SAFELOCK(_rig);

  if (const int status { rig_get_xit(_rigp, RIG_VFO_CURR, &hz ) }; status != RIG_OK)
  {  _error_alert("Hamlib error obtaining XIT offset"s);
     return 0;
  }

  return static_cast<int>(hz);
}

/// lock the VFO
void rig_interface::lock(void) const
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
    raw_command("LK1;"s, RESPONSE::NOT_EXPECTED);
  else
  { constexpr int v { 1 };

    if (const int status { rig_set_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, v) }; status != RIG_OK)
      _error_alert("Hamlib error locking VFO"s);
  }
}

/// unlock the VFO
void rig_interface::unlock(void) const
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
    raw_command("LK0;"s, RESPONSE::NOT_EXPECTED);
  else
  { constexpr int v { 0 };

    if (const int status { rig_set_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, v) }; status != RIG_OK)
      _error_alert("Hamlib error unlocking VFO"s);
  }
}

/*! \brief      Turn sub-receiver on/off
    \param  b   turn sub-receiver on if TRUE, otherwise turn off
*/
void rig_interface::sub_receiver(const bool torf) const
{ if (_model == RIG_MODEL_K3)
    raw_command( (torf ? "SB1;"s : "SB0;"s), RESPONSE::NOT_EXPECTED);
}

/// is sub-receiver on?
bool rig_interface::sub_receiver(void) const
{ if (_model == RIG_MODEL_K3)
  { if (const string response { raw_command("SB;"sv, RESPONSE::EXPECTED) }; response.length() < 3)
    { _error_alert("SUBRX Short response"sv);
      return false;
    }
    else
      return (response[2] == '1');
  }

  return false;   // not K3
}

/*! \brief          Set the keyer speed
    \param  wpm     keyer speed in WPM
*/
void rig_interface::keyer_speed(const int wpm) const
{ SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { const string cmd { "KS"s + pad_leftz(wpm, 3) + SEMICOLON };

    raw_command(cmd, RESPONSE::NOT_EXPECTED);
  }
  else
  { value_t v;

    v.i = wpm;

    if (const int status { rig_set_level(_rigp, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, v) }; status != RIG_OK)
      _error_alert("Hamlib error setting keyer speed"s);
  }
}

/// get the keyer speed in WPM
int rig_interface::keyer_speed(void) const
{ constexpr int DEFAULT_WPM { 26 };

  SAFELOCK(_rig);

  if (_model == RIG_MODEL_K3)
  { if ( const string response { raw_command("KS;"sv, RESPONSE::EXPECTED) }; contains_at(response, ';', 5) and contains_at(response, "KS"sv, 0) )
      return from_string<int>(substring <std::string> (response, 2, 3));
    else
    { _error_alert("Invalid response getting keyer_speed: "s + response);
      return false;
    }
  }
  else              // not K3
  { value_t v;

    if (const int status { rig_get_level(_rigp, RIG_VFO_CURR, RIG_LEVEL_KEYSPD, &v) }; status != RIG_OK)
    { _error_alert("Hamlib error getting keyer speed"s);
      return DEFAULT_WPM;  // default speed
    }

    return v.i;
  }
}

/// pause until the rig is no longer transmitting
void rig_interface::wait_until_not_busy(void) const
{ while (is_transmitting())
    sleep_for(100ms);
}

// explicit K3 commands

/*! \brief                  Send a raw command to the rig
    \param  cmd             the command to send
    \param  expectation     whether a response is expected
    \param  expected_len    expected length of response
    \return                 the response from the rig, or the empty string

    Currently any expected length is ignored; the routine looks for the concluding ";" instead
*/
string rig_interface::raw_command(const string_view cmd, const RESPONSE expectation/*, const int expected_len */) const
{ constexpr chrono::duration RETRY_TIME { 10ms };  // wait time if a retry is necessary

  const bool response_expected { expectation == RESPONSE::EXPECTED };

  struct rig_state* rs_p { &(_rigp->state) };

  const int fd { _file_descriptor() };

  constexpr int INBUF_SIZE { 1000 };        // size of input buffer

  static array<char, INBUF_SIZE> c_in;

  unsigned int total_read { 0 };

  string rcvd { };

  static string rcvd_buf { };

  const bool is_p3_screenshot { (cmd == "#BMP;"sv) };   // this has to be treated differently: the response is long and has no concluding semicolon

  if (!_rig_connected)
    return string { };

  if (cmd.empty())
    return string { };

  constexpr int MAX_ATTEMPTS         { 10 };
  constexpr int TIMEOUT_MICROSECONDS { 100'000 };    // 100 milliseconds

// sanity check ... on K3 all commands end in a ";"
  if (last_char(cmd) != SEMICOLON)
  {  _error_alert("Invalid rig command: "s + cmd);
    return string();
  }

  bool completed { false };

  { SAFELOCK(_rig);             // hold lock until we're done

    rig_flush(&rs_p->rigport);

    if (_instrumented)
      ost << "sent to rig: " << cmd << endl;

    write(fd, cmd.data(), cmd.length());     // send the command

    rig_flush(&rs_p->rigport);
    sleep_for(RETRY_TIME);            // wait for a bit

    fd_set set;
    struct timeval timeout;  // time_t (seconds), long (microseconds)

    auto set_timeout = [fd, &set, &timeout] (const time_t sec, const long usec)
      { FD_ZERO(&set);        // clear the set
        FD_SET(fd, &set);     // add the file descriptor to the set
        //fd_set_value(set, fd);    // requires socket support

        timeout.tv_sec = sec;
        timeout.tv_usec = usec;
      };

    int counter { 0 };

    constexpr int SCREEN_BITS { 131'640 };

    if (response_expected)
    { if (is_p3_screenshot)
      { array<char, SCREEN_BITS> c_in;    // hide the static array

        const int n_bits { SCREEN_BITS * 10 };
        const int n_secs { n_bits / static_cast<int>(baud_rate() )};

        while (!completed and (counter < (n_secs + 5)) )    // add 5 extra seconds
        { set_timeout(1, 0);

          if (int status { select(fd + 1, &set, NULL, NULL, &timeout) }; status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)
             ost << "timeout in select() in raw_command: " << cmd << endl;
            else
            { const ssize_t n_read { read(fd, c_in.data(), SCREEN_BITS - total_read) };

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                rcvd.append(c_in.data(), n_read);

                if (rcvd.length() == SCREEN_BITS)
                  completed = true;
              }
            }
          }

          counter++;

          if (!completed)
          { static const string percent_str { "%%"s };

            const int percent { static_cast<int>(rcvd.length() * 100) / SCREEN_BITS };

            alert("P3 screendump progress: "s + to_string(percent) + percent_str);
            sleep_for(1s);      // we have the lock for all this time
          }
        }
      }
      else                                              // not a P3 screenshot; keep reading until we receive at least one ";"
      { static int rig_communication_failures { 0 };

        while (!completed and (counter < MAX_ATTEMPTS) )
        { set_timeout(0, TIMEOUT_MICROSECONDS);

          if (counter)                          // we've already slept the first time through
            sleep_for(RETRY_TIME);

          if (const int status { select(fd + 1, &set, NULL, NULL, &timeout) }; status == -1)
            ost << "Error in select() in raw_command()" << endl;
          else
          { if (status == 0)                // possibly a timeout error
            { if (counter == MAX_ATTEMPTS - 1)
              { if (cmd == "TQ;"sv)       // transmit query
                { rig_communication_failures++;

                  if (rig_communication_failures == 1)
                    ost << "status communication with rig failed" << endl;
                }
                else
                  ost << "last-attempt timeout (" << (TIMEOUT_MICROSECONDS * MAX_ATTEMPTS) << " microseconds) in select() in raw_command: " << cmd << endl;
              }
            }
            else
            { if (cmd == "TQ;"sv)       // was this a transmit query?
              { if (rig_communication_failures != 0)
                { ost << "status communication with rig restored after " << rig_communication_failures << " failure" << (rig_communication_failures == 1 ? "" : "s") << endl;
                  rig_communication_failures = 0;
                }
              }

              const ssize_t n_read { read(fd, c_in.data(), INBUF_SIZE / 2) };   // read a maximum of 500 characters into the static array; halve to provide margin of error

              if (n_read > 0)                      // should always be true
              { total_read += n_read;
                c_in[n_read] = static_cast<char>(0);    // append a null byte

                rcvd += string(c_in.data());

                if (rcvd.contains(SEMICOLON))
                  completed = true;
              }
            }
          }

          counter++;
        }
      }
    }
  }

  if (response_expected and completed)
  { if (_instrumented)
    { if (is_p3_screenshot)
        ost << "screenshot received from rig" << endl;
      else
        ost << "received from rig: " << to_printable_string(rcvd) << endl;
    }

    return rcvd;
  }

  return string { };
}

/// is the VFO locked?
bool rig_interface::is_locked(void) const
{ if (_model == RIG_MODEL_K3)
  { //if ( const string response { raw_command("LK;"sv, RESPONSE::EXPECTED) }; contains_at(response, ';', 3) and contains_at(response, "LK"sv, 0) )
    if ( const string response { raw_command("LK;"sv, RESPONSE::EXPECTED) }; contains_at(response, SEMICOLON, 3) and contains_at(response, "LK"sv, 0) )
      return (response[2] == '1');
    else
    { _error_alert("Invalid response getting locked status: "s + response);
      return false;  // default is unlocked
    }
  }
  else
  { SAFELOCK(_rig);

    int v;

    if (const int status { rig_get_func(_rigp, RIG_VFO_CURR, RIG_FUNC_LOCK, &v) }; status != RIG_OK)
    { _error_alert("Hamlib error getting lock status"s);
      return false;
    }

    return (v == 1);
  }
}

/// get the bandwidth as a string, in Hz
string rig_interface::bandwidth_str(void) const
{ if  ( (!_rig_connected) or (_model != RIG_MODEL_K3) )
    return string { };

  SAFELOCK(_rig);

//  if ( const string response { raw_command("BW;"sv, RESPONSE::EXPECTED) }; contains_at(response, ';', 6) and contains_at(response, "BW"sv, 0) )
  if ( const string response { raw_command("BW;"sv, RESPONSE::EXPECTED) }; contains_at(response, SEMICOLON, 6) and contains_at(response, "BW"sv, 0) )
  { const string bw_in_tens { remove_leading <std::string> (substring <std::string> (response, 2, 4), '0') };   // returned value is in tens of Hz, so convert to Hz

    return (bw_in_tens + '0');   // returned value is in tens of Hz, so convert to Hz
  }
  else
  { _error_alert("Invalid response getting bandwidth_str: "s + response);
    return string { };
  }
}

/*! \brief      Get the bandwidth in Hz
    \return     the current audio bandwidth, in hertz
*/
int rig_interface::bandwidth(void) const
{ if  ( (!_rig_connected) or (_model != RIG_MODEL_K3) )
    return 0;

  SAFELOCK(_rig);

  if ( const string response { raw_command("BW;"sv, RESPONSE::EXPECTED) }; contains_at(response, SEMICOLON, 6) and contains_at(response, "BW"sv, 0) )
    return from_string<int>(substring <std::string> (response, 2, 4)) * 10;
  else
  { _error_alert("Invalid response getting bandwidth: "s + response);
    return 0;
  }
}

/*! \brief      Get the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \return     the rig's most recent frequency for bandmode <i>bm</i>
*/
frequency rig_interface::get_last_frequency(const bandmode bm) const
{ SAFELOCK(_rig);

  return MUM_VALUE(_last_frequency, bm);    // returns empty frequency if not present in the map
}

/*! \brief      Set a new value for the most recent frequency for a particular band and mode
    \param  bm  band and mode
    \param  f   frequency
*/
void rig_interface::set_last_frequency(const bandmode bm, const frequency f)
{ SAFELOCK(_rig);

  const auto [b, m] { bm };

  if (BAND(f) != b)
  { alert("Attempt to set out-of-band frequency in set_last_frequency()"); 
    ost << "Attempt to set out-of-band frequency in set_last_frequency(): band = " << b << ", mode = " << m << ", f  " << f << endl;
  }
  else
    _last_frequency[bm] = f;
}

/*! \brief      Is the rig transmitting?
    \return     whether the rig is currently transmitting

    With the K3, this is unreliable: the routine frequently takes the _error_alert() path, even if the rig is not transmitting.
    (This is, unfortunately, just one example of the unreliability of the K3 in responding to commands. I could write a book;
    or at least a paper.)
*/
bool rig_interface::is_transmitting(void) const
{ constexpr int MAX_UNREACHABLE { 5 };              // maximum number of attempts before assuming rig is powered down

  static int n_unreachable { 0 };

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { if (const string response { raw_command("TQ;"sv, RESPONSE::EXPECTED) }; (contains_at(response, SEMICOLON, 3) and contains_at(response, "TQ"sv, 0)))
      { n_unreachable = 0;
        return  (response[2] == '1');
      }
      else
      { n_unreachable++;

        if (n_unreachable <= MAX_UNREACHABLE)
          _error_alert("Invalid response getting transmit status: "s + response);
        else
        { if (n_unreachable == (MAX_UNREACHABLE + 1))
            _error_alert("Assuming that rig is no longer physically connected and powered up");
        }

        return true;                // be paranoid
      }
    }
    else        // connected but not K3
      return false;            // this should really be true to be safe; but hamlib doesn't seem to provide a function, so we have to assume that it's OK to proceed
  }
  else              // no rig connected
    return false;
}

/*! \brief      Is the rig in TEST mode?
    \return     whether the rig is currently in TEST mode
*/
bool rig_interface::test(void) const
{ if (_model == RIG_MODEL_DUMMY)
    return true;

  bool rv { false };    // default

  if (_rig_connected)
  { if (TEST())
    { if (_model == RIG_MODEL_K3)
      { if ( const string response { raw_command("IC;"sv, RESPONSE::EXPECTED) }; contains_at(response, SEMICOLON, 7) and contains_at(response, "IC"sv, 0) )
        { const char c { response[2] };

          return (c bitand (1 << 5));
        }
        else
        { _error_alert("Invalid response retrieving icons and status: "s + response);
          return true;                // be paranoid
        }
      }
    }
  }

  return rv;
}

/*! \brief      Explicitly put the rig into or out of TEST mode
    \param  b   whether to enter TEST mode

    This works only with the K3.
*/
void rig_interface::test(const bool b) const
{ if (TEST())
  { if (test() != b)
    { if (_rig_connected)
      { if (_model == RIG_MODEL_K3)
          raw_command("SWH18;"sv);    // toggles state
      }
    }
  }
}

/*! \brief      Which VFO is currently used for transmitting?
    \return     the VFO that is currently set to be used when transmitting
*/
VFO rig_interface::tx_vfo(void) const
{ if (!_rig_connected)
    return VFO::A;

  return (split_enabled() ? VFO::B : VFO::A);  // this is the recommended procedure from the hamlib reflector
// I think this is ridiculous, because it does not allow for the case where the rig is in reverse split,
// with the TX on A and RX on B.

// Even worse, something like the next few lines, which logically should provide the desired result, is,
// according to comments on the hamlib reflector, NOT guaranteed to give the correct answer

/*
  vfo_t v;
  const int status = rig_get_vfo(_rigp, &v);

  if (status != RIG_OK)
    throw rig_interface_error(RIG_HAMLIB_ERROR, "Hamlib error getting active VFO");

  return (v == RIG_VFO_A ? VFO_A : VFO_B);
*/
}

/*! \brief      Set the bandwidth of VFO A
    \param  hz  desired bandwidth, in Hz
*/
void rig_interface::bandwidth_a(const unsigned int hz) const
{ constexpr std::chrono::duration RETRY_TIME { 100ms };       // period between retries for the brain-dead K3
  constexpr int                   PRECISION  { 50 };          // hertz

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)                             // astonishingly, there is no hamlib function to do this
    { wait_until_not_busy();

      const string k3_bw_units { pad_leftz(((hz + 5) / 10), 4) };
      const string bw_command  { "BW"s + k3_bw_units + SEMICOLON };

      raw_command(bw_command);

      while (abs(bandwidth() - static_cast<int>(hz)) > PRECISION)       // the K3 is brain-dead
      { sleep_for(RETRY_TIME);
        raw_command(bw_command);
      }
    }
  }
}

/*! \brief      Set the bandwidth of VFO B
    \param  hz  desired bandwidth, in Hz
*/
void rig_interface::bandwidth_b(const unsigned int hz) const
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)                             // astonishingly, there is no hamlib function to do this
    { const string k3_bw_units { pad_leftz(((hz + 5) / 10), 4) };

      raw_command("BW$"s + k3_bw_units + SEMICOLON);
    }
  }
}

/*! \brief      Get audio centre frequency, in Hz, as a printable string
    \return     The audio centre frequency, in Hz, as a printable string

    Works only with K3
*/
string rig_interface::centre_frequency_str(void) const
{ if  ( (!_rig_connected) or (_model != RIG_MODEL_K3) )
    return string { };

  SAFELOCK(_rig);

  if ( const string response { raw_command("IS;"sv, RESPONSE::EXPECTED) }; contains_at(response, SEMICOLON, 7) and contains_at(response, "IS"sv, 0) )
    return remove_leading <std::string> (substring <std::string_view> (response, 3, 4), '0');
  else
  { _error_alert("Invalid response getting centre frequency: "s + response);
    return string { };
  }
}

/*! \brief      Get audio centre frequency, in Hz
    \return     The audio centre frequency, in Hz

    Works only with K3
*/
unsigned int rig_interface::centre_frequency(void) const
{ if  ( (!_rig_connected) or (_model != RIG_MODEL_K3) )
    return 0;

  SAFELOCK(_rig);

  return from_string<unsigned int>(centre_frequency_str());
}

/*! \brief      Set audio centre frequency
    \patam  fc  the audio centre frequency, in Hz

    Works only with K3
*/
void rig_interface::centre_frequency(const unsigned int fc) const
{ if ( (!_rig_connected) or (_model != RIG_MODEL_K3) )
    return;

  SAFELOCK(_rig);

  const string fc_str { pad_leftz(fc, 4) };

  if (fc_str.length() != 4)
  { const string msg { "ERROR: length of centre frequency string != 4: "s + fc_str  };

    ost << msg << endl;
    _error_alert(msg);

    return; 
  }

  raw_command( ("IS "s + fc_str + SEMICOLON), RESPONSE::NOT_EXPECTED);
}

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use

    Works only with K3
*/
bool rig_interface::rx_ant(void) const
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { const string result { raw_command("AR;"s, RESPONSE::EXPECTED) };

      if ( (result != "AR0;"sv) and (result != "AR1;"sv) )
        ost << "ERROR in rx_ant(): result = " << result << endl;

      return (result == "AR1;"sv);
    }
  }

  return false;
}

/*! \brief          Control use of the RX antenna
    \param  torf    whether to use the RX antenna

    Works only with K3
*/
void rig_interface::rx_ant(const bool torf) const
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
      raw_command( torf ? "AR1;"sv : "AR0;"sv );
  }
}

/*! \brief              Is notch enabled?
    \param  ds_result   previously-obtained result of a DS command, or empty string
    \return             whether notch is currently enabled

    Works only with K3
*/
bool rig_interface::notch_enabled(const string_view ds_result) const
{ constexpr char K3_NOTCH_BIT { 0b00000010 };

  if (!_rig_connected)
    return false;

  if (_model != RIG_MODEL_K3)
    return false;

// it's a K3
  const string result { ds_result.empty() ? raw_command("DS;"sv, RESPONSE::EXPECTED) : ds_result };

  if ( (result.size() != 13) or (result[12] != SEMICOLON) )
    ost << "ERROR in notch_enabled(); result = " << result << endl;
  else
  { const char c         { result[11] };    // icon flash data
    const bool notch_bit { (c bitand K3_NOTCH_BIT) == K3_NOTCH_BIT };

    return notch_bit;
  }

  return false;
}

/*! \brief  Toggle the notch status

    Works only with K3
*/
void rig_interface::toggle_notch_status(void) const
{ if (AUTO_NOTCH())
  { if (_model == RIG_MODEL_K3)
      k3_press(K3_BUTTON_TAP::NOTCH);
  }
  else
    ost << "Attempt to control notch status on rig without AUTO_NOTCH capability" << endl;
}

/*! \brief      Set the K3 command mode (either NORMAL or EXTENDED)
    \param  cm  command mode

    Works only with K3
*/
void rig_interface::k3_command_mode(const K3_COMMAND_MODE cm)
{ if (_model == RIG_MODEL_K3)
  { switch (cm)
    { case K3_COMMAND_MODE::EXTENDED :
        raw_command("K31;"sv);
        break;

      case K3_COMMAND_MODE::NORMAL :
        raw_command("K30;"sv);
        break;
    }
  }
}

/*! \brief      Get the K3 command mode (either NORMAL or EXTENDED)
    \return     the K3 command mode

    Works only with K3
*/
K3_COMMAND_MODE rig_interface::k3_command_mode(void) const
{ if (_model == RIG_MODEL_K3)
  { const string result { raw_command("K3;", RESPONSE::EXPECTED) };

    if (result == "K31"sv)
      return K3_COMMAND_MODE::EXTENDED;
  }

  return K3_COMMAND_MODE::NORMAL;
}

/*! \brief          Emulate the tapping or holding of a K3 button
    \param  button  the K3 button to tap or hold

    Works only with K3
*/
void rig_interface::k3_press(const variant<K3_BUTTON_TAP, K3_BUTTON_HOLD>& button) const
{ if (_model == RIG_MODEL_K3)
  { const int    button_int { std::visit([] (auto&& arg) -> int { return static_cast<int>(arg); }, button) };
    const string button_str { pad_leftz(button_int, 2) };
    const string cmd        { (holds_alternative<K3_BUTTON_TAP>(button) ? "SWT"s : "SWH"s) + button_str + SEMICOLON };

    raw_command(cmd);
  }
}

/*! \brief          Emulate double-tapping a K3 button
    \param  button  the K3 button to tap

    Works only with K3 this is CURRENTLY UNUSED
*/
void rig_interface::k3_double_tap(const K3_BUTTON_TAP button) const
{ if (_model == RIG_MODEL_K3)
  { constexpr std::chrono::duration K3_TIME_BETWEEN_TAPS { 50ms };

    k3_press(button);
    sleep_for(K3_TIME_BETWEEN_TAPS);
    k3_press(button);
  }
}

/*! \brief      Set audio centre frequency and width
    \param  af  the characteristics to set 
*/
void rig_interface::filter(const audio_filter& af) const
{ //const int bw { af.bandwidth() };
  //const int cf { af.centre() };

  if (const int bw { af.bandwidth() }; bw)
    bandwidth(bw);

  if (const int cf { af.centre() }; cf)
    centre_frequency(cf);
}

/// register a function for alerting the user
void rig_interface::register_error_alert_function(void (*error_alert_function)(const string_view) )
{ SAFELOCK(_rig);
  _error_alert_function = error_alert_function;
}

/// set RIT, split, sub-rx off
void rig_interface::base_state(void) const
{ constexpr std::chrono::duration K3_INTERCOMMAND_TIME { 1s };      // the K3 is awfully slow; this should allow plenty of time between commands

  auto pause { [ K3_INTERCOMMAND_TIME, this] (void) { if (_model == RIG_MODEL_K3) sleep_for(K3_INTERCOMMAND_TIME); } };

  if (rit_enabled())
  { rit_disable();
    pause();
  }

  if (split_enabled())
  { split_disable();
    pause();
  }

  if (sub_receiver_enabled())
  { sub_receiver_disable();
    pause();
  }
}

/*! \brief      Turn on instrumentation
*/
void rig_interface::instrument(void)
{ if (_rig_connected)
    _instrumented = true;
}

/*! \brief      Turn off instrumentation
*/
void rig_interface::uninstrument(void)
{ if (_rig_connected)
    _instrumented = false;
}

/*! \brief                  Put the current VFOs into a rig memory
    \param  rig_memory_nr   the number of the rig memory into which the VFO frequencies are to be stored (0 <= value <= 99)

    This works only on a K3.

    The documentation for the K3 MC command doesn't explain th edifference between setting and getting a memory
*/
#if 0
void rig_interface::set_rig_memory(const int rig_memory_nr) const
{ if (_model != RIG_MODEL_K3)
    return;

  const int    memory_nr     { max(min(rig_memory_nr, 99), 0) };
  const string memory_nr_str { pad_leftz(to_string(memory_nr), 3) };
  const string cmd           { "MC"s + memory_nr_str + SEMICOLON };

  raw_command(cmd);
}
#endif

/*! \brief                  Set the VFOs from a rig memory
    \param  rig_memory_nr   the number of the rig memory from which the VFO frequencies are to be (non-destructively) recalled (0 <= value <= 99)

    This works only on a K3.

    The documentation for the K3 MC command doesn't explain th edifference between setting and getting a memory
*/
#if 0
void rig_interface::get_rig_memory(const int rig_memory_nr) const
{ if (_model != RIG_MODEL_K3)
    return;

  const int    memory_nr     { max(min(rig_memory_nr, 99), 0) };
  const string memory_nr_str { pad_leftz(to_string(memory_nr), 3) };
  const string cmd           { "MC"s + memory_nr_str + SEMICOLON };

  raw_command(cmd);
}
#endif

/// Poll the rig for the important status information; only one thread should ever call this function
polled_status rig_interface::poll(void)
{
// for now use an old-fashioned switch. Probably replace with polymorphism when it's all working

  switch (_model)
  { case RIG_MODEL_K3 :
    { const string status_str { raw_command("IF;"sv, RESPONSE::EXPECTED) };               // K3 should return 38 characters

      if (status_str.length() == K3_STATUS_REPLY_LENGTH)
      { if (const k3_status rig_status { status_str }; rig_status)    // execute all this only if the response from the rig is sensible
        { const frequency f_a   { rig_status.freq() };                  // frequency of VFO A
          const frequency f_b   { rig_frequency_b() };                  // frequency of VFO B
          const MODE      m     { rig_mode() };                         // mode

          bool notch { false };                                         // whether notch is enabled

          if (m == MODE::MODE_SSB)
          { if (k3_command_mode() == K3_COMMAND_MODE::NORMAL)  // NB this is the mode drlog /thinks/ is correct, but it saves time
              k3_command_mode(K3_COMMAND_MODE::EXTENDED);

            const string ds_reply_str { raw_command("DS;"sv, RESPONSE::EXPECTED) };  // K3 should return 13 characters; currently needed only in SSB

            if (ds_reply_str.length() == K3_DS_REPLY_LENGTH)
              notch = notch_enabled(ds_reply_str);   // only needed in SSB
          }

          using enum K3_OPERATING_MODE;

          const auto   om       { rig_status.op_mode() };
          const string mode_str { (om == LSB) ? "LSB "s : ( (om == USB) ?"USB "s : ( (om == CW) ? "CW "s : "UNK "s) ) };

          const int abs_rit_xit_offset { rig_status.abs_rit_offset().hz() };
          const int rit_xit_offset     { rig_status.rit_positive() ? abs_rit_xit_offset : -abs_rit_xit_offset };
          const int rit_offset         { rig_status.rit_is_on() ? rit_xit_offset : 0 };
          const int xit_offset         { rig_status.xit_is_on() ? rit_xit_offset : 0 };

// update the status object
          _status.f_a(f_a);
          _status.f_b(f_b);
          _status.m(m);
          _status.notch(notch);
          _status.mode_str(mode_str);
          _status.rit_enabled(rig_status.rit_is_on());
          _status.xit_enabled(rig_status.xit_is_on());
          _status.rit_offset(rit_offset);
          _status.xit_offset(xit_offset);
          _status.split_enabled(rig_status.is_split());
          _status.bw_str(bandwidth_str());
          _status.centre_str(centre_frequency_str());
          _status.is_locked(is_locked());
          _status.sub_rx_enabled(sub_receiver_enabled());
          _status.test_mode(test());
          _status.rx_ant(rx_ant());
        }
      }

      break;
    }

//    case RIG_MODEL_IC7610 :
//    {
//    }

// NB I may be mistaken about some of these; the documentation for HAMLIB is, shall we say, sparse
// I welcome corrections, particularly about capabilities that really are supported by HAMLIB
// (and if a capability *is* incorrectly noted below as unsupported by HAMLIB, PLEASE EXPLAIN HOW TO READ
// THE RELEVANT VALUE FROM THE RIG -- or provide code to do so)
    default :             // HAMLIB
    { if (VFO_A())        // should always be true
        _status.f_a(_rig_frequency(VFO::A));

      if (VFO_B())
        _status.f_b(_rig_frequency(VFO::B));

      _status.m(rig_mode());    // either MODE_CW or MODE_SSB

      if (AUTO_NOTCH())
        _status.notch(notch_enabled());

      static const map<rmode_t, string> hamlib_mode_to_str { { RIG_MODE_CW, "CW "s },
                                                             { RIG_MODE_LSB, "LSB "s },
                                                             { RIG_MODE_USB, "USB "s }
                                                           };

      pbwidth_t tmp_bandwidth;
      rmode_t   tmp_mode;

      { SAFELOCK(_rig);
        rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);
      }

      _status.mode_str(MUM_VALUE(hamlib_mode_to_str, tmp_mode, "UNK "s));

// the HAMLIB model appears to be that there is no distinct R/XIT ENABLED capability;
// rather it appears to assume that ENABLED is synonymous with non-zero (shakes head)
      if (RIT())
      { SAFELOCK(_rig);

        shortfreq_t hz { 0 };

        if (const int status { rig_get_rit(_rigp, RIG_VFO_CURR, &hz) }; status != RIG_OK)
          _error_alert("Hamlib error while getting RIT offset"s);

        _status.rit_enabled( (hz != 0) );

        if (hz)
          _status.rit_offset(hz);
      }

      if (XIT())
      { SAFELOCK(_rig);

        shortfreq_t hz { 0 };

        if (const int status { rig_get_xit(_rigp, RIG_VFO_CURR, &hz) }; status != RIG_OK)
          _error_alert("Hamlib error while getting XIT offset"s);

        _status.xit_enabled( (hz != 0) );

        if (hz)
          _status.xit_offset(hz);
      }

      if (SPLIT())
        _status.split_enabled(split_enabled());

      if (AUDIO_BW())
      { pbwidth_t tmp_bandwidth;
        rmode_t   tmp_mode;

        SAFELOCK(_rig);

        int status = rig_get_mode(_rigp, RIG_VFO_CURR, &tmp_mode, &tmp_bandwidth);

        if (status != RIG_OK)
          _error_alert("Error getting mode prior to setting mode");
        else
          _status.bw_str(::to_string(tmp_bandwidth));
      }

      if (AUDIO_CENTRE())   // this doesn't seem to be supported by HAMLIB
        { }

      if (LOCK_A())         // this doesn't seem to be supported by HAMLIB
        { }

      if (LOCK_B())         // this doesn't seem to be supported by HAMLIB
        { }

      if (SUB_RX())
      { if (rig_has_get_func(_rigp, RIG_FUNC_DUAL_WATCH))
        { int value;

          int retcode = rig_get_func(_rigp, RIG_VFO_CURR, RIG_FUNC_DUAL_WATCH, &value);

          if (retcode != RIG_OK)
            ost << "rig_get_func_dual_watch error: %s\n" << rigerror(retcode) << endl;
          else
            _status.sub_rx_enabled(value != 0);
        }
      }
    }
  }

  return _status;
}

/*! \brief      Set frequency of a VFO
    \param  f   new frequency
    \param  v   VFO

    Does nothing if <i>f</i> is not within a ham band.
    Attempts to confirm that the frequency was actually set to <i>f</i>.
*/
void elecraft_k3_interface::_rig_frequency(const frequency f, const VFO v)
{ constexpr std::chrono::duration RETRY_TIME { 100ms };      // period between retries

  if (f.is_within_ham_band())
  { switch (v)
    { case VFO::A :
        _last_commanded_frequency = f;
        break;

      case VFO::B :
        _last_commanded_frequency_b = f;
        break;
    }

    if (_rig_connected)
    { SAFELOCK(_rig);           // hold the lock until we have received confirmation that the frequency is correct

//      if (_model == RIG_MODEL_K3)
      { bool retry { true };

        constexpr int MAX_RETRIES { 2 };

        int n_retries { 0 };

        while ( (retry) and (n_retries++ <= MAX_RETRIES) )
        { raw_command( ((v == VFO::A) ? "FA"s : "FB"s) + pad_leftz(to_string(f.hz()), 11) + ';');

          const frequency rf { _rig_frequency(v) };

          if (f != rf)
          { const string msg { "frequency mismatch: commanded = "s + to_string(f.hz()) + "; actual = "s + to_string(rf.hz()) };

            _error_alert(msg);
            ost << msg << "; retrying" << endl;


            if (n_retries <= MAX_RETRIES)
              sleep_for(RETRY_TIME);
            else
            ost << "Maximum number of retries exceeded; quitting attempt to set frequency" << endl;
          }
          else
            retry = false;
        }
      }
#if 0
      else      // not K3
      { bool retry { true };

        constexpr int MAX_RETRIES { 2 };

        int n_retries { 0 };

        while ( (retry) and (n_retries++ <= MAX_RETRIES) )
        { if (const int status { rig_set_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_A : RIG_VFO_B ), f.hz()) }; status != RIG_OK)
              _error_alert("Error setting frequency of VFO "s + ((v == VFO::A) ? "A"s : "B"s));

          if (debug)
            ost << "commanded frequency = " << to_string(f.hz()) << "; actual frequency = " << to_string(_rig_frequency(v).hz()) << endl;

          if (const auto rf {_rig_frequency(v)}; rf != f)     // explicitly check the frequency -- hamlib, however, will simply assume for a period of one second that the SET worked!!!

          { const string msg { "frequency mismatch: commanded = "s + to_string(f.hz()) + "; actual = "s + to_string(rf.hz()) };

            _error_alert(msg);
            ost << msg << "; retrying" << endl;
//          ost << "frequency mismatch: commanded = " << f << "; actual = " << rf << "; retrying" << endl;        // DEBUG for when we get stuck

            if (n_retries <= MAX_RETRIES)
              sleep_for(RETRY_TIME);
            else
              ost << "Maximum number of retries exceeded; quitting attempt to set frequency" << endl;
          }
          else
            retry = false;
        }
      }
#endif
    }
  }
  else
    _error_alert("Commanded frequency is not within ham band: "s + to_string(f.hz()) + " Hz"s);
}

/*! \brief      Get the frequency of a VFO
    \param  v   VFO
    \return     frequency of v
*/
frequency elecraft_k3_interface::_rig_frequency(const VFO v) const
{ if (!_rig_connected)
    return ( (v == VFO::A) ? _last_commanded_frequency : _last_commanded_frequency_b);

  constexpr int                   MAX_RETRIES { 2 };
  constexpr std::chrono::duration RETRY_TIME  { 100ms };      // period between retries

  int       counter { 0 };
  frequency rv      { 0 };

  while ( (counter++ < MAX_RETRIES) and !rv )
  { const string response { raw_command( (v == VFO::A) ? "FA;"sv : "FB;"sv, RESPONSE::EXPECTED) };

    rv = frequency(from_string<uint32_t>(substring <string_view> (response, 2, 11)));

    if (!rv)
      sleep_for(RETRY_TIME);
  }

  if (!rv)
    ost << "Warning: returning 0 from _rig_frequency, VFO " << ((v == VFO::A) ? "A" : "B") << endl;

  return rv;
}

/*! \brief              Prepare rig for use
    \param  context     context for the contest
*/
void elecraft_k3_interface::prepare(const drlog_context& context)
{ ost << "preparing K3 interface for use" << endl;

  rig_interface::prepare(context);

  using enum RIG_CAPABILITIES;

  _rcaps = rig_capabilities(std::set<RIG_CAPABILITIES> { VFO_A, VFO_B,    RIT,    XIT,  EQUAL_RIT_XIT_QRG,
                                                         SPLIT, LOCK_A,   SUB_RX, TEST, RX_ANT,
                                                         AUTO_NOTCH, AUDIO_BW, AUDIO_CENTRE
                                                       });

  k3_command_mode(K3_COMMAND_MODE::EXTENDED);
}

// --------------------------------------- rig_capabilities ----------------------

/*! \class  rig_capabilities
    \brief  The capabilities of a rig
*/

rig_capabilities::rig_capabilities(const std::string& fn)
{
#define READ_CAPABILITY(y) \
  if (sv == #y) \
  { set(RIG_CAPABILITIES::y);  \
    goto end_test;  \
  }

  if (file_exists(fn))
  { const vector<string> contents { ::to_lines <string> (read_file(fn)) };

    for (const string& line : contents)
    { const string uc      { to_upper(line) };
      const string_view sv { remove_peripheral_spaces <string_view> (remove_trailing_comment <string_view> (uc)) };

      if (!sv.empty())
      { //if (sv == "VFO_A"sv)
        //{ set(RIG_CAPABILITIES::VFO_A);
        //  goto end_test;
        //}
        READ_CAPABILITY(VFO_A);
        READ_CAPABILITY(VFO_B);
        READ_CAPABILITY(RIT);
        READ_CAPABILITY(XIT);
        READ_CAPABILITY(EQUAL_RIT_XIT_QRG);
        READ_CAPABILITY(SPLIT);
        READ_CAPABILITY(REVERSE_SPLIT);
        READ_CAPABILITY(LOCK_A);
        READ_CAPABILITY(LOCK_B);
        READ_CAPABILITY(SUB_RX);
        READ_CAPABILITY(TEST);
        READ_CAPABILITY(RX_ANT);
        READ_CAPABILITY(AUTO_NOTCH);
        READ_CAPABILITY(AUDIO_BW);
        READ_CAPABILITY(AUDIO_CENTRE);

//        if (sv == "VFO_B"sv)
//          set(RIG_CAPABILITIES::VFO_B);

//        if (sv == "RIT"sv)
//          set(RIG_CAPABILITIES::RIT);

end_test:
      }
    }
  }

#undef READ_CAPABILITY
}

rig_capabilities::rig_capabilities(const std::vector<std::string>& path, const std::string& fn)
{ const string valid_filename { find_file(path, fn) };

  if (!valid_filename.empty())
    *this = rig_capabilities { valid_filename };
}

void rig_capabilities::set(const RIG_CAPABILITIES rc)
{
#define SET_CAPABILITY(y) \
    case RIG_CAPABILITIES::y : \
      y##_set(); \
      break;

  switch (rc)
  { SET_CAPABILITY(VFO_A);
    SET_CAPABILITY(VFO_B);
    SET_CAPABILITY(RIT);
    SET_CAPABILITY(XIT);
    SET_CAPABILITY(EQUAL_RIT_XIT_QRG);
    SET_CAPABILITY(SPLIT);
    SET_CAPABILITY(REVERSE_SPLIT);
    SET_CAPABILITY(LOCK_A);
    SET_CAPABILITY(LOCK_B);
    SET_CAPABILITY(SUB_RX);
    SET_CAPABILITY(TEST);
    SET_CAPABILITY(RX_ANT);
    SET_CAPABILITY(AUTO_NOTCH);
    SET_CAPABILITY(AUDIO_BW);
    SET_CAPABILITY(AUDIO_CENTRE);

    //case RIG_CAPABILITIES::VFO_A :
    //  VFO_A_set();
    //  break;

    //case RIG_CAPABILITIES::VFO_B :
    //  VFO_B_set();
    //  break;

    default :
      break;
  }

#undef SET_CAPABILITY
}

void rig_capabilities::clear(const RIG_CAPABILITIES rc)
{
#define CLEAR_CAPABILITY(y) \
    case RIG_CAPABILITIES::y : \
      y##_clear(); \
      break;

  switch (rc)
  { CLEAR_CAPABILITY(VFO_A);
    CLEAR_CAPABILITY(VFO_B);
    CLEAR_CAPABILITY(RIT);
    CLEAR_CAPABILITY(XIT);
    CLEAR_CAPABILITY(EQUAL_RIT_XIT_QRG);
    CLEAR_CAPABILITY(SPLIT);
    CLEAR_CAPABILITY(REVERSE_SPLIT);
    CLEAR_CAPABILITY(LOCK_A);
    CLEAR_CAPABILITY(LOCK_B);
    CLEAR_CAPABILITY(SUB_RX);
    CLEAR_CAPABILITY(TEST);
    CLEAR_CAPABILITY(RX_ANT);
    CLEAR_CAPABILITY(AUTO_NOTCH);
    CLEAR_CAPABILITY(AUDIO_BW);
    CLEAR_CAPABILITY(AUDIO_CENTRE);
//    case RIG_CAPABILITIES::VFO_A :
//      VFO_A_clear();
//      break;

//    case RIG_CAPABILITIES::VFO_B :
//      VFO_B_clear();
//      break;

    default :
      break;
  }

#undef CLEAR_CAPABILITY

}

string rig_capabilities::to_string(void) const
{ string rv { };

#define WRITE_OUT(y) rv += ""s #y " IS "s + (y() ? ""s : "NOT "s) + "set"s + EOL;

//  rv += "VFO_A IS "s + (VFO_A() ? ""s : "NOT "s) + "set"s + EOL;
//  rv += "VFO_B IS "s + (VFO_B() ? ""s : "NOT "s) + "set"s + EOL;
  WRITE_OUT(VFO_A);
  WRITE_OUT(VFO_B);
  WRITE_OUT(RIT);
  WRITE_OUT(XIT);
  WRITE_OUT(EQUAL_RIT_XIT_QRG);
  WRITE_OUT(SPLIT);
  WRITE_OUT(REVERSE_SPLIT);
  WRITE_OUT(LOCK_A);
  WRITE_OUT(LOCK_B);
  WRITE_OUT(SUB_RX);
  WRITE_OUT(TEST);
  WRITE_OUT(RX_ANT);
  WRITE_OUT(AUTO_NOTCH);
  WRITE_OUT(AUDIO_BW);
  WRITE_OUT(AUDIO_CENTRE);

#undef WRITE_OUT

  return rv;
}

// -------------------------------------------------------------------------------

/*! \brief      Convert a hamlib error code to a printable string
    \param  e   hamlib error code
    \return     Printable string corresponding to error code <i>e</i>
*/
string hamlib_error_code_to_string(const int e)
{ switch (e)
  { case RIG_OK :
      return "No error, operation completed sucessfully"s;

    case RIG_EINVAL :
      return "Invalid parameter"s;

    case RIG_ECONF :
      return "Invalid configuration (serial,..)"s;

    case RIG_ENOMEM :
      return "Memory shortage"s;

    case RIG_ENIMPL :
      return "Function not implemented, but will be"s;

    case RIG_ETIMEOUT :
      return "Communication timed out"s;

    case RIG_EIO :
      return "IO error, including open failed"s;

    case RIG_EINTERNAL :
      return "Internal Hamlib error, huh!"s;

    case RIG_EPROTO :
      return "Protocol error"s;

    case RIG_ERJCTED :
      return "Command rejected by the rig"s;

    case RIG_ETRUNC :
      return "Command performed, but arg truncated"s;

    case RIG_ENAVAIL :
      return "Function not available"s;

    case RIG_ENTARGET :
      return "VFO not targetable"s;

    case RIG_BUSERROR :
      return "Error talking on the bus"s;

    case RIG_BUSBUSY :
      return "Collision on the bus"s;

    case RIG_EARG :
      return "NULL RIG handle or any invalid pointer parameter in get arg"s;

    case RIG_EVFO :
      return "Invalid VFO"s;

    case RIG_EDOM :
      return "Argument out of domain of function"s;

    default :
      return "Unknown hamlib error number: "s + to_string(e);
  }
}
