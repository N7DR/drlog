// $Id: rig_interface.cpp 272 2025-07-13 22:28:31Z  $

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

#include "hamlib/serial.h"

#include <chrono>
#include <iostream>
#include <thread>

#include <cstring>

#include <termios.h>
#include <cstdio>
#include <fcntl.h>

using namespace std;
using namespace   chrono;        // std::chrono
using namespace   this_thread;   // std::this_thread

using namespace std::literals::chrono_literals;

extern bool debug;                             ///< debug frequency changes
extern bool rig_is_split;

//extern void alert(const string& msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);     ///< alert the user (not used for errors)
extern void alert(const string_view msg, const SHOW_TIME show_time = SHOW_TIME::SHOW);     ///< alert the user (not used for errors)

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
 *   The fundamental problem in all this is that the protocol has no concept of a transaction. It is unclear why simple 1980s-era
 *   protocols are still being used to exchange information with rigs. Baud rates now are fast enough that, even for serial lines, real
 *   protocols could be run over, for example, SLIP.
*/

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
//void rig_interface::_error_alert(const string& msg) const
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
//void rig_interface::_rig_frequency(const frequency& f, const VFO v)
void rig_interface::_rig_frequency(const frequency f, const VFO v)
{ //constexpr std::chrono::milliseconds RETRY_TIME { milliseconds(100) };      // period between retries
  constexpr std::chrono::duration RETRY_TIME { 100ms };      // period between retries

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

      bool retry { true };

      constexpr int MAX_RETRIES { 2 };

      int n_retries { 0 };

// the brain-dead K3 will sometimes infinitely fail to go to the correct frequency
      while ( (retry) and (n_retries++ <= MAX_RETRIES) )
      { if (const int status { rig_set_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_A : RIG_VFO_B ), f.hz()) }; status != RIG_OK)
          _error_alert("Error setting frequency of VFO "s + ((v == VFO::A) ? "A"s : "B"s));

//        if (debug)
        { ost << "commanded frequency = " << to_string(f.hz()) << "; actual frequency = " << to_string(_rig_frequency(v).hz()) << endl;
        }

        if (const auto rf {_rig_frequency(v)}; rf != f)     // explicitly check the frequency
// if we use the following line, then sometimes we get trapped and cannot move down in frequency
// if the band marker is just *above* a bm entry (say, if we have manually moved up slightly in frequency)
//        if (const auto rf {_rig_frequency(v)}; (abs(rf - f) > MAX_ERROR) )    // explicitly check the frequency
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
  else
  { freq_t hz;

    SAFELOCK(_rig);

    if (const int status { rig_get_freq(_rigp, ( (v == VFO::A) ? RIG_VFO_CURR : RIG_VFO_B ), &hz) }; status != RIG_OK)
    { _error_alert("Error getting frequency of VFO "s + ((v == VFO::A) ? "A"s : "B"s));
      return ( (v == VFO::A) ? _last_commanded_frequency : _last_commanded_frequency_b) ;
    }

    return frequency(hz);
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
*/
string rig_interface::_retried_raw_command(const string& cmd, /*const int expected_len, */const int timeout_ms, const int n_retries)
{ string rv;

  int counter    { 0 };
  bool completed { false };

  while (!completed and (counter != n_retries))  
  { rv = raw_command(cmd, RESPONSE::EXPECTED/*, expected_len*/);    // issue the command each time

    completed = (rv.empty() ? false : last_char(rv) == ';');

    if (!completed)
    { counter++;
      sleep_for(milliseconds(timeout_ms));
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
{ _port_name = context.rig1_port();
  rig_set_debug(RIG_DEBUG_NONE);
  rig_load_all_backends();              // this function returns an int -- in true Linux fashion, there is no documentation as to possible values and their meaning
                                        // see: http://hamlib.sourceforge.net/manuals/1.2.15/group__rig.html

  const string rig_type { context.rig1_type() };

// ugly map of name to hamlib model number
  if (rig_type == "K3"sv)
    _model = RIG_MODEL_K3;

  if (_model == RIG_MODEL_DUMMY and !rig_type.empty())
    _error_alert("Unknown rig type: "s + rig_type);

  _rigp = rig_init(_model);

  if (!_rigp)
    throw rig_interface_error(RIG_UNABLE_TO_INITIALISE, "Unable to initialise rig structure for rig type "s + (rig_type.empty() ? "DUMMY"s : rig_type));

  if (_model != RIG_MODEL_DUMMY)        // hamlib documentation is unclear as to whether this is necessary; probably not
  { baud_rate(context.rig1_baud());
    data_bits(context.rig1_data_bits());
    stop_bits(context.rig1_stop_bits());

    if (_port_name.size() >= (FILPATHLEN - 2))                        // ridiculous C-ism
    { const string msg { "Port name is too long: "s + _port_name };

      _error_alert(msg);
      ost << msg << endl;
      exit(-1);
    }

    strncpy(_rigp->state.rigport.pathname, _port_name.c_str(), FILPATHLEN - 1);     // !! -1 to remove compiler warning about length
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
            { const string k3_mode_cmd { (rig_frequency().mhz() < 10) ? "MD1;"sv : "MD2;"sv };

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
        hamlib_m = ( (rig_frequency().mhz() < 10) ? RIG_MODE_LSB : RIG_MODE_USB );

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
  { raw_command("FT1;"s);
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
  { raw_command("FR0;"s);
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

    if ( const string transmit_vfo { raw_command("FT;"s, RESPONSE::EXPECTED) }; contains_at(transmit_vfo, ';', 3) and contains_at(transmit_vfo, "FT"s, 0) )
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
      raw_command("RC;"s);
    else
    { const int    positive_hz { abs(hz) };
      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s) + pad_leftz(positive_hz, 4) };

      raw_command("RO"s + hz_str + ";"s);
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
  { if ( const string value { raw_command("RO;"s, RESPONSE::EXPECTED) }; contains_at(value, ';', 7) and contains_at(value, "RO"s, 0) )
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
    raw_command("RT1;"s);           // proper enable for the K3
  else
    rit(1);                         // 1 Hz offset, since a zero offset would disable RIT
}

/*! \brief  Turn rit off

    This is a kludge, as hamlib equates an offset of zero with rit turned off (!)
*/
void rig_interface::rit_disable(void) const
{ if (_model == RIG_MODEL_K3)
    raw_command("RT0;"s); // proper disable for the K3
  else
    rit(0);                         // 0 Hz offset, which hamlib regards as disabling RIT
}

/// is rit enabled?
bool rig_interface::rit_enabled(void) const
{ switch (_model)
  { case RIG_MODEL_K3 :
    { if ( const string response { raw_command("RT;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 3) and contains_at(response, "RT"s, 0) )
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
    raw_command("XT1;"s);
  else
    xit(1);                 // 1 Hz offset
}

/*! \brief  Turn xit off

    This is a kludge, as hamlib equates an offset of zero with xit turned off (!)
*/
void rig_interface::xit_disable(void) const
{ if (_model == RIG_MODEL_K3)
    raw_command("XT0;"s);
  else
    xit(1);                 // 1 Hz offset
}

/// is xit enabled?
bool rig_interface::xit_enabled(void) const
{ switch (_model)
  { case RIG_MODEL_K3 :
    { if ( const string response { raw_command("XT;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 3) and contains_at(response, "XT"s, 0) )
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
      raw_command("RC;"s);
    else
    { const int    positive_hz { abs(hz) };
      const string hz_str      { ( (hz >= 0) ? "+"s : "-"s ) + pad_leftz(positive_hz, 4) };

      raw_command("RO"s + hz_str + ";"s);
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
  { if (const string response { raw_command("SB;"s, RESPONSE::EXPECTED) }; response.length() < 3)
    { _error_alert("SUBRX Short response"s);
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
  { const string cmd { "KS"s + pad_leftz(wpm, 3) + ";"s };

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
  { if ( const string response { raw_command("KS;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 5) and contains_at(response, "KS"s, 0) )
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
  if (last_char(cmd) != ';')
  {  _error_alert("Invalid rig command: "s + cmd);
    return string();
  }

  bool completed { false };

  { SAFELOCK(_rig);             // hold lock until we're done

    serial_flush(&rs_p->rigport);

    if (_instrumented)
      ost << "sent to rig: " << cmd << endl;

    write(fd, cmd.data(), cmd.length());     // send the command

    serial_flush(&rs_p->rigport);
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

                if (contains(rcvd, ';'))
                  completed = true;
              }
            }
          }

          counter++;
        }
      }
    }
  }

  if (response_expected and !completed)
  { // _error_alert("Incomplete response from rig to cmd: " + cmd + " length = " + to_string(rcvd.length()) + " " + rcvd);
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
  { if ( const string response { raw_command("LK;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 3) and contains_at(response, "LK"s, 0) )
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

  if ( const string response { raw_command("BW;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 6) and contains_at(response, "BW"sv, 0) )
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

  if ( const string response { raw_command("BW;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 6) and contains_at(response, "BW"sv, 0) )
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
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { if (const string response { raw_command("TQ;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 3) and contains_at(response, "TQ"s, 0))
        return  (response[2] == '1');
      else
      { _error_alert("Invalid response getting transmit status: "s + response);
        return true;                // be paranoid
      }
    }
    else        // connected but not K3
      return true;            // be paranoid
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
  { if (_model == RIG_MODEL_K3)
    { if ( const string response { raw_command("IC;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 7) and contains_at(response, "IC"sv, 0) )
      { const char c { response[2] };

        return (c bitand (1 << 5));
      }
      else
      { _error_alert("Invalid response retrieving icons and status: "s + response);
        return true;                // be paranoid
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
{ if (test() != b)
  { if (_rig_connected)
    { if (_model == RIG_MODEL_K3)
        raw_command("SWH18;"s);    // toggles state
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
{ //constexpr std::chrono::milliseconds RETRY_TIME { 100ms };       // period between retries for the brain-dead K3
  constexpr std::chrono::duration RETRY_TIME { 100ms };       // period between retries for the brain-dead K3
  constexpr int                   PRECISION  { 50 };          // hertz

  if (_rig_connected)
  { if (_model == RIG_MODEL_K3)                             // astonishingly, there is no hamlib function to do this
    { wait_until_not_busy();

      const string k3_bw_units { pad_leftz(((hz + 5) / 10), 4) };
      const string bw_command  { "BW"s + k3_bw_units + ";"s };

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

      raw_command("BW$"s + k3_bw_units + ";"s);
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

  if ( const string response { raw_command("IS;"s, RESPONSE::EXPECTED) }; contains_at(response, ';', 7) and contains_at(response, "IS"s, 0) )
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

  raw_command( ("IS "s + fc_str + ";"s), RESPONSE::NOT_EXPECTED);
}

/*! \brief      Is an RX antenna in use?
    \return     whether an RX antenna is in use

    Works only with K3
*/
bool rig_interface::rx_ant(void) const
{ if (_rig_connected)
  { if (_model == RIG_MODEL_K3)
    { const string result { raw_command("AR;", RESPONSE::EXPECTED) };

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
      raw_command( torf ? "AR1;"s : "AR0;"s );
  }
}

/*! \brief              Is notch enabled?
    \param  ds_result   previously-obtained result of a DS command, or empty string
    \return             whether notch is currently enabled

    Works only with K3
*/
//bool rig_interface::notch_enabled(const string& ds_result) const
bool rig_interface::notch_enabled(const string_view ds_result) const
{ if (!_rig_connected)
    return false;

  if (_model != RIG_MODEL_K3)
    return false;

// it's a K3
  const string result { ds_result.empty() ? raw_command("DS;"sv, RESPONSE::EXPECTED) : ds_result };

  if ( (result.size() != 13) or (result[12] != ';') )
    ost << "ERROR in notch_enabled(); result = " << result << endl;
  else
  { const char c         { result[11] };    // icon flash data
    const bool notch_bit { (c bitand 0x02) == 0x02 };

    return notch_bit;
  }

  return false;
}

/*! \brief  Toggle the notch status

    Works only with K3
*/
void rig_interface::toggle_notch_status(void) const
{ if (_model == RIG_MODEL_K3)
    k3_tap(K3_BUTTON::NOTCH);
}

/*! \brief      Set the K3 command mode (either NORMAL or EXTENDED)
    \param  cm  command mode

    Works only with K3
*/
void rig_interface::k3_command_mode(const K3_COMMAND_MODE cm)
{ if (_model == RIG_MODEL_K3)
  { switch (cm)
    { case K3_COMMAND_MODE::EXTENDED :
        raw_command("K31;"s);
        break;

      case K3_COMMAND_MODE::NORMAL :
        raw_command("K30;"s);
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

    if (result == "K31"s)
      return K3_COMMAND_MODE::EXTENDED;
  }

  return K3_COMMAND_MODE::NORMAL;
}

/*! \brief          Emulate the tapping or holding of a K3 button
    \param  n       the K3 button to tap or hold
    \param  torh    whether to press or hold

    Works only with K3
*/
void rig_interface::k3_press_button(const K3_BUTTON n, const PRESS torh) const
{ if (_model == RIG_MODEL_K3)
  { const string n_str      { pad_leftz(static_cast<int>(n), 2) };    // the cast is necessary
    const string press_code { (torh == PRESS::TAP) ? "SWT"s : "SWH"s };
    const string command    { press_code + n_str + ";"s };

    raw_command(command);
  }
}

/*! \brief      Set audio centre frequency and width
    \param  af  the characteristics to set 
*/
void rig_interface::filter(const audio_filter& af) const
{ const int bw { af.bandwidth() };
  const int cf { af.centre() };

  if (bw)
    bandwidth(bw);

  if (cf)
    centre_frequency(cf);
}

/// register a function for alerting the user
//void rig_interface::register_error_alert_function(void (*error_alert_function)(const string&) )
void rig_interface::register_error_alert_function(void (*error_alert_function)(const string_view) )
{ SAFELOCK(_rig);
  _error_alert_function = error_alert_function;
}

/// set RIT, split, sub-rx off
void rig_interface::base_state(void) const
{ if (rit_enabled())
  { rit_disable();

    if (_model == RIG_MODEL_K3)
      sleep_for(1s);          // the K3 is awfully slow; this should allow plenty of time before the next command
  }

  if (split_enabled())
  { split_disable();

    if (_model == RIG_MODEL_K3)
      sleep_for(1s);          // the K3 is awfully slow; this should allow plenty of time before the next command
  }

  if (sub_receiver_enabled())
  { sub_receiver_disable();

    if (_model == RIG_MODEL_K3)
      sleep_for(1s);          // the K3 is awfully slow; this should allow plenty of time before the next command
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
